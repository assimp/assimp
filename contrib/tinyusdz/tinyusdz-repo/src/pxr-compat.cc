#include "pxr-compat.hh"

#include "tinyusdz.hh"

namespace pxr {

USD_API UsdStageRefPtr UsdStage::Open(const std::string &filepath, InitialLoadSet loadset) {
  (void)filepath;
  (void)loadset;

  // TODO:
  return std::shared_ptr<UsdStage>(nullptr);
};

USD_API UsdStageRefPtr UsdStage::CreateNew(const std::string &filepath, InitialLoadSet loadset) {
  (void)filepath;
  (void)loadset;

  // TODO:
  return std::shared_ptr<UsdStage>(nullptr);
};

USD_API UsdPrim UsdStage::GetPrimAtPath(const SdfPath &path) {
  (void)path;

  return UsdPrim();
};

} // namespace pxr
