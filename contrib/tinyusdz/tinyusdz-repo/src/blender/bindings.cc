#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace pybind11::literals;  // to bring in the `_a` literal

static double test_api() {
  // TODO: Implement
  return 4.14;
}

PYBIND11_MODULE(tinyusd_blender, m) {
  m.doc() = "TinyUSD Python binding for Blender";

  m.def("test_api", &test_api, "Test API");
}
