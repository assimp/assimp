// All-in-one TinyUSDZ core
#include "tinyusdz.hh"

// Import to_string() and operator<< features
#include <iostream>

#include "pprinter.hh"
#include "prim-pprint.hh"
#include "value-pprint.hh"

//
// To read asset in custom format, this example provides
//
// - AssetResolution handler(for AssetResolution::open_asset)
//   - Optional. Create on-memory asset system
//   - You can use built-in file based AssetResolution handler if you provide
//   .my file
// - File format API(read/write data with custom format)
//   - Simple 4 byte binary storing a float value.
//

//
// def "muda" ( references = @bora.my@ ) {
// }
//
// =>
//
// def "muda" () {
//    uniform float myval = 3.14
// }
//

static std::map<std::string, float> g_map;

//
// Asset resolution handlers
//
static int MyARResolve(const char *asset_name,
                       const std::vector<std::string> &search_paths,
                       std::string *resolved_asset_name, std::string *err,
                       void *userdata) {
  (void)err;
  (void)userdata;
  (void)search_paths;

  std::cout << "Resolve " << asset_name << "\n";

  if (!asset_name) {
    return -2;  // err
  }

  if (!resolved_asset_name) {
    return -2;  // err
  }

  if (g_map.count(asset_name)) {
    (*resolved_asset_name) = asset_name;
    std::cout << "Resolved as " << resolved_asset_name << "\n";
    return 0;  // OK
  }
  std::cerr << "Can't resolve asset: " << asset_name << "\n";

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

  std::cout << "Asset size " << sizeof(float) << "\n";

  (*nbytes) = sizeof(float);
  return 0;  // OK
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

  if (req_nbytes < sizeof(float)) {
    return -2;
  }

  if (g_map.count(asset_name)) {
    float val = g_map[asset_name];
    memcpy(out_buf, &val, sizeof(float));

    (*nbytes) = sizeof(float);
    return 0;
  }

  //
  return -1;
}

//
// custom File-format handlers
//
static bool MyCheck(const tinyusdz::Asset &asset, std::string *warn,
                    std::string *err, void *user_data) {
  if (asset.size() != 4) {
    return false;
  }

  return true;
}

static bool MyRead(const tinyusdz::Asset &asset,
                   tinyusdz::PrimSpec &ps /* inout */, std::string *warn,
                   std::string *err, void *user_data) {
  //
  // `tinyusdz::Asset` is a simple buffer: data() : bytes buffer, size() :
  // buffer size
  //

  if (asset.size() != 4) {
    return false;
  }

  //
  // Create a PrimSpec:
  //
  // def "my01" {
  //   uniform float myval = ...
  // }
  //

  float val;
  memcpy(&val, asset.data(), 4);

  tinyusdz::Attribute attr;
  attr.set_value(val);
  attr.set_name("myval");
  attr.variability() = tinyusdz::Variability::Uniform;

  ps.props()["myval"] = tinyusdz::Property(attr, /* custom */ false);

  ps.name() = "my01";  // must set PrimSpec name

  return true;
}

static bool MyWrite(const tinyusdz::PrimSpec &ps, tinyusdz::Asset *asset_out,
                    std::string *warn, std::string *err, void *user_data) {
  // TOOD
  return false;
}

int main(int argc, char **argv) {
  g_map["bora.my"] = 3.14f;
  g_map["dora.my"] = 6.14f;

  tinyusdz::FileFormatHandler my_handler;
  my_handler.extension = "my";
  my_handler.description = "Custom fileformat example.";
  my_handler.checker = MyCheck;
  my_handler.reader = MyRead;
  my_handler.writer = MyWrite;
  my_handler.userdata = nullptr;

  tinyusdz::Stage stage;  // empty scene

  // path to <tinyusdz>/data/fileformat_my.usda
  std::string input_usd_filepath = "../data/fileformat_my.usda";
  if (argc > 1) {
    input_usd_filepath = argv[1];
  }

  std::string warn, err;

  tinyusdz::Layer layer;
  bool ret =
      tinyusdz::LoadLayerFromFile(input_usd_filepath, &layer, &warn, &err);

  if (warn.size()) {
    std::cout << "WARN: " << warn << "\n";
  }

  if (!ret) {
    std::cerr << err << "\n";
    exit(-1);
  }

  tinyusdz::AssetResolutionResolver resolver;
  // Register on-memory filesystem handler for `.my` asset.
  tinyusdz::AssetResolutionHandler ar_handler;
  ar_handler.resolve_fun = MyARResolve;
  ar_handler.size_fun = MyARSize;
  ar_handler.read_fun = MyARRead;
  ar_handler.write_fun = nullptr;  // not used in this example.
  ar_handler.userdata = nullptr;   // not used in this example;
  resolver.register_asset_resolution_handler("my", ar_handler);

  tinyusdz::ReferencesCompositionOptions options;
  options.fileformats["my"] = my_handler;

  // Do `references` composition to materialize `references = @***.my@`
  tinyusdz::Layer composited_layer;
  if (!tinyusdz::CompositeReferences(resolver, layer, &composited_layer, &warn,
                                     &err, options)) {
    std::cerr << "Failed to composite `references`: " << err << "\n";
    return -1;
  }

  // Print USD scene as Ascii.
  std::cout << composited_layer << "\n";

  return EXIT_SUCCESS;
}
