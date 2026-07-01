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

#include "PostProcessing/GenerateMikkTSpaceTangents.h"
#include <assimp/mesh.h>
#include <assimp/scene.h>

class utGenerateMikkTSpaceTangents : public ::testing::Test {
public:
    utGenerateMikkTSpaceTangents() : Test()  {
        // empty
    }
};

using namespace Assimp;

TEST_F(utGenerateMikkTSpaceTangents, calculateMikkTSpaceTangents) {
    aiMesh *mesh = new aiMesh();
    mesh->mNumVertices = 3;
    mesh->mVertices = new aiVector3D[3];
    mesh->mNormals = new aiVector3D[3];
    mesh->mTextureCoords[0] = new aiVector3D[3];
    mesh->mTangents = new aiVector3D[3];
    mesh->mBitangents = new aiVector3D[3];
    mesh->mNumFaces = 1;
    mesh->mFaces = new aiFace[1];
    mesh->mFaces[0].mNumIndices = 3;
    mesh->mFaces[0].mIndices = new unsigned int[3];
    mesh->mFaces[0].mIndices[0] = 0;
    mesh->mFaces[0].mIndices[1] = 1;
    mesh->mFaces[0].mIndices[2] = 2;

    // Set up a simple triangle with positions, normals, and texture coordinates
    mesh->mVertices[0] = aiVector3D(0.5f, 0.5f, 0.0f);
    mesh->mVertices[1] = aiVector3D(1.0f, 0.0f, 0.0f);
    mesh->mVertices[2] = aiVector3D(0.0f, 1.0f, 0.0f);

    mesh->mNormals[0] = aiVector3D(0.0f, 0.0f, 1.0f);
    mesh->mNormals[1] = aiVector3D(0.0f, 0.0f, 1.0f);
    mesh->mNormals[2] = aiVector3D(0.0f, 0.0f, 1.0f);

    mesh->mTextureCoords[0][0] = aiVector3D(0.0f, 0.0f, 0.0f);
    mesh->mTextureCoords[0][1] = aiVector3D(1.0f, 0.0f, 0.0f);
    mesh->mTextureCoords[0][2] = aiVector3D(0.5f, 1.0f, 0.0f);

    aiScene scene;
    scene.mNumMeshes = 1;
    scene.mMeshes = new aiMesh*[1];
    scene.mMeshes[0] = mesh;
    // Generate tangents using MikkTSpace
    GenerateMikkTSpaceTangents process;
    process.Execute(&scene);

    // Check that tangents and bitangents are generated
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        EXPECT_NE(mesh->mTangents[i], aiVector3D(0.0f, 0.0f, 0.0f));
        EXPECT_NE(mesh->mBitangents[i], aiVector3D(1.0f, 0.0f, 0.0f));
    }
}
