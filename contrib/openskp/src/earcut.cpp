#include <algorithm>
#include <cmath>

#include "internal.hpp"

namespace openskp {
namespace {
double area(const std::vector<EarPoint>& p) {
  double a = 0;
  for (size_t i = 0; i < p.size(); ++i) {
    auto& q = p[(i + 1) % p.size()];
    a += p[i].x * q.y - q.x * p[i].y;
  }
  return a * .5;
}

double cross(const EarPoint& a, const EarPoint& b, const EarPoint& c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool inside(const EarPoint& p, const EarPoint& a, const EarPoint& b, const EarPoint& c) {
  auto x = cross(a, b, p), y = cross(b, c, p), z = cross(c, a, p);
  return x >= -1e-12 && y >= -1e-12 && z >= -1e-12;
}

std::vector<std::array<EntityId, 3>> clip(std::vector<EarPoint> p) {
  std::vector<std::array<EntityId, 3>> out;
  if (area(p) < 0) std::reverse(p.begin(), p.end());
  size_t guard = 0;
  while (p.size() > 3 && guard++ < p.size() * p.size() * 4) {
    bool cut = false;
    for (size_t i = 0; i < p.size(); ++i) {
      auto a = (i + p.size() - 1) % p.size(), c = (i + 1) % p.size();
      if (cross(p[a], p[i], p[c]) <= 1e-12) continue;
      bool hit = false;
      for (size_t j = 0; j < p.size(); ++j)
        if (j != a && j != i && j != c && inside(p[j], p[a], p[i], p[c]) && p[j].id != p[a].id &&
            p[j].id != p[i].id && p[j].id != p[c].id) {
          hit = true;
          break;
        }
      if (hit) continue;
      out.push_back({p[a].id, p[i].id, p[c].id});
      p.erase(p.begin() + i);
      cut = true;
      break;
    }
    if (!cut) {
      for (size_t i = 0; i < p.size(); ++i) {
        auto a = (i + p.size() - 1) % p.size(), c = (i + 1) % p.size();
        if (std::abs(cross(p[a], p[i], p[c])) < 1e-10) {
          p.erase(p.begin() + i);
          cut = true;
          break;
        }
      }
      if (!cut) break;
    }
  }
  if (p.size() == 3 && std::abs(cross(p[0], p[1], p[2])) > 1e-12)
    out.push_back({p[0].id, p[1].id, p[2].id});
  return out;
}

// Standard ray-casting point-in-polygon test (even-odd rule) - true when
// (x, y) lies strictly inside the given closed 2D ring. Mirrors the
// Python/.NET/TypeScript/Dart ports' identical check exactly (openskp#285).
bool point_in_polygon(double x, double y, const std::vector<EarPoint>& polygon) {
  bool inside_result = false;
  const size_t n = polygon.size();
  for (size_t i = 0, j = n - 1; i < n; j = i++) {
    double xi = polygon[i].x, yi = polygon[i].y;
    double xj = polygon[j].x, yj = polygon[j].y;
    bool intersects = ((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
    if (intersects) inside_result = !inside_result;
  }
  return inside_result;
}

bool any_vertex_inside(const std::vector<EarPoint>& points, const std::vector<EarPoint>& polygon) {
  for (auto& p : points)
    if (point_in_polygon(p.x, p.y, polygon)) return true;
  return false;
}

// True when at least one pair of hole loops (index 1+ in `loops`) shares
// real area - not just a boundary point or edge, which is a normal,
// common pattern (e.g. two holes sharing a cut line). Detected via a
// vertex-containment test: for genuinely overlapping simple polygons (in
// particular the convex/circular holes real drilled geometry produces),
// the overlap region always contains at least one polygon's own vertex
// inside the other. Mirrors the Python/.NET/TypeScript/Dart ports'
// identical check exactly (openskp#285).
bool holes_overlap(const std::vector<std::vector<EarPoint>>& loops) {
  for (size_t i = 1; i < loops.size(); ++i)
    for (size_t j = i + 1; j < loops.size(); ++j)
      if (any_vertex_inside(loops[i], loops[j]) || any_vertex_inside(loops[j], loops[i]))
        return true;
  return false;
}

// Fallback path when two or more hole loops genuinely overlap: bridging
// each hole into the outer boundary independently (earcut_2d's normal
// strategy) assumes disjoint holes - bridging a second hole into a
// polygon a first, overlapping hole has already been spliced into can
// produce a self-intersecting merged ring, which clip()'s degenerate-
// vertex fallback then silently erodes far beyond the affected holes
// (observed on a real fixture: triangle count dropped by ~10%, not just
// the handful of triangles near the two overlapping holes). Ported from
// the same fix already shipped in Python/.NET/TypeScript/Dart
// (openskp#285): triangulate the outer boundary ALONE (no holes bridged
// in at all) and discard any resulting triangle whose centroid falls
// inside ANY hole - the same "triangulate then filter" strategy this
// project's own Python pipeline used before its earcut migration.
std::vector<std::array<EntityId, 3>> triangulate_by_filtering_holes(
    const std::vector<std::vector<EarPoint>>& loops) {
  auto outer_tris = clip(loops[0]);
  std::vector<std::array<EntityId, 3>> result;
  std::map<EntityId, EarPoint> by_id;
  for (auto& p : loops[0]) by_id[p.id] = p;
  for (auto& tri : outer_tris) {
    auto& a = by_id.at(tri[0]);
    auto& b = by_id.at(tri[1]);
    auto& c = by_id.at(tri[2]);
    double cx = (a.x + b.x + c.x) / 3.0;
    double cy = (a.y + b.y + c.y) / 3.0;
    bool inside_any_hole = false;
    for (size_t h = 1; h < loops.size(); ++h)
      if (point_in_polygon(cx, cy, loops[h])) {
        inside_any_hole = true;
        break;
      }
    if (!inside_any_hole) result.push_back(tri);
  }
  return result;
}
}  // namespace

std::vector<std::array<EntityId, 3>> earcut_2d(std::vector<std::vector<EarPoint>> loops) {
  if (loops.empty()) return {};
  if (loops.size() > 2 && holes_overlap(loops)) return triangulate_by_filtering_holes(loops);
  auto poly = std::move(loops[0]);
  if (area(poly) < 0) std::reverse(poly.begin(), poly.end());
  for (size_t h = 1; h < loops.size(); ++h) {
    auto hole = std::move(loops[h]);
    if (hole.empty()) continue;
    if (area(hole) > 0) std::reverse(hole.begin(), hole.end());
    size_t hi = 0;
    for (size_t i = 1; i < hole.size(); ++i)
      if (hole[i].x > hole[hi].x || (hole[i].x == hole[hi].x && hole[i].y < hole[hi].y)) hi = i;
    size_t oi = 0;
    double best = 1e300;
    for (size_t i = 0; i < poly.size(); ++i) {
      double dx = poly[i].x - hole[hi].x;
      if (dx >= 0) {
        double d = dx * dx + (poly[i].y - hole[hi].y) * (poly[i].y - hole[hi].y);
        if (d < best) {
          best = d;
          oi = i;
        }
      }
    }
    std::vector<EarPoint> merged;
    for (size_t i = 0; i <= oi; ++i) merged.push_back(poly[i]);
    merged.push_back(hole[hi]);
    for (size_t k = 1; k < hole.size(); ++k) merged.push_back(hole[(hi + k) % hole.size()]);
    merged.push_back(hole[hi]);
    merged.push_back(poly[oi]);
    for (size_t i = oi + 1; i < poly.size(); ++i) merged.push_back(poly[i]);
    poly = std::move(merged);
  }
  return clip(std::move(poly));
}
}  // namespace openskp
