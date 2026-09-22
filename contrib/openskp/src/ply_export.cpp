#include "openskp/ply_export.hpp"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace openskp {

namespace {

struct ColorRgba {
  std::uint8_t r, g, b, a;
};

ColorRgba get_material_rgba(const Scene& scene, std::size_t mat_idx) {
  if (mat_idx < scene.gltf_materials.size()) {
    const auto& mat = scene.gltf_materials[mat_idx];
    const auto& color = mat.pbr_metallic_roughness.base_color_factor;
    float r_flt = std::round(static_cast<float>(color[0]) * 255.0f);
    float g_flt = std::round(static_cast<float>(color[1]) * 255.0f);
    float b_flt = std::round(static_cast<float>(color[2]) * 255.0f);
    float a_flt = std::round(static_cast<float>(color[3]) * 255.0f);

    std::uint8_t r = static_cast<std::uint8_t>(std::max(0.0f, std::min(255.0f, r_flt)));
    std::uint8_t g = static_cast<std::uint8_t>(std::max(0.0f, std::min(255.0f, g_flt)));
    std::uint8_t b = static_cast<std::uint8_t>(std::max(0.0f, std::min(255.0f, b_flt)));
    std::uint8_t a = static_cast<std::uint8_t>(std::max(0.0f, std::min(255.0f, a_flt)));
    return {r, g, b, a};
  }
  return {200, 200, 200, 255};
}

void write_float_le(std::vector<std::uint8_t>& buf, float value) {
  std::uint32_t uval;
  std::memcpy(&uval, &value, 4);
  buf.push_back(static_cast<std::uint8_t>(uval & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((uval >> 8) & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((uval >> 16) & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((uval >> 24) & 0xFF));
}

void write_int32_le(std::vector<std::uint8_t>& buf, std::int32_t value) {
  std::uint32_t uval = static_cast<std::uint32_t>(value);
  buf.push_back(static_cast<std::uint8_t>(uval & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((uval >> 8) & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((uval >> 16) & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((uval >> 24) & 0xFF));
}

}  // namespace

std::string to_ply_ascii(const Scene& scene) {
  std::size_t total_vertices = 0;
  std::size_t total_faces = 0;
  for (const auto& prim : scene.glb_primitives) {
    total_vertices += prim.positions.size() / 3;
    total_faces += prim.indices.size() / 3;
  }

  std::ostringstream ss;
  ss.imbue(std::locale::classic());

  ss << "ply\n";
  ss << "format ascii 1.0\n";
  ss << "comment Created by OpenSKP\n";
  ss << "element vertex " << total_vertices << "\n";
  ss << "property float x\n";
  ss << "property float y\n";
  ss << "property float z\n";
  ss << "property float nx\n";
  ss << "property float ny\n";
  ss << "property float nz\n";
  ss << "property float u\n";
  ss << "property float v\n";
  ss << "property uchar red\n";
  ss << "property uchar green\n";
  ss << "property uchar blue\n";
  ss << "property uchar alpha\n";
  ss << "element face " << total_faces << "\n";
  ss << "property list uchar int vertex_indices\n";
  ss << "end_header\n";

  for (const auto& prim : scene.glb_primitives) {
    ColorRgba color = get_material_rgba(scene, prim.material_index);
    std::size_t vert_count = prim.positions.size() / 3;
    for (std::size_t i = 0; i < vert_count; ++i) {
      double px = prim.positions[i * 3];
      double py = prim.positions[i * 3 + 1];
      double pz = prim.positions[i * 3 + 2];

      double nx = (i * 3 < prim.normals.size()) ? prim.normals[i * 3] : 0.0;
      double ny = (i * 3 + 1 < prim.normals.size()) ? prim.normals[i * 3 + 1] : 0.0;
      double nz = (i * 3 + 2 < prim.normals.size()) ? prim.normals[i * 3 + 2] : 0.0;

      double u = (i * 2 < prim.uvs.size()) ? prim.uvs[i * 2] : 0.0;
      double v = (i * 2 + 1 < prim.uvs.size()) ? prim.uvs[i * 2 + 1] : 0.0;

      ss << std::fixed << std::setprecision(6) << px << " " << py << " " << pz << " " << nx << " "
         << ny << " " << nz << " " << u << " " << v << " " << static_cast<int>(color.r) << " "
         << static_cast<int>(color.g) << " " << static_cast<int>(color.b) << " "
         << static_cast<int>(color.a) << "\n";
    }
  }

  std::size_t vert_offset = 0;
  for (const auto& prim : scene.glb_primitives) {
    std::size_t tri_count = prim.indices.size() / 3;
    for (std::size_t i = 0; i < tri_count; ++i) {
      std::uint32_t i0 = prim.indices[i * 3] + static_cast<std::uint32_t>(vert_offset);
      std::uint32_t i1 = prim.indices[i * 3 + 1] + static_cast<std::uint32_t>(vert_offset);
      std::uint32_t i2 = prim.indices[i * 3 + 2] + static_cast<std::uint32_t>(vert_offset);

      ss << "3 " << i0 << " " << i1 << " " << i2 << "\n";
    }
    vert_offset += prim.positions.size() / 3;
  }

  return ss.str();
}

std::vector<std::uint8_t> to_ply_binary(const Scene& scene) {
  std::size_t total_vertices = 0;
  std::size_t total_faces = 0;
  for (const auto& prim : scene.glb_primitives) {
    total_vertices += prim.positions.size() / 3;
    total_faces += prim.indices.size() / 3;
  }

  std::ostringstream ss_hdr;
  ss_hdr.imbue(std::locale::classic());
  ss_hdr << "ply\n";
  ss_hdr << "format binary_little_endian 1.0\n";
  ss_hdr << "comment Created by OpenSKP\n";
  ss_hdr << "element vertex " << total_vertices << "\n";
  ss_hdr << "property float x\n";
  ss_hdr << "property float y\n";
  ss_hdr << "property float z\n";
  ss_hdr << "property float nx\n";
  ss_hdr << "property float ny\n";
  ss_hdr << "property float nz\n";
  ss_hdr << "property float u\n";
  ss_hdr << "property float v\n";
  ss_hdr << "property uchar red\n";
  ss_hdr << "property uchar green\n";
  ss_hdr << "property uchar blue\n";
  ss_hdr << "property uchar alpha\n";
  ss_hdr << "element face " << total_faces << "\n";
  ss_hdr << "property list uchar int vertex_indices\n";
  ss_hdr << "end_header\n";

  std::string header_text = ss_hdr.str();

  std::vector<std::uint8_t> data;
  data.reserve(header_text.size() + total_vertices * 36 + total_faces * 13);

  data.insert(data.end(), header_text.begin(), header_text.end());

  for (const auto& prim : scene.glb_primitives) {
    ColorRgba color = get_material_rgba(scene, prim.material_index);
    std::size_t vert_count = prim.positions.size() / 3;
    for (std::size_t i = 0; i < vert_count; ++i) {
      float px = static_cast<float>(prim.positions[i * 3]);
      float py = static_cast<float>(prim.positions[i * 3 + 1]);
      float pz = static_cast<float>(prim.positions[i * 3 + 2]);

      float nx = (i * 3 < prim.normals.size()) ? static_cast<float>(prim.normals[i * 3]) : 0.0f;
      float ny =
          (i * 3 + 1 < prim.normals.size()) ? static_cast<float>(prim.normals[i * 3 + 1]) : 0.0f;
      float nz =
          (i * 3 + 2 < prim.normals.size()) ? static_cast<float>(prim.normals[i * 3 + 2]) : 0.0f;

      float u = (i * 2 < prim.uvs.size()) ? static_cast<float>(prim.uvs[i * 2]) : 0.0f;
      float v = (i * 2 + 1 < prim.uvs.size()) ? static_cast<float>(prim.uvs[i * 2 + 1]) : 0.0f;

      write_float_le(data, px);
      write_float_le(data, py);
      write_float_le(data, pz);

      write_float_le(data, nx);
      write_float_le(data, ny);
      write_float_le(data, nz);

      write_float_le(data, u);
      write_float_le(data, v);

      data.push_back(color.r);
      data.push_back(color.g);
      data.push_back(color.b);
      data.push_back(color.a);
    }
  }

  std::size_t vert_offset = 0;
  for (const auto& prim : scene.glb_primitives) {
    std::size_t tri_count = prim.indices.size() / 3;
    for (std::size_t i = 0; i < tri_count; ++i) {
      std::int32_t i0 =
          static_cast<std::int32_t>(prim.indices[i * 3] + static_cast<std::uint32_t>(vert_offset));
      std::int32_t i1 = static_cast<std::int32_t>(prim.indices[i * 3 + 1] +
                                                  static_cast<std::uint32_t>(vert_offset));
      std::int32_t i2 = static_cast<std::int32_t>(prim.indices[i * 3 + 2] +
                                                  static_cast<std::uint32_t>(vert_offset));

      data.push_back(3);
      write_int32_le(data, i0);
      write_int32_le(data, i1);
      write_int32_le(data, i2);
    }
    vert_offset += prim.positions.size() / 3;
  }

  return data;
}

void export_ply(const Scene& scene, const std::filesystem::path& path, bool binary) {
  auto parent = path.parent_path();
  if (!parent.empty() && !std::filesystem::exists(parent)) {
    std::filesystem::create_directories(parent);
  }

  if (binary) {
    auto data = to_ply_binary(scene);
    std::ofstream out(path, std::ios::binary);
    if (!out) {
      throw std::runtime_error("Cannot open PLY binary output file: " + path.string());
    }
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
  } else {
    auto text = to_ply_ascii(scene);
    std::ofstream out(path);
    if (!out) {
      throw std::runtime_error("Cannot open PLY ASCII output file: " + path.string());
    }
    out << text;
  }
}

}  // namespace openskp
