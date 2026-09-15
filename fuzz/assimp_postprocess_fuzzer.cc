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
#include <assimp/cimport.h>
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Assimp;

// Post-processing steps not applied by the generic or round-trip fuzzers.
static const unsigned int kSteps[] = {
    aiProcess_RemoveComponent,
    aiProcess_FixInfacingNormals,
    aiProcess_PopulateArmatureData,
    aiProcess_TransformUVCoords,
    aiProcess_FindInstances,
    aiProcess_OptimizeMeshes,
    aiProcess_OptimizeGraph,
    aiProcess_SplitByBoneCount,
    aiProcess_Debone,
    aiProcess_GlobalScale,
    aiProcess_EmbedTextures,
    aiProcess_DropNormals,
    aiProcess_GenBoundingBoxes,
};

static const size_t kNumSteps = sizeof(kSteps) / sizeof(kSteps[0]);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t dataSize) {
    // Limit input size to 1MB to prevent OOMs and timeouts
    if (dataSize > 1024 * 1024) {
        return 0;
    }

    // The trailing 4 bytes select the post-processing steps and their parameters.
    if (dataSize < 5) {
        return 0;
    }
    const size_t modelSize = dataSize - 4;
    const uint8_t *ctrl = data + modelSize;

    const unsigned int selector = static_cast<unsigned int>(ctrl[0]) |
                                  (static_cast<unsigned int>(ctrl[1]) << 8);

    unsigned int flags = 0;
    for (size_t i = 0; i < kNumSteps; ++i) {
        if (selector & (1u << i)) {
            flags |= kSteps[i];
        }
    }
    if (flags == 0) {
        return 0;
    }

    Importer importer;
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                              static_cast<float>((ctrl[2] % 64) + 1) * 0.5f);
    importer.SetPropertyInteger(AI_CONFIG_PP_SBBC_MAX_BONES, (ctrl[3] % 64) + 1);

    unsigned int rvc = 0;
    if (ctrl[3] & 0x01) rvc |= aiComponent_NORMALS;
    if (ctrl[3] & 0x02) rvc |= aiComponent_TANGENTS_AND_BITANGENTS;
    if (ctrl[3] & 0x04) rvc |= aiComponent_COLORS;
    if (ctrl[3] & 0x08) rvc |= aiComponent_TEXCOORDS;
    if (ctrl[3] & 0x10) rvc |= aiComponent_BONEWEIGHTS;
    if (ctrl[3] & 0x20) rvc |= aiComponent_ANIMATIONS;
    if (ctrl[3] & 0x40) rvc |= aiComponent_TEXTURES;
    if (ctrl[3] & 0x80) rvc |= aiComponent_LIGHTS;
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, static_cast<int>(rvc));

    const aiScene *sc = importer.ReadFileFromMemory(data, modelSize, 0, nullptr);
    if (sc == nullptr) {
        return 0;
    }

    importer.ApplyPostProcessing(flags);

    return 0;
}
