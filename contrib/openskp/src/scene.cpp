#include <algorithm>
#include <cmath>
#include <regex>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>

#include "face_groups.hpp"
#include "internal.hpp"

namespace openskp {
namespace {

std::string safe(std::string s) {
  for (size_t p = 0; (p = s.find(" / ", p)) != std::string::npos;) s.replace(p, 3, "__");
  std::replace(s.begin(), s.end(), ' ', '_');
  if (s.size() > 80) s.resize(80);
  return s;
}

// Matches SketchUp's own auto-generated placeholder definition names
// ("Group#1", "Component#12"), which carry no more meaning than the
// internal index they'd otherwise fall back to - mirrors Python's
// _is_generic_definition_name() exactly (same pattern, duplicated rather
// than shared, exactly as openskp.scene.build_scene duplicates it from
// openskp.instanced_scene - see instanced_scene.cpp's identical helper).
bool is_generic_definition_name(const std::string& name) {
  static const std::regex kPattern(R"(^(?:Group|Component)\d*#\d+$)");
  return std::regex_match(name, kPattern);
}
}  // namespace

Scene build_scene_raw(RawParsed&& p, const ParseOptions& o) {
  Scene scene;
  scene.scene_hierarchy = {"ROOT", "ROOT_MODEL", "Layer0", {0, 0, 0}, {}, {}, {}};
  scene.layer_hidden = p.layer_hidden;
  emit_log(o, LogLevel::information,
           "Building scene: " + std::to_string(p.definitions.size()) + " definitions available");
  std::map<GroupKey, size_t> materials;
  size_t mesh_counter = 0, instance_counter = 0;
  std::set<EntityId> active;

  // Deferred mesh backfill: a mesh's own path is recorded verbatim as a
  // path_updates key by the exact instance that placed the definition
  // that mesh's own faces belong to (never an ancestor's), so a direct
  // O(1) lookup per mesh after the whole tree is built is enough - no
  // cascading from an ancestor down to its descendants' own meshes.
  // Needed because a mesh's own MeshMetadata is built once per
  // definition's own geometry (shared across every instance of that
  // definition), before the instance placing it - and so its real
  // per-instance name/properties/attribute dictionaries - is even known.
  // Matches openskp.scene.build_scene's identical path_updates mechanism.
  std::map<std::string, std::tuple<std::map<std::string, std::string>, std::string,
                                   std::map<std::string, std::map<std::string, std::string>>>>
      path_updates;

  // Textures deduplicated by bytes: the same image routinely backs
  // several materials, and re-embedding it per material would multiply
  // the export size for nothing.
  std::map<std::string, std::size_t> texture_index_by_key;
  auto texture_index_for =
      [&](const std::shared_ptr<RawMaterial>& mat) -> std::optional<std::size_t> {
    if (!mat || !mat->texture || !mat->texture->data || mat->texture->data->empty()) {
      return std::nullopt;
    }
    const auto& data = *mat->texture->data;
    auto mime_type = sniff_image_mime(data);
    if (!mime_type) return std::nullopt;  // a format glTF cannot carry
    // length plus a short byte prefix is enough to tell real images apart
    // without hashing megabytes on every face
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
  std::function<std::vector<InstanceNode>(const GeometryBuilder&, const std::string&,
                                          std::optional<EntityId>, const std::vector<double>&,
                                          const std::string&, const std::string&,
                                          std::optional<FaceColorKey>)>
      bake;
  bake = [&](const GeometryBuilder& b, const std::string& defname, std::optional<EntityId> defid,
             const std::vector<double>& matrix, const std::string& layer, const std::string& path,
             std::optional<FaceColorKey> inherited) {
    const auto fallback = inherited.value_or(default_color(p, layer));
    auto groups = build_local_face_groups(p, b, FaceGroupContext{texture_index_for, fallback});
    for (auto& kv : groups) {
      auto& g = kv.second;
      if (g.tris.empty()) continue;
      auto geom = "mesh_" + std::to_string(mesh_counter++) + "_" + safe(path) + "_" + layer;
      if (groups.size() > 1)
        geom += "_" + std::to_string(kv.first.color[0]) + "_" + std::to_string(kv.first.color[1]) +
                "_" + std::to_string(kv.first.color[2]) + "_" + std::to_string(kv.first.color[3]) +
                (kv.first.double_sided ? "_ds" : "_ss");
      MeshMetadata meta;
      meta.name =
          path == "ROOT"
              ? "ROOT"
              : path.substr(path.rfind(" / ") == std::string::npos ? 0 : path.rfind(" / ") + 3);
      meta.definition_name = defname;
      meta.layer = layer;
      meta.path = path;
      meta.position_mm = {matrix.size() > 9 ? matrix[9] * 25.4 : 0,
                          matrix.size() > 10 ? matrix[10] * 25.4 : 0,
                          matrix.size() > 11 ? matrix[11] * 25.4 : 0};
      scene.mesh_index[geom] = meta;
      GlbPrimitive prim;
      prim.geom_name = geom;
      const auto mirrored = transform_determinant(matrix) < 0.0;
      for (auto& triangle : g.tris) {
        prim.indices.push_back(static_cast<uint32_t>(triangle[0]));
        prim.indices.push_back(static_cast<uint32_t>(triangle[mirrored ? 2 : 1]));
        prim.indices.push_back(static_cast<uint32_t>(triangle[mirrored ? 1 : 2]));
      }
      for (auto& m : g.map) {
        auto pt = transform_point(matrix, b.vertices.at(std::get<0>(m.first)));
        prim.positions.resize(g.verts.size() * 3);
        prim.normals.resize(g.verts.size() * 3);
        prim.uvs.resize(g.verts.size() * 2);
        auto i = m.second;
        prim.positions[i * 3] = float(pt[0] * .0254);
        prim.positions[i * 3 + 1] = float(pt[2] * .0254);
        prim.positions[i * 3 + 2] = float(-pt[1] * .0254);
        prim.uvs[i * 2] = g.uvs[i][0];
        prim.uvs[i * 2 + 1] = g.uvs[i][1];
        auto n = g.normals[m.first];
        double l = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (l < 1e-6)
          n = {0, 0, 1};
        else
          for (auto& x : n) x /= l;
        auto w = transform_normal(matrix, n);
        l = std::sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
        if (l < 1e-6) {
          w = n;
          l = 1.0;
        }
        prim.normals[i * 3] = float(w[0] / l);
        prim.normals[i * 3 + 1] = float(w[2] / l);
        prim.normals[i * 3 + 2] = float(-w[1] / l);
      }
      auto mi = materials.find(kv.first);
      if (mi == materials.end()) {
        auto idx = scene.gltf_materials.size();
        GltfMaterial gm;
        gm.pbr_metallic_roughness.base_color_factor = {
            kv.first.color[0] / 255., kv.first.color[1] / 255., kv.first.color[2] / 255.,
            kv.first.color[3] / 255.};
        gm.pbr_metallic_roughness.base_color_texture = kv.first.texture_index;
        gm.double_sided = kv.first.double_sided;
        scene.gltf_materials.push_back(gm);
        mi = materials.emplace(kv.first, idx).first;
      }
      prim.material_index = mi->second;
      scene.glb_primitives.push_back(std::move(prim));
    }
    std::vector<InstanceNode> children;
    for (auto& i : b.instances) {
      std::string child_layer = layer;
      if (!i.layer.empty()) {
        try {
          auto li = p.layer_id_to_name.find(std::stoll(i.layer));
          if (li != p.layer_id_to_name.end()) child_layer = li->second;
        } catch (...) {
          child_layer = i.layer;
          emit_log(o, LogLevel::debug, "Failed to resolve layer id '" + i.layer + "' to a name");
        }
      }
      auto child_color = inherited;
      if (auto color = material_color(find_material(p, i.material_id))) child_color = color;

      // def_name is looked up before building the node, since the
      // name-resolution fallback chain below needs it - same order as
      // openskp.instanced_scene / openskp.scene's own build_scene.
      std::string def_name;
      if (i.ref_idx) {
        auto dd = p.definitions.find(*i.ref_idx);
        if (dd != p.definitions.end()) def_name = dd->second.name;
      }
      const bool def_name_is_real = !def_name.empty() && !is_generic_definition_name(def_name);

      // Same fallback order as instanced_scene.cpp: attribute-dict
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

      const std::string inst_name =
          !i.name.empty()    ? i.name
          : def_name_is_real ? def_name
                             : ("Component_" + (i.ref_idx ? std::to_string(*i.ref_idx) : ""));
      const std::string display_name = name_override.value_or(inst_name);

      auto child_path = path + " / " + inst_name;
      auto mat = multiply_matrices(matrix, i.matrix);
      std::vector<InstanceNode> nested;
      if (i.ref_idx) {
        if (active.count(*i.ref_idx))
          throw SkpParseError("Recursive component definition", ParseStage::build_scene, {}, {}, {},
                              {}, *i.ref_idx);
        auto d = p.definitions.find(*i.ref_idx);
        if (d != p.definitions.end()) {
          active.insert(*i.ref_idx);
          nested = bake(d->second.builder, d->second.name, *i.ref_idx, mat, child_layer, child_path,
                        child_color);
          active.erase(*i.ref_idx);
        }
      }
      InstanceNode node{display_name,
                        def_name,
                        child_layer,
                        {mat.size() > 9 ? mat[9] * 25.4 : 0, mat.size() > 10 ? mat[10] * 25.4 : 0,
                         mat.size() > 11 ? mat[11] * 25.4 : 0},
                        i.properties,
                        std::move(nested),
                        i.attribute_dicts};
      path_updates[child_path] = {i.properties, display_name, i.attribute_dicts};
      children.push_back(std::move(node));
      if (++instance_counter % progress_interval == 0)
        emit_progress(o, ParseStage::build_scene, instance_counter, instance_counter);
    }
    return children;
  };
  std::vector<double> identity{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};
  scene.scene_hierarchy.children =
      bake(p.root.builder, "ROOT_MODEL", {}, identity, "Layer0", "ROOT", {});

  // Apply the deferred backfill: each mesh's real per-instance name/
  // properties/attribute dictionaries, now that the whole tree (and so
  // every instance's own data) has been walked.
  for (auto& [geom_name, meta] : scene.mesh_index) {
    auto found = path_updates.find(meta.path);
    if (found != path_updates.end()) {
      std::tie(meta.properties, meta.name, meta.attribute_dictionaries) = found->second;
    }
  }

  emit_log(o, LogLevel::information,
           "Scene build complete: " + std::to_string(instance_counter) + " instances, " +
               std::to_string(scene.mesh_index.size()) + " meshes, " +
               std::to_string(scene.glb_primitives.size()) + " primitives");
  return scene;
}
}  // namespace openskp
