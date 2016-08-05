/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team

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

#include <FindInvalidDataProcess.h>
#include "../../include/assimp/mesh.h"


using namespace std;
using namespace Assimp;

class FindInvalidDataProcessTest : public ::testing::Test
{
public:
    virtual void SetUp();
    virtual void TearDown();

protected:
    aiMesh* pcMesh;
    FindInvalidDataProcess* piProcess;
};

// ------------------------------------------------------------------------------------------------
void FindInvalidDataProcessTest::SetUp()
{
    ASSERT_TRUE( AI_MAX_NUMBER_OF_TEXTURECOORDS >= 3);

    piProcess = new FindInvalidDataProcess();
    pcMesh = new aiMesh();

    pcMesh->mNumVertices = 1000;
    pcMesh->mVertices = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000;++i)
        pcMesh->mVertices[i] = aiVector3D((float)i);

    pcMesh->mNormals = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000;++i)
        pcMesh->mNormals[i] = aiVector3D((float)i+1);

    pcMesh->mTangents = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000;++i)
        pcMesh->mTangents[i] = aiVector3D((float)i);

    pcMesh->mBitangents = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000;++i)
        pcMesh->mBitangents[i] = aiVector3D((float)i);

    for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS;++a)
    {
        pcMesh->mTextureCoords[a] = new aiVector3D[1000];
        for (unsigned int i = 0; i < 1000;++i)
            pcMesh->mTextureCoords[a][i] = aiVector3D((float)i);
    }
}

// ------------------------------------------------------------------------------------------------
void FindInvalidDataProcessTest::TearDown()
{
    delete piProcess;
    delete pcMesh;
}

// ------------------------------------------------------------------------------------------------
TEST_F(FindInvalidDataProcessTest, testStepNegativeResult)
{
    ::memset(pcMesh->mNormals,0,pcMesh->mNumVertices*sizeof(aiVector3D));
    ::memset(pcMesh->mBitangents,0,pcMesh->mNumVertices*sizeof(aiVector3D));

    pcMesh->mTextureCoords[2][455] = aiVector3D( std::numeric_limits<float>::quiet_NaN() );

    piProcess->ProcessMesh(pcMesh);

    EXPECT_TRUE(NULL != pcMesh->mVertices);
    EXPECT_TRUE(NULL == pcMesh->mNormals);
    EXPECT_TRUE(NULL == pcMesh->mTangents);
    EXPECT_TRUE(NULL == pcMesh->mBitangents);

    for (unsigned int i = 0; i < 2;++i)
        EXPECT_TRUE(NULL != pcMesh->mTextureCoords[i]);

    for (unsigned int i = 2; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
        EXPECT_TRUE(NULL == pcMesh->mTextureCoords[i]);
}

// ------------------------------------------------------------------------------------------------
TEST_F(FindInvalidDataProcessTest, testStepPositiveResult)
{
    piProcess->ProcessMesh(pcMesh);

    EXPECT_TRUE(NULL != pcMesh->mVertices);

    EXPECT_TRUE(NULL != pcMesh->mNormals);
    EXPECT_TRUE(NULL != pcMesh->mTangents);
    EXPECT_TRUE(NULL != pcMesh->mBitangents);

    for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
        EXPECT_TRUE(NULL != pcMesh->mTextureCoords[i]);
}
