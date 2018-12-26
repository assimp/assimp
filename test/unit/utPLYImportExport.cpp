/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team


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

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include "AbstractImportExportBase.h"
#include <assimp/postprocess.h>

using namespace ::Assimp;

class utPLYImportExport : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/PLY/cube.ply", aiProcess_ValidateDataStructure);
        EXPECT_EQ( 1u, scene->mNumMeshes );
        EXPECT_NE( nullptr, scene->mMeshes[0] );
        EXPECT_EQ( 8u, scene->mMeshes[0]->mNumVertices );
        EXPECT_EQ( 6u, scene->mMeshes[0]->mNumFaces );
        
        return (nullptr != scene);
    }

#ifndef ASSIMP_BUILD_NO_EXPORT
    virtual bool exporterTest() {
        Importer importer;
        Exporter exporter;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube.ply", aiProcess_ValidateDataStructure);
        EXPECT_NE(nullptr, scene);
        EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "ply", ASSIMP_TEST_MODELS_DIR "/PLY/cube_test.ply"));

        return true;
    }
#endif // ASSIMP_BUILD_NO_EXPORT
};

TEST_F( utPLYImportExport, importTest_Success ) {
    EXPECT_TRUE( importerTest() );
}

#ifndef ASSIMP_BUILD_NO_EXPORT

TEST_F(utPLYImportExport, exportTest_Success ) {
    EXPECT_TRUE(exporterTest());
}

#endif // ASSIMP_BUILD_NO_EXPORT

//Test issue 1623, crash when loading two PLY files in a row
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
    //This test model is using n-gons, so 6 faces instead of 12 tris
    EXPECT_EQ(6u, scene->mMeshes[0]->mNumFaces);
    EXPECT_EQ(aiPrimitiveType_POLYGON, scene->mMeshes[0]->mPrimitiveTypes);
    EXPECT_EQ(true, scene->mMeshes[0]->HasTextureCoords(0));
}

TEST_F(utPLYImportExport, importBinaryPLY) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/PLY/cube_binary.ply", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
    EXPECT_NE(nullptr, scene->mMeshes[0]);
    //This test model is double sided, so 12 faces instead of 6
    EXPECT_EQ(12u, scene->mMeshes[0]->mNumFaces);
}

TEST_F( utPLYImportExport, vertexColorTest ) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/PLY/float-color.ply", aiProcess_ValidateDataStructure);
    EXPECT_NE( nullptr, scene );
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

    //Could not use aiProcess_ValidateDataStructure since it's missing faces.
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

TEST_F( utPLYImportExport, parseErrorTest ) {
    Assimp::Importer importer;
    //Could not use aiProcess_ValidateDataStructure since it's missing faces.
    const aiScene *scene = importer.ReadFileFromMemory( test_file, strlen( test_file ), 0);
    EXPECT_NE( nullptr, scene );
}
