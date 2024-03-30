#include "shader-network.hh"
#include "prim-apply.hh"

#include "prim-types.hh"
#include "usdShade.hh"
#include "pprinter.hh"
#include "prim-pprint.hh"
#include "value-pprint.hh"
#include "stage.hh"
#include "common-macros.inc"
#include "tydra/scene-access.hh"

namespace tinyusdz {
namespace tydra {

namespace {

// TODO: There are lots of duplicated codes with EvaluateAttribute()
// Use EvaluateAttribute and deprecate EvaluateShaderAttribute?

bool EvaluateUsdPreviewSurfaceAttribute(
  const Stage &stage,
  const UsdPreviewSurface &shader,
  const std::string &attr_name,
  const uint32_t req_type_id,
  value::Value &out_val,
  std::string *err,
  const value::TimeCode timeCode) {

  (void)stage;

  if ((attr_name == "diffuseColor") && (req_type_id == value::TypeTraits<value::color3f>::type_id())) {
    if (shader.diffuseColor.authored()) {

    } else {
      value::color3f col;
      if (shader.diffuseColor.get_value().get_scalar(&col)) {
        out_val = col;
        return true;
      }
    }
  }

  (void)err;
  (void)timeCode;

  return false;
}


} // namespace local


template<typename T>
bool EvaluateShaderAttribute(
  const Stage &stage,
  const Shader &shader, const std::string &attr_name,
  T * out_val,
  std::string *err,
  const value::TimeCode timeCode) {

  if (!out_val) {
    return false;
  }

  uint32_t tyid = value::TypeTraits<T>::type_id();
  value::Value outval;

  bool result = false;

  if (const auto *psurf = shader.value.as<UsdPreviewSurface>()) {
    result = EvaluateUsdPreviewSurfaceAttribute(stage, *psurf, attr_name, tyid, outval, err, timeCode);
    if (const auto pt = outval.as<T>()) {
      (*out_val) = (*pt);
    } else {
      if (err) {
        (*err) += "[InternalError] Type mismatch.\n";
      }
      return false;
    }
  } else {
    if (err) {
      (*err) += "Unsupported shader type: " + shader.value.type_name() + "\n";
    }
    return false;
  }

  return result;
}

#define INSTANCIATE_EVAL_SHADER(__ty) \
template bool EvaluateShaderAttribute( const Stage &stage, const Shader &shader, const std::string &attr_name, __ty * out_val, std::string *err, const value::TimeCode timeCode)

INSTANCIATE_EVAL_SHADER(value::token);
INSTANCIATE_EVAL_SHADER(std::string);
INSTANCIATE_EVAL_SHADER(value::half);
INSTANCIATE_EVAL_SHADER(value::half2);
INSTANCIATE_EVAL_SHADER(value::half3);
INSTANCIATE_EVAL_SHADER(value::half4);
INSTANCIATE_EVAL_SHADER(int32_t);
INSTANCIATE_EVAL_SHADER(value::int2);
INSTANCIATE_EVAL_SHADER(value::int3);
INSTANCIATE_EVAL_SHADER(value::int4);
INSTANCIATE_EVAL_SHADER(uint32_t);
INSTANCIATE_EVAL_SHADER(value::uint2);
INSTANCIATE_EVAL_SHADER(value::uint3);
INSTANCIATE_EVAL_SHADER(value::uint4);
INSTANCIATE_EVAL_SHADER(float);
INSTANCIATE_EVAL_SHADER(value::float2);
INSTANCIATE_EVAL_SHADER(value::float3);
INSTANCIATE_EVAL_SHADER(value::float4);
INSTANCIATE_EVAL_SHADER(double);
INSTANCIATE_EVAL_SHADER(value::double2);
INSTANCIATE_EVAL_SHADER(value::double3);
INSTANCIATE_EVAL_SHADER(value::double4);
INSTANCIATE_EVAL_SHADER(value::quath);
INSTANCIATE_EVAL_SHADER(value::quatf);
INSTANCIATE_EVAL_SHADER(value::quatd);
INSTANCIATE_EVAL_SHADER(value::color3h);
INSTANCIATE_EVAL_SHADER(value::color3f);
INSTANCIATE_EVAL_SHADER(value::color3d);
INSTANCIATE_EVAL_SHADER(value::color4h);
INSTANCIATE_EVAL_SHADER(value::color4f);
INSTANCIATE_EVAL_SHADER(value::color4d);
INSTANCIATE_EVAL_SHADER(value::vector3h);
INSTANCIATE_EVAL_SHADER(value::vector3f);
INSTANCIATE_EVAL_SHADER(value::vector3d);
INSTANCIATE_EVAL_SHADER(value::point3h);
INSTANCIATE_EVAL_SHADER(value::point3f);
INSTANCIATE_EVAL_SHADER(value::point3d);
INSTANCIATE_EVAL_SHADER(value::normal3h);
INSTANCIATE_EVAL_SHADER(value::normal3f);
INSTANCIATE_EVAL_SHADER(value::normal3d);
INSTANCIATE_EVAL_SHADER(value::matrix2d);
INSTANCIATE_EVAL_SHADER(value::matrix3d);
INSTANCIATE_EVAL_SHADER(value::matrix4d);

// instanciations

namespace {

bool GetSinglePath(const Relationship &rel, Path *path) {
  if (!path) {
    return false;
  }

  if (rel.is_path()) {
    (*path) = rel.targetPath;
    return true;
  } else if (rel.is_pathvector()) {
    if (rel.targetPathVector.size() > 0) {
      (*path) = rel.targetPathVector[0];
      return true;
    }
  }

  return false;
}

} // namespace local

bool GetBoundMaterial(
  const Stage &_stage,
  const Prim &prim,
  const std::string &suffix,
  tinyusdz::Path *materialPath,
  const Material **material,
  std::string *err) {

  if (materialPath == nullptr) {
    return false;
  }

  (void)err;

  auto apply_fun = [&](const Stage &stage, const GPrim *gprim) -> bool {
    if (suffix.empty()) {
      if (gprim->materialBinding.has_value()) {
        if (GetSinglePath(gprim->materialBinding.value(), materialPath)) {

          const Prim *p;
          if (stage.find_prim_at_path(*materialPath, p, err)) {
            if (p->is<Material>() && (material != nullptr)) {
              (*material) = p->as<Material>();
            } else {
              (*material) = nullptr;
            }
          }

          return true;
        }
      }
    } else if (suffix == "collection") {
      if (gprim->materialBindingCollection.has_value()) {
        if (GetSinglePath(gprim->materialBindingCollection.value(), materialPath)) {

          const Prim *p;
          if (stage.find_prim_at_path(*materialPath, p, err)) {
            if (p->is<Material>() && (material != nullptr)) {
              (*material) = p->as<Material>();
            } else {
              (*material) = nullptr;
            }
          }

          return true;
        }
      }
    } else if (suffix == "preview") {
      if (gprim->materialBindingPreview.has_value()) {
        if (GetSinglePath(gprim->materialBindingPreview.value(), materialPath)) {

          const Prim *p{nullptr};
          if (stage.find_prim_at_path(*materialPath, p, err)) {
            if (p->is<Material>() && (material != nullptr)) {
              (*material) = p->as<Material>();
            } else {
              (*material) = nullptr;
            }
          }

          return true;
        }
      }
    } else {
      return false;
    }

    return false;
  };

  bool ret = ApplyToGPrim(_stage, prim, apply_fun);

  return ret;
}

bool FindBoundMaterial(
  const Stage &_stage,
  const Path &abs_path,
  const std::string &suffix,
  tinyusdz::Path *materialPath,
  const Material **material,
  std::string *err) {

  if (materialPath == nullptr) {
    return false;
  }

  const Prim *prim{nullptr};
  bool ret = _stage.find_prim_at_path(abs_path, prim, err);

  if (!ret) {
    return false;
  }

  auto apply_fun = [&](const Stage &stage, const GPrim *gprim) -> bool {
    if (suffix.empty()) {
      if (gprim->materialBinding.has_value()) {
        if (GetSinglePath(gprim->materialBinding.value(), materialPath)) {
          DCOUT("GPrim has materialBinding.");
          const Prim *p;
          if (stage.find_prim_at_path(*materialPath, p, err)) {
            DCOUT("material = " << material);
            if (p->is<Material>() && (material != nullptr)) {
              DCOUT("Store mat.");
              (*material) = p->as<Material>();
            } else {
              DCOUT("nullmat.");
              (*material) = nullptr;
            }
          }
          return true;
        }
      } else {
          DCOUT("GPrim has no materialBinding.");
      }
    } else if (suffix == "collection") {
      if (gprim->materialBindingCollection.has_value()) {
        if (GetSinglePath(gprim->materialBindingCollection.value(), materialPath)) {

          const Prim *p;
          if (stage.find_prim_at_path(*materialPath, p, err)) {
            if (p->is<Material>() && (material != nullptr)) {
              (*material) = p->as<Material>();
            } else {
              (*material) = nullptr;
            }
          }

          return true;
        }
      }
    } else if (suffix == "preview") {
      if (gprim->materialBindingPreview.has_value()) {
        if (GetSinglePath(gprim->materialBindingPreview.value(), materialPath)) {

          const Prim *p{nullptr};
          if (stage.find_prim_at_path(*materialPath, p, err)) {
            if (p->is<Material>() && (material != nullptr)) {
              (*material) = p->as<Material>();
            } else {
              (*material) = nullptr;
            }
          }

          return true;
        }
      }
    } else {
      return false;
    }

    return false;
  };

  ret = ApplyToGPrim(_stage, *prim, apply_fun);
  if (ret) {
    return true;
  }

  // Search parent Prim's materialBinding.
  Path currentPath = abs_path;

  int depth = 0;
  while (depth < 1024*1024*128) { // to avoid infinite loop.
    Path parentPath = currentPath.get_parent_prim_path();
    DCOUT("search parent: " << parentPath.full_path_name());

    if (parentPath.is_valid() && (!parentPath.is_root_path())) {
      ret = _stage.find_prim_at_path(parentPath, prim, err);
      if (!ret) {
        return false;
      }

      ret = ApplyToGPrim(_stage, *prim, apply_fun);
      if (ret) {
        return true;
      }

    } else {
      return false;
    }

    if (parentPath.is_root_prim()) {
      // no further parent Prim.
      return false;
    }

    currentPath = parentPath;

    depth++;
  }

  return false;
}

} // namespace tydra
} // namespace tinyusdz
