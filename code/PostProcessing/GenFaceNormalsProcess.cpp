/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

/** 
 * @file Implementation of the post-processing step to generate face
 * normals for all imported faces.
 */

#include "GenFaceNormalsProcess.h"
#include <assimp/Exceptional.h>
#include <assimp/postprocess.h>
#include <assimp/qnan.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is in the given flag field.
bool GenFaceNormalsProcess::IsActive(unsigned int pFlags) const {
    force_ = (pFlags & aiProcess_ForceGenNormals) != 0;
    flippedWindingOrder_ = (pFlags & aiProcess_FlipWindingOrder) != 0;
    leftHanded_ = (pFlags & aiProcess_MakeLeftHanded) != 0;
    return (pFlags & aiProcess_GenNormals) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post-processing step on the given imported data.
void GenFaceNormalsProcess::Execute(aiScene *pScene) {
    ASSIMP_LOG_DEBUG("GenFaceNormalsProcess begin");

    if (pScene->mFlags & AI_SCENE_FLAGS_NON_VERBOSE_FORMAT) {
        throw DeadlyImportError("Post-processing order mismatch: expecting pseudo-indexed (\"verbose\") vertices here");
    }

    bool bHas = false;
    for (unsigned int a = 0; a < pScene->mNumMeshes; a++) {
        if (this->GenMeshFaceNormals(pScene->mMeshes[a])) {
            bHas = true;
        }
    }
    if (bHas) {
        ASSIMP_LOG_INFO("GenFaceNormalsProcess finished. "
                        "Face normals have been calculated");
    } else {
        ASSIMP_LOG_DEBUG("GenFaceNormalsProcess finished. "
                         "Normals are already there");
    }
}

// ------------------------------------------------------------------------------------------------
// Executes the post-processing step on the given imported data.
bool GenFaceNormalsProcess::GenMeshFaceNormals(aiMesh *pMesh) {
    if (nullptr != pMesh->mNormals) {
        if (force_) {
            delete[] pMesh->mNormals;
        } else {
            return false;
        }
    }

    // If the mesh consists of lines and/or points but not of
    // triangles or higher-order polygons the normal vectors
    // are undefined.
    if (!(pMesh->mPrimitiveTypes & (aiPrimitiveType_TRIANGLE | aiPrimitiveType_POLYGON))) {
        ASSIMP_LOG_INFO("Normal vectors are undefined for line and point meshes");
        return false;
    }

    // allocate an array to hold the output normals
    std::vector<aiVector3D> normals;
    normals.resize(pMesh->mNumVertices);

    // mask to indicate if a vertex was already referenced and needs to be duplicated
    std::vector<bool> alreadyReferenced;
    alreadyReferenced.resize(pMesh->mNumVertices, false);
    std::vector<aiVector3D> duplicatedVertices;

    auto storeNormalSplitVertex = [&](unsigned int index, const aiVector3D& normal) {
        if (!alreadyReferenced[index]) {
            normals[index]           = normal;
            alreadyReferenced[index] = true;
        } else {
            duplicatedVertices.push_back(pMesh->mVertices[index]);
            normals.push_back(normal);
            index = pMesh->mNumVertices + static_cast<unsigned int>(duplicatedVertices.size() - 1);
        }
        return index;
    };

    const aiVector3D undefinedNormal = aiVector3D(get_qnan());

    // iterate through all faces and compute per-face normals but store them per-vertex.
    for (unsigned int a = 0; a < pMesh->mNumFaces; a++) {
        const aiFace &face = pMesh->mFaces[a];
        if (face.mNumIndices < 3) {
            // either a point or a line -> no well-defined normal vector
            for (unsigned int i = 0; i < face.mNumIndices; ++i) {
                face.mIndices[i] = storeNormalSplitVertex(face.mIndices[i], undefinedNormal);
            }
            continue;
        }

        const aiVector3D *pV1 = &pMesh->mVertices[face.mIndices[0]];
        const aiVector3D *pV2 = &pMesh->mVertices[face.mIndices[1]];
        const aiVector3D *pV3 = &pMesh->mVertices[face.mIndices[face.mNumIndices - 1]];
        // Boolean XOR - if either but not both of these flags are set, then the winding order has
        // changed and the cross-product to calculate the normal needs to be reversed
        if (flippedWindingOrder_ != leftHanded_) 
            std::swap(pV2, pV3);
        const aiVector3D vNor = ((*pV2 - *pV1) ^ (*pV3 - *pV1)).NormalizeSafe();

        for (unsigned int i = 0; i < face.mNumIndices; ++i) {
            face.mIndices[i] = storeNormalSplitVertex(face.mIndices[i], vNor);
        }
    }

    // store normals (and additional vertices) back into the mesh
    if (!duplicatedVertices.empty()) {
        const aiVector3D * oldVertices = pMesh->mVertices;
        auto oldNumVertices = pMesh->mNumVertices;
        pMesh->mNumVertices += static_cast<unsigned int>(duplicatedVertices.size());
        pMesh->mVertices = new aiVector3D[pMesh->mNumVertices];
        memcpy(pMesh->mVertices, oldVertices, oldNumVertices * sizeof(aiVector3D));
        memcpy(pMesh->mVertices + oldNumVertices, duplicatedVertices.data(), duplicatedVertices.size() * sizeof(aiVector3D));
        delete[] oldVertices;
    }
    pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];
    memcpy(pMesh->mNormals, normals.data(), normals.size() * sizeof(aiVector3D));

    return true;
}
