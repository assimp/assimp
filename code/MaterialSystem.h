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

/** @file Definition of the base class for all importer worker classes. */
#ifndef AI_MATERIALSYSTEM_H_INC
#define AI_MATERIALSYSTEM_H_INC

#include "../include/aiMaterial.h"

namespace Assimp
{

// ---------------------------------------------------------------------------
/** \brief Helper function to do platform independent string comparison.
 *
 *  This is required since stricmp() is not consistently available on
 *  all platforms. Some platforms use the '_' prefix, others don't even
 *  have such a function. Yes, this is called an ISO standard.
 *
 *  \param s1 First input string
 *  \param s2 Second input string
 */
// ---------------------------------------------------------------------------
inline int ASSIMP_stricmp(const char *s1, const char *s2)
{
#if (defined _MSC_VER)

	return _stricmp(s1,s2);

#else
	const char *a1, *a2;
	a1 = s1;
	a2 = s2;

	while (true)
	{
		char c1 = (char)tolower(*a1); 
		char c2 = (char)tolower(*a2);
		if ((0 == c1) && (0 == c2)) return 0;
		if (c1 < c2) return-1;
		if (c1 > c2) return 1;
		++a1; 
		++a2;
	}
#endif
}

// ---------------------------------------------------------------------------
/** \brief Helper function to do platform independent string comparison.
 *
 *  This is required since strincmp() is not consistently available on
 *  all platforms. Some platforms use the '_' prefix, others don't even
 *  have such a function. Yes, this is called an ISO standard.
 *
 *  \param s1 First input string
 *  \param s2 Second input string
 *  \param n Macimum number of characters to compare
 */
// ---------------------------------------------------------------------------
inline int ASSIMP_strincmp(const char *s1, const char *s2, unsigned int n)
{
#if (defined _MSC_VER)

	return _strnicmp(s1,s2,n);

#else
	const char *a1, *a2;
	a1 = s1;
	a2 = s2;

	unsigned int p = 0;

	while (true)
	{
		if (p >= n)return 0;

		char c1 = (char)tolower(*a1); 
		char c2 = (char)tolower(*a2);
		if ((0 == c1) && (0 == c2)) return 0;
		if (c1 < c2) return-1;
		if (c1 > c2) return 1;
		++a1; 
		++a2;
		++p;
	}
#endif
}


// ---------------------------------------------------------------------------
/** Internal material helper class. Can be used to fill an aiMaterial
    structure easily. */
class MaterialHelper : public ::aiMaterial
{
public:

	inline MaterialHelper();
	inline ~MaterialHelper();

	// -------------------------------------------------------------------
	/** Add a property with a given key and type info to the material
	 *  structure 
	 *
	 *  \param pInput Pointer to input data
	 *  \param pSizeInBytes Size of input data
	 *  \param pKey Key/Usage of the property (AI_MATKEY_XXX)
	 *  \param pType Type information hint
     */
	aiReturn AddBinaryProperty (const void* pInput,
		const unsigned int pSizeInBytes,
		const char* pKey,
		aiPropertyTypeInfo pType);


	// -------------------------------------------------------------------
	/** Add a string property with a given key and type info to the 
	 *  material structure 
	 *
	 *  \param pInput Input string
	 *  \param pKey Key/Usage of the property (AI_MATKEY_XXX)
	 */
	aiReturn AddProperty (const aiString* pInput,
		const char* pKey);


	// -------------------------------------------------------------------
	/** Add a property with a given key to the material structure 
	 *  \param pInput Pointer to the input data
	 *  \param pNumValues Number of values in the array
	 *  \param pKey Key/Usage of the property (AI_MATKEY_XXX)
	 */
	template<class TYPE>
	aiReturn AddProperty (const TYPE* pInput,
		const unsigned int pNumValues,
		const char* pKey);


	// -------------------------------------------------------------------
	/** Remove a given key from the list
	 *  The function fails if the key isn't found
	 *
	 *  \param pKey Key/Usage to be deleted
	 */
	aiReturn RemoveProperty (const char* pKey);

	// -------------------------------------------------------------------
	/** Copy the property list of a material
	 *  \param pcDest Destination material
	 *  \param pcSrc Source material
	 */
	static void CopyPropertyList(MaterialHelper* pcDest, 
		const MaterialHelper* pcSrc);
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline MaterialHelper::MaterialHelper()
	{
	// allocate 5 entries by default
	this->mNumProperties = 0;
	this->mNumAllocated = 5;
	this->mProperties = new aiMaterialProperty*[5];
	return;
	}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline MaterialHelper::~MaterialHelper()
	{
	for (unsigned int i = 0; i < this->mNumProperties;++i)
		{
		// be careful ...
		if(NULL != this->mProperties[i])
			{
			delete[] this->mProperties[i]->mKey;
			delete[] this->mProperties[i]->mData;
			delete this->mProperties[i];
			}
		}
	return;
	}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template<class TYPE>
aiReturn MaterialHelper::AddProperty (const TYPE* pInput,
	const unsigned int pNumValues,
	const char* pKey)
{
	return this->AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(TYPE),
		pKey,aiPTI_Buffer);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template<>
inline aiReturn MaterialHelper::AddProperty<float> (const float* pInput,
	const unsigned int pNumValues,
	const char* pKey)
{
	return this->AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(float),
		pKey,aiPTI_Float);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template<>
inline aiReturn MaterialHelper::AddProperty<aiColor4D> (const aiColor4D* pInput,
	const unsigned int pNumValues,
	const char* pKey)
{
	return this->AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(aiColor4D),
		pKey,aiPTI_Float);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template<>
inline aiReturn MaterialHelper::AddProperty<aiColor3D> (const aiColor3D* pInput,
	const unsigned int pNumValues,
	const char* pKey)
{
	return this->AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(aiColor3D),
		pKey,aiPTI_Float);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template<>
inline aiReturn MaterialHelper::AddProperty<int> (const int* pInput,
	const unsigned int pNumValues,
	const char* pKey)
{
	return this->AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(int),
		pKey,aiPTI_Integer);
}
}


#endif //!! AI_MATERIALSYSTEM_H_INC