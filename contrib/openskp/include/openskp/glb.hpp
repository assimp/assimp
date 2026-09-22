#pragma once

#include <filesystem>

#include <openskp/export.hpp>
#include <openskp/model.hpp>
#include <openskp/scene.hpp>

namespace openskp {

/// Options for to_glb()/export_glb().
struct GlbOptions {
  /// Embed the scene's texture images in the GLB and point each textured
  /// material's baseColorTexture at them. Off by default, matching every
  /// other language's exporter: photographic textures can multiply the
  /// file size, and the geometry alone is what most callers are after.
  bool textures{false};
};

/// Serialize a baked scene as a binary glTF 2.0 (GLB) asset.
OPENSKP_EXPORT ByteBuffer to_glb(const Scene& scene, const GlbOptions& options = {});

/// Serialize a baked scene and write the resulting bytes to a file.
OPENSKP_EXPORT void export_glb(const Scene& scene, const std::filesystem::path& output_path,
                               const GlbOptions& options = {});

}  // namespace openskp
