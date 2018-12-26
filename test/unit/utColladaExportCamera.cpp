/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team



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
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

TEST_F(ColladaExportCamera, testExportCamera)
{
    const char* file = "cameraExp.dae";

    const aiScene* pTest = im->ReadFile(ASSIMP_TEST_MODELS_DIR "/Collada/cameras.dae", aiProcess_ValidateDataStructure);
    ASSERT_TRUE(pTest!=NULL);
    ASSERT_TRUE(pTest->HasCameras());


    EXPECT_EQ(AI_SUCCESS,ex->Export(pTest,"collada",file));
    const unsigned int origNumCams( pTest->mNumCameras );
    std::unique_ptr<float[]> origFOV( new float[ origNumCams ] );
    std::unique_ptr<float[]> orifClipPlaneNear( new float[ origNumCams ] );
    std::unique_ptr<float[]> orifClipPlaneFar( new float[ origNumCams ] );
    std::unique_ptr<aiString[]> names( new aiString[ origNumCams ] );
    std::unique_ptr<aiVector3D[]> pos( new aiVector3D[ origNumCams ] );
    for (size_t i = 0; i < origNumCams; i++) {
        const aiCamera *orig = pTest->mCameras[ i ];
        ASSERT_TRUE( orig != nullptr );

        origFOV[ i ] = orig->mHorizontalFOV;
        orifClipPlaneNear[ i ] = orig->mClipPlaneNear;
        orifClipPlaneFar[ i ] = orig->mClipPlaneFar;
        names[ i ] = orig->mName;
        pos[ i ] = orig->mPosition;
    }
    const aiScene* imported = im->ReadFile(file, aiProcess_ValidateDataStructure);

    ASSERT_TRUE(imported!=NULL);

    EXPECT_TRUE( imported->HasCameras() );
    EXPECT_EQ( origNumCams, imported->mNumCameras );

    for(size_t i=0; i< imported->mNumCameras;i++){
        const aiCamera *read = imported->mCameras[ i ];

        EXPECT_TRUE( names[ i ] == read->mName );
        EXPECT_NEAR( origFOV[ i ],read->mHorizontalFOV, 0.0001f );
        EXPECT_FLOAT_EQ( orifClipPlaneNear[ i ], read->mClipPlaneNear);
        EXPECT_FLOAT_EQ( orifClipPlaneFar[ i ], read->mClipPlaneFar);

        EXPECT_FLOAT_EQ( pos[ i ].x,read->mPosition.x);
        EXPECT_FLOAT_EQ( pos[ i ].y,read->mPosition.y);
        EXPECT_FLOAT_EQ( pos[ i ].z,read->mPosition.z);
    }
}

#endif // ASSIMP_BUILD_NO_EXPORT


