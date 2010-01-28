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

/** @file Implementation of the VertexTriangleAdjacency helper class */
#include "AssimpPCH.h"

// internal headers
#include "VertexTriangleAdjacency.h"
using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Compute a vertex-to-faces adjacency table. To save small memory allocations, the information
// is encoded in three continous buffers - the offset table maps a single vertex index to an
// entry in the adjacency table, which in turn contains n entries, which are the faces adjacent
// to the vertex. n is taken from the third buffer, which stores face counts per vertex.
// ------------------------------------------------------------------------------------------------
VertexTriangleAdjacency::VertexTriangleAdjacency(aiFace *pcFaces,
	unsigned int iNumFaces,
	unsigned int iNumVertices /*= 0*/,
	bool bComputeNumTriangles /*= false*/)
{
	// ---------------------------------------------------------------------
	// 0: compute the number of referenced vertices if not 
	// specified by the caller.
	// ---------------------------------------------------------------------
	const aiFace* const pcFaceEnd = pcFaces + iNumFaces;
	if (!iNumVertices)	{

		for (aiFace* pcFace = pcFaces; pcFace != pcFaceEnd; ++pcFace)	{
			for (unsigned int i = 0; i < pcFace->mNumIndices; ++i) {
				iNumVertices = std::max(iNumVertices,pcFace->mIndices[i]);
			}
		}
	}
	unsigned int* pi;

	// ---------------------------------------------------------------------
	// 1. Allocate output storage
	// ---------------------------------------------------------------------
	if (bComputeNumTriangles)	{
		pi = mLiveTriangles = new unsigned int[iNumVertices+1];
		memset(mLiveTriangles,0,sizeof(unsigned int)*(iNumVertices+1));

		mOffsetTable = new unsigned int[iNumVertices+2]+1;
	}
	else {
		pi = mOffsetTable = new unsigned int[iNumVertices+2]+1;
		memset(mOffsetTable,0,sizeof(unsigned int)*(iNumVertices+1));
		 // important, otherwise the d'tor would crash
		mLiveTriangles = NULL;
	}

	unsigned int* piEnd = pi+iNumVertices;
	*piEnd++ = 0u;

	// ---------------------------------------------------------------------
	// 2. Compute the number of faces referencing each vertex
	// ---------------------------------------------------------------------
	for (aiFace* pcFace = pcFaces; pcFace != pcFaceEnd; ++pcFace)	{
		for (unsigned int i = 0; i < pcFace->mNumIndices; ++i) {
			pi[pcFace->mIndices[i]]++;	
		}
	}

	// ---------------------------------------------------------------------
	// 3. Compute the offset table to map each face to a
	// start position in the adjacency table.
	// ---------------------------------------------------------------------
	unsigned int iSum = 0;
	unsigned int* piCurOut = mOffsetTable;
	for (unsigned int* piCur = pi; piCur != piEnd;++piCur,++piCurOut)	{

		unsigned int iLastSum = iSum;
		iSum += *piCur; 
		*piCurOut = iLastSum;
	}
	pi = mOffsetTable;

	// ---------------------------------------------------------------------
	// 4. Compute the final adjacency table
	// ---------------------------------------------------------------------
	mAdjacencyTable = new unsigned int[iSum];
	iSum = 0;
	for (aiFace* pcFace = pcFaces; pcFace != pcFaceEnd; ++pcFace,++iSum)	{
		
		for (unsigned int i = 0; i < pcFace->mNumIndices; ++i) {
			mAdjacencyTable[pi[pcFace->mIndices[i]]++] = iSum;
		}
	}

	// ---------------------------------------------------------------------
	// 5. Undo the offset computations made during step 4
	// ---------------------------------------------------------------------
	--mOffsetTable;
	*mOffsetTable = 0u;
}

// ------------------------------------------------------------------------------------------------
VertexTriangleAdjacency::~VertexTriangleAdjacency()
{
	delete[] mOffsetTable;
	delete[] mAdjacencyTable;
	delete[] mLiveTriangles;
}
