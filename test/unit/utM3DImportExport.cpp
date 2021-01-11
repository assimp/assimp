/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp tea


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
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

class utM3DImportExport : public AbstractImportExportBase {
public:
	bool importerTest() override  {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/M3D/cube_normals.m3d", aiProcess_ValidateDataStructure);
#ifndef ASSIMP_BUILD_NO_M3D_IMPORTER
		return nullptr != scene;
#else
        return nullptr == scene;
#endif // ASSIMP_BUILD_NO_M3D_IMPORTER
    }

#ifndef ASSIMP_BUILD_NO_EXPORT
    bool exporterTest() override {
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/M3D/cube_normals.m3d", aiProcess_ValidateDataStructure);
		Exporter exporter;
		aiReturn ret = exporter.Export(scene, "m3d", ASSIMP_TEST_MODELS_DIR "/M3D/cube_normals_out.m3d");
		return ret == AI_SUCCESS;
    }
#endif
};

TEST_F(utM3DImportExport, importM3DFromFileTest) {
    EXPECT_TRUE(importerTest());
}

#ifndef ASSIMP_BUILD_NO_EXPORT
TEST_F(utM3DImportExport, exportM3DFromFileTest) {
	EXPECT_TRUE(exporterTest());
}
#endif //  ASSIMP_BUILD_NO_EXPORT
