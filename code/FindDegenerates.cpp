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

/** @file Implementation of the DeterminePTypeHelperProcess and
 *  SortByPTypeProcess post-process steps.
*/

#include "AssimpPCH.h"

// internal headers
#include "ProcessHelper.h"
#include "FindDegenerates.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
FindDegeneratesProcess::FindDegeneratesProcess()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
FindDegeneratesProcess::~FindDegeneratesProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool FindDegeneratesProcess::IsActive( unsigned int pFlags) const
{
	return 0 != (pFlags & aiProcess_FindDegenerates);
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void FindDegeneratesProcess::Execute( aiScene* pScene)
{
	DefaultLogger::get()->debug("FindDegeneratesProcess begin");
	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
	{
		aiMesh* mesh = pScene->mMeshes[i];
		mesh->mPrimitiveTypes = 0;

		unsigned int deg = 0;
		for (unsigned int a = 0; a < mesh->mNumFaces; ++a)
		{
			aiFace& face = mesh->mFaces[a];
			bool first = true;

			// check whether the face contains degenerated entries
			for (register unsigned int i = 0; i < face.mNumIndices; ++i)
			{
				for (register unsigned int a = i+1; a < face.mNumIndices; ++a)
				{
					if (mesh->mVertices[face.mIndices[i]] == mesh->mVertices[face.mIndices[a]])
					{
						// we have found a matching vertex position
						// remove the corresponding index from the array
						for (unsigned int m = a; m < face.mNumIndices-1; ++m)
						{
							face.mIndices[m] = face.mIndices[m+1];
						}
						--a;
						--face.mNumIndices;

						// NOTE: we set the removed vertex index to an unique value
						// to make sure the developer gets notified when his
						// application attemps to access this data.
						face.mIndices[face.mNumIndices] = 0xdeadbeef;


						if(first)
						{
							++deg;
							first = false;
						}
					}
				}
			}

			// We need to update the primitive flags array of the mesh.
			// Unfortunately it is not possible to execute
			// FindDegenerates before DeterminePType. The latter does
			// nothing if the primitive flags have already been set by
			// the loader - our changes would be ignored. Although
			// we could use some tricks regarding - i.e setting 
			// mPrimitiveTypes to 0 in every case - but this is the cleanest 
			//  way and causes no additional dependencies in the pipeline.
			switch (face.mNumIndices)
			{
			case 1u:
				mesh->mPrimitiveTypes |= aiPrimitiveType_POINT;
				break;
			case 2u:
				mesh->mPrimitiveTypes |= aiPrimitiveType_LINE;
				break;
			case 3u:
				mesh->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
				break;
			default:
				mesh->mPrimitiveTypes |= aiPrimitiveType_POLYGON;
				break;
			};
		}
		if (deg && !DefaultLogger::isNullLogger())
		{
			char s[64];
			itoa10(s,deg); 
			DefaultLogger::get()->warn(std::string("Found ") + s + " degenerated primitives");
		}
	}
	DefaultLogger::get()->debug("FindDegeneratesProcess finished");
}