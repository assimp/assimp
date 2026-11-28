// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#pragma once

#include <assimp/vector3.inl>
#include <assimp/matrix4x4.inl>
#include <assimp/quaternion.inl>

static constexpr float PI = 3.14159265358979323846;

using Mat4 = aiMatrix4x4t<float>;
using Vec3 = aiVector3t<float>;
using Quat = aiQuaterniont<float>;

Mat4 look_at_rh(const Vec3& eye, const Vec3& at, const Vec3& up);
Mat4 perspective_fov_rh(float fov_y, float aspect, float z_near, float z_far);

inline Vec3 interpolate(const Vec3& a, const Vec3& b, float f)
{
    return a + (b - a) * f;
}

inline Quat interpolate(const Quat& a, const Quat& b, float f)
{
    Quat q;
    Quat::Interpolate(q, a, b, f);
    return q.Normalize();
}

inline Mat4 inversed(Mat4 m)
{
    return m.Inverse();
}

inline Mat4 transposed(Mat4 m)
{
    return m.Transpose();
}
