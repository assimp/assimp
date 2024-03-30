// SPDX-License-Identifier: Apache 2.0
// Copyright 2022-Present Light Transport Entertainment, Inc.
//
// Scene access API
//
// NOTE: Tydra API does not use nonstd::optional and nonstd::expected,
// std::functions and other non basic STL feature for easier language bindings.
//
#pragma once

#include <map>

#include "prim-types.hh"
#include "stage.hh"
#include "usdGeom.hh"
#include "usdShade.hh"
#include "usdSkel.hh"
#include "value-types.hh"

namespace tinyusdz {
namespace tydra {

// key = fully absolute Prim path in string(e.g. "/xform/geom0")
template <typename T>
using PathPrimMap = std::map<std::string, const T *>;

//
// value = pair of Shader Prim which contains the Shader type T("info:id") and
// its concrete Shader type(UsdPreviewSurface)
//
template <typename T>
using PathShaderMap =
    std::map<std::string, std::pair<const Shader *, const T *>>;

///
/// List Prim of type T from the Stage.
/// Returns false when unsupported/unimplemented Prim type T is given.
///
template <typename T>
bool ListPrims(const tinyusdz::Stage &stage, PathPrimMap<T> &m /* output */);

///
/// List Shader of shader type T from the Stage.
/// Returns false when unsupported/unimplemented Shader type T is given.
/// TODO: User defined shader type("info:id")
///
template <typename T>
bool ListShaders(const tinyusdz::Stage &stage,
                 PathShaderMap<T> &m /* output */);

///
/// Get parent Prim from Path.
/// Path must be fully expanded absolute path.
///
/// Example: Return "/xform" Prim for "/xform/mesh0" path
///
/// Returns nullptr when the given Path is a root Prim or invalid Path(`err`
/// will be filled when failed).
///
const Prim *GetParentPrim(const tinyusdz::Stage &stage,
                          const tinyusdz::Path &path, std::string *err);

///
/// Visit Stage and invoke callback functions for each Prim.
/// Can be used for alternative method of Stage::Traverse() in pxrUSD
///

///
/// Use old-style Callback function approach for easier language bindings
///
/// @param[in] abs_path Prim's absolute path(e.g. "/xform/mesh0")
/// @param[in] prim Prim
/// @param[in] tree_depth Tree depth of this Prim. 0 = root prim.
/// @param[inout] userdata User data.
/// @param[out] error message.
///
/// @return Usually true. return false + no error message to notify early
/// termination of visiting Prims.
///
typedef bool (*VisitPrimFunction)(const Path &abs_path, const Prim &prim,
                                  const int32_t tree_depth, void *userdata,
                                  std::string *err);

///
/// Visit Prims in Stage.
/// Use `primChildren` metadatum to determine traversal order of Prims if
/// exists(USDC usually contains `primChildren`) Traversal will be failed when
/// no Prim found specified in `primChildren`(if exists)
///
/// @param[out] err Error message.
///
bool VisitPrims(const tinyusdz::Stage &stage, VisitPrimFunction visitor_fun,
                void *userdata = nullptr, std::string *err = nullptr);

///
/// Get Property(Attribute or Relationship) of given Prim by name.
/// Similar to UsdPrim::GetProperty() in pxrUSD.
///
/// @param[in] prim Prim
/// @param[in] prop_name Property name
/// @param[out] prop Property
/// @param[out] err Error message(filled when returning false)
///
/// @return true if Property found in given Prim.
/// @return false if Property is not found in given Prim.
///
bool GetProperty(const tinyusdz::Prim &prim, const std::string &prop_name,
                 Property *prop, std::string *err);

///
/// Get List of Property(Attribute and Relationship) names of given Prim by name.
/// It includes authored builtin Property names.
///
/// @param[in] prim Prim
/// @param[out] prop_names Property names
/// @param[out] err Error message(filled when returning false)
///
/// @return true upon success.
/// @return false when something go wrong.
///
bool GetPropertyNames(const tinyusdz::Prim &prim, std::vector<std::string> *prop_names, std::string *err);

///
/// Get List of Attribute names of given Prim.
/// It includes authored builtin Attribute names(e.g. "points" for `GeomMesh`).
///
/// @param[in] prim Prim
/// @param[out] attr_names Attribute names
/// @param[out] err Error message(filled when returning false)
///
/// @return true upon success.
/// @return false when something go wrong.
///
bool GetAttributeNames(const tinyusdz::Prim &prim, std::vector<std::string> *attr_names, std::string *err);

///
/// Get List of Relationship names of given Prim.
/// It includes authored builtin Relationship names(e.g. "proxyPrim" for `GeomMesh`).
///
/// @param[in] prim Prim
/// @param[out] rel_names Relationship names
/// @param[out] err Error message(filled when returning false)
///
/// @return true upon success.
/// @return false when something go wrong.
///
bool GetRelationshipNames(const tinyusdz::Prim &prim, std::vector<std::string> *rel_names, std::string *err);

///
/// Get Attribute of given Prim by name.
/// Similar to UsdPrim::GetAttribute() in pxrUSD.
///
/// @param[in] prim Prim
/// @param[in] attr_name Attribute name
/// @param[out] attr Attribute
/// @param[out] err Error message(filled when returning false)
///
/// @return true if Attribute found in given Prim.
/// @return false if Attribute is not found in given Prim, or `attr_name` is a
/// Relationship.
///
bool GetAttribute(const tinyusdz::Prim &prim, const std::string &attr_name,
                  Attribute *attr, std::string *err);

///
/// Check if Prim has Attribute.
///
/// @param[in] prim Prim
/// @param[in] attr_name Attribute name to query.
///
/// @return true if `attr_name` Attribute exists in the Prim.
///
bool HasAttribute(const tinyusdz::Prim &prim, const std::string &attr_name);

///
/// Get Relationship of given Prim by name.
/// Similar to UsdPrim::GetRelationship() in pxrUSD.
///
/// @param[in] prim Prim
/// @param[in] rel_name Relationship name
/// @param[out] rel Relationship
/// @param[out] err Error message(filled when returning false)
///
/// @return true if Relationship found in given Prim.
/// @return false if Relationship is not found in given Prim, or `rel_name` is a
/// Attribute.
///
bool GetRelationship(const tinyusdz::Prim &prim, const std::string &rel_name,
                     Relationship *rel, std::string *err);

///
/// Check if Prim has Relationship.
///
/// @param[in] prim Prim
/// @param[in] rel_name Relationship name to query.
///
/// @return true if `rel_name` Relationship exists in the Prim.
///
bool HasRelationship(const tinyusdz::Prim &prim, const std::string &rel_name);

///
/// Terminal Attribute value at specified timecode.
///
/// - No `None`(Value Blocked)
/// - No connection(connection target is evaluated(fetch value producing
/// attribute))
/// - No timeSampled value
///
class TerminalAttributeValue {
 public:
  TerminalAttributeValue() = default;

  TerminalAttributeValue(const value::Value &v) : _empty{false}, _value(v) {}
  TerminalAttributeValue(value::Value &&v)
      : _empty{false}, _value(std::move(v)) {}

  // "empty" attribute(type info only)
  void set_empty_attribute(const std::string &type_name) {
    _empty = true;
    _type_name = type_name;
  }

  TerminalAttributeValue(const std::string &type_name) {
    set_empty_attribute(type_name);
  }

  bool is_empty() const { return _empty; }

  template <typename T>
  const T *as() const {
    if (_empty) {
      return nullptr;
    }
    return _value.as<T>();
  }

  template <typename T>
  bool is() const {
    if (_empty) {
      return false;
    }

    if (_value.as<T>()) {
      return true;
    }
    return false;
  }

  void set_value(const value::Value &v) {
    _value = v;
    _empty = false;
  }

  void set_value(value::Value &&v) {
    _value = std::move(v);
    _empty = false;
  }

  const std::string type_name() const {
    if (_empty) {
      return _type_name;
    }

    return _value.type_name();
  }

  uint32_t type_id() const {
    if (_empty) {
      return value::GetTypeId(_type_name);
    }

    return _value.type_id();
  }

  Variability variability() const { return _variability; }
  Variability &variability() { return _variability; }

  const AttrMeta &meta() const { return _meta; }
  AttrMeta &meta() { return _meta; }

 private:
  bool _empty{true};
  std::string _type_name;
  Variability _variability{Variability::Varying};
  value::Value _value{nullptr};
  AttrMeta _meta;
};

///
/// Evaluate Attribute of the specied Prim and retrieve terminal Attribute
/// value.
///
/// - If the attribute is empty(e.g. `float outputs:r`), return "empty"
/// Attribute
/// - If the attribute is scalar value, simply returns it.
/// - If the attribute is timeSamples value, evaluate the value at specified
/// time.
/// - If the attribute is connection, follow the connection target
///
/// @param[in] stage Stage
/// @param[in] prim Prim
/// @param[in] attr_name Attribute name
/// @param[out] value Evaluated terminal attribute value.
/// @param[out] err Error message(filled when false returned)
/// @param[in] t (optional) TimeCode(for timeSamples Attribute)
/// @param[in] tinterp (optional) Interpolation type for timeSamples value
///
/// Return false when:
///
/// - If the attribute is None(ValueBlock)
/// - Requested attribute not found in a Prim.
/// - Invalid connection(e.g. type mismatch, circular referencing, targetPath
/// points non-existing path, etc),
/// - Other error happens.
///
bool EvaluateAttribute(
    const tinyusdz::Stage &stage, const tinyusdz::Prim &prim,
    const std::string &attr_name, TerminalAttributeValue *value,
    std::string *err, const double t = tinyusdz::value::TimeCode::Default(),
    const tinyusdz::value::TimeSampleInterpolationType tinterp =
        tinyusdz::value::TimeSampleInterpolationType::Held);

///
/// For efficient Xform retrieval from Stage.
///
/// XformNode's pointer value and hierarchy become invalid when Prim is
/// removed/added to Stage. If you change the content of Stage, please rebuild
/// XformNode using BuildXformNodeFromStage() again
///
/// TODO: Use prim_id and deprecate the pointer to Prim.
///
struct XformNode {
  std::string element_name;  // e.g. "geom0"
  Path absolute_path;        // e.g. "/xform/geom0"

  const Prim *prim{nullptr};  // The pointer to Prim.
  int64_t prim_id{-1};        // Prim id(1 or greater for valid Prim ID)

  XformNode *parent{nullptr};  // pointer to parent
  std::vector<XformNode> children;

  const value::matrix4d &get_local_matrix() const { return _local_matrix; }

  // world matrix = parent_world_matrix x local_matrix
  // Equivalent to GetLocalToWorldMatrix in pxrUSD
  // if !resetXformStack! exists in Prim's xformOpOrder, this returns Prim's local matrix
  // (clears parent's world matrix)
  const value::matrix4d &get_world_matrix() const { return _world_matrix; }

  const value::matrix4d &get_parent_world_matrix() const {
    return _parent_world_matrix;
  }

  // TODO: accessible only from Friend class?
  void set_local_matrix(const value::matrix4d &m) { _local_matrix = m; }

  void set_world_matrix(const value::matrix4d &m) { _world_matrix = m; }

  void set_parent_world_matrix(const value::matrix4d &m) {
    _parent_world_matrix = m;
  }

  // true: Prim with Xform(e.g. GeomMesh)
  // false: Prim with no Xform(e.g. Stage root("/"), Scope, Material, ...)
  bool has_xform() const { return _has_xform; }
  bool &has_xform() { return _has_xform; }

  bool has_resetXformStack() const { return _has_resetXformStack; }
  bool &has_resetXformStack() { return _has_resetXformStack; }

 private:
  bool _has_xform{false};
  bool _has_resetXformStack{false};  // !resetXformStack! in xformOps
  value::matrix4d _local_matrix{value::matrix4d::identity()};
  value::matrix4d _world_matrix{value::matrix4d::identity()};
  value::matrix4d _parent_world_matrix{value::matrix4d::identity()};
};

///
/// Build Xform scene hierachy from Stage.
///
/// You can build Xform node graph using BuildXformNodeFromStage()
///
/// Set a time, and compute xform of each Prim and store its cache(i.e. read
/// only).
///
/// TODO: Support timeSamples.
///
bool BuildXformNodeFromStage(
    const tinyusdz::Stage &stage, XformNode *root, /* out */
    const double t = tinyusdz::value::TimeCode::Default(),
    const tinyusdz::value::TimeSampleInterpolationType tinterp =
        tinyusdz::value::TimeSampleInterpolationType::Held);

std::string DumpXformNode(const XformNode &root);

///
/// Get GeomSubset children of the given Prim path
///
/// The pointer address is valid until Stage's content is unchanged. 
///
/// @param[in] familyName Get GeomSubset having this `familyName`. empty token = return all GeomSubsets. 
/// @param[in] prim_must_be_geommesh Prim path must point to GeomMesh Prim.
///
/// (TODO: Return id of GeomSubset Prim object, instead of the ponter address)
///
/// @return array of GeomSubset pointers. Empty array when failed or no GeomSubset Prim(with `familyName`) attached to the Prim.
///
///
std::vector<const GeomSubset *> GetGeomSubsets(const tinyusdz::Stage &stage, const tinyusdz::Path &prim_path, const tinyusdz::value::token &familyName, bool prim_must_be_geommesh = true);

///
/// Get GeomSubset children of the given Prim
///
/// The pointer address is valid until Stage's content is unchanged. 
///
/// @param[in] familyName Get GeomSubset having this `familyName`. empty token = return all GeomSubsets. 
/// @param[in] prim_must_be_geommesh Prim must be GeomMesh Prim type.
///
/// (TODO: Return id of GeomSubset Prim object, instead of the ponter address)
///
/// @return array of GeomSubset pointers. Empty array when failed or no GeomSubset Prim(with `familyName`) attached to the Prim.
///
std::vector<const GeomSubset *> GetGeomSubsetChildren(const tinyusdz::Prim &prim, const tinyusdz::value::token &familyName, bool prim_must_be_geommesh = true);

#if 0 // TODO
///
/// Get list of GeomSubset PrimSpecs attached to the PrimSpec
/// Prim path must point to GeomMesh PrimSpec.
///
/// The pointer address is valid until Layer's content is unchanged. 
///
/// (TODO: Return PrimSpec index instead of the ponter address)
///
std::vector<const PrimSpec *> GetGeomSubsetPrimSpecs(const tinyusdz::Layer &layer, const tinyusdz::Path &prim_path);

std::vector<const PrimSpec *> GetGeomSubsetChildren(const tinyusdz::Path &prim_path);
#endif

///
/// For composition. Convert Concrete Prim(Xform, GeomMesh, ...) to PrimSpec, generic Prim container.
/// TODO: Move to *core* module?
///
bool PrimToPrimSpec(const Prim &prim, PrimSpec &ps, std::string *err);

///
/// For MaterialX
/// TODO: Move to shader-network.hh?
///
bool ShaderToPrimSpec(const UsdUVTexture &node, PrimSpec &ps, std::string *warn, std::string *err);
bool ShaderToPrimSpec(const UsdTransform2d &node, PrimSpec &ps, std::string *warn, std::string *err);

template<typename T>
bool ShaderToPrimSpec(const UsdPrimvarReader<T> &node, PrimSpec &ps, std::string *warn, std::string *err);

//
// Utilities and Query for CollectionAPI
// 

///
/// Get `Collection` object(properties defined in Collection API) from a given Prim.
///
/// @param[in] prim Prim
/// @param[out] Pointer to the pointer of found Collection.
/// @return true upon success.
///
bool GetCollection(const Prim &prim, const Collection **collection);

class CollectionMembershipQuery
{
 public:
  
 private:
  std::map<Path, CollectionInstance::ExpansionRule> _expansionRuleMap;

};

///
/// Build Collection Membership
///
/// It traverse collection paths starting from `seedCollectionInstance` in the Stage.
/// Note: No circular referencing path allowed.
///
/// @returns CollectionMembershipQuery object. When encountered an error, CollectionMembershipQuery contains empty info(i.e, all query will fail) 
///
CollectionMembershipQuery BuildCollectionMembershipQuery(
  const Stage &stage, const CollectionInstance &seedCollectionInstance);

bool IsPathIncluded(const CollectionMembershipQuery &query, const Stage &stage, const Path &abs_path, const CollectionInstance::ExpansionRule expansionRule = CollectionInstance::ExpansionRule::ExpandPrims);

// TODO: Layer version
// bool IsPathIncluded(const Layer &layer, const Path &abs_path, const CollectionInstance::ExpansionRule expansionRule = CollectionInstance::ExpansionRule::ExpandPrims);

//
// For USDZ AR extensions
//

///
/// List up `sceneName` of given Prim's children
/// https://developer.apple.com/documentation/realitykit/usdz-schemas-for-ar
///
/// Prim's Kind must be `sceneLibrary`
/// @param[out] List of pair of (Is Specifier `over`, sceneName). For `def`
/// Specifier(primary scene), it is set to false.
///
///
bool ListSceneNames(const tinyusdz::Prim &root,
                    std::vector<std::pair<bool, std::string>> *sceneNames);

}  // namespace tydra
}  // namespace tinyusdz
