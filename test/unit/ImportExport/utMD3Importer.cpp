/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

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
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <cstring>
#include <vector>

using namespace Assimp;

TEST(utMD3Importer, importWatercan) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/MD3/watercan.md3", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utMD3Importer, importWatercan_dmg) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/MD3/watercan_dmg.md3", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utMD3Importer, importEuropean_fnt_v2) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/MD3/q3root/models/mapobjects/kt_kubalwagon/european_fnt_v2.md3", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

namespace {

// Field offsets inside the first MD3 surface header, which starts right
// after the 108-byte file header.
constexpr size_t SurfaceStart = 108;
constexpr size_t SurfaceNumVertices = SurfaceStart + 80;
constexpr size_t SurfaceNumTriangles = SurfaceStart + 84;
constexpr size_t SurfaceOfsTriangles = SurfaceStart + 88;
constexpr size_t SurfaceOfsST = SurfaceStart + 96;
constexpr size_t SurfaceOfsXyzNormal = SurfaceStart + 100;
constexpr size_t SurfaceOfsEnd = SurfaceStart + 104;

void putUInt32(std::vector<uint8_t> &data, size_t offset, uint32_t value) {
    for (size_t i = 0; i < 4; ++i) {
        data[offset + i] = static_cast<uint8_t>(value >> (8 * i));
    }
}

// 400-byte MD3 with one frame and numSurfaces zeroed surface headers.
std::vector<uint8_t> createMD3(uint32_t numSurfaces) {
    constexpr uint32_t fileSize = 400;
    std::vector<uint8_t> data(fileSize, 0);
    std::memcpy(data.data(), "IDP3", 4);
    putUInt32(data, 4, 15); // VERSION
    putUInt32(data, 76, 1); // NUM_FRAMES
    putUInt32(data, 84, numSurfaces); // NUM_SURFACES
    putUInt32(data, 100, static_cast<uint32_t>(SurfaceStart)); // OFS_SURFACES
    putUInt32(data, 104, fileSize); // OFS_EOF
    return data;
}

} // namespace

TEST(utMD3Importer, importSurfaceHeaderBeforeBuffer) {
    // The first surface is empty and its signed OFS_END moves the second
    // surface header 100 bytes in front of the file buffer.
    std::vector<uint8_t> data = createMD3(2);
    putUInt32(data, SurfaceOfsEnd, static_cast<uint32_t>(-208));

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "md3");
    EXPECT_EQ(nullptr, scene);
}

TEST(utMD3Importer, importSurfaceOffsetWrapsAt4GiB) {
    // ofs + OFS_TRIANGLES wraps to 0 in 32 bits, but the triangle list is
    // read from the surface pointer plus OFS_TRIANGLES, 4 GiB past the buffer.
    std::vector<uint8_t> data = createMD3(1);
    putUInt32(data, SurfaceNumVertices, 1);
    putUInt32(data, SurfaceNumTriangles, 1);
    putUInt32(data, SurfaceOfsTriangles, 0u - static_cast<uint32_t>(SurfaceStart));
    putUInt32(data, SurfaceOfsST, 108);
    putUInt32(data, SurfaceOfsXyzNormal, 108);
    putUInt32(data, SurfaceOfsEnd, 108);

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(data.data(), data.size(), 0, "md3");
    EXPECT_EQ(nullptr, scene);
}
