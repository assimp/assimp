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

#ifndef AI_PROCESS_HELPER_H_INCLUDED
#define AI_PROCESS_HELPER_H_INCLUDED

#include "../include/aiPostProcess.h"

#include "SpatialSort.h"
#include "BaseProcess.h"

namespace Assimp {

// some aliases to make the whole stuff easier to read
typedef std::pair	< unsigned int,float > PerVertexWeight;
typedef std::vector	< PerVertexWeight    > VertexWeightTable;

// -------------------------------------------------------------------------------
/** Little helper function to calculate the quadratic difference 
 * of two colours. 
 * @param pColor1 First color
 * @param pColor2 second color
 * @return Quadratic color difference
 */
inline float GetColorDifference( const aiColor4D& pColor1, const aiColor4D& pColor2) 
{
	const aiColor4D c (pColor1.r - pColor2.r, pColor1.g - pColor2.g, 
		pColor1.b - pColor2.b, pColor1.a - pColor2.a);

	return c.r*c.r + c.g*c.g + c.b*c.b + c.a*c.a;
}

// -------------------------------------------------------------------------------
// Compute the AABB of a mesh
inline void FindAABB (const aiMesh* mesh, aiVector3D& min, aiVector3D& max)
{
	min = aiVector3D (10e10f,  10e10f, 10e10f);
	max = aiVector3D (-10e10f,-10e10f,-10e10f);
	for (unsigned int i = 0;i < mesh->mNumVertices;++i)
	{
		const aiVector3D& v = mesh->mVertices[i];
		min.x = ::std::min(v.x,min.x);
		min.y = ::std::min(v.y,min.y);
		min.z = ::std::min(v.z,min.z);
		max.x = ::std::max(v.x,max.x);
		max.y = ::std::max(v.y,max.y);
		max.z = ::std::max(v.z,max.z);
	}
}

// -------------------------------------------------------------------------------
// Compute the AABB of a mesh after applying a given transform
inline void FindAABBTransformed (const aiMesh* mesh, aiVector3D& min, aiVector3D& max, 
	const aiMatrix4x4& m)
{
	min = aiVector3D (10e10f,  10e10f, 10e10f);
	max = aiVector3D (-10e10f,-10e10f,-10e10f);
	for (unsigned int i = 0;i < mesh->mNumVertices;++i)
	{
		const aiVector3D v = m * mesh->mVertices[i];
		min.x = ::std::min(v.x,min.x);
		min.y = ::std::min(v.y,min.y);
		min.z = ::std::min(v.z,min.z);
		max.x = ::std::max(v.x,max.x);
		max.y = ::std::max(v.y,max.y);
		max.z = ::std::max(v.z,max.z);
	}
}

// -------------------------------------------------------------------------------
// Helper function to determine the 'real' center of a mesh
inline void FindMeshCenter (aiMesh* mesh, aiVector3D& out, aiVector3D& min, aiVector3D& max)
{
	FindAABB(mesh,min,max);
	out = min + (max-min)*0.5f;
}

// -------------------------------------------------------------------------------
// Helper function to determine the 'real' center of a mesh after applying a given transform
inline void FindMeshCenterTransformed (aiMesh* mesh, aiVector3D& out, aiVector3D& min,
	aiVector3D& max, const aiMatrix4x4& m)
{
	FindAABBTransformed(mesh,min,max,m);
	out = min + (max-min)*0.5f;
}

// -------------------------------------------------------------------------------
// Helper function to determine the 'real' center of a mesh
inline void FindMeshCenter (aiMesh* mesh, aiVector3D& out)
{
	aiVector3D min,max;
	FindMeshCenter(mesh,out,min,max);
}

// -------------------------------------------------------------------------------
// Helper function to determine the 'real' center of a mesh after applying a given transform
inline void FindMeshCenterTransformed (aiMesh* mesh, aiVector3D& out,
	const aiMatrix4x4& m)
{
	aiVector3D min,max;
	FindMeshCenterTransformed(mesh,out,min,max,m);
}

// -------------------------------------------------------------------------------
// Compute a good epsilon value for position comparisons on a mesh
inline float ComputePositionEpsilon(const aiMesh* pMesh)
{
	const float epsilon = 1e-5f;

	// calculate the position bounds so we have a reliable epsilon to check position differences against 
	aiVector3D minVec, maxVec;
	FindAABB(pMesh,minVec,maxVec);
	return (maxVec - minVec).Length() * epsilon;
}

// -------------------------------------------------------------------------------
// Compute an unique value for the vertex format of a mesh
inline unsigned int GetMeshVFormatUnique(aiMesh* pcMesh)
{
	ai_assert(NULL != pcMesh);

	// FIX: the hash may never be 0. Otherwise a comparison against
	// nullptr could be successful
	unsigned int iRet = 1;

	// normals
	if (pcMesh->HasNormals())iRet |= 0x2;
	// tangents and bitangents
	if (pcMesh->HasTangentsAndBitangents())iRet |= 0x4;

	BOOST_STATIC_ASSERT(8 >= AI_MAX_NUMBER_OF_COLOR_SETS);
	BOOST_STATIC_ASSERT(8 >= AI_MAX_NUMBER_OF_TEXTURECOORDS);

	// texture coordinates
	unsigned int p = 0;
	while (pcMesh->HasTextureCoords(p))
	{
		iRet |= (0x100 << p);
		if (3 == pcMesh->mNumUVComponents[p])
			iRet |= (0x10000 << p);

		++p;
	}
	// vertex colors
	p = 0;
	while (pcMesh->HasVertexColors(p))iRet |= (0x1000000 << p++);
	return iRet;
}

// -------------------------------------------------------------------------------
// Compute a per-vertex bone weight table
// please .... delete result with operator delete[] ...
inline VertexWeightTable* ComputeVertexBoneWeightTable(aiMesh* pMesh)
{
	if (!pMesh || !pMesh->mNumVertices || !pMesh->mNumBones)
		return NULL;

	VertexWeightTable* avPerVertexWeights = new VertexWeightTable[pMesh->mNumVertices];
	for (unsigned int i = 0; i < pMesh->mNumBones;++i)
	{
		aiBone* bone = pMesh->mBones[i];
		for (unsigned int a = 0; a < bone->mNumWeights;++a)	{
			const aiVertexWeight& weight = bone->mWeights[a];
			avPerVertexWeights[weight.mVertexId].push_back( 
				std::pair<unsigned int,float>(i,weight.mWeight));
		}
	}
	return avPerVertexWeights;
}


// -------------------------------------------------------------------------------
// Get a string for a given aiTextureType
inline const char* TextureTypeToString(aiTextureType in)
{
	switch (in)
	{
	case aiTextureType_DIFFUSE:
		return "Diffuse";
	case aiTextureType_SPECULAR:
		return "Specular";
	case aiTextureType_AMBIENT:
		return "Ambient";
	case aiTextureType_EMISSIVE:
		return "Emissive";
	case aiTextureType_OPACITY:
		return "Opacity";
	case aiTextureType_NORMALS:
		return "Normals";
	case aiTextureType_HEIGHT:
		return "Height";
	case aiTextureType_SHININESS:
		return "Shininess";
	case aiTextureType_DISPLACEMENT:
		return "Displacement";
	case aiTextureType_LIGHTMAP:
		return "Lightmap";
	case aiTextureType_REFLECTION:
		return "Reflection";
	case aiTextureType_UNKNOWN:
		return "Unknown";
    default:
        return "HUGE ERROR, please leave the room immediately and call the police";        
	}
}

// -------------------------------------------------------------------------------
// Get a string for a given aiTextureMapping
inline const char* MappingTypeToString(aiTextureMapping in)
{
	switch (in)
	{
	case aiTextureMapping_UV:
		return "UV";
	case aiTextureMapping_BOX:
		return "Box";
	case aiTextureMapping_SPHERE:
		return "Sphere";
	case aiTextureMapping_CYLINDER:
		return "Cylinder";
	case aiTextureMapping_PLANE:
		return "Plane";
	case aiTextureMapping_OTHER:
		return "Other";
    default:
        return "HUGE ERROR, please leave the room immediately and call the police";        
	}
}

// -------------------------------------------------------------------------------
// Utility postprocess step to share the spatial sort tree between
// all steps which use it to speedup its computations.
class ComputeSpatialSortProcess : public BaseProcess
{
	bool IsActive( unsigned int pFlags) const
	{
		return NULL != shared && 0 != (pFlags & (aiProcess_CalcTangentSpace | 
			aiProcess_GenNormals | aiProcess_JoinIdenticalVertices));
	}

	void Execute( aiScene* pScene)
	{
		typedef std::pair<SpatialSort, float> _Type;

		std::vector<_Type>* p = new std::vector<_Type>(pScene->mNumMeshes); 
		std::vector<_Type>::iterator it = p->begin();

		for (unsigned int i = 0; i < pScene->mNumMeshes; ++i, ++it)	{
			aiMesh* mesh = pScene->mMeshes[i];
			_Type& blubb = *it;
			blubb.first.Fill(mesh->mVertices,mesh->mNumVertices,sizeof(aiVector3D));
			blubb.second = ComputePositionEpsilon(mesh);
		}

		shared->AddProperty(AI_SPP_SPATIAL_SORT,p);
	}
};

// -------------------------------------------------------------------------------
// ... and the same again to cleanup the whole stuff
class DestroySpatialSortProcess : public BaseProcess
{
	bool IsActive( unsigned int pFlags) const
	{
		return NULL != shared && 0 != (pFlags & (aiProcess_CalcTangentSpace | 
			aiProcess_GenNormals | aiProcess_JoinIdenticalVertices));
	}

	void Execute( aiScene* pScene)
	{
		shared->RemoveProperty(AI_SPP_SPATIAL_SORT);
	}
};

} // ! namespace Assimp
#endif // !! AI_PROCESS_HELPER_H_INCLUDED
