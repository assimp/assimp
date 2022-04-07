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
#include "SceneDiffer.h"
#include "UnitTestPCH.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>

using namespace Assimp;

static const float VertComponents[24 * 3] = {
    -0.500000, 0.500000, 0.500000,
    -0.500000, 0.500000, -0.500000,
    -0.500000, -0.500000, -0.500000,
    -0.500000, -0.500000, 0.500000,
    -0.500000, -0.500000, -0.500000,
    0.500000, -0.500000, -0.500000,
    0.500000, -0.500000, 0.500000,
    -0.500000, -0.500000, 0.500000,
    -0.500000, 0.500000, -0.500000,
    0.500000, 0.500000, -0.500000,
    0.500000, -0.500000, -0.500000,
    -0.500000, -0.500000, -0.500000,
    0.500000, 0.500000, 0.500000,
    0.500000, 0.500000, -0.500000,
    -0.500000, 0.500000, -0.500000,
    -0.500000, 0.500000, 0.500000,
    0.500000, -0.500000, 0.500000,
    0.500000, 0.500000, 0.500000,
    -0.500000, 0.500000, 0.500000,
    -0.500000, -0.500000, 0.500000,
    0.500000, -0.500000, -0.500000,
    0.500000, 0.500000, -0.500000,
    0.500000, 0.500000, 0.500000f,
    0.500000, -0.500000, 0.500000f
};

static const char *ObjModel =
        "o 1\n"
        "\n"
        "# Vertex list\n"
        "\n"
        "v -0.5 -0.5  0.5\n"
        "v -0.5 -0.5 -0.5\n"
        "v -0.5  0.5 -0.5\n"
        "v -0.5  0.5  0.5\n"
        "v  0.5 -0.5  0.5\n"
        "v  0.5 -0.5 -0.5\n"
        "v  0.5  0.5 -0.5\n"
        "v  0.5  0.5  0.5\n"
        "\n"
        "# Point / Line / Face list\n"
        "\n"
        "g Box01\n"
        "usemtl Default\n"
        "f 4 3 2 1\n"
        "f 2 6 5 1\n"
        "f 3 7 6 2\n"
        "f 8 7 3 4\n"
        "f 5 8 4 1\n"
        "f 6 7 8 5\n"
        "\n"
        "# End of file\n";

static const char *ObjModel_Issue1111 =
        "o 1\n"
        "\n"
        "# Vertex list\n"
        "\n"
        "v -0.5 -0.5  0.5\n"
        "v -0.5 -0.5 -0.5\n"
        "v -0.5  0.5 -0.5\n"
        "\n"
        "usemtl\n"
        "f 1 2 3\n"
        "\n"
        "# End of file\n";

class utObjImportExport : public AbstractImportExportBase {
protected:
    void SetUp() override {
        m_im = new Assimp::Importer;
    }

    void TearDown() override {
        delete m_im;
        m_im = nullptr;
    }

    aiScene *createScene() {
        aiScene *expScene = new aiScene;
        expScene->mNumMeshes = 1;
        expScene->mMeshes = new aiMesh *[1];
        aiMesh *mesh = new aiMesh;
        mesh->mName.Set("Box01");
        mesh->mNumVertices = 24;
        mesh->mVertices = new aiVector3D[24];
        ::memcpy(&mesh->mVertices->x, &VertComponents[0], sizeof(float) * 24 * 3);
        mesh->mNumFaces = 6;
        mesh->mFaces = new aiFace[mesh->mNumFaces];

        mesh->mFaces[0].mNumIndices = 4;
        mesh->mFaces[0].mIndices = new unsigned int[mesh->mFaces[0].mNumIndices];
        mesh->mFaces[0].mIndices[0] = 0;
        mesh->mFaces[0].mIndices[1] = 1;
        mesh->mFaces[0].mIndices[2] = 2;
        mesh->mFaces[0].mIndices[3] = 3;

        mesh->mFaces[1].mNumIndices = 4;
        mesh->mFaces[1].mIndices = new unsigned int[mesh->mFaces[0].mNumIndices];
        mesh->mFaces[1].mIndices[0] = 4;
        mesh->mFaces[1].mIndices[1] = 5;
        mesh->mFaces[1].mIndices[2] = 6;
        mesh->mFaces[1].mIndices[3] = 7;

        mesh->mFaces[2].mNumIndices = 4;
        mesh->mFaces[2].mIndices = new unsigned int[mesh->mFaces[0].mNumIndices];
        mesh->mFaces[2].mIndices[0] = 8;
        mesh->mFaces[2].mIndices[1] = 9;
        mesh->mFaces[2].mIndices[2] = 10;
        mesh->mFaces[2].mIndices[3] = 11;

        mesh->mFaces[3].mNumIndices = 4;
        mesh->mFaces[3].mIndices = new unsigned int[mesh->mFaces[0].mNumIndices];
        mesh->mFaces[3].mIndices[0] = 12;
        mesh->mFaces[3].mIndices[1] = 13;
        mesh->mFaces[3].mIndices[2] = 14;
        mesh->mFaces[3].mIndices[3] = 15;

        mesh->mFaces[4].mNumIndices = 4;
        mesh->mFaces[4].mIndices = new unsigned int[mesh->mFaces[0].mNumIndices];
        mesh->mFaces[4].mIndices[0] = 16;
        mesh->mFaces[4].mIndices[1] = 17;
        mesh->mFaces[4].mIndices[2] = 18;
        mesh->mFaces[4].mIndices[3] = 19;

        mesh->mFaces[5].mNumIndices = 4;
        mesh->mFaces[5].mIndices = new unsigned int[mesh->mFaces[0].mNumIndices];
        mesh->mFaces[5].mIndices[0] = 20;
        mesh->mFaces[5].mIndices[1] = 21;
        mesh->mFaces[5].mIndices[2] = 22;
        mesh->mFaces[5].mIndices[3] = 23;

        expScene->mMeshes[0] = mesh;

        expScene->mNumMaterials = 1;
        expScene->mMaterials = new aiMaterial *[expScene->mNumMaterials];

        return expScene;
    }

    bool importerTest() override {
        ::Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/spider.obj", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }

#ifndef ASSIMP_BUILD_NO_EXPORT

    bool exporterTest() override {
        ::Assimp::Importer importer;
        ::Assimp::Exporter exporter;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/spider.obj", aiProcess_ValidateDataStructure);
        EXPECT_NE(nullptr, scene);
        EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "obj", ASSIMP_TEST_MODELS_DIR "/OBJ/spider_out.obj"));
        EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "objnomtl", ASSIMP_TEST_MODELS_DIR "/OBJ/spider_nomtl_out.obj"));

        return true;
    }

#endif // ASSIMP_BUILD_NO_EXPORT

protected:
    ::Assimp::Importer *m_im;
    aiScene *m_expectedScene;
};

TEST_F(utObjImportExport, importObjFromFileTest) {
    EXPECT_TRUE(importerTest());
}

#ifndef ASSIMP_BUILD_NO_EXPORT

TEST_F(utObjImportExport, exportObjFromFileTest) {
    EXPECT_TRUE(exporterTest());
}

#endif // ASSIMP_BUILD_NO_EXPORT

TEST_F(utObjImportExport, obj_import_test) {
    const aiScene *scene = m_im->ReadFileFromMemory((void *)ObjModel, strlen(ObjModel), 0);
    aiScene *expected = createScene();
    EXPECT_NE(nullptr, scene);

    SceneDiffer differ;
    EXPECT_TRUE(differ.isEqual(expected, scene));
    differ.showReport();

    m_im->FreeScene();
    for (unsigned int i = 0; i < expected->mNumMeshes; ++i) {
        delete expected->mMeshes[i];
    }
    delete[] expected->mMeshes;
    expected->mMeshes = nullptr;
    delete[] expected->mMaterials;
    expected->mMaterials = nullptr;
    delete expected;
}

TEST_F(utObjImportExport, issue1111_no_mat_name_Test) {
    const aiScene *scene = m_im->ReadFileFromMemory((void *)ObjModel_Issue1111, strlen(ObjModel_Issue1111), 0);
    EXPECT_NE(nullptr, scene);
}

TEST_F(utObjImportExport, issue809_vertex_color_Test) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/cube_with_vertexcolors.obj", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);

#ifndef ASSIMP_BUILD_NO_EXPORT
    ::Assimp::Exporter exporter;
    EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "obj", ASSIMP_TEST_MODELS_DIR "/OBJ/test_out.obj"));
#endif // ASSIMP_BUILD_NO_EXPORT
}

TEST_F(utObjImportExport, issue1923_vertex_color_Test) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/cube_with_vertexcolors_uni.obj", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);

    scene = importer.GetOrphanedScene();

#ifndef ASSIMP_BUILD_NO_EXPORT
    ::Assimp::Exporter exporter;
    const aiExportDataBlob *blob = exporter.ExportToBlob(scene, "obj");
    EXPECT_NE(nullptr, blob);

    const aiScene *sceneReImport = importer.ReadFileFromMemory(blob->data, blob->size, aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);

    SceneDiffer differ;
    EXPECT_TRUE(differ.isEqual(scene, sceneReImport));
#endif // ASSIMP_BUILD_NO_EXPORT

    delete scene;
}

TEST_F(utObjImportExport, issue1453_segfault) {
    static const char *curObjModel =
            "v  0.0  0.0  0.0\n"
            "v  0.0  0.0  1.0\n"
            "v  0.0  1.0  0.0\n"
            "v  0.0  1.0  1.0\n"
            "v  1.0  0.0  0.0\n"
            "v  1.0  0.0  1.0\n"
            "v  1.0  1.0  0.0\n"
            "v  1.0  1.0  1.0\nB";

    Assimp::Importer myimporter;
    const aiScene *scene = myimporter.ReadFileFromMemory(curObjModel, strlen(curObjModel), aiProcess_ValidateDataStructure);
    EXPECT_EQ(nullptr, scene);
}

TEST_F(utObjImportExport, relative_indices_Test) {
    static const char *curObjModel =
            "v -0.500000 0.000000 0.400000\n"
            "v -0.500000 0.000000 -0.800000\n"
            "v -0.500000 1.000000 -0.800000\n"
            "v -0.500000 1.000000 0.400000\n"
            "f -4 -3 -2 -1\nB";

    Assimp::Importer myimporter;
    const aiScene *scene = myimporter.ReadFileFromMemory(curObjModel, strlen(curObjModel), aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);

    EXPECT_EQ(scene->mNumMeshes, 1U);
    const aiMesh *mesh = scene->mMeshes[0];
    EXPECT_EQ(mesh->mNumVertices, 4U);
    EXPECT_EQ(mesh->mNumFaces, 1U);
    const aiFace face = mesh->mFaces[0];
    EXPECT_EQ(face.mNumIndices, 4U);
    for (unsigned int i = 0; i < face.mNumIndices; ++i) {
        EXPECT_EQ(face.mIndices[i], i);
    }
}

TEST_F(utObjImportExport, homogeneous_coordinates_Test) {
    static const char *curObjModel =
            "v -0.500000 0.000000 0.400000 0.50000\n"
            "v -0.500000 0.000000 -0.800000 1.00000\n"
            "v 0.500000 1.000000 -0.800000 0.5000\n"
            "f 1 2 3\nB";

    Assimp::Importer myimporter;
    const aiScene *scene = myimporter.ReadFileFromMemory(curObjModel, strlen(curObjModel), aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);

    EXPECT_EQ(scene->mNumMeshes, 1U);
    const aiMesh *mesh = scene->mMeshes[0];
    EXPECT_EQ(mesh->mNumVertices, 3U);
    EXPECT_EQ(mesh->mNumFaces, 1U);
    const aiFace face = mesh->mFaces[0];
    EXPECT_EQ(face.mNumIndices, 3U);
    const aiVector3D vertice = mesh->mVertices[0];
    EXPECT_EQ(vertice.x, -1.0f);
    EXPECT_EQ(vertice.y, 0.0f);
    EXPECT_EQ(vertice.z, 0.8f);
}

TEST_F(utObjImportExport, homogeneous_coordinates_divide_by_zero_Test) {
    static const char *curObjModel =
            "v -0.500000 0.000000 0.400000 0.\n"
            "v -0.500000 0.000000 -0.800000 1.00000\n"
            "v 0.500000 1.000000 -0.800000 0.5000\n"
            "f 1 2 3\nB";

    Assimp::Importer myimporter;
    const aiScene *scene = myimporter.ReadFileFromMemory(curObjModel, std::strlen(curObjModel), aiProcess_ValidateDataStructure);
    EXPECT_EQ(nullptr, scene);
}

TEST_F(utObjImportExport, 0based_array_Test) {
    static const char *curObjModel =
            "v -0.500000 0.000000 0.400000\n"
            "v -0.500000 0.000000 -0.800000\n"
            "v -0.500000 1.000000 -0.800000\n"
            "f 0 1 2\nB";

    Assimp::Importer myImporter;
    const aiScene *scene = myImporter.ReadFileFromMemory(curObjModel, strlen(curObjModel), 0);
    EXPECT_EQ(nullptr, scene);
}

TEST_F(utObjImportExport, invalid_normals_uvs) {
    static const char *curObjModel =
            "v -0.500000 0.000000 0.400000\n"
            "v -0.500000 0.000000 -0.800000\n"
            "v -0.500000 1.000000 -0.800000\n"
            "vt 0 0\n"
            "vn 0 1 0\n"
            "f 1/1/1 1/1/1 2/2/2\nB";

    Assimp::Importer myImporter;
    const aiScene *scene = myImporter.ReadFileFromMemory(curObjModel, strlen(curObjModel), 0);
    EXPECT_NE(nullptr, scene);
}

TEST_F(utObjImportExport, no_vt_just_vns) {
    static const char *curObjModel =
            "v 0 0 0\n"
            "v 0 0 0\n"
            "v 0 0 0\n"
            "v 0 0 0\n"
            "v 0 0 0\n"
            "v 0 0 0\n"
            "v 0 0 0\n"
            "v 0 0 0\n"
            "v 0 0 0\n"
            "v 0 0 0\n"
            "v 10 0 0\n"
            "v 0 10 0\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "vn 0 0 1\n"
            "f 10/10 11/11 12/12\n";

    Assimp::Importer myImporter;
    const aiScene *scene = myImporter.ReadFileFromMemory(curObjModel, strlen(curObjModel), 0);
    EXPECT_NE(nullptr, scene);
}

TEST_F(utObjImportExport, mtllib_after_g) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/cube_mtllib_after_g.obj", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);

    EXPECT_EQ(scene->mNumMeshes, 1U);
    const aiMesh *mesh = scene->mMeshes[0];
    const aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
    aiString name;
    ASSERT_EQ(aiReturn_SUCCESS, mat->Get(AI_MATKEY_NAME, name));
    EXPECT_STREQ("MyMaterial", name.C_Str());
}

TEST_F(utObjImportExport, import_point_cloud) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/point_cloud.obj", 0);
    ASSERT_NE(nullptr, scene);
}

TEST_F(utObjImportExport, import_without_linend) {
    Assimp::Importer myImporter;
    const aiScene *scene = myImporter.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/box_without_lineending.obj", 0);
    ASSERT_NE(nullptr, scene);
}

TEST_F(utObjImportExport, import_with_line_continuations) {
    static const char *curObjModel =
            "v -0.5 -0.5 0.5\n"
            "v -0.5 \\\n"
            "  -0.5 -0.5\n"
            "v -0.5 \\\n"
            "   0.5 \\\n"
            "   -0.5\n"
            "f 1 2 3\n";

    Assimp::Importer myImporter;
    const aiScene *scene = myImporter.ReadFileFromMemory(curObjModel, strlen(curObjModel), 0);
    EXPECT_NE(nullptr, scene);

    EXPECT_EQ(scene->mNumMeshes, 1U);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 3U);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 1U);

    auto vertices = scene->mMeshes[0]->mVertices;
    const float threshold = 0.0001f;

    EXPECT_NEAR(vertices[0].x, -0.5f, threshold);
    EXPECT_NEAR(vertices[0].y, -0.5f, threshold);
    EXPECT_NEAR(vertices[0].z, 0.5f, threshold);

    EXPECT_NEAR(vertices[1].x, -0.5f, threshold);
    EXPECT_NEAR(vertices[1].y, -0.5f, threshold);
    EXPECT_NEAR(vertices[1].z, -0.5f, threshold);

    EXPECT_NEAR(vertices[2].x, -0.5f, threshold);
    EXPECT_NEAR(vertices[2].y, 0.5f, threshold);
    EXPECT_NEAR(vertices[2].z, -0.5f, threshold);
}
