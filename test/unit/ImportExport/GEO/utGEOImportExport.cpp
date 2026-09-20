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
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using namespace Assimp;

class utGEOImportExport : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/GEO/gray_plane.3dg", aiProcess_ValidateDataStructure);
#ifndef ASSIMP_BUILD_NO_GEO_IMPORTER
        return nullptr != scene;
#else
        return nullptr == scene;
#endif
    }
};

TEST_F(utGEOImportExport, importGEOFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utGEOImportExport, importInvalidEmpty) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/invalid/empty.geo", aiProcess_ValidateDataStructure);
    ASSERT_EQ(scene, nullptr);
}

TEST_F(utGEOImportExport, import3DGFaceColors) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/GEO/gray_plane.3dg", aiProcess_ValidateDataStructure);
#ifndef ASSIMP_BUILD_NO_GEO_IMPORTER
    ASSERT_NE(scene, nullptr);
    ASSERT_EQ(1u, scene->mNumMeshes);
    const aiMesh *mesh = scene->mMeshes[0];
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(4u, mesh->mNumVertices);
    EXPECT_EQ(1u, mesh->mNumFaces);
    ASSERT_NE(nullptr, mesh->mColors[0]);
    // 0x808080 hex path -> RGB (128/255)
    EXPECT_NEAR(128.0f / 255.0f, mesh->mColors[0][0].r, 1e-4f);
    EXPECT_NEAR(128.0f / 255.0f, mesh->mColors[0][0].g, 1e-4f);
    EXPECT_NEAR(128.0f / 255.0f, mesh->mColors[0][0].b, 1e-4f);
    EXPECT_NEAR(1.0f, mesh->mColors[0][0].a, 1e-4f);
#else
    ASSERT_EQ(scene, nullptr);
#endif
}

TEST_F(utGEOImportExport, importGOURVertexColors) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/GEO/colored_vertices.gour", aiProcess_ValidateDataStructure);
#ifndef ASSIMP_BUILD_NO_GEO_IMPORTER
    ASSERT_NE(scene, nullptr);
    ASSERT_EQ(1u, scene->mNumMeshes);
    const aiMesh *mesh = scene->mMeshes[0];
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(4u, mesh->mNumVertices);
    EXPECT_EQ(1u, mesh->mNumFaces);
    ASSERT_NE(nullptr, mesh->mColors[0]);
    // First vertex color token 0xff0000 is decoded as (1,0,0) by LookupColor(rgbH)
    EXPECT_NEAR(1.0f, mesh->mColors[0][0].r, 1e-4f);
    EXPECT_NEAR(0.0f, mesh->mColors[0][0].g, 1e-4f);
    EXPECT_NEAR(0.0f, mesh->mColors[0][0].b, 1e-4f);
#else
    ASSERT_EQ(scene, nullptr);
#endif
}

TEST_F(utGEOImportExport, importGEOExtensionAlias) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/GEO/colored_vertices.geo", aiProcess_ValidateDataStructure);
#ifndef ASSIMP_BUILD_NO_GEO_IMPORTER
    ASSERT_NE(scene, nullptr);
    ASSERT_EQ(1u, scene->mNumMeshes);
    EXPECT_NE(nullptr, scene->mMeshes[0]->mColors[0]);
#else
    ASSERT_EQ(scene, nullptr);
#endif
}

TEST_F(utGEOImportExport, importPyramidPaletteFaces) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/GEO/pyramid.geo", aiProcess_ValidateDataStructure);
#ifndef ASSIMP_BUILD_NO_GEO_IMPORTER
    ASSERT_NE(scene, nullptr);
    ASSERT_EQ(1u, scene->mNumMeshes);
    const aiMesh *mesh = scene->mMeshes[0];
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(16u, mesh->mNumVertices); // 4*3 + 4
    EXPECT_EQ(5u, mesh->mNumFaces);
    ASSERT_NE(nullptr, mesh->mColors[0]);
#else
    ASSERT_EQ(scene, nullptr);
#endif
}

TEST_F(utGEOImportExport, importBox3DG) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/GEO/box.3dg", aiProcess_ValidateDataStructure);
#ifndef ASSIMP_BUILD_NO_GEO_IMPORTER
    ASSERT_NE(scene, nullptr);
    ASSERT_EQ(1u, scene->mNumMeshes);
    const aiMesh *mesh = scene->mMeshes[0];
    ASSERT_NE(nullptr, mesh);
    EXPECT_EQ(24u, mesh->mNumVertices); // 6 quads * 4
    EXPECT_EQ(6u, mesh->mNumFaces);
#else
    ASSERT_EQ(scene, nullptr);
#endif
}
