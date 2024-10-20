/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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
#include <assimp/scene.h>

using namespace Assimp;

class utX3DImportExport : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X3D/HelloX3dTrademark.x3d", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utX3DImportExport, importX3DFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utX3DImportExport, importX3DIndexedLineSet) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X3D/IndexedLineSet.x3d", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(scene->mNumMeshes, 1u);
    ASSERT_EQ(scene->mMeshes[0]->mNumFaces, 4u);
    ASSERT_EQ(scene->mMeshes[0]->mPrimitiveTypes, aiPrimitiveType_LINE);
    ASSERT_EQ(scene->mMeshes[0]->mNumVertices, 4u);
    for (unsigned int i = 0; i < scene->mMeshes[0]->mNumFaces; i++) {
        ASSERT_EQ(scene->mMeshes[0]->mFaces[i].mNumIndices, 2u);
    }
}

TEST_F(utX3DImportExport, importX3DComputerKeyboard) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X3D/ComputerKeyboard.x3d", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    // TODO: CHANGE INCORRECT VALUE WHEN IMPORTER FIXED
    //   As noted in assimp issue 4992, X3D importer was severely broken with 5 Oct 2020 commit 3b9d4cf.
    //   ComputerKeyboard.x3d should have 100 meshes but broken importer only has 4
    ASSERT_EQ(4u, scene->mNumMeshes);  // Incorrect value from currently broken importer
    ASSERT_NE(100u, scene->mNumMeshes); // Correct value, to be restored when importer fixed
}

TEST_F(utX3DImportExport, importX3DChevyTahoe) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/X3D/Chevy/ChevyTahoe.x3d", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    // TODO: CHANGE INCORRECT VALUE WHEN IMPORTER FIXED
    //   As noted in assimp issue 4992, X3D importer was severely broken with 5 Oct 2020 commit 3b9d4cf.
    //   ChevyTahoe.x3d should have 20 meshes but broken importer only has 19
    ASSERT_EQ(19u, scene->mNumMeshes); // Incorrect value from currently broken importer
    ASSERT_NE(20u, scene->mNumMeshes); // Correct value, to be restored when importer fixed
}
