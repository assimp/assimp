#pragma once

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "internal.hpp"

namespace openskp {

// Local-space face grouping, shared by the baked (scene.cpp) and instanced
// (instanced_scene.cpp) scene builders.
//
// Extracted from scene.cpp unchanged (openskp#200, mirroring TypeScript's
// face-groups.ts): a definition's faces are grouped by resolved (color,
// double_sided, texture) identity in DEFINITION-LOCAL space (inches,
// SketchUp Z-up) - exactly what the baked builder assembles just before
// applying an instance's world matrix, and exactly what the instanced
// builder keeps local and puts on the node instead. Keeping one
// implementation is what makes the two paths agree on triangulation, UV
// seams, normals and front/back handling by construction rather than by
// parallel maintenance.
//
// Faithful to the pre-existing baked behavior it was extracted from: an
// unpainted face falls back to the caller-supplied fallback_color for
// color, but its material (and therefore texture tile size) is resolved
// from the face's OWN material_id/back_material_id only - an instance's
// painted material is not consulted for texture purposes here. That is an
// existing characteristic of this port (TypeScript's reference
// additionally falls back to the inherited material itself for texture
// tile size on unpainted faces), preserved rather than changed by this
// extraction. Also preserved: color includes a 4th (alpha/transparency)
// component here, unlike the other three ports' 3-component RGB key - C++
// already had that before this port.

// Color key: (r, g, b, alpha-from-transparency). Richer than the other
// three ports' 3-component RGB key - an existing C++ characteristic,
// preserved rather than changed by this extraction.
using FaceColorKey = std::array<int, 4>;

struct GroupKey {
  FaceColorKey color;
  bool double_sided{};
  // The texture is part of the identity, not just the color: two
  // different images can average to the same RGB (real files do this),
  // and keying on color alone would merge them into one material and
  // lose one of the images.
  std::optional<std::size_t> texture_index;

  bool operator<(const GroupKey& other) const {
    return std::tie(color, double_sided, texture_index) <
           std::tie(other.color, other.double_sided, other.texture_index);
  }
};

// A vertex is keyed by (source vertex id, u, v): UVs are inherently
// per-face, so a vertex position shared by two faces that disagree on
// texture mapping must become two distinct output vertices (glTF requires
// position/normal/uv aligned per index).
using FaceVKey = std::tuple<EntityId, double, double>;

struct Group {
  std::vector<Vec3> verts;
  std::vector<std::array<float, 2>> uvs;
  std::vector<std::array<size_t, 3>> tris;
  std::map<FaceVKey, size_t> map;
  std::map<FaceVKey, Vec3> normals;
};

std::shared_ptr<RawMaterial> find_material(const RawParsed& parsed, std::optional<EntityId> id);
std::optional<FaceColorKey> material_color(const std::shared_ptr<RawMaterial>& material);
FaceColorKey default_color(const RawParsed& parsed, const std::string& layer);
std::optional<std::string> sniff_image_mime(const ByteBuffer& data);

// Everything build_local_face_groups needs from its caller that isn't the
// builder itself.
struct FaceGroupContext {
  std::function<std::optional<std::size_t>(const std::shared_ptr<RawMaterial>&)> texture_index_for;
  // Color an unpainted face falls back to (already resolved by the
  // caller: the instance's inherited paint color, or the effective
  // layer's color when nothing is inherited).
  FaceColorKey fallback_color{};
};

// Group a definition's faces by resolved material identity, in local
// space.
//
// A face whose front/back resolve to the SAME color is emitted once with
// double_sided set; a face whose sides genuinely differ is emitted as two
// single-sided triangle sets (one normal-wound front, one reverse-wound
// back) so each side keeps its own color.
std::map<GroupKey, Group> build_local_face_groups(const RawParsed& parsed, const GeometryBuilder& b,
                                                  const FaceGroupContext& ctx);

}  // namespace openskp
