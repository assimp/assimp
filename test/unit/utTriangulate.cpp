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

#include <assimp/scene.h>
#include <TriangulateProcess.h>


using namespace std;
using namespace Assimp;

class TriangulateProcessTest : public ::testing::Test {
public:
    virtual void SetUp();
    virtual void TearDown();

protected:
    aiMesh* pcMesh;
    TriangulateProcess* piProcess;
};

void TriangulateProcessTest::SetUp() {
    piProcess = new TriangulateProcess();
    pcMesh = new aiMesh();

    pcMesh->mNumFaces = 1000;
    pcMesh->mFaces = new aiFace[1000];
    pcMesh->mVertices = new aiVector3D[10000];

    pcMesh->mPrimitiveTypes = aiPrimitiveType_POINT | aiPrimitiveType_LINE |
    aiPrimitiveType_LINE | aiPrimitiveType_POLYGON;

    for (unsigned int m = 0, t = 0, q = 4; m < 1000; ++m) {
        ++t;
        aiFace& face = pcMesh->mFaces[m];
        face.mNumIndices = t;
        if (4 == t) {
            face.mNumIndices = q++;
            t = 0;

            if (10 == q)q = 4;
        }
        face.mIndices = new unsigned int[face.mNumIndices];
        for (unsigned int p = 0; p < face.mNumIndices; ++p) {
            face.mIndices[ p ] = pcMesh->mNumVertices;

            // construct fully convex input data in ccw winding, xy plane
            aiVector3D& v = pcMesh->mVertices[pcMesh->mNumVertices++];
            v.z = 0.f;
            v.x = cos (p * (float)(AI_MATH_TWO_PI)/face.mNumIndices);
            v.y = sin (p * (float)(AI_MATH_TWO_PI)/face.mNumIndices);
        }
    }
}

void TriangulateProcessTest::TearDown() {
    delete piProcess;
    delete pcMesh;
}

TEST_F(TriangulateProcessTest, testTriangulation) {
    piProcess->TriangulateMesh(pcMesh);

    for (unsigned int m = 0, t = 0, q = 4, max = 1000, idx = 0; m < max;++m) {
        ++t;
        aiFace& face = pcMesh->mFaces[m];
        if (4 == t) {
            t = 0;
            max += q-3;

            std::vector<bool> ait(q,false);

            for (unsigned int i = 0, tt = q-2; i < tt; ++i,++m) {
                aiFace& face = pcMesh->mFaces[m];
                EXPECT_EQ(3U, face.mNumIndices);

                for (unsigned int qqq = 0; qqq < face.mNumIndices; ++qqq) {
                    ait[face.mIndices[qqq]-idx] = true;
                }
            }
            for (std::vector<bool>::const_iterator it = ait.begin(); it != ait.end(); ++it) {
                EXPECT_TRUE(*it);
            }
            --m;
            idx+=q;
            if ( ++q == 10 ) {
                q = 4;
            }
        } else {
            EXPECT_EQ(t, face.mNumIndices);

            for (unsigned int i = 0; i < face.mNumIndices; ++i,++idx) {
                EXPECT_EQ(idx, face.mIndices[i]);
            }
        }
    }

    // we should have no valid normal vectors now necause we aren't a pure polygon mesh
    EXPECT_TRUE(pcMesh->mNormals == NULL);
}
