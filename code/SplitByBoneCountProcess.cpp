/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

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

----------------------------------------------------------------------
*/


/// @file SplitByBoneCountProcess.cpp 
/// Implementation of the SplitByBoneCount postprocessing step

// internal headers of the post-processing framework
#include "SplitByBoneCountProcess.h"
#include "../include/assimp/postprocess.h"
#include "../include/assimp/DefaultLogger.hpp"

#include <limits>
#include <boost/format.hpp>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor
SplitByBoneCountProcess::SplitByBoneCountProcess()
{
	// set default, might be overriden by importer config
	mMaxBoneCount = AI_SBBC_DEFAULT_MAX_BONES;
}

// ------------------------------------------------------------------------------------------------
// Destructor
SplitByBoneCountProcess::~SplitByBoneCountProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag.
bool SplitByBoneCountProcess::IsActive( unsigned int pFlags) const
{
	return !!(pFlags & aiProcess_SplitByBoneCount);
}

// ------------------------------------------------------------------------------------------------
// Updates internal properties
void SplitByBoneCountProcess::SetupProperties(const Importer* pImp)
{
	mMaxBoneCount = pImp->GetPropertyInteger(AI_CONFIG_PP_SBBC_MAX_BONES,AI_SBBC_DEFAULT_MAX_BONES);
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void SplitByBoneCountProcess::Execute( aiScene* pScene)
{
	DefaultLogger::get()->debug("SplitByBoneCountProcess begin");

	// early out 
	bool isNecessary = false;
	for( size_t a = 0; a < pScene->mNumMeshes; ++a)
		if( pScene->mMeshes[a]->mNumBones > mMaxBoneCount )
			isNecessary = true;

	if( !isNecessary )
	{
		DefaultLogger::get()->debug( boost::str( boost::format( "SplitByBoneCountProcess early-out: no meshes with more than %d bones.") % mMaxBoneCount));
		return;
	}

	// we need to do something. Let's go.
	mSubMeshIndices.clear();
	mSubMeshIndices.resize( pScene->mNumMeshes);

	// build a new array of meshes for the scene
	std::vector<aiMesh*> meshes;

	for( size_t a = 0; a < pScene->mNumMeshes; ++a)
	{
		aiMesh* srcMesh = pScene->mMeshes[a];

		std::vector<aiMesh*> newMeshes;
		SplitMesh( pScene->mMeshes[a], newMeshes);

		// mesh was split
		if( !newMeshes.empty() )
		{
			// store new meshes and indices of the new meshes
			for( size_t b = 0; b < newMeshes.size(); ++b)
			{
				mSubMeshIndices[a].push_back( meshes.size());
				meshes.push_back( newMeshes[b]);
			}

			// and destroy the source mesh. It should be completely contained inside the new submeshes
			delete srcMesh;
		}
		else
		{
			// Mesh is kept unchanged - store it's new place in the mesh array
			mSubMeshIndices[a].push_back( meshes.size());
			meshes.push_back( srcMesh);
		}
	}

	// rebuild the scene's mesh array
	pScene->mNumMeshes = meshes.size();
	delete [] pScene->mMeshes;
	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
	std::copy( meshes.begin(), meshes.end(), pScene->mMeshes);

	// recurse through all nodes and translate the node's mesh indices to fit the new mesh array
	UpdateNode( pScene->mRootNode);

	DefaultLogger::get()->debug( boost::str( boost::format( "SplitByBoneCountProcess end: split %d meshes into %d submeshes.") % mSubMeshIndices.size() % meshes.size()));
}

// ------------------------------------------------------------------------------------------------
// Splits the given mesh by bone count.
void SplitByBoneCountProcess::SplitMesh( const aiMesh* pMesh, std::vector<aiMesh*>& poNewMeshes) const
{
	// skip if not necessary
	if( pMesh->mNumBones <= mMaxBoneCount )
		return;

	// necessary optimisation: build a list of all affecting bones for each vertex
	// TODO: (thom) maybe add a custom allocator here to avoid allocating tens of thousands of small arrays
	typedef std::pair<size_t, float> BoneWeight;
	std::vector< std::vector<BoneWeight> > vertexBones( pMesh->mNumVertices);
	for( size_t a = 0; a < pMesh->mNumBones; ++a)
	{
		const aiBone* bone = pMesh->mBones[a];
		for( size_t b = 0; b < bone->mNumWeights; ++b)
			vertexBones[ bone->mWeights[b].mVertexId ].push_back( BoneWeight( a, bone->mWeights[b].mWeight));
	}

	size_t numFacesHandled = 0;
	std::vector<bool> isFaceHandled( pMesh->mNumFaces, false);
	while( numFacesHandled < pMesh->mNumFaces )
	{
		// which bones are used in the current submesh
		size_t numBones = 0;
		std::vector<bool> isBoneUsed( pMesh->mNumBones, false);
		// indices of the faces which are going to go into this submesh
		std::vector<size_t> subMeshFaces;
		subMeshFaces.reserve( pMesh->mNumFaces);
		// accumulated vertex count of all the faces in this submesh
		size_t numSubMeshVertices = 0;
		// a small local array of new bones for the current face. State of all used bones for that face
		// can only be updated AFTER the face is completely analysed. Thanks to imre for the fix.
		std::vector<size_t> newBonesAtCurrentFace;

		// add faces to the new submesh as long as all bones affecting the faces' vertices fit in the limit
		for( size_t a = 0; a < pMesh->mNumFaces; ++a)
		{
			// skip if the face is already stored in a submesh
			if( isFaceHandled[a] )
				continue;

			const aiFace& face = pMesh->mFaces[a];
			// check every vertex if its bones would still fit into the current submesh
			for( size_t b = 0; b < face.mNumIndices; ++b )
			{
				const std::vector<BoneWeight>& vb = vertexBones[face.mIndices[b]];
				for( size_t c = 0; c < vb.size(); ++c)
				{
					size_t boneIndex = vb[c].first;
					// if the bone is already used in this submesh, it's ok
					if( isBoneUsed[boneIndex] )
						continue;

					// if it's not used, yet, we would need to add it. Store its bone index
					if( std::find( newBonesAtCurrentFace.begin(), newBonesAtCurrentFace.end(), boneIndex) == newBonesAtCurrentFace.end() )
						newBonesAtCurrentFace.push_back( boneIndex);
				}
			}

			// leave out the face if the new bones required for this face don't fit the bone count limit anymore
			if( numBones + newBonesAtCurrentFace.size() > mMaxBoneCount )
				continue;

			// mark all new bones as necessary
			while( !newBonesAtCurrentFace.empty() )
			{
				size_t newIndex = newBonesAtCurrentFace.back();
				newBonesAtCurrentFace.pop_back(); // this also avoids the deallocation which comes with a clear()
				if( isBoneUsed[newIndex] ) 
					continue;

				isBoneUsed[newIndex] = true;
				numBones++;
			}

			// store the face index and the vertex count
			subMeshFaces.push_back( a);
			numSubMeshVertices += face.mNumIndices;

			// remember that this face is handled
			isFaceHandled[a] = true;
			numFacesHandled++;
		}

		// create a new mesh to hold this subset of the source mesh
		aiMesh* newMesh = new aiMesh;
		if( pMesh->mName.length > 0 )
			newMesh->mName.Set( boost::str( boost::format( "%s_sub%d") % pMesh->mName.data % poNewMeshes.size()));
		newMesh->mMaterialIndex = pMesh->mMaterialIndex;
		newMesh->mPrimitiveTypes = pMesh->mPrimitiveTypes;
		poNewMeshes.push_back( newMesh);

		// create all the arrays for this mesh if the old mesh contained them
		newMesh->mNumVertices = numSubMeshVertices;
		newMesh->mNumFaces = subMeshFaces.size();
		newMesh->mVertices = new aiVector3D[newMesh->mNumVertices];
		if( pMesh->HasNormals() )
			newMesh->mNormals = new aiVector3D[newMesh->mNumVertices];
		if( pMesh->HasTangentsAndBitangents() )
		{
			newMesh->mTangents = new aiVector3D[newMesh->mNumVertices];
			newMesh->mBitangents = new aiVector3D[newMesh->mNumVertices];
		}
		for( size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a )
		{
			if( pMesh->HasTextureCoords( a) )
				newMesh->mTextureCoords[a] = new aiVector3D[newMesh->mNumVertices];
			newMesh->mNumUVComponents[a] = pMesh->mNumUVComponents[a];
		}
		for( size_t a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; ++a )
		{
			if( pMesh->HasVertexColors( a) )
				newMesh->mColors[a] = new aiColor4D[newMesh->mNumVertices];
		}

		// and copy over the data, generating faces with linear indices along the way
		newMesh->mFaces = new aiFace[subMeshFaces.size()];
		size_t nvi = 0; // next vertex index
		std::vector<size_t> previousVertexIndices( numSubMeshVertices, std::numeric_limits<size_t>::max()); // per new vertex: its index in the source mesh
		for( size_t a = 0; a < subMeshFaces.size(); ++a )
		{
			const aiFace& srcFace = pMesh->mFaces[subMeshFaces[a]];
			aiFace& dstFace = newMesh->mFaces[a];
			dstFace.mNumIndices = srcFace.mNumIndices;
			dstFace.mIndices = new unsigned int[dstFace.mNumIndices];

			// accumulate linearly all the vertices of the source face
			for( size_t b = 0; b < dstFace.mNumIndices; ++b )
			{
				size_t srcIndex = srcFace.mIndices[b];
				dstFace.mIndices[b] = nvi;
				previousVertexIndices[nvi] = srcIndex;

				newMesh->mVertices[nvi] = pMesh->mVertices[srcIndex];
				if( pMesh->HasNormals() )
					newMesh->mNormals[nvi] = pMesh->mNormals[srcIndex];
				if( pMesh->HasTangentsAndBitangents() )
				{
					newMesh->mTangents[nvi] = pMesh->mTangents[srcIndex];
					newMesh->mBitangents[nvi] = pMesh->mBitangents[srcIndex];
				}
				for( size_t c = 0; c < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++c )
				{
					if( pMesh->HasTextureCoords( c) )
						newMesh->mTextureCoords[c][nvi] = pMesh->mTextureCoords[c][srcIndex];
				}
				for( size_t c = 0; c < AI_MAX_NUMBER_OF_COLOR_SETS; ++c )
				{
					if( pMesh->HasVertexColors( c) )
						newMesh->mColors[c][nvi] = pMesh->mColors[c][srcIndex];
				}

				nvi++;
			}
		}

		ai_assert( nvi == numSubMeshVertices );

		// Create the bones for the new submesh: first create the bone array
		newMesh->mNumBones = 0;
		newMesh->mBones = new aiBone*[numBones];

		std::vector<size_t> mappedBoneIndex( pMesh->mNumBones, std::numeric_limits<size_t>::max());
		for( size_t a = 0; a < pMesh->mNumBones; ++a )
		{
			if( !isBoneUsed[a] )
				continue;

			// create the new bone
			const aiBone* srcBone = pMesh->mBones[a];
			aiBone* dstBone = new aiBone;
			mappedBoneIndex[a] = newMesh->mNumBones;
			newMesh->mBones[newMesh->mNumBones++] = dstBone;
			dstBone->mName = srcBone->mName;
			dstBone->mOffsetMatrix = srcBone->mOffsetMatrix;
			dstBone->mNumWeights = 0;
		}

		ai_assert( newMesh->mNumBones == numBones );

		// iterate over all new vertices and count which bones affected its old vertex in the source mesh
		for( size_t a = 0; a < numSubMeshVertices; ++a )
		{
			size_t oldIndex = previousVertexIndices[a];
			const std::vector<BoneWeight>& bonesOnThisVertex = vertexBones[oldIndex];

			for( size_t b = 0; b < bonesOnThisVertex.size(); ++b )
			{
				size_t newBoneIndex = mappedBoneIndex[ bonesOnThisVertex[b].first ];
				if( newBoneIndex != std::numeric_limits<size_t>::max() )
					newMesh->mBones[newBoneIndex]->mNumWeights++;
			}
		}

		// allocate all bone weight arrays accordingly
		for( size_t a = 0; a < newMesh->mNumBones; ++a )
		{
			aiBone* bone = newMesh->mBones[a];
			ai_assert( bone->mNumWeights > 0 );
			bone->mWeights = new aiVertexWeight[bone->mNumWeights];
			bone->mNumWeights = 0; // for counting up in the next step
		}

		// now copy all the bone vertex weights for all the vertices which made it into the new submesh
		for( size_t a = 0; a < numSubMeshVertices; ++a)
		{
			// find the source vertex for it in the source mesh
			size_t previousIndex = previousVertexIndices[a];
			// these bones were affecting it
			const std::vector<BoneWeight>& bonesOnThisVertex = vertexBones[previousIndex];
			// all of the bones affecting it should be present in the new submesh, or else
			// the face it comprises shouldn't be present
			for( size_t b = 0; b < bonesOnThisVertex.size(); ++b)
			{
				size_t newBoneIndex = mappedBoneIndex[ bonesOnThisVertex[b].first ];
				ai_assert( newBoneIndex != std::numeric_limits<size_t>::max() );
				aiVertexWeight* dstWeight = newMesh->mBones[newBoneIndex]->mWeights + newMesh->mBones[newBoneIndex]->mNumWeights;
				newMesh->mBones[newBoneIndex]->mNumWeights++;

				dstWeight->mVertexId = a;
				dstWeight->mWeight = bonesOnThisVertex[b].second;
			}
		}

		// I have the strange feeling that this will break apart at some point in time...
	}
}

// ------------------------------------------------------------------------------------------------
// Recursively updates the node's mesh list to account for the changed mesh list
void SplitByBoneCountProcess::UpdateNode( aiNode* pNode) const
{
	// rebuild the node's mesh index list
	if( pNode->mNumMeshes > 0 )
	{
		std::vector<size_t> newMeshList;
		for( size_t a = 0; a < pNode->mNumMeshes; ++a)
		{
			size_t srcIndex = pNode->mMeshes[a];
			const std::vector<size_t>& replaceMeshes = mSubMeshIndices[srcIndex];
			newMeshList.insert( newMeshList.end(), replaceMeshes.begin(), replaceMeshes.end());
		}

		delete pNode->mMeshes;
		pNode->mNumMeshes = newMeshList.size();
		pNode->mMeshes = new unsigned int[pNode->mNumMeshes];
		std::copy( newMeshList.begin(), newMeshList.end(), pNode->mMeshes);
	}

	// do that also recursively for all children
	for( size_t a = 0; a < pNode->mNumChildren; ++a )
	{
		UpdateNode( pNode->mChildren[a]);
	}
}
