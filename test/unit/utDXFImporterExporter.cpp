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

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

class utDXFImporterExporter : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/DXF/PinkEggFromLW.dxf", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utDXFImporterExporter, importDXFFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utDXFImporterExporter, importerWithoutExtensionTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/DXF/lineTest", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
}

TEST_F(utDXFImporterExporter, issue2229) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/DXF/issue_2229.dxf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
}

TEST_F(utDXFImporterExporter, importPointEntities) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/DXF/points.dxf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(1u, scene->mNumMeshes);

    // Both POINT entities live on layer "0" and end up in one point-only mesh.
    const aiMesh *mesh = scene->mMeshes[0];
    EXPECT_EQ(static_cast<unsigned int>(aiPrimitiveType_POINT), mesh->mPrimitiveTypes);
    ASSERT_EQ(2u, mesh->mNumVertices);
    ASSERT_EQ(2u, mesh->mNumFaces);
    EXPECT_EQ(1u, mesh->mFaces[0].mNumIndices);
    EXPECT_EQ(1u, mesh->mFaces[1].mNumIndices);

    // Vertices are checked in mesh-local space, before the root node's
    // AutoCAD-to-assimp axis conversion is applied.
    EXPECT_NEAR(1.0, mesh->mVertices[0].x, 1e-6);
    EXPECT_NEAR(2.0, mesh->mVertices[0].y, 1e-6);
    EXPECT_NEAR(3.0, mesh->mVertices[0].z, 1e-6);
    EXPECT_NEAR(-4.0, mesh->mVertices[1].x, 1e-6);
    EXPECT_NEAR(5.5, mesh->mVertices[1].y, 1e-6);
    EXPECT_NEAR(0.25, mesh->mVertices[1].z, 1e-6);
}


TEST_F(utDXFImporterExporter, importWuson) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/DXF/wuson.dxf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
}


TEST_F(utDXFImporterExporter, importRifle) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/DXF/rifle.dxf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
}
