#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <openskp/instanced_glb.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define tinygltf openskp_tinygltf_instanced
#include <tiny_gltf.h>
#undef tinygltf

namespace openskp {
namespace {

namespace gltf = openskp_tinygltf_instanced;

constexpr std::size_t kGlbLimit = std::numeric_limits<std::uint32_t>::max();

void check_finite(double value, const std::string& field) {
  if (!std::isfinite(value)) throw std::invalid_argument(field + " must be finite");
}

void check_binary_growth(std::size_t current, std::size_t count, std::size_t element_size) {
  if (count > kGlbLimit / element_size || current > kGlbLimit - count * element_size) {
    throw std::length_error("scene geometry exceeds GLB's 32-bit binary-buffer limit");
  }
}

void align_buffer(ByteBuffer& buffer) {
  while (buffer.size() % 4 != 0) {
    if (buffer.size() == kGlbLimit) {
      throw std::length_error("scene geometry exceeds GLB's 32-bit binary-buffer limit");
    }
    buffer.push_back(0);
  }
}

void append_u32(ByteBuffer& buffer, std::uint32_t value) {
  buffer.push_back(static_cast<std::uint8_t>(value));
  buffer.push_back(static_cast<std::uint8_t>(value >> 8));
  buffer.push_back(static_cast<std::uint8_t>(value >> 16));
  buffer.push_back(static_cast<std::uint8_t>(value >> 24));
}

void append_f32(ByteBuffer& buffer, float value) {
  static_assert(sizeof(float) == sizeof(std::uint32_t), "GLB export requires 32-bit floats");
  std::uint32_t bits{};
  std::memcpy(&bits, &value, sizeof(bits));
  append_u32(buffer, bits);
}

template <typename T, typename Append>
std::size_t append_values(ByteBuffer& buffer, const std::vector<T>& values, Append append) {
  align_buffer(buffer);
  check_binary_growth(buffer.size(), values.size(), sizeof(T));
  const auto offset = buffer.size();
  buffer.reserve(buffer.size() + values.size() * sizeof(T));
  for (const auto value : values) append(buffer, value);
  return offset;
}

int add_view(gltf::Model& model, std::size_t offset, std::size_t length, int target) {
  gltf::BufferView view;
  view.buffer = 0;
  view.byteOffset = offset;
  view.byteLength = length;
  view.target = target;
  model.bufferViews.push_back(std::move(view));
  return static_cast<int>(model.bufferViews.size() - 1);
}

int add_accessor(gltf::Model& model, int view, std::size_t count, int component_type, int type) {
  gltf::Accessor accessor;
  accessor.bufferView = view;
  accessor.count = count;
  accessor.componentType = component_type;
  accessor.type = type;
  model.accessors.push_back(std::move(accessor));
  return static_cast<int>(model.accessors.size() - 1);
}

void validate_scene(const InstancedScene& scene) {
  constexpr auto kMaxModelEntries = static_cast<std::size_t>(std::numeric_limits<int>::max());
  if (scene.gltf_materials.size() > kMaxModelEntries) {
    throw std::length_error("scene has too many GLB model entries");
  }

  for (std::size_t material_index = 0; material_index < scene.gltf_materials.size();
       ++material_index) {
    const auto& pbr = scene.gltf_materials[material_index].pbr_metallic_roughness;
    for (std::size_t channel = 0; channel < pbr.base_color_factor.size(); ++channel) {
      const auto value = pbr.base_color_factor[channel];
      check_finite(value, "material " + std::to_string(material_index) + " base_color_factor");
      if (value < 0.0 || value > 1.0) {
        throw std::invalid_argument("material " + std::to_string(material_index) +
                                    " base_color_factor must be between 0 and 1");
      }
    }
    check_finite(pbr.metallic_factor,
                 "material " + std::to_string(material_index) + " metallic_factor");
    check_finite(pbr.roughness_factor,
                 "material " + std::to_string(material_index) + " roughness_factor");
  }

  for (std::size_t resource_index = 0; resource_index < scene.mesh_resources.size();
       ++resource_index) {
    const auto& resource = scene.mesh_resources[resource_index];
    for (std::size_t primitive_index = 0; primitive_index < resource.primitives.size();
         ++primitive_index) {
      const auto& primitive = resource.primitives[primitive_index];
      const auto prefix = "resource " + std::to_string(resource_index) + " primitive " +
                          std::to_string(primitive_index) + " ";
      if (primitive.positions.empty()) {
        throw std::invalid_argument(prefix + "positions must not be empty");
      }
      if (primitive.positions.size() % 3 != 0) {
        throw std::invalid_argument(prefix + "positions must contain complete vec3 values");
      }
      if (primitive.normals.size() != primitive.positions.size()) {
        throw std::invalid_argument(prefix + "normals must match positions");
      }
      if (primitive.uvs.size() != primitive.positions.size() / 3 * 2) {
        throw std::invalid_argument(prefix + "uvs must match positions");
      }
      if (primitive.indices.size() % 3 != 0) {
        throw std::invalid_argument(prefix + "indices must contain complete triangles");
      }
      if (primitive.indices.empty()) {
        throw std::invalid_argument(prefix + "indices must not be empty");
      }
      if (primitive.material_index >= scene.gltf_materials.size()) {
        throw std::invalid_argument(prefix + "references an invalid material");
      }

      const auto vertex_count = primitive.positions.size() / 3;
      for (const auto value : primitive.positions) check_finite(value, prefix + "position");
      for (const auto value : primitive.normals) check_finite(value, prefix + "normal");
      for (const auto value : primitive.uvs) check_finite(value, prefix + "uv");
      for (const auto index : primitive.indices) {
        if (index >= vertex_count) throw std::invalid_argument(prefix + "index is out of range");
      }
    }
  }
}

bool is_identity(const std::array<double, 16>& m) {
  static constexpr std::array<double, 16> kIdentity{
      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
  };
  for (std::size_t i = 0; i < 16; ++i) {
    if (std::abs(m[i] - kIdentity[i]) > 1e-12) return false;
  }
  return true;
}

int emit_node(gltf::Model& model, const InstancedNode& node,
              const std::map<std::string, int>& mesh_index_by_id) {
  gltf::Node gltf_node;
  if (!node.name.empty()) {
    gltf_node.name = node.name;
  } else if (!node.definition_name.empty()) {
    gltf_node.name = node.definition_name;
  }

  // glTF treats an omitted matrix as the identity; writing it out anyway
  // just costs bytes on every node of a large scene.
  if (!is_identity(node.matrix)) {
    gltf_node.matrix.assign(node.matrix.begin(), node.matrix.end());
  }

  if (node.mesh_resource_id) {
    auto found = mesh_index_by_id.find(*node.mesh_resource_id);
    if (found != mesh_index_by_id.end()) gltf_node.mesh = found->second;
  }

  const auto idx = static_cast<int>(model.nodes.size());
  model.nodes.push_back(gltf_node);

  for (const auto& child : node.children) {
    const auto child_idx = emit_node(model, child, mesh_index_by_id);
    model.nodes[static_cast<std::size_t>(idx)].children.push_back(child_idx);
  }
  return idx;
}

gltf::Model make_model(const InstancedScene& scene, bool embed_textures) {
  validate_scene(scene);

  gltf::Model model;
  model.asset.version = "2.0";
  model.asset.generator = "OpenSKP Instanced Exporter";
  model.defaultScene = 0;
  model.scenes.emplace_back();
  model.scenes[0].name = "OpenSKP Instanced Scene";

  for (const auto& source : scene.gltf_materials) {
    const auto& source_pbr = source.pbr_metallic_roughness;
    gltf::Material material;
    material.pbrMetallicRoughness.baseColorFactor.assign(source_pbr.base_color_factor.begin(),
                                                         source_pbr.base_color_factor.end());
    material.pbrMetallicRoughness.metallicFactor = source_pbr.metallic_factor;
    material.pbrMetallicRoughness.roughnessFactor = source_pbr.roughness_factor;
    material.doubleSided = source.double_sided;
    // See glb.cpp's make_model for why: BLEND for genuinely translucent
    // materials, MASK (a safe no-op on an opaque-alpha texture) for
    // textured ones, otherwise glTF's default OPAQUE.
    if (source_pbr.base_color_factor[3] < 1.0) {
      material.alphaMode = "BLEND";
    } else if (source_pbr.base_color_texture) {
      material.alphaMode = "MASK";
    }
    if (embed_textures && source_pbr.base_color_texture) {
      material.pbrMetallicRoughness.baseColorTexture.index =
          static_cast<int>(*source_pbr.base_color_texture);
    }
    model.materials.push_back(std::move(material));
  }

  model.buffers.emplace_back();
  auto& binary = model.buffers[0].data;

  // append_values() below reserves the buffer to exactly its own new
  // required size on every call (one call per primitive per attribute
  // array) - std::vector::reserve() has no obligation to over-allocate,
  // so a size that only ever grows by "just enough" forces a full
  // reallocation-and-copy of everything appended so far, every single
  // time. On a scene with many mesh resources this turns what should be
  // amortized-O(1) appends into O(total resources squared) copying.
  // Reserving the true final size once, upfront, is a pure amortized-cost
  // fix - it changes no output, only how many times the buffer moves.
  {
    std::size_t total_bytes = 0;
    for (const auto& resource : scene.mesh_resources) {
      for (const auto& source : resource.primitives) {
        total_bytes += source.positions.size() * 4 + source.normals.size() * 4 +
                       source.uvs.size() * 4 + source.indices.size() * 4;
      }
    }
    binary.reserve(total_bytes);
  }

  std::map<std::string, int> mesh_index_by_id;

  for (const auto& resource : scene.mesh_resources) {
    gltf::Mesh mesh;
    mesh.name = resource.definition_name.empty() ? resource.id : resource.definition_name;

    for (const auto& source : resource.primitives) {
      const auto position_offset =
          append_values(binary, source.positions,
                        [](ByteBuffer& bytes, float value) { append_f32(bytes, value); });
      const auto position_view = add_view(model, position_offset, source.positions.size() * 4,
                                          TINYGLTF_TARGET_ARRAY_BUFFER);
      const auto position_accessor =
          add_accessor(model, position_view, source.positions.size() / 3,
                       TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3);
      auto& accessor = model.accessors[static_cast<std::size_t>(position_accessor)];
      accessor.minValues = {source.positions[0], source.positions[1], source.positions[2]};
      accessor.maxValues = accessor.minValues;
      for (std::size_t index = 3; index < source.positions.size(); index += 3) {
        for (std::size_t axis = 0; axis < 3; ++axis) {
          accessor.minValues[axis] = std::min(accessor.minValues[axis],
                                              static_cast<double>(source.positions[index + axis]));
          accessor.maxValues[axis] = std::max(accessor.maxValues[axis],
                                              static_cast<double>(source.positions[index + axis]));
        }
      }

      const auto normal_offset = append_values(
          binary, source.normals, [](ByteBuffer& bytes, float value) { append_f32(bytes, value); });
      const auto normal_view =
          add_view(model, normal_offset, source.normals.size() * 4, TINYGLTF_TARGET_ARRAY_BUFFER);
      const auto normal_accessor = add_accessor(model, normal_view, source.normals.size() / 3,
                                                TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3);

      const auto uv_offset = append_values(
          binary, source.uvs, [](ByteBuffer& bytes, float value) { append_f32(bytes, value); });
      const auto uv_view =
          add_view(model, uv_offset, source.uvs.size() * 4, TINYGLTF_TARGET_ARRAY_BUFFER);
      const auto uv_accessor = add_accessor(model, uv_view, source.uvs.size() / 2,
                                            TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC2);

      const auto index_offset =
          append_values(binary, source.indices,
                        [](ByteBuffer& bytes, std::uint32_t value) { append_u32(bytes, value); });
      const auto index_view = add_view(model, index_offset, source.indices.size() * 4,
                                       TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);
      const auto index_accessor =
          add_accessor(model, index_view, source.indices.size(),
                       TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, TINYGLTF_TYPE_SCALAR);

      gltf::Primitive primitive;
      primitive.attributes["POSITION"] = position_accessor;
      primitive.attributes["NORMAL"] = normal_accessor;
      primitive.attributes["TEXCOORD_0"] = uv_accessor;
      primitive.indices = index_accessor;
      primitive.material = static_cast<int>(source.material_index);
      primitive.mode = TINYGLTF_MODE_TRIANGLES;
      mesh.primitives.push_back(std::move(primitive));
    }

    if (mesh.primitives.empty()) continue;
    mesh_index_by_id[resource.id] = static_cast<int>(model.meshes.size());
    model.meshes.push_back(std::move(mesh));
  }

  if (embed_textures && !scene.textures.empty()) {
    model.samplers.emplace_back();
    model.samplers[0].wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
    model.samplers[0].wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;

    for (const auto& tex : scene.textures) {
      const auto image_offset = append_values(
          binary, tex.data, [](ByteBuffer& bytes, std::uint8_t value) { bytes.push_back(value); });

      // Not add_view(): that helper always sets BufferView::target, but an
      // image bufferView must leave target unset (glTF's target enum only
      // covers vertex/index buffers - 0 is not a valid value, and TinyGLTF
      // only omits the key from the output when target is its -1 default).
      gltf::BufferView view;
      view.buffer = 0;
      view.byteOffset = image_offset;
      view.byteLength = tex.data.size();
      model.bufferViews.push_back(std::move(view));
      const auto image_view = static_cast<int>(model.bufferViews.size() - 1);

      gltf::Image image;
      image.mimeType = tex.mime_type;
      image.bufferView = image_view;
      model.images.push_back(std::move(image));

      gltf::Texture texture;
      texture.sampler = 0;
      texture.source = static_cast<int>(model.images.size() - 1);
      model.textures.push_back(std::move(texture));
    }
  }

  const auto root_idx = emit_node(model, scene.scene_hierarchy, mesh_index_by_id);
  model.scenes[0].nodes.push_back(root_idx);

  return model;
}

}  // namespace

ByteBuffer to_instanced_glb(const InstancedScene& scene, const InstancedGlbOptions& options) {
  auto model = make_model(scene, options.textures);
  std::ostringstream stream(std::ios::binary | std::ios::out);
  try {
    gltf::TinyGLTF writer;
    if (!writer.WriteGltfSceneToStream(&model, stream, false, true) || !stream) {
      throw std::runtime_error("TinyGLTF failed to serialize the instanced scene");
    }
  } catch (const std::runtime_error&) {
    throw;
  } catch (const std::exception& error) {
    throw std::runtime_error(std::string("failed to serialize instanced GLB: ") + error.what());
  }

  const auto serialized = stream.str();
  if (serialized.size() > kGlbLimit) {
    throw std::length_error("serialized GLB exceeds its 32-bit file-size limit");
  }
  return ByteBuffer(serialized.begin(), serialized.end());
}

void export_instanced_glb(const InstancedScene& scene, const std::filesystem::path& output_path,
                          const InstancedGlbOptions& options) {
  const auto bytes = to_instanced_glb(scene, options);
  std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
  if (!stream) throw std::runtime_error("failed to open GLB output file");
  stream.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
  if (!stream) throw std::runtime_error("failed to write GLB output file");
}

}  // namespace openskp
