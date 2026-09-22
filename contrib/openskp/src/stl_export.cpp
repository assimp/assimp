#include "openskp/stl_export.hpp"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace openskp {

namespace {

struct Vector3 {
  float x, y, z;
};

Vector3 calculate_normal(const Vector3& v0, const Vector3& v1, const Vector3& v2) {
  float e1x = v1.x - v0.x;
  float e1y = v1.y - v0.y;
  float e1z = v1.z - v0.z;

  float e2x = v2.x - v0.x;
  float e2y = v2.y - v0.y;
  float e2z = v2.z - v0.z;

  float nx = e1y * e2z - e1z * e2y;
  float ny = e1z * e2x - e1x * e2z;
  float nz = e1x * e2y - e1y * e2x;

  float len = std::sqrt(nx * nx + ny * ny + nz * nz);
  if (len > 1e-12f) {
    return {nx / len, ny / len, nz / len};
  }
  return {0.0f, 0.0f, 0.0f};
}

void write_float_le(std::vector<std::uint8_t>& buf, float value) {
  std::uint32_t uval;
  std::memcpy(&uval, &value, 4);
  buf.push_back(static_cast<std::uint8_t>(uval & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((uval >> 8) & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((uval >> 16) & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((uval >> 24) & 0xFF));
}

void write_uint16_le(std::vector<std::uint8_t>& buf, std::uint16_t value) {
  buf.push_back(static_cast<std::uint8_t>(value & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

void write_uint32_le(std::vector<std::uint8_t>& buf, std::uint32_t value) {
  buf.push_back(static_cast<std::uint8_t>(value & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
  buf.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
}

}  // namespace

std::string to_stl_ascii(const Scene& scene, float scale) {
  std::ostringstream ss;
  ss.imbue(std::locale::classic());
  ss << "solid OpenSKP_Model\n";

  for (const auto& prim : scene.glb_primitives) {
    std::size_t tri_count = prim.indices.size() / 3;
    for (std::size_t i = 0; i < tri_count; ++i) {
      std::uint32_t i0 = prim.indices[i * 3];
      std::uint32_t i1 = prim.indices[i * 3 + 1];
      std::uint32_t i2 = prim.indices[i * 3 + 2];

      Vector3 v0 = {static_cast<float>(prim.positions[i0 * 3]) * scale,
                    static_cast<float>(prim.positions[i0 * 3 + 1]) * scale,
                    static_cast<float>(prim.positions[i0 * 3 + 2]) * scale};
      Vector3 v1 = {static_cast<float>(prim.positions[i1 * 3]) * scale,
                    static_cast<float>(prim.positions[i1 * 3 + 1]) * scale,
                    static_cast<float>(prim.positions[i1 * 3 + 2]) * scale};
      Vector3 v2 = {static_cast<float>(prim.positions[i2 * 3]) * scale,
                    static_cast<float>(prim.positions[i2 * 3 + 1]) * scale,
                    static_cast<float>(prim.positions[i2 * 3 + 2]) * scale};

      Vector3 n = calculate_normal(v0, v1, v2);

      ss << "  facet normal " << std::fixed << std::setprecision(6) << n.x << " " << n.y << " "
         << n.z << "\n";
      ss << "    outer loop\n";
      ss << "      vertex " << v0.x << " " << v0.y << " " << v0.z << "\n";
      ss << "      vertex " << v1.x << " " << v1.y << " " << v1.z << "\n";
      ss << "      vertex " << v2.x << " " << v2.y << " " << v2.z << "\n";
      ss << "    endloop\n";
      ss << "  endfacet\n";
    }
  }

  ss << "endsolid OpenSKP_Model\n";
  return ss.str();
}

std::vector<std::uint8_t> to_stl_binary(const Scene& scene, float scale) {
  std::uint32_t total_triangles = 0;
  for (const auto& prim : scene.glb_primitives) {
    total_triangles += static_cast<std::uint32_t>(prim.indices.size() / 3);
  }

  std::vector<std::uint8_t> data;
  data.reserve(80 + 4 + total_triangles * 50);

  // 80-byte header
  std::string header = "# OpenSKP Binary STL Export";
  for (std::size_t i = 0; i < 80; ++i) {
    data.push_back(i < header.size() ? static_cast<std::uint8_t>(header[i]) : 0);
  }

  // 4-byte triangle count
  write_uint32_le(data, total_triangles);

  for (const auto& prim : scene.glb_primitives) {
    std::size_t tri_count = prim.indices.size() / 3;
    for (std::size_t i = 0; i < tri_count; ++i) {
      std::uint32_t i0 = prim.indices[i * 3];
      std::uint32_t i1 = prim.indices[i * 3 + 1];
      std::uint32_t i2 = prim.indices[i * 3 + 2];

      Vector3 v0 = {static_cast<float>(prim.positions[i0 * 3]) * scale,
                    static_cast<float>(prim.positions[i0 * 3 + 1]) * scale,
                    static_cast<float>(prim.positions[i0 * 3 + 2]) * scale};
      Vector3 v1 = {static_cast<float>(prim.positions[i1 * 3]) * scale,
                    static_cast<float>(prim.positions[i1 * 3 + 1]) * scale,
                    static_cast<float>(prim.positions[i1 * 3 + 2]) * scale};
      Vector3 v2 = {static_cast<float>(prim.positions[i2 * 3]) * scale,
                    static_cast<float>(prim.positions[i2 * 3 + 1]) * scale,
                    static_cast<float>(prim.positions[i2 * 3 + 2]) * scale};

      Vector3 n = calculate_normal(v0, v1, v2);

      // Normal (3x float)
      write_float_le(data, n.x);
      write_float_le(data, n.y);
      write_float_le(data, n.z);

      // Vertices (9x float)
      write_float_le(data, v0.x);
      write_float_le(data, v0.y);
      write_float_le(data, v0.z);

      write_float_le(data, v1.x);
      write_float_le(data, v1.y);
      write_float_le(data, v1.z);

      write_float_le(data, v2.x);
      write_float_le(data, v2.y);
      write_float_le(data, v2.z);

      // Attribute byte count (uint16)
      write_uint16_le(data, 0);
    }
  }

  return data;
}

void export_stl(const Scene& scene, const std::filesystem::path& path, bool binary, float scale) {
  auto parent = path.parent_path();
  if (!parent.empty() && !std::filesystem::exists(parent)) {
    std::filesystem::create_directories(parent);
  }

  if (binary) {
    auto data = to_stl_binary(scene, scale);
    std::ofstream out(path, std::ios::binary);
    if (!out) {
      throw std::runtime_error("Cannot open STL binary output file: " + path.string());
    }
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
  } else {
    auto text = to_stl_ascii(scene, scale);
    std::ofstream out(path);
    if (!out) {
      throw std::runtime_error("Cannot open STL ASCII output file: " + path.string());
    }
    out << text;
  }
}

}  // namespace openskp
