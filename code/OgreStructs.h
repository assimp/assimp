/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
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

#ifndef AI_OGRESTRUCTS_H_INC
#define AI_OGRESTRUCTS_H_INC

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include "AssimpPCH.h"
#include "MemoryIOWrapper.h"

/** @note Parts of this implementation, for example enums, deserialization constants and logic
	has been copied directly with minor modifications from the MIT licensed Ogre3D code base.
	See more from https://bitbucket.org/sinbad/ogre. */

namespace Assimp
{
namespace Ogre
{

// Forward decl
class Mesh;
class SubMesh2;

#define OGRE_SAFE_DELETE(p) delete p; p=0;

// Typedefs
typedef Assimp::StreamReaderLE MemoryStreamReader;
typedef Assimp::MemoryIOStream MemoryStream;
typedef boost::shared_ptr<MemoryStream> MemoryStreamPtr;
typedef std::map<uint16_t, MemoryStreamPtr> VertexBufferBindings;

// Ogre Vertex Element
class VertexElement
{
public:
	/// Vertex element semantics, used to identify the meaning of vertex buffer contents
	enum Semantic {
		/// Position, 3 reals per vertex
		VES_POSITION = 1,
		/// Blending weights
		VES_BLEND_WEIGHTS = 2,
		/// Blending indices
		VES_BLEND_INDICES = 3,
		/// Normal, 3 reals per vertex
		VES_NORMAL = 4,
		/// Diffuse colours
		VES_DIFFUSE = 5,
		/// Specular colours
		VES_SPECULAR = 6,
		/// Texture coordinates
		VES_TEXTURE_COORDINATES = 7,
		/// Binormal (Y axis if normal is Z)
		VES_BINORMAL = 8,
		/// Tangent (X axis if normal is Z)
		VES_TANGENT = 9,
		/// The  number of VertexElementSemantic elements (note - the first value VES_POSITION is 1) 
		VES_COUNT = 9
	};

	/// Vertex element type, used to identify the base types of the vertex contents
	enum Type
	{
		VET_FLOAT1 = 0,
		VET_FLOAT2 = 1,
		VET_FLOAT3 = 2,
		VET_FLOAT4 = 3,
		/// alias to more specific colour type - use the current rendersystem's colour packing
		VET_COLOUR = 4,
		VET_SHORT1 = 5,
		VET_SHORT2 = 6,
		VET_SHORT3 = 7,
		VET_SHORT4 = 8,
		VET_UBYTE4 = 9,
		/// D3D style compact colour
		VET_COLOUR_ARGB = 10,
		/// GL style compact colour
		VET_COLOUR_ABGR = 11,
		VET_DOUBLE1 = 12,
		VET_DOUBLE2 = 13,
		VET_DOUBLE3 = 14,
		VET_DOUBLE4 = 15,
		VET_USHORT1 = 16,
		VET_USHORT2 = 17,
		VET_USHORT3 = 18,
		VET_USHORT4 = 19,      
		VET_INT1 = 20,
		VET_INT2 = 21,
		VET_INT3 = 22,
		VET_INT4 = 23,
		VET_UINT1 = 24,
		VET_UINT2 = 25,
		VET_UINT3 = 26,
		VET_UINT4 = 27
	};

	VertexElement();

	/// Size of the vertex element in bytes.
	size_t Size() const;
	
	/// Count of components in this element, eg. VET_FLOAT3 return 3.
	size_t ComponentCount() const;
	
	/// Type as string.
	std::string TypeToString();
	
	/// Semantic as string.
	std::string SemanticToString();
	
	static size_t TypeSize(Type type);
	static size_t ComponentCount(Type type);
	static std::string TypeToString(Type type);
	static std::string SemanticToString(Semantic semantic);

	uint16_t index;
	uint16_t source;
	uint16_t offset;
	Type type;
	Semantic semantic;
};

typedef std::vector<VertexElement> VertexElementList;

// Ogre Vertex Data
class VertexData
{
public:
	VertexData();
	~VertexData();

	/// Releases all memory that this data structure owns.
	void Reset();

	/// Get vertex size for @c source.
	uint32_t VertexSize(uint16_t source) const;

	/// Get vertex buffer for @c source.
	MemoryStream *VertexBuffer(uint16_t source);

	/// Get vertex element for @c semantic for @c index.
	VertexElement *GetVertexElement(VertexElement::Semantic semantic, uint16_t index = 0);

	/// Vertex count.
	uint32_t count;

	/// Vertex elements.
	VertexElementList vertexElements;

	/// Vertex buffers mapped to bind index.
	VertexBufferBindings vertexBindings;
};

// Ogre Index Data
class IndexData
{
public:
	IndexData();
	~IndexData();

	/// Releases all memory that this data structure owns.
	void Reset();

	/// Index size in bytes.
	size_t IndexSize() const;

	/// Face size in bytes.
	size_t FaceSize() const;

	/// Index count.
	uint32_t count;
	
	/// Face count.
	uint32_t faceCount;

	/// If has 32-bit indexes.
	bool is32bit;

	/// Index buffer.
	MemoryStreamPtr buffer;
};

/// Ogre Pose
class Pose
{
public:
	struct Vertex
	{
		uint32_t index;
		aiVector3D offset;
		aiVector3D normal;
	};
	typedef std::map<uint32_t, Vertex> PoseVertexMap;

	Pose() : target(0), hasNormals(false) {}

	/// Name.
	std::string name;
	
	/// Target.
	uint16_t target;
	
	/// Does vertices map have normals.
	bool hasNormals;
	
	/// Vertex offset and normals.
	PoseVertexMap vertices;
};

/// Ogre Pose Key Frame Ref
struct PoseRef
{
	uint16_t index;
	float influence;
};

/// Ogre Pose Key Frame
struct PoseKeyFrame
{
	/// Time position in the animation.
	float timePos;

	std::vector<PoseRef> references;
};

/// Ogre Morph Key Frame
struct MorphKeyFrame
{
	/// Time position in the animation.
	float timePos;

	MemoryStreamPtr buffer;
};

/// Ogre Animation Track
struct VertexAnimationTrack
{
	enum Type
	{
		/// No animation
		VAT_NONE = 0,
		/// Morph animation is made up of many interpolated snapshot keyframes
		VAT_MORPH = 1,
		/// Pose animation is made up of a single delta pose keyframe
		VAT_POSE = 2
	};

	/// Vertex data target.
	/**  0 == shared geometry
		>0 == submesh index + 1 */
	uint16_t target;
	Type type;
	
	std::vector<PoseKeyFrame> poseKeyFrames;
	std::vector<MorphKeyFrame> morphKeyFrames;
};

/// Ogre Animation
/** @todo Port OgreImporter::Animation to this and rename this to Animation! */
class Animation2
{
public:
	Animation2(Mesh *_parentMesh);

	/// Returns the associated vertex data for a track in this animation.
	VertexData *AssociatedVertexData(VertexAnimationTrack *track) const;
	
	/// Parent mesh.
	Mesh *parentMesh;
	
	/// Animation name.
	std::string name;
	
	/// Base animation name.
	std::string baseName;
	
	/// Length in seconds.
	float length;
	
	/// Base animation key time.
	float baseTime;

	/// Animation tracks.
	std::vector<VertexAnimationTrack> tracks;
};

/// Ogre Vertex Bone Assignment
struct VertexBoneAssignment
{
	uint32_t vertexIndex;
	uint16_t boneIndex;
	float weight;
};

/// Ogre SubMesh
/** @todo Port OgreImporter::SubMesh to this and rename this to SubMesh! */
class SubMesh2
{
public:
	/// @note Full list of Ogre types, not all of them are supported and exposed to Assimp.
	enum OperationType
	{
		/// A list of points, 1 vertex per point
		OT_POINT_LIST = 1,
		/// A list of lines, 2 vertices per line
		OT_LINE_LIST = 2,
		/// A strip of connected lines, 1 vertex per line plus 1 start vertex
		OT_LINE_STRIP = 3,
		/// A list of triangles, 3 vertices per triangle
		OT_TRIANGLE_LIST = 4,
		/// A strip of triangles, 3 vertices for the first triangle, and 1 per triangle after that 
		OT_TRIANGLE_STRIP = 5,
		/// A fan of triangles, 3 vertices for the first triangle, and 1 per triangle after that
		OT_TRIANGLE_FAN = 6
	};

	SubMesh2();
	~SubMesh2();

	/// Releases all memory that this data structure owns.
	/** @note Vertex and index data contains shared ptrs
		that are freed automatically. In practice the ref count
		should be 0 after this reset. */
	void Reset();
	
	/// Covert to Assimp mesh.
	aiMesh *ConvertToAssimpMesh(Mesh *parent);

	/// SubMesh index.
	unsigned int index;

	/// SubMesh name.
	std::string name;

	/// Material used by this submesh.
	std::string materialRef;
	
	/// Texture alias information.
	std::string textureAliasName;
	std::string textureAliasRef;

	/// Assimp scene material index used by this submesh.
	/** -1 if no material or material could not be imported. */
	int materialIndex;

	/// Vertex data.
	VertexData *vertexData;

	/// Index data.
	IndexData *indexData;

	/// If submesh uses shared geometry from parent mesh.
	bool usesSharedVertexData;

	/// Operation type.
	OperationType operationType;
	
	/// Bone assignments.
	std::vector<VertexBoneAssignment> boneAssignments;
};

/// Ogre Mesh
class Mesh
{
public:
	Mesh();
	~Mesh();

	/// Releases all memory that this data structure owns.
	void Reset();

	/// Returns number of subMeshes.
	size_t NumSubMeshes() const;

	/// Returns submesh for @c index.
	SubMesh2 *SubMesh(uint16_t index) const;

	/// Convert mesh to Assimp scene.
	void ConvertToAssimpScene(aiScene* dest);

	/// Mesh has skeletal animations.
	bool hasSkeletalAnimations;
	
	/// Skeleton reference.
	std::string skeletonRef;

	/// Vertex data
	VertexData *sharedVertexData;

	/// Sub meshes.
	std::vector<SubMesh2*> subMeshes;

	/// Animations
	std::vector<Animation2*> animations;

	/// Bone assignments.
	std::vector<VertexBoneAssignment> boneAssignments;

	/// Poses
	std::vector<Pose*> poses;
};

} // Ogre
} // Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
#endif // AI_OGRESTRUCTS_H_INC
