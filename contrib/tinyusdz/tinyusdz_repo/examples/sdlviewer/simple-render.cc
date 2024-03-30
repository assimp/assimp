#include "simple-render.hh"

#include <atomic>
#include <cassert>
#include <thread>

#include "nanort.h"
#include "nanosg.h"

// Loading image is the part of TinyUSDZ core
//#define STB_IMAGE_IMPLEMENTATION
//#include "external/stb_image.h"

// common
//#include "mapbox/earcut.hpp"  // For polygon triangulation
#include "matrix.h"
#include "trackball.h"

#define PAR_SHAPES_IMPLEMENTATION
#include "par_shapes.h"  // For meshing


const float kPI = 3.141592f;

typedef nanort::real3<float> float3;

namespace example {

struct DifferentialGeometry {

  float t;               // hit t
  float bary_u, bary_v;  // barycentric coordinate.
  uint32_t geom_id;      // geom id(Currently GeomMesh only)
  float tex_u, tex_v;    // texture u and v

  float3 position;
  float3 shading_normal;
  float3 geometric_normal;
};

struct PointLight {
  float3 position{1000.0f, 1000.0f, 1000.0f};
  float3 color{0.8f, 0.8f, 0.8f};
  float intensity{1.0f};
};

inline float3 Lerp3(float3 v0, float3 v1, float3 v2, float u, float v) {
  return (1.0f - u - v) * v0 + u * v1 + v * v2;
}

inline void CalcNormal(float3& N, float3 v0, float3 v1, float3 v2) {
  float3 v10 = v1 - v0;
  float3 v20 = v2 - v0;

  N = vcross(v10, v20);
  N = vnormalize(N);
}

#if 0
bool LoadTextureImage(const tinyusdz::UVTexture &tex, Image *out_image) {
  
  // Asssume asset name = file name
  std::string filename = tex.asset;

  // TODO: 16bit PNG image, EXR image
  int w, h, channels;
  stbi_uc *image = stbi_load(filename.c_str(), &w, &h, &channels, /* desired_channels */3);     
  if (!image) {
    return false;      
  }

  size_t n = w * h * channels;
  out_image->image.resize(n);
  memcpy(out_image->image.data(), image, n);

  out_image->width = w;
  out_image->height = h;
  out_image->channels = channels;

  return true;
 
}
#endif
    
#if 0
bool ConvertToRenderMesh(const tinyusdz::GeomSphere& sphere,
                         DrawGeomMesh* dst) {
  // TODO: Write our own sphere -> polygon converter

  // TODO: Read subdivision parameter from somewhere.
  int slices = 16;
  int stacks = 8;

  // icohedron subdivision does not generate UV coordinate, so use
  // par_shapes_create_parametric_sphere for now
  par_shapes_mesh* par_mesh =
      par_shapes_create_parametric_sphere(slices, stacks);

  dst->vertices.resize(par_mesh->npoints * 3);

  // TODO: Animated radius
  float radius = 1.0;
  if (sphere.radius.IsTimeSampled()) {
    // TODO
  } else {
    // TODO
    //radius = sphere.radius.Get();
  }

  // scale by radius
  for (size_t i = 0; i < dst->vertices.size(); i++) {
    dst->vertices[i] = par_mesh->points[i] * radius;
  }

  std::vector<uint32_t> facevertex_indices;
  std::vector<float> facevarying_normals;
  std::vector<float> facevarying_texcoords;

  // Make uv and normal facevarying
  // ntriangles = slices * 2 + (stacks - 2) * slices * 2
  for (size_t i = 0; i < par_mesh->ntriangles; i++) {
    PAR_SHAPES_T vidx0 = par_mesh->triangles[3 * i + 0];
    PAR_SHAPES_T vidx1 = par_mesh->triangles[3 * i + 1];
    PAR_SHAPES_T vidx2 = par_mesh->triangles[3 * i + 2];

    facevertex_indices.push_back(vidx0);
    facevertex_indices.push_back(vidx1);
    facevertex_indices.push_back(vidx2);

    facevarying_normals.push_back(par_mesh->normals[3 * vidx0 + 0]);
    facevarying_normals.push_back(par_mesh->normals[3 * vidx0 + 1]);
    facevarying_normals.push_back(par_mesh->normals[3 * vidx0 + 2]);

    facevarying_normals.push_back(par_mesh->normals[3 * vidx1 + 0]);
    facevarying_normals.push_back(par_mesh->normals[3 * vidx1 + 1]);
    facevarying_normals.push_back(par_mesh->normals[3 * vidx1 + 2]);

    facevarying_normals.push_back(par_mesh->normals[3 * vidx2 + 0]);
    facevarying_normals.push_back(par_mesh->normals[3 * vidx2 + 1]);
    facevarying_normals.push_back(par_mesh->normals[3 * vidx2 + 2]);

    facevarying_texcoords.push_back(par_mesh->tcoords[2 * vidx0 + 0]);
    facevarying_texcoords.push_back(par_mesh->tcoords[2 * vidx0 + 1]);

    facevarying_texcoords.push_back(par_mesh->tcoords[2 * vidx1 + 0]);
    facevarying_texcoords.push_back(par_mesh->tcoords[2 * vidx1 + 1]);

    facevarying_texcoords.push_back(par_mesh->tcoords[2 * vidx2 + 0]);
    facevarying_texcoords.push_back(par_mesh->tcoords[2 * vidx2 + 1]);
  }

  par_shapes_free_mesh(par_mesh);

  dst->facevertex_indices = facevertex_indices;

  return true;
}
#endif

bool ConvertToRenderMesh(const tinyusdz::GeomMesh& mesh, DrawGeomMesh* dst) {
#if 0
  // Trianglate mesh
  // vertex points should be vec3f
  if (dst->vertices.size() != (mesh.GetNumPoints() * 3)) {
    std::cerr << __func__ << ":The number of vertices mismatch. " << dst->vertices.size()
              << " must be equal to mesh.GetNumPoints() * 3: " << mesh.GetNumPoints() * 3 << "\n";
    return false;
  }
  dst->vertices.resize(mesh.points.size() * 3);
  memcpy(dst->vertices.data(), mesh.points.data(),
         dst->vertices.size() * sizeof(tinyusdz::value::point3f));
  std::cout << __func__ << "# of mesh.points = " << mesh.points.size() << "\n";

  //std::vector<float> facevarying_normals;
  //if (!mesh.GetFacevaryingNormals(&facevarying_normals)) {
  //  std::cout << __func__ << ":Warn: failed to retrieve facevarying normals\n";
  //}

  //std::vector<float> facevarying_texcoords;
  //if (!mesh.GetFacevaryingTexcoords(&facevarying_texcoords)) {
  //  std::cout << __func__
  //            << ":Warn: failed to retrieve facevarying texcoords\n";
  //}

  //std::cout << "# of facevarying normals = " << facevarying_normals.size() / 3
  //          << "\n";

  //std::cout << "# of faceVertexCounts: " << mesh.faceVertexCounts.size()
  //          << "\n";
  //std::cout << "# of faceVertexIndices: " << mesh.faceVertexIndices.size()
  //          << "\n";

  // for (size_t i = 0; i < facevarying_normals.size() / 3; i++) {
  //  std::cout << "fid[" << i << "] = " << facevarying_normals[3 * i + 0] << ",
  //  " <<
  //                                        facevarying_normals[3 * i + 1] << ",
  //                                        " << facevarying_normals[3 * i + 2]
  //                                        << "\n";
  //}

  // Triangulate mesh
  dst->facevarying_normals.clear();

  // Make facevarying indices
  // TODO(LTE): Make facevarying uvs, ...
  {
    size_t face_offset = 0;
    for (size_t fid = 0; fid < mesh.faceVertexCounts.size(); fid++) {
      int f_count = mesh.faceVertexCounts[fid];

      // std::cout << "f_count = " << f_count << "\n";

      assert(f_count >= 3);

      if (f_count == 3) {
        for (size_t f = 0; f < f_count; f++) {
          dst->facevertex_indices.push_back(
              mesh.faceVertexIndices[face_offset + f]);

#if 0
          if (facevarying_normals.size()) {
            // x, y, z
            dst->facevarying_normals.push_back(
                facevarying_normals[3 * (face_offset + f) + 0]);
            dst->facevarying_normals.push_back(
                facevarying_normals[3 * (face_offset + f) + 1]);
            dst->facevarying_normals.push_back(
                facevarying_normals[3 * (face_offset + f) + 2]);
          }

          if (facevarying_texcoords.size()) {
            // u, v
            dst->facevarying_texcoords.push_back(
                facevarying_texcoords[2 * (face_offset + f) + 0]);
            dst->facevarying_texcoords.push_back(
                facevarying_texcoords[2 * (face_offset + f) + 1]);
          }
#endif
        }

      } else {
        // std::cout << "f_count " << f_count << "\n";

        // Simple triangulation with triangle-fan decomposition
        for (size_t f = 0; f < f_count - 2; f++) {
          size_t f0 = 0;
          size_t f1 = f + 1;
          size_t f2 = f + 2;

          dst->facevertex_indices.push_back(
              mesh.faceVertexIndices[face_offset + f0]);
          dst->facevertex_indices.push_back(
              mesh.faceVertexIndices[face_offset + f1]);
          dst->facevertex_indices.push_back(
              mesh.faceVertexIndices[face_offset + f2]);

#if 0
          if (facevarying_normals.size()) {
            size_t fid0 = face_offset + f0;
            size_t fid1 = face_offset + f1;
            size_t fid2 = face_offset + f2;

            // std::cout << "fid0 = " << fid0 << "\n";

            // x, y, z
            dst->facevarying_normals.push_back(
                facevarying_normals[3 * fid0 + 0]);
            dst->facevarying_normals.push_back(
                facevarying_normals[3 * fid0 + 1]);
            dst->facevarying_normals.push_back(
                facevarying_normals[3 * fid0 + 2]);

            dst->facevarying_normals.push_back(
                facevarying_normals[3 * fid1 + 0]);
            dst->facevarying_normals.push_back(
                facevarying_normals[3 * fid1 + 1]);
            dst->facevarying_normals.push_back(
                facevarying_normals[3 * fid1 + 2]);

            dst->facevarying_normals.push_back(
                facevarying_normals[3 * fid2 + 0]);
            dst->facevarying_normals.push_back(
                facevarying_normals[3 * fid2 + 1]);
            dst->facevarying_normals.push_back(
                facevarying_normals[3 * fid2 + 2]);
          }

          if (facevarying_texcoords.size()) {
            size_t fid0 = face_offset + f0;
            size_t fid1 = face_offset + f1;
            size_t fid2 = face_offset + f2;

            // std::cout << "fid0 = " << fid0 << "\n";

            dst->facevarying_texcoords.push_back(
                facevarying_texcoords[2 * fid0 + 0]);
            dst->facevarying_texcoords.push_back(
                facevarying_texcoords[2 * fid0 + 1]);

            dst->facevarying_texcoords.push_back(
                facevarying_texcoords[2 * fid1 + 0]);
            dst->facevarying_texcoords.push_back(
                facevarying_texcoords[2 * fid1 + 1]);

            dst->facevarying_texcoords.push_back(
                facevarying_texcoords[2 * fid2 + 0]);
            dst->facevarying_texcoords.push_back(
                facevarying_texcoords[2 * fid2 + 1]);
          }
#endif
        }
      }
      face_offset += f_count;
    }
  }

#if 0  // TODO: Rewrite
  // Other facevarying attributes(property, primvars)
  dst->float_primvars.clear();
  dst->float_primvars_map.clear();

  dst->int_primvars.clear();
  dst->int_primvars_map.clear();

  for (const auto& attrib : mesh.attribs) {
    if (attrib.second.interpolation != tinyusdz::Interpolation::InterpolationFaceVarying) {
      std::cerr << "Interpolation must be facevarying\n";
      continue;
    }

    if (attrib.second.buffer.data.empty()) {
      continue;
    }

    if (attrib.second.buffer.GetDataType() ==
        tinyusdz::BufferData::BUFFER_DATA_TYPE_FLOAT) {
      Buffer<float> buf;
      buf.num_coords = attrib.second.buffer.GetNumCoords();
      if (auto p = attrib.second.buffer.GetAsFloatArray()) {
        buf.data = (*p);
      } else {
        std::cerr << "Failed to get attribute value as float array\n";
        continue;
      }

      dst->float_primvars_map[attrib.first] = dst->float_primvars.size();
      dst->float_primvars.push_back(buf);

      std::cout << "Added [" << attrib.first << "] to float_primvars\n";

    } else if (attrib.second.buffer.GetDataType() ==
               tinyusdz::BufferData::BUFFER_DATA_TYPE_INT) {
      Buffer<int32_t> buf;
      buf.num_coords = attrib.second.buffer.GetNumCoords();
      if (auto p = attrib.second.buffer.GetAsInt32Array()) {
        buf.data = (*p);
        std::cerr << "Failed to get attribute value as int array\n";
      }

      dst->int_primvars_map[attrib.first] = dst->int_primvars.size();
      dst->int_primvars.push_back(buf);

      std::cout << "Added [" << attrib.first << "] to int_primvars\n";

    } else {
      // TODO
    }
  }
#endif

  std::cout << "num points = " << dst->vertices.size() / 3 << "\n";
  std::cout << "num triangulated faces = " << dst->facevertex_indices.size() / 3
            << "\n";

#endif
  return false;
}

float3 Shade(const DrawGeomMesh& mesh, const DifferentialGeometry &dg, const PointLight &light) {

  float3 ldir = vnormalize(light.position - dg.position);

  // TODO
  float d = vdot(ldir, vnormalize(dg.shading_normal));
  float ambient = 0.2f;

  d = std::max(ambient, d);

  float3 color{d, d, d};

  return color;
}

void BuildCameraFrame(float3* origin, float3* corner, float3* u, float3* v,
                      const float quat[4], float eye[3], float lookat[3],
                      float up[3], float fov, int width, int height) {
  float e[4][4];

  Matrix::LookAt(e, eye, lookat, up);

  float r[4][4];
  build_rotmatrix(r, quat);

  float3 lo;
  lo[0] = lookat[0] - eye[0];
  lo[1] = lookat[1] - eye[1];
  lo[2] = lookat[2] - eye[2];
  float dist = vlength(lo);

  float dir[3];
  dir[0] = 0.0;
  dir[1] = 0.0;
  dir[2] = dist;

  Matrix::Inverse(r);

  float rr[4][4];
  float re[4][4];
  float zero[3] = {0.0f, 0.0f, 0.0f};
  float localUp[3] = {0.0f, 1.0f, 0.0f};
  Matrix::LookAt(re, dir, zero, localUp);

  // translate
  re[3][0] += eye[0];  // 0.0; //lo[0];
  re[3][1] += eye[1];  // 0.0; //lo[1];
  re[3][2] += (eye[2] - dist);

  // rot -> trans
  Matrix::Mult(rr, r, re);

  float m[4][4];
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      m[j][i] = rr[j][i];
    }
  }

  float vzero[3] = {0.0f, 0.0f, 0.0f};
  float eye1[3];
  Matrix::MultV(eye1, m, vzero);

  float lookat1d[3];
  dir[2] = -dir[2];
  Matrix::MultV(lookat1d, m, dir);
  float3 lookat1(lookat1d[0], lookat1d[1], lookat1d[2]);

  float up1d[3];
  Matrix::MultV(up1d, m, up);

  float3 up1(up1d[0], up1d[1], up1d[2]);

  // absolute -> relative
  up1[0] -= eye1[0];
  up1[1] -= eye1[1];
  up1[2] -= eye1[2];
  // printf("up1(after) = %f, %f, %f\n", up1[0], up1[1], up1[2]);

  // Use original up vector
  // up1[0] = up[0];
  // up1[1] = up[1];
  // up1[2] = up[2];

  {
    float flen =
        (0.5f * (float)height / tanf(0.5f * (float)(fov * kPI / 180.0f)));
    float3 look1;
    look1[0] = lookat1[0] - eye1[0];
    look1[1] = lookat1[1] - eye1[1];
    look1[2] = lookat1[2] - eye1[2];
    // vcross(u, up1, look1);
    // flip
    (*u) = nanort::vcross(look1, up1);
    (*u) = vnormalize((*u));

    (*v) = vcross(look1, (*u));
    (*v) = vnormalize((*v));

    look1 = vnormalize(look1);
    look1[0] = flen * look1[0] + eye1[0];
    look1[1] = flen * look1[1] + eye1[1];
    look1[2] = flen * look1[2] + eye1[2];
    (*corner)[0] = look1[0] - 0.5f * (width * (*u)[0] + height * (*v)[0]);
    (*corner)[1] = look1[1] - 0.5f * (width * (*u)[1] + height * (*v)[1]);
    (*corner)[2] = look1[2] - 0.5f * (width * (*u)[2] + height * (*v)[2]);

    (*origin)[0] = eye1[0];
    (*origin)[1] = eye1[1];
    (*origin)[2] = eye1[2];
  }
}

bool Render(const RenderScene& scene, const Camera& cam, AOV* output) {
  int width = output->width;
  int height = output->height;

  float eye[3] = {cam.eye[0], cam.eye[1], cam.eye[2]};
  float look_at[3] = {cam.look_at[0], cam.look_at[1], cam.look_at[2]};
  float up[3] = {cam.up[0], cam.up[1], cam.up[2]};
  float fov = cam.fov;
  float3 origin, corner, u, v;
  BuildCameraFrame(&origin, &corner, &u, &v, cam.quat, eye, look_at, up, fov,
                   width, height);

  std::vector<std::thread> workers;
  std::atomic<int> i(0);

  uint32_t num_threads = std::max(1U, std::thread::hardware_concurrency());

  // auto startT = std::chrono::system_clock::now();

  for (uint32_t t = 0; t < num_threads; t++) {
    workers.emplace_back(std::thread([&]() {
      int y = 0;
      while ((y = i++) < height) {
        for (int x = 0; x < width; x++) {
          nanort::Ray<float> ray;
          ray.org[0] = origin[0];
          ray.org[1] = origin[1];
          ray.org[2] = origin[2];

          float3 dir;

          float u0 = 0.5f;
          float u1 = 0.5f;

          dir = corner + (float(x) + u0) * u + (float(y) + u1) * v;

          dir = vnormalize(dir);
          ray.dir[0] = dir[0];
          ray.dir[1] = dir[1];
          ray.dir[2] = dir[2];

          size_t pixel_idx = y * width + x;

          bool hit = false;

          output->rgb[3 * pixel_idx + 0] = 0.0f;
          output->rgb[3 * pixel_idx + 1] = 0.0f;
          output->rgb[3 * pixel_idx + 2] = 0.0f;

          if (scene.draw_meshes.size()) {
            // FIXME(syoyo): Use NanoSG to trace meshes in the scene.
            const DrawGeomMesh& mesh = scene.draw_meshes[0];

            // Intersector functor.
            nanort::TriangleIntersector<> triangle_intersector(
                mesh.vertices.data(), mesh.facevertex_indices.data(),
                sizeof(float) * 3);

            nanort::TriangleIntersection<> isect;  // stores isect info

            hit = mesh.accel.Traverse(ray, triangle_intersector, &isect);

            if (hit) {
              float3 Ng;
              {
                // geometric normal.
                float3 v0;
                float3 v1;
                float3 v2;

                size_t vid0 = mesh.facevertex_indices[3 * isect.prim_id + 0];
                size_t vid1 = mesh.facevertex_indices[3 * isect.prim_id + 1];
                size_t vid2 = mesh.facevertex_indices[3 * isect.prim_id + 2];

                v0[0] = mesh.vertices[3 * vid0 + 0];
                v0[1] = mesh.vertices[3 * vid0 + 1];
                v0[2] = mesh.vertices[3 * vid0 + 2];

                v1[0] = mesh.vertices[3 * vid1 + 0];
                v1[1] = mesh.vertices[3 * vid1 + 1];
                v1[2] = mesh.vertices[3 * vid1 + 2];

                v2[0] = mesh.vertices[3 * vid2 + 0];
                v2[1] = mesh.vertices[3 * vid2 + 1];
                v2[2] = mesh.vertices[3 * vid2 + 2];

                CalcNormal(Ng, v0, v1, v2);
              }

              float3 Ns;
              if (mesh.facevarying_normals.size()) {
                float3 n0;
                float3 n1;
                float3 n2;

                n0[0] = mesh.facevarying_normals[9 * isect.prim_id + 0];
                n0[1] = mesh.facevarying_normals[9 * isect.prim_id + 1];
                n0[2] = mesh.facevarying_normals[9 * isect.prim_id + 2];

                n1[0] = mesh.facevarying_normals[9 * isect.prim_id + 3];
                n1[1] = mesh.facevarying_normals[9 * isect.prim_id + 4];
                n1[2] = mesh.facevarying_normals[9 * isect.prim_id + 5];

                n2[0] = mesh.facevarying_normals[9 * isect.prim_id + 6];
                n2[1] = mesh.facevarying_normals[9 * isect.prim_id + 7];
                n2[2] = mesh.facevarying_normals[9 * isect.prim_id + 8];

                // lerp normal.
                Ns = vnormalize(Lerp3(n0, n1, n2, isect.u, isect.v));
              } else {
                Ns = Ng;
              }

              float3 texcoord = {0.0f, 0.0f, 0.0f};
              if (mesh.facevarying_texcoords.size()) {
                float3 t0;
                float3 t1;
                float3 t2;

                t0[0] = mesh.facevarying_texcoords[6 * isect.prim_id + 0];
                t0[1] = mesh.facevarying_texcoords[6 * isect.prim_id + 1];
                t0[2] = 0.0f;

                t1[0] = mesh.facevarying_texcoords[6 * isect.prim_id + 2];
                t1[1] = mesh.facevarying_texcoords[6 * isect.prim_id + 3];
                t1[2] = 0.0f;

                t2[0] = mesh.facevarying_texcoords[6 * isect.prim_id + 4];
                t2[1] = mesh.facevarying_texcoords[6 * isect.prim_id + 5];
                t2[2] = 0.0f;

                texcoord = Lerp3(t0, t1, t2, isect.u, isect.v);
              }

              // For shading
              DifferentialGeometry dg;
              dg.tex_u = texcoord[0];
              dg.tex_v = texcoord[1];
              dg.bary_u = isect.u;
              dg.bary_v = isect.v;
              dg.geom_id = 0;  // FIXME
              dg.geometric_normal = Ng;
              dg.shading_normal = Ns;

              PointLight light; // dummy
              float3 rgb = Shade(mesh, dg, light);

              output->rgb[3 * pixel_idx + 0] = rgb[0];
              output->rgb[3 * pixel_idx + 1] = rgb[1];
              output->rgb[3 * pixel_idx + 2] = rgb[2];

              output->geometric_normal[3 * pixel_idx + 0] = 0.5f * Ns[0] + 0.5f;
              output->geometric_normal[3 * pixel_idx + 1] = 0.5f * Ns[1] + 0.5f;
              output->geometric_normal[3 * pixel_idx + 2] = 0.5f * Ns[2] + 0.5f;

              output->shading_normal[3 * pixel_idx + 0] = 0.5f * Ns[0] + 0.5f;
              output->shading_normal[3 * pixel_idx + 1] = 0.5f * Ns[1] + 0.5f;
              output->shading_normal[3 * pixel_idx + 2] = 0.5f * Ns[2] + 0.5f;

              output->texcoords[2 * pixel_idx + 0] = texcoord[0];
              output->texcoords[2 * pixel_idx + 1] = texcoord[1];
            }
          } else {
          }

          if (!hit) {
            output->geometric_normal[3 * pixel_idx + 0] = 0.0f;
            output->geometric_normal[3 * pixel_idx + 1] = 0.0f;
            output->geometric_normal[3 * pixel_idx + 2] = 0.0f;

            output->shading_normal[3 * pixel_idx + 0] = 0.0f;
            output->shading_normal[3 * pixel_idx + 1] = 0.0f;
            output->shading_normal[3 * pixel_idx + 2] = 0.0f;

            output->texcoords[2 * pixel_idx + 0] = 0.0f;
            output->texcoords[2 * pixel_idx + 1] = 0.0f;
          }
        }
      }
    }));
  }

  for (auto& th : workers) {
    th.join();
  }

  // auto endT = std::chrono::system_clock::now();

  return true;
}

bool RenderLines(int start_y, int end_y, const RenderScene& scene,
                 const Camera& cam, AOV* output) {
  int width = output->width;
  int height = output->height;

  float eye[3] = {cam.eye[0], cam.eye[1], cam.eye[2]};
  float look_at[3] = {cam.look_at[0], cam.look_at[1], cam.look_at[2]};
  float up[3] = {cam.up[0], cam.up[1], cam.up[2]};
  float fov = cam.fov;
  float3 origin, corner, u, v;
  BuildCameraFrame(&origin, &corner, &u, &v, cam.quat, eye, look_at, up, fov,
                   width, height);

  // Single threaded
  for (int y = start_y; y < std::min(end_y, height); y++) {
    for (int x = 0; x < width; x++) {
      nanort::Ray<float> ray;
      ray.org[0] = origin[0];
      ray.org[1] = origin[1];
      ray.org[2] = origin[2];

      float3 dir;

      float u0 = 0.5f;
      float u1 = 0.5f;

      dir = corner + (float(x) + u0) * u + (float(y) + u1) * v;

      dir = vnormalize(dir);
      ray.dir[0] = dir[0];
      ray.dir[1] = dir[1];
      ray.dir[2] = dir[2];

      size_t pixel_idx = y * width + x;

      // HACK. Use the first mesh
      const DrawGeomMesh& mesh = scene.draw_meshes[0];

      // Intersector functor.
      nanort::TriangleIntersector<> triangle_intersector(
          mesh.vertices.data(), mesh.facevertex_indices.data(),
          sizeof(float) * 3);
      nanort::TriangleIntersection<> isect;  // stores isect info

      bool hit = mesh.accel.Traverse(ray, triangle_intersector, &isect);

      if (hit) {
        float3 Ng;
        {
          // geometric normal.
          float3 v0;
          float3 v1;
          float3 v2;

          size_t vid0 = mesh.facevertex_indices[3 * isect.prim_id + 0];
          size_t vid1 = mesh.facevertex_indices[3 * isect.prim_id + 1];
          size_t vid2 = mesh.facevertex_indices[3 * isect.prim_id + 2];

          v0[0] = mesh.vertices[3 * vid0 + 0];
          v0[1] = mesh.vertices[3 * vid0 + 1];
          v0[2] = mesh.vertices[3 * vid0 + 2];

          v1[0] = mesh.vertices[3 * vid1 + 0];
          v1[1] = mesh.vertices[3 * vid1 + 1];
          v1[2] = mesh.vertices[3 * vid1 + 2];

          v2[0] = mesh.vertices[3 * vid2 + 0];
          v2[1] = mesh.vertices[3 * vid2 + 1];
          v2[2] = mesh.vertices[3 * vid2 + 2];

          CalcNormal(Ng, v0, v1, v2);
        }

        float3 Ns;
        if (mesh.facevarying_normals.size()) {
          float3 n0;
          float3 n1;
          float3 n2;

          n0[0] = mesh.facevarying_normals[9 * isect.prim_id + 0];
          n0[1] = mesh.facevarying_normals[9 * isect.prim_id + 1];
          n0[2] = mesh.facevarying_normals[9 * isect.prim_id + 2];

          n1[0] = mesh.facevarying_normals[9 * isect.prim_id + 3];
          n1[1] = mesh.facevarying_normals[9 * isect.prim_id + 4];
          n1[2] = mesh.facevarying_normals[9 * isect.prim_id + 5];

          n2[0] = mesh.facevarying_normals[9 * isect.prim_id + 6];
          n2[1] = mesh.facevarying_normals[9 * isect.prim_id + 7];
          n2[2] = mesh.facevarying_normals[9 * isect.prim_id + 8];

          // lerp normal.
          Ns = vnormalize(Lerp3(n0, n1, n2, isect.u, isect.v));
        } else {
          Ns = Ng;
        }

        float3 texcoord = {0.0f, 0.0f, 0.0f};
        if (mesh.facevarying_texcoords.size()) {
          float3 t0;
          float3 t1;
          float3 t2;

          t0[0] = mesh.facevarying_texcoords[6 * isect.prim_id + 0];
          t0[1] = mesh.facevarying_texcoords[6 * isect.prim_id + 1];
          t0[2] = 0.0f;

          t1[0] = mesh.facevarying_texcoords[6 * isect.prim_id + 2];
          t1[1] = mesh.facevarying_texcoords[6 * isect.prim_id + 3];
          t1[2] = 0.0f;

          t2[0] = mesh.facevarying_texcoords[6 * isect.prim_id + 4];
          t2[1] = mesh.facevarying_texcoords[6 * isect.prim_id + 5];
          t2[2] = 0.0f;

          texcoord = Lerp3(t0, t1, t2, isect.u, isect.v);
        }

        output->rgb[3 * pixel_idx + 0] = 0.5f * Ns[0] + 0.5f;
        output->rgb[3 * pixel_idx + 1] = 0.5f * Ns[1] + 0.5f;
        output->rgb[3 * pixel_idx + 2] = 0.5f * Ns[2] + 0.5f;

        output->geometric_normal[3 * pixel_idx + 0] = 0.5f * Ns[0] + 0.5f;
        output->geometric_normal[3 * pixel_idx + 1] = 0.5f * Ns[1] + 0.5f;
        output->geometric_normal[3 * pixel_idx + 2] = 0.5f * Ns[2] + 0.5f;

        output->shading_normal[3 * pixel_idx + 0] = 0.5f * Ns[0] + 0.5f;
        output->shading_normal[3 * pixel_idx + 1] = 0.5f * Ns[1] + 0.5f;
        output->shading_normal[3 * pixel_idx + 2] = 0.5f * Ns[2] + 0.5f;

        output->texcoords[2 * pixel_idx + 0] = texcoord[0];
        output->texcoords[2 * pixel_idx + 1] = texcoord[1];

      } else {
        output->rgb[3 * pixel_idx + 0] = 0.0f;
        output->rgb[3 * pixel_idx + 1] = 0.0f;
        output->rgb[3 * pixel_idx + 2] = 0.0f;

        output->geometric_normal[3 * pixel_idx + 0] = 0.0f;
        output->geometric_normal[3 * pixel_idx + 1] = 0.0f;
        output->geometric_normal[3 * pixel_idx + 2] = 0.0f;

        output->shading_normal[3 * pixel_idx + 0] = 0.0f;
        output->shading_normal[3 * pixel_idx + 1] = 0.0f;
        output->shading_normal[3 * pixel_idx + 2] = 0.0f;

        output->texcoords[2 * pixel_idx + 0] = 0.0f;
        output->texcoords[2 * pixel_idx + 1] = 0.0f;
      }
    }
  }

  return true;
}

bool RenderScene::Setup() {
  //
  // Construct scene
  //
  {
    float local_xform[4][4];  // TODO

    for (size_t i = 0; i < draw_meshes.size(); i++) {
      // Construct Node by passing the pointer to draw_meshes[i]
      // Pointer address of draw_meshes[i] must be identical during app's
      // lifetime.
      nanosg::Node<float, example::DrawGeomMesh> node(&draw_meshes[i]);

      std::cout << "SetName: " << draw_meshes[i].ref_mesh->name << "\n";

      node.SetName(draw_meshes[i].ref_mesh->name);
      node.SetLocalXform(local_xform);

      this->nodes.push_back(node);
      this->scene.AddNode(node);
    }
  }

  for (size_t i = 0; i < draw_meshes.size(); i++) {
    if (!ConvertToRenderMesh(*(draw_meshes[i].ref_mesh), &draw_meshes[i])) {
      return false;
    }

    DrawGeomMesh& draw_mesh = draw_meshes[i];

    nanort::TriangleMesh<float> triangle_mesh(
        draw_mesh.vertices.data(), draw_mesh.facevertex_indices.data(),
        sizeof(float) * 3);
    nanort::TriangleSAHPred<float> triangle_pred(
        draw_mesh.vertices.data(), draw_mesh.facevertex_indices.data(),
        sizeof(float) * 3);

    bool ret = draw_mesh.accel.Build(draw_mesh.facevertex_indices.size() / 3,
                                     triangle_mesh, triangle_pred);
    if (!ret) {
      std::cerr << "Failed to build BVH\n";
      return false;
    }

    nanort::BVHBuildStatistics stats = draw_mesh.accel.GetStatistics();

    printf("  BVH statistics:\n");
    printf("    # of leaf   nodes: %d\n", stats.num_leaf_nodes);
    printf("    # of branch nodes: %d\n", stats.num_branch_nodes);
    printf("  Max tree depth     : %d\n", stats.max_tree_depth);
    float bmin[3], bmax[3];
    draw_mesh.accel.BoundingBox(bmin, bmax);
    printf("  Bmin               : %f, %f, %f\n", bmin[0], bmin[1], bmin[2]);
    printf("  Bmax               : %f, %f, %f\n", bmax[0], bmax[1], bmax[2]);
  }

  return true;
}

}  // namespace example
