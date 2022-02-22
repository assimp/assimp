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

#include <assimp/mesh.h>

using namespace Assimp;

class utMesh : public ::testing::Test {
protected:
  aiMesh* mesh = nullptr;

  void SetUp() override {
    mesh = new aiMesh;
  }

  void TearDown() override {
    delete mesh;
    mesh = nullptr;
  }
};

TEST_F(utMesh, emptyMeshHasNoContentTest) {
  EXPECT_EQ(0u, mesh->mName.length);
  EXPECT_FALSE(mesh->HasPositions());
  EXPECT_FALSE(mesh->HasFaces());
  EXPECT_FALSE(mesh->HasNormals());
  EXPECT_FALSE(mesh->HasTangentsAndBitangents());
  EXPECT_FALSE(mesh->HasVertexColors(0));
  EXPECT_FALSE(mesh->HasVertexColors(AI_MAX_NUMBER_OF_COLOR_SETS));
  EXPECT_FALSE(mesh->HasTextureCoords(0));
  EXPECT_FALSE(mesh->HasTextureCoords(AI_MAX_NUMBER_OF_TEXTURECOORDS));
  EXPECT_EQ(0u, mesh->GetNumUVChannels());
  EXPECT_EQ(0u, mesh->GetNumColorChannels());
  EXPECT_FALSE(mesh->HasBones());
  EXPECT_FALSE(mesh->HasTextureCoordsName(0));
  EXPECT_FALSE(mesh->HasTextureCoordsName(AI_MAX_NUMBER_OF_TEXTURECOORDS));
}

TEST_F(utMesh, setTextureCoordsName) {
  EXPECT_FALSE(mesh->HasTextureCoordsName(0));
  const aiString texcoords_name("texcoord_name");
  mesh->SetTextureCoordsName(0, texcoords_name);
  EXPECT_TRUE(mesh->HasTextureCoordsName(0u));
  EXPECT_FALSE(mesh->HasTextureCoordsName(1u));
  ASSERT_NE(nullptr, mesh->mTextureCoordsNames);
  ASSERT_NE(nullptr, mesh->mTextureCoordsNames[0]);
  EXPECT_STREQ(texcoords_name.C_Str(), mesh->mTextureCoordsNames[0]->C_Str());
  EXPECT_STREQ(texcoords_name.C_Str(), mesh->GetTextureCoordsName(0)->C_Str());

  // Now clear the name
  mesh->SetTextureCoordsName(0, aiString());
  EXPECT_FALSE(mesh->HasTextureCoordsName(0));
  ASSERT_NE(nullptr, mesh->mTextureCoordsNames);
  EXPECT_EQ(nullptr, mesh->mTextureCoordsNames[0]);
  EXPECT_EQ(nullptr, mesh->GetTextureCoordsName(0));
}

