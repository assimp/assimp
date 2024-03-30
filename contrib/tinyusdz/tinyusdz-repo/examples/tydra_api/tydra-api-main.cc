#include <algorithm>
#include <iostream>
#include <sstream>

#include "tinyusdz.hh"
#include "tydra/render-data.hh"
#include "tydra/scene-access.hh"
#include "tydra/shader-network.hh"
#include "usdShade.hh"
#include "pprinter.hh"
#include "prim-pprint.hh"
#include "value-pprint.hh"
#include "value-types.hh"

static std::string GetFileExtension(const std::string &filename) {
  if (filename.find_last_of('.') != std::string::npos)
    return filename.substr(filename.find_last_of('.') + 1);
  return "";
}

static std::string str_tolower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 // static_cast<int(*)(int)>(std::tolower)         // wrong
                 // [](int c){ return std::tolower(c); }           // wrong
                 // [](char c){ return std::tolower(c); }          // wrong
                 [](unsigned char c) { return std::tolower(c); }  // correct
  );
  return s;
}

// key = Full absolute prim path(e.g. `/bora/dora`)
using XformMap = std::map<std::string, const tinyusdz::Xform *>;
using MeshMap = std::map<std::string, const tinyusdz::GeomMesh *>;
using MaterialMap = std::map<std::string, const tinyusdz::Material *>;
using PreviewSurfaceMap =
    std::map<std::string, std::pair<const tinyusdz::Shader *, const tinyusdz::UsdPreviewSurface *>>;
using UVTextureMap = std::map<std::string, std::pair<const tinyusdz::Shader *, const tinyusdz::UsdUVTexture *>>;
using PrimvarReader_float2Map =
    std::map<std::string, std::pair<const tinyusdz::Shader *, const tinyusdz::UsdPrimvarReader_float2 *>>;

#if 0
template <typename T>
static bool TraverseRec(const std::string &path_prefix,
                        const tinyusdz::Prim &prim, uint32_t depth,
                        std::map<std::string, const T *> &itemmap) {
  if (depth > 1024 * 128) {
    // Too deep
    return false;
  }

  std::string prim_abs_path = path_prefix + "/" + prim.local_path().full_path_name();

  if (prim.is<tinyusdz::Material>()) {
    if (const T *pv = prim.as<T>()) {
      std::cout << "Path : <" << prim_abs_path << "> is "
                << tinyusdz::value::TypeTraits<T>::type_name() << ".\n";
      itemmap[prim_abs_path] = pv;
    }
  }

  for (const auto &child : prim.children()) {
    if (!TraverseRec(prim_abs_path, child, depth + 1, itemmap)) {
      return false;
    }
  }

  return true;
}

template <typename T>
static bool TraverseShaderRec(const std::string &path_prefix,
                        const tinyusdz::Prim &prim, uint32_t depth,
                        std::map<std::string, const T *> &itemmap) {
  if (depth > 1024 * 128) {
    // Too deep
    return false;
  }

  std::string prim_abs_path = path_prefix + "/" + prim.local_path().full_path_name();

  // First test if Shader prim.
  if (const tinyusdz::Shader *ps = prim.as<tinyusdz::Shader>()) {
    // Concrete Shader object(e.g. UsdUVTexture) is stored in .data.
    if (const T *s = ps->value.as<T>()) {
      std::cout << "Path : <" << prim_abs_path << "> is "
                << tinyusdz::value::TypeTraits<T>::type_name() << ".\n";
      itemmap[prim_abs_path] = s;
    }
  }

  for (const auto &child : prim.children()) {
    if (!TraverseShaderRec(prim_abs_path, child, depth + 1, itemmap)) {
      return false;
    }
  }

  return true;
}

static void TraverseMaterial(const tinyusdz::Stage &stage, MaterialMap &m) {
  for (const auto &prim : stage.GetRootPrims()) {
    TraverseRec(/* root */ "", prim, 0, m);
  }
}

static void TraversePreviewSurface(const tinyusdz::Stage &stage,
                                   PreviewSurfaceMap &m) {
  for (const auto &prim : stage.GetRootPrims()) {
    TraverseShaderRec(/* root */ "", prim, 0, m);
  }
}

static void TraverseUVTexture(const tinyusdz::Stage &stage, UVTextureMap &m) {
  for (const auto &prim : stage.GetRootPrims()) {
    TraverseShaderRec(/* root */ "", prim, 0, m);
  }
}

static void TraversePrimvarReader_float2(const tinyusdz::Stage &stage,
                                         PrimvarReader_float2Map &m) {
  for (const auto &prim : stage.GetRootPrims()) {
    TraverseShaderRec(/* root */ "", prim, 0, m);
  }
}
#endif

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Need USD file with Material/Shader(e.g. `<tinyusdz>/models/texturescube.usda`\n" << std::endl;
    return EXIT_FAILURE;
  }

  std::string filepath = argv[1];
  std::string warn;
  std::string err;

  std::string ext = str_tolower(GetFileExtension(filepath));

  tinyusdz::Stage stage;

  if (ext.compare("usdc") == 0) {
    bool ret = tinyusdz::LoadUSDCFromFile(filepath, &stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
      // return EXIT_FAILURE;
    }

    if (!ret) {
      std::cerr << "Failed to load USDC file: " << filepath << "\n";
      return EXIT_FAILURE;
    }
  } else if (ext.compare("usda") == 0) {
    bool ret = tinyusdz::LoadUSDAFromFile(filepath, &stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
      // return EXIT_FAILURE;
    }

    if (!ret) {
      std::cerr << "Failed to load USDA file: " << filepath << "\n";
      return EXIT_FAILURE;
    }
  } else if (ext.compare("usdz") == 0) {
    // std::cout << "usdz\n";
    bool ret = tinyusdz::LoadUSDZFromFile(filepath, &stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
      // return EXIT_FAILURE;
    }

    if (!ret) {
      std::cerr << "Failed to load USDZ file: " << filepath << "\n";
      return EXIT_FAILURE;
    }

  } else {
    // try to auto detect format.
    bool ret = tinyusdz::LoadUSDFromFile(filepath, &stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
      // return EXIT_FAILURE;
    }

    if (!ret) {
      std::cerr << "Failed to load USD file: " << filepath << "\n";
      return EXIT_FAILURE;
    }
  }

  std::string s = stage.ExportToString();
  std::cout << s << "\n";
  std::cout << "--------------------------------------"
            << "\n";

  // Visit all Prims in the Stage.
  auto prim_visit_fun = [](const tinyusdz::Path &abs_path, const tinyusdz::Prim &prim, const int32_t level, void *userdata, std::string *err) -> bool {
    (void)err;
    std::cout << tinyusdz::pprint::Indent(level) << "[" << level << "] (" << prim.data().type_name() << ") " << prim.local_path().prim_part() << " : AbsPath " << tinyusdz::to_string(abs_path) << "\n";

    // Use as() or is() for Prim specific processing.
    if (const tinyusdz::Material *pm = prim.as<tinyusdz::Material>()) {
      (void)pm;
      std::cout << tinyusdz::pprint::Indent(level) << "  Got Material!\n";
      // return false + `err` empty if you want to terminate traversal earlier.
      //return false;
    }

    return true; 
  };

  void *userdata = nullptr;

  tinyusdz::tydra::VisitPrims(stage, prim_visit_fun, userdata);


  std::cout << "--------------------------------------"
            << "\n";

  // Compute Xform of each Prim at time t.
  {
    tinyusdz::tydra::XformNode xformnode;
    double t = tinyusdz::value::TimeCode::Default();
    tinyusdz::value::TimeSampleInterpolationType tinterp = tinyusdz::value::TimeSampleInterpolationType::Held; // Held or Linear

    if (!tinyusdz::tydra::BuildXformNodeFromStage(stage, &xformnode, t, tinterp)) {
      std::cerr << "BuildXformNodeFromStage error.\n";
    } else {
      std::cout << tinyusdz::tydra::DumpXformNode(xformnode) << "\n";
    }
  }


  // Mapping hold the pointer to concrete Prim object,
  // So stage content should not be changed(no Prim addition/deletion).
  XformMap xformmap;
  MeshMap meshmap;
  MaterialMap matmap;
  PreviewSurfaceMap surfacemap;
  UVTextureMap texmap;
  PrimvarReader_float2Map preadermap;

  // Collect and make full path <-> Prim/Shader mapping
  tinyusdz::tydra::ListPrims(stage, xformmap);
  tinyusdz::tydra::ListPrims(stage, meshmap);
  tinyusdz::tydra::ListPrims(stage, matmap);
  tinyusdz::tydra::ListShaders(stage, surfacemap);
  tinyusdz::tydra::ListShaders(stage, texmap);
  tinyusdz::tydra::ListShaders(stage, preadermap);

  //
  // Query example
  //
  for (const auto &item : matmap) {
    nonstd::expected<const tinyusdz::Prim*, std::string> mat = stage.GetPrimAtPath(tinyusdz::Path(item.first, /* prop name */""));
    if (mat) {
      std::cout << "Found Material <" << item.first << "> from Stage:\n";
      if (const tinyusdz::Material *mp = mat.value()->as<tinyusdz::Material>()) { // this should be true though.
        std::cout << tinyusdz::to_string(*mp) << "\n";
      }
    } else {
      std::cerr << "Err: " << mat.error() << "\n";
    }
  }

  for (const auto &item : surfacemap) {
    // Returned Prim is Shader class
    nonstd::expected<const tinyusdz::Prim*, std::string> shader = stage.GetPrimAtPath(tinyusdz::Path(item.first, /* prop name */""));
    if (shader) {
      std::cout << "Found Shader(UsdPreviewSurface) <" << item.first << "> from Stage:\n";

      const tinyusdz::Shader *sp = shader.value()->as<tinyusdz::Shader>();
      if (sp) { // this should be true though.

        if (const tinyusdz::UsdPreviewSurface *surf = sp->value.as<tinyusdz::UsdPreviewSurface>()) {
          // Print Shader
          std::cout << tinyusdz::to_string(*sp) << "\n";
        }
      }

    } else {
      std::cerr << "Err: " << shader.error() << "\n";
    }
  }

  for (const auto &item : texmap) {
    // Returned Prim is Shader class
    nonstd::expected<const tinyusdz::Prim*, std::string> shader = stage.GetPrimAtPath(tinyusdz::Path(item.first, /* prop name */""));
    if (shader) {
      std::cout << "Found Shader(UsdUVTexture) <" << item.first << "> from Stage:\n";

      const tinyusdz::Shader *sp = shader.value()->as<tinyusdz::Shader>();
      if (sp) { // this should be true though.
        if (const tinyusdz::UsdUVTexture *tex = sp->value.as<tinyusdz::UsdUVTexture>()) {
          std::cout << tinyusdz::to_string(*sp);
        }
      }

    } else {
      std::cerr << "Err: " << shader.error() << "\n";
    }
  }

  for (const auto &item : preadermap) {
    // Returned Prim is Shader class
    nonstd::expected<const tinyusdz::Prim*, std::string> shader = stage.GetPrimAtPath(tinyusdz::Path(item.first, /* prop name */""));
    if (shader) {
      std::cout << "Found Shader(UsdPrimvarReader_float2) <" << item.first << "> from Stage:\n";

      const tinyusdz::Shader *sp = shader.value()->as<tinyusdz::Shader>();
      if (sp) { // this should be true though.

        if (const tinyusdz::UsdPrimvarReader_float2 *preader = sp->value.as<tinyusdz::UsdPrimvarReader_float2>()) {
          std::cout << tinyusdz::to_string(*sp) << "\n";
        }
      }

    } else {
      std::cerr << "Err: " << shader.error() << "\n";
    }
  }

  //
  // -- Querying Parent Prim
  //
  for (const auto &item : surfacemap) {
    // Usually Parent is Material
    std::string err;
    if (const tinyusdz::Prim *p = tinyusdz::tydra::GetParentPrim(stage, tinyusdz::Path(item.first, /* property */""), &err)) {
      std::cout << "Input path = " << tinyusdz::to_string(item.first) << "\n";
      std::cout << "Parent prim = " << tinyusdz::prim::print_prim(*p) << "\n";
    } else {
      std::cerr << err;
    }
  }

  std::cout << "GetProperty example -------------\n";
  for (const auto &item : xformmap) {
    std::string err;
    tinyusdz::Property prop;
    if (tinyusdz::tydra::GetProperty(*item.second, "xformOp:transform", &prop, &err)) {
      std::cout << "Property value = " << print_prop(prop, "xformOp:transform", 0) << "\n";
    } else {
      std::cerr << err;
    }
  }


  //
  // Find bound Material
  //
  std::cout << "FindBoundMaterial example -------------\n";
  for (const auto &item : meshmap) {
    // FindBoundMaterial seaches bound material for parent GPrim.
    tinyusdz::Path matPath;
    const tinyusdz::Material *material{nullptr};
    std::string err;
    bool ret = tinyusdz::tydra::FindBoundMaterial(stage, tinyusdz::Path(item.first, ""), /* suffix */"", &matPath, &material, &err);

    if (ret) {
      std::cout << item.first << " has bound Material. Material Path = " << matPath << "\n";
      std::cout << "mat = " << material << "\n";
      if (material) {
        std::cout << to_string(*material, /* indent */1) << "\n";
      }
    } else {
      std::cout << "Bound material not found for Prim path : " << item.first << "\n";
    }
  }



  //
  // Shader attribute evaluation example.
  //
  std::cout << "EvaluateAttribute example -------------\n";
  for (const auto &item : preadermap) {
    // Returned Prim is Shader class
    nonstd::expected<const tinyusdz::Prim*, std::string> shader = stage.GetPrimAtPath(tinyusdz::Path(item.first, /* prop name */""));
    if (shader) {
      std::cout << "Shader(UsdPrimvarReader_float2) <" << item.first << "> from Stage:\n";

      const tinyusdz::Shader *sp = shader.value()->as<tinyusdz::Shader>();
      if (sp) { // this should be true though.

        if (const tinyusdz::UsdPrimvarReader_float2 *preader = sp->value.as<tinyusdz::UsdPrimvarReader_float2>()) {

          tinyusdz::tydra::TerminalAttributeValue tav;
          std::string err;
          bool ret = tinyusdz::tydra::EvaluateAttribute(stage, *shader.value(), "inputs:varname", &tav, &err);

          if (!ret) {
            std::cout << "Resolving `inputs:varname` failed: " << err << "\n";
          }

          std::cout << "type = " << tav.type_name() << "\n";

          if (auto pv = tav.as<tinyusdz::value::token>()) {
            std::cout << "inputs:varname = " << (*pv) << "\n";
          }
        }
      }

    } else {
      std::cerr << "Err: " << shader.error() << "\n";
    }
  }



  return 0;
}
