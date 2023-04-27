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

#include "AbstractImportExportBase.h"
#include "UnitTestPCH.h"

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

class ut3DSImportExport : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3DS/fels.3ds", aiProcess_ValidateDataStructure);
#ifndef ASSIMP_BUILD_NO_3DS_IMPORTER
        return nullptr != scene;
#else
        return nullptr == scene;
#endif // ASSIMP_BUILD_NO_3DS_IMPORTER
    }
};

TEST_F(ut3DSImportExport, import3DSFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(ut3DSImportExport, import3DSformatdetection) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3DS/testFormatDetection", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importCameraRollAnim) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3DS/CameraRollAnim.3ds", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importCameraRollAnimWithChildObject) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3DS/CameraRollAnimWithChildObject.3ds", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importCubesWithAlpha) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3DS/cubes_with_alpha.3DS", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importCubeWithDiffuseTexture) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3DS/cube_with_diffuse_texture.3DS", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importCubeWithSpecularTexture) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3DS/cube_with_specular_texture.3DS", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importRotatingCube) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3DS/RotatingCube.3DS", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importTargetCameraAnim) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3DS/TargetCameraAnim.3ds", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importTest1) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/3DS/test1.3ds", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importCartWheel) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/3DS/cart_wheel.3DS", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importGranate) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/3DS/Granate.3DS", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importJeep1) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/3DS/jeep1.3ds", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importMarRifle) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/3DS/mar_rifle.3ds", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importMp5Sil) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/3DS/mp5_sil.3ds", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(ut3DSImportExport, importPyramob) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/3DS/pyramob.3DS", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}
