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

using namespace Assimp;

class utAMFImportExport : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test1.amf", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utAMFImportExport, importAMFFromFileTest) {
    EXPECT_TRUE(importerTest());
}


// TODO: test models-nonbsd/AMF/3_bananas.amf.7z
//       requires uncompressing it in memory and we don't currently have 7z decompressor


TEST_F(utAMFImportExport, importTest2) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test2.amf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utAMFImportExport, importTest3) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test3.amf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utAMFImportExport, importTest4) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test4.amf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utAMFImportExport, importTest5) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test5.amf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utAMFImportExport, importTest5a) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test5a.amf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utAMFImportExport, importTest6) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test6.amf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utAMFImportExport, importTest7) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test7.amf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


#if 0
// FIXME: these tests are disabled because they leak memory in AMFImporter_Postprocess.cpp


TEST_F(utAMFImportExport, importTest8) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test8.amf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utAMFImportExport, importTest9) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test9.amf", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


#endif  // 0


TEST_F(utAMFImportExport, importAMFWithMatFromFileTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/AMF/test_with_mat.amf", aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
}
