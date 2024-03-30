#include <cstdio>
#include <cmath>
#include <iostream>

#include "matrix.h"

//using namespace mallie;

static inline float vdot(float a[3], float b[3]) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline void vcross(float c[3], float a[3], float b[3]) {
  c[0] = a[1] * b[2] - a[2] * b[1];
  c[1] = a[2] * b[0] - a[0] * b[2];
  c[2] = a[0] * b[1] - a[1] * b[0];
}

static inline float vlength(float v[3]) {
  float len2 = vdot(v, v);
  if (std::abs(len2) > 1.0e-6f) {
    return sqrt(len2);
  }
  return 0.0f;
}

static void vnormalize(float v[3]) {
  float len = vlength(v);
  if (std::abs(len) > 1.0e-6f) {
    float inv_len = 1.0f / len;
    v[0] *= inv_len;
    v[1] *= inv_len;
    v[2] *= inv_len;
  }
}

void Matrix::Print(float m[4][4]) {
  for (int i = 0; i < 4; i++) {
    std::cout << "m[" << i << "] = " << m[i][0] << ", " << m[i][1] << ", " << m[i][2] << ", " << m[i][3] << std::endl;
  }
}

void Matrix::LookAt(float m[4][4], float eye[3], float lookat[3],
                    float up[3]) {

  float u[3], v[3];
  float look[3];
  look[0] = lookat[0] - eye[0];
  look[1] = lookat[1] - eye[1];
  look[2] = lookat[2] - eye[2];
  vnormalize(look);

  vcross(u, look, up);
  vnormalize(u);

  vcross(v, u, look);
  vnormalize(v);

#if 0
  m[0][0] = u[0];
  m[0][1] = v[0];
  m[0][2] = -look[0];
  m[0][3] = 0.0;

  m[1][0] = u[1];
  m[1][1] = v[1];
  m[1][2] = -look[1];
  m[1][3] = 0.0;

  m[2][0] = u[2];
  m[2][1] = v[2];
  m[2][2] = -look[2];
  m[2][3] = 0.0;

  m[3][0] = eye[0];
  m[3][1] = eye[1];
  m[3][2] = eye[2];
  m[3][3] = 1.0;
#else
  m[0][0] = u[0];
  m[1][0] = v[0];
  m[2][0] = -look[0];
  m[3][0] = eye[0];

  m[0][1] = u[1];
  m[1][1] = v[1];
  m[2][1] = -look[1];
  m[3][1] = eye[1];

  m[0][2] = u[2];
  m[1][2] = v[2];
  m[2][2] = -look[2];
  m[3][2] = eye[2];

  m[0][3] = 0.0;
  m[1][3] = 0.0;
  m[2][3] = 0.0;
  m[3][3] = 1.0;

#endif
}

void Matrix::Inverse(float m[4][4]) {
  /*
   * codes from intel web
   * cramer's rule version
   */
  int i, j;
  float tmp[12];  /* tmp array for pairs */
  float tsrc[16]; /* array of transpose source matrix */
  float det;      /* determinant */

  /* transpose matrix */
  for (i = 0; i < 4; i++) {
    tsrc[i] = m[i][0];
    tsrc[i + 4] = m[i][1];
    tsrc[i + 8] = m[i][2];
    tsrc[i + 12] = m[i][3];
  }

  /* calculate pair for first 8 elements(cofactors) */
  tmp[0] = tsrc[10] * tsrc[15];
  tmp[1] = tsrc[11] * tsrc[14];
  tmp[2] = tsrc[9] * tsrc[15];
  tmp[3] = tsrc[11] * tsrc[13];
  tmp[4] = tsrc[9] * tsrc[14];
  tmp[5] = tsrc[10] * tsrc[13];
  tmp[6] = tsrc[8] * tsrc[15];
  tmp[7] = tsrc[11] * tsrc[12];
  tmp[8] = tsrc[8] * tsrc[14];
  tmp[9] = tsrc[10] * tsrc[12];
  tmp[10] = tsrc[8] * tsrc[13];
  tmp[11] = tsrc[9] * tsrc[12];

  /* calculate first 8 elements(cofactors) */
  m[0][0] = tmp[0] * tsrc[5] + tmp[3] * tsrc[6] + tmp[4] * tsrc[7];
  m[0][0] -= tmp[1] * tsrc[5] + tmp[2] * tsrc[6] + tmp[5] * tsrc[7];
  m[0][1] = tmp[1] * tsrc[4] + tmp[6] * tsrc[6] + tmp[9] * tsrc[7];
  m[0][1] -= tmp[0] * tsrc[4] + tmp[7] * tsrc[6] + tmp[8] * tsrc[7];
  m[0][2] = tmp[2] * tsrc[4] + tmp[7] * tsrc[5] + tmp[10] * tsrc[7];
  m[0][2] -= tmp[3] * tsrc[4] + tmp[6] * tsrc[5] + tmp[11] * tsrc[7];
  m[0][3] = tmp[5] * tsrc[4] + tmp[8] * tsrc[5] + tmp[11] * tsrc[6];
  m[0][3] -= tmp[4] * tsrc[4] + tmp[9] * tsrc[5] + tmp[10] * tsrc[6];
  m[1][0] = tmp[1] * tsrc[1] + tmp[2] * tsrc[2] + tmp[5] * tsrc[3];
  m[1][0] -= tmp[0] * tsrc[1] + tmp[3] * tsrc[2] + tmp[4] * tsrc[3];
  m[1][1] = tmp[0] * tsrc[0] + tmp[7] * tsrc[2] + tmp[8] * tsrc[3];
  m[1][1] -= tmp[1] * tsrc[0] + tmp[6] * tsrc[2] + tmp[9] * tsrc[3];
  m[1][2] = tmp[3] * tsrc[0] + tmp[6] * tsrc[1] + tmp[11] * tsrc[3];
  m[1][2] -= tmp[2] * tsrc[0] + tmp[7] * tsrc[1] + tmp[10] * tsrc[3];
  m[1][3] = tmp[4] * tsrc[0] + tmp[9] * tsrc[1] + tmp[10] * tsrc[2];
  m[1][3] -= tmp[5] * tsrc[0] + tmp[8] * tsrc[1] + tmp[11] * tsrc[2];

  /* calculate pairs for second 8 elements(cofactors) */
  tmp[0] = tsrc[2] * tsrc[7];
  tmp[1] = tsrc[3] * tsrc[6];
  tmp[2] = tsrc[1] * tsrc[7];
  tmp[3] = tsrc[3] * tsrc[5];
  tmp[4] = tsrc[1] * tsrc[6];
  tmp[5] = tsrc[2] * tsrc[5];
  tmp[6] = tsrc[0] * tsrc[7];
  tmp[7] = tsrc[3] * tsrc[4];
  tmp[8] = tsrc[0] * tsrc[6];
  tmp[9] = tsrc[2] * tsrc[4];
  tmp[10] = tsrc[0] * tsrc[5];
  tmp[11] = tsrc[1] * tsrc[4];

  /* calculate second 8 elements(cofactors) */
  m[2][0] = tmp[0] * tsrc[13] + tmp[3] * tsrc[14] + tmp[4] * tsrc[15];
  m[2][0] -= tmp[1] * tsrc[13] + tmp[2] * tsrc[14] + tmp[5] * tsrc[15];
  m[2][1] = tmp[1] * tsrc[12] + tmp[6] * tsrc[14] + tmp[9] * tsrc[15];
  m[2][1] -= tmp[0] * tsrc[12] + tmp[7] * tsrc[14] + tmp[8] * tsrc[15];
  m[2][2] = tmp[2] * tsrc[12] + tmp[7] * tsrc[13] + tmp[10] * tsrc[15];
  m[2][2] -= tmp[3] * tsrc[12] + tmp[6] * tsrc[13] + tmp[11] * tsrc[15];
  m[2][3] = tmp[5] * tsrc[12] + tmp[8] * tsrc[13] + tmp[11] * tsrc[14];
  m[2][3] -= tmp[4] * tsrc[12] + tmp[9] * tsrc[13] + tmp[10] * tsrc[14];
  m[3][0] = tmp[2] * tsrc[10] + tmp[5] * tsrc[11] + tmp[1] * tsrc[9];
  m[3][0] -= tmp[4] * tsrc[11] + tmp[0] * tsrc[9] + tmp[3] * tsrc[10];
  m[3][1] = tmp[8] * tsrc[11] + tmp[0] * tsrc[8] + tmp[7] * tsrc[10];
  m[3][1] -= tmp[6] * tsrc[10] + tmp[9] * tsrc[11] + tmp[1] * tsrc[8];
  m[3][2] = tmp[6] * tsrc[9] + tmp[11] * tsrc[11] + tmp[3] * tsrc[8];
  m[3][2] -= tmp[10] * tsrc[11] + tmp[2] * tsrc[8] + tmp[7] * tsrc[9];
  m[3][3] = tmp[10] * tsrc[10] + tmp[4] * tsrc[8] + tmp[9] * tsrc[9];
  m[3][3] -= tmp[8] * tsrc[9] + tmp[11] * tsrc[0] + tmp[5] * tsrc[8];

  /* calculate determinant */
  det = tsrc[0] * m[0][0] + tsrc[1] * m[0][1] + tsrc[2] * m[0][2] +
        tsrc[3] * m[0][3];

  /* calculate matrix inverse */
  det = 1.0f / det;

  for (j = 0; j < 4; j++) {
    for (i = 0; i < 4; i++) {
      m[j][i] *= det;
    }
  }
}

void Matrix::Mult(float dst[4][4], float m0[4][4], float m1[4][4]) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      dst[i][j] = 0;
      for (int k = 0; k < 4; ++k) {
        dst[i][j] += m0[k][j] * m1[i][k];
      }
    }
  }
}

void Matrix::MultV(float dst[3], float m[4][4], float v[3]) {
  // printf("v = %f, %f, %f\n", v[0], v[1], v[2]);
  dst[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0];
  dst[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1];
  dst[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2];
  // printf("m = %f, %f, %f\n", m[3][0], m[3][1], m[3][2]);
  // printf("dst = %f, %f, %f\n", dst[0], dst[1], dst[2]);
}
