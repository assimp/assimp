/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team

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

/** @file  HL1FileData.h
 *  @brief Definition of in-memory structures for the
 *        Half-Life 1 MDL file format.
 */

#ifndef AI_HL1FILEDATA_INCLUDED
#define AI_HL1FILEDATA_INCLUDED

#include "HalfLifeMDLBaseHeader.h"

#include <assimp/Compiler/pushpack1.h>
#include <assimp/types.h>

namespace Assimp {
namespace MDL {
namespace HalfLife {

using vec3_t = float[3];

struct Header_HL1 : HalfLifeMDLBaseHeader {
    char name[64];
    int32_t length;

    vec3_t eyeposition; // ideal eye position
    vec3_t min; // ideal movement hull size
    vec3_t max;

    vec3_t bbmin; // clipping bounding box
    vec3_t bbmax;

    int32_t unused; // was flags

    int32_t numbones; // bones
    int32_t boneindex;

    int32_t numbonecontrollers; // bone controllers
    int32_t bonecontrollerindex;

    int32_t numhitboxes; // complex bounding boxes
    int32_t hitboxindex;

    int32_t numseq; // animation sequences
    int32_t seqindex;

    int32_t numseqgroups; // demand loaded sequences
    int32_t seqgroupindex;

    int32_t numtextures; // raw textures
    int32_t textureindex;
    int32_t texturedataindex;

    int32_t numskinref; // replaceable textures
    int32_t numskinfamilies;
    int32_t skinindex;

    int32_t numbodyparts;
    int32_t bodypartindex;

    int32_t numattachments; // queryable attachable points
    int32_t attachmentindex;

    int32_t unused2; // was "soundtable"
    int32_t unused3; // was "soundindex"
    int32_t unused4; // was "soundgroups"
    int32_t unused5; // was "soundgroupindex"

    int32_t numtransitions; // animation node to animation node transition graph
    int32_t transitionindex;
} PACK_STRUCT;

// header for demand loaded sequence group data
struct SequenceHeader_HL1 : HalfLifeMDLBaseHeader {
    char name[64];
    int32_t length;
} PACK_STRUCT;

// bones
struct Bone_HL1 {
    char name[32]; // bone name for symbolic links
    int32_t parent; // parent bone
    int32_t unused; // was "flags" -- ??
    int32_t bonecontroller[6]; // bone controller index, -1 == none
    float value[6]; // default DoF values
    float scale[6]; // scale for delta DoF values
} PACK_STRUCT;

// bone controllers
struct BoneController_HL1 {
    int32_t bone; // -1 == 0
    int32_t type; // X, Y, Z, XR, YR, ZR, M
    float start;
    float end;
    int32_t unused; // was "rest" - byte index value at rest
    int32_t index; // 0-3 user set controller, 4 mouth
} PACK_STRUCT;

// intersection boxes
struct Hitbox_HL1 {
    int32_t bone;
    int32_t group; // intersection group
    vec3_t bbmin; // bounding box
    vec3_t bbmax;
} PACK_STRUCT;

//
// demand loaded sequence groups
//
struct SequenceGroup_HL1 {
    char label[32]; // textual name
    char name[64]; // file name
    int32_t unused; // was "cache"  - index pointer
    int32_t unused2; // was "data" -  hack for group 0
} PACK_STRUCT;

// The type of blending for a sequence.
enum SequenceBlendMode_HL1 {
    NoBlend = 1,
    TwoWayBlending = 2,
    FourWayBlending = 4,
};

// sequence descriptions
struct SequenceDesc_HL1 {
    char label[32]; // sequence label

    float fps; // frames per second
    int32_t flags; // looping/non-looping flags

    int32_t activity;
    int32_t actweight;

    int32_t numevents;
    int32_t eventindex;

    int32_t numframes; // number of frames per sequence

    int32_t unused; // was "numpivots" - number of foot pivots
    int32_t unused2; // was "pivotindex"

    int32_t motiontype;
    int32_t motionbone;
    vec3_t linearmovement;
    int32_t unused3; // was "automoveposindex"
    int32_t unused4; // was "automoveangleindex"

    vec3_t bbmin; // per sequence bounding box
    vec3_t bbmax;

    int32_t numblends;
    int32_t animindex; // mstudioanim_t pointer relative to start of sequence group data
                   // [blend][bone][X, Y, Z, XR, YR, ZR]

    int32_t blendtype[2]; // X, Y, Z, XR, YR, ZR
    float blendstart[2]; // starting value
    float blendend[2]; // ending value
    int32_t unused5; // was "blendparent"

    int32_t seqgroup; // sequence group for demand loading

    int32_t entrynode; // transition node at entry
    int32_t exitnode; // transition node at exit
    int32_t nodeflags; // transition rules

    int32_t unused6; // was "nextseq" - auto advancing sequences
} PACK_STRUCT;

// events
struct AnimEvent_HL1 {
    int32_t frame;
    int32_t event;
    int32_t unused; // was "type"
    char options[64];
} PACK_STRUCT;

// attachment
struct Attachment_HL1 {
    char unused[32]; // was "name"
    int32_t unused2; // was "type"
    int32_t bone;
    vec3_t org; // attachment point
    vec3_t unused3[3]; // was "vectors"
} PACK_STRUCT;

struct AnimValueOffset_HL1 {
    unsigned short offset[6];
} PACK_STRUCT;

// animation frames
union AnimValue_HL1 {
    struct {
        uint8_t valid;
        uint8_t total;
    } num PACK_STRUCT;
    short value;
};

// body part index
struct Bodypart_HL1 {
    char name[64];
    int32_t nummodels;
    int32_t base;
    int32_t modelindex; // index into models array
} PACK_STRUCT;

// skin info
struct Texture_HL1 {
    char name[64];
    int32_t flags;
    int32_t width;
    int32_t height;
    int32_t index;
} PACK_STRUCT;

// studio models
struct Model_HL1 {
    char name[64];

    int32_t unused; // was "type"

    float unused2; // was "boundingradius"

    int32_t nummesh;
    int32_t meshindex;

    int32_t numverts; // number of unique vertices
    int32_t vertinfoindex; // vertex bone info
    int32_t vertindex; // vertex vec3_t
    int32_t numnorms; // number of unique surface normals
    int32_t norminfoindex; // normal bone info
    int32_t normindex; // normal vec3_t

    int32_t unused3; // was "numgroups" - deformation groups
    int32_t unused4; // was "groupindex"
} PACK_STRUCT;

// meshes
struct Mesh_HL1 {
    int32_t numtris;
    int32_t triindex;
    int32_t skinref;
    int32_t numnorms; // per mesh normals
    int32_t unused; // was "normindex" - normal vec3_t
} PACK_STRUCT;

struct Trivert {
    short vertindex; // index into vertex array
    short normindex; // index into normal array
    short s, t; // s,t position on skin
} PACK_STRUCT;

#include <assimp/Compiler/poppack1.h>

#if (!defined AI_MDL_HL1_VERSION)
#define AI_MDL_HL1_VERSION 10
#endif
#if (!defined AI_MDL_HL1_MAX_TRIANGLES)
#define AI_MDL_HL1_MAX_TRIANGLES 20000
#endif
#if (!defined AI_MDL_HL1_MAX_VERTICES)
#define AI_MDL_HL1_MAX_VERTICES 2048
#endif
#if (!defined AI_MDL_HL1_MAX_SEQUENCES)
#define AI_MDL_HL1_MAX_SEQUENCES 2048
#endif
#if (!defined AI_MDL_HL1_MAX_SEQUENCE_GROUPS)
#define AI_MDL_HL1_MAX_SEQUENCE_GROUPS 32
#endif
#if (!defined AI_MDL_HL1_MAX_TEXTURES)
#define AI_MDL_HL1_MAX_TEXTURES 100
#endif
#if (!defined AI_MDL_HL1_MAX_SKIN_FAMILIES)
#define AI_MDL_HL1_MAX_SKIN_FAMILIES 100
#endif
#if (!defined AI_MDL_HL1_MAX_BONES)
#define AI_MDL_HL1_MAX_BONES 128
#endif
#if (!defined AI_MDL_HL1_MAX_BODYPARTS)
#define AI_MDL_HL1_MAX_BODYPARTS 32
#endif
#if (!defined AI_MDL_HL1_MAX_MODELS)
#define AI_MDL_HL1_MAX_MODELS 32
#endif
#if (!defined AI_MDL_HL1_MAX_MESHES)
#define AI_MDL_HL1_MAX_MESHES 256
#endif
#if (!defined AI_MDL_HL1_MAX_EVENTS)
#define AI_MDL_HL1_MAX_EVENTS 1024
#endif
#if (!defined AI_MDL_HL1_MAX_BONE_CONTROLLERS)
#define AI_MDL_HL1_MAX_BONE_CONTROLLERS 8
#endif
#if (!defined AI_MDL_HL1_MAX_ATTACHMENTS)
#define AI_MDL_HL1_MAX_ATTACHMENTS 512
#endif

// lighting options
#if (!defined AI_MDL_HL1_STUDIO_NF_FLATSHADE)
#define AI_MDL_HL1_STUDIO_NF_FLATSHADE 0x0001
#endif
#if (!defined AI_MDL_HL1_STUDIO_NF_CHROME)
#define AI_MDL_HL1_STUDIO_NF_CHROME 0x0002
#endif
#if (!defined AI_MDL_HL1_STUDIO_NF_ADDITIVE)
#define AI_MDL_HL1_STUDIO_NF_ADDITIVE 0x0020
#endif
#if (!defined AI_MDL_HL1_STUDIO_NF_MASKED)
#define AI_MDL_HL1_STUDIO_NF_MASKED 0x0040
#endif

} // namespace HalfLife
} // namespace MDL
} // namespace Assimp

#endif // AI_HL1FILEDATA_INCLUDED
