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
#include "UriTestHelpers.h"

#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include <assimp/commonMetaData.h>
#include <assimp/scene.h>

#include <sstream>
#include <string>

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


using namespace Assimp::UriTest;

namespace {

// Builds a minimal glTF 1.0 triangle whose buffer uses the given URI.
std::string Gltf1Buffer(const std::string &bufferUri) {
    std::ostringstream json;
    json << "{\n"
            "  \"asset\": {\"version\": \"1.0\"},\n"
            "  \"scene\": \"s\",\n"
            "  \"scenes\": {\"s\": {\"nodes\": [\"n\"]}},\n"
            "  \"nodes\": {\"n\": {\"meshes\": [\"m\"]}},\n"
            "  \"meshes\": {\"m\": {\"primitives\": [{\"attributes\": {\"POSITION\": \"p\"}}]}},\n"
            "  \"accessors\": {\"p\": {\"bufferView\": \"v\", \"byteOffset\": 0, \"componentType\": 5126, \"count\": 3, \"type\": \"VEC3\"}},\n"
            "  \"bufferViews\": {\"v\": {\"buffer\": \"b\", \"byteOffset\": 0, \"byteLength\": 36}},\n"
            "  \"buffers\": {\"b\": {\"byteLength\": 36, \"type\": \"arraybuffer\", \"uri\": \""
         << JsonEscape(bufferUri) << "\"}}\n"
            "}\n";
    return json.str();
}

// Builds a minimal glTF 1.0 triangle whose diffuse image uses the given URI.
std::string Gltf1Image(const std::string &imageUri) {
    std::ostringstream json;
    json << "{\n"
            "  \"asset\": {\"version\": \"1.0\"},\n"
            "  \"scene\": \"s\",\n"
            "  \"scenes\": {\"s\": {\"nodes\": [\"n\"]}},\n"
            "  \"nodes\": {\"n\": {\"meshes\": [\"m\"]}},\n"
            "  \"meshes\": {\"m\": {\"primitives\": [{\"attributes\": {\"POSITION\": \"p\"}, \"material\": \"mat\"}]}},\n"
            "  \"materials\": {\"mat\": {\"values\": {\"diffuse\": \"tex\"}}},\n"
            "  \"textures\": {\"tex\": {\"source\": \"img\"}},\n"
            "  \"images\": {\"img\": {\"uri\": \""
         << JsonEscape(imageUri) << "\"}},\n"
            "  \"accessors\": {\"p\": {\"bufferView\": \"v\", \"byteOffset\": 0, \"componentType\": 5126, \"count\": 3, \"type\": \"VEC3\"}},\n"
            "  \"bufferViews\": {\"v\": {\"buffer\": \"b\", \"byteOffset\": 0, \"byteLength\": 36}},\n"
            "  \"buffers\": {\"b\": {\"byteLength\": 36, \"type\": \"arraybuffer\", \"uri\": \""
         << kDataUri36Zeros << "\"}}\n"
            "}\n";
    return json.str();
}

} // namespace

TEST_F(utglTFImportExport, externalUriIsContained) {
    const ScratchPackage scratch("assimp-issue6933-gltf1-");
    ASSERT_TRUE(scratch.Populate());

    ExpectFileLoads(scratch.Write("benign.gltf", Gltf1Buffer("triangle.bin")));
    ExpectFileRejected(scratch.Write("traversal.gltf", Gltf1Buffer("../secret.bin")), nullptr, "secret.bin");
    ExpectTextureUriKept(scratch.Write("image_ok.gltf", Gltf1Image("tex/ok.png")), "tex/ok.png");
    ExpectFileRejected(scratch.Write("image_bad.gltf", Gltf1Image("../secret.png")), nullptr, "secret.png");

    ExpectMemoryRejected(Gltf1Buffer("/etc/passwd"), nullptr);
    ExpectMemoryLoads(Gltf1Buffer(kDataUri36Zeros));
}
