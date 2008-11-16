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

/** @file GenUVCoords step */


#include "AssimpPCH.h"
#include "ComputeUVMappingProcess.h"
#include "ProcessHelper.h"

using namespace Assimp;


// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ComputeUVMappingProcess::ComputeUVMappingProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
ComputeUVMappingProcess::~ComputeUVMappingProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool ComputeUVMappingProcess::IsActive( unsigned int pFlags) const
{
	return	(pFlags & aiProcess_GenUVCoords) != 0;
}

// ------------------------------------------------------------------------------------------------
unsigned int ComputeUVMappingProcess::ComputeSphereMapping(aiMesh* mesh,aiAxis axis)
{
	DefaultLogger::get()->error("Mapping type currently not implemented");
	return 0;
}

// ------------------------------------------------------------------------------------------------
unsigned int ComputeUVMappingProcess::ComputeCylinderMapping(aiMesh* mesh,aiAxis axis)
{
	DefaultLogger::get()->error("Mapping type currently not implemented");
	return 0;
}

// ------------------------------------------------------------------------------------------------
unsigned int ComputeUVMappingProcess::ComputePlaneMapping(aiMesh* mesh,aiAxis axis)
{
	DefaultLogger::get()->error("Mapping type currently not implemented");
	return 0;
}

// ------------------------------------------------------------------------------------------------
unsigned int ComputeUVMappingProcess::ComputeBoxMapping(aiMesh* mesh)
{
	DefaultLogger::get()->error("Mapping type currently not implemented");
	return 0;
}

// ------------------------------------------------------------------------------------------------
void ComputeUVMappingProcess::Execute( aiScene* pScene) 
{
	DefaultLogger::get()->debug("GenUVCoordsProcess begin");
	char buffer[1024];

	/*  Iterate through all materials and search for non-UV mapped textures
	 */
	for (unsigned int i = 0; i < pScene->mNumMaterials;++i)
	{
		aiMaterial* mat = pScene->mMaterials[i];
		for (unsigned int a = 0; a < mat->mNumProperties;++a)
		{
			aiMaterialProperty* prop = mat->mProperties[a];
			if (!::strcmp( prop->mKey.data, "$tex.mapping"))
			{
				aiTextureMapping mapping = *((aiTextureMapping*)prop->mData);
				if (aiTextureMapping_UV != mapping)
				{
					if (!DefaultLogger::isNullLogger())
					{
						sprintf(buffer, "Found non-UV mapped texture (%s,%i). Mapping type: %s",
							TextureTypeToString((aiTextureType)prop->mSemantic),prop->mIndex,
							MappingTypeToString(mapping));

						DefaultLogger::get()->info(buffer);
					}

					aiAxis axis;

					// Get further properties - currently only the major axis
					for (unsigned int a2 = 0; a2 < mat->mNumProperties;++a2)
					{
						aiMaterialProperty* prop2 = mat->mProperties[a2];
						if (prop2->mSemantic != prop->mSemantic || prop2->mIndex != prop->mIndex)
							continue;

						if ( !::strcmp( prop2->mKey.data, "$tex.mapaxis"))
						{
							axis = *((aiAxis*)prop2->mData);
							break;
						}
					}

					/*   We have found a non-UV mapped texture. Now
					 *   we need to find all meshes using this material
					 *   that we can compute UV channels for them.
					 */
					for (unsigned int m = 0; m < pScene->mNumMeshes;++m)
					{
						aiMesh* mesh = pScene->mMeshes[m];
						if (mesh->mMaterialIndex != i) continue;

						switch (mapping)
						{
						case aiTextureMapping_SPHERE:
							ComputeSphereMapping(mesh,axis);
							break;
						case aiTextureMapping_CYLINDER:
							ComputeCylinderMapping(mesh,axis);
							break;
						case aiTextureMapping_PLANE:
							ComputePlaneMapping(mesh,axis);
							break;
						case aiTextureMapping_BOX:
							ComputeBoxMapping(mesh);
							break;
						}
					}
				}
			}
		}
	}

	DefaultLogger::get()->debug("GenUVCoordsProcess finished");
}
