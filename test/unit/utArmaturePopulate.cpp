/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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
#include "TestModelFactory.h"
#include "UnitTestPCH.h"

#include "AbstractImportExportBase.h"

#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/Importer.hpp>

#include "PostProcessing/ArmaturePopulate.h"

namespace Assimp {
namespace UnitTest {

class utArmaturePopulate : public ::testing::Test {
    // empty
};

TEST_F(utArmaturePopulate, importCheckForArmatureTest) {
    Assimp::Importer importer;
    unsigned int mask = aiProcess_PopulateArmatureData | aiProcess_ValidateDataStructure;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/huesitos.fbx", mask);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mNumMeshes, 1u);
    aiMesh *mesh = scene->mMeshes[0];
    EXPECT_EQ(mesh->mNumFaces, 68u);
    EXPECT_EQ(mesh->mNumVertices, 256u);
    EXPECT_GT(mesh->mNumBones, 0u);

    aiBone *exampleBone = mesh->mBones[0];
    EXPECT_NE(exampleBone, nullptr);
    EXPECT_NE(exampleBone->mArmature, nullptr);
    EXPECT_NE(exampleBone->mNode, nullptr);
}

} // Namespace UnitTest
} // Namespace Assimp
