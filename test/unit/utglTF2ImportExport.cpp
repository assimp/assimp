/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

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
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>


#include <array>

#include <assimp/pbrmaterial.h>
using namespace Assimp;

class utglTF2ImportExport : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf",
                aiProcess_ValidateDataStructure);
        EXPECT_NE(scene, nullptr);
        if (!scene) {
            return false;
        }

        EXPECT_TRUE(scene->HasMaterials());
        if (!scene->HasMaterials()) {
            return false;
        }
        const aiMaterial *material = scene->mMaterials[0];

        aiString path;
        aiTextureMapMode modes[2];
        EXPECT_EQ(aiReturn_SUCCESS, material->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr,
                                            nullptr, nullptr, modes));
        EXPECT_STREQ(path.C_Str(), "CesiumLogoFlat.png");
        EXPECT_EQ(modes[0], aiTextureMapMode_Mirror);
        EXPECT_EQ(modes[1], aiTextureMapMode_Clamp);

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
    EXPECT_TRUE(importerTest());
}

TEST_F(utglTF2ImportExport, importBinaryglTF2FromFileTest) {
    EXPECT_TRUE(binaryImporterTest());
}

#ifndef ASSIMP_BUILD_NO_EXPORT
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
    std::array<unsigned int, 5> l1 = { { 0u, 3u, 2u, 1u, 0u } };
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
    std::array<unsigned int, 5> l1 = { { 0, 3u, 2u, 1u, 0u } };
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
    std::array<unsigned int, 5> l1 = { { 0u, 3u, 2u, 1u, 0u } };
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
    std::array<unsigned int, 3> f1 = { { 0u, 3u, 1u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<unsigned int, 3> f2 = { { 1u, 3u, 2u } };
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
    std::array<unsigned int, 3> f1 = { { 0u, 3u, 2u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<unsigned int, 3> f2 = { { 0u, 2u, 1u } };
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

std::vector<char> ReadFile(const char *name) {
    std::vector<char> ret;

    FILE *p = ::fopen(name, "r");
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

TEST_F(utglTF2ImportExport, texture_transform_test) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/textureTransform/TextureTransformTest.gltf",
            aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
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
        EXPECT_TRUE(abs(length) < 1e-6 || abs(length - 1) < 1e-6);
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
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTexcoords-glTF/boxTexcoords.gltf",
            aiProcess_ValidateDataStructure);
    ASSERT_NE(scene, nullptr);

    ASSERT_TRUE(scene->HasMaterials());
    const aiMaterial *material = scene->mMaterials[0];

    aiString path;
    aiTextureMapMode modes[2];
    EXPECT_EQ(aiReturn_SUCCESS, material->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr,
                                        nullptr, nullptr, modes));
    EXPECT_STREQ(path.C_Str(), "texture.png");

    int uvIndex = -1;
    EXPECT_EQ(aiGetMaterialInteger(material, AI_MATKEY_GLTF_TEXTURE_TEXCOORD(aiTextureType_DIFFUSE, 0), &uvIndex), aiReturn_SUCCESS);
    EXPECT_EQ(uvIndex, 0);

    // Using manual macro expansion of AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE here.
    // The following works with some but not all compilers:
    // #define APPLY(X, Y) X(Y)
    // ..., APPLY(AI_MATKEY_GLTF_TEXTURE_TEXCOORD, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE), ...
    EXPECT_EQ(aiGetMaterialInteger(material, AI_MATKEY_GLTF_TEXTURE_TEXCOORD(aiTextureType_UNKNOWN, 0), &uvIndex), aiReturn_SUCCESS);
    EXPECT_EQ(uvIndex, 1);
}

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
