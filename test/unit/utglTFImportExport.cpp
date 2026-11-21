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

// A glTF 1.0 primitive with an index accessor of the given type over three uint32 values:
// a triangle for SCALAR, a single point otherwise.
static std::string makeTriangleWithIndexType(const char *type) {
    const bool scalar = std::string(type) == "SCALAR";
    // 3 x VEC3 float positions followed by 3 x uint32 indices 0, 1, 2
    std::string json = R"({
  "asset": { "version": "1.0" },
  "buffers": { "buf": { "byteLength": 48, "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAEAAAACAAAA" } },
  "bufferViews": {
    "pos_view": { "buffer": "buf", "byteOffset": 0, "byteLength": 36, "target": 34962 },
    "idx_view": { "buffer": "buf", "byteOffset": 36, "byteLength": 12, "target": 34963 }
  },
  "accessors": {
    "pos": { "bufferView": "pos_view", "byteOffset": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
    "idx": { "bufferView": "idx_view", "byteOffset": 0, "componentType": 5125, "count": )";
    json += scalar ? "3" : "1";
    json += R"(, "type": ")";
    json += type;
    json += R"(" }
  },
  "meshes": { "tri": { "primitives": [ { "attributes": { "POSITION": "pos" }, "indices": "idx", "mode": )";
    json += scalar ? "4" : "0";
    json += R"( } ] } },
  "nodes": { "node": { "meshes": [ "tri" ] } },
  "scenes": { "scene": { "nodes": [ "node" ] } },
  "scene": "scene"
})";
    return json;
}

TEST_F(utglTFImportExport, indexAccessorMustBeScalar) {
    {
        const std::string json = makeTriangleWithIndexType("SCALAR");
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFileFromMemory(json.c_str(), json.size(), 0, "gltf");
        ASSERT_NE(nullptr, scene);
        ASSERT_EQ(1u, scene->mNumMeshes);
        EXPECT_EQ(1u, scene->mMeshes[0]->mNumFaces);
    }
    {
        // the indices are read as unsigned ints, a VEC3 element does not fit
        const std::string json = makeTriangleWithIndexType("VEC3");
        Assimp::Importer importer;
        EXPECT_EQ(nullptr, importer.ReadFileFromMemory(json.c_str(), json.size(), 0, "gltf"));
    }
}

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
