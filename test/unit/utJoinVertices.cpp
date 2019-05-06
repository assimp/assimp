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

#include <assimp/scene.h>
#include <JoinVerticesProcess.h>


using namespace std;
using namespace Assimp;

class JoinVerticesTest : public ::testing::Test
{
public:
    virtual void SetUp();
    virtual void TearDown();

protected:
    JoinVerticesProcess* piProcess;
    aiMesh* pcMesh;
};

// ------------------------------------------------------------------------------------------------
void JoinVerticesTest::SetUp()
{
    // construct the process
    piProcess = new JoinVerticesProcess();

    // create a quite small mesh for testing purposes -
    // the mesh itself is *something* but it has redundant vertices
    pcMesh = new aiMesh();

    pcMesh->mNumVertices = 900;
    aiVector3D*& pv = pcMesh->mVertices = new aiVector3D[900];
    for (unsigned int i = 0; i < 3;++i)
    {
        const unsigned int base = i*300;
        for (unsigned int a = 0; a < 300;++a)
        {
            pv[base+a].x = pv[base+a].y = pv[base+a].z = (float)a;
        }
    }

    // generate faces - each vertex is referenced once
    pcMesh->mNumFaces = 300;
    pcMesh->mFaces = new aiFace[300];
    for (unsigned int i = 0,p = 0; i < 300;++i)
    {
        aiFace& face = pcMesh->mFaces[i];
        face.mIndices = new unsigned int[ face.mNumIndices = 3 ];
        for (unsigned int a = 0; a < 3;++a)
            face.mIndices[a] = p++;
    }

    // generate extra members - set them to zero to make sure they're identical
    pcMesh->mTextureCoords[0] = new aiVector3D[900];
    for (unsigned int i = 0; i < 900;++i)pcMesh->mTextureCoords[0][i] = aiVector3D( 0.f );

    pcMesh->mNormals = new aiVector3D[900];
    for (unsigned int i = 0; i < 900;++i)pcMesh->mNormals[i] = aiVector3D( 0.f );

    pcMesh->mTangents = new aiVector3D[900];
    for (unsigned int i = 0; i < 900;++i)pcMesh->mTangents[i] = aiVector3D( 0.f );

    pcMesh->mBitangents = new aiVector3D[900];
    for (unsigned int i = 0; i < 900;++i)pcMesh->mBitangents[i] = aiVector3D( 0.f );
}

// ------------------------------------------------------------------------------------------------
void JoinVerticesTest::TearDown()
{
    delete this->pcMesh;
    delete this->piProcess;
}

// ------------------------------------------------------------------------------------------------
TEST_F(JoinVerticesTest, testProcess)
{
    // execute the step on the given data
    piProcess->ProcessMesh(pcMesh,0);

    // the number of faces shouldn't change
    ASSERT_EQ(300U, pcMesh->mNumFaces);
    ASSERT_EQ(300U, pcMesh->mNumVertices);

    ASSERT_TRUE(NULL != pcMesh->mNormals);
    ASSERT_TRUE(NULL != pcMesh->mTangents);
    ASSERT_TRUE(NULL != pcMesh->mBitangents);
    ASSERT_TRUE(NULL != pcMesh->mTextureCoords[0]);

    // the order doesn't care
    float fSum = 0.f;
    for (unsigned int i = 0; i < 300;++i)
    {
        aiVector3D& v = pcMesh->mVertices[i];
        fSum += v.x + v.y + v.z;

        EXPECT_FALSE(pcMesh->mNormals[i].x);
        EXPECT_FALSE(pcMesh->mTangents[i].x);
        EXPECT_FALSE(pcMesh->mBitangents[i].x);
        EXPECT_FALSE(pcMesh->mTextureCoords[0][i].x);
    }
    EXPECT_EQ(150.f*299.f*3.f, fSum); // gaussian sum equation
}

