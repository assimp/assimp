/** @file Implementation of the post processing step to split up
 * all faces with more than three indices into triangles.
 */
#include <vector>
#include <assert.h>
#include "TriangulateProcess.h"
#include "../include/aiPostProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
TriangulateProcess::TriangulateProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
TriangulateProcess::~TriangulateProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool TriangulateProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_Triangulate) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void TriangulateProcess::Execute( aiScene* pScene)
{
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
		TriangulateMesh( pScene->mMeshes[a]);
}

// ------------------------------------------------------------------------------------------------
// Triangulates the given mesh.
void TriangulateProcess::TriangulateMesh( aiMesh* pMesh)
{
	std::vector<aiFace> newFaces;
	newFaces.reserve( pMesh->mNumFaces*2);
	for( unsigned int a = 0; a < pMesh->mNumFaces; a++)
	{
		const aiFace& face = pMesh->mFaces[a];

		// if it's a simple primitive, just copy it
		if( face.mNumIndices == 3)
		{
			newFaces.push_back( aiFace());
			aiFace& nface = newFaces.back();
			nface.mNumIndices = face.mNumIndices;
			nface.mIndices = new unsigned int[nface.mNumIndices];
			memcpy( nface.mIndices, face.mIndices, nface.mNumIndices * sizeof( unsigned int));
		} else
		{
			assert( face.mNumIndices > 3);
			for( unsigned int b = 0; b < face.mNumIndices - 2; b++)
			{
				newFaces.push_back( aiFace());
				aiFace& nface = newFaces.back();
				nface.mNumIndices = 3;
				nface.mIndices = new unsigned int[3];
				nface.mIndices[0] = face.mIndices[0];
				nface.mIndices[1] = face.mIndices[b+1];
				nface.mIndices[2] = face.mIndices[b+2];
			}
		}
	}

	// kill the old faces
	delete [] pMesh->mFaces;
	// and insert our newly generated faces
	pMesh->mNumFaces = newFaces.size();
	pMesh->mFaces = new aiFace[pMesh->mNumFaces];
	for( unsigned int a = 0; a < newFaces.size(); a++)
		pMesh->mFaces[a] = newFaces[a];
}
