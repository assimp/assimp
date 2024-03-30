// SPDX-License-Identifier: Apache 2.0
// Copyright 2022 - Present, Light Transport Entertainment, Inc.
//
// Shader network evaluation

#include <unordered_map>

#include "nonstd/expected.hpp"
#include "value-types.hh"

namespace tinyusdz {

// forward decl of prim-types.hh
class Path;
class Stage;
class Prim;

// forward decl of usdShade
struct Material;
struct Shader;
enum class MaterialBindingStrength;

template<typename T>
struct UsdPrimvarReader;

using UsdPrimvarReader_float = UsdPrimvarReader<float>;
using UsdPrimvarReader_float2 = UsdPrimvarReader<value::float2>;
using UsdPrimvarReader_float3 = UsdPrimvarReader<value::float3>;
using UsdPrimvarReader_float4 = UsdPrimvarReader<value::float4>;

namespace tydra {

// GLSL like data types
using vec2 = value::float2;
using vec3 = value::float3;
using vec4 = value::float4;
using mat2 = value::matrix2f;  // float precision


///
/// Evaluate and return COPIED terminal value of the shader attribute
///
/// If specified attribute has a value(including timeSamples value), returns the value of it.
/// If specified attribute is a connection, it follows the value producing Attribute and returns the value of it.
///
/// Since the type of shader connection is known in advance, we return the value by template type T, not by `value::Value`
/// NOTE: The returned value is COPIED. This should be OK for Shader network since usually it does not hold large data...
///
/// @param[in] stage Stage
/// @param[in] shader Shader node in `stage`.
/// @param[in] attr_name Attribute name
/// @param[out] out_val Output evaluated value.
/// @param[out] err Error message
/// @param[in] timeCode (Optional) Evaluate the value at specified time(for timeSampled value)
///
/// @return true when specified `prop_name` exists in the given `shader` and can resolve the connection and retrieve "terminal" value.
///
template<typename T>
bool EvaluateShaderAttribute(
  const Stage &stage,
  const Shader &shader, const std::string &attr_name,
  T * out_val,
  std::string *err,
  const value::TimeCode timeCode = value::TimeCode::Default());

extern template bool EvaluateShaderAttribute(const Stage &stage, const Shader &shader, const std::string &attr_name, value::token * out_val, std::string *err, const value::TimeCode timeCode);
 
// Currently float2 only
//std::vector<UsdPrimvarReader_float2> ExtractPrimvarReadersFromMaterialNode(const Prim &node);


///
/// Get material:binding target Path of given Prim.
/// Do not seek `material:binding` of parent Path.
///
/// @param[in] stage Prim
/// @param[in] prim Prim
/// @param[in] suffix extra suffix(e.g. empty string is given, look into `material:binding`. `collection` is given, look into `material:binding:collection`.)
/// @param[out] materialPath Found target Path.
/// @param[out] material THe pointer to found Material object in Stage(if no Material object found in Stage, returns nullptr)
/// @return true when bound Material Path is found.
///
bool GetBoundMaterial(
  const Stage &stage,
  const Prim &prim,
  const std::string &suffix,
  tinyusdz::Path *materialPath, 
  const Material **material,
  std::string *err);

///
/// Find material:binding target Path from given Prim absolute path.
/// The bahavior is:
///
/// 1. If material:binding is assigned to a Prim of given absolute path(e.g. "/xform/mesh0"), return it
/// 2. Look into parent Prim's material:binding
///
/// @param[in] stage Prim
/// @param[in] prim Prim
/// @param[in] suffix extra suffix(e.g. empty string is given, look into `material:binding`. `collection` is given, look into `material:binding:collection`.)
/// @param[out] materialPath Found target Path.
/// @param[out] material THe pointer to found Material object in Stage(if no Material object found in Stage, returns nullptr)
/// @return true when bound Material Path is found.
///
bool FindBoundMaterial(
  const Stage &stage,
  const Path &abs_path,
  const std::string &suffix,
  tinyusdz::Path *materialPath, 
  const Material **material,
  std::string *err);

}  // namespace tydra
}  // namespace tinyusdz
