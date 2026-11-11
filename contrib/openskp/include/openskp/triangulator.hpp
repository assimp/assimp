#pragma once

#include <array>
#include <map>
#include <optional>
#include <vector>

#include <openskp/model.hpp>

namespace openskp {

OPENSKP_EXPORT std::optional<Vec3> compute_face_normal(const std::vector<Vec3>& points);

OPENSKP_EXPORT std::vector<std::array<EntityId, 3>> triangulate_face_3d(
    const std::map<EntityId, Vertex>& vertices, const std::vector<std::vector<EntityId>>& loops,
    Vec3 normal);
}  // namespace openskp
