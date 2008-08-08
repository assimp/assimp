/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
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

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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

/** @file Defines the helper data structures for importing MDR files  */
#ifndef AI_MDRFILEHELPER_H_INC
#define AI_MDRFILEHELPER_H_INC

#include "../include/aiTypes.h"
#include "../include/aiMesh.h"
#include "../include/aiAnim.h"

#if defined(_MSC_VER) ||  defined(__BORLANDC__) ||	defined (__BCPLUSPLUS__)
#	pragma pack(push,1)
#	define PACK_STRUCT
#elif defined( __GNUC__ )
#	define PACK_STRUCT	__attribute__((packed))
#else
#	error Compiler not supported
#endif


namespace Assimp {
namespace MDR {

#define AI_MDR_MAGIC_NUMBER_BE	'RDM5'
#define AI_MDR_MAGIC_NUMBER_LE	'5MDR'

// common limitations
#define AI_MDR_VERSION			2
#define AI_MDR_MAXQPATH			64
#define	AI_MDR_MAX_BONES		128

// ---------------------------------------------------------------------------
/** \brief Data structure for a vertex weight in a MDR file
 */
struct  Weight
{
	//! these are indexes into the boneReferences
	//! not the global per-frame bone list
	uint32_t	boneIndex;	

	//! weight of this bone
	float		boneWeight;

	//! offset of this bone
	aiVector3D	offset;
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \brief Data structure for a vertex in a MDR file
 */
struct Vertex
{
	aiVector3D		normal;
	aiVector2D		texCoords;
	uint32_t		numWeights;
	Weight	weights[1];		// variable sized
} PACK_STRUCT;

// ---------------------------------------------------------------------------
/** \brief Data structure for a triangle in a MDR file
 */
struct Triangle
{
	uint32_t			indexes[3];
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \brief Data structure for a surface in a MDR file
 */
struct Surface
{
	uint32_t	ident;

	char		name[AI_MDR_MAXQPATH];	// polyset name
	char		shader[AI_MDR_MAXQPATH];
	uint32_t	shaderIndex;	

	int32_t		ofsHeader;	// this will be a negative number

	uint32_t	numVerts;
	uint32_t	ofsVerts;

	uint32_t	numTriangles;
	uint32_t	ofsTriangles;

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	uint32_t	numBoneReferences;
	uint32_t	ofsBoneReferences;

	uint32_t	ofsEnd;		// next surface follows
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \brief Data structure for a bone in a MDR file
 */
struct Bone
{
	float		matrix[3][4];
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \brief Data structure for a frame in a MDR file
 */
struct Frame {
	aiVector3D	bounds[2];		// bounds of all surfaces of all LOD's for this frame
	aiVector3D	localOrigin;	// midpoint of bounds, used for sphere cull
	float		radius;			// dist from localOrigin to corner
	char		name[16];
	Bone	bones[1];			// [numBones]
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \brief Data structure for a compressed bone in a MDR file
 */
struct CompBone
{
        unsigned char Comp[24]; // MC_COMP_BYTES is in MatComp.h, but don't want to couple
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \brief Data structure for a compressed frame in a MDR file
 */
struct CompFrame
{
        aiVector3D  bounds[2];		// bounds of all surfaces of all LOD's for this frame
        aiVector3D  localOrigin;		// midpoint of bounds, used for sphere cull
        float      radius;			// dist from localOrigin to corner
        CompBone   bones[1];		// [numBones]
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \brief Data structure for a LOD in a MDR file
 */
struct LOD
{
	uint32_t			numSurfaces;
	uint32_t			ofsSurfaces;		// first surface, others follow
	uint32_t			ofsEnd;				// next lod follows
} ;


// ---------------------------------------------------------------------------
/** \brief Data structure for a tag (= attachment) in a MDR file
 */
struct Tag
{
        uint32_t                     boneIndex;
        char            name[32];
} PACK_STRUCT;


// ---------------------------------------------------------------------------
/** \brief Header data structure for a MDR file
 */
struct Header
{
	uint32_t	ident;
	uint32_t	version;

	char		name[AI_MDR_MAXQPATH];	

	// frames and bones are shared by all levels of detail
	int32_t		numFrames;
	uint32_t	numBones;
	uint32_t	ofsFrames;			

	// each level of detail has completely separate sets of surfaces
	uint32_t	numLODs;
	uint32_t	ofsLODs;

    uint32_t    numTags;
	uint32_t    ofsTags;

	uint32_t	ofsEnd;				
} PACK_STRUCT;


// reset packing to the original value
#if defined(_MSC_VER) ||  defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#	pragma pack( pop )
#endif
#undef PACK_STRUCT


};
};

#endif // !! AI_MDRFILEHELPER_H_INC