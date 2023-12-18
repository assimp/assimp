/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2023, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

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
---------------------------------------------------------------------- */
#include "LimitBoneWeightsProcess.h"
#include <assimp/SmallVector.h>
#include <assimp/StringUtils.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/scene.h>
#include <stdio.h>

namespace Assimp {

// Make sure this value is set.
#ifndef AI_LMW_MAX_WEIGHTS
#   define AI_LMW_MAX_WEIGHTS 16
#endif

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
LimitBoneWeightsProcess::LimitBoneWeightsProcess() : mMaxWeights(AI_LMW_MAX_WEIGHTS) {
    // empty
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool LimitBoneWeightsProcess::IsActive( unsigned int pFlags) const {
    return (pFlags & aiProcess_LimitBoneWeights) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void LimitBoneWeightsProcess::Execute( aiScene* pScene) {
    ai_assert(pScene != nullptr);
              
    ASSIMP_LOG_DEBUG("LimitBoneWeightsProcess begin");

    for (unsigned int m = 0; m < pScene->mNumMeshes; ++m) {
        ProcessMesh(pScene->mMeshes[m]);
    }

    ASSIMP_LOG_DEBUG("LimitBoneWeightsProcess end");
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void LimitBoneWeightsProcess::SetupProperties(const Importer* pImp) {
    this->mMaxWeights = pImp->GetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS,AI_LMW_MAX_WEIGHTS);
    this->mRemoveEmptyBones = pImp->GetPropertyInteger(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, 1) != 0;
}

// ------------------------------------------------------------------------------------------------
static unsigned int removeEmptyBones(aiMesh *pMesh) {
    ai_assert(pMesh != nullptr);
    
    unsigned int writeBone = 0;
    for (unsigned int readBone = 0; readBone< pMesh->mNumBones; ++readBone) {
        aiBone* bone = pMesh->mBones[readBone];
        if (bone->mNumWeights > 0) {
            pMesh->mBones[writeBone++] = bone;
        } else {
            delete bone;
        }
    }
    
    return writeBone;
}

// ------------------------------------------------------------------------------------------------
// Unites identical vertices in the given mesh
void LimitBoneWeightsProcess::ProcessMesh(aiMesh* pMesh) {
    if (!pMesh->HasBones())
        return;

    // collect all bone weights per vertex
    typedef SmallVector<Weight,8> VertexWeightArray;
    typedef std::vector<VertexWeightArray> WeightsPerVertex;
    WeightsPerVertex vertexWeights(pMesh->mNumVertices);
    size_t maxVertexWeights = 0;

    for (unsigned int b = 0; b < pMesh->mNumBones; ++b) {
        const aiBone* bone = pMesh->mBones[b];
        for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
            const aiVertexWeight& vw = bone->mWeights[w];

            if (vertexWeights.size() <= vw.mVertexId)
                continue;

            vertexWeights[vw.mVertexId].push_back(Weight(b, vw.mWeight));
            maxVertexWeights = std::max(maxVertexWeights, vertexWeights[vw.mVertexId].size());
        }
    }

    if (maxVertexWeights <= mMaxWeights)
        return;

    unsigned int removed = 0, old_bones = pMesh->mNumBones;

    // now cut the weight count if it exceeds the maximum
    for (WeightsPerVertex::iterator vit = vertexWeights.begin(); vit != vertexWeights.end(); ++vit) {
        if (vit->size() <= mMaxWeights)
            continue;

        // more than the defined maximum -> first sort by weight in descending order. That's
        // why we defined the < operator in such a weird way.
        std::sort(vit->begin(), vit->end());

        // now kill everything beyond the maximum count
        unsigned int m = static_cast<unsigned int>(vit->size());
        vit->resize(mMaxWeights);
        removed += static_cast<unsigned int>(m - vit->size());

        // and renormalize the weights
        float sum = 0.0f;
        for(const Weight* it = vit->begin(); it != vit->end(); ++it) {
            sum += it->mWeight;
        }
        if (0.0f != sum) {
            const float invSum = 1.0f / sum;
            for(Weight* it = vit->begin(); it != vit->end(); ++it) {
                it->mWeight *= invSum;
            }
        }
    }

    // clear weight count for all bone
    for (unsigned int a = 0; a < pMesh->mNumBones; ++a) {
        pMesh->mBones[a]->mNumWeights = 0;
    }

    // rebuild the vertex weight array for all bones
    for (unsigned int a = 0; a < vertexWeights.size(); ++a) {
        const VertexWeightArray& vw = vertexWeights[a];
        for (const Weight* it = vw.begin(); it != vw.end(); ++it) {
            aiBone* bone = pMesh->mBones[it->mBone];
            bone->mWeights[bone->mNumWeights++] = aiVertexWeight(a, it->mWeight);
        }
    }

    // remove empty bones
    if (mRemoveEmptyBones) {
        pMesh->mNumBones = removeEmptyBones(pMesh);
    }

    if (!DefaultLogger::isNullLogger()) {
        ASSIMP_LOG_INFO("Removed ", removed, " weights. Input bones: ", old_bones, ". Output bones: ", pMesh->mNumBones);
    }
}

} // namespace Assimp
