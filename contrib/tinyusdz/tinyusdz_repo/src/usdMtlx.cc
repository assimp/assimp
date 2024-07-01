// SPDX-License-Identifier: Apache 2.0
// Copyright 2023 - Present, Light Transport Entertainment, Inc.

#if defined(TINYUSDZ_USE_USDMTLX)

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "external/pugixml.hpp"
// #include "external/jsonhpp/nlohmann/json.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif  // TINYUSDZ_USE_USDMTLX

#include <sstream>

#include "usdMtlx.hh"

#if defined(TINYUSDZ_USE_USDMTLX)

#include "ascii-parser.hh"  // To parse color3f value
#include "common-macros.inc"
#include "external/dtoa_milo.h"
#include "io-util.hh"
#include "pprinter.hh"
#include "tiny-format.hh"
#include "value-pprint.hh"

inline std::string dtos(const double v) {
  char buf[128];
  dtoa_milo(v, buf);

  return std::string(buf);
}

#define PushWarn(msg) \
  do {                \
    if (warn) {       \
      (*warn) += msg; \
    }                 \
  } while (0);

#define PushError(msg) \
  do {                 \
    if (err) {         \
      (*err) += msg;   \
    }                  \
  } while (0);

namespace tinyusdz {

// defined in ascii-parser-base-types.cc
namespace ascii {

extern template bool AsciiParser::SepBy1BasicType<float>(
    const char sep, std::vector<float> *ret);

}  // namespace ascii

namespace detail {

template <typename T>
Property MakeProperty(const T &value) {
  Attribute attr(value);
  Property prop(attr, /* custom */ false);

  return prop;
}

bool is_supported_type(const std::string &typeName);

bool is_supported_type(const std::string &typeName) {
  if (typeName.compare("integer") == 0) return true;
  if (typeName.compare("boolean") == 0) return true;
  if (typeName.compare("float") == 0) return true;
  if (typeName.compare("color3") == 0) return true;
  if (typeName.compare("color4") == 0) return true;
  if (typeName.compare("vector2") == 0) return true;
  if (typeName.compare("vector3") == 0) return true;
  if (typeName.compare("vector4") == 0) return true;
  if (typeName.compare("matrix33") == 0) return true;
  if (typeName.compare("matrix44") == 0) return true;
  if (typeName.compare("string") == 0) return true;
  if (typeName.compare("filename") == 0) return true;

  if (typeName.compare("integerarray") == 0) return true;
  if (typeName.compare("floatarray") == 0) return true;
  if (typeName.compare("vector2array") == 0) return true;
  if (typeName.compare("vector3array") == 0) return true;
  if (typeName.compare("vector4array") == 0) return true;
  if (typeName.compare("color3array") == 0) return true;
  if (typeName.compare("color4array") == 0) return true;
  if (typeName.compare("stringarray") == 0) return true;

  // No matrixarray

  // TODO
  // if (typeName.compare("color") == 0) return true;
  // if (typeName.compare("geomname") == 0) return true;
  // if (typeName.compare("geomnamearray") == 0) return true;

  return false;
}

template <typename T>
bool ParseValue(tinyusdz::ascii::AsciiParser &parser, T &ret, std::string *err);

template <>
bool ParseValue<int>(tinyusdz::ascii::AsciiParser &parser, int &ret,
                     std::string *err) {
  int val;
  if (!parser.ReadBasicType(&val)) {
    PUSH_ERROR_AND_RETURN(fmt::format("Failed to parse a value of type `{}`",
                                      value::TypeTraits<int>::type_name()));
  }

  ret = val;

  return true;
}

template <>
bool ParseValue<bool>(tinyusdz::ascii::AsciiParser &parser, bool &ret,
                      std::string *err) {
  bool val;
  if (!parser.ReadBasicType(&val)) {
    PUSH_ERROR_AND_RETURN(fmt::format("Failed to parse a value of type `{}`",
                                      value::TypeTraits<bool>::type_name()));
  }

  ret = val;

  return true;
}

template <>
bool ParseValue<float>(tinyusdz::ascii::AsciiParser &parser, float &ret,
                       std::string *err) {
  float val;
  if (!parser.ReadBasicType(&val)) {
    PUSH_ERROR_AND_RETURN(fmt::format("Failed to parse a value of type `{}`",
                                      value::TypeTraits<float>::type_name()));
  }

  ret = val;

  return true;
}

template <>
bool ParseValue<std::string>(tinyusdz::ascii::AsciiParser &parser,
                             std::string &ret, std::string *err) {
  std::string val;
  if (!parser.ReadBasicType(&val)) {
    PUSH_ERROR_AND_RETURN(
        fmt::format("Failed to parse a value of type `{}`",
                    value::TypeTraits<std::string>::type_name()));
  }

  ret = val;

  return true;
}

template <>
bool ParseValue<value::float2>(tinyusdz::ascii::AsciiParser &parser,
                               value::float2 &ret, std::string *err) {
  std::vector<float> values;
  if (!parser.SepBy1BasicType(',', &values)) {
    PUSH_ERROR_AND_RETURN(
        fmt::format("Failed to parse a value of type `{}`",
                    value::TypeTraits<value::float3>::type_name()));
  }

  if (values.size() != 2) {
    PUSH_ERROR_AND_RETURN(fmt::format(
        "type `{}` expects the number of elements is 2, but got {}",
        value::TypeTraits<value::float2>::type_name(), values.size()));
  }

  ret[0] = values[0];
  ret[1] = values[1];

  return true;
}

template <>
bool ParseValue<value::float3>(tinyusdz::ascii::AsciiParser &parser,
                               value::float3 &ret, std::string *err) {
  std::vector<float> values;
  if (!parser.SepBy1BasicType(',', &values)) {
    PUSH_ERROR_AND_RETURN(
        fmt::format("Failed to parse a value of type `{}`",
                    value::TypeTraits<value::float3>::type_name()));
  }

  if (values.size() != 3) {
    PUSH_ERROR_AND_RETURN(fmt::format(
        "type `{}` expects the number of elements is 3, but got {}",
        value::TypeTraits<value::float3>::type_name(), values.size()));
  }

  ret[0] = values[0];
  ret[1] = values[1];
  ret[2] = values[2];

  return true;
}

template <>
bool ParseValue<value::vector3f>(tinyusdz::ascii::AsciiParser &parser,
                                 value::vector3f &ret, std::string *err) {
  std::vector<float> values;
  if (!parser.SepBy1BasicType(',', &values)) {
    PUSH_ERROR_AND_RETURN(
        fmt::format("Failed to parse a value of type `{}`",
                    value::TypeTraits<value::vector3f>::type_name()));
  }

  if (values.size() != 3) {
    PUSH_ERROR_AND_RETURN(fmt::format(
        "type `{}` expects the number of elements is 3, but got {}",
        value::TypeTraits<value::vector3f>::type_name(), values.size()));
  }

  ret[0] = values[0];
  ret[1] = values[1];
  ret[2] = values[2];

  return true;
}

template <>
bool ParseValue<value::normal3f>(tinyusdz::ascii::AsciiParser &parser,
                                 value::normal3f &ret, std::string *err) {
  std::vector<float> values;
  if (!parser.SepBy1BasicType(',', &values)) {
    PUSH_ERROR_AND_RETURN(
        fmt::format("Failed to parse a value of type `{}`",
                    value::TypeTraits<value::normal3f>::type_name()));
  }

  if (values.size() != 3) {
    PUSH_ERROR_AND_RETURN(fmt::format(
        "type `{}` expects the number of elements is 3, but got {}",
        value::TypeTraits<value::normal3f>::type_name(), values.size()));
  }

  ret[0] = values[0];
  ret[1] = values[1];
  ret[2] = values[2];

  return true;
}


template <>
bool ParseValue<value::color3f>(tinyusdz::ascii::AsciiParser &parser,
                                 value::color3f &ret, std::string *err) {
  std::vector<float> values;
  if (!parser.SepBy1BasicType(',', &values)) {
    PUSH_ERROR_AND_RETURN(
        fmt::format("Failed to parse a value of type `{}`",
                    value::TypeTraits<value::color3f>::type_name()));
  }

  if (values.size() != 3) {
    PUSH_ERROR_AND_RETURN(fmt::format(
        "type `{}` expects the number of elements is 3, but got {}",
        value::TypeTraits<value::color3f>::type_name(), values.size()));
  }

  ret[0] = values[0];
  ret[1] = values[1];
  ret[2] = values[2];

  return true;
}

template <>
bool ParseValue<value::float4>(tinyusdz::ascii::AsciiParser &parser,
                               value::float4 &ret, std::string *err) {
  std::vector<float> values;
  if (!parser.SepBy1BasicType(',', &values)) {
    PUSH_ERROR_AND_RETURN(
        fmt::format("Failed to parse a value of type `{}`",
                    value::TypeTraits<value::float4>::type_name()));
  }

  if (values.size() != 4) {
    PUSH_ERROR_AND_RETURN(fmt::format(
        "type `{}` expects the number of elements is 4, but got {}",
        value::TypeTraits<value::float4>::type_name(), values.size()));
  }

  ret[0] = values[0];
  ret[1] = values[1];
  ret[2] = values[2];
  ret[3] = values[3];

  return true;
}

///
/// For MaterialX XML.
/// Parse string representation of Attribute value.
/// e.g. "0.0, 1.1" for vector2 type
/// NOTE: no parenthesis('(', '[') for vector and array type.
///
/// @param[in] typeName typeName(e.g. "vector2")
/// @param[in] str Ascii representation of value.
/// @param[out] value Ascii representation of value.
/// @param[out] err Parse error message when returning false.
///
///
/// Supported data type: boolean, float, color3, color4, vector2, vector3,
/// vector4, matrix33, matrix44, string, filename, integerarray, floatarray,
/// color3array, color4array, vector2array, vector3array, vector4array,
/// stringarray. Unsupported data type: geomname, geomnamearray
///
bool ParseMaterialXValue(const std::string &typeName, const std::string &str,
                         value::Value *value, std::string *err);

bool ParseMaterialXValue(const std::string &typeName, const std::string &str,
                         value::Value *value, std::string *err) {
  (void)value;

  if (!is_supported_type(typeName)) {
    PUSH_ERROR_AND_RETURN(
        fmt::format("Invalid/unsupported type: {}", typeName));
  }

  tinyusdz::StreamReader sr(reinterpret_cast<const uint8_t *>(str.data()),
                            str.size(), /* swap endian */ false);
  tinyusdz::ascii::AsciiParser parser(&sr);

  if (typeName.compare("integer") == 0) {
    int val;
    if (!ParseValue(parser, val, err)) {
      return false;
    }
  } else if (typeName.compare("boolean") == 0) {
    bool val;
    if (!ParseValue(parser, val, err)) {
      return false;
    }
  } else if (typeName.compare("vector2") == 0) {
    value::float2 val;
    if (!ParseValue(parser, val, err)) {
      return false;
    }
  } else if (typeName.compare("vector3") == 0) {
    value::float3 val;
    if (!ParseValue(parser, val, err)) {
      return false;
    }
  } else if (typeName.compare("vector4") == 0) {
    value::float4 val;
    if (!ParseValue(parser, val, err)) {
      return false;
    }
  } else {
    PUSH_ERROR_AND_RETURN("TODO: " + typeName);
  }

  // TODO
  return false;
}

template <typename T>
bool ParseMaterialXValue(const std::string &str, T *value, std::string *err) {
  tinyusdz::StreamReader sr(reinterpret_cast<const uint8_t *>(str.data()),
                            str.size(), /* swap endian */ false);
  tinyusdz::ascii::AsciiParser parser(&sr);

  T val;

  if (!ParseValue(parser, val, err)) {
    return false;
  }

  (*value) = val;
  return true;
}

template <typename T>
std::string to_xml_string(const T &val);

template <>
std::string to_xml_string(const float &val) {
  return dtos(double(val));
}

template <>
std::string to_xml_string(const int &val) {
  return std::to_string(val);
}

template <>
std::string to_xml_string(const value::color3f &val) {
  return dtos(double(val.r)) + ", " + dtos(double(val.g)) + ", " +
         dtos(double(val.b));
}

template <>
std::string to_xml_string(const value::normal3f &val) {
  return dtos(double(val.x)) + ", " + dtos(double(val.y)) + ", " +
         dtos(double(val.z));
}

template <typename T>
bool SerializeAttribute(const std::string &attr_name,
                        const TypedAttributeWithFallback<Animatable<T>> &attr,
                        std::string &value_str, std::string *err) {
  std::stringstream value_ss;

  if (attr.is_connection()) {
    PUSH_ERROR_AND_RETURN(fmt::format("TODO: connection attribute"));
  } else if (attr.is_blocked()) {
    // do nothing
    value_str = "";
    return true;
  } else {
    const Animatable<T> &animatable_value = attr.get_value();
    if (animatable_value.is_scalar()) {
      T value;
      if (animatable_value.get_scalar(&value)) {
        value_ss << "\"" << to_xml_string(value) << "\"";
      } else {
        PUSH_ERROR_AND_RETURN(fmt::format(
            "Failed to get the value at default time of `{}`", attr_name));
      }
    } else if (animatable_value.is_timesamples()) {
      // no time-varying attribute in MaterialX. Use the value at default
      // timecode.
      T value;
      if (animatable_value.get(value::TimeCode::Default(), &value)) {
        value_ss << "\"" << to_xml_string(value) << "\"";
      } else {
        PUSH_ERROR_AND_RETURN(fmt::format(
            "Failed to get the value at default time of `{}`", attr_name));
      }
    } else {
      PUSH_ERROR_AND_RETURN(
          fmt::format("Failed to get the value of `{}`", attr_name));
    }
  }

  value_str = value_ss.str();
  return true;
}

static bool WriteMaterialXToString(const MtlxUsdPreviewSurface &shader,
                                   std::string &xml_str, std::string *warn,
                                   std::string *err) {
  (void)warn;

  // We directly write xml string for simplicity.
  //
  // TODO:
  // - [ ] Use pugixml to write xml string.

  std::stringstream ss;

  std::string node_name = "SR_default";

  ss << "<?xml version=\"1.0\"?>\n";
  // TODO: colorspace
  ss << "<materialx version=\"1.38\" colorspace=\"lin_rec709\">\n";
  ss << pprint::Indent(1) << "<UsdPreviewSurface name=\"" << node_name
     << "\" type =\"surfaceshader\">\n";

#define EMIT_ATTRIBUTE(__name, __tyname, __attr)                            \
  {                                                                         \
    std::string value_str;                                                  \
    if (!SerializeAttribute(__name, __attr, value_str, err)) {              \
      return false;                                                         \
    }                                                                       \
    if (value_str.size()) {                                                 \
      ss << pprint::Indent(2) << "<input name=\"" << __name << "\" type=\"" \
         << __tyname << "\" value=\"" << value_str << "\" />\n";            \
    }                                                                       \
  }

  // TODO: Attribute Connection
  EMIT_ATTRIBUTE("diffuseColor", "color3", shader.diffuseColor)
  EMIT_ATTRIBUTE("emissiveColor", "color3", shader.emissiveColor)
  EMIT_ATTRIBUTE("useSpecularWorkflow", "integer", shader.useSpecularWorkflow)
  EMIT_ATTRIBUTE("specularColor", "color3", shader.specularColor)
  EMIT_ATTRIBUTE("metallic", "float", shader.metallic)
  EMIT_ATTRIBUTE("roughness", "float", shader.roughness)
  EMIT_ATTRIBUTE("clearcoat", "float", shader.clearcoat)
  EMIT_ATTRIBUTE("clearcoatRoughness", "float", shader.clearcoatRoughness)
  EMIT_ATTRIBUTE("opacity", "float", shader.opacity)
  EMIT_ATTRIBUTE("opacityThreshold", "float", shader.opacityThreshold)
  EMIT_ATTRIBUTE("ior", "float", shader.ior)
  EMIT_ATTRIBUTE("normal", "vector3", shader.normal)
  EMIT_ATTRIBUTE("displacement", "float", shader.displacement)
  EMIT_ATTRIBUTE("occlusion", "float", shader.occlusion)

  ss << pprint::Indent(1) << "</UsdPreviewSurface>\n";

  ss << pprint::Indent(1)
     << "<surfacematerial name=\"USD_Default\" type=\"material\">\n";
  ss << pprint::Indent(2)
     << "<input name=\"surfaceshader\" type=\"surfaceshader\" nodename=\""
     << node_name << "\" />\n";
  ss << pprint::Indent(1) << "</surfacematerial>\n";

  ss << "</materialx>\n";

  xml_str = ss.str();

  return true;
}

static bool ConvertPlace2d(const pugi::xml_node &node, PrimSpec &ps,
                           std::string *warn, std::string *err) {
  // texcoord(vector2). default index=0 uv coordinate
  // pivot(vector2). default (0, 0)
  // scale(vector2). default (1, 1)
  // rotate(float). in degrees, Conter-clockwise
  // offset(vector2)
  if (pugi::xml_attribute texcoord_attr = node.attribute("texcoord")) {
    PUSH_WARN("TODO: `texcoord` attribute.\n");
  }

  if (pugi::xml_attribute pivot_attr = node.attribute("pivot")) {
    value::float2 value;
    if (!ParseMaterialXValue(pivot_attr.as_string(), &value, err)) {
      ps.props()["inputs:pivot"] = Property(Attribute::Uniform(value));
    }
  }

  if (pugi::xml_attribute scale_attr = node.attribute("scale")) {
    value::float2 value;
    if (!ParseMaterialXValue(scale_attr.as_string(), &value, err)) {
      PUSH_ERROR_AND_RETURN(
          "Failed to parse `rotate` attribute of `place2d`.\n");
    }
    ps.props()["inputs:scale"] = Property(Attribute::Uniform(value));
  }

  if (pugi::xml_attribute rotate_attr = node.attribute("rotate")) {
    float value;
    if (!ParseMaterialXValue(rotate_attr.as_string(), &value, err)) {
      PUSH_ERROR_AND_RETURN(
          "Failed to parse `rotate` attribute of `place2d`.\n");
    }
    ps.props()["inputs:rotate"] = Property(Attribute::Uniform(value));
  }

  pugi::xml_attribute offset_attr = node.attribute("offset");
  if (offset_attr) {
    value::float2 value;
    if (!ParseMaterialXValue(offset_attr.as_string(), &value, err)) {
      PUSH_ERROR_AND_RETURN(
          "Failed to parse `offset` attribute of `place2d`.\n");
    }
    ps.props()["inputs:offset"] = Property(Attribute::Uniform(value));
  }

  ps.specifier() = Specifier::Def;
  ps.typeName() = kShader;
  ps.props()[kShaderInfoId] =
      Property(Attribute::Uniform(value::token(kUsdTransform2d)));

  return true;
}

static bool ConvertNodeGraphRec(const uint32_t depth,
                                const pugi::xml_node &node, PrimSpec &ps_out,
                                std::string *warn, std::string *err) {
  if (depth > (1024 * 1024)) {
    PUSH_ERROR_AND_RETURN("Network too deep.\n");
  }

  PrimSpec ps;

  std::string node_name = node.name();

  if (node_name == "place2d") {
    if (!ConvertPlace2d(node, ps, warn, err)) {
      return false;
    }
  } else {
    PUSH_ERROR_AND_RETURN("Unknown/unsupported Shader Node: " << node.name());
  }

  for (const auto &child : node.children()) {
    PrimSpec child_ps;
    if (!ConvertNodeGraphRec(depth + 1, child, child_ps, warn, err)) {
      return false;
    }

    ps.children().emplace_back(std::move(child_ps));
  }

  ps_out = std::move(ps);

  return true;
}

#if 0  // TODO
static bool ConvertPlace2d(const pugi::xml_node &node, UsdTransform2d &tx, std::string *warn, std::string *err) {
  // texcoord(vector2). default index=0 uv coordinate
  // pivot(vector2). default (0, 0)
  // scale(vector2). default (1, 1)
  // rotate(float). in degrees, Conter-clockwise
  // offset(vector2)
  if (pugi::xml_attribute texcoord_attr = node.attribute("texcoord")) {
    PUSH_WARN("TODO: `texcoord` attribute.\n");
  }

  if (pugi::xml_attribute pivot_attr = node.attribute("pivot")) {
    PUSH_WARN("TODO: `pivot` attribute.\n");
  }

  if (pugi::xml_attribute scale_attr = node.attribute("scale")) {
    value::float2 value;
    if (!ParseMaterialXValue(scale_attr.as_string(), &value, err)) {
      PUSH_ERROR_AND_RETURN("Failed to parse `rotate` attribute of `place2d`.\n");
    }
    tx.scale = value;
  }

  if (pugi::xml_attribute rotate_attr = node.attribute("rotate")) {
    float value;
    if (!ParseMaterialXValue(rotate_attr.as_string(), &value, err)) {
      PUSH_ERROR_AND_RETURN("Failed to parse `rotate` attribute of `place2d`.\n");
    }
    tx.rotation = value;
  }

  pugi::xml_attribute offset_attr = node.attribute("offset");
  if (offset_attr) {
    PUSH_WARN("TODO: `offset` attribute.\n");
  }

  return true;
}

static bool ConvertTiledImage(const pugi::xml_node &node, UsdUVTexture &tex, std::string *err) {
  (void)tex;
  // file: uniform filename
  // default: float or colorN or vectorN
  // texcoord: vector2
  // uvtiling: vector2(default 1.0, 1.0)
  // uvoffset: vector2(default 0.0, 0.0)
  // realworldimagesize: vector2
  // realworldtilesize: vector2
  // filtertype: string: "closest", "linear" or "cubic"
  if (pugi::xml_attribute file_attr = node.attribute("file")) {
    std::string filename;
    if (!ParseMaterialXValue(file_attr.as_string(), &filename, err)) {
      PUSH_ERROR_AND_RETURN("Failed to parse `file` attribute in `tiledimage`.\n");
    }
  } else {
    PUSH_ERROR_AND_RETURN("`file` attribute not found.");
  }

  // TODO...

  return true;

}
#endif

}  // namespace detail

bool ReadMaterialXFromString(const std::string &str,
                             const std::string &asset_path, MtlxModel *mtlx,
                             std::string *warn, std::string *err) {
#define GET_ATTR_VALUE(__xml, __name, __ty, __var)                        \
  do {                                                                    \
    pugi::xml_attribute attr = __xml.attribute(__name);                   \
    if (!attr) {                                                          \
      PUSH_ERROR_AND_RETURN(                                              \
          fmt::format("Required XML Attribute `{}` not found.", __name)); \
    }                                                                     \
    __ty v;                                                               \
    if (!detail::ParseMaterialXValue(attr.as_string(), &v, err)) {        \
      return false;                                                       \
    }                                                                     \
    __var = v;                                                            \
  } while (0)

#define GET_SHADER_PARAM(__name, __typeName, __inp_name, __tyname, __ty, \
                         __valuestr, __attr)                             \
  if (__name == __inp_name) {                                            \
    if (__typeName != __tyname) {                                        \
      PUSH_ERROR_AND_RETURN(                                             \
          fmt::format("type `{}` expected for input `{}`, but got `{}`", \
                      __typeName, __inp_name, __tyname));                \
    }                                                                    \
    __ty v;                                                              \
    if (!detail::ParseMaterialXValue(__valuestr, &v, err)) {             \
      return false;                                                      \
    }                                                                    \
    __attr.set_value(v);                                                 \
  } else

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_string(str.c_str());
  if (!result) {
    std::string msg(result.description());
    PUSH_ERROR_AND_RETURN("Failed to parse XML: " + msg);
  }

  pugi::xml_node root = doc.child("materialx");
  if (!root) {
    PUSH_ERROR_AND_RETURN("<materialx> tag not found: " + asset_path);
  }

  // Attributes for a <materialx> element:
  //
  // - [x] version(string, required)
  //   - [x] validate version string
  // - [x] cms(string, optional)
  // - [x] cmsconfig(filename, optional)
  // - [x] colorspace(string, optional)
  // - [x] namespace(string, optional)

  pugi::xml_attribute ver_attr = root.attribute("version");
  if (!ver_attr) {
    PUSH_ERROR_AND_RETURN("version attribute not found in <materialx>:" +
                          asset_path);
  }

  // parse version string as floating point
  {
    DCOUT("version = " << ver_attr.as_string());
    float ver{0.0};
    if (!detail::ParseMaterialXValue(ver_attr.as_string(), &ver, err)) {
      return false;
    }

    if (ver < 1.38f) {
      PUSH_ERROR_AND_RETURN(
          fmt::format("TinyUSDZ only supports MaterialX version 1.38 or "
                      "greater, but got {}",
                      ver_attr.as_string()));
    }
    mtlx->version = ver_attr.as_string();
  }

  pugi::xml_attribute cms_attr = root.attribute("cms");
  if (cms_attr) {
    mtlx->cms = cms_attr.as_string();
  }

  pugi::xml_attribute cmsconfig_attr = root.attribute("cms");
  if (cmsconfig_attr) {
    mtlx->cmsconfig = cmsconfig_attr.as_string();
  }
  pugi::xml_attribute colorspace_attr = root.attribute("colorspace");
  if (colorspace_attr) {
    mtlx->color_space = colorspace_attr.as_string();
  }

  pugi::xml_attribute namespace_attr = root.attribute("namespace");
  if (namespace_attr) {
    mtlx->name_space = namespace_attr.as_string();
  }

  std::vector<PrimSpec> nodegraph_pss;

  // NodeGraph
  for (auto ng : root.children("nodegraph")) {
    PrimSpec root_ps;
    if (detail::ConvertNodeGraphRec(0, ng, root_ps, warn, err)) {
      return false;
    }

    nodegraph_pss.emplace_back(std::move(root_ps));
  }

  // standard_surface
  for (auto sd_surface : root.children("standard_surface")) {
    PUSH_WARN("TODO: `look`");
    // TODO
    (void)sd_surface;
  }

  // standard_surface
  for (auto usd_surface : root.children("UsdPreviewSurface")) {
    std::string surface_name;
    {
      std::string typeName;

      GET_ATTR_VALUE(usd_surface, "name", std::string, surface_name);
      GET_ATTR_VALUE(usd_surface, "type", std::string, typeName);

      if (typeName != "surfaceshader") {
        PUSH_ERROR_AND_RETURN(
            fmt::format("`surfaceshader` expected for type of "
                        "UsdPreviewSurface, but got `{}`",
                        typeName));
      }
    }

    MtlxUsdPreviewSurface surface;
    for (auto inp : usd_surface.children("input")) {
      std::string name;
      std::string typeName;
      std::string valueStr;
      GET_ATTR_VALUE(inp, "name", std::string, name);
      GET_ATTR_VALUE(inp, "type", std::string, typeName);
      GET_ATTR_VALUE(inp, "value", std::string, valueStr);

      // TODO: connection
      GET_SHADER_PARAM(name, typeName, "diffuseColor", "color3", value::color3f,
                       valueStr, surface.diffuseColor)
      GET_SHADER_PARAM(name, typeName, "emissiveColor", "color3",
                       value::color3f, valueStr, surface.emissiveColor)
      GET_SHADER_PARAM(name, typeName, "useSpecularWorkflow", "integer", int,
                       valueStr, surface.useSpecularWorkflow)
      GET_SHADER_PARAM(name, typeName, "specularColor", "color3",
                       value::color3f, valueStr, surface.specularColor)
      GET_SHADER_PARAM(name, typeName, "metallic", "float", float, valueStr,
                       surface.metallic)
      GET_SHADER_PARAM(name, typeName, "roughness", "float", float, valueStr,
                       surface.roughness)
      GET_SHADER_PARAM(name, typeName, "clearcoat", "float", float, valueStr,
                       surface.clearcoat)
      GET_SHADER_PARAM(name, typeName, "clearcoatRoughness", "float", float,
                       valueStr, surface.clearcoatRoughness)
      GET_SHADER_PARAM(name, typeName, "opacity", "float", float, valueStr,
                       surface.opacity)
      GET_SHADER_PARAM(name, typeName, "opacityThreshold", "float", float,
                       valueStr, surface.opacityThreshold)
      GET_SHADER_PARAM(name, typeName, "ior", "float", float, valueStr,
                       surface.ior)
      GET_SHADER_PARAM(name, typeName, "normal", "vector3f", value::normal3f,
                       valueStr, surface.normal)
      GET_SHADER_PARAM(name, typeName, "displacement", "float", float, valueStr,
                       surface.displacement)
      GET_SHADER_PARAM(name, typeName, "occlusion", "float", float, valueStr,
                       surface.occlusion) {
        PUSH_WARN("Unknown/unsupported input " << name);
      }
    }

    mtlx->shaders[surface_name] = surface;
  }

  // surfacematerial
  for (auto surfacematerial : root.children("surfacematerial")) {
    std::string material_name;
    {
      std::string typeName;
      GET_ATTR_VALUE(surfacematerial, "name", std::string, material_name);
      GET_ATTR_VALUE(surfacematerial, "type", std::string, typeName);

      if (typeName != "material") {
        PUSH_ERROR_AND_RETURN(fmt::format(
            "`material` expected for type of surfacematerial, but got `{}`",
            typeName));
      }
    }

    std::string typeName;
    std::string nodename;
    for (auto inp : surfacematerial.children("input")) {
      std::string name;
      GET_ATTR_VALUE(inp, "name", std::string, name);
      GET_ATTR_VALUE(inp, "type", std::string, typeName);
      GET_ATTR_VALUE(inp, "nodename", std::string, nodename);

      if (name != "surfaceshader") {
        PUSH_ERROR_AND_RETURN(
            fmt::format("Currently only `surfaceshader` supported for "
                        "`surfacematerial`'s input, but got `{}`",
                        name));
      }

      if (typeName != "surfaceshader") {
        PUSH_ERROR_AND_RETURN(
            fmt::format("Currently only `surfaceshader` supported for "
                        "`surfacematerial` input type, but got `{}`",
                        typeName));
      }
    }

    MtlxMaterial mat;
    mat.name = material_name;
    mat.typeName = typeName;
    mat.nodename = nodename;
    mtlx->surface_materials[material_name] = mat;
  }

  // look.
  for (auto look : root.children("look")) {
    PUSH_WARN("TODO: `look`");
    // TODO
    (void)look;
  }

#undef GET_ATTR_VALUE

  return true;
}

bool ReadMaterialXFromFile(const AssetResolutionResolver &resolver,
                           const std::string &asset_path, MtlxModel *mtlx,
                           std::string *warn, std::string *err) {
  std::string filepath = resolver.resolve(asset_path);
  if (filepath.empty()) {
    PUSH_ERROR_AND_RETURN("Asset not found: " + asset_path);
  }

  // up to 16MB xml
  size_t max_bytes = 1024 * 1024 * 16;

  std::vector<uint8_t> data;
  if (!io::ReadWholeFile(&data, err, filepath, max_bytes,
                         /* userdata */ nullptr)) {
    PUSH_ERROR_AND_RETURN("Read file failed.");
  }

  std::string str(reinterpret_cast<const char *>(&data[0]), data.size());
  return ReadMaterialXFromString(str, asset_path, mtlx, warn, err);
}

bool WriteMaterialXToString(const MtlxModel &mtlx, std::string &xml_str,
                            std::string *warn, std::string *err) {
  if (auto usdps = mtlx.shader.as<MtlxUsdPreviewSurface>()) {
    return detail::WriteMaterialXToString(*usdps, xml_str, warn, err);
  } else if (auto adskss = mtlx.shader.as<MtlxAutodeskStandardSurface>()) {
    // TODO
    PUSH_ERROR_AND_RETURN("TODO: AutodeskStandardSurface");
  } else {
    // TODO
    PUSH_ERROR_AND_RETURN("Unknown/unsupported shader: " << mtlx.shader_name);
  }

  return false;
}

bool ToPrimSpec(const MtlxModel &model, PrimSpec &ps, std::string *err) {
  //
  // def "MaterialX" {
  //
  //   def "Materials" {
  //     def Material ... {
  //     }
  //   }
  //   def "Shaders" {
  //   }
  constexpr auto kAutodeskStandardSurface = "AutodeskStandardSurface";

  if (model.shader_name == kUsdPreviewSurface) {
    ps.props()["info:id"] =
        detail::MakeProperty(value::token(kUsdPreviewSurface));
  } else if (model.shader_name == kAutodeskStandardSurface) {
    ps.props()["info:id"] =
        detail::MakeProperty(value::token(kAutodeskStandardSurface));
  } else {
    PUSH_ERROR_AND_RETURN("Unsupported shader_name: " << model.shader_name);
  }

  PrimSpec materials;
  materials.name() = "Materials";
  materials.specifier() = Specifier::Def;

  for (const auto &item : model.surface_materials) {
    PrimSpec material;
    material.specifier() = Specifier::Def;
    material.typeName() = "Material";

    material.name() = item.second.name;
  }

  PrimSpec shaders;
  shaders.name() = "Shaders";
  shaders.specifier() = Specifier::Def;

  PrimSpec root;
  root.name() = "MaterialX";
  root.specifier() = Specifier::Def;

  root.children().push_back(materials);
  root.children().push_back(shaders);

  ps = std::move(root);

  return true;
}

bool LoadMaterialXFromAsset(const Asset &asset, const std::string &asset_path,
                            PrimSpec &ps /* inout */, std::string *warn,
                            std::string *err) {
  (void)asset_path;
  (void)warn;

  if (asset.size() < 32) {
    if (err) {
      (*err) += "MateiralX: Asset size too small.\n";
    }
    return false;
  }

  std::string str(reinterpret_cast<const char *>(asset.data()), asset.size());

  MtlxModel mtlx;
  if (!ReadMaterialXFromString(str, asset_path, &mtlx, warn, err)) {
    PUSH_ERROR_AND_RETURN("Failed to read MaterialX.");
  }

  if (!ToPrimSpec(mtlx, ps, err)) {
    PUSH_ERROR_AND_RETURN("Failed to convert MaterialX to USD PrimSpec.");
  }

  return true;
}

//} // namespace usdMtlx
}  // namespace tinyusdz

#else

namespace tinyusdz {

bool ReadMaterialXFromFile(const AssetResolutionResolver &resolver,
                           const std::string &asset_path, MtlxModel *mtlx,
                           std::string *warn, std::string *err) {
  (void)resolver;
  (void)asset_path;
  (void)mtlx;
  (void)warn;

  if (err) {
    (*err) += "MaterialX support is disabled in this build.\n";
  }
  return false;
}

bool WriteMaterialXToString(const MtlxModel &mtlx, std::string &xml_str,
                            std::string *warn, std::string *err) {
  (void)mtlx;
  (void)xml_str;
  (void)warn;

  if (err) {
    (*err) += "MaterialX support is disabled in this build.\n";
  }
  return false;
}

bool LoadMaterialXFromAsset(const Asset &asset, const std::string &asset_path,
                            PrimSpec &ps /* inout */, std::string *warn,
                            std::string *err) {
  (void)asset;
  (void)asset_path;
  (void)ps;
  (void)warn;

  if (err) {
    (*err) += "MaterialX support is disabled in this build.\n";
  }

  return false;
}

#if 0
bool ToPrimSpec(const MtlxModel &model, PrimSpec &ps, std::string *err)
  (void)model;
  (void)ps;

  if (err) {
    (*err) += "MaterialX support is disabled in this build.\n";
  }
  return false;

}
#endif

}  // namespace tinyusdz

#endif  // TINYUSDZ_USE_USDMTLX
