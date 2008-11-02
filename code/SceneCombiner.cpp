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

/** @file Implements Assimp::SceneCombiner. This is a smart utility
 *    class that can be used to combine several scenes, meshes, ...
 *    in one.
 */

#include "AssimpPCH.h"
#include "SceneCombiner.h"
#include "fast_atof.h"

namespace Assimp
{

// ------------------------------------------------------------------------------------------------
// Add a prefix to a string
inline void PrefixString(aiString& string,const char* prefix, unsigned int len)
{
	// Add the prefix
	::memmove(string.data+len,string.data,string.length+1);
	::memcpy (string.data, prefix, len);

	// And update the string's length
	string.length += len;
}

// ------------------------------------------------------------------------------------------------
// Add a name prefix to all nodes in a hierarchy
void SceneCombiner::AddNodePrefixes(aiNode* node, const char* prefix, unsigned int len)
{
	ai_assert(NULL != prefix);

	PrefixString(node->mName,prefix,len);

	// Process all children recursively
	for (unsigned int i = 0; i < node->mNumChildren;++i)
		AddNodePrefixes(node->mChildren[i],prefix,len);
}

// ------------------------------------------------------------------------------------------------
// Merges two scenes. Currently only used by the LWS loader.
void SceneCombiner::MergeScenes(aiScene* dest,std::vector<aiScene*>& src,
	unsigned int flags)
{
	ai_assert(NULL != dest);
	ai_assert(0 == dest->mNumTextures);
	ai_assert(0 == dest->mNumLights);
	ai_assert(0 == dest->mNumCameras);
	ai_assert(0 == dest->mNumMeshes);
	ai_assert(0 == dest->mNumMaterials);

	if (src.empty())return;

	// some iterators will be needed multiple times
	std::vector<aiScene*>::iterator begin = src.begin(),
		end = src.end(), cur;

	// this helper array is used as lookup table several times
	std::vector<unsigned int> offset(src.size());
	std::vector<unsigned int>::iterator ofsbegin = offset.begin(),
		offend = offset.end(), ofscur;
	
	unsigned int cnt;
	bool bNeedPrefix = false;

	// First find out how large the respective output arrays must be
	for ( cur = begin; cur != end; ++cur)
	{
		dest->mNumTextures   += (*cur)->mNumTextures;
		dest->mNumMaterials  += (*cur)->mNumMaterials;
		dest->mNumMeshes     += (*cur)->mNumMeshes;
		dest->mNumLights     += (*cur)->mNumLights;
		dest->mNumCameras    += (*cur)->mNumCameras;
		dest->mNumAnimations += (*cur)->mNumAnimations;

		if ((*cur)->mNumAnimations > 0 ||
			(*cur)->mNumCameras    > 0 ||
			(*cur)->mNumLights     > 0)
		{
			bNeedPrefix = true;
		}
	}

	// generate the output texture list + an offset table
	if (dest->mNumTextures)
	{
		aiTexture** pip = dest->mTextures = new aiTexture*[dest->mNumMaterials];
		for ( cur = begin, ofscur = ofsbegin,cnt = 0; cur != end; ++cur,++ofscur)
		{
			for (unsigned int i = 0; i < (*cur)->mNumTextures;++i,++pip)
				*pip = (*cur)->mTextures[i];

			*ofscur = cnt;
			cnt += (*cur)->mNumTextures;
		}
	}

	// generate the output material list + an offset table
	if (dest->mNumMaterials)
	{
		aiMaterial** pip = dest->mMaterials = new aiMaterial*[dest->mNumMaterials];
		for ( cur = begin, ofscur = ofsbegin,cnt = 0; cur != end; ++cur,++ofscur)
		{
			for (unsigned int i = 0; i < (*cur)->mNumMaterials;++i,++pip)
			{
				*pip = (*cur)->mMaterials[i];

				if ((*cur)->mNumTextures != dest->mNumTextures)
				{
					// We need to update all texture indices of the mesh.
					// So we need to search for a material property like 
					// that follows the following pattern: "$tex.file.<s>.<n>"
					// where s is the texture type (i.e. diffuse) and n is 
					// the index of the texture.

					for (unsigned int a = 0; a < (*pip)->mNumProperties;++a)
					{
						aiMaterialProperty* prop = (*pip)->mProperties[a];
						if (!strncmp(prop->mKey.data,"$tex.file",9))
						{
							// Check whether this texture is an embedded texture.
							// In this case the property looks like this: *<n>,
							// where n is the index of the texture.
							aiString& s = *((aiString*)prop->mData);
							if ('*' == s.data[0])
							{
								// Offset the index and write it back ..
								const unsigned int idx = strtol10(&s.data[1]) + *ofscur;
								itoa10(&s.data[1],sizeof(s.data)-1,idx);
							}
						}
					}
				}
			}

			*ofscur = cnt;
			cnt += (*cur)->mNumMaterials;
		}
	}

	// generate the output mesh list + again an offset table
	if (dest->mNumMeshes)
	{
		aiMesh** pip = dest->mMeshes = new aiMesh*[dest->mNumMeshes];
		for ( cur = begin, ofscur = ofsbegin,cnt = 0; cur != end; ++cur,++ofscur)
		{
			for (unsigned int i = 0; i < (*cur)->mNumMeshes;++i,++pip)
			{
				*pip = (*cur)->mMeshes[i];

				// update the material index of the mesh
				(*pip)->mMaterialIndex += *ofscur;
			}

			// reuse the offset array - store now the mesh offset in it
			*ofscur = cnt;
			cnt += (*cur)->mNumMeshes;
		}
	}

	// Now generate the output node graph. We need to make those
	// names in the graph that are referenced by anims or lights
	// or cameras unique. So we add a prefix to them ... $SC1_
	// First step for now: find out which nodes are referenced by
	// anim bones or cameras and add the prefix to their names.

	if (bNeedPrefix)
	{
		// Allocate space for light sources, cameras and animations
		aiLight** ppLights = dest->mLights = (dest->mNumLights 
			? new aiLight*[dest->mNumLights] : NULL);

		aiCamera** ppCameras = dest->mCameras = (dest->mNumCameras 
			? new aiCamera*[dest->mNumCameras] : NULL);

		aiAnimation** ppAnims = dest->mAnimations = (dest->mNumAnimations 
			? new aiAnimation*[dest->mNumAnimations] : NULL);


		for (cur = begin, cnt = 0; cur != end; ++cur)
		{
			char buffer[10];
			buffer[0] = '$';
			buffer[1] = 'S';
			buffer[2] = 'C';

			char* sz = &buffer[itoa10(&buffer[3],sizeof(buffer)-3, cnt++)+2];
			*sz++ = '_';
			*sz++ = '\0';

			const unsigned int len = (unsigned int)::strlen(buffer);
			AddNodePrefixes((*cur)->mRootNode,buffer,len);

			// Copy light sources, add the prefix to them, too
			for (unsigned int i = 0; i < (*cur)->mNumLights;++i,++ppLights)
			{
				*ppLights = (*cur)->mLights[i];
				PrefixString((*ppLights)->mName,buffer,len);
			}
			// Copy cameras, add the prefix to them, too
			for (unsigned int i = 0; i < (*cur)->mNumCameras;++i,++ppCameras)
			{
				*ppCameras = (*cur)->mCameras[i];
				PrefixString((*ppCameras)->mName,buffer,len);
			}

			// Copy animations, add the prefix to them, too
			for (unsigned int i = 0; i < (*cur)->mNumAnimations;++i,++ppAnims)
			{
				*ppAnims = (*cur)->mAnimations[i];
				PrefixString((*ppAnims)->mName,buffer,len);
			}
		}
	}

	// now delete all input scenes
	for (cur = begin; cur != end; ++cur)
	{
		aiScene* deleteMe = *cur;

		// We need to delete the arrays before the destructor is called -
		// we are reusing the array members
		delete[] deleteMe->mMeshes;     deleteMe->mMeshes     = NULL;
		delete[] deleteMe->mCameras;    deleteMe->mCameras    = NULL;
		delete[] deleteMe->mLights;     deleteMe->mLights     = NULL;
		delete[] deleteMe->mMaterials;  deleteMe->mMaterials  = NULL;
		delete[] deleteMe->mAnimations; deleteMe->mAnimations = NULL;

		delete[] deleteMe->mRootNode->mChildren;
		deleteMe->mRootNode->mChildren = NULL;

		// Now we can safely delete the scene
		delete[] deleteMe;
		AI_DEBUG_INVALIDATE_PTR(*cur);
	}
}


// ------------------------------------------------------------------------------------------------
void SceneCombiner::MergeScenes(aiScene* dest, const aiScene* master, 
	std::vector<AttachmentInfo>& src,
	unsigned int flags)
{
}


// ------------------------------------------------------------------------------------------------
void SceneCombiner::MergeMeshes(aiMesh* dest,std::vector<aiMesh*>& src,
	unsigned int flags)
{
}

}