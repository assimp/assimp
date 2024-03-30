#include <nanobind/nanobind.h>

#include "prim-types.hh"
#include "usda-reader.hh"
//#include "ascii-writer.hh"

#include "nonstd/optional.hpp"

namespace nb = nanobind;

using namespace nb::literals;  // to bring in the `_a` literal

static double test_api() {
  // TODO: Implement
  return 4.14;
}

// stub classes
struct Stage
{
  std::string filepath;

  static Stage Open(const std::string &_filepath) {
    // TODO
    Stage stage;
    stage.filepath = _filepath;

    return stage;
  }

  bool Export(const std::string &_filepath) {
    // TODO
    return false;
  }

  nonstd::optional<tinyusdz::GPrim> GetPrimAtPath(const std::string &_path) const {
    // TODO
    tinyusdz::GPrim prim;

    if (_path == "/bora") {
      return nonstd::nullopt;
    }

    return prim;

  }

  static tinyusdz::GPrim DefinePrim(const std::string &_path, const std::string &type) {
    tinyusdz::GPrim prim;

    if (type == "Xform") {
      // TODO:...
      prim.prim_type = "Xform";
      return prim;
    }

    return prim;
  }

};

NB_MODULE(pytinyusd, m) {

  m.def("test_api", &test_api, "Test API");

  auto UsdModule = m.def_submodule("Usd");

  nb::class_<Stage>(UsdModule, "Stage")
    .def(nb::init<>())
    .def_static("Open", &Stage::Open)
    .def("Export", &Stage::Export)
    .def("GetPrimAtPath", [](const Stage &s, const std::string &path) -> nb::object {
      if (auto p = s.GetPrimAtPath(path)) {
        return nb::cast(*p);
      }
      return nb::none();
    });
  ;

  nb::class_<tinyusdz::GPrim>(UsdModule, "GPrim");


  nb::class_<tinyusdz::GeomSphere>(m, "Sphere")
    .def(nb::init<>())
  ;

  //py::class_<tinyusdz::GeomSphere>(m, "Sphere")
  //  .def(py::init<>())
  //  .def
}
