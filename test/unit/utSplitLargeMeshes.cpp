/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2017, assimp team


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
#include <SplitLargeMeshes.h>


using namespace std;
using namespace Assimp;

class SplitLargeMeshesTest : public ::testing::Test
{
public:

    virtual void SetUp();
    virtual void TearDown();

protected:

    SplitLargeMeshesProcess_Triangle* piProcessTriangle;
    SplitLargeMeshesProcess_Vertex* piProcessVertex;

};

// ------------------------------------------------------------------------------------------------
void SplitLargeMeshesTest::SetUp()
{
    // construct the processes
    this->piProcessTriangle = new SplitLargeMeshesProcess_Triangle();
    this->piProcessVertex = new SplitLargeMeshesProcess_Vertex();

    this->piProcessTriangle->SetLimit(1000);
    this->piProcessVertex->SetLimit(1000);

}

// ------------------------------------------------------------------------------------------------
void SplitLargeMeshesTest::TearDown()
{
    delete this->piProcessTriangle;
    delete this->piProcessVertex;
}

// ------------------------------------------------------------------------------------------------
TEST_F(SplitLargeMeshesTest, testVertexSplit)
{
    std::vector< std::pair<aiMesh*, unsigned int> > avOut;

     aiMesh *pcMesh1 = new aiMesh();
     pcMesh1->mNumVertices = 2100; // quersumme: 3
     pcMesh1->mVertices = new aiVector3D[pcMesh1->mNumVertices];
     pcMesh1->mNormals = new aiVector3D[pcMesh1->mNumVertices];

     pcMesh1->mNumFaces = pcMesh1->mNumVertices / 3;
     pcMesh1->mFaces = new aiFace[pcMesh1->mNumFaces];

     unsigned int qq = 0;
     for (unsigned int i = 0; i < pcMesh1->mNumFaces;++i)
     {
         aiFace& face = pcMesh1->mFaces[i];
         face.mNumIndices = 3;
         face.mIndices = new unsigned int[3];
         face.mIndices[0] = qq++;
         face.mIndices[1] = qq++;
         face.mIndices[2] = qq++;
     }


    int iOldFaceNum = (int)pcMesh1->mNumFaces;
    piProcessVertex->SplitMesh(0,pcMesh1,avOut);

    for (std::vector< std::pair<aiMesh*, unsigned int> >::const_iterator
        iter =  avOut.begin(), end = avOut.end();
        iter != end; ++iter)
    {
        aiMesh* mesh = (*iter).first;
        EXPECT_LT(mesh->mNumVertices, 1000U);
        EXPECT_TRUE(NULL != mesh->mNormals);
        EXPECT_TRUE(NULL != mesh->mVertices);

        iOldFaceNum -= mesh->mNumFaces;
        delete mesh;
    }
    EXPECT_EQ(0, iOldFaceNum);
}

// ------------------------------------------------------------------------------------------------
TEST_F(SplitLargeMeshesTest, testTriangleSplit)
{
    std::vector< std::pair<aiMesh*, unsigned int> > avOut;

    // generate many, many faces with randomized indices for
    // the second mesh
    aiMesh *pcMesh2 = new aiMesh();
    pcMesh2->mNumVertices = 3000;
    pcMesh2->mVertices = new aiVector3D[pcMesh2->mNumVertices];
    pcMesh2->mNormals = new aiVector3D[pcMesh2->mNumVertices];

    pcMesh2->mNumFaces = 10000;
    pcMesh2->mFaces = new aiFace[pcMesh2->mNumFaces];

    for (unsigned int i = 0; i < pcMesh2->mNumFaces;++i)
    {
        aiFace& face = pcMesh2->mFaces[i];
        face.mNumIndices = 3;
        face.mIndices = new unsigned int[3];
        face.mIndices[0] = (unsigned int)((rand() / (float)RAND_MAX) * pcMesh2->mNumVertices);
        face.mIndices[1] = (unsigned int)((rand() / (float)RAND_MAX) * pcMesh2->mNumVertices);
        face.mIndices[2] = (unsigned int)((rand() / (float)RAND_MAX) * pcMesh2->mNumVertices);
    }

    // the number of faces shouldn't change
    int iOldFaceNum = (int)pcMesh2->mNumFaces;
    piProcessTriangle->SplitMesh(0,pcMesh2,avOut);

    for (std::vector< std::pair<aiMesh*, unsigned int> >::const_iterator
        iter =  avOut.begin(), end = avOut.end();
        iter != end; ++iter)
    {
        aiMesh* mesh = (*iter).first;
        EXPECT_LT(mesh->mNumFaces, 1000U);
        EXPECT_TRUE(NULL != mesh->mNormals);
        EXPECT_TRUE(NULL != mesh->mVertices);

        iOldFaceNum -= mesh->mNumFaces;
        delete mesh;
    }
    EXPECT_EQ(0, iOldFaceNum);
}
