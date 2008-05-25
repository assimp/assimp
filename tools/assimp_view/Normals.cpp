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


#include "stdafx.h"
#include "assimp_view.h"

#include "GenFaceNormalsProcess.h"
#include "GenVertexNormalsProcess.h"
#include "JoinVerticesProcess.h"
#include "CalcTangentsProcess.h"
#include "MakeVerboseFormat.h"

namespace AssimpView {


bool g_bWasFlipped = false;

//-------------------------------------------------------------------------------
// Flip all normal vectors
//-------------------------------------------------------------------------------
void AssetHelper::FlipNormals()
{
	// invert all normal vectors
	for (unsigned int i = 0; i < this->pcScene->mNumMeshes;++i)
	{
		aiMesh* pcMesh = this->pcScene->mMeshes[i];
		for (unsigned int a = 0; a < pcMesh->mNumVertices;++a)
		{
			pcMesh->mNormals[a] *= -1.0f;
		}
	}
	// recreate native data
	DeleteAssetData(true);
	CreateAssetData();
	g_bWasFlipped = ! g_bWasFlipped;
}

//-------------------------------------------------------------------------------
// Set the normal set of the scene
//-------------------------------------------------------------------------------
void AssetHelper::SetNormalSet(unsigned int iSet)
{
	if (this->iNormalSet == iSet)return;

	// we need to build an unique set of vertices for this ...
	{
		Assimp::MakeVerboseFormatProcess* pcProcess = new Assimp::MakeVerboseFormatProcess();
		pcProcess->Execute(this->pcScene);
		delete pcProcess;

		for (unsigned int i = 0; i < this->pcScene->mNumMeshes;++i)
		{
			if (!this->apcMeshes[i]->pvOriginalNormals)
			{
				this->apcMeshes[i]->pvOriginalNormals = new aiVector3D[this->pcScene->mMeshes[i]->mNumVertices];
				memcpy( this->apcMeshes[i]->pvOriginalNormals,this->pcScene->mMeshes[i]->mNormals,
					this->pcScene->mMeshes[i]->mNumVertices * sizeof(aiVector3D));
			}
			delete[] this->pcScene->mMeshes[i]->mNormals;
			this->pcScene->mMeshes[i]->mNormals = NULL;
		}
	}


	// now we can start to calculate a new set of normals
	if (HARD == iSet)
	{
		Assimp::GenFaceNormalsProcess* pcProcess = new Assimp::GenFaceNormalsProcess();
		pcProcess->Execute(this->pcScene);
		delete pcProcess;
	}
	else if (SMOOTH == iSet)
	{
		Assimp::GenVertexNormalsProcess* pcProcess = new Assimp::GenVertexNormalsProcess();
		pcProcess->Execute(this->pcScene);
		delete pcProcess;
	}
	else if (ORIGINAL == iSet)
	{
		for (unsigned int i = 0; i < this->pcScene->mNumMeshes;++i)
		{
			if (this->apcMeshes[i]->pvOriginalNormals)
			{
				delete[] this->pcScene->mMeshes[i]->mNormals;
				this->pcScene->mMeshes[i]->mNormals = this->apcMeshes[i]->pvOriginalNormals;
				this->apcMeshes[i]->pvOriginalNormals = NULL;
			}
		}
	}

	// recalculate tangents and bitangents
	Assimp::BaseProcess* pcProcess = new Assimp::CalcTangentsProcess();
	pcProcess->Execute(this->pcScene);
	delete pcProcess;

	// join the mesh vertices again
	pcProcess = new Assimp::JoinVerticesProcess();
	pcProcess->Execute(this->pcScene);
	delete pcProcess;

	this->iNormalSet = iSet;

	if (g_bWasFlipped)
	{
		// invert all normal vectors
		for (unsigned int i = 0; i < this->pcScene->mNumMeshes;++i)
		{
			aiMesh* pcMesh = this->pcScene->mMeshes[i];
			for (unsigned int a = 0; a < pcMesh->mNumVertices;++a)
			{
				pcMesh->mNormals[a] *= -1.0f;
			}
		}
	}

	// recreate native data
	DeleteAssetData(true);
	CreateAssetData();
	return;
}

};