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
#include "UnitTestPCH.h"

#include "AbstractImportExportBase.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>

using namespace ::Assimp;

class utUSDImport : public AbstractImportExportBase {
};

TEST_F(utUSDImport, meshTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/../models-nonbsd/USD/usdc/suzanne.usdc", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(1u, scene->mNumMeshes);
    EXPECT_NE(nullptr, scene->mMeshes[0]);
    EXPECT_EQ(1968u, scene->mMeshes[0]->mNumVertices); // Note: suzanne is authored with only 507 vertices, but TinyUSDZ rebuilds the vertex array. see https://github.com/lighttransport/tinyusdz/blob/36f2aabb256b360365989c01a52f839a57dfe2a6/src/tydra/render-data.cc#L2673-L2690 
    EXPECT_EQ(968u, scene->mMeshes[0]->mNumFaces);
}

TEST_F(utUSDImport, skinnedMeshTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/../models-nonbsd/USD/usda/simple-skin-test.usda", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_TRUE(scene->HasMeshes());

    const aiMesh *mesh = scene->mMeshes[0];
    EXPECT_EQ(2, mesh->mNumBones);

    // Check bone names and make sure scene has nodes of the same name
    EXPECT_EQ(mesh->mBones[0]->mName, aiString("Bone"));
    EXPECT_EQ(mesh->mBones[1]->mName, aiString("Bone/Bone_001"));

    EXPECT_NE(nullptr, scene->mRootNode->FindNode("Bone"));
    EXPECT_NE(nullptr, scene->mRootNode->FindNode("Bone/Bone_001"));
}

TEST_F(utUSDImport, singleAnimationTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/../models-nonbsd/USD/usda/simple-skin-animation-test.usda", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_TRUE(scene->HasAnimations());
    EXPECT_EQ(2, scene->mAnimations[0]->mNumChannels);  // 2 bones. 1 channel for each bone
}

// Note: Add multi-animation test once supported by USD
// See https://github.com/lighttransport/tinyusdz/issues/122 for details.
