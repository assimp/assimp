#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/numpy.h>

#include "tiny-format.hh"
#include "tinyusdz.hh"
#include "prim-pprint.hh"
#include "tydra/render-data.hh"
//
#include "value-type-macros.inc"

//
// NOTE:
// - pybind11 does not provide binding for `array.array` module(`py::array` is for numpy type)
//   - so implement dedicated binding for array data through `vector<T>` stl binding.
//   - Converting `numpy`, `array.array` and other Python array/list types must be converted at Python layer, not here(C++ binding).
// - Memory management: TinyUSDZ does not use smart pointer, so use `return_value_policy::reference` or `return_value_policy::reference_internal` as much as posssible.
//   - For methods returning a const pointer(doe not dynamically allocate memory)(e.g. `Stage::GetPrimAtPath`) 
// - Use return_value_policy::reference_internal for a method which returns const/nonconst lvalue reference
//   - e.g. `const StageMeta &Stage::metas() const`, `StageMeta &Stage::metas()`
//   - Use py::def  or def_property with C++ lambdas(since def_readwrite cannot specity C++ method(there may be a solution, but could'nt find example code and hard to understand pybind11 templates))
// -

namespace py = pybind11;


#if 0

#define MAKE_OPAQUE_ARRAY_TYPE(__ty) \
  PYBIND11_MAKE_OPAQUE(std::vector<__ty>);

using namespace tinyusdz;

APPLY_FUNC_TO_NUMERIC_VALUE_TYPES(MAKE_OPAQUE_ARRAY_TYPE)

#undef MAKE_OPAQUE_ARRAY_TYPE

#endif

//PYBIND11_MAKE_OPAQUE(std::vector<int>);
PYBIND11_MAKE_OPAQUE(std::vector<tinyusdz::Prim>);

// define custom types
struct float16 {
  uint16_t h;
};

namespace PYBIND11_NAMESPACE { namespace detail {
    template <> struct type_caster<float16> {
    public:
        /**
         * This macro establishes the name 'inty' in
         * function signatures and declares a local variable
         * 'value' of type inty
         */
        PYBIND11_TYPE_CASTER(float16, const_name("float16"));

        /**
         * Conversion part 1 (Python->C++): convert a PyObject into a inty
         * instance or return false upon failure. The second argument
         * indicates whether implicit conversions should be applied.
         */
        bool load(handle src, bool) {
            /* Extract PyObject from handle */
            PyObject *source = src.ptr();
            /* Try converting into a Python integer value */
            PyObject *tmp = PyNumber_Long(source);
            if (!tmp)
                return false;
            /* Now try to convert into a C++ half */
            value.h = uint16_t(PyLong_AsLong(tmp));
            Py_DECREF(tmp);
            /* Ensure return code was OK (to avoid out-of-range errors etc) */
            return true; // !PyErr_Occurred()
        }

        /**
         * Conversion part 2 (C++ -> Python): convert an inty instance into
         * a Python object. The second and third arguments are used to
         * indicate the return value policy and parent object (for
         * ``return_value_policy::reference_internal``) and are generally
         * ignored by implicit casters.
         */
        static handle cast(float16 src, return_value_policy /* policy */, handle /* parent */) {
            return PyLong_FromLong(src.h);
        }
    };
}} // namespace PYBIND11_NAMESPACE::detail


//PYBIND11_NUMPY_DTYPE(float16, h);

// using namespace py::literals;  // to bring in the `_a` literal

static double test_api() {
  return 4.14;
}

class PyTest {
 public:
  std::vector<int> intv;
  const std::vector<int> &intvfun() const {
    return intv;
  }
  std::vector<int> &intvfun() {
    return intv;
  }
};

namespace internal {

tinyusdz::Stage load_usd(const std::string &filename) {
  tinyusdz::Stage stage;

  if (!tinyusdz::IsUSD(filename)) {
    std::string s(tinyusdz::fmt::format("{} not found or not a USD file."));
    PyErr_SetString(PyExc_FileNotFoundError, s.c_str());
    throw py::error_already_set();
  }

  std::string warn;
  std::string err;
  bool ret = tinyusdz::LoadUSDFromFile(filename, &stage, &warn, &err);

  if (warn.size()) {
    py::print("[ctinyusdz::load_usd] ", warn);
  }

  if (!ret) {
    std::string msg = "Failed to load USD";
    if (err.size()) {
      msg += ": " + err;
    }

    PyErr_SetString(PyExc_FileNotFoundError, filename.c_str());
    throw py::error_already_set();
  }

  return stage;
}

bool is_usd(const std::string &filename) { return tinyusdz::IsUSD(filename); }

std::string detect_usd_format(const std::string &filename) {
  std::string format;

  if (tinyusdz::IsUSD(filename, &format)) {
    return format;
  }
  return format;  // empty
}

}  // namespace internal

PYBIND11_MODULE(ctinyusdz, m) {
  using namespace tinyusdz;

  m.doc() = "Python binding for TinyUSDZ.";

  m.def("test_api", &test_api, "Test API");

  // auto UsdModule = m.def_submodule("Usd");

  py::class_<USDLoadOptions>(m, "USDLoadOptions")
      .def(py::init<>())
      .def_readwrite("num_threads", &USDLoadOptions::num_threads)
      .def_readwrite("load_assets", &USDLoadOptions::load_assets)
      .def_readwrite("max_memory_limit_in_mb",
                     &USDLoadOptions::max_memory_limit_in_mb)
      .def_readwrite("do_composition", &USDLoadOptions::do_composition);

  m.def(
      "format", &internal::detect_usd_format,
      "Detect USD format(USDA/USDC/USDZ) of file. Returns `\"usda\"`, "
      "`\"usdc\"` `\"usdz\"` or empty string(when a file is not a USD file).");
  m.def("is_usd", &internal::is_usd, "Load USD/USDA/USDC/USDZ from a file.");
  m.def("load_usd", &internal::load_usd,
        "Load USD/USDA/USDC/USDZ from a file.");

  py::class_<PyTest>(m, "PyTest")
      .def(py::init<>())
      .def_readwrite("intv", &PyTest::intv)
      .def_property(
          //"intv", static_cast<const std::vector<int> &(PyTest::*)(void) const>(&PyTest::intvfun)
          // In C++14, We can use py::overload_cast to simplify type cast
          "intv", py::overload_cast<>(&PyTest::intvfun, py::const_)
          , nullptr,
          py::return_value_policy::reference_internal)
      ;

    // TODO: Use attr?
#define SET_VALUE(__ty) \
    .def("set", [](primvar::PrimVar &p, const __ty &v) { p.set_value(v); })

  py::class_<primvar::PrimVar>(m, "PrimVar")
    .def(py::init<>())
    .def_property("dtype", &primvar::PrimVar::type_name, nullptr)
    SET_VALUE(int32_t)
    SET_VALUE(int64_t)
    SET_VALUE(uint32_t)
    SET_VALUE(uint64_t)
    SET_VALUE(double)
    SET_VALUE(float)
    .def("set_obj", [](primvar::PrimVar &p, const py::object &obj) {
      py::print("set_obj", obj);
      //py::buffer_info info = obj.request();
      py::print("buf info", obj.get_type());

      py::object c_float = py::module::import("ctypes").attr("c_float");
      py::print("c_float", c_float.get_type());
      py::print("isnstance(c_float)", py::isinstance(obj, c_float));
      //py::print("val = ", obj.cast<float>());
    
    })
    .def("set_buf", [](primvar::PrimVar &p, const py::buffer &buf) {
      py::print("set_buf", buf);
      py::buffer_info info = buf.request();
      py::print("buf info", info.format);
    })
    .def("set_array", [](primvar::PrimVar &p, const py::array_t<int32_t> v) {
      py::print("set_arr int[]");
    })
    .def("get_array", [](primvar::PrimVar &p) -> py::array_t<float> {
      std::vector<float> v;

      auto result = py::array_t<float>(16);
      
      return result;
    })
    ;

  py::class_<Prim>(m, "Prim")
      // default ctor: Create Prim with Model type.
      .def(py::init([]() { return Prim(Model()); }))
      .def(py::init([](const std::string &prim_name) {
        return Prim(Model());
       }))
      .def_property(
          "prim_id", [](const Prim &p) -> int64_t { return p.prim_id(); },
          [](Prim &p) -> int64_t & { return p.prim_id(); })
      //.def_property("children", [](const Prim &p) -> const std::vector<Prim> &
      //{
      //  return p.children();
      //}, [](Prim &p, const Prim &c) {
      //  p.children().push_back(c);
      //}, py::return_value_policy::reference)
      .def(
          "children",
          [](Prim &p) -> std::vector<Prim> & { return p.children(); },
          py::return_value_policy::reference)
      //.def_property("primChildren", static_cast<const std::vector<Prim> &(Prim::*)(void)>Prim::children,
      //}, [](Prim &p, const std::vector<Prim> &v) {
      //  py::print("setter");
      //  p.children() = v;
      //})
      .def("__str__", [](const Prim &p) {
        return to_string(p);       
      })
      ;

  py::class_<StageMetas>(m, "StageMetas")
    .def(py::init<>())
    .def_property("metersPerUnit", 
      [](const StageMetas &m) -> const double {
        py::print("metersPerUnit get");
        return m.metersPerUnit.get_value();
      }, [](StageMetas &m, const double v) {
        py::print("metersPerUnit set");
        m.metersPerUnit.set_value(v);
        py::print("metersPerUnit ", m.metersPerUnit.get_value());
      }, py::return_value_policy::reference_internal)
    ;

  py::class_<Stage>(m, "Stage")
      .def(py::init<>())
      // Use rvp::reference for lvalue C++ reference.
      .def("metas", [](Stage &s) -> StageMetas & {
        py::print("metas method"); return s.metas(); }, py::return_value_policy::reference)
      .def("commit", &Stage::commit)
      .def(
          "root_prims",
          [](Stage &stage) -> std::vector<Prim> & { return stage.root_prims(); },
          py::return_value_policy::reference)
      .def("GetPrimAtPath",
           [](const Stage &s, const std::string &path_str) -> py::object {
             Path path(path_str, "");

             if (auto p = s.GetPrimAtPath(path)) {
               return py::cast(*p);
             }

             return py::none();
           })
      .def("ExportToString", &Stage::ExportToString)
      .def("dump_prim_tree", &Stage::dump_prim_tree)
      .def("find_prim_by_prim_id",
           [](Stage &s, uint64_t prim_id) -> py::object {

             Prim *prim{nullptr};
             if (auto p = s.find_prim_by_prim_id(prim_id, prim)) {
               return py::cast(prim);
             }

             return py::none();
           }, py::return_value_policy::reference);
  

  m.def("LoadUSDFromFile", &LoadUSDAFromFile);

  py::class_<std::vector<Prim>>(m, "PrimVector")
      .def(py::init<>())
      .def("clear", &std::vector<Prim>::clear)
      .def(
          "append",
          [](std::vector<Prim> &pv, const Prim &prim) { pv.push_back(prim); },
          py::keep_alive<1, 2>())
      .def("__len__", [](const std::vector<Prim> &v) { return v.size(); })
      .def(
          "__iter__",
          [](std::vector<Prim> &v) {
            return py::make_iterator(v.begin(), v.end());
          },
          py::keep_alive<0, 1>());

  // py::class_<tinyusdz::GPrim>(UsdModule, "GPrim");

  // py::class_<tinyusdz::GeomSphere>(m, "Sphere")
  //   .def(py::init<>())
  //;

  // py::class_<tinyusdz::GeomSphere>(m, "Sphere")
  //   .def(py::init<>())
  //   .def

#if 0

#if 1

#if 0
#define DEFINE_ARRAY_TYPE(__ty, __name) \
  py::class_<std::vector<__ty>>(m, __name) \
      .def(py::init<>()) \
      .def("clear", &std::vector<__ty>::clear) \
      .def( \
          "append", \
          [](std::vector<__ty> &pv, const __ty &v) { pv.push_back(v); }, \
          py::keep_alive<1, 2>()) \
      .def("__len__", [](const std::vector<__ty> &v) { return v.size(); }) \
      .def( \
          "__iter__", \
          [](std::vector<__ty> &v) { \
            return py::make_iterator(v.begin(), v.end()); \
          }, \
          py::keep_alive<0, 1>())
#else

#define DEFINE_ARRAY_TYPE(__ty, __name) \
  py::class_<std::vector<__ty>>(m, __name) \
      .def(py::init<>()) \
      .def("clear", &std::vector<__ty>::clear) 

#endif

  DEFINE_ARRAY_TYPE(float, "FloatVector");
  DEFINE_ARRAY_TYPE(bool, "BoolVector");
  DEFINE_ARRAY_TYPE(uint8_t, "ByteVector");
  DEFINE_ARRAY_TYPE(uint16_t, "UInt16Vector"); // TODO: deprecate?
  DEFINE_ARRAY_TYPE(int32_t, "IntVector");
  DEFINE_ARRAY_TYPE(value::int2, "Int2Vector");
  DEFINE_ARRAY_TYPE(value::int3, "Int3Vector");
  DEFINE_ARRAY_TYPE(value::int4, "Int4Vector");
  DEFINE_ARRAY_TYPE(uint32_t, "UIntVector");
  DEFINE_ARRAY_TYPE(value::uint2, "UInt2Vector");
  DEFINE_ARRAY_TYPE(value::uint3, "UInt3Vector");
  DEFINE_ARRAY_TYPE(value::uint4, "UInt4Vector");

  DEFINE_ARRAY_TYPE(int64_t, "Int64Vector");
  DEFINE_ARRAY_TYPE(uint64_t, "UInt64Vector");
  DEFINE_ARRAY_TYPE(value::half, "HalfVector");
  DEFINE_ARRAY_TYPE(value::half2, "Half2Vector");
  DEFINE_ARRAY_TYPE(value::half3, "Half3Vector");
  DEFINE_ARRAY_TYPE(value::half4, "Half4Vector");
  DEFINE_ARRAY_TYPE(float, "FloatVector");
  DEFINE_ARRAY_TYPE(value::float2, "Float2Vector");
  DEFINE_ARRAY_TYPE(value::float3, "Float3Vector");
  DEFINE_ARRAY_TYPE(value::float4, "Float4Vector");
  DEFINE_ARRAY_TYPE(double, "DoubleVector");
  DEFINE_ARRAY_TYPE(value::double2, "Double2Vector");
  DEFINE_ARRAY_TYPE(value::double3, "Double3Vector");
  DEFINE_ARRAY_TYPE(value::double4, "Double4Vector");

  DEFINE_ARRAY_TYPE(value::quath, "QuathVector");
  DEFINE_ARRAY_TYPE(value::quatf, "QuatfVector");
  DEFINE_ARRAY_TYPE(value::quatd, "QuatdVector");

  DEFINE_ARRAY_TYPE(value::normal3h, "Normal3hVector");
  DEFINE_ARRAY_TYPE(value::normal3f, "Normal3fVector");
  DEFINE_ARRAY_TYPE(value::normal3d, "Normal3dVector");

  DEFINE_ARRAY_TYPE(value::vector3h, "Vector3hVector");
  DEFINE_ARRAY_TYPE(value::vector3f, "Vector3fVector");
  DEFINE_ARRAY_TYPE(value::vector3d, "Vector3dVector");

  DEFINE_ARRAY_TYPE(value::point3h, "Point2hVector");
  DEFINE_ARRAY_TYPE(value::point3f, "Point3fVector");
  DEFINE_ARRAY_TYPE(value::point3d, "Point3dVector");

  DEFINE_ARRAY_TYPE(value::color3h, "Color3hVector");
  DEFINE_ARRAY_TYPE(value::color3f, "Color3fVector");
  DEFINE_ARRAY_TYPE(value::color3d, "Color3dVector");
  DEFINE_ARRAY_TYPE(value::color4h, "Color4hVector");
  DEFINE_ARRAY_TYPE(value::color4f, "Color4fVector");
  DEFINE_ARRAY_TYPE(value::color4d, "Color4dVector");

  DEFINE_ARRAY_TYPE(value::texcoord2h, "Texcoord2hVector");
  DEFINE_ARRAY_TYPE(value::texcoord2f, "Texcoord2fVector");
  DEFINE_ARRAY_TYPE(value::texcoord2d, "Texcoord2dVector");
  DEFINE_ARRAY_TYPE(value::texcoord3h, "Texcoord3hVector");
  DEFINE_ARRAY_TYPE(value::texcoord3f, "Texcoord3fVector");
  DEFINE_ARRAY_TYPE(value::texcoord3d, "Texcoord3dVector");

  DEFINE_ARRAY_TYPE(value::matrix2d, "Matrix2dVector");
  DEFINE_ARRAY_TYPE(value::matrix3d, "Matrix3dVector");
  DEFINE_ARRAY_TYPE(value::matrix4d, "Matrix4dVector");
  DEFINE_ARRAY_TYPE(value::frame4d, "Frame4dVector");

#undef DEFINE_ARRAY_TYPE


#else

  // very slow to compile...
  py::bind_vector<std::vector<bool>>(m, "BoolVector");
  py::bind_vector<std::vector<uint8_t>>(m, "ByteVector");
  py::bind_vector<std::vector<uint16_t>>(m, "UInt16Vector"); // TODO: deprecate?
  py::bind_vector<std::vector<int32_t>>(m, "IntVector");
  py::bind_vector<std::vector<value::int2>>(m, "Int2Vector");
  py::bind_vector<std::vector<value::int3>>(m, "Int3Vector");
  py::bind_vector<std::vector<value::int4>>(m, "Int4Vector");
  py::bind_vector<std::vector<uint32_t>>(m, "UIntVector");
  py::bind_vector<std::vector<value::uint2>>(m, "UInt2Vector");
  py::bind_vector<std::vector<value::uint3>>(m, "UInt3Vector");
  py::bind_vector<std::vector<value::uint4>>(m, "UInt4Vector");

  py::bind_vector<std::vector<int64_t>>(m, "Int64Vector");
  py::bind_vector<std::vector<uint64_t>>(m, "UInt64Vector");
  py::bind_vector<std::vector<value::half>>(m, "HalfVector");
  py::bind_vector<std::vector<value::half2>>(m, "Half2Vector");
  py::bind_vector<std::vector<value::half3>>(m, "Half3Vector");
  py::bind_vector<std::vector<value::half4>>(m, "Half4Vector");
  py::bind_vector<std::vector<float>>(m, "FloatVector");
  py::bind_vector<std::vector<value::float2>>(m, "Float2Vector");
  py::bind_vector<std::vector<value::float3>>(m, "Float3Vector");
  py::bind_vector<std::vector<value::float4>>(m, "Float4Vector");
  py::bind_vector<std::vector<double>>(m, "DoubleVector");
  py::bind_vector<std::vector<value::double2>>(m, "Double2Vector");
  py::bind_vector<std::vector<value::double3>>(m, "Double3Vector");
  py::bind_vector<std::vector<value::double4>>(m, "Double4Vector");

  py::bind_vector<std::vector<value::quath>>(m, "QuathVector");
  py::bind_vector<std::vector<value::quatf>>(m, "QuatfVector");
  py::bind_vector<std::vector<value::quatd>>(m, "QuatdVector");

  py::bind_vector<std::vector<value::normal3h>>(m, "Normal3hVector");
  py::bind_vector<std::vector<value::normal3f>>(m, "Normal3fVector");
  py::bind_vector<std::vector<value::normal3d>>(m, "Normal3dVector");

  py::bind_vector<std::vector<value::vector3h>>(m, "Vector3hVector");
  py::bind_vector<std::vector<value::vector3f>>(m, "Vector3fVector");
  py::bind_vector<std::vector<value::vector3d>>(m, "Vector3dVector");

  py::bind_vector<std::vector<value::point3h>>(m, "Point3hVector");
  py::bind_vector<std::vector<value::point3f>>(m, "Point3fVector");
  py::bind_vector<std::vector<value::point3d>>(m, "Point3dVector");

  py::bind_vector<std::vector<value::color3h>>(m, "Color3hVector");
  py::bind_vector<std::vector<value::color3f>>(m, "Color3fVector");
  py::bind_vector<std::vector<value::color3d>>(m, "Color3dVector");
  py::bind_vector<std::vector<value::color4h>>(m, "Color4hVector");
  py::bind_vector<std::vector<value::color4f>>(m, "Color4fVector");
  py::bind_vector<std::vector<value::color4d>>(m, "Color4dVector");

  py::bind_vector<std::vector<value::texcoord2h>>(m, "Texcoord2hVector");
  py::bind_vector<std::vector<value::texcoord2f>>(m, "Texcoord2fVector");
  py::bind_vector<std::vector<value::texcoord2d>>(m, "Texcoord2dVector");
  py::bind_vector<std::vector<value::texcoord3h>>(m, "Texcoord3hVector");
  py::bind_vector<std::vector<value::texcoord3f>>(m, "Texcoord3fVector");
  py::bind_vector<std::vector<value::texcoord3d>>(m, "Texcoord3dVector");

  py::bind_vector<std::vector<value::matrix2d>>(m, "Matrix2dVector");
  py::bind_vector<std::vector<value::matrix3d>>(m, "Matrix3dVector");
  py::bind_vector<std::vector<value::matrix4d>>(m, "Matrix4dVector");
  py::bind_vector<std::vector<value::frame4d>>(m, "Frame4dVector");
#endif

#endif

  // Tydra
  {
    auto m_tydra = m.def_submodule("tydra");

    py::class_<tydra::RenderSceneConverterConfig>(m_tydra, "RenderSceneConverterConfig")
      .def(py::init<>())
      .def_readwrite("load_texture_assets", &tydra::RenderSceneConverterConfig::load_texture_assets)
    ;

    m_tydra.def("to_render_scene", [](const Stage &stage) {
      py::print("TODO");
    }, py::arg("config") = tydra::RenderSceneConverterConfig());
  }
}
