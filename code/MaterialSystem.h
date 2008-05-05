/** @file Definition of the base class for all importer worker classes. */
#ifndef AI_MATERIALSYSTEM_H_INC
#define AI_MATERIALSYSTEM_H_INC

#include "../include/aiMaterial.h"

namespace Assimp
{

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline int ASSIMP_stricmp(const char *s1, const char *s2)
{
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
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
inline int ASSIMP_strincmp(const char *s1, const char *s2, unsigned int n)
{
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
	    structure  */
	aiReturn AddBinaryProperty (const void* pInput,
		const unsigned int pSizeInBytes,
		const char* pKey,
		aiPropertyTypeInfo pType);


	// -------------------------------------------------------------------
	/** Add a string property with a given key and type info to the 
	    material structure  */
	aiReturn AddProperty (const aiString* pInput,
		const char* pKey);


	// -------------------------------------------------------------------
	/** Add a property with a given key to the material structure  */
	template<class TYPE>
	aiReturn AddProperty (const TYPE* pInput,
		const unsigned int pNumValues,
		const char* pKey);
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