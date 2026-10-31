#include "face_groups.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

namespace openskp {
namespace {

std::pair<double, double> tile_size(const std::shared_ptr<RawMaterial>& material) {
  double w = 1.0, h = 1.0;
  if (material && material->texture) {
    if (material->texture->x_scale > 1e-9) w = material->texture->x_scale;
    if (material->texture->y_scale > 1e-9) h = material->texture->y_scale;
  }
  return {w, h};
}

// Inverse of a row-major 3x3 matrix, via the cofactor/adjugate method.
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

// Face-plane basis vectors (xr, yr) for UV projection, from a face normal.
std::pair<Vec3, Vec3> face_uv_basis(const Vec3& n) {
  double cx = -n[1], cy = n[0];
  double clen = std::sqrt(cx * cx + cy * cy);
  if (clen < 1e-9) {
    return {Vec3{1, 0, 0}, Vec3{0, n[2] >= 0 ? 1.0 : -1.0, 0}};
  }
  Vec3 xr{cx / clen, cy / clen, 0};
  Vec3 yr{n[1] * xr[2] - n[2] * xr[1], n[2] * xr[0] - n[0] * xr[2], n[0] * xr[1] - n[1] * xr[0]};
  return {xr, yr};
}

// UV of point p (inches, local/object space) on a face with the given
// plane basis, per-face uv_transform (or nullopt for the default
// projection), and material tile size (inches).
std::pair<double, double> compute_face_uv(const Vec3& p, const Vec3& xr, const Vec3& yr,
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

std::vector<EntityId> loop_vertices(const std::vector<CoEdge>& loop, const GeometryBuilder& b) {
  std::vector<EntityId> v;
  for (auto& c : loop) {
    auto i = b.edges.find(c.edge_id);
    if (i == b.edges.end()) continue;
    auto id = c.orientation == 1 ? i->second.first : i->second.second;
    if (id && (v.empty() || v.back() != *id)) v.push_back(*id);
  }
  if (v.size() > 1 && v.front() == v.back()) v.pop_back();
  return v;
}

}  // namespace

std::shared_ptr<RawMaterial> find_material(const RawParsed& parsed, std::optional<EntityId> id) {
  if (!id) return nullptr;
  auto name = parsed.material_id_to_name.find(*id);
  if (name == parsed.material_id_to_name.end()) return nullptr;
  auto direct = parsed.materials.find(name->second);
  if (direct != parsed.materials.end()) return direct->second;
  auto folder = parsed.materials_by_folder.find(name->second);
  if (folder != parsed.materials_by_folder.end()) return folder->second;
  return nullptr;
}

std::optional<FaceColorKey> material_color(const std::shared_ptr<RawMaterial>& material) {
  if (!material) return {};
  // Two independent SketchUp mechanisms can reduce a material's opacity:
  // the plain RGBA color record's alpha byte (material->a), and the newer
  // XML material definition's own trans/useTrans attribute (already
  // resolved into material->transparency). A real material only ever
  // populates one of the two, but multiplying both is safe either way: the
  // untouched one defaults to fully-opaque (255 or 1.0), so it never
  // silently darkens a material that only used the other mechanism.
  const double combined =
      (std::clamp(material->a, 0, 255) / 255.0) * std::clamp(material->transparency, 0.0, 1.0);
  return FaceColorKey{material->r, material->g, material->b,
                      static_cast<int>(std::lround(combined * 255.0))};
}

FaceColorKey default_color(const RawParsed& parsed, const std::string& layer) {
  auto found = parsed.layer_colors.find(layer);
  auto color = found == parsed.layer_colors.end() ? Color3{136, 136, 136} : found->second;
  return {color[0], color[1], color[2], 255};
}

// Identifies an image's MIME type from its magic bytes. Returns nullopt
// for anything glTF cannot carry (glTF only allows PNG and JPEG).
std::optional<std::string> sniff_image_mime(const ByteBuffer& data) {
  if (data.size() >= 3 && data[0] == 0xff && data[1] == 0xd8 && data[2] == 0xff) {
    return "image/jpeg";
  }
  if (data.size() >= 8 && data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4e &&
      data[3] == 0x47 && data[4] == 0x0d && data[5] == 0x0a && data[6] == 0x1a && data[7] == 0x0a) {
    return "image/png";
  }
  return std::nullopt;
}

std::map<GroupKey, Group> build_local_face_groups(const RawParsed& parsed, const GeometryBuilder& b,
                                                  const FaceGroupContext& ctx) {
  std::map<GroupKey, Group> groups;

  for (auto& fv : b.faces) {
    auto& f = fv.second;
    const auto front_mat = find_material(parsed, f.material_id);
    const auto back_mat = find_material(parsed, f.back_material_id);
    const auto front = material_color(front_mat).value_or(ctx.fallback_color);
    const auto back = material_color(back_mat).value_or(ctx.fallback_color);

    std::vector<std::vector<EntityId>> loops;
    for (auto& l : f.loops) {
      auto x = loop_vertices(l, b);
      if (!x.empty()) loops.push_back(std::move(x));
    }
    if (loops.empty()) continue;

    std::map<EntityId, Vertex> vv;
    for (auto& x : b.vertices) vv[x.first] = {x.first, x.second[0], x.second[1], x.second[2]};
    const auto triangles = triangulate_face_3d(vv, loops, f.normal);

    // Not a structured binding: those can't be captured by the nested
    // add_side lambda below in C++17 (only from C++20 onward).
    const auto uv_basis = face_uv_basis(f.normal);
    const Vec3& xr = uv_basis.first;
    const Vec3& yr = uv_basis.second;
    const auto add_side = [&](const GroupKey& key, bool reverse,
                              const std::optional<std::array<double, 9>>& uv_transform,
                              double tile_w, double tile_h) {
      auto& group = groups[key];
      for (auto triangle : triangles) {
        if (reverse) std::swap(triangle[1], triangle[2]);
        std::array<size_t, 3> indices{};
        for (int vertex = 0; vertex < 3; ++vertex) {
          auto id = triangle[vertex];
          const auto& pos = b.vertices.at(id);
          const auto [u, v] = compute_face_uv(pos, xr, yr, uv_transform, tile_w, tile_h);
          const FaceVKey vkey{id, u, v};
          auto it = group.map.find(vkey);
          if (it == group.map.end()) {
            it = group.map.emplace(vkey, group.verts.size()).first;
            group.verts.push_back(pos);
            group.uvs.push_back({float(u), float(v)});
          }
          indices[vertex] = it->second;
          auto& normal = group.normals[vkey];
          for (int axis = 0; axis < 3; ++axis)
            normal[axis] += reverse ? -f.normal[axis] : f.normal[axis];
        }
        group.tris.push_back(indices);
      }
    };
    if (front == back) {
      const auto [tw, th] = tile_size(front_mat);
      add_side({front, true, ctx.texture_index_for(front_mat)}, false, f.uv_transform, tw, th);
    } else {
      const auto [ftw, fth] = tile_size(front_mat);
      add_side({front, false, ctx.texture_index_for(front_mat)}, false, f.uv_transform, ftw, fth);
      const auto [btw, bth] = tile_size(back_mat);
      add_side({back, false, ctx.texture_index_for(back_mat)}, true, f.uv_transform_back, btw, bth);
    }
  }

  return groups;
}

}  // namespace openskp
