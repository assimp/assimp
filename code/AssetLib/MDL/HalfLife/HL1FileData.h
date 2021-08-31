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

/** @file  HL1FileData.h
 *  @brief Definition of in-memory structures for the
 *         Half-Life 1 MDL file format.
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

/** \struct Header_HL1
 *  \brief Data structure for the HL1 MDL file header.
 */
struct Header_HL1 : HalfLifeMDLBaseHeader {
    //! The model name.
    char name[64];

    //! The total file size in bytes.
    int32_t length;

    //! Ideal eye position.
    vec3_t eyeposition;

    //! Ideal movement hull size.
    vec3_t min;
    vec3_t max;

    //! Clipping bounding box.
    vec3_t bbmin;
    vec3_t bbmax;

    //! Was "flags".
    int32_t unused;

    //! The number of bones.
    int32_t numbones;

    //! Offset to the first bone chunk.
    int32_t boneindex;

    //! The number of bone controllers.
    int32_t numbonecontrollers;

    //! Offset to the first bone controller chunk.
    int32_t bonecontrollerindex;

    //! The number of hitboxes.
    int32_t numhitboxes;

    //! Offset to the first hitbox chunk.
    int32_t hitboxindex;

    //! The number of sequences.
    int32_t numseq;

    //! Offset to the first sequence description chunk.
    int32_t seqindex;

    //! The number of sequence groups.
    int32_t numseqgroups;

    //! Offset to the first sequence group chunk.
    int32_t seqgroupindex;

    //! The number of textures.
    int32_t numtextures;

    //! Offset to the first texture chunk.
    int32_t textureindex;

    //! Offset to the first texture's image data.
    int32_t texturedataindex;

    //! The number of replaceable textures.
    int32_t numskinref;

    //! The number of skin families.
    int32_t numskinfamilies;

    //! Offset to the first replaceable texture.
    int32_t skinindex;

    //! The number of bodyparts.
    int32_t numbodyparts;

    //! Offset the the first bodypart.
    int32_t bodypartindex;

    //! The number of attachments.
    int32_t numattachments;

    //! Offset the the first attachment chunk.
    int32_t attachmentindex;

    //! Was "soundtable".
    int32_t unused2;

    //! Was "soundindex".
    int32_t unused3;

    //! Was "soundgroups".
    int32_t unused4;

    //! Was "soundgroupindex".
    int32_t unused5;

    //! The number of nodes in the sequence transition graph.
    int32_t numtransitions;

    //! Offset the the first sequence transition.
    int32_t transitionindex;
} PACK_STRUCT;

/** \struct SequenceHeader_HL1
 *  \brief Data structure for the file header of a demand loaded
 *         HL1 MDL sequence group file.
 */
struct SequenceHeader_HL1 : HalfLifeMDLBaseHeader {
    //! The sequence group file name.
    char name[64];

    //! The total file size in bytes.
    int32_t length;
} PACK_STRUCT;

/** \struct Bone_HL1
 *  \brief Data structure for a bone in HL1 MDL files.
 */
struct Bone_HL1 {
    //! The bone name.
    char name[32];

    //! The parent bone index. (-1) If it has no parent.
    int32_t parent;

    //! Was "flags".
    int32_t unused;

    //! Available bone controller per motion type.
    //! (-1) if no controller is available.
    int32_t bonecontroller[6];

    /*! Default position and rotation values where
     * scale[0] = position.X
     * scale[1] = position.Y
     * scale[2] = position.Z
     * scale[3] = rotation.X
     * scale[4] = rotation.Y
     * scale[5] = rotation.Z
     */
    float value[6];

    /*! Compressed scale values where
     * scale[0] = position.X scale
     * scale[1] = position.Y scale
     * scale[2] = position.Z scale
     * scale[3] = rotation.X scale
     * scale[4] = rotation.Y scale
     * scale[5] = rotation.Z scale
     */
    float scale[6];
} PACK_STRUCT;

/** \struct BoneController_HL1
 *  \brief Data structure for a bone controller in HL1 MDL files.
 */
struct BoneController_HL1 {
    //! Bone affected by this controller.
    int32_t bone;

    //! The motion type.
    int32_t type;

    //! The minimum and maximum values.
    float start;
    float end;

    // Was "rest".
    int32_t unused;

    // The bone controller channel.
    int32_t index;
} PACK_STRUCT;

/** \struct Hitbox_HL1
 *  \brief Data structure for a hitbox in HL1 MDL files.
 */
struct Hitbox_HL1 {
    //! The bone this hitbox follows.
    int32_t bone;

    //! The hit group.
    int32_t group;

    //! The hitbox minimum and maximum extents.
    vec3_t bbmin;
    vec3_t bbmax;
} PACK_STRUCT;

/** \struct SequenceGroup_HL1
 *  \brief Data structure for a sequence group in HL1 MDL files.
 */
struct SequenceGroup_HL1 {
    //! A textual name for this sequence group.
    char label[32];

    //! The file name.
    char name[64];

    //! Was "cache".
    int32_t unused;

    //! Was "data".
    int32_t unused2;
} PACK_STRUCT;

//! The type of blending for a sequence.
enum SequenceBlendMode_HL1 {
    NoBlend = 1,
    TwoWayBlending = 2,
    FourWayBlending = 4,
};

/** \struct SequenceDesc_HL1
 *  \brief Data structure for a sequence description in HL1 MDL files.
 */
struct SequenceDesc_HL1 {
    //! The sequence name.
    char label[32];

    //! Frames per second.
    float fps;

    //! looping/non-looping flags.
    int32_t flags;

    //! The sequence activity.
    int32_t activity;

    //! The sequence activity weight.
    int32_t actweight;

    //! The number of animation events.
    int32_t numevents;

    //! Offset the the first animation event chunk.
    int32_t eventindex;

    //! The number of frames in the sequence.
    int32_t numframes;

    //! Was "numpivots".
    int32_t unused;

    //! Was "pivotindex".
    int32_t unused2;

    //! Linear motion type.
    int32_t motiontype;

    //! Linear motion bone.
    int32_t motionbone;

    //! Linear motion.
    vec3_t linearmovement;

    //! Was "automoveposindex".
    int32_t unused3;

    //! Was "automoveangleindex".
    int32_t unused4;

    //! The sequence minimum and maximum extents.
    vec3_t bbmin;
    vec3_t bbmax;

    //! The number of blend animations.
    int32_t numblends;

    //! Offset to first the AnimValueOffset_HL1 chunk.
    //! This offset is relative to the SequenceHeader_HL1 of the file
    //! that contains the animation data.
    int32_t animindex;

    //! The motion type of each blend controller.
    int32_t blendtype[2];

    //! The starting value of each blend controller.
    float blendstart[2];

    //! The ending value of each blend controller.
    float blendend[2];

    //! Was "blendparent".
    int32_t unused5;

    //! The sequence group.
    int32_t seqgroup;

    //! The node at entry in the sequence transition graph.
    int32_t entrynode;

    //! The node at exit in the sequence transition graph.
    int32_t exitnode;

    //! Transition rules.
    int32_t nodeflags;

    //! Was "nextseq"
    int32_t unused6;
} PACK_STRUCT;

/** \struct AnimEvent_HL1
 *  \brief Data structure for an animation event in HL1 MDL files.
 */
struct AnimEvent_HL1 {
    //! The frame at which this animation event occurs.
    int32_t frame;

    //! The script event type.
    int32_t event;

    //! was "type"
    int32_t unused;

    //! Options. Could be path to sound WAVE files.
    char options[64];
} PACK_STRUCT;

/** \struct Attachment_HL1
 *  \brief Data structure for an attachment in HL1 MDL files.
 */
struct Attachment_HL1 {
    //! Was "name".
    char unused[32];

    //! Was "type".
    int32_t unused2;

    //! The bone this attachment follows.
    int32_t bone;

    //! The attachment origin.
    vec3_t org;

    //! Was "vectors"
    vec3_t unused3[3];
} PACK_STRUCT;

/** \struct AnimValueOffset_HL1
 *  \brief Data structure to hold offsets (one per motion type)
 *         to the first animation frame value for a single bone
 *         in HL1 MDL files.
 */
struct AnimValueOffset_HL1 {
    unsigned short offset[6];
} PACK_STRUCT;

/** \struct AnimValue_HL1
 *  \brief Data structure for an animation frame in HL1 MDL files.
 */
union AnimValue_HL1 {
    struct {
        uint8_t valid;
        uint8_t total;
    } num;
    short value;
} PACK_STRUCT;

/** \struct Bodypart_HL1
 *  \brief Data structure for a bodypart in HL1 MDL files.
 */
struct Bodypart_HL1 {
    //! The bodypart name.
    char name[64];

    //! The number of available models for this bodypart.
    int32_t nummodels;

    //! Used to convert from a global model index
    //! to a local bodypart model index.
    int32_t base;

    //! The offset to the first model chunk.
    int32_t modelindex;
} PACK_STRUCT;

/** \struct Texture_HL1
 *  \brief Data structure for a texture in HL1 MDL files.
 */
struct Texture_HL1 {
    //! Texture file name.
    char name[64];

    //! Texture flags.
    int32_t flags;

    //! Texture width in pixels.
    int32_t width;

    //! Texture height in pixels.
    int32_t height;

    //! Offset to the image data.
    //! This offset is relative to the texture file header.
    int32_t index;
} PACK_STRUCT;

/** \struct Model_HL1
 *  \brief Data structure for a model in HL1 MDL files.
 */
struct Model_HL1 {
    //! Model name.
    char name[64];

    //! Was "type".
    int32_t unused;

    //! Was "boundingradius".
    float unused2;

    //! The number of meshes in the model.
    int32_t nummesh;

    //! Offset to the first mesh chunk.
    int32_t meshindex;

    //! The number of unique vertices.
    int32_t numverts;

    //! Offset to the vertex bone array.
    int32_t vertinfoindex;

    //! Offset to the vertex array.
    int32_t vertindex;

    //! The number of unique normals.
    int32_t numnorms;

    //! Offset to the normal bone array.
    int32_t norminfoindex;

    //! Offset to the normal array.
    int32_t normindex;

    //! Was "numgroups".
    int32_t unused3;

    //! Was "groupindex".
    int32_t unused4;
} PACK_STRUCT;

/** \struct Mesh_HL1
 *  \brief Data structure for a mesh in HL1 MDL files.
 */
struct Mesh_HL1 {
    //! Can be interpreted as the number of triangles in the mesh.
    int32_t numtris;

    //! Offset to the start of the tris sequence.
    int32_t triindex;

    //! The skin index.
    int32_t skinref;

    //! The number of normals in the mesh.
    int32_t numnorms;

    //! Was "normindex".
    int32_t unused;
} PACK_STRUCT;

/** \struct Trivert
 *  \brief Data structure for a trivert in HL1 MDL files.
 */
struct Trivert {
    //! Index into Model_HL1 vertex array.
    short vertindex;

    //! Index into Model_HL1 normal array.
    short normindex;

    //! Texture coordinates in absolute space (unnormalized).
    short s, t;
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
