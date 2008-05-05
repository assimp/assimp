/** @file Implementation of the post processing step to generate face
* normals for all imported faces.
*/
#include "GenFaceNormalsProcess.h"
#include "../include/aiPostProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"

using namespace Assimp;

// Constructor to be privately used by Importer
GenFaceNormalsProcess::GenFaceNormalsProcess()
	{
	}

// Destructor, private as well
GenFaceNormalsProcess::~GenFaceNormalsProcess()
	{
	// nothing to do here
	}

// -------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool GenFaceNormalsProcess::IsActive( unsigned int pFlags) const
{
	return	(pFlags & aiProcess_GenNormals) != 0;
}

// -------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void GenFaceNormalsProcess::Execute( aiScene* pScene)
{
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
		this->GenMeshFaceNormals( pScene->mMeshes[a]);
}

// -------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void GenFaceNormalsProcess::GenMeshFaceNormals (aiMesh* pMesh)
{
	if (NULL != pMesh->mNormals)return;

	pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];

	for( unsigned int a = 0; a < pMesh->mNumFaces; a++)
	{
		const aiFace& face = pMesh->mFaces[a];
		
		// assume it is a triangle
		aiVector3D* pV1 = &pMesh->mVertices[face.mIndices[0]];
		aiVector3D* pV2 = &pMesh->mVertices[face.mIndices[1]];
		aiVector3D* pV3 = &pMesh->mVertices[face.mIndices[2]];

		aiVector3D pDelta1 = *pV2 - *pV1;
		aiVector3D pDelta2 = *pV3 - *pV1;
		aiVector3D vNor = pDelta1 ^ pDelta2;
		
		// NOTE: Never normalize here. Causes problems ...
		//float fLength = vNor.Length();
		//if (0.0f != fLength)vNor /= fLength;

		pMesh->mNormals[face.mIndices[0]] = vNor;
		pMesh->mNormals[face.mIndices[1]] = vNor;
		pMesh->mNormals[face.mIndices[2]] = vNor;
	}
	return;
}