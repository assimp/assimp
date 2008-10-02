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
/** @file Implementation of the post processing step to remove
 *        any parts of the mesh structure from the imported data.
*/
#include "RemoveVCProcess.h"
#include "../include/DefaultLogger.h"
#include "../include/aiPostProcess.h"
#include "../include/aiScene.h"
#include "../include/aiConfig.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
RemoveVCProcess::RemoveVCProcess()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
RemoveVCProcess::~RemoveVCProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool RemoveVCProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_RemVertexComponentXYZ) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void RemoveVCProcess::Execute( aiScene* pScene)
{
	DefaultLogger::get()->debug("RemoveVCProcess begin");

	bool bHas = false;
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
	{
		if(	this->ProcessMesh( pScene->mMeshes[a]))
			bHas = true;
	}
	if (bHas)DefaultLogger::get()->info("RemoveVCProcess finished. The specified vertex components have been removed");
	else DefaultLogger::get()->debug("RemoveVCProcess finished. There was nothing to do ..");

}

// ------------------------------------------------------------------------------------------------
// Setup configuration properties for the step
void RemoveVCProcess::SetupProperties(const Importer* pImp)
{
	configDeleteFlags = pImp->GetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,0x0);
	if (!configDeleteFlags)
	{
		DefaultLogger::get()->warn("RemoveVCProcess: AI_CONFIG_PP_RVC_FLAGS is zero, no streams selected");
	}
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
bool RemoveVCProcess::ProcessMesh(aiMesh* pMesh)
{

	// handle normals
	if (configDeleteFlags & aiVertexComponent_NORMALS && pMesh->mNormals)
	{
		delete[] pMesh->mNormals;
		pMesh->mNormals = NULL;
	}

	// handle tangents and bitangents
	if (configDeleteFlags & aiVertexComponent_TANGENTS_AND_BITANGENTS && pMesh->mTangents)
	{
		delete[] pMesh->mTangents;
		pMesh->mTangents = NULL;

		delete[] pMesh->mBitangents;
		pMesh->mBitangents = NULL;
	}

	// handle texture coordinates
	register bool b = (0 != (configDeleteFlags & aiVertexComponent_TEXCOORDS));
	for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i)
	{
		if (!pMesh->mTextureCoords[i])break;
		if (configDeleteFlags & aiVertexComponent_TEXCOORDSn(i) || b)
		{
			delete pMesh->mTextureCoords[i];
			pMesh->mTextureCoords[i] = NULL;

			if (!b)
			{
				// collapse the rest of the array
				unsigned int a;
				for (a = i+1; a < AI_MAX_NUMBER_OF_TEXTURECOORDS;++a)
				{
					pMesh->mTextureCoords[a-1] = pMesh->mTextureCoords[a];
				}
				pMesh->mTextureCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS-1] = NULL;
			}
		}
	}

	// handle vertex colors
	b = (0 != (configDeleteFlags & aiVertexComponent_COLORS));
	for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i)
	{
		if (!pMesh->mColors[i])break;
		if (configDeleteFlags & aiVertexComponent_COLORSn(i) || b)
		{
			delete pMesh->mColors[i];
			pMesh->mColors[i] = NULL;

			if (!b)
			{
				// collapse the rest of the array
				unsigned int a;
				for (a = i+1; a < AI_MAX_NUMBER_OF_COLOR_SETS;++a)
				{
					pMesh->mColors[a-1] = pMesh->mColors[a];
				}
				pMesh->mColors[AI_MAX_NUMBER_OF_COLOR_SETS-1] = NULL;
			}
		}
	}
	
	return true;
}
