/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team


All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

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

----------------------------------------------------------------------
*/

/**
 * @file ISM2FileData.h
 * @brief Definition of in-memory structs for Compile Heart's ISM2 file format
 *
 * The specification has been taken from:
 * - Random Talking Bush's ISM2 3DS Max import script
 */

#ifndef AI_ISM2FILEHELPER_H_INC
#define AI_ISM2FILEHELPER_H_INC

#include <assimp/Compiler/pushpack1.h>
#include <stdint.h>

namespace Assimp {
namespace ISM2 {

#define AI_ISM2_MAGIC "ISM2"

// -------------------------------------------------------------------------------------
/** \enum Section
 *  \brief Known section IDs for ISM2's various data sections. Not all are yet known.
 */
enum Section {
    Section_Bones = 3,
    Section_VertexHeaderHeader = 10,
    Section_VertexBlockHeader,
    Section_BoneTranslation = 20,
    Section_BoneScale,
    Section_Strings = 33,
    Section_Textures = 46,
    Section_BoneMatrices = 50,
    Section_PolygonBlock = 69,
    Section_Polygon,
    Section_SurfaceOffsets = 76,
    Section_VertexBlock = 89,
    Section_BoneTransforms = 91,
    Section_BoneParenting,
    Section_BoneX,
    Section_BoneY,
    Section_BoneZ,
    Section_Materials = 97,
    Section_BoneRotationX = 103,
    Section_BoneRotationY,
    Section_BoneRotationZ,
    Section_BoundingBox = 110,
    Section_CollisionFlag = 112,
    Section_CollisionRadius,
    Section_PhysicsFlag,
    Section_PhysicsRadius,
    Section_PhysicsCost,
    Section_PhysicsMass,
    Section_PhysicsExpand,
    Section_PhysicsShapeMemory
};

// -------------------------------------------------------------------------------------
/** \struct ModelHeaderMeta
 *  \brief Data structure for ISM2 model header metadata
 */
struct ModelHeaderMeta {
    char ISM2[4];
    uint8_t version[4];
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct ModelHeader
 *  \brief Data structure for ISM2 model header. Unlike the metadata, this is endian dependent.
 */
struct ModelHeader {
    uint32_t _08;
    uint32_t _0C;
    uint32_t fileSize;

    //! Use this field to determine endianness
    uint32_t sectionCount;

    uint32_t _18;
    uint32_t _1C;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct BoneDataHeader
 *  \brief Data structure for ISM2 bone data header.
 */
struct BoneDataHeader {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t total;
    uint32_t dataStringIndex[2];
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct BoneHeader
 *  \brief Data structure for ISM2 bone header.
 */
struct BoneHeader {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t headerTotal;
    uint32_t nameStringIndex[2];
    uint32_t _14;
    uint32_t _18;
    uint32_t parentOffset;
    uint32_t _20;
    uint32_t _24;
    uint32_t _28;
    int32_t id;
    uint32_t _30;
    uint32_t _34;
    uint32_t _38;
    uint32_t _3C;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct SurfaceOffsetsHeader
 *  \brief Data structure for ISM2 surface offsets header.
 */
struct SurfaceOffsetsHeader {
    uint32_t headerSize;
    uint32_t total;
    uint32_t nameStringIndex;
    uint32_t _0C;
    uint32_t _10;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct SurfaceHeader
 *  \brief Data structure for ISM2 surface header.
 */
struct SurfaceHeader {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t total;
    uint32_t materialNameStringIndex;
    uint32_t textureNameStringIndex;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct TransformHeader
 *  \brief Data structure for ISM2 transform header.
 */
struct TransformHeader {
    uint32_t size;
    uint32_t total;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct VertexBlockHeader
 *  \brief Data structure for ISM2 vertex block header.
 */
struct VertexBlockHeader {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t headerTotal;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct VertexHeaderHeader
 *  \brief Data structure for ISM2 *vertex header* header.
 */
struct VertexHeaderHeader {
    uint32_t headerSize;
    uint32_t headerTotal;
    uint32_t _08;
    uint32_t _0C;
    uint32_t _10;
    uint32_t _14;
    uint32_t _18;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct PolygonBlockHeader
 *  \brief Data structure for ISM2 polygon block header
 */
struct PolygonBlockHeader {
    uint32_t dataSize;
    uint32_t dataTotal;
    uint32_t nameStringIndex;
    uint32_t _0C; // always blank?
    uint32_t _10;
    uint32_t _14;
    uint32_t polygonTotal;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct PolygonHeader
 *  \brief Data structure for ISM2 polygon header
 */
struct PolygonHeader {
    uint32_t size;
    uint32_t total;
    uint16_t type[2];
    uint32_t _0C; // always blank?
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct VertexHeader
 *  \brief Data structure for ISM2 vertex header
 */
struct VertexHeader {
    uint32_t length;
    uint32_t total;
    uint16_t type[2];
    uint32_t count;
    uint32_t size;
    uint32_t _14;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct VertexOffsetHeader
 *  \brief Data structure for ISM2 vertex offset header
 */
struct VertexOffsetHeader {
    uint32_t _00;
    uint32_t _04;
    uint32_t _08;
    uint32_t _0C;
    uint32_t _10;
    uint32_t startOffset;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct Vertex
 *  \brief Data structure for ISM2 vertex
 */
struct Vertex {
    float position[3];
    uint16_t normal1[3];
    uint16_t textureCoordX;
    uint16_t normal2[3];
    uint16_t textureCoordY;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct StringHeader
 *  \brief Data structure for ISM2 string header
 */
struct StringHeader {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t total;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct TextureHeader
 *  \brief Data structure for ISM2 texture header
 */
struct TextureHeader {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t total;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct Texture
 *  \brief Data structure for ISM2 texture reference
 */
struct Texture {
    uint32_t sectionType;
    uint32_t dataStringIndex[3];
    uint32_t nameStringIndex;
    uint32_t _18;
    uint32_t _1C;
    uint32_t _20;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct MaterialHeader
 *  \brief Data structure for ISM2 material header
 */
struct MaterialHeader {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t total;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct MaterialA
 *  \brief Data structure for ISM2 material (part A)
 */
struct MaterialA {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t total;
    uint32_t nameStringIndex;
    uint32_t stringIndex[2];
    uint32_t _18; // always blank?
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct MaterialB
 *  \brief Data structure for ISM2 material (part B)
 */
struct MaterialB {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t total;
    uint32_t cOffset;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct MaterialC
 *  \brief Data structure for ISM2 material (part C)
 */
struct MaterialC {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t total;
    uint32_t stringIndex;
    uint32_t _10; // always blank?
    uint32_t dOffset;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct MaterialD
 *  \brief Data structure for ISM2 material (part D)
 */
struct MaterialD {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t total;
    uint32_t _0C;
    uint32_t _10;
    uint32_t _14; // always blank?
    uint32_t eOffset;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct MaterialE
 *  \brief Data structure for ISM2 material (part E)
 */
struct MaterialE {
    uint32_t sectionType;
    uint32_t headerSize;
    uint32_t total;
    uint32_t _0C;
    uint32_t _10;
    uint32_t _14; // always blank?
    uint32_t fOffset;
} PACK_STRUCT;

#include <assimp/Compiler/poppack1.h>

inline
float wtof(uint16_t input16)
{
    uint16_t sign = input16 & 0x8000;
    uint16_t exp = ((input16 & 0x7C00) >> 10) + 127;
    uint16_t frac = input16 & 0x3FF;
    uint32_t out = ((frac << 13) | (exp << 23)) | (sign << 31);

    // fix rounding down to 0 properly for rigging
    if (out == 931135488) out = 0;

    return *reinterpret_cast<float*>(&out);
}

}
}

#endif // AI_ISM2FILEHELPER_H_INC
