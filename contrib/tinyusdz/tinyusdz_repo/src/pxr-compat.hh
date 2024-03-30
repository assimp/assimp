// SPDX-License-Identifier: Apache 2.0

//
// Experimental pxr USD compatible API
//
#pragma once

#include <memory>
#include <string>

#include "prim-types.hh"

#if !defined(PXR_DYNAMIC)
  #define USD_API
#else
  #if defined(USD_EXPORTS)
    #if defined(_WIN32)
      #if defined(_MSC_VER)
        #define USD_API __declspec(dllexport)
      #else
        // assume gcc or clang
        #define USD_API __attribute__((dllexport))
      #endif
    #else
        #define USD_API
    #endif
  #else // import

    #if defined(_WIN32)
      #if defined(_MSC_VER)
        #define USD_API __declspec(dllimport)
      #else
        // assume gcc or clang
        #define USD_API __attribute__((dllimport))
      #endif
    #else
      #define USD_API
    #endif
  #endif

#endif

#define PXR_INTERNAL_NS pxr

namespace pxr {

//namespace Sdf {

using TfToken = tinyusdz::value::token;

struct SdfLayer;

// pxr USD uses special pointer class(shared_ptr + alpha) for Handle, but we simply use shared_ptr.
using SdfLayerHandle = std::shared_ptr<SdfLayer>;


//} // namespae Sdf

//namespace Usd {

struct UsdStage;

using UsdStagePtr = std::weak_ptr<UsdStage>;
using UsdStageRefPtr = std::shared_ptr<UsdStage>;
typedef UsdStagePtr UsdStageWeakPtr; 

// prim could be invalid(empty)
struct UsdPrim
{
  UsdPrim() : _prim(nullptr) {}
  //UsdPrim(tinyusdz::GPrim *prim) : _prim(prim) {}

  bool IsValid() const {
    if (_prim.type_id() == tinyusdz::value::TypeId::TYPE_ID_INVALID) {
      return false;
    }

    // TODO: if (IsConcrete(_type))

    return true;
  }

  explicit operator bool() {
    return IsValid();
  }

  // TODO: Use raw pointer?
  tinyusdz::value::Value _prim;
};

struct SdfPath
{
  std::string path;
};

class UsdPrimRange
{
 public:
  class iterator;

  iterator begin() const;

  bool empty() const;

};

enum class Usd_PrimFlags {
    Usd_PrimActiveFlag,
    Usd_PrimLoadedFlag,
    Usd_PrimModelFlag,
    Usd_PrimGroupFlag,
    Usd_PrimAbstractFlag,
    Usd_PrimDefinedFlag,
    Usd_PrimHasDefiningSpecifierFlag,
    Usd_PrimInstanceFlag,

    Usd_PrimHasPayloadFlag,
    Usd_PrimClipsFlag,
    Usd_PrimDeadFlag,
    Usd_PrimPrototypeFlag,
    Usd_PrimInstanceProxyFlag,
    Usd_PrimPseudoRootFlag,

    Usd_PrimAllFlag
};


class Usd_PrimFlagsPredicate;

struct UsdStage
{

  enum InitialLoadSet
  {
    LoadAll,
    LoadNone
  };

  USD_API static UsdStageRefPtr CreateNew(const std::string &filepath, InitialLoadSet = LoadAll);
  USD_API static UsdStageRefPtr CreateInMemory(InitialLoadSet = LoadAll);

  USD_API static UsdStageRefPtr Open(const std::string &filepath, InitialLoadSet = LoadAll);

  USD_API void Save();
  USD_API void SaveSessionLayers();

  USD_API bool Export(const std::string &filename, bool addSourceFileComments=true) const;
  USD_API bool ExportToString(std::string *result, bool addSourceFileComments=true) const;

  USD_API UsdPrim DefinePrim(const SdfPath &path, const TfToken &typeName = TfToken());
  USD_API UsdPrim OverridePrim(const SdfPath &path);

  // returns invalid(empty) prim if corresponding path does not exit in the stage.
  USD_API UsdPrim GetPrimAtPath(const SdfPath &path);

  USD_API UsdPrimRange Traverse();
  USD_API UsdPrimRange Traverse(const Usd_PrimFlagsPredicate &predicate);
  USD_API UsdPrimRange TraverseAll();

};

//} // namespace Usd;
} // namespace pxr
