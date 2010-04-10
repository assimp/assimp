/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
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

/** @file MaterialSystem.h
 *  Definition of the #MaterialHelper utility class.
 */
#ifndef AI_MATERIALSYSTEM_H_INC
#define AI_MATERIALSYSTEM_H_INC

#include "../include/aiMaterial.h"
namespace Assimp	{

// ----------------------------------------------------------------------------------------
/** Internal material helper class deriving from aiMaterial.
 *
 *  Intended to be used to fill an aiMaterial structure more easily.
 */
class ASSIMP_API MaterialHelper : public ::aiMaterial
{
public:

	// Construction and destruction
	MaterialHelper();
	~MaterialHelper();

	// ------------------------------------------------------------------------------
	/** @brief Add a property with a given key and type info to the material
	 *  structure 
	 *
	 *  @param pInput Pointer to input data
	 *  @param pSizeInBytes Size of input data
	 *  @param pKey Key/Usage of the property (AI_MATKEY_XXX)
	 *  @param type Set by the AI_MATKEY_XXX macro
	 *  @param index Set by the AI_MATKEY_XXX macro
	 *  @param pType Type information hint
     */
	aiReturn AddBinaryProperty (const void* pInput,
		unsigned int pSizeInBytes,
		const char* pKey,
		unsigned int type ,
		unsigned int index ,
		aiPropertyTypeInfo pType);

	// ------------------------------------------------------------------------------
	/** @brief Add a string property with a given key and type info to the 
	 *  material structure 
	 *
	 *  @param pInput Input string
	 *  @param pKey Key/Usage of the property (AI_MATKEY_XXX)
	 *  @param type Set by the AI_MATKEY_XXX macro
	 *  @param index Set by the AI_MATKEY_XXX macro
	 */
	aiReturn AddProperty (const aiString* pInput,
		const char* pKey,
		unsigned int type  = 0,
		unsigned int index = 0);

	// ------------------------------------------------------------------------------
	/** @brief Add a property with a given key to the material structure 
	 *  @param pInput Pointer to the input data
	 *  @param pNumValues Number of values in the array
	 *  @param pKey Key/Usage of the property (AI_MATKEY_XXX)
	 *  @param type Set by the AI_MATKEY_XXX macro
	 *  @param index Set by the AI_MATKEY_XXX macro
	 */
	template<class TYPE>
	aiReturn AddProperty (const TYPE* pInput,
		unsigned int pNumValues,
		const char* pKey,
		unsigned int type  = 0,
		unsigned int index = 0);

	// ------------------------------------------------------------------------------
	/** @brief Remove a given key from the list.
	 *
	 *  The function fails if the key isn't found
	 *  @param pKey Key to be deleted
	 */
	aiReturn RemoveProperty (const char* pKey,
		unsigned int type  = 0,
		unsigned int index = 0);

	// ------------------------------------------------------------------------------
	/** @brief Removes all properties from the material.
	 *
	 *  The data array remains allocated so adding new properties is quite fast.
	 */
	void Clear();

	// ------------------------------------------------------------------------------
	/** Computes a hash (hopefully unique) from all material properties
	 *  The hash value reflects the current property state, so if you add any
	 *  proprty and call this method again, the resulting hash value will be 
	 *  different.
	 *
	 *  @param  includeMatName Set to 'true' to take all properties with
	 *    '?' as initial character in their name into account. 
	 *    Currently #AI_MATKEY_NAME is the only example.
	 *  @return Unique hash
	 */
	uint32_t ComputeHash(bool includeMatName = false);

	// ------------------------------------------------------------------------------
	/** Copy the property list of a material
	 *  @param pcDest Destination material
	 *  @param pcSrc Source material
	 */
	static void CopyPropertyList(MaterialHelper* pcDest, 
		const MaterialHelper* pcSrc);

public:
	// For internal use. That's why it's public.
	void _InternDestruct();
};


// ----------------------------------------------------------------------------------------
template<class TYPE>
aiReturn MaterialHelper::AddProperty (const TYPE* pInput,
	const unsigned int pNumValues,
	const char* pKey,
	unsigned int type,
	unsigned int index)
{
	return AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(TYPE),
		pKey,type,index,aiPTI_Buffer);
}

// ----------------------------------------------------------------------------------------
template<>
inline aiReturn MaterialHelper::AddProperty<float> (const float* pInput,
	const unsigned int pNumValues,
	const char* pKey,
	unsigned int type,
	unsigned int index)
{
	return AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(float),
		pKey,type,index,aiPTI_Float);
}

// ----------------------------------------------------------------------------------------
template<>
inline aiReturn MaterialHelper::AddProperty<aiUVTransform> (const aiUVTransform* pInput,
	const unsigned int pNumValues,
	const char* pKey,
	unsigned int type,
	unsigned int index)
{
	return AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(aiUVTransform),
		pKey,type,index,aiPTI_Float);
}

// ----------------------------------------------------------------------------------------
template<>
inline aiReturn MaterialHelper::AddProperty<aiColor4D> (const aiColor4D* pInput,
	const unsigned int pNumValues,
	const char* pKey,
	unsigned int type,
	unsigned int index)
{
	return AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(aiColor4D),
		pKey,type,index,aiPTI_Float);
}

// ----------------------------------------------------------------------------------------
template<>
inline aiReturn MaterialHelper::AddProperty<aiColor3D> (const aiColor3D* pInput,
	const unsigned int pNumValues,
	const char* pKey,
	unsigned int type,
	unsigned int index)
{
	return AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(aiColor3D),
		pKey,type,index,aiPTI_Float);
}

// ----------------------------------------------------------------------------------------
template<>
inline aiReturn MaterialHelper::AddProperty<aiVector3D> (const aiVector3D* pInput,
	const unsigned int pNumValues,
	const char* pKey,
	unsigned int type,
	unsigned int index)
{
	return AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(aiVector3D),
		pKey,type,index,aiPTI_Float);
}

// ----------------------------------------------------------------------------------------
template<>
inline aiReturn MaterialHelper::AddProperty<int> (const int* pInput,
	const unsigned int pNumValues,
	const char* pKey,
	unsigned int type,
	unsigned int index)
{
	return AddBinaryProperty((const void*)pInput,
		pNumValues * sizeof(int),
		pKey,type,index,aiPTI_Integer);
}
} // ! namespace Assimp

#endif //!! AI_MATERIALSYSTEM_H_INC
