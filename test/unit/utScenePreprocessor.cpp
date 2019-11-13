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

#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "Common/ScenePreprocessor.h"

using namespace std;
using namespace Assimp;

class ScenePreprocessorTest : public ::testing::Test {
public:
    ScenePreprocessorTest()
    : Test()
    , mScenePreprocessor(nullptr)
    , mScene(nullptr) {
        // empty
    }

protected:
    virtual void SetUp();
    virtual void TearDown();

protected:
    void CheckIfOnly(aiMesh* p, unsigned int num, unsigned flag);
    void ProcessAnimation(aiAnimation* anim) { mScenePreprocessor->ProcessAnimation(anim); }
    void ProcessMesh(aiMesh* mesh) { mScenePreprocessor->ProcessMesh(mesh); }

private:
    ScenePreprocessor *mScenePreprocessor;
    aiScene *mScene;
};

// ------------------------------------------------------------------------------------------------
void ScenePreprocessorTest::SetUp() {
    // setup a dummy scene with a single node
    mScene = new aiScene();
    mScene->mRootNode = new aiNode();
    mScene->mRootNode->mName.Set("<test>");

    // add some translation
    mScene->mRootNode->mTransformation.a4 = 1.f;
    mScene->mRootNode->mTransformation.b4 = 2.f;
    mScene->mRootNode->mTransformation.c4 = 3.f;

    // and allocate a ScenePreprocessor to operate on the scene
    mScenePreprocessor = new ScenePreprocessor(mScene);
}

// ------------------------------------------------------------------------------------------------
void ScenePreprocessorTest::TearDown() {
    delete mScenePreprocessor;
    delete mScene;
}

// ------------------------------------------------------------------------------------------------
// Check whether ProcessMesh() returns flag for a mesh that consist of primitives with num indices
void ScenePreprocessorTest::CheckIfOnly(aiMesh* p, unsigned int num, unsigned int flag) {
    // Triangles only
    for (unsigned i = 0; i < p->mNumFaces;++i) {
        p->mFaces[i].mNumIndices = num;
    }
    mScenePreprocessor->ProcessMesh(p);
    EXPECT_EQ(flag, p->mPrimitiveTypes);
    p->mPrimitiveTypes = 0;
}

// ------------------------------------------------------------------------------------------------
// Check whether a mesh is preprocessed correctly. Case 1: The mesh needs preprocessing
TEST_F(ScenePreprocessorTest, testMeshPreprocessingPos) {
    aiMesh* p = new aiMesh;
    p->mNumFaces = 100;
    p->mFaces = new aiFace[p->mNumFaces];

    p->mTextureCoords[0] = new aiVector3D[10];
    p->mNumUVComponents[0] = 0;
    p->mNumUVComponents[1] = 0;

    CheckIfOnly(p,1,aiPrimitiveType_POINT);
    CheckIfOnly(p,2,aiPrimitiveType_LINE);
    CheckIfOnly(p,3,aiPrimitiveType_TRIANGLE);
    CheckIfOnly(p,4,aiPrimitiveType_POLYGON);
    CheckIfOnly(p,1249,aiPrimitiveType_POLYGON);

    // Polygons and triangles mixed
    unsigned i;
    for (i = 0; i < p->mNumFaces/2;++i) {
        p->mFaces[i].mNumIndices = 3;
    }
    for (; i < p->mNumFaces-p->mNumFaces/4;++i) {
        p->mFaces[i].mNumIndices = 4;
    }
    for (; i < p->mNumFaces;++i)    {
        p->mFaces[i].mNumIndices = 10;
    }
    ProcessMesh(p);
    EXPECT_EQ(static_cast<unsigned int>(aiPrimitiveType_TRIANGLE|aiPrimitiveType_POLYGON),
              p->mPrimitiveTypes);
    EXPECT_EQ(2U, p->mNumUVComponents[0]);
    EXPECT_EQ(0U, p->mNumUVComponents[1]);
    delete p;
}

// ------------------------------------------------------------------------------------------------
// Check whether a mesh is preprocessed correctly. Case 1: The mesh doesn't need preprocessing
TEST_F(ScenePreprocessorTest, testMeshPreprocessingNeg) {
    aiMesh* p = new aiMesh;
    p->mPrimitiveTypes = aiPrimitiveType_TRIANGLE|aiPrimitiveType_POLYGON;
    ProcessMesh(p);

    // should be unmodified
    EXPECT_EQ(static_cast<unsigned int>(aiPrimitiveType_TRIANGLE|aiPrimitiveType_POLYGON),
              p->mPrimitiveTypes);

    delete p;
}

// ------------------------------------------------------------------------------------------------
// Make a dummy animation with a single channel, '<test>'
aiAnimation* MakeDummyAnimation() {
    aiAnimation* p = new aiAnimation();
    p->mNumChannels = 1;
    p->mChannels = new aiNodeAnim*[1];
    aiNodeAnim* anim = p->mChannels[0] = new aiNodeAnim();
    anim->mNodeName.Set("<test>");
    return p;
}

// ------------------------------------------------------------------------------------------------
// Check whether an anim is preprocessed correctly. Case 1: The anim needs preprocessing
TEST_F(ScenePreprocessorTest, testAnimationPreprocessingPos) {
    aiAnimation* p = MakeDummyAnimation();
    aiNodeAnim* anim = p->mChannels[0];

    // we don't set the animation duration, but generate scaling channels
    anim->mNumScalingKeys = 10;
    anim->mScalingKeys = new aiVectorKey[10];

    for (unsigned int i = 0; i < 10;++i)    {
        anim->mScalingKeys[i].mTime = i;
        anim->mScalingKeys[i].mValue = aiVector3D((float)i);
    }
    ProcessAnimation(p);

    // we should now have a proper duration
    EXPECT_NEAR(p->mDuration, 9., 0.005);

    // ... one scaling key
    EXPECT_TRUE(anim->mNumPositionKeys == 1 &&
        anim->mPositionKeys &&
        anim->mPositionKeys[0].mTime == 0.0 &&
        anim->mPositionKeys[0].mValue == aiVector3D(1.f,2.f,3.f));

    // ... and one rotation key
    EXPECT_TRUE(anim->mNumRotationKeys == 1 && anim->mRotationKeys &&
        anim->mRotationKeys[0].mTime == 0.0);

    delete p;
}

