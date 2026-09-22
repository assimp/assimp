#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <flatbuffers/flatbuffers.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <miniz.h>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <openskp/_fragments_fb/index_generated.h>
#include <openskp/fragments_export.hpp>
#include <openskp/json_export.hpp>

namespace openskp {
namespace {

namespace fb = openskp::fragments_fb;

// Fragments' Shell uses `ushort` point indices by default; a shell with
// more points than this must use the wide BigShell encoding (`uint`
// indices) instead - confirmed against the real importer's own
// `points.length > ushortMaxValue` check.
constexpr std::size_t kUshortMax = 65535;

// Per-axis scale magnitudes within this of 1.0 are treated as exactly
// unit scale for cache-key rounding purposes - matches typical
// floating-point accumulation noise from matrix composition, not a
// meaningful tolerance for an actually-intended resize.
constexpr int kScaleRoundNdigits = 6;

using Mat4 = std::array<double, 16>;

constexpr Mat4 kIdentityMatrix{
    1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
};

// Column-major 4x4 multiply, a*b - same convention as instanced_scene.cpp's
// own internal mul4 (duplicated here rather than shared, matching this
// project's existing convention between closely-related-but-separate
// translation units).
Mat4 mat4_mul(const Mat4& a, const Mat4& b) {
  Mat4 out{};
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      double s = 0.0;
      for (int k = 0; k < 4; ++k)
        s += a[static_cast<std::size_t>(k * 4 + row)] * b[static_cast<std::size_t>(col * 4 + k)];
      out[static_cast<std::size_t>(col * 4 + row)] = s;
    }
  }
  return out;
}

struct Leaf {
  const InstancedNode* node;
  Mat4 world;
};

// Walk the instanced scene's tree, accumulating each node's GLOBAL (world)
// transform, and return every node worth tracking as its own item - "leaf"
// in the geometry sense (carries a mesh) OR a real, named organizational
// wrapper with no geometry of its own (e.g. a SketchUp group like "W-2"
// that only exists to hold several separately-meshed parts). Mirrors
// openskp.export.fragments._collect_leaves exactly - see that function's
// own docstring for the full rationale.
void collect_leaves(const InstancedNode& node, const InstancedNode* root, const Mat4& parent_matrix,
                    std::vector<Leaf>& out) {
  const Mat4 world = mat4_mul(parent_matrix, node.matrix);
  const bool is_named_wrapper = (&node != root) && !node.name_is_generated;
  if (node.mesh_resource_id || is_named_wrapper) out.push_back(Leaf{&node, world});
  for (auto& child : node.children) collect_leaves(child, root, world, out);
}

struct Trs {
  std::array<double, 3> position;
  std::array<double, 3> x_dir;
  std::array<double, 3> y_dir;
  std::array<double, 3> scale;
  bool mirrored;
};

// Decompose a column-major 4x4 instance transform into
// (position, x_direction, y_direction, scale, mirrored) - a direct port of
// openskp.export.fragments._decompose_trs. See that function's own
// docstring for why mirroring is resolved this way.
Trs decompose_trs(const Mat4& m) {
  const std::array<double, 3> x_axis{m[0], m[1], m[2]};
  const std::array<double, 3> y_axis{m[4], m[5], m[6]};
  const std::array<double, 3> z_axis{m[8], m[9], m[10]};
  const std::array<double, 3> pos{m[12], m[13], m[14]};

  auto norm = [](const std::array<double, 3>& v) {
    double s = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    return s > 0.0 ? s : 1.0;
  };
  const double sx = norm(x_axis), sy = norm(y_axis), sz = norm(z_axis);

  const double det = x_axis[0] * (y_axis[1] * z_axis[2] - y_axis[2] * z_axis[1]) -
                     x_axis[1] * (y_axis[0] * z_axis[2] - y_axis[2] * z_axis[0]) +
                     x_axis[2] * (y_axis[0] * z_axis[1] - y_axis[1] * z_axis[0]);
  const bool mirrored = det < 0.0;

  std::array<double, 3> x_dir{x_axis[0] / sx, x_axis[1] / sx, x_axis[2] / sx};
  if (mirrored) {
    x_dir = {-x_dir[0], -x_dir[1], -x_dir[2]};
  }
  const std::array<double, 3> y_dir{y_axis[0] / sy, y_axis[1] / sy, y_axis[2] / sy};

  return Trs{pos, x_dir, y_dir, {sx, sy, sz}, mirrored};
}

struct BakedGeometry {
  std::vector<std::array<float, 3>> points;
  std::vector<std::array<std::uint32_t, 3>> triangles;
};

// Apply an instance's scale/mirror directly to a copy of its resource's
// LOCAL points and triangle winding, so the resulting geometry is correct
// when placed by a purely rigid Transform - a direct port of
// openskp.export.fragments._bake_primitive.
BakedGeometry bake_primitive(const LocalPrimitive& prim, const std::array<double, 3>& scale,
                             bool mirrored) {
  const double sx = mirrored ? -scale[0] : scale[0];
  const double sy = scale[1], sz = scale[2];

  BakedGeometry out;
  const std::size_t n_verts = prim.positions.size() / 3;
  out.points.reserve(n_verts);
  for (std::size_t i = 0; i < n_verts; ++i) {
    out.points.push_back({
        static_cast<float>(prim.positions[i * 3] * sx),
        static_cast<float>(prim.positions[i * 3 + 1] * sy),
        static_cast<float>(prim.positions[i * 3 + 2] * sz),
    });
  }

  const std::size_t n_tris = prim.indices.size() / 3;
  out.triangles.reserve(n_tris);
  for (std::size_t i = 0; i < n_tris; ++i) {
    std::array<std::uint32_t, 3> tri{prim.indices[i * 3], prim.indices[i * 3 + 1],
                                     prim.indices[i * 3 + 2]};
    if (mirrored) std::swap(tri[1], tri[2]);
    out.triangles.push_back(tri);
  }
  return out;
}

// Round a (mirrored, scale) pair to a stable cache key - the mirror flag
// folds into the X component's sign, since bake_primitive only ever
// negates X for a mirrored instance. Matches
// openskp.export.fragments._scale_cache_key.
std::array<double, 3> scale_cache_key(bool mirrored, const std::array<double, 3>& scale) {
  const double mult = std::pow(10.0, kScaleRoundNdigits);
  auto rnd = [&](double v) { return std::round(v * mult) / mult; };
  return {rnd(mirrored ? -scale[0] : scale[0]), rnd(scale[1]), rnd(scale[2])};
}

// ---- Read side (openskp#285) below ----

// RepresentationClass values this module can read geometry for today -
// mirrors Python's own _SUPPORTED_REPRESENTATION_CLASSES and its note on
// why (only SHELL is written by any of this project's own exporters yet).
bool is_supported_representation_class(fb::RepresentationClass cls) {
  return cls == fb::RepresentationClass_SHELL;
}

std::array<double, 3> cross3(const std::array<double, 3>& a, const std::array<double, 3>& b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

// One normal per vertex, flat-shaded: each triangle's own face normal,
// duplicated across its 3 vertices - the most a Shell (points + indices,
// nothing else) can ever give back. Mirrors Python's
// _compute_flat_normals.
std::vector<std::array<double, 3>> compute_flat_normals(
    const std::vector<std::array<double, 3>>& points,
    const std::vector<std::array<std::uint32_t, 3>>& triangles) {
  std::vector<std::array<double, 3>> normals(points.size(), std::array<double, 3>{0.0, 0.0, 1.0});
  for (const auto& tri : triangles) {
    const auto& pa = points[tri[0]];
    const auto& pb = points[tri[1]];
    const auto& pc = points[tri[2]];
    const std::array<double, 3> u{pb[0] - pa[0], pb[1] - pa[1], pb[2] - pa[2]};
    const std::array<double, 3> v{pc[0] - pa[0], pc[1] - pa[1], pc[2] - pa[2]};
    auto n = cross3(u, v);
    const double length = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (length > 1e-12) {
      n = {n[0] / length, n[1] / length, n[2] / length};
    }
    normals[tri[0]] = n;
    normals[tri[1]] = n;
    normals[tri[2]] = n;
  }
  return normals;
}

// Reassemble a glTF-style column-major 4x4 matrix from a Fragments
// Transform struct (position + x/y direction unit vectors). The struct
// never stores a Z direction - reconstructed here as `x_dir cross
// y_dir`, matching the same right-handed orthonormal frame
// decompose_trs() produces on export. Mirrors Python's
// _transform_to_matrix16.
Mat4 transform_to_matrix16(const fb::Transform& transform) {
  const auto& pos = transform.position();
  const auto& x_dir_s = transform.x_direction();
  const auto& y_dir_s = transform.y_direction();
  const std::array<double, 3> x_dir{x_dir_s.x(), x_dir_s.y(), x_dir_s.z()};
  const std::array<double, 3> y_dir{y_dir_s.x(), y_dir_s.y(), y_dir_s.z()};
  const auto z_dir = cross3(x_dir, y_dir);
  return {
      x_dir[0], x_dir[1], x_dir[2], 0.0, y_dir[0], y_dir[1], y_dir[2], 0.0,
      z_dir[0], z_dir[1], z_dir[2], 0.0, pos.x(),  pos.y(),  pos.z(),  1.0,
  };
}

// Minimal recursive-descent JSON reader for the read side (from_fragments)
// - the mirror of the JSON-writing helpers this file's write side uses
// (JsonValue, in json_export.hpp) - needed because this package
// deliberately takes on no JSON library dependency (miniz/flatbuffers are
// its only ones). Handles the full JSON value grammar (null/bool/number/
// string/array/object) since a real .frag file's Attribute::data()/
// Model::metadata() strings aren't guaranteed to come from this project's
// own writer - a genuine ThatOpen IfcImporter export is a valid input
// too.
class MinimalJsonValue {
 public:
  enum class Kind { Null, Bool, Number, String, Array, Object };

  MinimalJsonValue() : kind_(Kind::Null) {}

  static MinimalJsonValue make_bool(bool b) {
    MinimalJsonValue v;
    v.kind_ = Kind::Bool;
    v.bool_ = b;
    return v;
  }

  static MinimalJsonValue make_string(std::string s) {
    MinimalJsonValue v;
    v.kind_ = Kind::String;
    v.string_ = std::move(s);
    return v;
  }

  static MinimalJsonValue make_array(std::vector<MinimalJsonValue> a) {
    MinimalJsonValue v;
    v.kind_ = Kind::Array;
    v.array_ = std::move(a);
    return v;
  }

  static MinimalJsonValue make_object(std::map<std::string, MinimalJsonValue> o) {
    MinimalJsonValue v;
    v.kind_ = Kind::Object;
    v.object_ = std::move(o);
    return v;
  }

  Kind kind() const { return kind_; }

  bool as_bool() const { return bool_; }

  const std::string& as_string() const { return string_; }

  const std::vector<MinimalJsonValue>& as_array() const { return array_; }

  const std::map<std::string, MinimalJsonValue>& as_object() const { return object_; }

 private:
  Kind kind_;
  bool bool_{false};
  double number_{0.0};
  std::string string_;
  std::vector<MinimalJsonValue> array_;
  std::map<std::string, MinimalJsonValue> object_;
};

class MinimalJsonParser {
 public:
  // Owns a copy rather than storing a `const std::string&` - a caller
  // passing a temporary (e.g. FlatBuffers' own `String::str()`, which
  // returns by value) would otherwise leave this dangling the instant the
  // constructor call's full expression ends, since a reference member
  // isn't a binding that extends a temporary's lifetime the way a local
  // `const std::string&` variable is. Caught directly: a real test
  // (metadata's layer_hidden round-trip) silently came back empty
  // instead of crashing outright, exactly the kind of UB that doesn't
  // reliably announce itself.
  explicit MinimalJsonParser(std::string s) : s_(std::move(s)), i_(0) {}

  MinimalJsonValue parse() { return parse_value(); }

 private:
  std::string s_;
  std::size_t i_;

  void skip_ws() {
    while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) ++i_;
  }

  MinimalJsonValue parse_value() {
    skip_ws();
    if (i_ >= s_.size()) throw std::runtime_error("unexpected end of JSON");
    const char c = s_[i_];
    if (c == '"') return parse_string_value();
    if (c == '{') return parse_object();
    if (c == '[') return parse_array();
    if (c == 't' && s_.compare(i_, 4, "true") == 0) {
      i_ += 4;
      return MinimalJsonValue::make_bool(true);
    }
    if (c == 'f' && s_.compare(i_, 5, "false") == 0) {
      i_ += 5;
      return MinimalJsonValue::make_bool(false);
    }
    if (c == 'n' && s_.compare(i_, 4, "null") == 0) {
      i_ += 4;
      return MinimalJsonValue();
    }
    // number - skip over it, this module never needs a parsed number value
    while (i_ < s_.size() && (std::isdigit(static_cast<unsigned char>(s_[i_])) || s_[i_] == '-' ||
                              s_[i_] == '+' || s_[i_] == '.' || s_[i_] == 'e' || s_[i_] == 'E')) {
      ++i_;
    }
    return MinimalJsonValue();
  }

  std::string parse_raw_string() {
    ++i_;  // opening quote
    std::string out;
    while (i_ < s_.size() && s_[i_] != '"') {
      char c = s_[i_];
      if (c == '\\') {
        ++i_;
        switch (s_[i_]) {
          case '"':
            out.push_back('"');
            break;
          case '\\':
            out.push_back('\\');
            break;
          case '/':
            out.push_back('/');
            break;
          case 'n':
            out.push_back('\n');
            break;
          case 'r':
            out.push_back('\r');
            break;
          case 't':
            out.push_back('\t');
            break;
          case 'b':
            out.push_back('\b');
            break;
          case 'f':
            out.push_back('\f');
            break;
          case 'u': {
            const unsigned code =
                static_cast<unsigned>(std::stoul(s_.substr(i_ + 1, 4), nullptr, 16));
            if (code < 0x80) {
              out.push_back(static_cast<char>(code));
            } else if (code < 0x800) {
              out.push_back(static_cast<char>(0xC0 | (code >> 6)));
              out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            } else {
              out.push_back(static_cast<char>(0xE0 | (code >> 12)));
              out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
              out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
            i_ += 4;
            break;
          }
          default:
            break;
        }
        ++i_;
      } else {
        out.push_back(c);
        ++i_;
      }
    }
    ++i_;  // closing quote
    return out;
  }

  MinimalJsonValue parse_string_value() {
    return MinimalJsonValue::make_string(parse_raw_string());
  }

  MinimalJsonValue parse_array() {
    std::vector<MinimalJsonValue> items;
    ++i_;  // '['
    skip_ws();
    if (i_ < s_.size() && s_[i_] == ']') {
      ++i_;
      return MinimalJsonValue::make_array(std::move(items));
    }
    while (true) {
      items.push_back(parse_value());
      skip_ws();
      if (i_ < s_.size() && s_[i_] == ',') {
        ++i_;
        continue;
      }
      if (i_ < s_.size() && s_[i_] == ']') {
        ++i_;
        break;
      }
      throw std::runtime_error("malformed JSON array");
    }
    return MinimalJsonValue::make_array(std::move(items));
  }

  MinimalJsonValue parse_object() {
    std::map<std::string, MinimalJsonValue> obj;
    ++i_;  // '{'
    skip_ws();
    if (i_ < s_.size() && s_[i_] == '}') {
      ++i_;
      return MinimalJsonValue::make_object(std::move(obj));
    }
    while (true) {
      skip_ws();
      const std::string key = parse_raw_string();
      skip_ws();
      ++i_;  // ':'
      obj.emplace(key, parse_value());
      skip_ws();
      if (i_ < s_.size() && s_[i_] == ',') {
        ++i_;
        continue;
      }
      if (i_ < s_.size() && s_[i_] == '}') {
        ++i_;
        break;
      }
      throw std::runtime_error("malformed JSON object");
    }
    return MinimalJsonValue::make_object(std::move(obj));
  }
};

// Pull the `["Name", value, "STRING"]` entry out of an item's
// Attribute::data() strings, matching the exact convention to_fragments()
// (and the real IfcImporter) writes. Mirrors Python's
// _extract_name_attribute.
std::string extract_name_attribute(const fb::Attribute* attribute) {
  if (attribute == nullptr || attribute->data() == nullptr) return "";
  for (const auto* raw_offset : *attribute->data()) {
    const std::string raw = raw_offset->str();
    try {
      MinimalJsonParser parser(raw);
      const MinimalJsonValue parsed = parser.parse();
      if (parsed.kind() == MinimalJsonValue::Kind::Array) {
        const auto& arr = parsed.as_array();
        if (arr.size() >= 2 && arr[0].kind() == MinimalJsonValue::Kind::String &&
            arr[0].as_string() == "Name" && arr[1].kind() == MinimalJsonValue::Kind::String) {
          return arr[1].as_string();
        }
      }
    } catch (const std::exception&) {
      continue;
    }
  }
  return "";
}

// RFC 1950 (zlib) inflate via miniz's mz_uncompress, growing the
// destination buffer and retrying on MZ_BUF_ERROR since (unlike
// compression) the decompressed size isn't known upfront.
std::vector<std::uint8_t> zlib_decompress(const std::vector<std::uint8_t>& data) {
  mz_ulong dest_cap = static_cast<mz_ulong>(std::max<std::size_t>(data.size() * 4, 4096));
  for (int attempt = 0; attempt < 20; ++attempt) {
    std::vector<std::uint8_t> dest(dest_cap);
    mz_ulong dest_len = dest_cap;
    const int rc =
        mz_uncompress(dest.data(), &dest_len, data.data(), static_cast<mz_ulong>(data.size()));
    if (rc == MZ_OK) {
      dest.resize(dest_len);
      return dest;
    }
    if (rc != MZ_BUF_ERROR) {
      throw std::runtime_error("Fragments import: zlib decompression failed (miniz error " +
                               std::to_string(rc) + ")");
    }
    dest_cap *= 2;
  }
  throw std::runtime_error(
      "Fragments import: zlib decompression failed (destination buffer never large enough)");
}

}  // namespace

std::vector<std::uint8_t> to_fragments(const InstancedScene& scene, bool raw) {
  std::vector<Leaf> leaves;
  collect_leaves(scene.scene_hierarchy, &scene.scene_hierarchy, kIdentityMatrix, leaves);

  std::map<std::string, const InstancedMeshResource*> resource_by_id;
  for (auto& r : scene.mesh_resources) resource_by_id[r.id] = &r;

  ::flatbuffers::FlatBufferBuilder fbb(1024 * 64);

  // ---- Shells/Representations/Materials, built lazily as leaves are
  // walked below: keyed by (resource, primitive, baked scale) so every
  // placement sharing the same definition AND the same scale/mirror state
  // dedupes onto one Shell - only a genuinely distinct scale factor pays
  // for its own geometry copy. ----
  using ShellKey = std::tuple<std::string, int, double, double, double>;
  std::map<ShellKey, std::vector<std::size_t>> shell_key_to_index;
  std::vector<::flatbuffers::Offset<fb::Shell>> shell_offsets;
  std::vector<std::pair<std::array<float, 3>, std::array<float, 3>>> representation_bounds;
  std::map<int, std::size_t> material_key_to_index;
  std::vector<std::array<std::uint8_t, 4>> material_rgba;

  auto get_material_index = [&](int material_index) -> std::size_t {
    auto found = material_key_to_index.find(material_index);
    if (found != material_key_to_index.end()) return found->second;
    std::array<double, 4> base{1.0, 1.0, 1.0, 1.0};
    if (material_index >= 0 &&
        static_cast<std::size_t>(material_index) < scene.gltf_materials.size()) {
      const auto& gm = scene.gltf_materials[static_cast<std::size_t>(material_index)];
      base = gm.pbr_metallic_roughness.base_color_factor;
    }
    std::array<std::uint8_t, 4> rgba{
        static_cast<std::uint8_t>(std::lround(base[0] * 255)),
        static_cast<std::uint8_t>(std::lround(base[1] * 255)),
        static_cast<std::uint8_t>(std::lround(base[2] * 255)),
        static_cast<std::uint8_t>(std::lround(base[3] * 255)),
    };
    const auto idx = material_rgba.size();
    material_rgba.push_back(rgba);
    material_key_to_index[material_index] = idx;
    return idx;
  };

  auto bake_one_shell =
      [&](const std::vector<std::array<float, 3>>& points,
          const std::vector<std::array<std::uint32_t, 3>>& triangles) -> std::size_t {
    const bool is_big = points.size() > kUshortMax;

    std::vector<::flatbuffers::Offset<fb::ShellProfile>> profile_offsets;
    std::vector<::flatbuffers::Offset<fb::BigShellProfile>> big_profile_offsets;
    for (auto& tri : triangles) {
      if (is_big) {
        std::vector<std::uint32_t> idx{tri[0], tri[1], tri[2]};
        big_profile_offsets.push_back(fb::CreateBigShellProfileDirect(fbb, &idx));
      } else {
        std::vector<std::uint16_t> idx{static_cast<std::uint16_t>(tri[0]),
                                       static_cast<std::uint16_t>(tri[1]),
                                       static_cast<std::uint16_t>(tri[2])};
        profile_offsets.push_back(fb::CreateShellProfileDirect(fbb, &idx));
      }
    }

    std::vector<fb::FloatVector> points_struct;
    points_struct.reserve(points.size());
    for (auto& p : points) points_struct.emplace_back(p[0], p[1], p[2]);

    // Sequential per-shell profile ids (0..triangles.size()-1, not tied to
    // any upstream SketchUp face identity - see the caller for how a
    // too-large triangle list is chunked before reaching here), so
    // re-numbering from 0 per shell is exactly consistent with the
    // single-shell behavior this is a straight extraction of.
    std::vector<std::uint16_t> face_ids(triangles.size());
    for (std::size_t i = 0; i < face_ids.size(); ++i) face_ids[i] = static_cast<std::uint16_t>(i);

    std::vector<::flatbuffers::Offset<fb::ShellHole>> no_holes;
    std::vector<::flatbuffers::Offset<fb::BigShellHole>> no_big_holes;

    const auto shell_off = fb::CreateShellDirect(
        fbb, &profile_offsets, &no_holes, &points_struct, &big_profile_offsets, &no_big_holes,
        is_big ? fb::ShellType_BIG : fb::ShellType_NONE, &face_ids);

    const auto index = shell_offsets.size();
    shell_offsets.push_back(shell_off);

    std::array<float, 3> lo{std::numeric_limits<float>::infinity(),
                            std::numeric_limits<float>::infinity(),
                            std::numeric_limits<float>::infinity()};
    std::array<float, 3> hi{-std::numeric_limits<float>::infinity(),
                            -std::numeric_limits<float>::infinity(),
                            -std::numeric_limits<float>::infinity()};
    for (auto& p : points) {
      for (int k = 0; k < 3; ++k) {
        if (p[static_cast<std::size_t>(k)] < lo[static_cast<std::size_t>(k)])
          lo[static_cast<std::size_t>(k)] = p[static_cast<std::size_t>(k)];
        if (p[static_cast<std::size_t>(k)] > hi[static_cast<std::size_t>(k)])
          hi[static_cast<std::size_t>(k)] = p[static_cast<std::size_t>(k)];
      }
    }
    representation_bounds.emplace_back(lo, hi);

    return index;
  };

  auto get_or_bake_shell = [&](const std::string& resource_id, int prim_idx,
                               const LocalPrimitive& prim, const std::array<double, 3>& scale,
                               bool mirrored) -> std::vector<std::size_t> {
    const auto sk = scale_cache_key(mirrored, scale);
    const ShellKey key{resource_id, prim_idx, sk[0], sk[1], sk[2]};
    auto found = shell_key_to_index.find(key);
    if (found != shell_key_to_index.end()) return found->second;

    const auto baked = bake_primitive(prim, scale, mirrored);

    // `profiles_face_ids` (written above) has no "big"/uint32 counterpart
    // anywhere in the real Fragments schema (index.fbs only declares
    // `profiles_face_ids: [ushort]` - unlike points, which DO get a
    // BigShellProfile/uint32-index escape hatch past 65535 of them). A
    // single shell genuinely cannot represent more than 65535 triangles no
    // matter how points are encoded - see Python's own fix (openskp#285/
    // PR #355) for the real production incident this was ported from.
    // Splitting into multiple shells, each within the ushort limit, is the
    // only way to represent this - the format has no cap on shell COUNT,
    // just per-shell triangle count. Each sub-shell duplicates the full
    // (shared) points array rather than remapping to a local subset:
    // simpler and lower-risk than a vertex-remapping pass, at the cost of
    // some extra file size in this rare oversized-mesh case.
    std::vector<std::size_t> indices;
    if (baked.triangles.size() > kUshortMax) {
      for (std::size_t i = 0; i < baked.triangles.size(); i += kUshortMax) {
        const std::size_t end = std::min(i + kUshortMax, baked.triangles.size());
        std::vector<std::array<std::uint32_t, 3>> chunk(
            baked.triangles.begin() + static_cast<std::ptrdiff_t>(i),
            baked.triangles.begin() + static_cast<std::ptrdiff_t>(end));
        indices.push_back(bake_one_shell(baked.points, chunk));
      }
    } else {
      indices.push_back(bake_one_shell(baked.points, baked.triangles));
    }

    shell_key_to_index[key] = indices;
    return indices;
  };

  // ---- Model-level items + geometry samples ----
  std::vector<std::uint32_t> local_ids;
  std::vector<std::string> categories;
  std::vector<std::string> names;
  std::vector<std::string> guids;
  std::vector<std::string> generated_name_guids;
  std::vector<std::size_t> sample_material;
  std::vector<std::size_t> sample_representation;
  std::vector<std::uint32_t> meshes_items;
  std::vector<Trs> global_transform_data;
  std::map<const InstancedNode*, std::uint32_t> item_index_by_node;

  // Real-world SketchUp files can carry a non-unique per-instance GUID
  // (SketchUp's native Copy/Move+Copy/Array duplicates an instance's
  // attribute dictionaries - and whatever GUID a plugin wrote into one -
  // verbatim). See the identical, more fully-documented fix in Python's
  // export/fragments.py (openskp#290) for the full rationale; ported here
  // unchanged: the first instance to claim a real GUID keeps it, any later
  // instance sharing that value falls back to a synthetic one instead.
  std::set<std::string> seen_guids;

  for (std::size_t item_index = 0; item_index < leaves.size(); ++item_index) {
    const auto& leaf = leaves[item_index];
    const auto& node = *leaf.node;
    const InstancedMeshResource* res = nullptr;
    if (node.mesh_resource_id) {
      auto found = resource_by_id.find(*node.mesh_resource_id);
      if (found == resource_by_id.end())
        continue;  // real error case: leaf declared a resource that never baked
      res = found->second;
    }

    local_ids.push_back(static_cast<std::uint32_t>(item_index));
    categories.push_back(node.layer.empty() ? "Layer0" : node.layer);
    names.push_back(node.name);

    const std::string raw_guid = node.guid;
    const std::string item_guid = (!raw_guid.empty() && !seen_guids.count(raw_guid))
                                      ? raw_guid
                                      : ("openskp-" + std::to_string(item_index));
    seen_guids.insert(item_guid);
    guids.push_back(item_guid);
    if (node.name_is_generated) generated_name_guids.push_back(item_guid);
    item_index_by_node[&node] = static_cast<std::uint32_t>(item_index);

    if (res != nullptr) {
      const auto trs = decompose_trs(leaf.world);
      for (std::size_t prim_idx = 0; prim_idx < res->primitives.size(); ++prim_idx) {
        const auto& prim = res->primitives[prim_idx];
        const auto material_index = get_material_index(static_cast<int>(prim.material_index));
        // Normally exactly one shell; more than one only when the
        // primitive's own triangle count exceeded what a single shell can
        // represent (see get_or_bake_shell) - each extra shell becomes its
        // own additional Sample of the same item/material/transform, the
        // same pattern this loop already uses for multiple primitives of
        // one item.
        for (auto shell_index :
             get_or_bake_shell(*node.mesh_resource_id, static_cast<int>(prim_idx), prim, trs.scale,
                               trs.mirrored)) {
          sample_material.push_back(material_index);
          sample_representation.push_back(shell_index);
          meshes_items.push_back(static_cast<std::uint32_t>(item_index));
          global_transform_data.push_back(trs);
        }
      }
    }
  }

  const std::size_t n_samples = sample_material.size();

  const auto shells_vec = fbb.CreateVector(shell_offsets);

  std::vector<fb::Material> material_structs;
  material_structs.reserve(material_rgba.size());
  for (auto& rgba : material_rgba) {
    material_structs.emplace_back(rgba[0], rgba[1], rgba[2], rgba[3], fb::RenderedFaces_ONE,
                                  fb::Stroke_DEFAULT);
  }
  const auto materials_vec = fbb.CreateVectorOfStructs(material_structs);

  std::vector<fb::Representation> representation_structs;
  representation_structs.reserve(representation_bounds.size());
  for (std::size_t i = 0; i < representation_bounds.size(); ++i) {
    const auto& [lo, hi] = representation_bounds[i];
    representation_structs.emplace_back(
        static_cast<std::uint32_t>(i),
        fb::BoundingBox(fb::FloatVector(lo[0], lo[1], lo[2]), fb::FloatVector(hi[0], hi[1], hi[2])),
        fb::RepresentationClass_SHELL);
  }
  const auto representations_vec = fbb.CreateVectorOfStructs(representation_structs);

  std::vector<fb::Sample> sample_structs;
  sample_structs.reserve(n_samples);
  for (std::size_t i = 0; i < n_samples; ++i) {
    sample_structs.emplace_back(static_cast<std::uint32_t>(i),
                                static_cast<std::uint32_t>(sample_material[i]),
                                static_cast<std::uint32_t>(sample_representation[i]), 0u);
  }
  const auto samples_vec = fbb.CreateVectorOfStructs(sample_structs);

  const auto meshes_items_vec = fbb.CreateVector(meshes_items);

  std::vector<fb::Transform> global_transform_structs;
  global_transform_structs.reserve(global_transform_data.size());
  for (auto& trs : global_transform_data) {
    global_transform_structs.emplace_back(
        fb::DoubleVector(trs.position[0], trs.position[1], trs.position[2]),
        fb::FloatVector(static_cast<float>(trs.x_dir[0]), static_cast<float>(trs.x_dir[1]),
                        static_cast<float>(trs.x_dir[2])),
        fb::FloatVector(static_cast<float>(trs.y_dir[0]), static_cast<float>(trs.y_dir[1]),
                        static_cast<float>(trs.y_dir[2])));
  }
  const auto global_transforms_vec = fbb.CreateVectorOfStructs(global_transform_structs);

  // One shared identity local transform - no per-geometry sub-offset is
  // needed since every primitive's points are already in the resource's
  // own local space (now with scale/mirror already baked in).
  std::vector<fb::Transform> local_transform_structs{
      fb::Transform(fb::DoubleVector(0, 0, 0), fb::FloatVector(1, 0, 0), fb::FloatVector(0, 1, 0)),
  };
  const auto local_transforms_vec = fbb.CreateVectorOfStructs(local_transform_structs);

  std::vector<::flatbuffers::Offset<fb::CircleExtrusion>> no_circle_extrusions;
  const auto circle_extrusions_vec = fbb.CreateVector(no_circle_extrusions);

  const fb::Transform coordinates(fb::DoubleVector(0, 0, 0), fb::FloatVector(1, 0, 0),
                                  fb::FloatVector(0, 1, 0));

  const auto meshes_off = fb::CreateMeshes(
      fbb, &coordinates, meshes_items_vec, samples_vec, representations_vec, materials_vec,
      circle_extrusions_vec, shells_vec, local_transforms_vec, global_transforms_vec);

  const auto categories_vec = fbb.CreateVectorOfStrings(categories);
  const auto local_ids_vec = fbb.CreateVector(local_ids);
  const auto guid_str = fbb.CreateString("00000000-0000-0000-0000-000000000000");
  const auto guids_vec = fbb.CreateVectorOfStrings(guids);
  const auto guids_items_vec = fbb.CreateVector(local_ids);

  // One Attribute per tracked item (same order as local_ids/categories),
  // carrying the item's real display name encoded as a `["Name", value,
  // "STRING"]` JSON triple - the exact convention the real IfcImporter
  // uses for its own "Name" attribute. Matches
  // openskp.export.fragments.to_fragments exactly.
  std::vector<::flatbuffers::Offset<fb::Attribute>> attribute_offsets;
  attribute_offsets.reserve(names.size());
  for (auto& name : names) {
    std::vector<::flatbuffers::Offset<::flatbuffers::String>> data_offsets;
    if (!name.empty()) {
      JsonValue::Array triple;
      triple.push_back(JsonValue("Name"));
      triple.push_back(JsonValue(name));
      triple.push_back(JsonValue("STRING"));
      data_offsets.push_back(fbb.CreateString(to_json_string(JsonValue(triple), 0)));
    }
    attribute_offsets.push_back(fb::CreateAttributeDirect(fbb, &data_offsets));
  }
  const auto attributes_vec = fbb.CreateVector(attribute_offsets);

  // The source file's own per-layer visibility has no equivalent field
  // anywhere in the Fragments schema itself - `metadata` is the schema's
  // own general-purpose "JSON string for generic data about the file"
  // field, same sidecar convention Python's exporter uses.
  JsonValue::Object metadata_obj;
  JsonValue::Object layer_hidden_obj;
  for (auto& [layer, hidden] : scene.layer_hidden)
    layer_hidden_obj.emplace_back(layer, JsonValue(hidden));
  metadata_obj.emplace_back("layer_hidden", JsonValue(layer_hidden_obj));
  JsonValue::Array generated_arr;
  for (auto& g : generated_name_guids) generated_arr.push_back(JsonValue(g));
  metadata_obj.emplace_back("generated_name_guids", JsonValue(generated_arr));
  const auto metadata_off = fbb.CreateString(to_json_string(JsonValue(metadata_obj), 0));

  // Spatial structure: one SpatialStructure node per InstancedNode in the
  // ORIGINAL tree (not just leaves), so real component nesting comes
  // through, not just a flat list. Matches
  // openskp.export.fragments.to_fragments's build_spatial_node exactly.
  std::function<::flatbuffers::Offset<fb::SpatialStructure>(const InstancedNode&)>
      build_spatial_node;
  build_spatial_node =
      [&](const InstancedNode& node) -> ::flatbuffers::Offset<fb::SpatialStructure> {
    std::vector<::flatbuffers::Offset<fb::SpatialStructure>> child_offsets;
    child_offsets.reserve(node.children.size());
    for (auto& child : node.children) child_offsets.push_back(build_spatial_node(child));
    const auto children_vec = fbb.CreateVector(child_offsets);

    ::flatbuffers::Offset<::flatbuffers::String> category_off = 0;
    if (!node.layer.empty()) category_off = fbb.CreateString(node.layer);

    ::flatbuffers::Optional<std::uint32_t> local_id = ::flatbuffers::nullopt;
    auto found = item_index_by_node.find(&node);
    if (found != item_index_by_node.end()) local_id = found->second;

    return fb::CreateSpatialStructure(fbb, local_id, category_off, children_vec);
  };
  const auto root_spatial = build_spatial_node(scene.scene_hierarchy);

  const auto model_off = fb::CreateModel(
      fbb, metadata_off, guids_vec, guids_items_vec, static_cast<std::uint32_t>(local_ids.size()),
      local_ids_vec, categories_vec, meshes_off, attributes_vec, /*relations*/ 0,
      /*relations_items*/ 0, guid_str, root_spatial);

  fbb.Finish(model_off, "0001");

  const std::uint8_t* buf = fbb.GetBufferPointer();
  const std::size_t size = fbb.GetSize();

  if (raw) {
    return std::vector<std::uint8_t>(buf, buf + size);
  }

  mz_ulong bound = mz_compressBound(static_cast<mz_ulong>(size));
  std::vector<std::uint8_t> compressed(bound);
  mz_ulong compressed_len = bound;
  const int rc = mz_compress2(compressed.data(), &compressed_len, buf, static_cast<mz_ulong>(size),
                              MZ_DEFAULT_LEVEL);
  if (rc != MZ_OK) {
    throw std::runtime_error("Fragments export: zlib compression failed (miniz error " +
                             std::to_string(rc) + ")");
  }
  compressed.resize(compressed_len);
  return compressed;
}

void export_fragments(const InstancedScene& scene, const std::filesystem::path& output_path,
                      bool raw) {
  std::filesystem::create_directories(output_path.parent_path());
  const auto data = to_fragments(scene, raw);
  std::ofstream out(output_path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

InstancedScene from_fragments(const std::vector<std::uint8_t>& data) {
  std::vector<std::uint8_t> raw_bytes;
  try {
    raw_bytes = zlib_decompress(data);
  } catch (const std::exception&) {
    raw_bytes = data;
  }

  const fb::Model* model = fb::GetModel(raw_bytes.data());
  const fb::Meshes* meshes = model->meshes();

  // ---- Materials (flat RGBA - no texture concept exists in this format
  // at all). ----
  std::vector<GltfMaterial> gltf_materials;
  if (meshes != nullptr && meshes->materials() != nullptr) {
    for (const auto* mat : *meshes->materials()) {
      GltfMaterial gm;
      gm.pbr_metallic_roughness.base_color_factor = {mat->r() / 255.0, mat->g() / 255.0,
                                                     mat->b() / 255.0, mat->a() / 255.0};
      gm.pbr_metallic_roughness.metallic_factor = 0.0;
      gm.pbr_metallic_roughness.roughness_factor = 1.0;
      gm.double_sided = mat->rendered_faces() != fb::RenderedFaces_ONE;
      gltf_materials.push_back(std::move(gm));
    }
  }
  if (gltf_materials.empty()) {
    GltfMaterial gm;
    gm.pbr_metallic_roughness.base_color_factor = {0.8, 0.8, 0.8, 1.0};
    gm.pbr_metallic_roughness.metallic_factor = 0.0;
    gm.pbr_metallic_roughness.roughness_factor = 1.0;
    gltf_materials.push_back(std::move(gm));
  }

  // ---- Shells -> (points, triangles), decoded once per shell index,
  // reused by every sample referencing it. ----
  const std::size_t n_shells =
      (meshes != nullptr && meshes->shells() != nullptr) ? meshes->shells()->size() : 0;
  std::vector<std::vector<std::array<double, 3>>> shell_points(n_shells);
  std::vector<std::vector<std::array<std::uint32_t, 3>>> shell_triangles(n_shells);
  std::vector<bool> shell_decoded(n_shells, false);

  auto decode_shell = [&](std::size_t shell_idx) -> void {
    if (shell_decoded[shell_idx]) return;
    const fb::Shell* shell = meshes->shells()->Get(static_cast<flatbuffers::uoffset_t>(shell_idx));
    auto& points = shell_points[shell_idx];
    if (shell->points() != nullptr) {
      points.reserve(shell->points()->size());
      for (const auto* p : *shell->points()) {
        points.push_back({p->x(), p->y(), p->z()});
      }
    }

    auto& triangles = shell_triangles[shell_idx];
    const bool is_big = shell->type() == fb::ShellType_BIG;
    if (is_big) {
      if (shell->big_profiles() != nullptr) {
        for (const auto* profile : *shell->big_profiles()) {
          if (profile->indices() != nullptr && profile->indices()->size() >= 3) {
            triangles.push_back({profile->indices()->Get(0), profile->indices()->Get(1),
                                 profile->indices()->Get(2)});
          }
        }
      }
    } else {
      if (shell->profiles() != nullptr) {
        for (const auto* profile : *shell->profiles()) {
          if (profile->indices() != nullptr && profile->indices()->size() >= 3) {
            triangles.push_back({profile->indices()->Get(0), profile->indices()->Get(1),
                                 profile->indices()->Get(2)});
          }
        }
      }
    }
    shell_decoded[shell_idx] = true;
  };

  // ---- Group samples by item (Meshes::meshes_items()[k] -> item index,
  // NOT Sample::item() - see to_fragments's own comment on why the two
  // differ; Sample::item() is just the sample's own position, always). ----
  const std::size_t n_samples =
      (meshes != nullptr && meshes->samples() != nullptr) ? meshes->samples()->size() : 0;
  std::map<std::uint32_t, std::vector<std::size_t>> samples_by_item;
  for (std::size_t k = 0; k < n_samples; ++k) {
    const std::uint32_t item_idx =
        meshes->meshes_items()->Get(static_cast<flatbuffers::uoffset_t>(k));
    samples_by_item[item_idx].push_back(k);
  }

  // ---- Per-item metadata: name, guid, category, world transform.
  // local_ids[i] IS the item index space - see to_fragments's own
  // local_ids.push_back(item_index). ----
  const std::size_t n_items = (model->local_ids() != nullptr) ? model->local_ids()->size() : 0;
  std::map<std::uint32_t, std::size_t> local_id_to_item_index;
  for (std::size_t i = 0; i < n_items; ++i) {
    local_id_to_item_index[model->local_ids()->Get(static_cast<flatbuffers::uoffset_t>(i))] = i;
  }

  std::map<std::size_t, std::string> guid_by_item;
  const std::size_t n_guid_items =
      (model->guids_items() != nullptr) ? model->guids_items()->size() : 0;
  for (std::size_t i = 0; i < n_guid_items; ++i) {
    const std::uint32_t lid = model->guids_items()->Get(static_cast<flatbuffers::uoffset_t>(i));
    const auto it = local_id_to_item_index.find(lid);
    if (it != local_id_to_item_index.end() && model->guids() != nullptr &&
        i < model->guids()->size()) {
      guid_by_item[it->second] = model->guids()->Get(static_cast<flatbuffers::uoffset_t>(i))->str();
    }
  }

  std::map<std::string, bool> layer_hidden;
  std::set<std::string> generated_name_guids;
  if (model->metadata() != nullptr) {
    try {
      MinimalJsonParser parser(model->metadata()->str());
      const MinimalJsonValue metadata = parser.parse();
      if (metadata.kind() == MinimalJsonValue::Kind::Object) {
        const auto& obj = metadata.as_object();
        const auto lh_it = obj.find("layer_hidden");
        if (lh_it != obj.end() && lh_it->second.kind() == MinimalJsonValue::Kind::Object) {
          for (const auto& [key, value] : lh_it->second.as_object()) {
            if (value.kind() == MinimalJsonValue::Kind::Bool) layer_hidden[key] = value.as_bool();
          }
        }
        const auto gn_it = obj.find("generated_name_guids");
        if (gn_it != obj.end() && gn_it->second.kind() == MinimalJsonValue::Kind::Array) {
          for (const auto& item : gn_it->second.as_array()) {
            if (item.kind() == MinimalJsonValue::Kind::String)
              generated_name_guids.insert(item.as_string());
          }
        }
      }
    } catch (const std::exception&) {
      // leave layer_hidden/generated_name_guids empty
    }
  }

  std::vector<InstancedMeshResource> mesh_resources;
  std::map<std::string, std::string> resource_by_signature;
  bool warned_unsupported = false;

  std::function<std::string(std::size_t, const std::string&)> build_resource_for_item =
      [&](std::size_t item_idx, const std::string& item_name) -> std::string {
    const auto sample_it = samples_by_item.find(static_cast<std::uint32_t>(item_idx));
    std::vector<std::size_t> sample_indices =
        sample_it != samples_by_item.end() ? sample_it->second : std::vector<std::size_t>{};
    std::vector<std::pair<std::uint32_t, std::uint32_t>> signature;
    std::vector<LocalPrimitive> primitives;
    for (const auto k : sample_indices) {
      const fb::Sample* sample = meshes->samples()->Get(static_cast<flatbuffers::uoffset_t>(k));
      const std::uint32_t rep_idx = sample->representation();
      const std::uint32_t mat_idx = sample->material();
      const fb::Representation* representation =
          (meshes->representations() != nullptr && rep_idx < meshes->representations()->size())
              ? meshes->representations()->Get(static_cast<flatbuffers::uoffset_t>(rep_idx))
              : nullptr;
      const fb::RepresentationClass rep_class = representation != nullptr
                                                    ? representation->representation_class()
                                                    : fb::RepresentationClass_SHELL;
      if (!is_supported_representation_class(rep_class)) {
        if (!warned_unsupported) {
          std::cerr
              << "openskp from_fragments: skipping a sample with unsupported RepresentationClass="
              << static_cast<int>(rep_class) << " (only SHELL is read today).\n";
          warned_unsupported = true;
        }
        continue;
      }
      signature.emplace_back(rep_idx, mat_idx);
      // Representation::id() is the index into Meshes::shells() - NOT the
      // representation's own position in the representations vector. See
      // Python's from_fragments's own comment on why this must follow
      // id(), matching the real reader's own fetch-functions.ts.
      const std::uint32_t shell_idx = representation->id();
      if (shell_idx >= n_shells) continue;
      decode_shell(shell_idx);
      const auto& points = shell_points[shell_idx];
      const auto& triangles = shell_triangles[shell_idx];
      const auto normals = compute_flat_normals(points, triangles);

      LocalPrimitive prim;
      prim.positions.reserve(points.size() * 3);
      prim.normals.reserve(points.size() * 3);
      for (std::size_t i = 0; i < points.size(); ++i) {
        prim.positions.push_back(static_cast<float>(points[i][0]));
        prim.positions.push_back(static_cast<float>(points[i][1]));
        prim.positions.push_back(static_cast<float>(points[i][2]));
        prim.normals.push_back(static_cast<float>(normals[i][0]));
        prim.normals.push_back(static_cast<float>(normals[i][1]));
        prim.normals.push_back(static_cast<float>(normals[i][2]));
      }
      prim.uvs.assign(points.size() * 2, 0.0f);
      prim.indices.reserve(triangles.size() * 3);
      for (const auto& tri : triangles) {
        prim.indices.push_back(tri[0]);
        prim.indices.push_back(tri[1]);
        prim.indices.push_back(tri[2]);
      }
      prim.material_index = mat_idx < gltf_materials.size() ? mat_idx : 0;
      primitives.push_back(std::move(prim));
    }

    std::string sig_key;
    for (const auto& [r, m] : signature) {
      sig_key += std::to_string(r) + ":" + std::to_string(m) + ",";
    }
    if (!sig_key.empty()) {
      const auto found = resource_by_signature.find(sig_key);
      if (found != resource_by_signature.end()) return found->second;
    }

    const std::string resource_id = "frag-" + std::to_string(item_idx);
    InstancedMeshResource resource;
    resource.id = resource_id;
    resource.definition_id = static_cast<EntityId>(item_idx);
    resource.definition_name = item_name.empty() ? resource_id : item_name;
    resource.variant_key = "default";
    resource.primitives = std::move(primitives);
    mesh_resources.push_back(std::move(resource));
    if (!sig_key.empty()) resource_by_signature[sig_key] = resource_id;
    return resource_id;
  };

  // ---- Spatial structure -> InstancedNode tree. ----
  std::function<InstancedNode(const fb::SpatialStructure*)> build_node =
      [&](const fb::SpatialStructure* spatial) -> InstancedNode {
    const auto local_id = spatial->local_id();
    std::string category = spatial->category() != nullptr ? spatial->category()->str() : "";

    InstancedNode node;
    node.definition_name = category;
    node.layer = category;

    if (local_id.has_value()) {
      const auto item_it = local_id_to_item_index.find(local_id.value());
      if (item_it != local_id_to_item_index.end()) {
        const std::size_t item_idx = item_it->second;
        const fb::Attribute* attribute =
            (model->attributes() != nullptr && item_idx < model->attributes()->size())
                ? model->attributes()->Get(static_cast<flatbuffers::uoffset_t>(item_idx))
                : nullptr;
        node.name = extract_name_attribute(attribute);
        const auto guid_it = guid_by_item.find(item_idx);
        node.guid = guid_it != guid_by_item.end() ? guid_it->second : "";
        node.name_is_generated = !node.guid.empty() && generated_name_guids.count(node.guid) > 0;
        const auto sample_it = samples_by_item.find(static_cast<std::uint32_t>(item_idx));
        if (sample_it != samples_by_item.end()) {
          node.mesh_resource_id =
              build_resource_for_item(item_idx, node.name.empty() ? category : node.name);
          const fb::Transform* transform = meshes->global_transforms()->Get(
              static_cast<flatbuffers::uoffset_t>(sample_it->second[0]));
          node.matrix = transform_to_matrix16(*transform);
        }
      }
    }

    if (spatial->children() != nullptr) {
      node.children.reserve(spatial->children()->size());
      for (const auto* child : *spatial->children()) {
        node.children.push_back(build_node(child));
      }
    }
    return node;
  };

  InstancedScene scene;
  scene.bounds = std::nullopt;
  if (model->spatial_structure() != nullptr) {
    scene.scene_hierarchy = build_node(model->spatial_structure());
  } else {
    scene.scene_hierarchy.name = "ROOT";
    scene.scene_hierarchy.definition_name = "ROOT";
  }
  scene.mesh_resources = std::move(mesh_resources);
  scene.gltf_materials = std::move(gltf_materials);
  scene.layer_hidden = std::move(layer_hidden);

  return scene;
}

InstancedScene read_fragments(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("Fragments import: could not open " + path.string());
  std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)),
                                 std::istreambuf_iterator<char>());
  return from_fragments(data);
}

}  // namespace openskp
