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
#include "UnitTestPCH.h"

#include "PostProcessing/TextureTransform.h"

#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <memory>

using namespace ::Assimp;

namespace {

aiScene *CreateSceneWithUVTransform(const void *trafoData, unsigned int trafoDataLength) {
    auto *mat = new aiMaterial;
    aiString path("texture.png");
    mat->AddProperty(&path, AI_MATKEY_TEXTURE_DIFFUSE(0));
    mat->AddBinaryProperty(trafoData, trafoDataLength, "$tex.uvtrafo", aiTextureType_DIFFUSE, 0, aiPTI_Float);

    auto *scene = new aiScene;
    scene->mNumMaterials = 1;
    scene->mMaterials = new aiMaterial *[1];
    scene->mMaterials[0] = mat;
    scene->mRootNode = new aiNode;

    return scene;
}

} // namespace

TEST(utTextureTransform, uvTransformPropertyIsConsumed) {
    const ai_real trafo[5] = { 1, 2, 3, 4, 5 };
    std::unique_ptr<aiScene> scene(CreateSceneWithUVTransform(trafo, sizeof(trafo)));

    Importer importer;
    TextureTransformStep step;
    step.SetupProperties(&importer);
    step.Execute(scene.get());

    aiUVTransform readBack;
    EXPECT_NE(AI_SUCCESS, scene->mMaterials[0]->Get(AI_MATKEY_UVTRANSFORM_DIFFUSE(0), readBack));
}

TEST(utTextureTransform, uvTransformPropertyShorterThanTheTransform) {
    // A $tex.uvtrafo property holding fewer values than an aiUVTransform must not be
    // read beyond its own data.
    const ai_real truncated[2] = { 1, 2 };
    std::unique_ptr<aiScene> scene(CreateSceneWithUVTransform(truncated, sizeof(truncated)));

    Importer importer;
    TextureTransformStep step;
    step.SetupProperties(&importer);
    step.Execute(scene.get());

    SUCCEED();
}
