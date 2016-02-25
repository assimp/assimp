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

#include <FindDegenerates.h>


using namespace std;
using namespace Assimp;

class FindDegeneratesProcessTest : public ::testing::Test
{
public:

    virtual void SetUp();
    virtual void TearDown();

protected:

    aiMesh* mesh;
    FindDegeneratesProcess* process;
};

// ------------------------------------------------------------------------------------------------
void FindDegeneratesProcessTest::SetUp()
{
    mesh = new aiMesh();
    process = new FindDegeneratesProcess();

    mesh->mNumFaces = 1000;
    mesh->mFaces = new aiFace[1000];

    mesh->mNumVertices = 5000*2;
    mesh->mVertices = new aiVector3D[5000*2];

    for (unsigned int i = 0; i < 5000; ++i) {
        mesh->mVertices[i] = mesh->mVertices[i+5000] = aiVector3D((float)i);
    }

    mesh->mPrimitiveTypes = aiPrimitiveType_LINE | aiPrimitiveType_POINT |
    aiPrimitiveType_POLYGON | aiPrimitiveType_TRIANGLE;

    unsigned int numOut = 0, numFaces = 0;
    for (unsigned int i = 0; i < 1000; ++i) {
        aiFace& f = mesh->mFaces[i];
    f.mNumIndices = (i % 5)+1; // between 1 and 5
    f.mIndices = new unsigned int[f.mNumIndices];
    bool had = false;
    for (unsigned int n = 0; n < f.mNumIndices;++n) {
        // FIXME
#if 0
        // some duplicate indices
        if ( n && n == (i / 200)+1) {
            f.mIndices[n] = f.mIndices[n-1];
            had = true;
        }
        // and some duplicate vertices
#endif
        if (n && i % 2 && 0 == n % 2) {
            f.mIndices[n] = f.mIndices[n-1]+5000;
            had = true;
        }
        else {
            f.mIndices[n] = numOut++;
            }
        }
        if (!had)
            ++numFaces;
    }
    mesh->mNumUVComponents[0] = numOut;
    mesh->mNumUVComponents[1] = numFaces;
}

// ------------------------------------------------------------------------------------------------
void FindDegeneratesProcessTest::TearDown()
{
    delete mesh;
    delete process;
}

// ------------------------------------------------------------------------------------------------
TEST_F(FindDegeneratesProcessTest, testDegeneratesDetection)
{
    process->EnableInstantRemoval(false);
    process->ExecuteOnMesh(mesh);

    unsigned int out = 0;
    for (unsigned int i = 0; i < 1000; ++i) {
        aiFace& f = mesh->mFaces[i];
        out += f.mNumIndices;
    }

    EXPECT_EQ(1000U, mesh->mNumFaces);
    EXPECT_EQ(10000U, mesh->mNumVertices);
    EXPECT_EQ(out, mesh->mNumUVComponents[0]);
    EXPECT_EQ(static_cast<unsigned int>(
                  aiPrimitiveType_LINE | aiPrimitiveType_POINT |
                  aiPrimitiveType_POLYGON | aiPrimitiveType_TRIANGLE),
              mesh->mPrimitiveTypes);
}

// ------------------------------------------------------------------------------------------------
TEST_F(FindDegeneratesProcessTest, testDegeneratesRemoval)
{
    process->EnableInstantRemoval(true);
    process->ExecuteOnMesh(mesh);

    EXPECT_EQ(mesh->mNumUVComponents[1], mesh->mNumFaces);
}

