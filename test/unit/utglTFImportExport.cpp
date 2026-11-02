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

#include <assimp/DefaultIOSystem.h>
#include <assimp/Importer.hpp>
#include <assimp/commonMetaData.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cerrno>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#    include <direct.h>
#    include <process.h>
#else
#    include <sys/stat.h>
#    include <unistd.h>
#endif

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


namespace {

const unsigned char kTriangleBytes[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00
};

const char kDataUri36Zeros[] =
        "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

class RecordingIOSystem final : public Assimp::IOSystem {
public:
    std::vector<std::string> opened;

    bool Exists(const char *pFile) const override { return mInner.Exists(pFile); }
    char getOsSeparator() const override { return mInner.getOsSeparator(); }
    IOStream *Open(const char *pFile, const char *pMode) override {
        opened.emplace_back(pFile != nullptr ? pFile : "");
        return mInner.Open(pFile, pMode);
    }
    void Close(IOStream *pFile) override { mInner.Close(pFile); }
    bool ComparePaths(const char *one, const char *second) const override {
        return mInner.ComparePaths(one, second);
    }

private:
    Assimp::DefaultIOSystem mInner;
};

int CurrentPid() {
#ifdef _WIN32
    return _getpid();
#else
    return static_cast<int>(::getpid());
#endif
}

std::string JoinPath(const std::string &dir, const std::string &name) {
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    if (dir.empty() || dir.back() == '/' || dir.back() == '\\') {
        return dir + name;
    }
    return dir + sep + name;
}

bool EnsureDir(const std::string &path) {
#ifdef _WIN32
    if (::_mkdir(path.c_str()) != 0 && errno != EEXIST) {
        return false;
    }
#else
    if (::mkdir(path.c_str(), 0777) != 0 && errno != EEXIST) {
        return false;
    }
#endif
    return true;
}

bool WriteBytes(const std::string &path, const void *data, size_t length) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out.write(static_cast<const char *>(data), static_cast<std::streamsize>(length));
    return static_cast<bool>(out);
}

std::string JsonEscape(const std::string &in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        if (c == '\\' || c == '"') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

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

bool OpenedContains(const RecordingIOSystem &io, const char *needle) {
    for (const std::string &path : io.opened) {
        if (path.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST_F(utglTFImportExport, externalUriIsContained) {
#ifdef _WIN32
    const char *tmp = std::getenv("TEMP");
    if (tmp == nullptr || tmp[0] == '\0') {
        tmp = ".";
    }
    const std::string root = JoinPath(tmp, "assimp-issue6933-gltf1-" + std::to_string(CurrentPid()));
#else
    const std::string root = "/tmp/assimp-issue6933-gltf1-" + std::to_string(CurrentPid());
#endif
    const std::string pkg = JoinPath(root, "pkg");
    ASSERT_TRUE(EnsureDir(root));
    ASSERT_TRUE(EnsureDir(pkg));

    const unsigned char secret[36] = {
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
    };
    ASSERT_TRUE(WriteBytes(JoinPath(root, "secret.bin"), secret, sizeof(secret)));
    ASSERT_TRUE(WriteBytes(JoinPath(pkg, "triangle.bin"), kTriangleBytes, sizeof(kTriangleBytes)));

    const std::string benignText = Gltf1Buffer("triangle.bin");
    const std::string traversalText = Gltf1Buffer("../secret.bin");
    const std::string imageOkText = Gltf1Image("tex/ok.png");
    const std::string imageBadText = Gltf1Image("../secret.png");
    const std::string benign = JoinPath(pkg, "benign.gltf");
    const std::string traversal = JoinPath(pkg, "traversal.gltf");
    const std::string imageOk = JoinPath(pkg, "image_ok.gltf");
    const std::string imageBad = JoinPath(pkg, "image_bad.gltf");
    ASSERT_TRUE(WriteBytes(benign, benignText.data(), benignText.size()));
    ASSERT_TRUE(WriteBytes(traversal, traversalText.data(), traversalText.size()));
    ASSERT_TRUE(WriteBytes(imageOk, imageOkText.data(), imageOkText.size()));
    ASSERT_TRUE(WriteBytes(imageBad, imageBadText.data(), imageBadText.size()));

    {
        Assimp::Importer fileImporter;
        const aiScene *scene = fileImporter.ReadFile(benign, aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene) << fileImporter.GetErrorString();
        ASSERT_EQ(1u, scene->mNumMeshes);
        EXPECT_EQ(3u, scene->mMeshes[0]->mNumVertices);
        EXPECT_EQ(ai_real(1), scene->mMeshes[0]->mVertices[1].x);
    }

    {
        Assimp::Importer fileImporter;
        auto *io = new RecordingIOSystem();
        fileImporter.SetIOHandler(io);
        const aiScene *scene = fileImporter.ReadFile(traversal, aiProcess_ValidateDataStructure);
        EXPECT_EQ(nullptr, scene) << fileImporter.GetErrorString();
        EXPECT_FALSE(OpenedContains(*io, "secret.bin"));
    }

    {
        Assimp::Importer fileImporter;
        const aiScene *scene = fileImporter.ReadFile(imageOk, aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene) << fileImporter.GetErrorString();
        ASSERT_GT(scene->mNumMaterials, 0u);
        aiString texPath;
        EXPECT_EQ(aiReturn_SUCCESS, scene->mMaterials[0]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath));
        EXPECT_STREQ("tex/ok.png", texPath.C_Str());
    }

    {
        Assimp::Importer fileImporter;
        auto *io = new RecordingIOSystem();
        fileImporter.SetIOHandler(io);
        const aiScene *scene = fileImporter.ReadFile(imageBad, aiProcess_ValidateDataStructure);
        EXPECT_EQ(nullptr, scene) << fileImporter.GetErrorString();
        EXPECT_FALSE(OpenedContains(*io, "secret.png"));
    }

    const std::string memoryExternal = Gltf1Buffer("/etc/passwd");
    {
        Assimp::Importer memoryImporter;
        auto *io = new RecordingIOSystem();
        memoryImporter.SetIOHandler(io);
        const aiScene *scene = memoryImporter.ReadFileFromMemory(
                memoryExternal.data(), memoryExternal.size(), aiProcess_ValidateDataStructure, "gltf");
        EXPECT_EQ(nullptr, scene) << memoryImporter.GetErrorString();
        EXPECT_TRUE(io->opened.empty());
    }

    const std::string memoryData = Gltf1Buffer(kDataUri36Zeros);
    {
        Assimp::Importer memoryImporter;
        const aiScene *scene = memoryImporter.ReadFileFromMemory(
                memoryData.data(), memoryData.size(), aiProcess_ValidateDataStructure, "gltf");
        ASSERT_NE(nullptr, scene) << memoryImporter.GetErrorString();
        ASSERT_EQ(1u, scene->mNumMeshes);
        EXPECT_EQ(3u, scene->mMeshes[0]->mNumVertices);
    }
}
