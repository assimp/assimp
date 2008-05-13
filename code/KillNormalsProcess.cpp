/*
---------------------------------------------------------------------------
Free Asset Import Library (ASSIMP)
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
/** @file Implementation of the post processing step tokill mesh normals
*/
#include "KillNormalsProcess.h"
#include "DefaultLogger.h"
#include "../include/aiPostProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"

using namespace Assimp;

// Constructor to be privately used by Importer
KillNormalsProcess::KillNormalsProcess()
{
}

// Destructor, private as well
KillNormalsProcess::~KillNormalsProcess()
{
	// nothing to do here
}

// -------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool KillNormalsProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_KillNormals) != 0;
}

// -------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void KillNormalsProcess::Execute( aiScene* pScene)
{
	DefaultLogger::get()->debug("KillNormalsProcess begin");

	bool bHas = false;
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
	{
		if(	this->KillMeshNormals( pScene->mMeshes[a]))
			bHas = true;
	}
	if (bHas)DefaultLogger::get()->info("KillNormalsProcess finished. Found normals to kill");
	else DefaultLogger::get()->debug("KillNormalsProcess finished. There was nothing to do.");

}

// -------------------------------------------------------------------
// Executes the post processing step on the given imported data.
bool KillNormalsProcess::KillMeshNormals(aiMesh* pMesh)
{
	if (!pMesh->mNormals)return false;
	delete[] pMesh->mNormals;
	pMesh->mNormals = NULL;
	return true;
}