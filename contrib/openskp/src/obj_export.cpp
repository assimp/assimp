#include "openskp/obj_export.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace openskp {

static std::string sanitize_material_name(const std::string& name) {
  std::string clean = name;
  clean.erase(0, clean.find_first_not_of(" \t\n\r"));
  clean.erase(clean.find_last_not_of(" \t\n\r") + 1);
  for (char& c : clean) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '-') {
      c = '_';
    }
  }
  return clean.empty() ? "default_material" : clean;
}

std::string to_mtl(const Scene& scene) {
  std::ostringstream ss;
  ss.imbue(std::locale::classic());
  ss << "# OpenSKP MTL Material Library Export\n";
  ss << "# Materials: " << scene.gltf_materials.size() << "\n\n";

  for (std::size_t idx = 0; idx < scene.gltf_materials.size(); ++idx) {
    const auto& mat = scene.gltf_materials[idx];
    std::string raw_name = mat.name.empty() ? ("Material_" + std::to_string(idx)) : mat.name;
    std::string mat_name = sanitize_material_name(raw_name);

    float r = mat.pbr_metallic_roughness.base_color_factor[0];
    float g = mat.pbr_metallic_roughness.base_color_factor[1];
    float b = mat.pbr_metallic_roughness.base_color_factor[2];
    float a = mat.pbr_metallic_roughness.base_color_factor[3];

    ss << "newmtl " << mat_name << "\n";
    ss << "Ka 1.000000 1.000000 1.000000\n";
    ss << "Kd " << std::fixed << std::setprecision(6) << r << " " << g << " " << b << "\n";
    ss << "Ks 0.200000 0.200000 0.200000\n";
    ss << "Ns 32.000000\n";
    ss << "d " << std::fixed << std::setprecision(6) << a << "\n";
    ss << "illum 2\n";

    if (!mat.texture_path.empty()) {
      std::string tex_name = std::filesystem::path(mat.texture_path).filename().string();
      ss << "map_Kd " << tex_name << "\n";
    }

    ss << "\n";
  }

  return ss.str();
}

std::string to_obj(const Scene& scene, const std::string& mtl_filename) {
  std::ostringstream ss;
  ss.imbue(std::locale::classic());
  ss << "# OpenSKP OBJ Export\n";
  ss << "# Primitives: " << scene.glb_primitives.size() << "\n";

  if (!mtl_filename.empty()) {
    ss << "mtllib " << mtl_filename << "\n";
  }
  ss << "\n";

  std::uint32_t vert_offset = 1;
  std::uint32_t uv_offset = 1;
  std::uint32_t norm_offset = 1;

  for (const auto& prim : scene.glb_primitives) {
    ss << "o " << prim.geom_name << "\n";

    std::size_t vert_count = prim.positions.size() / 3;
    for (std::size_t i = 0; i < vert_count; ++i) {
      ss << "v " << std::fixed << std::setprecision(6) << prim.positions[i * 3] << " "
         << prim.positions[i * 3 + 1] << " " << prim.positions[i * 3 + 2] << "\n";
    }

    std::size_t uv_count = prim.uvs.size() / 2;
    for (std::size_t i = 0; i < uv_count; ++i) {
      ss << "vt " << std::fixed << std::setprecision(6) << prim.uvs[i * 2] << " "
         << prim.uvs[i * 2 + 1] << "\n";
    }

    std::size_t norm_count = prim.normals.size() / 3;
    for (std::size_t i = 0; i < norm_count; ++i) {
      ss << "vn " << std::fixed << std::setprecision(6) << prim.normals[i * 3] << " "
         << prim.normals[i * 3 + 1] << " " << prim.normals[i * 3 + 2] << "\n";
    }

    std::int32_t mat_idx = prim.material_index;
    if (mat_idx >= 0 && static_cast<std::size_t>(mat_idx) < scene.gltf_materials.size()) {
      std::string mat_raw = scene.gltf_materials[mat_idx].name.empty()
                                ? ("Material_" + std::to_string(mat_idx))
                                : scene.gltf_materials[mat_idx].name;
      ss << "usemtl " << sanitize_material_name(mat_raw) << "\n";
    }

    std::size_t tri_count = prim.indices.size() / 3;
    bool has_uvs = (uv_count == vert_count);
    bool has_normals = (norm_count == vert_count);

    for (std::size_t i = 0; i < tri_count; ++i) {
      std::uint32_t i0 = prim.indices[i * 3];
      std::uint32_t i1 = prim.indices[i * 3 + 1];
      std::uint32_t i2 = prim.indices[i * 3 + 2];

      std::uint32_t v0 = i0 + vert_offset;
      std::uint32_t v1 = i1 + vert_offset;
      std::uint32_t v2 = i2 + vert_offset;

      if (has_uvs && has_normals) {
        std::uint32_t vt0 = i0 + uv_offset;
        std::uint32_t vt1 = i1 + uv_offset;
        std::uint32_t vt2 = i2 + uv_offset;
        std::uint32_t vn0 = i0 + norm_offset;
        std::uint32_t vn1 = i1 + norm_offset;
        std::uint32_t vn2 = i2 + norm_offset;
        ss << "f " << v0 << "/" << vt0 << "/" << vn0 << " " << v1 << "/" << vt1 << "/" << vn1 << " "
           << v2 << "/" << vt2 << "/" << vn2 << "\n";
      } else if (has_uvs) {
        std::uint32_t vt0 = i0 + uv_offset;
        std::uint32_t vt1 = i1 + uv_offset;
        std::uint32_t vt2 = i2 + uv_offset;
        ss << "f " << v0 << "/" << vt0 << " " << v1 << "/" << vt1 << " " << v2 << "/" << vt2
           << "\n";
      } else if (has_normals) {
        std::uint32_t vn0 = i0 + norm_offset;
        std::uint32_t vn1 = i1 + norm_offset;
        std::uint32_t vn2 = i2 + norm_offset;
        ss << "f " << v0 << "//" << vn0 << " " << v1 << "//" << vn1 << " " << v2 << "//" << vn2
           << "\n";
      } else {
        ss << "f " << v0 << " " << v1 << " " << v2 << "\n";
      }
    }

    vert_offset += static_cast<std::uint32_t>(vert_count);
    if (has_uvs) uv_offset += static_cast<std::uint32_t>(uv_count);
    if (has_normals) norm_offset += static_cast<std::uint32_t>(norm_count);

    ss << "\n";
  }

  return ss.str();
}

void export_obj(const Scene& scene, const std::filesystem::path& path, bool export_mtl) {
  auto parent = path.parent_path();
  if (!parent.empty() && !std::filesystem::exists(parent)) {
    std::filesystem::create_directories(parent);
  }

  std::string mtl_name = export_mtl ? (path.stem().string() + ".mtl") : "";

  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error("Cannot open OBJ output file: " + path.string());
  }
  out << to_obj(scene, mtl_name);
  if (!out) {
    throw std::runtime_error("Failed to write OBJ output file: " + path.string());
  }

  if (export_mtl && !mtl_name.empty()) {
    auto mtl_path = parent / mtl_name;
    std::ofstream mtl_out(mtl_path);
    if (!mtl_out) {
      throw std::runtime_error("Cannot open MTL output file: " + mtl_path.string());
    }
    mtl_out << to_mtl(scene);
  }
}

}  // namespace openskp
