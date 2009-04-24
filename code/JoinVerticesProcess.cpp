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

/** @file Implementation of the post processing step to join identical vertices
 * for all imported meshes
 */

#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_JOINVERTICES_PROCESS

#include "JoinVerticesProcess.h"
#include "ProcessHelper.h"

using namespace Assimp;

// Data structure to keep a vertex in an interlaced format
struct Vertex
{
	aiVector3D mPosition;
	aiVector3D mNormal;
	aiVector3D mTangent, mBitangent;
	aiColor4D  mColors     [AI_MAX_NUMBER_OF_COLOR_SETS];
	aiVector3D mTexCoords  [AI_MAX_NUMBER_OF_TEXTURECOORDS];
};

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
JoinVerticesProcess::JoinVerticesProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
JoinVerticesProcess::~JoinVerticesProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool JoinVerticesProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_JoinIdenticalVertices) != 0;
}
// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void JoinVerticesProcess::Execute( aiScene* pScene)
{
	DefaultLogger::get()->debug("JoinVerticesProcess begin");

	// get the total number of vertices BEFORE the step is executed
	int iNumOldVertices = 0;
	if (!DefaultLogger::isNullLogger()) {
		for( unsigned int a = 0; a < pScene->mNumMeshes; a++)	{
			iNumOldVertices +=	pScene->mMeshes[a]->mNumVertices;
		}
	}

	// execute the step
	int iNumVertices = 0;
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)	{
		iNumVertices +=	ProcessMesh( pScene->mMeshes[a],a);
	}

	// if logging is active, print detailed statistics
	if (!DefaultLogger::isNullLogger())
	{
		if (iNumOldVertices == iNumVertices)DefaultLogger::get()->debug("JoinVerticesProcess finished ");
		else
		{
			char szBuff[128]; // should be sufficiently large in every case
			sprintf(szBuff,"JoinVerticesProcess finished | Verts in: %i out: %i | ~%.1f%%",
				iNumOldVertices,
				iNumVertices,
				((iNumOldVertices - iNumVertices) / (float)iNumOldVertices) * 100.f);
			DefaultLogger::get()->info(szBuff);
		}
	}

	pScene->mFlags |= AI_SCENE_FLAGS_NON_VERBOSE_FORMAT;
}

// ------------------------------------------------------------------------------------------------
// Unites identical vertices in the given mesh
int JoinVerticesProcess::ProcessMesh( aiMesh* pMesh, unsigned int meshIndex)
{
	BOOST_STATIC_ASSERT( AI_MAX_NUMBER_OF_COLOR_SETS    == 4);
	BOOST_STATIC_ASSERT( AI_MAX_NUMBER_OF_TEXTURECOORDS == 4);

	// Return early if we don't have any positions
	if (!pMesh->HasPositions() || !pMesh->HasFaces())
		return 0;

	// We'll never have more vertices afterwards.
	std::vector<Vertex> uniqueVertices;
	uniqueVertices.reserve( pMesh->mNumVertices);

	// For each vertex the index of the vertex it was replaced by. 
	std::vector<unsigned int> replaceIndex( pMesh->mNumVertices, 0xffffffff);
	// for each vertex whether it was replaced by an existing unique vertex (true) or a new vertex was created for it (false)
	std::vector<bool> isVertexUnique( pMesh->mNumVertices, false);

	// A little helper to find locally close vertices faster
	// FIX: check whether we can reuse the SpatialSort of a previous step
	const static float epsilon = 1e-5f;
	float posEpsilonSqr;
	SpatialSort*  vertexFinder = NULL;
	SpatialSort  _vertexFinder;
	if (shared)	{
		std::vector<std::pair<SpatialSort,float> >* avf;
		shared->GetProperty(AI_SPP_SPATIAL_SORT,avf);
		if (avf)	{
			std::pair<SpatialSort,float>& blubb = avf->operator [] (meshIndex);
			vertexFinder  = &blubb.first;posEpsilonSqr = blubb.second;
		}
	}
	if (!vertexFinder)	{
		_vertexFinder.Fill(pMesh->mVertices, pMesh->mNumVertices, sizeof( aiVector3D));
		vertexFinder = &_vertexFinder; 
		posEpsilonSqr = ComputePositionEpsilon(pMesh);
	}

	// squared because we check against squared length of the vector difference
	static const float squareEpsilon = epsilon * epsilon;

	// again, better waste some bytes than a realloc ...
	std::vector<unsigned int> verticesFound;
	verticesFound.reserve(10);

	// run an optimized code path if we don't have multiple UVs or vertex colors
	const bool complex = (
		pMesh->mTextureCoords[1]	||
		pMesh->mTextureCoords[2]	|| 
		pMesh->mTextureCoords[3]	||
		pMesh->mColors[0]			|| 
		pMesh->mColors[1]			|| 
		pMesh->mColors[2]			||
		pMesh->mColors[3] );

	// Now check each vertex if it brings something new to the table
	for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
	{
		// collect the vertex data
		Vertex v;
		v.mPosition = pMesh->mVertices[a];

		if (pMesh->mNormals)
			v.mNormal = pMesh->mNormals[a];
		if (pMesh->mTangents)
			v.mTangent = pMesh->mTangents[a];
		if (pMesh->mBitangents)
			v.mBitangent = pMesh->mBitangents[a];

		if (pMesh->mColors[0]) { // manually unrolled here
			v.mColors[0] = pMesh->mColors[0][a];
			if (pMesh->mColors[1]) {
				v.mColors[1] = pMesh->mColors[1][a];
				if (pMesh->mColors[2]) {
					v.mColors[2] = pMesh->mColors[2][a];
					if (pMesh->mColors[3]) {
						v.mColors[3] = pMesh->mColors[3][a];
		}}}}
		if (pMesh->mTextureCoords[0]) { // manually unrolled here
			v.mTexCoords[0] = pMesh->mTextureCoords[0][a];
			if (pMesh->mTextureCoords[1]) {
				v.mTexCoords[1] = pMesh->mTextureCoords[1][a];
				if (pMesh->mTextureCoords[2]) {
					v.mTexCoords[2] = pMesh->mTextureCoords[2][a];
					if (pMesh->mTextureCoords[3]) {
						v.mTexCoords[3] = pMesh->mTextureCoords[3][a];
		}}}}

		// collect all vertices that are close enough to the given position
		vertexFinder->FindPositions( v.mPosition, posEpsilonSqr, verticesFound);

		unsigned int matchIndex = 0xffffffff;
		// check all unique vertices close to the position if this vertex is already present among them
		for( unsigned int b = 0; b < verticesFound.size(); b++)
		{
			const unsigned int vidx = verticesFound[b];
			const unsigned int uidx = replaceIndex[ vidx];
			if( uidx == 0xffffffff || !isVertexUnique[ vidx])
				continue;

			const Vertex& uv = uniqueVertices[ uidx];
			// Position mismatch is impossible - the vertex finder already discarded all non-matching positions

			// We just test the other attributes even if they're not present in the mesh.
			// In this case they're initialized to 0 so the comparision succeeds. 
			// By this method the non-present attributes are effectively ignored in the comparision.
			if( (uv.mNormal - v.mNormal).SquareLength() > squareEpsilon)
				continue;
			if( (uv.mTangent - v.mTangent).SquareLength() > squareEpsilon)
				continue;
			if( (uv.mBitangent - v.mBitangent).SquareLength() > squareEpsilon)
				continue;

			if( (uv.mTexCoords[0] - v.mTexCoords[0]).SquareLength() > squareEpsilon)
				continue;

			// Usually we won't have vertex colors or multiple UVs, so we can skip from here
			// Actually this increases runtime performance slightly.
			if (complex) 
			{
				// manually unrolled because continue wouldn't work as desired in an inner loop
				if( GetColorDifference( uv.mColors[0], v.mColors[0]) > squareEpsilon)
					continue;
				if( GetColorDifference( uv.mColors[1], v.mColors[1]) > squareEpsilon)
					continue;
				if( GetColorDifference( uv.mColors[2], v.mColors[2]) > squareEpsilon)
					continue;
				if( GetColorDifference( uv.mColors[3], v.mColors[3]) > squareEpsilon)
					continue;

				// texture coord matching manually unrolled as well
				if( (uv.mTexCoords[1] - v.mTexCoords[1]).SquareLength() > squareEpsilon)
					continue;
				if( (uv.mTexCoords[2] - v.mTexCoords[2]).SquareLength() > squareEpsilon)
					continue;
				if( (uv.mTexCoords[3] - v.mTexCoords[3]).SquareLength() > squareEpsilon)
					continue;
			}

			// we're still here -> this vertex perfectly matches our given vertex
			matchIndex = uidx;
			break;
		}

		// found a replacement vertex among the uniques?
		if( matchIndex != 0xffffffff)
		{
			// store where to found the matching unique vertex
			replaceIndex[a] = matchIndex;
			isVertexUnique[a] = false;
		}
		else
		{
			// no unique vertex matches it upto now -> so add it
			replaceIndex[a] = (unsigned int)uniqueVertices.size();
			uniqueVertices.push_back( v);
			isVertexUnique[a] = true;
		}
	}

	if (!DefaultLogger::isNullLogger() && DefaultLogger::get()->getLogSeverity() == Logger::VERBOSE)
	{
		char szBuff[128]; // should be sufficiently large in every case
		::sprintf(szBuff,"Mesh %i | Verts in: %i out: %i | ~%.1f%%",
			meshIndex,
			pMesh->mNumVertices,
			(int)uniqueVertices.size(),
			((pMesh->mNumVertices - uniqueVertices.size()) / (float)pMesh->mNumVertices) * 100.f);
		DefaultLogger::get()->debug(szBuff);
	}

	// replace vertex data with the unique data sets
	pMesh->mNumVertices = (unsigned int)uniqueVertices.size();

	// Position
	delete [] pMesh->mVertices;
	pMesh->mVertices = new aiVector3D[pMesh->mNumVertices];
	for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
		pMesh->mVertices[a] = uniqueVertices[a].mPosition;

	// Normals, if present
	if( pMesh->mNormals)
	{
		delete [] pMesh->mNormals;
		pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];
		for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
			pMesh->mNormals[a] = uniqueVertices[a].mNormal;
	}
	// Tangents, if present
	if( pMesh->mTangents)
	{
		delete [] pMesh->mTangents;
		pMesh->mTangents = new aiVector3D[pMesh->mNumVertices];
		for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
			pMesh->mTangents[a] = uniqueVertices[a].mTangent;
	}
	// Bitangents as well
	if( pMesh->mBitangents)
	{
		delete [] pMesh->mBitangents;
		pMesh->mBitangents = new aiVector3D[pMesh->mNumVertices];
		for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
			pMesh->mBitangents[a] = uniqueVertices[a].mBitangent;
	}
	// Vertex colors
	for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++)
	{
		if( !pMesh->mColors[a])
			break;

		delete [] pMesh->mColors[a];
		pMesh->mColors[a] = new aiColor4D[pMesh->mNumVertices];
		for( unsigned int b = 0; b < pMesh->mNumVertices; b++)
			pMesh->mColors[a][b] = uniqueVertices[b].mColors[a];
	}
	// Texture coords
	for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++)
	{
		if( !pMesh->mTextureCoords[a])
			break;

		delete [] pMesh->mTextureCoords[a];
		pMesh->mTextureCoords[a] = new aiVector3D[pMesh->mNumVertices];
		for( unsigned int b = 0; b < pMesh->mNumVertices; b++)
			pMesh->mTextureCoords[a][b] = uniqueVertices[b].mTexCoords[a];
	}

	// adjust the indices in all faces
	for( unsigned int a = 0; a < pMesh->mNumFaces; a++)
	{
		aiFace& face = pMesh->mFaces[a];
		for( unsigned int b = 0; b < face.mNumIndices; b++)	{
			face.mIndices[b] = replaceIndex[face.mIndices[b]];
		}
	}

	// adjust bone vertex weights.
	for( int a = 0; a < (int)pMesh->mNumBones; a++)
	{
		aiBone* bone = pMesh->mBones[a];
		std::vector<aiVertexWeight> newWeights;
		newWeights.reserve( bone->mNumWeights);

		for( unsigned int b = 0; b < bone->mNumWeights; b++)
		{
			const aiVertexWeight& ow = bone->mWeights[b];
			// if the vertex is a unique one, translate it
			if( isVertexUnique[ow.mVertexId])
			{
				aiVertexWeight nw;
				nw.mVertexId = replaceIndex[ow.mVertexId];
				nw.mWeight = ow.mWeight;
				newWeights.push_back( nw);
			}
		}

		if (newWeights.size() > 0) {
			// kill the old and replace them with the translated weights
			delete [] bone->mWeights;
			bone->mNumWeights = (unsigned int)newWeights.size();

			bone->mWeights = new aiVertexWeight[bone->mNumWeights];
			memcpy( bone->mWeights, &newWeights[0], bone->mNumWeights * sizeof( aiVertexWeight));
		}
		else {
		
			/*  NOTE:
			 *
			 *  In the algorithm above we're assuming that there are no vertices
			 *  with a different bone weight setup at the same position. That wouldn't
			 *  make sense, but it is not absolutely impossible. SkeletonMeshBuilder
			 *  for example generates such input data if two skeleton points
			 *  share the same position. Again this doesn't make sense but is
			 *  reality for some model formats (MD5 for example uses these special
			 *  nodes as attachment tags for its weapons). 
			 *
			 *  Then it is possible that a bone has no weights anymore .... as a quick
			 *  workaround, we're just removing these bones. If they're animated,
			 *  model geometry might be modified but at least there's no risk of a crash.
			 */
			delete bone;
			--pMesh->mNumBones;
			for (unsigned int n = a; n < pMesh->mNumBones; ++n) 
				pMesh->mBones[n] = pMesh->mBones[n+1];

			--a; 
			DefaultLogger::get()->warn("Removing bone -> no weights remaining");
		}
	}
	return pMesh->mNumVertices;
}

#endif // !! ASSIMP_BUILD_NO_JOINVERTICES_PROCESS