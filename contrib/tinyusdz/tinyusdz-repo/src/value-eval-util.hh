#pragma once

#include "value-types.hh"
#include "linear-algebra.hh"

namespace tinyusdz {

// half

inline value::half2 operator+(const value::half2 &a, const value::half2 &b) {
  return {a[0] + b[0], a[1] + b[1]};
}

inline value::half2 operator+(const float a, const value::half2 &b) {
  return {a + b[0], a + b[1]};
}

inline value::half2 operator+(const value::half2 &a, const float b) {
  return {a[0] + b, a[1] + b};
}

inline value::half2 operator-(const value::half2 &a, const value::half2 &b) {
  return {a[0] - b[0], a[1] - b[1]};
}

inline value::half2 operator-(const float a, const value::half2 &b) {
  return {a - b[0], a - b[1]};
}

inline value::half2 operator-(const value::half2 &a, const float b) {
  return {a[0] - b, a[1] - b};
}

inline value::half2 operator*(const value::half2 &a, const value::half2 &b) {
  return {a[0] * b[0], a[1] * b[1]};
}

inline value::half2 operator*(const float a, const value::half2 &b) {
  return {a * b[0], a * b[1]};
}

inline value::half2 operator*(const value::half2 &a, const float b) {
  return {a[0] * b, a[1] * b};
}

inline value::half2 operator/(const value::half2 &a, const value::half2 &b) {
  return {a[0] / b[0], a[1] / b[1]};
}

inline value::half2 operator/(const float a, const value::half2 &b) {
  return {a / b[0], a / b[1]};
}

inline value::half2 operator/(const value::half2 &a, const float b) {
  return {a[0] / b, a[1] / b};
}

inline value::half3 operator+(const value::half3 &a, const value::half3 &b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline value::half3 operator+(const float a, const value::half3 &b) {
  return {a + b[0], a + b[1], a + b[2]};
}

inline value::half3 operator+(const value::half3 &a, const float b) {
  return {a[0] + b, a[1] + b, a[2] + b};
}

inline value::half3 operator*(const value::half3 &a, const value::half3 &b) {
  return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
}

inline value::half3 operator*(const float a, const value::half3 &b) {
  return {a * b[0], a * b[1], a * b[2]};
}

inline value::half3 operator*(const value::half3 &a, const float b) {
  return {a[0] * b, a[1] * b, a[2] * b};
}

inline value::half3 operator/(const value::half3 &a, const value::half3 &b) {
  return {a[0] / b[0], a[1] / b[1], a[2] / b[2]};
}

inline value::half3 operator/(const float a, const value::half3 &b) {
  return {a / b[0], a / b[1], a / b[2]};
}

inline value::half3 operator/(const value::half3 &a, const float b) {
  return {a[0] / b, a[1] / b, a[2] / b};
}

inline value::half4 operator+(const value::half4 &a, const value::half4 &b) {
  return {a[0] + b[0], a[1] + b[1], a[2] - b[2], a[3] - b[3]};
}

inline value::half4 operator+(const float a, const value::half4 &b) {
  return {a + b[0], a + b[1], a + b[2], a + b[3]};
}

inline value::half4 operator+(const value::half4 &a, const float b) {
  return {a[0] + b, a[1] + b, a[2] + b, a[3] + b};
}

inline value::half4 operator-(const value::half4 &a, const value::half4 &b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]};
}

inline value::half4 operator-(const float a, const value::half4 &b) {
  return {a - b[0], a - b[1], a - b[2], a - b[3]};
}

inline value::half4 operator-(const value::half4 &a, const float b) {
  return {a[0] - b, a[1] - b, a[2] - b, a[3] - b};
}

inline value::half4 operator*(const value::half4 &a, const value::half4 &b) {
  return {a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]};
}

inline value::half4 operator*(const float a, const value::half4 &b) {
  return {a * b[0], a * b[1], a * b[2], a * b[3]};
}

inline value::half4 operator*(const value::half4 &a, const float b) {

  return {a[0] * b, a[1] * b, a[2] * b, a[3] * b};
}

inline value::half4 operator/(const value::half4 &a, const value::half4 &b) {
  return {a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3]};
}

inline value::half4 operator/(const float a, const value::half4 &b) {
  return {a / b[0], a / b[1], a / b[2], a / b[3]};
}

inline value::half4 operator/(const value::half4 &a, const float b) {

  return {a[0] / b, a[1] / b, a[2] / b, a[3] / b};
}

// float

inline value::float2 operator+(const value::float2 &a, const value::float2 &b) {
  return {a[0] + b[0], a[1] + b[1]};
}

inline value::float2 operator+(const float a, const value::float2 &b) {
  return {a + b[0], a + b[1]};
}

inline value::float2 operator+(const value::float2 &a, const float b) {
  return {a[0] + b, a[1] + b};
}


inline value::float2 operator-(const value::float2 &a, const value::float2 &b) {
  return {a[0] - b[0], a[1] - b[1]};
}

inline value::float2 operator-(const float a, const value::float2 &b) {
  return {a - b[0], a - b[1]};
}

inline value::float2 operator-(const value::float2 &a, const float b) {
  return {a[0] - b, a[1] - b};
}

inline value::float2 operator*(const value::float2 &a, const value::float2 &b) {
  return {a[0] * b[0], a[1] * b[1]};
}

inline value::float2 operator*(const float a, const value::float2 &b) {
  return {a * b[0], a * b[1]};
}

inline value::float2 operator*(const value::float2 &a, const float b) {
  return {a[0] * b, a[1] * b};
}

inline value::float2 operator/(const value::float2 &a, const value::float2 &b) {
  return {a[0] / b[0], a[1] / b[1]};
}

inline value::float2 operator/(const float a, const value::float2 &b) {
  return {a / b[0], a / b[1]};
}

inline value::float2 operator/(const value::float2 &a, const float b) {
  return {a[0] / b, a[1] / b};
}

inline value::float3 operator+(const value::float3 &a, const value::float3 &b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline value::float3 operator+(const float a, const value::float3 &b) {
  return {a + b[0], a + b[1], a + b[2]};
}

inline value::float3 operator+(const value::float3 &a, const float b) {
  return {a[0] + b, a[1] + b, a[2] + b};
}

inline value::float3 operator-(const value::float3 &a, const value::float3 &b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline value::float3 operator-(const float a, const value::float3 &b) {
  return {a - b[0], a - b[1], a - b[2]};
}

inline value::float3 operator-(const value::float3 &a, const float b) {
  return {a[0] - b, a[1] - b, a[2] - b};
}

inline value::float3 operator*(const value::float3 &a, const value::float3 &b) {
  return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
}

inline value::float3 operator*(const float a, const value::float3 &b) {
  return {a * b[0], a * b[1], a * b[2]};
}

inline value::float3 operator*(const value::float3 &a, const float b) {
  return {a[0] * b, a[1] * b, a[2] * b};
}

inline value::float3 operator/(const value::float3 &a, const value::float3 &b) {
  return {a[0] / b[0], a[1] / b[1], a[2] / b[2]};
}

inline value::float3 operator/(const float a, const value::float3 &b) {
  return {a / b[0], a / b[1], a / b[2]};
}

inline value::float3 operator/(const value::float3 &a, const float b) {
  return {a[0] / b, a[1] / b, a[2] / b};
}

inline value::float4 operator+(const value::float4 &a, const value::float4 &b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]};
}

inline value::float4 operator+(const float a, const value::float4 &b) {
  return {a + b[0], a + b[1], a + b[2], a + b[3]};
}

inline value::float4 operator+(const value::float4 &a, const float b) {
  return {a[0] + b, a[1] + b, a[2] + b, a[3] + b};
}

inline value::float4 operator-(const value::float4 &a, const value::float4 &b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]};
}

inline value::float4 operator-(const float a, const value::float4 &b) {
  return {a - b[0], a - b[1], a - b[2], a - b[3]};
}

inline value::float4 operator-(const value::float4 &a, const float b) {
  return {a[0] - b, a[1] - b, a[2] - b, a[3] - b};
}



inline value::float4 operator*(const value::float4 &a, const value::float4 &b) {
  return {a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]};
}

inline value::float4 operator*(const float a, const value::float4 &b) {
  return {a * b[0], a * b[1], a * b[2], a * b[3]};
}

inline value::float4 operator*(const value::float4 &a, const float b) {

  return {a[0] * b, a[1] * b, a[2] * b, a[3] * b};
}

// double

inline value::double2 operator+(const value::double2 &a, const value::double2 &b) {
  return {a[0] + b[0], a[1] + b[1]};
}

inline value::double2 operator+(const double a, const value::double2 &b) {
  return {a + b[0], a + b[1]};
}

inline value::double2 operator+(const value::double2 &a, const double b) {
  return {a[0] + b, a[1] + b};
}


inline value::double2 operator-(const value::double2 &a, const value::double2 &b) {
  return {a[0] - b[0], a[1] - b[1]};
}

inline value::double2 operator-(const double a, const value::double2 &b) {
  return {a - b[0], a - b[1]};
}

inline value::double2 operator-(const value::double2 &a, const double b) {
  return {a[0] - b, a[1] - b};
}

inline value::double2 operator*(const value::double2 &a, const value::double2 &b) {
  return {a[0] * b[0], a[1] * b[1]};
}

inline value::double2 operator*(const double a, const value::double2 &b) {
  return {a * b[0], a * b[1]};
}

inline value::double2 operator*(const value::double2 &a, const double b) {
  return {a[0] * b, a[1] * b};
}

inline value::double2 operator/(const value::double2 &a, const value::double2 &b) {
  return {a[0] / b[0], a[1] / b[1]};
}

inline value::double2 operator/(const double a, const value::double2 &b) {
  return {a / b[0], a / b[1]};
}

inline value::double2 operator/(const value::double2 &a, const double b) {
  return {a[0] / b, a[1] / b};
}

inline value::double3 operator+(const value::double3 &a, const value::double3 &b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline value::double3 operator+(const double a, const value::double3 &b) {
  return {a + b[0], a + b[1], a + b[2]};
}

inline value::double3 operator+(const value::double3 &a, const double b) {
  return {a[0] + b, a[1] + b, a[2] + b};
}

inline value::double3 operator-(const value::double3 &a, const value::double3 &b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline value::double3 operator-(const double a, const value::double3 &b) {
  return {a - b[0], a - b[1], a - b[2]};
}

inline value::double3 operator-(const value::double3 &a, const double b) {
  return {a[0] - b, a[1] - b, a[2] - b};
}

inline value::double3 operator*(const value::double3 &a, const value::double3 &b) {
  return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
}

inline value::double3 operator*(const double a, const value::double3 &b) {
  return {a * b[0], a * b[1], a * b[2]};
}

inline value::double3 operator*(const value::double3 &a, const double b) {
  return {a[0] * b, a[1] * b, a[2] * b};
}

inline value::double3 operator/(const value::double3 &a, const value::double3 &b) {
  return {a[0] / b[0], a[1] / b[1], a[2] / b[2]};
}

inline value::double3 operator/(const double a, const value::double3 &b) {
  return {a / b[0], a / b[1], a / b[2]};
}

inline value::double3 operator/(const value::double3 &a, const double b) {
  return {a[0] / b, a[1] / b, a[2] / b};
}

inline value::double4 operator+(const value::double4 &a, const value::double4 &b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]};
}

inline value::double4 operator+(const double a, const value::double4 &b) {
  return {a + b[0], a + b[1], a + b[2], a + b[3]};
}

inline value::double4 operator+(const value::double4 &a, const double b) {
  return {a[0] + b, a[1] + b, a[2] + b, a[3] + b};
}

inline value::double4 operator-(const value::double4 &a, const value::double4 &b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]};
}

inline value::double4 operator-(const double a, const value::double4 &b) {
  return {a - b[0], a - b[1], a - b[2], a - b[3]};
}

inline value::double4 operator-(const value::double4 &a, const double b) {
  return {a[0] - b, a[1] - b, a[2] - b, a[3] - b};
}



inline value::double4 operator*(const value::double4 &a, const value::double4 &b) {
  return {a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]};
}

inline value::double4 operator*(const double a, const value::double4 &b) {
  return {a * b[0], a * b[1], a * b[2], a * b[3]};
}

inline value::double4 operator*(const value::double4 &a, const double b) {

  return {a[0] * b, a[1] * b, a[2] * b, a[3] * b};
}

// normal
inline value::normal3f operator+(const value::normal3f &a, const value::normal3f &b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline value::normal3f operator+(const float a, const value::normal3f &b) {
  return {a + b[0], a + b[1], a + b[2]};
}

inline value::normal3f operator+(const value::normal3f &a, const float b) {
  return {a[0] + b, a[1] + b, a[2] + b};
}

inline value::normal3f operator-(const value::normal3f &a, const value::normal3f &b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline value::normal3f operator-(const float a, const value::normal3f &b) {
  return {a - b[0], a - b[1], a - b[2]};
}

inline value::normal3f operator-(const value::normal3f &a, const float b) {
  return {a[0] - b, a[1] - b, a[2] - b};
}

inline value::normal3f operator*(const value::normal3f &a, const value::normal3f &b) {
  return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
}

inline value::normal3f operator*(const float a, const value::normal3f &b) {
  return {a * b[0], a * b[1], a * b[2]};
}

inline value::normal3f operator*(const value::normal3f &a, const float b) {
  return {a[0] * b, a[1] * b, a[2] * b};
}

inline value::normal3f operator/(const value::normal3f &a, const value::normal3f &b) {
  return {a[0] / b[0], a[1] / b[1], a[2] / b[2]};
}

inline value::normal3f operator/(const float a, const value::normal3f &b) {
  return {a / b[0], a / b[1], a / b[2]};
}

inline value::normal3f operator/(const value::normal3f &a, const float b) {
  return {a[0] / b, a[1] / b, a[2] / b};
}

// normal
inline value::normal3d operator+(const value::normal3d &a, const value::normal3d &b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline value::normal3d operator+(const double a, const value::normal3d &b) {
  return {a + b[0], a + b[1], a + b[2]};
}

inline value::normal3d operator+(const value::normal3d &a, const double b) {
  return {a[0] + b, a[1] + b, a[2] + b};
}

inline value::normal3d operator-(const value::normal3d &a, const value::normal3d &b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline value::normal3d operator-(const double a, const value::normal3d &b) {
  return {a - b[0], a - b[1], a - b[2]};
}

inline value::normal3d operator-(const value::normal3d &a, const double b) {
  return {a[0] - b, a[1] - b, a[2] - b};
}

inline value::normal3d operator*(const value::normal3d &a, const value::normal3d &b) {
  return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
}

inline value::normal3d operator*(const double a, const value::normal3d &b) {
  return {a * b[0], a * b[1], a * b[2]};
}

inline value::normal3d operator*(const value::normal3d &a, const double b) {
  return {a[0] * b, a[1] * b, a[2] * b};
}

inline value::normal3d operator/(const value::normal3d &a, const value::normal3d &b) {
  return {a[0] / b[0], a[1] / b[1], a[2] / b[2]};
}

inline value::normal3d operator/(const double a, const value::normal3d &b) {
  return {a / b[0], a / b[1], a / b[2]};
}

inline value::normal3d operator/(const value::normal3d &a, const double b) {
  return {a[0] / b, a[1] / b, a[2] / b};
}

// vector
inline value::vector3f operator+(const value::vector3f &a, const value::vector3f &b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

inline value::vector3f operator+(const float a, const value::vector3f &b) {
  return {a + b[0], a + b[1], a + b[2]};
}

inline value::vector3f operator+(const value::vector3f &a, const float b) {
  return {a[0] + b, a[1] + b, a[2] + b};
}

inline value::vector3f operator-(const value::vector3f &a, const value::vector3f &b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline value::vector3f operator-(const float a, const value::vector3f &b) {
  return {a - b[0], a - b[1], a - b[2]};
}

inline value::vector3f operator-(const value::vector3f &a, const float b) {
  return {a[0] - b, a[1] - b, a[2] - b};
}

inline value::vector3f operator*(const value::vector3f &a, const value::vector3f &b) {
  return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
}

inline value::vector3f operator*(const float a, const value::vector3f &b) {
  return {a * b[0], a * b[1], a * b[2]};
}

inline value::vector3f operator*(const value::vector3f &a, const float b) {
  return {a[0] * b, a[1] * b, a[2] * b};
}

inline value::vector3f operator/(const value::vector3f &a, const value::vector3f &b) {
  return {a[0] / b[0], a[1] / b[1], a[2] / b[2]};
}

inline value::vector3f operator/(const float a, const value::vector3f &b) {
  return {a / b[0], a / b[1], a / b[2]};
}

inline value::vector3f operator/(const value::vector3f &a, const float b) {
  return {a[0] / b, a[1] / b, a[2] / b};
}

// -- lerp

// no lerp by default
template <typename T>
inline T lerp(const T &a, const T &b, const double t) {
  (void)b;
  (void)t;
  return a;
}

template <>
inline value::half lerp(const value::half &a, const value::half &b, const double t) {
  return float(1.0 - t) * a + float(t) * b;
}

template <>
inline value::half2 lerp(const value::half2 &a, const value::half2 &b, const double t) {
  return float(1.0 - t) * a + float(t) * b;
}

template <>
inline value::half3 lerp(const value::half3 &a, const value::half3 &b, const double t) {
  return float(1.0 - t) * a + float(t) * b;
}

template <>
inline value::half4 lerp(const value::half4 &a, const value::half4 &b, const double t) {
  return float(1.0 - t) * a + float(t) * b;
}

template <>
inline float lerp(const float &a, const float &b, const double t) {
  return float(1.0 - t) * a + float(t) * b;
}

template <>
inline value::float2 lerp(const value::float2 &a, const value::float2 &b, const double t) {
  return float(1.0 - t) * a + float(t) * b;
}

template <>
inline value::float3 lerp(const value::float3 &a, const value::float3 &b, const double t) {
  return float(1.0 - t) * a + float(t) * b;
}

template <>
inline value::normal3f lerp(const value::normal3f &a, const value::normal3f &b, const double t) {
  return float(1.0 - t) * a + float(t) * b;
}

template <>
inline value::float4 lerp(const value::float4 &a, const value::float4 &b, const double t) {
  return float(1.0 - t) * a + float(t) * b;
}

template <typename T>
inline std::vector<T> lerp(const std::vector<T> &a, const std::vector<T> &b,
                           const double t) {
  std::vector<T> dst;

  // Choose shorter one
  size_t n = std::min(a.size(), b.size());
  if (n == 0) {
    return dst;
  }

  dst.resize(n);

  if (a.size() != b.size()) {
    return dst;
  }
  for (size_t i = 0; i < n; i++) {
    dst[i] = lerp(a[i], b[i], t);
  }

  return dst;
}

template <>
inline value::quath lerp(const value::quath &a, const value::quath &b, const double t) {
  // to float.
  value::quatf af;
  value::quatf bf;
  af.real = half_to_float(a.real);
  af.imag[0] = half_to_float(a.imag[0]);
  af.imag[1] = half_to_float(a.imag[1]);
  af.imag[2] = half_to_float(a.imag[2]);

  bf.real = half_to_float(b.real);
  bf.imag[0] = half_to_float(b.imag[0]);
  bf.imag[1] = half_to_float(b.imag[1]);
  bf.imag[2] = half_to_float(b.imag[2]);

  value::quatf ret =  slerp(af, bf, float(t));
  value::quath h;
  h.real = value::float_to_half_full(ret.real);
  h.imag[0] = value::float_to_half_full(ret.imag[0]);
  h.imag[1] = value::float_to_half_full(ret.imag[1]);
  h.imag[2] = value::float_to_half_full(ret.imag[2]);
  
  return h;
}

template <>
inline value::quatf lerp(const value::quatf &a, const value::quatf &b, const double t) {
  // slerp
  return slerp(a, b, float(t));
}

template <>
inline value::quatd lerp(const value::quatd &a, const value::quatd &b, const double t) {
  // slerp
  return slerp(a, b, t);
}

// specializations for non-lerp-able types
template <>
inline value::AssetPath lerp(const value::AssetPath &a,
                             const value::AssetPath &b, const double t) {
  (void)b;
  (void)t;
  // no interpolation
  return a;
}

template <>
inline std::vector<value::AssetPath> lerp(
    const std::vector<value::AssetPath> &a,
    const std::vector<value::AssetPath> &b, const double t) {
  (void)b;
  (void)t;
  // no interpolation
  return a;
}


} // namespace tinyusdz
