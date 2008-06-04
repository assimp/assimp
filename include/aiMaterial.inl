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

/** @file Defines the material system of the library
 *
 */

#ifndef AI_MATERIAL_INL_INC
#define AI_MATERIAL_INL_INC


// ---------------------------------------------------------------------------
/** @brief A class that provides easy access to the property list of a
 *  material (aiMaterial) via template methods. You can cast an
 *  aiMaterial* to aiMaterialCPP*
 *  @note This extra class is necessary since template methods
 *  are not allowed within C-linkage blocks (extern "C")
 */
class aiMaterialCPP : public aiMaterial
{
public:

	// -------------------------------------------------------------------
    /** Retrieve an array of Type values with a specific key 
     *  from the material
     *
     * @param pKey Key to search for. One of the AI_MATKEY_XXX constants.
     * @param pOut Pointer to a buffer to receive the result. 
     * @param pMax Specifies the size of the given buffer, in Type's.
     * Receives the number of values (not bytes!) read. 
     * NULL is a valid value for this parameter.
     */
    template <typename Type>
    inline aiReturn Get(const char* pKey,Type* pOut,
        unsigned int* pMax);

    // -------------------------------------------------------------------
    /** Retrieve a Type value with a specific key 
     *  from the material
     *
     * @param pKey Key to search for. One of the AI_MATKEY_XXX constants.
     * @param pOut Reference to receive the output value
     */
    template <typename Type>
	inline aiReturn Get(const char* pKey,Type& pOut);
};

// ---------------------------------------------------------------------------
template <typename Type>
inline aiReturn aiMaterialCPP::Get(const char* pKey,Type* pOut,
	unsigned int* pMax)
{
	unsigned int iNum = pMax ? *pMax : 1;

	aiMaterialProperty* prop;
	aiReturn ret = aiGetMaterialProperty(this,pKey,&prop);
	if ( AI_SUCCESS == ret )
	{
		if (prop->mDataLength < sizeof(Type)*iNum)return AI_FAILURE;
		if (strcmp(prop->mData,(char*)aiPTI_Buffer)!=0)return AI_FAILURE;

		iNum = std::min(iNum,prop->mDataLength / sizeof(Type));
		::memcpy(pOut,prop->mData,iNum * sizeof(Type));
		if (pMax)*pMax = iNum;
	}
	return ret;
}
// ---------------------------------------------------------------------------
template <typename Type>
inline aiReturn aiMaterialCPP::Get(const char* pKey,Type& pOut)
{
	aiMaterialProperty* prop;
	aiReturn ret = aiGetMaterialProperty(this,pKey,&prop);
	if ( AI_SUCCESS == ret )
	{
		if (prop->mDataLength < sizeof(Type))return AI_FAILURE;
		if (strcmp(prop->mData,(char*)aiPTI_Buffer)!=0)return AI_FAILURE;

		::memcpy(&pOut,prop->mData,sizeof(Type));
	}
	return ret;
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterialCPP::Get<float>(const char* pKey,float* pOut,
	unsigned int* pMax)
{
	return aiGetMaterialFloatArray(this,pKey,pOut,pMax);
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterialCPP::Get<int>(const char* pKey,int* pOut,
	unsigned int* pMax)
{
	return aiGetMaterialIntegerArray(this,pKey,pOut,pMax);
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterialCPP::Get<float>(const char* pKey,float& pOut)
{
	return aiGetMaterialFloat(this,pKey,&pOut);
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterialCPP::Get<int>(const char* pKey,int& pOut)
{
	return aiGetMaterialInteger(this,pKey,&pOut);
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterialCPP::Get<aiColor4D>(const char* pKey,aiColor4D& pOut)
{
	return aiGetMaterialColor(this,pKey,&pOut);
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterialCPP::Get<aiString>(const char* pKey,aiString& pOut)
{
	return aiGetMaterialString(this,pKey,&pOut);
}

#endif //! AI_MATERIAL_INL_INC