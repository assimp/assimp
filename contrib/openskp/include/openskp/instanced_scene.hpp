#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <openskp/export.hpp>
#include <openskp/model.hpp>
#include <openskp/scene.hpp>

namespace openskp {

/// One reusable, DEFINITION-LOCAL triangulated mesh: the instanced
/// counterpart of GlbPrimitive, minus the world transform.
///
/// Positions and normals stay in the definition's own local frame (metres,
/// glTF Y-up - already converted, same as GlbPrimitive), so N placements
/// of the same definition share this one buffer set instead of getting N
/// transformed copies of it. Normal transformation is deferred to the
/// consumer/renderer's node transform (glTF's own inverse-transpose rule),
/// which is what keeps mirrored/non-uniform-scale placements correct
/// without a per-instance normal copy.
struct LocalPrimitive {
  std::vector<float> positions;
  std::vector<float> normals;
  std::vector<float> uvs;
  std::vector<std::uint32_t> indices;
  std::size_t material_index{};
};

/// A definition's geometry, resolved for one specific rendering context
/// and ready to be referenced by any number of InstancedNodes.
///
/// One SketchUp definition can yield MORE than one resource: the same
/// component painted with two different colors renders differently and
/// therefore needs a separate variant - see variant_key.
struct InstancedMeshResource {
  std::string id;
  /// nullopt for the root definition (loose, non-component geometry).
  std::optional<EntityId> definition_id;
  std::string definition_name;
  std::string variant_key;
  std::vector<LocalPrimitive> primitives;
};

/// One placed node in the instanced scene graph.
///
/// Carries the transform that places its mesh_resource_id (and its whole
/// subtree) into the scene, instead of that transform having been baked
/// into vertex data.
struct InstancedNode {
  std::string name;
  /// True when `name` is a fallback this project generated (no real name
  /// anywhere - no attribute-dict override, no instance name, no
  /// meaningfully-named definition) rather than something a person or a
  /// plugin actually named. A consumer can use this to render such nodes
  /// distinctly (greyed out, routed to an "Uncategorized" bucket) instead
  /// of presenting a synthetic placeholder as if it were real data.
  bool name_is_generated{false};
  std::string definition_name;
  std::string layer;
  /// This instance's own persistent GUID (a real one from the source SKP
  /// file when available - modern/VFF files carry a genuine 16-byte
  /// instance GUID per placement; legacy pre-2021 files don't currently
  /// expose one, so this is "" for them). Never used for geometry/
  /// placement - purely an identity string a consumer (e.g. a
  /// Fragments-format exporter) can carry through so a clicked/selected
  /// element has something stable to key off of.
  std::string guid;
  /// This node's transform RELATIVE TO ITS PARENT, as a 16-element
  /// column-major glTF matrix (metres, Y-up) - directly usable as a glTF
  /// node "matrix". The root node's matrix is the identity.
  std::array<double, 16> matrix{
      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
  };
  std::array<double, 3> position_mm{};
  std::map<std::string, std::string> properties;
  /// Every OTHER attribute dictionary this instance carries, keyed by the
  /// dictionary's own name - a third-party plugin (BIM/steel-detailing
  /// tool, etc.) commonly attaches its own richer per-instance data under
  /// its own dictionary name instead of `properties` (SketchUp's own
  /// Dynamic Components data).
  std::map<std::string, std::map<std::string, std::string>> attribute_dictionaries;
  std::optional<std::string> mesh_resource_id;
  std::vector<InstancedNode> children;
};

/// Axis-aligned bounds of the scene as PLACED, metres and Y-up.
struct SceneBounds {
  std::array<double, 3> min{};
  std::array<double, 3> max{};
  std::array<double, 3> size{};
  std::array<double, 3> center{};
};

/// The result of build_instanced_scene(): the placed scene graph with
/// SketchUp's instancing PRESERVED rather than baked out.
///
/// Where Scene emits one world-space vertex buffer per placement, this
/// emits each distinct definition+context once (mesh_resources) and refers
/// to it from every placement (scene_hierarchy). Scene size therefore
/// scales with unique geometry + instance transforms instead of
/// definition geometry x placement count.
///
/// Lossless: no decimation, quantisation or geometry approximation of any
/// kind. The triangles are the same triangles the baked path produces,
/// just stored once and referenced N times.
struct OPENSKP_EXPORT InstancedScene {
  std::optional<SceneBounds> bounds;
  InstancedNode scene_hierarchy;
  std::vector<InstancedMeshResource> mesh_resources;
  std::vector<GltfMaterial> gltf_materials;
  /// Distinct texture images the placed materials use, deduplicated by
  /// source bytes - same as Scene::textures.
  std::vector<SceneTexture> textures;
  /// The source file's own per-layer visibility (VFF 8E3C tag), keyed by
  /// layer name - a consumer (e.g. the Fragments exporter) that wants to
  /// respect "this layer was off by default in SketchUp" carries this
  /// through, since the layer concept itself doesn't survive into the
  /// instanced tree/mesh data any other way.
  std::map<std::string, bool> layer_hidden;
};

}  // namespace openskp
