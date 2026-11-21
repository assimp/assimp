// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "math.hpp"

#include <assimp/vector3.inl>
#include <assimp/matrix4x4.inl>
#include <assimp/MathFunctions.h>

Mat4 perspective_fov_rh(float fov_y, float aspect, float z_near, float z_far)
{
    const float f = 1.0f / std::tan(fov_y * 0.5f);
    return Mat4(
      f / aspect, 0.0f, 0.0f, 0.0f,
      0.0f, f, 0.0f, 0.0f,
      0.0f, 0.0f, (z_far + z_near) / (z_near - z_far), (2.0f * z_far * z_near) / (z_near - z_far),
      0.0f, 0.0f, -1.0f, 0.0f);
}

Mat4 look_at_rh(const Vec3& eye, const Vec3& at, const Vec3& up)
{
    const Vec3 z = (eye - at).Normalize();
    const Vec3 x = (up ^ z).Normalize();
    const Vec3 y = z ^ x;
    return Mat4(
      x.x, x.y, x.z, -(x * eye),
      y.x, y.y, y.z, -(y * eye),
      z.x, z.y, z.z, -(z * eye),
      0.0f, 0.0f, 0.0f, 1.0f);
}
