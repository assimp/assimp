#include <cmath>

#include "internal.hpp"

namespace openskp {

std::optional<Vec3> compute_face_normal(const std::vector<Vec3>& points) {
  if (points.size() < 3) return {};

  Vec3 normal{};
  for (std::size_t i = 0; i < points.size(); ++i) {
    const auto& current = points[i];
    const auto& next = points[(i + 1) % points.size()];
    normal[0] += (current[1] - next[1]) * (current[2] + next[2]);
    normal[1] += (current[2] - next[2]) * (current[0] + next[0]);
    normal[2] += (current[0] - next[0]) * (current[1] + next[1]);
  }

  const double length =
      std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
  if (length < 1e-12) return {};
  for (double& value : normal) value /= length;
  return normal;
}

std::vector<std::array<EntityId, 3>> triangulate_face_3d(
    const std::map<EntityId, Vertex>& verts, const std::vector<std::vector<EntityId>>& loops,
    Vec3 n) {
  if (loops.empty()) return {};
  if (loops.size() == 1 && loops[0].size() == 3) return {{loops[0][0], loops[0][1], loops[0][2]}};
  if (loops.size() == 1 && loops[0].size() == 4) {
    return {{{loops[0][0], loops[0][1], loops[0][2]}}, {{loops[0][0], loops[0][2], loops[0][3]}}};
  }
  double len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
  if (len < 1e-6)
    n = {0, 0, 1};
  else
    for (auto& x : n) x /= len;
  Vec3 seed = std::abs(n[0]) < .9 ? Vec3{1, 0, 0} : Vec3{0, 1, 0};
  Vec3 u{n[1] * seed[2] - n[2] * seed[1], n[2] * seed[0] - n[0] * seed[2],
         n[0] * seed[1] - n[1] * seed[0]};
  len = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
  if (len < 1e-12)
    u = {1, 0, 0};
  else
    for (auto& x : u) x /= len;
  Vec3 v{n[1] * u[2] - n[2] * u[1], n[2] * u[0] - n[0] * u[2], n[0] * u[1] - n[1] * u[0]};
  std::vector<std::vector<EarPoint>> projected;
  for (auto& loop : loops) {
    std::vector<EarPoint> p;
    for (auto id : loop) {
      auto i = verts.find(id);
      if (i == verts.end()) return {};
      auto& q = i->second;
      p.push_back({q.x * u[0] + q.y * u[1] + q.z * u[2], q.x * v[0] + q.y * v[1] + q.z * v[2], id});
    }
    projected.push_back(std::move(p));
  }
  return earcut_2d(std::move(projected));
}
}  // namespace openskp
