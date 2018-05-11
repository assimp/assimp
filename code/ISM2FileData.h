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
 * - https://github.com/haolink/ISM2Import/
 */

#ifndef AI_ISM2FILEHELPER_H_INC
#define AI_ISM2FILEHELPER_H_INC

#include <assimp/Compiler/pushpack1.h>
#include <stdint.h>

namespace Assimp {
namespace ISM2 {

#define AI_ISM2_MAGIC 0x324D5349

// -------------------------------------------------------------------------------------
/** \enum Section
 *  \brief Known section IDs for ISM2's various data sections. Not all are yet known.
 */
enum Section: uint32_t {
    Section_Bones = 3,
    Section_VertexMetaHeader = 10,
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
/** \struct ModelHeader
 *  \brief Data structure for ISM2 model header
 */
struct ModelHeader {
    char ISM2[4];
    uint8_t version[4];
    uint32_t _3;
    uint32_t _4;
    uint32_t fileSize;

    //! Use this field to determine endianness
    uint32_t sectionCount;

    uint32_t _7;
    uint32_t _8;
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
    uint32_t _1;
    uint32_t _2;
    uint32_t parentOffset;
    uint32_t _4;
    uint32_t _5;
    uint32_t _6;
    int32_t id;
    uint32_t _8;
    uint32_t _9;
    uint32_t _10;
    uint32_t _11;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct SurfaceOffsetsHeader
 *  \brief Data structure for ISM2 surface offsets header.
 */
struct SurfaceOffsetsHeader {
    uint32_t headerSize;
    uint32_t total;
    uint32_t nameStringIndex;
    uint32_t _1;
    uint32_t _2;
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
/** \struct VertexMetaHeader
 *  \brief Data structure for ISM2 vertex meta header.
 */
struct VertexMetaHeader {
    uint32_t headerSize;
    uint32_t headerTotal;
    uint32_t _1;
    uint32_t _2;
    uint32_t _3;
    uint32_t _4;
    uint32_t _5;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct PolygonBlockHeader
 *  \brief Data structure for ISM2 polygon block header
 */
struct PolygonBlockHeader {
    uint32_t dataSize;
    uint32_t dataTotal;
    uint32_t nameStringIndex;
    uint32_t _blank;
    uint32_t _1;
    uint32_t _2;
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
    uint32_t _blank;
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
    uint32_t _stuff;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct VertexOffsetHeader
 *  \brief Data structure for ISM2 vertex offset header
 */
struct VertexOffsetHeader {
    uint32_t _1;
    uint32_t _2;
    uint32_t _3;
    uint32_t _4;
    uint32_t _5;
    uint32_t startOffset;
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct Vertex1
 *  \brief Data structure for ISM2 vertex
 */
struct Vertex1 {
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
/** \struct Vertex3Size16
 *  \brief Data structure for ISM2 vertex
 */
struct Vertex3Size16 {
    uint8_t bones[4];
    uint16_t weights[4];
    uint8_t _3[4];
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct Vertex3Size32V1
 *  \brief Data structure for ISM2 vertex
 */
struct Vertex3Size32V1 {
    uint8_t bones[4];
    float weights[4];
    uint8_t _3[12];
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct Vertex3Size32V2
 *  \brief Data structure for ISM2 vertex
 */
struct Vertex3Size32V2 {
    uint16_t bones[4];
    float weights[4];
    uint8_t _3[8];
} PACK_STRUCT;

// -------------------------------------------------------------------------------------
/** \struct Vertex3Size48
 *  \brief Data structure for ISM2 vertex
 */
struct Vertex3Size48 {
    uint16_t bones[8];
    float weights[8];
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
    uint32_t _1;
    uint32_t _2;
    uint32_t _3;
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
    uint32_t _blank;
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
    uint32_t _blank;
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
    uint32_t _a;
    uint32_t _b;
    uint32_t _blank;
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
    uint32_t _a;
    uint32_t _b;
    uint32_t _blank;
    uint32_t fOffset;
} PACK_STRUCT;

#include <assimp/Compiler/poppack1.h>

struct SectionData {
    uint32_t *types;
    uint32_t *offsets;

    SectionData(): types(nullptr), offsets(nullptr)
    {
    }

    ~SectionData() {
        if (types) delete[] types;
        if (offsets) delete[] offsets;
    }
};

struct StringBlock {
    StringHeader header;
    uint32_t *offsets;
    std::string *strings;

    StringBlock(): offsets(nullptr), strings(nullptr)
    {
    }

    ~StringBlock() {
        if (offsets) delete[] offsets;
        if (strings) delete[] strings;
    }
};

union TransformSectionData {
    float translation[3];
    float scale[3];
    float x[4];
    float y[4];
    float z[4];
    float xRotate[4];
    float yRotate[4];
    float zRotate[4];
};

struct TransformSection {
    uint32_t type;
    TransformSectionData data;
};

struct BoneSection {
    uint32_t type;
    SurfaceOffsetsHeader surfaceOffsetsHeader;
    uint32_t *surfaceOffsets;
    SurfaceHeader *surfaces;
    TransformHeader transformHeader;
    uint32_t *transformOffsets;
    TransformSection *transformSections;

    BoneSection(): surfaceOffsets(nullptr), surfaces(nullptr),
        transformOffsets(nullptr), transformSections(nullptr)
    {
    }

    ~BoneSection() {
        if (surfaceOffsets) delete[] surfaceOffsets;
        if (surfaces) delete[] surfaces;
        if (transformOffsets) delete[] transformOffsets;
        if (transformSections) delete[] transformSections;
    }
};

struct Bone {
    BoneHeader header;
    uint32_t *sectionOffsets;
    BoneSection *sections;

    Bone(): sectionOffsets(nullptr), sections(nullptr)
    {
    }

    ~Bone() {
        if (sectionOffsets) delete[] sectionOffsets;
        if (sections) delete[] sections;
    }
};

struct BoneBlock {
    BoneDataHeader header;
    uint32_t *offsets;
    Bone *bones;

    BoneBlock(): offsets(nullptr), bones(nullptr) {
    }
    ~BoneBlock() {
        if (offsets) delete[] offsets;
        if (bones) delete[] bones;
    }
};

struct Polygon {
    uint32_t type;
    PolygonHeader header;
    uint32_t (*faces)[3];

    Polygon(): faces(nullptr)
    {
    }

    ~Polygon() {
        if (faces) delete[] faces;
    }
};

struct PolygonBlock {
    PolygonBlockHeader header;
    uint32_t *offsets;
    Polygon *polygons;

    PolygonBlock(): offsets(nullptr), polygons(nullptr)
    {
    }

    ~PolygonBlock() {
        if (offsets) delete[] offsets;
        if (polygons) delete[] polygons;
    }
};

union Vertex {
    Vertex1 type1;
    Vertex3Size16 type3Size16;
    Vertex3Size32V1 type3Size32V1;
    Vertex3Size32V2 type3Size32V2;
    Vertex3Size48 type3Size48;
};

struct VertexData {
    VertexHeader header;
    uint32_t *offsets;
    VertexOffsetHeader *offsetHeaders;
    Vertex *vertices;

    VertexData(): offsets(nullptr), offsetHeaders(nullptr), vertices(nullptr)
    {
    }

    ~VertexData() {
        if (offsets) delete[] offsets;
        if (offsetHeaders) delete[] offsetHeaders;
        if (vertices) delete[] vertices;
    }
};

struct VertexHeaderSection {
    uint32_t type;
    PolygonBlock polygonBlock;
    VertexData data;
};

struct VertexBlockSection {
    uint32_t type;
    VertexMetaHeader header;
    uint32_t *offsets;
    VertexHeaderSection *sections;

    VertexBlockSection(): offsets(nullptr), sections(nullptr)
    {
    }

    ~VertexBlockSection() {
        if (offsets) delete[] offsets;
        if (sections) delete[] sections;
    }
};

struct VertexBlock {
    VertexBlockHeader header;
    uint32_t *offsets;
    VertexBlockSection *sections;

    VertexBlock(): offsets(nullptr), sections(nullptr)
    {
    }

    ~VertexBlock() {
        if (offsets) delete[] offsets;
        if (sections) delete[] sections;
    }
};

struct TextureBlock {
    TextureHeader header;
    uint32_t *offsets;
    Texture *textures;

    TextureBlock(): offsets(nullptr), textures(nullptr)
    {
    }

    ~TextureBlock() {
        if (offsets) delete[] offsets;
        if (textures) delete[] textures;
    }
};

struct Material {
    MaterialA a;
    uint32_t bOffset;
    MaterialB b;
    MaterialC c;
    MaterialD d;
    MaterialE e;
    uint32_t textureNameStringIndex;
};

struct MaterialBlock {
    MaterialHeader header;
    uint32_t *offsets;
    Material *materials;

    MaterialBlock(): offsets(nullptr), materials(nullptr)
    {
    }

    ~MaterialBlock() {
        if (offsets) delete[] offsets;
        if (materials) delete[] materials;
    }
};

struct Model {
    ModelHeader header;
    SectionData sectionData;
    StringBlock stringBlock;
    BoneBlock boneBlock;
    VertexBlock vertexBlock;
    TextureBlock textureBlock;
    MaterialBlock materialBlock;
    uint32_t numPolygons;

    Model(): numPolygons(1) {
    }
};

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
