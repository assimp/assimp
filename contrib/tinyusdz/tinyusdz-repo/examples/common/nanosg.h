/*
The MIT License (MIT)

Copyright (c) 2017 Light Transport Entertainment, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef NANOSG_H_
#define NANOSG_H_

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <iostream>
#include <limits>
#include <vector>

#include "nanort.h"

namespace nanosg {

template <class T>
class PrimitiveInterface;

template <class T>
class PrimitiveInterface {
 public:
  void print() { static_cast<T &>(this)->print(); }
};

class SpherePrimitive : PrimitiveInterface<SpherePrimitive> {
 public:
  void print() { std::cout << "Sphere" << std::endl; }
};

// 4x4 matrix
template <typename T>
class Matrix {
 public:
  Matrix();
  ~Matrix();

  static void Print(const T m[4][4]) {
    for (int i = 0; i < 4; i++) {
      printf("m[%d] = %f, %f, %f, %f\n", i, m[i][0], m[i][1], m[i][2], m[i][3]);
    }
  }

  static void Identity(T m[4][4]) {
    m[0][0] = static_cast<T>(1);
    m[0][1] = static_cast<T>(0);
    m[0][2] = static_cast<T>(0);
    m[0][3] = static_cast<T>(0);
    m[1][0] = static_cast<T>(0);
    m[1][1] = static_cast<T>(1);
    m[1][2] = static_cast<T>(0);
    m[1][3] = static_cast<T>(0);
    m[2][0] = static_cast<T>(0);
    m[2][1] = static_cast<T>(0);
    m[2][2] = static_cast<T>(1);
    m[2][3] = static_cast<T>(0);
    m[3][0] = static_cast<T>(0);
    m[3][1] = static_cast<T>(0);
    m[3][2] = static_cast<T>(0);
    m[3][3] = static_cast<T>(1);
  }

  static void Copy(T dst[4][4], const T src[4][4]) {
    memcpy(dst, src, sizeof(T) * 16);
  }

  static void Inverse(T m[4][4]) {
    /*
     * codes from intel web
     * cramer's rule version
     */
    int i, j;
    T tmp[12];  /* tmp array for pairs */
    T tsrc[16]; /* array of transpose source matrix */
    T det;      /* determinant */

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
    det = static_cast<T>(1.0) / det;

    for (j = 0; j < 4; j++) {
      for (i = 0; i < 4; i++) {
        m[j][i] *= det;
      }
    }
  }

  static void Transpose(T m[4][4]) {
    T t[4][4];

    // Transpose
    for (int j = 0; j < 4; j++) {
      for (int i = 0; i < 4; i++) {
        t[j][i] = m[i][j];
      }
    }

    // Copy
    for (int j = 0; j < 4; j++) {
      for (int i = 0; i < 4; i++) {
        m[j][i] = t[j][i];
      }
    }
  }

  static void Mult(T dst[4][4], const T m0[4][4], const T m1[4][4]) {
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        dst[i][j] = 0;
        for (int k = 0; k < 4; ++k) {
          dst[i][j] += m0[k][j] * m1[i][k];
        }
      }
    }
  }

  static void MultV(T dst[3], const T m[4][4], const T v[3]) {
    T tmp[3];
    tmp[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0];
    tmp[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1];
    tmp[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2];
    dst[0] = tmp[0];
    dst[1] = tmp[1];
    dst[2] = tmp[2];
  }

  static void MultV(nanort::real3<T> &dst, const T m[4][4], const T v[3]) {
    T tmp[3];
    tmp[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0];
    tmp[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1];
    tmp[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2];
    dst[0] = tmp[0];
    dst[1] = tmp[1];
    dst[2] = tmp[2];
  }
};

// typedef Matrix<float> Matrixf;
// typedef Matrix<double> Matrixd;

template <typename T>
static void XformBoundingBox(T xbmin[3],  // out
                             T xbmax[3],  // out
                             T bmin[3], T bmax[3], T m[4][4]) {
  // create bounding vertex from (bmin, bmax)
  T b[8][3];

  b[0][0] = bmin[0];
  b[0][1] = bmin[1];
  b[0][2] = bmin[2];
  b[1][0] = bmax[0];
  b[1][1] = bmin[1];
  b[1][2] = bmin[2];
  b[2][0] = bmin[0];
  b[2][1] = bmax[1];
  b[2][2] = bmin[2];
  b[3][0] = bmax[0];
  b[3][1] = bmax[1];
  b[3][2] = bmin[2];

  b[4][0] = bmin[0];
  b[4][1] = bmin[1];
  b[4][2] = bmax[2];
  b[5][0] = bmax[0];
  b[5][1] = bmin[1];
  b[5][2] = bmax[2];
  b[6][0] = bmin[0];
  b[6][1] = bmax[1];
  b[6][2] = bmax[2];
  b[7][0] = bmax[0];
  b[7][1] = bmax[1];
  b[7][2] = bmax[2];

  T xb[8][3];
  for (int i = 0; i < 8; i++) {
    Matrix<T>::MultV(xb[i], m, b[i]);
  }

  xbmin[0] = xb[0][0];
  xbmin[1] = xb[0][1];
  xbmin[2] = xb[0][2];
  xbmax[0] = xb[0][0];
  xbmax[1] = xb[0][1];
  xbmax[2] = xb[0][2];

  for (int i = 1; i < 8; i++) {
    xbmin[0] = std::min(xb[i][0], xbmin[0]);
    xbmin[1] = std::min(xb[i][1], xbmin[1]);
    xbmin[2] = std::min(xb[i][2], xbmin[2]);

    xbmax[0] = std::max(xb[i][0], xbmax[0]);
    xbmax[1] = std::max(xb[i][1], xbmax[1]);
    xbmax[2] = std::max(xb[i][2], xbmax[2]);
  }
}

///
/// Intersection info struct for Node intersector
///
/// @tparam T Precision(usually `float` or `double`)
///
template <typename T>
struct Intersection {
  // required fields.
  T t;                   // hit distance
  unsigned int prim_id;  // primitive ID of the hit
  T u;
  T v;

  unsigned int node_id;  // node ID of the hit.
  nanort::real3<T> P;    // intersection point
  nanort::real3<T> Ns;   // shading normal
  nanort::real3<T> Ng;   // geometric normal
};

///
/// Renderable node
///
/// @tparam T Type of xform and bounding box(usually `float` or `double`).
/// @tparam M Mesh class
///
template <typename T, class M>
class Node {
 public:
  typedef Node<T, M> type;

  explicit Node(const M *mesh) : mesh_(mesh) {
    xbmin_[0] = xbmin_[1] = xbmin_[2] = std::numeric_limits<T>::max();
    xbmax_[0] = xbmax_[1] = xbmax_[2] = -std::numeric_limits<T>::max();

    lbmin_[0] = lbmin_[1] = lbmin_[2] = std::numeric_limits<T>::max();
    lbmax_[0] = lbmax_[1] = lbmax_[2] = -std::numeric_limits<T>::max();

    Matrix<T>::Identity(local_xform_);
    Matrix<T>::Identity(xform_);
    Matrix<T>::Identity(inv_xform_);
    Matrix<T>::Identity(inv_xform33_);
    inv_xform33_[3][3] = static_cast<T>(0.0);
    Matrix<T>::Identity(inv_transpose_xform33_);
    inv_transpose_xform33_[3][3] = static_cast<T>(0.0);
  }

  ~Node() {}

  void Copy(const type &rhs) {
    Matrix<T>::Copy(local_xform_, rhs.local_xform_);
    Matrix<T>::Copy(xform_, rhs.xform_);
    Matrix<T>::Copy(inv_xform_, rhs.inv_xform_);
    Matrix<T>::Copy(inv_xform33_, rhs.inv_xform33_);
    Matrix<T>::Copy(inv_transpose_xform33_, rhs.inv_transpose_xform33_);

    lbmin_[0] = rhs.lbmin_[0];
    lbmin_[1] = rhs.lbmin_[1];
    lbmin_[2] = rhs.lbmin_[2];

    lbmax_[0] = rhs.lbmax_[0];
    lbmax_[1] = rhs.lbmax_[1];
    lbmax_[2] = rhs.lbmax_[2];

    xbmin_[0] = rhs.xbmin_[0];
    xbmin_[1] = rhs.xbmin_[1];
    xbmin_[2] = rhs.xbmin_[2];

    xbmax_[0] = rhs.xbmax_[0];
    xbmax_[1] = rhs.xbmax_[1];
    xbmax_[2] = rhs.xbmax_[2];

    mesh_ = rhs.mesh_;
    name_ = rhs.name_;

    children_ = rhs.children_;
  }

  Node(const type &rhs) { Copy(rhs); }

  const type &operator=(const type &rhs) {
    Copy(rhs);
    return (*this);
  }

  void SetName(const std::string &name) { name_ = name; }

  const std::string &GetName() const { return name_; }

  ///
  /// Add child node.
  ///
  void AddChild(const type &child) { children_.push_back(child); }

  ///
  /// Get chidren
  ///
  const std::vector<type> &GetChildren() const { return children_; }

  std::vector<type> &GetChildren() { return children_; }

  ///
  /// Update internal state.
  ///
  void Update(const T parent_xform[4][4]) {
    if (!accel_.IsValid() && mesh_ && (mesh_->vertices.size() > 3) &&
        (mesh_->faces.size() >= 3)) {
      // Assume mesh is composed of triangle faces only.
      nanort::TriangleMesh<float> triangle_mesh(
          mesh_->vertices.data(), mesh_->faces.data(), mesh_->stride);
      nanort::TriangleSAHPred<float> triangle_pred(
          mesh_->vertices.data(), mesh_->faces.data(), mesh_->stride);

      bool ret =
          accel_.Build(static_cast<unsigned int>(mesh_->faces.size()) / 3,
                       triangle_mesh, triangle_pred);

      // Update local bbox.
      if (ret) {
        accel_.BoundingBox(lbmin_, lbmax_);
      }
    }

    // xform = parent_xform x local_xform
    Matrix<T>::Mult(xform_, parent_xform, local_xform_);

    // Compute the bounding box in world coordinate.
    XformBoundingBox(xbmin_, xbmax_, lbmin_, lbmax_, xform_);

    // Inverse(xform)
    Matrix<T>::Copy(inv_xform_, xform_);
    Matrix<T>::Inverse(inv_xform_);

    // Clear translation, then inverse(xform)
    Matrix<T>::Copy(inv_xform33_, xform_);
    inv_xform33_[3][0] = static_cast<T>(0.0);
    inv_xform33_[3][1] = static_cast<T>(0.0);
    inv_xform33_[3][2] = static_cast<T>(0.0);
    Matrix<T>::Inverse(inv_xform33_);

    // Inverse transpose of xform33
    Matrix<T>::Copy(inv_transpose_xform33_, inv_xform33_);
    Matrix<T>::Transpose(inv_transpose_xform33_);

    // Update children nodes
    for (size_t i = 0; i < children_.size(); i++) {
      children_[i].Update(xform_);
    }
  }

  ///
  /// Set local transformation.
  ///
  void SetLocalXform(const T xform[4][4]) {
    memcpy(local_xform_, xform, sizeof(float) * 16);
  }

  const T *GetLocalXformPtr() const { return &local_xform_[0][0]; }

  const T *GetXformPtr() const { return &xform_[0][0]; }

  const M *GetMesh() const { return mesh_; }

  const nanort::BVHAccel<T> &GetAccel() const { return accel_; }

  inline void GetWorldBoundingBox(T bmin[3], T bmax[3]) const {
    bmin[0] = xbmin_[0];
    bmin[1] = xbmin_[1];
    bmin[2] = xbmin_[2];

    bmax[0] = xbmax_[0];
    bmax[1] = xbmax_[1];
    bmax[2] = xbmax_[2];
  }

  inline void GetLocalBoundingBox(T bmin[3], T bmax[3]) const {
    bmin[0] = lbmin_[0];
    bmin[1] = lbmin_[1];
    bmin[2] = lbmin_[2];

    bmax[0] = lbmax_[0];
    bmax[1] = lbmax_[1];
    bmax[2] = lbmax_[2];
  }

  T local_xform_[4][4];  // Node's local transformation matrix.
  T xform_[4][4];        // Parent xform x local_xform.
  T inv_xform_[4][4];    // inverse(xform). world -> local
  T inv_xform33_[4][4];  // inverse(xform0 with upper-left 3x3 elemets only(for
                         // transforming direction vector)
  T inv_transpose_xform33_[4][4];  // inverse(transpose(xform)) with upper-left
                                   // 3x3 elements only(for transforming normal
                                   // vector)

 private:
  // bounding box(local space)
  T lbmin_[3];
  T lbmax_[3];

  // bounding box after xform(world space)
  T xbmin_[3];
  T xbmax_[3];

  nanort::BVHAccel<T> accel_;

  std::string name_;

  const M *mesh_{nullptr};

  std::vector<type> children_;
};

// -------------------------------------------------

// Predefined SAH predicator for cube.
template <typename T, class M>
class NodeBBoxPred {
 public:
  NodeBBoxPred(const std::vector<Node<T, M> > *nodes)
      : axis_(0), pos_(0.0f), nodes_(nodes) {}

  void Set(int axis, float pos) const {
    axis_ = axis;
    pos_ = pos;
  }

  bool operator()(unsigned int i) const {
    int axis = axis_;
    float pos = pos_;

    T bmin[3], bmax[3];

    (*nodes_)[i].GetWorldBoundingBox(bmin, bmax);

    T center = bmax[axis] - bmin[axis];

    return (center < pos);
  }

 private:
  mutable int axis_;
  mutable float pos_;
  const std::vector<Node<T, M> > *nodes_;
};

template <typename T, class M>
class NodeBBoxGeometry {
 public:
  NodeBBoxGeometry(const std::vector<Node<T, M> > *nodes) : nodes_(nodes) {}

  /// Compute bounding box for `prim_index`th cube.
  /// This function is called for each primitive in BVH build.
  void BoundingBox(nanort::real3<T> *bmin, nanort::real3<T> *bmax,
                   unsigned int prim_index) const {
    T a[3], b[3];
    (*nodes_)[prim_index].GetWorldBoundingBox(a, b);
    (*bmin)[0] = a[0];
    (*bmin)[1] = a[1];
    (*bmin)[2] = a[2];
    (*bmax)[0] = b[0];
    (*bmax)[1] = b[1];
    (*bmax)[2] = b[2];
  }

  const std::vector<Node<T, M> > *nodes_;
  mutable nanort::real3<T> ray_org_;
  mutable nanort::real3<T> ray_dir_;
  mutable nanort::BVHTraceOptions trace_options_;
  int _pad_;
};

class NodeBBoxIntersection {
 public:
  NodeBBoxIntersection() {}

  float normal[3];

  // Required member variables.
  float t;
  unsigned int prim_id;
};

template <typename T, class M>
class NodeBBoxIntersector {
 public:
  NodeBBoxIntersector(const std::vector<Node<T, M> > *nodes) : nodes_(nodes) {}

  bool Intersect(float *out_t_min, float *out_t_max,
                 unsigned int prim_index) const {
    T bmin[3], bmax[3];

    (*nodes_)[prim_index].GetWorldBoundingBox(bmin, bmax);

    float tmin, tmax;

    const float min_x = ray_dir_sign_[0] ? bmax[0] : bmin[0];
    const float min_y = ray_dir_sign_[1] ? bmax[1] : bmin[1];
    const float min_z = ray_dir_sign_[2] ? bmax[2] : bmin[2];
    const float max_x = ray_dir_sign_[0] ? bmin[0] : bmax[0];
    const float max_y = ray_dir_sign_[1] ? bmin[1] : bmax[1];
    const float max_z = ray_dir_sign_[2] ? bmin[2] : bmax[2];

    // X
    const float tmin_x = (min_x - ray_org_[0]) * ray_inv_dir_[0];
    const float tmax_x = (max_x - ray_org_[0]) * ray_inv_dir_[0];

    // Y
    const float tmin_y = (min_y - ray_org_[1]) * ray_inv_dir_[1];
    const float tmax_y = (max_y - ray_org_[1]) * ray_inv_dir_[1];

    // Z
    const float tmin_z = (min_z - ray_org_[2]) * ray_inv_dir_[2];
    const float tmax_z = (max_z - ray_org_[2]) * ray_inv_dir_[2];

    tmin = nanort::safemax(tmin_z, nanort::safemax(tmin_y, tmin_x));
    tmax = nanort::safemin(tmax_z, nanort::safemin(tmax_y, tmax_x));

    if (tmin <= tmax) {
      (*out_t_min) = tmin;
      (*out_t_max) = tmax;
      return true;
    }

    return false;
  }

  /// Prepare BVH traversal(e.g. compute inverse ray direction)
  /// This function is called only once in BVH traversal.
  void PrepareTraversal(const nanort::Ray<float> &ray) const {
    ray_org_[0] = ray.org[0];
    ray_org_[1] = ray.org[1];
    ray_org_[2] = ray.org[2];

    ray_dir_[0] = ray.dir[0];
    ray_dir_[1] = ray.dir[1];
    ray_dir_[2] = ray.dir[2];

    // FIXME(syoyo): Consider zero div case.
    ray_inv_dir_[0] = static_cast<T>(1.0) / ray.dir[0];
    ray_inv_dir_[1] = static_cast<T>(1.0) / ray.dir[1];
    ray_inv_dir_[2] = static_cast<T>(1.0) / ray.dir[2];

    ray_dir_sign_[0] = ray.dir[0] < static_cast<T>(0.0) ? 1 : 0;
    ray_dir_sign_[1] = ray.dir[1] < static_cast<T>(0.0) ? 1 : 0;
    ray_dir_sign_[2] = ray.dir[2] < static_cast<T>(0.0) ? 1 : 0;
  }

  const std::vector<Node<T, M> > *nodes_;
  mutable nanort::real3<T> ray_org_;
  mutable nanort::real3<T> ray_dir_;
  mutable nanort::real3<T> ray_inv_dir_;
  mutable int ray_dir_sign_[3];
};

template <typename T, class M>
class Scene {
 public:
  Scene() {
    bmin_[0] = bmin_[1] = bmin_[2] = (std::numeric_limits<T>::max)();
    bmax_[0] = bmax_[1] = bmax_[2] = -(std::numeric_limits<T>::max)();
  }

  ~Scene() {}

  ///
  /// Add intersectable node to the scene.
  ///
  bool AddNode(const Node<T, M> &node) {
    nodes_.push_back(node);
    return true;
  }

  const std::vector<Node<T, M> > &GetNodes() const { return nodes_; }

  bool FindNode(const std::string &name, Node<T, M> **found_node) {
    if (!found_node) {
      return false;
    }

    if (name.empty()) {
      return false;
    }

    // Simple exhaustive search.
    for (size_t i = 0; i < nodes_.size(); i++) {
      if (FindNodeRecursive(name, &(nodes_[i]), found_node)) {
        return true;
      }
    }

    return false;
  }

  ///
  /// Commit the scene. Must be called before tracing rays into the scene.
  ///
  bool Commit() {
    // the scene should contains something
    if (nodes_.size() == 0) {
      std::cerr << "You are attempting to commit an empty scene!\n";
      return false;
    }

    // Update nodes.
    for (size_t i = 0; i < nodes_.size(); i++) {
      T ident[4][4];
      Matrix<T>::Identity(ident);

      nodes_[i].Update(ident);
    }

    // Build toplevel BVH.
    NodeBBoxGeometry<T, M> geom(&nodes_);
    NodeBBoxPred<T, M> pred(&nodes_);

    // FIXME(LTE): Limit one leaf contains one node bbox primitive. This would
    // work, but would be inefficient.
    // e.g. will miss some node when constructed BVH depth is larger than the
    // value of BVHBuildOptions.
    // Implement more better and efficient BVH build and traverse for Toplevel
    // BVH.
    nanort::BVHBuildOptions<T> build_options;
    build_options.min_leaf_primitives = 1;

    bool ret = toplevel_accel_.Build(static_cast<unsigned int>(nodes_.size()),
                                     geom, pred, build_options);

    nanort::BVHBuildStatistics stats = toplevel_accel_.GetStatistics();
    (void)stats;

    // toplevel_accel_.Debug();

    if (ret) {
      toplevel_accel_.BoundingBox(bmin_, bmax_);
    } else {
      // Set invalid bbox value.
      bmin_[0] = std::numeric_limits<T>::max();
      bmin_[1] = std::numeric_limits<T>::max();
      bmin_[2] = std::numeric_limits<T>::max();

      bmax_[0] = -std::numeric_limits<T>::max();
      bmax_[1] = -std::numeric_limits<T>::max();
      bmax_[2] = -std::numeric_limits<T>::max();
    }

    return ret;
  }

  ///
  /// Get the scene bounding box.
  ///
  void GetBoundingBox(T bmin[3], T bmax[3]) const {
    bmin[0] = bmin_[0];
    bmin[1] = bmin_[1];
    bmin[2] = bmin_[2];

    bmax[0] = bmax_[0];
    bmax[1] = bmax_[1];
    bmax[2] = bmax_[2];
  }

  ///
  /// Trace the ray into the scene.
  /// First find the intersection of nodes' bounding box using toplevel BVH.
  /// Then, trace into the hit node to find the intersection of the primitive.
  ///
  /// @tparam H Hit intersection info class
  /// @tparam I Intersector class
  ///
  template <class H, class I>
  bool Traverse(nanort::Ray<T> &ray, H *isect,
                const bool cull_back_face = false) const {
    if (!toplevel_accel_.IsValid()) {
      return false;
    }

    const int kMaxIntersections = 64;

    bool has_hit = false;

    NodeBBoxIntersector<T, M> isector(&nodes_);
    nanort::StackVector<nanort::NodeHit<T>, 128> node_hits;
    bool may_hit = toplevel_accel_.ListNodeIntersections(ray, kMaxIntersections,
                                                         isector, &node_hits);

    if (may_hit) {
      T t_max = std::numeric_limits<T>::max();
      T t_nearest = t_max;

      nanort::BVHTraceOptions trace_options;
      trace_options.cull_back_face = cull_back_face;

      // Find actual intersection point.
      for (size_t i = 0; i < node_hits->size(); i++) {
        // Early cull test.
        if (t_nearest < node_hits[i].t_min) {
          // printf("near: %f, t_min: %f, t_max: %f\n", t_nearest,
          // node_hits[i].t_min, node_hits[i].t_max);
          continue;
        }

        assert(node_hits[i].node_id < nodes_.size());
        const Node<T, M> &node = nodes_[node_hits[i].node_id];

        // Transform ray into node's local space
        // TODO(LTE): Set ray tmin and tmax
        nanort::Ray<T> local_ray;
        Matrix<T>::MultV(local_ray.org, node.inv_xform_, ray.org);
        Matrix<T>::MultV(local_ray.dir, node.inv_xform33_, ray.dir);

        // TODO(LTE): Provide custom intersector
        //nanort::TriangleIntersector<T, H> triangle_intersector(
        //    node.GetMesh()->vertices.data(), node.GetMesh()->faces.data(),
        //    node.GetMesh()->stride);
        I intersector(node.GetMesh());
        H local_isect;

        bool hit = node.GetAccel().Traverse(local_ray, intersector,
                                            &local_isect);

        if (hit) {
          // Calulcate hit distance in world coordiante.
          T local_P[3];
          local_P[0] = local_ray.org[0] + local_isect.t * local_ray.dir[0];
          local_P[1] = local_ray.org[1] + local_isect.t * local_ray.dir[1];
          local_P[2] = local_ray.org[2] + local_isect.t * local_ray.dir[2];

          T world_P[3];
          Matrix<T>::MultV(world_P, node.xform_, local_P);

          nanort::real3<T> po;
          po[0] = world_P[0] - ray.org[0];
          po[1] = world_P[1] - ray.org[1];
          po[2] = world_P[2] - ray.org[2];

          float t_world = vlength(po);
          // printf("tworld %f, tnear %f\n", t_world, t_nearest);

          if (t_world < t_nearest) {
            t_nearest = t_world;
            has_hit = true;
            //(*isect) = local_isect;
            isect->node_id = node_hits[i].node_id;
            isect->prim_id = local_isect.prim_id;
            isect->u = local_isect.u;
            isect->v = local_isect.v;

            // TODO(LTE): Implement
            T Ng[3], Ns[3];  // geometric normal, shading normal.

            node.GetMesh()->GetNormal(Ng, Ns, isect->prim_id, isect->u,
                                      isect->v);

            // Convert position and normal into world coordinate.
            isect->t = t_world;
            Matrix<T>::MultV(isect->P, node.xform_, local_P);
            Matrix<T>::MultV(isect->Ng, node.inv_transpose_xform33_, Ng);
            Matrix<T>::MultV(isect->Ns, node.inv_transpose_xform33_, Ns);
          }
        }
      }
    }

    return has_hit;
  }

 private:
  ///
  /// Find a node by name.
  ///
  bool FindNodeRecursive(const std::string &name, Node<T, M> *root,
                         Node<T, M> **found_node) {
    if (root->GetName().compare(name) == 0) {
      (*found_node) = root;
      return true;
    }

    // Simple exhaustive search.
    for (size_t i = 0; i < root->GetChildren().size(); i++) {
      if (FindNodeRecursive(name, &(root->GetChildren()[i]), found_node)) {
        return true;
      }
    }

    return false;
  }

  // Scene bounding box.
  // Valid after calling `Commit()`.
  T bmin_[3];
  T bmax_[3];

  // Toplevel BVH accel.
  nanort::BVHAccel<T> toplevel_accel_;
  std::vector<Node<T, M> > nodes_;
};

}  // namespace nanosg

#endif  // NANOSG_H_
