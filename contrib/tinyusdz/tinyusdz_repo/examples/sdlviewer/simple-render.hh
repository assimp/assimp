#pragma once

#include <unordered_map>

#include "nanort.h"
#include "nanosg.h"
#include "tinyusdz.hh"

namespace example {

// GLES-like naming

using vec3 = tinyusdz::value::float3;
using vec2 = tinyusdz::value::float2;
using mat2 = tinyusdz::value::matrix2f;

struct AOV {
  size_t width;
  size_t height;

  std::vector<float> rgb;               // 3 x width x height
  std::vector<float> shading_normal;    // 3 x width x height
  std::vector<float> geometric_normal;  // 3 x width x height
  std::vector<float> texcoords;         // 2 x width x height

  void Resize(size_t w, size_t h) {
    width = w;
    height = h;

    rgb.resize(width * height * 3);
    memset(rgb.data(), 0, sizeof(float) * rgb.size());

    shading_normal.resize(width * height * 3);
    memset(shading_normal.data(), 0, sizeof(float) * shading_normal.size());

    geometric_normal.resize(width * height * 3);
    memset(geometric_normal.data(), 0, sizeof(float) * geometric_normal.size());

    texcoords.resize(width * height * 2);
    memset(texcoords.data(), 0, sizeof(float) * texcoords.size());
  }
};

struct Camera {
  float eye[3] = {0.0f, 0.0f, 25.0f};
  float up[3] = {0.0f, 1.0f, 0.0f};
  float look_at[3] = {0.0f, 0.0f, 0.0f};
  float quat[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float fov = 60.0f;  // in degree
};

template <typename T>
struct Buffer {
  size_t num_coords{1};  // e.g. 3 for vec3 type.
  std::vector<T> data;
};

//
// Renderable Node class for NanoSG. Includes xform
//
struct DrawNode {
  std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
  std::array<float, 3> rotation{0.0f, 0.0f, 0.0f};  // euler rotation
  std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
};

//
// Renderable Mesh class for tinyusdz::GeomMesh
// Mesh data is converted to triangle meshes.
//
struct DrawGeomMesh {
  DrawGeomMesh(const tinyusdz::GeomMesh *p) : ref_mesh(p) {}

  // Pointer to Reference GeomMesh.
  const tinyusdz::GeomMesh *ref_mesh = nullptr;

  ///
  /// Required accessor API for NanoSG
  ///
  const float *GetVertices() const {
     return nullptr; // TODO
    //return reinterpret_cast<const float *>(ref_mesh->points.data());
  }

  size_t GetVertexStrideBytes() const { return sizeof(float) * 3; }

  std::vector<float> vertices;  // vec3f
  std::vector<uint32_t>
      facevertex_indices;  // triangulated indices. 3 x num_faces
  std::vector<float> facevarying_normals;    // 3 x 3 x num_faces
  std::vector<float> facevarying_texcoords;  // 2 x 3 x num_faces

  // arbitrary primvars(including texcoords(float2))
  std::vector<Buffer<float>> float_primvars;
  std::map<std::string, size_t>
      float_primvars_map;  // <name, index to `float_primvars`>

  // arbitrary primvars in int type(e.g. texcoord indices(int3))
  std::vector<Buffer<int32_t>> int_primvars;
  std::map<std::string, size_t>
      int_primvars_map;  // <name, index to `int_primvars`>

  int material_id{-1};  // per-geom material. index to `RenderScene::materials`

  nanort::BVHAccel<float> accel;
};

template<typename T>
struct UVReader {

  static_assert(std::is_same<T, float>::value || std::is_same<T, vec2>::value || std::is_same<T, vec3>::value,
                "Unsupported type for UVReader");

  int32_t st_id{-1};       // index to DrawGeomMesh::float_primvars
  int32_t indices_id{-1};  // index to DrawGeomMesh::int_primvars

  mat2 uv_transform;

  // Fetch interpolated UV coordinate
  T fetch_uv(size_t face_id, float varyu, float varyv);
};

struct Texture {
  enum Channel {
    TEXTURE_CHANNEL_R,
    TEXTURE_CHANNEL_G,
    TEXTURE_CHANNEL_B,
    TEXTURE_CHANNEL_RGB,
    TEXTURE_CHANNEL_RGBA,
  };

  UVReader<vec2> uv_reader;
  int32_t image_id{-1};

  // NOTE: for single channel(e.g. R), [0] will be filled for the return value.
  std::array<float, 4> fetch(size_t face_id, float varyu, float varyv,
                             Channel channel);
};

// https://graphics.pixar.com/usd/release/spec_usdpreviewsurface.html#texture-reader
// https://learn.foundry.com/modo/901/content/help/pages/uving/udim_workflow.html
// Up to 10 tiles for U direction.
// Not sure there is an limitation for V direction. Anyway maximum tile id is
// 9999.

#if 0
static uint32_t GetUDIMTileId(uint32_t u, uint32_t v)
{
  uint32_t uu = std::max(1u, std::min(10u, u+1)); // clamp U dir.

  return 1000 + v * 10 + uu;
}
#endif

struct UDIMTexture {
  UVReader<vec2> uv_reader;
  std::unordered_map<uint32_t, int32_t>
      images;  // key: udim tile_id, value: image_id

  // NOTE: for single channel(e.g. R), [0] will be filled for the return value.
  std::array<float, 4> fetch(size_t face_id, float varyu, float varyv,
                             Texture::Channel channel);
};

// base color(fallback color) or Texture
template <typename T>
struct ShaderParam {
  ShaderParam(const T &t) { value = t; }

  T value;
  int32_t texture_id{-1};
};

// UsdPreviewSurface
struct PreviewSurfaceShader {
  bool useSpecularWorkFlow{false};

  ShaderParam<vec3> diffuseColor{{0.18f, 0.18f, 0.18f}};
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
};

struct Material {
  PreviewSurfaceShader shader;
};


// Simple LDR texture
struct Image {
  std::vector<uint8_t> image;
  int32_t width{-1};
  int32_t height{-1};
  int32_t channels{-1};  // e.g. 3 for RGB.
};

class RenderScene {
 public:
  std::vector<DrawGeomMesh> draw_meshes;
  std::vector<Material> materials;
  std::vector<Texture> textures;
  std::vector<Image> images;

  std::vector<nanosg::Node<float, DrawGeomMesh>> nodes;
  nanosg::Scene<float, DrawGeomMesh> scene;

  // Convert meshes and build BVH
  bool Setup();
};

bool Render(const RenderScene &scene, const Camera &cam, AOV *output);

// Render images for lines [start_y, end_y]
// single-threaded. for webassembly.
bool RenderLines(int start_y, int end_y, const RenderScene &scene,
                 const Camera &cam, AOV *output);

}  // namespace example
