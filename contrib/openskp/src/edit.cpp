// C++ port of packages/python/src/openskp/edit.py - see the module docstring at the top of
// include/openskp/edit.hpp for the overall approach and the exact, deliberately-scoped fidelity
// gaps this replay carries over from the Python port.

#include <cmath>
#include <fstream>
#include <functional>
#include <iterator>
#include <random>
#include <set>
#include <sstream>
#include <system_error>

#include <openskp/edit.hpp>
#include <openskp/parser.hpp>

#include "internal.hpp"  // is_legacy - not part of the public API, see that header's own comment

namespace openskp {
namespace {

// ---------------------------------------------------------------------------------------------
// Loop/edge reconstruction - mirrors packages/python/src/openskp/scene.py's own
// _reconstruct_loop_vertices exactly (both edit.py and this file build on the same algorithm the
// glTF scene baker already uses to turn a face's CoEdge loop into an ordered vertex list).
// ---------------------------------------------------------------------------------------------

using EdgeMap = std::map<EntityId, std::pair<EntityId, EntityId>>;

EdgeMap build_edge_map(const Definition& defn) {
  EdgeMap m;
  for (const auto& [id, e] : defn.edges) m[id] = {e.v1_id, e.v2_id};
  return m;
}

std::vector<EntityId> reconstruct_loop_vertices(const std::vector<CoEdge>& loop,
                                                const EdgeMap& edges) {
  std::vector<EntityId> loop_verts;
  for (const auto& c : loop) {
    auto it = edges.find(c.edge_id);
    if (it == edges.end()) continue;
    EntityId v_start = c.orientation == 1 ? it->second.first : it->second.second;
    if (loop_verts.empty() || loop_verts.back() != v_start) loop_verts.push_back(v_start);
  }
  if (loop_verts.size() > 1 && loop_verts.front() == loop_verts.back()) loop_verts.pop_back();
  return loop_verts;
}

// ---------------------------------------------------------------------------------------------
// UV replay helpers - mirrors packages/python/src/openskp/scene.py's _face_uv_basis /
// _compute_face_uv (the same formulas src/scene.cpp already uses for glTF baking; duplicated
// locally here rather than reused, since scene.cpp's copies have internal linkage).
// ---------------------------------------------------------------------------------------------

std::array<double, 9> invert_3x3(const std::array<double, 9>& m) {
  double a = m[0], b = m[1], c = m[2];
  double d = m[3], e = m[4], f = m[5];
  double g = m[6], h = m[7], i = m[8];
  double det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
  if (std::abs(det) < 1e-12) return {1, 0, 0, 0, 1, 0, 0, 0, 1};
  double inv_det = 1.0 / det;
  return {
      (e * i - f * h) * inv_det, (c * h - b * i) * inv_det, (b * f - c * e) * inv_det,
      (f * g - d * i) * inv_det, (a * i - c * g) * inv_det, (c * d - a * f) * inv_det,
      (d * h - e * g) * inv_det, (b * g - a * h) * inv_det, (a * e - b * d) * inv_det,
  };
}

std::pair<Point3, Point3> face_uv_basis(Point3 n) {
  double cx = -n[1], cy = n[0];
  double clen = std::sqrt(cx * cx + cy * cy);
  if (clen < 1e-9) return {Point3{1.0, 0.0, 0.0}, Point3{0.0, n[2] >= 0 ? 1.0 : -1.0, 0.0}};
  Point3 xr{cx / clen, cy / clen, 0.0};
  Point3 yr{n[1] * xr[2] - n[2] * xr[1], n[2] * xr[0] - n[0] * xr[2], n[0] * xr[1] - n[1] * xr[0]};
  return {xr, yr};
}

std::pair<double, double> compute_face_uv(Point3 p, Point3 xr, Point3 yr,
                                          const std::optional<std::array<double, 9>>& uv_transform,
                                          double tile_w, double tile_h) {
  double px = p[0] * xr[0] + p[1] * xr[1] + p[2] * xr[2];
  double py = p[0] * yr[0] + p[1] * yr[1] + p[2] * yr[2];
  if (!uv_transform) return {px / tile_w, py / tile_h};
  auto inv = invert_3x3(*uv_transform);
  double u = px * inv[0] + py * inv[3] + inv[6];
  double v = px * inv[1] + py * inv[4] + inv[7];
  double q = px * inv[2] + py * inv[5] + inv[8];
  if (std::abs(q) < 1e-12) q = 1.0;
  return {(u / q) / tile_w, (v / q) / tile_h};
}

// ---------------------------------------------------------------------------------------------
// Material/layer replay.
// ---------------------------------------------------------------------------------------------

std::map<const Material*, int> replay_materials(SkpBuilder& builder, const SkpModel& model,
                                                std::vector<std::string>& warnings) {
  std::map<const Material*, int> slots;
  std::mt19937_64 rng{std::random_device{}()};
  for (const auto& mat : model.materials) {
    int slot;
    if (mat.texture && mat.texture->data) {
      std::string suffix = ".png";
      auto dot = mat.texture->filename.find_last_of('.');
      if (dot != std::string::npos) suffix = mat.texture->filename.substr(dot);
      if (suffix.empty()) suffix = ".png";
      std::ostringstream name_stream;
      name_stream << "openskp_edit_tex_" << rng() << suffix;
      std::filesystem::path tmp_path = std::filesystem::temp_directory_path() / name_stream.str();
      {
        std::ofstream f(tmp_path, std::ios::binary);
        if (!f) throw SkpWriteError("cannot create temporary texture file: " + tmp_path.string());
        f.write(reinterpret_cast<const char*>(mat.texture->data->data()),
                static_cast<std::streamsize>(mat.texture->data->size()));
      }
      try {
        // applied_height: 1.0 - every textured face is now replayed with
        // an explicit front_uv/back_uv (see replay_uv), whose pins already
        // bake in the SOURCE's real tile size via compute_face_uv - the
        // material's own stored applied height must be a no-op divisor
        // (1.0) or the read-side UV formula divides by it a second time.
        // This happens to match add_texture_material's own default too,
        // but is kept explicit here since it's a hard requirement of this
        // call site specifically, not just a safe default.
        slot = builder.add_texture_material(mat.name, tmp_path, 1.0);
      } catch (...) {
        std::error_code ec;
        std::filesystem::remove(tmp_path, ec);
        throw;
      }
      std::error_code ec;
      std::filesystem::remove(tmp_path, ec);
      if (mat.colorized) {
        warnings.push_back("material '" + mat.name +
                           "': colorized tint not reproduced (base texture only)");
      }
    } else {
      if (mat.texture) {
        warnings.push_back("material '" + mat.name +
                           "': texture image data missing - replayed as solid color");
      }
      slot = builder.add_material(mat.name, mat.color);
    }
    slots[&mat] = slot;
  }
  return slots;
}

std::optional<int> material_slot(std::optional<EntityId> material_id,
                                 const std::map<EntityId, const Material*>& by_id,
                                 const std::map<const Material*, int>& slots) {
  if (!material_id) return std::nullopt;
  auto it = by_id.find(*material_id);
  if (it == by_id.end()) return std::nullopt;
  auto sit = slots.find(it->second);
  if (sit == slots.end()) return std::nullopt;
  return sit->second;
}

std::map<std::string, int> replay_layers(SkpBuilder& builder, const SkpModel& model) {
  std::map<std::string, int> slots;
  for (const auto& layer : model.layers) {
    LayerOptions options;
    options.color = Color4{layer.color[0], layer.color[1], layer.color[2], 255};
    options.hidden = layer.hidden;
    slots[layer.name] = builder.add_layer(layer.name, options);
  }
  return slots;
}

// ---------------------------------------------------------------------------------------------
// Definition ordering - mirrors edit.py's own _definition_order: dependencies (nested-instance
// targets) before dependents, the same ordering constraint ComponentDefinitionBuilder::
// add_instance documents, via a standard DFS post-order topological sort.
// ---------------------------------------------------------------------------------------------

std::vector<EntityId> definition_order(const SkpModel& model) {
  std::set<EntityId> visited, in_progress;
  std::vector<EntityId> order;
  std::function<void(EntityId)> visit = [&](EntityId def_id) {
    if (visited.count(def_id)) return;
    if (in_progress.count(def_id)) {
      throw SkpWriteError("circular component-definition reference involving definition " +
                          std::to_string(def_id));
    }
    in_progress.insert(def_id);
    auto it = model.definitions.find(def_id);
    if (it != model.definitions.end()) {
      for (const auto& inst : it->second.instances) {
        if (inst.ref_idx && model.definitions.count(*inst.ref_idx)) visit(*inst.ref_idx);
      }
    }
    in_progress.erase(def_id);
    visited.insert(def_id);
    order.push_back(def_id);
  };
  for (const auto& [def_id, defn] : model.definitions) {
    (void)defn;
    visit(def_id);
  }
  return order;
}

bool definition_has_content(const Definition& defn,
                            const std::map<EntityId, ComponentDefinitionBuilder*>& def_builders) {
  EdgeMap edges = build_edge_map(defn);
  for (const auto& [id, face] : defn.faces) {
    (void)id;
    if (face.loops.empty()) continue;
    if (reconstruct_loop_vertices(face.loops[0], edges).size() >= 3) return true;
  }
  for (const auto& inst : defn.instances) {
    if (inst.ref_idx && def_builders.count(*inst.ref_idx)) return true;
  }
  return false;
}

// ---------------------------------------------------------------------------------------------
// Body replay (faces + instances) - templated on Target since SkpBuilder and
// ComponentDefinitionBuilder expose the identical add_face/add_instance shape this needs (the
// same duck-typing edit.py relies on for its `target` parameter).
// ---------------------------------------------------------------------------------------------

// front_uv/back_uv need exactly 3 correspondences whose (u, v) values are
// NOT collinear (an affine fit is impossible otherwise) - real faces can
// have a "flat" vertex (three consecutive vertices genuinely collinear in
// 3D), which points[0..2] alone isn't guaranteed to avoid. Search for the
// first non-collinear triple - an affine map preserves collinearity, so a
// non-collinear triple in 3D is also non-collinear in (u, v).
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

// Explicit front_uv/back_uv for EVERY textured face, not just already-
// positioned ones - compute_face_uv already computes the correct final UV
// for the untouched-projection case too (uv_transform is unset) using the
// source's real tile size, so this reproduces a default-projected face's
// true rendering exactly without needing the material's own applied
// width/height to match (which, post-replay, is intentionally 1.0 - see
// replay_materials).
std::optional<UvCorrespondence> replay_uv(std::optional<EntityId> material_id,
                                          const std::optional<std::array<double, 9>>& uv_transform,
                                          bool projected, const std::vector<Point3>& points,
                                          const std::optional<Vec3>& normal,
                                          const std::map<EntityId, const Material*>& by_id,
                                          std::vector<std::string>& warnings,
                                          const std::string& context, const std::string& side) {
  if (!material_id) return std::nullopt;
  auto it = by_id.find(*material_id);
  const Material* mat = it != by_id.end() ? it->second : nullptr;
  if (!mat || !mat->texture) return std::nullopt;  // solid color - no UV to replay
  if (projected) {
    warnings.push_back(context + ": " + side +
                       " texture is projected/draped - falls back to default projection");
    return std::nullopt;
  }
  if (!normal) return std::nullopt;
  double tile_w = mat->texture->width != 0.0 ? mat->texture->width : 1.0;
  double tile_h = mat->texture->height != 0.0 ? mat->texture->height : 1.0;
  Point3 n{(*normal)[0], (*normal)[1], (*normal)[2]};
  auto [xr, yr] = face_uv_basis(n);
  auto sample = non_collinear_triple(points);
  if (!sample) return std::nullopt;  // every vertex triple collinear - a sliver face
  UvCorrespondence pairs;
  for (const auto& p : *sample) {
    auto [u, v] = compute_face_uv(p, xr, yr, uv_transform, tile_w, tile_h);
    pairs.push_back({p, std::array<double, 2>{u, v}});
  }
  return pairs;
}

template <class Target>
void replay_face(Target& target, const Face& face, const Definition& defn, const EdgeMap& edges,
                 const std::map<EntityId, const Material*>& by_id,
                 const std::map<const Material*, int>& material_slots,
                 std::vector<std::string>& warnings, const std::string& context) {
  if (face.loops.empty()) {
    warnings.push_back(context + ": face " + std::to_string(face.id) + " has no loops - skipped");
    return;
  }
  auto vert_ids = reconstruct_loop_vertices(face.loops[0], edges);
  if (vert_ids.size() < 3) {
    warnings.push_back(context + ": face " + std::to_string(face.id) +
                       " has fewer than 3 usable points - skipped");
    return;
  }
  std::vector<Point3> points;
  points.reserve(vert_ids.size());
  for (EntityId vid : vert_ids) {
    auto it = defn.vertices.find(vid);
    if (it == defn.vertices.end()) {
      warnings.push_back(context + ": face " + std::to_string(face.id) +
                         " references a missing vertex - skipped");
      return;
    }
    points.push_back({it->second.x, it->second.y, it->second.z});
  }

  std::vector<std::vector<Point3>> holes;
  for (std::size_t li = 1; li < face.loops.size(); ++li) {
    auto hole_vert_ids = reconstruct_loop_vertices(face.loops[li], edges);
    if (hole_vert_ids.size() < 3) {
      warnings.push_back(context + ": face " + std::to_string(face.id) +
                         " has a hole with fewer than 3 usable points - skipped");
      return;
    }
    std::vector<Point3> hole_points;
    hole_points.reserve(hole_vert_ids.size());
    for (EntityId vid : hole_vert_ids) {
      auto it = defn.vertices.find(vid);
      if (it == defn.vertices.end()) {
        warnings.push_back(context + ": face " + std::to_string(face.id) +
                           " has a hole referencing a missing vertex - skipped");
        return;
      }
      hole_points.push_back({it->second.x, it->second.y, it->second.z});
    }
    holes.push_back(std::move(hole_points));
  }

  bool hidden_edges = false, soft_edges = false, smooth_edges = false;
  for (const auto& coedge : face.loops[0]) {
    auto it = defn.edges.find(coedge.edge_id);
    if (it == defn.edges.end()) continue;
    hidden_edges = hidden_edges || it->second.hidden;
    soft_edges = soft_edges || it->second.soft;
    smooth_edges = smooth_edges || it->second.smooth;
  }

  FaceOptions options;
  options.material = material_slot(face.material_id, by_id, material_slots);
  options.back_material = material_slot(face.back_material_id, by_id, material_slots);
  options.hidden = face.hidden;
  options.soft_edges = soft_edges;
  options.smooth_edges = smooth_edges;
  options.hidden_edges = hidden_edges;
  options.front_uv = replay_uv(face.material_id, face.uv_transform, face.uv_projected, points,
                               face.normal, by_id, warnings, context, "front");
  options.back_uv = replay_uv(face.back_material_id, face.uv_transform_back, face.uv_projected_back,
                              points, face.normal, by_id, warnings, context, "back");
  options.holes = holes;

  try {
    target.add_face(points, options);
  } catch (const SkpWriteError& exc) {
    warnings.push_back(context + ": face " + std::to_string(face.id) + " skipped (" + exc.what() +
                       ")");
  }
}

template <class Target>
void replay_instance(Target& target, const Instance& inst,
                     const std::map<EntityId, ComponentDefinitionBuilder*>& def_builders,
                     const std::map<EntityId, const Material*>& by_id,
                     const std::map<const Material*, int>& material_slots,
                     const std::map<std::string, int>& layer_slots,
                     std::vector<std::string>& warnings, const std::string& context) {
  if (!inst.ref_idx || def_builders.find(*inst.ref_idx) == def_builders.end()) {
    warnings.push_back(context + ": instance '" + inst.name +
                       "' references unavailable definition - skipped");
    return;
  }
  ComponentDefinitionBuilder* def_builder = def_builders.at(*inst.ref_idx);

  // Ground truth (see legacy.cpp's CComponentInstance/CGroup reader): the stored transform is 9
  // f64 (row-major 3x3) + 3 f64 (translation) + a trailing 1.0 that isn't stored here - matching
  // create.hpp's own write_instance_like layout exactly, hence the same [0:9]/[9:12] slicing
  // edit.py's own _replay_instance uses.
  std::optional<Matrix3x3> matrix3x3;
  Point3 translation{0.0, 0.0, 0.0};
  if (inst.matrix.size() >= 9) {
    Matrix3x3 m{};
    for (int i = 0; i < 9; ++i)
      m[static_cast<std::size_t>(i)] = inst.matrix[static_cast<std::size_t>(i)];
    matrix3x3 = m;
  }
  if (inst.matrix.size() >= 12) {
    translation = {inst.matrix[9], inst.matrix[10], inst.matrix[11]};
  }

  InstanceOptions options;
  // options.name = inst.name unconditionally, not `if (!inst.name.empty())
  // ...` - an explicit empty string is a real, valid instance name
  // (SketchUp itself stores it that way when a placement was never
  // renamed, showing the definition's name in the Outliner only as a
  // UI-level fallback); add_instance's own `options.name.value_or(
  // definition.name())` fallback only triggers when options.name has NO
  // value at all, so leaving it unset here for an empty name lets that
  // name get silently replaced with the definition's own name instead.
  options.name = inst.name;
  options.translation = translation;
  options.matrix3x3 = matrix3x3;
  options.material = material_slot(inst.material_id, by_id, material_slots);
  if (!inst.layer.empty()) {
    auto it = layer_slots.find(inst.layer);
    if (it != layer_slots.end()) options.layer = it->second;
  }
  options.hidden = inst.hidden;
  if (!inst.properties.empty()) {
    for (const auto& [key, value] : inst.properties) options.attributes[key] = value;
    options.attribute_dict_name = "dynamic_attributes";
  }

  try {
    target.add_instance(*def_builder, options);
  } catch (const SkpWriteError& exc) {
    warnings.push_back(context + ": instance '" + inst.name + "' skipped (" + exc.what() + ")");
  }
}

template <class Target>
void replay_body(Target& target, const Definition& defn,
                 const std::map<EntityId, const Material*>& by_id,
                 const std::map<const Material*, int>& material_slots,
                 const std::map<std::string, int>& layer_slots, std::vector<std::string>& warnings,
                 const std::string& context,
                 const std::map<EntityId, ComponentDefinitionBuilder*>& def_builders) {
  EdgeMap edges = build_edge_map(defn);
  for (const auto& [id, face] : defn.faces) {
    (void)id;
    replay_face(target, face, defn, edges, by_id, material_slots, warnings, context);
  }
  for (const auto& inst : defn.instances) {
    replay_instance(target, inst, def_builders, by_id, material_slots, layer_slots, warnings,
                    context);
  }
}

}  // namespace

OpenExistingResult open_existing(const std::filesystem::path& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) throw SkpWriteError("cannot open file: " + path.string());
  ByteBuffer data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

  if (!is_legacy(data)) {
    throw SkpWriteError("'" + path.string() +
                        "' is not a legacy-format (SketchUp 2013-2020) .skp file - openskp::create "
                        "only ever writes that format, so only a legacy-format source file can be "
                        "rebuilt through it (see edit.hpp's own docstring for why an arbitrary "
                        "existing file can't simply be patched)");
  }

  // const: materials_by_id() (below) has a non-const overload returning map<EntityId, Material*>
  // and a const one returning map<EntityId, const Material*> - by_id's declared type wants the
  // latter, so model must be const at the call site to select it.
  const SkpModel model = parse_skp(std::move(data));

  OpenExistingResult result;
  result.builder = create();
  SkpBuilder& builder = *result.builder;

  std::map<EntityId, const Material*> by_id = model.materials_by_id();
  std::map<const Material*, int> material_slots = replay_materials(builder, model, result.warnings);
  std::map<std::string, int> layer_slots = replay_layers(builder, model);

  std::map<EntityId, ComponentDefinitionBuilder*> def_builders;
  for (EntityId def_id : definition_order(model)) {
    auto it = model.definitions.find(def_id);
    if (it == model.definitions.end()) continue;
    const Definition& defn = it->second;
    std::string context_name =
        defn.name.empty() ? ("Definition" + std::to_string(def_id)) : defn.name;
    std::string context = "definition '" + context_name + "'";
    if (!definition_has_content(defn, def_builders)) {
      result.warnings.push_back(context + ": skipped (no replayable geometry)");
      continue;
    }
    // defn.name unconditionally, not the context_name fallback above - an
    // explicit empty string is a real, valid definition name. SketchUp
    // Groups are internally just unnamed component definitions (unlike
    // Components, which SketchUp auto-names), so an empty name is common
    // in real files - same reasoning as replay_instance's own name
    // handling below. context_name stays fallback-safe since it's only
    // ever used for a human-readable warning message.
    ComponentDefinitionBuilder& db = builder.add_component_definition(defn.name);
    replay_body(db, defn, by_id, material_slots, layer_slots, result.warnings, context,
                def_builders);
    db.close();
    def_builders[def_id] = &db;
  }

  replay_body(builder, model.root(), by_id, material_slots, layer_slots, result.warnings, "root",
              def_builders);

  for (const auto& [def_id, db] : def_builders) {
    const std::string& n = model.definitions.at(def_id).name;
    if (!n.empty())
      result.definitions[n] = db;  // if two source definitions share a name, the later one wins
  }
  return result;
}

}  // namespace openskp
