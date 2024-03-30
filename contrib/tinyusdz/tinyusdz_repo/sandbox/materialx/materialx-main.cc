#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "pugixml.hpp"
#include "nlohmann/json.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

int main(int argc, char **argv)
{
  std::string filename = "../../data/materialx/UsdPreviewSurface/usd_preview_surface_default.mtlx";

  if (argc > 1) {
    filename = argv[1];
  }

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_file(filename.c_str());
  if (!result) {
    if (result.description()) {
      std::cerr << "XML Parising error: " << result.description() << "\n";
      std::cerr << "  offset: " << result.offset << "\n";
    }
    return -1;
  }
 
  std::cout << "Read OK\n";

  pugi::xml_node mtlx = doc.child("materialx");
  if (!mtlx) {
    std::cerr << "<materialx> node not found.\n";
    return -1;
  }

  pugi::xml_attribute ver_attr = mtlx.attribute("version");
  if (!ver_attr) {
    std::cerr << "version attribute not found in <materialx>.\n";
    return -1;
  }
  std::cout << "version = " << ver_attr.as_string() << "\n";

  pugi::xml_attribute cspace_attr = mtlx.attribute("colorspace");
  if (!cspace_attr) {
    std::cerr << "colorspace attribute not found in <materialx>.\n";
    return -1;
  }
  std::cout << "colorspace = " << cspace_attr.as_string() << "\n";

  for (auto usdp : mtlx.child("UsdPreviewSurface")) {
    std::cout << "UsdPreviewSurface: " << usdp.name() << "\n";

  }

  // nodegraph
  for (auto ng : mtlx.children("nodegraph")) {
    std::cout << "nodegraph: " << ng.name() << "\n";
    for (auto ti : ng.children("tiledimage")) {
      std::cout << "tiledimage: " << ti.attribute("name").as_string() << "\n";
    }
  }


  // surfacematerial
  for (auto sm : mtlx.children("surfacematerial")) {
    std::cout << "surfacematerial: " << sm.name() << "\n";
    for (auto inp : sm.children("input")) {
      std::cout << "input: " << inp.attribute("name").as_string() << "\n";
      std::cout << "  " << inp.attribute("type").as_string() << "\n";
      std::cout << "  " << inp.attribute("nodename").as_string() << "\n";
    }
  }
  return 0;
}
