/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

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

class utXImporterExporter : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/test.x", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utXImporterExporter, importXFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utXImporterExporter, heap_overflow_in_tokenizer) {
    Assimp::Importer importer;
    EXPECT_NO_THROW(importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/OV_GetNextToken", 0));
}

TEST(utXImporter, importAnimTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/anim_test.x", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utXImporter, importBCNEpileptic) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/BCN_Epileptic.X", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utXImporter, importFromTrueSpaceBin32) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/fromtruespace_bin32.x", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utXImporter, import_kwxport_test_cubewithvcolors) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/kwxport_test_cubewithvcolors.x", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utXImporter, importTestCubeBinary) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/test_cube_binary.x", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utXImporter, importTestCubeCompressed) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/test_cube_compressed.x", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utXImporter, importTestCubeText) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/test_cube_text.x", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utXImporter, importTestWuson) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/Testwuson.X", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utXImporter, TestFormatDetection) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/X/TestFormatDetection", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utXImporter, importDwarf) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/X/dwarf.x", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}
