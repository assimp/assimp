// SPDX-License-Identifier: Apache 2.0
// Copyright 2020 - 2023 Syoyo Fujita.
// Copyright 2023 - Present Light Transport Entertainment Inc.

#ifdef _MSC_VER
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#endif

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "subdiv.hh"

#include "common-macros.inc"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/topologyDescriptor.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

using namespace OpenSubdiv;

namespace tinyusdz {

//------------------------------------------------------------------------------
// Face-varying container implementation.
//
// We are using a uv texture layout as a 'face-varying' primitive variable
// attribute. Because face-varying data is specified 'per-face-per-vertex',
// we cannot use the same container that we use for 'vertex' or 'varying'
// data. We specify a new container, which only carries (u,v) coordinates.
// Similarly to our 'Vertex' container, we add a minimalistic interpolation
// interface with a 'Clear()' and 'AddWithWeight()' methods.
//
struct FVarVertexUV {
  // Minimal required interface ----------------------
  void Clear() { u = v = 0.0f; }

  void AddWithWeight(FVarVertexUV const &src, float weight) {
    u += weight * src.u;
    v += weight * src.v;
  }

  // Basic 'uv' layout channel
  float u, v;
};

struct FVarVertexColor {
  // Minimal required interface ----------------------
  void Clear() { r = g = b = a = 0.0f; }

  void AddWithWeight(FVarVertexColor const &src, float weight) {
    r += weight * src.r;
    g += weight * src.g;
    b += weight * src.b;
    a += weight * src.a;
  }

  // Basic 'color' layout channel
  float r, g, b, a;
};

bool subdivide(int subd_level, const ControlQuadMesh &in_mesh, SubdividedMesh *out_mesh,
               std::string *err,
               bool dump) {
  if (subd_level < 0) {
    subd_level = 0;
  }

  DCOUT("SubD: level = " << subd_level);

  const auto start_t = std::chrono::system_clock::now();

  int maxlevel = subd_level;

  if (maxlevel > 8) {
    maxlevel = 8;
    DCOUT("SubD: limit subd level to " << maxlevel);
  }

  typedef Far::TopologyDescriptor Descriptor;

  Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_CATMARK;

  Sdc::Options options;
  options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);
  options.SetFVarLinearInterpolation(Sdc::Options::FVAR_LINEAR_NONE);

  // Populate a topology descriptor with our raw data
  Descriptor desc;
  desc.numVertices = int(in_mesh.vertices.size() / 3);
  desc.numFaces = int(in_mesh.verts_per_faces.size());
  desc.numVertsPerFace = in_mesh.verts_per_faces.data();
  desc.vertIndicesPerFace = in_mesh.indices.data();

  // TODO(syoyo): Support various vertex attributes.

#if 0  // TODO
    int channelUV = 0;
    int channelColor = 1;

    // Create a face-varying channel descriptor
    Descriptor::FVarChannel channels[2];
    channels[channelUV].numValues = g_nuvs;
    channels[channelUV].valueIndices = g_uvIndices;
    channels[channelColor].numValues = g_ncolors;
    channels[channelColor].valueIndices = g_colorIndices;

    // Add the channel topology to the main descriptor
    desc.numFVarChannels = 2;
    desc.fvarChannels = channels;
#else
  desc.numFVarChannels = 0;
#endif

  // Instantiate a Far::TopologyRefiner from the descriptor
  Far::TopologyRefiner *refiner =
      Far::TopologyRefinerFactory<Descriptor>::Create(
          desc,
          Far::TopologyRefinerFactory<Descriptor>::Options(type, options));

  // Uniformly refine the topology up to 'maxlevel'
  // note: fullTopologyInLastLevel must be true to work with face-varying data
  {
    Far::TopologyRefiner::UniformOptions refineOptions(maxlevel);
    refineOptions.fullTopologyInLastLevel = true;
    refiner->RefineUniform(refineOptions);
  }

  // Allocate and initialize the 'vertex' primvar data (see tutorial 2 for
  // more details).
  std::vector<Vertex> vbuffer(size_t(refiner->GetNumVerticesTotal()));
  Vertex *verts = &vbuffer[0];

  for (size_t i = 0; i < in_mesh.vertices.size() / 3; ++i) {
    verts[i].SetPosition(in_mesh.vertices[3 * i + 0],
                         in_mesh.vertices[3 * i + 1],
                         in_mesh.vertices[3 * i + 2]);
  }

#if 0
    // Allocate and initialize the first channel of 'face-varying' primvar data (UVs)
    std::vector<FVarVertexUV> fvBufferUV(refiner->GetNumFVarValuesTotal(channelUV));
    FVarVertexUV * fvVertsUV = &fvBufferUV[0];
    for (int i=0; i<g_nuvs; ++i) {
        fvVertsUV[i].u = in_mesh.facevarying_uvs[2 * i + 0];
        fvVertsUV[i].v = in_mesh.facevarying_uvs[2 * i + 1];
    }

    // Allocate & interpolate the 'face-varying' primvar data (colors)
    std::vector<FVarVertexColor> fvBufferColor(refiner->GetNumFVarValuesTotal(channelColor));
    FVarVertexColor * fvVertsColor = &fvBufferColor[0];
    for (int i=0; i<g_ncolors; ++i) {
        fvVertsColor[i].r = g_colors[i][0];
        fvVertsColor[i].g = g_colors[i][1];
        fvVertsColor[i].b = g_colors[i][2];
        fvVertsColor[i].a = g_colors[i][3];
    }
#endif

  // Interpolate both vertex and face-varying primvar data
  Far::PrimvarRefiner primvarRefiner(*refiner);

  Vertex *srcVert = verts;
  // FVarVertexUV * srcFVarUV = fvVertsUV;
  // FVarVertexColor * srcFVarColor = fvVertsColor;

  for (int level = 1; level <= maxlevel; ++level) {
    Vertex *dstVert = srcVert + refiner->GetLevel(level - 1).GetNumVertices();
    // FVarVertexUV * dstFVarUV = srcFVarUV +
    // refiner->GetLevel(level-1).GetNumFVarValues(channelUV); FVarVertexColor *
    // dstFVarColor = srcFVarColor +
    // refiner->GetLevel(level-1).GetNumFVarValues(channelColor);

    primvarRefiner.Interpolate(level, srcVert, dstVert);
    // primvarRefiner.InterpolateFaceVarying(level, srcFVarUV, dstFVarUV,
    // channelUV); primvarRefiner.InterpolateFaceVarying(level, srcFVarColor,
    // dstFVarColor, channelColor);

    srcVert = dstVert;
    // srcFVarUV = dstFVarUV;
    // srcFVarColor = dstFVarColor;
  }

  {  // Output

    std::ofstream ofs;
    if (dump) {
      ofs.open("subd.obj");
    }

    Far::TopologyLevel const &refLastLevel = refiner->GetLevel(maxlevel);

    int nverts = refLastLevel.GetNumVertices();
    // int nuvs   = refLastLevel.GetNumFVarValues(channelUV);
    // int ncolors= refLastLevel.GetNumFVarValues(channelColor);
    int nfaces = refLastLevel.GetNumFaces();

    DCOUT("nverts = " << nverts << ", nfaces = " << nfaces);

    // Print vertex positions
    int firstOfLastVerts = refiner->GetNumVerticesTotal() - nverts;

    out_mesh->vertices.resize(size_t(nverts) * 3);

    for (size_t vert = 0; vert < size_t(nverts); ++vert) {
      float const *pos = verts[size_t(firstOfLastVerts) + vert].GetPosition();
      ofs << "v " << pos[0] << " " << pos[1] << " " << pos[2] << "\n";
      out_mesh->vertices[3 * vert + 0] = pos[0];
      out_mesh->vertices[3 * vert + 1] = pos[1];
      out_mesh->vertices[3 * vert + 2] = pos[2];
    }

#if 0
        // Print uvs
        int firstOfLastUvs = refiner->GetNumFVarValuesTotal(channelUV) - nuvs;

        for (int fvvert = 0; fvvert < nuvs; ++fvvert) {
            FVarVertexUV const & uv = fvVertsUV[firstOfLastUvs + fvvert];
            printf("vt %f %f\n", uv.u, uv.v);
        }

        // Print colors
        int firstOfLastColors = refiner->GetNumFVarValuesTotal(channelColor) - ncolors;

        for (int fvvert = 0; fvvert < nuvs; ++fvvert) {
            FVarVertexColor const & c = fvVertsColor[firstOfLastColors + fvvert];
            printf("c %f %f %f %f\n", c.r, c.g, c.b, c.a);
        }
#endif

    out_mesh->triangulated_indices.clear();
    out_mesh->face_num_verts.clear();
    out_mesh->face_index_offsets.clear();
    out_mesh->face_ids.clear();
    out_mesh->face_triangle_ids.clear();
    out_mesh->material_ids.clear();

    for (int face = 0; face < nfaces; ++face) {
      Far::ConstIndexArray fverts = refLastLevel.GetFaceVertices(face);
      // Far::ConstIndexArray fuvs   = refLastLevel.GetFaceFVarValues(face,
      // channelUV);

      // all refined Catmark faces should be quads
      // assert(fverts.size()==4 && fuvs.size()==4);
      if (fverts.size() != 4) {
        if (err) {
          (*err) += "All refined Catmark faces should be quads.\n";
        }
        return false;
      }

      out_mesh->face_index_offsets.push_back(uint32_t(out_mesh->face_num_verts.size()));

      out_mesh->face_num_verts.push_back(uint8_t(fverts.size()));

      if (dump) {
        ofs << "f";
      }
      for (int vert = 0; vert < fverts.size(); ++vert) {
        out_mesh->face_indices.push_back(uint8_t(fverts[vert]));

        if (dump) {
          // OBJ uses 1-based arrays...
          ofs << " " << fverts[vert] + 1;
        }
      }

      if (dump) {
        ofs << "\n";
      }

      // triangulated face
      out_mesh->triangulated_indices.push_back(uint8_t(fverts[0]));
      out_mesh->triangulated_indices.push_back(uint8_t(fverts[1]));
      out_mesh->triangulated_indices.push_back(uint8_t(fverts[2]));

      out_mesh->triangulated_indices.push_back(uint8_t(fverts[2]));
      out_mesh->triangulated_indices.push_back(uint8_t(fverts[3]));
      out_mesh->triangulated_indices.push_back(uint8_t(fverts[0]));

      // some face attribs.
      out_mesh->face_ids.push_back(uint32_t(face));
      out_mesh->face_ids.push_back(uint32_t(face));

      out_mesh->face_triangle_ids.push_back(0);
      out_mesh->face_triangle_ids.push_back(1);

      // -1 = no material
      out_mesh->material_ids.push_back(-1);
      out_mesh->material_ids.push_back(-1);
    }
  }

  const auto end_t = std::chrono::system_clock::now();
  const double elapsed = double(
      std::chrono::duration_cast<std::chrono::milliseconds>(end_t - start_t)
          .count());

  (void)elapsed;
  DCOUT("SubD time : " << elapsed << " [ms]");

  if (dump) {
    std::cout << "dumped subdivided mesh as `subd.obj`\n";
  }

  return true;
}

}  // namespace tinyusdz
