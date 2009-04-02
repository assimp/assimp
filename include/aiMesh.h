/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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
---------------------------------------------------------------------------
*/

/** @file aiMesh.h
 *  @brief Declares the data structures in which the imported geometry is 
    returned by ASSIMP: aiMesh, aiFace and aiBone data structures.
 */
#ifndef INCLUDED_AI_MESH_H
#define INCLUDED_AI_MESH_H

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
/** @brief A single face in a mesh, referring to multiple vertices. 
 *
 * If mNumIndices is 3, we call the face 'triangle', for mNumIndices > 3 
 * it's called 'polygon' (hey, that's just a definition!).
 * <br>
 * aiMesh::mPrimitiveTypes can be queried to quickly examine which types of
 * primitive are actually present in a mesh. The #aiProcess_SortByPType flag 
 * executes a special post-processing algorithm which splits meshes with
 * *different* primitive types mixed up (e.g. lines and triangles) in several
 * 'clean' submeshes. Furthermore there is a configuration option (
 * #AI_CONFIG_PP_SBP_REMOVE) to force #aiProcess_SortByPType to remove 
 * specific kinds of primitives from the imported scene, completely and forever.
 * In many cases you'll probably want to set this setting to 
 * @code 
 * aiPrimitiveType_LINE|aiPrimitiveType_POINT
 * @endcode
 * Together with the #aiProcess_Triangulate flag you can then be sure that
 * #aiFace::mNumIndices is always 3. 
 * @note Take a look at the @link data Data Structures page @endlink for
 * more information on the layout and winding order of a face.
 */
struct aiFace
{
	//! Number of indices defining this face. 3 for a triangle, >3 for polygon
	unsigned int mNumIndices; 

	//! Pointer to the indices array. Size of the array is given in numIndices.
	unsigned int* mIndices;   

#ifdef __cplusplus

	//! Default constructor
	aiFace()
	{
		mNumIndices = 0; mIndices = NULL;
	}

	//! Default destructor. Delete the index array
	~aiFace()
	{
		delete [] mIndices;
	}

	//! Copy constructor. Copy the index array
	aiFace( const aiFace& o)
	{
		mIndices = NULL;
		*this = o;
	}

	//! Assignment operator. Copy the index array
	const aiFace& operator = ( const aiFace& o)
	{
		if (&o == this)
			return *this;

		delete[] mIndices;
		mNumIndices = o.mNumIndices;
		mIndices = new unsigned int[mNumIndices];
		::memcpy( mIndices, o.mIndices, mNumIndices * sizeof( unsigned int));
		return *this;
	}

	//! Comparison operator. Checks whether the index array 
	//! of two faces is identical
	bool operator== (const aiFace& o) const
	{
		if (mIndices == o.mIndices)return true;
		else if (mIndices && mNumIndices == o.mNumIndices)
		{
			for (unsigned int i = 0;i < this->mNumIndices;++i)
				if (mIndices[i] != o.mIndices[i])return false;
			return true;
		}
		return false;
	}

	//! Inverse comparison operator. Checks whether the index 
	//! array of two faces is NOT identical
	bool operator != (const aiFace& o) const
	{
		return !(*this == o);
	}
#endif // __cplusplus
}; // struct aiFace


// ---------------------------------------------------------------------------
/** @brief A single influence of a bone on a vertex.
 */
struct aiVertexWeight
{
	//! Index of the vertex which is influenced by the bone.
	unsigned int mVertexId;

	//! The strength of the influence in the range (0...1).
	//! The influence from all bones at one vertex amounts to 1.
	float mWeight;     

#ifdef __cplusplus

	//! Default constructor
	aiVertexWeight() { }

	//! Initialisation from a given index and vertex weight factor
	//! \param pID ID
	//! \param pWeight Vertex weight factor
	aiVertexWeight( unsigned int pID, float pWeight) 
		: mVertexId( pID), mWeight( pWeight) 
	{ /* nothing to do here */ }

#endif // __cplusplus
};


// ---------------------------------------------------------------------------
/** @brief A single bone of a mesh.
 *
 *  A bone has a name by which it can be found in the frame hierarchy and by
 *  which it can be addressed by animations. In addition it has a number of 
 *  influences on vertices.
 */
struct aiBone
{
	//! The name of the bone. 
	C_STRUCT aiString mName;

	//! The number of vertices affected by this bone
	unsigned int mNumWeights;

	//! The vertices affected by this bone
	C_STRUCT aiVertexWeight* mWeights;

	//! Matrix that transforms from mesh space to bone space in bind pose
	C_STRUCT aiMatrix4x4 mOffsetMatrix;

#ifdef __cplusplus

	//! Default constructor
	aiBone()
	{
		mNumWeights = 0; mWeights = NULL;
	}

	//! Copy constructor
	aiBone(const aiBone& other)
	{
		mNumWeights = other.mNumWeights;
		mOffsetMatrix = other.mOffsetMatrix;
		mName = other.mName;

		if (other.mWeights && other.mNumWeights)
		{
			mWeights = new aiVertexWeight[mNumWeights];
			::memcpy(mWeights,other.mWeights,mNumWeights * sizeof(aiVertexWeight));
		}
	}

	//! Destructor - deletes the array of vertex weights
	~aiBone()
	{
		delete [] mWeights;
	}
#endif // __cplusplus
};

#ifndef AI_MAX_NUMBER_OF_COLOR_SETS
// ---------------------------------------------------------------------------
/** @def AI_MAX_NUMBER_OF_COLOR_SETS
 *  Maximum number of vertex color sets per mesh.
 *
 *  Normally: Diffuse, specular, ambient and emissive
 *  However one could use the vertex color sets for any other purpose, too.
 *
 *  @note Some internal structures expect (and assert) this value
 *    to be at least 4. For the moment it is absolutely safe to assume that
 *    this will never change.
 */
#	define AI_MAX_NUMBER_OF_COLOR_SETS 0x4
#endif // !! AI_MAX_NUMBER_OF_COLOR_SETS

#ifndef AI_MAX_NUMBER_OF_TEXTURECOORDS
// ---------------------------------------------------------------------------
/** @def AI_MAX_NUMBER_OF_TEXTURECOORDS
 *  Maximum number of texture coord sets (UV(W) channels) per mesh 
 *
 *  The material system uses the AI_MATKEY_UVWSRC_XXX keys to specify 
 *  which UVW channel serves as data source for a texture.
 *
 *  @note Some internal structures expect (and assert) this value
 *    to be at least 4. For the moment it is absolutely safe to assume that
 *    this will never change.
*/
#	define AI_MAX_NUMBER_OF_TEXTURECOORDS 0x4
#endif // !! AI_MAX_NUMBER_OF_TEXTURECOORDS


// ---------------------------------------------------------------------------
/** @brief Enumerates the types of geometric primitives supported by Assimp.
 *  
 *  @see aiFace Face data structure
 *  @see aiProcess_SortByPType Per-primitive sorting of meshes
 *  @see aiProcess_Triangulate Automatic triangulation
 *  @see AI_CONFIG_PP_SBP_REMOVE Removal of specific primitive types.
 */
enum aiPrimitiveType
{
	/** A point primitive. 
	 *
	 * This is just a single vertex in the virtual world, 
	 * #aiFace contains just one index for such a primitive.
	 */
	aiPrimitiveType_POINT       = 0x1,

	/** A line primitive. 
	 *
	 * This is a line defined through a start and an end position.
	 * #aiFace contains exactly two indices for such a primitive.
	 */
	aiPrimitiveType_LINE        = 0x2,

	/** A triangular primitive. 
	 *
	 * A triangle consists of three indices.
	 */
	aiPrimitiveType_TRIANGLE    = 0x4,

	/** A higher-level polygon with more than 3 edges.
	 *
	 * A triangle is a polygon, but polygon in this context means
	 * "all polygons that are not triangles". The "Triangulate"-Step
	 * is provided for your convenience, it splits all polygons in
	 * triangles (which are much easier to handle).
	 */
	aiPrimitiveType_POLYGON     = 0x8,


	/** This value is not used. It is just here to force the
	 *  compiler to map this enum to a 32 Bit integer.
	 */
	_aiPrimitiveType_Force32Bit = 0x9fffffff
}; //! enum aiPrimitiveType

// Get the #aiPrimitiveType flag for a specific number of face indices
#define AI_PRIMITIVE_TYPE_FOR_N_INDICES(n) \
	((n) > 3 ? aiPrimitiveType_POLYGON : (aiPrimitiveType)(1u << ((n)-1)))

// ---------------------------------------------------------------------------
/** @brief A mesh represents a geometry or model with a single material. 
*
* It usually consists of a number of vertices and a series of primitives/faces 
* referencing the vertices. In addition there might be a series of bones, each 
* of them addressing a number of vertices with a certain weight. Vertex data 
* is presented in channels with each channel containing a single per-vertex 
* information such as a set of texture coords or a normal vector.
* If a data pointer is non-null, the corresponding data stream is present.
* From C++-programs you can also use the comfort functions Has*() to
* test for the presence of various data streams.
*
* A Mesh uses only a single material which is referenced by a material ID.
* @note The mPositions member is usually not optional. However, vertex positions 
* *could* be missing if the AI_SCENE_FLAGS_INCOMPLETE flag is set in 
* @code
* aiScene::mFlags
* @endcode
*/
struct aiMesh
{
	/** Bitwise combination of the members of the #aiPrimitiveType enum.
	 * This specifies which types of primitives are present in the mesh.
	 * The "SortByPrimitiveType"-Step can be used to make sure the 
	 * output meshes consist of one primitive type each.
	 */
	unsigned int mPrimitiveTypes;

	/** The number of vertices in this mesh. 
	* This is also the size of all of the per-vertex data arrays
	*/
	unsigned int mNumVertices;

	/** The number of primitives (triangles, polygons, lines) in this  mesh. 
	* This is also the size of the mFaces array 
	*/
	unsigned int mNumFaces;

	/** Vertex positions. 
	* This array is always present in a mesh. The array is 
	* mNumVertices in size. 
	*/
	C_STRUCT aiVector3D* mVertices;

	/** Vertex normals. 
	* The array contains normalized vectors, NULL if not present. 
	* The array is mNumVertices in size. Normals are undefined for
	* point and line primitives. A mesh consisting of points and
	* lines only may not have normal vectors. Meshes with mixed
	* primitive types (i.e. lines and triangles) may have normals,
	* but the normals for vertices that are only referenced by
	* point or line primitives are undefined and set to QNaN (WARN:
	* qNaN compares to inequal to *everything*, even to qNaN itself.
	* Use code like this
	* @code
	* #define IS_QNAN(f) (f != f)
	* @endcode
	* to check whether a field is qnan).
	* @note Normal vectors computed by Assimp are always unit-length.
	* However, this needn't apply for normals that have been taken
	*   directly from the model file.
	*/
	C_STRUCT aiVector3D* mNormals;

	/** Vertex tangents. 
	* The tangent of a vertex points in the direction of the positive 
	* X texture axis. The array contains normalized vectors, NULL if
	* not present. The array is mNumVertices in size. A mesh consisting 
	* of points and lines only may not have normal vectors. Meshes with 
	* mixed primitive types (i.e. lines and triangles) may have 
	* normals, but the normals for vertices that are only referenced by
	* point or line primitives are undefined and set to QNaN. 
	* @note If the mesh contains tangents, it automatically also 
	* contains bitangents (the bitangent is just the cross product of
	* tangent and normal vectors). 
	*/
	C_STRUCT aiVector3D* mTangents;

	/** Vertex bitangents. 
	* The bitangent of a vertex points in the direction of the positive 
	* Y texture axis. The array contains normalized vectors, NULL if not
	* present. The array is mNumVertices in size. 
	* @note If the mesh contains tangents, it automatically also contains
	* bitangents. 
	*/
	C_STRUCT aiVector3D* mBitangents;

	/** Vertex color sets. 
	* A mesh may contain 0 to #AI_MAX_NUMBER_OF_COLOR_SETS vertex 
	* colors per vertex. NULL if not present. Each array is
	* mNumVertices in size if present.
	*/
	C_STRUCT aiColor4D* mColors[AI_MAX_NUMBER_OF_COLOR_SETS];

	/** Vertex texture coords, also known as UV channels.
	* A mesh may contain 0 to AI_MAX_NUMBER_OF_TEXTURECOORDS per
	* vertex. NULL if not present. The array is mNumVertices in size. 
	*/
	C_STRUCT aiVector3D* mTextureCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];

	/** Specifies the number of components for a given UV channel.
	* Up to three channels are supported (UVW, for accessing volume
	* or cube maps). If the value is 2 for a given channel n, the
	* component p.z of mTextureCoords[n][p] is set to 0.0f.
	* If the value is 1 for a given channel, p.y is set to 0.0f, too.
	* @note 4D coords are not supported 
	*/
	unsigned int mNumUVComponents[AI_MAX_NUMBER_OF_TEXTURECOORDS];

	/** The faces the mesh is constructed from. 
	* Each face refers to a number of vertices by their indices. 
	* This array is always present in a mesh, its size is given 
	* in mNumFaces. If the AI_SCENE_FLAGS_NON_VERBOSE_FORMAT
	* is NOT set each face references an unique set of vertices.
	*/
	C_STRUCT aiFace* mFaces;

	/** The number of bones this mesh contains. 
	* Can be 0, in which case the mBones array is NULL. 
	*/
	unsigned int mNumBones;

	/** The bones of this mesh. 
	* A bone consists of a name by which it can be found in the
	* frame hierarchy and a set of vertex weights.
	*/
	C_STRUCT aiBone** mBones;

	/** The material used by this mesh. 
	 * A mesh does use only a single material. If an imported model uses
	 * multiple materials, the import splits up the mesh. Use this value 
	 * as index into the scene's material list.
	 */
	unsigned int mMaterialIndex;

#ifdef __cplusplus

	//! Default constructor. Initializes all members to 0
	aiMesh()
	{
		mNumVertices    = 0; 
		mNumFaces       = 0;
		mPrimitiveTypes = 0;
		mVertices = NULL; mFaces    = NULL;
		mNormals  = NULL; mTangents = NULL;
		mBitangents = NULL;
		for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++)
		{
			mNumUVComponents[a] = 0;
			mTextureCoords[a] = NULL;
		}
		for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++)
			mColors[a] = NULL;
		mNumBones = 0; mBones = NULL;
		mMaterialIndex = 0;
	}

	//! Deletes all storage allocated for the mesh
	~aiMesh()
	{
		delete [] mVertices; 
		delete [] mNormals;
		delete [] mTangents;
		delete [] mBitangents;
		for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++)
			delete [] mTextureCoords[a];
		for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++)
			delete [] mColors[a];

		// DO NOT REMOVE THIS ADDITIONAL CHECK
		if (mNumBones && mBones)
		{
			for( unsigned int a = 0; a < mNumBones; a++)
				delete mBones[a];
			delete [] mBones;
		}
		delete [] mFaces;
	}

	//! Check whether the mesh contains positions. If no special scene flags
	//! (such as AI_SCENE_FLAGS_ANIM_SKELETON_ONLY) are set this will
	//! always return true 
	bool HasPositions() const 
		{ return mVertices != NULL && mNumVertices > 0; }

	//! Check whether the mesh contains faces. If no special scene flags
	//! are set this should always return true
	bool HasFaces() const 
		{ return mFaces != NULL && mNumFaces > 0; }

	//! Check whether the mesh contains normal vectors
	bool HasNormals() const 
		{ return mNormals != NULL && mNumVertices > 0; }

	//! Check whether the mesh contains tangent and bitangent vectors
	//! It is not possible that it contains tangents and no bitangents
	//! (or the other way round). The existence of one of them
	//! implies that the second is there, too.
	bool HasTangentsAndBitangents() const 
		{ return mTangents != NULL && mBitangents != NULL && mNumVertices > 0; }

	//! Check whether the mesh contains a vertex color set
	//! \param pIndex Index of the vertex color set
	bool HasVertexColors( unsigned int pIndex) const
	{ 
		if( pIndex >= AI_MAX_NUMBER_OF_COLOR_SETS) 
			return false; 
		else 
			return mColors[pIndex] != NULL && mNumVertices > 0; 
	}

	//! Check whether the mesh contains a texture coordinate set
	//! \param pIndex Index of the texture coordinates set
	bool HasTextureCoords( unsigned int pIndex) const
	{ 
		if( pIndex >= AI_MAX_NUMBER_OF_TEXTURECOORDS) 
			return false; 
		else 
			return mTextureCoords[pIndex] != NULL && mNumVertices > 0; 
	}

	//! Get the number of UV channels the mesh contains
	unsigned int GetNumUVChannels() const 
	{
		unsigned int n = 0;
		while (n < AI_MAX_NUMBER_OF_TEXTURECOORDS && mTextureCoords[n])++n;
		return n;
	}

	//! Get the number of vertex color channels the mesh contains
	unsigned int GetNumColorChannels() const 
	{
		unsigned int n = 0;
		while (n < AI_MAX_NUMBER_OF_COLOR_SETS && mColors[n])++n;
		return n;
	}

	//! Check whether the mesh contains bones
	inline bool HasBones() const
		{ return mBones != NULL && mNumBones > 0; }

#endif // __cplusplus
};

#ifdef __cplusplus
}
#endif //! extern "C"
#endif // __AI_MESH_H_INC

