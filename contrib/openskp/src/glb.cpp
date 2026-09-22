#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <openskp/glb.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define tinygltf openskp_tinygltf
#include <tiny_gltf.h>
#undef tinygltf

namespace openskp {
namespace {

namespace gltf = openskp_tinygltf;

constexpr std::size_t kGlbLimit = std::numeric_limits<std::uint32_t>::max();

void check_finite(double value, const std::string& field) {
  if (!std::isfinite(value)) throw std::invalid_argument(field + " must be finite");
}

void check_unit_factor(double value, const std::string& field) {
  check_finite(value, field);
  if (value < 0.0 || value > 1.0) {
    throw std::invalid_argument(field + " must be between 0 and 1");
  }
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

void validate_scene(const Scene& scene) {
  constexpr auto kMaxModelEntries = static_cast<std::size_t>(std::numeric_limits<int>::max());
  if (scene.gltf_materials.size() > kMaxModelEntries ||
      scene.glb_primitives.size() > kMaxModelEntries / 3) {
    throw std::length_error("scene has too many GLB model entries");
  }

  for (std::size_t material_index = 0; material_index < scene.gltf_materials.size();
       ++material_index) {
    const auto& pbr = scene.gltf_materials[material_index].pbr_metallic_roughness;
    for (std::size_t channel = 0; channel < pbr.base_color_factor.size(); ++channel) {
      check_unit_factor(pbr.base_color_factor[channel],
                        "material " + std::to_string(material_index) + " base_color_factor[" +
                            std::to_string(channel) + "]");
    }
    check_unit_factor(pbr.metallic_factor,
                      "material " + std::to_string(material_index) + " metallic_factor");
    check_unit_factor(pbr.roughness_factor,
                      "material " + std::to_string(material_index) + " roughness_factor");
  }

  std::size_t estimated_binary_size = 0;
  for (std::size_t primitive_index = 0; primitive_index < scene.glb_primitives.size();
       ++primitive_index) {
    const auto& primitive = scene.glb_primitives[primitive_index];
    const auto prefix = "primitive " + std::to_string(primitive_index) + " ";
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

    for (const auto* values : {&primitive.positions, &primitive.normals, &primitive.uvs}) {
      estimated_binary_size = (estimated_binary_size + 3) & ~std::size_t{3};
      check_binary_growth(estimated_binary_size, values->size(), sizeof(float));
      estimated_binary_size += values->size() * sizeof(float);
    }
    estimated_binary_size = (estimated_binary_size + 3) & ~std::size_t{3};
    check_binary_growth(estimated_binary_size, primitive.indices.size(), sizeof(std::uint32_t));
    estimated_binary_size += primitive.indices.size() * sizeof(std::uint32_t);
  }
}

gltf::Model make_model(const Scene& scene, bool embed_textures) {
  validate_scene(scene);

  gltf::Model model;
  model.asset.version = "2.0";
  model.asset.generator = "OpenSKP";
  model.defaultScene = 0;
  model.scenes.emplace_back();
  model.scenes[0].name = "OpenSKP Scene";

  for (const auto& source : scene.gltf_materials) {
    const auto& source_pbr = source.pbr_metallic_roughness;
    gltf::Material material;
    material.pbrMetallicRoughness.baseColorFactor.assign(source_pbr.base_color_factor.begin(),
                                                         source_pbr.base_color_factor.end());
    material.pbrMetallicRoughness.metallicFactor = source_pbr.metallic_factor;
    material.pbrMetallicRoughness.roughnessFactor = source_pbr.roughness_factor;
    material.doubleSided = source.double_sided;
    // glTF's default alphaMode is OPAQUE, which tells a conformant
    // renderer to ignore alpha entirely - both the material's own opacity
    // and any texture's alpha channel. Genuinely translucent materials
    // (glass, water) need BLEND so baseColorFactor's alpha actually takes
    // effect. A textured-but-otherwise-opaque material gets MASK instead:
    // many SketchUp Warehouse assets (tree foliage, fences, signage) rely
    // on the image's own alpha channel to cut a shape out of an otherwise
    // flat quad, and without MASK a renderer would show the full
    // rectangle. MASK is a no-op for a texture with no real cutout, so
    // this is safe to set unconditionally rather than trying to detect
    // which textures need it.
    if (source_pbr.base_color_factor[3] < 1.0) {
      material.alphaMode = "BLEND";
    } else if (source_pbr.base_color_texture) {
      material.alphaMode = "MASK";
    }
    // baseColorTexture.index defaults to -1 (unset); TinyGLTF omits the
    // key from the serialized JSON in that case, so leaving it untouched
    // when not embedding is the strip - no separate pass needed, unlike
    // the hand-rolled writers in the other languages.
    if (embed_textures && source_pbr.base_color_texture) {
      material.pbrMetallicRoughness.baseColorTexture.index =
          static_cast<int>(*source_pbr.base_color_texture);
    }
    model.materials.push_back(std::move(material));
  }

  if (scene.glb_primitives.empty()) return model;

  model.buffers.emplace_back();
  auto& binary = model.buffers[0].data;
  gltf::Mesh mesh;
  mesh.name = "OpenSKP Mesh";

  for (const auto& source : scene.glb_primitives) {
    const auto position_offset = append_values(
        binary, source.positions, [](ByteBuffer& bytes, float value) { append_f32(bytes, value); });
    const auto position_view =
        add_view(model, position_offset, source.positions.size() * 4, TINYGLTF_TARGET_ARRAY_BUFFER);
    const auto position_accessor = add_accessor(model, position_view, source.positions.size() / 3,
                                                TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3);
    auto& accessor = model.accessors[static_cast<std::size_t>(position_accessor)];
    accessor.minValues = {source.positions[0], source.positions[1], source.positions[2]};
    accessor.maxValues = accessor.minValues;
    for (std::size_t index = 3; index < source.positions.size(); index += 3) {
      for (std::size_t axis = 0; axis < 3; ++axis) {
        accessor.minValues[axis] =
            std::min(accessor.minValues[axis], static_cast<double>(source.positions[index + axis]));
        accessor.maxValues[axis] =
            std::max(accessor.maxValues[axis], static_cast<double>(source.positions[index + axis]));
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
        add_accessor(model, index_view, source.indices.size(), TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
                     TINYGLTF_TYPE_SCALAR);

    gltf::Primitive primitive;
    primitive.attributes["POSITION"] = position_accessor;
    primitive.attributes["NORMAL"] = normal_accessor;
    primitive.attributes["TEXCOORD_0"] = uv_accessor;
    primitive.indices = index_accessor;
    primitive.material = static_cast<int>(source.material_index);
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    mesh.primitives.push_back(std::move(primitive));
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

  model.meshes.push_back(std::move(mesh));
  model.nodes.emplace_back();
  model.nodes[0].name = "OpenSKP Scene";
  model.nodes[0].mesh = 0;
  model.scenes[0].nodes.push_back(0);
  return model;
}

}  // namespace

ByteBuffer to_glb(const Scene& scene, const GlbOptions& options) {
  auto model = make_model(scene, options.textures);
  std::ostringstream stream(std::ios::binary | std::ios::out);
  try {
    gltf::TinyGLTF writer;
    if (!writer.WriteGltfSceneToStream(&model, stream, false, true) || !stream) {
      throw std::runtime_error("TinyGLTF failed to serialize the scene");
    }
  } catch (const std::runtime_error&) {
    throw;
  } catch (const std::exception& error) {
    throw std::runtime_error(std::string("failed to serialize GLB: ") + error.what());
  }

  const auto serialized = stream.str();
  if (serialized.size() > kGlbLimit) {
    throw std::length_error("serialized GLB exceeds its 32-bit file-size limit");
  }
  return ByteBuffer(serialized.begin(), serialized.end());
}

void export_glb(const Scene& scene, const std::filesystem::path& output_path,
                const GlbOptions& options) {
  const auto bytes = to_glb(scene, options);
  std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
  if (!stream) throw std::runtime_error("failed to open GLB output file");
  stream.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
  if (!stream) throw std::runtime_error("failed to write GLB output file");
}

}  // namespace openskp
