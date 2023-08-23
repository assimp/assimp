/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

#include "PostProcessing/GenBoundingBoxesProcess.h"
#include <assimp/mesh.h>
#include <assimp/scene.h>

using namespace Assimp;

class utGenBoundingBoxesProcess : public ::testing::Test {
public:
    utGenBoundingBoxesProcess() :
            Test(), mProcess(nullptr), mMesh(nullptr), mScene(nullptr) {
        // empty
    }

    void SetUp() override {
        mProcess = new GenBoundingBoxesProcess;
        mMesh = new aiMesh();
        mMesh->mNumVertices = 100;
        mMesh->mVertices = new aiVector3D[100];
        for (unsigned int i = 0; i < 100; ++i) {
            mMesh->mVertices[i] = aiVector3D((ai_real)i, (ai_real)i, (ai_real)i);
        }
        mScene = new aiScene();
        mScene->mNumMeshes = 1;
        mScene->mMeshes = new aiMesh *[1];
        mScene->mMeshes[0] = mMesh;
    }

    void TearDown() override {
        delete mProcess;
        delete mScene;
    }

protected:
    GenBoundingBoxesProcess *mProcess;
    aiMesh *mMesh;
    aiScene *mScene;
};

TEST_F(utGenBoundingBoxesProcess, executeTest) {
    mProcess->Execute(mScene);

    aiMesh *mesh = mScene->mMeshes[0];
    EXPECT_NE(nullptr, mesh);
    EXPECT_EQ(0, mesh->mAABB.mMin.x);
    EXPECT_EQ(0, mesh->mAABB.mMin.y);
    EXPECT_EQ(0, mesh->mAABB.mMin.z);

    EXPECT_EQ(99, mesh->mAABB.mMax.x);
    EXPECT_EQ(99, mesh->mAABB.mMax.y);
    EXPECT_EQ(99, mesh->mAABB.mMax.z);
}
