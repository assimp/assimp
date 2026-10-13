/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

/** @file gaussian.h
 *  @brief ABI-safe side channel for 3D Gaussian Splatting attributes.
 *
 *  Does **not** extend aiMesh / aiScene layout. Splat arrays live in
 *  scene-private storage and are reached via getters. Positions stay in
 *  aiMesh::mVertices (point cloud; 1-index faces per vertex).
 *
 *  Does **not** bake splat attrs into aiMaterial (Kd/Ks/opacity/PBR, …):
 *  values are per-point logits/SH coeffs; conversion is the renderer's job.
 *  See doc/PLY.md for the on-disk PLY convention (f_dc_*, f_rest_*, …).
 */
#pragma once
#ifndef AI_GAUSSIAN_H_INC
#define AI_GAUSSIAN_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/defs.h>
#include <assimp/types.h>
#include <assimp/vector3.h>
#include <assimp/color4.h>

#ifdef __cplusplus
extern "C" {
#endif

struct aiScene;

/** Scene / node metadata: present and true when at least one mesh has splat data. */
#define AI_METADATA_GAUSSIAN_SPLAT "IsGaussianSplat"

/** Scene metadata: number of f_rest coeffs per point (often 0 or 45). */
#define AI_METADATA_GAUSSIAN_SH_REST_COUNT "GaussianSHRestCount"

/**
 * Per-mesh 3DGS attributes (graphdeco / INRIA PLY layout).
 *
 * Positions: use the corresponding aiMesh::mVertices (mNumPoints == mNumVertices).
 * Values are stored as in the file (opacity / scale logits, rot_0..3 order).
 */
struct aiGaussianSplat {
    /** Number of Gaussians (== owning mesh vertex count). */
    unsigned int mNumPoints;

    /** Number of f_rest_* scalars per point (0 if only DC). */
    unsigned int mNumRestCoeffs;

    /** SH DC colour coeffs f_dc_0..2 as xyz. Size: mNumPoints. */
    C_STRUCT aiVector3D *mDC;

    /** Higher-order SH coeffs in file order f_rest_0 .. f_rest_(N-1).
     *  Size: mNumPoints * mNumRestCoeffs. nullptr if mNumRestCoeffs == 0. */
    ai_real *mRest;

    /** Log-scale scale_0..2 as xyz. Size: mNumPoints. */
    C_STRUCT aiVector3D *mScale;

    /** Rotation quaternion rot_0..3 as (r,g,b,a) == (rot_0, rot_1, rot_2, rot_3).
     *  Size: mNumPoints. */
    C_STRUCT aiColor4D *mRotation;

    /** Opacity logits. Size: mNumPoints. */
    ai_real *mOpacity;

#ifdef __cplusplus
    aiGaussianSplat() AI_NO_EXCEPT
            : mNumPoints(0),
              mNumRestCoeffs(0),
              mDC(nullptr),
              mRest(nullptr),
              mScale(nullptr),
              mRotation(nullptr),
              mOpacity(nullptr) {}

    ~aiGaussianSplat() {
        delete[] mDC;
        delete[] mRest;
        delete[] mScale;
        delete[] mRotation;
        delete[] mOpacity;
    }

private:
    aiGaussianSplat(const aiGaussianSplat &) = delete;
    aiGaussianSplat &operator=(const aiGaussianSplat &) = delete;
#endif
};

/**
 * @brief Non-zero if the scene carries any Gaussian splat side data.
 */
ASSIMP_API int aiSceneHasGaussianSplat(const C_STRUCT aiScene *pScene);

/**
 * @brief Get Gaussian attributes for a mesh, or nullptr if none.
 *
 * Lifetime is tied to the aiScene (valid until aiReleaseImport / delete).
 * Not preserved by aiCopyScene / SceneCombiner copies.
 */
ASSIMP_API const C_STRUCT aiGaussianSplat *aiGetGaussianSplat(
        const C_STRUCT aiScene *pScene,
        unsigned int meshIndex);

#ifdef __cplusplus
} // extern "C"

namespace Assimp {

/** Attach ownership of @p splat for @p meshIndex (importer use). Takes ownership. */
void AttachGaussianSplat(aiScene *scene, unsigned int meshIndex, aiGaussianSplat *splat);

} // namespace Assimp
#endif

#endif // AI_GAUSSIAN_H_INC
