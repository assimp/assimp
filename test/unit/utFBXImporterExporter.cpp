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

#include "AbstractImportExportBase.h"
#include "UnitTestPCH.h"

#include <assimp/commonMetaData.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

class utFBXImporterExporter : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/spider.fbx", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utFBXImporterExporter, importXFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utFBXImporterExporter, importBareBoxWithoutColorsAndTextureCoords) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/box.fbx", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mNumMeshes, 1u);
    aiMesh *mesh = scene->mMeshes[0];
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
    ASSERT_STREQ(child10->mName.C_Str(), "\xd0\x9a\xd1\x83\xd0\xb1\x31");
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
    const char *chainStr[chain_length] = {
        "Cube1_$AssimpFbx$_Translation",
        "Cube1_$AssimpFbx$_RotationPivot",
        "Cube1_$AssimpFbx$_RotationPivotInverse",
        "Cube1_$AssimpFbx$_ScalingOffset",
        "Cube1_$AssimpFbx$_ScalingPivot",
        "Cube1_$AssimpFbx$_Scaling",
        "Cube1_$AssimpFbx$_ScalingPivotInverse",
        "Cube1"
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

TEST_F(utFBXImporterExporter, importPhongMaterial) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/phong_cube.fbx", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(1u, scene->mNumMaterials);
    const aiMaterial *mat = scene->mMaterials[0];
    EXPECT_NE(nullptr, mat);
    float f;
    aiColor3D c;

    // phong_cube.fbx has all properties defined
    EXPECT_EQ(mat->Get(AI_MATKEY_COLOR_DIFFUSE, c), aiReturn_SUCCESS);
    EXPECT_EQ(c, aiColor3D(0.5, 0.25, 0.25));
    EXPECT_EQ(mat->Get(AI_MATKEY_COLOR_SPECULAR, c), aiReturn_SUCCESS);
    EXPECT_EQ(c, aiColor3D(0.25, 0.25, 0.5));
    EXPECT_EQ(mat->Get(AI_MATKEY_SHININESS_STRENGTH, f), aiReturn_SUCCESS);
    EXPECT_EQ(f, 0.5f);
    EXPECT_EQ(mat->Get(AI_MATKEY_SHININESS, f), aiReturn_SUCCESS);
    EXPECT_EQ(f, 10.0f);
    EXPECT_EQ(mat->Get(AI_MATKEY_COLOR_AMBIENT, c), aiReturn_SUCCESS);
    EXPECT_EQ(c, aiColor3D(0.125, 0.25, 0.25));
    EXPECT_EQ(mat->Get(AI_MATKEY_COLOR_EMISSIVE, c), aiReturn_SUCCESS);
    EXPECT_EQ(c, aiColor3D(0.25, 0.125, 0.25));
    EXPECT_EQ(mat->Get(AI_MATKEY_COLOR_TRANSPARENT, c), aiReturn_SUCCESS);
    EXPECT_EQ(c, aiColor3D(0.75, 0.5, 0.25));
    EXPECT_EQ(mat->Get(AI_MATKEY_OPACITY, f), aiReturn_SUCCESS);
    EXPECT_EQ(f, 0.5f);
}

TEST_F(utFBXImporterExporter, importUnitScaleFactor) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/global_settings.fbx", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
    EXPECT_NE(nullptr, scene->mMetaData);

    float factor(0.0f);
    scene->mMetaData->Get("UnitScaleFactor", factor);
    EXPECT_EQ(500.0f, factor);

    scene->mMetaData->Set("UnitScaleFactor", factor * 2.0f);
    scene->mMetaData->Get("UnitScaleFactor", factor);
    EXPECT_EQ(1000.0f, factor);
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

TEST_F(utFBXImporterExporter, importOrphantEmbeddedTextureTest) {
    // see https://github.com/assimp/assimp/issues/1957
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/box_orphant_embedded_texture.fbx", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);

    EXPECT_EQ(1u, scene->mNumMaterials);
    aiMaterial *mat = scene->mMaterials[0];
    ASSERT_NE(nullptr, mat);

    aiString path;
    aiTextureMapMode modes[2];
    ASSERT_EQ(aiReturn_SUCCESS, mat->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr, nullptr, nullptr, modes));
    ASSERT_STREQ(path.C_Str(), "..\\Primitives\\GridGrey.tga");

    ASSERT_EQ(1u, scene->mNumTextures);
    ASSERT_TRUE(scene->mTextures[0]->pcData);
    ASSERT_EQ(9026u, scene->mTextures[0]->mWidth) << "FBX ASCII base64 compression used for a texture.";
}

TEST_F(utFBXImporterExporter, sceneMetadata) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/global_settings.fbx",
            aiProcess_ValidateDataStructure);
    ASSERT_NE(scene, nullptr);
    ASSERT_NE(scene->mMetaData, nullptr);
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_FORMAT));
        aiString format;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT, format));
        ASSERT_EQ(strcmp(format.C_Str(), "Autodesk FBX Importer"), 0);
    }
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_FORMAT_VERSION));
        aiString version;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT_VERSION, version));
        ASSERT_EQ(strcmp(version.C_Str(), "7400"), 0);
    }
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_GENERATOR));
        aiString generator;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_GENERATOR, generator));
        ASSERT_EQ(strncmp(generator.C_Str(), "Blender", 7), 0);
    }
}

TEST_F(utFBXImporterExporter, importCubesWithOutOfRangeFloat) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/cubes_with_outofrange_float.fbx", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_TRUE(scene->mRootNode);
}

TEST_F(utFBXImporterExporter, importMaxPbrMaterialsMetalRoughness) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/maxPbrMaterial_metalRough.fbx", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_TRUE(scene->mRootNode);

    ASSERT_EQ(scene->mNumMaterials, 1u);
    const aiMaterial* mat = scene->mMaterials[0];
    aiString texture;
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_BASE_COLOR, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\albedo.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\metalness.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_EMISSION_COLOR, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\emission.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMAL_CAMERA, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\normal.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\roughness.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_AMBIENT_OCCLUSION, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\occlusion.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_OPACITY, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\opacity.png"));

    // The material contains values for standard properties (e.g. SpecularColor), where 3ds Max has presumably
    // used formulas to map the Pbr values into the standard material model. However, the pbr values themselves
    // are available in the material as untyped "raw" properties. We check that these are correctly parsed:

    aiColor4D baseColor;
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|basecolor", aiTextureType_NONE, 0, baseColor), aiReturn_SUCCESS);
    EXPECT_EQ(baseColor, aiColor4D(0, 1, 1, 1));

    float metalness;
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|metalness", aiTextureType_NONE, 0, metalness), aiReturn_SUCCESS);
    EXPECT_EQ(metalness, 0.25f);

    float roughness;
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|roughness", aiTextureType_NONE, 0, roughness), aiReturn_SUCCESS);
    EXPECT_EQ(roughness, 0.5f);

    int useGlossiness;
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|useGlossiness", aiTextureType_NONE, 0, useGlossiness), aiReturn_SUCCESS);
    EXPECT_EQ(useGlossiness, 2); // 1 = Roughness map is glossiness, 2 = Roughness map is roughness.

    float bumpMapAmt; // Presumably amount.
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|bump_map_amt", aiTextureType_NONE, 0, bumpMapAmt), aiReturn_SUCCESS);
    EXPECT_EQ(bumpMapAmt, 0.75f);

    aiColor4D emitColor;
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|emit_color", aiTextureType_NONE, 0, emitColor), aiReturn_SUCCESS);
    EXPECT_EQ(emitColor, aiColor4D(1, 1, 0, 1));
}

TEST_F(utFBXImporterExporter, importMaxPbrMaterialsSpecularGloss) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/maxPbrMaterial_specGloss.fbx", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_TRUE(scene->mRootNode);

    ASSERT_EQ(scene->mNumMaterials, 1u);
    const aiMaterial* mat = scene->mMaterials[0];
    aiString texture;
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_BASE_COLOR, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\albedo.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\specular.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_EMISSION_COLOR, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\emission.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMAL_CAMERA, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\normal.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_SHININESS, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\glossiness.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_AMBIENT_OCCLUSION, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\occlusion.png"));
    ASSERT_EQ(mat->Get(AI_MATKEY_TEXTURE(aiTextureType_OPACITY, 0), texture), AI_SUCCESS);
    EXPECT_EQ(texture, aiString("Textures\\opacity.png"));

    // The material contains values for standard properties (e.g. SpecularColor), where 3ds Max has presumably
    // used formulas to map the Pbr values into the standard material model. However, the pbr values themselves
    // are available in the material as untyped "raw" properties. We check that these are correctly parsed:

    aiColor4D baseColor;
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|basecolor", aiTextureType_NONE, 0, baseColor), aiReturn_SUCCESS);
    EXPECT_EQ(baseColor, aiColor4D(0, 1, 1, 1));

    aiColor4D specular;
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|Specular", aiTextureType_NONE, 0, specular), aiReturn_SUCCESS);
    EXPECT_EQ(specular, aiColor4D(1, 1, 0, 1));

    float glossiness;
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|glossiness", aiTextureType_NONE, 0, glossiness), aiReturn_SUCCESS);
    EXPECT_EQ(glossiness, 0.33f);

    int useGlossiness;
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|useGlossiness", aiTextureType_NONE, 0, useGlossiness), aiReturn_SUCCESS);
    EXPECT_EQ(useGlossiness, 1); // 1 = Glossiness map is glossiness, 2 = Glossiness map is roughness.

    float bumpMapAmt; // Presumably amount.
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|bump_map_amt", aiTextureType_NONE, 0, bumpMapAmt), aiReturn_SUCCESS);
    EXPECT_EQ(bumpMapAmt, 0.66f);

    aiColor4D emitColor;
    ASSERT_EQ(mat->Get("$raw.3dsMax|main|emit_color", aiTextureType_NONE, 0, emitColor), aiReturn_SUCCESS);
    EXPECT_EQ(emitColor, aiColor4D(1, 0, 1, 1));
}

TEST_F(utFBXImporterExporter, importSkeletonTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/animation_with_skeleton.fbx", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_TRUE(scene->mRootNode);
}
