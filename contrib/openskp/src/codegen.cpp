// See include/openskp/codegen.hpp for the full module doc.

#include <array>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <openskp/codegen.hpp>
#include <openskp/create.hpp>

namespace openskp {
namespace {

using EdgeMap = std::map<EntityId, std::pair<EntityId, EntityId>>;

EdgeMap build_edge_map(const Definition& defn) {
  EdgeMap m;
  for (const auto& [id, e] : defn.edges) m[id] = {e.v1_id, e.v2_id};
  return m;
}

// Mirrors edit.cpp's own reconstruct_loop_vertices exactly - duplicated here (that one lives in
// edit.cpp's own anonymous namespace, not shared across translation units) rather than shared,
// matching this project's own established per-file-duplication precedent for this exact helper
// (Dart's codegen.dart/edit.dart do the same).
std::vector<EntityId> reconstruct_loop_vertices(const std::vector<CoEdge>& loop,
                                                const EdgeMap& edges) {
  std::vector<EntityId> verts;
  for (const auto& u : loop) {
    auto it = edges.find(u.edge_id);
    if (it == edges.end()) continue;
    EntityId v_start = u.orientation == 1 ? it->second.first : it->second.second;
    if (verts.empty() || verts.back() != v_start) verts.push_back(v_start);
  }
  if (verts.size() > 1 && verts.front() == verts.back()) verts.pop_back();
  return verts;
}

std::optional<std::vector<Point3>> loop_points(const std::vector<CoEdge>& loop,
                                               const EdgeMap& edges, const Definition& defn) {
  auto vert_ids = reconstruct_loop_vertices(loop, edges);
  if (vert_ids.size() < 3) return std::nullopt;
  std::vector<Point3> points;
  points.reserve(vert_ids.size());
  for (EntityId vid : vert_ids) {
    auto it = defn.vertices.find(vid);
    if (it != defn.vertices.end()) points.push_back({it->second.x, it->second.y, it->second.z});
  }
  if (points.size() < 3) return std::nullopt;
  return points;
}

// front_uv/back_uv need exactly 3 correspondences whose (u, v) values are NOT collinear - see
// edit.cpp's own non_collinear_triple for the full rationale (duplicated here for the same
// per-file reason as reconstruct_loop_vertices above).
std::optional<std::array<Point3, 3>> non_collinear_triple(const std::vector<Point3>& points) {
  for (std::size_t i = 0; i < points.size(); ++i) {
    for (std::size_t j = i + 1; j < points.size(); ++j) {
      for (std::size_t k = j + 1; k < points.size(); ++k) {
        const Point3& a = points[i];
        const Point3& b = points[j];
        const Point3& c = points[k];
        Point3 e1{b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        Point3 e2{c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        double cx = e1[1] * e2[2] - e1[2] * e2[1];
        double cy = e1[2] * e2[0] - e1[0] * e2[2];
        double cz = e1[0] * e2[1] - e1[1] * e2[0];
        if (cx * cx + cy * cy + cz * cz > 1e-9) return std::array<Point3, 3>{a, b, c};
      }
    }
  }
  return std::nullopt;
}

std::pair<Point3, Point3> face_uv_basis(Point3 n) {
  double cx = -n[1], cy = n[0];
  double clen = std::sqrt(cx * cx + cy * cy);
  if (clen < 1e-9) {
    return {{1.0, 0.0, 0.0}, {0.0, n[2] >= 0 ? 1.0 : -1.0, 0.0}};
  }
  Point3 xr{cx / clen, cy / clen, 0.0};
  Point3 yr{n[1] * xr[2] - n[2] * xr[1], n[2] * xr[0] - n[0] * xr[2], n[0] * xr[1] - n[1] * xr[0]};
  return {xr, yr};
}

std::array<double, 9> invert3x3(const std::array<double, 9>& m) {
  double a = m[0], b = m[1], c = m[2];
  double d = m[3], e = m[4], f = m[5];
  double g = m[6], h = m[7], i = m[8];
  double det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
  double inv_det = std::abs(det) < 1e-15 ? 0.0 : 1.0 / det;
  return {
      (e * i - f * h) * inv_det, (c * h - b * i) * inv_det, (b * f - c * e) * inv_det,
      (f * g - d * i) * inv_det, (a * i - c * g) * inv_det, (c * d - a * f) * inv_det,
      (d * h - e * g) * inv_det, (b * g - a * h) * inv_det, (a * e - b * d) * inv_det,
  };
}

std::pair<double, double> compute_face_uv(Point3 p, Point3 xr, Point3 yr,
                                          const std::optional<std::array<double, 9>>& uv_transform,
                                          double tile_w, double tile_h) {
  double px = p[0] * xr[0] + p[1] * xr[1] + p[2] * xr[2];
  double py = p[0] * yr[0] + p[1] * yr[1] + p[2] * yr[2];
  if (!uv_transform) return {px / tile_w, py / tile_h};
  auto inv = invert3x3(*uv_transform);
  double u = px * inv[0] + py * inv[3] + inv[6];
  double v = px * inv[1] + py * inv[4] + inv[7];
  double q = px * inv[2] + py * inv[5] + inv[8];
  if (std::abs(q) < 1e-12) q = 1.0;
  return {u / q / tile_w, v / q / tile_h};
}

std::string to_base64(const ByteBuffer& bytes) {
  static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((bytes.size() + 2) / 3) * 4);
  for (std::size_t i = 0; i < bytes.size(); i += 3) {
    std::uint32_t b0 = bytes[i];
    bool has1 = i + 1 < bytes.size();
    bool has2 = i + 2 < bytes.size();
    std::uint32_t b1 = has1 ? bytes[i + 1] : 0;
    std::uint32_t b2 = has2 ? bytes[i + 2] : 0;
    out += chars[b0 >> 2];
    out += chars[((b0 & 0x03) << 4) | (b1 >> 4)];
    out += has1 ? chars[((b1 & 0x0f) << 2) | (b2 >> 6)] : '=';
    out += has2 ? chars[b2 & 0x3f] : '=';
  }
  return out;
}

double round4(double n) {
  double r = std::round(n * 10000.0) / 10000.0;
  return r == 0.0 ? 0.0 : r;
}

std::string point_str(Point3 p) {
  std::ostringstream ss;
  ss << "{" << round4(p[0]) << ", " << round4(p[1]) << ", " << round4(p[2]) << "}";
  return ss.str();
}

std::string matrix3x3_str(const std::vector<double>& m9) {
  std::ostringstream ss;
  ss << "Matrix3x3{";
  for (std::size_t i = 0; i < 9; ++i) {
    ss << round4(i < m9.size() ? m9[i] : (i % 4 == 0 ? 1.0 : 0.0));
    if (i != 8) ss << ", ";
  }
  ss << "}";
  return ss.str();
}

std::string cpp_string(const std::string& s) {
  std::ostringstream ss;
  ss << '"';
  for (char c : s) {
    switch (c) {
      case '"':
        ss << "\\\"";
        break;
      case '\\':
        ss << "\\\\";
        break;
      case '\n':
        ss << "\\n";
        break;
      case '\r':
        ss << "\\r";
        break;
      case '\t':
        ss << "\\t";
        break;
      default:
        ss << c;
    }
  }
  ss << '"';
  return ss.str();
}

// Splits `s` into adjacent-string-literal chunks small enough for MSVC's ~16380-char single
// string literal limit (a real texture's base64 blob routinely exceeds it; C++ concatenates
// adjacent literals at compile time regardless of how many source lines they span).
std::vector<std::string> cpp_string_literal_lines(const std::string& s) {
  constexpr std::size_t kChunk = 16000;
  std::vector<std::string> out;
  if (s.empty()) {
    out.push_back(cpp_string(s));
    return out;
  }
  for (std::size_t i = 0; i < s.size(); i += kChunk) out.push_back(cpp_string(s.substr(i, kChunk)));
  return out;
}

}  // namespace

std::string to_cpp_code(const SkpModel& model) {
  std::vector<std::string> lines;
  auto push = [&lines](const std::string& s) { lines.push_back(s); };

  auto materials_by_id = model.materials_by_id();
  std::map<std::string, std::string> mat_var;
  std::set<std::string> textured_mats;

  bool has_textured_material = false;
  for (const auto& mat : model.materials) {
    if (mat.texture && mat.texture->data && !mat.texture->data->empty()) {
      has_textured_material = true;
      break;
    }
  }

  push("#include <fstream>");
  push("#include <cstdio>");
  push("#include <openskp/openskp.hpp>");
  push("");
  push("using namespace openskp;");
  push("");
  if (has_textured_material) {
    // Forward-declared here since their full definitions are appended AFTER build() below (they
    // need textured_mats, only known once the materials loop below has run) - build() calls them
    // before that point in the file, so a plain function-call-before-definition won't compile.
    push("ByteBuffer openskp_codegen_base64_decode(const std::string& s);");
    push(
        "std::string openskp_codegen_write_temp_file(const ByteBuffer& bytes, const std::string& "
        "suffix);");
    push("");
  }
  push("ByteBuffer build() {");
  push("  auto builder = create();");
  push("");
  push("  // --- Materials (" + std::to_string(model.materials.size()) + ") ---");
  {
    int i = 0;
    for (const auto& mat : model.materials) {
      std::string var_name = "mat" + std::to_string(i);
      mat_var[mat.name] = var_name;
      if (mat.texture && mat.texture->data && !mat.texture->data->empty()) {
        textured_mats.insert(mat.name);
        std::string b64 = to_base64(*mat.texture->data);
        std::string suffix = ".png";
        auto dot = mat.texture->filename.find_last_of('.');
        if (dot != std::string::npos) suffix = mat.texture->filename.substr(dot);
        if (suffix.empty()) suffix = ".png";
        // The real applied size: the generated add_face pins are in tiles and the writer scales
        // them by the material's size, so the material keeps its size and the faces their mapping.
        //
        // var_name is declared here, then only ASSIGNED inside the nested `{ }` block below (its
        // temp-file cleanup needs its own scope) - a fresh declaration in that inner scope would
        // shadow this one and be invisible to the rest of the function.
        push("  int " + var_name + ";");
        push("  {");
        auto b64_lines = cpp_string_literal_lines(b64);
        push("    std::string b64_" + std::to_string(i) + " =");
        for (std::size_t li = 0; li < b64_lines.size(); ++li) {
          bool last = (li + 1 == b64_lines.size());
          push("        " + b64_lines[li] + (last ? ";" : ""));
        }
        push("    ByteBuffer tex_bytes_" + std::to_string(i) +
             " = openskp_codegen_base64_decode(b64_" + std::to_string(i) + ");");
        push("    std::string tex_path_" + std::to_string(i) +
             " = openskp_codegen_write_temp_file(tex_bytes_" + std::to_string(i) + ", " +
             cpp_string(suffix) + ");");
        double app_h = (mat.texture && mat.texture->height > 1e-9) ? mat.texture->height : 1.0;
        double app_w = (mat.texture && mat.texture->width > 1e-9) ? mat.texture->width : 1.0;
        std::ostringstream extra_args_ss;
        extra_args_ss << ", " << round4(app_h) << ", " << round4(app_w);
        if (mat.transparency < 1.0 - 1e-6) {
          extra_args_ss << ", " << round4(mat.transparency);
        }
        push("    " + var_name + " = builder->add_texture_material(" + cpp_string(mat.name) +
             ", tex_path_" + std::to_string(i) + extra_args_ss.str() + ");");
        push("    std::remove(tex_path_" + std::to_string(i) + ".c_str());");
        push("  }");
      } else {
        auto& c = mat.color;
        push("  int " + var_name + " = builder->add_material(" + cpp_string(mat.name) +
             ", Color4{" + std::to_string(c[0]) + ", " + std::to_string(c[1]) + ", " +
             std::to_string(c[2]) + ", " + std::to_string(c[3]) + "});");
      }
      ++i;
    }
  }

  push("");
  push("  // --- Layers (" + std::to_string(model.layers.size()) + ") ---");
  {
    int i = 0;
    for (const auto& layer : model.layers) {
      std::string var_name = "layer" + std::to_string(i);
      std::string opts_var = "layer_opts" + std::to_string(i);
      push("  LayerOptions " + opts_var + ";");
      push("  " + opts_var + ".color = Color4{" + std::to_string(layer.color[0]) + ", " +
           std::to_string(layer.color[1]) + ", " + std::to_string(layer.color[2]) + ", 255};");
      push("  " + opts_var + ".hidden = " + (layer.hidden ? "true" : "false") + ";");
      push("  int " + var_name + " = builder->add_layer(" + cpp_string(layer.name) + ", " +
           opts_var + ");");
      ++i;
    }
  }

  auto uv_triple_str = [&](const std::vector<Point3>& points, const std::optional<Vec3>& normal,
                           const std::optional<std::array<double, 9>>& uv_transform, double tile_w,
                           double tile_h) -> std::optional<std::string> {
    if (!normal || points.size() < 3) return std::nullopt;
    auto sample = non_collinear_triple(points);
    if (!sample) return std::nullopt;
    Point3 n{(*normal)[0], (*normal)[1], (*normal)[2]};
    auto [xr, yr] = face_uv_basis(n);
    std::ostringstream ss;
    ss << "UvCorrespondence{";
    for (std::size_t idx = 0; idx < 3; ++idx) {
      auto [u, v] = compute_face_uv((*sample)[idx], xr, yr, uv_transform, tile_w, tile_h);
      ss << "{" << point_str((*sample)[idx]) << ", std::array<double, 2>{" << round4(u) << ", "
         << round4(v) << "}}";
      if (idx != 2) ss << ", ";
    }
    ss << "}";
    return ss.str();
  };

  // Statement lines assigning `opts_var`'s .material/.back_material/.front_uv/.back_uv fields
  // (the project's own convention - default-construct then field-assign - not C++20 designated
  // initializers; see edit.cpp's replay_face/replay_layers for the same pattern).
  auto material_opts_lines =
      [&](const Face& face, const std::vector<Point3>& points,
          const std::string& opts_var) -> std::pair<std::vector<std::string>, bool> {
    std::vector<std::string> parts;
    bool has_uv = false;
    if (face.material_id) {
      auto it = materials_by_id.find(*face.material_id);
      if (it != materials_by_id.end()) {
        const Material* m = it->second;
        parts.push_back(opts_var + ".material = " + mat_var[m->name] + ";");
        if (textured_mats.count(m->name)) {
          double tw = (m->texture && m->texture->width != 0.0) ? m->texture->width : 1.0;
          double th = (m->texture && m->texture->height != 0.0) ? m->texture->height : 1.0;
          auto triple = uv_triple_str(points, face.normal, face.uv_transform, tw, th);
          if (triple) {
            parts.push_back(opts_var + ".front_uv = " + *triple + ";");
            has_uv = true;
          }
        }
      }
    }
    if (face.back_material_id) {
      auto it = materials_by_id.find(*face.back_material_id);
      if (it != materials_by_id.end()) {
        const Material* m = it->second;
        parts.push_back(opts_var + ".back_material = " + mat_var[m->name] + ";");
        if (textured_mats.count(m->name)) {
          double tw = (m->texture && m->texture->width != 0.0) ? m->texture->width : 1.0;
          double th = (m->texture && m->texture->height != 0.0) ? m->texture->height : 1.0;
          auto triple = uv_triple_str(points, face.normal, face.uv_transform_back, tw, th);
          if (triple) {
            parts.push_back(opts_var + ".back_uv = " + *triple + ";");
            has_uv = true;
          }
        }
      }
    }
    return {parts, has_uv};
  };

  int faces_skipped_degenerate = 0;
  int face_opts_counter = 0;

  // `member_op` is "->" when target_var is `builder` (a smart pointer) and "." when it's a
  // ComponentDefinitionBuilder variable (add_component_definition returns a reference, not a
  // pointer - unlike builder itself).
  auto emit_faces = [&](const Definition& defn, const std::string& target_var,
                        const std::string& member_op, const std::string& indent) {
    EdgeMap edges = build_edge_map(defn);
    for (const auto& [fid, face] : defn.faces) {
      if (face.loops.empty()) continue;
      auto points_opt = loop_points(face.loops[0], edges, defn);
      if (!points_opt) {
        ++faces_skipped_degenerate;
        continue;
      }
      const auto& points = *points_opt;

      std::vector<std::vector<Point3>> holes;
      for (std::size_t hi = 1; hi < face.loops.size(); ++hi) {
        auto hole_points = loop_points(face.loops[hi], edges, defn);
        if (hole_points) holes.push_back(*hole_points);
      }

      std::string opts_var = "face_opts" + std::to_string(face_opts_counter++);
      auto [opt_lines, has_uv] = material_opts_lines(face, points, opts_var);
      std::string points_str;
      for (std::size_t i = 0; i < points.size(); ++i) {
        points_str += point_str(points[i]);
        if (i + 1 != points.size()) points_str += ", ";
      }

      std::vector<std::string> extra;
      // auto_triangulate: true - mirrors real SketchUp's own tolerance for a not-quite-flat
      // polygon; incompatible with front_uv/back_uv, so only added when this face has neither.
      // Harmless alongside holes - the writer takes the direct (non-triangulated) path whenever
      // holes are present either way.
      if (!has_uv) extra.push_back(opts_var + ".auto_triangulate = true;");
      if (!holes.empty()) {
        std::string holes_str = "{";
        for (std::size_t hi = 0; hi < holes.size(); ++hi) {
          holes_str += "{";
          for (std::size_t pi = 0; pi < holes[hi].size(); ++pi) {
            holes_str += point_str(holes[hi][pi]);
            if (pi + 1 != holes[hi].size()) holes_str += ", ";
          }
          holes_str += "}";
          if (hi + 1 != holes.size()) holes_str += ", ";
        }
        holes_str += "}";
        extra.push_back(opts_var + ".holes = " + holes_str + ";");
      }

      if (opt_lines.empty() && extra.empty()) {
        push(indent + target_var + member_op + "add_face({" + points_str + "});");
      } else {
        push(indent + "FaceOptions " + opts_var + ";");
        for (const auto& l : opt_lines) push(indent + l);
        for (const auto& l : extra) push(indent + l);
        push(indent + target_var + member_op + "add_face({" + points_str + "}, " + opts_var + ");");
      }
    }
  };

  // Statement lines assigning `opts_var`'s .material/.name fields.
  auto instance_opts_lines = [&](const Instance& inst, const std::string& def_name,
                                 const std::string& opts_var) {
    std::vector<std::string> parts;
    if (inst.material_id) {
      auto it = materials_by_id.find(*inst.material_id);
      if (it != materials_by_id.end())
        parts.push_back(opts_var + ".material = " + mat_var[it->second->name] + ";");
    }
    // Explicit even when inst.name is empty: add_instance defaults an OMITTED name to the
    // definition's own name (options.name.value_or(definition.name())), so a source instance
    // with a genuinely empty name would otherwise come out with that name baked in for real.
    if (inst.name != def_name) parts.push_back(opts_var + ".name = " + cpp_string(inst.name) + ";");
    return parts;
  };

  std::map<EntityId, std::string> def_var;
  int def_counter = 0;
  int inst_opts_counter = 0;

  std::function<std::optional<std::string>(EntityId, std::set<EntityId>&)> get_or_build_def =
      [&](EntityId def_id, std::set<EntityId>& visiting) -> std::optional<std::string> {
    auto existing = def_var.find(def_id);
    if (existing != def_var.end()) return existing->second;
    if (visiting.count(def_id)) return std::nullopt;
    visiting.insert(def_id);

    auto it = model.definitions.find(def_id);
    if (it == model.definitions.end()) return std::nullopt;
    const Definition& defn = it->second;
    if (defn.faces.empty() && defn.instances.empty()) return std::nullopt;

    for (const auto& inst : defn.instances) {
      if (inst.ref_idx) get_or_build_def(*inst.ref_idx, visiting);
    }

    std::string var_name = "def" + std::to_string(def_counter++);
    // defn.name unconditionally, not `defn.name.empty() ? ... :
    // defn.name` - an explicit empty string is a real, valid definition
    // name, and this same value also feeds instance_opts_lines's
    // comparison below, which needs the TRUE definition name to
    // correctly decide whether an instance's own name differs from it -
    // a fabricated fallback here would corrupt that comparison, not just
    // the written name. var_name (the emitted identifier, e.g. "def0")
    // is unrelated and always safe.
    std::string def_name = defn.name;
    def_var[def_id] = var_name;

    push("");
    push("  // " + defn.name + " - " + std::to_string(defn.faces.size()) + " faces, " +
         std::to_string(defn.instances.size()) + " nested instances");
    push("  auto& " + var_name + " = builder->add_component_definition(" + cpp_string(def_name) +
         ");");
    emit_faces(defn, var_name, ".", "  ");
    for (const auto& inst : defn.instances) {
      if (!inst.ref_idx) continue;
      auto child_it = def_var.find(*inst.ref_idx);
      if (child_it == def_var.end()) continue;
      std::vector<double> m9(9, 0.0), t(3, 0.0);
      if (inst.matrix.size() >= 9)
        for (int k = 0; k < 9; ++k)
          m9[static_cast<std::size_t>(k)] = inst.matrix[static_cast<std::size_t>(k)];
      if (inst.matrix.size() >= 12)
        for (int k = 0; k < 3; ++k)
          t[static_cast<std::size_t>(k)] = inst.matrix[9 + static_cast<std::size_t>(k)];
      std::string opts_var = "inst_opts" + std::to_string(inst_opts_counter++);
      push("  InstanceOptions " + opts_var + ";");
      push("  " + opts_var + ".translation = Point3{" + std::to_string(round4(t[0])) + ", " +
           std::to_string(round4(t[1])) + ", " + std::to_string(round4(t[2])) + "};");
      push("  " + opts_var + ".matrix3x3 = " + matrix3x3_str(m9) + ";");
      for (const auto& l : instance_opts_lines(inst, def_name, opts_var)) push("  " + l);
      push("  " + var_name + ".add_instance(" + child_it->second + ", " + opts_var + ");");
    }
    push("  " + var_name + ".close();");
    return var_name;
  };

  for (const auto& [def_id, _] : model.definitions) {
    std::set<EntityId> visiting;
    get_or_build_def(def_id, visiting);
  }

  push("");
  push("  // --- Root instances (" + std::to_string(model.root().instances.size()) + ") ---");
  for (const auto& inst : model.root().instances) {
    if (!inst.ref_idx) continue;
    auto child_it = def_var.find(*inst.ref_idx);
    if (child_it == def_var.end()) continue;
    std::string child_def_name;
    auto def_it = model.definitions.find(*inst.ref_idx);
    if (def_it != model.definitions.end()) child_def_name = def_it->second.name;
    std::vector<double> m9(9, 0.0), t(3, 0.0);
    if (inst.matrix.size() >= 9)
      for (int k = 0; k < 9; ++k)
        m9[static_cast<std::size_t>(k)] = inst.matrix[static_cast<std::size_t>(k)];
    if (inst.matrix.size() >= 12)
      for (int k = 0; k < 3; ++k)
        t[static_cast<std::size_t>(k)] = inst.matrix[9 + static_cast<std::size_t>(k)];
    std::string opts_var = "inst_opts" + std::to_string(inst_opts_counter++);
    push("  InstanceOptions " + opts_var + ";");
    push("  " + opts_var + ".translation = Point3{" + std::to_string(round4(t[0])) + ", " +
         std::to_string(round4(t[1])) + ", " + std::to_string(round4(t[2])) + "};");
    push("  " + opts_var + ".matrix3x3 = " + matrix3x3_str(m9) + ";");
    for (const auto& l : instance_opts_lines(inst, child_def_name, opts_var)) push("  " + l);
    push("  builder->add_instance(" + child_it->second + ", " + opts_var + ");");
  }
  emit_faces(model.root(), "builder", "->", "  ");

  push("");
  push("  return builder->to_bytes();");
  push("}");

  if (!textured_mats.empty()) {
    push("");
    push("ByteBuffer openskp_codegen_base64_decode(const std::string& s) {");
    push("  static const std::string chars =");
    push("      \"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\";");
    push("  ByteBuffer out;");
    push("  int val = 0, bits = -8;");
    push("  for (unsigned char c : s) {");
    push("    if (c == '=') break;");
    push("    auto pos = chars.find(static_cast<char>(c));");
    push("    if (pos == std::string::npos) continue;");
    push("    val = (val << 6) + static_cast<int>(pos);");
    push("    bits += 6;");
    push("    if (bits >= 0) {");
    push("      out.push_back(static_cast<std::uint8_t>((val >> bits) & 0xFF));");
    push("      bits -= 8;");
    push("    }");
    push("  }");
    push("  return out;");
    push("}");
    push("");
    push(
        "std::string openskp_codegen_write_temp_file(const ByteBuffer& bytes, const std::string& "
        "suffix) {");
    push("  std::string path = std::tmpnam(nullptr);");
    push("  path += suffix;");
    push("  std::ofstream f(path, std::ios::binary);");
    push(
        "  f.write(reinterpret_cast<const char*>(bytes.data()), "
        "static_cast<std::streamsize>(bytes.size()));");
    push("  return path;");
    push("}");
  }

  if (faces_skipped_degenerate > 0) {
    lines.insert(lines.begin(), "// " + std::to_string(faces_skipped_degenerate) +
                                    " degenerate face(s) (fewer than 3 resolvable vertices) were "
                                    "skipped during generation.");
  }

  std::string result;
  for (const auto& l : lines) {
    result += l;
    result += "\n";
  }
  return result;
}

}  // namespace openskp
