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

#include <assimp/types.h>
#include <assimp/mesh.h>

#include "Common/VertexTriangleAdjacency.h"

using namespace std;
using namespace Assimp;

class VTAdjacencyTest : public ::testing::Test {
protected:
    void checkMesh(const aiMesh& mesh);
};

// ------------------------------------------------------------------------------------------------
TEST_F(VTAdjacencyTest, largeRandomDataSet)
{
    // build a test mesh with randomized input data
    // *******************************************************************************
    aiMesh mesh;

    mesh.mNumVertices = 500;
    mesh.mNumFaces = 600;

    mesh.mFaces = new aiFace[600];
    unsigned int iCurrent = 0;
    for (unsigned int i = 0; i < 600;++i)
    {
        aiFace& face = mesh.mFaces[i];
        face.mNumIndices = 3;
        face.mIndices = new unsigned int[3];

        if (499 == iCurrent)iCurrent = 0;
        face.mIndices[0] = iCurrent++;


        while(face.mIndices[0] == ( face.mIndices[1] = (unsigned int)(((float)rand()/RAND_MAX)*499)));
        while(face.mIndices[0] == ( face.mIndices[2] = (unsigned int)(((float)rand()/RAND_MAX)*499)) ||
            face.mIndices[1] == face.mIndices[2]);
    }

    checkMesh(mesh);
}

// ------------------------------------------------------------------------------------------------
TEST_F(VTAdjacencyTest, smallDataSet)
{

    // build a test mesh - this one is extremely small
    // *******************************************************************************
    aiMesh mesh;

    mesh.mNumVertices = 5;
    mesh.mNumFaces = 3;

    mesh.mFaces = new aiFace[3];
    mesh.mFaces[0].mIndices = new unsigned int[3];
    mesh.mFaces[0].mNumIndices = 3;
    mesh.mFaces[1].mIndices = new unsigned int[3];
    mesh.mFaces[1].mNumIndices = 3;
    mesh.mFaces[2].mIndices = new unsigned int[3];
    mesh.mFaces[2].mNumIndices = 3;

    mesh.mFaces[0].mIndices[0] = 1;
    mesh.mFaces[0].mIndices[1] = 3;
    mesh.mFaces[0].mIndices[2] = 2;

    mesh.mFaces[1].mIndices[0] = 0;
    mesh.mFaces[1].mIndices[1] = 2;
    mesh.mFaces[1].mIndices[2] = 3;

    mesh.mFaces[2].mIndices[0] = 3;
    mesh.mFaces[2].mIndices[1] = 0;
    mesh.mFaces[2].mIndices[2] = 4;

    checkMesh(mesh);
}

// ------------------------------------------------------------------------------------------------
TEST_F(VTAdjacencyTest, unreferencedVerticesSet)
{
    // build a test mesh which does not reference all vertices
    // *******************************************************************************
    aiMesh mesh;

    mesh.mNumVertices = 500;
    mesh.mNumFaces = 600;

    mesh.mFaces = new aiFace[600];
    unsigned int iCurrent = 0;
    for (unsigned int i = 0; i < 600;++i)
    {
        aiFace& face = mesh.mFaces[i];
        face.mNumIndices = 3;
        face.mIndices = new unsigned int[3];

        if (499 == iCurrent)iCurrent = 0;
        face.mIndices[0] = iCurrent++;

        if (499 == iCurrent)iCurrent = 0;
        face.mIndices[1] = iCurrent++;

        if (499 == iCurrent)iCurrent = 0;
        face.mIndices[2] = iCurrent++;

        if (rand() > RAND_MAX/2 && face.mIndices[0])
        {
            face.mIndices[0]--;
        }
        else if (face.mIndices[1]) face.mIndices[1]--;
    }

    checkMesh(mesh);
}

// ------------------------------------------------------------------------------------------------
void VTAdjacencyTest::checkMesh(const aiMesh& mesh)
{
    VertexTriangleAdjacency adj(mesh.mFaces,mesh.mNumFaces,mesh.mNumVertices,true);

    unsigned int* const piNum = adj.mLiveTriangles;

    // check the primary adjacency table and check whether all faces
    // are contained in the list
    unsigned int maxOfs = 0;
    for (unsigned int i = 0; i < mesh.mNumFaces;++i)
    {
        aiFace& face = mesh.mFaces[i];
        for (unsigned int qq = 0; qq < 3 ;++qq)
        {
            const unsigned int idx = face.mIndices[qq];
            const unsigned int num = piNum[idx];

            // go to this offset
            const unsigned int ofs = adj.mOffsetTable[idx];
            maxOfs = std::max(ofs+num,maxOfs);
            unsigned int* pi = &adj.mAdjacencyTable[ofs];

            // and search for us ...
            unsigned int tt = 0;
            for (; tt < num;++tt,++pi)
            {
                if (i == *pi)
                {
                    // mask our entry in the table. Finally all entries should be masked
                    *pi = 0xffffffff;

                    // there shouldn't be two entries for the same face
                    break;
                }
            }
            // assert if *this* vertex has not been found in the table
            EXPECT_LT(tt, num);
        }
    }

    // now check whether there are invalid faces
    const unsigned int* pi = adj.mAdjacencyTable;
    for (unsigned int i = 0; i < maxOfs;++i,++pi)
    {
        EXPECT_EQ(0xffffffff, *pi);
    }

    // check the numTrianglesPerVertex table
    for (unsigned int i = 0; i < mesh.mNumFaces;++i)
    {
        aiFace& face = mesh.mFaces[i];
        for (unsigned int qq = 0; qq < 3 ;++qq)
        {
            const unsigned int idx = face.mIndices[qq];

            // we should not reach 0 here ...
            EXPECT_NE(0U, piNum[idx]);
            piNum[idx]--;
        }
    }

    // check whether we reached 0 in all entries
    for (unsigned int i = 0; i < mesh.mNumVertices;++i)
    {
        EXPECT_FALSE(piNum[i]);
    }
}
