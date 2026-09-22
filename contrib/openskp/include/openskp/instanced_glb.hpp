#pragma once

#include <filesystem>

#include <openskp/export.hpp>
#include <openskp/instanced_scene.hpp>
#include <openskp/model.hpp>

namespace openskp {

/// Options for to_instanced_glb()/export_instanced_glb().
struct InstancedGlbOptions {
  /// Embed the scene's texture images in the GLB and point each textured
  /// material's baseColorTexture at them. Off by default, matching
  /// GlbOptions::textures: photographic textures can multiply the file
  /// size, and the geometry alone is what most callers are after.
  bool textures{false};
};

/// Serialize an InstancedScene as a binary glTF 2.0 (GLB) asset,
/// PRESERVING instancing: each mesh resource is written to the binary
/// buffer exactly once, and every placement is a glTF node whose "mesh"
/// points at it.
///
/// This is what to_glb() cannot do from a baked Scene, whose primitives
/// already have the world transform folded into their vertex data - there
/// is nothing left to share. Here, a component placed 1,000 times
/// contributes one copy of its vertex/index buffers plus 1,000 node
/// transforms. to_glb() is untouched and still produces exactly what it
/// always has.
OPENSKP_EXPORT ByteBuffer to_instanced_glb(const InstancedScene& scene,
                                           const InstancedGlbOptions& options = {});

/// Serialize an InstancedScene and write the resulting bytes to a file.
OPENSKP_EXPORT void export_instanced_glb(const InstancedScene& scene,
                                         const std::filesystem::path& output_path,
                                         const InstancedGlbOptions& options = {});

}  // namespace openskp
