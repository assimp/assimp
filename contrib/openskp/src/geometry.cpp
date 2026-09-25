#include <algorithm>
#include <charconv>
#include <sstream>

#include "internal.hpp"

namespace openskp {
static const TlvNode* find_node(const std::vector<TlvNode>& ns, const std::string& t) {
  for (auto& n : ns) {
    if (n.tag == t) return &n;
    if (auto* r = find_node(n.children, t)) return r;
  }
  return nullptr;
}

static void find_all(const std::vector<TlvNode>& ns, const std::string& t,
                     std::vector<const TlvNode*>& out) {
  for (auto& n : ns) {
    if (n.tag == t) out.push_back(&n);
    find_all(n.children, t, out);
  }
}

static std::optional<EntityId> entity_id(const TlvNode& n) {
  for (auto& c : n.children) {
    if (c.tag == "DE05" && !c.payload.empty())
      return static_cast<EntityId>(parse_varint(c.payload, 0, c.payload.size()));
    if (c.tag == "DC05" && c.payload.size() >= 6 && c.payload[0] == 0xde && c.payload[1] == 5) {
      auto z = read_u32(c.payload, 2);
      if (z <= c.payload.size() - 6) return static_cast<EntityId>(parse_varint(c.payload, 6, z));
    }
  }
  for (auto& c : n.children)
    if (auto v = entity_id(c)) return v;
  return {};
}

static EntityId dc_id(const ByteBuffer& p) {
  if (p.size() >= 6 && p[0] == 0xde && p[1] == 5) {
    auto z = read_u32(p, 2);
    if (z <= p.size() - 6) return parse_varint(p, 6, z);
  }
  return parse_varint(p, 0, p.size());
}

static std::string text(const ByteBuffer& p) {
  return {reinterpret_cast<const char*>(p.data()), p.size()};
}

static std::string hex(const ByteBuffer& p) {
  static char h[] = "0123456789ABCDEF";
  std::string s;
  s.reserve(p.size() * 2);
  for (auto b : p) {
    s += h[b >> 4];
    s += h[b & 15];
  }
  return s;
}

static const ByteBuffer* flat_find(const std::vector<std::pair<std::string, ByteBuffer>>& s,
                                   const std::string& t) {
  for (auto& v : s)
    if (v.first == t) return &v.second;
  return nullptr;
}

static std::pair<std::optional<std::array<double, 9>>, std::optional<std::array<double, 9>>> uv(
    const ByteBuffer& p) {
  auto a = parse_flat(p);
  auto q = flat_find(a, "DD05");
  if (!q) return {};
  auto b = parse_flat(*q);
  q = flat_find(b, "B136");
  if (!q) return {};
  auto c = parse_flat(*q);
  q = flat_find(c, "B236");
  if (!q) return {};
  auto d = parse_flat(*q);
  q = flat_find(d, "1027");
  if (!q) return {};
  auto sides = parse_flat(*q);
  auto side = [&](const char* t) -> std::optional<std::array<double, 9>> {
    auto x = flat_find(sides, t);
    if (!x) return {};
    auto s1 = parse_flat(*x);
    x = flat_find(s1, "1327");
    if (!x) return {};
    auto s2 = parse_flat(*x);
    x = flat_find(s2, "1527");
    if (!x || x->size() != 72) return {};
    std::array<double, 9> m{};
    for (int i = 0; i < 9; ++i) m[i] = read_f64(*x, i * 8);
    return m;
  };
  return {side("1127"), side("1227")};
}

// Decode a flat span of the DC05 payload into (tag, offset, size) triples -
// same shape as Python's _core.parse_flat, used below to walk siblings at
// one level without recursing into their children automatically (needed to
// find a dictionary's own entries container, which is a SIBLING of its
// B436 name node, not a child of it - see extract_attribute_dictionaries's
// own docstring in _core.py for the exact same structure).
static std::vector<std::pair<std::string, std::pair<size_t, size_t>>> parse_flat_spans(
    const ByteBuffer& p, size_t a, size_t z) {
  std::vector<std::pair<std::string, std::pair<size_t, size_t>>> out;
  while (a + 6 <= z) {
    auto n = read_u32(p, a + 2);
    if (n > z - a - 6) break;
    std::string t;
    static char h[] = "0123456789ABCDEF";
    t += h[p[a] >> 4];
    t += h[p[a] & 15];
    t += h[p[a + 1] >> 4];
    t += h[p[a + 1] & 15];
    out.emplace_back(t, std::make_pair(a + 6, a + 6 + n));
    a += 6 + n;
  }
  return out;
}

// Tags parse_tlv_recursive treats as worth descending into when building
// the real tree (mirrors Python's _PROP_CONTAINER_TAGS exactly) - anything
// else's payload is raw data (a string, a number), never more nested TLV
// structure, so recursing into it would misinterpret arbitrary bytes as
// tags/lengths.
static bool is_prop_container_tag(const std::string& t) {
  return t == "DD05" || t == "B536" || t == "B136" || t == "B236" || t == "B336" || t == "B036" ||
         t == "A438" || t == "AE38";
}

// Shortest decimal string that round-trips exactly back to v - the same
// "clean" formatting Python's str(float)/.NET's ToString()/TypeScript's/
// Dart's toString() all produce naturally (15.5 -> "15.5", not
// "15.500000" or 17-digit noise like "15.500000000000000"). Deliberately
// NOT json_export.cpp's own "%.17g" convention: that call site needs
// lossless machine round-tripping for a JSON *number*, this one needs a
// human-readable attribute *string* comparable to what the other 4 ports
// produce for the same value - different jobs, different formatting.
static std::string format_double(double v) {
  char buf[32];
  auto result = std::to_chars(buf, buf + sizeof(buf), v);
  return std::string(buf, result.ptr);
}

// A438 wraps exactly one attribute value: its payload holds a single
// nested span whose OWN tag says the real type (AD38 string, A938/AF38
// double, A738 int32, B438/B538 a 3xf64 point/vector, AE38 a nested
// array) - matching Python's _decode_vff_attr_value exactly (openskp#285).
// No native bool/time_t tag has ever been observed in real data, so 7/9
// is the real ceiling, not an arbitrary stopping point.
//
// Unlike the other 4 ports, this one never exposes a richer-than-string
// decoded value anywhere (attribute_dictionaries is std::map<std::string,
// std::string> by design here) - matching Python's/every other port's own
// PUBLIC attribute_dictionaries contract, which is string-valued too
// (scene.py stringifies via _stringify_vff_attr_value before ever storing
// into InstanceNode.attribute_dictionaries; the richer typed value only
// exists in each port's internal decode step). So this function decodes
// AND stringifies in one pass rather than keeping the two steps separate.
// An A438 with no children (Python: None) or an unrecognized value tag
// both stringify to "" - a real, present key with an empty value, not a
// missing key, matching extract_entries's own unconditional assignment.
static std::string decode_a438_value(const ByteBuffer& p, size_t a, size_t z) {
  auto spans = parse_flat_spans(p, a, z);
  if (spans.empty()) return "";
  auto& [tag, span] = spans[0];
  auto [sa, sz] = span;
  if (tag == "AD38") {
    return std::string(reinterpret_cast<const char*>(p.data() + sa), sz - sa);
  }
  if ((tag == "AF38" || tag == "A938") && sz - sa == 8) {
    return format_double(read_f64(p, sa));
  }
  if (tag == "A738" && sz - sa == 4) {
    return std::to_string(read_i32(p, sa));
  }
  if ((tag == "B438" || tag == "B538") && sz - sa == 24) {
    return format_double(read_f64(p, sa)) + "," + format_double(read_f64(p, sa + 8)) + "," +
           format_double(read_f64(p, sa + 16));
  }
  if (tag == "AE38") {
    std::string joined;
    bool first = true;
    for (auto& [ctag, cspan] : parse_flat_spans(p, sa, sz)) {
      if (ctag != "A438") continue;
      auto [ca, cz] = cspan;
      if (!first) joined += ",";
      joined += decode_a438_value(p, ca, cz);
      first = false;
    }
    return joined;
  }
  return "";
}

// Extract every attribute dictionary attached to a DC05 payload, keyed by
// the dictionary's own declared name (tag B436) rather than merged into
// one flat map (matches _core.extract_attribute_dictionaries exactly).
// Each dictionary is a B436 (name) node immediately followed by a sibling
// entries container (typically B536) holding that dictionary's own B636
// (key name) / A438 (value wrapper) pairs - and, same as Python, B436 can
// occur at any depth reachable purely through container tags, not only at
// the payload's own top level, so both the dictionary search and the
// entries walk recurse into every container-tagged node, not just scan
// one level.
static void extract_attribute_dictionaries(
    const ByteBuffer& p, std::map<std::string, std::map<std::string, std::string>>& out) {
  std::function<void(size_t, size_t, std::map<std::string, std::string>&)> extract_entries;
  extract_entries = [&](size_t a, size_t z, std::map<std::string, std::string>& entries) {
    std::string key;
    for (auto& [tag, span] : parse_flat_spans(p, a, z)) {
      auto [sa, sz] = span;
      if (tag == "B636") {
        key = std::string(reinterpret_cast<const char*>(p.data() + sa), sz - sa);
      } else if (tag == "A438" && !key.empty()) {
        entries[key] = decode_a438_value(p, sa, sz);
        key.clear();
      } else if (is_prop_container_tag(tag)) {
        extract_entries(sa, sz, entries);
      }
    }
  };

  std::function<void(size_t, size_t)> walk_dicts;
  walk_dicts = [&](size_t a, size_t z) {
    auto here = parse_flat_spans(p, a, z);
    for (std::size_t i = 0; i < here.size(); ++i) {
      auto& [tag, span] = here[i];
      auto [sa, sz] = span;
      if (tag == "B436") {
        std::string name(reinterpret_cast<const char*>(p.data() + sa), sz - sa);
        std::map<std::string, std::string> entries;
        if (i + 1 < here.size()) {
          auto [ea, ez] = here[i + 1].second;
          extract_entries(ea, ez, entries);
        }
        out[name] = std::move(entries);
      } else if (is_prop_container_tag(tag)) {
        walk_dicts(sa, sz);
      }
    }
  };

  walk_dicts(0, p.size());
}

void collect_geometry(const std::vector<TlvNode>& es, GeometryBuilder& b) {
  for (auto& e : es) {
    if (e.tag == "C409") {
      auto id = entity_id(e);
      auto* p = find_node(e.children, "C509");
      if (id && p && p->payload.size() >= 24)
        b.vertices[*id] = {read_f64(p->payload, 0), read_f64(p->payload, 8),
                           read_f64(p->payload, 16)};
    } else if (e.tag == "B80B") {
      auto id = entity_id(e);
      if (id) {
        auto *x = find_node(e.children, "B90B"), *y = find_node(e.children, "BA0B");
        std::optional<EntityId> a, c;
        if (x && !x->payload.empty()) a = parse_varint(x->payload, 0, x->payload.size());
        if (y && !y->payload.empty()) c = parse_varint(y->payload, 0, y->payload.size());
        b.edges[*id] = {a, c};
        for (auto& z : e.children)
          if (z.tag == "D007")
            for (auto& w : z.children)
              if (w.tag == "D307" && !w.payload.empty()) b.edge_flags[*id] = w.payload[0];
      }
    } else if (e.tag == "AC0D") {
      auto id = entity_id(e);
      if (id) {
        RawFace f;
        auto* n = find_node(e.children, "AD0D");
        if (n && n->payload.size() >= 24)
          f.normal = {read_f64(n->payload, 0), read_f64(n->payload, 8), read_f64(n->payload, 16)};
        auto* l = find_node(e.children, "AE0D");
        if (l) {
          std::vector<const TlvNode*> loops;
          find_all(l->children, "9411", loops);
          for (auto* ln : loops) {
            std::vector<CoEdge> co;
            std::vector<const TlvNode*> cs;
            find_all(ln->children, "A00F", cs);
            for (auto* cn : cs) {
              std::optional<EntityId> eid;
              std::optional<std::int64_t> ori;
              for (auto& v : parse_flat(cn->payload)) {
                if (v.first == "A10F")
                  eid = parse_varint(v.second, 0, v.second.size());
                else if (v.first == "A20F") {
                  const auto raw_orientation = parse_varint(v.second, 0, v.second.size());
                  ori = raw_orientation == 0 ? 1 : -1;
                }
              }
              if (eid && ori) co.push_back({*eid, *ori});
            }
            if (!co.empty()) f.loops.push_back(std::move(co));
          }
        }
        for (auto& d : e.children)
          if (d.tag == "D007")
            for (auto& x : d.children) {
              if (x.tag == "D107" && !x.payload.empty())
                f.material_id = parse_varint(x.payload, 0, x.payload.size());
              else if (x.tag == "DC05")
                std::tie(f.uv_transform, f.uv_transform_back) = uv(x.payload);
              // D307 = display flags, same record edges already read (base
              // 0x06, +0x01 hidden) - faces carry the identical tag under
              // their own D007 container.
              else if (x.tag == "D307" && !x.payload.empty())
                f.hidden = (x.payload[0] & 0x01) != 0;
            }
        for (auto& x : e.children)
          if (x.tag == "AF0D" && !x.payload.empty())
            f.back_material_id = parse_varint(x.payload, 0, x.payload.size());
        b.faces[*id] = std::move(f);
      }
    } else if (e.tag == "6419") {
      RawInstance i;
      i.offset = e.offset;
      i.children = e.children;
      auto* g = find_node(e.children, "6819");
      if (g && g->payload.size() == 16) i.ref_guid = hex(g->payload);
      auto* r = find_node(e.children, "6719");
      if (r && !r->payload.empty()) i.ref_idx = parse_varint(r->payload, 0, r->payload.size());
      auto* n = find_node(e.children, "6519");
      if (n) i.name = text(n->payload);
      auto* m = find_node(e.children, "6619");
      if (m && m->payload.size() >= 104)
        for (int q = 0; q < 13; ++q) i.matrix.push_back(read_f64(m->payload, q * 8));
      for (auto& d : e.children)
        if (d.tag == "D007")
          for (auto& x : d.children) {
            if (x.tag == "D107" && !x.payload.empty())
              i.material_id = parse_varint(x.payload, 0, x.payload.size());
            else if (x.tag == "D207" && !x.payload.empty())
              i.layer = std::to_string(parse_varint(x.payload, 0, x.payload.size()));
            else if (x.tag == "DC05") {
              std::map<std::string, std::map<std::string, std::string>> all_dicts;
              extract_attribute_dictionaries(x.payload, all_dicts);
              for (auto& [dict_name, entries] : all_dicts) {
                if (dict_name == "dynamic_attributes") {
                  i.properties = std::move(entries);
                } else if (dict_name != "SU_InstanceSet") {
                  i.attribute_dicts[dict_name] = std::move(entries);
                }
              }
            }
            // D307 = display flags, same record edges/faces already read
            // (base 0x06, +0x01 hidden).
            else if (x.tag == "D307" && !x.payload.empty())
              i.hidden = (x.payload[0] & 0x01) != 0;
          }
      b.instances.push_back(std::move(i));
    } else if (!e.children.empty())
      collect_geometry(e.children, b);
  }
}

void collect_layers(const std::vector<TlvNode>& ns, std::map<EntityId, std::string>& out,
                    std::map<std::string, bool>& hidden) {
  for (auto& e : ns) {
    if (e.tag == "993A")
      for (auto& c : e.children)
        if (c.tag == "8C3C") {
          auto *d = find_node(c.children, "DC05"), *n = find_node(c.children, "8D3C");
          if (d && n && !d->payload.empty()) out[dc_id(d->payload)] = text(n->payload);
          // 8E3C: a single byte, 1 = hidden / 0 = visible - the VFF-format
          // counterpart of the legacy format's already-known layer-hidden
          // flag, confirmed byte-for-byte against a real production
          // file's own Tags panel (matches Python's _core.py exactly).
          if (n) {
            auto* h = find_node(c.children, "8E3C");
            if (h && !h->payload.empty()) hidden[text(n->payload)] = h->payload[0] == 1;
          }
        }
    collect_layers(e.children, out, hidden);
  }
}

void collect_material_ids(const std::vector<TlvNode>& ns, std::map<EntityId, std::string>& out) {
  for (auto& e : ns) {
    if (e.tag == "C832") {
      auto *d = find_node(e.children, "DC05"), *n = find_node(e.children, "CC32");
      if (d && n && !d->payload.empty()) out[dc_id(d->payload)] = text(n->payload);
    }
    collect_material_ids(e.children, out);
  }
}

void collect_definitions(const std::vector<TlvNode>& ns, std::map<EntityId, RawDefinition>& out) {
  for (auto& e : ns) {
    if (e.tag == "7C15") {
      RawDefinition d;
      for (auto& c : e.children) {
        if (c.tag == "7D15" && c.payload.size() == 16)
          d.guid = hex(c.payload);
        else if (c.tag == "7E15")
          d.name = text(c.payload);
        else if (c.tag == "8315" && !c.payload.empty())
          d.is_image = parse_varint(c.payload, 0, c.payload.size()) == 2;
        else if (c.tag == "581B")
          for (auto& v : parse_flat(c.payload)) {
            if (v.first == "5D1B" && !v.second.empty())
              d.always_faces_camera = parse_varint(v.second, 0, v.second.size()) == 1;
            else if (v.first == "5E1B" && !v.second.empty())
              d.shadows_face_sun = parse_varint(v.second, 0, v.second.size()) == 1;
          }
      }
      collect_geometry(e.children, d.builder);
      if (auto id = entity_id(e)) out[*id] = std::move(d);
    }
    collect_definitions(e.children, out);
  }
}
}  // namespace openskp
