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
#include <assimp/Importer.hpp>

#include "MDL/MDLHL1TestFiles.h"

using namespace Assimp;

class utMDLImporter : public AbstractImportExportBase {
public:
    virtual bool importerTest() {

        Assimp::Importer importer;
        importerTest_HL1(&importer);

        // Add further MDL tests...

        return true;
    }

private:
    void importerTest_HL1(Assimp::Importer *const importer) {
        const aiScene *scene = importer->ReadFile(MDL_HL1_FILE_MAN, 0);
        EXPECT_NE(nullptr, scene);

        // Test that the importer can directly load an HL1 MDL external texture file.
        scene = importer->ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "manT.mdl", aiProcess_ValidateDataStructure);
        EXPECT_NE(nullptr, scene);
        EXPECT_NE(0u, scene->mNumTextures);
        EXPECT_NE(0u, scene->mNumMaterials);
    }
};

TEST_F(utMDLImporter, importMDLFromFileTest) {
    EXPECT_TRUE(importerTest());
}

#ifndef ASSIMP_BUILD_NO_MDL_IMPORTER
// Regression test for OSS-Fuzz issue #6793:
// CreateTextureARGB8_3DGS_MDL3() computed skinwidth * skinheight as signed int32,
// triggering UBSan integer-overflow when the product exceeds INT32_MAX.
// After the fix the importer rejects the file before reaching the multiply.
TEST_F(utMDLImporter, Quake1MDL3SkinDimensionOverflow) {
    // Minimal Quake 1 MDL (ident="IDPO", version=6) with a group-skin whose
    // skinwidth * skinheight = 50000 * 43000 = 2,150,000,000 > INT32_MAX.
    // The importer must reject the file (return nullptr) rather than causing a
    // signed-integer overflow UBSan crash.
    static const unsigned char kMalformedMDL3[] = {
        // Header: ident="IDPO"
        0x49, 0x44, 0x50, 0x4F,
        // version=6
        0x06, 0x00, 0x00, 0x00,
        // scale[3]=1.0,1.0,1.0
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F,
        // translate[3]=0.0,0.0,0.0
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // boundingradius=1.0
        0x00, 0x00, 0x80, 0x3F,
        // vEyePos[3]=0.0,0.0,0.0
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // num_skins=1, skinwidth=50000 (0xC350), skinheight=43000 (0xA7F8)
        0x01, 0x00, 0x00, 0x00, 0x50, 0xC3, 0x00, 0x00, 0xF8, 0xA7, 0x00, 0x00,
        // num_verts=3, num_tris=1, num_frames=1, synctype=0, flags=0
        0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Skin data: group=1 (group skin), nb=1 (one image)
        0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        // 1 float of group skin timing, then pixel stub (all zeros)
        0xCD, 0xCC, 0xCC, 0x3D, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(
        kMalformedMDL3, sizeof(kMalformedMDL3), 0, "mdl");
    EXPECT_EQ(nullptr, scene);
}
#endif // ASSIMP_BUILD_NO_MDL_IMPORTER
