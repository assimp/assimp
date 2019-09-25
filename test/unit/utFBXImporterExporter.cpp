/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team



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
#include "SceneDiffer.h"
#include "AbstractImportExportBase.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/types.h>

using namespace Assimp;

class utFBXImporterExporter : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/FBX/spider.fbx", aiProcess_ValidateDataStructure );
        return nullptr != scene;
    }
};

TEST_F( utFBXImporterExporter, importXFromFileTest ) {
    EXPECT_TRUE( importerTest() );
}

TEST_F( utFBXImporterExporter, importBareBoxWithoutColorsAndTextureCoords ) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/FBX/box.fbx", aiProcess_ValidateDataStructure );
    EXPECT_NE( nullptr, scene );
    EXPECT_EQ(scene->mNumMeshes, 1u);
    aiMesh* mesh = scene->mMeshes[0];
    EXPECT_EQ(mesh->mNumFaces, 12u);
    EXPECT_EQ(mesh->mNumVertices, 36u);
}

TEST_F(utFBXImporterExporter, importCubesWithNoNames) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/cubes_nonames.fbx", aiProcess_ValidateDataStructure);
    ASSERT_TRUE(scene);

    ASSERT_TRUE(scene->mRootNode);
    const auto root = scene->mRootNode;
    ASSERT_STREQ(root->mName.C_Str(), "RootNode");
    ASSERT_TRUE(root->mChildren);
    ASSERT_EQ(root->mNumChildren, 2u);

    const auto child0 = root->mChildren[0];
    ASSERT_TRUE(child0);
    ASSERT_STREQ(child0->mName.C_Str(), "RootNode001");
    ASSERT_TRUE(child0->mChildren);
    ASSERT_EQ(child0->mNumChildren, 1u);

    const auto child00 = child0->mChildren[0];
    ASSERT_TRUE(child00);
    ASSERT_STREQ(child00->mName.C_Str(), "RootNode001001");

    const auto child1 = root->mChildren[1];
    ASSERT_TRUE(child1);
    ASSERT_STREQ(child1->mName.C_Str(), "RootNode002");
    ASSERT_TRUE(child1->mChildren);
    ASSERT_EQ(child1->mNumChildren, 1u);

    const auto child10 = child1->mChildren[0];
    ASSERT_TRUE(child10);
    ASSERT_STREQ(child10->mName.C_Str(), "RootNode002001");
}

TEST_F(utFBXImporterExporter, importCubesWithUnicodeDuplicatedNames) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/cubes_with_names.fbx", aiProcess_ValidateDataStructure);
    ASSERT_TRUE(scene);

    ASSERT_TRUE(scene->mRootNode);
    const auto root = scene->mRootNode;
    ASSERT_STREQ(root->mName.C_Str(), "RootNode");
    ASSERT_TRUE(root->mChildren);
    ASSERT_EQ(root->mNumChildren, 2u);

    const auto child0 = root->mChildren[0];
    ASSERT_TRUE(child0);
    ASSERT_STREQ(child0->mName.C_Str(), "Cube2");
    ASSERT_TRUE(child0->mChildren);
    ASSERT_EQ(child0->mNumChildren, 1u);

    const auto child00 = child0->mChildren[0];
    ASSERT_TRUE(child00);
    ASSERT_STREQ(child00->mName.C_Str(), "\xd0\x9a\xd1\x83\xd0\xb1\x31");

    const auto child1 = root->mChildren[1];
    ASSERT_TRUE(child1);
    ASSERT_STREQ(child1->mName.C_Str(), "Cube3");
    ASSERT_TRUE(child1->mChildren);
    ASSERT_EQ(child1->mNumChildren, 1u);

    const auto child10 = child1->mChildren[0];
    ASSERT_TRUE(child10);
    ASSERT_STREQ(child10->mName.C_Str(), "\xd0\x9a\xd1\x83\xd0\xb1\x31""001");
}

TEST_F(utFBXImporterExporter, importCubesComplexTransform) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/cubes_with_mirroring_and_pivot.fbx", aiProcess_ValidateDataStructure);
    ASSERT_TRUE(scene);

    ASSERT_TRUE(scene->mRootNode);
    const auto root = scene->mRootNode;
    ASSERT_STREQ(root->mName.C_Str(), "RootNode");
    ASSERT_TRUE(root->mChildren);
    ASSERT_EQ(root->mNumChildren, 2u);

    const auto child0 = root->mChildren[0];
    ASSERT_TRUE(child0);
    ASSERT_STREQ(child0->mName.C_Str(), "Cube2");
    ASSERT_TRUE(child0->mChildren);
    ASSERT_EQ(child0->mNumChildren, 1u);

    const auto child00 = child0->mChildren[0];
    ASSERT_TRUE(child00);
    ASSERT_STREQ(child00->mName.C_Str(), "Cube1");

    const auto child1 = root->mChildren[1];
    ASSERT_TRUE(child1);
    ASSERT_STREQ(child1->mName.C_Str(), "Cube3");

    auto parent = child1;
    const size_t chain_length = 8u;
    const char* chainStr[chain_length] = {
        "Cube1001_$AssimpFbx$_Translation",
        "Cube1001_$AssimpFbx$_RotationPivot",
        "Cube1001_$AssimpFbx$_RotationPivotInverse",
        "Cube1001_$AssimpFbx$_ScalingOffset",
        "Cube1001_$AssimpFbx$_ScalingPivot",
        "Cube1001_$AssimpFbx$_Scaling",
        "Cube1001_$AssimpFbx$_ScalingPivotInverse",
        "Cube1001"
    };
    for (size_t i = 0; i < chain_length; ++i) {
        ASSERT_TRUE(parent->mChildren);
        ASSERT_EQ(parent->mNumChildren, 1u);
        auto node = parent->mChildren[0];
        ASSERT_TRUE(node);
        ASSERT_STREQ(node->mName.C_Str(), chainStr[i]);
        parent = node;
    }
    ASSERT_EQ(0u, parent->mNumChildren) << "Leaf node";
}

TEST_F(utFBXImporterExporter, importCloseToIdentityTransforms) {
    Assimp::Importer importer;
    // This was asserting in FBXConverter.cpp because the transforms appeared to be the identity by one test, but not by another.
    // This asset should now load successfully.
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/close_to_identity_transforms.fbx", aiProcess_ValidateDataStructure);
    ASSERT_TRUE(scene);
}

TEST_F( utFBXImporterExporter, importPhongMaterial ) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/FBX/phong_cube.fbx", aiProcess_ValidateDataStructure );
    EXPECT_NE( nullptr, scene );
    EXPECT_EQ( 1u, scene->mNumMaterials );
    const aiMaterial *mat = scene->mMaterials[0];
    EXPECT_NE( nullptr, mat );
    float f;
    aiColor3D c;

    // phong_cube.fbx has all properties defined
    EXPECT_EQ( mat->Get(AI_MATKEY_COLOR_DIFFUSE, c), aiReturn_SUCCESS );
    EXPECT_EQ( c, aiColor3D(0.5, 0.25, 0.25) );
    EXPECT_EQ( mat->Get(AI_MATKEY_COLOR_SPECULAR, c), aiReturn_SUCCESS );
    EXPECT_EQ( c, aiColor3D(0.25, 0.25, 0.5) );
    EXPECT_EQ( mat->Get(AI_MATKEY_SHININESS_STRENGTH, f), aiReturn_SUCCESS );
    EXPECT_EQ( f, 0.5f );
    EXPECT_EQ( mat->Get(AI_MATKEY_SHININESS, f), aiReturn_SUCCESS );
    EXPECT_EQ( f, 10.0f );
    EXPECT_EQ( mat->Get(AI_MATKEY_COLOR_AMBIENT, c), aiReturn_SUCCESS );
    EXPECT_EQ( c, aiColor3D(0.125, 0.25, 0.25) );
    EXPECT_EQ( mat->Get(AI_MATKEY_COLOR_EMISSIVE, c), aiReturn_SUCCESS );
    EXPECT_EQ( c, aiColor3D(0.25, 0.125, 0.25) );
    EXPECT_EQ( mat->Get(AI_MATKEY_COLOR_TRANSPARENT, c), aiReturn_SUCCESS );
    EXPECT_EQ( c, aiColor3D(0.75, 0.5, 0.25) );
    EXPECT_EQ( mat->Get(AI_MATKEY_OPACITY, f), aiReturn_SUCCESS );
    EXPECT_EQ( f, 0.5f );
}

TEST_F(utFBXImporterExporter, importUnitScaleFactor) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/global_settings.fbx", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
    EXPECT_NE(nullptr, scene->mMetaData);

    double factor(0.0);
    scene->mMetaData->Get("UnitScaleFactor", factor);
    EXPECT_DOUBLE_EQ(500.0, factor);
}

TEST_F(utFBXImporterExporter, importEmbeddedAsciiTest) {
    // see https://github.com/assimp/assimp/issues/1957
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/embedded_ascii/box.FBX", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);

    EXPECT_EQ(1u, scene->mNumMaterials);
    aiMaterial *mat = scene->mMaterials[0];
    ASSERT_NE(nullptr, mat);

    aiString path;
    aiTextureMapMode modes[2];
    EXPECT_EQ(aiReturn_SUCCESS, mat->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr, nullptr, nullptr, modes));
    ASSERT_STREQ(path.C_Str(), "..\\..\\..\\Desktop\\uv_test.png");

    ASSERT_EQ(1u, scene->mNumTextures);
    ASSERT_TRUE(scene->mTextures[0]->pcData);
    ASSERT_EQ(439176u, scene->mTextures[0]->mWidth) << "FBX ASCII base64 compression splits data by 512Kb, it should be two parts for this texture";
}

TEST_F(utFBXImporterExporter, importEmbeddedFragmentedAsciiTest) {
    // see https://github.com/assimp/assimp/issues/1957
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/embedded_ascii/box_embedded_texture_fragmented.fbx", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);

    EXPECT_EQ(1u, scene->mNumMaterials);
    aiMaterial *mat = scene->mMaterials[0];
    ASSERT_NE(nullptr, mat);

    aiString path;
    aiTextureMapMode modes[2];
    ASSERT_EQ(aiReturn_SUCCESS, mat->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr, nullptr, nullptr, modes));
    ASSERT_STREQ(path.C_Str(), "paper.png");

    ASSERT_EQ(1u, scene->mNumTextures);
    ASSERT_TRUE(scene->mTextures[0]->pcData);
    ASSERT_EQ(968029u, scene->mTextures[0]->mWidth) << "FBX ASCII base64 compression splits data by 512Kb, it should be two parts for this texture";
}

TEST_F(utFBXImporterExporter, fbxTokenizeTestTest) {
    //Assimp::Importer importer;
    //const aiScene* scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/transparentTest2.fbx", aiProcess_ValidateDataStructure);
    //EXPECT_NE(nullptr, scene);
}
