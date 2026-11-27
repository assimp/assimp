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
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>


using namespace Assimp;

TEST(utXGLImporter, importBCN_Epileptic) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/XGL/BCN_Epileptic.zgl", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(scene->mFlags, 0u);
    EXPECT_EQ(scene->mNumMaterials, 6u);
    EXPECT_EQ(scene->mNumAnimations, 0u);
    EXPECT_EQ(scene->mNumTextures, 0u);
    EXPECT_EQ(scene->mNumLights, 0u);
    EXPECT_EQ(scene->mNumCameras, 0u);

    EXPECT_EQ(scene->mNumSkeletons, 0u);
    ASSERT_EQ(scene->mNumMeshes, 3u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 6108u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 2036u);
    EXPECT_EQ(scene->mMeshes[1]->mNumVertices, 3372u);
    EXPECT_EQ(scene->mMeshes[1]->mNumFaces, 1124u);
    EXPECT_EQ(scene->mMeshes[2]->mNumVertices, 5898u);
    EXPECT_EQ(scene->mMeshes[2]->mNumFaces, 1966u);
}

TEST(utXGLImporter, importCubesWithAlpha) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/XGL/cubes_with_alpha.zgl", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(scene->mFlags, 0u);
    EXPECT_EQ(scene->mNumMaterials, 6u);
    EXPECT_EQ(scene->mNumAnimations, 0u);
    EXPECT_EQ(scene->mNumTextures, 0u);
    EXPECT_EQ(scene->mNumLights, 0u);
    EXPECT_EQ(scene->mNumCameras, 0u);

    EXPECT_EQ(scene->mNumSkeletons, 0u);
    ASSERT_EQ(scene->mNumMeshes, 5u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 12u);
    EXPECT_EQ(scene->mMeshes[1]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[1]->mNumFaces, 12u);
    EXPECT_EQ(scene->mMeshes[2]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[2]->mNumFaces, 12u);
    EXPECT_EQ(scene->mMeshes[3]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[3]->mNumFaces, 12u);
    EXPECT_EQ(scene->mMeshes[4]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[4]->mNumFaces, 12u);
}

TEST(utXGLImporter, importSample_official) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/XGL/sample_official.xgl", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(scene->mFlags, 0u);
    EXPECT_EQ(scene->mNumMaterials, 1u);
    EXPECT_EQ(scene->mNumAnimations, 0u);
    EXPECT_EQ(scene->mNumTextures, 0u);
    EXPECT_EQ(scene->mNumLights, 0u);
    EXPECT_EQ(scene->mNumCameras, 0u);

    EXPECT_EQ(scene->mNumSkeletons, 0u);
    ASSERT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 12u);
}

TEST(utXGLImporter, importSample_official_asxml) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/XGL/sample_official_asxml.xml", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(scene->mFlags, 0u);
    EXPECT_EQ(scene->mNumMaterials, 1u);
    EXPECT_EQ(scene->mNumAnimations, 0u);
    EXPECT_EQ(scene->mNumTextures, 0u);
    EXPECT_EQ(scene->mNumLights, 0u);
    EXPECT_EQ(scene->mNumCameras, 0u);

    EXPECT_EQ(scene->mNumSkeletons, 0u);
    ASSERT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 12u);
}

TEST(utXGLImporter, importSphereWithMatGloss) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/XGL/sphere_with_mat_gloss_10pc.zgl", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(scene->mFlags, 0u);
    EXPECT_EQ(scene->mNumMaterials, 3u);
    EXPECT_EQ(scene->mNumAnimations, 0u);
    EXPECT_EQ(scene->mNumTextures, 0u);
    EXPECT_EQ(scene->mNumLights, 0u);
    EXPECT_EQ(scene->mNumCameras, 0u);

    EXPECT_EQ(scene->mNumSkeletons, 0u);
    ASSERT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 1584u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 528u);
}

TEST(utXGLImporter, importSpiderASCII) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/XGL/Spider_ascii.zgl", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(scene->mFlags, 0u);
    EXPECT_EQ(scene->mNumMaterials, 1u);
    EXPECT_EQ(scene->mNumAnimations, 0u);
    EXPECT_EQ(scene->mNumTextures, 0u);
    EXPECT_EQ(scene->mNumLights, 0u);
    EXPECT_EQ(scene->mNumCameras, 0u);

    EXPECT_EQ(scene->mNumSkeletons, 0u);
    ASSERT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 3936u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 1312u);
}

TEST(utXGLImporter, importWuson) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/XGL/Wuson.zgl", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(scene->mFlags, 0u);
    EXPECT_EQ(scene->mNumMaterials, 2u);
    EXPECT_EQ(scene->mNumAnimations, 0u);
    EXPECT_EQ(scene->mNumTextures, 0u);
    EXPECT_EQ(scene->mNumLights, 0u);
    EXPECT_EQ(scene->mNumCameras, 0u);

    EXPECT_EQ(scene->mNumSkeletons, 0u);
    ASSERT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 11196u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 3732u);
}

TEST(utXGLImporter, importWusonDXF) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/XGL/wuson_dxf.zgl", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(scene->mFlags, 0u);
    EXPECT_EQ(scene->mNumMaterials, 1u);
    EXPECT_EQ(scene->mNumAnimations, 0u);
    EXPECT_EQ(scene->mNumTextures, 0u);
    EXPECT_EQ(scene->mNumLights, 0u);
    EXPECT_EQ(scene->mNumCameras, 0u);

    EXPECT_EQ(scene->mNumSkeletons, 0u);
    ASSERT_EQ(scene->mNumMeshes, 1u);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 11196u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 3732u);
}
