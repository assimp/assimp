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


#include "stdafx.h"
#include "assimp_view.h"

#include "GenFaceNormalsProcess.h"
#include "GenVertexNormalsProcess.h"
#include "JoinVerticesProcess.h"
#include "CalcTangentsProcess.h"

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
	for (unsigned int i = 0; i < this->pcScene->mNumMeshes;++i)
	{
		aiMesh* pcMesh = this->pcScene->mMeshes[i];
		const unsigned int iNumVerts = pcMesh->mNumFaces*3;

		aiVector3D* pvPositions = new aiVector3D[iNumVerts];
		aiVector3D* pvNormals = new aiVector3D[iNumVerts];
		aiVector3D* pvTangents(NULL), *pvBitangents(NULL);

		ai_assert(AI_MAX_NUMBER_OF_TEXTURECOORDS == 4);
		ai_assert(AI_MAX_NUMBER_OF_COLOR_SETS == 4);
		aiVector3D* apvTextureCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS] = {NULL,NULL,NULL,NULL};
		aiColor4D* apvColorSets[AI_MAX_NUMBER_OF_COLOR_SETS] = {NULL,NULL,NULL,NULL};


		unsigned int p = 0;
		while (pcMesh->HasTextureCoords(p))
			apvTextureCoords[p++] = new aiVector3D[iNumVerts];

		p = 0;
		while (pcMesh->HasVertexColors(p))
			apvColorSets[p++] = new aiColor4D[iNumVerts];

		// iterate through all faces and build a clean list
		unsigned int iIndex = 0;
		for (unsigned int a = 0; a< pcMesh->mNumFaces;++a)
		{
			aiFace* pcFace = &pcMesh->mFaces[a];
			for (unsigned int q = 0; q < 3;++q,++iIndex)
			{
				pvPositions[iIndex] = pcMesh->mVertices[pcFace->mIndices[q]];
				pvNormals[iIndex] = pcMesh->mNormals[pcFace->mIndices[q]];

				unsigned int p = 0;
				while (pcMesh->HasTextureCoords(p))
				{
					apvTextureCoords[p][iIndex] = pcMesh->mTextureCoords[p][pcFace->mIndices[q]];
					++p;
				}
				p = 0;
				while (pcMesh->HasVertexColors(p))
				{
					apvColorSets[p][iIndex] = pcMesh->mColors[p][pcFace->mIndices[q]];
					++p;
				}
				pcFace->mIndices[q] = iIndex;
			}
		}

		// delete the old members
		delete[] pcMesh->mVertices;
		pcMesh->mVertices = pvPositions;

		p = 0;
		while (pcMesh->HasTextureCoords(p))
		{
			delete pcMesh->mTextureCoords[p];
			pcMesh->mTextureCoords[p] = apvTextureCoords[p];
			++p;
		}
		p = 0;
		while (pcMesh->HasVertexColors(p))
		{
			delete pcMesh->mColors[p];
			pcMesh->mColors[p] = apvColorSets[p];
			++p;
		}
		pcMesh->mNumVertices = iNumVerts;

		// keep the pointer to the normals 
		delete[] pcMesh->mNormals;
		pcMesh->mNormals = NULL;

		if (!this->apcMeshes[i]->pvOriginalNormals)
			this->apcMeshes[i]->pvOriginalNormals = pvNormals;
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
			this->pcScene->mMeshes[i]->mNormals = this->apcMeshes[i]->pvOriginalNormals;
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

	if (g_bWasFlipped && ORIGINAL != iSet)
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