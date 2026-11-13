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

#include "AbstractImportExportBase.h"
#include "UnitTestPCH.h"

#include <assimp/commonMetaData.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/Importer.hpp>

#include <fstream>
#include <map>
#include <sstream>

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

TEST_F(utFBXImporterExporter, importEmbeddedTextureNumericUV) {
    for (const char *variant : { "integer", "float", "double", "override" }) {
        SCOPED_TRACE(variant);
        const std::string path = std::string(ASSIMP_TEST_MODELS_DIR) + "/FBX/embedded_" + variant + "_uv.fbx";
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene) << importer.GetErrorString();
        ASSERT_EQ(1u, scene->mNumMeshes);
        ASSERT_EQ(1u, scene->mNumMaterials);
        ASSERT_EQ(1u, scene->mNumTextures);
        ASSERT_EQ(21u, scene->mTextures[0]->mWidth);
        EXPECT_EQ(0u, scene->mTextures[0]->mHeight);
        ASSERT_NE(nullptr, scene->mTextures[0]->pcData);
        const auto *pixel = reinterpret_cast<const unsigned char *>(scene->mTextures[0]->pcData);
        EXPECT_EQ(255u, pixel[20]);
        EXPECT_STREQ("tga", scene->mTextures[0]->achFormatHint);
        const aiMaterial *material = scene->mMaterials[0];
        aiString texture;
        ASSERT_EQ(AI_SUCCESS, material->GetTexture(aiTextureType_DIFFUSE, 0, &texture));
        EXPECT_EQ(scene->mTextures[0], scene->GetEmbeddedTexture(texture.C_Str()));
        aiUVTransform uv;
        ASSERT_EQ(AI_SUCCESS, material->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, 0), uv));
        const bool overridden = std::string(variant) == "override";
        EXPECT_FLOAT_EQ(overridden ? 6.f : -2.f, uv.mTranslation.x);
        EXPECT_FLOAT_EQ(overridden ? 7.f : 3.f, uv.mTranslation.y);
        EXPECT_FLOAT_EQ(overridden ? 8.f : 4.f, uv.mScaling.x);
        EXPECT_FLOAT_EQ(overridden ? 9.f : 5.f, uv.mScaling.y);
    }
}

TEST_F(utFBXImporterExporter, rejectNonNumericTextureUV) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/embedded_invalid_uv.fbx", 0);
    ASSERT_NE(nullptr, scene) << importer.GetErrorString();
    ASSERT_EQ(1u, scene->mNumMaterials);
    EXPECT_EQ(0u, scene->mNumTextures);
    EXPECT_EQ(0u, scene->mMaterials[0]->GetTextureCount(aiTextureType_DIFFUSE));
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

TEST_F(utFBXImporterExporter, importCustomAxes) {
    // see https://github.com/assimp/assimp/issues/5494
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/embedded_ascii/box.FBX", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);

    // The ASCII box has customised the Up and Forward axes, verify that the RootNode transform has applied it
    ASSERT_FALSE(scene->mRootNode->mTransformation.IsIdentity()) << "Did not apply the custom axis transform";

    aiVector3D upVec{ 0, 0, 1 }; // Up is +Z
    aiVector3D forwardVec{ 0, -1, 0 }; // Forward is -Y
    aiVector3D rightVec{ 1, 0, 0 }; // Right is +X
    aiMatrix4x4 mat(rightVec.x, rightVec.y, rightVec.z, 0.0f,
            upVec.x, upVec.y, upVec.z, 0.0f,
            forwardVec.x, forwardVec.y, forwardVec.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

    EXPECT_EQ(mat, scene->mRootNode->mTransformation);
}

TEST_F(utFBXImporterExporter, importIgnoreCustomAxes) {
    // see https://github.com/assimp/assimp/issues/5494
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_IGNORE_UP_DIRECTION, true);
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/embedded_ascii/box.FBX", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);

    // Verify that the RootNode transform has NOT applied the custom axes
    EXPECT_TRUE(scene->mRootNode->mTransformation.IsIdentity());
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

namespace {

std::string fbxTriangleWithLastIndex(const char *lastIndex) {
    return std::string(
            "; FBX 7.4.0 project file\n"
            "FBXHeaderExtension:  {\n"
            "\tFBXHeaderVersion: 1003\n"
            "\tFBXVersion: 7400\n"
            "}\n"
            "Objects:  {\n"
            "\tGeometry: 10, \"Geometry::\", \"Mesh\" {\n"
            "\t\tVertices: *9 {\n"
            "\t\t\ta: 0,0,0,1,0,0,0,1,0\n"
            "\t\t}\n"
            "\t\tPolygonVertexIndex: *3 {\n"
            "\t\t\ta: 0,1,") +
           lastIndex +
           "\n"
           "\t\t}\n"
           "\t}\n"
           "\tModel: 20, \"Model::triangle\", \"Mesh\" {\n"
           "\t}\n"
           "}\n"
           "Connections:  {\n"
           "\tC: \"OO\",10,20\n"
           "\tC: \"OO\",20,0\n"
           "}\n";
}

} // namespace

TEST_F(utFBXImporterExporter, importPolygonVertexIndexIntMin) {
    // The last index of a polygon is stored as -(vertex + 1); decoding INT_MIN
    // must not overflow, and it is out of range for the mesh.
    Assimp::Importer importer;
    const std::string valid = fbxTriangleWithLastIndex("-3");
    const aiScene *scene = importer.ReadFileFromMemory(valid.data(), valid.size(), aiProcess_ValidateDataStructure, "fbx");
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(1u, scene->mNumMeshes);
    EXPECT_EQ(1u, scene->mMeshes[0]->mNumFaces);

    // The geometry is rejected as out of range, so the triangle mesh is not imported.
    const std::string intMin = fbxTriangleWithLastIndex("-2147483648");
    scene = importer.ReadFileFromMemory(intMin.data(), intMin.size(), aiProcess_ValidateDataStructure, "fbx");
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(0u, scene->mNumMeshes);
}

// The skin_*.fbx files in FBX/skinning hold a mesh skinned to two bones, each vertex to one bone, with the skeleton at its bind pose.
// Bone global transform * offset * vertex must then give the position the file describes at bind: the geometric transform,
// then the mesh's transform at bind, which the cluster's Transform records.
static void checkSkinnedBindPose(const aiScene *scene, const std::vector<aiVector3D> &expected) {
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(1u, scene->mNumMeshes);
    const aiMesh *mesh = scene->mMeshes[0];
    ASSERT_EQ(2u, mesh->mNumBones);

    unsigned int checked = 0;
    for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
        const aiBone *bone = mesh->mBones[b];
        const aiNode *node = scene->mRootNode->FindNode(bone->mName);
        ASSERT_NE(nullptr, node);
        aiMatrix4x4 global = node->mTransformation;
        for (const aiNode *parent = node->mParent; parent != nullptr; parent = parent->mParent) {
            global = parent->mTransformation * global;
        }
        const aiMatrix4x4 skin = global * bone->mOffsetMatrix;
        for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
            const aiVector3D position = skin * mesh->mVertices[bone->mWeights[w].mVertexId];
            ai_real nearest = std::numeric_limits<ai_real>::max();
            for (const aiVector3D &e : expected) {
                nearest = std::min(nearest, (position - e).Length());
            }
            EXPECT_LT(nearest, 1e-3) << "bone " << bone->mName.C_Str() << ", vertex at " << position.x << " " << position.y << " " << position.z;
            ++checked;
        }
    }
    EXPECT_EQ(mesh->mNumVertices, checked);
}

static const std::vector<aiVector3D> SkinnedBindPose = {
    { 10.0f, 20.0f, 30.0f }, { 44.641f, 20.0f, 10.0f }, { 10.0f, 60.0f, 30.0f }, { 49.641f, 60.0f, 18.6603f },
    { 50.0f, 20.0f, 99.282f }, { 84.641f, 20.0f, 79.282f }, { 55.0f, 60.0f, 107.9423f }, { 89.641f, 60.0f, 87.9423f }
};

TEST_F(utFBXImporterExporter, importSkinOfMeshMovedAfterBinding) {
    // The mesh's node was moved after binding; the skin keeps the transform recorded at bind
    for (const bool preservePivots : { true, false }) {
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, preservePivots);
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/skinning/skin_moved_mesh.fbx", aiProcess_ValidateDataStructure);
        checkSkinnedBindPose(scene, SkinnedBindPose);
    }
}

TEST_F(utFBXImporterExporter, importSkinWithGeometricTransform) {
    // The geometric transform applies to the mesh before the skin, whether or not pivots are preserved
    const std::vector<aiVector3D> expected = {
        { 22.0622f, 17.0f, 36.8923f }, { 46.5571f, 45.2843f, 22.7502f }, { -2.4327f, 45.2843f, 51.0344f }, { 32.0622f, 73.5685f, 54.2128f },
        { 102.0622f, 17.0f, 175.4564f }, { 126.5571f, 45.2843f, 161.3142f }, { 87.5673f, 45.2843f, 206.919f }, { 112.0622f, 73.5685f, 192.7769f }
    };
    for (const bool preservePivots : { true, false }) {
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, preservePivots);
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/skinning/skin_geometric.fbx", aiProcess_ValidateDataStructure);
        checkSkinnedBindPose(scene, expected);
    }
}

// The files in FBX/skinning/ufbx were exported by Maya and Blender, each with the tool's own .obj export of the skinned mesh
// (FBX/skinning/ufbx/LICENSE.md). Assimp's skinning, bone global transform * offset * vertex, must give the tool's result.
namespace {

std::vector<aiVector3D> readObjPositions(const std::string &path) {
    std::vector<aiVector3D> positions;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.compare(0, 2, "v ") == 0) {
            std::istringstream values(line.substr(2));
            aiVector3D position;
            values >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
    }
    return positions;
}

aiVector3D sampleKeys(const aiVectorKey *keys, unsigned int count, double ticks, const aiVector3D &none) {
    if (count == 0) {
        return none;
    }
    for (unsigned int i = 1; i < count; ++i) {
        if (ticks <= keys[i].mTime) {
            const ai_real t = static_cast<ai_real>(std::max(0.0, (ticks - keys[i - 1].mTime) / (keys[i].mTime - keys[i - 1].mTime)));
            return keys[i - 1].mValue + (keys[i].mValue - keys[i - 1].mValue) * t;
        }
    }
    return keys[count - 1].mValue;
}

aiQuaternion sampleKeys(const aiQuatKey *keys, unsigned int count, double ticks, const aiQuaternion &none) {
    if (count == 0) {
        return none;
    }
    for (unsigned int i = 1; i < count; ++i) {
        if (ticks <= keys[i].mTime) {
            const ai_real t = static_cast<ai_real>(std::max(0.0, (ticks - keys[i - 1].mTime) / (keys[i].mTime - keys[i - 1].mTime)));
            aiQuaternion rotation;
            aiQuaternion::Interpolate(rotation, keys[i - 1].mValue, keys[i].mValue, t);
            return rotation.Normalize();
        }
    }
    return keys[count - 1].mValue;
}

// Global transforms of all nodes at the given time, with the keys sampled linearly
void collectGlobals(const aiNode *node, const aiMatrix4x4 &parent, const aiAnimation *animation, double ticks, std::map<const aiNode *, aiMatrix4x4> &globals) {
    aiMatrix4x4 local = node->mTransformation;
    for (unsigned int c = 0; animation != nullptr && c < animation->mNumChannels; ++c) {
        const aiNodeAnim *channel = animation->mChannels[c];
        if (channel->mNodeName == node->mName) {
            aiVector3D scaling, position;
            aiQuaternion rotation;
            node->mTransformation.Decompose(scaling, rotation, position);
            local = aiMatrix4x4(sampleKeys(channel->mScalingKeys, channel->mNumScalingKeys, ticks, scaling),
                    sampleKeys(channel->mRotationKeys, channel->mNumRotationKeys, ticks, rotation),
                    sampleKeys(channel->mPositionKeys, channel->mNumPositionKeys, ticks, position));
        }
    }
    const aiMatrix4x4 global = parent * local;
    globals[node] = global;
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        collectGlobals(node->mChildren[i], global, animation, ticks, globals);
    }
}

// Positions of all meshes at the given time. Vertices with no weight follow their node.
// The files Maya exports with its "transform skin" option also move the skinned result by the mesh node's move since its
// bind pose: meshBindPose compensates it, as ufbx's tests do.
std::vector<aiVector3D> skinnedPositions(const aiScene *scene, const aiAnimation *animation, double ticks, const aiMatrix4x4 *meshBindPose) {
    std::map<const aiNode *, aiMatrix4x4> globals;
    collectGlobals(scene->mRootNode, aiMatrix4x4(), animation, ticks, globals);

    std::vector<aiVector3D> positions;
    for (const auto &[node, global] : globals) {
        for (unsigned int m = 0; m < node->mNumMeshes; ++m) {
            const aiMesh *mesh = scene->mMeshes[node->mMeshes[m]];
            std::vector<aiVector3D> sums(mesh->mNumVertices);
            std::vector<ai_real> weights(mesh->mNumVertices);
            for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
                const aiBone *bone = mesh->mBones[b];
                const aiNode *boneNode = scene->mRootNode->FindNode(bone->mName);
                EXPECT_NE(nullptr, boneNode) << bone->mName.C_Str();
                if (boneNode == nullptr) {
                    continue;
                }
                const aiMatrix4x4 skin = globals[boneNode] * bone->mOffsetMatrix;
                for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                    const aiVertexWeight &weight = bone->mWeights[w];
                    sums[weight.mVertexId] += weight.mWeight * (skin * mesh->mVertices[weight.mVertexId]);
                    weights[weight.mVertexId] += weight.mWeight;
                }
            }
            aiMatrix4x4 move;
            if (meshBindPose != nullptr && mesh->mNumBones > 0) {
                move = global * aiMatrix4x4(*meshBindPose).Inverse();
            }
            for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                positions.push_back(weights[v] > 0 ? move * (sums[v] / weights[v]) : global * mesh->mVertices[v]);
            }
        }
    }
    return positions;
}

// The largest distance from a point of one set to the nearest point of the other, both ways, as Assimp splits vertices per face
ai_real distanceBetween(const std::vector<aiVector3D> &a, const std::vector<aiVector3D> &b) {
    ai_real largest = 0;
    for (const auto &[from, to] : { std::make_pair(&a, &b), std::make_pair(&b, &a) }) {
        for (const aiVector3D &p : *from) {
            ai_real nearest = std::numeric_limits<ai_real>::max();
            for (const aiVector3D &q : *to) {
                nearest = std::min(nearest, (p - q).Length());
            }
            largest = std::max(largest, nearest);
        }
    }
    return largest;
}

struct ToolFrame {
    const char *obj;
    double ticks; // Assimp moves each animation to start at 0, so this is the frame minus the frame the animation starts at
};

void checkAgainstTool(const char *fbx, std::initializer_list<ToolFrame> frames, const aiMatrix4x4 *meshBindPose = nullptr) {
    const std::string folder = ASSIMP_TEST_MODELS_DIR "/FBX/skinning/ufbx/";
    for (const bool preservePivots : { true, false }) {
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, preservePivots);
        // The .obj exports keep the file's own axes and units
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_IGNORE_UP_DIRECTION, true);
        const aiScene *scene = importer.ReadFile(folder + fbx, aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene) << fbx;
        const aiAnimation *animation = scene->mNumAnimations > 0 ? scene->mAnimations[0] : nullptr;
        for (const ToolFrame &frame : frames) {
            const std::vector<aiVector3D> expected = readObjPositions(folder + frame.obj);
            ASSERT_FALSE(expected.empty()) << frame.obj;
            EXPECT_LT(distanceBetween(skinnedPositions(scene, animation, frame.ticks, meshBindPose), expected), 1e-3)
                    << fbx << " against " << frame.obj << ", pivots preserved: " << preservePivots;
        }
    }
}

// The skinned mesh's bind pose in the two Maya files below, from their Pose object
aiMatrix4x4 mayaSkinBindPose() {
    aiMatrix4x4 pose;
    return aiMatrix4x4::Translation(aiVector3D(4, 0, 0), pose);
}

} // namespace

TEST_F(utFBXImporterExporter, importSkinOfMayaNodeMovedAfterBinding) {
    const aiMatrix4x4 bindPose = mayaSkinBindPose();
    checkAgainstTool("maya_transformed_skin_7700_ascii.fbx",
            { { "maya_transformed_skin_0.obj", 0 }, { "maya_transformed_skin_4.obj", 4 }, { "maya_transformed_skin_8.obj", 8 } },
            &bindPose);
}

TEST_F(utFBXImporterExporter, importSkinOfMayaInstancedMesh) {
    // Two nodes share the skinned mesh; only the first one has a bind pose, which ufbx's tests use for both
    const aiMatrix4x4 bindPose = mayaSkinBindPose();
    checkAgainstTool("maya_instanced_skin_7700_ascii.fbx",
            { { "maya_instanced_skin_0.obj", 0 }, { "maya_instanced_skin_4.obj", 4 }, { "maya_instanced_skin_8.obj", 8 } },
            &bindPose);
}

TEST_F(utFBXImporterExporter, importSkinMatchesMayaAndBlender) {
    checkAgainstTool("maya_skinned_pivot_7700_ascii.fbx", { { "maya_skinned_pivot.obj", 0 } });
    checkAgainstTool("blender_293_half_skinned_7400_binary.fbx", { { "blender_293_half_skinned.obj", 0 } });
    // This animation starts at frame 1
    checkAgainstTool("maya_game_sausage_7500_ascii_wiggle.fbx",
            { { "maya_game_sausage_wiggle_10.obj", 9 }, { "maya_game_sausage_wiggle_18.obj", 17 } });
}
