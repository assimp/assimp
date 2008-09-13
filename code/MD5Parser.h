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


/** @file Definition of the .MD5 parser class.
http://www.modwiki.net/wiki/MD5_(file_format)
*/
#ifndef AI_MD5PARSER_H_INCLUDED
#define AI_MD5PARSER_H_INCLUDED

#include "../include/aiTypes.h"
#include "ParsingUtils.h"
#include <vector>

struct aiFace;

namespace Assimp	{
namespace MD5		{


// ---------------------------------------------------------------------------
/** Represents a single element in a MD5 file
 *  
 *  Elements are always contained in sections.
*/
struct Element
{
	//! Points to the starting point of the element
	//! Whitespace at the beginning and at the end have been removed,
	//! Elements are terminated with \0
	char* szStart;

	//! Original line number (can be used in error messages
	//! if a parsing error occurs)
	unsigned int iLineNumber;
};

typedef std::vector< Element > ElementList;

// ---------------------------------------------------------------------------
/** Represents a section of a MD5 file (such as the mesh or the joints section)
 *  
 *  A section is always enclosed in { and } brackets.
*/
struct Section
{
	//! Original line number (can be used in error messages
	//! if a parsing error occurs)
	unsigned int iLineNumber;

	//! List of all elements which have been parsed in this section.
	ElementList mElements;

	//! Name of the section
	std::string mName;

	//! For global elements: the value of the element as string
	//! Iif !length() the section is not a global element
	std::string mGlobalValue;
};

typedef std::vector< Section> SectionList;

// ---------------------------------------------------------------------------
/** Represents a bone (joint) descriptor in a MD5Mesh file
*/
struct BoneDesc
{
	//! Name of the bone
	aiString mName;

	//! Parent index of the bone
	int mParentIndex;

	//! Relative position of the bone
	aiVector3D mPositionXYZ;

	//! Relative rotation of the bone
	aiVector3D mRotationQuat;

	//! Absolute transformation of the bone
	//! (temporary)
	aiMatrix4x4 mTransform;

	//! Inverse transformation of the bone
	//! (temporary)
	aiMatrix4x4 mInvTransform;

	//! Internal
	unsigned int mMap;
};

typedef std::vector< BoneDesc > BoneList;

// ---------------------------------------------------------------------------
/** Represents a bone (joint) descriptor in a MD5Anim file
*/
struct AnimBoneDesc
{
	//! Name of the bone
	aiString mName;

	//! Parent index of the bone
	int mParentIndex;

	//! Flags (AI_MD5_ANIMATION_FLAG_xxx)
	unsigned int iFlags;

	//! Index of the first key that corresponds to this anim bone
	unsigned int iFirstKeyIndex;
};

typedef std::vector< AnimBoneDesc > AnimBoneList;


// ---------------------------------------------------------------------------
/** Represents a base frame descriptor in a MD5Anim file
*/
struct BaseFrameDesc
{
	aiVector3D vPositionXYZ;
	aiVector3D vRotationQuat;
};

typedef std::vector< BaseFrameDesc > BaseFrameList;


// ---------------------------------------------------------------------------
/** Represents a frame descriptor in a MD5Anim file
*/
struct FrameDesc
{
	//! Index of the frame
	unsigned int iIndex;

	//! Animation keyframes - a large blob of data at first
	std::vector< float > mValues;
};

typedef std::vector< FrameDesc > FrameList;

// ---------------------------------------------------------------------------
/** Represents a vertex  descriptor in a MD5 file
*/
struct VertexDesc
{
	VertexDesc()
		: mFirstWeight	(0)
		, mNumWeights	(0)
	{}

	//! UV cordinate of the vertex
	aiVector2D mUV;

	//! Index of the first weight of the vertex in
	//! the vertex weight list
	unsigned int mFirstWeight;

	//! Number of weights assigned to this vertex
	unsigned int mNumWeights;
};

typedef std::vector< VertexDesc > VertexList;

// ---------------------------------------------------------------------------
/** Represents a vertex weight descriptor in a MD5 file
*/
struct WeightDesc
{
	//! Index of the bone to which this weight refers
	unsigned int mBone;

	//! The weight value
	float mWeight;

	//! The offset position of this weight
	// ! (in the coordinate system defined by the parent bone)
	aiVector3D vOffsetPosition;
};

typedef std::vector< WeightDesc > WeightList;
typedef std::vector< aiFace > FaceList;

// ---------------------------------------------------------------------------
/** Represents a mesh in a MD5 file
*/
struct MeshDesc
{
	//! Weights of the mesh
	WeightList mWeights;

	//! Vertices of the mesh
	VertexList mVertices;

	//! Faces of the mesh
	FaceList mFaces;

	//! Name of the shader (=texture) to be assigned to the mesh
	aiString mShader;
};

typedef std::vector< MeshDesc > MeshList;

// ---------------------------------------------------------------------------
/** Parses the data sections of a MD5 mesh file
*/
class MD5MeshParser
{
public:

	// -------------------------------------------------------------------
	/** Constructs a new MD5MeshParser instance from an existing
	 *  preparsed list of file sections.
	 *
	 *  @param mSections List of file sections (output of MD5Parser)
	 */
	MD5MeshParser(SectionList& mSections);

	//! List of all meshes
	MeshList mMeshes;

	//! List of all joints
	BoneList mJoints;
};

#define AI_MD5_ANIMATION_FLAG_TRANSLATE_X 0x1
#define AI_MD5_ANIMATION_FLAG_TRANSLATE_Y 0x2
#define AI_MD5_ANIMATION_FLAG_TRANSLATE_Z 0x4

#define AI_MD5_ANIMATION_FLAG_ROTQUAT_X 0x8
#define AI_MD5_ANIMATION_FLAG_ROTQUAT_Y 0x10
#define AI_MD5_ANIMATION_FLAG_ROTQUAT_Z 0x12

// remove this flag if you need to the bounding box data
#define AI_MD5_PARSE_NO_BOUNDS

// ---------------------------------------------------------------------------
/** Parses the data sections of a MD5 animation file
*/
class MD5AnimParser
{
public:

	// -------------------------------------------------------------------
	/** Constructs a new MD5AnimParser instance from an existing
	 *  preparsed list of file sections.
	 *
	 *  @param mSections List of file sections (output of MD5Parser)
	 */
	MD5AnimParser(SectionList& mSections);

	
	//! Output frame rate
	float fFrameRate;

	//! List of animation bones
	AnimBoneList mAnimatedBones;

	//! List of base frames
	BaseFrameList mBaseFrames;

	//! List of animation frames
	FrameList mFrames;

	//! Number of animated components
	unsigned int mNumAnimatedComponents;
};

// ---------------------------------------------------------------------------
/** Parses the block structure of MD5MESH and MD5ANIM files (but does no
 *  further processing)
*/
class MD5Parser
{
public:

	// -------------------------------------------------------------------
	/** Constructs a new MD5Parser instance from an existing buffer.
	 *
	 *  @param buffer File buffer
	 *  @param fileSize Length of the file in bytes (excluding a terminal 0)
	 */
	MD5Parser(char* buffer, unsigned int fileSize);

	
	// -------------------------------------------------------------------
	/** Report a specific error message and throw an exception
	 *  @param error Error message to be reported
	 *  @param line Index of the line where the error occured
	 */
	static void ReportError (const char* error, unsigned int line);

	// -------------------------------------------------------------------
	/** Report a specific warning
	 *  @param warn Warn message to be reported
	 *  @param line Index of the line where the error occured
	 */
	static void ReportWarning (const char* warn, unsigned int line);


	inline void ReportError (const char* error)
		{return ReportError(error, this->lineNumber);}

	inline void ReportWarning (const char* warn)
		{return ReportWarning(warn, this->lineNumber);}


public:

	//! List of all sections which have been read
	SectionList mSections;

private:

	// -------------------------------------------------------------------
	/** Parses a file section. The current file pointer must be outside
	 *  of a section.
	 *  @param out Receives the section data
	 *  @return true if the end of the file has been reached
	 *  @throws ImportErrorException if an error occurs
	 */
	bool ParseSection(Section& out);

	// -------------------------------------------------------------------
	/** Parses the file header
	 *  @throws ImportErrorException if an error occurs
	 */
	void ParseHeader();


	// override these functions to make sure the line counter gets incremented
	// -------------------------------------------------------------------
	inline bool SkipLine( const char* in, const char** out)
	{
		++lineNumber;
		return ::SkipLine(in,out);
	}
	// -------------------------------------------------------------------
	inline bool SkipLine( )
	{
		return SkipLine(buffer,(const char**)&buffer);
	}
	// -------------------------------------------------------------------
	inline bool SkipSpacesAndLineEnd( const char* in, const char** out)
	{
		bool bHad = false;
		while (true) 
		{
			if( *in == '\r' || *in == '\n')
			{
				if (!bHad) // we open files in binary mode, so there could be \r\n sequences ...
				{
					bHad = true;
					++lineNumber;
				}
			}
			else if (*in == '\t' || *in == ' ')bHad = false;
			else break;
			in++;
		}

		*out = in;
		return *in != '\0';
	}
	// -------------------------------------------------------------------
	inline bool SkipSpacesAndLineEnd( )
	{
		return SkipSpacesAndLineEnd(buffer,(const char**)&buffer);
	}
	// -------------------------------------------------------------------
	inline bool SkipSpaces( )
	{
		return ::SkipSpaces((const char**)&buffer);
	}

	char* buffer;
	unsigned int fileSize;
	unsigned int lineNumber;
};
}}

#endif // AI_MD5PARSER_H_INCLUDED
