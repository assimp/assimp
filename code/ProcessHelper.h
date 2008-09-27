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

#include "SpatialSort.h"
#include "../include/aiPostProcess.h"

namespace Assimp {

// ------------------------------------------------------------------------------------------------
inline float ComputePositionEpsilon(const aiMesh* pMesh)
{
	const float epsilon = 1e-5f;

	// calculate the position bounds so we have a reliable epsilon to check position differences against 
	aiVector3D minVec( 1e10f, 1e10f, 1e10f), maxVec( -1e10f, -1e10f, -1e10f);
	for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
	{
		minVec.x = std::min( minVec.x, pMesh->mVertices[a].x);
		minVec.y = std::min( minVec.y, pMesh->mVertices[a].y);
		minVec.z = std::min( minVec.z, pMesh->mVertices[a].z);
		maxVec.x = std::max( maxVec.x, pMesh->mVertices[a].x);
		maxVec.y = std::max( maxVec.y, pMesh->mVertices[a].y);
		maxVec.z = std::max( maxVec.z, pMesh->mVertices[a].z);
	}
	return (maxVec - minVec).Length() * epsilon;
}

// ------------------------------------------------------------------------------------------------
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

		for (unsigned int i = 0; i < pScene->mNumMeshes; ++i, ++it)
		{
			aiMesh* mesh = pScene->mMeshes[i];
			_Type& blubb = *it;
			blubb.first.Fill(mesh->mVertices,mesh->mNumVertices,sizeof(aiVector3D));

			blubb.second = ComputePositionEpsilon(mesh);
		}

		shared->AddProperty(AI_SPP_SPATIAL_SORT,p);
	}
};

// ------------------------------------------------------------------------------------------------
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

} // !! Assimp
#endif // !! AI_PROCESS_HELPER_H_INCLUDED
