#if 0 // TODO: Remove
/// Need to define in header file.
namespace staticstruct {

using namespace tinyusdz::value;

// -- For Reconstructor

template <>
struct Converter<half> {
  typedef uint16_t shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            half &value) {
    value.value = shadow;

    return nullptr;  // success
  }

  static void to_shadow(const half &value, shadow_type &shadow) {
    shadow = value.value;
  }
};

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
struct Converter<matrix2f> {
  typedef std::array<float, 4> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            matrix2f &value) {
    memcpy(&value.m[0][0], &shadow[0], sizeof(float) * 4);

    return nullptr;  // success
  }

  static void to_shadow(const matrix2f &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value.m[0][0], sizeof(float) * 4);
  }
};

template <>
struct Converter<matrix3f> {
  typedef std::array<float, 9> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            matrix3f &value) {
    memcpy(&value.m[0][0], &shadow[0], sizeof(float) * 9);

    return nullptr;  // success
  }

  static void to_shadow(const matrix3f &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value.m[0][0], sizeof(float) * 9);
  }
};

template <>
struct Converter<matrix4f> {
  typedef std::array<float, 16> shadow_type;

  static std::unique_ptr<Error> from_shadow(const shadow_type &shadow,
                                            matrix4f &value) {
    memcpy(&value.m[0][0], &shadow[0], sizeof(float) * 16);

    return nullptr;  // success
  }

  static void to_shadow(const matrix4f &value, shadow_type &shadow) {
    memcpy(&shadow[0], &value.m[0][0], sizeof(float) * 16);
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
#endif

#if 0 // TODO: Remove
//
// Concrete struct reconstruction from AttribMap
//
class Reconstructor {
 public:
  Reconstructor() = default;

  template <class T>
  Reconstructor &property(std::string name, T *pointer,
                     uint32_t flags = staticstruct::Flags::Default) {
    h.add_property(name, pointer, flags, TypeTrait<T>::type_id);

    return *this;
  }

  bool reconstruct(AttribMap &amap);

  std::string get_error() const { return err_; }

 private:
  staticstruct::ObjectHandler h;
  std::string err_;
};
#endif

