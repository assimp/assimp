// SPDX-License-Identifier: Apache 2.0
// Copyright 2022 - 2023, Syoyo Fujita.
// Copyright 2023 - Present, Light Transport Entertainment Inc.
//
// UsdGeom API implementations


#include <sstream>

#include "pprinter.hh"
#include "value-types.hh"
#include "prim-types.hh"
#include "str-util.hh"
#include "tiny-format.hh"
#include "xform.hh"
#include "usdGeom.hh"
//
//#include "external/simple_match/include/simple_match/simple_match.hpp"
//
#include "common-macros.inc"
#include "math-util.inc"
#include "str-util.hh"
#include "value-pprint.hh"

#define SET_ERROR_AND_RETURN(msg) \
  if (err) {                      \
    (*err) = (msg);               \
  }                               \
  return false

#if 0
// NOTE: Some types are not supported on pxrUSD(e.g. string)
#define APPLY_GEOMPRIVAR_TYPE(__FUNC) \
  __FUNC(value::half)                 \
  __FUNC(value::half2)                \
  __FUNC(value::half3)                \
  __FUNC(value::half4)                \
  __FUNC(int)                         \
  __FUNC(value::int2)                 \
  __FUNC(value::int3)                 \
  __FUNC(value::int4)                 \
  __FUNC(uint32_t)                    \
  __FUNC(value::uint2)                \
  __FUNC(value::uint3)                \
  __FUNC(value::uint4)                \
  __FUNC(float)                       \
  __FUNC(value::float2)               \
  __FUNC(value::float3)               \
  __FUNC(value::float4)               \
  __FUNC(double)                      \
  __FUNC(value::double2)              \
  __FUNC(value::double3)              \
  __FUNC(value::double4)              \
  __FUNC(value::matrix2d)             \
  __FUNC(value::matrix3d)             \
  __FUNC(value::matrix4d)             \
  __FUNC(value::quath)                \
  __FUNC(value::quatf)                \
  __FUNC(value::quatd)                \
  __FUNC(value::normal3h)             \
  __FUNC(value::normal3f)             \
  __FUNC(value::normal3d)             \
  __FUNC(value::vector3h)             \
  __FUNC(value::vector3f)             \
  __FUNC(value::vector3d)             \
  __FUNC(value::point3h)              \
  __FUNC(value::point3f)              \
  __FUNC(value::point3d)              \
  __FUNC(value::color3f)              \
  __FUNC(value::color3d)              \
  __FUNC(value::color4f)              \
  __FUNC(value::color4d)              \
  __FUNC(value::texcoord2h)           \
  __FUNC(value::texcoord2f)           \
  __FUNC(value::texcoord2d)           \
  __FUNC(value::texcoord3h)           \
  __FUNC(value::texcoord3f)           \
  __FUNC(value::texcoord3d)
#endif

// TODO: Followings are not supported on pxrUSD. Enable it in TinyUSDZ?
#if 0
 __FUNC(int64_t) \
  __FUNC(uint64_t) \
  __FUNC(std::string) \
  __FUNC(bool)
#endif

namespace tinyusdz {

namespace {

constexpr auto kPrimvars = "primvars:";
constexpr auto kIndices = ":indices";
constexpr auto kPrimvarsNormals = "primvars:normals";

///
/// Computes
///
///  for i in len(indices):
///    dest[i] = values[indices[i]]
///
/// `dest` = `values` when `indices` is empty
///
template <typename T>
nonstd::expected<bool, std::string> ExpandWithIndices(
    const std::vector<T> &values, const std::vector<int32_t> &indices,
    std::vector<T> *dest) {
  if (!dest) {
    return nonstd::make_unexpected("`dest` is nullptr.");
  }

  if (indices.empty()) {
    (*dest) = values;
    return true;
  }

  dest->resize(indices.size());

  std::vector<size_t> invalidIndices;

  bool valid = true;
  for (size_t i = 0; i < indices.size(); i++) {
    int32_t idx = indices[i];
    if ((idx >= 0) && (size_t(idx) < values.size())) {
      (*dest)[i] = values[size_t(idx)];
    } else {
      invalidIndices.push_back(i);
      valid = false;
    }
  }

  if (invalidIndices.size()) {
    return nonstd::make_unexpected(
        "Invalid indices found: " +
        value::print_array_snipped(invalidIndices,
                                   /* N to display */ 5));
  }

  return valid;
}

}  // namespace

bool IsSupportedGeomPrimvarType(uint32_t tyid) {
  //
  // scalar and 1D
  //
#define SUPPORTED_TYPE_FUN(__ty)                                           \
  case value::TypeTraits<__ty>::type_id(): {                                 \
    return true;                                                           \
  }                                                                        \
  case (value::TypeTraits<__ty>::type_id() | value::TYPE_ID_1D_ARRAY_BIT): { \
    return true;                                                           \
  }

  switch (tyid) {
    APPLY_GEOMPRIVAR_TYPE(SUPPORTED_TYPE_FUN)
    default:
      return false;
  }

#undef SUPPORTED_TYPE_FUN
}

bool IsSupportedGeomPrimvarType(const std::string &type_name) {
  return IsSupportedGeomPrimvarType(value::GetTypeId(type_name));
}

bool GeomPrimvar::has_elementSize() const {
  return _elementSize.has_value();
}

uint32_t GeomPrimvar::get_elementSize() const {
  if (_elementSize.has_value()) {
    return _elementSize.value();
  }
  return 1;
}

bool GeomPrimvar::has_interpolation() const {
  return _interpolation.has_value();
}

Interpolation GeomPrimvar::get_interpolation() const {
  if (_interpolation.has_value()) {
    return _interpolation.value();
  }

  return Interpolation::Constant;  // unauthored
}

bool GPrim::has_primvar(const std::string &varname) const {
  std::string primvar_name = kPrimvars + varname;
  return props.count(primvar_name);
}

bool GPrim::get_primvar(const std::string &varname, GeomPrimvar *out_primvar,
                        std::string *err) const {
  if (!out_primvar) {
    SET_ERROR_AND_RETURN("Output GeomPrimvar is nullptr.");
  }

  GeomPrimvar primvar;

  std::string primvar_name = kPrimvars + varname;

  const auto it = props.find(primvar_name);
  if (it == props.end()) {
    return false;
  }

  // Currently connection attribute is not supported.
  if (it->second.is_attribute()) {
    const Attribute &attr = it->second.get_attribute();

    primvar.set_value(attr);
    primvar.set_name(varname);
    if (attr.metas().interpolation.has_value()) {
      primvar.set_interpolation(attr.metas().interpolation.value());
    }
    if (attr.metas().elementSize.has_value()) {
      primvar.set_elementSize(attr.metas().elementSize.value());
    }

  } else {
    SET_ERROR_AND_RETURN("GeomPrimvar of non-Attribute property is not supported.");
  }

  // has indices?
  std::string index_name = primvar_name + kIndices;
  const auto indexIt = props.find(index_name);

  if (indexIt != props.end()) {
    if (indexIt->second.is_attribute()) {
      const Attribute &indexAttr = indexIt->second.get_attribute();

      if (indexAttr.is_connection()) {
        SET_ERROR_AND_RETURN(
            "TODO: Connetion is not supported for index Attribute at the "
            "moment.");
      } else if (indexAttr.is_timesamples()) {
        SET_ERROR_AND_RETURN(
            "TODO: Index attribute with timeSamples is not supported yet.");
      } else if (indexAttr.is_blocked()) {
        SET_ERROR_AND_RETURN("TODO: Index attribute is blocked(ValueBlock).");
      } else if (indexAttr.is_value()) {
        // Check if int[] type.
        // TODO: Support uint[]?
        std::vector<int32_t> indices;
        if (!indexAttr.get_value(&indices)) {
          SET_ERROR_AND_RETURN(
              fmt::format("Index Attribute is not int[] type. Got {}",
                          indexAttr.type_name()));
        }

        if (!(primvar.get_attribute().type_id() & value::TYPE_ID_1D_ARRAY_BIT)) {
          SET_ERROR_AND_RETURN(
              fmt::format("Indexed GeomPrimVar for scalar PrimVar Attribute is not supported. PrimVar name: {}", primvar_name));
        }

        primvar.set_indices(indices);
      } else {
        SET_ERROR_AND_RETURN("[Internal Error] Invalid Index Attribute.");
      }
    } else {
      // indices are optional, so ok to skip it.
    }
  }

  (*out_primvar) = primvar;

  return true;
}

template <typename T>
bool GeomPrimvar::flatten_with_indices(std::vector<T> *dest, std::string *err) const {
  if (!dest) {
    if (err) {
      (*err) += "Output value is nullptr.";
    }
    return false;
  }

  if (_attr.is_timesamples()) {
    if (err) {
      (*err) += "TimeSamples attribute is TODO.";
    }
    return false;
  }

  if (_attr.is_value()) {
    if (!IsSupportedGeomPrimvarType(_attr.type_id())) {
      if (err) {
        (*err) += fmt::format("Unsupported type for GeomPrimvar. type = `{}`",
                              _attr.type_name());
      }
      return false;
    }

    if (auto pv = _attr.get_value<std::vector<T>>()) {
      std::vector<T> expanded_val;
      auto ret = ExpandWithIndices(pv.value(), _indices, &expanded_val);
      if (ret) {
        (*dest) = expanded_val;
        // Currently we ignore ret.value()
        return true;
      } else {
        const std::string &err_msg = ret.error();
        if (err) {
          (*err) += fmt::format(
              "[Internal Error] Failed to expand for GeomPrimvar type = `{}`",
              _attr.type_name());
          if (err_msg.size()) {
            (*err) += "\n" + err_msg;
          }
        }
      }
    }
  }

  return false;
}

// instanciation
#define INSTANCIATE_FLATTEN_WITH_INDICES(__ty) \
  template bool GeomPrimvar::flatten_with_indices(std::vector<__ty> *dest, std::string *err) const;

APPLY_GEOMPRIVAR_TYPE(INSTANCIATE_FLATTEN_WITH_INDICES)

#undef INSTANCIATE_FLATTEN_WITH_INDICES

bool GeomPrimvar::flatten_with_indices(value::Value *dest, std::string *err) const {
  // using namespace simple_match;
  // using namespace simple_match::placeholders;

  value::Value val;

  if (!dest) {
    if (err) {
      (*err) += "Output value is nullptr.";
    }
    return false;
  }

  if (_attr.is_timesamples()) {
    if (err) {
      (*err) += "TimeSamples attribute is TODO.";
    }
    return false;
  }

  bool processed = false;

  if (_attr.is_value()) {
    if (!IsSupportedGeomPrimvarType(_attr.type_id())) {
      if (err) {
        (*err) += fmt::format("Unsupported type for GeomPrimvar. type = `{}`",
                              _attr.type_name());
      }
      return false;
    }

    if (!(_attr.type_id() & value::TYPE_ID_1D_ARRAY_BIT)) {
      // Just return value as-is for scalar type
      (*dest) = _attr.get_var().value_raw();
    } else {
      std::string err_msg;

#if 0
#define APPLY_FUN(__ty)                                                      \
  value::TypeTraits<__ty>::type_id | value::TYPE_ID_1D_ARRAY_BIT,            \
      [this, &val, &processed, &err_msg]() {                                 \
        std::vector<__ty> expanded_val;                                      \
        if (auto pv = _attr.get_value<std::vector<__ty>>()) {                \
          auto ret = ExpandWithIndices(pv.value(), _indices, &expanded_val); \
          if (ret) {                                                         \
            processed = ret.value();                                         \
            if (processed) {                                                 \
              val = expanded_val;                                            \
            }                                                                \
          } else {                                                           \
            err_msg = ret.error();                                           \
          }                                                                  \
        }                                                                    \
      },

      match(_attr.type_id(), APPLY_GEOMPRIVAR_TYPE(APPLY_FUN) _,
            [&processed]() { processed = false; });
#else

#define APPLY_FUN(__ty)                                                  \
  case value::TypeTraits<__ty>::type_id() | value::TYPE_ID_1D_ARRAY_BIT: { \
    std::vector<__ty> expanded_val;                                      \
    if (auto pv = _attr.get_value<std::vector<__ty>>()) {                \
      auto ret = ExpandWithIndices(pv.value(), _indices, &expanded_val); \
      if (ret) {                                                         \
        processed = ret.value();                                         \
        if (processed) {                                                 \
          val = expanded_val;                                            \
        }                                                                \
      } else {                                                           \
        err_msg = ret.error();                                           \
      }                                                                  \
    }                                                                    \
    break;                                                               \
  }

#endif

      switch (_attr.type_id()) {
        APPLY_GEOMPRIVAR_TYPE(APPLY_FUN)
        default: {
          processed = false;
        }
      }

#undef APPLY_FUN

      if (processed) {
        (*dest) = std::move(val);
      } else {
        if (err) {
          (*err) += fmt::format(
              "[Internal Error] Failed to expand for GeomPrimvar type = `{}`",
              _attr.type_name());
          if (err_msg.size()) {
            (*err) += "\n" + err_msg;
          }
        }
      }
    }
  }

  return processed;
}

template <typename T>
bool GeomPrimvar::get_value(T *dest, std::string *err) const {
  static_assert(tinyusdz::value::TypeTraits<T>::type_id() != value::TypeTraits<value::token>::type_id(), "`token` type is not supported as a GeomPrimvar");
  static_assert(tinyusdz::value::TypeTraits<T>::type_id() != value::TypeTraits<std::vector<value::token>>::type_id(), "`token[]` type is not supported as a GeomPrimvar");

  if (!dest) {
    if (err) {
      (*err) += "Output value is nullptr.";
    }
    return false;
  }

  if (_attr.is_timesamples()) {
    if (err) {
      (*err) += "TimeSamples attribute is TODO.";
    }
    return false;
  }

  if (_attr.is_blocked()) {
    if (err) {
      (*err) += "Attribute is blocked.";
    }
    return false;
  }

  if (_attr.is_value()) {
    if (!IsSupportedGeomPrimvarType(_attr.type_id())) {
      if (err) {
        (*err) += fmt::format("Unsupported type for GeomPrimvar. type = `{}`",
                              _attr.type_name());
      }
      return false;
    }

    if (auto pv = _attr.get_value<T>()) {

      // copy
      (*dest) = pv.value();
      return true;

    } else {
      if (err) {
        (*err) += fmt::format("Attribute value type mismatch. Requested type `{}` but Attribute has type `{}`", value::TypeTraits<T>::type_id(), _attr.type_name());
      }
      return false;
    }
  }

  return false;
}

// instanciation
#define INSTANCIATE_GET_VALUE(__ty) \
  template bool GeomPrimvar::get_value(__ty *dest, std::string *err) const; \
  template bool GeomPrimvar::get_value(std::vector<__ty> *dest, std::string *err) const;

APPLY_GEOMPRIVAR_TYPE(INSTANCIATE_GET_VALUE)

#undef INSTANCIATE_GET_VALUE

std::vector<GeomPrimvar> GPrim::get_primvars() const {
  std::vector<GeomPrimvar> gpvars;

  for (const auto &prop : props) {
    if (startsWith(prop.first, kPrimvars)) {
      // skip `:indices`. Attribute with `:indices` suffix is handled in
      // `get_primvar`
      if (props.count(prop.first + kIndices)) {
        continue;
      }

      GeomPrimvar gprimvar;
      if (get_primvar(removePrefix(prop.first, kPrimvars), &gprimvar)) {
        gpvars.emplace_back(std::move(gprimvar));
      }
    }
  }

  return gpvars;
}

bool GPrim::set_primvar(const GeomPrimvar &primvar,
                        std::string *err) {
  if (primvar.name().empty()) {
    if (err) {
      (*err) += "GeomPrimvar.name is empty.";
    }
    return false;
  }

  if (startsWith(primvar.name(), "primvars:")) {
    if (err) {
      (*err) += "GeomPrimvar.name must not start with `primvars:` namespace. name = " + primvar.name();
    }
    return false;
  }

  std::string primvar_name = kPrimvars + primvar.name();

  // Overwrite existing primvar prop.
  // TODO: Report warn when primvar name already exists.

  Attribute attr = primvar.get_attribute();

  if (primvar.has_interpolation()) {
    attr.metas().interpolation = primvar.get_interpolation();
  }

  if (primvar.has_elementSize()) {
    attr.metas().elementSize = primvar.get_elementSize();
  }

  props[primvar_name] = attr;

  if (primvar.has_indices()) {

    std::string index_name = primvar_name + kIndices;

    Attribute indices;
    indices.set_value(primvar.get_indices());

    props[index_name] = indices;
  }

  return true;
}

bool GPrim::get_displayColor(value::color3f *dst, double t, const value::TimeSampleInterpolationType tinterp)
{
  // TODO: timeSamples
  (void)t;
  (void)tinterp;

  GeomPrimvar primvar;
  std::string err;
  if (!get_primvar("displayColor", &primvar, &err)) {
    // TODO: report err
    return false;
  }

  return primvar.get_value(dst);
}

bool GPrim::get_displayOpacity(float *dst, double t, const value::TimeSampleInterpolationType tinterp)
{
  // TODO: timeSamples
  (void)t;
  (void)tinterp;

  GeomPrimvar primvar;
  std::string err;
  if (!get_primvar("displayOpacity", &primvar, &err)) {
    // TODO: report err
    return false;
  }

  return primvar.get_value(dst);
}

const std::vector<value::point3f> GeomMesh::get_points(
    double time, value::TimeSampleInterpolationType interp) const {
  std::vector<value::point3f> dst;

  if (!points.authored() || points.is_blocked()) {
    return dst;
  }

  if (points.is_connection()) {
    // TODO: connection
    return dst;
  }

  if (auto pv = points.get_value()) {
    std::vector<value::point3f> val;
    if (pv.value().get(time, &val, interp)) {
      dst = std::move(val);
    }
  }

  return dst;
}

const std::vector<value::normal3f> GeomMesh::get_normals(
    double time, value::TimeSampleInterpolationType interp) const {
  std::vector<value::normal3f> dst;

  if (props.count(kPrimvarsNormals)) {
    const auto prop = props.at(kPrimvarsNormals);
    if (prop.is_relationship()) {
      // TODO:
      return dst;
    }

    if (prop.get_attribute().get_var().is_timesamples()) {
      // TODO:
      return dst;
    }

    if (prop.get_attribute().type_name() == "normal3f[]") {
      if (auto pv =
              prop.get_attribute().get_value<std::vector<value::normal3f>>()) {
        dst = pv.value();
      }
    }
  } else if (normals.authored()) {
    if (normals.is_connection()) {
      // TODO
      return dst;
    } else if (normals.is_blocked()) {
      return dst;
    }

    if (normals.get_value()) {
      std::vector<value::normal3f> val;
      if (normals.get_value().value().get(time, &val, interp)) {
        dst = std::move(val);
      }
    }
  }

  return dst;
}

Interpolation GeomMesh::get_normalsInterpolation() const {
  if (props.count(kPrimvarsNormals)) {
    const auto &prop = props.at(kPrimvarsNormals);
    if (prop.get_attribute().type_name() == "normal3f[]") {
      if (prop.get_attribute().metas().interpolation) {
        return prop.get_attribute().metas().interpolation.value();
      }
    }
  } else if (normals.metas().interpolation) {
    return normals.metas().interpolation.value();
  }

  return Interpolation::Vertex;  // default 'vertex'
}

const std::vector<int32_t> GeomMesh::get_faceVertexCounts() const {
  std::vector<int32_t> dst;

  if (!faceVertexCounts.authored() || faceVertexCounts.is_blocked()) {
    return dst;
  }

  if (faceVertexCounts.is_connection()) {
    // TODO: connection
    return dst;
  }

  if (auto pv = faceVertexCounts.get_value()) {
    std::vector<int32_t> val;
    // TOOD: timesamples
    if (pv.value().get_scalar(&val)) {
      dst = std::move(val);
    }
  }
  return dst;
}

const std::vector<int32_t> GeomMesh::get_faceVertexIndices() const {
  std::vector<int32_t> dst;

  if (!faceVertexIndices.authored() || faceVertexIndices.is_blocked()) {
    return dst;
  }

  if (faceVertexIndices.is_connection()) {
    // TODO: connection
    return dst;
  }

  if (auto pv = faceVertexIndices.get_value()) {
    std::vector<int32_t> val;
    // TOOD: timesamples
    if (pv.value().get_scalar(&val)) {
      dst = std::move(val);
    }
  }
  return dst;
}

// static
bool GeomSubset::ValidateSubsets(
    const std::vector<const GeomSubset *> &subsets,
    const size_t elementCount,
    const FamilyType &familyType, std::string *err) {

  if (subsets.empty()) {
    return true;
  }

  // All subsets must have the same elementType.
  GeomSubset::ElementType elementType = subsets[0]->elementType.get_value();
  for (const auto psubset : subsets) {
    if (psubset->elementType.get_value() != elementType) {
      if (err) {
        (*err) = fmt::format("GeomSubset {}'s elementType must be `{}`, but got `{}`.\n",
          psubset->name, to_string(elementType), to_string(psubset->elementType.get_value()));
      }

      return false;
    }
  }

  std::set<int32_t> indicesInFamily;

  bool valid = true;
  std::stringstream ss;

  // TODO: TimeSampled indices
  for (const auto psubset : subsets) {
    Animatable<std::vector<int32_t>> indices;
    if (!psubset->indices.get_value(&indices)) {
      ss << fmt::format("GeomSubset {}'s indices is not value Attribute. Connection or ValueBlock?\n",
          psubset->name);

      valid = false;
    }

    if (indices.is_blocked()) {
      ss << fmt::format("GeomSubset {}'s indices is Value Blocked.\n", psubset->name);
      valid = false;
    }

    if (indices.is_timesamples()) {
      ss << fmt::format("ValidateSubsets: TimeSampled GeomSubset.indices is not yet supported.\n");
      valid = false;
    }

    std::vector<int32_t> subsetIndices;
    if (!indices.get_scalar(&subsetIndices)) {
      ss << fmt::format("ValidateSubsets: Internal error. Failed to get GeomSubset.indices.\n");
      valid = false;
    }

    for (const int32_t index : subsetIndices) {
      if (!indicesInFamily.insert(index).second && (familyType != FamilyType::Unrestricted)) {
        ss << fmt::format("Found overlapping index {} in GeomSubset `{}`\n", index, psubset->name);
        valid = false;
      }
    }

    // Make sure every index appears exactly once if it's a partition.
    if ((familyType == FamilyType::Partition) && (indicesInFamily.size() != elementCount)) {
      ss << fmt::format("ValidateSubsets: The number of unique indices {} must be equal to input elementCount {}\n", indicesInFamily.size(), elementCount);
      valid = false;
    }

    // Ensure that the indices are in the range [0, faceCount)
    size_t maxIndex = static_cast<size_t>(*indicesInFamily.rbegin());
    int minIndex = *indicesInFamily.begin();

    if (maxIndex >= elementCount) {
      ss << fmt::format("ValidateSubsets: All indices must be in range [0, elementSize {}), but one or more indices are greater than elementSize. Maximum = {}\n", elementCount, maxIndex);

      valid = false;
    }

    if (minIndex < 0) {
      ss << fmt::format("ValidateSubsets: Found one or more indices that are less than 0. Minumum = {}\n", minIndex);

      valid = false;
    }

  }

  if (!valid) {
    if (err) {
      (*err) += ss.str();
    }
  }

  return true;

}

}  // namespace tinyusdz
