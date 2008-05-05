/** @file Implementation of the post processing step tokill mesh normals
*/
#include "KillNormalsProcess.h"
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
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
		this->KillMeshNormals( pScene->mMeshes[a]);
}

// -------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void KillNormalsProcess::KillMeshNormals(aiMesh* pMesh)
{
	delete[] pMesh->mNormals;
	pMesh->mNormals = NULL;
	return;
}