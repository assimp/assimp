/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team

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

#include "PostProcessing/FindInvalidDataProcess.h"
#include <assimp/mesh.h>

using namespace std;
using namespace Assimp;

class utFindInvalidDataProcess : public ::testing::Test {
public:
    utFindInvalidDataProcess()
    : Test()
    , mMesh(nullptr)
    , mProcess(nullptr) {
        // empty
    }

protected:
    virtual void SetUp();
    virtual void TearDown();

protected:
    aiMesh* mMesh;
    FindInvalidDataProcess* mProcess;
};

// ------------------------------------------------------------------------------------------------
void utFindInvalidDataProcess::SetUp() {
    ASSERT_TRUE( AI_MAX_NUMBER_OF_TEXTURECOORDS >= 3);

    mProcess = new FindInvalidDataProcess();
    mMesh = new aiMesh();

    mMesh->mNumVertices = 1000;
    mMesh->mVertices = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000; ++i) {
        mMesh->mVertices[i] = aiVector3D((float)i);
    }

    mMesh->mNormals = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000; ++i) {
        mMesh->mNormals[i] = aiVector3D((float)i + 1);
    }

    mMesh->mTangents = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000; ++i) {
        mMesh->mTangents[i] = aiVector3D((float)i);
    }

    mMesh->mBitangents = new aiVector3D[1000];
    for (unsigned int i = 0; i < 1000; ++i) {
        mMesh->mBitangents[i] = aiVector3D((float)i);
    }

    for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS;++a) {
        mMesh->mTextureCoords[a] = new aiVector3D[1000];
        for (unsigned int i = 0; i < 1000; ++i) {
            mMesh->mTextureCoords[a][i] = aiVector3D((float)i);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void utFindInvalidDataProcess::TearDown() {
    delete mProcess;
    delete mMesh;
}

// ------------------------------------------------------------------------------------------------
TEST_F(utFindInvalidDataProcess, testStepNegativeResult) {
    ::memset(mMesh->mNormals, 0, mMesh->mNumVertices*sizeof(aiVector3D) );
    ::memset(mMesh->mBitangents, 0, mMesh->mNumVertices*sizeof(aiVector3D) );

    mMesh->mTextureCoords[2][455] = aiVector3D( std::numeric_limits<float>::quiet_NaN() );

    mProcess->ProcessMesh(mMesh);

    EXPECT_TRUE(NULL != mMesh->mVertices);
    EXPECT_EQ(NULL, mMesh->mNormals);
    EXPECT_EQ(NULL, mMesh->mTangents);
    EXPECT_EQ(NULL, mMesh->mBitangents);

    for (unsigned int i = 0; i < 2; ++i) {
        EXPECT_TRUE(NULL != mMesh->mTextureCoords[i]);
    }

    for (unsigned int i = 2; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
        EXPECT_EQ(NULL, mMesh->mTextureCoords[i]);
    }
}

// ------------------------------------------------------------------------------------------------
TEST_F(utFindInvalidDataProcess, testStepPositiveResult) {
    mProcess->ProcessMesh(mMesh);

    EXPECT_NE(nullptr, mMesh->mVertices);

    EXPECT_NE(nullptr, mMesh->mNormals);
    EXPECT_NE(nullptr, mMesh->mTangents);
    EXPECT_NE(nullptr, mMesh->mBitangents);

    for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
        EXPECT_NE(nullptr, mMesh->mTextureCoords[i]);
    }
}
