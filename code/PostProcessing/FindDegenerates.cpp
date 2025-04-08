/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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

/** @file  FindDegenerates.cpp
 *  @brief Implementation of the FindDegenerates post-process step.
 */

#include "FindDegenerates.h"
#include "Geometry/GeometryUtils.h"
#include "ProcessHelper.h"

#include <assimp/Exceptional.h>

#include <unordered_map>

using namespace Assimp;

// Correct node indices to meshes and remove references to deleted mesh
static void updateSceneGraph(aiNode *pNode, const std::unordered_map<unsigned int, unsigned int> &meshMap);

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
FindDegeneratesProcess::FindDegeneratesProcess() :
        mConfigRemoveDegenerates(false),
        mConfigCheckAreaOfTriangle(false) {
    // empty
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool FindDegeneratesProcess::IsActive(unsigned int pFlags) const {
    return 0 != (pFlags & aiProcess_FindDegenerates);
}

// ------------------------------------------------------------------------------------------------
// Setup import configuration
void FindDegeneratesProcess::SetupProperties(const Importer *pImp) {
    // Get the current value of AI_CONFIG_PP_FD_REMOVE
    mConfigRemoveDegenerates = (0 != pImp->GetPropertyInteger(AI_CONFIG_PP_FD_REMOVE, 0));
    mConfigCheckAreaOfTriangle = (0 != pImp->GetPropertyInteger(AI_CONFIG_PP_FD_CHECKAREA));
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void FindDegeneratesProcess::Execute(aiScene *pScene) {
    ASSIMP_LOG_DEBUG("FindDegeneratesProcess begin");
    if (nullptr == pScene) {
        return;
    }

    std::unordered_map<unsigned int, unsigned int> meshMap;
    meshMap.reserve(pScene->mNumMeshes);

    const unsigned int originalNumMeshes = pScene->mNumMeshes;
    unsigned int targetIndex = 0;
    for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
        // Do not process point cloud, ExecuteOnMesh works only with faces data
        if ((pScene->mMeshes[i]->mPrimitiveTypes != aiPrimitiveType::aiPrimitiveType_POINT) && ExecuteOnMesh(pScene->mMeshes[i])) {
            delete pScene->mMeshes[i];
            // Not strictly required, but clean:
            pScene->mMeshes[i] = nullptr;
        } else {
            meshMap[i] = targetIndex;
            pScene->mMeshes[targetIndex] = pScene->mMeshes[i];
            ++targetIndex;
        }
    }
    pScene->mNumMeshes = targetIndex;

    if (meshMap.size() < originalNumMeshes) {
        updateSceneGraph(pScene->mRootNode, meshMap);
    }

    ASSIMP_LOG_DEBUG("FindDegeneratesProcess finished");
}

static void updateSceneGraph(aiNode *pNode, const std::unordered_map<unsigned int, unsigned int> &meshMap) {
    unsigned int targetIndex = 0;
    for (unsigned i = 0; i < pNode->mNumMeshes; ++i) {
        const unsigned int sourceMeshIndex = pNode->mMeshes[i];
        auto it = meshMap.find(sourceMeshIndex);
        if (it != meshMap.end()) {
            pNode->mMeshes[targetIndex] = it->second;
            ++targetIndex;
        }
    }
    pNode->mNumMeshes = targetIndex;
    // recurse to all children
    for (unsigned i = 0; i < pNode->mNumChildren; ++i) {
        updateSceneGraph(pNode->mChildren[i], meshMap);
    }
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported mesh
bool FindDegeneratesProcess::ExecuteOnMesh(aiMesh *mesh) {
    mesh->mPrimitiveTypes = 0;

    std::vector<bool> remove_me;
    if (mConfigRemoveDegenerates) {
        remove_me.resize(mesh->mNumFaces, false);
    }

    unsigned int deg = 0, limit;
    for (unsigned int a = 0; a < mesh->mNumFaces; ++a) {
        aiFace &face = mesh->mFaces[a];
        bool first = true;
        auto vertex_in_range = [numVertices = mesh->mNumVertices](unsigned int vertex_idx) { return vertex_idx < numVertices; };

        // check whether the face contains degenerated entries
        for (unsigned int i = 0; i < face.mNumIndices; ++i) {
            if (!std::all_of(face.mIndices, face.mIndices + face.mNumIndices, vertex_in_range))
                continue;

            // Polygons with more than 4 points are allowed to have double points, that is
            // simulating polygons with holes just with concave polygons. However,
            // double points may not come directly after another.
            limit = face.mNumIndices;
            if (face.mNumIndices > 4) {
                limit = std::min(limit, i + 2);
            }

            for (unsigned int t = i + 1; t < limit; ++t) {
                if (mesh->mVertices[face.mIndices[i]] == mesh->mVertices[face.mIndices[t]]) {
                    // we have found a matching vertex position
                    // remove the corresponding index from the array
                    --face.mNumIndices;
                    --limit;
                    for (unsigned int m = t; m < face.mNumIndices; ++m) {
                        face.mIndices[m] = face.mIndices[m + 1];
                    }
                    --t;

                    // NOTE: we set the removed vertex index to an unique value
                    // to make sure the developer gets notified when his
                    // application attempts to access this data.
                    face.mIndices[face.mNumIndices] = 0xdeadbeef;

                    if (first) {
                        ++deg;
                        first = false;
                    }

                    if (mConfigRemoveDegenerates) {
                        remove_me[a] = true;
                        goto evil_jump_outside; // hrhrhrh ... yeah, this rocks baby!
                    }
                }
            }

            if (mConfigCheckAreaOfTriangle) {
                if (face.mNumIndices == 3) {
                    ai_real area = GeometryUtils::calculateAreaOfTriangle(face, mesh);
                    if (area < ai_epsilon) {
                        if (mConfigRemoveDegenerates) {
                            remove_me[a] = true;
                            ++deg;
                            goto evil_jump_outside;
                        }

                        // todo: check for index which is corrupt.
                    }
                }
            }
        }

        // We need to update the primitive flags array of the mesh.
        switch (face.mNumIndices) {
        case 1u:
            mesh->mPrimitiveTypes |= aiPrimitiveType_POINT;
            break;
        case 2u:
            mesh->mPrimitiveTypes |= aiPrimitiveType_LINE;
            break;
        case 3u:
            mesh->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
            break;
        default:
            mesh->mPrimitiveTypes |= aiPrimitiveType_POLYGON;
            break;
        };
    evil_jump_outside:
        continue;
    }

    // If AI_CONFIG_PP_FD_REMOVE is true, remove degenerated faces from the import
    if (mConfigRemoveDegenerates && deg) {
        unsigned int n = 0;
        for (unsigned int a = 0; a < mesh->mNumFaces; ++a) {
            aiFace &face_src = mesh->mFaces[a];
            if (!remove_me[a]) {
                aiFace &face_dest = mesh->mFaces[n++];

                // Do a manual copy, keep the index array
                face_dest.mNumIndices = face_src.mNumIndices;
                face_dest.mIndices = face_src.mIndices;

                if (&face_src != &face_dest) {
                    // clear source
                    face_src.mNumIndices = 0;
                    face_src.mIndices = nullptr;
                }
            } else {
                // Otherwise delete it if we don't need this face
                delete[] face_src.mIndices;
                face_src.mIndices = nullptr;
                face_src.mNumIndices = 0;
            }
        }
        // Just leave the rest of the array unreferenced, we don't care for now
        mesh->mNumFaces = n;
        if (!mesh->mNumFaces) {
            // The whole mesh consists of degenerated faces
            // signal upward, that this mesh should be deleted.
            ASSIMP_LOG_VERBOSE_DEBUG("FindDegeneratesProcess removed a mesh full of degenerated primitives");
            return true;
        }
    }

    if (deg && !DefaultLogger::isNullLogger()) {
        ASSIMP_LOG_WARN("Found ", deg, " degenerated primitives");
    }
    return false;
}
