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
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace Assimp;

class utOgreImportExport : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/Ogre/TheThing/Mesh.mesh.xml", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utOgreImportExport, importerTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utOgreImportExport, importMeshWithWrappingIndexBufferSize) {
    std::ifstream in(ASSIMP_TEST_MODELS_NONBSD_DIR "/Ogre/OgreSDK/razor.mesh", std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    constexpr size_t indexCountOffset = 0x4A03; // index count of the submesh
    ASSERT_GT(data.size(), indexCountOffset + sizeof(uint32_t));

    // 0x80000002 sixteen bit indices are 2^32 + 4 bytes, which wraps around to 4 bytes in 32-bit.
    const unsigned char count[sizeof(uint32_t)] = { 0x02, 0x00, 0x00, 0x80 };
    std::memcpy(&data[indexCountOffset], count, sizeof(count));

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "mesh");
    ASSERT_EQ(nullptr, scene);
    EXPECT_NE(std::string::npos, std::string(importer.GetErrorString()).find("larger than the remaining data"));
}

TEST_F(utOgreImportExport, importMeshWithIndexBufferSizeBeyondFile) {
    std::ifstream in(ASSIMP_TEST_MODELS_NONBSD_DIR "/Ogre/OgreSDK/razor.mesh", std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    constexpr size_t indexCountOffset = 0x4A03; // index count of the submesh
    ASSERT_GT(data.size(), indexCountOffset + sizeof(uint32_t));

    // No wraparound here, but about 4 GiB of index data can not be in a 62 KB file.
    const unsigned char count[sizeof(uint32_t)] = { 0xFF, 0xFF, 0xFF, 0x7F };
    std::memcpy(&data[indexCountOffset], count, sizeof(count));

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "mesh");
    ASSERT_EQ(nullptr, scene);
    EXPECT_NE(std::string::npos, std::string(importer.GetErrorString()).find("larger than the remaining data"));
}

TEST_F(utOgreImportExport, importBinaryMeshRazor) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/Ogre/OgreSDK/razor.mesh", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene) << importer.GetErrorString();
    EXPECT_EQ(1u, scene->mNumMeshes);
}
