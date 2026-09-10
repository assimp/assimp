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

#include "AbstractImportExportBase.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>

#include <sstream>
#include <string>
#include <vector>

using namespace ::Assimp;

class utPLYImportExport : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube.ply", aiProcess_ValidateDataStructure);
        EXPECT_EQ(1u, scene->mNumMeshes);
        EXPECT_NE(nullptr, scene->mMeshes[0]);
        if (nullptr == scene->mMeshes[0]) {
            return false;
        }
        EXPECT_EQ(8u, scene->mMeshes[0]->mNumVertices);
        EXPECT_EQ(6u, scene->mMeshes[0]->mNumFaces);

        return (nullptr != scene);
    }

#ifndef ASSIMP_BUILD_NO_EXPORT
    virtual bool exporterTest() {
        Importer importer;
        Exporter exporter;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube.ply", aiProcess_ValidateDataStructure);
        EXPECT_NE(nullptr, scene);
        EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "ply", ASSIMP_TEST_MODELS_DIR "/PLY/cube_out.ply"));

        return true;
    }
#endif // ASSIMP_BUILD_NO_EXPORT
};

TEST_F(utPLYImportExport, importTest_Success) {
    EXPECT_TRUE(importerTest());
}

#ifndef ASSIMP_BUILD_NO_EXPORT

TEST_F(utPLYImportExport, exportTest_Success) {
    EXPECT_TRUE(exporterTest());
}

#endif // ASSIMP_BUILD_NO_EXPORT

// Test issue 1623, crash when loading two PLY files in a row
TEST_F(utPLYImportExport, importerMultipleTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube.ply", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);

    scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube.ply", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
    EXPECT_NE(nullptr, scene->mMeshes[0]);
    EXPECT_EQ(6u, scene->mMeshes[0]->mNumFaces);
}

TEST_F(utPLYImportExport, importPLYwithUV) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube_uv.ply", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
    EXPECT_NE(nullptr, scene->mMeshes[0]);
    // This test model is using n-gons, so 6 faces instead of 12 tris
    EXPECT_EQ(6u, scene->mMeshes[0]->mNumFaces);
    EXPECT_EQ(aiPrimitiveType_POLYGON, scene->mMeshes[0]->mPrimitiveTypes);
    EXPECT_EQ(true, scene->mMeshes[0]->HasTextureCoords(0));
}

TEST_F(utPLYImportExport, importBinaryPLY) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube_binary.ply", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
    EXPECT_NE(nullptr, scene->mMeshes[0]);
    // This test model is double sided, so 12 faces instead of 6
    EXPECT_EQ(12u, scene->mMeshes[0]->mNumFaces);
}

// Tests of a PLY file gets read with \r\n as newlines instead of just \n (i.e. solidwork exported ply files)
TEST_F(utPLYImportExport, importBinaryPLYWithRNNewline) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube_binary_header_with_RN_newline.ply", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
    ASSERT_NE(nullptr, scene->mMeshes[0]);
    // This test model is double sided, so 12 faces instead of 6
    ASSERT_EQ(12u, scene->mMeshes[0]->mNumFaces);
    // Also check if the indices were parsed correctly
    ASSERT_EQ(3u, scene->mMeshes[0]->mFaces[0].mNumIndices);
    EXPECT_EQ(0u, scene->mMeshes[0]->mFaces[0].mIndices[0]);
    EXPECT_EQ(1u, scene->mMeshes[0]->mFaces[0].mIndices[1]);
    EXPECT_EQ(2u, scene->mMeshes[0]->mFaces[0].mIndices[2]);
}

// Tests of a PLY file gets read with \n as the fist character in the BINARY part
TEST_F(utPLYImportExport, importBinaryPLYWithNewlineInBinary) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube_binary_starts_with_nl.ply", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
    ASSERT_NE(nullptr, scene->mMeshes[0]);
    ASSERT_EQ(8u, scene->mMeshes[0]->mNumVertices);
    // Make sure the first binary float was read correctly
    ASSERT_FLOAT_EQ(5.967534f, scene->mMeshes[0]->mVertices[0][0]);
    ASSERT_FLOAT_EQ(0, scene->mMeshes[0]->mVertices[0][1]);
    ASSERT_FLOAT_EQ(0, scene->mMeshes[0]->mVertices[0][2]);

    ASSERT_EQ(6u, scene->mMeshes[0]->mNumFaces);
    // Also check if the indices were parsed correctly
    ASSERT_EQ(4u, scene->mMeshes[0]->mFaces[0].mNumIndices);
    EXPECT_EQ(0u, scene->mMeshes[0]->mFaces[0].mIndices[0]);
    EXPECT_EQ(1u, scene->mMeshes[0]->mFaces[0].mIndices[1]);
    EXPECT_EQ(2u, scene->mMeshes[0]->mFaces[0].mIndices[2]);
}

TEST_F(utPLYImportExport, vertexColorTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/float-color.ply", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(1u, scene->mMeshes[0]->mNumFaces);
    EXPECT_EQ(aiPrimitiveType_TRIANGLE, scene->mMeshes[0]->mPrimitiveTypes);
    EXPECT_EQ(true, scene->mMeshes[0]->HasVertexColors(0));

    auto first_face = scene->mMeshes[0]->mFaces[0];
    EXPECT_EQ(3u, first_face.mNumIndices);
    EXPECT_EQ(0u, first_face.mIndices[0]);
    EXPECT_EQ(1u, first_face.mIndices[1]);
    EXPECT_EQ(2u, first_face.mIndices[2]);
}

// Test issue #623, PLY importer should not automatically create faces
TEST_F(utPLYImportExport, pointcloudTest) {
    Assimp::Importer importer;

    // Could not use aiProcess_ValidateDataStructure since it's missing faces.
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/issue623.ply", 0);
    EXPECT_NE(nullptr, scene);

    EXPECT_EQ(1u, scene->mNumMeshes);
    EXPECT_NE(nullptr, scene->mMeshes[0]);
    EXPECT_EQ(24u, scene->mMeshes[0]->mNumVertices);
    EXPECT_EQ(aiPrimitiveType::aiPrimitiveType_POINT, scene->mMeshes[0]->mPrimitiveTypes);
    EXPECT_EQ(0u, scene->mMeshes[0]->mNumFaces);
}

static const char *test_file =
        "ply\n"
        "format ascii 1.0\n"
        "element vertex 4\n"
        "property float x\n"
        "property float y\n"
        "property float z\n"
        "property uchar red\n"
        "property uchar green\n"
        "property uchar blue\n"
        "property float nx\n"
        "property float ny\n"
        "property float nz\n"
        "end_header\n"
        "0.0 0.0 0.0 255 255 255 0.0 1.0 0.0\n"
        "0.0 0.0 1.0 255 0 255 0.0 0.0 1.0\n"
        "0.0 1.0 0.0 255 255 0 1.0 0.0 0.0\n"
        "0.0 1.0 1.0 0 255 255 1.0 1.0 0.0\n";

TEST_F(utPLYImportExport, parseErrorTest) {
    Assimp::Importer importer;
    // Could not use aiProcess_ValidateDataStructure since it's missing faces.
    const aiScene *scene = importer.ReadFileFromMemory(test_file, strlen(test_file), 0);
    EXPECT_NE(nullptr, scene);
}

// This file is invalid, we just want to ensure that the importer is not crashing
TEST_F(utPLYImportExport, parseInvalid) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/invalid/crash-30d6d0f7c529b3b66b4131700b7a4580cd7082df.ply", 0);
    EXPECT_EQ(nullptr, scene);
}

TEST_F(utPLYImportExport, payload_JVN42386607) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/payload_JVN42386607", 0);
    EXPECT_EQ(nullptr, scene);
}

// Tests Issue #5729. Test, if properties defined multiple times. Unclear what to do, better to abort than to crash entirely
TEST_F(utPLYImportExport, parseInvalidDoubleProperty) {
    const char data[] = "ply\n"
                        "format ascii 1.0\n"
                        "element vertex 4\n"
                        "property float x\n"
                        "property float y\n"
                        "property float z\n"
                        "element vertex 8\n"
                        "property float x\n"
                        "property float y\n"
                        "property float z\n"
                        "end_header\n"
                        "0.0 0.0 0.0 0.0 0.0 0.0\n"
                        "0.0 0.0 1.0 0.0 0.0 1.0\n"
                        "0.0 1.0 0.0 0.0 1.0 0.0\n"
                        "0.0 0.0 1.0\n"
                        "0.0 1.0 0.0 0.0 0.0 1.0\n"
                        "0.0 1.0 1.0 0.0 1.0 1.0\n";

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(data, sizeof(data), 0);
    EXPECT_EQ(nullptr, scene);
}

// Tests Issue #5729. Test, if properties defined multiple times. Unclear what to do, better to abort than to crash entirely
TEST_F(utPLYImportExport, parseInvalidDoubleCustomProperty) {
    const char data[] = "ply\n"
                        "format ascii 1.0\n"
                        "element vertex 4\n"
                        "property float x\n"
                        "property float y\n"
                        "property float z\n"
                        "element name 8\n"
                        "property float x\n"
                        "element name 5\n"
                        "property float x\n"
                        "end_header\n"
                        "0.0 0.0 0.0 100.0 10.0\n"
                        "0.0 0.0 1.0 200.0 20.0\n"
                        "0.0 1.0 0.0 300.0 30.0\n"
                        "0.0 1.0 1.0 400.0 40.0\n"
                        "0.0 0.0 0.0 500.0 50.0\n"
                        "0.0 0.0 1.0 600.0 60.0\n"
                        "0.0 1.0 0.0 700.0 70.0\n"
                        "0.0 1.0 1.0 800.0 80.0\n";

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(data, sizeof(data), 0);
    EXPECT_EQ(nullptr, scene);
}

#ifndef ASSIMP_BUILD_NO_EXPORT

// A PLY header has to declare exactly the properties that each vertex row carries. Anything else is
// unreadable for a conformant parser (ASCII) or silently misaligned (binary).
static void expectBlobHeaderMatchesVertexRows(const aiExportDataBlob *blob, const char *model) {
    std::istringstream ply(std::string(static_cast<const char *>(blob->data), blob->size));
    std::string line, element;
    unsigned int declared = 0, vertices = 0;
    while (std::getline(ply, line)) {
        std::istringstream words(line);
        std::string first;
        words >> first;
        if (first == "element") {
            words >> element >> vertices;
        } else if (first == "property" && element == "vertex") {
            ++declared;
        } else if (first == "end_header") {
            break;
        }
    }
    ASSERT_GT(declared, 0u) << model;
    ASSERT_GT(vertices, 0u) << model;

    for (unsigned int i = 0; i < vertices; ++i) {
        ASSERT_TRUE(std::getline(ply, line)) << model << ": vertex row " << i << " missing";
        std::istringstream values(line);
        unsigned int written = 0;
        for (std::string value; values >> value;) {
            ++written;
        }
        EXPECT_EQ(declared, written) << model << ": vertex row " << i
                                     << " carries " << written << " values but the header declares "
                                     << declared << " properties";
    }
}

static void expectHeaderMatchesVertexRows(const char *model) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(model, aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene) << model;

    Assimp::Exporter exporter;
    const aiExportDataBlob *blob = exporter.ExportToBlob(scene, "ply");
    ASSERT_NE(nullptr, blob) << model;
    expectBlobHeaderMatchesVertexRows(blob, model);
}

// Tests that the exported header describes the exported payload, for a mesh with neither texture
// coordinates nor vertex colours, with texture coordinates, and with vertex colours.
TEST_F(utPLYImportExport, exportedHeaderMatchesVertexRows) {
    expectHeaderMatchesVertexRows(ASSIMP_TEST_MODELS_DIR "/PLY/cube.ply");
    expectHeaderMatchesVertexRows(ASSIMP_TEST_MODELS_DIR "/PLY/cube_uv.ply");
    expectHeaderMatchesVertexRows(ASSIMP_TEST_MODELS_DIR "/PLY/float-color.ply");
}

// Tests the same invariant for a 3-component (UVW) texture-coordinate channel, which is written with
// one value more per vertex than the usual 2-component channel.
TEST_F(utPLYImportExport, exportedHeaderMatchesVertexRowsForUVW) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube_uv.ply", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(1u, scene->mNumMeshes);
    ASSERT_TRUE(scene->mMeshes[0]->HasTextureCoords(0));
    scene->mMeshes[0]->mNumUVComponents[0] = 3;

    Assimp::Exporter exporter;
    const aiExportDataBlob *blob = exporter.ExportToBlob(scene, "ply");
    ASSERT_NE(nullptr, blob);
    expectBlobHeaderMatchesVertexRows(blob, "cube_uv.ply as UVW");
}

// Tests that texture coordinates and vertex colours survive an export/import round trip.
TEST_F(utPLYImportExport, exportImportRoundTripKeepsTexCoords) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube_uv.ply", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(1u, scene->mNumMeshes);
    ASSERT_TRUE(scene->mMeshes[0]->HasTextureCoords(0));
    const unsigned int numVertices = scene->mMeshes[0]->mNumVertices;
    std::vector<aiVector3D> expected(scene->mMeshes[0]->mTextureCoords[0],
            scene->mMeshes[0]->mTextureCoords[0] + numVertices);

    Assimp::Exporter exporter;
    const aiExportDataBlob *blob = exporter.ExportToBlob(scene, "ply");
    ASSERT_NE(nullptr, blob);

    Assimp::Importer reimporter;
    const aiScene *roundTripped = reimporter.ReadFileFromMemory(blob->data, blob->size, 0, "ply");
    ASSERT_NE(nullptr, roundTripped);
    ASSERT_EQ(1u, roundTripped->mNumMeshes);
    ASSERT_EQ(numVertices, roundTripped->mMeshes[0]->mNumVertices);
    ASSERT_TRUE(roundTripped->mMeshes[0]->HasTextureCoords(0));
    for (unsigned int i = 0; i < numVertices; ++i) {
        EXPECT_NEAR(expected[i].x, roundTripped->mMeshes[0]->mTextureCoords[0][i].x, 1e-5) << "vertex " << i;
        EXPECT_NEAR(expected[i].y, roundTripped->mMeshes[0]->mTextureCoords[0][i].y, 1e-5) << "vertex " << i;
    }
}

#endif // ASSIMP_BUILD_NO_EXPORT
