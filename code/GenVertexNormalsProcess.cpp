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

/** @file Implementation of the post processing step to generate face
* normals for all imported faces.
*/
#include "GenVertexNormalsProcess.h"
#include "SpatialSort.h"
#include "DefaultLogger.h"
#include "../include/aiPostProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"

using namespace Assimp;

// Constructor to be privately used by Importer
GenVertexNormalsProcess::GenVertexNormalsProcess()
{
}

// Destructor, private as well
GenVertexNormalsProcess::~GenVertexNormalsProcess()
{
	// nothing to do here
}

// -------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool GenVertexNormalsProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_GenSmoothNormals) != 0;
}

// -------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void GenVertexNormalsProcess::Execute( aiScene* pScene)
{
	DefaultLogger::get()->debug("GenVertexNormalsProcess begin");

	bool bHas = false;
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
	{
		if(this->GenMeshVertexNormals( pScene->mMeshes[a]))
			bHas = true;
	}

	if (bHas)
	{
		DefaultLogger::get()->info("GenVertexNormalsProcess finished. "
			"Vertex normals have been calculated");
	}
	else DefaultLogger::get()->debug("GenVertexNormalsProcess finished. "
		"There was nothing to do");
}

// -------------------------------------------------------------------
// Executes the post processing step on the given imported data.
bool GenVertexNormalsProcess::GenMeshVertexNormals (aiMesh* pMesh)
{
	if (NULL != pMesh->mNormals)return false;

	pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];
	for( unsigned int a = 0; a < pMesh->mNumFaces; a++)
	{
		const aiFace& face = pMesh->mFaces[a];

		aiVector3D* pV1 = &pMesh->mVertices[face.mIndices[0]];
		aiVector3D* pV2 = &pMesh->mVertices[face.mIndices[1]];
		aiVector3D* pV3 = &pMesh->mVertices[face.mIndices[2]];

		aiVector3D pDelta1 = *pV2 - *pV1;
		aiVector3D pDelta2 = *pV3 - *pV1;
		aiVector3D vNor = pDelta1 ^ pDelta2;

		for (unsigned int i = 0;i < face.mNumIndices;++i)
		{
			pMesh->mNormals[face.mIndices[i]] = vNor;
		}
	}

	// calculate the position bounds so we have a reliable epsilon to 
	// check position differences against 
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

	const float posEpsilon = (maxVec - minVec).Length() * 1e-5f;
	// set up a SpatialSort to quickly find all vertices close to a given position
	SpatialSort vertexFinder( pMesh->mVertices, pMesh->mNumVertices, sizeof( aiVector3D));
	std::vector<unsigned int> verticesFound;

	aiVector3D* pcNew = new aiVector3D[pMesh->mNumVertices];
	for (unsigned int i = 0; i < pMesh->mNumVertices;++i)
	{
		const aiVector3D& posThis = pMesh->mVertices[i];

		// get all vertices that share this one ...
		vertexFinder.FindPositions( posThis, posEpsilon, verticesFound);

		aiVector3D pcNor; 
		for (unsigned int a = 0; a < verticesFound.size(); ++a)
		{
			unsigned int vidx = verticesFound[a];
			pcNor += pMesh->mNormals[vidx];
		}
		pcNor /= (float) verticesFound.size();
		pcNew[i] = pcNor;
	}
	delete pMesh->mNormals;
	pMesh->mNormals = pcNew;

	return true;
}