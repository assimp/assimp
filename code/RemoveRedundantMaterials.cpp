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
/** @file Implementation of the "RemoveRedundantMaterials" post processing step 
*/

// internal headers
#include "AssimpPCH.h"
#include "RemoveRedundantMaterials.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
RemoveRedundantMatsProcess::RemoveRedundantMatsProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
RemoveRedundantMatsProcess::~RemoveRedundantMatsProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool RemoveRedundantMatsProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_RemoveRedundantMaterials) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void RemoveRedundantMatsProcess::Execute( aiScene* pScene)
{
	DefaultLogger::get()->debug("RemoveRedundantMatsProcess begin");

	unsigned int iCnt = 0, unreferenced = 0;
	if (pScene->mNumMaterials)
	{
		// TODO: reimplement this algorithm to work in-place

		unsigned int* aiMappingTable = new unsigned int[pScene->mNumMaterials];
		unsigned int iNewNum = 0;

		std::vector<bool> abReferenced(pScene->mNumMaterials,false);
		for (unsigned int i = 0;i < pScene->mNumMeshes;++i)
			abReferenced[pScene->mMeshes[i]->mMaterialIndex] = true;

		// iterate through all materials and calculate a hash for them
		// store all hashes in a list and so a quick search whether
		// we do already have a specific hash. This allows us to
		// determine which materials are identical.
		uint32_t* aiHashes;
		aiHashes = new uint32_t[pScene->mNumMaterials];
		for (unsigned int i = 0; i < pScene->mNumMaterials;++i)
		{
			// if the material is not referenced ... remove it
			if (!abReferenced[i])
			{
				++unreferenced;
				continue;
			}

			uint32_t me = aiHashes[i] = ((MaterialHelper*)pScene->mMaterials[i])->ComputeHash();
			for (unsigned int a = 0; a < i;++a)
			{
				if (me == aiHashes[a])
				{				
					++iCnt;
					me = 0;
					aiMappingTable[i] = aiMappingTable[a];
					delete pScene->mMaterials[i];
					break;
				}
			}
			if (me)
			{
				aiMappingTable[i] = iNewNum++;
			}
		}
		if (iCnt)
		{
			// build an output material list
			aiMaterial** ppcMaterials = new aiMaterial*[iNewNum];
			::memset(ppcMaterials,0,sizeof(void*)*iNewNum); 
			for (unsigned int p = 0; p < pScene->mNumMaterials;++p)
			{
				// if the material is not referenced ... remove it
				if (!abReferenced[p])continue;

				// generate new names for all modified materials
				const unsigned int idx = aiMappingTable[p]; 
				if (ppcMaterials[idx]) 
				{
					aiString sz;
					sz.length = ::sprintf(sz.data,"aiMaterial #%i",p);
					((MaterialHelper*)ppcMaterials[idx])->AddProperty(&sz,AI_MATKEY_NAME);
				}
				else ppcMaterials[idx] = pScene->mMaterials[p];
			}
			// update all material indices
			for (unsigned int p = 0; p < pScene->mNumMeshes;++p)
			{
				aiMesh* mesh = pScene->mMeshes[p];
				mesh->mMaterialIndex = aiMappingTable[mesh->mMaterialIndex];
			}
			// delete the old material list
			delete[] pScene->mMaterials;
			pScene->mMaterials = ppcMaterials;
			pScene->mNumMaterials = iNewNum;
		}
		// delete temporary storage
		delete[] aiHashes;
		delete[] aiMappingTable;
	}
	if (!iCnt)DefaultLogger::get()->debug("RemoveRedundantMatsProcess finished ");
	else 
	{
		char szBuffer[128]; // should be sufficiently large
		::sprintf(szBuffer,"RemoveRedundantMatsProcess finished. %i redundant and %i unused materials",
			iCnt,unreferenced);
		DefaultLogger::get()->info(szBuffer);
	}
}
