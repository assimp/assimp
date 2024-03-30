///
/// Type-erasure technique for PrimVar, a Value class which can have 30+ different types.
/// Neigher std::any nor std::variant is applicable for such usecases, so write our own.
///
#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "../../src/external/staticstruct.hh"

// clang and gcc
#if defined(__EXCEPTIONS) || defined(__cpp_exceptions) 
#define nsel_CONFIG_NO_EXCEPTIONS 0
#define nssv_CONFIG_NO_EXCEPTIONS 0
#else
// -fno-exceptions
#define nsel_CONFIG_NO_EXCEPTIONS 1
#define nssv_CONFIG_NO_EXCEPTIONS 1
#endif
#include "../../src/nonstd/expected.hpp"
#include "../../src/nonstd/optional.hpp"
#include "../../src/nonstd/string_view.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// string literal. represent it as string_view
using token = nonstd::string_view;

constexpr uint32_t TYPE_ID_1D_ARRAY_BIT = 1 << 10;
constexpr uint32_t TYPE_ID_2D_ARRAY_BIT = 1 << 11;

// TODO(syoyo): Use compile-time string hash?
enum TypeId {
  TYPE_ID_INVALID,  // = 0

  TYPE_ID_TOKEN,  
  TYPE_ID_STRING,

  TYPE_ID_BOOL,

  // TYPE_ID_INT8,
  TYPE_ID_HALF,
  TYPE_ID_INT32,
  TYPE_ID_INT64,

  TYPE_ID_HALF2,
  TYPE_ID_HALF3,
  TYPE_ID_HALF4,

  TYPE_ID_INT2,  // int32 x 2
  TYPE_ID_INT3,
  TYPE_ID_INT4,

  TYPE_ID_UCHAR,  // uint8
  TYPE_ID_UINT32,
  TYPE_ID_UINT64,

  TYPE_ID_UINT2,
  TYPE_ID_UINT3,
  TYPE_ID_UINT4,

  TYPE_ID_FLOAT,
  TYPE_ID_FLOAT2,
  TYPE_ID_FLOAT3,
  TYPE_ID_FLOAT4,

  TYPE_ID_DOUBLE,
  TYPE_ID_DOUBLE2,
  TYPE_ID_DOUBLE3,
  TYPE_ID_DOUBLE4,

  TYPE_ID_QUATH,
  TYPE_ID_QUATF,
  TYPE_ID_QUATD,

  TYPE_ID_MATRIX2D,
  TYPE_ID_MATRIX3D,
  TYPE_ID_MATRIX4D,

  TYPE_ID_COLOR3H,
  TYPE_ID_COLOR3F,
  TYPE_ID_COLOR3D,

  TYPE_ID_COLOR4H,
  TYPE_ID_COLOR4F,
  TYPE_ID_COLOR4D,

  TYPE_ID_POINT3H,
  TYPE_ID_POINT3F,
  TYPE_ID_POINT3D,

  TYPE_ID_NORMAL3H,
  TYPE_ID_NORMAL3F,
  TYPE_ID_NORMAL3D,

  TYPE_ID_VECTOR3H,
  TYPE_ID_VECTOR3F,
  TYPE_ID_VECTOR3D,

  TYPE_ID_FRAME4D,

  TYPE_ID_TEXCOORD2H,
  TYPE_ID_TEXCOORD2F,
  TYPE_ID_TEXCOORD2D,

  TYPE_ID_TEXCOORD3H,
  TYPE_ID_TEXCOORD3F,
  TYPE_ID_TEXCOORD3D,

  TYPE_ID_TIMECODE,
  TYPE_ID_TIMESAMPLE,

  TYPE_ID_DICT,

  TYPE_ID_ALL  // terminator
};

struct timecode
{
  double value;
};

using half = uint16_t;

using half2 = std::array<half, 2>;
using half3 = std::array<half, 3>;
using half4 = std::array<half, 4>;

using int2 = std::array<int32_t, 2>;
using int3 = std::array<int32_t, 3>;
using int4 = std::array<int32_t, 4>;

using uint2 = std::array<uint32_t, 2>;
using uint3 = std::array<uint32_t, 3>;
using uint4 = std::array<uint32_t, 4>;

using float2 = std::array<float, 2>;
using float3 = std::array<float, 3>;
using float4 = std::array<float, 4>;

using double2 = std::array<double, 2>;
using double3 = std::array<double, 3>;
using double4 = std::array<double, 4>;

struct matrix2d {
  matrix2d() {
    m[0][0] = 1.0;
    m[0][1] = 0.0;

    m[1][0] = 0.0;
    m[1][1] = 1.0;
  }

  double m[2][2];
};

struct matrix3d {
  matrix3d() {
    m[0][0] = 1.0;
    m[0][1] = 0.0;
    m[0][2] = 0.0;

    m[1][0] = 0.0;
    m[1][1] = 1.0;
    m[1][2] = 0.0;

    m[2][0] = 0.0;
    m[2][1] = 0.0;
    m[2][2] = 1.0;
  }

  double m[3][3];
};

struct matrix4d {
  matrix4d() {
    m[0][0] = 1.0;
    m[0][1] = 0.0;
    m[0][2] = 0.0;
    m[0][3] = 0.0;

    m[1][0] = 0.0;
    m[1][1] = 1.0;
    m[1][2] = 0.0;
    m[1][3] = 0.0;

    m[2][0] = 0.0;
    m[2][1] = 0.0;
    m[2][2] = 1.0;
    m[2][3] = 0.0;

    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[3][3] = 1.0;
  }

  double m[4][4];
};

// = matrix4d
struct frame4d {
  frame4d() {
    m[0][0] = 1.0;
    m[0][1] = 0.0;
    m[0][2] = 0.0;
    m[0][3] = 0.0;

    m[1][0] = 0.0;
    m[1][1] = 1.0;
    m[1][2] = 0.0;
    m[1][3] = 0.0;

    m[2][0] = 0.0;
    m[2][1] = 0.0;
    m[2][2] = 1.0;
    m[2][3] = 0.0;

    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[3][3] = 1.0;
  }
  double m[4][4];
};

struct quath {
  half real;
  half3 imag;
};

struct quatf {
  float real;
  float3 imag;
};

struct quatd {
  double real;
  double3 imag;
};

struct vector3h {
  half x, y, z;

  half operator[](size_t idx) { return *(&x + idx); }
};

struct vector3f {
  float x, y, z;

  float operator[](size_t idx) { return *(&x + idx); }
};

struct vector3d {
  double x, y, z;

  double operator[](size_t idx) { return *(&x + idx); }
};

struct normal3h {
  half x, y, z;

  half operator[](size_t idx) { return *(&x + idx); }
};

struct normal3f {
  float x, y, z;

  float operator[](size_t idx) { return *(&x + idx); }
};

struct normal3d {
  double x, y, z;

  double operator[](size_t idx) { return *(&x + idx); }
};

struct point3h {
  half x, y, z;

  half operator[](size_t idx) { return *(&x + idx); }
};

struct point3f {
  float x, y, z;

  float operator[](size_t idx) { return *(&x + idx); }
};

struct point3d {
  double x, y, z;

  double operator[](size_t idx) { return *(&x + idx); }
};

struct color3f {
  float r, g, b;

  // C++11 or later, struct is tightly packed, so use the pointer offset is
  // valid.
  float operator[](size_t idx) { return *(&r + idx); }
};

struct color4f {
  float r, g, b, a;

  // C++11 or later, struct is tightly packed, so use the pointer offset is
  // valid.
  float operator[](size_t idx) { return *(&r + idx); }
};

struct color3d {
  double r, g, b;

  // C++11 or later, struct is tightly packed, so use the pointer offset is
  // valid.
  double operator[](size_t idx) { return *(&r + idx); }
};

struct color4d {
  double r, g, b, a;

  // C++11 or later, struct is tightly packed, so use the pointer offset is
  // valid.
  double operator[](size_t idx) { return *(&r + idx); }
};

struct texcoord2h {
  half s, t;
};

struct texcoord2f {
  float s, t;
};

struct texcoord2d {
  double s, t;
};

struct texcoord3h {
  half s, t, r;
};

struct texcoord3f {
  float s, t, r;
};

struct texcoord3d {
  double s, t, r;
};

using double2 = std::array<double, 2>;
using double3 = std::array<double, 3>;
using double4 = std::array<double, 4>;

struct any_value;

using dict = std::map<std::string, any_value>;

//
// Simple variant-lile type
//

template <class dtype>
struct TypeTrait;

#define DEFINE_TYPE_TRAIT(__dty, __name, __tyid, __nc)           \
  template <>                                                    \
  struct TypeTrait<__dty> {                                      \
    using value_type = __dty;                                    \
    using value_underlying_type = __dty;                         \
    static constexpr uint32_t ndim = 0; /* array dim */          \
    static constexpr uint32_t ncomp =                            \
        __nc; /* the number of components(e.g. float3 => 3) */   \
    static constexpr uint32_t type_id = __tyid;                  \
    static constexpr uint32_t underlying_type_id = __tyid;       \
    static std::string type_name() { return __name; }            \
    static std::string underlying_type_name() { return __name; } \
  }

// `role` type. Requies underlying type.
#define DEFINE_ROLE_TYPE_TRAIT(__dty, __name, __tyid, __uty)                  \
  template <>                                                                 \
  struct TypeTrait<__dty> {                                                   \
    using value_type = __dty;                                                 \
    using value_underlying_type = TypeTrait<__uty>::value_type;               \
    static constexpr uint32_t ndim = 0; /* array dim */                       \
    static constexpr uint32_t ncomp = TypeTrait<__uty>::ncomp;                \
    static constexpr uint32_t type_id = __tyid;                               \
    static constexpr uint32_t underlying_type_id = TypeTrait<__uty>::type_id; \
    static std::string type_name() { return __name; }                         \
    static std::string underlying_type_name() {                               \
      return TypeTrait<__uty>::type_name();                                   \
    }                                                                         \
  }

DEFINE_TYPE_TRAIT(bool, "bool", TYPE_ID_BOOL, 1);
DEFINE_TYPE_TRAIT(uint8_t, "uchar", TYPE_ID_UCHAR, 1);
DEFINE_TYPE_TRAIT(half, "half", TYPE_ID_HALF, 1);

DEFINE_TYPE_TRAIT(int32_t, "int", TYPE_ID_INT32, 1);
DEFINE_TYPE_TRAIT(uint32_t, "uint", TYPE_ID_UINT32, 1);

DEFINE_TYPE_TRAIT(int64_t, "int64", TYPE_ID_INT64, 1);
DEFINE_TYPE_TRAIT(uint64_t, "uint64", TYPE_ID_UINT64, 1);

DEFINE_TYPE_TRAIT(int2, "int2", TYPE_ID_INT2, 2);
DEFINE_TYPE_TRAIT(int3, "int3", TYPE_ID_INT3, 3);
DEFINE_TYPE_TRAIT(int4, "int4", TYPE_ID_INT4, 4);

DEFINE_TYPE_TRAIT(uint2, "uint2", TYPE_ID_UINT2, 2);
DEFINE_TYPE_TRAIT(uint3, "uint3", TYPE_ID_UINT3, 3);
DEFINE_TYPE_TRAIT(uint4, "uint4", TYPE_ID_UINT4, 4);

DEFINE_TYPE_TRAIT(half2, "half2", TYPE_ID_HALF2, 2);
DEFINE_TYPE_TRAIT(half3, "half3", TYPE_ID_HALF3, 3);
DEFINE_TYPE_TRAIT(half4, "half4", TYPE_ID_HALF4, 4);

DEFINE_TYPE_TRAIT(float, "float", TYPE_ID_FLOAT, 1);
DEFINE_TYPE_TRAIT(float2, "float2", TYPE_ID_FLOAT2, 2);
DEFINE_TYPE_TRAIT(float3, "float3", TYPE_ID_FLOAT3, 3);
DEFINE_TYPE_TRAIT(float4, "float4", TYPE_ID_FLOAT4, 4);

DEFINE_TYPE_TRAIT(double, "double", TYPE_ID_DOUBLE, 1);
DEFINE_TYPE_TRAIT(double2, "double2", TYPE_ID_DOUBLE2, 2);
DEFINE_TYPE_TRAIT(double3, "double3", TYPE_ID_DOUBLE3, 3);
DEFINE_TYPE_TRAIT(double4, "double4", TYPE_ID_DOUBLE4, 4);


DEFINE_TYPE_TRAIT(quath, "quath", TYPE_ID_QUATH, 1);
DEFINE_TYPE_TRAIT(quatf, "quatf", TYPE_ID_QUATF, 1);
DEFINE_TYPE_TRAIT(quatd, "quatd", TYPE_ID_QUATD, 1);

DEFINE_TYPE_TRAIT(matrix2d, "matrix2d", TYPE_ID_MATRIX2D, 1);
DEFINE_TYPE_TRAIT(matrix3d, "matrix3d", TYPE_ID_MATRIX3D, 1);
DEFINE_TYPE_TRAIT(matrix4d, "matrix4d", TYPE_ID_MATRIX4D, 1);

DEFINE_TYPE_TRAIT(timecode, "timecode", TYPE_ID_TIMECODE, 1);

//
// Role types
//
DEFINE_ROLE_TYPE_TRAIT(vector3h, "vector3h", TYPE_ID_VECTOR3H, half3);
DEFINE_ROLE_TYPE_TRAIT(vector3f, "vector3f", TYPE_ID_VECTOR3F, float3);
DEFINE_ROLE_TYPE_TRAIT(vector3d, "vector3d", TYPE_ID_VECTOR3D, double3);

DEFINE_ROLE_TYPE_TRAIT(normal3h, "normal3h", TYPE_ID_NORMAL3H, half3);
DEFINE_ROLE_TYPE_TRAIT(normal3f, "normal3f", TYPE_ID_NORMAL3F, float3);
DEFINE_ROLE_TYPE_TRAIT(normal3d, "normal3d", TYPE_ID_NORMAL3D, double3);

DEFINE_ROLE_TYPE_TRAIT(point3h, "point3h", TYPE_ID_POINT3H, half3);
DEFINE_ROLE_TYPE_TRAIT(point3f, "point3f", TYPE_ID_POINT3F, float3);
DEFINE_ROLE_TYPE_TRAIT(point3d, "point3d", TYPE_ID_POINT3D, double3);

DEFINE_ROLE_TYPE_TRAIT(frame4d, "frame4d", TYPE_ID_FRAME4D, matrix4d);

DEFINE_ROLE_TYPE_TRAIT(color3f, "color3f", TYPE_ID_COLOR3F, float3);
DEFINE_ROLE_TYPE_TRAIT(color4f, "color4f", TYPE_ID_COLOR4F, float4);
DEFINE_ROLE_TYPE_TRAIT(color3d, "color3d", TYPE_ID_COLOR3D, double3);
DEFINE_ROLE_TYPE_TRAIT(color4d, "color4d", TYPE_ID_COLOR4D, double4);

DEFINE_ROLE_TYPE_TRAIT(texcoord2h, "texcoord2h", TYPE_ID_TEXCOORD2H, half2);
DEFINE_ROLE_TYPE_TRAIT(texcoord2f, "texcoord2f", TYPE_ID_TEXCOORD2F, float2);
DEFINE_ROLE_TYPE_TRAIT(texcoord2d, "texcoord2d", TYPE_ID_TEXCOORD2D, double2);

DEFINE_ROLE_TYPE_TRAIT(texcoord3h, "texcoord3h", TYPE_ID_TEXCOORD3H, half3);
DEFINE_ROLE_TYPE_TRAIT(texcoord3f, "texcoord3f", TYPE_ID_TEXCOORD3F, float3);
DEFINE_ROLE_TYPE_TRAIT(texcoord3d, "texcoord3d", TYPE_ID_TEXCOORD3D, double3);

//
//
//
DEFINE_TYPE_TRAIT(token, "token", TYPE_ID_TOKEN, 1);
DEFINE_TYPE_TRAIT(std::string, "string", TYPE_ID_STRING, 1);
DEFINE_TYPE_TRAIT(dict, "dictionary", TYPE_ID_DICT, 1);

#undef DEFINE_TYPE_TRAIT

// 1D Array
template <typename T>
struct TypeTrait<std::vector<T>> {
  using value_type = std::vector<T>;
  static constexpr uint32_t ndim = 1; /* array dim */
  static constexpr uint32_t ncomp = TypeTrait<T>::ncomp;
  static constexpr uint32_t type_id =
      TypeTrait<T>::type_id | TYPE_ID_1D_ARRAY_BIT;
  static constexpr uint32_t underlying_type_id =
      TypeTrait<T>::underlying_type_id | TYPE_ID_1D_ARRAY_BIT;
  static std::string type_name() { return TypeTrait<T>::type_name() + "[]"; }
  static std::string underlying_type_name() {
    return TypeTrait<T>::underlying_type_name() + "[]";
  }
};

// 2D Array
// TODO(syoyo): support 3D array?
template <typename T>
struct TypeTrait<std::vector<std::vector<T>>> {
  using value_type = std::vector<std::vector<T>>;
  static constexpr uint32_t ndim = 2; /* array dim */
  static constexpr uint32_t ncomp = TypeTrait<T>::ncomp;
  static constexpr uint32_t type_id =
      TypeTrait<T>::type_id | TYPE_ID_2D_ARRAY_BIT;
  static constexpr uint32_t underlying_type_id =
      TypeTrait<T>::underlying_type_id | TYPE_ID_2D_ARRAY_BIT;
  static std::string type_name() { return TypeTrait<T>::type_name() + "[][]"; }
  static std::string underlying_type_name() {
    return TypeTrait<T>::underlying_type_name() + "[][]";
  }
};

static std::string GetTypeName(uint32_t tyid) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

  static std::map<uint32_t, std::string> m;

#ifdef __clang__
#pragma clang diagnostic pop
#endif

  if (m.empty()) {
    // initialize
    m[TYPE_ID_BOOL] = TypeTrait<bool>::type_name();
    m[TYPE_ID_UCHAR] = TypeTrait<uint8_t>::type_name();
    m[TYPE_ID_INT32] = TypeTrait<int32_t>::type_name();
    m[TYPE_ID_UINT32] = TypeTrait<uint32_t>::type_name();
    // TODO: ...

    m[TYPE_ID_INT32 | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<int>>::type_name();
    m[TYPE_ID_FLOAT | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<float>>::type_name();
    m[TYPE_ID_FLOAT2 | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<float2>>::type_name();
    m[TYPE_ID_FLOAT3 | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<float3>>::type_name();
    m[TYPE_ID_FLOAT4 | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<float4>>::type_name();

    m[TYPE_ID_VECTOR3H | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<vector3h>>::type_name();
    m[TYPE_ID_VECTOR3F | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<vector3f>>::type_name();
    m[TYPE_ID_VECTOR3D | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<vector3d>>::type_name();
  }

  if (!m.count(tyid)) {
    return "(GetTypeName) [[Unknown or unsupported type_id: " +
           std::to_string(tyid) + "]]";
  }

  return m.at(tyid);
}

struct base_value {
  virtual ~base_value();
  virtual const std::string type_name() const = 0;
  virtual const std::string underlying_type_name() const = 0;
  virtual uint32_t type_id() const = 0;
  virtual uint32_t underlying_type_id() const = 0;

  virtual uint32_t ndim() const = 0;
  virtual uint32_t ncomp() const = 0;

  virtual const void *value() const = 0;
  virtual void *value() = 0;
};

base_value::~base_value() {}

template <typename T>
struct value_impl : public base_value {
  using type = typename TypeTrait<T>::value_type;

  value_impl(const T &v) : _value(v) {}

  const std::string type_name() const override {
    return TypeTrait<T>::type_name();
  }

  const std::string underlying_type_name() const override {
    return TypeTrait<T>::underlying_type_name();
  }

  uint32_t type_id() const override { return TypeTrait<T>::type_id; }
  uint32_t underlying_type_id() const override {
    return TypeTrait<T>::underlying_type_id;
  }

  const void *value() const override {
    return reinterpret_cast<const void *>(&_value);
  }

  void *value() override { return reinterpret_cast<void *>(&_value); }

  uint32_t ndim() const override { return TypeTrait<T>::ndim; }
  uint32_t ncomp() const override { return TypeTrait<T>::ncomp; }

  T _value;
};

struct any_value {
  any_value() = default;

  template <typename T>
  any_value(const T &v) {
    p.reset(new value_impl<T>(v));
  }

  const std::string type_name() const {
    if (p) {
      return p->type_name();
    }
    return std::string();
  }

  const std::string underlying_type_name() const {
    if (p) {
      return p->underlying_type_name();
    }
    return std::string();
  }

  uint32_t type_id() const {
    if (p) {
      return p->type_id();
    }

    return TYPE_ID_INVALID;
  }

  uint32_t underlying_type_id() const {
    if (p) {
      return p->underlying_type_id();
    }

    return TYPE_ID_INVALID;
  }

  int32_t ndim() const {
    if (p) {
      return int32_t(p->ndim());
    }

    return -1;  // invalid
  }

  uint32_t ncomp() const {
    if (p) {
      return p->ncomp();
    }

    return 0;  // empty
  }

  const void *value() const {
    if (p) {
      return p->value();
    }
    return nullptr;
  }

  void *value() {
    if (p) {
      return p->value();
    }
    return nullptr;
  }

  template <class T>
  operator T() const {
    assert(TypeTrait<T>::type_id == p->type_id());

    return *(reinterpret_cast<const T *>(p->value()));
  }

  std::shared_ptr<base_value> p;
};

struct TimeSample {
  std::vector<double> times;
  std::vector<any_value> values;
};

// simple linear interpolator
template<typename T>
struct LinearInterpolator
{
  static T interpolate(const T *values, const size_t n, const double _t) {
    if (n == 0) {
      return static_cast<T>(0);
    } else if (n == 1) {
      return values[0];
    }

    // [0.0, 1.0]
    double t = std::fmin(0.0, std::fmax(_t, 1.0));

    size_t idx0 = std::max(n-1, size_t(t * double(n)));
    size_t idx1 = std::max(n-1, idx0+1);

    return (1.0 - t) * values[idx0] + t * values[idx1];
  } 
};

// Explicitly typed version of `TimeSample`
template<typename T>
struct AnimatableValue
{
  std::vector<double> times; // Assume sorted
  std::vector<T> values;

  bool is_scalar() const {
    return (times.size() == 0) && (values.size() == 1);
  }

  bool is_timesample() const {
    return (times.size() > 0) && (times.size() == values.size());
  }

  template<class Interpolator>
  T Get(double time = 0.0) {
    std::vector<double>::iterator it = std::lower_bound(times.begin(), times.end(), time);

    size_t idx0, idx1;
    if (it != times.end()) {
      idx0 = std::distance(times.begin(), it);
      idx1 = std::min(idx0 + 1, times.size() - 1);
    } else {
      idx0 = idx1 = times.size() - 1;
    }
    double slope = times[idx1] - times[idx0];
    if (slope < std::numeric_limits<double>::epsilon()) {
      slope = 1.0;
    }

    const double t = (times[idx1] - time) / slope;

    T val = Interpolator::interpolate(values.data(), values.size(), t);
    return val;
  }
};

struct PrimVar {
  // For scalar value, times.size() == 0, and values.size() == 1
  TimeSample var;

  bool is_scalar() const {
    return (var.times.size() == 0) && (var.values.size() == 1);
  }

  bool is_timesample() const {
    return (var.times.size() > 0) && (var.times.size() == var.values.size());
  }

  bool is_valid() const { return is_scalar() || is_timesample(); }

  std::string type_name() const {
    if (!is_valid()) {
      return std::string();
    }
    return var.values[0].type_name();
  }

  uint32_t type_id() const {
    if (!is_valid()) {
      return TYPE_ID_INVALID;
    }

    return var.values[0].type_id();
  }
};

// using Object = std::map<std::string, any_value>;

class Value {
 public:
  // using Dict = std::map<std::string, Value>;

  Value() = default;

  template <class T>
  Value(const T &v) : v_(v) {}

  std::string type_name() const { return v_.type_name(); }
  std::string underlying_type_name() const { return v_.underlying_type_name(); }

  uint32_t type_id() const { return v_.type_id(); }
  uint32_t underlying_type_id() const { return v_.underlying_type_id(); }

  // Return nullptr when type conversion failed.
  template <class T>
  const T *as() const {
    if (TypeTrait<T>::type_id == v_.type_id()) {
      return reinterpret_cast<const T *>(v_.value());
    } else {
      return nullptr;
    }
  }

  // Useful function to retrieve concrete value with type T.
  // Undefined behavior(usually will triger segmentation fault) when
  // type-mismatch. (We don't throw exception)
  template <class T>
  const T &value() const {
    return (*reinterpret_cast<const T *>(v_.value()));
  }

  // Type-safe way to get concrete value.
  template <class T>
  nonstd::optional<T> get_value() const {
    if (TypeTrait<T>::type_id == v_.type_id()) {
      return std::move(value<T>());
    } else if (TypeTrait<T>::underlying_type_id == v_.underlying_type_id()) {
      // `roll` type. Can be able to cast to underlying type since the memory
      // layout does not change.
      return std::move(value<T>());
    }
    return nonstd::nullopt;
  }

  template <class T>
  Value &operator=(const T &v) {
    v_ = v;
    return (*this);
  }

  bool is_array() const { return v_.ndim() > 0; }
  int32_t ndim() const { return v_.ndim(); }

  uint32_t ncomp() const { return v_.ncomp(); }

  bool is_vector_type() const { return v_.ncomp() > 1; }

  friend std::ostream &operator<<(std::ostream &os, const Value &v);

 private:
  any_value v_;
};

// Frequently-used utility function
bool is_float3(const Value &v);
bool is_float4(const Value &v);
bool is_double3(const Value &v);
bool is_double4(const Value &v);

bool is_float3(const Value &v) {
  if (v.underlying_type_name() == "float3") {
    return true;
  }

  return false;
}

bool is_float4(const Value &v) {
  if (v.underlying_type_name() == "float4") {
    return true;
  }

  return false;
}

bool is_double3(const Value &v) {
  if (v.underlying_type_name() == "double3") {
    return true;
  }

  return false;
}

bool is_double4(const Value &v) {
  if (v.underlying_type_name() == "double4") {
    return true;
  }

  return false;
}

std::ostream &operator<<(std::ostream &os, const int2 &v);
std::ostream &operator<<(std::ostream &os, const int3 &v);
std::ostream &operator<<(std::ostream &os, const int4 &v);

std::ostream &operator<<(std::ostream &os, const uint2 &v);
std::ostream &operator<<(std::ostream &os, const uint3 &v);
std::ostream &operator<<(std::ostream &os, const uint4 &v);

std::ostream &operator<<(std::ostream &os, const half2 &v);
std::ostream &operator<<(std::ostream &os, const half3 &v);
std::ostream &operator<<(std::ostream &os, const half4 &v);

std::ostream &operator<<(std::ostream &os, const float2 &v);
std::ostream &operator<<(std::ostream &os, const float3 &v);
std::ostream &operator<<(std::ostream &os, const float4 &v);
std::ostream &operator<<(std::ostream &os, const double2 &v);
std::ostream &operator<<(std::ostream &os, const double3 &v);
std::ostream &operator<<(std::ostream &os, const double4 &v);

std::ostream &operator<<(std::ostream &os, const normal3h &v);
std::ostream &operator<<(std::ostream &os, const normal3f &v);
std::ostream &operator<<(std::ostream &os, const normal3d &v);

std::ostream &operator<<(std::ostream &os, const vector3h &v);
std::ostream &operator<<(std::ostream &os, const vector3f &v);
std::ostream &operator<<(std::ostream &os, const vector3d &v);

std::ostream &operator<<(std::ostream &os, const point3h &v);
std::ostream &operator<<(std::ostream &os, const point3f &v);
std::ostream &operator<<(std::ostream &os, const point3d &v);

std::ostream &operator<<(std::ostream &os, const color3f &v);
std::ostream &operator<<(std::ostream &os, const color3d &v);
std::ostream &operator<<(std::ostream &os, const color4f &v);
std::ostream &operator<<(std::ostream &os, const color4d &v);

std::ostream &operator<<(std::ostream &os, const texcoord2h &v);
std::ostream &operator<<(std::ostream &os, const texcoord2f &v);
std::ostream &operator<<(std::ostream &os, const texcoord2d &v);

std::ostream &operator<<(std::ostream &os, const texcoord3h &v);
std::ostream &operator<<(std::ostream &os, const texcoord3f &v);
std::ostream &operator<<(std::ostream &os, const texcoord3d &v);

std::ostream &operator<<(std::ostream &os, const quath &v);
std::ostream &operator<<(std::ostream &os, const quatf &v);
std::ostream &operator<<(std::ostream &os, const quatd &v);

std::ostream &operator<<(std::ostream &os, const dict &v);
std::ostream &operator<<(std::ostream &os, const TimeSample &ts);

std::ostream &operator<<(std::ostream &os, const any_value &v);

std::ostream &operator<<(std::ostream &os, const matrix2d &v);
std::ostream &operator<<(std::ostream &os, const matrix3d &v);
std::ostream &operator<<(std::ostream &os, const matrix4d &v);

std::ostream &operator<<(std::ostream &os, const matrix2d &v) {
  os << "(";
  os << "(" << v.m[0][0] << ", " << v.m[0][1] << "), ";
  os << "(" << v.m[1][0] << ", " << v.m[1][1] << ")";
  os << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const matrix3d &v) {
  os << "(";
  os << "(" << v.m[0][0] << ", " << v.m[0][1] << ", " << v.m[0][2] << "), ";
  os << "(" << v.m[1][0] << ", " << v.m[1][1] << ", " << v.m[1][2] << ")";
  os << "(" << v.m[2][0] << ", " << v.m[2][1] << ", " << v.m[2][2] << ")";
  os << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const matrix4d &v) {
  os << "(";
  os << "(" << v.m[0][0] << ", " << v.m[0][1] << ", " << v.m[0][2] << ", "
     << v.m[0][3] << "), ";
  os << "(" << v.m[1][0] << ", " << v.m[1][1] << ", " << v.m[1][2] << ", "
     << v.m[1][3] << ")";
  os << "(" << v.m[2][0] << ", " << v.m[2][1] << ", " << v.m[2][2] << ", "
     << v.m[2][3] << ")";
  os << "(" << v.m[3][0] << ", " << v.m[3][1] << ", " << v.m[3][2] << ", "
     << v.m[3][3] << ")";
  os << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const int2 &v) {
  os << "(" << v[0] << ", " << v[1] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const int3 &v) {
  os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const int4 &v) {
  os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const uint2 &v) {
  os << "(" << v[0] << ", " << v[1] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const uint3 &v) {
  os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const uint4 &v) {
  os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const half2 &v) {
  os << "(" << v[0] << ", " << v[1] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const half3 &v) {
  os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const half4 &v) {
  os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const float2 &v) {
  os << "(" << v[0] << ", " << v[1] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const float3 &v) {
  os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const float4 &v) {
  os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const double2 &v) {
  os << "(" << v[0] << ", " << v[1] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const double3 &v) {
  os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const double4 &v) {
  os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const vector3h &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const vector3f &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const vector3d &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const normal3h &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const normal3f &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const normal3d &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const point3h &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const point3f &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const point3d &v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const color3f &v) {
  os << "(" << v.r << ", " << v.g << ", " << v.b << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const color3d &v) {
  os << "(" << v.r << ", " << v.g << ", " << v.b << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const color4f &v) {
  os << "(" << v.r << ", " << v.g << ", " << v.b << ", " << v.a << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const color4d &v) {
  os << "(" << v.r << ", " << v.g << ", " << v.b << ", " << v.a << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const quath &v) {
  os << "(" << v.real << ", " << v.imag[0] << ", " << v.imag[1] << ", "
     << v.imag[2] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const quatf &v) {
  os << "(" << v.real << ", " << v.imag[0] << ", " << v.imag[1] << ", "
     << v.imag[2] << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const quatd &v) {
  os << "(" << v.real << ", " << v.imag[0] << ", " << v.imag[1] << ", "
     << v.imag[2] << ")";
  return os;
}

// Simple is_vector
template <typename>
struct is_vector : std::false_type {};
template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
  static_assert(!is_vector<T>::value, "T must NOT be std::vector");

  os << "[";
  for (size_t i = 0; i < v.size(); i++) {
    os << v[i];
    if (i != (v.size() - 1)) {
      os << ", ";
    }
  }
  os << "]";
  return os;
}

template <typename T>
std::ostream &operator<<(std::ostream &os,
                         const std::vector<std::vector<T>> &v) {
  os << "[";
  if (v.size() == 0) {
    os << "[]";
  } else {
    for (size_t i = 0; i < v.size(); i++) {
      os << v[i];  // std::vector<T>
      if (i != (v.size() - 1)) {
        os << ", ";
      }
    }
  }
  os << "]";
  return os;
}

std::ostream &operator<<(std::ostream &os, const TimeSample &ts) {
  os << "{";
  for (size_t i = 0; i < ts.times.size(); i++) {
    os << ts.times[i] << ": " << ts.values[i];

    if (i != (ts.times.size() - 1)) {
      os << ", ";
    }
  }
  os << "}";

  return os;
}

std::ostream &operator<<(std::ostream &os, const dict &v) {
  for (auto const &item : v) {
    static uint32_t cnt = 0;
    os << item.first << ":" << item.second;

    if (cnt != (v.size() - 1)) {
      os << ", ";
    }

    cnt++;
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const any_value &v) {
  // Simple brute-force way..
  // TODO: Use std::function or some template technique?

#define BASETYPE_CASE_EXPR(__ty)                      \
  case TypeTrait<__ty>::type_id: {                    \
    os << *reinterpret_cast<const __ty *>(v.value()); \
    break;                                            \
  }

#define ARRAY1DTYPE_CASE_EXPR(__ty)                                \
  case TypeTrait<std::vector<__ty>>::type_id: {                    \
    os << *reinterpret_cast<const std::vector<__ty> *>(v.value()); \
    break;                                                         \
  }

#define ARRAY2DTYPE_CASE_EXPR(__ty)                                  \
  case TypeTrait<std::vector<std::vector<__ty>>>::type_id: {         \
    os << *reinterpret_cast<const std::vector<std::vector<__ty>> *>( \
        v.value());                                                  \
    break;                                                           \
  }

#define CASE_EXR_LIST(__FUNC) \
  __FUNC(token)               \
  __FUNC(std::string)         \
  __FUNC(half)                \
  __FUNC(half2)               \
  __FUNC(half3)               \
  __FUNC(half4)               \
  __FUNC(int32_t)             \
  __FUNC(uint32_t)            \
  __FUNC(int2)                \
  __FUNC(int3)                \
  __FUNC(int4)                \
  __FUNC(uint2)               \
  __FUNC(uint3)               \
  __FUNC(uint4)               \
  __FUNC(int64_t)             \
  __FUNC(uint64_t)            \
  __FUNC(float)               \
  __FUNC(float2)              \
  __FUNC(float3)              \
  __FUNC(float4)              \
  __FUNC(double)              \
  __FUNC(double2)             \
  __FUNC(double3)             \
  __FUNC(double4)             \
  __FUNC(matrix2d)            \
  __FUNC(matrix3d)            \
  __FUNC(matrix4d)            \
  __FUNC(quath)               \
  __FUNC(quatf)               \
  __FUNC(quatd)               \
  __FUNC(normal3h)            \
  __FUNC(normal3f)            \
  __FUNC(normal3d)            \
  __FUNC(vector3h)            \
  __FUNC(vector3f)            \
  __FUNC(vector3d)            \
  __FUNC(point3h)             \
  __FUNC(point3f)             \
  __FUNC(point3d)             \
  __FUNC(color3f)             \
  __FUNC(color3d)             \
  __FUNC(color4f)             \
  __FUNC(color4d)

  switch (v.type_id()) {
    // no `bool` type for 1D and 2D array
    BASETYPE_CASE_EXPR(bool)

    // no std::vector<dict> and std::vector<std::vector<dict>>, ...
    BASETYPE_CASE_EXPR(dict)

    // base type
    CASE_EXR_LIST(BASETYPE_CASE_EXPR)

    // 1D array
    CASE_EXR_LIST(ARRAY1DTYPE_CASE_EXPR)

    // 2D array
    CASE_EXR_LIST(ARRAY2DTYPE_CASE_EXPR)

    // TODO: List-up all case and remove `default` clause.
    default: {
      os << "PPRINT: TODO: (type: " << v.type_name() << ") ";
    }
  }

#undef BASETYPE_CASE_EXPR
#undef ARRAY1DTYPE_CASE_EXPR
#undef ARRAY2DTYPE_CASE_EXPR
#undef CASE_EXPR_LIST

  return os;
}

std::ostream &operator<<(std::ostream &os, const Value &v) {
  os << v.v_;  // delegate to operator<<(os, any_value)
  return os;
}

//
// typecast from type_id
//
template <uint32_t tid>
struct typecast {};

#define TYPECAST_BASETYPE(__tid, __ty)                   \
  template <>                                            \
  struct typecast<__tid> {                               \
    static __ty to(const any_value &v) {                 \
      return *reinterpret_cast<const __ty *>(v.value()); \
    }                                                    \
  }

TYPECAST_BASETYPE(TYPE_ID_BOOL, bool);
TYPECAST_BASETYPE(TYPE_ID_UCHAR, uint8_t);
TYPECAST_BASETYPE(TYPE_ID_HALF, half);

TYPECAST_BASETYPE(TYPE_ID_UINT32, uint32_t);
TYPECAST_BASETYPE(TYPE_ID_FLOAT, float);
TYPECAST_BASETYPE(TYPE_ID_DOUBLE, double);

TYPECAST_BASETYPE(TYPE_ID_FLOAT | TYPE_ID_1D_ARRAY_BIT, std::vector<float>);

#undef TYPECAST_BASETYPE

// reconstruct

struct Mesh {
  std::vector<vector3f> vertices;
  std::vector<int32_t> indices;
};

static bool ReconstructVertrices(const any_value &v, Mesh &mesh) {
  if (v.type_id() == (TYPE_ID_VECTOR3F | TYPE_ID_1D_ARRAY_BIT)) {
    mesh.vertices = *reinterpret_cast<const std::vector<vector3f> *>(v.value());
    return true;
  }

  return false;
}

namespace staticstruct {

// -- 00conv

template <>
struct Converter<quath> {
  typedef std::array<uint16_t, 4> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            quath &value) {
    memcpy(&value.real, &shadow[0], sizeof(uint16_t) * 4);

    return nullptr;  // success
  }

  static void to_shadow(const quath &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value.real, sizeof(uint16_t) * 4);
  }
};

template <>
struct Converter<quatf> {
  typedef std::array<float, 4> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            quatf &value) {
    memcpy(&value.real, &shadow[0], sizeof(float) * 4);

    return nullptr;  // success
  }

  static void to_shadow(const quatf &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value.real, sizeof(float) * 4);
  }
};

template <>
struct Converter<quatd> {
  typedef std::array<double, 4> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            quatd &value) {
    memcpy(&value.real, &shadow[0], sizeof(double) * 4);

    return nullptr;  // success
  }

  static void to_shadow(const quatd &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value.real, sizeof(double) * 4);
  }
};

template <>
struct Converter<matrix2d> {
  typedef std::array<double, 4> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            matrix2d &value) {
    memcpy(&value.m[0][0], &shadow[0], sizeof(double) * 4);

    return nullptr;  // success
  }

  static void to_shadow(const matrix2d &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value.m[0][0], sizeof(double) * 4);
  }
};

template <>
struct Converter<matrix3d> {
  typedef std::array<double, 9> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            matrix3d &value) {
    memcpy(&value.m[0][0], &shadow[0], sizeof(double) * 9);

    return nullptr;  // success
  }

  static void to_shadow(const matrix3d &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value.m[0][0], sizeof(double) * 9);
  }
};

template <>
struct Converter<matrix4d> {
  typedef std::array<double, 16> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            matrix4d &value) {
    memcpy(&value.m[0][0], &shadow[0], sizeof(double) * 16);

    return nullptr;  // success
  }

  static void to_shadow(const matrix4d &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value.m[0][0], sizeof(double) * 16);
  }
};

template <>
struct Converter<vector3h> {
  typedef std::array<uint16_t, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            vector3h &value) {
    memcpy(&value, &shadow[0], sizeof(uint16_t) * 3);

    return nullptr;  // success
  }

  static void to_shadow(const vector3h &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value, sizeof(uint16_t) * 3);
  }
};

template <>
struct Converter<vector3f> {
  typedef std::array<float, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            vector3f &value) {
    value.x = shadow[0];
    value.y = shadow[1];
    value.z = shadow[2];

    return nullptr;  // success
  }

  static void to_shadow(const vector3f &value, shadow_type &shadow) {
    shadow[0] = value.x;
    shadow[1] = value.y;
    shadow[2] = value.z;
  }
};

template <>
struct Converter<vector3d> {
  typedef std::array<double, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            vector3d &value) {
    value.x = shadow[0];
    value.y = shadow[1];
    value.z = shadow[2];

    return nullptr;  // success
  }

  static void to_shadow(const vector3d &value, shadow_type &shadow) {
    shadow[0] = value.x;
    shadow[1] = value.y;
    shadow[2] = value.z;
  }
};

template <>
struct Converter<normal3h> {
  typedef std::array<uint16_t, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            normal3h &value) {
    memcpy(&value, &shadow[0], sizeof(uint16_t) * 3);

    return nullptr;  // success
  }

  static void to_shadow(const normal3h &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value, sizeof(uint16_t) * 3);
  }
};

template <>
struct Converter<normal3f> {
  typedef std::array<float, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            normal3f &value) {
    value.x = shadow[0];
    value.y = shadow[1];
    value.z = shadow[2];

    return nullptr;  // success
  }

  static void to_shadow(const normal3f &value, shadow_type &shadow) {
    shadow[0] = value.x;
    shadow[1] = value.y;
    shadow[2] = value.z;
  }
};

template <>
struct Converter<normal3d> {
  typedef std::array<double, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            normal3d &value) {
    value.x = shadow[0];
    value.y = shadow[1];
    value.z = shadow[2];

    return nullptr;  // success
  }

  static void to_shadow(const normal3d &value, shadow_type &shadow) {
    shadow[0] = value.x;
    shadow[1] = value.y;
    shadow[2] = value.z;
  }
};

template <>
struct Converter<point3h> {
  typedef std::array<uint16_t, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            point3h &value) {
    memcpy(&value, &shadow[0], sizeof(uint16_t) * 3);

    return nullptr;  // success
  }

  static void to_shadow(const point3h &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value, sizeof(uint16_t) * 3);
  }
};

template <>
struct Converter<point3f> {
  typedef std::array<float, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            point3f &value) {
    value.x = shadow[0];
    value.y = shadow[1];
    value.z = shadow[2];

    return nullptr;  // success
  }

  static void to_shadow(const point3f &value, shadow_type &shadow) {
    shadow[0] = value.x;
    shadow[1] = value.y;
    shadow[2] = value.z;
  }
};

template <>
struct Converter<point3d> {
  typedef std::array<double, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            point3d &value) {
    value.x = shadow[0];
    value.y = shadow[1];
    value.z = shadow[2];

    return nullptr;  // success
  }

  static void to_shadow(const point3d &value, shadow_type &shadow) {
    shadow[0] = value.x;
    shadow[1] = value.y;
    shadow[2] = value.z;
  }
};

template <>
struct Converter<color3f> {
  typedef std::array<float, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            color3f &value) {
    value.r = shadow[0];
    value.g = shadow[1];
    value.b = shadow[2];

    return nullptr;  // success
  }

  static void to_shadow(const color3f &value, shadow_type &shadow) {
    shadow[0] = value.r;
    shadow[1] = value.g;
    shadow[2] = value.b;
  }
};

template <>
struct Converter<color3d> {
  typedef std::array<double, 3> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            color3d &value) {
    value.r = shadow[0];
    value.g = shadow[1];
    value.b = shadow[2];

    return nullptr;  // success
  }

  static void to_shadow(const color3d &value, shadow_type &shadow) {
    shadow[0] = value.r;
    shadow[1] = value.g;
    shadow[2] = value.b;
  }
};

template <>
struct Converter<color4f> {
  typedef std::array<float, 4> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            color4f &value) {
    value.r = shadow[0];
    value.g = shadow[1];
    value.b = shadow[2];
    value.a = shadow[3];

    return nullptr;  // success
  }

  static void to_shadow(const color4f &value, shadow_type &shadow) {
    shadow[0] = value.r;
    shadow[1] = value.g;
    shadow[2] = value.b;
    shadow[3] = value.a;
  }
};

template <>
struct Converter<color4d> {
  typedef std::array<double, 4> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            color4d &value) {
    value.r = shadow[0];
    value.g = shadow[1];
    value.b = shadow[2];
    value.a = shadow[2];

    return nullptr;  // success
  }

  static void to_shadow(const color4d &value, shadow_type &shadow) {
    shadow[0] = value.r;
    shadow[1] = value.g;
    shadow[2] = value.b;
    shadow[3] = value.a;
  }
};

}  // namespace staticstruct

struct AttribMap {
  std::map<std::string, any_value> attribs;
};

class Register {
 public:
  Register() = default;

  template <class T>
  Register &property(std::string name, T *pointer,
                     uint32_t flags = staticstruct::Flags::Default) {
    h.add_property(name, pointer, flags, TypeTrait<T>::type_id);

    return *this;
  }

  bool reconstruct(AttribMap &amap) {
    err_.clear();

    staticstruct::Reader r;

#define CONVERT_TYPE_SCALAR(__ty, __value)       \
  case TypeTrait<__ty>::type_id: {               \
    __ty *p = reinterpret_cast<__ty *>(__value); \
    staticstruct::Handler<__ty> _h(p);           \
    return _h.write(&handler);                   \
  }

#define CONVERT_TYPE_1D(__ty, __value)                                     \
  case (TypeTrait<__ty>::type_id | TYPE_ID_1D_ARRAY_BIT): {                \
    std::vector<__ty> *p = reinterpret_cast<std::vector<__ty> *>(__value); \
    staticstruct::Handler<std::vector<__ty>> _h(p);                        \
    return _h.write(&handler);                                             \
  }

#define CONVERT_TYPE_2D(__ty, __value)                               \
  case (TypeTrait<__ty>::type_id | TYPE_ID_2D_ARRAY_BIT): {          \
    std::vector<std::vector<__ty>> *p =                              \
        reinterpret_cast<std::vector<std::vector<__ty>> *>(__value); \
    staticstruct::Handler<std::vector<std::vector<__ty>>> _h(p);     \
    return _h.write(&handler);                                       \
  }

#define CONVERT_TYPE_LIST(__FUNC) \
  __FUNC(half, v)                 \
  __FUNC(half2, v)                \
  __FUNC(half3, v)                \
  __FUNC(half4, v)                \
  __FUNC(int32_t, v)              \
  __FUNC(uint32_t, v)             \
  __FUNC(int2, v)                 \
  __FUNC(int3, v)                 \
  __FUNC(int4, v)                 \
  __FUNC(uint2, v)                \
  __FUNC(uint3, v)                \
  __FUNC(uint4, v)                \
  __FUNC(int64_t, v)              \
  __FUNC(uint64_t, v)             \
  __FUNC(float, v)                \
  __FUNC(float2, v)               \
  __FUNC(float3, v)               \
  __FUNC(float4, v)               \
  __FUNC(double, v)               \
  __FUNC(double2, v)              \
  __FUNC(double3, v)              \
  __FUNC(double4, v)              \
  __FUNC(quath, v)                \
  __FUNC(quatf, v)                \
  __FUNC(quatd, v)                \
  __FUNC(vector3h, v)             \
  __FUNC(vector3f, v)             \
  __FUNC(vector3d, v)             \
  __FUNC(normal3h, v)             \
  __FUNC(normal3f, v)             \
  __FUNC(normal3d, v)             \
  __FUNC(point3h, v)              \
  __FUNC(point3f, v)              \
  __FUNC(point3d, v)              \
  __FUNC(color3f, v)              \
  __FUNC(color3d, v)              \
  __FUNC(color4f, v)              \
  __FUNC(color4d, v)              \
  __FUNC(matrix2d, v)             \
  __FUNC(matrix3d, v)             \
  __FUNC(matrix4d, v)

    bool ret = r.ParseStruct(
        &h,
        [&amap](std::string key, uint32_t flags, uint32_t user_type_id,
                staticstruct::BaseHandler &handler) -> bool {
          std::cout << "key = " << key
                    << ", count = " << amap.attribs.count(key) << "\n";

          if (!amap.attribs.count(key)) {
            if (flags & staticstruct::Flags::Optional) {
              return true;
            } else {
              return false;
            }
          }

          auto &value = amap.attribs[key];
          if (amap.attribs[key].type_id() == user_type_id) {
            void *v = value.value();

            switch (user_type_id) {
              CONVERT_TYPE_SCALAR(bool, v)

              CONVERT_TYPE_LIST(CONVERT_TYPE_SCALAR)
              CONVERT_TYPE_LIST(CONVERT_TYPE_1D)
              CONVERT_TYPE_LIST(CONVERT_TYPE_2D)

              default: {
                std::cerr << "Unsupported type: " << GetTypeName(user_type_id)
                          << "\n";
                return false;
              }
            }
          } else {
            std::cerr << "type: " << amap.attribs[key].type_name() << "(a.k.a "
                      << amap.attribs[key].underlying_type_name()
                      << ") expected but got " << GetTypeName(user_type_id)
                      << " for attribute \"" << key << "\"\n";
            return false;
          }
        },
        &err_);

    return ret;
  }

#undef CONVERT_TYPE_SCALAR
#undef CONVERT_TYPE_1D
#undef CONVERT_TYPE_2D
#undef CONVERT_TYPE_LIST

  std::string get_error() const { return err_; }

 private:
  staticstruct::ObjectHandler h;
  std::string err_;
};

static bool ReconstructAttribTest0() {
  Mesh mesh;
  Register r;

  r.property("vertices", &mesh.vertices).property("indices", &mesh.indices);

  AttribMap amap;
  amap.attribs["vertices"] =
      std::vector<vector3f>({{1.0f, 2.0f, 3.0f}, {0.5f, 2.1f, 4.3f}});
  amap.attribs["indices"] =
      std::vector<vector3f>({{1.0f, 2.0f, 3.0f}, {0.5f, 2.1f, 4.3f}});

  bool ret = r.reconstruct(amap);

  if (!ret) {
    std::cerr << r.get_error() << "\n";
  }

  return ret;
}

static bool ReconstructAttribTest() {
  AttribMap amap;
  amap.attribs["vertices"] =
      std::vector<vector3f>({{1.0f, 2.0f, 3.0f}, {0.5f, 2.1f, 4.3f}});

  Mesh mesh;

  std::cout << "mesh.vertices typename = "
            << TypeTrait<decltype(mesh.vertices)>::type_name() << "\n";

  staticstruct::ObjectHandler h;
  h.add_property("vertices", &mesh.vertices, 0,
                 TypeTrait<decltype(mesh.vertices)>::type_id);

  staticstruct::Reader r;
  std::string err;
  bool ret = r.ParseStruct(
      &h,
      [&amap](std::string key, uint32_t flags, uint32_t user_type_id,
              staticstruct::BaseHandler &handler) -> bool {
        std::cout << "key = " << key << ", count = " << amap.attribs.count(key)
                  << "\n";

        if (!amap.attribs.count(key)) {
          if (flags & staticstruct::Flags::Optional) {
            return true;
          } else {
            return false;
          }
        }

        auto &value = amap.attribs[key];
        if (amap.attribs[key].type_id() == user_type_id) {
          if (user_type_id == (TYPE_ID_VECTOR3F | TYPE_ID_1D_ARRAY_BIT)) {
            std::vector<float3> *p =
                reinterpret_cast<std::vector<float3> *>(value.value());
            staticstruct::Handler<std::vector<float3>> _h(p);
            return _h.write(&handler);
          } else {
            std::cerr << "Unsupported type: " << GetTypeName(user_type_id)
                      << "\n";
            return false;
          }
        } else {
          std::cerr << "type: " << amap.attribs[key].type_name() << "(a.k.a "
                    << amap.attribs[key].underlying_type_name()
                    << ") expected but got " << GetTypeName(user_type_id)
                    << " for attribute \"" << key << "\"\n";
          return false;
        }
      },
      &err);

  if (!ret) {
    if (!err.empty()) {
      std::cerr << "Attrib reconstruction failed. ERR: " << err << "\n";
    }
  }

  std::cout << mesh.vertices << "\n";

  return ret;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  {
    any_value f = 1.2f;
    float a = typecast<TYPE_ID_FLOAT>::to(f);
    std::cout << "a = " << a << "\n";

    f = double(4.5);
    double b = typecast<TYPE_ID_DOUBLE>::to(f);
    std::cout << "b = " << b << "\n";

    std::vector<float> v = {1.0f, 2.0f};
    f = v;
    auto c = typecast<TYPE_ID_FLOAT | TYPE_ID_1D_ARRAY_BIT>::to(f);
    std::cout << "c = " << c << "\n";
  }

  {
    bool ok = ReconstructAttribTest0();
    std::cout << "ReconstructAttribTest0: " << ok << "\n";
  }

  bool ok = ReconstructAttribTest();
  std::cout << "ReconstructAttribTest: " << ok << "\n";

  {
    Mesh mesh;
    staticstruct::ObjectHandler h;
    h.add_property("vertices", &mesh.vertices, 0,
                   TYPE_ID_VECTOR3F | TYPE_ID_1D_ARRAY_BIT);
    staticstruct::Reader r;
    std::string err;
    bool ret = r.ParseStruct(
        &h,
        [](std::string key, uint32_t flags, uint32_t user_type_id,
           staticstruct::BaseHandler &handler) -> bool {
          (void)flags;
          (void)key;
          (void)user_type_id;
          (void)handler;
          return false;
        },
        &err);

    if (!ret) {
      std::cout << "reconstruct failed\n";
    }
  }

  {
    any_value a(4.2f);

    float fval = a;

    std::cout << "fval = " << fval << "\n";
  }

  // std::cout << "sizeof(U) = " << sizeof(Value::U) << "\n";

  // std::map<int, TypeTrait<T>> bora;

  dict o;
  o["muda"] = 1.3;

  Value v;

  v = 1.3f;

  std::cout << "val\n";
  std::cout << v << "\n";

  v = 1.3;

  std::cout << "val\n";
  std::cout << v << "\n";

  std::vector<float> din = {1.0, 2.0};
  v = din;

  {
    std::vector<float3> vs = {{1.0, 2.0, 3.0}, {0.32f, 0.21f, 1.3f}};
    any_value val(vs);
    Mesh mesh;

    if (ReconstructVertrices(val, mesh)) {
      std::cout << "Reconstruct mesh.vertices ok\n";
    } else {
      std::cout << "Reconstruct mesh.vertices failed\n";
    }
  }

  std::cout << "val\n";
  std::cout << v << "\n";

  std::vector<std::vector<float>> din2 = {{1.0, 2.0}, {3.0, 4.0}};
  v = din2;
  std::cout << "val\n"
            << "vty: " << v.type_name() << "\n";
  std::cout << v << "\n";

  std::vector<int> vids = {1, 2, 3};
  v = vids;

  v = o;

  std::cout << "val\n";
  std::cout << v << "\n";

  auto ret = v.get_value<double>();
  if (ret) {
    std::cout << "double!\n";
  }

  v = 1.2;
  ret = v.get_value<double>();
  if (ret) {
    std::cout << "double!\n";
  }

  {
    std::vector<std::vector<float>> vvf;
    std::cout << vvf << "\n";

    vvf = {{1.0}, {2.0, 3.0}};
    std::cout << vvf << "\n";

    v = vvf;
    std::cout << v << "\n";
  }

#if 0
  if (v.get_if<double>()) {
    std::cout << "double!" << "\n";
  }
#endif
}

static_assert(sizeof(half) == 2, "sizeof(half) must be 2");
static_assert(sizeof(float3) == 12, "sizeof(float3) must be 12");
static_assert(sizeof(color3f) == 12, "sizeof(color3f) must be 12");
static_assert(sizeof(color4f) == 16, "sizeof(color4f) must be 16");
