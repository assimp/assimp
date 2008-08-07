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

/** @file Implementation of the post processing step to validate 
 * the data structure returned by Assimp
 */

// STL headers
#include <vector>
#include <assert.h>

// internal headers
#include "ValidateDataStructure.h"
#include "BaseImporter.h"
#include "StringComparison.h"
#include "fast_atof.h"

// public ASSIMP headers
#include "../include/DefaultLogger.h"
#include "../include/aiPostProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"

// CRT headers
#include <stdarg.h>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ValidateDSProcess::ValidateDSProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
ValidateDSProcess::~ValidateDSProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool ValidateDSProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_ValidateDataStructure) != 0;
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::ReportError(const char* msg,...)
{
	ai_assert(NULL != msg);

	va_list args;
	va_start(args,msg);

	char szBuffer[3000];

	int iLen;
#if _MSC_VER >= 1400
	iLen = vsprintf_s(szBuffer,msg,args);
#else
	iLen = vsprintf(szBuffer,msg,args);
#endif

	if (0 >= iLen)
	{
		// :-) should not happen ...
		throw new ImportErrorException("Idiot ... learn coding!");
	}
	va_end(args);
	ai_assert(false);
	throw new ImportErrorException("Validation failed: " + std::string(szBuffer,iLen));
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::ReportWarning(const char* msg,...)
{
	ai_assert(NULL != msg);

	va_list args;
	va_start(args,msg);

	char szBuffer[3000];

	int iLen;
#if _MSC_VER >= 1400
	iLen = vsprintf_s(szBuffer,msg,args);
#else
	iLen = vsprintf(szBuffer,msg,args);
#endif

	if (0 >= iLen)
	{
		// :-) should not happen ...
		throw new ImportErrorException("Idiot ... learn coding!");
	}
	va_end(args);
	DefaultLogger::get()->warn("Validation warning: " + std::string(szBuffer,iLen));
}
// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void ValidateDSProcess::Execute( aiScene* pScene)
{
	this->mScene = pScene;
	DefaultLogger::get()->debug("ValidateDataStructureProcess begin");

	// validate all meshes
	if (pScene->mNumMeshes)
	{
		if (!pScene->mMeshes)
		{
			this->ReportError("aiScene::mMeshes is NULL (aiScene::mNumMeshes is %i)",
				pScene->mNumMeshes);
		}
		for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		{
			if (!pScene->mMeshes[i])
			{
				this->ReportError("aiScene::mMeshes[%i] is NULL (aiScene::mNumMeshes is %i)",
					i,pScene->mNumMeshes);
			}
			this->Validate(pScene->mMeshes[i]);
		}
	}
	else if (!(this->mScene->mFlags & AI_SCENE_FLAGS_ANIM_SKELETON_ONLY))
	{
		this->ReportError("aiScene::mNumMeshes is 0. At least one mesh must be there");
	}

	// validate all animations
	if (pScene->mNumAnimations)
	{
		if (!pScene->mAnimations)
		{
			this->ReportError("aiScene::mAnimations is NULL (aiScene::mNumAnimations is %i)",
				pScene->mNumAnimations);
		}
		for (unsigned int i = 0; i < pScene->mNumAnimations;++i)
		{
			if (!pScene->mAnimations[i])
			{
				this->ReportError("aiScene::mAnimations[%i] is NULL (aiScene::mNumAnimations is %i)",
					i,pScene->mNumAnimations);
			}
			this->Validate(pScene->mAnimations[i]);

			// check whether there are duplicate animation names
			for (unsigned int a = i+1; a < pScene->mNumAnimations;++a)
			{
				if (pScene->mAnimations[i]->mName == pScene->mAnimations[a]->mName)
				{
					this->ReportError("aiScene::mAnimations[%i] has the same name as "
						"aiScene::mAnimations[%i]",i,a);
				}
			}
		}
	}
	else if (this->mScene->mFlags & AI_SCENE_FLAGS_ANIM_SKELETON_ONLY)
	{
		this->ReportError("aiScene::mNumAnimations is 0 and the "
			"AI_SCENE_FLAGS_ANIM_SKELETON_ONLY flag is set.");
	}

	// validate all textures
	if (pScene->mNumTextures)
	{
		if (!pScene->mTextures)
		{
			this->ReportError("aiScene::mTextures is NULL (aiScene::mNumTextures is %i)",
				pScene->mNumTextures);
		}
		for (unsigned int i = 0; i < pScene->mNumTextures;++i)
		{
			if (!pScene->mTextures[i])
			{
				this->ReportError("aiScene::mTextures[%i] is NULL (aiScene::mNumTextures is %i)",
					i,pScene->mNumTextures);
			}
			this->Validate(pScene->mTextures[i]);
		}
	}

	// validate all materials
	if (pScene->mNumMaterials)
	{
		if (!pScene->mMaterials)
		{
			this->ReportError("aiScene::mMaterials is NULL (aiScene::mNumMaterials is %i)",
				pScene->mNumMaterials);
		}
		for (unsigned int i = 0; i < pScene->mNumMaterials;++i)
		{
			if (!pScene->mMaterials[i])
			{
				this->ReportError("aiScene::mMaterials[%i] is NULL (aiScene::mNumMaterials is %i)",
					i,pScene->mNumMaterials);
			}
			this->Validate(pScene->mMaterials[i]);
		}
	}
	else this->ReportError("aiScene::mNumMaterials is 0. At least one material must be there.");

	// validate the node graph of the scene
	this->Validate(pScene->mRootNode);

	DefaultLogger::get()->debug("ValidateDataStructureProcess end");
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::Validate( const aiMesh* pMesh)
{
	// validate the material index of the mesh
	if (pMesh->mMaterialIndex >= this->mScene->mNumMaterials)
	{
		this->ReportError("aiMesh::mMaterialIndex is invalid (value: %i maximum: %i)",
			pMesh->mMaterialIndex,this->mScene->mNumMaterials-1);
	}

	if (this->mScene->mFlags & AI_SCENE_FLAGS_ANIM_SKELETON_ONLY)
	{
		if (pMesh->mNumVertices || pMesh->mVertices ||
			pMesh->mNumFaces || pMesh->mFaces)
		{
			this->ReportWarning("The mesh contains vertices and faces although "
				"the AI_SCENE_FLAGS_ANIM_SKELETON_ONLY flag is set");
		}
	}
	else
	{
		// positions must always be there ...
		if (!pMesh->mNumVertices || !pMesh->mVertices)
		{
			this->ReportError("The mesh contains no vertices");
		}

		// faces, too
		if (!pMesh->mNumFaces || !pMesh->mFaces)
		{
			this->ReportError("The mesh contains no faces");
		}

		// now check whether the face indexing layout is correct:
		// unique vertices, pseudo-indexed.
		std::vector<bool> abRefList;
		abRefList.resize(pMesh->mNumVertices,false);
		for (unsigned int i = 0; i < pMesh->mNumFaces;++i)
		{
			aiFace& face = pMesh->mFaces[i];
			if (!face.mIndices)this->ReportError("aiMesh::mFaces[%i].mIndices is NULL",i);
			if (face.mNumIndices < 3)this->ReportError(
				"aiMesh::mFaces[%i].mIndices is not a triangle or polygon",i);

			for (unsigned int a = 0; a < face.mNumIndices;++a)
			{
				if (face.mIndices[a] >= pMesh->mNumVertices)
				{
					this->ReportError("aiMesh::mFaces[%i]::mIndices[%i] is out of range",i,a);
				}
				// the MSB flag is temporarily used by the extra verbose
				// mode to tell us that the JoinVerticesProcess might have 
				// been executed already.
				if ( !(this->mScene->mFlags & 0x80000000 ) && abRefList[face.mIndices[a]])
				{
					this->ReportError("aiMesh::mVertices[%i] is referenced twice - second "
						"time by aiMesh::mFaces[%i]::mIndices[%i]",face.mIndices[a],i,a);
				}
				abRefList[face.mIndices[a]] = true;
			}
		}
		// check whether there are vertices that aren't referenced by a face
		for (unsigned int i = 0; i < pMesh->mNumVertices;++i)
		{
			if (!abRefList[i])this->ReportError("aiMesh::mVertices[%i] is not referenced",i);
		}
		abRefList.clear();

		// texture channel 2 may not be set if channel 1 is zero ...
		{
			unsigned int i = 0;
			for (;i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
			{
				if (!pMesh->HasTextureCoords(i))break;
			}
			for (;i < AI_MAX_NUMBER_OF_TEXTURECOORDS;++i)
				if (pMesh->HasTextureCoords(i))
				{
					this->ReportError("Texture coordinate channel %i is existing, "
						"although the previous channel was NULL.",i);
				}
		}
		// the same for the vertex colors
		{
			unsigned int i = 0;
			for (;i < AI_MAX_NUMBER_OF_COLOR_SETS;++i)
			{
				if (!pMesh->HasVertexColors(i))break;
			}
			for (;i < AI_MAX_NUMBER_OF_COLOR_SETS;++i)
				if (pMesh->HasVertexColors(i))
				{
					this->ReportError("Vertex color channel %i is existing, "
						"although the previous channel was NULL.",i);
				}
		}
	}

	// now validate all bones
	if (pMesh->HasBones())
	{
		if (!pMesh->mBones)
		{
			this->ReportError("aiMesh::mBones is NULL (aiMesh::mNumBones is %i)",
				pMesh->mNumBones);
		}
		float* afSum = NULL;
		if (pMesh->mNumVertices)
		{
			afSum = new float[pMesh->mNumVertices];
			for (unsigned int i = 0; i < pMesh->mNumVertices;++i)
				afSum[i] = 0.0f;
		}

		// check whether there are duplicate bone names
		for (unsigned int i = 0; i < pMesh->mNumBones;++i)
		{
			if (!pMesh->mBones[i])
			{
				delete[] afSum;
				this->ReportError("aiMesh::mBones[%i] is NULL (aiMesh::mNumBones is %i)",
					i,pMesh->mNumBones);
			}
			this->Validate(pMesh,pMesh->mBones[i],afSum);

			for (unsigned int a = i+1; a < pMesh->mNumBones;++a)
			{
				if (pMesh->mBones[i]->mName == pMesh->mBones[a]->mName)
				{
					delete[] afSum;
					this->ReportError("aiMesh::mBones[%i] has the same name as "
						"aiMesh::mBones[%i]",i,a);
				}
			}
		}
		// check whether all bone weights for a vertex sum to 1.0 ...
		for (unsigned int i = 0; i < pMesh->mNumVertices;++i)
		{
			if (afSum[i] && (afSum[i] <= 0.995 || afSum[i] >= 1.005))
			{
				this->ReportWarning("aiMesh::mVertices[%i]: bone weight sum != 1.0 (sum is %f)",i,afSum[i]);
			}
		}
		delete[] afSum;
	}
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::Validate( const aiMesh* pMesh,
	const aiBone* pBone,float* afSum)
{
	this->Validate(&pBone->mName);

   	if (!pBone->mNumWeights)
	{
		this->ReportError("aiBone::mNumWeights is zero");
	}

	// check whether all vertices affected by this bone are valid
	for (unsigned int i = 0; i < pBone->mNumWeights;++i)
	{
		if (pBone->mWeights[i].mVertexId > pMesh->mNumVertices)
		{
			this->ReportError("aiBone::mWeights[%i].mVertexId is out of range",i);
		}
		else if (!pBone->mWeights[i].mWeight || pBone->mWeights[i].mWeight > 1.0f)
		{
			this->ReportWarning("aiBone::mWeights[%i].mWeight has an invalid value",i);
		}
		afSum[pBone->mWeights[i].mVertexId] += pBone->mWeights[i].mWeight;
	}
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::Validate( const aiAnimation* pAnimation)
{
	this->Validate(&pAnimation->mName);

	// validate all materials
	if (pAnimation->mNumBones)
	{
		if (!pAnimation->mBones)
		{
			this->ReportError("aiAnimation::mBones is NULL (aiAnimation::mNumBones is %i)",
				pAnimation->mBones);
		}
		for (unsigned int i = 0; i < pAnimation->mNumBones;++i)
		{
			if (!pAnimation->mBones[i])
			{
				this->ReportError("aiAnimation::mBones[%i] is NULL (aiAnimation::mNumBones is %i)",
					i,pAnimation->mNumBones);
			}
			this->Validate(pAnimation, pAnimation->mBones[i]);
		}
	}
	else this->ReportError("aiAnimation::mNumBones is 0. At least one bone animation channel must be there.");

	// Animation duration is allowed to be zero in cases where the anim contains only a single key frame.
	// if (!pAnimation->mDuration)this->ReportError("aiAnimation::mDuration is zero");
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::SearchForInvalidTextures(const aiMaterial* pMaterial,
	const char* szType)
{
	ai_assert(NULL != szType);

	// search all keys of the material ...
	// textures must be specified with rising indices (e.g. diffuse #2 may not be
	// specified if diffuse #1 is not there ...)

	// "$tex.file.<szType>[<index>]"
	char szBaseBuf[512];
	int iLen;
#if _MSC_VER >= 1400
	iLen = ::sprintf_s(szBaseBuf,"$tex.file.%s",szType);
#else
	iLen = ::sprintf(szBaseBuf,"$tex.file.%s",szType);
#endif
	if (0 >= iLen)return;

	int iNumIndices = 0;
	int iIndex = -1;
	for (unsigned int i = 0; i < pMaterial->mNumProperties;++i)
	{
		aiMaterialProperty* prop = pMaterial->mProperties[i];
		if (0 == ASSIMP_strincmp( prop->mKey.data, szBaseBuf, iLen ))
		{
			const char* sz = &prop->mKey.data[iLen];
			if (*sz)
			{
				++sz;
				iIndex = std::max(iIndex, (int)strtol10(sz,NULL));
				++iNumIndices;
			}

			if (aiPTI_String != prop->mType)
				this->ReportError("Material property %s is expected to be a string",prop->mKey);
		}
	}
	if (iIndex +1 != iNumIndices)
	{
		this->ReportError("%s #%i is set, but there are only %i %s textures",
			szType,iIndex,iNumIndices,szType);
	}

	// now check whether all UV indices are valid ...
#if _MSC_VER >= 1400
	iLen = ::sprintf_s(szBaseBuf,"$tex.uvw.%s",szType);
#else
	iLen = ::sprintf(szBaseBuf,"$tex.uvw.%s",szType);
#endif
	if (0 >= iLen)return;
	
	for (unsigned int i = 0; i < pMaterial->mNumProperties;++i)
	{
		aiMaterialProperty* prop = pMaterial->mProperties[i];
		if (0 == ASSIMP_strincmp( prop->mKey.data, szBaseBuf, iLen ))
		{
			if (aiPTI_Integer != prop->mType || sizeof(int) > prop->mDataLength)
				this->ReportError("Material property %s is expected to be an integer",prop->mKey);

			const char* sz = &prop->mKey.data[iLen];
			if (*sz)
			{
				++sz;
				iIndex = strtol10(sz,NULL);

				// ignore UV indices for texture channel that are not there ...
				if (iIndex >= iNumIndices)
				{
					// get the value
					iIndex = *((unsigned int*)prop->mData);

					// check whether there is a mesh using this material
					// which has not enough UV channels ...
					for (unsigned int a = 0; a < this->mScene->mNumMeshes;++a)
					{
						aiMesh* mesh = this->mScene->mMeshes[a];
						if(mesh->mMaterialIndex == iIndex)
						{
							int iChannels = 0;
							while (mesh->HasTextureCoords(iChannels++));
							if (iIndex >= iChannels)
							{
								this->ReportError("Invalid UV index: %i (key %s). Mesh %i has only %i UV channels",
									iIndex,prop->mKey,a,iChannels);
							}
						}
					}
				}
			}
		}
	}
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::Validate( const aiMaterial* pMaterial)
{
	// check whether there are material keys that are obviously not legal
	for (unsigned int i = 0; i < pMaterial->mNumProperties;++i)
	{
		const aiMaterialProperty* prop = pMaterial->mProperties[i];
		if (!prop)
		{
			this->ReportError("aiMaterial::mProperties[%i] is NULL (aiMaterial::mNumProperties is %i)",
				i,pMaterial->mNumProperties);
		}
		if (!prop->mDataLength || !prop->mData)
		{
			this->ReportError("aiMaterial::mProperties[%i].mDataLength or "
				"aiMaterial::mProperties[%i].mData is 0",i,i);
		}
		// check all predefined types
		if (aiPTI_String == prop->mType)
		{
			// FIX: strings are now stored in a less expensive way ...
			if (prop->mDataLength < sizeof(size_t) + ((const aiString*)prop->mData)->length + 1)
			{
				this->ReportError("aiMaterial::mProperties[%i].mDataLength is "
					"too small to contain a string (%i, needed: %i)",
					i,prop->mDataLength,sizeof(aiString));
			}
			this->Validate((const aiString*)prop->mData);
		}
		else if (aiPTI_Float == prop->mType)
		{
			if (prop->mDataLength < sizeof(float))
			{
				this->ReportError("aiMaterial::mProperties[%i].mDataLength is "
					"too small to contain a float (%i, needed: %i)",
					i,prop->mDataLength,sizeof(float));
			}
		}
		else if (aiPTI_Integer == prop->mType)
		{
			if (prop->mDataLength < sizeof(int))
			{
				this->ReportError("aiMaterial::mProperties[%i].mDataLength is "
					"too small to contain an integer (%i, needed: %i)",
					i,prop->mDataLength,sizeof(int));
			}
		}
		// TODO: check whether there is a key with an unknown name ...
	}

	// make some more specific tests 
	float fTemp;
	int iShading;
	if (AI_SUCCESS == aiGetMaterialInteger( pMaterial,AI_MATKEY_SHADING_MODEL,&iShading))
	{
		switch ((aiShadingMode)iShading)
		{
		case aiShadingMode_Blinn:
		case aiShadingMode_CookTorrance:
		case aiShadingMode_Phong:

			if (AI_SUCCESS != aiGetMaterialFloat(pMaterial,AI_MATKEY_SHININESS,&fTemp))
			{
				this->ReportWarning("A specular shading model is specified but there is no "
					"AI_MATKEY_SHININESS key");
			}
			if (AI_SUCCESS == aiGetMaterialFloat(pMaterial,AI_MATKEY_SHININESS_STRENGTH,&fTemp) && !fTemp)
			{
				this->ReportWarning("A specular shading model is specified but the value of the "
					"AI_MATKEY_SHININESS_STRENGTH key is 0.0");
			}
			break;
		};
	}

	// check whether there are invalid texture keys
	SearchForInvalidTextures(pMaterial,"diffuse");
	SearchForInvalidTextures(pMaterial,"specular");
	SearchForInvalidTextures(pMaterial,"ambient");
	SearchForInvalidTextures(pMaterial,"emissive");
	SearchForInvalidTextures(pMaterial,"opacity");
	SearchForInvalidTextures(pMaterial,"shininess");
	SearchForInvalidTextures(pMaterial,"normals");
	SearchForInvalidTextures(pMaterial,"height");
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::Validate( const aiTexture* pTexture)
{
	// the data section may NEVER be NULL
	if (!pTexture->pcData)
	{
		this->ReportError("aiTexture::pcData is NULL");
	}
	if (pTexture->mHeight)
	{
		if (!pTexture->mWidth)this->ReportError("aiTexture::mWidth is zero "
			"(aiTexture::mHeight is %i, uncompressed texture)",pTexture->mHeight);
	}
	else 
	{
		if (!pTexture->mWidth)this->ReportError("aiTexture::mWidth is zero (compressed texture)");
		if ('.' == pTexture->achFormatHint[0])
		{
			char szTemp[5];
			szTemp[0] = pTexture->achFormatHint[0];
			szTemp[1] = pTexture->achFormatHint[1];
			szTemp[2] = pTexture->achFormatHint[2];
			szTemp[3] = pTexture->achFormatHint[3];
			szTemp[4] = '\0';

			this->ReportWarning("aiTexture::achFormatHint should contain a file extension "
				"without a leading dot (format hint: %s).",szTemp);
		}
	}

	const char* sz = pTexture->achFormatHint;
 	if (	sz[0] >= 'A' && sz[0] <= 'Z' ||
		sz[1] >= 'A' && sz[1] <= 'Z' ||
		sz[2] >= 'A' && sz[2] <= 'Z' ||
		sz[3] >= 'A' && sz[3] <= 'Z')
	{
		this->ReportError("aiTexture::achFormatHint contains non-lowercase characters");
	}
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::Validate( const aiAnimation* pAnimation,
	 const aiBoneAnim* pBoneAnim)
{
	this->Validate(&pBoneAnim->mBoneName);

#if 0
	// check whether there is a bone with this name ...
	unsigned int i = 0;
	for (; i < this->mScene->mNumMeshes;++i)
	{
		aiMesh* mesh = this->mScene->mMeshes[i];
		for (unsigned int a = 0; a < mesh->mNumBones;++a)
		{
			if (mesh->mBones[a]->mName == pBoneAnim->mBoneName)
				goto __break_out;
		}
	}
__break_out:
	if (i == this->mScene->mNumMeshes)
	{
		this->ReportWarning("aiBoneAnim::mBoneName is \"%s\". However, no bone with this name was found",
			pBoneAnim->mBoneName.data);
	}
	if (!pBoneAnim->mNumPositionKeys && !pBoneAnim->mNumRotationKeys && !pBoneAnim->mNumScalingKeys)
	{
		this->ReportWarning("A bone animation channel has no keys");
	}
#endif
	// otherwise check whether one of the keys exceeds the total duration of the animation
	if (pBoneAnim->mNumPositionKeys)
	{
		if (!pBoneAnim->mPositionKeys)
		{
			this->ReportError("aiBoneAnim::mPositionKeys is NULL (aiBoneAnim::mNumPositionKeys is %i)",
				pBoneAnim->mNumPositionKeys);
		}
		double dLast = -0.1;
		for (unsigned int i = 0; i < pBoneAnim->mNumPositionKeys;++i)
		{
			if (pBoneAnim->mPositionKeys[i].mTime > pAnimation->mDuration)
			{
				this->ReportError("aiBoneAnim::mPositionKeys[%i].mTime (%.5f) is larger "
					"than aiAnimation::mDuration (which is %.5f)",i,
					(float)pBoneAnim->mPositionKeys[i].mTime,
					(float)pAnimation->mDuration);
			}
			if (pBoneAnim->mPositionKeys[i].mTime <= dLast)
			{
				this->ReportError("aiBoneAnim::mPositionKeys[%i].mTime (%.5f) is larger "
					"than aiAnimation::mPositionKeys[%i] (which is %.5f)",i,
					(float)pBoneAnim->mPositionKeys[i].mTime,
					i, (float)dLast);
			}
			dLast = pBoneAnim->mPositionKeys[i].mTime;
		}
	}
	// rotation keys
	if (pBoneAnim->mNumRotationKeys)
	{
		if (!pBoneAnim->mRotationKeys)
		{
			this->ReportError("aiBoneAnim::mRotationKeys is NULL (aiBoneAnim::mNumRotationKeys is %i)",
				pBoneAnim->mNumRotationKeys);
		}
		double dLast = -0.1;
		for (unsigned int i = 0; i < pBoneAnim->mNumRotationKeys;++i)
		{
			if (pBoneAnim->mRotationKeys[i].mTime > pAnimation->mDuration)
			{
				this->ReportError("aiBoneAnim::mRotationKeys[%i].mTime (%.5f) is larger "
					"than aiAnimation::mDuration (which is %.5f)",i,
					(float)pBoneAnim->mRotationKeys[i].mTime,
					(float)pAnimation->mDuration);
			}
			if (pBoneAnim->mRotationKeys[i].mTime <= dLast)
			{
				this->ReportError("aiBoneAnim::mRotationKeys[%i].mTime (%.5f) is larger "
					"than aiAnimation::mRotationKeys[%i] (which is %.5f)",i,
					(float)pBoneAnim->mRotationKeys[i].mTime,
					i, (float)dLast);
			}
			dLast = pBoneAnim->mRotationKeys[i].mTime;
		}
	}
	// scaling keys
	if (pBoneAnim->mNumScalingKeys)
	{
		if (!pBoneAnim->mScalingKeys)
		{
			this->ReportError("aiBoneAnim::mScalingKeys is NULL (aiBoneAnim::mNumScalingKeys is %i)",
				pBoneAnim->mNumScalingKeys);
		}
		double dLast = -0.1;
		for (unsigned int i = 0; i < pBoneAnim->mNumScalingKeys;++i)
		{
			if (pBoneAnim->mScalingKeys[i].mTime > pAnimation->mDuration)
			{
				this->ReportError("aiBoneAnim::mScalingKeys[%i].mTime (%.5f) is larger "
					"than aiAnimation::mDuration (which is %.5f)",i,
					(float)pBoneAnim->mScalingKeys[i].mTime,
					(float)pAnimation->mDuration);
			}
			if (pBoneAnim->mScalingKeys[i].mTime <= dLast)
			{
				this->ReportError("aiBoneAnim::mScalingKeys[%i].mTime (%.5f) is larger "
					"than aiAnimation::mScalingKeys[%i] (which is %.5f)",i,
					(float)pBoneAnim->mScalingKeys[i].mTime,
					i, (float)dLast);
			}
			dLast = pBoneAnim->mScalingKeys[i].mTime;
		}
	}
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::Validate( const aiNode* pNode)
{
	if (!pNode)this->ReportError("A node of the scenegraph is NULL");
	if (pNode != this->mScene->mRootNode && !pNode->mParent)
		this->ReportError("A node has no valid parent (aiNode::mParent is NULL)");

	this->Validate(&pNode->mName);

	// validate all meshes
	if (pNode->mNumMeshes)
	{
		if (!pNode->mMeshes)
		{
			this->ReportError("aiNode::mMeshes is NULL (aiNode::mNumMeshes is %i)",
				pNode->mNumMeshes);
		}
		std::vector<bool> abHadMesh;
		abHadMesh.resize(this->mScene->mNumMeshes,false);
		for (unsigned int i = 0; i < pNode->mNumMeshes;++i)
		{
			if (pNode->mMeshes[i] >= this->mScene->mNumMeshes)
			{
				this->ReportError("aiNode::mMeshes[%i] is out of range (maximum is %i)",
					pNode->mMeshes[i],this->mScene->mNumMeshes-1);
			}
			if (abHadMesh[pNode->mMeshes[i]])
			{
				this->ReportError("aiNode::mMeshes[%i] is already referenced by this node (value: %i)",
					i,pNode->mMeshes[i]);
			}
			abHadMesh[pNode->mMeshes[i]] = true;
		}
	}
	if (pNode->mNumChildren)
	{
		if (!pNode->mChildren)
		{
			this->ReportError("aiNode::mChildren is NULL (aiNode::mNumChildren is %i)",
				pNode->mNumChildren);
		}
		for (unsigned int i = 0; i < pNode->mNumChildren;++i)
		{
			this->Validate(pNode->mChildren[i]);
		}
	}
}
// ------------------------------------------------------------------------------------------------
void ValidateDSProcess::Validate( const aiString* pString)
{
	if (pString->length > MAXLEN)
	{
		this->ReportError("aiString::length is too large (%i, maximum is %i)",
			pString->length,MAXLEN);
	}
	const char* sz = pString->data;
	while (true)
	{
		if ('\0' == *sz)
		{
			if (pString->length != (unsigned int)(sz-pString->data))
				this->ReportError("aiString::data is invalid: the terminal zero is at a wrong offset");
			break;
		}
		else if (sz >= &pString->data[MAXLEN])
			this->ReportError("aiString::data is invalid. There is no terminal character");
		++sz;
	}
}
