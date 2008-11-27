/** Helper structures for the Collada loader */

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

#ifndef AI_COLLADAHELPER_H_INC
#define AI_COLLADAHELPER_H_INC

namespace Assimp
{
namespace Collada
{

/** Transformation types that can be applied to a node */
enum TransformType
{
	TF_LOOKAT,
	TF_ROTATE,
	TF_TRANSLATE,
	TF_SCALE,
	TF_SKEW,
	TF_MATRIX
};

/** Contains all data for one of the different transformation types */
struct Transform
{
	TransformType mType;
	float f[16]; ///< Interpretation of data depends on the type of the transformation 
};

/** A reference to a mesh inside a node, including materials assigned to the various subgroups */
struct MeshInstance
{
	std::string mMesh; ///< ID of the mesh
	std::map<std::string, std::string> mMaterials; ///< Map of materials by the subgroup ID they're applied to
};

/** A node in a scene hierarchy */
struct Node
{
	std::string mName;
	std::string mID;
	Node* mParent;
	std::vector<Node*> mChildren;

	/** Operations in order to calculate the resulting transformation to parent. */
	std::vector<Transform> mTransforms;

	std::vector<MeshInstance> mMeshes; ///< Meshes at this node

	Node() { mParent = NULL; }
	~Node() { for( std::vector<Node*>::iterator it = mChildren.begin(); it != mChildren.end(); ++it) delete *it; }
};

/** Data source array */
struct Data
{
	std::vector<float> mValues;
};

/** Accessor to a data array */
struct Accessor
{
	size_t mCount;   // in number of objects
	size_t mOffset;  // in number of values
	size_t mStride;  // Stride in number of values
	std::vector<std::string> mParams; // names of the data streams in the accessors. Empty string tells to ignore. 
	size_t mSubOffset[4]; // Suboffset inside the object for the common 4 elements. For a vector, thats XYZ, for a color RGBA and so on.
						  // For example, SubOffset[0] denotes which of the values inside the object is the vector X component.
	std::string mSource;   // URL of the source array
	mutable const Data* mData; // Pointer to the source array, if resolved. NULL else

	Accessor() 
	{ 
		mCount = 0; mOffset = 0; mStride = 0; mData = NULL; 
		mSubOffset[0] = mSubOffset[1] = mSubOffset[2] = mSubOffset[3] = 0;
	}
};

/** A single face in a mesh */
struct Face
{
	std::vector<size_t> mIndices;
};

/** Different types of input data to a vertex or face */
enum InputType
{
	IT_Invalid,
	IT_Vertex,  // special type for per-index data referring to the <vertices> element carrying the per-vertex data.
	IT_Position,
	IT_Normal,
	IT_Texcoord,
	IT_Color
};

/** An input channel for mesh data, referring to a single accessor */
struct InputChannel
{
	InputType mType;      // Type of the data
	size_t mIndex;		  // Optional index, if multiple sets of the same data type are given
	size_t mOffset;       // Index offset in the indices array of per-face indices. Don't ask, can't explain that any better.
	std::string mAccessor; // ID of the accessor where to read the actual values from.
	mutable const Accessor* mResolved; // Pointer to the accessor, if resolved. NULL else

	InputChannel() { mType = IT_Invalid; mIndex = 0; mOffset = 0; mResolved = NULL; }
};

/** Contains data for a single mesh */
struct Mesh
{
	std::string mVertexID; // just to check if there's some sophisticated addressing involved... which we don't support, and therefore should warn about.
	std::vector<InputChannel> mPerVertexData; // Vertex data addressed by vertex indices

	// actual mesh data, assembled on encounter of a <p> element. Verbose format, not indexed
	std::vector<aiVector3D> mPositions;
	std::vector<aiVector3D> mNormals;
	std::vector<aiVector2D> mTexCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];
	std::vector<aiColor4D> mColors[AI_MAX_NUMBER_OF_COLOR_SETS];

	// Faces. Stored are only the number of vertices for each face. 1 == point, 2 == line, 3 == triangle, 4+ == poly
	std::vector<size_t> mFaceSize;
};

/** Which type of primitives the ReadPrimitives() function is going to read */
enum PrimitiveType
{
	Prim_Invalid,
	Prim_Lines,
	Prim_LineStrip,
	Prim_Triangles,
	Prim_TriStrips,
	Prim_TriFans,
	Prim_Polylist,
	Prim_Polygon
};

/** A collada material. Pretty much the only member is a reference to an effect. */
struct Material
{
	std::string mEffect;
};

/** Shading type supported by the standard effect spec of Collada */
enum ShadeType
{
	Shade_Invalid,
	Shade_Constant,
	Shade_Lambert,
	Shade_Phong,
	Shade_Blinn
};


/** A collada effect. Can contain about anything according to the Collada spec, but we limit our version to a reasonable subset. */
struct Effect
{
	ShadeType mShadeType;
	aiColor4D mEmmisive, mAmbient, mDiffuse, mSpecular;
	aiColor4D mReflectivity, mRefractivity;
	std::string mTexEmmisive, mTexAmbient, mTexDiffuse, mTexSpecular;
	float mShininess, mRefractIndex;

	Effect() : mEmmisive( 0, 0, 0, 1), mAmbient( 0.1f, 0.1f, 0.1f, 1),
		mDiffuse( 0.6f, 0.6f, 0.6f, 1), mSpecular( 0.4f, 0.4f, 0.4f, 1),
		mReflectivity( 0, 0, 0, 0), mRefractivity( 0, 0, 0, 0)
	{ 
		mShadeType = Shade_Phong; 
		mShininess = 10;
		mRefractIndex = 1;
	}
};

/** An image, meaning texture */
struct Image
{
	std::string mFileName;
};

} // end of namespace Collada
} // end of namespace Assimp

#endif // AI_COLLADAHELPER_H_INC
