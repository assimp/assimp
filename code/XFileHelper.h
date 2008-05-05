/** @file Defines the helper data structures for importing XFiles */
#ifndef AI_XFILEHELPER_H_INC
#define AI_XFILEHELPER_H_INC

#include <string>
#include <vector>

#include "../include/aiTypes.h"
#include "../include/aiQuaternion.h"
#include "../include/aiMesh.h"
#include "../include/aiAnim.h"

namespace Assimp
{
namespace XFile
{

/** Helper structure representing a XFile mesh face */
struct Face
{
	std::vector<unsigned int> mIndices;
};

/** Helper structure representing a XFile material */
struct Material
{
	std::string mName;
	bool mIsReference; // if true, mName holds a name by which the actual material can be found in the material list
	aiColor4D mDiffuse;
	float mSpecularExponent;
	aiColor3D mSpecular;
	aiColor3D mEmissive;
	std::vector<std::string> mTextures;

	Material() { mIsReference = false; }
};

/** Helper structure to represent a bone weight */
struct BoneWeight
{
	unsigned int mVertex;
	float mWeight;
};

/** Helper structure to represent a bone in a mesh */
struct Bone
{
	std::string mName;
	std::vector<BoneWeight> mWeights;
	aiMatrix4x4 mOffsetMatrix;
};

/** Helper structure to represent an XFile mesh */
struct Mesh
{
	std::vector<aiVector3D> mPositions;
	std::vector<Face> mPosFaces;
	std::vector<aiVector3D> mNormals;
	std::vector<Face> mNormFaces;
	unsigned int mNumTextures;
	std::vector<aiVector2D> mTexCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];
	unsigned int mNumColorSets;
	std::vector<aiColor4D> mColors[AI_MAX_NUMBER_OF_COLOR_SETS];

	std::vector<unsigned int> mFaceMaterials;
	std::vector<Material> mMaterials;

	std::vector<Bone> mBones;

	Mesh() { mNumTextures = 0; mNumColorSets = 0; }
};

/** Helper structure to represent a XFile frame */
struct Node
{
	std::string mName;
	aiMatrix4x4 mTrafoMatrix;
	Node* mParent;
	std::vector<Node*> mChildren;
	std::vector<Mesh*> mMeshes;

	Node() { mParent = NULL; }
	Node( Node* pParent) { mParent = pParent; }
	~Node() 
	{
		for( unsigned int a = 0; a < mChildren.size(); a++)
			delete mChildren[a];
		for( unsigned int a = 0; a < mMeshes.size(); a++)
			delete mMeshes[a];
	}
};

struct MatrixKey
{
	double mTime;
	aiMatrix4x4 mMatrix;
};

/** Helper structure representing a single animated bone in a XFile */
struct AnimBone
{
	std::string mBoneName;
	std::vector<aiVectorKey> mPosKeys;  // either three separate key sequences for position, rotation, scaling
	std::vector<aiQuatKey> mRotKeys;
	std::vector<aiVectorKey> mScaleKeys;
	std::vector<MatrixKey> mTrafoKeys; // or a combined key sequence of transformation matrices.
};

/** Helper structure to represent an animation set in a XFile */
struct Animation
{
	std::string mName;
	std::vector<AnimBone*> mAnims;

	~Animation() 
	{
		for( unsigned int a = 0; a < mAnims.size(); a++)
			delete mAnims[a];
	}
};

/** Helper structure analogue to aiScene */
struct Scene
{
	Node* mRootNode;

	std::vector<Mesh*> mGlobalMeshes; // global meshes found outside of any frames
	std::vector<Material> mGlobalMaterials; // global materials found outside of any meshes.

	std::vector<Animation*> mAnims;
	unsigned int mAnimTicksPerSecond;

	Scene() { mRootNode = NULL; mAnimTicksPerSecond = 0; }
	~Scene()
	{
		delete mRootNode;
		for( unsigned int a = 0; a < mGlobalMeshes.size(); a++)
			delete mGlobalMeshes[a];
		for( unsigned int a = 0; a < mAnims.size(); a++)
			delete mAnims[a];
	}
};

} // end of namespace XFile
} // end of namespace Assimp

#endif // AI_XFILEHELPER_H_INC