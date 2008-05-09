/*
Free Asset Import Library (ASSIMP)
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

#include "assimp.h"
#include "aiMaterial.h"
#include "assimp.hpp"
#include "MaterialSystem.h"


#include "../include/aiAssert.h"

using namespace Assimp;


// ------------------------------------------------------------------------------------------------
aiReturn aiGetMaterialProperty(const aiMaterial* pMat, 
	const char* pKey,
	const aiMaterialProperty** pPropOut)
{
	ai_assert (pMat != NULL);
	ai_assert (pKey != NULL);
	ai_assert (pPropOut != NULL);

	for (unsigned int i = 0; i < pMat->mNumProperties;++i)
		{
		if (NULL != pMat->mProperties[i])
			{
			if (0 == ASSIMP_stricmp( pMat->mProperties[i]->mKey->data, pKey ))
				{
				*pPropOut = pMat->mProperties[i];
				return AI_SUCCESS;
				}
			}
		}
	*pPropOut = NULL;
	return AI_FAILURE;
}
// ------------------------------------------------------------------------------------------------
aiReturn aiGetMaterialFloatArray(const aiMaterial* pMat, 
	const char* pKey,
	float* pOut,
	unsigned int* pMax)
{
	ai_assert (pMat != NULL);
	ai_assert (pKey != NULL);
	ai_assert (pOut != NULL);

	for (unsigned int i = 0; i < pMat->mNumProperties;++i)
	{
		if (NULL != pMat->mProperties[i])
		{
			if (0 == ASSIMP_stricmp( pMat->mProperties[i]->mKey->data, pKey ))
			{
				// data is given in floats, simply copy it
				if( aiPTI_Float == pMat->mProperties[i]->mType ||
					aiPTI_Buffer == pMat->mProperties[i]->mType)
				{
					unsigned int iWrite = pMat->mProperties[i]->
						mDataLength / sizeof(float);

					if (NULL != pMax)
						iWrite = *pMax < iWrite ? *pMax : iWrite;

					memcpy (pOut, pMat->mProperties[i]->mData, iWrite * sizeof (float));
					
					if (NULL != pMax)
						*pMax = iWrite;
				}
				// data is given in ints, convert to float
				else if( aiPTI_Integer == pMat->mProperties[i]->mType)
				{
					unsigned int iWrite = pMat->mProperties[i]->
						mDataLength / sizeof(int);

					if (NULL != pMax)
						iWrite = *pMax < iWrite ? *pMax : iWrite;

					for (unsigned int a = 0; a < iWrite;++a)
					{
						pOut[a] = (float) ((int*)pMat->mProperties[i]->mData)[a];
					}
					if (NULL != pMax)
						*pMax = iWrite;
				}
				// it is a string ... no way to read something out of this
				else
				{
					if (NULL != pMax)
						*pMax = 0;
					return AI_FAILURE;
				}
				return AI_SUCCESS;
			}
		}
	}
	return AI_FAILURE;
}
// ------------------------------------------------------------------------------------------------
aiReturn aiGetMaterialIntegerArray(const aiMaterial* pMat, 
	const char* pKey,
	int* pOut,
	unsigned int* pMax)
{
	ai_assert (pMat != NULL);
	ai_assert (pKey != NULL);
	ai_assert (pOut != NULL);
	for (unsigned int i = 0; i < pMat->mNumProperties;++i)
	{
		if (NULL != pMat->mProperties[i])
		{
			if (0 == ASSIMP_stricmp( pMat->mProperties[i]->mKey->data, pKey ))
			{
				// data is given in ints, simply copy it
				if( aiPTI_Integer == pMat->mProperties[i]->mType ||
					aiPTI_Buffer == pMat->mProperties[i]->mType)
				{
					unsigned int iWrite = pMat->mProperties[i]->
						mDataLength / sizeof(int);

					if (NULL != pMax)
						iWrite = *pMax < iWrite ? *pMax : iWrite;

					memcpy (pOut, pMat->mProperties[i]->mData, iWrite * sizeof (int));
					
					if (NULL != pMax)
						*pMax = iWrite;
				}
				// data is given in floats convert to int (lossy!)
				else if( aiPTI_Float == pMat->mProperties[i]->mType)
				{
					unsigned int iWrite = pMat->mProperties[i]->
						mDataLength / sizeof(float);

					if (NULL != pMax)
						iWrite = *pMax < iWrite ? *pMax : iWrite;

					for (unsigned int a = 0; a < iWrite;++a)
					{
						pOut[a] = (int) ((float*)pMat->mProperties[i]->mData)[a];
					}
					if (NULL != pMax)
						*pMax = iWrite;
				}
				// it is a string ... no way to read something out of this
				else
				{
					if (NULL != pMax)
						*pMax = 0;
					return AI_FAILURE;
				}
				return AI_SUCCESS;
			}
		}
	}
	return AI_FAILURE;
}
// ------------------------------------------------------------------------------------------------
aiReturn aiGetMaterialColor(const aiMaterial* pMat, 
	const char* pKey,
	aiColor4D* pOut)
{
	unsigned int iMax = 4;
	aiReturn eRet = aiGetMaterialFloatArray(pMat,pKey,(float*)pOut,&iMax);

	// if no alpha channel is provided set it to 1.0 by default
	if (3 == iMax)pOut->a = 1.0f;
	return eRet;
}
// ------------------------------------------------------------------------------------------------
aiReturn aiGetMaterialString(const aiMaterial* pMat, 
	const char* pKey,
	aiString* pOut)
{
	ai_assert (pMat != NULL);
	ai_assert (pKey != NULL);
	ai_assert (pOut != NULL);

	for (unsigned int i = 0; i < pMat->mNumProperties;++i)
	{
		if (NULL != pMat->mProperties[i])
		{
			if (0 == ASSIMP_stricmp( pMat->mProperties[i]->mKey->data, pKey ))
			{
				if( aiPTI_String == pMat->mProperties[i]->mType)
				{
					memcpy (pOut, pMat->mProperties[i]->mData, 
						sizeof(aiString));
				}
				// wrong type
				else return AI_FAILURE;
				return AI_SUCCESS;
			}
		}
	}
	return AI_FAILURE;
}
// ------------------------------------------------------------------------------------------------
aiReturn MaterialHelper::AddBinaryProperty (const void* pInput,
	const unsigned int pSizeInBytes,
	const char* pKey,
	aiPropertyTypeInfo pType)
{
	ai_assert (pInput != NULL);
	ai_assert (pKey != NULL);
	ai_assert (0 != pSizeInBytes);

	aiMaterialProperty* pcNew = new aiMaterialProperty();

	// fill this
	pcNew->mKey = new aiString();
	pcNew->mType = pType;

	pcNew->mDataLength = pSizeInBytes;
	pcNew->mData = new char[pSizeInBytes];
	memcpy (pcNew->mData,pInput,pSizeInBytes);

	pcNew->mKey->length = strlen(pKey);
	ai_assert ( MAXLEN > pcNew->mKey->length);
	strcpy( pcNew->mKey->data, pKey );

	// resize the array ... allocate
	// storage for 5 other properties
	if (this->mNumProperties == this->mNumAllocated)
	{
		unsigned int iOld = this->mNumAllocated;
		this->mNumAllocated += 5;

		aiMaterialProperty** ppTemp = new aiMaterialProperty*[this->mNumAllocated];
		if (NULL == ppTemp)return AI_OUTOFMEMORY;

		memcpy (ppTemp,this->mProperties,iOld * sizeof(void*));

		delete[] this->mProperties;
		this->mProperties = ppTemp;
	}
	// push back ...
	this->mProperties[this->mNumProperties++] = pcNew;
	return AI_SUCCESS;
}
// ------------------------------------------------------------------------------------------------
aiReturn MaterialHelper::AddProperty (const aiString* pInput,
	const char* pKey)
{
	return this->AddBinaryProperty(pInput,
		sizeof(aiString),pKey,aiPTI_String);
}
// ------------------------------------------------------------------------------------------------
void MaterialHelper::CopyPropertyList(MaterialHelper* pcDest, 
	const MaterialHelper* pcSrc)
{
	ai_assert(NULL != pcDest);
	ai_assert(NULL != pcSrc);

	unsigned int iOldNum = pcDest->mNumProperties;
	pcDest->mNumAllocated += pcSrc->mNumAllocated;
	pcDest->mNumProperties += pcSrc->mNumProperties;

	aiMaterialProperty** pcOld = pcDest->mProperties;
	pcDest->mProperties = new aiMaterialProperty*[pcDest->mNumAllocated];

	if (pcOld)
	{
		for (unsigned int i = 0; i < iOldNum;++i)
			pcDest->mProperties[i] = pcOld[i];

		delete[] pcDest->mProperties;
	}
	for (unsigned int i = iOldNum; i< pcDest->mNumProperties;++i)
	{
		pcDest->mProperties[i]->mKey = new aiString(*pcSrc->mProperties[i]->mKey);
		pcDest->mProperties[i]->mDataLength = pcSrc->mProperties[i]->mDataLength;
		pcDest->mProperties[i]->mType = pcSrc->mProperties[i]->mType;
		pcDest->mProperties[i]->mData = new char[pcDest->mProperties[i]->mDataLength];
		memcpy(pcDest->mProperties[i]->mData,pcSrc->mProperties[i]->mData,
			pcDest->mProperties[i]->mDataLength);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
aiReturn aiGetMaterialTexture(const aiMaterial* pcMat,
	unsigned int iIndex,
	unsigned int iTexType,
	aiString* szOut,
	unsigned int* piUVIndex,
	float* pfBlendFactor,
	aiTextureOp* peTextureOp)
{
	ai_assert(NULL != pcMat);
	ai_assert(NULL != szOut);

	const char* szPathBase;
	const char* szUVBase;
	const char* szBlendBase;
	const char* szOpBase;
	switch (iTexType)
	{
	case AI_TEXTYPE_DIFFUSE:
		szPathBase = AI_MATKEY_TEXTURE_DIFFUSE_;
		szUVBase = AI_MATKEY_UVWSRC_DIFFUSE_;
		szBlendBase = AI_MATKEY_TEXBLEND_DIFFUSE_;
		szOpBase = AI_MATKEY_TEXOP_DIFFUSE_;
		break;
	case AI_TEXTYPE_SPECULAR:
		szPathBase = AI_MATKEY_TEXTURE_SPECULAR_;
		szUVBase = AI_MATKEY_UVWSRC_SPECULAR_;
		szBlendBase = AI_MATKEY_TEXBLEND_SPECULAR_;
		szOpBase = AI_MATKEY_TEXOP_SPECULAR_;
		break;
	case AI_TEXTYPE_AMBIENT:
		szPathBase = AI_MATKEY_TEXTURE_AMBIENT_;
		szUVBase = AI_MATKEY_UVWSRC_AMBIENT_;
		szBlendBase = AI_MATKEY_TEXBLEND_AMBIENT_;
		szOpBase = AI_MATKEY_TEXOP_AMBIENT_;
		break;
	case AI_TEXTYPE_EMISSIVE:
		szPathBase = AI_MATKEY_TEXTURE_EMISSIVE_;
		szUVBase = AI_MATKEY_UVWSRC_EMISSIVE_;
		szBlendBase = AI_MATKEY_TEXBLEND_EMISSIVE_;
		szOpBase = AI_MATKEY_TEXOP_EMISSIVE_;
		break;
	case AI_TEXTYPE_HEIGHT:
		szPathBase = AI_MATKEY_TEXTURE_HEIGHT_;
		szUVBase = AI_MATKEY_UVWSRC_HEIGHT_;
		szBlendBase = AI_MATKEY_TEXBLEND_HEIGHT_;
		szOpBase = AI_MATKEY_TEXOP_HEIGHT_;
		break;
	case AI_TEXTYPE_NORMALS:
		szPathBase = AI_MATKEY_TEXTURE_NORMALS_;
		szUVBase = AI_MATKEY_UVWSRC_NORMALS_;
		szBlendBase = AI_MATKEY_TEXBLEND_NORMALS_;
		szOpBase = AI_MATKEY_TEXOP_NORMALS_;
		break;
	case AI_TEXTYPE_SHININESS:
		szPathBase = AI_MATKEY_TEXTURE_SHININESS_;
		szUVBase = AI_MATKEY_UVWSRC_SHININESS_;
		szBlendBase = AI_MATKEY_TEXBLEND_SHININESS_;
		szOpBase = AI_MATKEY_TEXOP_SHININESS_;
		break;
	default: return AI_FAILURE;
	};

	char szKey[256];

	// get the path to the texture
	sprintf(szKey,"%s[%i]",szPathBase,iIndex);
	if (AI_SUCCESS != aiGetMaterialString(pcMat,szKey,szOut))
	{
		return AI_FAILURE;
	}
	// get the UV index of the texture
	if (piUVIndex)
	{
		int iUV;
		sprintf(szKey,"%s[%i]",szUVBase,iIndex);
		if (AI_SUCCESS != aiGetMaterialInteger(pcMat,szKey,&iUV))
			iUV = 0;

		*piUVIndex = iUV;
	}
	// get the blend factor of the texture
	if (pfBlendFactor)
	{
		float fBlend;
		sprintf(szKey,"%s[%i]",szBlendBase,iIndex);
		if (AI_SUCCESS != aiGetMaterialFloat(pcMat,szKey,&fBlend))
			fBlend = 1.0f;

		*pfBlendFactor = fBlend;
	}
	// get the texture operation of the texture
	if (peTextureOp)
	{
		aiTextureOp op;
		sprintf(szKey,"%s[%i]",szOpBase,iIndex);
		if (AI_SUCCESS != aiGetMaterialInteger(pcMat,szKey,(int*)&op))
			op = aiTextureOp_Multiply;

		*peTextureOp = op;
	}
	return AI_SUCCESS;
}

