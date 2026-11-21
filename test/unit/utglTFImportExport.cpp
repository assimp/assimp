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
#include "AbstractImportExportBase.h"
#include "UnitTestPCH.h"

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include "ReadSizeRecordingIOSystem.h"

#include <assimp/commonMetaData.h>
#include <assimp/scene.h>

using namespace Assimp;

class utglTFImportExport : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF/TwoBoxes/TwoBoxes.gltf", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utglTFImportExport, importglTFFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utglTFImportExport, incorrect_vertex_arrays) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF/IncorrectVertexArrays/Cube_v1.gltf",
            aiProcess_ValidateDataStructure);
    EXPECT_NE(nullptr, scene);
    EXPECT_EQ(scene->mMeshes[0]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[0]->mNumFaces, 12u);
    EXPECT_EQ(scene->mMeshes[1]->mNumVertices, 35u);
    EXPECT_EQ(scene->mMeshes[1]->mNumFaces, 11u);
    EXPECT_EQ(scene->mMeshes[2]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[2]->mNumFaces, 18u);
    EXPECT_EQ(scene->mMeshes[3]->mNumVertices, 35u);
    EXPECT_EQ(scene->mMeshes[3]->mNumFaces, 17u);
    EXPECT_EQ(scene->mMeshes[4]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[4]->mNumFaces, 12u);
    EXPECT_EQ(scene->mMeshes[5]->mNumVertices, 35u);
    EXPECT_EQ(scene->mMeshes[5]->mNumFaces, 11u);
    EXPECT_EQ(scene->mMeshes[6]->mNumVertices, 36u);
    EXPECT_EQ(scene->mMeshes[6]->mNumFaces, 18u);
    EXPECT_EQ(scene->mMeshes[7]->mNumVertices, 35u);
    EXPECT_EQ(scene->mMeshes[7]->mNumFaces, 17u);
}

// A binary glTF 1.0 file: header (magic, version 1, total length, scene length, scene format
// JSON) and a scene with one triangle. The scene length in the header can be overridden.
static std::string makeBinaryTriangle(uint32_t declaredSceneLength = 0) {
    const std::string scene = R"({"asset":{"version":"1.0"},)"
            R"("buffers":{"buf":{"byteLength":36,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAA"}},)"
            R"("bufferViews":{"v":{"buffer":"buf","byteOffset":0,"byteLength":36}},)"
            R"("accessors":{"pos":{"bufferView":"v","byteOffset":0,"componentType":5126,"count":3,"type":"VEC3"}},)"
            R"("meshes":{"tri":{"primitives":[{"attributes":{"POSITION":"pos"},"mode":4}]}},)"
            R"("nodes":{"node":{"meshes":["tri"]}},"scenes":{"scene":{"nodes":["node"]}},"scene":"scene"})";
    // the body that follows the scene starts at a multiple of four bytes
    std::string padded = scene + std::string((4 - scene.size() % 4) % 4, ' ');
    auto put = [](std::string &out, uint32_t v) {
        for (int i = 0; i < 4; ++i) {
            out += static_cast<char>((v >> (8 * i)) & 0xff);
        }
    };
    std::string file = "glTF";
    put(file, 1);
    put(file, static_cast<uint32_t>(20 + padded.size()));
    put(file, declaredSceneLength ? declaredSceneLength : static_cast<uint32_t>(padded.size()));
    put(file, 0);
    return file + padded;
}

TEST_F(utglTFImportExport, binarySceneLengthLargerThanFile) {
    {
        const std::string data = makeBinaryTriangle();
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "glb");
        ASSERT_NE(nullptr, scene);
        EXPECT_EQ(1u, scene->mNumMeshes);
    }

    // The header declares a 2 GB scene. The importer reports only that no reader accepted
    // the file, so record what the loaders ask the stream for: the format detection reads a
    // few hundred bytes, the scene buffer of the declared size must not be read.
    const std::string data = makeBinaryTriangle(0x80000000u);
    size_t largestRead = 0;
    Assimp::Importer importer;
    importer.SetIOHandler(new Assimp::ReadSizeRecordingIOSystem("test.glb", reinterpret_cast<const uint8_t *>(data.data()), data.size(), largestRead));
    EXPECT_EQ(nullptr, importer.ReadFile("test.glb", 0));
    EXPECT_LT(largestRead, 4096u);
}

TEST_F(utglTFImportExport, sceneMetadata) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/glTF/TwoBoxes/TwoBoxes.gltf", aiProcess_ValidateDataStructure);
    ASSERT_TRUE(scene);
    ASSERT_TRUE(scene->mMetaData);
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_FORMAT));
        aiString format;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT, format));
        ASSERT_EQ(strcmp(format.C_Str(), "glTF Importer"), 0);
    }
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_FORMAT_VERSION));
        aiString version;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_FORMAT_VERSION, version));
        ASSERT_EQ(strcmp(version.C_Str(), "1.0"), 0);
    }
    {
        ASSERT_TRUE(scene->mMetaData->HasKey(AI_METADATA_SOURCE_GENERATOR));
        aiString generator;
        ASSERT_TRUE(scene->mMetaData->Get(AI_METADATA_SOURCE_GENERATOR, generator));
        ASSERT_EQ(strncmp(generator.C_Str(), "collada2gltf", 12), 0);
    }
}
