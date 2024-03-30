#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "tinyusdz.hh"
#include "pprinter.hh"
#include "str-util.hh"
#include "io-util.hh"

struct CompositionFeatures {
  bool subLayers{true};
  bool inherits{true};
  bool variantSets{true};
  bool references{true};
  bool payload{true}; // Not 'payloads'
  bool specializes{true};
};

static std::string GetFileExtension(const std::string &filename) {
  if (filename.find_last_of('.') != std::string::npos)
    return filename.substr(filename.find_last_of('.') + 1);
  return "";
}

static std::string str_tolower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); }
  );
  return s;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Usage tusdcat [--flatten] [--composition=STRLIST] [--relative] [--extract-variants] input.usda/usdc/usdz\n";
    std::cout << "\n --flatten (not fully implemented yet) Do composition(load sublayers, refences, payload, evaluate `over`, inherit, variants..)";
    std::cout << "  --composition: Specify which composition feature to be "
                 "enabled(valid when `--flatten` is supplied). Comma separated "
                 "list. \n    l "
                 "`subLayers`, i `inherits`, v `variantSets`, r `references`, "
                 "p `payload`, s `specializes`. \n    Example: "
                 "--composition=r,p --composition=references,subLayers\n";
    std::cout << "\n --extract-variants (w.i.p) Dump variants information to .json\n";
    std::cout << "\n --relative (not implemented yet) Print Path as relative Path\n";
    return EXIT_FAILURE;
  }

  bool has_flatten{false};
  bool has_relative{false};
  bool has_extract_variants{false};

  constexpr int kMaxIteration = 128;

  std::string filepath;

  int input_index = -1;
  CompositionFeatures comp_features;

  for (size_t i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg.compare("--flatten") == 0) {
      has_flatten = true;
    } else if (arg.compare("--relative") == 0) {
      has_relative = true;
    } else if (arg.compare("--extract-variants") == 0) {
      has_extract_variants = true;
    } else if (tinyusdz::startsWith(arg, "--composition=")) {
      std::string value_str = tinyusdz::removePrefix(arg, "--composition=");
      if (value_str.empty()) {
        std::cerr << "No values specified to --composition.\n";
        exit(-1);
      }

      std::vector<std::string> items = tinyusdz::split(value_str, ",");
      comp_features.subLayers = false;
      comp_features.inherits = false;
      comp_features.variantSets = false;
      comp_features.references = false;
      comp_features.payload = false;
      comp_features.specializes = false;

      for (const auto &item : items) {
        if ((item == "l") || (item == "subLayers")) {
          comp_features.subLayers = true;
        } else if ((item == "i") || (item == "inherits")) {
          comp_features.inherits = true;
        } else if ((item == "v") || (item == "variantSets")) {
          comp_features.variantSets = true;
        } else if ((item == "r") || (item == "references")) {
          comp_features.references = true;
        } else if ((item == "p") || (item == "payload")) {
          comp_features.payload = true;
        } else if ((item == "s") || (item == "specializes")) {
          comp_features.specializes = true;
        } else {
          std::cerr << "Invalid string for --composition : " << item << "\n";
          exit(-1);
        }
      }

    } else {
      filepath = arg;
      input_index = i;
    }
  }

  if (filepath.empty() || (input_index < 0)) {
    std::cout << "Input USD filename missing.\n";
    return EXIT_FAILURE;
  }

  std::string warn;
  std::string err;

  std::string ext = str_tolower(GetFileExtension(filepath));
  std::string base_dir;
  base_dir = tinyusdz::io::GetBaseDir(filepath);

  if (has_flatten) {

    // TODO: flatten for USDZ
    if (tinyusdz::IsUSDZ(filepath)) {

      std::cout << "--flatten is ignored for USDZ at the moment.\n";

      tinyusdz::Stage stage;

      bool ret = tinyusdz::LoadUSDZFromFile(filepath, &stage, &warn, &err);
      if (!warn.empty()) {
        std::cerr << "WARN : " << warn << "\n";
      }
      if (!err.empty()) {
        std::cerr << "ERR : " << err << "\n";
        //return EXIT_FAILURE;
      }

      if (!ret) {
        std::cerr << "Failed to load USDZ file: " << filepath << "\n";
        return EXIT_FAILURE;
      }

      std::cout << to_string(stage) << "\n";

      return EXIT_SUCCESS;
    }

    tinyusdz::Layer root_layer;
    bool ret = tinyusdz::LoadLayerFromFile(filepath, &root_layer, &warn, &err);
    if (warn.size()) {
      std::cout << "WARN: " << warn << "\n";
    }

    if (!ret) {
      std::cerr << "Failed to read USD data as Layer: \n";
      std::cerr << err << "\n";
      return -1;
    }

    std::cout << "# input\n";
    std::cout << root_layer << "\n";

    tinyusdz::Stage stage;
    stage.metas() = root_layer.metas();

    std::string warn;

    tinyusdz::AssetResolutionResolver resolver;
    resolver.set_current_working_path(base_dir);
    resolver.set_search_paths({base_dir});

    //
    // LIVRPS strength ordering
    // - [x] Local(subLayers)
    // - [x] Inherits
    // - [x] VariantSets
    // - [x] References
    // - [x] Payload
    // - [ ] Specializes
    //

    tinyusdz::Layer src_layer = root_layer;
    if (comp_features.subLayers) {
      tinyusdz::Layer composited_layer;
      if (!tinyusdz::CompositeSublayers(resolver, src_layer, &composited_layer, &warn, &err)) {
        std::cerr << "Failed to composite subLayers: " << err << "\n";
        return -1;
      }

      if (warn.size()) {
        std::cout << "WARN: " << warn << "\n";
      }

      std::cout << "# `subLayers` composited\n";
      std::cout << composited_layer << "\n";

      src_layer = std::move(composited_layer);
    }

    // TODO: Find more better way to Recursively resolve references/payload/variants
    for (int i = 0; i < kMaxIteration; i++) {

      bool has_unresolved = false;

      if (comp_features.references) {
        if (!src_layer.check_unresolved_references()) {
          std::cout << "# iter " << i << ": no unresolved references.\n";
        } else {
          has_unresolved = true;

          tinyusdz::Layer composited_layer;
          if (!tinyusdz::CompositeReferences(resolver, src_layer, &composited_layer, &warn, &err)) {
            std::cerr << "Failed to composite `references`: " << err << "\n";
            return -1;
          }

          if (warn.size()) {
            std::cout << "WARN: " << warn << "\n";
          }

          std::cout << "# `references` composited\n";
          std::cout << composited_layer << "\n";

          src_layer = std::move(composited_layer);
        }
      }

      if (comp_features.payload) {
        if (!src_layer.check_unresolved_payload()) {
          std::cout << "# iter " << i << ": no unresolved payload.\n";
        } else {
          has_unresolved = true;

          tinyusdz::Layer composited_layer;
          if (!tinyusdz::CompositePayload(resolver, src_layer, &composited_layer, &warn, &err)) {
            std::cerr << "Failed to composite `payload`: " << err << "\n";
            return -1;
          }

          if (warn.size()) {
            std::cout << "WARN: " << warn << "\n";
          }

          std::cout << "# `payload` composited\n";
          std::cout << composited_layer << "\n";

          src_layer = std::move(composited_layer);
        }
      }

      if (comp_features.inherits) {
        if (!src_layer.check_unresolved_inherits()) {
          std::cout << "# iter " << i << ": no unresolved inherits.\n";
        } else {
          has_unresolved = true;

          tinyusdz::Layer composited_layer;
          if (!tinyusdz::CompositeInherits(src_layer, &composited_layer, &warn, &err)) {
            std::cerr << "Failed to composite `inherits`: " << err << "\n";
            return -1;
          }

          if (warn.size()) {
            std::cout << "WARN: " << warn << "\n";
          }

          std::cout << "# `inherits` composited\n";
          std::cout << composited_layer << "\n";

          src_layer = std::move(composited_layer);
        }
      }

      if (comp_features.variantSets) {
        if (!src_layer.check_unresolved_variant()) {
          std::cout << "# iter " << i << ": no unresolved variant.\n";
        } else {
          has_unresolved = true;

          tinyusdz::Layer composited_layer;
          if (!tinyusdz::CompositeVariant(src_layer, &composited_layer, &warn, &err)) {
            std::cerr << "Failed to composite `variantSet`: " << err << "\n";
            return -1;
          }

          if (warn.size()) {
            std::cout << "WARN: " << warn << "\n";
          }

          std::cout << "# `variantSet` composited\n";
          std::cout << composited_layer << "\n";

          src_layer = std::move(composited_layer);
        }
      }

      // TODO
      // - [ ] specializes
      // - [ ] `class` Prim?

      std::cout << "# has_unresolved_references: " << src_layer.check_unresolved_references() << "\n";
      std::cout << "# all resolved? " << !has_unresolved << "\n";

      if (!has_unresolved) {
        std::cout << "# of composition iteration to resolve fully: " << (i + 1) << "\n";
        break;
      }
    }

    if (has_extract_variants) {
      tinyusdz::Dictionary dict;
      if (!tinyusdz::ExtractVariants(src_layer, &dict, &err)) {
        std::cerr << "Failed to extract variants info: " << err;
      } else {
        std::cout << "== Variants info ==\n" << tinyusdz::to_string(dict) << "\n";
      }

    }

  } else {

    tinyusdz::Stage stage;

    if (ext.compare("usdc") == 0) {
      tinyusdz::USDLoadOptions options;
      options.do_composition = has_flatten;

      bool ret = tinyusdz::LoadUSDCFromFile(filepath, &stage, &warn, &err, options);
      if (!warn.empty()) {
        std::cerr << "WARN : " << warn << "\n";
      }
      if (!err.empty()) {
        std::cerr << "ERR : " << err << "\n";
        //return EXIT_FAILURE;
      }

      if (!ret) {
        std::cerr << "Failed to load USDC file: " << filepath << "\n";
        return EXIT_FAILURE;
      }
    } else if (ext.compare("usda") == 0) {
      tinyusdz::USDLoadOptions options;
      options.do_composition = has_flatten;

      bool ret = tinyusdz::LoadUSDAFromFile(filepath, &stage, &warn, &err, options);
      if (!warn.empty()) {
        std::cerr << "WARN : " << warn << "\n";
      }
      if (!err.empty()) {
        std::cerr << "ERR : " << err << "\n";
        //return EXIT_FAILURE;
      }

      if (!ret) {
        std::cerr << "Failed to load USDA file: " << filepath << "\n";
        return EXIT_FAILURE;
      }
    } else if (ext.compare("usdz") == 0) {
      if (has_flatten) {
        std::cout << "--flatten is ignored for USDZ model at the moment.\n";
      }
      //std::cout << "usdz\n";
      bool ret = tinyusdz::LoadUSDZFromFile(filepath, &stage, &warn, &err);
      if (!warn.empty()) {
        std::cerr << "WARN : " << warn << "\n";
      }
      if (!err.empty()) {
        std::cerr << "ERR : " << err << "\n";
        //return EXIT_FAILURE;
      }

      if (!ret) {
        std::cerr << "Failed to load USDZ file: " << filepath << "\n";
        return EXIT_FAILURE;
      }
    } else {
      tinyusdz::USDLoadOptions options;
      options.do_composition = has_flatten;

      // try to auto detect format.
      bool ret = tinyusdz::LoadUSDFromFile(filepath, &stage, &warn, &err, options);
      if (!warn.empty()) {
        std::cerr << "WARN : " << warn << "\n";
      }
      if (!err.empty()) {
        std::cerr << "ERR : " << err << "\n";
        //return EXIT_FAILURE;
      }

      if (!ret) {
        std::cerr << "Failed to load USD file: " << filepath << "\n";
        return EXIT_FAILURE;
      }
    }

    std::string s = stage.ExportToString(has_relative);
    std::cout << s << "\n";

    if (has_extract_variants) {
      tinyusdz::Dictionary dict;
      if (!tinyusdz::ExtractVariants(stage, &dict, &err)) {
        std::cerr << "Failed to extract variants info: " << err;
      } else {
        std::cout << "== Variants info ==\n" << tinyusdz::to_string(dict) << "\n";
      }

    }
  }

  return EXIT_SUCCESS;
}
