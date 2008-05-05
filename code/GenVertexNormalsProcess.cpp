/** @file Implementation of the post processing step to generate face
* normals for all imported faces.
*/
#include "GenVertexNormalsProcess.h"
#include "SpatialSort.h"
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
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
		this->GenMeshVertexNormals( pScene->mMeshes[a]);
}

// -------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void GenVertexNormalsProcess::GenMeshVertexNormals (aiMesh* pMesh)
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
		float fLength = vNor.Length();
		if (0.0f != fLength)vNor /= fLength;

		pMesh->mNormals[face.mIndices[0]] = vNor;
		pMesh->mNormals[face.mIndices[1]] = vNor;
		pMesh->mNormals[face.mIndices[2]] = vNor;
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

	return;
}