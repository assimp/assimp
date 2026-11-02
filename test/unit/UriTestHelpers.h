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
#pragma once

#include "UnitTestPCH.h"

#include <assimp/DefaultIOSystem.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace Assimp {
namespace UriTest {

// Three float32 vertices: (0,0,0), (1,0,0), (0,1,0).
inline constexpr unsigned char kTriangleBytes[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00
};

// 36 bytes of 0x11 placed outside the asset directory; must never be read.
inline constexpr unsigned char kSecretBytes[36] = {
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
};

// Embedded buffer holding 36 zero bytes.
inline constexpr char kDataUri36Zeros[] =
        "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

// IOSystem that records every path the importer tries to open.
class RecordingIOSystem final : public IOSystem {
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
    DefaultIOSystem mInner;
};

// Joins a directory and a file name with the platform separator.
inline std::string JoinPath(const std::string &dir, const std::string &name) {
    return (std::filesystem::path(dir) / name).string();
}

// Creates a directory (and missing parents) accessible only by the owner.
inline bool EnsureDir(const std::string &path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        return false;
    }
    std::filesystem::permissions(path, std::filesystem::perms::owner_all,
            std::filesystem::perm_options::replace, ec);
    return !ec;
}

// Creates a uniquely named owner-only scratch directory under the working directory.
inline std::string MakeScratchDir(const std::string &prefix) {
    std::error_code ec;
    const std::filesystem::path base = std::filesystem::current_path(ec);
    if (ec) {
        return std::string();
    }
    std::random_device random;
    for (int attempt = 0; attempt < 16; ++attempt) {
        const std::filesystem::path dir = base / (prefix + std::to_string(random()));
        if (std::filesystem::create_directory(dir, ec)) {
            std::filesystem::permissions(dir, std::filesystem::perms::owner_all,
                    std::filesystem::perm_options::replace, ec);
            if (ec) {
                std::filesystem::remove_all(dir, ec);
                return std::string();
            }
            return dir.string();
        }
        if (ec) {
            return std::string();
        }
    }
    return std::string();
}

// Removes a directory tree when the test scope ends, even after a failed assertion.
struct TempDirGuard {
    explicit TempDirGuard(std::string dir) : path(std::move(dir)) {}
    ~TempDirGuard() {
        if (!path.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    }
    TempDirGuard(const TempDirGuard &) = delete;
    TempDirGuard &operator=(const TempDirGuard &) = delete;

    std::string path;
};

// Writes raw bytes to a file readable and writable only by the owner.
inline bool WriteBytes(const std::string &path, const void *data, size_t length) {
    {
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            return false;
        }
        out.write(static_cast<const char *>(data), static_cast<std::streamsize>(length));
        if (!out) {
            return false;
        }
    }
    std::error_code ec;
    std::filesystem::permissions(path,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace, ec);
    return !ec;
}

// Writes a text file readable and writable only by the owner.
inline bool WriteText(const std::string &path, const std::string &text) {
    return WriteBytes(path, text.data(), text.size());
}

// Escapes backslashes and quotes for embedding in a JSON string.
inline std::string JsonEscape(const std::string &in) {
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

// Returns true if any recorded open path contains the given fragment.
inline bool OpenedContains(const RecordingIOSystem &io, const char *needle) {
    for (const std::string &path : io.opened) {
        if (path.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Scratch layout: <root>/secret.bin next to <root>/pkg/triangle.bin, removed on destruction.
struct ScratchPackage {
    explicit ScratchPackage(const std::string &prefix) :
            root(MakeScratchDir(prefix)), guard(root), pkg(JoinPath(root, "pkg")) {}

    // Creates the package directory and writes the secret and triangle files.
    bool Populate() const {
        return !root.empty() && EnsureDir(pkg) &&
               WriteBytes(JoinPath(root, "secret.bin"), kSecretBytes, sizeof(kSecretBytes)) &&
               WriteBytes(JoinPath(pkg, "triangle.bin"), kTriangleBytes, sizeof(kTriangleBytes));
    }

    // Writes a file inside the package directory and returns its path, or an empty string.
    std::string Write(const std::string &name, const std::string &text) const {
        const std::string path = JoinPath(pkg, name);
        return WriteText(path, text) ? path : std::string();
    }

    const std::string root;
    const TempDirGuard guard;
    const std::string pkg;
};

// Expects the scene to hold the reference triangle.
inline void ExpectVertices(const aiScene *scene) {
    ASSERT_NE(nullptr, scene);
    ASSERT_EQ(1u, scene->mNumMeshes);
    ASSERT_NE(nullptr, scene->mMeshes[0]);
    ASSERT_EQ(3u, scene->mMeshes[0]->mNumVertices);
    EXPECT_EQ(ai_real(0), scene->mMeshes[0]->mVertices[0].x);
    EXPECT_EQ(ai_real(1), scene->mMeshes[0]->mVertices[1].x);
    EXPECT_EQ(ai_real(0), scene->mMeshes[0]->mVertices[1].y);
    EXPECT_EQ(ai_real(1), scene->mMeshes[0]->mVertices[2].y);
}

// Expects the file to import and hold the reference triangle.
inline void ExpectFileLoads(const std::string &path) {
    Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene) << path << ": " << importer.GetErrorString();
    ASSERT_NO_FATAL_FAILURE(ExpectVertices(scene));
}

// Expects the import to fail (with errorFragment, if given) without opening the forbidden file.
inline void ExpectFileRejected(const std::string &path, const char *errorFragment, const char *forbidden) {
    Importer importer;
    auto *io = new RecordingIOSystem();
    importer.SetIOHandler(io);
    const aiScene *scene = importer.ReadFile(path, aiProcess_ValidateDataStructure);
    EXPECT_EQ(nullptr, scene) << path << ": " << importer.GetErrorString();
    if (errorFragment != nullptr) {
        const std::string error = importer.GetErrorString();
        EXPECT_NE(std::string::npos, error.find(errorFragment)) << path << ": " << error;
    }
    EXPECT_FALSE(OpenedContains(*io, forbidden)) << path;
}

// Expects the file to import and keep the given texture URI on the first diffuse slot.
inline void ExpectTextureUriKept(const std::string &path, const char *uri) {
    Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene) << path << ": " << importer.GetErrorString();
    ASSERT_GT(scene->mNumMaterials, 0u);
    aiString texPath;
    EXPECT_EQ(aiReturn_SUCCESS, scene->mMaterials[0]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath));
    EXPECT_STREQ(uri, texPath.C_Str());
}

// Expects an in-memory import to fail (with errorFragment, if given) without opening any file.
inline void ExpectMemoryRejected(const std::string &text, const char *errorFragment) {
    Importer importer;
    auto *io = new RecordingIOSystem();
    importer.SetIOHandler(io);
    const aiScene *scene = importer.ReadFileFromMemory(
            text.data(), text.size(), aiProcess_ValidateDataStructure, "gltf");
    EXPECT_EQ(nullptr, scene) << importer.GetErrorString();
    if (errorFragment != nullptr) {
        const std::string error = importer.GetErrorString();
        EXPECT_NE(std::string::npos, error.find(errorFragment)) << error;
    }
    EXPECT_TRUE(io->opened.empty());
}

// Expects an in-memory import to succeed with a three-vertex mesh.
inline void ExpectMemoryLoads(const std::string &text) {
    Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(
            text.data(), text.size(), aiProcess_ValidateDataStructure, "gltf");
    ASSERT_NE(nullptr, scene) << importer.GetErrorString();
    ASSERT_EQ(1u, scene->mNumMeshes);
    EXPECT_EQ(3u, scene->mMeshes[0]->mNumVertices);
}

} // namespace UriTest
} // namespace Assimp
