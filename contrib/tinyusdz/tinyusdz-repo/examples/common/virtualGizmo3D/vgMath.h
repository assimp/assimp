//------------------------------------------------------------------------------
//  Copyright (c) 2018-2020 Michele Morrone
//  All rights reserved.
//
//  https://michelemorrone.eu - https://BrutPitt.com
//
//  twitter: https://twitter.com/BrutPitt - github: https://github.com/BrutPitt
//
//  mailto:brutpitt@gmail.com - mailto:me@michelemorrone.eu
//  
//  This software is distributed under the terms of the BSD 2-Clause license
//------------------------------------------------------------------------------
#pragma once

#include "vgConfig.h"

#ifdef VGM_USES_DOUBLE_PRECISION
    #define VG_T_TYPE double
    #define VGM_USES_TEMPLATE
#else
    #define VG_T_TYPE float
#endif

#ifdef VGIZMO_USES_GLM
    #ifndef VGM_USES_TEMPLATE
        #define VGM_USES_TEMPLATE    // glm uses template ==> vGizmo needs to know
    #endif

    #define VGM_NAMESPACE glm

    #include <glm/glm.hpp>
    #include <glm/gtx/vector_angle.hpp>
    #include <glm/gtx/exterior_product.hpp>
    #include <glm/gtc/type_ptr.hpp>
    #include <glm/gtc/quaternion.hpp>
    #include <glm/gtc/matrix_transform.hpp>

    using tVec2 = glm::tvec2<VG_T_TYPE>;
    using tVec3 = glm::tvec3<VG_T_TYPE>;
    using tVec4 = glm::tvec4<VG_T_TYPE>;
    using tQuat = glm::tquat<VG_T_TYPE>;
    using tMat3 = glm::tmat3x3<VG_T_TYPE>;
    using tMat4 = glm::tmat4x4<VG_T_TYPE>;

    #define T_PI glm::pi<VG_T_TYPE>()
    #define T_INV_PI glm::one_over_pi<VG_T_TYPE>()

    #define VGIZMO_BASE_CLASS virtualGizmoBaseClass<T>
    #define TEMPLATE_TYPENAME_T  template<typename T>

#else // use vGizmoMath

    #include <math.h>
    #include <cmath>
    #include <stdint.h>
    #define VGM_NAMESPACE vgm

    #ifdef VGM_USES_TEMPLATE
        #define TEMPLATE_TYPENAME_T  template<typename T>

        #define VEC2_T Vec2<T>
        #define VEC3_T Vec3<T>
        #define VEC4_T Vec4<T>
        #define QUAT_T Quat<T>
        #define MAT3_T Mat3<T>
        #define MAT4_T Mat4<T>

        #define VEC2_PRECISION Vec2<VG_T_TYPE>
        #define VEC3_PRECISION Vec3<VG_T_TYPE>
        #define VEC4_PRECISION Vec4<VG_T_TYPE>
        #define QUAT_PRECISION Quat<VG_T_TYPE>
        #define MAT3_PRECISION Mat3<VG_T_TYPE>
        #define MAT4_PRECISION Mat4<VG_T_TYPE>
        
        #define T_PI vgm::pi<VG_T_TYPE>()
        #define T_INV_PI vgm::one_over_pi<VG_T_TYPE>()
    
    #else
        #define TEMPLATE_TYPENAME_T

        #define VEC2_T Vec2
        #define VEC3_T Vec3
        #define VEC4_T Vec4
        #define QUAT_T Quat
        #define MAT3_T Mat3
        #define MAT4_T Mat4

        #define VEC2_PRECISION Vec2
        #define VEC3_PRECISION Vec3
        #define VEC4_PRECISION Vec4
        #define QUAT_PRECISION Quat
        #define MAT3_PRECISION Mat3
        #define MAT4_PRECISION Mat4

        #define T_PI vgm::pi()
        #define T_INV_PI vgm::one_over_pi()
    #endif

namespace vgm {

TEMPLATE_TYPENAME_T class Vec3;
TEMPLATE_TYPENAME_T class Vec4;
TEMPLATE_TYPENAME_T class Mat4;

#if !defined(VGM_USES_TEMPLATE)
    #define T VG_T_TYPE
#endif

// Vec2
//////////////////////////
TEMPLATE_TYPENAME_T class Vec2 {
public:
    union {
        struct { T x, y; };
        struct { T u, v; };
    };

    Vec2()              = default;
    Vec2(const VEC2_T&) = default;
    explicit Vec2(T s)  : x(s), y(s) {}
    Vec2(T x, T y)      : x(x), y(y) {}
    Vec2(const VEC3_T&);

    Vec2 operator-() const { return {-x, -y}; }

    Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
    Vec2& operator-=(const Vec2& v) { x -= v.x; y -= v.y; return *this; }
    Vec2& operator*=(const Vec2& v) { x *= v.x; y *= v.y; return *this; }
    Vec2& operator/=(const Vec2& v) { x /= v.x; y /= v.y; return *this; }
    Vec2& operator*=(T s)           { x *= s  ; y *= s  ; return *this; }
    Vec2& operator/=(T s)           { x /= s  ; y /= s  ; return *this; }

    Vec2 operator+(const Vec2& v) const { return { x + v.x, y + v.y }; }
    Vec2 operator-(const Vec2& v) const { return { x - v.x, y - v.y }; }
    Vec2 operator*(const Vec2& v) const { return { x * v.x, y * v.y }; }
    Vec2 operator/(const Vec2& v) const { return { x / v.x, y / v.y }; }
    Vec2 operator*(T s)           const { return { x * s  , y * s   }; }
    Vec2 operator/(T s)           const { return { x / s  , y / s   }; }

    const T& operator[](int i) const { return *(&x + i); }
          T& operator[](int i)       { return *(&x + i); }

    explicit operator const T *() const { return &x; }
    explicit operator       T *()       { return &x; }
};
// Vec3
//////////////////////////
TEMPLATE_TYPENAME_T class Vec3 {
public:
    union {
        struct { T x, y, z; };
        struct { T r, g, b; };
    };

    Vec3()                              = default;
    Vec3(const VEC3_T&)                 = default;
    explicit Vec3(T s)                  : x(s), y(s), z(s)      {}
    Vec3(T x, T y, T z)                 : x(x), y(y), z(z)      {}
    explicit Vec3(T s, const VEC2_T& v) : x(s), y(v.x), z(v.y)  {}
    explicit Vec3(const VEC2_T& v, T s) : x(v.x), y(v.y), z(s)  {}
    Vec3(const VEC4_T& v);

    Vec3 operator-() const { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vec3& operator*=(const Vec3& v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
    Vec3& operator/=(const Vec3& v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
    Vec3& operator*=(T s)           { x *= s  ; y *= s  ; z *= s  ; return *this; }
    Vec3& operator/=(T s)           { x /= s  ; y /= s  ; z /= s  ; return *this; }

    Vec3 operator+(const Vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    Vec3 operator-(const Vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    Vec3 operator*(const Vec3& v) const { return { x * v.x, y * v.y, z * v.z }; }
    Vec3 operator/(const Vec3& v) const { return { x / v.x, y / v.y, z / v.z }; }
    Vec3 operator*(T s)           const { return { x * s  , y * s  , z * s   }; }
    Vec3 operator/(T s)           const { return { x / s  , y / s  , z / s   }; }

    const T& operator[](int i) const { return *(&x + i); }
          T& operator[](int i)       { return *(&x + i); }

    explicit operator const T *() const { return &x; }
    explicit operator       T *()       { return &x; }
};
// Vec4
//////////////////////////
TEMPLATE_TYPENAME_T class Vec4 {
public:
    union {
        struct { T x, y, z, w; };
        struct { T r, g, b, a; };
    };

    Vec4()                              = default;
    Vec4(const VEC4_T&)                 = default;
    explicit Vec4(T s)                  : x(s),   y(s),   z(s),   w(s)   {}
    Vec4(T x, T y, T z, T w)            : x(x),   y(y),   z(z),   w(w)   {}
    explicit Vec4(const VEC3_T& v, T s) : x(v.x), y(v.y), z(v.z), w(s)   {}
    explicit Vec4(T s, const VEC3_T& v) : x(s),   y(v.x), z(v.y), w(v.z) {}

    //operator VEC3_T() const { return *((VEC3_T *) &x); }
    Vec4 operator-() const { return {-x, -y, -z, -w}; }
    
    Vec4& operator+=(const Vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    Vec4& operator-=(const Vec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    Vec4& operator*=(const Vec4& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    Vec4& operator/=(const Vec4& v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }
    Vec4& operator*=(T s)           { x *= s  ; y *= s  ; z *= s  ; w *= s  ; return *this; }
    Vec4& operator/=(T s)           { x /= s  ; y /= s  ; z /= s  ; w /= s  ; return *this; }

    Vec4 operator+(const Vec4& v) const { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
    Vec4 operator-(const Vec4& v) const { return { x - v.x, y - v.y, z - v.z, w - v.w }; }
    Vec4 operator*(const Vec4& v) const { return { x * v.x, y * v.y, z * v.z, w * v.w }; }
    Vec4 operator/(const Vec4& v) const { return { x / v.x, y / v.y, z / v.z, w / v.w }; }
    Vec4 operator*(T s)           const { return { x * s  , y * s  , z * s  , w * s   }; }
    Vec4 operator/(T s)           const { return { x / s  , y / s  , z / s  , w / s   }; }

    const T& operator[](int i) const { return *(&x + i); }
          T& operator[](int i)       { return *(&x + i); }

    explicit operator const T *() const { return &x; }
    explicit operator       T *()       { return &x; }
};
// Quat
//////////////////////////
TEMPLATE_TYPENAME_T class Quat {
public:
    T x, y, z, w;

    Quat()                              = default;
    Quat(const QUAT_T&)                 = default;
    Quat(T w, T x, T y, T z)            : x(x),   y(y),   z(z),   w(w)   {}
    explicit Quat(T s, const VEC3_T& v) : x(v.x), y(v.y), z(v.z), w(s)   {}

    Quat operator-() const { return Quat(-w, -x, -y, -z); }

    Quat& operator+=(const Quat& q)  { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
    Quat& operator-=(const Quat& q)  { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
    Quat& operator*=(const Quat& q)  { return *this = *this * q; }
    Quat& operator*=(T s)            { x *= s  ; y *= s  ; z *= s  ; w *= s  ; return *this; }
    Quat& operator/=(T s)            { x /= s  ; y /= s  ; z /= s  ; w /= s  ; return *this; }

    Quat operator+(const Quat& q) const { return { w + q.w, x + q.x, y + q.y, z + q.z }; }
    Quat operator-(const Quat& q) const { return { w - q.w, x - q.x, y - q.y, z - q.z }; }
    Quat operator*(const Quat& q) const { return { w * q.w - x * q.x - y * q.y - z * q.z,
                                                   w * q.x + x * q.w + y * q.z - z * q.y,
                                                   w * q.y + y * q.w + z * q.x - x * q.z,
                                                   w * q.z + z * q.w + x * q.y - y * q.x }; }
                                                
    Quat operator*(T s) const { return { w * s, x * s  , y * s  , z * s }; }
    Quat operator/(T s) const { return { w / s, x / s  , y / s  , z / s }; }

    const T& operator[](int i) const { return *(&x + i); }
          T& operator[](int i)       { return *(&x + i); }

    explicit operator const T *() const { return &x; }
    explicit operator       T *()       { return &x; }
};
// Mat3
//////////////////////////
TEMPLATE_TYPENAME_T class Mat3 {
public:
    union {
        VEC3_T v[3];
        struct { T m00, m01, m02,
                   m10, m11, m12,
                   m20, m21, m22; };
    };

    Mat3()                  = default;
    Mat3(const MAT3_T &)    = default;
    explicit Mat3(T s) : v { VEC3_T(s, 0, 0), VEC3_T(0, s, 0), VEC3_T(0, 0, s) } {}
    Mat3(const VEC3_T& v0, const VEC3_T& v1, const VEC3_T& v2) : v {v0, v1, v2 } {}
    Mat3(const MAT4_T& m);
    Mat3(T v0x, T v0y, T v0z,
         T v1x, T v1y, T v1z,
         T v2x, T v2y, T v2z) : v { VEC3_T(v0x, v0y, v0z), VEC3_T(v1x, v1y, v1z), VEC3_T(v2x, v2y, v2z) } {}

    const VEC3_T& operator[](int i) const { return v[i]; }
          VEC3_T& operator[](int i)       { return v[i]; }

    Mat3 operator-() const { return Mat3(-v[0], -v[1], -v[2]); }
    
    Mat3& operator+=(const Mat3& m) { v[0] += m.v[0]; v[1] += m.v[1]; v[2] += m.v[2]; return *this; }
    Mat3& operator-=(const Mat3& m) { v[0] -= m.v[0]; v[1] -= m.v[1]; v[2] -= m.v[2]; return *this; }
    Mat3& operator/=(const Mat3& m) { v[0] /= m.v[0]; v[1] /= m.v[1]; v[2] /= m.v[2]; return *this; }
    Mat3& operator*=(T s)           { v[0] *= s;      v[1] *= s;      v[2] *= s;      return *this; }
    Mat3& operator/=(T s)           { v[0] /= s;      v[1] /= s;      v[2] /= s;      return *this; }
    Mat3& operator*=(const Mat3& m) { return *this = *this * m;  }

    Mat3 operator+(const Mat3& m) const { return { v[0] + m.v[0], v[1] + m.v[1], v[2] + m.v[2] }; }
    Mat3 operator-(const Mat3& m) const { return { v[0] - m.v[0], v[1] - m.v[1], v[2] - m.v[2] }; }
#define M(X,Y) (m##X * m.m##Y)
    Mat3 operator*(const Mat3& m) const { return { M(00,00) + M(10,01) + M(20,02),
                                                   M(01,00) + M(11,01) + M(21,02),
                                                   M(02,00) + M(12,01) + M(22,02),
                                                   M(00,10) + M(10,11) + M(20,12),
                                                   M(01,10) + M(11,11) + M(21,12),
                                                   M(02,10) + M(12,11) + M(22,12),
                                                   M(00,20) + M(10,21) + M(20,22),
                                                   M(01,20) + M(11,21) + M(21,22),
                                                   M(02,20) + M(12,21) + M(22,22)}; }
#undef M
    Mat3 operator*(T s) const { return { v[0] * s, v[1] * s, v[2] * s }; }
    Mat3 operator/(T s) const { return { v[0] / s, v[1] / s, v[2] / s }; }

    VEC3_T operator*(const VEC3_T& v) const { return { m00 * v.x + m10 * v.y + m20 * v.z,
                                                       m01 * v.x + m11 * v.y + m21 * v.z,
                                                       m02 * v.x + m12 * v.y + m22 * v.z }; }
    explicit operator const T *() const { return &m00; }
    explicit operator       T *()       { return &m00; }
};
// Mat4
//////////////////////////
TEMPLATE_TYPENAME_T class Mat4 {
public:
    union {
        VEC4_T v[4];
        struct { T m00, m01, m02, m03,
                   m10, m11, m12, m13,
                   m20, m21, m22, m23,
                   m30, m31, m32, m33; };
    };

    Mat4() = default;
    explicit Mat4(T s) : v { VEC4_T(s, 0, 0, 0), VEC4_T(0, s, 0, 0), VEC4_T(0, 0, s, 0), VEC4_T(0, 0, 0, s)} {}
    Mat4(const VEC4_T& v0, const VEC4_T& v1, const VEC4_T& v2, const VEC4_T& v3) : v {v0, v1, v2, v3} {}
    Mat4(const MAT3_T& m) : v {VEC4_T(m.v[0],0), VEC4_T(m.v[1],0), VEC4_T(m.v[2],0), VEC4_T(0, 0, 0, 1)}  {}
    Mat4(T v0x, T v0y, T v0z, T v0w,
         T v1x, T v1y, T v1z, T v1w,
         T v2x, T v2y, T v2z, T v2w,
         T v3x, T v3y, T v3z, T v3w) : v {VEC4_T(v0x, v0y, v0z, v0w), VEC4_T(v1x, v1y, v1z, v1w), VEC4_T(v2x, v2y, v2z, v2w), VEC4_T(v3x, v3y, v3z, v3w) } {}

    const VEC4_T& operator[](int i) const { return v[i]; }
          VEC4_T& operator[](int i)       { return v[i]; }

    Mat4 operator-() const { return { -v[0], -v[1], -v[2], -v[3] }; }

    Mat4& operator+=(const Mat4& m) { v[0] += m.v[0]; v[1] += m.v[1]; v[2] += m.v[2]; v[3] += m.v[3]; return *this; }
    Mat4& operator-=(const Mat4& m) { v[0] -= m.v[0]; v[1] -= m.v[1]; v[2] -= m.v[2]; v[3] -= m.v[3]; return *this; }
    Mat4& operator/=(const Mat4& m) { v[0] /= m.v[0]; v[1] /= m.v[1]; v[2] /= m.v[2]; v[3] /= m.v[3]; return *this; }
    Mat4& operator*=(T s)           { v[0] *= s;      v[1] *= s;      v[2] *= s;      v[3] *= s;      return *this; }
    Mat4& operator/=(T s)           { v[0] /= s;      v[1] /= s;      v[2] /= s;      v[3] /= s;      return *this; }
    Mat4& operator*=(const Mat4& m) { return *this = *this * m; }

    Mat4 operator+(const Mat4& m) const { return { v[0] + m.v[0], v[1] + m.v[1], v[2] + m.v[2], v[3] + m.v[3] }; }
    Mat4 operator-(const Mat4& m) const { return { v[0] - m.v[0], v[1] - m.v[1], v[2] - m.v[2], v[3] - m.v[3] }; }
    Mat4 operator*(T s)           const { return { v[0] * s     , v[1] * s     , v[2] * s     , v[3] * s      }; }
    Mat4 operator/(T s)           const { return { v[0] / s     , v[1] / s     , v[2] / s     , v[3] / s      }; }
#define M(X,Y) (m##X * m.m##Y)
    Mat4 operator*(const Mat4& m) const { return { M(00,00) + M(10,01) + M(20,02) + M(30,03),
                                                   M(01,00) + M(11,01) + M(21,02) + M(31,03),
                                                   M(02,00) + M(12,01) + M(22,02) + M(32,03),
                                                   M(03,00) + M(13,01) + M(23,02) + M(33,03),
                                                   M(00,10) + M(10,11) + M(20,12) + M(30,13),
                                                   M(01,10) + M(11,11) + M(21,12) + M(31,13),
                                                   M(02,10) + M(12,11) + M(22,12) + M(32,13),
                                                   M(03,10) + M(13,11) + M(23,12) + M(33,13),
                                                   M(00,20) + M(10,21) + M(20,22) + M(30,23),
                                                   M(01,20) + M(11,21) + M(21,22) + M(31,23),
                                                   M(02,20) + M(12,21) + M(22,22) + M(32,23),
                                                   M(03,20) + M(13,21) + M(23,22) + M(33,23),
                                                   M(00,30) + M(10,31) + M(20,32) + M(30,33),
                                                   M(01,30) + M(11,31) + M(21,32) + M(31,33),
                                                   M(02,30) + M(12,31) + M(22,32) + M(32,33),
                                                   M(03,30) + M(13,31) + M(23,32) + M(33,33) };  }
#undef M
    VEC4_T operator*(const VEC4_T& v) const { return { m00 * v.x + m10 * v.y + m20 * v.z + m30 * v.w,
                                                       m01 * v.x + m11 * v.y + m21 * v.z + m31 * v.w,
                                                       m02 * v.x + m12 * v.y + m22 * v.z + m32 * v.w,
                                                       m03 * v.x + m13 * v.y + m23 * v.z + m33 * v.w }; }
    explicit operator const T *() const { return &m00; }
    explicit operator       T *()       { return &m00; }
};
// cast / conversion
//////////////////////////
TEMPLATE_TYPENAME_T inline VEC2_T::Vec2(const VEC3_T& v) : VEC2_T{ v.x, v.y } {}
TEMPLATE_TYPENAME_T inline VEC3_T::Vec3(const VEC4_T& v) : VEC3_T{ v.x, v.y, v.z } {}
TEMPLATE_TYPENAME_T inline MAT3_T::Mat3(const MAT4_T& m) : v { VEC3_T(m.v[0]), m.v[1], m.v[2] } {}
TEMPLATE_TYPENAME_T inline MAT3_T mat3_cast(QUAT_T const& q) {
    T xx(q.x * q.x); T yy(q.y * q.y); T zz(q.z * q.z);
    T xz(q.x * q.z); T xy(q.x * q.y); T yz(q.y * q.z);
    T wx(q.w * q.x); T wy(q.w * q.y); T wz(q.w * q.z);

    return { T(1) - T(2) * (yy + zz),         T(2) * (xy + wz),         T(2) * (xz - wy),
                    T(2) * (xy - wz),  T(1) - T(2) * (xx + zz),         T(2) * (yz + wx),
                    T(2) * (xz + wy),         T(2) * (yz - wx),  T(1) - T(2) * (xx + yy) }; }
TEMPLATE_TYPENAME_T inline MAT4_T mat4_cast(QUAT_T const& q) { return { mat3_cast(q) }; }
inline float uintBitsToFloat(uint32_t const v) { return *((float *)(&v)); }
inline uint32_t floatBitsToUint(float const v) { return *((uint32_t *)(&v)); }
// dot
//////////////////////////
TEMPLATE_TYPENAME_T inline T dot(const VEC2_T& v0, const VEC2_T& v1) { return v0.x * v1.x + v0.y * v1.y; }
TEMPLATE_TYPENAME_T inline T dot(const VEC3_T& v0, const VEC3_T& v1) { return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z; }
TEMPLATE_TYPENAME_T inline T dot(const VEC4_T& v0, const VEC4_T& v1) { return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w; }
TEMPLATE_TYPENAME_T inline T dot(const QUAT_T& q0, const QUAT_T& q1) { return q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w; }
// cross
//////////////////////////
TEMPLATE_TYPENAME_T inline      T cross(const VEC2_T& u, const VEC2_T& v) { return u.x * v.y - v.x * u.y; }
TEMPLATE_TYPENAME_T inline VEC3_T cross(const VEC3_T& u, const VEC3_T& v) { return { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x }; }
// length
//////////////////////////
TEMPLATE_TYPENAME_T inline T length(const VEC2_T& v) { return sqrt(dot(v, v)); }
TEMPLATE_TYPENAME_T inline T length(const VEC3_T& v) { return sqrt(dot(v, v)); }
TEMPLATE_TYPENAME_T inline T length(const VEC4_T& v) { return sqrt(dot(v, v)); }
TEMPLATE_TYPENAME_T inline T length(const QUAT_T& q) { return sqrt(dot(q, q)); }
// distance
//////////////////////////
TEMPLATE_TYPENAME_T inline T distance(const VEC2_T& v0, const VEC2_T& v1) { return length(v1 - v0); }
TEMPLATE_TYPENAME_T inline T distance(const VEC3_T& v0, const VEC3_T& v1) { return length(v1 - v0); }
TEMPLATE_TYPENAME_T inline T distance(const VEC4_T& v0, const VEC4_T& v1) { return length(v1 - v0); }
// abs
//////////////////////////
TEMPLATE_TYPENAME_T inline T tAbs(T x) { return x>=T(0) ? x : -x; }
TEMPLATE_TYPENAME_T inline VEC2_T abs(const VEC2_T& v) { return { tAbs(v.x), tAbs(v.y) }; }
TEMPLATE_TYPENAME_T inline VEC3_T abs(const VEC3_T& v) { return { tAbs(v.x), tAbs(v.y), tAbs(v.z) }; }
TEMPLATE_TYPENAME_T inline VEC4_T abs(const VEC4_T& v) { return { tAbs(v.x), tAbs(v.y), tAbs(v.z), tAbs(v.w) }; }
TEMPLATE_TYPENAME_T inline QUAT_T abs(const QUAT_T& q) { return { tAbs(q.w), tAbs(q.x), tAbs(q.y), tAbs(q.z) }; }
// sign
//////////////////////////
TEMPLATE_TYPENAME_T inline T sign(const T v) { return v>T(0) ? T(1) : ( v<T(0) ? T(-1) : T(0)); }
// normalize
//////////////////////////
TEMPLATE_TYPENAME_T inline VEC2_T normalize(const VEC2_T& v) { return v / length(v); }
TEMPLATE_TYPENAME_T inline VEC3_T normalize(const VEC3_T& v) { return v / length(v); }
TEMPLATE_TYPENAME_T inline VEC4_T normalize(const VEC4_T& v) { return v / length(v); }
TEMPLATE_TYPENAME_T inline QUAT_T normalize(const QUAT_T& q) { return q / length(q); }
// mix
//////////////////////////
TEMPLATE_TYPENAME_T inline      T mix(const      T  x, const      T  y, const T a)   { return x + (y-x) * a; }
TEMPLATE_TYPENAME_T inline VEC2_T mix(const VEC2_T& x, const VEC2_T& y, const T a)   { return x + (y-x) * a; }
TEMPLATE_TYPENAME_T inline VEC3_T mix(const VEC3_T& x, const VEC3_T& y, const T a)   { return x + (y-x) * a; }
TEMPLATE_TYPENAME_T inline VEC4_T mix(const VEC4_T& x, const VEC4_T& y, const T a)   { return x + (y-x) * a; }
// pow
//////////////////////////
TEMPLATE_TYPENAME_T inline VEC2_T pow(const VEC2_T& b, const VEC2_T& e) { return { ::pow(b.x,e.x), ::pow(b.y,e.y) }; }
TEMPLATE_TYPENAME_T inline VEC3_T pow(const VEC3_T& b, const VEC3_T& e) { return { ::pow(b.x,e.x), ::pow(b.y,e.y), ::pow(b.z,e.z) }; }
TEMPLATE_TYPENAME_T inline VEC4_T pow(const VEC4_T& b, const VEC4_T& e) { return { ::pow(b.x,e.x), ::pow(b.y,e.y), ::pow(b.z,e.z), ::pow(b.w,e.w) }; }
// value_ptr
//////////////////////////
TEMPLATE_TYPENAME_T inline T *value_ptr(const VEC2_T &v) { return const_cast<T *>(&v.x); }
TEMPLATE_TYPENAME_T inline T *value_ptr(const VEC3_T &v) { return const_cast<T *>(&v.x); }
TEMPLATE_TYPENAME_T inline T *value_ptr(const VEC4_T &v) { return const_cast<T *>(&v.x); }
TEMPLATE_TYPENAME_T inline T *value_ptr(const QUAT_T &q) { return const_cast<T *>(&q.x); }
TEMPLATE_TYPENAME_T inline T *value_ptr(const MAT3_T &m) { return const_cast<T *>(&m.m00); }
TEMPLATE_TYPENAME_T inline T *value_ptr(const MAT4_T &m) { return const_cast<T *>(&m.m00); }
// transpose
//////////////////////////
TEMPLATE_TYPENAME_T inline MAT3_T transpose(MAT3_T m) {
    return { m.m00, m.m10, m.m20,
             m.m01, m.m11, m.m21,
             m.m02, m.m12, m.m22}; }
TEMPLATE_TYPENAME_T inline MAT4_T transpose(MAT4_T m) {
    return { m.m00, m.m10, m.m20, m.m30,
             m.m01, m.m11, m.m21, m.m31,
             m.m02, m.m12, m.m22, m.m32,
             m.m03, m.m13, m.m23, m.m33}; }
// inverse
//////////////////////////
#define M(X,Y) (m.m##X * m.m##Y)
TEMPLATE_TYPENAME_T inline QUAT_T inverse(QUAT_T const &q) { return QUAT_T(q.w, -q.x, -q.y, -q.z) / dot(q, q); }
TEMPLATE_TYPENAME_T inline MAT3_T inverse(MAT3_T const &m) {
    T invDet = T(1) / (m.m00 * (M(11,22) - M(21,12)) - m.m10 * (M(01,22) - M(21,02)) + m.m20 * (M(01,12) - M(11,02)));
    return MAT3_T(  (M(11,22) - M(21,12)), - (M(01,22) - M(21,02)),   (M(01,12) - M(11,02)),
                  - (M(10,22) - M(20,12)),   (M(00,22) - M(20,02)), - (M(00,12) - M(10,02)),
                    (M(10,21) - M(20,11)), - (M(00,21) - M(20,01)),   (M(00,11) - M(10,01))) * invDet; } // ==> "operator *" is faster
TEMPLATE_TYPENAME_T inline MAT4_T inverse(MAT4_T const &m) {
    const T c0 = M(22,33) - M(32,23);   VEC4_T f0(c0, c0, M(12,33) - M(32,13), M(12,23) - M(22,13));
    const T c1 = M(21,33) - M(31,23);   VEC4_T f1(c1, c1, M(11,33) - M(31,13), M(11,23) - M(21,13));
    const T c2 = M(21,32) - M(31,22);   VEC4_T f2(c2, c2, M(11,32) - M(31,12), M(11,22) - M(21,12));
    const T c3 = M(20,33) - M(30,23);   VEC4_T f3(c3, c3, M(10,33) - M(30,13), M(10,23) - M(20,13));
    const T c4 = M(20,32) - M(30,22);   VEC4_T f4(c4, c4, M(10,32) - M(30,12), M(10,22) - M(20,12));
    const T c5 = M(20,31) - M(30,21);   VEC4_T f5(c5, c5, M(10,31) - M(30,11), M(10,21) - M(20,11));
#undef M
    VEC4_T v0(m.m10, m.m00, m.m00, m.m00);
    VEC4_T v1(m.m11, m.m01, m.m01, m.m01);
    VEC4_T v2(m.m12, m.m02, m.m02, m.m02);
    VEC4_T v3(m.m13, m.m03, m.m03, m.m03);

    VEC4_T signV(T(1), T(-1),  T(1), T(-1));
    MAT4_T inv((v1 * f0 - v2 * f1 + v3 * f2) *  signV,
               (v0 * f0 - v2 * f3 + v3 * f4) * -signV,
               (v0 * f1 - v1 * f3 + v3 * f5) *  signV,
               (v0 * f2 - v1 * f4 + v2 * f5) * -signV);
            
    VEC4_T v0r0(m.v[0] * VEC4_T(inv.m00, inv.m10, inv.m20, inv.m30));
    return inv * (T(1) / (v0r0.x + v0r0.y + v0r0.z + v0r0.w)); }// 1/determinant ==> "operator *" is faster
// external operators
//////////////////////////
TEMPLATE_TYPENAME_T inline VEC2_T operator*(const T s, const VEC2_T& v) {  return v * s; }
TEMPLATE_TYPENAME_T inline VEC3_T operator*(const T s, const VEC3_T& v) {  return v * s; }
TEMPLATE_TYPENAME_T inline VEC4_T operator*(const T s, const VEC4_T& v) {  return v * s; }
TEMPLATE_TYPENAME_T inline QUAT_T operator*(const T s, const QUAT_T& q) {  return q * s; }

TEMPLATE_TYPENAME_T inline VEC2_T operator/(const T s, const VEC2_T& v) {  return { s/v.x, s/v.y }; }
TEMPLATE_TYPENAME_T inline VEC3_T operator/(const T s, const VEC3_T& v) {  return { s/v.x, s/v.y, s/v.z }; }
TEMPLATE_TYPENAME_T inline VEC4_T operator/(const T s, const VEC4_T& v) {  return { s/v.x, s/v.y, s/v.z, s/v.w }; }
TEMPLATE_TYPENAME_T inline QUAT_T operator/(const T s, const QUAT_T& q) {  return { s/q.x, s/q.y, s/q.z, s/q.w }; }

TEMPLATE_TYPENAME_T inline VEC3_T operator*(const QUAT_T& q, const VEC3_T& v) {
    const VEC3_T qV(q.x, q.y, q.z), uv(cross(qV, v));
    return v + ((uv * q.w) + cross(qV, uv)) * T(2); }
TEMPLATE_TYPENAME_T inline  VEC3_T operator*(const VEC3_T& v, const QUAT_T& q) {  return inverse(q) * v; }
// translate / scale
//////////////////////////
TEMPLATE_TYPENAME_T inline MAT4_T translate(MAT4_T const& m, VEC3_T const& v) {
    MAT4_T r(m); r[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3]; 
    return r; }
TEMPLATE_TYPENAME_T inline MAT4_T scale(MAT4_T const& m, VEC3_T const& v) {
    return MAT4_T(m[0] * v[0], m[1] * v[1], m[2] * v[2], m[3]); }
// quat angle/axis
//////////////////////////
TEMPLATE_TYPENAME_T inline QUAT_T angleAxis(T const &a, VEC3_T const &v) {	return QUAT_T(cos(a * T(0.5)), v * sin(a * T(0.5))); }
TEMPLATE_TYPENAME_T inline T angle(QUAT_T const& q) { return acos(q.w) * T(2); }
TEMPLATE_TYPENAME_T inline VEC3_T axis(QUAT_T const& q) {
    const T t1 = T(1) - q.w * q.w; if(t1 <= T(0)) return VEC3_T(0, 0, 1);
    const T t2 = T(1) / sqrt(t1);  return VEC3_T(q.x * t2, q.y * t2, q.z * t2); }
// trigonometric
//////////////////////////
TEMPLATE_TYPENAME_T inline T radians(T d) { return d * T(0.0174532925199432957692369076849); }
TEMPLATE_TYPENAME_T inline T degrees(T r) { return r * T(57.295779513082320876798154814105); }
TEMPLATE_TYPENAME_T inline T pi() { return T(3.1415926535897932384626433832795029); }
TEMPLATE_TYPENAME_T inline T one_over_pi() { return T(0.318309886183790671537767526745028724); }

// lookAt
//////////////////////////
TEMPLATE_TYPENAME_T inline MAT4_T lookAt(const VEC3_T& pov, const VEC3_T& tgt, const VEC3_T& up)
{
#ifdef VGM_USES_LEFT_HAND_AXES
    VEC3_T k = normalize(tgt - pov), i = normalize(cross(up, k)), j = cross(k, i);
#else
    VEC3_T k = normalize(tgt - pov), i = normalize(cross(k, up)), j = cross(i, k);   k = -k;
#endif
    return {     i.x,          j.x,          k.x,     T(0),
                 i.y,          j.y,          k.y,     T(0),
                 i.z,          j.z,          k.z,     T(0),
            -dot(i, pov), -dot(j, pov), -dot(k, pov), T(1)}; }
// ortho
//////////////////////////
TEMPLATE_TYPENAME_T inline MAT4_T ortho(T l, T r, T b, T t, T n, T f)
{
#ifdef VGM_USES_LEFT_HAND_AXES
    const T v = T(2);
#else
    const T v = T(-2);
#endif
    return {  T(2)/(r-l),     T(0),         T(0),     T(0),
                T(0),       T(2)/(t-b),     T(0),     T(0),
                T(0),         T(0),        v/(f-n),   T(0),
            -(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), T(1)}; }
// perspective
//////////////////////////
TEMPLATE_TYPENAME_T inline MAT4_T perspective(T fovy, T a, T n, T f)
{
#ifdef VGM_USES_LEFT_HAND_AXES
    const T v = T(1), f_n = (f+n)/(f-n);
#else
    const T v = T(-1), f_n = -(f+n)/(f-n);
#endif
    const T hFovy = tan(fovy / T(2));
    return { T(1)/(a*hFovy),  T(0),           T(0),      T(0),
               T(0),        T(1)/(hFovy),     T(0),      T(0),
               T(0),          T(0),            f_n,        v ,
               T(0),          T(0),   -(T(2)*f*n)/(f-n), T(0)}; }
// perspectiveFov
//////////////////////////
TEMPLATE_TYPENAME_T inline MAT4_T perspectiveFov(T fovy, T w, T h, T n, T f) { return perspective(fovy, w/h, n, f); }
// frustrum
//////////////////////////
TEMPLATE_TYPENAME_T inline MAT4_T frustum(T l, T r, T b, T t, T n, T f)
{
#ifdef VGM_USES_LEFT_HAND_AXES
    const T v = T(1),  f_n =  (f+n)/(f-n);
#else
    const T v = T(-1), f_n = -(f+n)/(f-n);
#endif
    return { (T(2)*n)/(r-l),       T(0),         T(0),         T(0),
                   T(0),     (T(2)*n)/(t-b),     T(0),         T(0),
                (r+l)/(r-l),    (t+b)/(t-b),      f_n,           v ,
                   T(0),           T(0),    -(T(2)*f*n)/(f-n), T(0)}; }

} // end namespace vgm

#ifdef VGM_USES_TEMPLATE
    using vec2 = vgm::Vec2<float>;
    using vec3 = vgm::Vec3<float>;
    using vec4 = vgm::Vec4<float>;
    using quat = vgm::Quat<float>;
    using mat3 = vgm::Mat3<float>;
    using mat4 = vgm::Mat4<float>;
    using mat3x3 = mat3;
    using mat4x4 = mat4;

    using dvec2 = vgm::Vec2<double>;
    using dvec3 = vgm::Vec3<double>;
    using dvec4 = vgm::Vec4<double>;
    using dquat = vgm::Quat<double>;
    using dmat3 = vgm::Mat3<double>;
    using dmat4 = vgm::Mat4<double>;
    using dmat3x3 = dmat3;
    using dmat4x4 = dmat4;

    using ivec2 = vgm::Vec2<int32_t>;
    using ivec3 = vgm::Vec3<int32_t>;
    using ivec4 = vgm::Vec4<int32_t>;

    using uvec2 = vgm::Vec2<uint32_t>;
    using uvec3 = vgm::Vec3<uint32_t>;
    using uvec4 = vgm::Vec4<uint32_t>;

#ifdef VGIZMO_USES_HLSL_TYPES // testing phase
    using float2   = vgm::Vec2<float>;
    using float3   = vgm::Vec3<float>;
    using float4   = vgm::Vec4<float>;
    using float3x3 = vgm::Mat3<float>;
    using float4x4 = vgm::Mat4<float>;

    using double2   = vgm::Vec2<double>;
    using double3   = vgm::Vec3<double>;
    using double4   = vgm::Vec4<double>;
    using double3x3 = vgm::Mat3<double>;
    using double4x4 = vgm::Mat4<double>;

    using int2 = vgm::Vec2<int32_t>;
    using int3 = vgm::Vec3<int32_t>;
    using int4 = vgm::Vec4<int32_t>;

    using uint2 = vgm::Vec2<uint32_t>;
    using uint3 = vgm::Vec3<uint32_t>;
    using uint4 = vgm::Vec4<uint32_t>;
#endif
#else
    using vec2 = vgm::Vec2;
    using vec3 = vgm::Vec3;
    using vec4 = vgm::Vec4;
    using quat = vgm::Quat;
    using mat3 = vgm::Mat3;
    using mat4 = vgm::Mat4;
    using mat3x3 = mat3;
    using mat4x4 = mat4;

#ifdef VGIZMO_USES_HLSL_TYPES
    using float2   = vgm::Vec2;
    using float3   = vgm::Vec3;
    using float4   = vgm::Vec4;
    using float3x3 = vgm::Mat3;
    using float4x4 = vgm::Mat4;
#endif

#endif
// Internal vGizmo USES ONLY
    using tVec2 = vgm::VEC2_PRECISION;
    using tVec3 = vgm::VEC3_PRECISION;
    using tVec4 = vgm::VEC4_PRECISION;
    using tQuat = vgm::QUAT_PRECISION;
    using tMat3 = vgm::MAT3_PRECISION;
    using tMat4 = vgm::MAT4_PRECISION;

    using uint8  = uint8_t;
    using  int8  =  int8_t;
    using uint   = uint32_t;
    using  int32 =  int32_t;
    using uint32 = uint32_t;
    using  int64 =  int64_t;
    using uint64 = uint64_t;

    #undef VEC2_T
    #undef VEC3_T
    #undef VEC4_T
    #undef QUAT_T
    #undef MAT3_T
    #undef MAT4_T

    #undef VEC2_PRECISION
    #undef VEC3_PRECISION
    #undef VEC4_PRECISION
    #undef QUAT_PRECISION
    #undef MAT3_PRECISION
    #undef MAT4_PRECISION


#endif // use vGizmoMath
#if !defined(VGM_DISABLE_AUTO_NAMESPACE) || defined(VGIZMO_H_FILE)
    using namespace VGM_NAMESPACE;
#endif

#undef VGM_NAMESPACE
#undef T // if used T as #define, undef it
