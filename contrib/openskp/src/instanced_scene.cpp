#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <regex>
#include <set>
#include <sstream>
#include <utility>

#include "face_groups.hpp"
#include "internal.hpp"

namespace openskp {
namespace {

// Matches SketchUp's own auto-generated placeholder definition names
// ("Group#1", "Component#12"), which carry no more meaning than the
// internal index they'd otherwise fall back to - mirrors Python's
// _is_generic_definition_name() exactly (same pattern, same purpose).
bool is_generic_definition_name(const std::string& name) {
  static const std::regex kPattern(R"(^(?:Group|Component)\d*#\d+$)");
  return std::regex_match(name, kPattern);
}

constexpr double kInchesToMm = 25.4;
constexpr double kInchesToM = 0.0254;

constexpr std::array<double, 16> kIdentityGltf{
    1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
};

// Convert one instance's 13-element SketchUp matrix (inches, Z-up) into a
// 16-element column-major glTF matrix (metres, Y-up).
//
// The axis change is the similarity transform C * M * C^-1 with
// C: (x, y, z) -> (x, z, -y), so it composes correctly through nesting:
// converting each level and multiplying gives the same result as
// converting the fully-composed SketchUp matrix. Translation is scaled to
// metres; the rotation/scale block is unitless and is not.
std::array<double, 16> to_gltf_matrix(const std::vector<double>& m) {
  double a = m[0], b = m[1], c = m[2];
  double d = m[3], e = m[4], f = m[5];
  double g = m[6], h = m[7], i = m[8];
  double tx = m.size() > 9 ? m[9] : 0.0;
  double ty = m.size() > 10 ? m[10] : 0.0;
  double tz = m.size() > 11 ? m[11] : 0.0;

  double r00 = a, r01 = c, r02 = -b;
  double r10 = g, r11 = i, r12 = -h;
  double r20 = -d, r21 = -f, r22 = e;

  return {
      r00,
      r10,
      r20,
      0,
      r01,
      r11,
      r21,
      0,
      r02,
      r12,
      r22,
      0,
      tx * kInchesToM,
      tz * kInchesToM,
      -ty * kInchesToM,
      1,
  };
}

// Multiply two 16-element column-major matrices (out = a * b).
std::array<double, 16> mul4(const std::array<double, 16>& a, const std::array<double, 16>& b) {
  std::array<double, 16> out{};
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      double s = 0;
      for (int k = 0; k < 4; ++k) s += a[k * 4 + row] * b[col * 4 + k];
      out[col * 4 + row] = s;
    }
  }
  return out;
}

std::string resource_key(std::optional<EntityId> defid, const FaceColorKey& color) {
  std::string key = defid ? std::to_string(*defid) : "ROOT";
  key += '|';
  key += std::to_string(color[0]) + ',' + std::to_string(color[1]) + ',' +
         std::to_string(color[2]) + ',' + std::to_string(color[3]);
  return key;
}

}  // namespace

InstancedScene build_instanced_scene_raw(RawParsed&& p, const ParseOptions& o) {
  InstancedScene scene;
  emit_log(o, LogLevel::information,
           "Building instanced scene: " + std::to_string(p.definitions.size()) +
               " definitions available");
  std::size_t instance_counter = 0;
  std::set<EntityId> active;

  // Textures deduplicated by bytes, exactly as the baked path does.
  std::map<std::string, std::size_t> texture_index_by_key;
  auto texture_index_for =
      [&](const std::shared_ptr<RawMaterial>& mat) -> std::optional<std::size_t> {
    if (!mat || !mat->texture || !mat->texture->data || mat->texture->data->empty()) {
      return std::nullopt;
    }
    const auto& data = *mat->texture->data;
    auto mime_type = sniff_image_mime(data);
    if (!mime_type) return std::nullopt;
    std::ostringstream key_stream;
    key_stream << data.size() << ':';
    for (std::size_t i = 0; i < data.size() && i < 16; ++i) {
      key_stream << std::hex << static_cast<int>(data[i]);
    }
    const auto key = key_stream.str();
    auto found = texture_index_by_key.find(key);
    if (found != texture_index_by_key.end()) return found->second;
    const auto idx = scene.textures.size();
    scene.textures.push_back(SceneTexture{data, *mime_type, mat->texture->filename});
    texture_index_by_key.emplace(key, idx);
    return idx;
  };

  // Materials deduped globally (same key that groups faces, matching
  // scene.cpp's own reuse of GroupKey for both purposes).
  std::map<GroupKey, std::size_t> material_index_by_key;

  auto get_material_index = [&](const GroupKey& key) -> std::size_t {
    auto found = material_index_by_key.find(key);
    if (found != material_index_by_key.end()) return found->second;
    const auto idx = scene.gltf_materials.size();
    GltfMaterial gm;
    gm.pbr_metallic_roughness.base_color_factor = {key.color[0] / 255., key.color[1] / 255.,
                                                   key.color[2] / 255., key.color[3] / 255.};
    gm.pbr_metallic_roughness.base_color_texture = key.texture_index;
    gm.double_sided = key.double_sided;
    scene.gltf_materials.push_back(gm);
    material_index_by_key.emplace(key, idx);
    return idx;
  };

  std::map<std::string, std::size_t> resource_index_by_key;

  // Identity of a mesh resource: (definition, effective fallback color) -
  // the ONLY inputs that can change what build_local_face_groups produces
  // for this definition, since (faithfully to the baked path this was
  // extracted from - see face_groups.hpp's own docs) it resolves each
  // face's material from the face's OWN material id only, never from an
  // instance's painted material. Caching on the definition id alone would
  // still be wrong: the same definition renders a different fallback
  // color depending on the layer/paint context it's placed in, and
  // merging those would silently repaint geometry.
  std::function<std::optional<std::string>(const GeometryBuilder&, const std::string&,
                                           std::optional<EntityId>, std::optional<FaceColorKey>,
                                           const std::string&)>
      mesh_resource_for_builder;
  mesh_resource_for_builder = [&](const GeometryBuilder& builder, const std::string& defname,
                                  std::optional<EntityId> defid,
                                  std::optional<FaceColorKey> inherited,
                                  const std::string& layer) -> std::optional<std::string> {
    if (builder.faces.empty()) return std::nullopt;

    const auto fallback = inherited.value_or(default_color(p, layer));
    const auto key = resource_key(defid, fallback);
    auto hit = resource_index_by_key.find(key);
    if (hit != resource_index_by_key.end()) {
      return scene.mesh_resources[hit->second].id;
    }

    auto groups =
        build_local_face_groups(p, builder, FaceGroupContext{texture_index_for, fallback});

    std::vector<LocalPrimitive> primitives;
    for (auto& kv : groups) {
      auto& g = kv.second;
      if (g.tris.empty()) continue;

      LocalPrimitive prim;
      prim.positions.resize(g.verts.size() * 3);
      prim.normals.resize(g.verts.size() * 3);
      prim.uvs.resize(g.verts.size() * 2);

      for (auto& m : g.map) {
        const auto i = m.second;
        const auto& v = g.verts[i];
        // Local space, so no instance matrix is applied - only the
        // inches->metres scale and SketchUp Z-up -> glTF Y-up axis swap,
        // the same fixed conventions the baked path applies.
        prim.positions[i * 3] = float(v[0] * kInchesToM);
        prim.positions[i * 3 + 1] = float(v[2] * kInchesToM);
        prim.positions[i * 3 + 2] = float(-v[1] * kInchesToM);
        prim.uvs[i * 2] = g.uvs[i][0];
        prim.uvs[i * 2 + 1] = g.uvs[i][1];

        auto n = g.normals[m.first];
        double l = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (l < 1e-6) {
          n = {0, 0, 1};
        } else {
          for (auto& x : n) x /= l;
        }
        // Same axis swap as positions. No instance-matrix normal
        // transform here: that belongs to the node, and deferring it is
        // precisely what keeps mirrored/non-uniform scales correct per
        // placement.
        prim.normals[i * 3] = float(n[0]);
        prim.normals[i * 3 + 1] = float(n[2]);
        prim.normals[i * 3 + 2] = float(-n[1]);
      }

      prim.indices.reserve(g.tris.size() * 3);
      for (auto& tri : g.tris) {
        prim.indices.push_back(static_cast<std::uint32_t>(tri[0]));
        prim.indices.push_back(static_cast<std::uint32_t>(tri[1]));
        prim.indices.push_back(static_cast<std::uint32_t>(tri[2]));
      }

      prim.material_index = get_material_index(kv.first);
      primitives.push_back(std::move(prim));
    }

    if (primitives.empty()) return std::nullopt;

    InstancedMeshResource resource;
    resource.id = "mesh_" + std::to_string(scene.mesh_resources.size());
    resource.definition_id = defid;
    resource.definition_name = defname;
    resource.variant_key = key;
    resource.primitives = std::move(primitives);
    const auto index = scene.mesh_resources.size();
    resource_index_by_key.emplace(key, index);
    const auto id = resource.id;
    scene.mesh_resources.push_back(std::move(resource));
    return id;
  };

  // Walk a definition's placed instances, emitting one node each.
  // current_matrix is the accumulated SketchUp-space matrix and is used
  // ONLY to report each node's absolute position_mm (matching the baked
  // path's metadata); the geometry itself never sees it.
  std::function<std::vector<InstancedNode>(const GeometryBuilder&, std::optional<EntityId>,
                                           const std::vector<double>&, const std::string&,
                                           std::optional<FaceColorKey>)>
      walk;
  walk = [&](const GeometryBuilder& builder, std::optional<EntityId> defid,
             const std::vector<double>& current_matrix, const std::string& parent_layer,
             std::optional<FaceColorKey> inherited) {
    std::vector<InstancedNode> nodes;
    for (auto& i : builder.instances) {
      const auto new_matrix = multiply_matrices(current_matrix, i.matrix);

      std::string l_name = parent_layer;
      if (!i.layer.empty()) {
        try {
          auto li = p.layer_id_to_name.find(std::stoll(i.layer));
          if (li != p.layer_id_to_name.end()) l_name = li->second;
        } catch (...) {
          l_name = i.layer;
          emit_log(o, LogLevel::debug, "Failed to resolve layer id '" + i.layer + "' to a name");
        }
      }

      auto inst_color = inherited;
      if (auto color = material_color(find_material(p, i.material_id))) inst_color = color;

      // def_name is looked up before building the node, since the name-
      // resolution fallback chain below needs it - same order as Python's
      // instanced_scene.py.
      std::string def_name;
      if (i.ref_idx) {
        auto dd = p.definitions.find(*i.ref_idx);
        if (dd != p.definitions.end()) def_name = dd->second.name;
      }
      const bool def_name_is_real = !def_name.empty() && !is_generic_definition_name(def_name);

      // Same fallback order as openskp.instanced_scene: attribute-dict
      // override (any OTHER dictionary the instance carries, whichever
      // plugin wrote it - "name"/"label"/"code", first dictionary and
      // first key found wins), then the instance's own name, then the
      // definition's own name if it's not itself an auto-generated
      // "Group#1"-style placeholder, then finally the internal index.
      std::optional<std::string> name_override;
      for (auto& [dict_name, entries] : i.attribute_dicts) {
        for (const char* key : {"name", "label", "code"}) {
          auto it = entries.find(key);
          if (it != entries.end() && !it->second.empty()) {
            name_override = it->second;
            break;
          }
        }
        if (name_override) break;
      }

      const std::string inst_name = !i.name.empty() ? i.name
                                    : def_name_is_real
                                        ? def_name
                                        : ("Component_" + std::to_string(i.ref_idx.value_or(0)));
      const std::string display_name = name_override.value_or(inst_name);
      const bool name_is_generated = !(name_override || !i.name.empty() || def_name_is_real);

      InstancedNode node;
      node.name = display_name;
      node.name_is_generated = name_is_generated;
      node.guid = i.ref_guid;
      node.layer = l_name;
      node.matrix = to_gltf_matrix(i.matrix);
      node.position_mm = {new_matrix.size() > 9 ? new_matrix[9] * kInchesToMm : 0,
                          new_matrix.size() > 10 ? new_matrix[10] * kInchesToMm : 0,
                          new_matrix.size() > 11 ? new_matrix[11] * kInchesToMm : 0};
      node.properties = i.properties;
      node.attribute_dictionaries = i.attribute_dicts;

      if (i.ref_idx) {
        if (active.count(*i.ref_idx)) {
          throw SkpParseError("Recursive component definition", ParseStage::build_scene, {}, {}, {},
                              {}, *i.ref_idx);
        }
        auto d = p.definitions.find(*i.ref_idx);
        if (d != p.definitions.end()) {
          active.insert(*i.ref_idx);
          node.definition_name = d->second.name;
          node.mesh_resource_id = mesh_resource_for_builder(d->second.builder, d->second.name,
                                                            *i.ref_idx, inst_color, l_name);
          node.children = walk(d->second.builder, *i.ref_idx, new_matrix, l_name, inst_color);
          active.erase(*i.ref_idx);
        }
      }

      nodes.push_back(std::move(node));
      if (++instance_counter % progress_interval == 0)
        emit_progress(o, ParseStage::build_scene, instance_counter, instance_counter);
    }
    return nodes;
  };

  std::vector<double> identity{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};
  auto root_children = walk(p.root.builder, std::nullopt, identity, "Layer0", std::nullopt);

  // Loose geometry drawn straight into the model (not inside any
  // component/group) is kept, as the baked path keeps it: it becomes the
  // root node's own mesh resource.
  auto root_mesh_resource_id =
      mesh_resource_for_builder(p.root.builder, "ROOT_MODEL", std::nullopt, std::nullopt, "Layer0");

  scene.layer_hidden = p.layer_hidden;

  scene.scene_hierarchy = InstancedNode{};
  scene.scene_hierarchy.name = "ROOT";
  scene.scene_hierarchy.definition_name = "ROOT_MODEL";
  scene.scene_hierarchy.layer = "Layer0";
  scene.scene_hierarchy.matrix = kIdentityGltf;
  scene.scene_hierarchy.mesh_resource_id = root_mesh_resource_id;
  scene.scene_hierarchy.children = std::move(root_children);

  // Bounds of the scene AS PLACED: walk the tree, transform each
  // resource's local corners by the accumulated node matrix. Only the 8
  // corners of each resource's local box are transformed rather than
  // every vertex - an affine transform maps a box's corners to the
  // corners of the transformed box, so the result is exact for the
  // axis-aligned bounds, at a fraction of the cost.
  std::map<std::string, std::size_t> resource_index_by_id;
  for (std::size_t idx = 0; idx < scene.mesh_resources.size(); ++idx) {
    resource_index_by_id[scene.mesh_resources[idx].id] = idx;
  }
  std::map<std::string, std::optional<std::pair<std::array<double, 3>, std::array<double, 3>>>>
      local_box_cache;

  std::function<std::optional<std::pair<std::array<double, 3>, std::array<double, 3>>>(
      const std::string&)>
      local_box;
  local_box = [&](const std::string& resource_id)
      -> std::optional<std::pair<std::array<double, 3>, std::array<double, 3>>> {
    auto cached = local_box_cache.find(resource_id);
    if (cached != local_box_cache.end()) return cached->second;

    std::optional<std::pair<std::array<double, 3>, std::array<double, 3>>> box;
    auto found = resource_index_by_id.find(resource_id);
    if (found != resource_index_by_id.end()) {
      const auto& res = scene.mesh_resources[found->second];
      std::array<double, 3> lo{std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::infinity()};
      std::array<double, 3> hi{-std::numeric_limits<double>::infinity(),
                               -std::numeric_limits<double>::infinity(),
                               -std::numeric_limits<double>::infinity()};
      for (auto& prim : res.primitives) {
        for (std::size_t i = 0; i + 2 < prim.positions.size(); i += 3) {
          for (int k = 0; k < 3; ++k) {
            const double v = prim.positions[i + static_cast<std::size_t>(k)];
            if (v < lo[static_cast<std::size_t>(k)]) lo[static_cast<std::size_t>(k)] = v;
            if (v > hi[static_cast<std::size_t>(k)]) hi[static_cast<std::size_t>(k)] = v;
          }
        }
      }
      if (!std::isinf(lo[0])) box = std::make_pair(lo, hi);
    }
    local_box_cache.emplace(resource_id, box);
    return box;
  };

  std::array<double, 3> b_min{std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::infinity()};
  std::array<double, 3> b_max{-std::numeric_limits<double>::infinity(),
                              -std::numeric_limits<double>::infinity(),
                              -std::numeric_limits<double>::infinity()};

  std::function<void(const InstancedNode&, const std::array<double, 16>&)> accumulate;
  accumulate = [&](const InstancedNode& node, const std::array<double, 16>& parent) {
    const auto world = mul4(parent, node.matrix);
    if (node.mesh_resource_id) {
      auto box = local_box(*node.mesh_resource_id);
      if (box) {
        const auto& [lo, hi] = *box;
        for (int c = 0; c < 8; ++c) {
          const double x = (c & 1) ? hi[0] : lo[0];
          const double y = (c & 2) ? hi[1] : lo[1];
          const double z = (c & 4) ? hi[2] : lo[2];
          const double wx = world[0] * x + world[4] * y + world[8] * z + world[12];
          const double wy = world[1] * x + world[5] * y + world[9] * z + world[13];
          const double wz = world[2] * x + world[6] * y + world[10] * z + world[14];
          if (wx < b_min[0]) b_min[0] = wx;
          if (wy < b_min[1]) b_min[1] = wy;
          if (wz < b_min[2]) b_min[2] = wz;
          if (wx > b_max[0]) b_max[0] = wx;
          if (wy > b_max[1]) b_max[1] = wy;
          if (wz > b_max[2]) b_max[2] = wz;
        }
      }
    }
    for (auto& child : node.children) accumulate(child, world);
  };
  accumulate(scene.scene_hierarchy, kIdentityGltf);

  if (!std::isinf(b_min[0])) {
    SceneBounds bounds;
    bounds.min = b_min;
    bounds.max = b_max;
    bounds.size = {b_max[0] - b_min[0], b_max[1] - b_min[1], b_max[2] - b_min[2]};
    bounds.center = {(b_min[0] + b_max[0]) / 2, (b_min[1] + b_max[1]) / 2,
                     (b_min[2] + b_max[2]) / 2};
    scene.bounds = bounds;
  }

  emit_log(o, LogLevel::information,
           "Instanced scene build complete: " + std::to_string(instance_counter) + " instances, " +
               std::to_string(scene.mesh_resources.size()) + " mesh resources");
  return scene;
}

}  // namespace openskp
