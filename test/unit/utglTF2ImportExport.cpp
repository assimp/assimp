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
#include "Tools/TestTools.h"
#ifndef ASSIMP_BUILD_NO_EXPORT
#   include "AssetLib/glTF2/glTF2AssetWriter.h"
#endif // ASSIMP_BUILD_NO_EXPORT
#include <assimp/commonMetaData.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>

#include <rapidjson/schema.h>

#include <assimp/DefaultIOSystem.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <assimp/material.h>
#include <assimp/GltfMaterial.h>

using namespace Assimp;

class utglTF2ImportExport : public AbstractImportExportBase {
public:
    virtual bool importerMatTest(const char *file, bool spec, bool gloss, std::array<aiTextureMapMode, 2> exp_modes = { aiTextureMapMode_Wrap, aiTextureMapMode_Wrap }) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(file, aiProcess_ValidateDataStructure);
        EXPECT_NE(scene, nullptr);
        if (!scene) {
            return false;
        }

        EXPECT_TRUE(scene->HasMaterials());
        if (!scene->HasMaterials()) {
            return false;
        }
        const aiMaterial *material = scene->mMaterials[0];

        // This Material should be a PBR
        aiShadingMode shadingMode;
        EXPECT_EQ(aiReturn_SUCCESS, material->Get(AI_MATKEY_SHADING_MODEL, shadingMode));
        EXPECT_EQ(aiShadingMode_PBR_BRDF, shadingMode);

        // Should import the texture as diffuse and as base color
        aiString path;
        std::array<aiTextureMapMode,2> modes;
        EXPECT_EQ(aiReturn_SUCCESS, material->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr,
                                            nullptr, nullptr, modes.data()));
        EXPECT_STREQ(path.C_Str(), "CesiumLogoFlat.png");
        EXPECT_EQ(exp_modes, modes);

        // Also as Base Color
        EXPECT_EQ(aiReturn_SUCCESS, material->GetTexture(aiTextureType_BASE_COLOR, 0, &path, nullptr, nullptr,
                                            nullptr, nullptr, modes.data()));
        EXPECT_STREQ(path.C_Str(), "CesiumLogoFlat.png");
        EXPECT_EQ(exp_modes, modes);

        // Should have a MetallicFactor (default is 1.0)
        ai_real metal_factor = ai_real(0.5);
        EXPECT_EQ(aiReturn_SUCCESS, material->Get(AI_MATKEY_METALLIC_FACTOR, metal_factor));
        EXPECT_EQ(ai_real(0.0), metal_factor);

        // And a roughness factor (default is 1.0)
        ai_real roughness_factor = ai_real(0.5);
        EXPECT_EQ(aiReturn_SUCCESS, material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness_factor));
        EXPECT_EQ(ai_real(1.0), roughness_factor);

        aiColor3D spec_color = { 0, 0, 0 };
        ai_real glossiness = ai_real(0.5);
        if (spec) {
            EXPECT_EQ(aiReturn_SUCCESS, material->Get(AI_MATKEY_COLOR_SPECULAR, spec_color));
            constexpr ai_real spec_val(0.20000000298023225); // From the file
            EXPECT_EQ(spec_val, spec_color.r);
            EXPECT_EQ(spec_val, spec_color.g);
            EXPECT_EQ(spec_val, spec_color.b);
        } else {
            EXPECT_EQ(aiReturn_FAILURE, material->Get(AI_MATKEY_COLOR_SPECULAR, spec_color));
        }
        if (gloss) {
            EXPECT_EQ(aiReturn_SUCCESS, material->Get(AI_MATKEY_GLOSSINESS_FACTOR, glossiness));
            EXPECT_EQ(ai_real(1.0), glossiness);
        } else {
            EXPECT_EQ(aiReturn_FAILURE, material->Get(AI_MATKEY_GLOSSINESS_FACTOR, glossiness));
        }

        return true;
    }

    virtual bool binaryImporterTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/2CylinderEngine-glTF-Binary/2CylinderEngine.glb",
                aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }

#ifndef ASSIMP_BUILD_NO_EXPORT
    virtual bool exporterTest() {
        Assimp::Importer importer;
        Assimp::Exporter exporter;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf",
                aiProcess_ValidateDataStructure);
        EXPECT_NE(nullptr, scene);
        EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "gltf2", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured_out.gltf"));

        return true;
    }
#endif // ASSIMP_BUILD_NO_EXPORT
};

TEST_F(utglTF2ImportExport, importglTF2FromFileTest) {
    EXPECT_TRUE(importerMatTest(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf", false, false, {aiTextureMapMode_Mirror, aiTextureMapMode_Clamp}));
}

TEST_F(utglTF2ImportExport, importBinaryglTF2FromFileTest) {
    EXPECT_TRUE(binaryImporterTest());
}

TEST_F(utglTF2ImportExport, importglTF2_KHR_materials_pbrSpecularGlossiness) {
    EXPECT_TRUE(importerMatTest(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF-pbrSpecularGlossiness/BoxTextured.gltf", true, true));
}

void VerifyClearCoatScene(const aiScene *scene) {
        ASSERT_NE(nullptr, scene);

    ASSERT_TRUE(scene->HasMaterials());

    // Find a specific Clearcoat material and check the values
    const aiString partial_coated("Partial_Coated");
    bool found_partial_coat = false;
    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial *material = scene->mMaterials[i];
        ASSERT_NE(nullptr, material);
        if (material->GetName() == partial_coated) {
            found_partial_coat = true;

            ai_real clearcoat_factor(0.0f);
            EXPECT_EQ(aiReturn_SUCCESS, material->Get(AI_MATKEY_CLEARCOAT_FACTOR, clearcoat_factor));
            EXPECT_EQ(ai_real(1.0f), clearcoat_factor);

            ai_real clearcoat_rough_factor(0.0f);
            EXPECT_EQ(aiReturn_SUCCESS, material->Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, clearcoat_rough_factor));
            EXPECT_EQ(ai_real(0.03f), clearcoat_rough_factor);

            // Should import the texture as diffuse and as base color
            aiString path;
            std::array<aiTextureMapMode, 2> modes;
            static const std::array<aiTextureMapMode, 2> exp_modes = { aiTextureMapMode_Wrap, aiTextureMapMode_Wrap };
            EXPECT_EQ(aiReturn_SUCCESS, material->GetTexture(AI_MATKEY_CLEARCOAT_TEXTURE, &path, nullptr, nullptr,
                                                nullptr, nullptr, modes.data()));
            EXPECT_STREQ(path.C_Str(), "PartialCoating.png");
            EXPECT_EQ(exp_modes, modes);
        }
    }
    EXPECT_TRUE(found_partial_coat);
}

TEST_F(utglTF2ImportExport, importglTF2_KHR_materials_clearcoat) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/ClearCoat-glTF/ClearCoatTest.gltf", aiProcess_ValidateDataStructure);
    VerifyClearCoatScene(scene);
}

#ifndef ASSIMP_BUILD_NO_EXPORT

TEST_F(utglTF2ImportExport, exportExternalBasisUniversalTexture) {
    glTF2::Asset asset;
    asset.extensionsUsed.KHR_texture_basisu = true;

    auto image = asset.images.Create("image");
    image->uri = "textures/albedo.KTX2";
    image->mimeType = "image/ktx2";

    auto texture = asset.textures.Create("texture");
    texture->source = image;

    glTF2::AssetWriter writer(asset);
    ASSERT_TRUE(writer.mDoc.HasMember("images"));
    ASSERT_EQ(writer.mDoc["images"].Size(), 1u);
    const auto &image_json = writer.mDoc["images"][0];
    ASSERT_TRUE(image_json.HasMember("uri"));
    EXPECT_STREQ(image_json["uri"].GetString(), "textures/albedo.KTX2");
    ASSERT_TRUE(image_json.HasMember("mimeType"));
    EXPECT_STREQ(image_json["mimeType"].GetString(), "image/ktx2");

    ASSERT_TRUE(writer.mDoc.HasMember("textures"));
    ASSERT_EQ(writer.mDoc["textures"].Size(), 1u);
    const auto &texture_json = writer.mDoc["textures"][0];
    EXPECT_FALSE(texture_json.HasMember("source"));
    ASSERT_TRUE(texture_json.HasMember("extensions"));
    ASSERT_TRUE(texture_json["extensions"].HasMember("KHR_texture_basisu"));
    const auto &basisu = texture_json["extensions"]["KHR_texture_basisu"];
    ASSERT_TRUE(basisu.HasMember("source"));
    EXPECT_EQ(basisu["source"].GetUint(), 0u);
}

TEST_F(utglTF2ImportExport, exportExternalBasisUniversalTexturePath) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
            ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf",
            aiProcess_ValidateDataStructure);
    ASSERT_NE(scene, nullptr);
    ASSERT_GT(scene->mNumMaterials, 0u);

    scene->mMaterials[0]->Clear();
    aiString texture_path("textures/albedo.KTX2");
    scene->mMaterials[0]->AddProperty(
            &texture_path, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    const std::string output =
            std::string(ASSIMP_TEST_MODELS_DIR) +
            "/glTF2/external_basis_texture_path_test.gltf";
    Assimp::Exporter exporter;
    ASSERT_EQ(aiReturn_SUCCESS,
              exporter.Export(scene, "gltf2", output.c_str()))
            << exporter.GetErrorString();

    std::ifstream input(output);
    ASSERT_TRUE(input.good());
    std::stringstream contents;
    contents << input.rdbuf();
    rapidjson::Document document;
    document.Parse(contents.str().c_str());
    ASSERT_FALSE(document.HasParseError());

    ASSERT_TRUE(document.HasMember("images"));
    ASSERT_EQ(document["images"].Size(), 1u);
    EXPECT_STREQ(document["images"][0]["mimeType"].GetString(),
                 "image/ktx2");
    ASSERT_TRUE(document.HasMember("textures"));
    ASSERT_EQ(document["textures"].Size(), 1u);
    EXPECT_FALSE(document["textures"][0].HasMember("source"));
    EXPECT_EQ(document["textures"][0]["extensions"]["KHR_texture_basisu"]
                      ["source"]
                              .GetUint(),
              0u);

    ::remove(output.c_str());
    const std::string binary = output.substr(0, output.size() - 5) + ".bin";
    ::remove(binary.c_str());
}

TEST_F(utglTF2ImportExport, importglTF2AndExport_KHR_materials_clearcoat) {
    {
        Assimp::Importer importer;
        Assimp::Exporter exporter;
        const aiScene* scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/ClearCoat-glTF/ClearCoatTest.gltf", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);
        // Export
        EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "glb2", ASSIMP_TEST_MODELS_DIR "/glTF2/ClearCoat-glTF/ClearCoatTest_out.glb"));
    }

    // And re-import
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/ClearCoat-glTF/ClearCoatTest_out.glb", aiProcess_ValidateDataStructure);
    VerifyClearCoatScene(scene);
}

TEST_F(utglTF2ImportExport, importglTF2AndExport_KHR_materials_pbrSpecularGlossiness) {
    Assimp::Importer importer;
    Assimp::Exporter exporter;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF-pbrSpecularGlossiness/BoxTextured.gltf",
            aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    
    // Export with specular glossiness disabled
    EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "glb2", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF-pbrSpecularGlossiness/BoxTextured_out.glb"));
    
    // And re-import
    EXPECT_TRUE(importerMatTest(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF-pbrSpecularGlossiness/BoxTextured_out.glb", true, false));

    // Export with specular glossiness enabled
    ExportProperties props;
    props.SetPropertyBool(AI_CONFIG_USE_GLTF_PBR_SPECULAR_GLOSSINESS, true);
    EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "glb2", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF-pbrSpecularGlossiness/BoxTextured_out.glb", 0, &props));

    // And re-import
    EXPECT_TRUE(importerMatTest(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF-pbrSpecularGlossiness/BoxTextured_out.glb", true, true));
}

TEST_F(utglTF2ImportExport, importglTF2AndExportToOBJ) {
    Assimp::Importer importer;
    Assimp::Exporter exporter;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf",
            aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "obj", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured_out.obj"));
}

TEST_F(utglTF2ImportExport, importglTF2EmbeddedAndExportToOBJ) {
    Assimp::Importer importer;
    Assimp::Exporter exporter;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF-Embedded/BoxTextured.gltf",
            aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "obj", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF-Embedded/BoxTextured_out.obj"));
}

#endif // ASSIMP_BUILD_NO_EXPORT

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModePointsWithoutIndices) {
    Assimp::Importer importer;
    //Points without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_00.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 1024u);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 1u);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], i);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLinesWithoutIndices) {
    Assimp::Importer importer;
    //Lines without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_01.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 8u);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 2u);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], i * 2u);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], i * 2u + 1u);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLinesLoopWithoutIndices) {
    Assimp::Importer importer;
    //Lines loop without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_02.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4u);

    std::array<unsigned int, 5> l1 = { { 0u, 1u, 2u, 3u, 0u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 2u);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 2u);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], l1[i]);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], l1[i + 1u]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLinesStripWithoutIndices) {
    Assimp::Importer importer;
    //Lines strip without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_03.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 5u);

    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 2u);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 2u);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], i);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], i + 1u);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeTrianglesStripWithoutIndices) {
    Assimp::Importer importer;
    //Triangles strip without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_04.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    std::array<unsigned int, 3> f1 = { { 0u, 1u, 2u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3u);
    for (unsigned int i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<unsigned int, 3> f2 = { { 2u, 1u, 3u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeTrianglesFanWithoutIndices) {
    Assimp::Importer importer;
    //Triangles fan without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_05.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    std::array<unsigned int, 3> f1 = { { 0u, 1u, 2u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<unsigned int, 3> f2 = { { 0u, 2u, 3u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeTrianglesWithoutIndices) {
    Assimp::Importer importer;
    //Triangles without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_06.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 6u);
    std::array<unsigned int, 3> f1 = { { 0u, 1u, 2u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<unsigned int, 3> f2 = { { 3u, 4u, 5u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModePoints) {
    Assimp::Importer importer;
    //Line loop
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_07.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 1024u);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 1u);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], i);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLines) {
    Assimp::Importer importer;
    //Lines
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_08.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    std::array<unsigned int, 5> l1 = { { 0u, 1u, 2u, 3u, 0u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 2u);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], l1[i]);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], l1[i + 1]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLineLoop) {
    Assimp::Importer importer;
    //Line loop
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_09.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    std::array<unsigned int, 5> l1 = { { 0, 1u, 2u, 3u, 0u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 2u);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], l1[i]);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], l1[i + 1]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLineStrip) {
    Assimp::Importer importer;
    //Lines Strip
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_10.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    std::array<unsigned int, 5> l1 = { { 0u, 1u, 2u, 3u, 0u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 2u);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], l1[i]);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], l1[i + 1]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeTrianglesStrip) {
    Assimp::Importer importer;
    //Triangles strip
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_11.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    std::array<unsigned int, 3> f1 = { { 0u, 1u, 2u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<unsigned int, 3> f2 = { { 2u, 1u, 3u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeTrianglesFan) {
    Assimp::Importer importer;
    //Triangles fan
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_12.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2u);
    std::array<unsigned int, 3> f1 = { { 0u, 1u, 2u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<unsigned int, 3> f2 = { { 0u, 2u, 3u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

std::vector<char> ReadFile(const char *name) {
    std::vector<char> ret;

    FILE *p{ nullptr };
    EXPECT_TRUE(Unittest::TestTools::openFilestream(&p, name, "r"));
    if (nullptr == p) {
        return ret;
    }

    ::fseek(p, 0, SEEK_END);
    const size_t size = ::ftell(p);
    ::fseek(p, 0, SEEK_SET);

    ret.resize(size);
    const size_t readSize = ::fread(&ret[0], 1, size, p);
    EXPECT_EQ(readSize, size);
    ::fclose(p);

    return ret;
}

TEST_F(utglTF2ImportExport, importglTF2FromMemory) {
    /*const auto flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_RemoveComponent |
        aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices | aiProcess_FixInfacingNormals |
        aiProcess_FindDegenerates | aiProcess_GenUVCoords | aiProcess_SortByPType;
    const auto& buff = ReadFile("C:\\Users\\kimkulling\\Downloads\\camel\\camel\\scene.gltf");*/
    /*const aiScene* Scene = ::aiImportFileFromMemory(&buff[0], buff.size(), flags, ".gltf");
    EXPECT_EQ( nullptr, Scene );*/
}

TEST_F(utglTF2ImportExport, bug_import_simple_skin) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/simple_skin/simple_skin.gltf",
            aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
}

bool checkSkinnedScene(const aiScene *scene){
    float eps = 0.001f;
    bool result = true;
    EXPECT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mMeshes[0]->mNumBones, 10u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[0].x - -1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[0].y - -1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[0].z - 0), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[1].x - 1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[1].y - -1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[1].z - 0), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[2].x - 1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[2].y - 1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[2].z - 0), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[3].x - -1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[3].y - 1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[3].z - 0), eps);

    unsigned int numWeights[] = {4u, 4u, 4u, 4u, 2u , 1u, 1u, 2u, 1u, 1u};
    float weights[10][4] = {{0.207f, 0.291f, 0.057f, 0.303f},
                        {0.113f, 0.243f, 0.499f, 0.251f},
                        {0.005f, 0.010f, 0.041f, 0.093f},
                        {0.090f, 0.234f, 0.404f, 0.243f},
                        {0.090f, 0.222f, 0.000f, 0.000f},
                        {0.216f, 0.000f, 0.000f, 0.000f},
                        {0.058f, 0.000f, 0.000f, 0.000f},
                        {0.086f, 0.000f, 0.000f, 0.111f},
                        {0.088f, 0.000f, 0.000f, 0.000f},
                        {0.049f, 0.000f, 0.000f, 0.000f}};
    for (size_t boneIndex = 0; boneIndex < 10u; ++boneIndex) {
        EXPECT_EQ(scene->mMeshes[0]->mBones[boneIndex]->mNumWeights, numWeights[boneIndex]);
        std::map<unsigned int, float> map;
        for (size_t jointIndex = 0; jointIndex < scene->mMeshes[0]->mBones[boneIndex]->mNumWeights; ++jointIndex){
            auto key = scene->mMeshes[0]->mBones[boneIndex]->mWeights[jointIndex].mVertexId;
            auto weight = scene->mMeshes[0]->mBones[boneIndex]->mWeights[jointIndex].mWeight;
            map[key] = weight;
        }

        for (unsigned int jointIndex = 0; jointIndex < scene->mMeshes[0]->mBones[boneIndex]->mNumWeights; ++jointIndex) {
            auto weight = map[jointIndex];
            EXPECT_LT(abs(ai_real(weight) - ai_real(weights[boneIndex][jointIndex])), 0.002);
        }
    }

    return result;
}

void checkSkinnedSceneLimited(const aiScene *scene){
    float eps = 0.001f;
    EXPECT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mMeshes[0]->mNumBones, 10u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[0].x - -1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[0].y - -1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[0].z - 0), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[1].x - 1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[1].y - -1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[1].z - 0), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[2].x - 1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[2].y - 1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[2].z - 0), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[3].x - -1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[3].y - 1), eps);
    EXPECT_LT(abs(scene->mMeshes[0]->mVertices[3].z - 0), eps);

    unsigned int numWeights[] = {4u, 4u, 1u, 4u, 1u , 1u, 1u, 1u, 1u, 1u};
    float weights[10][4] = {{0.207f, 0.291f, 0.057f, 0.303f},
                            {0.113f, 0.243f, 0.499f, 0.251f},
                            {0.000f, 0.000f, 0.041f, 0.000f},
                            {0.090f, 0.234f, 0.404f, 0.243f},
                            {0.000f, 0.222f, 0.000f, 0.000f},
                            {0.216f, 0.000f, 0.000f, 0.000f},
                            {0.000f, 0.000f, 0.000f, 0.000f},
                            {0.000f, 0.000f, 0.000f, 0.111f},
                            {0.000f, 0.000f, 0.000f, 0.000f},
                            {0.000f, 0.000f, 0.000f, 0.000f}};
    for (unsigned int boneIndex = 0; boneIndex < 10u; ++boneIndex) {
        EXPECT_EQ(scene->mMeshes[0]->mBones[boneIndex]->mNumWeights, numWeights[boneIndex]);
        std::map<unsigned int, float> map;
        for (unsigned int jointIndex = 0; jointIndex < scene->mMeshes[0]->mBones[boneIndex]->mNumWeights; ++jointIndex){
            auto key = scene->mMeshes[0]->mBones[boneIndex]->mWeights[jointIndex].mVertexId;
            auto weight = scene->mMeshes[0]->mBones[boneIndex]->mWeights[jointIndex].mWeight;
            map[key] = weight;
        }
        for (unsigned int jointIndex = 0; jointIndex < scene->mMeshes[0]->mBones[boneIndex]->mNumWeights; ++jointIndex) {
            auto weight = map[jointIndex];
            EXPECT_LT(std::abs(ai_real(weight) - ai_real(weights[boneIndex][jointIndex])), 0.002);
        }
    }
}

TEST_F(utglTF2ImportExport, bug_import_simple_skin2) {
    Assimp::Importer importer;
    Assimp::Exporter exporter;
    const aiScene *scene = importer.ReadFile(
            ASSIMP_TEST_MODELS_DIR "/glTF2/simple_skin/quad_skin.glb",
            aiProcess_ValidateDataStructure);
    checkSkinnedScene(scene);

    ASSERT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "glb2",
            ASSIMP_TEST_MODELS_DIR "/glTF2/simple_skin/quad_four_out.glb"));
    ASSERT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "gltf2",
            ASSIMP_TEST_MODELS_DIR "/glTF2/simple_skin/quad_four_out.gltf"));

    // enable more than four bones per vertex
    Assimp::ExportProperties properties = Assimp::ExportProperties();
    properties.SetPropertyBool(
      AI_CONFIG_EXPORT_GLTF_UNLIMITED_SKINNING_BONES_PER_VERTEX, true);
    ASSERT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "glb2",
            ASSIMP_TEST_MODELS_DIR "/glTF2/simple_skin/quad_all_out.glb", 0u, &properties));
    ASSERT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "gltf2",
            ASSIMP_TEST_MODELS_DIR "/glTF2/simple_skin/quad_all_out.gltf", 0u, &properties));

    // check skinning data of both exported files for limited number bones per vertex
    const aiScene *limitedSceneImported = importer.ReadFile(
            ASSIMP_TEST_MODELS_DIR "/glTF2/simple_skin/quad_four_out.gltf",
            aiProcess_ValidateDataStructure);
    checkSkinnedSceneLimited(limitedSceneImported);
    limitedSceneImported = importer.ReadFile(
            ASSIMP_TEST_MODELS_DIR "/glTF2/simple_skin/quad_four_out.glb",
            aiProcess_ValidateDataStructure);
    checkSkinnedSceneLimited(limitedSceneImported);

    // check skinning data of both exported files for unlimited number bones per vertex
    const aiScene *sceneImported = importer.ReadFile(
            ASSIMP_TEST_MODELS_DIR "/glTF2/simple_skin/quad_all_out.gltf",
            aiProcess_ValidateDataStructure);
    checkSkinnedScene(sceneImported);
    sceneImported = importer.ReadFile(
            ASSIMP_TEST_MODELS_DIR "/glTF2/simple_skin/quad_all_out.glb",
            aiProcess_ValidateDataStructure);
    checkSkinnedScene(sceneImported);


}

TEST_F(utglTF2ImportExport, import_cameras) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/cameras/Cameras.gltf",
            aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
}

TEST_F(utglTF2ImportExport, incorrect_vertex_arrays) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/IncorrectVertexArrays/Cube.gltf",
            aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 12u);
    EXPECT_EQ(scene->mMeshes[1]->mNumVertices, 35u);
    EXPECT_EQ(scene->mMeshes[1]->mNumFaces, 11u);
    EXPECT_EQ(scene->mMeshes[2]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[2]->mNumFaces, 18u);
    EXPECT_EQ(scene->mMeshes[3]->mNumVertices, 35u);
    EXPECT_EQ(scene->mMeshes[3]->mNumFaces, 17u);
    EXPECT_EQ(scene->mMeshes[4]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[4]->mNumFaces, 12u);
    EXPECT_EQ(scene->mMeshes[5]->mNumVertices, 35u);
    EXPECT_EQ(scene->mMeshes[5]->mNumFaces, 11u);
    EXPECT_EQ(scene->mMeshes[6]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[6]->mNumFaces, 18u);
    EXPECT_EQ(scene->mMeshes[7]->mNumVertices, 35u);
    EXPECT_EQ(scene->mMeshes[7]->mNumFaces, 17u);
}


#ifndef ASSIMP_BUILD_NO_EXPORT
TEST_F(utglTF2ImportExport, exportglTF2FromFileTest) {
    EXPECT_TRUE(exporterTest());
}

TEST_F(utglTF2ImportExport, crash_in_anim_mesh_destructor) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Sample-Models/AnimatedMorphCube-glTF/AnimatedMorphCube.gltf",
            aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    Assimp::Exporter exporter;
    ASSERT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "glb2", ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Sample-Models/AnimatedMorphCube-glTF/AnimatedMorphCube_out.glTF"));
}

TEST_F(utglTF2ImportExport, error_string_preserved) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/MissingBin/BoxTextured.gltf",
            aiProcess_ValidateDataStructure);
    ASSERT_EQ(nullptr, scene);
    std::string error = importer.GetErrorString();
    ASSERT_NE(error.find("BoxTextured0.bin"), std::string::npos) << "Error string should contain an error about missing .bin file";
}

TEST_F(utglTF2ImportExport, export_bad_accessor_bounds) {
    Assimp::Importer importer;
    Assimp::Exporter exporter;
    const aiScene* scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxWithInfinites-glTF-Binary/BoxWithInfinites.glb", aiProcess_ValidateDataStructure);
    ASSERT_NE(scene, nullptr);

    EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "glb2", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxWithInfinites-glTF-Binary/BoxWithInfinites_out.glb"));
    EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "gltf2", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxWithInfinites-glTF-Binary/BoxWithInfinites_out.gltf"));
}

TEST_F(utglTF2ImportExport, export_normalized_normals) {
    Assimp::Importer importer;
    Assimp::Exporter exporter;
    const aiScene* scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxBadNormals-glTF-Binary/BoxBadNormals.glb", aiProcess_ValidateDataStructure);
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "glb2", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxBadNormals-glTF-Binary/BoxBadNormals_out.glb"));

    // load in again and ensure normal-length normals but no Nan's or Inf's introduced
    scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxBadNormals-glTF-Binary/BoxBadNormals_out.glb", aiProcess_ValidateDataStructure);
    for ( auto i = 0u; i < scene->mMeshes[0]->mNumVertices; ++i ) {
        const auto length = scene->mMeshes[0]->mNormals[i].Length();
        EXPECT_TRUE(abs(length) < 1e-6 || abs(length - 1) < ai_epsilon);
    }
}

#endif // ASSIMP_BUILD_NO_EXPORT

TEST_F(utglTF2ImportExport, sceneMetadata) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf",
            aiProcess_ValidateDataStructure);
    ASSERT_NE(scene, nullptr);
    ASSERT_NE(scene->mMetaData, nullptr);
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_FORMAT));
        aiString format;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT, format));
        ASSERT_EQ(strcmp(format.C_Str(), "glTF2 Importer"), 0);
    }
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_FORMAT_VERSION));
        aiString version;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT_VERSION, version));
        ASSERT_EQ(strcmp(version.C_Str(), "2.0"), 0);
    }
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_GENERATOR));
        aiString generator;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_GENERATOR, generator));
        ASSERT_EQ(strcmp(generator.C_Str(), "COLLADA2GLTF"), 0);
    }
}

TEST_F(utglTF2ImportExport, texcoords) {

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTexcoords-glTF/boxTexcoords.gltf", aiProcess_ValidateDataStructure);
    ASSERT_NE(scene, nullptr);
    ASSERT_TRUE(scene->HasMaterials());
    const aiMaterial *material = scene->mMaterials[0];

    aiString path;
    unsigned int uvIndex = 255;
    aiTextureMapMode modes[2];
    EXPECT_EQ(aiReturn_SUCCESS, material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &path, nullptr, &uvIndex, nullptr, nullptr, modes));
    EXPECT_STREQ(path.C_Str(), "texture.png");
    EXPECT_EQ(uvIndex, 0u);

    uvIndex = 255;
    EXPECT_EQ(aiReturn_SUCCESS, material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &path, nullptr, &uvIndex, nullptr, nullptr, modes));
    EXPECT_STREQ(path.C_Str(), "texture.png");
    EXPECT_EQ(uvIndex, 1u);
}

#ifndef ASSIMP_BUILD_NO_EXPORT

TEST_F(utglTF2ImportExport, texcoords_export) {
    {
        Assimp::Importer importer;
        Assimp::Exporter exporter;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTexcoords-glTF/boxTexcoords.gltf", aiProcess_ValidateDataStructure);
        ASSERT_NE(scene, nullptr);
        ASSERT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "glb2", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTexcoords-glTF/boxTexcoords.gltf_out.glb"));
    }

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTexcoords-glTF/boxTexcoords.gltf", aiProcess_ValidateDataStructure);
    ASSERT_NE(scene, nullptr);

    ASSERT_TRUE(scene->HasMaterials());
    const aiMaterial *material = scene->mMaterials[0];

    aiString path;
    unsigned int uvIndex = 255;
    aiTextureMapMode modes[2];
    EXPECT_EQ(aiReturn_SUCCESS, material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &path, nullptr, &uvIndex, nullptr, nullptr, modes));
    EXPECT_STREQ(path.C_Str(), "texture.png");
    EXPECT_EQ(uvIndex, 0u);

    uvIndex = 255;
    EXPECT_EQ(aiReturn_SUCCESS, material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &path, nullptr, &uvIndex, nullptr, nullptr, modes));
    EXPECT_STREQ(path.C_Str(), "texture.png");
    EXPECT_EQ(uvIndex, 1u);
}

#endif // ASSIMP_BUILD_NO_EXPORT
TEST_F(utglTF2ImportExport, recursive_nodes) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/RecursiveNodes/RecursiveNodes.gltf", aiProcess_ValidateDataStructure);
    EXPECT_EQ(nullptr, scene);
}

TEST_F(utglTF2ImportExport, norootnode_noscene) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/TestNoRootNode/NoScene.gltf", aiProcess_ValidateDataStructure);
    ASSERT_EQ(scene, nullptr);
}

TEST_F(utglTF2ImportExport, norootnode_scenewithoutnodes) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/TestNoRootNode/SceneWithoutNodes.gltf", aiProcess_ValidateDataStructure);
    ASSERT_NE(scene, nullptr);
    ASSERT_NE(scene->mRootNode, nullptr);
}

// Shall not crash!
TEST_F(utglTF2ImportExport, norootnode_issue_3269) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/issue_3269/texcoord_crash.gltf", aiProcess_ValidateDataStructure);
    ASSERT_EQ(scene, nullptr);
}

TEST_F(utglTF2ImportExport, indexOutOfRange) {
    // The contents of an asset should not lead to an assert.
    Assimp::Importer importer;

    struct LogObserver : Assimp::LogStream {
        bool m_observedWarning = false;
        void write(const char *message) override {
            m_observedWarning = m_observedWarning || std::strstr(message, "faces were dropped");
        }
    };
    LogObserver logObserver;

    DefaultLogger::get()->attachStream(&logObserver);
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/IndexOutOfRange/IndexOutOfRange.gltf", aiProcess_ValidateDataStructure);
    ASSERT_NE(scene, nullptr);
    ASSERT_NE(scene->mRootNode, nullptr);
    ASSERT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 11u);
    DefaultLogger::get()->detachStream(&logObserver);
    EXPECT_TRUE(logObserver.m_observedWarning);
}

TEST_F(utglTF2ImportExport, allIndicesOutOfRange) {
    // The contents of an asset should not lead to an assert.
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/IndexOutOfRange/AllIndicesOutOfRange.gltf", aiProcess_ValidateDataStructure);
    ASSERT_EQ(scene, nullptr);
    std::string error = importer.GetErrorString();
    ASSERT_NE(error.find("Mesh \"Mesh\" has no faces"), std::string::npos);
}

TEST_F(utglTF2ImportExport, lineLoopWithSingleIndex) {
    // A LINE_LOOP primitive needs at least two indices. A single index must not
    // make the importer read past the end of the index buffer.
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/DegeneratePrimitives/LineLoopOneIndex.gltf", aiProcess_ValidateDataStructure);
    ASSERT_EQ(scene, nullptr);
    const std::string error = importer.GetErrorString();
    ASSERT_NE(error.find("no faces"), std::string::npos);
}

TEST_F(utglTF2ImportExport, lineStripWithSingleIndex) {
    // A LINE_STRIP primitive needs at least two indices. A single index must not
    // make the importer read past the end of the index buffer.
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/DegeneratePrimitives/LineStripOneIndex.gltf", aiProcess_ValidateDataStructure);
    ASSERT_EQ(scene, nullptr);
    const std::string error = importer.GetErrorString();
    ASSERT_NE(error.find("no faces"), std::string::npos);
}

TEST_F(utglTF2ImportExport, triangleFanWithTwoIndices) {
    // A TRIANGLE_FAN primitive needs at least three indices. Two indices must not
    // make the importer read past the end of the index buffer.
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/DegeneratePrimitives/TriangleFanTwoIndices.gltf", aiProcess_ValidateDataStructure);
    ASSERT_EQ(scene, nullptr);
    const std::string error = importer.GetErrorString();
    ASSERT_NE(error.find("no faces"), std::string::npos);
}

TEST_F(utglTF2ImportExport, triangleStripWithSingleIndex) {
    // A TRIANGLE_STRIP primitive needs at least three indices. A single index must not
    // make the face count underflow.
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/DegeneratePrimitives/TriangleStripOneIndex.gltf", aiProcess_ValidateDataStructure);
    ASSERT_EQ(scene, nullptr);
    const std::string error = importer.GetErrorString();
    ASSERT_NE(error.find("no faces"), std::string::npos);
}

TEST_F(utglTF2ImportExport, lineLoopWithOutOfRangeFirstIndex) {
    // The closing segment of a LINE_LOOP must not dereference the first face when that
    // face was dropped because its indices were out of range.
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/DegeneratePrimitives/LineLoopFirstIndexOutOfRange.gltf", aiProcess_ValidateDataStructure);
    ASSERT_EQ(scene, nullptr);
    const std::string error = importer.GetErrorString();
    ASSERT_NE(error.find("no faces"), std::string::npos);
}

/////////////////////////////////
// Draco decoding

TEST_F(utglTF2ImportExport, import_dracoEncoded) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/draco/2CylinderEngine.gltf",
            aiProcess_ValidateDataStructure);
#ifndef ASSIMP_ENABLE_DRACO
    // No draco support, scene should not load
    ASSERT_EQ(scene, nullptr);
#else
    ASSERT_NE(scene, nullptr);
    ASSERT_NE(scene->mMetaData, nullptr);
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_FORMAT));
        aiString format;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT, format));
        ASSERT_EQ(strcmp(format.C_Str(), "glTF2 Importer"), 0);
    }
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_FORMAT_VERSION));
        aiString version;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT_VERSION, version));
        ASSERT_EQ(strcmp(version.C_Str(), "2.0"), 0);
    }
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_GENERATOR));
        aiString generator;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_GENERATOR, generator));
        ASSERT_EQ(strcmp(generator.C_Str(), "COLLADA2GLTF"), 0);
    }
#endif
    const aiScene *robotScene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/draco/robot.glb",
        aiProcess_ValidateDataStructure);
#ifndef ASSIMP_ENABLE_DRACO
    // No draco support, scene should not load
    ASSERT_EQ(robotScene, nullptr);
#else
    ASSERT_NE(robotScene, nullptr);
    ASSERT_NE(robotScene->mMetaData, nullptr);
    {
        ASSERT_TRUE(robotScene->mMetaData->HasKey(AI_METADATA_SOURCE_FORMAT));
        aiString format;
        ASSERT_TRUE(robotScene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT, format));
        ASSERT_EQ(strcmp(format.C_Str(), "glTF2 Importer"), 0);
    }
    {
        ASSERT_TRUE(robotScene->mMetaData->HasKey(AI_METADATA_SOURCE_FORMAT_VERSION));
        aiString version;
        ASSERT_TRUE(robotScene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT_VERSION, version));
        ASSERT_EQ(strcmp(version.C_Str(), "2.0"), 0);
    }
#endif
}

TEST_F(utglTF2ImportExport, wrongTypes) {
    // Deliberately broken version of the BoxTextured.gltf asset.
    using tup_T = std::tuple<std::string, std::string, std::string, std::string>;
    std::vector<tup_T> wrongTypes = {
#ifdef __cpp_lib_constexpr_tuple
    #define TUPLE(x, y, z, w) {x, y, z, w}
#else
    #define TUPLE(x, y, z, w) tup_T(x, y, z, w)
#endif
        TUPLE("/glTF2/wrongTypes/badArray.gltf", "array", "primitives", "meshes[0]"),
        TUPLE("/glTF2/wrongTypes/badString.gltf", "string", "name", "scenes[0]"),
        TUPLE("/glTF2/wrongTypes/badUint.gltf", "uint", "index", "materials[0]"),
        TUPLE("/glTF2/wrongTypes/badNumber.gltf", "number", "scale", "materials[0]"),
        //TUPLE("/glTF2/wrongTypes/badObject.gltf", "object", "pbrMetallicRoughness", "materials[0]"),
        //TUPLE("/glTF2/wrongTypes/badExtension.gltf", "object", "KHR_texture_transform", "materials[0]")
#undef TUPLE
    };
    for (const auto& tuple : wrongTypes)
    {
        const auto& file = std::get<0>(tuple);
        const auto& type = std::get<1>(tuple);
        const auto& member = std::get<2>(tuple);
        const auto& context = std::get<3>(tuple);
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR + file , aiProcess_ValidateDataStructure);
        ASSERT_EQ(scene, nullptr);
        const std::string error = importer.GetErrorString();
        EXPECT_FALSE(error.empty());
        EXPECT_NE(error.find(member + "\" was not of type \"" + type + "\" when reading " + context), std::string::npos);
    }
}

TEST_F(utglTF2ImportExport, wrongObject) {
    // Deliberately broken version of the BoxTextured.gltf asset.
    using tup_T = std::tuple<std::string, std::string, std::string, std::string>;
    std::vector<tup_T> wrongTypes = {
#ifdef __cpp_lib_constexpr_tuple
#define TUPLE(x, y, z, w) \
    { x, y, z, w }
#else
#define TUPLE(x, y, z, w) tup_T(x, y, z, w)
#endif
    TUPLE("/glTF2/wrongTypes/badObject.gltf", "object", "pbrMetallicRoughness", "materials[0]"),
    TUPLE("/glTF2/wrongTypes/badExtension.gltf", "object", "KHR_texture_transform", "materials[0]"),
    TUPLE("/glTF2/wrongTypes/topLevelExtensionsNotObject.gltf", "object", "extensions", "the document")
#undef TUPLE
    };
    for (const auto &tuple : wrongTypes) {
        const auto &file = std::get<0>(tuple);
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR + file, aiProcess_ValidateDataStructure);
        ASSERT_NE(scene, nullptr);
    }
}

namespace {
    /// This class provides a fake schema to the GLTF importer.
    /// It just checks that the file has a top-level "scene" property which is an integer.
    class FakeSchemaProvider : public rapidjson::IRemoteSchemaDocumentProvider
    {
    public:
        FakeSchemaProvider(const char* schemaName) :
            m_schemaName(schemaName)
        {
            rapidjson::Document schemaDoc;
            schemaDoc.Parse(R"==({"properties":{"scene" : { "type" : "integer" }}, "required": [ "scene" ]})==");
            EXPECT_FALSE(schemaDoc.HasParseError());
        	m_schema.reset(new rapidjson::SchemaDocument(schemaDoc, m_schemaName.c_str(), static_cast<rapidjson::SizeType>(m_schemaName.size()), this));
        }

        const rapidjson::SchemaDocument* GetRemoteDocument(const char* uri, rapidjson::SizeType) override {
            if (m_schemaName == uri) {
                return m_schema.get();
            }
            return nullptr;
        }

    private:
        std::string m_schemaName;
        std::unique_ptr<const rapidjson::SchemaDocument> m_schema;
    };
}

TEST_F(utglTF2ImportExport, schemaCheckPass) {
    FakeSchemaProvider schemaProvider("glTF.schema.json");
    Assimp::Importer importer;
    importer.SetPropertyPointer(AI_CONFIG_IMPORT_SCHEMA_DOCUMENT_PROVIDER, &schemaProvider);
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(scene, nullptr);
    EXPECT_STREQ(importer.GetErrorString(), "");
}

TEST_F(utglTF2ImportExport, schemaCheckFail) {
    FakeSchemaProvider schemaProvider("glTF.schema.json");
    Assimp::Importer importer;
    importer.SetPropertyPointer(AI_CONFIG_IMPORT_SCHEMA_DOCUMENT_PROVIDER, &schemaProvider);
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/SchemaFailures/sceneWrongType.gltf", aiProcess_ValidateDataStructure);
    EXPECT_EQ(scene, nullptr);
    const std::string errorString = importer.GetErrorString();
    EXPECT_NE(errorString.find("The JSON document did not satisfy the glTF2 schema"), std::string::npos);
}

TEST_F(utglTF2ImportExport, noSchemaFound) {
    // More than one importer might make use the provider, but not all schemas might be present.
    // Check that the glTF importer handles the case when an non-null provider returns null when asked for schemas.
    FakeSchemaProvider schemaProvider("missingSchema.json");
    Assimp::Importer importer;
    importer.SetPropertyPointer(AI_CONFIG_IMPORT_SCHEMA_DOCUMENT_PROVIDER, &schemaProvider);
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(scene, nullptr);
    EXPECT_STREQ(importer.GetErrorString(), "");
}

// ------------------------------------------------------------------------------------------------
TEST_F(utglTF2ImportExport, testSetIdentityMatrixEpsilon) {
//    Assimp::Exporter exporter;
    Assimp::ExportProperties properties = Assimp::ExportProperties();
    EXPECT_EQ(AI_CONFIG_CHECK_IDENTITY_MATRIX_EPSILON_DEFAULT,
            properties.GetPropertyFloat("CHECK_IDENTITY_MATRIX_EPSILON",
                    AI_CONFIG_CHECK_IDENTITY_MATRIX_EPSILON_DEFAULT));
    aiMatrix4x4 m;
    m = aiMatrix4x4(1.02f, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    EXPECT_FALSE(m.IsIdentity());
    m = aiMatrix4x4(1.001f, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    EXPECT_TRUE(m.IsIdentity());

    bool b = properties.SetPropertyFloat("CHECK_IDENTITY_MATRIX_EPSILON", 0.0001f);
    EXPECT_TRUE(!b);
    ai_real epsilon = properties.GetPropertyFloat("CHECK_IDENTITY_MATRIX_EPSILON", 0.01f);
    EXPECT_EQ(0.0001f, epsilon);
    m = aiMatrix4x4(1.0002f, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    EXPECT_FALSE(m.IsIdentity(epsilon));
    m = aiMatrix4x4(1.00009f, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    EXPECT_TRUE(m.IsIdentity(epsilon));
}

TEST_F(utglTF2ImportExport, importMalformedSparseAccessor) {
    Assimp::Importer importer;
    // Attempt to load the proof-of-concept file we generated earlier
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/malformed_sparse.gltf", 0);
    
    // ASSERTION: The file must fail to load safely instead of crashing the program
    EXPECT_EQ(scene, nullptr);
    
    // ASSERTION: The thrown parser error must match our custom fail-fast string
    std::string errorString = importer.GetErrorString();
    EXPECT_NE(errorString.find("Invalid sparse accessor: missing required 'values' object."), std::string::npos);
}

TEST_F(utglTF2ImportExport, importAnimationInterpolationIsPreserved) {
    // The sampler's interpolation mode reaches the keys (da281b7f added the
    // field, #6543 taught glTF2 to fill it). Nothing covered it, so a
    // regression here would have been silent: every mode still imports, and
    // aiAnimInterpolation_Linear is also the default a key is constructed with.
    struct Expectation {
        const char *file;
        aiAnimInterpolation interpolation;
        unsigned int numPositionKeys;
    };
    // CUBICSPLINE stores in-tangent, value and out-tangent per keyframe, so two
    // keyframes arrive as six keys.
    const std::array<Expectation, 3> expectations = { {
        { "/glTF2/animation_interpolation_step.gltf", aiAnimInterpolation_Step, 2 },
        { "/glTF2/animation_interpolation_linear.gltf", aiAnimInterpolation_Linear, 2 },
        { "/glTF2/animation_interpolation_cubic.gltf", aiAnimInterpolation_Cubic_Spline, 6 },
    } };

    for (const Expectation &expected : expectations) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(std::string(ASSIMP_TEST_MODELS_DIR) + expected.file, 0);
        ASSERT_NE(nullptr, scene) << expected.file << ": " << importer.GetErrorString();
        ASSERT_EQ(1u, scene->mNumAnimations) << expected.file;

        const aiAnimation *animation = scene->mAnimations[0];
        ASSERT_EQ(1u, animation->mNumChannels) << expected.file;

        const aiNodeAnim *channel = animation->mChannels[0];
        EXPECT_STREQ("Mover", channel->mNodeName.C_Str()) << expected.file;
        ASSERT_EQ(expected.numPositionKeys, channel->mNumPositionKeys) << expected.file;
        for (unsigned int i = 0; i < channel->mNumPositionKeys; ++i) {
            EXPECT_EQ(expected.interpolation, channel->mPositionKeys[i].mInterpolation)
                    << expected.file << " key " << i;
        }
    }
}


namespace {

// Three float32 vertices: (0,0,0), (1,0,0), (0,1,0).
const unsigned char kTriangleBytes[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00
};

const char kDataUri36Zeros[] =
        "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

// IOSystem that records every path the importer tries to open.
class RecordingIOSystem final : public Assimp::IOSystem {
public:
    std::vector<std::string> opened;

    bool Exists(const char *pFile) const override { return mInner.Exists(pFile); }
    char getOsSeparator() const override { return mInner.getOsSeparator(); }
    IOStream *Open(const char *pFile, const char *pMode) override {
        opened.emplace_back(pFile != nullptr ? pFile : "");
        return mInner.Open(pFile, pMode);
    }
    void Close(IOStream *pFile) override { mInner.Close(pFile); }
    bool ComparePaths(const char *one, const char *second) const override {
        return mInner.ComparePaths(one, second);
    }

private:
    Assimp::DefaultIOSystem mInner;
};

// Joins a directory and a file name with the platform separator.
std::string JoinPath(const std::string &dir, const std::string &name) {
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    if (dir.empty() || dir.back() == '/' || dir.back() == '\\') {
        return dir + name;
    }
    return dir + sep + name;
}

// Creates a directory (and missing parents) accessible only by the owner.
bool EnsureDir(const std::string &path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        return false;
    }
    std::filesystem::permissions(path, std::filesystem::perms::owner_all,
            std::filesystem::perm_options::replace, ec);
    return !ec;
}

// Creates a uniquely named owner-only directory under the system temp directory.
std::string MakeTempDir(const std::string &prefix) {
    std::error_code ec;
    const std::filesystem::path base = std::filesystem::temp_directory_path(ec);
    if (ec) {
        return std::string();
    }
    std::random_device random;
    for (int attempt = 0; attempt < 16; ++attempt) {
        const std::filesystem::path dir = base / (prefix + std::to_string(random()));
        if (std::filesystem::create_directory(dir, ec)) {
            std::filesystem::permissions(dir, std::filesystem::perms::owner_all,
                    std::filesystem::perm_options::replace, ec);
            if (ec) {
                std::filesystem::remove_all(dir, ec);
                return std::string();
            }
            return dir.string();
        }
        if (ec) {
            return std::string();
        }
    }
    return std::string();
}

// Removes a temporary directory tree when the test scope ends, even after a failed assertion.
struct TempDirGuard {
    explicit TempDirGuard(std::string dir) : path(std::move(dir)) {}
    ~TempDirGuard() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
    TempDirGuard(const TempDirGuard &) = delete;
    TempDirGuard &operator=(const TempDirGuard &) = delete;

    std::string path;
};

// Writes raw bytes to a file readable and writable only by the owner.
bool WriteBytes(const std::string &path, const void *data, size_t length) {
    {
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            return false;
        }
        out.write(static_cast<const char *>(data), static_cast<std::streamsize>(length));
        if (!out) {
            return false;
        }
    }
    std::error_code ec;
    std::filesystem::permissions(path,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace, ec);
    return !ec;
}

// Writes a text file readable and writable only by the owner.
bool WriteText(const std::string &path, const std::string &text) {
    return WriteBytes(path, text.data(), text.size());
}

// Escapes backslashes and quotes for embedding in a JSON string.
std::string JsonEscape(const std::string &in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        if (c == '\\' || c == '"') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

// Builds a minimal glTF 2.0 triangle whose buffer uses the given URI.
std::string Gltf2Buffer(const std::string &bufferUri) {
    std::ostringstream json;
    json << "{\n"
            "  \"asset\": {\"version\": \"2.0\"},\n"
            "  \"scene\": 0,\n"
            "  \"scenes\": [{\"nodes\": [0]}],\n"
            "  \"nodes\": [{\"mesh\": 0}],\n"
            "  \"meshes\": [{\"primitives\": [{\"attributes\": {\"POSITION\": 0}}]}],\n"
            "  \"accessors\": [{\"bufferView\": 0, \"componentType\": 5126, \"count\": 3, \"type\": \"VEC3\"}],\n"
            "  \"bufferViews\": [{\"buffer\": 0, \"byteLength\": 36}],\n"
            "  \"buffers\": [{\"uri\": \""
         << JsonEscape(bufferUri) << "\", \"byteLength\": 36}]\n"
            "}\n";
    return json.str();
}

// Builds a minimal glTF 2.0 triangle whose base color image uses the given URI.
std::string Gltf2Image(const std::string &imageUri) {
    std::ostringstream json;
    json << "{\n"
            "  \"asset\": {\"version\": \"2.0\"},\n"
            "  \"scene\": 0,\n"
            "  \"scenes\": [{\"nodes\": [0]}],\n"
            "  \"nodes\": [{\"mesh\": 0}],\n"
            "  \"meshes\": [{\"primitives\": [{\"attributes\": {\"POSITION\": 0}, \"material\": 0}]}],\n"
            "  \"materials\": [{\"pbrMetallicRoughness\": {\"baseColorTexture\": {\"index\": 0}}}],\n"
            "  \"textures\": [{\"source\": 0}],\n"
            "  \"images\": [{\"uri\": \""
         << JsonEscape(imageUri) << "\"}],\n"
            "  \"accessors\": [{\"bufferView\": 0, \"componentType\": 5126, \"count\": 3, \"type\": \"VEC3\"}],\n"
            "  \"bufferViews\": [{\"buffer\": 0, \"byteLength\": 36}],\n"
            "  \"buffers\": [{\"uri\": \""
         << kDataUri36Zeros << "\", \"byteLength\": 36}]\n"
            "}\n";
    return json.str();
}

// Returns true if any recorded open path contains the given fragment.
bool OpenedContains(const RecordingIOSystem &io, const char *needle) {
    for (const std::string &path : io.opened) {
        if (path.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Expects the import to fail with the given error and without opening the forbidden file.
void ExpectFileRejected(const std::string &path, const char *errorFragment, const char *forbidden) {
    Assimp::Importer importer;
    auto *io = new RecordingIOSystem();
    importer.SetIOHandler(io);
    const aiScene *scene = importer.ReadFile(path, aiProcess_ValidateDataStructure);
    EXPECT_EQ(nullptr, scene) << path << ": " << importer.GetErrorString();
    const std::string error = importer.GetErrorString();
    EXPECT_NE(std::string::npos, error.find(errorFragment)) << path << ": " << error;
    EXPECT_FALSE(OpenedContains(*io, forbidden)) << path;
}

// Expects the scene to hold the reference triangle.
void ExpectVertices(const aiScene *scene) {
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(1u, scene->mNumMeshes);
    ASSERT_NE(nullptr, scene->mMeshes[0]);
    ASSERT_EQ(3u, scene->mMeshes[0]->mNumVertices);
    EXPECT_EQ(ai_real(0), scene->mMeshes[0]->mVertices[0].x);
    EXPECT_EQ(ai_real(1), scene->mMeshes[0]->mVertices[1].x);
    EXPECT_EQ(ai_real(0), scene->mMeshes[0]->mVertices[1].y);
    EXPECT_EQ(ai_real(1), scene->mMeshes[0]->mVertices[2].y);
}

} // namespace

TEST_F(utglTF2ImportExport, externalUriIsContained) {
    Assimp::Importer importer;
    const aiScene *benign = importer.ReadFile(
            ASSIMP_TEST_MODELS_DIR "/glTF2/issue_6933/triangle.gltf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, benign) << importer.GetErrorString();
    ASSERT_NO_FATAL_FAILURE(ExpectVertices(benign));

    const std::string root = MakeTempDir("assimp-issue6933-");
    ASSERT_FALSE(root.empty());
    const TempDirGuard rootGuard(root);
    const std::string pkg = JoinPath(root, "pkg");
    const std::string sub = JoinPath(pkg, "sub");
    ASSERT_TRUE(EnsureDir(pkg));
    ASSERT_TRUE(EnsureDir(sub));

    const unsigned char secret[36] = {
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
    };
    ASSERT_TRUE(WriteBytes(JoinPath(root, "secret.bin"), secret, sizeof(secret)));
    ASSERT_TRUE(WriteBytes(JoinPath(pkg, "triangle.bin"), kTriangleBytes, sizeof(kTriangleBytes)));
    ASSERT_TRUE(WriteBytes(JoinPath(sub, "triangle.bin"), kTriangleBytes, sizeof(kTriangleBytes)));

    const std::string dotSlash = JoinPath(pkg, "dotslash.gltf");
    const std::string nested = JoinPath(pkg, "nested.gltf");
    ASSERT_TRUE(WriteText(dotSlash, Gltf2Buffer("./triangle.bin")));
    ASSERT_TRUE(WriteText(nested, Gltf2Buffer("sub/triangle.bin")));

    {
        Assimp::Importer nestedImporter;
        const aiScene *scene = nestedImporter.ReadFile(dotSlash, aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene) << nestedImporter.GetErrorString();
        ASSERT_NO_FATAL_FAILURE(ExpectVertices(scene));
    }
    {
        Assimp::Importer nestedImporter;
        const aiScene *scene = nestedImporter.ReadFile(nested, aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene) << nestedImporter.GetErrorString();
        ASSERT_NO_FATAL_FAILURE(ExpectVertices(scene));
    }

    const std::string traversal = JoinPath(pkg, "traversal.gltf");
    const std::string nestedEscape = JoinPath(pkg, "nested_escape.gltf");
    const std::string backslash = JoinPath(pkg, "backslash.gltf");
    const std::string absolute = JoinPath(pkg, "absolute.gltf");
    const std::string drive = JoinPath(pkg, "drive.gltf");
    const std::string scheme = JoinPath(pkg, "scheme.gltf");
    ASSERT_TRUE(WriteText(traversal, Gltf2Buffer("../secret.bin")));
    ASSERT_TRUE(WriteText(nestedEscape, Gltf2Buffer("sub/../../secret.bin")));
    ASSERT_TRUE(WriteText(backslash, Gltf2Buffer("..\\secret.bin")));
    ASSERT_TRUE(WriteText(absolute, Gltf2Buffer("/etc/passwd")));
    ASSERT_TRUE(WriteText(drive, Gltf2Buffer("C:/Windows/win.ini")));
    ASSERT_TRUE(WriteText(scheme, Gltf2Buffer("file:///etc/passwd")));

    ExpectFileRejected(traversal, "asset directory", "secret.bin");
    ExpectFileRejected(nestedEscape, "asset directory", "secret.bin");
    ExpectFileRejected(backslash, "asset directory", "secret.bin");
    ExpectFileRejected(absolute, "asset directory", "passwd");
    ExpectFileRejected(drive, "asset directory", "win.ini");
    ExpectFileRejected(scheme, "asset directory", "passwd");

    const std::string imageOk = JoinPath(pkg, "image_ok.gltf");
    const std::string imageBad = JoinPath(pkg, "image_bad.gltf");
    ASSERT_TRUE(WriteText(imageOk, Gltf2Image("tex/ok.png")));
    ASSERT_TRUE(WriteText(imageBad, Gltf2Image("../secret.png")));

    {
        Assimp::Importer imageImporter;
        const aiScene *scene = imageImporter.ReadFile(imageOk, aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene) << imageImporter.GetErrorString();
        ASSERT_GT(scene->mNumMaterials, 0u);
        aiString texPath;
        EXPECT_EQ(aiReturn_SUCCESS, scene->mMaterials[0]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath));
        EXPECT_STREQ("tex/ok.png", texPath.C_Str());
    }
    ExpectFileRejected(imageBad, "asset directory", "secret.png");

    const std::string memoryExternal = Gltf2Buffer("triangle.bin");
    const std::string memoryAbsolute = Gltf2Buffer("/etc/passwd");
    const std::string memoryData = Gltf2Buffer(kDataUri36Zeros);
    {
        Assimp::Importer memoryImporter;
        auto *io = new RecordingIOSystem();
        memoryImporter.SetIOHandler(io);
        const aiScene *scene = memoryImporter.ReadFileFromMemory(
                memoryExternal.data(), memoryExternal.size(), aiProcess_ValidateDataStructure, "gltf");
        EXPECT_EQ(nullptr, scene) << memoryImporter.GetErrorString();
        const std::string error = memoryImporter.GetErrorString();
        EXPECT_NE(std::string::npos, error.find("loading from memory")) << error;
        EXPECT_TRUE(io->opened.empty());
    }
    {
        Assimp::Importer memoryImporter;
        auto *io = new RecordingIOSystem();
        memoryImporter.SetIOHandler(io);
        const aiScene *scene = memoryImporter.ReadFileFromMemory(
                memoryAbsolute.data(), memoryAbsolute.size(), aiProcess_ValidateDataStructure, "gltf");
        EXPECT_EQ(nullptr, scene) << memoryImporter.GetErrorString();
        const std::string error = memoryImporter.GetErrorString();
        EXPECT_NE(std::string::npos, error.find("loading from memory")) << error;
        EXPECT_TRUE(io->opened.empty());
    }
    {
        Assimp::Importer memoryImporter;
        const aiScene *scene = memoryImporter.ReadFileFromMemory(
                memoryData.data(), memoryData.size(), aiProcess_ValidateDataStructure, "gltf");
        ASSERT_NE(nullptr, scene) << memoryImporter.GetErrorString();
        ASSERT_EQ(1u, scene->mNumMeshes);
        EXPECT_EQ(3u, scene->mMeshes[0]->mNumVertices);
    }
}
