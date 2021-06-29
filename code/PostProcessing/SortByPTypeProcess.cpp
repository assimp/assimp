/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team



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

/** @file Implementation of the DeterminePTypeHelperProcess and
 *  SortByPTypeProcess post-process steps.
*/

// internal headers
#include "SortByPTypeProcess.h"
#include "ProcessHelper.h"
#include <assimp/Exceptional.h>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
SortByPTypeProcess::SortByPTypeProcess() :
        mConfigRemoveMeshes(0) {
    // empty
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
SortByPTypeProcess::~SortByPTypeProcess() {
    // nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool SortByPTypeProcess::IsActive(unsigned int pFlags) const {
    return (pFlags & aiProcess_SortByPType) != 0;
}

// ------------------------------------------------------------------------------------------------
void SortByPTypeProcess::SetupProperties(const Importer *pImp) {
    mConfigRemoveMeshes = pImp->GetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, 0);
}

// ------------------------------------------------------------------------------------------------
// Update changed meshes in all nodes
void UpdateNodes(const std::vector<unsigned int> &replaceMeshIndex, aiNode *node) {
    if (node->mNumMeshes) {
        unsigned int newSize = 0;
        for (unsigned int m = 0; m < node->mNumMeshes; ++m) {
            unsigned int add = node->mMeshes[m] << 2;
            for (unsigned int i = 0; i < 4; ++i) {
                if (UINT_MAX != replaceMeshIndex[add + i]) ++newSize;
            }
        }
        if (!newSize) {
            delete[] node->mMeshes;
            node->mNumMeshes = 0;
            node->mMeshes = nullptr;
        } else {
            // Try to reuse the old array if possible
            unsigned int *newMeshes = (newSize > node->mNumMeshes ? new unsigned int[newSize] : node->mMeshes);

            for (unsigned int m = 0; m < node->mNumMeshes; ++m) {
                unsigned int add = node->mMeshes[m] << 2;
                for (unsigned int i = 0; i < 4; ++i) {
                    if (UINT_MAX != replaceMeshIndex[add + i])
                        *newMeshes++ = replaceMeshIndex[add + i];
                }
            }
            if (newSize > node->mNumMeshes)
                delete[] node->mMeshes;

            node->mMeshes = newMeshes - (node->mNumMeshes = newSize);
        }
    }

    // call all subnodes recursively
    for (unsigned int m = 0; m < node->mNumChildren; ++m)
        UpdateNodes(replaceMeshIndex, node->mChildren[m]);
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void SortByPTypeProcess::Execute(aiScene *pScene) {
    if (0 == pScene->mNumMeshes) {
        ASSIMP_LOG_DEBUG("SortByPTypeProcess skipped, there are no meshes");
        return;
    }

    ASSIMP_LOG_DEBUG("SortByPTypeProcess begin");

    unsigned int aiNumMeshesPerPType[4] = { 0, 0, 0, 0 };

    std::vector<aiMesh *> outMeshes;
    outMeshes.reserve(pScene->mNumMeshes << 1u);

    bool bAnyChanges = false;

    std::vector<unsigned int> replaceMeshIndex(pScene->mNumMeshes * 4, UINT_MAX);
    std::vector<unsigned int>::iterator meshIdx = replaceMeshIndex.begin();
    for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
        aiMesh *const mesh = pScene->mMeshes[i];
        if (mesh->mPrimitiveTypes == 0) {
            throw DeadlyImportError("Mesh with invalid primitive type: ", mesh->mName.C_Str());
        }

        // if there's just one primitive type in the mesh there's nothing to do for us
        unsigned int num = 0;
        if (mesh->mPrimitiveTypes & aiPrimitiveType_POINT) {
            ++aiNumMeshesPerPType[0];
            ++num;
        }
        if (mesh->mPrimitiveTypes & aiPrimitiveType_LINE) {
            ++aiNumMeshesPerPType[1];
            ++num;
        }
        if (mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) {
            ++aiNumMeshesPerPType[2];
            ++num;
        }
        if (mesh->mPrimitiveTypes & aiPrimitiveType_POLYGON) {
            ++aiNumMeshesPerPType[3];
            ++num;
        }

        if (1 == num) {
            if (!(mConfigRemoveMeshes & mesh->mPrimitiveTypes)) {
                *meshIdx = static_cast<unsigned int>(outMeshes.size());
                outMeshes.push_back(mesh);
            } else {
                delete mesh;
                pScene->mMeshes[i] = nullptr;
                bAnyChanges = true;
            }

            meshIdx += 4;
            continue;
        }
        bAnyChanges = true;

        // reuse our current mesh arrays for the submesh
        // with the largest number of primitives
        unsigned int aiNumPerPType[4] = { 0, 0, 0, 0 };
        aiFace *pFirstFace = mesh->mFaces;
        aiFace *const pLastFace = pFirstFace + mesh->mNumFaces;

        unsigned int numPolyVerts = 0;
        for (; pFirstFace != pLastFace; ++pFirstFace) {
            if (pFirstFace->mNumIndices <= 3)
                ++aiNumPerPType[pFirstFace->mNumIndices - 1];
            else {
                ++aiNumPerPType[3];
                numPolyVerts += pFirstFace->mNumIndices;
            }
        }

        VertexWeightTable *avw = ComputeVertexBoneWeightTable(mesh);
        for (unsigned int real = 0; real < 4; ++real, ++meshIdx) {
            if (!aiNumPerPType[real] || mConfigRemoveMeshes & (1u << real)) {
                continue;
            }

            *meshIdx = (unsigned int)outMeshes.size();
            outMeshes.push_back(new aiMesh());
            aiMesh *out = outMeshes.back();

            // the name carries the adjacency information between the meshes
            out->mName = mesh->mName;

            // copy data members
            out->mPrimitiveTypes = 1u << real;
            out->mMaterialIndex = mesh->mMaterialIndex;

            // allocate output storage
            out->mNumFaces = aiNumPerPType[real];
            aiFace *outFaces = out->mFaces = new aiFace[out->mNumFaces];

            out->mNumVertices = (3 == real ? numPolyVerts : out->mNumFaces * (real + 1));

            aiVector3D *vert(nullptr), *nor(nullptr), *tan(nullptr), *bit(nullptr);
            aiVector3D *uv[AI_MAX_NUMBER_OF_TEXTURECOORDS];
            aiColor4D *cols[AI_MAX_NUMBER_OF_COLOR_SETS];

            if (mesh->mVertices) {
                vert = out->mVertices = new aiVector3D[out->mNumVertices];
            }

            if (mesh->mNormals) {
                nor = out->mNormals = new aiVector3D[out->mNumVertices];
            }

            if (mesh->mTangents) {
                tan = out->mTangents = new aiVector3D[out->mNumVertices];
                bit = out->mBitangents = new aiVector3D[out->mNumVertices];
            }

            for (unsigned int j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++j) {
                uv[j] = nullptr;
                if (mesh->mTextureCoords[j]) {
                    uv[j] = out->mTextureCoords[j] = new aiVector3D[out->mNumVertices];
                }

                out->mNumUVComponents[j] = mesh->mNumUVComponents[j];
            }

            for (unsigned int j = 0; j < AI_MAX_NUMBER_OF_COLOR_SETS; ++j) {
                cols[j] = nullptr;
                if (mesh->mColors[j]) {
                    cols[j] = out->mColors[j] = new aiColor4D[out->mNumVertices];
                }
            }

            if (mesh->mNumAnimMeshes > 0 && mesh->mAnimMeshes) {
                out->mNumAnimMeshes = mesh->mNumAnimMeshes;
                out->mAnimMeshes = new aiAnimMesh *[out->mNumAnimMeshes];
            }

            for (unsigned int j = 0; j < mesh->mNumAnimMeshes; ++j) {
                aiAnimMesh *animMesh = mesh->mAnimMeshes[j];
                aiAnimMesh *outAnimMesh = out->mAnimMeshes[j] = new aiAnimMesh;
                outAnimMesh->mNumVertices = out->mNumVertices;
                if (animMesh->mVertices)
                    outAnimMesh->mVertices = new aiVector3D[out->mNumVertices];
                else
                    outAnimMesh->mVertices = nullptr;
                if (animMesh->mNormals)
                    outAnimMesh->mNormals = new aiVector3D[out->mNumVertices];
                else
                    outAnimMesh->mNormals = nullptr;
                if (animMesh->mTangents)
                    outAnimMesh->mTangents = new aiVector3D[out->mNumVertices];
                else
                    outAnimMesh->mTangents = nullptr;
                if (animMesh->mBitangents)
                    outAnimMesh->mBitangents = new aiVector3D[out->mNumVertices];
                else
                    outAnimMesh->mBitangents = nullptr;
                for (int jj = 0; jj < AI_MAX_NUMBER_OF_COLOR_SETS; ++jj) {
                    if (animMesh->mColors[jj])
                        outAnimMesh->mColors[jj] = new aiColor4D[out->mNumVertices];
                    else
                        outAnimMesh->mColors[jj] = nullptr;
                }
                for (int jj = 0; jj < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++jj) {
                    if (animMesh->mTextureCoords[jj])
                        outAnimMesh->mTextureCoords[jj] = new aiVector3D[out->mNumVertices];
                    else
                        outAnimMesh->mTextureCoords[jj] = nullptr;
                }
            }

            typedef std::vector<aiVertexWeight> TempBoneInfo;
            std::vector<TempBoneInfo> tempBones(mesh->mNumBones);

            // try to guess how much storage we'll need
            for (unsigned int q = 0; q < mesh->mNumBones; ++q) {
                tempBones[q].reserve(mesh->mBones[q]->mNumWeights / (num - 1));
            }

            unsigned int outIdx = 0;
            unsigned int amIdx = 0; // AnimMesh index
            for (unsigned int m = 0; m < mesh->mNumFaces; ++m) {
                aiFace &in = mesh->mFaces[m];
                if ((real == 3 && in.mNumIndices <= 3) || (real != 3 && in.mNumIndices != real + 1)) {
                    continue;
                }

                outFaces->mNumIndices = in.mNumIndices;
                outFaces->mIndices = in.mIndices;

                for (unsigned int q = 0; q < in.mNumIndices; ++q) {
                    unsigned int idx = in.mIndices[q];

                    // process all bones of this index
                    if (avw) {
                        VertexWeightTable &tbl = avw[idx];
                        for (VertexWeightTable::const_iterator it = tbl.begin(), end = tbl.end();
                                it != end; ++it) {
                            tempBones[(*it).first].push_back(aiVertexWeight(outIdx, (*it).second));
                        }
                    }

                    if (vert) {
                        *vert++ = mesh->mVertices[idx];
                        //mesh->mVertices[idx].x = get_qnan();
                    }
                    if (nor) *nor++ = mesh->mNormals[idx];
                    if (tan) {
                        *tan++ = mesh->mTangents[idx];
                        *bit++ = mesh->mBitangents[idx];
                    }

                    for (unsigned int pp = 0; pp < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++pp) {
                        if (!uv[pp]) break;
                        *uv[pp]++ = mesh->mTextureCoords[pp][idx];
                    }

                    for (unsigned int pp = 0; pp < AI_MAX_NUMBER_OF_COLOR_SETS; ++pp) {
                        if (!cols[pp]) break;
                        *cols[pp]++ = mesh->mColors[pp][idx];
                    }

                    unsigned int pp = 0;
                    for (; pp < mesh->mNumAnimMeshes; ++pp) {
                        aiAnimMesh *animMesh = mesh->mAnimMeshes[pp];
                        aiAnimMesh *outAnimMesh = out->mAnimMeshes[pp];
                        if (animMesh->mVertices)
                            outAnimMesh->mVertices[amIdx] = animMesh->mVertices[idx];
                        if (animMesh->mNormals)
                            outAnimMesh->mNormals[amIdx] = animMesh->mNormals[idx];
                        if (animMesh->mTangents)
                            outAnimMesh->mTangents[amIdx] = animMesh->mTangents[idx];
                        if (animMesh->mBitangents)
                            outAnimMesh->mBitangents[amIdx] = animMesh->mBitangents[idx];
                        for (int jj = 0; jj < AI_MAX_NUMBER_OF_COLOR_SETS; ++jj) {
                            if (animMesh->mColors[jj])
                                outAnimMesh->mColors[jj][amIdx] = animMesh->mColors[jj][idx];
                        }
                        for (int jj = 0; jj < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++jj) {
                            if (animMesh->mTextureCoords[jj])
                                outAnimMesh->mTextureCoords[jj][amIdx] = animMesh->mTextureCoords[jj][idx];
                        }
                    }
                    if (pp == mesh->mNumAnimMeshes)
                        amIdx++;

                    in.mIndices[q] = outIdx++;
                }

                in.mIndices = nullptr;
                ++outFaces;
            }
            ai_assert(outFaces == out->mFaces + out->mNumFaces);

            // now generate output bones
            for (unsigned int q = 0; q < mesh->mNumBones; ++q) {
                if (!tempBones[q].empty()) {
                    ++out->mNumBones;
                }
            }

            if (out->mNumBones) {
                out->mBones = new aiBone *[out->mNumBones];
                for (unsigned int q = 0, boneIdx = 0; q < mesh->mNumBones; ++q) {
                    TempBoneInfo &in = tempBones[q];
                    if (in.empty()) {
                        continue;
                    }

                    aiBone *srcBone = mesh->mBones[q];
                    aiBone *bone = out->mBones[boneIdx] = new aiBone();

                    bone->mName = srcBone->mName;
                    bone->mOffsetMatrix = srcBone->mOffsetMatrix;

                    bone->mNumWeights = (unsigned int)in.size();
                    bone->mWeights = new aiVertexWeight[bone->mNumWeights];

                    ::memcpy(bone->mWeights, &in[0], bone->mNumWeights * sizeof(aiVertexWeight));

                    ++boneIdx;
                }
            }
        }

        // delete the per-vertex bone weights table
        delete[] avw;

        // delete the input mesh
        delete mesh;

        // avoid invalid pointer
        pScene->mMeshes[i] = nullptr;
    }

    if (outMeshes.empty()) {
        // This should not occur
        throw DeadlyImportError("No meshes remaining");
    }

    // If we added at least one mesh process all nodes in the node
    // graph and update their respective mesh indices.
    if (bAnyChanges) {
        UpdateNodes(replaceMeshIndex, pScene->mRootNode);
    }

    if (outMeshes.size() != pScene->mNumMeshes) {
        delete[] pScene->mMeshes;
        pScene->mNumMeshes = (unsigned int)outMeshes.size();
        pScene->mMeshes = new aiMesh *[pScene->mNumMeshes];
    }
    ::memcpy(pScene->mMeshes, &outMeshes[0], pScene->mNumMeshes * sizeof(void *));

    if (!DefaultLogger::isNullLogger()) {
        char buffer[1024];
        ::ai_snprintf(buffer, 1024, "Points: %u%s, Lines: %u%s, Triangles: %u%s, Polygons: %u%s (Meshes, X = removed)",
                aiNumMeshesPerPType[0], ((mConfigRemoveMeshes & aiPrimitiveType_POINT) ? "X" : ""),
                aiNumMeshesPerPType[1], ((mConfigRemoveMeshes & aiPrimitiveType_LINE) ? "X" : ""),
                aiNumMeshesPerPType[2], ((mConfigRemoveMeshes & aiPrimitiveType_TRIANGLE) ? "X" : ""),
                aiNumMeshesPerPType[3], ((mConfigRemoveMeshes & aiPrimitiveType_POLYGON) ? "X" : ""));
        ASSIMP_LOG_INFO(buffer);
        ASSIMP_LOG_DEBUG("SortByPTypeProcess finished");
    }
}
