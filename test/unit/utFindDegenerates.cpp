/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

#include "../../include/assimp/scene.h"
#include "PostProcessing/FindDegenerates.h"

#include <memory>

using namespace std;
using namespace Assimp;

class FindDegeneratesProcessTest : public ::testing::Test {
public:
    FindDegeneratesProcessTest() :
            Test(), mMesh(nullptr), mProcess(nullptr) {
        // empty
    }

protected:
    virtual void SetUp();
    virtual void TearDown();

protected:
    aiMesh *mMesh;
    FindDegeneratesProcess *mProcess;
};

void FindDegeneratesProcessTest::SetUp() {
    mMesh = new aiMesh();
    mProcess = new FindDegeneratesProcess();

    mMesh->mNumFaces = 1000;
    mMesh->mFaces = new aiFace[1000];

    mMesh->mNumVertices = 5000 * 2;
    mMesh->mVertices = new aiVector3D[5000 * 2];

    for (unsigned int i = 0; i < 5000; ++i) {
        mMesh->mVertices[i] = mMesh->mVertices[i + 5000] = aiVector3D((float)i);
    }

    mMesh->mPrimitiveTypes = aiPrimitiveType_LINE | aiPrimitiveType_POINT |
                             aiPrimitiveType_POLYGON | aiPrimitiveType_TRIANGLE;

    unsigned int numOut = 0, numFaces = 0;
    for (unsigned int i = 0; i < 1000; ++i) {
        aiFace &f = mMesh->mFaces[i];
        f.mNumIndices = (i % 5) + 1; // between 1 and 5
        f.mIndices = new unsigned int[f.mNumIndices];
        bool had = false;
        for (unsigned int n = 0; n < f.mNumIndices; ++n) {
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
                f.mIndices[n] = f.mIndices[n - 1] + 5000;
                had = true;
            } else {
                f.mIndices[n] = numOut++;
            }
        }
        if (!had)
            ++numFaces;
    }
    mMesh->mNumUVComponents[0] = numOut;
    mMesh->mNumUVComponents[1] = numFaces;
}

void FindDegeneratesProcessTest::TearDown() {
    delete mMesh;
    delete mProcess;
}

TEST_F(FindDegeneratesProcessTest, testDegeneratesDetection) {
    mProcess->EnableInstantRemoval(false);
    mProcess->ExecuteOnMesh(mMesh);

    unsigned int out = 0;
    for (unsigned int i = 0; i < 1000; ++i) {
        aiFace &f = mMesh->mFaces[i];
        out += f.mNumIndices;
    }

    EXPECT_EQ(1000U, mMesh->mNumFaces);
    EXPECT_EQ(10000U, mMesh->mNumVertices);
    EXPECT_EQ(out, mMesh->mNumUVComponents[0]);
    EXPECT_EQ(static_cast<unsigned int>(
                      aiPrimitiveType_LINE | aiPrimitiveType_POINT |
                      aiPrimitiveType_POLYGON | aiPrimitiveType_TRIANGLE),
            mMesh->mPrimitiveTypes);
}

TEST_F(FindDegeneratesProcessTest, testDegeneratesRemoval) {
    mProcess->EnableAreaCheck(false);
    mProcess->EnableInstantRemoval(true);
    mProcess->ExecuteOnMesh(mMesh);

    EXPECT_EQ(mMesh->mNumUVComponents[1], mMesh->mNumFaces);
}

TEST_F(FindDegeneratesProcessTest, testDegeneratesRemovalWithAreaCheck) {
    mProcess->EnableAreaCheck(true);
    mProcess->EnableInstantRemoval(true);
    mProcess->ExecuteOnMesh(mMesh);

    EXPECT_EQ(mMesh->mNumUVComponents[1] - 100, mMesh->mNumFaces);
}

namespace
{
    std::unique_ptr<aiMesh> getDegenerateMesh()
    {
        std::unique_ptr<aiMesh> mesh(new aiMesh);
        mesh->mNumVertices = 2;
        mesh->mVertices = new aiVector3D[2];
        mesh->mVertices[0] = aiVector3D{ 0.0f, 0.0f, 0.0f };
        mesh->mVertices[1] = aiVector3D{ 1.0f, 0.0f, 0.0f };
        mesh->mNumFaces = 1;
        mesh->mFaces = new aiFace[1];
        mesh->mFaces[0].mNumIndices = 3;
        mesh->mFaces[0].mIndices = new unsigned int[3];
        mesh->mFaces[0].mIndices[0] = 0;
        mesh->mFaces[0].mIndices[1] = 1;
        mesh->mFaces[0].mIndices[2] = 0;
        return mesh;
    }
}

TEST_F(FindDegeneratesProcessTest, meshRemoval) {
    mProcess->EnableAreaCheck(true);
    mProcess->EnableInstantRemoval(true);
    mProcess->ExecuteOnMesh(mMesh);

    std::unique_ptr<aiScene> scene(new aiScene);
    scene->mNumMeshes = 5;
    scene->mMeshes = new aiMesh*[5];

    /// Use the mesh which doesn't get completely stripped of faces from the main test.
    aiMesh* meshWhichSurvives = mMesh;
    mMesh = nullptr;

    scene->mMeshes[0] = getDegenerateMesh().release();
    scene->mMeshes[1] = getDegenerateMesh().release();
    scene->mMeshes[2] = meshWhichSurvives;
    scene->mMeshes[3] = getDegenerateMesh().release();
    scene->mMeshes[4] = getDegenerateMesh().release();

    scene->mRootNode = new aiNode;
    scene->mRootNode->mNumMeshes = 5;
    scene->mRootNode->mMeshes = new unsigned int[5];
    scene->mRootNode->mMeshes[0] = 0;
    scene->mRootNode->mMeshes[1] = 1;
    scene->mRootNode->mMeshes[2] = 2;
    scene->mRootNode->mMeshes[3] = 3;
    scene->mRootNode->mMeshes[4] = 4;

    mProcess->Execute(scene.get());

    EXPECT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mMeshes[0], meshWhichSurvives);
    EXPECT_EQ(scene->mRootNode->mNumMeshes, 1u);
    EXPECT_EQ(scene->mRootNode->mMeshes[0], 0u);
}

TEST_F(FindDegeneratesProcessTest, invalidVertexIndex) {
    mProcess->EnableAreaCheck(true);
    mProcess->EnableInstantRemoval(true);
    mProcess->ExecuteOnMesh(mMesh);

    std::unique_ptr<aiScene> scene(new aiScene);
    scene->mNumMeshes = 1;
    scene->mMeshes = new aiMesh *[1];

    std::unique_ptr<aiMesh> mesh(new aiMesh);
    mesh->mNumVertices = 1;
    mesh->mVertices = new aiVector3D[1];
    mesh->mVertices[0] = aiVector3D{ 0.0f, 0.0f, 0.0f };
    mesh->mNumFaces = 1;
    mesh->mFaces = new aiFace[1];
    mesh->mFaces[0].mNumIndices = 3;
    mesh->mFaces[0].mIndices = new unsigned int[3];
    mesh->mFaces[0].mIndices[0] = 0;
    mesh->mFaces[0].mIndices[1] = 1;
    mesh->mFaces[0].mIndices[2] = 99999;

    scene->mMeshes[0] = mesh.release();

    scene->mRootNode = new aiNode;
    scene->mRootNode->mNumMeshes = 1;
    scene->mRootNode->mMeshes = new unsigned int[1];
    scene->mRootNode->mMeshes[0] = 0;

    mProcess->Execute(scene.get());

    EXPECT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mRootNode->mNumMeshes, 1u);
    EXPECT_EQ(scene->mRootNode->mMeshes[0], 0u);
}
