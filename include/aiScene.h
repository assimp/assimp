/** @file Defines the data structures in which the imported scene is returned. */
#ifndef AI_SCENE_H_INC
#define AI_SCENE_H_INC

#include "aiTypes.h"
#include "aiMesh.h"
#include "aiMaterial.h"
#include "aiAnim.h"

#ifdef __cplusplus
extern "C" {
#endif


// ---------------------------------------------------------------------------
/** A node in the imported hierarchy. 
*
* Each node has name, a parent node (except for the root node), 
* a transformation relative to its parent and possibly several child nodes.
* Simple file formats don't support hierarchical structures, for these formats 
* the imported scene does consist of only a single root node with no childs.
*/
// ---------------------------------------------------------------------------
struct aiNode
{
	/** The name of the node. 
	*
	* The name might be empty (length of zero) but all nodes which 
	* need to be accessed afterwards by bones or anims are usually named.
	*/
	aiString mName;

	/** The transformation relative to the node's parent. */
	aiMatrix4x4 mTransformation;

	/** Parent node. NULL if this node is the root node. */
	aiNode* mParent;

	/** The number of child nodes of this node. */
	unsigned int mNumChildren;
	/** The child nodes of this node. NULL if mNumChildren is 0. */

	aiNode** mChildren;

	/** The number of meshes of this node. */
	unsigned int mNumMeshes;

	/** The meshes of this node. Each entry is an index into the mesh */
	unsigned int* mMeshes;

#ifdef __cplusplus
	/** Constructor */
	aiNode() 
	{ 
		mParent = NULL; 
		mNumChildren = 0; mChildren = NULL;
		mNumMeshes = 0; mMeshes = NULL;
	}

	/** Destructor */
	~aiNode()
	{
		for( unsigned int a = 0; a < mNumChildren; a++)
			delete mChildren[a];
		delete [] mChildren;
		delete [] mMeshes;
	}
#endif // __cplusplus
};


// ---------------------------------------------------------------------------
/** The root structure of the imported data. 
* 
* Everything that was imported from the given file can be accessed from here.
*/
// ---------------------------------------------------------------------------
struct aiScene
{
	/** The root node of the hierarchy. 
	* 
	* There will always be at least the root node if the import
	* was successful. Presence of further nodes depends on the 
	* format and content of the imported file.
	*/
	aiNode* mRootNode;

	/** The number of meshes in the scene. */
	unsigned int mNumMeshes;

	/** The array of meshes. 
	*
	* Use the indices given in the aiNode structure to access 
	* this array. The array is mNumMeshes in size.
	*/
	aiMesh** mMeshes;

	/** The number of materials in the scene. */
	unsigned int mNumMaterials;

	/** The array of materials. 
	* 
	* Use the index given in each aiMesh structure to access this
	* array. The array is mNumMaterials in size.
	*/
	aiMaterial** mMaterials;

	/** The number of animations in the scene. */
	unsigned int mNumAnimations; 

	/** The array of animations. 
	*
	* All animations imported from the given file are listed here.
	* The array is mNumAnimations in size.
	*/
	aiAnimation** mAnimations;

#ifdef __cplusplus
	aiScene()
	{
		mRootNode = NULL;
		mNumMeshes = 0; mMeshes = NULL;
		mNumMaterials = 0; mMaterials = NULL;
		mNumAnimations = 0; mAnimations = NULL;
	}

	~aiScene()
	{
		delete mRootNode;
		for( unsigned int a = 0; a < mNumMeshes; a++)
			delete mMeshes[a];
		delete [] mMeshes;
		for( unsigned int a = 0; a < mNumMaterials; a++)
			delete mMaterials[a];
		delete [] mMaterials;
		for( unsigned int a = 0; a < mNumAnimations; a++)
			delete mAnimations[a];
		delete [] mAnimations;
	}
#endif // __cplusplus
};

#ifdef __cplusplus
}
#endif

#endif // AI_SCENE_H_INC
