// SPDX-License-Identifier: Apache 2.0
// Copyright 2022 - 2023, Syoyo Fujita.
// Copyright 2023 - Present, Light Transport Entertainment Inc.
//
// Render data structure suited for WebGL and Raytracing render

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "asset-resolution.hh"
#include "nonstd/expected.hpp"
#include "usdShade.hh"
#include "value-types.hh"

namespace tinyusdz {

// forward decl
class Stage;
class Prim;
struct Material;
struct GeomMesh;
struct Xform;
struct AssetInfo;
class Path;

struct UsdPreviewSurface;
struct UsdUVTexture;

template <typename T>
struct UsdPrimvarReader;

using UsdPrimvarReader_int = UsdPrimvarReader<int>;
using UsdPrimvarReader_float = UsdPrimvarReader<float>;
using UsdPrimvarReader_float3 = UsdPrimvarReader<value::float3>;
using UsdPrimvarReader_float3 = UsdPrimvarReader<value::float3>;
using UsdPrimvarReader_string = UsdPrimvarReader<std::string>;
using UsdPrimvarReader_matrix4d = UsdPrimvarReader<value::matrix4d>;

namespace tydra {

// GLSL like data types
using vec2 = value::float2;
using vec3 = value::float3;
using vec4 = value::float4;
using quat = value::float4;
using mat2 = value::matrix2f;   // float precision
using mat3 = value::matrix3f;   // float precision
using mat4 = value::matrix4f;   // float precision
using dmat4 = value::matrix4d;  // float precision

// Simple string <-> id map
struct StringAndIdMap {
  void add(uint64_t key, const std::string &val) {
    _i_to_s[key] = val;
    _s_to_i[val] = key;
  }

  void add(const std::string &key, uint64_t val) {
    _s_to_i[key] = val;
    _i_to_s[val] = key;
  }

  size_t count(uint64_t i) const { return _i_to_s.count(i); }

  size_t count(const std::string &s) const { return _s_to_i.count(s); }

  std::string at(uint64_t i) const { return _i_to_s.at(i); }

  uint64_t at(std::string s) const { return _s_to_i.at(s); }

  std::map<uint64_t, std::string>::const_iterator find(uint64_t key) const {
    return _i_to_s.find(key);
  }

  std::map<std::string, uint64_t>::const_iterator find(
      const std::string &key) const {
    return _s_to_i.find(key);
  }

  std::map<std::string, uint64_t>::const_iterator s_begin() const {
    return _s_to_i.begin();
  }

  std::map<std::string, uint64_t>::const_iterator s_end() const {
    return _s_to_i.end();
  }

  std::map<uint64_t, std::string>::const_iterator i_begin() const {
    return _i_to_s.begin();
  }

  std::map<uint64_t, std::string>::const_iterator i_end() const {
    return _i_to_s.end();
  }

  size_t size() const {
    // size should be same, but just in case.
    if (_i_to_s.size() == _s_to_i.size()) {
      return _i_to_s.size();
    }

    return 0;
  }

  std::map<uint64_t, std::string> _i_to_s;  // index -> string
  std::map<std::string, uint64_t> _s_to_i;  // string -> index
};

// timeSamples in USD
// TODO: AttributeBlock support
template <typename T>
struct AnimationSample {
  float t{0.0};  // time is represented as float
  T value;
};

enum class VertexVariability {
  Constant,  // one value for all geometric elements
  Uniform,   // one value for each geometric elements(e.g. `face`, `UV patch`)
  Varying,   // per-vertex for each geometric elements. Bilinear interpolation.
  Vertex,  // Equvalent to `Varying` for Polygon mesh. The basis function of the
           // surface is used for the interpolation(Curves, Subdivision Surface,
           // etc).
  FaceVarying,  // per-Vertex per face. Bilinear interpolation.
  Indexed,      // Need to supply index buffer
};

std::string to_string(VertexVariability variability);

// Geometric, light and camera
enum class NodeType {
  Xform,
  Mesh,  // Polygon mesh
  PointLight,
  DomeLight,
  Camera,
  // TODO...
};

enum class ComponentType {
  UInt8,
  Int8,
  UInt16,
  Int16,
  UInt32,
  Int32,
  Half,
  Float,
  Double,
};

std::string to_string(ComponentType ty);

// glTF-like BufferData
struct BufferData {
  ComponentType componentType{ComponentType::UInt8};
  uint8_t count{1};           // up to 256
  std::vector<uint8_t> data;  // binary data

  // TODO: Stride
};

// glTF-like Attribute
struct Attribute {
  std::string path;     // Path string in Stage
  uint32_t slot_id{0};  // slot ID.

  int64_t buffer_id{-1};  // index to buffer_id
};

// Compound of ComponentType x component
enum class VertexAttributeFormat {
  Bool,     // bool(1 byte)
  Char,     // int8
  Char2,    // int8x2
  Char3,    // int8x3
  Char4,    // int8x4
  Byte,     // uint8
  Byte2,    // uint8x2
  Byte3,    // uint8x3
  Byte4,    // uint8x4
  Short,    // int16
  Short2,   // int16x2
  Short3,   // int16x2
  Short4,   // int16x2
  Ushort,   // uint16
  Ushort2,  // uint16x2
  Ushort3,  // uint16x2
  Ushort4,  // uint16x2
  Half,     // half
  Half2,    // half2
  Half3,    // half3
  Half4,    // half4
  Float,    // float
  Vec2,     // float2
  Vec3,     // float3
  Vec4,     // float4
  Int,      // int
  Ivec2,    // int2
  Ivec3,    // int3
  Ivec4,    // int4
  Uint,     // uint
  Uvec2,    // uint2
  Uvec3,    // uint3
  Uvec4,    // uint4
  Double,   // double
  Dvec2,    // double2
  Dvec3,    // double3
  Dvec4,    // double4
  Mat2,     // float 2x2
  Mat3,     // float 3x3
  Mat4,     // float 4x4
  Dmat2,    // double 2x2
  Dmat3,    // double 3x3
  Dmat4,    // double 4x4
};

static size_t VertexAttributeFormatSize(VertexAttributeFormat f) {
  size_t elemsize{0};

  switch (f) {
    case VertexAttributeFormat::Bool: {
      elemsize = 1;
      break;
    }
    case VertexAttributeFormat::Char: {
      elemsize = 1;
      break;
    }
    case VertexAttributeFormat::Char2: {
      elemsize = 2;
      break;
    }
    case VertexAttributeFormat::Char3: {
      elemsize = 3;
      break;
    }
    case VertexAttributeFormat::Char4: {
      elemsize = 4;
      break;
    }
    case VertexAttributeFormat::Byte: {
      elemsize = 1;
      break;
    }
    case VertexAttributeFormat::Byte2: {
      elemsize = 2;
      break;
    }
    case VertexAttributeFormat::Byte3: {
      elemsize = 3;
      break;
    }
    case VertexAttributeFormat::Byte4: {
      elemsize = 4;
      break;
    }
    case VertexAttributeFormat::Short: {
      elemsize = 2;
      break;
    }
    case VertexAttributeFormat::Short2: {
      elemsize = 4;
      break;
    }
    case VertexAttributeFormat::Short3: {
      elemsize = 6;
      break;
    }
    case VertexAttributeFormat::Short4: {
      elemsize = 8;
      break;
    }
    case VertexAttributeFormat::Ushort: {
      elemsize = 2;
      break;
    }
    case VertexAttributeFormat::Ushort2: {
      elemsize = 4;
      break;
    }
    case VertexAttributeFormat::Ushort3: {
      elemsize = 6;
      break;
    }
    case VertexAttributeFormat::Ushort4: {
      elemsize = 8;
      break;
    }
    case VertexAttributeFormat::Half: {
      elemsize = 2;
      break;
    }
    case VertexAttributeFormat::Half2: {
      elemsize = 4;
      break;
    }
    case VertexAttributeFormat::Half3: {
      elemsize = 6;
      break;
    }
    case VertexAttributeFormat::Half4: {
      elemsize = 8;
      break;
    }
    case VertexAttributeFormat::Mat2: {
      elemsize = 4 * 4;
      break;
    }
    case VertexAttributeFormat::Mat3: {
      elemsize = 4 * 9;
      break;
    }
    case VertexAttributeFormat::Mat4: {
      elemsize = 4 * 16;
      break;
    }
    case VertexAttributeFormat::Dmat2: {
      elemsize = 8 * 4;
      break;
    }
    case VertexAttributeFormat::Dmat3: {
      elemsize = 8 * 9;
      break;
    }
    case VertexAttributeFormat::Dmat4: {
      elemsize = 8 * 16;
      break;
    }
    case VertexAttributeFormat::Float: {
      elemsize = 4;
      break;
    }
    case VertexAttributeFormat::Vec2: {
      elemsize = sizeof(float) * 2;
      break;
    }
    case VertexAttributeFormat::Vec3: {
      elemsize = sizeof(float) * 3;
      break;
    }
    case VertexAttributeFormat::Vec4: {
      elemsize = sizeof(float) * 4;
      break;
    }
    case VertexAttributeFormat::Int: {
      elemsize = 4;
      break;
    }
    case VertexAttributeFormat::Ivec2: {
      elemsize = sizeof(int) * 2;
      break;
    }
    case VertexAttributeFormat::Ivec3: {
      elemsize = sizeof(int) * 3;
      break;
    }
    case VertexAttributeFormat::Ivec4: {
      elemsize = sizeof(int) * 4;
      break;
    }
    case VertexAttributeFormat::Uint: {
      elemsize = 4;
      break;
    }
    case VertexAttributeFormat::Uvec2: {
      elemsize = sizeof(uint32_t) * 2;
      break;
    }
    case VertexAttributeFormat::Uvec3: {
      elemsize = sizeof(uint32_t) * 3;
      break;
    }
    case VertexAttributeFormat::Uvec4: {
      elemsize = sizeof(uint32_t) * 4;
      break;
    }
    case VertexAttributeFormat::Double: {
      elemsize = sizeof(double);
      break;
    }
    case VertexAttributeFormat::Dvec2: {
      elemsize = sizeof(double) * 2;
      break;
    }
    case VertexAttributeFormat::Dvec3: {
      elemsize = sizeof(double) * 3;
      break;
    }
    case VertexAttributeFormat::Dvec4: {
      elemsize = sizeof(double) * 4;
      break;
    }
  }

  return elemsize;
}

std::string to_string(VertexAttributeFormat f);

///
/// Vertex attribute array. Stores raw vertex attribute data.
///
/// arrayLength = elementSize * vertexCount
/// arrayBytes = formatSize * elementSize * vertexCount
///
/// Example:
///    positions(float3, elementSize=1, n=2): [1.0, 1.1, 1.2,  0.4, 0.3, 0.2]
///    skinWeights(float, elementSize=4, n=2): [1.0, 1.0, 1.0, 1.0,  0.5, 0.5,
///    0.5, 0.5]
///
struct VertexAttribute {
  VertexAttributeFormat format{VertexAttributeFormat::Vec3};
  uint32_t elementSize{1};  // `elementSize` in USD terminology(i.e. # of
                            // samples per vertex data)
  uint32_t stride{
      0};  //  We don't support packed(interleaved) vertex data, so stride is
           //  usually sizeof(VertexAttributeFormat) * elementSize. 0 = tightly
           //  packed. Let app/gfx API decide actual stride bytes.
  std::vector<uint8_t> data;  // raw binary data(TODO: Use Buffer ID?)
  std::vector<uint32_t>
      indices;  // Dedicated Index buffer. Set when variability == Indexed.
                // empty = Use vertex index buffer
  VertexVariability variability{VertexVariability::FaceVarying};
  uint64_t handle{0};  // Handle ID for Graphics API. 0 = invalid

  // Returns the number of vertex items.
  // We use compound type for the format, so this returns 1 when the buffer is
  // composed of 3 floats and `format` is float3 for example.
  size_t vertex_count() const {
    if (stride != 0) {
      // TODO: return 0 when (data.size() % stride) != 0?
      return data.size() / stride;
    }

    size_t itemSize = stride_bytes();

    if ((data.size() % itemSize) != 0) {
      // data size mismatch
      return 0;
    }

    return data.size() / itemSize;
  }

  size_t num_bytes() const { return data.size(); }

  const void *buffer() const {
    return reinterpret_cast<const void *>(data.data());
  }

  const std::vector<uint8_t> &get_data() const { return data; }

  std::vector<uint8_t> &get_data() { return data; }

  //
  // Bytes for each vertex data: formatSize * elementSize
  //
  size_t stride_bytes() const {
    if (stride != 0) {
      return stride;
    }

    return element_size() * VertexAttributeFormatSize(format);
  }

  size_t element_size() const { return elementSize; }

  size_t format_size() const { return VertexAttributeFormatSize(format); }
};

#if 0  // TODO: Implement
///
/// Flatten(expand by vertexCounts and vertexIndices) VertexAttribute.
///
/// @param[in] src Input VertexAttribute.
/// @param[in] faceVertexCounts Array of faceVertex counts.
/// @param[in] faceVertexIndices Array of faceVertex indices.
/// @param[out] dst flattened VertexAttribute data.
/// @param[out] itemCount # of vertex items = dst.size() / src.stride_bytes().
///
static bool FlattenVertexAttribute(
    const VertexAttribute &src,
    const std::vector<uint32_t> &faceVertexCounts,
    const std::vector<uint32_t> &faceVertexIndices,
    std::vector<uint8_t> &dst,
    size_t &itemCount);
#else

#if 0  // TODO: Implement
///
/// Convert variability of `src` VertexAttribute to "facevarying".
///
/// @param[in] src Input VertexAttribute.
/// @param[in] faceVertexCounts  # of vertex per face. When the size is empty
/// and faceVertexIndices is not empty, treat `faceVertexIndices` as
/// triangulated mesh indices.
/// @param[in] faceVertexIndices
/// @param[out] dst VertexAttribute with facevarying variability. `dst.vertex_count()` become `sum(faceVertexCounts)`
///
static bool ToFacevaringVertexAttribute(
    const VertexAttribute &src, VertexAttribute &dst,
    const std::vector<uint32_t> &faceVertexCounts,
    const std::vector<uint32_t> &faceVertexIndices);
#endif
#endif

//
// Convert PrimVar(type-erased value) to typed VertexAttribute
//

enum class ColorSpace {
  sRGB,
  Linear,
  Rec709,
  OCIO,
  Lin_DisplayP3, // colorSpace 'lin_displayp3'
  sRGB_DisplayP3, // colorSpace 'srgb_displayp3'
  Custom,  // TODO: Custom colorspace
};

std::string to_string(ColorSpace cs);

bool from_token(const value::token &tok, ColorSpace *result);

struct TextureImage {
  std::string asset_identifier;  // (resolved) filename or asset identifier.

  ComponentType texelComponentType{
      ComponentType::UInt8};  // texel bit depth of `buffer_id`
  ComponentType assetTexelComponentType{
      ComponentType::UInt8};  // texel bit depth of UsdUVTexture asset

  ColorSpace colorSpace{ColorSpace::sRGB};  // color space of texel data.
  ColorSpace usdColorSpace{
      ColorSpace::sRGB};  // original color space info in UsdUVTexture

  int32_t width{-1};
  int32_t height{-1};
  int32_t channels{-1};  // e.g. 3 for RGB.
  int32_t miplevel{0};

  int64_t buffer_id{-1};  // index to buffer_id(texel data)

  uint64_t handle{0};  // Handle ID for Graphics API. 0 = invalid
};

// glTF-lie animation data

template <typename T>
struct AnimationSampler {
  std::vector<AnimationSample<T>> samples;

  // No cubicSpline
  enum class Interpolation {
    Linear,
    Step,  // Held in USD
  };

  Interpolation interpolation{Interpolation::Linear};
};

struct AnimationChannel {
  enum class ChannelType { Transform, Translation, Rotation, Scale };

  // Matrix precision is recuded to float-precision
  // NOTE: transform is not supported in glTF(you need to decompose transform
  // matrix into TRS)
  AnimationSampler<mat4> transforms;

  // half-types are upcasted to float precision
  AnimationSampler<vec3> translations;
  AnimationSampler<quat> rotations;  // Rotation is converted to quaternions
  AnimationSampler<vec3> scales;

  int64_t taget_node{-1};  // array index to RenderScene::nodes
};

struct Animation {
  std::string path;  // USD Prim path
  std::vector<AnimationChannel> channels;
};

struct Node {
  NodeType nodeType{NodeType::Xform};

  int32_t id;  // Index to node content(e.g. meshes[id] when nodeTypes == Mesh

  std::vector<uint32_t> children;

  // Every node have its transform at `default` timecode.
  value::matrix4d local_matrix;
  value::matrix4d global_matrix;

  uint64_t handle{0};  // Handle ID for Graphics API. 0 = invalid
};

// Currently normals and texcoords are converted as facevarying attribute.
struct RenderMesh {
  std::string element_name;  // element(leaf) Prim name
  std::string abs_name;      // absolute Prim path in USD

  // TODO: Support half-precision and double-precision.
  std::vector<vec3> points;
  std::vector<uint32_t> faceVertexIndices;
  // For triangulated mesh, array elements are all filled with 3 or
  // faceVertexCounts.size() == 0.
  std::vector<uint32_t> faceVertexCounts;

  // `normals` or `primvar:normals`. Empty when no normals exist in the
  // GeomMesh.
  std::vector<vec3> facevaryingNormals;
  Interpolation normalsInterpolation;  // Optional info. USD interpolation for
                                       // `facevaryingNormals`

  // key = slot ID. Usually 0 = primary
  // vec2(texCoord2f) only
  // TODO: Interpolation for UV?
  std::unordered_map<uint32_t, std::vector<vec2>> facevaryingTexcoords;
  StringAndIdMap texcoordSlotIdMap;  // st primvarname to slotID map

  //
  // tangents and binormals(single-frame only)
  //
  // When `normals`(or `normals` primvar) is not present in the GeomMesh,
  // tangents and normals are not computed.
  //
  // When `normals` is supplied, but no `tangents` and `binormals` are supplied,
  // Tydra computes it based on:
  // https://learnopengl.com/Advanced-Lighting/Normal-Mapping (when
  // MeshConverterConfig::compute_tangents_and_binormals is set to `true`)
  //
  // For UsdPreviewSurface, geom primvar name of `tangents` and `binormals` are
  // read from Material's inputs::frame:tangentsPrimvarName(default "tangents"),
  // inputs::frame::binormalsPrimvarName(default "binormals")
  // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
  //
  std::vector<vec3> facevaryingTangents;
  std::vector<vec3> facevaryingBinormals;

  std::vector<int32_t>
      materialIds;  // per-face material. -1 = no material assigned

  //
  // Primvars(User defined attributes).
  // VertexAttribute preserves input USD primvar variability(interpolation)
  // (e.g. skinWeight primvar has 'vertex' variability)
  //
  // This primvars excludes `st`, `tangents` and `binormals`(referenced by
  // UsdPrimvarReader)
  //
  std::map<uint32_t, VertexAttribute> primvars;

  // Index value = key to `primvars`
  StringAndIdMap primvarsMap;

  uint64_t handle{0};  // Handle ID for Graphics API. 0 = invalid
};

enum class UVReaderFloatComponentType {
  COMPONENT_FLOAT,
  COMPONENT_FLOAT2,
  COMPONENT_FLOAT3,
  COMPONENT_FLOAT4,
};

std::string to_string(UVReaderFloatComponentType ty);

// float, float2, float3 or float4 only
struct UVReaderFloat {
  UVReaderFloatComponentType componentType{
      UVReaderFloatComponentType::COMPONENT_FLOAT2};
  int64_t mesh_id{-1};   // index to RenderMesh
  int64_t coord_id{-1};  // index to RenderMesh::facevaryingTexcoords

  // mat2 transform; // UsdTransform2d

  // Returns interpolated UV coordinate with UV transform
  // # of components filled are equal to `componentType`.
  vec4 fetchUV(size_t faceId, float varyu, float varyv);
};

struct UVTexture {
  // NOTE: it looks no 'rgba' in UsdUvTexture
  enum class Channel { R, G, B, A, RGB, RGBA };

  // TextureWrap `black` in UsdUVTexture is mapped to `CLAMP_TO_BORDER`(app must
  // set border color to black) default is CLAMP_TO_EDGE and `useMetadata` wrap
  // mode is ignored.
  enum class WrapMode { CLAMP_TO_EDGE, REPEAT, MIRROR, CLAMP_TO_BORDER };

  WrapMode wrapS{WrapMode::CLAMP_TO_EDGE};
  WrapMode wrapT{WrapMode::CLAMP_TO_EDGE};

  // Do CPU texture mapping. For baking texels with transform, texturing in
  // raytracer(bake lighting), etc.
  //
  // This method accounts for `tranform` and `bias/scale`
  //
  // NOTE: for R, G, B channel, The value is replicated to output[0], output[1]
  // and output[2]. For A channel, The value is returned to output[3]
  vec4 fetch_uv(size_t faceId, float varyu, float varyv);

  // `fetch_uv` with user-specified channel. `outputChannel` is ignored.
  vec4 fetch_uv_channel(size_t faceId, float varyu, float varyv, Channel channel);

  // UVW version of `fetch_uv`.
  vec4 fetch_uvw(size_t faceId, float varyu, float varyv, float varyw);
  vec4 fetch_uvw_channel(size_t faceId, float varyu, float varyv, float varyw, Channel channel);

  // output channel info
  Channel outputChannel{Channel::RGB};

  // bias and scale for texel value
  vec4 bias{0.0f, 0.0f, 0.0f, 0.0f};
  vec4 scale{1.0f, 1.0f, 1.0f, 1.0f};

  UVReaderFloat uvreader;
  vec4 fallback_uv{0.0f, 0.0f, 0.0f, 0.0f};

  // UsdTransform2d
  // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_texture_transform
  // = scale * rotate + translation
  bool has_transform2d{false};  // true = `transform`, `tx_rotation`, `tx_scale`
                                // and `tx_translation` are filled;
  mat3 transform{value::matrix3f::identity()};

  // raw transform2d value
  float tx_rotation{0.0f};
  vec2 tx_scale{1.0f, 1.0f};
  vec2 tx_translation{0.0f, 0.0f};

  // UV primvars name(UsdPrimvarReader's inputs:varname)
  std::string varname_uv;

  int64_t texture_image_id{-1};  // Index to TextureImage
  uint64_t handle{0};            // Handle ID for Graphics API. 0 = invalid
};

std::string to_string(UVTexture::WrapMode ty);

struct UDIMTexture {
  enum class Channel { R, G, B, RGB, RGBA };

  // NOTE: for single channel(e.g. R) fetch, Only [0] will be filled for the
  // return value.
  vec4 fetch(size_t faceId, float varyu, float varyv, float varyw = 1.0f,
             Channel channel = Channel::RGB);

  // key = UDIM id(e.g. 1001)
  std::unordered_map<uint32_t, int32_t> imageTileIds;
};

// T or TextureId
template <typename T>
struct ShaderParam {
  ShaderParam(const T &t) { value = t; }

  bool is_texture() const { return textureId >= 0; }

  template <typename STy>
  void set_value(const STy &val) {
    // Currently we assume T == Sty.
    // TODO: support more type variant
    static_assert(value::TypeTraits<T>::underlying_type_id() ==
                      value::TypeTraits<STy>::underlying_type_id(),
                  "");
    static_assert(sizeof(T) >= sizeof(STy), "");
    memcpy(&value, &val, sizeof(T));
  }

  T value;
  int32_t textureId{-1};  // negative = invalid
};

// UsdPreviewSurface
struct PreviewSurfaceShader {
  bool useSpecularWorkFlow{false};

  ShaderParam<vec3> diffuseColor{{0.18f, 0.18f, 0.18f}};
  ShaderParam<vec3> emissiveColor{{0.0f, 0.0f, 0.0f}};
  ShaderParam<vec3> specularColor{{0.0f, 0.0f, 0.0f}};
  ShaderParam<float> metallic{0.0f};
  ShaderParam<float> roughness{0.5f};
  ShaderParam<float> clearcoat{0.0f};
  ShaderParam<float> clearcoatRoughness{0.01f};
  ShaderParam<float> opacity{1.0f};
  ShaderParam<float> opacityThreshold{0.0f};
  ShaderParam<float> ior{1.5f};
  ShaderParam<vec3> normal{{0.0f, 0.0f, 1.0f}};
  ShaderParam<float> displacement{0.0f};
  ShaderParam<float> occlusion{0.0f};

  uint64_t handle{0};  // Handle ID for Graphics API. 0 = invalid
};

// Material + Shader
struct RenderMaterial {
  std::string name;  // elementName in USD (e.g. "pbrMat")
  std::string
      abs_path;  // abosolute Prim path in USD (e.g. "/_material/scope/pbrMat")

  PreviewSurfaceShader surfaceShader;
  // TODO: displacement, volume.

  uint64_t handle{0};  // Handle ID for Graphics API. 0 = invalid
};

// Simple glTF-like Scene Graph
class RenderScene {
 public:
  std::vector<Node> nodes;  // Prims in USD
  std::vector<TextureImage> images;
  std::vector<RenderMaterial> materials;
  std::vector<UVTexture> textures;
  std::vector<RenderMesh> meshes;
  std::vector<Animation> animations;
  std::vector<BufferData>
      buffers;  // Various data storage(e.g. primvar texcoords)

  // int64_t default_root_node{-1}; // index to `nodes`. `defaultPrim` in USD
};

///
/// Texture image loader callback
///
/// The callback function should return TextureImage and Raw image data.
///
/// NOTE: TextureImage::buffer_id is filled in Tydra side after calling this
/// callback. NOTE: TextureImage::colorSpace will be overwritten if
/// `asset:sourceColorSpace` is authored in UsdUVTexture.
///
/// @param[in] asset Asset path
/// @param[in] assetInfo AssetInfo
/// @param[in] assetResolver AssetResolutionResolver context. Please pass
/// DefaultAssetResolutionResolver() if you don't have custom
/// AssetResolutionResolver.
/// @param[out] texImageOut TextureImage info.
/// @param[out] imageData Raw texture image data.
/// @param[inout] userdata User data.
/// @param[out] warn Optional. Warning message.
/// @param[out] error Optional. Error message.
///
/// @return true upon success.
/// termination of visiting Prims.
///
typedef bool (*TextureImageLoaderFunction)(
    const value::AssetPath &assetPath, const AssetInfo &assetInfo,
    AssetResolutionResolver &assetResolver, TextureImage *imageOut,
    std::vector<uint8_t> *imageData, void *userdata, std::string *warn,
    std::string *err);

bool DefaultTextureImageLoaderFunction(const value::AssetPath &assetPath,
                                       const AssetInfo &assetInfo,
                                       AssetResolutionResolver &assetResolver,
                                       TextureImage *imageOut,
                                       std::vector<uint8_t> *imageData,
                                       void *userdata, std::string *warn,
                                       std::string *err);

///
/// TODO: UDIM loder
///

#if 0  // TODO: remove
///
/// Easy API to convert USD Stage to RenderScene(glTF-like scene graph)
///
bool ConvertToRenderScene(
  const Stage &stage,
  RenderScene *scene,
  std::string *warn,
  std::string *err);

nonstd::expected<Node, std::string> Convert(const Stage &stage,
                                                  const Xform &xform);

nonstd::expected<RenderMesh, std::string> Convert(const Stage &stage,
                                                  const GeomMesh &mesh, bool triangulate=false);

// Currently float2 only
std::vector<UsdPrimvarReader_float2> ExtractPrimvarReadersFromMaterialNode(const Prim &node);
#endif

struct MeshConverterConfig {
  bool triangulate{true};
  bool compute_tangents_and_binormals{true};
};

struct MaterialConverterConfig {
  // DefaultTextureImageLoader will be used when nullptr;
  TextureImageLoaderFunction texture_image_loader_function{nullptr};
  void *texture_image_loader_function_userdata{nullptr};

  // For UsdUVTexture.
  //
  // Default configuration:
  //
  // - The converter converts 8bit texture to floating point image and texel
  // data is converted to linear space.
  // - Allow missing asset(texture) and asset load failure.
  //
  // Recommended configuration for mobile/WebGL
  //
  // - `preserve_texel_bitdepth` true
  // - `linearize_color_space` true
  //   - No sRGB -> Linear conversion in a shader

  // In the UsdUVTexture spec, 8bit texture image is converted to floating point
  // image of range `[0.0, 1.0]`. When this flag is set to false, 8bit and 16bit
  // texture image is converted to floating point image. When this flag is set
  // to true, 8bit and 16bit texture data is stored as-is to save memory usage.
  // Setting true is good if you want to render USD scene on mobile, WebGL, etc.
  bool preserve_texel_bitdepth{false};

  // Apply the inverse of a color space to make texture image in linear space.
  // When `preserve_texel_bitdepth` is set to true, linearization also preserse
  // texel bit depth (i.e, for 8bit sRGB image, 8bit linear-space image is
  // produced)
  bool linearize_color_space{false};

  // Allow asset(texture, shader, etc) path with Windows backslashes(e.g.
  // ".\textures\cat.png")? When true, convert it to forward slash('/') on
  // Posixish system.
  bool allow_backslash_in_asset_path{true};

  // Allow texture load failure?
  bool allow_texture_load_failure{true};

  // Allow asset(e.g. texture file/shader file) which does not exit?
  bool allow_missing_asset{true};

  // ------------------------------------------
};

struct RenderSceneConverterConfig {
  // Load texture image data on convert.
  // false: no actual texture file/asset access.
  // App/User must setup TextureImage manually after the conversion.
  bool load_texture_assets{true};
};

class RenderSceneConverter {
 public:
  RenderSceneConverter() = default;
  RenderSceneConverter(const RenderSceneConverter &rhs) = delete;
  RenderSceneConverter(RenderSceneConverter &&rhs) = delete;

  void set_scene_config(const RenderSceneConverterConfig &config) {
    _scene_config = config;
  }

  void set_mesh_config(const MeshConverterConfig &config) {
    _mesh_config = config;
  }

  void set_material_config(const MaterialConverterConfig &config) {
    _material_config = config;
  }

  void set_asset_resoluition_resolver(AssetResolutionResolver &&rhs) {
    _asset_resolver = std::move(rhs);
  }

  void set_search_paths(const std::vector<std::string> &paths) {
    _asset_resolver.set_search_paths(paths);
  }

  ///
  /// Convert Stage to RenderScene.
  /// Must be called after SetStage, SetMaterialConverterConfig(optional)
  ///
  bool ConvertToRenderScene(const Stage &stage, RenderScene *scene);

  const std::string &GetInfo() const { return _info; }
  const std::string &GetWarning() const { return _warn; }
  const std::string &GetError() const { return _err; }

  StringAndIdMap nodeMap;
  StringAndIdMap meshMap;
  StringAndIdMap materialMap;
  StringAndIdMap textureMap;
  StringAndIdMap imageMap;
  StringAndIdMap bufferMap;
  std::vector<Node> nodes;
  std::vector<RenderMesh> meshes;
  std::vector<RenderMaterial> materials;
  std::vector<UVTexture> textures;
  std::vector<TextureImage> images;
  std::vector<BufferData> buffers;

  ///
  /// @param[in] rmaterial_id RenderMaterial index. -1 if no material assigned
  /// to this Mesh. If the mesh has bounded material, RenderMaterial index must
  /// be obrained using ConvertMaterial method.
  /// @param[in] mesh Input GeomMesh
  /// @param[out] dst RenderMesh output
  ///
  /// @return true when success.
  ///
  /// TODO: per-face material(GeomSubset)
  ///
  bool ConvertMesh(const int64_t rmaterial_d, const tinyusdz::GeomMesh &mesh,
                   RenderMesh *dst);

  ///
  /// Convert USD Material/Shader to renderer-friendly Material
  ///
  /// @return true when success.
  ///
  bool ConvertMaterial(const tinyusdz::Path &abs_mat_path,
                       const tinyusdz::Material &material,
                       RenderMaterial *rmat_out);

  ///
  /// Convert UsdPreviewSurface Shader to renderer-friendly PreviewSurfaceShader
  ///
  /// @param[in] shader_abs_path USD Path to Shader Prim with UsdPreviewSurface info:id.
  /// @param[in] shader UsdPreviewSurface
  /// @param[in] pss_put PreviewSurfaceShader
  ///
  /// @return true when success.
  ///
  bool ConvertPreviewSurfaceShader(const tinyusdz::Path &shader_abs_path,
                                   const tinyusdz::UsdPreviewSurface &shader,
                                   PreviewSurfaceShader *pss_out);

  ///
  /// Convert UsdUvTexture to renderer-friendly UVTexture
  ///
  /// @param[in] tex_abs_path USD Path to Shader Prim with UsdUVTexture info:id.
  /// @param[in] assetInfo assetInfo Prim metadata of given Shader Prim
  /// @param[in] texture UsdUVTexture 
  /// @param[in] tex_out UVTexture 
  ///
  /// TODO: Retrieve assetInfo from `tex_abs_path`?
  ///
  /// @return true when success.
  ///
  bool ConvertUVTexture(const Path &tex_abs_path, const AssetInfo &assetInfo,
                        const UsdUVTexture &texture,
                        UVTexture *tex_out);

  const Stage *GetStagePtr() const { return _stage; }

 private:
  template <typename T, typename Dty>
  bool ConvertPreviewSurfaceShaderParam(
      const Path &shader_abs_path,
      const TypedAttributeWithFallback<Animatable<T>> &param,
      const std::string &param_name, ShaderParam<Dty> &dst_param);

  AssetResolutionResolver _asset_resolver;

  RenderSceneConverterConfig _scene_config;
  MeshConverterConfig _mesh_config;
  MaterialConverterConfig _material_config;
  const Stage *_stage{nullptr};

  void PushInfo(const std::string &msg) { _info += msg; }
  void PushWarn(const std::string &msg) { _warn += msg; }
  void PushError(const std::string &msg) { _err += msg; }

  std::string _info;
  std::string _err;
  std::string _warn;
};

// For debug
// Supported format: "kdl" (default. https://kdl.dev/), "json"
//
std::string DumpRenderScene(const RenderScene &scene,
                            const std::string &format = "kdl");

}  // namespace tydra
}  // namespace tinyusdz
