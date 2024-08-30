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

#include <assimp/scene.h>
#include <assimp/SceneCombiner.h>

using namespace Assimp;

class utScene : public ::testing::Test {
protected:
	aiScene *scene;

    void SetUp() override {
		scene = new aiScene;
    }

    void TearDown() override {
		delete scene;
		scene = nullptr;
    }
};

TEST_F(utScene, findNodeTest) {
	scene->mRootNode = new aiNode();
	scene->mRootNode->mName.Set("test");
	aiNode *child = new aiNode;
	child->mName.Set("child");
	scene->mRootNode->addChildren(1, &child);
	aiNode *found = scene->mRootNode->FindNode("child");
	EXPECT_EQ(child, found);
}

TEST_F(utScene, sceneHasContentTest) {
    EXPECT_FALSE(scene->HasAnimations());
	EXPECT_FALSE(scene->HasMaterials());
	EXPECT_FALSE(scene->HasMeshes());
	EXPECT_FALSE(scene->HasCameras());
	EXPECT_FALSE(scene->HasLights());
	EXPECT_FALSE(scene->HasTextures());
}

TEST_F(utScene, getShortFilenameTest) {
	std::string long_filename1 = "foo_bar/name";
    const char *name1 = scene->GetShortFilename(long_filename1.c_str());
	EXPECT_NE(nullptr, name1);

    std::string long_filename2 = "foo_bar\\name";
    const char *name2 = scene->GetShortFilename(long_filename2.c_str());
	EXPECT_NE(nullptr, name2);
}

TEST_F(utScene, deepCopyTest) {
    scene->mRootNode = new aiNode();

    scene->mNumMeshes = 1;
    scene->mMeshes = new aiMesh *[scene->mNumMeshes] ();
    scene->mMeshes[0] = new aiMesh ();

    scene->mMeshes[0]->SetTextureCoordsName (0, aiString ("test"));

    {
        aiScene* copied = nullptr;
        SceneCombiner::CopyScene(&copied,scene);
        delete copied;
    }
}

TEST_F(utScene, getEmbeddedTextureTest) {
}
