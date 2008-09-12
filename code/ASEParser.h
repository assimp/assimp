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


/** @file Defines the helper data structures for importing ASE files  */
#ifndef AI_ASEFILEHELPER_H_INC
#define AI_ASEFILEHELPER_H_INC

// STL/CRT headers
#include <string>
#include <vector>
#include <list>

// public ASSIMP headers
#include "../include/aiTypes.h"
#include "../include/aiMesh.h"
#include "../include/aiAnim.h"

// for some helper routines like IsSpace()
#include "ParsingUtils.h"
#include "qnan.h"

// ASE is quite similar to 3ds. We can reuse some structures
#include "3DSLoader.h"

namespace Assimp
{

// http://wiki.beyondunreal.com/Legacy:ASE_File_Format
namespace ASE
{
	using namespace Dot3DS;

// ---------------------------------------------------------------------------
/** Helper structure representing an ASE material */
struct Material : public Dot3DS::Material
{
	//! Default constructor
	Material() : pcInstance(NULL), bNeed (false)
	{}

	//! Contains all sub materials of this material
	std::vector<Material> avSubMaterials;

	//! MaterialHelper object
	MaterialHelper* pcInstance;

	//! Can we remove this material?
	bool bNeed;
};
// ---------------------------------------------------------------------------
/** Helper structure to represent an ASE file face */
struct Face : public FaceWithSmoothingGroup
{
	//! Default constructor. Initializes everything with 0
	Face()
	{
		mColorIndices[0] = mColorIndices[1] = mColorIndices[2] = 0;
		for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
		{
			amUVIndices[i][0] = amUVIndices[i][1] = amUVIndices[i][2] = 0;
		}

		iMaterial = DEFAULT_MATINDEX;
		iFace = 0;
	}

	//! special value to indicate that no material index has
	//! been assigned to a face. The default material index
	//! will replace this value later.
	static const unsigned int DEFAULT_MATINDEX = 0xFFFFFFFF;



	//! Indices into each list of texture coordinates
	unsigned int amUVIndices[AI_MAX_NUMBER_OF_TEXTURECOORDS][3];

	//! Index into the list of vertex colors
	unsigned int mColorIndices[3];

	//! (Sub)Material index to be assigned to this face
	unsigned int iMaterial;

	//! Index of the face. It is not specified whether it is
	//! a requirement of the file format that all faces are
	//! written in sequential order, so we have to expect this case
	unsigned int iFace;
};

// ---------------------------------------------------------------------------
/** Helper structure to represent an ASE file bone */
struct Bone
{
	//! Constructor
	Bone()
	{
		static int iCnt = 0;
		
		char szTemp[128];
		::sprintf(szTemp,"UNNAMED_%i",iCnt++);
		mName = szTemp;
	}

	//! Name of the bone
	std::string mName;
};

// ---------------------------------------------------------------------------
/** Helper structure to represent an ASE file bone vertex */
struct BoneVertex
{
	//! Bone and corresponding vertex weight.
	//! -1 for unrequired bones ....
	std::vector<std::pair<int,float> > mBoneWeights;

	//! Position of the bone vertex.
	//! MUST be identical to the vertex position
	//aiVector3D mPosition;
};

// ---------------------------------------------------------------------------
/** Helper structure to represent an ASE file animation */
struct Animation
{
	//! List of rotation keyframes
	std::vector< aiQuatKey > akeyRotations;

	//! List of position keyframes
	std::vector< aiVectorKey > akeyPositions;

};

// ---------------------------------------------------------------------------
/** Helper structure to represent the inheritance information of an ASE node */
struct InheritanceInfo
{
	//! Default constructor
	InheritanceInfo()
	{
		// set the inheritance flag for all axes by default to true
		for (unsigned int i = 0; i < 3;++i)
			abInheritPosition[i] = abInheritRotation[i] = abInheritScaling[i] = true;
	}

	//! Inherit the parent's position?, axis order is x,y,z
	bool abInheritPosition[3];

	//! Inherit the parent's rotation?, axis order is x,y,z
	bool abInheritRotation[3];

	//! Inherit the parent's scaling?, axis order is x,y,z
	bool abInheritScaling[3];
};

// ---------------------------------------------------------------------------
/** Stores a decomposed transformation matrix */
struct DecompTransform
{
	//! Construction from a reference to an existing matrix
	DecompTransform(aiMatrix4x4& ref)
		: vPosition(std::numeric_limits<float>::quiet_NaN(),0.0f,0.0f)
		, vScaling(1.0f,1.0f,1.0f)
		, mMatrix(ref)
	{}

	//! Translational component
	mutable aiVector3D vPosition;

	//! Rotational component
	mutable aiQuaternion qRotation;

	//! Scaling component
	mutable aiVector3D vScaling;

	//! Reference to the matrix being decomposed
	const aiMatrix4x4& mMatrix;

	//! Decomposes the matrix if this has not yet been done
	inline void NeedDecomposedMatrixNOW() const
	{
		if (is_qnan(vPosition.x))
		{
			mMatrix.Decompose(vScaling,qRotation,vPosition);
		}
	}
};

// ---------------------------------------------------------------------------
/** Helper structure to represent an ASE file mesh */
struct Mesh : public MeshWithSmoothingGroups<ASE::Face>
{
	//! Constructor. Creates a default name for the mesh
	Mesh() : bSkip(false)
	{
		static int iCnt = 0;
		char szTemp[128]; // should be sufficiently large
		::sprintf(szTemp,"UNNAMED_%i",iCnt++);
		mName = szTemp;

		// use 2 texture vertex components by default
		for (unsigned int c = 0; c < AI_MAX_NUMBER_OF_TEXTURECOORDS;++c)
			this->mNumUVComponents[c] = 2;

		// setup the default material index by default
		iMaterialIndex = Face::DEFAULT_MATINDEX;
	}

	//! Name of the mesh
	std::string mName;

	//! Name of the parent of the mesh
	//! "" if there is no parent ...
	std::string mParent;

	//! List of all texture coordinate sets
	std::vector<aiVector3D> amTexCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];

	//! List of all vertex color sets.
	std::vector<aiColor4D> mVertexColors;

	//! List of all bone vertices
	std::vector<BoneVertex> mBoneVertices;

	//! List of all bones
	std::vector<Bone> mBones;

	//! Transformation matrix of the mesh
	aiMatrix4x4 mTransform;

	//! Animation channels for the node
	Animation mAnim;

	//! Material index of the mesh
	unsigned int iMaterialIndex;

	//! Number of vertex components for each UVW set
	unsigned int mNumUVComponents[AI_MAX_NUMBER_OF_TEXTURECOORDS];

	//! used internally
	bool bSkip;

	//! Specifies which axes transformations a node inherits
	//! from its parent ...
	InheritanceInfo inherit;
};

// ---------------------------------------------------------------------------------
/** \brief Class to parse ASE files
 */
class Parser
{

private:

	Parser() {}

public:

	//! Construct a parser from a given input file which is
	//! guaranted to be terminated with zero.
	Parser (const char* szFile);

	// -------------------------------------------------------------------
	//! Parses the file into the parsers internal representation
	void Parse();


private:

	// -------------------------------------------------------------------
	//! Parse the *SCENE block in a file
	void ParseLV1SceneBlock();

	// -------------------------------------------------------------------
	//! Parse the *MATERIAL_LIST block in a file
	void ParseLV1MaterialListBlock();

	// -------------------------------------------------------------------
	//! Parse a *GEOMOBJECT block in a file
	//! \param mesh Mesh object to be filled
	void ParseLV1GeometryObjectBlock(Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse a *MATERIAL blocks in a material list
	//! \param mat Material structure to be filled
	void ParseLV2MaterialBlock(Material& mat);

	// -------------------------------------------------------------------
	//! Parse a *NODE_TM block in a file
	//! \param mesh Mesh object to be filled
	void ParseLV2NodeTransformBlock(Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse a *TM_ANIMATION block in a file
	//! \param mesh Mesh object to be filled
	void ParseLV2AnimationBlock(Mesh& mesh);
	void ParseLV3PosAnimationBlock(Mesh& mesh);
	void ParseLV3RotAnimationBlock(Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse a *MESH block in a file
	//! \param mesh Mesh object to be filled
	void ParseLV2MeshBlock(Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse the *MAP_XXXXXX blocks in a material
	//! \param map Texture structure to be filled
	void ParseLV3MapBlock(Texture& map);

	// -------------------------------------------------------------------
	//! Parse a *MESH_VERTEX_LIST block in a file
	//! \param iNumVertices Value of *MESH_NUMVERTEX, if present.
	//! Otherwise zero. This is used to check the consistency of the file.
	//! A warning is sent to the logger if the validations fails.
	//! \param mesh Mesh object to be filled
	void ParseLV3MeshVertexListBlock(
		unsigned int iNumVertices,Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse a *MESH_FACE_LIST block in a file
	//! \param iNumFaces Value of *MESH_NUMFACES, if present.
	//! Otherwise zero. This is used to check the consistency of the file.
	//! A warning is sent to the logger if the validations fails.
	//! \param mesh Mesh object to be filled
	void ParseLV3MeshFaceListBlock(
		unsigned int iNumFaces,Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse a *MESH_TVERT_LIST block in a file
	//! \param iNumVertices Value of *MESH_NUMTVERTEX, if present.
	//! Otherwise zero. This is used to check the consistency of the file.
	//! A warning is sent to the logger if the validations fails.
	//! \param mesh Mesh object to be filled
	//! \param iChannel Output UVW channel
	void ParseLV3MeshTListBlock(
		unsigned int iNumVertices,Mesh& mesh, unsigned int iChannel = 0);

	// -------------------------------------------------------------------
	//! Parse a *MESH_TFACELIST block in a file
	//! \param iNumFaces Value of *MESH_NUMTVFACES, if present.
	//! Otherwise zero. This is used to check the consistency of the file.
	//! A warning is sent to the logger if the validations fails.
	//! \param mesh Mesh object to be filled
	//! \param iChannel Output UVW channel
	void ParseLV3MeshTFaceListBlock(
		unsigned int iNumFaces,Mesh& mesh, unsigned int iChannel = 0);

	// -------------------------------------------------------------------
	//! Parse an additional mapping channel 
	//! (specified via *MESH_MAPPINGCHANNEL)
	//! \param iChannel Channel index to be filled
	//! \param mesh Mesh object to be filled
	void ParseLV3MappingChannel(
		unsigned int iChannel, Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse a *MESH_CVERTLIST block in a file
	//! \param iNumVertices Value of *MESH_NUMCVERTEX, if present.
	//! Otherwise zero. This is used to check the consistency of the file.
	//! A warning is sent to the logger if the validations fails.
	//! \param mesh Mesh object to be filled
	void ParseLV3MeshCListBlock(
		unsigned int iNumVertices, Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse a *MESH_CFACELIST block in a file
	//! \param iNumFaces Value of *MESH_NUMCVFACES, if present.
	//! Otherwise zero. This is used to check the consistency of the file.
	//! A warning is sent to the logger if the validations fails.
	//! \param mesh Mesh object to be filled
	void ParseLV3MeshCFaceListBlock(
		unsigned int iNumFaces, Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse a *MESH_NORMALS block in a file
	//! \param mesh Mesh object to be filled
	void ParseLV3MeshNormalListBlock(Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse a *MESH_WEIGHTSblock in a file
	//! \param mesh Mesh object to be filled
	void ParseLV3MeshWeightsBlock(Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse the bone list of a file
	//! \param mesh Mesh object to be filled
	//! \param iNumBones Number of bones in the mesh
	void ParseLV4MeshBones(unsigned int iNumBones,Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse the bone vertices list of a file
	//! \param mesh Mesh object to be filled
	//! \param iNumVertices Number of vertices to be parsed
	void ParseLV4MeshBonesVertices(unsigned int iNumVertices,Mesh& mesh);

	// -------------------------------------------------------------------
	//! Parse a *MESH_FACE block in a file
	//! \param out receive the face data
	void ParseLV4MeshFace(ASE::Face& out);

	// -------------------------------------------------------------------
	//! Parse a *MESH_VERT block in a file
	//! (also works for MESH_TVERT, MESH_CFACE, MESH_VERTCOL  ...)
	//! \param apOut Output buffer (3 floats)
	//! \param rIndexOut Output index
	void ParseLV4MeshFloatTriple(float* apOut, unsigned int& rIndexOut);

	// -------------------------------------------------------------------
	//! Parse a *MESH_VERT block in a file
	//! (also works for MESH_TVERT, MESH_CFACE, MESH_VERTCOL  ...)
	//! \param apOut Output buffer (3 floats)
	void ParseLV4MeshFloatTriple(float* apOut);

	// -------------------------------------------------------------------
	//! Parse a *MESH_TFACE block in a file
	//! (also works for MESH_CFACE)
	//! \param apOut Output buffer (3 ints)
	//! \param rIndexOut Output index
	void ParseLV4MeshLongTriple(unsigned int* apOut, unsigned int& rIndexOut);

	// -------------------------------------------------------------------
	//! Parse a *MESH_TFACE block in a file
	//! (also works for MESH_CFACE)
	//! \param apOut Output buffer (3 ints)
	void ParseLV4MeshLongTriple(unsigned int* apOut);

	// -------------------------------------------------------------------
	//! Parse a single float element 
	//! \param fOut Output float
	void ParseLV4MeshFloat(float& fOut);

	// -------------------------------------------------------------------
	//! Parse a single int element 
	//! \param iOut Output integer
	void ParseLV4MeshLong(unsigned int& iOut);

	// -------------------------------------------------------------------
	//! Skip everything to the next: '*' or '\0'
	bool SkipToNextToken();

	// -------------------------------------------------------------------
	//! Skip the current section until the token after the closing }.
	//! This function handles embedded subsections correctly
	bool SkipSection();

	// -------------------------------------------------------------------
	//! Output a warning to the logger
	//! \param szWarn Warn message
	void LogWarning(const char* szWarn);

	// -------------------------------------------------------------------
	//! Output a message to the logger
	//! \param szWarn Message
	void LogInfo(const char* szWarn);

	// -------------------------------------------------------------------
	//! Output an error to the logger
	//! \param szWarn Error message
	void LogError(const char* szWarn);

	// -------------------------------------------------------------------
	//! Parse a string, enclosed in double quotation marks
	//! \param out Output string
	//! \param szName Name of the enclosing element -> used in error
	//! messages.
	//! \return false if an error occured
	bool ParseString(std::string& out,const char* szName);

public:

	//! Pointer to current data
	const char* m_szFile;

	//! background color to be passed to the viewer
	//! QNAN if none was found
	aiColor3D m_clrBackground;

	//! Base ambient color to be passed to all materials
	//! QNAN if none was found
	aiColor3D m_clrAmbient;

	//! List of all materials found in the file
	std::vector<Material> m_vMaterials;

	//! List of all meshes found in the file
	std::vector<Mesh> m_vMeshes;

	//! Current line in the file
	unsigned int iLineNumber;


	//! First frame
	unsigned int iFirstFrame;

	//! Last frame
	unsigned int iLastFrame;

	//! Frame speed - frames per second
	unsigned int iFrameSpeed;

	//! Ticks per frame
	unsigned int iTicksPerFrame;

	//! true if the last character read was an end-line character
	bool bLastWasEndLine;
};


} // Namespace ASE
} // Namespace ASSIMP

#endif // !! include guard
