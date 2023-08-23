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
#include <assimp/scene.h>

using namespace Assimp;

class utASEImportExport : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/ThreeCubesGreen.ASE", aiProcess_ValidateDataStructure);
#ifndef ASSIMP_BUILD_NO_3DS_IMPORTER
        return nullptr != scene;
#else
        return nullptr == scene;
#endif // ASSIMP_BUILD_NO_3DS_IMPORTER
    }
};

TEST_F(utASEImportExport, importACFromFileTest) {
    EXPECT_TRUE(importerTest());
}


TEST_F(utASEImportExport, importAnim1) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/anim.ASE", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}


TEST_F(utASEImportExport, importAnim2) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/anim2.ASE", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}


TEST_F(utASEImportExport, importCameraRollAnim) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/CameraRollAnim.ase", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}


TEST_F(utASEImportExport, importMotionCaptureROM) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/MotionCaptureROM.ase", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}


TEST_F(utASEImportExport, importRotatingCube) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/RotatingCube.ASE", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}


TEST_F(utASEImportExport, importTargetCameraAnim) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/TargetCameraAnim.ase", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}


TEST_F(utASEImportExport, importTestFormatDetection) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/TestFormatDetection", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}


TEST_F(utASEImportExport, importThreeCubesGreen) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/ThreeCubesGreen.ASE", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);

    ::Assimp::Importer importerLE;
    const aiScene *sceneLE = importerLE.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/ThreeCubesGreen_UTF16LE.ASE", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, sceneLE);

    ::Assimp::Importer importerBE;
    const aiScene *sceneBE = importerBE.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/ThreeCubesGreen_UTF16BE.ASE", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, sceneBE);

    // TODO: these scenes should probably be identical
    // verify that is the case and then add tests to check it
}


TEST_F(utASEImportExport, importUVTransform_Normal) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/TestUVTransform/UVTransform_Normal.ASE", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}


TEST_F(utASEImportExport, importUVTransform_ScaleUV1_2_OffsetUV0_0_9_Rotate_72_mirrorU) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/TestUVTransform/UVTransform_ScaleUV1-2_OffsetUV0-0.9_Rotate-72_mirrorU.ase", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}


TEST_F(utASEImportExport, importUVTransform_ScaleUV2x) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/TestUVTransform/UVTransform_ScaleUV2x.ASE", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}


TEST_F(utASEImportExport, importUVTransform_ScaleUV2x_Rotate45) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/ASE/TestUVTransform/UVTransform_ScaleUV2x_Rotate45.ASE", aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, scene);
}
