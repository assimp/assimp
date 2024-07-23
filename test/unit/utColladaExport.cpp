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
#include "UnitTestPCH.h"

#include <assimp/cexport.h>
#include <assimp/commonMetaData.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>

#include <array>

#ifndef ASSIMP_BUILD_NO_EXPORT

class utColladaExport : public ::testing::Test {
public:
    void SetUp() override {
        ex = new Assimp::Exporter();
        im = new Assimp::Importer();
    }

    void TearDown() override {
        delete ex;
        ex = nullptr;
        delete im;
        im = nullptr;
    }

protected:
    Assimp::Exporter *ex;
    Assimp::Importer *im;
};

TEST_F(utColladaExport, testExportCamera) {
    const char *file = "cameraExp.dae";

    const aiScene *pTest = im->ReadFile(ASSIMP_TEST_MODELS_DIR "/Collada/cameras.dae", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, pTest);
    ASSERT_TRUE(pTest->HasCameras());

    EXPECT_EQ(AI_SUCCESS, ex->Export(pTest, "collada", file));
    const unsigned int origNumCams(pTest->mNumCameras);
    //std::vector<float> origFOV;
    std::unique_ptr<float[]> origFOV(new float[origNumCams]);
    std::unique_ptr<float[]> orifClipPlaneNear(new float[origNumCams]);
    std::unique_ptr<float[]> orifClipPlaneFar(new float[origNumCams]);
    std::unique_ptr<aiString[]> names(new aiString[origNumCams]);
    std::unique_ptr<aiVector3D[]> pos(new aiVector3D[origNumCams]);
    for (size_t i = 0; i < origNumCams; i++) {
        const aiCamera *orig = pTest->mCameras[i];
        ASSERT_NE(nullptr, orig);

        origFOV[i] = orig->mHorizontalFOV;
        orifClipPlaneNear[i] = orig->mClipPlaneNear;
        orifClipPlaneFar[i] = orig->mClipPlaneFar;
        names[i] = orig->mName;
        pos[i] = orig->mPosition;
    }
    const aiScene *imported = im->ReadFile(file, aiProcess_ValidateDataStructure);

    ASSERT_NE(nullptr, imported);

    EXPECT_TRUE(imported->HasCameras());
    EXPECT_EQ(origNumCams, imported->mNumCameras);

    for (size_t i = 0; i < imported->mNumCameras; i++) {
        const aiCamera *read = imported->mCameras[i];

        EXPECT_TRUE(names[i] == read->mName);
        EXPECT_NEAR(origFOV[i], read->mHorizontalFOV, 0.0001f);
        EXPECT_FLOAT_EQ(orifClipPlaneNear[i], read->mClipPlaneNear);
        EXPECT_FLOAT_EQ(orifClipPlaneFar[i], read->mClipPlaneFar);

        EXPECT_FLOAT_EQ(pos[i].x, read->mPosition.x);
        EXPECT_FLOAT_EQ(pos[i].y, read->mPosition.y);
        EXPECT_FLOAT_EQ(pos[i].z, read->mPosition.z);
    }
}

// ------------------------------------------------------------------------------------------------
TEST_F(utColladaExport, testExportLight) {
    const char *file = "lightsExp.dae";

    const aiScene *pTest = im->ReadFile(ASSIMP_TEST_MODELS_DIR "/Collada/lights.dae", aiProcess_ValidateDataStructure);
    ASSERT_NE(pTest, nullptr);
    ASSERT_TRUE(pTest->HasLights());

    const unsigned int origNumLights = pTest->mNumLights;
    // There are FIVE!!! LIGHTS!!!
    EXPECT_EQ(5u, origNumLights) << "lights.dae should contain five lights";

    std::vector<aiLight> origLights(5);
    for (size_t i = 0; i < origNumLights; i++) {
        origLights[i] = *(pTest->mLights[i]);
    }

    // Check loaded first light properly
    EXPECT_STREQ("Lamp", origLights[0].mName.C_Str());
    EXPECT_EQ(aiLightSource_POINT, origLights[0].mType);
    EXPECT_FLOAT_EQ(1.0f, origLights[0].mAttenuationConstant);
    EXPECT_FLOAT_EQ(0.0f, origLights[0].mAttenuationLinear);
    EXPECT_FLOAT_EQ(0.00111109f, origLights[0].mAttenuationQuadratic);

    // Common metadata
    // Confirm was loaded by the Collada importer
    aiString origImporter;
    EXPECT_TRUE(pTest->mMetaData->Get(AI_METADATA_SOURCE_FORMAT, origImporter)) << "No importer format metadata";
    EXPECT_STREQ("Collada Importer", origImporter.C_Str());

    aiString origGenerator;
    EXPECT_TRUE(pTest->mMetaData->Get(AI_METADATA_SOURCE_GENERATOR, origGenerator)) << "No generator metadata";
    EXPECT_EQ(strncmp(origGenerator.C_Str(), "Blender", 7), 0) << "AI_METADATA_SOURCE_GENERATOR was: " << origGenerator.C_Str();

    aiString origCopyright;
    EXPECT_TRUE(pTest->mMetaData->Get(AI_METADATA_SOURCE_COPYRIGHT, origCopyright)) << "No copyright metadata";
    EXPECT_STREQ("BSD", origCopyright.C_Str());

    aiString origCreated;
    EXPECT_TRUE(pTest->mMetaData->Get("Created", origCreated)) << "No created metadata";
    EXPECT_STREQ("2015-05-17T21:55:44", origCreated.C_Str());

    aiString origModified;
    EXPECT_TRUE(pTest->mMetaData->Get("Modified", origModified)) << "No modified metadata";
    EXPECT_STREQ("2015-05-17T21:55:44", origModified.C_Str());

    EXPECT_EQ(AI_SUCCESS, ex->Export(pTest, "collada", file));

    // Drop the pointer as about to become invalid
    pTest = nullptr;

    const aiScene *imported = im->ReadFile(file, aiProcess_ValidateDataStructure);

    ASSERT_TRUE(imported != nullptr);

    // Check common metadata survived roundtrip
    aiString readImporter;
    EXPECT_TRUE(imported->mMetaData->Get(AI_METADATA_SOURCE_FORMAT, readImporter)) << "No importer format metadata after export";
    EXPECT_STREQ(origImporter.C_Str(), readImporter.C_Str()) << "Assimp Importer Format changed";

    aiString readGenerator;
    EXPECT_TRUE(imported->mMetaData->Get(AI_METADATA_SOURCE_GENERATOR, readGenerator)) << "No generator metadata";
    EXPECT_STREQ(origGenerator.C_Str(), readGenerator.C_Str()) << "Generator changed";

    aiString readCopyright;
    EXPECT_TRUE(imported->mMetaData->Get(AI_METADATA_SOURCE_COPYRIGHT, readCopyright)) << "No copyright metadata";
    EXPECT_STREQ(origCopyright.C_Str(), readCopyright.C_Str()) << "Copyright changed";

    aiString readCreated;
    EXPECT_TRUE(imported->mMetaData->Get("Created", readCreated)) << "No created metadata";
    EXPECT_STREQ(origCreated.C_Str(), readCreated.C_Str()) << "Created date changed";

    aiString readModified;
    EXPECT_TRUE(imported->mMetaData->Get("Modified", readModified)) << "No modified metadata";
    EXPECT_STRNE(origModified.C_Str(), readModified.C_Str()) << "Modified date did not change";
    EXPECT_GT(readModified.length, ai_uint32(18)) << "Modified date too short";

    // Lights
    EXPECT_TRUE(imported->HasLights());
    EXPECT_EQ(origNumLights, imported->mNumLights);
    for (size_t i = 0; i < origNumLights; i++) {
        const aiLight *orig = &origLights[i];
        const aiLight *read = imported->mLights[i];
        EXPECT_EQ(0, strcmp(orig->mName.C_Str(), read->mName.C_Str()));
        EXPECT_EQ(orig->mType, read->mType);
        EXPECT_FLOAT_EQ(orig->mAttenuationConstant, read->mAttenuationConstant);
        EXPECT_FLOAT_EQ(orig->mAttenuationLinear, read->mAttenuationLinear);
        EXPECT_NEAR(orig->mAttenuationQuadratic, read->mAttenuationQuadratic, 0.001f);

        EXPECT_FLOAT_EQ(orig->mColorAmbient.r, read->mColorAmbient.r);
        EXPECT_FLOAT_EQ(orig->mColorAmbient.g, read->mColorAmbient.g);
        EXPECT_FLOAT_EQ(orig->mColorAmbient.b, read->mColorAmbient.b);

        EXPECT_FLOAT_EQ(orig->mColorDiffuse.r, read->mColorDiffuse.r);
        EXPECT_FLOAT_EQ(orig->mColorDiffuse.g, read->mColorDiffuse.g);
        EXPECT_FLOAT_EQ(orig->mColorDiffuse.b, read->mColorDiffuse.b);

        EXPECT_FLOAT_EQ(orig->mColorSpecular.r, read->mColorSpecular.r);
        EXPECT_FLOAT_EQ(orig->mColorSpecular.g, read->mColorSpecular.g);
        EXPECT_FLOAT_EQ(orig->mColorSpecular.b, read->mColorSpecular.b);

        EXPECT_NEAR(orig->mAngleInnerCone, read->mAngleInnerCone, 0.001);
        EXPECT_NEAR(orig->mAngleOuterCone, read->mAngleOuterCone, 0.001);
    }
}

#endif // ASSIMP_BUILD_NO_EXPORT
