/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

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

/** @file aiMaterial.inl
 *  @brief Defines the C++ getters for the material system
 */

#ifndef AI_MATERIAL_INL_INC
#define AI_MATERIAL_INL_INC

//! @cond never

// ---------------------------------------------------------------------------
inline aiReturn aiMaterial::GetTexture( aiTextureType type,
   unsigned int  index,
   C_STRUCT aiString* path,
   aiTextureMapping* mapping	/*= NULL*/,
   unsigned int* uvindex		/*= NULL*/,
   float* blend				   /*= NULL*/,
   aiTextureOp* op				/*= NULL*/,
   aiTextureMapMode* mapmode	/*= NULL*/) const
{
	return ::aiGetMaterialTexture(this,type,index,path,mapping,uvindex,blend,op,mapmode);
}

// ---------------------------------------------------------------------------
inline unsigned int aiMaterial::GetTextureCount(aiTextureType type) const
{
	return ::aiGetMaterialTextureCount(this,type);
}

// ---------------------------------------------------------------------------
template <typename Type>
inline aiReturn aiMaterial::Get(const char* pKey,unsigned int type,
	unsigned int idx, Type* pOut,
	unsigned int* pMax) const
{
	unsigned int iNum = pMax ? *pMax : 1;

	const aiMaterialProperty* prop;
	const aiReturn ret = ::aiGetMaterialProperty(this,pKey,type,idx,
		(const aiMaterialProperty**)&prop);
	if ( AI_SUCCESS == ret )	{

		if (prop->mDataLength < sizeof(Type)*iNum) {
			return AI_FAILURE;
		}

	//	if (::strcmp(prop->mData,(char*)aiPTI_Buffer)!=0)
	//		return AI_FAILURE;

		iNum = std::min((size_t)iNum,prop->mDataLength / sizeof(Type));
		memcpy(pOut,prop->mData,iNum * sizeof(Type));
		if (pMax) {
			*pMax = iNum;
		}
	}
	return ret;
}

// ---------------------------------------------------------------------------
template <typename Type>
inline aiReturn aiMaterial::Get(const char* pKey,unsigned int type,
	unsigned int idx,Type& pOut) const
{
	const aiMaterialProperty* prop;
	const aiReturn ret = ::aiGetMaterialProperty(this,pKey,type,idx,
		(const aiMaterialProperty**)&prop);
	if ( AI_SUCCESS == ret )	{

		if (prop->mDataLength < sizeof(Type)) {
			return AI_FAILURE;
		}

		if (strcmp(prop->mData,(char*)aiPTI_Buffer)!=0) {
			return AI_FAILURE;
		}

		memcpy(&pOut,prop->mData,sizeof(Type));
	}
	return ret;
}

// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterial::Get<float>(const char* pKey,unsigned int type,
	unsigned int idx,float* pOut,
	unsigned int* pMax) const
{
	return ::aiGetMaterialFloatArray(this,pKey,type,idx,pOut,pMax);
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterial::Get<int>(const char* pKey,unsigned int type,
	unsigned int idx,int* pOut,
	unsigned int* pMax) const
{
	return ::aiGetMaterialIntegerArray(this,pKey,type,idx,pOut,pMax);
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterial::Get<float>(const char* pKey,unsigned int type,
	unsigned int idx,float& pOut) const
{
	return aiGetMaterialFloat(this,pKey,type,idx,&pOut);
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterial::Get<int>(const char* pKey,unsigned int type,
	unsigned int idx,int& pOut) const
{
	return aiGetMaterialInteger(this,pKey,type,idx,&pOut);
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterial::Get<aiColor4D>(const char* pKey,unsigned int type,
	unsigned int idx,aiColor4D& pOut) const
{
	return aiGetMaterialColor(this,pKey,type,idx,&pOut);
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterial::Get<aiColor3D>(const char* pKey,unsigned int type,
	unsigned int idx,aiColor3D& pOut) const
{
	aiColor4D c;
	const aiReturn ret = aiGetMaterialColor(this,pKey,type,idx,&c);
	pOut = aiColor3D(c.r,c.g,c.b);
	return ret;
}
// ---------------------------------------------------------------------------
template <>
inline aiReturn aiMaterial::Get<aiString>(const char* pKey,unsigned int type,
	unsigned int idx,aiString& pOut) const
{
	return aiGetMaterialString(this,pKey,type,idx,&pOut);
}

//! @endcond

#endif //! AI_MATERIAL_INL_INC
