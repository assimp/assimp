// SPDX-License-Identifier: Apache 2.0
// Copyright 2022 - 2023, Syoyo Fujita.
// Copyright 2023 - Present, Light Transport Entertainment Inc.
//
// TODO:
//   - [ ] Subdivision surface to polygon mesh conversion.
//     - [ ] Correctly handle primvar with 'vertex' interpolation(Use the basis function of subd surface)
//   - [ ] Support time-varying shader attribute(timeSamples)
//
#include "image-loader.hh"
#include "image-util.hh"
#include "pprinter.hh"
#include "prim-types.hh"
#include "str-util.hh"
#include "tiny-format.hh"
#include "tinyusdz.hh"
#include "usdGeom.hh"
#include "usdShade.hh"
#include "value-pprint.hh"
#include "linear-algebra.hh"

#if defined(TINYUSDZ_WITH_COLORIO)
#include "external/tiny-color-io.h"
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

// For triangulation.
// TODO: Use tinyobjloader's triangulation
#include "external/mapbox/earcut/earcut.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

//
#include "common-macros.inc"
#include "math-util.inc"

//
#include "tydra/render-data.hh"
#include "tydra/scene-access.hh"
#include "tydra/shader-network.hh"

#if 0 // not used a.t.m.
#define SET_ERROR_AND_RETURN(msg) \
    if (err) {                      \
      (*err) = (msg);               \
    }                               \
    return false
#endif

namespace tinyusdz {

namespace tydra {

namespace {

inline std::string to_string(const UVTexture::Channel channel) {
  if (channel == UVTexture::Channel::RGB) {
    return "rgb";
  } else if (channel == UVTexture::Channel::R) {
    return "r";
  } else if (channel == UVTexture::Channel::G) {
    return "g";
  } else if (channel == UVTexture::Channel::B) {
    return "b";
  } else if (channel == UVTexture::Channel::A) {
    return "a";
  }

  return "[[InternalError. Invalid UVTexture::Channel]]";
}

#if 0
template <typename T>
inline T Get(const nonstd::optional<T> &nv, const T &default_value) {
  if (nv) {
    return nv.value();
  }
  return default_value;
}
#endif

//
// Convert vertex attribute with Uniform variability(interpolation) to facevarying attribute,
// by replicating uniform value per face over face vertices.
//
template <typename T>
nonstd::expected<std::vector<T>, std::string> UniformToFaceVarying(
    const std::vector<T> &inputs, const std::vector<uint32_t> &faceVertexCounts)
{
  std::vector<T> dst;

  if (inputs.size() == faceVertexCounts.size()) {
    return nonstd::make_unexpected(
        fmt::format("The number of inputs {} must be the same with "
                    "faceVertexCounts.size() {}",
                    inputs.size(), faceVertexCounts.size()));
  }

  for (size_t i = 0; i < faceVertexCounts.size(); i++) {
    size_t cnt = faceVertexCounts[i];

    // repeat cnt times.
    for (size_t k = 0; k < cnt; k++) {
      dst.emplace_back(inputs[i]);
    }
  }

  return dst;
}

// Generic uniform to facevarying conversion
nonstd::expected<std::vector<uint8_t>, std::string> UniformToFaceVarying(
    const std::vector<uint8_t> &src,
    const size_t stride_bytes,
    const std::vector<uint32_t> &faceVertexCounts)
{
  std::vector<uint8_t> dst;

  if (stride_bytes == 0) {
    return nonstd::make_unexpected("stride_bytes is zero.");
  }

  if ((src.size() % stride_bytes) != 0) {
    return nonstd::make_unexpected(
        fmt::format("input bytes {} must be the multiple of stride_bytes {}",
                    src.size(), stride_bytes));
  }

  size_t num_uniforms = src.size() / stride_bytes;

  if (num_uniforms == faceVertexCounts.size()) {
    return nonstd::make_unexpected(
        fmt::format("The number of input uniform attributes {} must be the same with "
                    "faceVertexCounts.size() {}",
                    num_uniforms, faceVertexCounts.size()));
  }

  std::vector<uint8_t> buf;
  buf.resize(stride_bytes);

  for (size_t i = 0; i < faceVertexCounts.size(); i++) {
    size_t cnt = faceVertexCounts[i];

    memcpy(buf.data(), src.data() + i * stride_bytes, stride_bytes);

    // repeat cnt times.
    for (size_t k = 0; k < cnt; k++) {
      dst.insert(dst.end(), buf.begin(), buf.end());
    }
  }

  return dst;
}

//
// Convert vertex attribute with Vertex variability(interpolation) to facevarying attribute,
// by expanding(flatten) the value per vertex per face.
//
template <typename T>
nonstd::expected<std::vector<T>, std::string> VertexToFaceVarying(
    const std::vector<T> &inputs, const std::vector<uint32_t> &faceVertexCounts,
    const std::vector<uint32_t> &faceVertexIndices) {
  std::vector<T> dst;

  size_t face_offset{0};
  for (size_t i = 0; i < faceVertexCounts.size(); i++) {
    size_t cnt = faceVertexCounts[i];

    for (size_t k = 0; k < cnt; k++) {
      size_t idx = k + face_offset;

      if (idx >= faceVertexIndices.size()) {
        return nonstd::make_unexpected(fmt::format(
            "faeVertexIndex out-of-range at faceVertexCount[{}]", i));
      }

      size_t v_idx = faceVertexIndices[idx];

      if (v_idx >= inputs.size()) {
        return nonstd::make_unexpected(
            fmt::format("faeVertexIndices[{}] {} exceeds input array size {}",
                        idx, v_idx, inputs.size()));
      }

      dst.emplace_back(inputs[v_idx]);
    }

    face_offset += cnt;
  }

  return dst;
}

// Generic vertex to facevarying conversion
nonstd::expected<std::vector<uint8_t>, std::string> VertexToFaceVarying(
    const std::vector<uint8_t> &src,
    const size_t stride_bytes,
    const std::vector<uint32_t> &faceVertexCounts,
    const std::vector<uint32_t> &faceVertexIndices) {
  std::vector<uint8_t> dst;

  if (src.empty()) {
      return nonstd::make_unexpected(
          "src data is empty.");
  }

  if (stride_bytes == 0) {
      return nonstd::make_unexpected(
          "stride_bytes must be non-zero.");
  }

  if ((src.size() % stride_bytes) != 0) {
      return nonstd::make_unexpected(fmt::format(
          "src size {} must be the multiple of stride_bytes {}", src.size(), stride_bytes));
  }

  const size_t num_vertices  = src.size() / stride_bytes;


  std::vector<uint8_t> buf;
  buf.resize(stride_bytes);

  size_t faceVertexIndexOffset{0};

  for (size_t i = 0; i < faceVertexCounts.size(); i++) {
    size_t cnt = faceVertexCounts[i];

    for (size_t k = 0; k < cnt; k++) {
      size_t fv_idx = k + faceVertexIndexOffset;

      if (fv_idx >= faceVertexIndices.size()) {
        return nonstd::make_unexpected(fmt::format(
            "faeVertexIndex {} out-of-range at faceVertexCount[{}]", fv_idx, i));
      }

      size_t v_idx = faceVertexIndices[fv_idx];

      if (v_idx >= num_vertices) {
        return nonstd::make_unexpected(
            fmt::format("faeVertexIndices[{}] {} exceeds the number of vertices {}",
                        fv_idx, v_idx, num_vertices));
      }

      memcpy(buf.data(), src.data() + v_idx * stride_bytes, stride_bytes);
      dst.insert(dst.end(), buf.begin(), buf.end());
    }

    faceVertexIndexOffset += cnt;
  }

  return dst;
}

#if 0 // unused a.t.m
// Copy single value to facevarying vertices.
template <typename T>
static nonstd::expected<std::vector<T>, std::string> ConstantToFaceVarying(
    const T &input, const std::vector<uint32_t> &faceVertexCounts) {
  std::vector<T> dst;

  for (size_t i = 0; i < faceVertexCounts.size(); i++) {
    size_t cnt = faceVertexCounts[i];

    for (size_t k = 0; k < cnt; k++) {
      dst.emplace_back(input);
    }
  }

  return dst;
}
#endif

static nonstd::expected<std::vector<uint8_t>, std::string> ConstantToFaceVarying(
    const std::vector<uint8_t> &src,
    const size_t stride_bytes,
    const std::vector<uint32_t> &faceVertexCounts) {
  std::vector<uint8_t> dst;

  if (src.empty()) {
      return nonstd::make_unexpected(
          "src data is empty.");
  }

  if (stride_bytes == 0) {
      return nonstd::make_unexpected(
          "stride_bytes must be non-zero.");
  }

  if ((src.size() != stride_bytes)) {
      return nonstd::make_unexpected(fmt::format(
          "src size {} must be equal to stride_bytes {}", src.size(), stride_bytes));
  }

  std::vector<uint8_t> buf;
  buf.resize(stride_bytes);

  for (size_t i = 0; i < faceVertexCounts.size(); i++) {
    size_t cnt = faceVertexCounts[i];

    for (size_t k = 0; k < cnt; k++) {
      dst.insert(dst.end(), buf.begin(), buf.end());
    }
  }

  return dst;
}

//
// name does not include "primvars:" prefix.
// TODO: timeSamples, connected attribute.
//
nonstd::expected<VertexAttribute, std::string> GetTextureCoordinate(
    const Stage &state, const GeomMesh &mesh, const std::string &name) {
  VertexAttribute vattr;

  (void)state;

  GeomPrimvar primvar;
  if (!mesh.get_primvar(name, &primvar)) {
    return nonstd::make_unexpected(fmt::format("No primvars:{}\n", name));
  }

  if (!primvar.has_value()) {
    return nonstd::make_unexpected("No value exist for primvars:" + name +
                                   "\n");
  }

  if (primvar.get_type_id() !=
      value::TypeTraits<std::vector<value::texcoord2f>>::type_id()) {
    return nonstd::make_unexpected(
        "Texture coordinate primvar must be texCoord2f[] type, but got " +
        primvar.get_type_name() + "\n");
  }

  if (primvar.get_interpolation() == Interpolation::Varying) {
    vattr.variability = VertexVariability::Varying;
  } else if (primvar.get_interpolation() == Interpolation::Constant) {
    vattr.variability = VertexVariability::Constant;
  } else if (primvar.get_interpolation() == Interpolation::Uniform) {
    vattr.variability = VertexVariability::Uniform;
  } else if (primvar.get_interpolation() == Interpolation::Vertex) {
    vattr.variability = VertexVariability::Vertex;
  } else if (primvar.get_interpolation() == Interpolation::FaceVarying) {
    vattr.variability = VertexVariability::FaceVarying;
  }

  std::vector<value::texcoord2f> uvs;
  if (!primvar.flatten_with_indices(&uvs)) {
    return nonstd::make_unexpected(
        "Failed to retrieve texture coordinate primvar with concrete type.\n");
  }

  vattr.format = VertexAttributeFormat::Vec2;
  vattr.data.resize(uvs.size() * sizeof(value::texcoord2f));
  memcpy(vattr.data.data(), uvs.data(), vattr.data.size());
  vattr.indices.clear();  // just in case.

  return std::move(vattr);
}

#if 0  // not used at the moment.
///
/// For GeomSubset. Build offset table to corresponding array index in
/// mesh.faceVertexIndices. No need to use this function for triangulated mesh,
/// since the index can be easily computed as `3 * subset.indices[i]`
///
bool BuildFaceVertexIndexOffsets(const std::vector<uint32_t> &faceVertexCounts,
                                 std::vector<size_t> &faceVertexIndexOffsets) {
  size_t offset = 0;
  for (size_t i = 0; i < faceVertexCounts.size(); i++) {
    uint32_t npolys = faceVertexCounts[i];

    faceVertexIndexOffsets.push_back(offset);
    offset += npolys;
  }

  return true;
}
#endif


#if 0 // unused a.t.m.
bool ToVertexAttributeData(const GeomPrimvar &primvar, VertexAttribute *dst, std::string *err)
{
  size_t elementSize = primvar.get_elementSize();
  if (elementSize == 0) {
    SET_ERROR_AND_RETURN(fmt::format("elementSize is zero for primvar: {}", primvar.name()));
  }

  VertexAttribute vattr;

  const tinyusdz::Attribute &attr = primvar.get_attribute();

#define TO_TYPED_VALUE(__ty, __va_ty) \
  if (attr.type_id() == value::TypeTraits<__ty>::type_id()) { \
    if (sizeof(__ty) != VertexAttributeFormatSize(__va_ty)) { \
      SET_ERROR_AND_RETURN("Internal error. type size mismatch.\n"); \
    } \
    __ty value; \
    if (!primvar.get_value(&value, err)) { \
      return false; \
    } \
    vattr.format = __va_ty; \
  } else if (attr.type_id() == (value::TypeTraits<__ty>::type_id() & value::TYPE_ID_1D_ARRAY_BIT)) { \
    std::vector<__ty> flattened; \
    if (!primvar.flatten_with_indices(&flattened, err)) { \
      return false; \
    } \
  } else

  TO_TYPED_VALUE(int, VertexAttributeFormat::Int)
  {
    SET_ERROR_AND_RETURN(fmt::format("Unknown or unsupported data type for Geom PrimVar: {}", attr.type_name()));
  }

#undef TO_TYPED_VALUE

  if (primvar.get_interpolation() == Interpolation::Varying) {
    vattr.variability = VertexVariability::Varying;
  } else if (primvar.get_interpolation() == Interpolation::Constant) {
    vattr.variability = VertexVariability::Constant;
  } else if (primvar.get_interpolation() == Interpolation::Uniform) {
    vattr.variability = VertexVariability::Uniform;
  } else if (primvar.get_interpolation() == Interpolation::Vertex) {
    vattr.variability = VertexVariability::Vertex;
  } else if (primvar.get_interpolation() == Interpolation::FaceVarying) {
    vattr.variability = VertexVariability::FaceVarying;
  }

  // TODO
  vattr.indices.clear();  // just in case.

  (*dst) = std::move(vattr);

  return false;
}
#endif

#if 0 // TODO: Remove
///
/// Triangulate Geom primvar.
///
/// triangulatted indices are computed in `TriangulatePolygon` API.
///
/// @param[in] mesh Geom mesh
/// @param[in] name Geom Primvar name.
/// @param[in] triangulatedFaceVertexIndices Triangulated faceVertexIndices(len
/// = 3 * triangles)
/// @param[in] triangulatedToOrigFaceVertexIndexMap Triangulated faceVertexIndex
/// to original faceVertexIndex remapping table. len = 3 * triangles.
///
nonstd::expected<VertexAttribute, std::string> TriangulateGeomPrimvar(
    const GeomMesh &mesh, const std::string &name,
    const std::vector<uint32_t> &faceVertexCounts,
    const std::vector<uint32_t> &faceVertexIndices,
    const std::vector<uint32_t> &triangulatedFaceVertexIndices,
    const std::vector<size_t> &triangulatedToOrigFaceVertexIndexMap) {
  GeomPrimvar primvar;

  if (triangulatedFaceVertexIndices.size() % 3 != 0) {
    return nonstd::make_unexpected(fmt::format(
        "triangulatedFaceVertexIndices.size {} must be the multiple of 3.\n",
        triangulatedFaceVertexIndices.size()));
  }

  if (!mesh.get_primvar(name, &primvar)) {
    return nonstd::make_unexpected(
        fmt::format("No primvars:{} found in GeomMesh {}\n", name, mesh.name));
  }

  if (!primvar.has_value()) {
    // TODO: Create empty VertexAttribute?
    return nonstd::make_unexpected(
        fmt::format("No value exist for primvars:{}\n", name));
  }

  //
  // Flatten Indexed PrimVar(return raw primvar for non-Indexed PrimVar)
  //
  std::string err;
  value::Value flattened;
  if (!primvar.flatten_with_indices(&flattened, &err)) {
    return nonstd::make_unexpected(fmt::format(
        "Failed to flatten Indexed PrimVar: {}. Error = {}\n", name, err));
  }

  VertexAttribute vattr;

  if (!ToVertexAttributeData(primvar, &vattr, &err)) {
    return nonstd::make_unexpected(fmt::format(
        "Failed to convert Geom PrimVar to VertexAttribute for {}. Error = {}\n", name, err));
  }

  return vattr;
}
#endif

///
/// Input: points, faceVertexCounts, faceVertexIndices
/// Output: triangulated faceVertexCounts(all filled with 3), triangulated
/// faceVertexIndices, triangulatedToOrigFaceVertexIndexMap (length =
/// triangulated faceVertexIndices. triangulatedToOrigFaceVertexIndexMap[i]
/// stores array index in original faceVertexIndices. For remapping primvar
/// attributes.)
///
/// Return false when a polygon is degenerated.
/// No overlap check at the moment
///
/// T = value::float3 or value::double3
/// BaseTy = float or double
template <typename T, typename BaseTy>
bool TriangulatePolygon(
    const std::vector<T> &points, const std::vector<uint32_t> &faceVertexCounts,
    const std::vector<uint32_t> &faceVertexIndices,
    std::vector<uint32_t> &triangulatedFaceVertexCounts,
    std::vector<uint32_t> &triangulatedFaceVertexIndices,
    std::vector<size_t> &triangulatedToOrigFaceVertexIndexMap,
    std::string &err) {
  triangulatedFaceVertexCounts.clear();
  triangulatedFaceVertexIndices.clear();

  triangulatedToOrigFaceVertexIndexMap.clear();

  size_t faceIndexOffset = 0;

  // For each polygon(face)
  for (size_t i = 0; i < faceVertexCounts.size(); i++) {
    uint32_t npolys = faceVertexCounts[i];

    if (npolys < 3) {
      err = fmt::format(
          "faceVertex count must be 3(triangle) or "
          "more(polygon), but got faceVertexCounts[{}] = {}\n",
          i, npolys);
      return false;
    }

    if (faceIndexOffset + npolys > faceVertexIndices.size()) {
      err = fmt::format(
          "Invalid faceVertexIndices or faceVertexCounts. faceVertex index "
          "exceeds faceVertexIndices.size() at [{}]\n",
          i);
      return false;
    }

    if (npolys == 3) {
      // No need for triangulation.
      triangulatedFaceVertexCounts.push_back(3);
      triangulatedFaceVertexIndices.push_back(
          faceVertexIndices[faceIndexOffset + 0]);
      triangulatedFaceVertexIndices.push_back(
          faceVertexIndices[faceIndexOffset + 1]);
      triangulatedFaceVertexIndices.push_back(
          faceVertexIndices[faceIndexOffset + 2]);
      triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset + 0);
      triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset + 1);
      triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset + 2);
#if 1
    } else if (npolys == 4) {
      // Use simple split
      // TODO: Split at shortest edge for better triangulation.
      triangulatedFaceVertexCounts.push_back(3);
      triangulatedFaceVertexCounts.push_back(3);

      triangulatedFaceVertexIndices.push_back(
          faceVertexIndices[faceIndexOffset + 0]);
      triangulatedFaceVertexIndices.push_back(
          faceVertexIndices[faceIndexOffset + 1]);
      triangulatedFaceVertexIndices.push_back(
          faceVertexIndices[faceIndexOffset + 2]);

      triangulatedFaceVertexIndices.push_back(
          faceVertexIndices[faceIndexOffset + 0]);
      triangulatedFaceVertexIndices.push_back(
          faceVertexIndices[faceIndexOffset + 2]);
      triangulatedFaceVertexIndices.push_back(
          faceVertexIndices[faceIndexOffset + 3]);

      triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset + 0);
      triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset + 1);
      triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset + 2);
      triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset + 0);
      triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset + 2);
      triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset + 3);
#endif
    } else {
      // Find the normal axis of the polygon using Newell's method
      T n = {BaseTy(0), BaseTy(0), BaseTy(0)};

      size_t vi0;
      size_t vi0_2;

      for (size_t k = 0; k < npolys; ++k) {
        vi0 = faceVertexIndices[faceIndexOffset + k];

        size_t j = (k + 1) % npolys;
        vi0_2 = faceVertexIndices[faceIndexOffset + j];

        if (vi0 >= points.size()) {
          err = fmt::format("Invalid vertex index.\n");
          return false;
        }

        if (vi0_2 >= points.size()) {
          err = fmt::format("Invalid vertex index.\n");
          return false;
        }

        T v0 = points[vi0];
        T v1 = points[vi0_2];

        const T point1 = {v0[0], v0[1], v0[2]};
        const T point2 = {v1[0], v1[1], v1[2]};

        T a = {point1[0] - point2[0], point1[1] - point2[1],
               point1[2] - point2[2]};
        T b = {point1[0] + point2[0], point1[1] + point2[1],
               point1[2] + point2[2]};

        n[0] += (a[1] * b[2]);
        n[1] += (a[2] * b[0]);
        n[2] += (a[0] * b[1]);
      }
      BaseTy length_n = vlength(n);
      // Check if zero length normal
      if (std::fabs(length_n) < std::numeric_limits<BaseTy>::epsilon()) {
        err = "Degenerated polygon found.\n";
        return false;
      }

      // Negative is to flip the normal to the correct direction
      n = vnormalize(n);

      T axis_w, axis_v, axis_u;
      axis_w = n;
      T a;
      if (std::fabs(axis_w[0]) > BaseTy(0.9999999)) {  // TODO: use 1.0 - eps?
        a = {BaseTy(0), BaseTy(1), BaseTy(0)};
      } else {
        a = {BaseTy(1), BaseTy(0), BaseTy(0)};
      }
      axis_v = vnormalize(vcross(axis_w, a));
      axis_u = vcross(axis_w, axis_v);

      using Point3D = std::array<BaseTy, 3>;
      using Point2D = std::array<BaseTy, 2>;
      std::vector<Point2D> polyline;

      // TMW change: Find best normal and project v0x and v0y to those
      // coordinates, instead of picking a plane aligned with an axis (which
      // can flip polygons).

      // Fill polygon data.
      for (size_t k = 0; k < npolys; k++) {
        size_t vidx = faceVertexIndices[faceIndexOffset + k];

        value::float3 v = points[vidx];
        // Point3 polypoint = {v0[0],v0[1],v0[2]};

        // world to local
        Point3D loc = {vdot(v, axis_u), vdot(v, axis_v), vdot(v, axis_w)};

        polyline.push_back({loc[0], loc[1]});
      }

      std::vector<std::vector<Point2D>> polygon_2d;
      // Single polygon only(no holes)

      std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(polygon_2d);
      //  => result = 3 * faces, clockwise

      if ((indices.size() % 3) != 0) {
        // This should not be happen, though.
        err = "Failed to triangulate.\n";
        return false;
      }

      size_t ntris = indices.size() / 3;

      for (size_t k = 0; k < ntris; k++) {
        triangulatedFaceVertexCounts.push_back(3);
        triangulatedFaceVertexIndices.push_back(
            faceVertexIndices[faceIndexOffset + indices[3 * k + 0]]);
        triangulatedFaceVertexIndices.push_back(
            faceVertexIndices[faceIndexOffset + indices[3 * k + 1]]);
        triangulatedFaceVertexIndices.push_back(
            faceVertexIndices[faceIndexOffset + indices[3 * k + 2]]);

        triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset +
                                                       indices[3 * k + 0]);
        triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset +
                                                       indices[3 * k + 1]);
        triangulatedToOrigFaceVertexIndexMap.push_back(faceIndexOffset +
                                                       indices[3 * k + 2]);
      }
    }

    faceIndexOffset += npolys;
  }

  return true;
}

#if 0
//
// `Shader` may be nested, so first list up all Shader nodes under Material.
//
struct ShaderNode
{
  std::name
};

nonstd::optional<UsdPrimvarReader_float2> FindPrimvarReader_float2Rec(
  const Prim &root,
  RenderMesh &mesh)
{
  if (auto sv = root.data.as<Shader>()) {
    const Shader &shader = (*sv);

    if (auto pv = shader.value.as<UsdUVTexture>()) {
      const UsdUVTexture &tex = (*pv);
      (void)tex;
    }
  }

  for (const auto &child : root.children) {
    auto ret = ListUpShaderGraphRec(child, mesh);
    if (!ret) {
      return nonstd::make_unexpected(ret.error());
    }
  }

  return true;
}
#endif


#if 0 // not used a.t.m.
// Building an Orthonormal Basis, Revisited
// http://jcgt.org/published/0006/01/01/
static void GenerateBasis(const vec3 &n, vec3 *tangent,
                         vec3 *binormal)
{
  if (n[2] < 0.0f) {
    const float a = 1.0f / (1.0f - n[2]);
    const float b = n[0] * n[1] * a;
    (*tangent) = vec3{1.0f - n[0] * n[0] * a, -b, n[0]};
    (*binormal) = vec3{b, n[1] * n[1] * a - 1.0f, -n[1]};
  } else {
    const float a = 1.0f / (1.0f + n[2]);
    const float b = -n[0] * n[1] * a;
    (*tangent) = vec3{1.0f - n[0] * n[0] * a, b, -n[0]};
    (*binormal) = vec3{b, 1.0f - n[1] * n[1] * a, -n[1]};
  }
}

///
/// Compute facevarying tangent and facevarying binormal.
///
/// Reference:
/// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping
///
/// Implemented code uses two adjacent edge composed from three vertices v_{i}, v_{i+1}, v_{i+2} for i < (N - 1)
/// , where N is the number of vertices per facet.
///
/// This may produce unwanted tangent/binormal frame for ill-defined polygon(quad, pentagon, ...).
/// Also, we assume input mesh has well-formed and has no or few vertices with similar property(position, uvs and normals)
///
/// TODO:
/// - [ ] Implement getSimilarVertexIndex in the above opengl-tutorial to better average tangent/binormal.
///   - https://github.com/huamulan/OpenGL-tutorial/blob/master/common/vboindexer.cpp uses simple linear-search,
///   - so could become quite slow when the input mesh has large number of vertices.
///   - Use kNN search(e.g. nanoflann https://github.com/jlblancoc/nanoflann ), or point-query by building BVH over the mesh points.
///     - BVH builder candidate:
///       - NanoRT https://github.com/lighttransport/nanort
///       - bvh https://github.com/madmann91/bvh
///   - Or we can quantize vertex attributes and compute locally sensitive hashsing? https://dl.acm.org/doi/10.1145/3188745.3188846
/// - [ ] Implement more robust tangent/binormal frame computation algorithm for arbitrary mesh?
///
///
static bool ComputeTangentsAndBinormals(
    const std::vector<vec3> &vertices,
    const std::vector<uint32_t> &faceVertexCounts,
    const std::vector<uint32_t> &faceVertexIndices,
    const std::vector<vec2> &facevarying_texcoords,
    const std::vector<vec3> &facevarying_normals,
    std::vector<vec3> *facevarying_tangents,
    std::vector<vec3> *facevarying_binormals,
    std::string *err) {


  if (vertices.empty()) {
    SET_ERROR_AND_RETURN("vertices is empty.");
  }

  // At least 1 triangle face should exist.
  if (faceVertexIndices.size() < 9) {
    SET_ERROR_AND_RETURN("faceVertexIndices.size < 9");
  }

  if (facevarying_texcoords.empty()) {
    SET_ERROR_AND_RETURN("facevarying_texcoords is empty");
  }

  if (facevarying_normals.empty()) {
    SET_ERROR_AND_RETURN("facevarying_normals is empty");
  }

  std::vector<value::normal3f> tn(vertices.size());
  memset(&tn.at(0), 0, sizeof(value::normal3f) * tn.size());
  std::vector<value::normal3f> bn(vertices.size());
  memset(&bn.at(0), 0, sizeof(value::normal3f) * bn.size());


  bool hasFaceVertexCounts{true};

  size_t num_faces = faceVertexCounts.size();
  if (num_faces == 0) {
    // Assume all triangle faces.
    if ((faceVertexIndices.size() % 3) != 0) {
      SET_ERROR_AND_RETURN("Invalid faceVertexIndices. It must be all triangles: faceVertexIndices.size % 3 == 0");
    }

    num_faces = faceVertexIndices.size() / 3;
    hasFaceVertexCounts = false;
  }

  //
  // 1. Compute tangent/binormal for each faceVertex.
  //
  size_t faceVertexIndexOffset{0};
  for (size_t i = 0; i < num_faces; i++) {
    size_t nv = hasFaceVertexCounts ? faceVertexCounts[i] : 3;

    if ((faceVertexIndexOffset + nv) >= faceVertexIndices.size()) {
      // Invalid faceVertexIndices
      SET_ERROR_AND_RETURN("Invalid value in faceVertexOffset.");
    }

    if (nv < 3) {
      SET_ERROR_AND_RETURN("Degenerated facet found.");
    }

    // Process each two-edges per facet.
    //
    // Example:
    //
    // fv3
    //  o----------------o fv2
    //   \              /
    //    \            /
    //     o----------o
    //    fv0         fv1

    // facet0:  fv0, fv1, fv2
    // facet1:  fv1, fv2, fv3

    for (size_t f = 0; f < nv - 2; f++) {

      size_t fid0 = f;
      size_t fid1 = f+1;
      size_t fid2 = f+2;

      uint32_t vf0 = faceVertexIndices[fid0];
      uint32_t vf1 = faceVertexIndices[fid1];
      uint32_t vf2 = faceVertexIndices[fid2];

      if ((vf0 >= vertices.size()) ||
          (vf1 >= vertices.size()) ||
          (vf2 >= vertices.size()) ) {

        // index out-of-range
        SET_ERROR_AND_RETURN("Invalid value in faceVertexIndices. some exceeds vertices.size()");
      }

      vec3 v1 = vertices[vf0];
      vec3 v2 = vertices[vf1];
      vec3 v3 = vertices[vf2];

      float v1x = v1[0];
      float v1y = v1[1];
      float v1z = v1[2];

      float v2x = v2[0];
      float v2y = v2[1];
      float v2z = v2[2];

      float v3x = v3[0];
      float v3y = v3[1];
      float v3z = v3[2];

      float w1x = 0.0f;
      float w1y = 0.0f;
      float w2x = 0.0f;
      float w2y = 0.0f;
      float w3x = 0.0f;
      float w3y = 0.0f;

      if ((fid0 >= facevarying_texcoords.size()) ||
          (fid1 >= facevarying_texcoords.size()) ||
          (fid2 >= facevarying_texcoords.size()) ) {

        // index out-of-range
        SET_ERROR_AND_RETURN("Invalid value in faceVertexCounts. some exceeds facevarying_texcoords.size()");
      }

      {
        vec2 uv1 = facevarying_texcoords[fid0];
        vec2 uv2 = facevarying_texcoords[fid1];
        vec2 uv3 = facevarying_texcoords[fid2];

        w1x = uv1[0];
        w1y = uv1[1];
        w2x = uv2[0];
        w2y = uv2[1];
        w3x = uv3[0];
        w3y = uv3[1];
      }

      float x1 = v2x - v1x;
      float x2 = v3x - v1x;
      float y1 = v2y - v1y;
      float y2 = v3y - v1y;
      float z1 = v2z - v1z;
      float z2 = v3z - v1z;

      float s1 = w2x - w1x;
      float s2 = w3x - w1x;
      float t1 = w2y - w1y;
      float t2 = w3y - w1y;

      float r = 1.0;

      if (std::fabs(double(s1 * t2 - s2 * t1)) > 1.0e-20) {
        r /= (s1 * t2 - s2 * t1);
      }

      vec3 tdir{(t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
                  (t2 * z1 - t1 * z2) * r};
      vec3 bdir{(s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
                  (s1 * z2 - s2 * z1) * r};


      tn[vf0][0] += tdir[0];
      tn[vf0][1] += tdir[1];
      tn[vf0][2] += tdir[2];

      tn[vf1][0] += tdir[0];
      tn[vf1][1] += tdir[1];
      tn[vf1][2] += tdir[2];

      tn[vf2][0] += tdir[0];
      tn[vf2][1] += tdir[1];
      tn[vf2][2] += tdir[2];

      bn[vf0][0] += bdir[0];
      bn[vf0][1] += bdir[1];
      bn[vf0][2] += bdir[2];

      bn[vf1][0] += bdir[0];
      bn[vf1][1] += bdir[1];
      bn[vf1][2] += bdir[2];

      bn[vf2][0] += bdir[0];
      bn[vf2][1] += bdir[1];
      bn[vf2][2] += bdir[2];
    }

    faceVertexIndexOffset += nv;
  }

  //
  // 2. normalize * orthogonalize;
  //
  facevarying_tangents->resize(facevarying_normals.size());
  facevarying_binormals->resize(facevarying_normals.size());

  faceVertexIndexOffset = 0;
  for (size_t i = 0; i < num_faces; i++) {
    size_t nv = hasFaceVertexCounts ? faceVertexCounts[i] : 3;

    for (size_t f = 0; f < nv - 2; f++) {
      uint32_t vf[3];

      vf[0] = faceVertexIndices[faceVertexIndexOffset + f];
      vf[1] = faceVertexIndices[faceVertexIndexOffset + f + 1];
      vf[2] = faceVertexIndices[faceVertexIndexOffset + f + 2];

      value::normal3f n[3];

      // http://www.terathon.com/code/tangent.html

      {
        n[0][0] = facevarying_normals[faceVertexIndexOffset + f + 0][0];
        n[0][1] = facevarying_normals[faceVertexIndexOffset + f + 0][1];
        n[0][2] = facevarying_normals[faceVertexIndexOffset + f + 0][2];

        n[1][0] = facevarying_normals[faceVertexIndexOffset + f + 1][0];
        n[1][1] = facevarying_normals[faceVertexIndexOffset + f + 1][1];
        n[1][2] = facevarying_normals[faceVertexIndexOffset + f + 1][2];

        n[2][0] = facevarying_normals[faceVertexIndexOffset + f + 2][0];
        n[2][1] = facevarying_normals[faceVertexIndexOffset + f + 2][1];
        n[2][2] = facevarying_normals[faceVertexIndexOffset + f + 2][2];
      }

      size_t dst_idx[3];
      dst_idx[0] = f;
      dst_idx[1] = f+1;
      dst_idx[2] = f+2;

      for (size_t k = 0; k < 3; k++) {
        value::normal3f Tn = tn[vf[k]];
        value::normal3f Bn = bn[vf[k]];

        if (vlength(Bn) > 0.0f) {
          Bn = vnormalize(Bn);
        }

        // Gram-Schmidt orthogonalize
        Tn = (Tn - n[k] * vdot(n[k], Tn));
        if (vlength(Tn) > 0.0f) {
          Tn = vnormalize(Tn);
        }

        // Calculate handedness
        if (vdot(vcross(n[k], Tn), Bn) < 0.0f) {
          Tn = Tn * -1.0f;
        }

        ((*facevarying_tangents)[faceVertexIndexOffset + dst_idx[k]])[0] = Tn[0];
        ((*facevarying_tangents)[faceVertexIndexOffset + dst_idx[k]])[1] = Tn[1];
        ((*facevarying_tangents)[faceVertexIndexOffset + dst_idx[k]])[2] = Tn[2];

        ((*facevarying_binormals)[faceVertexIndexOffset + dst_idx[k]])[0] = Bn[0];
        ((*facevarying_binormals)[faceVertexIndexOffset + dst_idx[k]])[1] = Bn[1];
        ((*facevarying_binormals)[faceVertexIndexOffset + dst_idx[k]])[2] = Bn[2];
      }
    }

    faceVertexIndexOffset += nv;
  }

  return true;
}
#endif


}  // namespace


#if 0
// Currently float2 only
std::vector<UsdPrimvarReader_float2> ExtractPrimvarReadersFromMaterialNode(
    const Prim &node) {
  std::vector<UsdPrimvarReader_float2> dst;

  if (!node.is<Material>()) {
    return dst;
  }

  for (const auto &child : node.children()) {
    (void)child;
  }

  // Traverse and find PrimvarReader_float2 shader.
  return dst;
}

nonstd::expected<Node, std::string> Convert(const Stage &stage,
                                            const Xform &xform) {
  (void)stage;

  // TODO: timeSamples

  Node node;
  if (auto m = xform.GetLocalMatrix()) {
    node.local_matrix = m.value();
  }

  return std::move(node);
}
#endif

namespace {

bool ListUVNames(const RenderMaterial &material,
                 const std::vector<UVTexture> &textures,
                 StringAndIdMap &si_map) {
  // TODO: Use auto
  auto fun_vec3 = [&](const ShaderParam<vec3> &param) {
    int32_t texId = param.textureId;
    if ((texId >= 0) && (size_t(texId) < textures.size())) {
      const UVTexture &tex = textures[size_t(texId)];
      if (tex.varname_uv.size()) {
        if (!si_map.count(tex.varname_uv)) {
          uint64_t slotId = si_map.size();
          DCOUT("Add textureSlot: " << tex.varname_uv << ", " << slotId);
          si_map.add(tex.varname_uv, slotId);
        }
      }
    }
  };

  auto fun_float = [&](const ShaderParam<float> &param) {
    int32_t texId = param.textureId;
    if ((texId >= 0) && (size_t(texId) < textures.size())) {
      const UVTexture &tex = textures[size_t(texId)];
      if (tex.varname_uv.size()) {
        if (!si_map.count(tex.varname_uv)) {
          uint64_t slotId = si_map.size();
          DCOUT("Add textureSlot: " << tex.varname_uv << ", " << slotId);
          si_map.add(tex.varname_uv, slotId);
        }
      }
    }
  };

  fun_vec3(material.surfaceShader.diffuseColor);
  fun_vec3(material.surfaceShader.normal);
  fun_float(material.surfaceShader.metallic);
  fun_float(material.surfaceShader.roughness);
  fun_float(material.surfaceShader.clearcoat);
  fun_float(material.surfaceShader.clearcoatRoughness);
  fun_float(material.surfaceShader.opacity);
  fun_float(material.surfaceShader.opacityThreshold);
  fun_float(material.surfaceShader.ior);
  fun_float(material.surfaceShader.displacement);
  fun_float(material.surfaceShader.occlusion);

  return true;
}

}  // namespace

bool RenderSceneConverter::ConvertMesh(const int64_t rmaterial_id,
                                       const GeomMesh &mesh,
                                       RenderMesh *dstMesh) {
  if (!dstMesh) {
    PUSH_ERROR_AND_RETURN("`dst` mesh pointer is nullptr");
  }

  RenderMesh dst;

  bool triangulate = _mesh_config.triangulate;

  // indices
  // TODO: timeSamples, connections
  if (const auto pv = mesh.faceVertexIndices.get_value()) {
    std::vector<int32_t> indices;
    if (pv.value().get_scalar(&indices)) {
      // to uint32
      std::transform(indices.cbegin(), indices.cend(),
                     std::back_inserter(dst.faceVertexIndices),
                     [](int32_t v) { return uint32_t(v); });
    }
  }

  if (const auto pv = mesh.faceVertexCounts.get_value()) {
    std::vector<int32_t> counts;
    if (pv.value().get_scalar(&counts)) {
      // to uint32
      std::transform(counts.cbegin(), counts.cend(),
                     std::back_inserter(dst.faceVertexCounts),
                     [](int32_t v) { return uint32_t(v); });
    }
  }

  if (mesh.get_points().size()) {
    dst.points.resize(mesh.get_points().size());
    memcpy(dst.points.data(), mesh.get_points().data(),
           sizeof(value::float3) * mesh.get_points().size());
  }

  // normals
  {
    std::vector<value::normal3f> normals = mesh.get_normals();
    Interpolation interp = mesh.get_normalsInterpolation();

    if (normals.size()) {
      if (interp == Interpolation::Uniform) {
        auto result = UniformToFaceVarying(normals, dst.faceVertexCounts);
        if (!result) {
          PUSH_ERROR_AND_RETURN(
              fmt::format("Convert uniform `normals` attribute to failed: {}",
                          result.error()));
        }

        dst.facevaryingNormals.resize(result.value().size());
        memcpy(dst.facevaryingNormals.data(), result.value().data(),
               sizeof(value::normal3f) * result.value().size());

      } else if ((interp == Interpolation::Vertex) ||
                 (interp == Interpolation::Varying)) {
        auto result = VertexToFaceVarying(normals, dst.faceVertexCounts,
                                          dst.faceVertexIndices);
        if (!result) {
          PUSH_ERROR_AND_RETURN(fmt::format(
              "Convert vertex/varying `normals` attribute to failed: {}",
              result.error()));
        }

        dst.facevaryingNormals.resize(result.value().size());
        memcpy(dst.facevaryingNormals.data(), result.value().data(),
               sizeof(value::normal3f) * result.value().size());

      } else if (interp == Interpolation::FaceVarying) {
        dst.facevaryingNormals.resize(normals.size());
        memcpy(dst.facevaryingNormals.data(), normals.data(),
               sizeof(value::normal3f) * normals.size());
      } else {
        PUSH_ERROR_AND_RETURN(
            "Unsupported/unimplemented interpolation for `normals` "
            "attribute: " +
            to_string(interp) + ".\n");
      }
    } else {
      dst.facevaryingNormals.clear();
      dst.normalsInterpolation = interp;
    }
  }

  // Compute total faceVarying elements;
  size_t num_fvs = 0;
  for (size_t i = 0; i < dst.faceVertexCounts.size(); i++) {
    num_fvs += dst.faceVertexCounts[i];
  }

  DCOUT("num_fvs = " << num_fvs);

  // uvs from primvars.
  // uvname(varname) is grabbed from RenderMaterial.
  //
  // TODO: list-up varname from PreviewSurfaceShader members.
  //
  // Procedure:
  // - Find Shader
  // - Lookup PrimvarReader
  //

  if ((rmaterial_id > -1) && (size_t(rmaterial_id) < materials.size())) {
    const RenderMaterial &material = materials[size_t(rmaterial_id)];

    StringAndIdMap uvname_map;
    if (!ListUVNames(material, textures, uvname_map)) {
      return false;
    }

    for (auto it = uvname_map.i_begin(); it != uvname_map.i_end(); it++) {
      uint64_t slotId = it->first;
      std::string uvname = it->second;

      auto ret = GetTextureCoordinate(*_stage, mesh, uvname);
      if (ret) {
        const VertexAttribute vattr = ret.value();

        if (vattr.format != VertexAttributeFormat::Vec2) {
          PUSH_ERROR_AND_RETURN(
              fmt::format("Texcoord VertexAttribute must be Vec2 type.\n"));
        }

        if (vattr.element_size() != 1) {
          PUSH_ERROR_AND_RETURN(
              fmt::format("Multi-element UV texcoord attribute(`elementSize != 1` in USD Attribute metadataum)  is not supported "
                          "match to the number of facevarying elements {}\n",
                          vattr.vertex_count(), num_fvs));
        }


        DCOUT("Add texcoord attr `" << uvname << "` to slot Id " << slotId);

        if (vattr.variability == VertexVariability::Constant) {

          auto result = ConstantToFaceVarying(vattr.get_data(), vattr.stride_bytes(),
            dst.faceVertexCounts);
          if (!result) {
            PUSH_ERROR_AND_RETURN(
                fmt::format("Failed to convert 'constant' attribute to 'facevarying': {}", result.error()));
          }

          std::vector<vec2> uvs;
          uvs.resize(result.value().size() / sizeof(vec2));
          memcpy(uvs.data(), result.value().data(), result.value().size());

          dst.facevaryingTexcoords[uint32_t(slotId)] = uvs;


        } else if (vattr.variability == VertexVariability::Uniform) {
          auto result = UniformToFaceVarying(vattr.get_data(), vattr.stride_bytes(),
            dst.faceVertexCounts);
          if (!result) {
            PUSH_ERROR_AND_RETURN(
                fmt::format("Failed to convert 'uniform' attribute to 'facevarying': {}", result.error()));
          }

          std::vector<vec2> uvs;
          uvs.resize(result.value().size() / sizeof(vec2));
          memcpy(uvs.data(), result.value().data(), result.value().size());

          dst.facevaryingTexcoords[uint32_t(slotId)] = uvs;
        } else if ((vattr.variability == VertexVariability::Varying) ||
            (vattr.variability == VertexVariability::Vertex)) {
          auto result = VertexToFaceVarying(vattr.get_data(), vattr.stride_bytes(),
            dst.faceVertexCounts, dst.faceVertexIndices);
          if (!result) {
            PUSH_ERROR_AND_RETURN(
                fmt::format("Failed to convert 'vertex' or 'varying' attribute to 'facevarying': {}", result.error()));
          }

          std::vector<vec2> uvs;
          uvs.resize(result.value().size() / sizeof(vec2));
          memcpy(uvs.data(), result.value().data(), result.value().size());

          dst.facevaryingTexcoords[uint32_t(slotId)] = uvs;
        } else if (vattr.variability == VertexVariability::FaceVarying) {

          if (vattr.vertex_count() != num_fvs) {
            PUSH_ERROR_AND_RETURN(
                fmt::format("The number of UV texcoord attributes {} does not "
                            "match to the number of facevarying elements {}\n",
                            vattr.vertex_count(), num_fvs));
          }

          std::vector<vec2> uvs(vattr.vertex_count());
          memcpy(uvs.data(), vattr.data.data(), vattr.data.size());

          dst.facevaryingTexcoords[uint32_t(slotId)] = uvs;
        } else {
          PUSH_ERROR_AND_RETURN("Internal error. Invalid variability value in TexCoord attribute.");
        }

      } else {
        PUSH_ERROR_AND_RETURN(ret.error());
      }
    }
  }

  if (triangulate) {
    std::string err;

    std::vector<uint32_t> triangulatedFaceVertexCounts;  // should be all 3's
    std::vector<uint32_t> triangulatedFaceVertexIndices;
    std::vector<size_t> faceVertexIndexMap;
    if (!TriangulatePolygon<value::float3, float>(
            dst.points, dst.faceVertexCounts, dst.faceVertexIndices,
            triangulatedFaceVertexCounts, triangulatedFaceVertexIndices,
            faceVertexIndexMap, err)) {
      PUSH_ERROR_AND_RETURN("Triangulation failed: " + err);
    }

    dst.faceVertexCounts = std::move(triangulatedFaceVertexCounts);
    dst.faceVertexIndices = std::move(triangulatedFaceVertexIndices);

    if (dst.facevaryingNormals.size()) {
      std::vector<tydra::vec3> triangulatedFacevaryingNormals;

      for (size_t i = 0; i < faceVertexIndexMap.size(); i++) {
        size_t fvIdx = faceVertexIndexMap[i];
        triangulatedFacevaryingNormals.push_back(dst.facevaryingNormals[fvIdx]);
      }

      dst.facevaryingNormals = std::move(triangulatedFacevaryingNormals);
    }

    if (dst.facevaryingTexcoords.size()) {
      std::unordered_map<uint32_t, std::vector<tydra::vec2>>
          triangulatedFacevaryingTexcoords;

      for (auto &slot : dst.facevaryingTexcoords) {
        std::vector<tydra::vec2> texcoords;
        for (size_t i = 0; i < faceVertexIndexMap.size(); i++) {
          size_t fvIdx = faceVertexIndexMap[i];

          texcoords.push_back(slot.second[fvIdx]);
        }
        triangulatedFacevaryingTexcoords[slot.first] = texcoords;
      }

      dst.facevaryingTexcoords = std::move(triangulatedFacevaryingTexcoords);
    }

    // TODO: Triangulate other primvars

  }  // triangulate

#if 0  // TODO: GeomSubsets.
  // for GeomSubsets
  if (mesh.geom_subset_children.size()) {
    std::vector<size_t> faceVertexIndexOffsets;

    if (!BuildFaceVertexIndexOffsets(dst.faceVertexCounts,
                                     faceVertexIndexOffsets)) {
      PUSH_ERROR_AND_RETURN("Build faceVertexIndexOffsets failed.");
    }
  }
#endif

  (*dstMesh) = std::move(dst);
  return true;
}

namespace {

#if 1
// Convert UsdTranform2d -> PrimvarReader_float2 shader network.
nonstd::expected<bool, std::string> ConvertTexTransform2d(
    const Stage &stage, const Path &tx_abs_path, const UsdTransform2d &tx,
    UVTexture *tex_out) {
  float rotation;  // in angles
  if (!tx.rotation.get_value().get_scalar(&rotation)) {
    return nonstd::make_unexpected(
        fmt::format("Failed to retrieve rotation attribute from {}\n",
                    tx_abs_path.full_path_name()));
  }

  value::float2 scale;
  if (!tx.scale.get_value().get_scalar(&scale)) {
    return nonstd::make_unexpected(
        fmt::format("Failed to retrieve scale attribute from {}\n",
                    tx_abs_path.full_path_name()));
  }

  value::float2 translation;
  if (!tx.translation.get_value().get_scalar(&translation)) {
    return nonstd::make_unexpected(
        fmt::format("Failed to retrieve translation attribute from {}\n",
                    tx_abs_path.full_path_name()));
  }

  // must be authored and connected to PrimvarReader.
  if (!tx.in.authored()) {
    return nonstd::make_unexpected("`inputs:in` must be authored.\n");
  }

  if (!tx.in.is_connection()) {
    return nonstd::make_unexpected("`inputs:in` must be a connection.\n");
  }

  const auto &paths = tx.in.get_connections();
  if (paths.size() != 1) {
    return nonstd::make_unexpected(
        "`inputs:in` must be a single connection Path.\n");
  }

  std::string prim_part = paths[0].prim_part();
  std::string prop_part = paths[0].prop_part();

  if (prop_part != "outputs:result") {
    return nonstd::make_unexpected(
        "`inputs:in` connection Path's property part must be "
        "`outputs:result`\n");
  }

  std::string err;

  const Prim *pprim{nullptr};
  if (!stage.find_prim_at_path(Path(prim_part, ""), pprim, &err)) {
    return nonstd::make_unexpected(fmt::format(
        "`inputs:in` connection Path not found in the Stage. {}\n", prim_part));
  }

  if (!pprim) {
    return nonstd::make_unexpected(
        fmt::format("[InternalError] Prim is nullptr: {}\n", prim_part));
  }

  const Shader *pshader = pprim->as<Shader>();
  if (!pshader) {
    return nonstd::make_unexpected(
        fmt::format("{} must be Shader Prim, but got {}\n", prim_part,
                    pprim->prim_type_name()));
  }

  const UsdPrimvarReader_float2 *preader =
      pshader->value.as<UsdPrimvarReader_float2>();
  if (preader) {
    return nonstd::make_unexpected(fmt::format(
        "Shader {} must be UsdPrimvarReader_float2 type, but got {}\n",
        prim_part, pshader->info_id));
  }

  // Get value producing attribute(i.e, follow .connection and return
  // terminal Attribute value)
  value::token varname;
#if 0
  if (!tydra::EvaluateShaderAttribute(stage, *pshader, "inputs:varname",
                                      &varname, &err)) {
    return nonstd::make_unexpected(
        fmt::format("Failed to evaluate UsdPrimvarReader_float2's "
                    "inputs:varname: {}\n",
                    err));
  }
#else
  TerminalAttributeValue attr;
  if (!tydra::EvaluateAttribute(stage, *pprim, "inputs:varname", &attr, &err)) {
    return nonstd::make_unexpected(
        "`inputs:varname` evaluation failed: " + err + "\n");
  }
  if (auto pv = attr.as<value::token>()) {
    varname = *pv;
  } else {
    return nonstd::make_unexpected(
        "`inputs:varname` must be `token` type, but got " + attr.type_name() +
        "\n");
  }
  if (varname.str().empty()) {
    return nonstd::make_unexpected("`inputs:varname` is empty token\n");
  }
  DCOUT("inputs:varname = " << varname);
#endif

  // Build transform matrix.
  // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_texture_transform
  // Since USD uses post-multiply,
  //
  // matrix = scale * rotate * translate
  //
  {
    mat3 s;
    s.set_scale(scale[0], scale[1], 1.0f);

    mat3 r = mat3::identity();

    r.m[0][0] = std::cos(math::radian(rotation));
    r.m[0][1] = std::sin(math::radian(rotation));

    r.m[1][0] = -std::sin(math::radian(rotation));
    r.m[1][1] = std::cos(math::radian(rotation));

    mat3 t = mat3::identity();
    t.set_translation(translation[0], translation[1], 1.0f);

    tex_out->transform = s * r * t;
  }

  tex_out->tx_rotation = rotation;
  tex_out->tx_translation = translation;
  tex_out->tx_scale = scale;
  tex_out->has_transform2d = true;

  tex_out->varname_uv = varname.str();

  return true;
}
#endif

template <typename T>
nonstd::expected<bool, std::string> GetConnectedUVTexture(
    const Stage &stage, const TypedAnimatableAttributeWithFallback<T> &src,
    Path *tex_abs_path, const UsdUVTexture **dst, const Shader **shader_out) {
  if (!dst) {
    return nonstd::make_unexpected("[InternalError] dst is nullptr.\n");
  }

  if (!src.is_connection()) {
    return nonstd::make_unexpected("Attribute must be connection.\n");
  }

  if (src.get_connections().size() != 1) {
    return nonstd::make_unexpected(
        "Attribute connections must be single connection Path.\n");
  }

  //
  // Example: color3f inputs:diffuseColor.connect = </path/to/tex.outputs:rgb>
  //
  // => path.prim_part : /path/to/tex
  // => path.prop_part : outputs:rgb
  //

  const Path &path = src.get_connections()[0];

  const std::string prim_part = path.prim_part();
  const std::string prop_part = path.prop_part();

  // NOTE: no `outputs:rgba` in the spec.
  constexpr auto kOutputsRGB = "outputs:rgb";
  constexpr auto kOutputsR = "outputs:r";
  constexpr auto kOutputsG = "outputs:g";
  constexpr auto kOutputsB = "outputs:b";
  constexpr auto kOutputsA = "outputs:a";

  if (prop_part == kOutputsRGB) {
    // ok
  } else if (prop_part == kOutputsR) {
    // ok
  } else if (prop_part == kOutputsG) {
    // ok
  } else if (prop_part == kOutputsB) {
    // ok
  } else if (prop_part == kOutputsA) {
    // ok
  } else {
    return nonstd::make_unexpected(
        fmt::format("connection Path's property part must be `{}`, `{}`, `{}` or `{}` "
        "for "
        "UsdUVTexture, but got `{}`\n", kOutputsRGB, kOutputsR, kOutputsG, kOutputsB, kOutputsA, prop_part));
  }

  const Prim *prim{nullptr};
  std::string err;
  if (!stage.find_prim_at_path(Path(prim_part, ""), prim, &err)) {
    return nonstd::make_unexpected(
        fmt::format("Prim {} not found in the Stage: {}\n", prim_part, err));
  }

  if (!prim) {
    return nonstd::make_unexpected("[InternalError] Prim ptr is null.\n");
  }

  if (tex_abs_path) {
    (*tex_abs_path) = Path(prim_part, "");
  }

  if (const Shader *pshader = prim->as<Shader>()) {
    if (const UsdUVTexture *ptex = pshader->value.as<UsdUVTexture>()) {
      DCOUT("ptex = " << ptex);
      (*dst) = ptex;

      if (shader_out) {
        (*shader_out) = pshader;
      }

      return true;
    }
  }

  return nonstd::make_unexpected(fmt::format(
      "Prim {} must be `Shader` Prim type, but got `{}`", prim_part, prim->prim_type_name()));
}

}  // namespace

// W.I.P.
// Convert UsdUVTexture shader node.
// @return true upon conversion success(textures.back() contains the converted
// UVTexture)
//
// Possible network configuration
//
// - UsdUVTexture -> UsdPrimvarReader
// - UsdUVTexture -> UsdTransform2d -> UsdPrimvarReader
bool RenderSceneConverter::ConvertUVTexture(const Path &tex_abs_path,
                                            const AssetInfo &assetInfo,
                                            const UsdUVTexture &texture,
                                            UVTexture *tex_out) {
  DCOUT("ConvertUVTexture " << tex_abs_path);

  if (!tex_out) {
    PUSH_ERROR_AND_RETURN("tex_out arg is nullptr.");
  }
  std::string err;

  UVTexture tex;

  // First load texture file.
  if (!texture.file.authored()) {
    PUSH_ERROR_AND_RETURN(fmt::format("`asset:file` is not authored. Path = {}",
                                      tex_abs_path.prim_part()));
  }

  value::AssetPath assetPath;
  if (auto apath = texture.file.get_value()) {
    if (!apath.value().get_scalar(&assetPath)) {
      PUSH_ERROR_AND_RETURN(
          fmt::format("Failed to get `asset:file` value from Path {} (Maybe "
                      "`asset:file` is timeSample value?)",
                      tex_abs_path.prim_part()));
    }
  } else {
    PUSH_ERROR_AND_RETURN(
        fmt::format("Failed to get `asset:file` value from Path {}",
                    tex_abs_path.prim_part()));
  }

  // TextureImage and BufferData
  {
    TextureImage texImage;
    BufferData assetImageBuffer;

    // Texel data is treated as byte array
    assetImageBuffer.componentType = ComponentType::UInt8;
    assetImageBuffer.count = 1;

    if (_scene_config.load_texture_assets) {
      std::string warn;

      TextureImageLoaderFunction tex_loader_fun =
          _material_config.texture_image_loader_function;

      if (!tex_loader_fun) {
        tex_loader_fun = DefaultTextureImageLoaderFunction;
      }

      bool tex_ok = tex_loader_fun(
          assetPath, assetInfo, _asset_resolver, &texImage,
          &assetImageBuffer.data,
          _material_config.texture_image_loader_function_userdata, &warn, &err);

      if (!tex_ok && !_material_config.allow_texture_load_failure) {
        PUSH_ERROR_AND_RETURN("Failed to load texture image: " + err);
      }

      if (warn.size()) {
        DCOUT("WARN: " << warn);
        PushWarn(warn);
      }

      if (err.size()) {
        // report as warning.
        PushWarn(err);
      }

      // store unresolved asset path.
      texImage.asset_identifier = assetPath.GetAssetPath();

    } else {
      // store resolved asset path.
      texImage.asset_identifier =
          _asset_resolver.resolve(assetPath.GetAssetPath());
    }

    // colorSpace.
    // First look into `colorSpace` metadata of asset, then
    // look into `inputs:sourceColorSpace' attribute.
    if (texture.file.metas().has_colorSpace()) {
      ColorSpace cs;
      value::token cs_token = texture.file.metas().get_colorSpace();
      if (!from_token(cs_token, &cs)) {
        PUSH_ERROR_AND_RETURN(fmt::format("Invalid or unsupported token value for 'colorSpace': `{}` ", cs_token.str()));
      }
      texImage.usdColorSpace = cs;

    }

    {
      if (texture.sourceColorSpace.authored()) {
        UsdUVTexture::SourceColorSpace cs;
        if (texture.sourceColorSpace.get_value().get_scalar(&cs)) {
          if (cs == UsdUVTexture::SourceColorSpace::SRGB) {
            texImage.usdColorSpace = tydra::ColorSpace::sRGB;
          } else if (cs == UsdUVTexture::SourceColorSpace::Raw) {
            texImage.usdColorSpace = tydra::ColorSpace::Linear;
          } else if (cs == UsdUVTexture::SourceColorSpace::Auto) {
            // TODO: Read colorspace from a file.
            if ((texImage.assetTexelComponentType == ComponentType::UInt8) ||
                (texImage.assetTexelComponentType == ComponentType::Int8)) {
              texImage.usdColorSpace = tydra::ColorSpace::sRGB;
            } else {
              texImage.usdColorSpace = tydra::ColorSpace::Linear;
            }
          }
        }
      }
    }

    BufferData imageBuffer;

    // Linearlization and widen texel bit depth if required.
    if (_material_config.linearize_color_space) {
      size_t width = size_t(texImage.width);
      size_t height = size_t(texImage.height);
      size_t channels = size_t(texImage.channels);
      if (channels == 4) {
        PUSH_ERROR_AND_RETURN(
            fmt::format("TODO: RGBA color channels are not supported yet."));
      }
      if (channels > 4) {
        PUSH_ERROR_AND_RETURN(
            fmt::format("TODO: Multiband color channels(5 or more) are not "
                        "supported(yet)."));
      }

      if (assetImageBuffer.componentType == tydra::ComponentType::UInt8) {
        if (texImage.usdColorSpace == tydra::ColorSpace::sRGB) {
          if (_material_config.preserve_texel_bitdepth) {
            // u8 sRGB -> u8 Linear
            imageBuffer.componentType = tydra::ComponentType::UInt8;

            bool ret = srgb_8bit_to_linear_8bit(
                assetImageBuffer.data, width, height, channels,
                /* channel stride */ channels, &imageBuffer.data);
            if (!ret) {
              PUSH_ERROR_AND_RETURN(
                  "Failed to convert sRGB u8 image to Linear u8 image.");
            }

            imageBuffer.count = 1;

          } else {
            // u8 sRGB -> fp32 Linear
            imageBuffer.componentType = tydra::ComponentType::Float;

            std::vector<float> buf;
            bool ret = srgb_8bit_to_linear_f32(
                assetImageBuffer.data, width, height, channels,
                /* channel stride */ channels, &buf);
            if (!ret) {
              PUSH_ERROR_AND_RETURN(
                  "Failed to convert sRGB u8 image to Linear f32 image.");
            }

            imageBuffer.data.resize(buf.size() * sizeof(float));
            memcpy(imageBuffer.data.data(), buf.data(),
                   sizeof(float) * buf.size());
            imageBuffer.count = 1;
          }

          texImage.colorSpace = tydra::ColorSpace::Linear;

        } else if (texImage.usdColorSpace == tydra::ColorSpace::Linear) {
          if (_material_config.preserve_texel_bitdepth) {
            // no op.
            imageBuffer = std::move(assetImageBuffer);

          } else {
            // u8 -> fp32
            imageBuffer.componentType = tydra::ComponentType::Float;

            std::vector<float> buf;
            bool ret = u8_to_f32_image(assetImageBuffer.data, width, height,
                                       channels, &buf);
            if (!ret) {
              PUSH_ERROR_AND_RETURN("Failed to convert u8 image to f32 image.");
            }

            imageBuffer.data.resize(buf.size() * sizeof(float));
            memcpy(imageBuffer.data.data(), buf.data(),
                   sizeof(float) * buf.size());
            imageBuffer.count = 1;
          }

          texImage.colorSpace = tydra::ColorSpace::Linear;

        } else {
          PUSH_ERROR(fmt::format("TODO: Color space {}",
                                 to_string(texImage.usdColorSpace)));
        }

      } else if (assetImageBuffer.componentType ==
                 tydra::ComponentType::Float) {
        // ignore preserve_texel_bitdepth

        if (texImage.usdColorSpace == tydra::ColorSpace::sRGB) {
          // srgb f32 -> linear f32
          std::vector<float> in_buf;
          std::vector<float> out_buf;
          in_buf.resize(assetImageBuffer.data.size() / sizeof(float));
          memcpy(in_buf.data(), assetImageBuffer.data.data(),
                 in_buf.size() * sizeof(float));

          out_buf.resize(assetImageBuffer.data.size() / sizeof(float));

          bool ret =
              srgb_f32_to_linear_f32(in_buf, width, height, channels,
                                     /* channel stride */ channels, &out_buf);

          imageBuffer.data.resize(assetImageBuffer.data.size());
          memcpy(imageBuffer.data.data(), out_buf.data(),
                 imageBuffer.data.size());

          if (!ret) {
            PUSH_ERROR_AND_RETURN(
                "Failed to convert sRGB f32 image to Linear f32 image.");
          }

        } else if (texImage.usdColorSpace == tydra::ColorSpace::Linear) {
          // no op
          imageBuffer = std::move(assetImageBuffer);

        } else {
          PUSH_ERROR(fmt::format("TODO: Color space {}",
                                 to_string(texImage.usdColorSpace)));
        }

      } else {
        PUSH_ERROR(fmt::format("TODO: asset texture texel format {}",
                               to_string(assetImageBuffer.componentType)));
      }

    } else {
      // Same color space.

      if (assetImageBuffer.componentType == tydra::ComponentType::UInt8) {
        if (_material_config.preserve_texel_bitdepth) {
          // Do nothing.
          imageBuffer = std::move(assetImageBuffer);

        } else {
          // u8 to f32
          imageBuffer.componentType = tydra::ComponentType::Float;
        }

        texImage.colorSpace = texImage.usdColorSpace;

      } else if (assetImageBuffer.componentType ==
                 tydra::ComponentType::Float) {
        // ignore preserve_texel_bitdepth

        // f32 to f32, so no op
        imageBuffer = std::move(assetImageBuffer);

      } else {
        PUSH_ERROR(fmt::format("TODO: asset texture texel format {}",
                               to_string(assetImageBuffer.componentType)));
      }
    }

    // Assign buffer id
    texImage.buffer_id = int64_t(buffers.size());

    // TODO: Share image data as much as possible.
    // e.g. Texture A and B uses same image file, but texturing parameter is
    // different.
    buffers.emplace_back(imageBuffer);

    tex.texture_image_id = int64_t(images.size());

    images.emplace_back(texImage);

    std::stringstream ss;
    ss << "Loaded texture image " << assetPath.GetAssetPath()
       << " : buffer_id " + std::to_string(texImage.buffer_id) << "\n";
    ss << "  width x height x components " << texImage.width << " x "
       << texImage.height << " x " << texImage.channels << "\n";
    ss << "  colorSpace " << tinyusdz::tydra::to_string(texImage.colorSpace)
       << "\n";
    PushInfo(ss.str());
  }

  //
  // Set outputChannel
  //
  if (texture.outputsRGB.authored()) {
    tex.outputChannel = UVTexture::Channel::RGB;
  } else if (texture.outputsA.authored()) {
    tex.outputChannel = UVTexture::Channel::A;
  } else if (texture.outputsR.authored()) {
    tex.outputChannel = UVTexture::Channel::R;
  } else if (texture.outputsG.authored()) {
    tex.outputChannel = UVTexture::Channel::G;
  } else if (texture.outputsB.authored()) {
    tex.outputChannel = UVTexture::Channel::B;
  } else {
    PUSH_WARN("No valid output channel attribute authored. Default to RGB");
    tex.outputChannel = UVTexture::Channel::RGB;
  }

  //
  // Convert other UVTexture parameters
  //

  if (texture.bias.authored()) {
    tex.bias = texture.bias.get_value();
  }

  if (texture.scale.authored()) {
    tex.scale = texture.scale.get_value();
  }

  if (texture.st.authored()) {
    if (texture.st.is_connection()) {
      const auto &paths = texture.st.get_connections();
      if (paths.size() != 1) {
        PUSH_ERROR_AND_RETURN(
            "UsdUVTexture inputs:st connection must be single Path.");
      }
      const Path &path = paths[0];

      const Prim *readerPrim{nullptr};
      if (!_stage->find_prim_at_path(Path(path.prim_part(), ""), readerPrim,
                                     &err)) {
        PUSH_ERROR_AND_RETURN(
            "UsdUVTexture inputs:st connection targetPath not found in the "
            "Stage: " +
            err);
      }

      if (!readerPrim) {
        PUSH_ERROR_AND_RETURN(
            "[InternlError] Invalid Prim connected to inputs:st");
      }

      const Shader *pshader = readerPrim->as<Shader>();
      if (!pshader) {
        PUSH_ERROR_AND_RETURN(
            fmt::format("UsdUVTexture inputs:st connected Prim must be "
                        "Shader Prim, but got {} Prim",
                        readerPrim->prim_type_name()));
      }

      // currently UsdTranform2d or PrimvarReaer_float2 only for inputs:st
      if (const UsdPrimvarReader_float2 *preader =
              pshader->value.as<UsdPrimvarReader_float2>()) {
        if (!preader) {
          PUSH_ERROR_AND_RETURN(
              fmt::format("Shader's info:id must be UsdPrimvarReader_float2, "
                          "but got {}",
                          pshader->info_id));
        }

        // Get value producing attribute(i.e, follow .connection and return
        // terminal Attribute value)
        std::string varname;
        TerminalAttributeValue attr;
        if (!tydra::EvaluateAttribute(*_stage, *readerPrim, "inputs:varname",
                                      &attr, &err)) {
          PUSH_ERROR_AND_RETURN(
              fmt::format("Failed to evaluate UsdPrimvarReader_float2's "
                          "inputs:varname.\n{}",
                          err));
        }

        if (auto pv = attr.as<value::token>()) {
          varname = (*pv).str();
        } else if (auto pvs = attr.as<std::string>()) {
          varname = (*pvs);
        } else if (auto pvsd = attr.as<value::StringData>()) {
          varname = (*pvsd).value;
        } else {
          PUSH_ERROR_AND_RETURN(
              "`inputs:varname` must be `string` or `token` type, but got " +
              attr.type_name());
        }
        if (varname.empty()) {
          PUSH_ERROR_AND_RETURN("`inputs:varname` is empty token.");
        }
        DCOUT("inputs:varname = " << varname);

        tex.varname_uv = varname;
      } else if (const UsdTransform2d *ptransform =
                     pshader->value.as<UsdTransform2d>()) {
        auto result = ConvertTexTransform2d(*_stage, path, *ptransform, &tex);
        if (!result) {
          PUSH_ERROR_AND_RETURN(result.error());
        }
      } else {
        PUSH_ERROR_AND_RETURN(
            "Unsupported Shader type for `inputs:st` connection: " +
            pshader->info_id + "\n");
      }

    } else {
      Animatable<value::texcoord2f> fallbacks = texture.st.get_value();
      value::texcoord2f uv;
      if (fallbacks.get_scalar(&uv)) {
        tex.fallback_uv[0] = uv[0];
        tex.fallback_uv[1] = uv[1];
      } else {
        // TODO: report warning.
        PUSH_WARN(
            "Failed to get fallback `st` texcoord attribute. Maybe `st` is "
            "timeSamples attribute?\n");
      }
    }
  }

  if (texture.wrapS.authored()) {
    tinyusdz::UsdUVTexture::Wrap wrap;

    if (!texture.wrapS.get_value().get_scalar(&wrap)) {
      PUSH_ERROR_AND_RETURN("Invalid UsdUVTexture inputs:wrapS value.");
    }

    if (wrap == UsdUVTexture::Wrap::Repeat) {
      tex.wrapS = UVTexture::WrapMode::REPEAT;
    } else if (wrap == UsdUVTexture::Wrap::Mirror) {
      tex.wrapS = UVTexture::WrapMode::MIRROR;
    } else if (wrap == UsdUVTexture::Wrap::Clamp) {
      tex.wrapS = UVTexture::WrapMode::CLAMP_TO_EDGE;
    } else if (wrap == UsdUVTexture::Wrap::Black) {
      tex.wrapS = UVTexture::WrapMode::CLAMP_TO_BORDER;
    } else {
      tex.wrapS = UVTexture::WrapMode::CLAMP_TO_EDGE;
    }
  }

  if (texture.wrapT.authored()) {
    tinyusdz::UsdUVTexture::Wrap wrap;

    if (!texture.wrapT.get_value().get_scalar(&wrap)) {
      PUSH_ERROR_AND_RETURN("Invalid UsdUVTexture inputs:wrapT value.");
    }

    if (wrap == UsdUVTexture::Wrap::Repeat) {
      tex.wrapT = UVTexture::WrapMode::REPEAT;
    } else if (wrap == UsdUVTexture::Wrap::Mirror) {
      tex.wrapT = UVTexture::WrapMode::MIRROR;
    } else if (wrap == UsdUVTexture::Wrap::Clamp) {
      tex.wrapT = UVTexture::WrapMode::CLAMP_TO_EDGE;
    } else if (wrap == UsdUVTexture::Wrap::Black) {
      tex.wrapT = UVTexture::WrapMode::CLAMP_TO_BORDER;
    } else {
      tex.wrapT = UVTexture::WrapMode::CLAMP_TO_EDGE;
    }
  }

  DCOUT("Converted UVTexture.");

  (*tex_out) = tex;
  return true;
}

template <typename T, typename Dty>
bool RenderSceneConverter::ConvertPreviewSurfaceShaderParam(
    const Path &shader_abs_path,
    const TypedAttributeWithFallback<Animatable<T>> &param,
    const std::string &param_name, ShaderParam<Dty> &dst_param) {
  if (!param.authored()) {
    return true;
  }

  if (param.is_blocked()) {
    PUSH_ERROR_AND_RETURN(fmt::format("{} attribute is blocked.", param_name));
  } else if (param.is_connection()) {
    DCOUT(fmt::format("{] is attribute connection.", param_name));

    const UsdUVTexture *ptex{nullptr};
    const Shader *pshader{nullptr};
    Path texPath;
    auto result =
        GetConnectedUVTexture(*_stage, param, &texPath, &ptex, &pshader);

    if (!result) {
      PUSH_ERROR_AND_RETURN(result.error());
    }

    if (!ptex) {
      PUSH_ERROR_AND_RETURN("[InternalError] ptex is nullptr.");
    }
    DCOUT("ptex = " << ptex->name);

    if (!pshader) {
      PUSH_ERROR_AND_RETURN("[InternalError] pshader is nullptr.");
    }

    DCOUT("Get connected UsdUVTexture Prim: " << texPath);

    UVTexture rtex;
    const AssetInfo &assetInfo = pshader->metas().get_assetInfo();
    if (!ConvertUVTexture(texPath, assetInfo, *ptex, &rtex)) {
      PUSH_ERROR_AND_RETURN(fmt::format(
          "Failed to convert UVTexture connected to {}", param_name));
    }

    uint64_t texId = textures.size();
    textures.push_back(rtex);

    textureMap.add(texId, shader_abs_path.prim_part() + "." + param_name);

    DCOUT(fmt::format("TexId {} = {}",
                      shader_abs_path.prim_part() + ".diffuseColor", texId));

    dst_param.textureId = int32_t(texId);

    return true;
  } else {
    T val;
    if (!param.get_value().get_scalar(&val)) {
      PUSH_ERROR_AND_RETURN(
          fmt::format("Failed to get {} at `default` timecode.", param_name));
    }

    dst_param.set_value(val);

    return true;
  }
}

// TODO: timeSamples
bool RenderSceneConverter::ConvertPreviewSurfaceShader(
    const Path &shader_abs_path, const UsdPreviewSurface &shader,
    PreviewSurfaceShader *rshader_out) {
  if (!rshader_out) {
    PUSH_ERROR_AND_RETURN("rshader_out arg is nullptr.");
  }

  PreviewSurfaceShader rshader;

  if (shader.useSpecularWorkflow.authored()) {
    if (shader.useSpecularWorkflow.is_blocked()) {
      PUSH_ERROR_AND_RETURN(
          fmt::format("useSpecularWorkflow attribute is blocked."));
    } else if (shader.useSpecularWorkflow.is_connection()) {
      PUSH_ERROR_AND_RETURN(
          fmt::format("TODO: useSpecularWorkflow with connection."));
    } else {
      int val;
      if (!shader.useSpecularWorkflow.get_value().get_scalar(&val)) {
        PUSH_ERROR_AND_RETURN(fmt::format(
            "Failed to get useSpcularWorkFlow value at `default` timecode."));
      }

      rshader.useSpecularWorkFlow = val ? true : false;
    }
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.diffuseColor,
                                        "diffuseColor", rshader.diffuseColor)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.emissiveColor,
                                        "emissiveColor",
                                        rshader.emissiveColor)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.specularColor,
                                        "specularColor",
                                        rshader.specularColor)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.normal,
                                        "normal", rshader.normal)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.roughness,
                                        "roughness", rshader.roughness)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.metallic,
                                        "metallic", rshader.metallic)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.clearcoat,
                                        "clearcoat", rshader.clearcoat)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(
          shader_abs_path, shader.clearcoatRoughness, "clearcoatRoughness",
          rshader.clearcoatRoughness)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.opacity,
                                        "opacity", rshader.opacity)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(
          shader_abs_path, shader.opacityThreshold, "opacityThreshold",
          rshader.opacityThreshold)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.ior, "ior",
                                        rshader.ior)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.occlusion,
                                        "occlusion", rshader.occlusion)) {
    return false;
  }

  if (!ConvertPreviewSurfaceShaderParam(shader_abs_path, shader.displacement,
                                        "displacement", rshader.displacement)) {
    return false;
  }

  (*rshader_out) = rshader;
  return true;
}

bool RenderSceneConverter::ConvertMaterial(const Path &mat_abs_path,
                                           const tinyusdz::Material &material,
                                           RenderMaterial *rmat_out) {
  if (!_stage) {
    PUSH_ERROR_AND_RETURN("stage is nullptr.");
  }

  if (!rmat_out) {
    PUSH_ERROR_AND_RETURN("rmat_out argument is nullptr.");
  }

  RenderMaterial rmat;
  rmat.abs_path = mat_abs_path.prim_part();
  rmat.name = mat_abs_path.element_name();
  DCOUT("rmat.abs_path = " << rmat.abs_path);
  DCOUT("rmat.name = " << rmat.name);

  std::string err;

  Path surfacePath;

  //
  // surface shader
  {
    if (material.surface.authored()) {
      auto paths = material.surface.get_connections();
      DCOUT("paths = " << paths);
      // must have single targetPath.
      if (paths.size() != 1) {
        PUSH_ERROR_AND_RETURN(
            fmt::format("{}'s outputs:surface must be connection with single "
                        "target Path.\n",
                        mat_abs_path.full_path_name()));
      }
      surfacePath = paths[0];
    } else {
      PUSH_ERROR_AND_RETURN(
          fmt::format("{}'s outputs:surface isn't authored.\n",
                      mat_abs_path.full_path_name()));
    }

    const Prim *shaderPrim{nullptr};
    if (!_stage->find_prim_at_path(
            Path(surfacePath.prim_part(), /* prop part */ ""), shaderPrim,
            &err)) {
      PUSH_ERROR_AND_RETURN(fmt::format(
          "{}'s outputs:surface isn't connected to exising Prim path.\n",
          mat_abs_path.full_path_name()));
    }

    if (!shaderPrim) {
      // this should not happen though.
      PUSH_ERROR_AND_RETURN("[InternalError] invalid Shader Prim.\n");
    }

    const Shader *shader = shaderPrim->as<Shader>();

    if (!shader) {
      PUSH_ERROR_AND_RETURN(
          fmt::format("{}'s outputs:surface must be connected to Shader Prim, "
                      "but connected to `{}` Prim.\n",
                      shaderPrim->prim_type_name()));
    }

    // Currently must be UsdPreviewSurface
    const UsdPreviewSurface *psurface = shader->value.as<UsdPreviewSurface>();
    if (!psurface) {
      PUSH_ERROR_AND_RETURN(
          fmt::format("Shader's info:id must be UsdPreviewSurface, but got {}",
                      shader->info_id));
    }

    // prop part must be `outputs:surface` for now.
    if (surfacePath.prop_part() != "outputs:surface") {
      PUSH_ERROR_AND_RETURN(
          fmt::format("{}'s outputs:surface connection must point to property "
                      "`outputs:surface`, but got `{}`",
                      mat_abs_path.full_path_name(), surfacePath.prop_part()));
    }

    PreviewSurfaceShader pss;
    if (!ConvertPreviewSurfaceShader(surfacePath, *psurface, &pss)) {
      PUSH_ERROR_AND_RETURN(fmt::format(
          "Failed to convert UsdPreviewSurface : {}", surfacePath.prim_part()));
    }

    rmat.surfaceShader = pss;
  }

  DCOUT("Converted Material: " << mat_abs_path);

  (*rmat_out) = rmat;
  return true;
}

namespace {

bool MeshVisitor(const tinyusdz::Path &abs_path, const tinyusdz::Prim &prim,
                 const int32_t level, void *userdata, std::string *err) {
  if (!userdata) {
    return false;
  }

  RenderSceneConverter *converter =
      reinterpret_cast<RenderSceneConverter *>(userdata);

  if (level > 1024 * 1024) {
    if (err) {
      (*err) += "Scene graph is too deep.\n";
    }
    // Too deep
    return false;
  }

  if (const tinyusdz::GeomMesh *pmesh = prim.as<tinyusdz::GeomMesh>()) {
    // Collect GeomSubsets
    // std::vector<const tinyusdz::GeomSubset *> subsets = GetGeomSubsets(;

    DCOUT("Material: " << abs_path);

    //
    // First convert Material
    //

    const std::string mesh_path_str = abs_path.full_path_name();

    std::vector<RenderMaterial> &rmaterials = converter->materials;

    tinyusdz::Path bound_material_path;
    const tinyusdz::Material *bound_material{nullptr};
    bool ret = tinyusdz::tydra::FindBoundMaterial(
        *converter->GetStagePtr(), /* GeomMesh prim path */ abs_path,
        /* suffix */ "", &bound_material_path, &bound_material, err);

    int64_t rmaterial_id = -1;

    if (ret && bound_material) {
      DCOUT("Bound material path: " << bound_material_path);

      const auto matIt =
          converter->materialMap.find(bound_material_path.full_path_name());

      if (matIt != converter->materialMap.s_end()) {
        // Got material in the cache.
        uint64_t mat_id = matIt->second;
        if (mat_id >=
            converter->materials.size()) {  // this should not happen though
          if (err) {
            (*err) += "Material index out-of-range.\n";
          }
          return false;
        }

        if (mat_id >= (std::numeric_limits<int64_t>::max)()) {
          if (err) {
            (*err) += "Material index too large.\n";
          }
          return false;
        }

        rmaterial_id = int64_t(mat_id);

      } else {
        RenderMaterial rmat;
        if (!converter->ConvertMaterial(bound_material_path, *bound_material,
                                        &rmat)) {
          if (err) {
            (*err) += fmt::format("Material conversion failed: {}",
                                  bound_material_path);
          }
          return false;
        }

        // Assign new material ID
        uint64_t mat_id = rmaterials.size();

        if (mat_id >= (std::numeric_limits<int64_t>::max)()) {
          if (err) {
            (*err) += "Material index too large.\n";
          }
          return false;
        }
        rmaterial_id = int64_t(mat_id);

        converter->materialMap.add(bound_material_path.full_path_name(),
                                   uint64_t(rmaterial_id));
        DCOUT("Add material: " << mat_id << " " << rmat.abs_path << " ( "
                               << rmat.name << " ) ");

        rmaterials.push_back(rmat);
      }
    }

    RenderMesh rmesh;

    if (!converter->ConvertMesh(rmaterial_id, *pmesh, &rmesh)) {
      if (err) {
        (*err) += fmt::format("Mesh conversion failed: {}",
                              abs_path.full_path_name());
      }
      return false;
    }

    DCOUT("renderMaterialId = " << rmaterial_id);

    // Do not assign materialIds when no material bound to this Mesh.
    // TODO: per-face material.
    if ((rmaterial_id > -1) &&
        (rmaterial_id < (std::numeric_limits<int32_t>::max)())) {
      rmesh.materialIds.resize(rmesh.faceVertexCounts.size(),
                               int32_t(rmaterial_id));
    }

    converter->meshes.emplace_back(std::move(rmesh));
  }

  return true;  // continue traversal
}
}  // namespace

bool RenderSceneConverter::ConvertToRenderScene(const Stage &stage,
                                                RenderScene *scene) {
  if (!scene) {
    PUSH_ERROR_AND_RETURN("nullptr for RenderScene argument.");
  }

  _stage = &stage;

  // Build Xform at default time.
  XformNode xform_node;
  if (!BuildXformNodeFromStage(stage, &xform_node)) {
    PUSH_ERROR_AND_RETURN("Failed to build Xform node hierarchy.\n");
  }

  // W.I.P.

  RenderScene render_scene;

  // 1. Visit GeomMesh
  // 2. If the mesh has bound material
  //   1. Create Material
  //
  // TODO: GeomSubset(per-face material)

  std::string err;

  // Pass `this` through userdata ptr
  bool ret = tydra::VisitPrims(stage, MeshVisitor, this, &err);

  if (!ret) {
    _err += err;

    return false;
  }

  // render_scene.meshMap = std::move(meshMap);
  // render_scene.materialMap = std::move(materialMap);
  // render_scene.textureMap = std::move(textureMap);
  // render_scene.imageMap = std::move(imageMap);
  // render_scene.bufferMap = std::move(bufferMap);

  render_scene.nodes = std::move(nodes);
  render_scene.meshes = std::move(meshes);
  render_scene.textures = std::move(textures);
  render_scene.images = std::move(images);
  render_scene.buffers = std::move(buffers);
  render_scene.materials = std::move(materials);

  (*scene) = std::move(render_scene);
  return true;
}

bool DefaultTextureImageLoaderFunction(const value::AssetPath &assetPath,
                                       const AssetInfo &assetInfo,
                                       AssetResolutionResolver &assetResolver,
                                       TextureImage *texImageOut,
                                       std::vector<uint8_t> *imageData,
                                       void *userdata, std::string *warn,
                                       std::string *err) {
  if (!texImageOut) {
    if (err) {
      (*err) = "`imageOut` argument is nullptr\n";
    }
    return false;
  }

  if (!imageData) {
    if (err) {
      (*err) = "`imageData` argument is nullptr\n";
    }
    return false;
  }

  // TODO: assetInfo
  (void)assetInfo;
  (void)userdata;
  (void)warn;

  std::string resolvedPath = assetResolver.resolve(assetPath.GetAssetPath());

  if (resolvedPath.empty()) {
    if (err) {
      (*err) += fmt::format("Failed to resolve asset path: {}\n",
                            assetPath.GetAssetPath());
    }
    return false;
  }

  DCOUT("Resolved asset path = " << resolvedPath);
  auto result = tinyusdz::image::LoadImageFromFile(resolvedPath);
  if (!result) {
    if (err) {
      (*err) += "Failed to load image file: " + result.error() + "\n";
    }
    return false;
  }

  TextureImage texImage;

  texImage.asset_identifier = resolvedPath;
  texImage.channels = result.value().image.channels;

  if (result.value().image.bpp == 8) {
    // assume uint8
    texImage.assetTexelComponentType = ComponentType::UInt8;
  } else {
    DCOUT("TODO: bpp = " << result.value().image.bpp);
    if (err) {
      (*err) = "TODO or unsupported bpp: " +
               std::to_string(result.value().image.bpp) + "\n";
    }
    return false;
  }

  texImage.channels = result.value().image.channels;
  texImage.width = result.value().image.width;
  texImage.height = result.value().image.height;

  (*texImageOut) = texImage;

  // raw image data
  (*imageData) = result.value().image.data;

  return true;
}

std::string to_string(ColorSpace cty) {
  std::string s;
  switch (cty) {
    case ColorSpace::sRGB: {
      s = "srgb";
      break;
    }
    case ColorSpace::Linear: {
      s = "linear";
      break;
    }
    case ColorSpace::Rec709: {
      s = "rec709";
      break;
    }
    case ColorSpace::OCIO: {
      s = "ocio";
      break;
    }
    case ColorSpace::Lin_DisplayP3: {
      s = "lin_displayp3";
      break;
    }
    case ColorSpace::sRGB_DisplayP3: {
      s = "srgb_displayp3";
      break;
    }
    case ColorSpace::Custom: {
      s = "custom";
      break;
    }
  }

  return s;
}

bool from_token(const value::token &tok, ColorSpace *cty) {
  if (!cty) {
    return false;
  }

  if (tok.str() == "raw") {
    (*cty) = ColorSpace::Linear; 
  } else if (tok.str() == "Raw") { // NOTE: Seems uncommon token
    (*cty) = ColorSpace::Linear; 
  } else if (tok.str() == "srgb") {
    (*cty) = ColorSpace::sRGB; 
  } else if (tok.str() == "linear") {
    (*cty) = ColorSpace::Linear; 
  } else if (tok.str() == "rec709") {
    (*cty) = ColorSpace::Rec709;
  } else if (tok.str() == "ocio") {
    (*cty) = ColorSpace::OCIO;
  } else if (tok.str() == "lin_displayp3") {
    (*cty) =ColorSpace::Lin_DisplayP3;
  } else if (tok.str() == "srgb_displayp3") {
    (*cty) = ColorSpace::sRGB_DisplayP3;
  } else if (tok.str() == "custom") {
    (*cty) = ColorSpace::Custom;
  } else {
    return false;
  }

  return true;
}

std::string to_string(ComponentType cty) {
  std::string s;
  switch (cty) {
    case ComponentType::UInt8: {
      s = "uint8";
      break;
    }
    case ComponentType::Int8: {
      s = "int8";
      break;
    }
    case ComponentType::UInt16: {
      s = "uint16";
      break;
    }
    case ComponentType::Int16: {
      s = "int16";
      break;
    }
    case ComponentType::UInt32: {
      s = "uint32";
      break;
    }
    case ComponentType::Int32: {
      s = "int32";
      break;
    }
    case ComponentType::Half: {
      s = "half";
      break;
    }
    case ComponentType::Float: {
      s = "float";
      break;
    }
    case ComponentType::Double: {
      s = "double";
      break;
    }
  }

  return s;
}

std::string to_string(UVTexture::WrapMode mode) {
  std::string s;
  switch (mode) {
    case UVTexture::WrapMode::REPEAT: {
      s = "repeat";
      break;
    }
    case UVTexture::WrapMode::CLAMP_TO_BORDER: {
      s = "clamp_to_border";
      break;
    }
    case UVTexture::WrapMode::CLAMP_TO_EDGE: {
      s = "clamp_to_edge";
      break;
    }
    case UVTexture::WrapMode::MIRROR: {
      s = "mirror";
      break;
    }
  }

  return s;
}

namespace {

std::string DumpMesh(const RenderMesh &mesh, uint32_t indent) {
  std::stringstream ss;

  ss << "RenderMesh {\n";

  ss << pprint::Indent(indent + 1) << "num_points "
     << std::to_string(mesh.points.size()) << "\n";
  ss << pprint::Indent(indent + 1) << "points \""
     << value::print_array_snipped(mesh.points) << "\"\n";
  ss << pprint::Indent(indent + 1) << "num_faceVertexCounts "
     << std::to_string(mesh.faceVertexCounts.size()) << "\n";
  ss << pprint::Indent(indent + 1) << "faceVertexCounts \""
     << value::print_array_snipped(mesh.faceVertexCounts) << "\"\n";
  ss << pprint::Indent(indent + 1) << "num_faceVertexIndices "
     << std::to_string(mesh.faceVertexIndices.size()) << "\n";
  ss << pprint::Indent(indent + 1) << "faceVertexIndices \""
     << value::print_array_snipped(mesh.faceVertexIndices) << "\"\n";
  ss << pprint::Indent(indent + 1) << "num_materialIds "
     << std::to_string(mesh.materialIds.size()) << "\n";
  ss << pprint::Indent(indent + 1) << "materialIds \""
     << value::print_array_snipped(mesh.materialIds) << "\"\n";
  ss << pprint::Indent(indent + 1) << "num_facevaryingNormals "
     << mesh.facevaryingNormals.size() << "\n";
  ss << pprint::Indent(indent + 1) << "facevaryingNormals \""
     << value::print_array_snipped(mesh.facevaryingNormals) << "\"\n";
  ss << pprint::Indent(indent + 1) << "num_texcoordSlots "
     << std::to_string(mesh.facevaryingTexcoords.size()) << "\n";
  for (const auto &uvs : mesh.facevaryingTexcoords) {
    ss << pprint::Indent(indent + 1) << "num_facevaryingTexcoords_"
       << std::to_string(uvs.first) << " " << uvs.second.size() << "\n";
    ss << pprint::Indent(indent + 2) << "facevaryingTexcoords_" << uvs.first << " \""
       << value::print_array_snipped(uvs.second) << "\"\n";
  }

  // TODO: primvars

  ss << "\n";

  ss << pprint::Indent(indent) << "}\n";

  return ss.str();
}

std::string DumpPreviewSurface(const PreviewSurfaceShader &shader,
                               uint32_t indent) {
  std::stringstream ss;

  ss << "PreviewSurfaceShader {\n";

  ss << pprint::Indent(indent + 1)
     << "useSpecularWorkFlow = " << std::to_string(shader.useSpecularWorkFlow)
     << "\n";

  ss << pprint::Indent(indent + 1) << "diffuseColor = ";
  if (shader.diffuseColor.is_texture()) {
    ss << "textureId[" << shader.diffuseColor.textureId << "]";
  } else {
    ss << shader.diffuseColor.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent + 1) << "metallic = ";
  if (shader.metallic.is_texture()) {
    ss << "textureId[" << shader.metallic.textureId << "]";
  } else {
    ss << shader.metallic.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent + 1) << "roughness = ";
  if (shader.roughness.is_texture()) {
    ss << "textureId[" << shader.roughness.textureId << "]";
  } else {
    ss << shader.roughness.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent + 1) << "ior = ";
  if (shader.ior.is_texture()) {
    ss << "textureId[" << shader.ior.textureId << "]";
  } else {
    ss << shader.ior.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent + 1) << "clearcoat = ";
  if (shader.clearcoat.is_texture()) {
    ss << "textureId[" << shader.clearcoat.textureId << "]";
  } else {
    ss << shader.clearcoat.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent + 1) << "clearcoatRoughness = ";
  if (shader.clearcoatRoughness.is_texture()) {
    ss << "textureId[" << shader.clearcoatRoughness.textureId << "]";
  } else {
    ss << shader.clearcoatRoughness.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent + 1) << "opacity = ";
  if (shader.opacity.is_texture()) {
    ss << "textureId[" << shader.opacity.textureId << "]";
  } else {
    ss << shader.opacity.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent + 1) << "opacityThreshold = ";
  if (shader.opacityThreshold.is_texture()) {
    ss << "textureId[" << shader.opacityThreshold.textureId << "]";
  } else {
    ss << shader.opacityThreshold.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent + 1) << "normal = ";
  if (shader.normal.is_texture()) {
    ss << "textureId[" << shader.normal.textureId << "]";
  } else {
    ss << shader.normal.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent + 1) << "displacement = ";
  if (shader.displacement.is_texture()) {
    ss << "textureId[" << shader.displacement.textureId << "]";
  } else {
    ss << shader.displacement.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent + 1) << "occlusion = ";
  if (shader.occlusion.is_texture()) {
    ss << "textureId[" << shader.occlusion.textureId << "]";
  } else {
    ss << shader.occlusion.value;
  }
  ss << "\n";

  ss << pprint::Indent(indent) << "}\n";

  return ss.str();
}

std::string DumpMaterial(const RenderMaterial &material, uint32_t indent) {
  std::stringstream ss;

  ss << "RenderMaterial " << material.abs_path << " ( " << material.name
     << " ) {\n";

  ss << pprint::Indent(indent + 1) << "surfaceShader = ";
  ss << DumpPreviewSurface(material.surfaceShader, indent + 1);
  ss << "\n";

  ss << pprint::Indent(indent) << "}\n";

  return ss.str();
}

std::string DumpUVTexture(const UVTexture &texture, uint32_t indent) {
  std::stringstream ss;

  // TODO
  ss << "UVTexture {\n";
  ss << pprint::Indent(indent + 1) << "primvar_name " << texture.varname_uv
     << "\n";
  ss << pprint::Indent(indent + 1) << "outputChannel " << to_string(texture.outputChannel) << "\n";
  ss << pprint::Indent(indent + 1) << "bias " << texture.bias << "\n";
  ss << pprint::Indent(indent + 1) << "scale " << texture.scale << "\n";
  ss << pprint::Indent(indent + 1) << "wrapS " << to_string(texture.wrapS)
     << "\n";
  ss << pprint::Indent(indent + 1) << "wrapT " << to_string(texture.wrapT)
     << "\n";
  ss << pprint::Indent(indent + 1) << "fallback_uv " << texture.fallback_uv
     << "\n";
  ss << pprint::Indent(indent + 1) << "textureImageID "
     << std::to_string(texture.texture_image_id) << "\n";
  ss << pprint::Indent(indent + 1) << "has UsdTransform2d "
     << std::to_string(texture.has_transform2d) << "\n";
  if (texture.has_transform2d) {
    ss << pprint::Indent(indent + 2) << "rotation " << texture.tx_rotation
       << "\n";
    ss << pprint::Indent(indent + 2) << "scale " << texture.tx_scale << "\n";
    ss << pprint::Indent(indent + 2) << "translation " << texture.tx_translation
       << "\n";
    ss << pprint::Indent(indent + 2) << "computed_transform "
       << texture.transform << "\n";
  }

  ss << "\n";

  ss << pprint::Indent(indent) << "}\n";

  return ss.str();
}

std::string DumpImage(const TextureImage &image, uint32_t indent) {
  std::stringstream ss;

  ss << "TextureImage {\n";
  ss << pprint::Indent(indent + 1) << "asset_identifier \""
     << image.asset_identifier << "\"\n";
  ss << pprint::Indent(indent + 1) << "channels "
     << std::to_string(image.channels) << "\n";
  ss << pprint::Indent(indent + 1) << "width " << std::to_string(image.width)
     << "\n";
  ss << pprint::Indent(indent + 1) << "height " << std::to_string(image.height)
     << "\n";
  ss << pprint::Indent(indent + 1) << "miplevel "
     << std::to_string(image.miplevel) << "\n";
  ss << pprint::Indent(indent + 1) << "colorSpace "
     << to_string(image.colorSpace) << "\n";
  ss << pprint::Indent(indent + 1) << "bufferID "
     << std::to_string(image.buffer_id) << "\n";

  ss << "\n";

  ss << pprint::Indent(indent) << "}\n";

  return ss.str();
}

std::string DumpBuffer(const BufferData &buffer, uint32_t indent) {
  std::stringstream ss;

  ss << "Buffer {\n";
  ss << pprint::Indent(indent + 1) << "bytes " << buffer.data.size() << "\n";
  ss << pprint::Indent(indent + 1) << "count " << std::to_string(buffer.count)
     << "\n";
  ss << pprint::Indent(indent + 1) << "componentType "
     << to_string(buffer.componentType) << "\n";

  ss << "\n";

  ss << pprint::Indent(indent) << "}\n";

  return ss.str();
}

}  // namespace

std::string DumpRenderScene(const RenderScene &scene,
                            const std::string &format) {
  std::stringstream ss;

  // Currently kdl only.
  if (format == "json") {
    // not supported yet.
  }

  // KDL does not support array, so quote it as done in USD

  ss << "title RenderScene\n";
  ss << "// # of Meshes : " << scene.meshes.size() << "\n";
  ss << "// # of Animations : " << scene.animations.size() << "\n";
  ss << "// # of Materials : " << scene.materials.size() << "\n";
  ss << "// # of UVTextures : " << scene.textures.size() << "\n";
  ss << "// # of TextureImages : " << scene.images.size() << "\n";
  ss << "// # of Buffers : " << scene.buffers.size() << "\n";

  ss << "\n";

  ss << "meshes {\n";
  for (size_t i = 0; i < scene.meshes.size(); i++) {
    ss << "[" << i << "] " << DumpMesh(scene.meshes[i], 1);
  }
  ss << "}\n";

  ss << "\n";
  ss << "materials {\n";
  for (size_t i = 0; i < scene.materials.size(); i++) {
    ss << "[" << i << "] " << DumpMaterial(scene.materials[i], 1);
  }
  ss << "}\n";

  ss << "\n";
  ss << "textures {\n";
  for (size_t i = 0; i < scene.textures.size(); i++) {
    ss << "[" << i << "] " << DumpUVTexture(scene.textures[i], 1);
  }
  ss << "}\n";

  ss << "\n";
  ss << "images {\n";
  for (size_t i = 0; i < scene.images.size(); i++) {
    ss << "[" << i << "] " << DumpImage(scene.images[i], 1);
  }
  ss << "}\n";

  ss << "\n";
  ss << "buffers {\n";
  for (size_t i = 0; i < scene.buffers.size(); i++) {
    ss << "[" << i << "] " << DumpBuffer(scene.buffers[i], 1);
  }
  ss << "}\n";

  // ss << "TODO: Animations, ...\n";

  return ss.str();
}

}  // namespace tydra
}  // namespace tinyusdz
