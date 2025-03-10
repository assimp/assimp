/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>

using namespace Assimp;

#ifndef ASSIMP_BUILD_NO_EXPORT

class utAssjsonImportExport : public AbstractImportExportBase {
public:
    bool exporterTest() override {
        Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/spider.obj", aiProcess_ValidateDataStructure);

        Exporter exporter;
        const char *testFileName = "./spider_test.json";
        aiReturn res = exporter.Export(scene, "assjson", testFileName);
        if (aiReturn_SUCCESS != res) {
            return false;
        }

        Assimp::ExportProperties exportProperties;
        exportProperties.SetPropertyBool("JSON_SKIP_WHITESPACES", true);
        const char *testNoWhitespaceFileName = "./spider_test_nowhitespace.json";
        aiReturn resNoWhitespace = exporter.Export(scene, "assjson", testNoWhitespaceFileName, 0u, &exportProperties);
        if (aiReturn_SUCCESS != resNoWhitespace) {
            return false;
        }

        // Cleanup, remove generated json
        if (0 != std::remove(testFileName)) {
            return false;
        }

        if (0 != std::remove(testNoWhitespaceFileName)) {
            return false;
        }

        return true;
    }
};

TEST_F(utAssjsonImportExport, exportTest) {
    EXPECT_TRUE(exporterTest());
}

#endif // ASSIMP_BUILD_NO_EXPORT
