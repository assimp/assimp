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
#include <GenVertexNormalsProcess.h>

using namespace ::std;
using namespace ::Assimp;

class GenNormalsTest : public ::testing::Test
{
public:
    virtual void SetUp();
    virtual void TearDown();

protected:
    aiMesh* pcMesh;
    GenVertexNormalsProcess* piProcess;
};

// ------------------------------------------------------------------------------------------------
void GenNormalsTest::SetUp()
{
    piProcess = new GenVertexNormalsProcess();
    pcMesh = new aiMesh();
    pcMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    pcMesh->mNumFaces = 1;
    pcMesh->mFaces = new aiFace[1];
    pcMesh->mFaces[0].mIndices = new unsigned int[pcMesh->mFaces[0].mNumIndices = 3];
    pcMesh->mFaces[0].mIndices[0] = 0;
    pcMesh->mFaces[0].mIndices[1] = 1;
    pcMesh->mFaces[0].mIndices[2] = 1;
    pcMesh->mNumVertices = 3;
    pcMesh->mVertices = new aiVector3D[3];
    pcMesh->mVertices[0] = aiVector3D(0.0f,1.0f,6.0f);
    pcMesh->mVertices[1] = aiVector3D(2.0f,3.0f,1.0f);
    pcMesh->mVertices[2] = aiVector3D(3.0f,2.0f,4.0f);
}

// ------------------------------------------------------------------------------------------------
void GenNormalsTest::TearDown()
{
    delete this->pcMesh;
    delete this->piProcess;
}

// ------------------------------------------------------------------------------------------------
TEST_F(GenNormalsTest, testSimpleTriangle)
{
    piProcess->GenMeshVertexNormals(pcMesh, 0);
    EXPECT_TRUE(pcMesh->mNormals != NULL);
}
