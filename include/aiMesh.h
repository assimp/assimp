/** @file Defines the data structures in which the imported geometry is returned. */
#ifndef AI_MESH_H_INC
#define AI_MESH_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
/** A single face in a mesh, referring to multiple vertices. 
*  If mNumIndices is 3, the face is a triangle, for mNumIndices > 3 it's a polygon.
*/
// ---------------------------------------------------------------------------
struct aiFace
{
	unsigned int mNumIndices; ///< Number of indices defining this face. 3 for a triangle, >3 for polygon
	unsigned int* mIndices;   ///< Pointer to the indices array. Size of the array is given in numIndices.

#ifdef __cplusplus
	aiFace()
	{
		mNumIndices = 0; mIndices = NULL;
	}

	~aiFace()
	{
		delete [] mIndices;
	}

	aiFace( const aiFace& o)
	{
		mIndices = NULL;
		*this = o;
	}

	const aiFace& operator = ( const aiFace& o)
	{
		if (&o == this)
			return *this;

		delete mIndices;
		mNumIndices = o.mNumIndices;
		mIndices = new unsigned int[mNumIndices];
		memcpy( mIndices, o.mIndices, mNumIndices * sizeof( unsigned int));
		return *this;
	}

#endif // __cplusplus
};


// ---------------------------------------------------------------------------
/** A single influence of a bone on a vertex. */
// ---------------------------------------------------------------------------
struct aiVertexWeight
{
	unsigned int mVertexId; ///< Index of the vertex which is influenced by the bone.
	float mWeight;     ///< The strength of the influence in the range (0...1). The influence from all bones at one vertex amounts to 1.

#ifdef __cplusplus
	aiVertexWeight() { }
	aiVertexWeight( unsigned int pID, float pWeight) : mVertexId( pID), mWeight( pWeight) { }
#endif // __cplusplus
};


// ---------------------------------------------------------------------------
/** A single bone of a mesh. A bone has a name by which it can be found 
* in the frame hierarchy and by which it can be addressed by animations. 
* In addition it has a number of influences on vertices.
*/
// ---------------------------------------------------------------------------
struct aiBone
{
	aiString mName; ///< The name of the bone. 
	unsigned int mNumWeights; ///< The number of vertices affected by this bone
	aiVertexWeight* mWeights; ///< The vertices affected by this bone
	aiMatrix4x4 mOffsetMatrix; ///< Matrix that transforms from mesh space to bone space in bind pose

#ifdef __cplusplus
	aiBone()
	{
		mNumWeights = 0; mWeights = NULL;
	}

	~aiBone()
	{
		delete [] mWeights;
	}
#endif // __cplusplus
};


/** Maximum number of vertex color sets per mesh.
*
* Diffuse, specular, ambient and emissive
*/
#define AI_MAX_NUMBER_OF_COLOR_SETS 0x4


/** Maximum number of texture coord sets (UV channels) per mesh 
*/
#define AI_MAX_NUMBER_OF_TEXTURECOORDS 0x4

// ---------------------------------------------------------------------------
/** A mesh represents a geometry or model with a single material. 
*
* It usually consists of a number of vertices and a series of primitives/faces 
* referencing the vertices. In addition there might be a series of bones, each 
* of them addressing a number of vertices with a certain weight. Vertex data is
* presented in channels with each channel containing a single per-vertex 
* information such as a set of texture coords or a normal vector.
* If a data pointer is non-null, the corresponding data stream is present.
* From C++-programs you can also use the comfort functions Has*() to
* test for the presence of various data streams.
*
* A Mesh uses only a single material which is referenced by a material ID.
*/
struct aiMesh
{
	/** The number of vertices in this mesh. 
	* This is also the size of all of the per-vertex data arrays
	*/
	unsigned int mNumVertices;

	/** The number of primitives (triangles, polygones, lines) in this  mesh. 
	* This is also the size of the mFaces array 
	*/
	unsigned int mNumFaces;

	/** Vertex positions. 
	* This array is always present in a mesh. The array is 
	* mNumVertices in size. 
	*/
	aiVector3D_t* mVertices;

	/** Vertex normals. 
	* The array contains normalized vectors, NULL if not present. 
	* The array is mNumVertices in size. 
	*/
	aiVector3D_t* mNormals;

	/** Vertex tangents. 
	* The tangent of a vertex points in the direction of the positive 
	* X texture axis. The array contains normalized vectors, NULL if
	* not present. The array is mNumVertices in size. 
	* @note If the mesh contains tangents, it automatically also 
	* contains bitangents. 
	*/
	aiVector3D_t* mTangents;

	/** Vertex bitangents. 
	* The bitangent of a vertex points in the direction of the positive 
	* Y texture axis. The array contains normalized vectors, NULL if not
	* present. The array is mNumVertices in size. 
	* @note If the mesh contains tangents, it automatically also contains
	* bitangents. 
	*/
	aiVector3D_t* mBitangents;

	/** Vertex color sets. 
	* A mesh may contain 0 to #AI_MAX_NUMBER_OF_COLOR_SETS vertex 
	* colors per vertex. NULL if not present. Each array is
	* mNumVertices in size if present.
	*/
	aiColor4D_t* mColors[AI_MAX_NUMBER_OF_COLOR_SETS];

	/** Vertex texture coords, also known as UV channels.
	* A mesh may contain 0 to AI_MAX_NUMBER_OF_TEXTURECOORDS per
	* vertex. NULL if not present. The array is mNumVertices in size. 
	*/
	aiVector3D_t* mTextureCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];

	/** Specifies the number of components for a given UV channel.
	* Up to three channels are supported (UVW, for accessing volume
	* or cube maps). If the value is 2 for a given channel n, the
	* component p.z of mTextureCoords[n][p] is set to 0.0f.
	* If the value is 1 for a given channel, p.y is set to 0.0f, too.
	* @note 4D coords are not supported 
	*/
	unsigned int mNumUVComponents[AI_MAX_NUMBER_OF_TEXTURECOORDS];

	/** The faces the mesh is contstructed from. 
	* Each face referres to a number of vertices by their indices. 
	* This array is always present in a mesh, its size is given 
	* in mNumFaces.
	*/
	aiFace* mFaces;

	/** The number of bones this mesh contains. 
	* Can be 0, in which case the mBones array is NULL. 
	*/
	unsigned int mNumBones;

	/** The bones of this mesh. 
	* A bone consists of a name by which it can be found in the
	* frame hierarchy and a set of vertex weights.
	*/
	aiBone** mBones;

	/** The material used by this mesh. 
	 * A mesh does use only a single material. If an imported model uses multiple materials,
	 * the import splits up the mesh. Use this value as index into the scene's material list.
	 */
	unsigned int mMaterialIndex;

#ifdef __cplusplus
	aiMesh()
	{
		mNumVertices = 0; mNumFaces = 0;
		mVertices = NULL; mFaces = NULL;
		mNormals = NULL; mTangents = NULL;
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

	~aiMesh()
	{
		delete [] mVertices; 
		delete [] mFaces;
		delete [] mNormals;
		delete [] mTangents;
		delete [] mBitangents;
		for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++)
			delete [] mTextureCoords[a];
		for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++)
			delete [] mColors[a];
		for( unsigned int a = 0; a < mNumBones; a++)
			delete mBones[a];
		delete [] mBones;
	}

	bool HasNormals() const { return mNormals != NULL; }
	bool HasTangentsAndBitangents() const { return mTangents != NULL && mBitangents != NULL; }
	bool HasVertexColors( unsigned int pIndex) 
	{ 
		if( pIndex >= AI_MAX_NUMBER_OF_COLOR_SETS) 
			return false; 
		else 
			return mColors[pIndex] != NULL; 
	}
	bool HasTextureCoords( unsigned int pIndex) 
	{ 
		if( pIndex > AI_MAX_NUMBER_OF_TEXTURECOORDS) 
			return false; 
		else 
			return mTextureCoords[pIndex] != NULL; 
	}
	bool HasBones() const { return mBones != NULL; }

#endif // __cplusplus
};

#ifdef __cplusplus
}
#endif

#endif // AI_MESH_H_INC