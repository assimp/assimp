#if 0
base_value::~base_value() {}


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

//std::ostream &operator<<(std::ostream &os, const Value &v) {
//  os << v.get_raw();  // delegate to operator<<(os, any_value)
//  return os;
//}

bool is_float(const any_value &v) {
  if (v.underlying_type_name() == "float") {
    return true;
  }

  return false;
}

bool is_double(const any_value &v) {
  if (v.underlying_type_name() == "double") {
    return true;
  }

  return false;
}

bool is_float(const Value &v) {
  if (v.underlying_type_name() == "float") {
    return true;
  }

  return false;
}

bool is_float2(const Value &v) {
  if (v.underlying_type_name() == "float2") {
    return true;
  }

  return false;
}

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

bool is_double(const Value &v) {
  if (v.underlying_type_name() == "double") {
    return true;
  }

  return false;
}

bool is_double2(const Value &v) {
  if (v.underlying_type_name() == "double2") {
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

#if 0
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

std::ostream &operator<<(std::ostream &os, const texcoord2f &v) {
  os << "(" << v.s << ", " << v.t << ")";
  return os;
}
#endif

bool Reconstructor::reconstruct(AttribMap &amap) {
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

#undef CONVERT_TYPE_SCALAR
#undef CONVERT_TYPE_1D
#undef CONVERT_TYPE_2D
#undef CONVERT_TYPE_LIST
}

std::string GetTypeName(uint32_t tyid) {
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
    m[TYPE_ID_POINT3H] = TypeTrait<point3h>::type_name();
    m[TYPE_ID_POINT3F] = TypeTrait<point3f>::type_name();
    m[TYPE_ID_POINT3D] = TypeTrait<point3d>::type_name();
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

    m[TYPE_ID_POINT3H | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<point3h>>::type_name();
    m[TYPE_ID_POINT3F | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<point3f>>::type_name();
    m[TYPE_ID_POINT3D | TYPE_ID_1D_ARRAY_BIT] =
        TypeTrait<std::vector<point3d>>::type_name();

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
#endif
