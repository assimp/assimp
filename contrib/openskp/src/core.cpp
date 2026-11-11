#include <algorithm>
#include <cctype>
#include <limits>
#include <miniz.h>
#include <regex>

#include "internal.hpp"

namespace openskp {

// Declared here (defined below, outside the anonymous namespace) so it's
// independently unit-testable via internal.hpp, the same way build_scene_raw
// and friends are - the anonymous-namespace members below it that call it
// (attr()) never leave this translation unit, but the decoder itself is a
// small, self-contained piece worth exercising directly.
std::string decode_xml_entities(const std::string& value);

namespace {
struct Zip {
  mz_zip_archive z{};
  std::vector<std::string> names;

  Zip(const ByteBuffer& d, std::size_t off) {
    if (!mz_zip_reader_init_mem(&z, d.data() + off, d.size() - off, 0))
      throw SkpParseError("Invalid ZIP container", ParseStage::zip_extract);
    auto n = mz_zip_reader_get_num_files(&z);
    for (mz_uint i = 0; i < n; ++i) {
      mz_zip_archive_file_stat s{};
      if (mz_zip_reader_file_stat(&z, i, &s) && !s.m_is_directory) names.emplace_back(s.m_filename);
    }
  }

  ~Zip() { mz_zip_reader_end(&z); }

  Zip(const Zip&) = delete;

  std::optional<ByteBuffer> get(const std::string& name) {
    int i = mz_zip_reader_locate_file(&z, name.c_str(), nullptr, 0);
    if (i < 0) return {};
    mz_zip_archive_file_stat s{};
    if (!mz_zip_reader_file_stat(&z, i, &s)) return {};
    validate_entry_size(s);
    ByteBuffer b(static_cast<std::size_t>(s.m_uncomp_size));
    if (!mz_zip_reader_extract_to_mem(&z, i, b.data(), b.size(), 0)) return {};
    return b;
  }

 private:
  // A ZIP entry's declared uncompressed size (m_uncomp_size) is untrusted
  // central-directory metadata - it can be set independently of what the
  // compressed stream actually decompresses to, and even when genuine,
  // DEFLATE can expand highly compressible data by three orders of
  // magnitude. mz_zip_reader_extract_to_heap allocates up to that declared
  // size with no ceiling of its own. Real production model.dat entries are
  // observed at ~10x compression, so both limits below leave generous
  // headroom for legitimate files while rejecting the kind of declared-size
  // lie or extreme ratio a genuine file would never need.
  static constexpr std::uint64_t kMaxUncompressedEntryBytes = 16ull * 1024 * 1024 * 1024;  // 16 GB
  static constexpr std::uint64_t kMaxCompressionRatio = 1000;
  static constexpr std::uint64_t kRatioCheckThresholdBytes = 1024ull * 1024;  // 1 MB

  static void validate_entry_size(const mz_zip_archive_file_stat& s) {
    auto declared = s.m_uncomp_size;
    if (declared == 0) return;

    if (declared > std::numeric_limits<std::size_t>::max()) {
      throw SkpParseError("ZIP entry '" + std::string(s.m_filename) + "' declares " +
                              std::to_string(declared) +
                              " bytes, exceeding addressable memory limits",
                          ParseStage::zip_extract);
    }

    if (declared > kMaxUncompressedEntryBytes) {
      throw SkpParseError("ZIP entry '" + std::string(s.m_filename) + "' declares " +
                              std::to_string(declared) + " bytes uncompressed, exceeding the " +
                              std::to_string(kMaxUncompressedEntryBytes) + "-byte safety ceiling",
                          ParseStage::zip_extract);
    }

    if (declared >= kRatioCheckThresholdBytes) {
      auto compressed = s.m_comp_size;
      if (compressed == 0 || declared / compressed > kMaxCompressionRatio) {
        throw SkpParseError("ZIP entry '" + std::string(s.m_filename) +
                                "' declares an implausible compression ratio (" +
                                std::to_string(declared) + " bytes from " +
                                std::to_string(compressed) +
                                " bytes compressed) - likely a decompression bomb",
                            ParseStage::zip_extract);
      }
    }
  }
};

std::size_t zip_offset(const ByteBuffer& d) {
  for (std::size_t i = 0; i + 4 <= d.size() && i < 4096; ++i)
    if (d[i] == 'P' && d[i + 1] == 'K' && d[i + 2] == 3 && d[i + 3] == 4) return i;
  return std::string::npos;
}

struct Header {
  std::size_t offset;
  std::size_t size;
};

std::vector<Header> headers(const ByteBuffer& d, std::size_t start, std::size_t end) {
  std::vector<Header> h;
  for (auto p = start; p + 6 <= end;) {
    auto n = read_u32(d, p + 2);
    if (n > end - p - 6) break;
    h.push_back({p, n});
    p += 6 + n;
  }
  return h;
}

std::string str(const ByteBuffer& b) { return {reinterpret_cast<const char*>(b.data()), b.size()}; }

// Decodes the 5 predefined XML entities plus numeric character references
// (&#39;, &#x27;) in a raw attribute value extracted by regex rather than a
// real XML parser. A real material name legitimately contains "<"/">" -
// SketchUp's own "&lt;auto&gt;" default-material naming convention - so
// without this, names like that come through still escaped instead of as
// the literal characters a real XML parser (this project's other four
// ports all use one) would produce. A single regex pass, not five
// sequential replacements: sequential replacement would double-decode
// "&amp;lt;" into "<" instead of the correct "&lt;".
std::string attr(const std::string& s, const std::string& key) {
  std::regex r("(?:^|\\s)" + key + "\\s*=\\s*[\\\"']([^\\\"']*)[\\\"']", std::regex::icase);
  std::smatch m;
  return std::regex_search(s, m, r) ? decode_xml_entities(m[1].str()) : "";
}

int integer(const std::string& s, int fallback) {
  try {
    return s.empty() ? fallback : std::stoi(s);
  } catch (...) {
    return fallback;
  }
}

double number(const std::string& s, double fallback) {
  try {
    return s.empty() ? fallback : std::stod(s);
  } catch (...) {
    return fallback;
  }
}

std::string basename(const std::string& s) {
  auto p = s.find_last_of('/');
  return p == std::string::npos ? s : s.substr(p + 1);
}

std::shared_ptr<RawMaterial> material_xml(Zip& zip, const std::string& path,
                                          const ByteBuffer& bytes) {
  auto xml = str(bytes);
  std::regex tag("<(?:[A-Za-z_][\\w.-]*:)?material\\b([^>]*)>", std::regex::icase);
  std::smatch m;
  if (!std::regex_search(xml, m, tag)) return {};
  auto a = m[1].str();
  auto out = std::make_shared<RawMaterial>();
  out->name = attr(a, "name");
  if (out->name.empty()) out->name = "unknown";
  out->r = integer(attr(a, "colorRed"), 128);
  out->g = integer(attr(a, "colorGreen"), 128);
  out->b = integer(attr(a, "colorBlue"), 128);
  if (attr(a, "useTrans") == "1")
    out->transparency = std::clamp(1.0 - number(attr(a, "trans"), 0), 0.0, 1.0);
  out->colorized = attr(a, "type") == "2";
  out->colorize_type = integer(attr(a, "colorizeType"), 0);
  std::regex tr("<(?:[A-Za-z_][\\w.-]*:)?texture\\b([^>]*)>", std::regex::icase);
  if (std::regex_search(xml, m, tr)) {
    auto ta = m[1].str();
    RawTexture t;
    t.filename = attr(ta, "textureFilename");
    t.x_scale = number(attr(ta, "xScale"), 0);
    t.y_scale = number(attr(ta, "yScale"), 0);
    auto slash = path.find_last_of('/');
    auto folder = slash == std::string::npos ? std::string{} : path.substr(0, slash);
    auto candidate = folder + "/" + t.filename;
    if (!t.filename.empty()) t.data = zip.get(candidate);
    if (!t.data)
      for (auto& n : zip.names)
        if (n.rfind(folder + "/", 0) == 0 && n != path && n.size() > 4 &&
            n.substr(n.size() - 4) != ".xml") {
          t.data = zip.get(n);
          if (t.filename.empty()) t.filename = basename(n);
          break;
        }
    if (!t.data) {
      std::regex ir("<(?:[A-Za-z_][\\w.-]*:)?image\\b([^>]*)", std::regex::icase);
      if (std::regex_search(xml, m, ir)) {
        auto ip = attr(m[1].str(), "path");
        while (!ip.empty() && (ip[0] == '.' || ip[0] == '/')) ip.erase(ip.begin());
        for (auto& c : {ip, folder + "/" + ip})
          if (!c.empty() && (t.data = zip.get(c))) {
            if (t.filename.empty()) t.filename = basename(c);
            break;
          }
      }
    }
    out->texture = std::move(t);
  }
  return out;
}

std::optional<RawStyle> style_xml(const ByteBuffer& bytes) {
  auto xml = str(bytes);
  std::regex st("<(?:[A-Za-z_][\\w.-]*:)?style\\b([^>]*)>", std::regex::icase);
  std::smatch m;
  if (!std::regex_search(xml, m, st)) return {};
  RawStyle o;
  o.name = attr(m[1].str(), "name");
  std::regex item(
      "<(?:[A-Za-z_][\\w.-]*:)?item\\b([^>]*)>([\\s\\S]*?)</(?:[A-Za-z_][\\w.-]*:)?item>",
      std::regex::icase);
  for (auto i = std::sregex_iterator(xml.begin(), xml.end(), item); i != std::sregex_iterator();
       ++i) {
    auto id = attr((*i)[1].str(), "id");
    if (id != "4000" && id != "4001") continue;
    std::regex vr("<(?:[A-Za-z_][\\w.-]*:)?variant[^>]*>\\s*(-?\\d+)", std::regex::icase);
    std::smatch v;
    auto body = (*i)[2].str();
    if (std::regex_search(body, v, vr)) {
      auto n = static_cast<std::uint32_t>(std::stoll(v[1].str()));
      Color3 c{std::uint8_t(n >> 16), std::uint8_t(n >> 8), std::uint8_t(n)};
      if (id == "4000")
        o.front_color = c;
      else
        o.back_color = c;
    }
  }
  return o;
}

// VFF model.dat wraps the file's definition list inside container tags
// F901 -> 7017 -> 7117 -> 7C15. We unwrap this container into individual
// 7C15 headers upfront so memory is bounded to one definition at a time,
// mirroring Python's _unwrap_definitions_container (Issue #264).
std::vector<Header> unwrap_definitions_container(const ByteBuffer& data, std::size_t offset,
                                                 std::size_t size) {
  auto level = headers(data, offset + 6, offset + 6 + size);
  for (const char* expected_tag : {"7017", "7117"}) {
    auto it = std::find_if(level.begin(), level.end(),
                           [&](const Header& h) { return tag_at(data, h.offset) == expected_tag; });
    if (it == level.end()) return {};
    level = headers(data, it->offset + 6, it->offset + 6 + it->size);
  }
  std::vector<Header> defs;
  for (const auto& h : level) {
    if (tag_at(data, h.offset) == "7C15") {
      defs.push_back(h);
    }
  }
  return defs;
}
}  // namespace

// Decodes the 5 predefined XML entities plus numeric character references
// (&#39;, &#x27;) in a raw attribute value extracted by regex rather than a
// real XML parser. A real material name legitimately contains "<"/">" -
// SketchUp's own "&lt;auto&gt;" default-material naming convention - so
// without this, names like that come through still escaped instead of as
// the literal characters a real XML parser (this project's other four
// ports all use one) would produce. A single regex pass, not five
// sequential replacements: sequential replacement would double-decode
// "&amp;lt;" into "<" instead of the correct "&lt;".
std::string decode_xml_entities(const std::string& value) {
  static const std::regex entity("&(lt|gt|amp|apos|quot|#[0-9]+|#[xX][0-9a-fA-F]+);");
  std::string out;
  out.reserve(value.size());
  auto begin = std::sregex_iterator(value.begin(), value.end(), entity);
  auto end = std::sregex_iterator();
  std::size_t lastPos = 0;
  for (auto it = begin; it != end; ++it) {
    const auto& m = *it;
    out.append(value, lastPos, static_cast<std::size_t>(m.position()) - lastPos);
    const std::string name = m[1].str();
    if (name == "lt") {
      out += '<';
    } else if (name == "gt") {
      out += '>';
    } else if (name == "amp") {
      out += '&';
    } else if (name == "apos") {
      out += '\'';
    } else if (name == "quot") {
      out += '"';
    } else if (name[0] == '#') {
      try {
        const bool hex = name.size() > 1 && (name[1] == 'x' || name[1] == 'X');
        const unsigned long codePoint =
            std::stoul(name.substr(hex ? 2 : 1), nullptr, hex ? 16 : 10);
        // UTF-8 encode. SketchUp material/style names are practically
        // always within the BMP, but handle the full range regardless.
        if (codePoint <= 0x7F) {
          out += static_cast<char>(codePoint);
        } else if (codePoint <= 0x7FF) {
          out += static_cast<char>(0xC0 | (codePoint >> 6));
          out += static_cast<char>(0x80 | (codePoint & 0x3F));
        } else if (codePoint <= 0xFFFF) {
          out += static_cast<char>(0xE0 | (codePoint >> 12));
          out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
          out += static_cast<char>(0x80 | (codePoint & 0x3F));
        } else {
          out += static_cast<char>(0xF0 | (codePoint >> 18));
          out += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
          out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
          out += static_cast<char>(0x80 | (codePoint & 0x3F));
        }
      } catch (...) {
        out += m[0].str();  // malformed numeric reference - leave as-is
      }
    } else {
      out += m[0].str();
    }
    lastPos = static_cast<std::size_t>(m.position() + m.length());
  }
  out.append(value, lastPos, std::string::npos);
  return out;
}

RawParsed full_parse(const ByteBuffer& data, const ParseOptions& o) {
  emit_log(o, LogLevel::information, "Parsing buffer (" + std::to_string(data.size()) + " bytes)");
  if (!valid_header(data))
    throw SkpParseError("Not a valid SketchUp file (bad header magic)", ParseStage::header);
  if (is_legacy(data)) {
    emit_log(o, LogLevel::debug, "Detected legacy MFC container; routing to legacy walker");
    return parse_legacy(data, o);
  }
  RawParsed p;
  p.version = extract_version(data);
  auto off = zip_offset(data);
  if (off == std::string::npos)
    throw SkpParseError("No ZIP container found", ParseStage::zip_extract);
  Zip zip(data, off);
  for (auto& n : zip.names)
    if (n.rfind("materials/", 0) == 0 && n.size() >= 12 &&
        n.substr(n.size() - 12) == "material.xml") {
      if (auto b = zip.get(n))
        if (auto m = material_xml(zip, n, *b)) {
          auto slash = n.find('/', 10);
          auto folder = n.substr(10, slash == std::string::npos ? std::string::npos : slash - 10);
          p.materials[m->name] = m;
          p.materials_by_folder[folder] = m;
          if (m->name.rfind("Layer_", 0) == 0) {
            auto layer_name = m->name.substr(6);
            if (!p.layer_colors.count(layer_name)) p.layer_order.push_back(layer_name);
            p.layer_colors[layer_name] = {std::uint8_t(m->r), std::uint8_t(m->g),
                                          std::uint8_t(m->b)};
            p.layer_hidden[layer_name] = false;
          }
        }
    }
  for (auto& n : zip.names)
    if (n.rfind("styles/", 0) == 0 && n.size() >= 9 && n.substr(n.size() - 9) == "style.xml")
      if (auto b = zip.get(n))
        if (auto s = style_xml(*b)) p.styles.push_back(*s);
  auto model = zip.get("model.dat");
  if (!model) throw SkpParseError("model.dat not found in ZIP container", ParseStage::zip_extract);
  auto hs = headers(*model, 0, model->size());
  if (hs.size() == 1 && model->at(hs[0].offset) == 0xf4 && model->at(hs[0].offset + 1) == 1)
    hs = headers(*model, hs[0].offset + 6, hs[0].offset + 6 + hs[0].size);

  std::vector<Header> expanded_hs;
  expanded_hs.reserve(hs.size());
  for (const auto& h : hs) {
    if (tag_at(*model, h.offset) == "F901") {
      auto def_headers = unwrap_definitions_container(*model, h.offset, h.size);
      if (!def_headers.empty()) {
        expanded_hs.insert(expanded_hs.end(), def_headers.begin(), def_headers.end());
        continue;
      }
    }
    expanded_hs.push_back(h);
  }
  hs = std::move(expanded_hs);

  std::map<std::string, Vec3> vertex_positions;
  std::map<std::string, std::vector<double>> instance_world;
  const TlvNode* page_node = nullptr;
  std::vector<TlvNode> page_node_owner;  // keeps page_node's subtree alive past the loop

  auto total = hs.size();
  for (std::size_t i = 0; i < total; ++i) {
    std::string tag;
    try {
      auto one = parse_tlv_recursive(*model, hs[i].offset, hs[i].offset + 6 + hs[i].size);
      if (one.empty()) {
        emit_log(o, LogLevel::debug,
                 "Failed to parse record at offset " + std::to_string(hs[i].offset));
        continue;
      }
      tag = one[0].tag;
      collect_layers(one, p.layer_id_to_name, p.layer_hidden);
      collect_material_ids(one, p.material_id_to_name);
      collect_definitions(one, p.definitions);
      scan_vertex_positions(one[0], vertex_positions);
      scan_instance_transforms(one[0], instance_world);
      if (!page_node) {
        if (auto* found = find_page_node(one[0])) {
          page_node_owner.push_back(*found);
          page_node = &page_node_owner.back();
        }
      }
      if (one[0].tag == "F601") collect_geometry(one[0].children, p.root.builder);
    } catch (const SkpParseError&) {
      throw;
    } catch (...) {
      throw SkpParseError("Failed while processing record", ParseStage::tlv_walk, i, total, tag,
                          hs[i].offset, {}, std::current_exception());
    }
    if (i % progress_interval == 0 || i + 1 == total)
      emit_progress(o, ParseStage::tlv_walk, i + 1, total);
  }
  // Units (meta/meta.dat) - VFF-only; legacy files carry no equivalent
  // container.
  try {
    if (auto meta = zip.get("meta/meta.dat")) p.units = read_meta_units(*meta);
  } catch (...) {
    p.units = std::nullopt;
    emit_log(o, LogLevel::debug, "Failed to read units from meta/meta.dat");
  }
  try {
    p.pages = parse_pages(page_node);
  } catch (...) {
    emit_log(o, LogLevel::debug, "Failed to parse pages");
  }
  if (!vertex_positions.empty()) {
    try {
      p.dimensions = parse_dimensions(*model, vertex_positions, instance_world);
    } catch (...) {
      emit_log(o, LogLevel::debug, "Failed to parse dimensions");
    }
  }
  model.reset();
  if (!p.layer_id_to_name.count(1)) p.layer_id_to_name[1] = "Layer0";
  if (!p.layer_colors.count("Layer0")) {
    p.layer_order.push_back("Layer0");
    p.layer_colors["Layer0"] = {136, 136, 136};
  }
  if (!p.layer_hidden.count("Layer0")) p.layer_hidden["Layer0"] = false;
  emit_log(o, LogLevel::information,
           "Parse complete: " + std::to_string(p.definitions.size()) + " defs");
  return p;
}
}  // namespace openskp
