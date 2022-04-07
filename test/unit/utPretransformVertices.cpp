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

#include "PostProcessing/PretransformVertices.h"
#include <assimp/scene.h>

using namespace std;
using namespace Assimp;

class PretransformVerticesTest : public ::testing::Test {
public:
    PretransformVerticesTest() :
            Test(), mScene(nullptr), mProcess(nullptr) {
        // empty
    }

protected:
    virtual void SetUp();
    virtual void TearDown();

protected:
    aiScene *mScene;
    PretransformVertices *mProcess;
};

// ------------------------------------------------------------------------------------------------
void AddNodes(unsigned int num, aiNode *father, unsigned int depth) {
    father->mChildren = new aiNode *[father->mNumChildren = 5];
    for (unsigned int i = 0; i < 5; ++i) {
        aiNode *nd = father->mChildren[i] = new aiNode();

        nd->mName.length = sprintf(nd->mName.data, "%i%i", depth, i);

        // spawn two meshes
        nd->mMeshes = new unsigned int[nd->mNumMeshes = 2];
        nd->mMeshes[0] = num * 5 + i;
        nd->mMeshes[1] = 24 - (num * 5 + i); // mesh 12 is special ... it references the same mesh twice

        // setup an unique transformation matrix
        nd->mTransformation.a1 = num * 5.f + i + 1;
    }

    if (depth > 1) {
        for (unsigned int i = 0; i < 5; ++i) {
            AddNodes(i, father->mChildren[i], depth - 1);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void PretransformVerticesTest::SetUp() {
    mScene = new aiScene();

    // add 5 empty materials
    mScene->mMaterials = new aiMaterial *[mScene->mNumMaterials = 5];
    for (unsigned int i = 0; i < 5; ++i) {
        mScene->mMaterials[i] = new aiMaterial();
    }

    // add 25 test meshes
    mScene->mMeshes = new aiMesh *[mScene->mNumMeshes = 25];
    for (unsigned int i = 0; i < 25; ++i) {
        aiMesh *mesh = mScene->mMeshes[i] = new aiMesh();

        mesh->mPrimitiveTypes = aiPrimitiveType_POINT;
        mesh->mFaces = new aiFace[mesh->mNumFaces = 10 + i];
        mesh->mVertices = new aiVector3D[mesh->mNumVertices = mesh->mNumFaces];
        for (unsigned int a = 0; a < mesh->mNumFaces; ++a) {
            aiFace &f = mesh->mFaces[a];
            f.mIndices = new unsigned int[f.mNumIndices = 1];
            f.mIndices[0] = a * 3;

            mesh->mVertices[a] = aiVector3D((float)i, (float)a, 0.f);
        }
        mesh->mMaterialIndex = i % 5;

        if (i % 2) {
            mesh->mNormals = new aiVector3D[mesh->mNumVertices];
            for (unsigned int normalIdx = 0; normalIdx < mesh->mNumVertices; ++normalIdx) {
                mesh->mNormals[normalIdx].x = 1.0f;
                mesh->mNormals[normalIdx].y = 1.0f;
                mesh->mNormals[normalIdx].z = 1.0f;
                mesh->mNormals[normalIdx].Normalize();
            }
        }
    }

    // construct some nodes (1+25)
    mScene->mRootNode = new aiNode();
    mScene->mRootNode->mName.Set("Root");
    AddNodes(0, mScene->mRootNode, 2);

    mProcess = new PretransformVertices();
}

// ------------------------------------------------------------------------------------------------
void PretransformVerticesTest::TearDown() {
    delete mScene;
    delete mProcess;
}

// ------------------------------------------------------------------------------------------------
TEST_F(PretransformVerticesTest, testProcessCollapseHierarchy) {
    mProcess->KeepHierarchy(false);
    mProcess->Execute(mScene);

    EXPECT_EQ(5U, mScene->mNumMaterials);
    EXPECT_EQ(10U, mScene->mNumMeshes); // every second mesh has normals
}

// ------------------------------------------------------------------------------------------------
TEST_F(PretransformVerticesTest, testProcessKeepHierarchy) {
    mProcess->KeepHierarchy(true);
    mProcess->Execute(mScene);

    EXPECT_EQ(5U, mScene->mNumMaterials);
    EXPECT_EQ(49U, mScene->mNumMeshes); // see note on mesh 12 above
}
