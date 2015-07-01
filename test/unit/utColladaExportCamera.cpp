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

class ColladaExportCamera : public ::testing::Test {
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
TEST_F(ColladaExportCamera, testExportCamera)
{
    const char* file = "cameraExp.dae";

    const aiScene* pTest = im->ReadFile(ASSIMP_TEST_MODELS_DIR "/Collada/cameras.dae",0);
    ASSERT_TRUE(pTest!=NULL);
    ASSERT_TRUE(pTest->HasCameras());


    EXPECT_EQ(AI_SUCCESS,ex->Export(pTest,"collada",file));

    const aiScene* imported = im->ReadFile(file,0);

    ASSERT_TRUE(imported!=NULL);

    EXPECT_TRUE(imported->HasCameras());
    EXPECT_EQ(pTest->mNumCameras,imported->mNumCameras);

    for(size_t i=0; i< pTest->mNumCameras;i++){

        const aiCamera *orig = pTest->mCameras[i];
        const aiCamera *read = imported->mCameras[i];

        EXPECT_TRUE(orig->mName==read->mName);
        EXPECT_FLOAT_EQ(orig->mHorizontalFOV,read->mHorizontalFOV);
        EXPECT_FLOAT_EQ(orig->mClipPlaneNear,read->mClipPlaneNear);
        EXPECT_FLOAT_EQ(orig->mClipPlaneFar,read->mClipPlaneFar);

        EXPECT_FLOAT_EQ(orig->mPosition.x,read->mPosition.x);
        EXPECT_FLOAT_EQ(orig->mPosition.y,read->mPosition.y);
        EXPECT_FLOAT_EQ(orig->mPosition.z,read->mPosition.z);
    }

}


#endif


