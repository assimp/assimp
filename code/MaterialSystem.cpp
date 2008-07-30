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

#include "MaterialSystem.h"
#include "StringComparison.h"

#include "../include/aiMaterial.h"
#include "../include/aiAssert.h"

using namespace Assimp;


// ------------------------------------------------------------------------------------------------
// hashing function taken from 
// http://www.azillionmonkeys.com/qed/hash.html
// (incremental version of the hashing function)
// (stdint.h should have been been included here)
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

// ------------------------------------------------------------------------------------------------
uint32_t SuperFastHash (const char * data, int len, uint32_t hash = 0) {
uint32_t tmp;
int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
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
uint32_t MaterialHelper::ComputeHash()
{
	uint32_t hash = 1503; // magic start value, choosen to be my birthday :-)
	for (unsigned int i = 0; i < this->mNumProperties;++i)
	{
		aiMaterialProperty* prop;

		// NOTE: We need to exclude the material name from the hash
		if ((prop = this->mProperties[i]) && 0 != ::strcmp(prop->mKey->data,AI_MATKEY_NAME)) 
		{
			hash = SuperFastHash(prop->mKey->data,prop->mKey->length,hash);
			hash = SuperFastHash(prop->mData,prop->mDataLength,hash);
		}
	}
	return hash;
}
// ------------------------------------------------------------------------------------------------
aiReturn MaterialHelper::RemoveProperty (const char* pKey)
{
	ai_assert(NULL != pKey);

	for (unsigned int i = 0; i < this->mNumProperties;++i)
	{
		if (this->mProperties[i]) // just for safety
		{
			if (0 == ASSIMP_stricmp( this->mProperties[i]->mKey->data, pKey ))
			{
				// delete this entry
				delete[] this->mProperties[i]->mData;
				delete this->mProperties[i];
				
				// collapse the array behind --.
				--this->mNumProperties;
				for (unsigned int a = i; a < this->mNumProperties;++a)
				{
					this->mProperties[a] = this->mProperties[a+1];
				}
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

	// first search the list whether there is already an entry
	// with this name.
	unsigned int iOutIndex = 0xFFFFFFFF;
	for (unsigned int i = 0; i < this->mNumProperties;++i)
	{
		if (this->mProperties[i])
		{
			if (0 == ASSIMP_stricmp( this->mProperties[i]->mKey->data, pKey ))
			{
				// delete this entry
				delete[] this->mProperties[i]->mData;
				delete this->mProperties[i];
				iOutIndex = i;
			}
		}
	}

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

	if (0xFFFFFFFF != iOutIndex)
	{
		this->mProperties[iOutIndex] = pcNew;
		return AI_SUCCESS;
	}

	// resize the array ... allocate storage for 5 other properties
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
// we need this dummy because the compiler would otherwise complain about
// empty, but controlled statements ...
void DummyAssertFunction()
{
	ai_assert(false);
}
// ------------------------------------------------------------------------------------------------
aiReturn aiGetMaterialTexture(const aiMaterial* pcMat,
	unsigned int iIndex,
	unsigned int iTexType,
	aiString* szOut,
	unsigned int* piUVIndex,
	float* pfBlendFactor,
	aiTextureOp* peTextureOp,
	aiTextureMapMode* peMapMode)
{
	ai_assert(NULL != pcMat);
	ai_assert(NULL != szOut);

	const char* szPathBase;
	const char* szUVBase;
	const char* szBlendBase;
	const char* szOpBase;
	const char* aszMapModeBase[3];
	switch (iTexType)
	{
	case AI_TEXTYPE_DIFFUSE:
		szPathBase	= AI_MATKEY_TEXTURE_DIFFUSE_;
		szUVBase	= AI_MATKEY_UVWSRC_DIFFUSE_;
		szBlendBase = AI_MATKEY_TEXBLEND_DIFFUSE_;
		szOpBase	= AI_MATKEY_TEXOP_DIFFUSE_;
		aszMapModeBase[0] = AI_MATKEY_MAPPINGMODE_U_DIFFUSE_;
		aszMapModeBase[1] = AI_MATKEY_MAPPINGMODE_V_DIFFUSE_;
		aszMapModeBase[2] = AI_MATKEY_MAPPINGMODE_W_DIFFUSE_;
		break;
	case AI_TEXTYPE_SPECULAR:
		szPathBase	= AI_MATKEY_TEXTURE_SPECULAR_;
		szUVBase	= AI_MATKEY_UVWSRC_SPECULAR_;
		szBlendBase = AI_MATKEY_TEXBLEND_SPECULAR_;
		szOpBase	= AI_MATKEY_TEXOP_SPECULAR_;
		aszMapModeBase[0] = AI_MATKEY_MAPPINGMODE_U_SPECULAR_;
		aszMapModeBase[1] = AI_MATKEY_MAPPINGMODE_V_SPECULAR_;
		aszMapModeBase[2] = AI_MATKEY_MAPPINGMODE_W_SPECULAR_;
		break;
	case AI_TEXTYPE_AMBIENT:
		szPathBase	= AI_MATKEY_TEXTURE_AMBIENT_;
		szUVBase	= AI_MATKEY_UVWSRC_AMBIENT_;
		szBlendBase = AI_MATKEY_TEXBLEND_AMBIENT_;
		szOpBase	= AI_MATKEY_TEXOP_AMBIENT_;
		aszMapModeBase[0] = AI_MATKEY_MAPPINGMODE_U_AMBIENT_;
		aszMapModeBase[1] = AI_MATKEY_MAPPINGMODE_V_AMBIENT_;
		aszMapModeBase[2] = AI_MATKEY_MAPPINGMODE_W_AMBIENT_;
		break;
	case AI_TEXTYPE_EMISSIVE:
		szPathBase	= AI_MATKEY_TEXTURE_EMISSIVE_;
		szUVBase	= AI_MATKEY_UVWSRC_EMISSIVE_;
		szBlendBase = AI_MATKEY_TEXBLEND_EMISSIVE_;
		szOpBase	= AI_MATKEY_TEXOP_EMISSIVE_;
		aszMapModeBase[0] = AI_MATKEY_MAPPINGMODE_U_EMISSIVE_;
		aszMapModeBase[1] = AI_MATKEY_MAPPINGMODE_V_EMISSIVE_;
		aszMapModeBase[2] = AI_MATKEY_MAPPINGMODE_W_EMISSIVE_;
		break;
	case AI_TEXTYPE_HEIGHT:
		szPathBase	= AI_MATKEY_TEXTURE_HEIGHT_;
		szUVBase	= AI_MATKEY_UVWSRC_HEIGHT_;
		szBlendBase = AI_MATKEY_TEXBLEND_HEIGHT_;
		szOpBase	= AI_MATKEY_TEXOP_HEIGHT_;
		aszMapModeBase[0] = AI_MATKEY_MAPPINGMODE_U_HEIGHT_;
		aszMapModeBase[1] = AI_MATKEY_MAPPINGMODE_V_HEIGHT_;
		aszMapModeBase[2] = AI_MATKEY_MAPPINGMODE_W_HEIGHT_;
		break;
	case AI_TEXTYPE_NORMALS:
		szPathBase	= AI_MATKEY_TEXTURE_NORMALS_;
		szUVBase	= AI_MATKEY_UVWSRC_NORMALS_;
		szBlendBase = AI_MATKEY_TEXBLEND_NORMALS_;
		szOpBase	= AI_MATKEY_TEXOP_NORMALS_;
		aszMapModeBase[0] = AI_MATKEY_MAPPINGMODE_U_NORMALS_;
		aszMapModeBase[1] = AI_MATKEY_MAPPINGMODE_V_NORMALS_;
		aszMapModeBase[2] = AI_MATKEY_MAPPINGMODE_W_NORMALS_;
		break;
	case AI_TEXTYPE_SHININESS:
		szPathBase	= AI_MATKEY_TEXTURE_SHININESS_;
		szUVBase	= AI_MATKEY_UVWSRC_SHININESS_;
		szBlendBase = AI_MATKEY_TEXBLEND_SHININESS_;
		szOpBase	= AI_MATKEY_TEXOP_SHININESS_;
		aszMapModeBase[0] = AI_MATKEY_MAPPINGMODE_U_SHININESS_;
		aszMapModeBase[1] = AI_MATKEY_MAPPINGMODE_V_SHININESS_;
		aszMapModeBase[2] = AI_MATKEY_MAPPINGMODE_W_SHININESS_;
		break;
	case AI_TEXTYPE_OPACITY:
		szPathBase	= AI_MATKEY_TEXTURE_OPACITY_;
		szUVBase	= AI_MATKEY_UVWSRC_OPACITY_;
		szBlendBase = AI_MATKEY_TEXBLEND_OPACITY_;
		szOpBase	= AI_MATKEY_TEXOP_OPACITY_;
		aszMapModeBase[0] = AI_MATKEY_MAPPINGMODE_U_OPACITY_;
		aszMapModeBase[1] = AI_MATKEY_MAPPINGMODE_V_OPACITY_;
		aszMapModeBase[2] = AI_MATKEY_MAPPINGMODE_W_OPACITY_;
		break;
	default: return AI_FAILURE;
	};

	char szKey[256];
	if (iIndex > 100)return AI_FAILURE;

	// get the path to the texture
#if _MSC_VER >= 1400
	if(0 >= sprintf_s(szKey,"%s[%i]",szPathBase,iIndex))DummyAssertFunction();
#else
	if(0 >= sprintf(szKey,"%s[%i]",szPathBase,iIndex))DummyAssertFunction();
#endif

	if (AI_SUCCESS != aiGetMaterialString(pcMat,szKey,szOut))
	{
		return AI_FAILURE;
	}
	// get the UV index of the texture
	if (piUVIndex)
	{
		int iUV;
#if _MSC_VER >= 1400
		if(0 >= sprintf_s(szKey,"%s[%i]",szUVBase,iIndex))DummyAssertFunction();
#else
		if(0 >= sprintf(szKey,"%s[%i]",szUVBase,iIndex))DummyAssertFunction();
#endif
		if (AI_SUCCESS != aiGetMaterialInteger(pcMat,szKey,&iUV))
			iUV = 0;

		*piUVIndex = iUV;
	}
	// get the blend factor of the texture
	if (pfBlendFactor)
	{
		float fBlend;
#if _MSC_VER >= 1400
		if(0 >= sprintf_s(szKey,"%s[%i]",szBlendBase,iIndex))DummyAssertFunction();
#else
		if(0 >= sprintf(szKey,"%s[%i]",szBlendBase,iIndex))DummyAssertFunction();
#endif
		if (AI_SUCCESS != aiGetMaterialFloat(pcMat,szKey,&fBlend))
			fBlend = 1.0f;

		*pfBlendFactor = fBlend;
	}

	// get the texture operation of the texture
	if (peTextureOp)
	{
		aiTextureOp op;
#if _MSC_VER >= 1400
		if(0 >= sprintf_s(szKey,"%s[%i]",szOpBase,iIndex))DummyAssertFunction();
#else
		if(0 >= sprintf(szKey,"%s[%i]",szOpBase,iIndex))DummyAssertFunction();
#endif
		if (AI_SUCCESS != aiGetMaterialInteger(pcMat,szKey,(int*)&op))
			op = aiTextureOp_Multiply;

		*peTextureOp = op;
	}

	// get the texture mapping modes for the texture
	if (peMapMode)
	{
		aiTextureMapMode eMode;
		for (unsigned int q = 0; q < 3;++q)
		{
#if _MSC_VER >= 1400
			if(0 >= sprintf_s(szKey,"%s[%i]",aszMapModeBase[q],iIndex))DummyAssertFunction();
#else
			if(0 >= sprintf(szKey,"%s[%i]",aszMapModeBase[q],iIndex))DummyAssertFunction();
#endif
			if (AI_SUCCESS != aiGetMaterialInteger(pcMat,szKey,(int*)&eMode))
			{
				eMode = aiTextureMapMode_Wrap;
			}
			peMapMode[q] = eMode;
		}
	}
	return AI_SUCCESS;
}

