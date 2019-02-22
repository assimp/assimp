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
#include "AbstractImportExportBase.h"

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <array>

using namespace Assimp;

class utglTF2ImportExport : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf", aiProcess_ValidateDataStructure);
        EXPECT_NE( scene, nullptr );
        if ( !scene ) return false;

        EXPECT_TRUE( scene->HasMaterials() );
        if ( !scene->HasMaterials() ) return false;
        const aiMaterial *material = scene->mMaterials[0];

        aiString path;
        aiTextureMapMode modes[2];
        EXPECT_EQ( aiReturn_SUCCESS, material->GetTexture(aiTextureType_DIFFUSE, 0, &path, nullptr, nullptr, nullptr, nullptr, modes) );
        EXPECT_STREQ( path.C_Str(), "CesiumLogoFlat.png" );
        EXPECT_EQ( modes[0], aiTextureMapMode_Mirror );
        EXPECT_EQ( modes[1], aiTextureMapMode_Clamp );

        return true;
    }

    virtual bool binaryImporterTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/glTF2/2CylinderEngine-glTF-Binary/2CylinderEngine.glb", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }

#ifndef ASSIMP_BUILD_NO_EXPORT
    virtual bool exporterTest() {
        Assimp::Importer importer;
        Assimp::Exporter exporter;
        const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf", aiProcess_ValidateDataStructure );
        EXPECT_NE( nullptr, scene );
        EXPECT_EQ( aiReturn_SUCCESS, exporter.Export( scene, "gltf2", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured_out.gltf" ) );

        return true;
    }
#endif // ASSIMP_BUILD_NO_EXPORT

};

TEST_F( utglTF2ImportExport, importglTF2FromFileTest ) {
    EXPECT_TRUE( importerTest() );
}

TEST_F( utglTF2ImportExport, importBinaryglTF2FromFileTest ) {
    EXPECT_TRUE( binaryImporterTest() );
}

#ifndef ASSIMP_BUILD_NO_EXPORT
TEST_F(utglTF2ImportExport, importglTF2AndExportToOBJ) {
    Assimp::Importer importer;
    Assimp::Exporter exporter;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "obj", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF/BoxTextured_out.obj"));
}

TEST_F(utglTF2ImportExport, importglTF2EmbeddedAndExportToOBJ) {
    Assimp::Importer importer;
    Assimp::Exporter exporter;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF-Embedded/BoxTextured.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "obj", ASSIMP_TEST_MODELS_DIR "/glTF2/BoxTextured-glTF-Embedded/BoxTextured_out.obj"));
}
#endif // ASSIMP_BUILD_NO_EXPORT

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModePointsWithoutIndices) {
    Assimp::Importer importer;
    //Points without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_00.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 1024);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 1);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], i);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLinesWithoutIndices) {
    Assimp::Importer importer;
    //Lines without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_01.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 8);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 2);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], i*2);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], i*2 + 1);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLinesLoopWithoutIndices) {
    Assimp::Importer importer;
    //Lines loop without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_02.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4);

    std::array<int, 5> l1 = {{ 0, 1, 2, 3, 0 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 2);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 2);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], l1[i]);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], l1[i + 1]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLinesStripWithoutIndices) {
    Assimp::Importer importer;
    //Lines strip without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_03.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 5);

    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 2);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 2);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], i);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], i + 1);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeTrianglesStripWithoutIndices) {
    Assimp::Importer importer;
    //Triangles strip without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_04.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4);
    std::array<int, 3> f1 = {{ 0, 1, 2 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<int, 3> f2 = {{ 2, 1, 3 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeTrianglesFanWithoutIndices) {
    Assimp::Importer importer;
    //Triangles fan without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_05.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4);
    std::array<int, 3> f1 = {{ 0, 1, 2 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<int, 3> f2 = {{ 0, 2, 3 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeTrianglesWithoutIndices) {
    Assimp::Importer importer;
    //Triangles without indices
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_06.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 6);
    std::array<int, 3> f1 = {{ 0, 1, 2 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<int, 3> f2 = {{ 3, 4, 5 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModePoints) {
    Assimp::Importer importer;
    //Line loop
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_07.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 1024);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 1);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], i);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLines) {
    Assimp::Importer importer;
    //Lines
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_08.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4);
    std::array<int, 5> l1 = {{ 0, 3, 2, 1, 0 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 2);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], l1[i]);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], l1[i + 1]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLineLoop) {
    Assimp::Importer importer;
    //Line loop
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_09.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4);
    std::array<int, 5> l1 = {{ 0, 3, 2, 1, 0 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 2);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], l1[i]);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], l1[i+1]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeLineStrip) {
    Assimp::Importer importer;
    //Lines Strip
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_10.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4);
    std::array<int, 5> l1 = {{ 0, 3, 2, 1, 0 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 2);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[0], l1[i]);
        EXPECT_EQ(scene->mMeshes[0]->mFaces[i].mIndices[1], l1[i + 1]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeTrianglesStrip) {
    Assimp::Importer importer;
    //Triangles strip
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_11.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4);
    std::array<int, 3> f1 = {{ 0, 3, 1 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<int, 3> f2 = {{ 1, 3, 2 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

TEST_F(utglTF2ImportExport, importglTF2PrimitiveModeTrianglesFan) {
    Assimp::Importer importer;
    //Triangles fan
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF2/glTF-Asset-Generator/Mesh_PrimitiveMode/Mesh_PrimitiveMode_12.gltf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 4);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2);
    std::array<int, 3> f1 = {{ 0, 3, 2 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mNumIndices, 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[0].mIndices[i], f1[i]);
    }

    std::array<int, 3> f2 = {{ 0, 2, 1 }};
    EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mNumIndices, 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(scene->mMeshes[0]->mFaces[1].mIndices[i], f2[i]);
    }
}

std::vector<char> ReadFile(const char* name) {
    std::vector<char> ret;

    FILE* p = ::fopen(name, "r");
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

#ifndef ASSIMP_BUILD_NO_EXPORT
TEST_F( utglTF2ImportExport, exportglTF2FromFileTest ) {
    EXPECT_TRUE( exporterTest() );
}

#endif // ASSIMP_BUILD_NO_EXPORT
