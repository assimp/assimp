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


#include "stdafx.h"
#include "assimp_view.h"

#include "GenFaceNormalsProcess.h"
#include "GenVertexNormalsProcess.h"
#include "JoinVerticesProcess.h"
#include "CalcTangentsProcess.h"
#include "MakeVerboseFormat.h"

namespace AssimpView {


// NOTE: These classes are necessary since the c'tors of all
// postprocess steps are protected. Generally they're not 
// intended to be used directly by applications.
//
// However, the viewer is an exception. It does nothing than
// displaxing the aiScene, so it wouldn't make sense to copy
// the data to another data structure.
//
class MyGenFaceNormalsProcess : public Assimp::GenFaceNormalsProcess
{
public:
	MyGenFaceNormalsProcess() {}
};

class MyGenVertexNormalsProcess : public Assimp::GenVertexNormalsProcess
{
public:
	MyGenVertexNormalsProcess() {}
};

class MyMakeVerboseFormatProcess : public Assimp::MakeVerboseFormatProcess
{
public:
	MyMakeVerboseFormatProcess() {}
};

class MyCalcTangentsProcess : public Assimp::CalcTangentsProcess
{
public:
	MyCalcTangentsProcess() {}
};

class MyJoinVerticesProcess : public Assimp::JoinVerticesProcess
{
public:
	MyJoinVerticesProcess() {}
};


bool g_bWasFlipped = false;
float g_smoothAngle = 80.f;

//-------------------------------------------------------------------------------
// Flip all normal vectors
//-------------------------------------------------------------------------------
void AssetHelper::FlipNormalsInt()
{
	// invert all normal vectors
	for (unsigned int i = 0; i < this->pcScene->mNumMeshes;++i)
	{
		aiMesh* pcMesh = this->pcScene->mMeshes[i];
	
		if (!pcMesh->mNormals)
			continue;

		for (unsigned int a = 0; a < pcMesh->mNumVertices;++a){
			pcMesh->mNormals[a] *= -1.0f;
		}
	}
}

//-------------------------------------------------------------------------------
void AssetHelper::FlipNormals()
{
	FlipNormalsInt();

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
	// we need to build an unique set of vertices for this ...
	{
		MyMakeVerboseFormatProcess* pcProcess = new MyMakeVerboseFormatProcess();
		pcProcess->Execute(pcScene);
		delete pcProcess;

		for (unsigned int i = 0; i < pcScene->mNumMeshes;++i)
		{
			if (!apcMeshes[i]->pvOriginalNormals)
			{
				apcMeshes[i]->pvOriginalNormals = new aiVector3D[pcScene->mMeshes[i]->mNumVertices];
				memcpy( apcMeshes[i]->pvOriginalNormals,pcScene->mMeshes[i]->mNormals,
					pcScene->mMeshes[i]->mNumVertices * sizeof(aiVector3D));
			}
			delete[] pcScene->mMeshes[i]->mNormals;
			pcScene->mMeshes[i]->mNormals = NULL;
		}
	}


	// now we can start to calculate a new set of normals
	if (HARD == iSet)
	{
		MyGenFaceNormalsProcess* pcProcess = new MyGenFaceNormalsProcess();
		pcProcess->Execute(pcScene);
		FlipNormalsInt();
		delete pcProcess;
	}
	else if (SMOOTH == iSet)
	{
		MyGenVertexNormalsProcess* pcProcess = new MyGenVertexNormalsProcess();
		pcProcess->SetMaxSmoothAngle((float)AI_DEG_TO_RAD(g_smoothAngle));
		pcProcess->Execute(pcScene);
		FlipNormalsInt();
		delete pcProcess;
	}
	else if (ORIGINAL == iSet)
	{
		for (unsigned int i = 0; i < pcScene->mNumMeshes;++i)
		{
			if (apcMeshes[i]->pvOriginalNormals)
			{
				delete[] pcScene->mMeshes[i]->mNormals;
				pcScene->mMeshes[i]->mNormals = apcMeshes[i]->pvOriginalNormals;
				apcMeshes[i]->pvOriginalNormals = NULL;
			}
		}
	}

	// recalculate tangents and bitangents
	Assimp::BaseProcess* pcProcess = new MyCalcTangentsProcess();
	pcProcess->Execute(pcScene);
	delete pcProcess;

	// join the mesh vertices again
	pcProcess = new MyJoinVerticesProcess();
	pcProcess->Execute(pcScene);
	delete pcProcess;

	iNormalSet = iSet;

	if (g_bWasFlipped)
	{
		// invert all normal vectors
		for (unsigned int i = 0; i < pcScene->mNumMeshes;++i)
		{
			aiMesh* pcMesh = pcScene->mMeshes[i];
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