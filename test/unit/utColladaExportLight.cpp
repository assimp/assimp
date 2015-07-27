/*
 * ColladaCameraExporter.cpp
 *
 *  Created on: May 17, 2015
 *      Author: wise
 */


#include "UnitTestPCH.h"

#include <assimp/cexport.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#ifndef ASSIMP_BUILD_NO_EXPORT

class ColladaExportLight : public ::testing::Test {
public:

    virtual void SetUp()
    {
        ex = new Assimp::Exporter();
        im = new Assimp::Importer();

    }

    virtual void TearDown()
    {
        delete ex;
        delete im;
    }

protected:


    Assimp::Exporter* ex;
    Assimp::Importer* im;
};

// ------------------------------------------------------------------------------------------------
TEST_F(ColladaExportLight, testExportLight)
{
    const char* file = "cameraExp.dae";

    const aiScene* pTest = im->ReadFile(ASSIMP_TEST_MODELS_DIR "/Collada/lights.dae",0);
    ASSERT_TRUE(pTest!=NULL);
    ASSERT_TRUE(pTest->HasLights());


    EXPECT_EQ(AI_SUCCESS,ex->Export(pTest,"collada",file));
    EXPECT_EQ(AI_SUCCESS,ex->Export(pTest,"collada","lightsExp.dae"));

    const aiScene* imported = im->ReadFile(file,0);

    ASSERT_TRUE(imported!=NULL);

    EXPECT_TRUE(imported->HasLights());
    EXPECT_EQ(pTest->mNumLights,imported->mNumLights);

    for(size_t i=0; i< pTest->mNumLights;i++){

        const aiLight *orig = pTest->mLights[i];
        const aiLight *read = imported->mLights[i];

        EXPECT_TRUE(orig->mName==read->mName);
        EXPECT_EQ(orig->mType,read->mType);
        EXPECT_FLOAT_EQ(orig->mAttenuationConstant,read->mAttenuationConstant);
        EXPECT_FLOAT_EQ(orig->mAttenuationLinear,read->mAttenuationLinear);
        EXPECT_FLOAT_EQ(orig->mAttenuationQuadratic,read->mAttenuationQuadratic);

        EXPECT_FLOAT_EQ(orig->mColorAmbient.r,read->mColorAmbient.r);
        EXPECT_FLOAT_EQ(orig->mColorAmbient.g,read->mColorAmbient.g);
        EXPECT_FLOAT_EQ(orig->mColorAmbient.b,read->mColorAmbient.b);

        EXPECT_FLOAT_EQ(orig->mColorDiffuse.r,read->mColorDiffuse.r);
        EXPECT_FLOAT_EQ(orig->mColorDiffuse.g,read->mColorDiffuse.g);
        EXPECT_FLOAT_EQ(orig->mColorDiffuse.b,read->mColorDiffuse.b);

        EXPECT_FLOAT_EQ(orig->mColorSpecular.r,read->mColorSpecular.r);
        EXPECT_FLOAT_EQ(orig->mColorSpecular.g,read->mColorSpecular.g);
        EXPECT_FLOAT_EQ(orig->mColorSpecular.b,read->mColorSpecular.b);

        EXPECT_NEAR(orig->mAngleInnerCone,read->mAngleInnerCone,0.001);
        EXPECT_NEAR(orig->mAngleOuterCone,read->mAngleOuterCone,0.001);
    }
}


#endif


