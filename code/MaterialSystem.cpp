

#include "assimp.h"
#include "aiMaterial.h"
#include "assimp.hpp"
#include "MaterialSystem.h"


#include "../include/aiAssert.h"

using namespace Assimp;


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
aiReturn aiGetMaterialProperty(const aiMaterial* pMat, 
	const char* pKey,
	const aiMaterialProperty** pPropOut)
{
#if (defined DEBUG)

	ai_assert (pMat != NULL);
	ai_assert (pKey != NULL);
	ai_assert (pPropOut != NULL);

#endif // ASSIMP_DEBUG

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


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
aiReturn aiGetMaterialFloatArray(const aiMaterial* pMat, 
	const char* pKey,
	float* pOut,
	unsigned int* pMax)
{
#if (defined DEBUG)

	ai_assert (pMat != NULL);
	ai_assert (pKey != NULL);
	ai_assert (pOut != NULL);

#endif // ASSIMP_DEBUG

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


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
aiReturn aiGetMaterialIntegerArray(const aiMaterial* pMat, 
	const char* pKey,
	int* pOut,
	unsigned int* pMax)
{
#if (defined DEBUG)

	ai_assert (pMat != NULL);
	ai_assert (pKey != NULL);
	ai_assert (pOut != NULL);

#endif // ASSIMP_DEBUG

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


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
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


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
aiReturn aiGetMaterialString(const aiMaterial* pMat, 
	const char* pKey,
	aiString* pOut)
{
#if (defined DEBUG)

	ai_assert (pMat != NULL);
	ai_assert (pKey != NULL);
	ai_assert (pOut != NULL);

#endif // ASSIMP_DEBUG

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


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
aiReturn MaterialHelper::AddBinaryProperty (const void* pInput,
	const unsigned int pSizeInBytes,
	const char* pKey,
	aiPropertyTypeInfo pType)
{
#if (defined DEBUG)

	ai_assert (pInput != NULL);
	ai_assert (pKey != NULL);
	ai_assert (0 != pSizeInBytes);

#endif // ASSIMP_DEBUG

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


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
aiReturn MaterialHelper::AddProperty (const aiString* pInput,
	const char* pKey)
{
	return this->AddBinaryProperty(pInput,
		sizeof(aiString),pKey,aiPTI_String);
}
