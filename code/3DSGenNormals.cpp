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

/** @file Implementation of the 3ds importer class */
#include "3DSLoader.h"
#include "MaterialSystem.h"
#include <algorithm>
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "3DSSpatialSort.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::GenNormals(Dot3DS::Mesh* sMesh)
{
	// First generate face normals
	sMesh->mNormals.resize(sMesh->mPositions.size(),aiVector3D());
	for( unsigned int a = 0; a < sMesh->mFaces.size(); a++)
	{
		const Dot3DS::Face& face = sMesh->mFaces[a];

		// assume it is a triangle
		aiVector3D* pV1 = &sMesh->mPositions[face.i1];
		aiVector3D* pV2 = &sMesh->mPositions[face.i2];
		aiVector3D* pV3 = &sMesh->mPositions[face.i3];

		aiVector3D pDelta1 = *pV2 - *pV1;
		aiVector3D pDelta2 = *pV3 - *pV1;
		aiVector3D vNor = pDelta1 ^ pDelta2;

		sMesh->mNormals[face.i1] = vNor;
		sMesh->mNormals[face.i2] = vNor;
		sMesh->mNormals[face.i3] = vNor;
	}

	// calculate the position bounds so we have a reliable epsilon to 
	// check position differences against 
	// @Schrompf: This is the 6th time this snippet is repeated!
	aiVector3D minVec( 1e10f, 1e10f, 1e10f), maxVec( -1e10f, -1e10f, -1e10f);
	for( unsigned int a = 0; a < sMesh->mPositions.size(); a++)
	{
		minVec.x = std::min( minVec.x, sMesh->mPositions[a].x);
		minVec.y = std::min( minVec.y, sMesh->mPositions[a].y);
		minVec.z = std::min( minVec.z, sMesh->mPositions[a].z);
		maxVec.x = std::max( maxVec.x, sMesh->mPositions[a].x);
		maxVec.y = std::max( maxVec.y, sMesh->mPositions[a].y);
		maxVec.z = std::max( maxVec.z, sMesh->mPositions[a].z);
	}
	const float posEpsilon = (maxVec - minVec).Length() * 1e-5f;
	std::vector<aiVector3D> avNormals;
	avNormals.resize(sMesh->mNormals.size());
	
	// now generate the spatial sort tree
	D3DSSpatialSorter sSort;
	for( std::vector<Dot3DS::Face>::iterator
		i =  sMesh->mFaces.begin();
		i != sMesh->mFaces.end();++i)
	{
		sSort.AddFace(&(*i),sMesh->mPositions);
	}
	sSort.Prepare();

	for( std::vector<Dot3DS::Face>::iterator
		i =  sMesh->mFaces.begin();
		i != sMesh->mFaces.end();++i)
	{
		std::vector<unsigned int> poResult;

		for (unsigned int c = 0; c < 3;++c)
		{

			sSort.FindPositions(sMesh->mPositions[(*i).mIndices[c]],(*i).iSmoothGroup,
				posEpsilon,poResult);

			aiVector3D vNormals;
			float fDiv = 0.0f;
			for (std::vector<unsigned int>::const_iterator
				a =  poResult.begin();
				a != poResult.end();++a)
			{
				vNormals += sMesh->mNormals[(*a)];
				fDiv += 1.0f;
			}
			vNormals.x /= fDiv;
			vNormals.y /= fDiv;
			vNormals.z /= fDiv;
			vNormals.Normalize();
			avNormals[(*i).mIndices[c]] = vNormals;
			poResult.clear();
		}
	}
	sMesh->mNormals = avNormals;
	return;
}