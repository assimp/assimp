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
#include "Common/assbin_chunks.h"
#include "UnitTestPCH.h"
#include <assimp/postprocess.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <cstdint>
#include <string>
#include <vector>

using namespace Assimp;

#ifndef ASSIMP_BUILD_NO_EXPORT

class utAssbinImportExport : public AbstractImportExportBase {
public:
    bool importerTest() override {
        Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/spider.obj", aiProcess_ValidateDataStructure);

        Exporter exporter;
        EXPECT_EQ(aiReturn_SUCCESS, exporter.Export(scene, "assbin", ASSIMP_TEST_MODELS_DIR "/OBJ/spider_out.assbin"));
        const aiScene *newScene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/spider_out.assbin", aiProcess_ValidateDataStructure);

        return newScene != nullptr;
    }
};

TEST_F(utAssbinImportExport, import3ExportAssbinDFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utAssbinImportExport, rejectOversizedNodeNameLengthInAssbin) {
    Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/OBJ/spider.obj", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);

    Exporter exporter;
    const aiExportDataBlob *blob = exporter.ExportToBlob(scene, "assbin");
    ASSERT_NE(nullptr, blob);
    ASSERT_NE(nullptr, blob->data);
    ASSERT_GT(blob->size, 0u);

    const auto *blobBytes = static_cast<const uint8_t *>(blob->data);
    std::vector<uint8_t> corruptedBlob(blobBytes, blobBytes + blob->size);
    ASSERT_GT(corruptedBlob.size(), static_cast<size_t>(ASSBIN_HEADER_LENGTH));

    auto readUint32LE = [&corruptedBlob](size_t offset) -> uint32_t {
        return static_cast<uint32_t>(corruptedBlob[offset + 0]) |
                (static_cast<uint32_t>(corruptedBlob[offset + 1]) << 8u) |
                (static_cast<uint32_t>(corruptedBlob[offset + 2]) << 16u) |
                (static_cast<uint32_t>(corruptedBlob[offset + 3]) << 24u);
    };

    auto writeUint32LE = [&corruptedBlob](size_t offset, uint32_t value) {
        corruptedBlob[offset + 0] = static_cast<uint8_t>(value & 0xffu);
        corruptedBlob[offset + 1] = static_cast<uint8_t>((value >> 8u) & 0xffu);
        corruptedBlob[offset + 2] = static_cast<uint8_t>((value >> 16u) & 0xffu);
        corruptedBlob[offset + 3] = static_cast<uint8_t>((value >> 24u) & 0xffu);
    };

    ASSERT_GE(corruptedBlob.size(), static_cast<size_t>(ASSBIN_HEADER_LENGTH) + sizeof(uint32_t) * 2u);
    ASSERT_EQ(static_cast<uint32_t>(ASSBIN_CHUNK_AISCENE), readUint32LE(ASSBIN_HEADER_LENGTH));

    const uint32_t sceneChunkSize = readUint32LE(ASSBIN_HEADER_LENGTH + sizeof(uint32_t));
    const size_t sceneChunkDataOffset = static_cast<size_t>(ASSBIN_HEADER_LENGTH) + sizeof(uint32_t) * 2u;
    const size_t sceneChunkEnd = sceneChunkDataOffset + static_cast<size_t>(sceneChunkSize);
    ASSERT_LE(sceneChunkEnd, corruptedBlob.size());

    size_t nodeChunkOffset = corruptedBlob.size();
    for (size_t offset = sceneChunkDataOffset; offset + sizeof(uint32_t) * 3u <= sceneChunkEnd; ++offset) {
        if (readUint32LE(offset) != static_cast<uint32_t>(ASSBIN_CHUNK_AINODE)) {
            continue;
        }

        const uint32_t chunkSize = readUint32LE(offset + sizeof(uint32_t));
        const size_t chunkEnd = offset + sizeof(uint32_t) * 2u + static_cast<size_t>(chunkSize);
        if (chunkEnd > sceneChunkEnd) {
            continue;
        }

        const size_t lengthOffset = offset + sizeof(uint32_t) * 2u;
        const uint32_t originalLength = readUint32LE(lengthOffset);
        if (lengthOffset + sizeof(uint32_t) + static_cast<size_t>(originalLength) > chunkEnd) {
            continue;
        }

        nodeChunkOffset = offset;
        break;
    }

    ASSERT_NE(corruptedBlob.size(), nodeChunkOffset);

    // Corrupt aiString length to exceed AI_MAXLEN to simulate malformed input.
    const size_t stringLengthOffset = nodeChunkOffset + sizeof(uint32_t) * 2u;
    writeUint32LE(stringLengthOffset, static_cast<uint32_t>(AI_MAXLEN));

    Importer corruptedImporter;
    const aiScene *corruptedScene = corruptedImporter.ReadFileFromMemory(corruptedBlob.data(), corruptedBlob.size(), 0, "assbin");
    EXPECT_EQ(nullptr, corruptedScene);
    EXPECT_NE(std::string::npos,
            std::string(corruptedImporter.GetErrorString())
                    .find("Invalid aiString length"));
}

#endif // #ifndef ASSIMP_BUILD_NO_EXPORT
