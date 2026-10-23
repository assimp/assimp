/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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
#include <assimp/SceneCombiner.h>
#include <assimp/mesh.h>
#include <memory>

using namespace ::Assimp;

class utSceneCombiner : public ::testing::Test {
    // empty
};

TEST_F(utSceneCombiner, MergeMeshes_ValidNames_Test) {
    std::vector<aiMesh *> merge_list;
    aiMesh *mesh1 = new aiMesh;
    mesh1->mName.Set("mesh_1");
    merge_list.push_back(mesh1);

    aiMesh *mesh2 = new aiMesh;
    mesh2->mName.Set("mesh_2");
    merge_list.push_back(mesh2);

    aiMesh *mesh3 = new aiMesh;
    mesh3->mName.Set("mesh_3");
    merge_list.push_back(mesh3);

    std::unique_ptr<aiMesh> out;
    aiMesh *ptr = nullptr;
    SceneCombiner::MergeMeshes(&ptr, 0, merge_list.begin(), merge_list.end());
    out.reset(ptr);
    std::string outName = out->mName.C_Str();
    EXPECT_EQ("mesh_1.mesh_2.mesh_3", outName);
}

TEST_F(utSceneCombiner, CopySceneWithNullptr_AI_NO_EXCEPTion) {
    EXPECT_NO_THROW(SceneCombiner::CopyScene(nullptr, nullptr));
    EXPECT_NO_THROW(SceneCombiner::CopySceneFlat(nullptr, nullptr));
}

// Copying a mesh must give the copy its own UV and color buffers for every
// populated channel, even when the channel is not the first one or when the
// mesh reports zero vertices. Otherwise the copy aliases the source arrays and
// destroying both double-frees them (assimp/assimp#6620).
TEST_F(utSceneCombiner, CopyMeshDoesNotAliasSparseOrEmptyChannels) {
    auto makeMesh = []() {
        aiMesh *mesh = new aiMesh;
        // A populated UV channel that is not channel 0.
        mesh->mNumVertices = 2;
        mesh->mVertices = new aiVector3D[2];
        mesh->mTextureCoords[1] = new aiVector3D[2]{};
        mesh->mNumUVComponents[1] = 2;
        // A populated color channel that is not channel 0.
        mesh->mColors[1] = new aiColor4D[2]{};
        return mesh;
    };

    // Non-contiguous channels: HasTextureCoords(0) is false, so the old loop
    // stopped before copying channel 1.
    {
        aiMesh *src = makeMesh();
        aiMesh *dst = nullptr;
        SceneCombiner::Copy(&dst, src);
        ASSERT_NE(dst, nullptr);
        EXPECT_NE(dst->mTextureCoords[1], src->mTextureCoords[1]);
        EXPECT_NE(dst->mColors[1], src->mColors[1]);
        delete dst;
        delete src;
    }

    // A populated channel on a zero-vertex mesh: HasTextureCoords() requires
    // mNumVertices > 0, so the old loop skipped it.
    {
        aiMesh *src = makeMesh();
        src->mNumVertices = 0;
        delete[] src->mVertices;
        src->mVertices = nullptr;
        // Move the populated channels to index 0 to exercise the mNumVertices
        // guard specifically.
        std::swap(src->mTextureCoords[0], src->mTextureCoords[1]);
        std::swap(src->mColors[0], src->mColors[1]);
        aiMesh *dst = nullptr;
        SceneCombiner::Copy(&dst, src);
        ASSERT_NE(dst, nullptr);
        EXPECT_NE(dst->mTextureCoords[0], src->mTextureCoords[0]);
        EXPECT_NE(dst->mColors[0], src->mColors[0]);
        delete dst;
        delete src;
    }
}
