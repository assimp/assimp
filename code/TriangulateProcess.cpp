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

/** @file Implementation of the post processing step to split up
 * all faces with more than three indices into triangles.
 */

#include "AssimpPCH.h"
#include "TriangulateProcess.h"

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
	DefaultLogger::get()->debug("TriangulateProcess begin");

	bool bHas = false;
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
	{
		if(	TriangulateMesh( pScene->mMeshes[a]))
			bHas = true;
	}
	if (bHas)DefaultLogger::get()->info("TriangulateProcess finished. Found polygons to triangulate");
	else DefaultLogger::get()->debug("TriangulateProcess finished. There was nothing to do.");
}

// ------------------------------------------------------------------------------------------------
// Triangulates the given mesh.
bool TriangulateProcess::TriangulateMesh( aiMesh* pMesh)
{
	// check whether we will need to do something ...
	// FIX: now we have aiMesh::mPrimitiveTypes, so this is only here for test cases
	if (!pMesh->mPrimitiveTypes)
	{
		bool bNeed = false;
		for( unsigned int a = 0; a < pMesh->mNumFaces; a++)
		{
			const aiFace& face = pMesh->mFaces[a];
			if( face.mNumIndices != 3)
			{
				bNeed = true;
			}
		}
		if (!bNeed)return false;
	}
	else if (!(pMesh->mPrimitiveTypes & aiPrimitiveType_POLYGON))
		return false;

	// the output mesh will contain triangles, but no polys anymore
	pMesh->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
	pMesh->mPrimitiveTypes &= ~aiPrimitiveType_POLYGON;

	std::vector<aiFace> newFaces;
	newFaces.reserve( pMesh->mNumFaces*2);
	for( unsigned int a = 0; a < pMesh->mNumFaces; a++)
	{
		aiFace& face = pMesh->mFaces[a];

		// if it's a simple primitive, just copy it
		if( face.mNumIndices <= 3)
		{
			newFaces.push_back( aiFace());
			aiFace& nface = newFaces.back();
			nface.mNumIndices = face.mNumIndices;
			nface.mIndices = face.mIndices;
			face.mIndices = NULL;
		} 
		else
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
	pMesh->mNumFaces = (unsigned int)newFaces.size();
	pMesh->mFaces = new aiFace[pMesh->mNumFaces];
	for( unsigned int a = 0; a < newFaces.size(); a++)
	{
		// operator= would copy the whole array which is definitely not necessary
		pMesh->mFaces[a].mNumIndices = newFaces[a].mNumIndices;
		pMesh->mFaces[a].mIndices    = newFaces[a].mIndices;
		newFaces[a].mIndices = NULL;
	}

	return true;
}
