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
#include <array>

using namespace Assimp;

TEST(utCOBImporter, importDwarfASCII) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/dwarf_ascii.cob", aiProcess_ValidateDataStructure);
    // FIXME: this is wrong, it should succeed
    // change to ASSERT_NE after it's been fixed
    ASSERT_EQ(nullptr, scene);
}

TEST(utCOBImporter, importDwarf) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/dwarf.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importMoleculeASCII) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/molecule_ascii.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importMolecule) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/molecule.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importSpider43ASCII) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/spider_4_3_ascii.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importSpider43) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/spider_4_3.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importSpider66ASCII) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/spider_6_6_ascii.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importSpider66) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/COB/spider_6_6.cob", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utCOBImporter, importInvalidHeader) {
    Assimp::Importer importer;
    constexpr char Header[] = "Caligari ??LZSCALEb";
    const aiScene *scene = importer.ReadFileFromMemory(Header, sizeof(Header), aiProcess_ValidateDataStructure);
    ASSERT_EQ(nullptr, scene);
}

namespace {

std::vector<char> createCOBBinaryHeader() {
    std::vector<char> data(32, ' ');
    const char magic[] = "Caligari V00.01BLH";
    std::memcpy(data.data(), magic, sizeof(magic) - 1);
    data[31] = '\n';
    return data;
}

void appendChunk(std::vector<char> &data, const char type[4], uint32_t id, const std::array<char, 4> &sizeBytes, size_t payloadZeros) {
    data.insert(data.end(), type, type + 4);
    data.insert(data.end(), 4, 0);
    data.push_back(static_cast<char>(id & 0xFF));
    data.push_back(static_cast<char>((id >> 8) & 0xFF));
    data.push_back(static_cast<char>((id >> 16) & 0xFF));
    data.push_back(static_cast<char>((id >> 24) & 0xFF));
    data.insert(data.end(), 4, 0);
    data.insert(data.end(), sizeBytes.begin(), sizeBytes.end());
    if (payloadZeros > 0) {
        data.insert(data.end(), payloadZeros, 0);
    }
}

} // namespace

TEST(utCOBImporter, chunkGuardLargeSizeNoSignedOverflow) {
    Assimp::Importer importer;
    auto data = createCOBBinaryHeader();
    appendChunk(data, "PolH", 0, { { 0x00, 0x00, 0x00, static_cast<char>(0x80) } }, 112);

    const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "cob");
    EXPECT_EQ(nullptr, scene);
}

TEST(utCOBImporter, chunkGuardMaxMinusOneSizeNoSignedOverflow) {
    Assimp::Importer importer;
    auto data = createCOBBinaryHeader();
    appendChunk(data, "PolH", 0, { { static_cast<char>(0xFE), static_cast<char>(0xFF), static_cast<char>(0xFF), static_cast<char>(0xFF) } }, 112);

    const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "cob");
    EXPECT_EQ(nullptr, scene);
}

TEST(utCOBImporter, chunkGuardValidChunkFollowedByEndChunk) {
    Assimp::Importer importer;
    auto data = createCOBBinaryHeader();
    appendChunk(data, "PolH", 1, { { 112, 0x00, 0x00, 0x00 } }, 112);
    appendChunk(data, "END ", 0, { { 0, 0, 0, 0 } }, 0);

    const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "cob");
    EXPECT_NE(nullptr, scene);
}

TEST(utCOBImporter, chunkGuardValidChunkWithTrailerData) {
    Assimp::Importer importer;
    auto data = createCOBBinaryHeader();
    appendChunk(data, "PolH", 1, { { 120, 0x00, 0x00, 0x00 } }, 112);
    data.insert(data.end(), 8, 0x7F);
    appendChunk(data, "END ", 0, { { 0, 0, 0, 0 } }, 0);

    const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "cob");
    EXPECT_NE(nullptr, scene);
}

TEST(utCOBImporter, unsupportedChunkBinaryLargeSizeNoSignedOverflow) {
    Assimp::Importer importer;
    auto data = createCOBBinaryHeader();
    appendChunk(data, "FooB", 0, { { 0x00, 0x00, 0x00, static_cast<char>(0x80) } }, 64);

    const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "cob");
    EXPECT_EQ(nullptr, scene);
}

TEST(utCOBImporter, unsupportedChunkAsciiLargeSizeNoSignedOverflow) {
    Assimp::Importer importer;
    std::string data = "Caligari V00.01ALH               \n";
    data += "FooB V00.01 Id 0000000001 Parent 0000000000 Size 2147483648\n";
    data.append(64, ' ');

    const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "cob");
    EXPECT_EQ(nullptr, scene);
}
