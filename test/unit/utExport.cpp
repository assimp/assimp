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
#include "UnitTestPCH.h"
#include <assimp/cexport.h>
#include <assimp/Exporter.hpp>

#ifndef ASSIMP_BUILD_NO_EXPORT

class ExporterTest : public ::testing::Test {
public:
    void SetUp() override {
        ex = new Assimp::Exporter();
        im = new Assimp::Importer();

        pTest = im->ReadFile(ASSIMP_TEST_MODELS_DIR "/X/test.x", aiProcess_ValidateDataStructure);
    }

    void TearDown() override {
        delete ex;
        delete im;
    }

protected:
    const aiScene* pTest;
    Assimp::Exporter* ex;
    Assimp::Importer* im;
};

// ------------------------------------------------------------------------------------------------
TEST_F(ExporterTest, testExportToFile) {
    const char* file = "unittest_output.dae";
    EXPECT_EQ(AI_SUCCESS,ex->Export(pTest,"collada",file));

    // check if we can read it again
    EXPECT_TRUE(im->ReadFile(file, aiProcess_ValidateDataStructure));
}

// ------------------------------------------------------------------------------------------------
TEST_F(ExporterTest, testExportToBlob) {
    const aiExportDataBlob* blob = ex->ExportToBlob(pTest,"collada");
    ASSERT_TRUE(blob);
    EXPECT_TRUE(blob->data);
    EXPECT_GT(blob->size,  0U);
    EXPECT_EQ(0U, blob->name.length);

    // XXX test chained blobs (i.e. obj file with accompanying mtl script)

    // check if we can read it again
    EXPECT_TRUE(im->ReadFileFromMemory(blob->data,blob->size,0,"dae"));
}

// ------------------------------------------------------------------------------------------------
TEST_F(ExporterTest, testCppExportInterface) {
    EXPECT_TRUE(ex->GetExportFormatCount() > 0);
    for(size_t i = 0; i < ex->GetExportFormatCount(); ++i) {
        const aiExportFormatDesc* const desc = ex->GetExportFormatDescription(i);
        ASSERT_TRUE(desc);
        EXPECT_TRUE(desc->description && strlen(desc->description));
        EXPECT_TRUE(desc->fileExtension && strlen(desc->fileExtension));
        EXPECT_TRUE(desc->id && strlen(desc->id));
    }

    EXPECT_TRUE(ex->IsDefaultIOHandler());
}

// ------------------------------------------------------------------------------------------------
TEST_F(ExporterTest, testCExportInterface) {
    EXPECT_TRUE(aiGetExportFormatCount() > 0);
    for(size_t i = 0; i < aiGetExportFormatCount(); ++i) {
        const aiExportFormatDesc* const desc = aiGetExportFormatDescription(i);
        EXPECT_TRUE(desc);
    }
}

#endif

