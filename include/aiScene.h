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

/** @file Defines the data structures in which the imported scene is returned. */
#ifndef AI_SCENE_H_INC
#define AI_SCENE_H_INC

#include "aiTypes.h"
#include "aiMesh.h"
#include "aiMaterial.h"
#include "aiTexture.h"
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
	C_STRUCT aiString mName;

	/** The transformation relative to the node's parent. */
	aiMatrix4x4 mTransformation;

	/** Parent node. NULL if this node is the root node. */
	C_STRUCT aiNode* mParent;

	/** The number of child nodes of this node. */
	unsigned int mNumChildren;
	/** The child nodes of this node. NULL if mNumChildren is 0. */

	C_STRUCT aiNode** mChildren;

	/** The number of meshes of this node. */
	unsigned int mNumMeshes;

	/** The meshes of this node. Each entry is an index into the mesh */
	unsigned int* mMeshes;

#ifdef __cplusplus
	/** Constructor */
	aiNode() 
	{ 
		// set all members to zero by default
		mParent = NULL; 
		mNumChildren = 0; mChildren = NULL;
		mNumMeshes = 0; mMeshes = NULL;
	}

	/** Construction from a specific name */
	aiNode(const std::string& name) 
	{ 
		// set all members to zero by default
		mParent = NULL; 
		mNumChildren = 0; mChildren = NULL;
		mNumMeshes = 0; mMeshes = NULL;
		mName = name;
	}

	/** Destructor */
	~aiNode()
	{
		// delete al children recursively
		for( unsigned int a = 0; a < mNumChildren; a++)
			delete mChildren[a];
		delete [] mChildren;
		delete [] mMeshes;
	}
#endif // __cplusplus
};

//! @def AI_SCENE_FLAGS_ANIM_SKELETON_ONLY
//! Specifies that no full model but only an animation skeleton has been
//! imported. There are no materials in this case. There are no
//! textures in this case. But there is a node graph, animation channels
//! and propably meshes with bones.
#define AI_SCENE_FLAGS_ANIM_SKELETON_ONLY	0x1


// ---------------------------------------------------------------------------
/** The root structure of the imported data. 
* 
* Everything that was imported from the given file can be accessed from here.
*/
// ---------------------------------------------------------------------------
struct aiScene
{

	/** Any combination of the AI_SCENE_FLAGS_XXX flags */
	unsigned int mFlags;


	/** The root node of the hierarchy. 
	* 
	* There will always be at least the root node if the import
	* was successful. Presence of further nodes depends on the 
	* format and content of the imported file.
	*/
	C_STRUCT aiNode* mRootNode;



	/** The number of meshes in the scene. */
	unsigned int mNumMeshes;

	/** The array of meshes. 
	*
	* Use the indices given in the aiNode structure to access 
	* this array. The array is mNumMeshes in size.
	*/
	C_STRUCT aiMesh** mMeshes;



	/** The number of materials in the scene. */
	unsigned int mNumMaterials;

	/** The array of materials. 
	* 
	* Use the index given in each aiMesh structure to access this
	* array. The array is mNumMaterials in size.
	*/
	C_STRUCT aiMaterial** mMaterials;



	/** The number of animations in the scene. */
	unsigned int mNumAnimations; 

	/** The array of animations. 
	*
	* All animations imported from the given file are listed here.
	* The array is mNumAnimations in size.
	*/
	C_STRUCT aiAnimation** mAnimations;



	/** The number of textures embedded into the file */
	unsigned int mNumTextures;

	/** The array of embedded textures.
	* 
	* Not many file formats embedd their textures into the file.
	* An example is Quake's MDL format (which is also used by
	* some GameStudio™ versions)
	*/
	C_STRUCT aiTexture** mTextures;

#ifdef __cplusplus

	//! Default constructor
	aiScene()
	{
		// set all members to zero by default
		mRootNode = NULL;
		mNumMeshes = 0; mMeshes = NULL;
		mNumMaterials = 0; mMaterials = NULL;
		mNumAnimations = 0; mAnimations = NULL;
		mNumTextures = 0; mTextures = NULL;
		mFlags = 0;
	}

	//! Destructor
	~aiScene()
	{
		// delete all subobjects recursively
		delete mRootNode;
		if (mNumMeshes) // fix to make the d'tor work for invalid scenes, too
		{
			for( unsigned int a = 0; a < mNumMeshes; a++)
				delete mMeshes[a];
			delete [] mMeshes;
		}
		if (mNumMaterials) // fix to make the d'tor work for invalid scenes, too
		{
			for( unsigned int a = 0; a < mNumMaterials; a++)
				delete mMaterials[a];
			delete [] mMaterials;
		}
		if (mNumAnimations) // fix to make the d'tor work for invalid scenes, too
		{
			for( unsigned int a = 0; a < mNumAnimations; a++)
				delete mAnimations[a];
			delete [] mAnimations;
		}
		if (mNumTextures) // fix to make the d'tor work for invalid scenes, too
		{
			for( unsigned int a = 0; a < mNumTextures; a++)
				delete mTextures[a];
			delete [] mTextures;
		}
	}
#endif // __cplusplus
};

#ifdef __cplusplus
}
#endif

#endif // AI_SCENE_H_INC
