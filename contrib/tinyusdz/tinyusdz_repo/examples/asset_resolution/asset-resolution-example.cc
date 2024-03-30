// All-in-one TinyUSDZ core
#include "tinyusdz.hh"

// Import to_string() and operator<< features
#include <iostream>

#include "pprinter.hh"
#include "prim-pprint.hh"
#include "value-pprint.hh"

// Tydra is a collection of APIs to access/convert USD Prim data
// (e.g. Can get Attribute by name)
// See <tinyusdz>/examples/tydra_api for more Tydra API examples.
#include "tydra/scene-access.hh"

std::map<std::string, std::string> g_map;

//
// Read asset(USD) through AssetResolutionResolver(something like a custom
// filesystem handler)
//
// - AssetResolution handler(for AssetResolution::open_asset)
//   - resolve
//   - size
//   - read
//

static int MyARResolve(const char *asset_name,
                       const std::vector<std::string> &search_paths,
                       std::string *resolved_asset_name, std::string *err,
                       void *userdata) {
  (void)err;
  (void)userdata;
  (void)search_paths;

  if (!asset_name) {
    return -2;  // err
  }

  if (!resolved_asset_name) {
    return -2;  // err
  }

  if (g_map.count(asset_name)) {
    (*resolved_asset_name) = asset_name;
    return 0;  // OK
  }

  return -1;  // failed to resolve.
}

// AssetResoltion handlers
static int MyARSize(const char *asset_name, uint64_t *nbytes, std::string *err,
                    void *userdata) {
  (void)userdata;

  if (!asset_name) {
    if (err) {
      (*err) += "asset_name arg is nullptr.\n";
    }
    return -1;
  }

  if (!nbytes) {
    if (err) {
      (*err) += "nbytes arg is nullptr.\n";
    }
    return -1;
  }

  if (g_map.count(asset_name)) {
    // Use strlen()(length until the first appearance of '\0' character), since
    // std::string::size() reports buffer size, not bytes until the first
    // appearance of '\0' character.
    (*nbytes) = strlen(g_map[asset_name].c_str());
    return 0;  // OK
  }

  return -1;
}

int MyARRead(const char *asset_name, uint64_t req_nbytes, uint8_t *out_buf,
             uint64_t *nbytes, std::string *err, void *userdata) {
  if (!asset_name) {
    if (err) {
      (*err) += "asset_name arg is nullptr.\n";
    }
    return -3;
  }

  if (!nbytes) {
    if (err) {
      (*err) += "nbytes arg is nullptr.\n";
    }
    return -3;
  }

  if (req_nbytes < 9) {  // at least 9 bytes(strlen("#usda 1.0")) or more
    return -2;
  }

  if (g_map.count(asset_name)) {
    size_t sz = strlen(g_map[asset_name].c_str());
    if (sz > req_nbytes) {
      if (err) {
        (*err) += "Insufficient dst buffer size.\n";
      }
      return -4;
    }

    std::cout << "read asset: " << asset_name << "\n";

    memcpy(out_buf, &g_map[asset_name][0], sz);

    (*nbytes) = sz;
    return 0;
  }

  //
  return -1;
}

int main(int argc, char **argv) {
  g_map["bora.usda"] = R"(#usda 1.0

def "bora" {
  float myval = 3.1
}
)";

  g_map["dora.usda"] = R"(#usda 1.0
def "dora" {
  float myval = 5.1
}
)";

  std::string input_usd_name = "bora.usda";
  if (argc > 1) {
    input_usd_name = argv[1];
  }

  std::string warn, err;

  tinyusdz::AssetResolutionResolver resolver;
  // Register filesystem handler for `.my` asset.
  tinyusdz::AssetResolutionHandler ar_handler;
  ar_handler.resolve_fun = MyARResolve;
  ar_handler.size_fun = MyARSize;
  ar_handler.read_fun = MyARRead;
  ar_handler.write_fun = nullptr;  // not used in this example.
  ar_handler.userdata = nullptr;   // not used in this example;
  resolver.register_asset_resolution_handler("usda", ar_handler);

  tinyusdz::Layer layer;
  bool ret = tinyusdz::LoadLayerFromAsset(resolver, input_usd_name, &layer,
                                          &warn, &err);

  if (warn.size()) {
    std::cout << "WARN:" << warn << "\n";
  }

  if (!ret) {
    std::cerr << "Failed to load asset: " + input_usd_name + "\n";
    if (err.size()) {
      std::cerr << "  " + err + "\n";
    }

    return -1;
  }

  // Print USD scene as Ascii.
  std::cout << layer << "\n";

  return EXIT_SUCCESS;
}
