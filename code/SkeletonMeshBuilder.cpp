/** Implementation of a little class to construct a dummy mesh for a skeleton */

/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
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

----------------------------------------------------------------------
*/

#include "AssimpPCH.h"
#include "../include/aiScene.h"
#include "SkeletonMeshBuilder.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// The constructor processes the given scene and adds a mesh there. 
SkeletonMeshBuilder::SkeletonMeshBuilder( aiScene* pScene)
{
	// nothing to do if there's mesh data already present at the scene
	if( pScene->mNumMeshes > 0 || pScene->mRootNode == NULL)
		return;

	// build some faces around each node 
	CreateGeometry( pScene->mRootNode);

	// create a mesh to hold all the generated faces
	pScene->mNumMeshes = 1;
	pScene->mMeshes = new aiMesh*[1];
	pScene->mMeshes[0] = CreateMesh();
	// and install it at the root node
	pScene->mRootNode->mNumMeshes = 1;
	pScene->mRootNode->mMeshes = new unsigned int[1];
	pScene->mRootNode->mMeshes[0] = 0;

	// create a dummy material for the mesh
	pScene->mNumMaterials = 1;
	pScene->mMaterials = new aiMaterial*[1];
	pScene->mMaterials[0] = CreateMaterial();
}

// ------------------------------------------------------------------------------------------------
// Recursively builds a simple mesh representation for the given node 
void SkeletonMeshBuilder::CreateGeometry( const aiNode* pNode)
{
	// add a joint entry for the node. 
	const unsigned int boneIndex = mBones.size();
	const unsigned int vertexStartIndex = mVertices.size();

	// now build the geometry. 
	if( pNode->mNumChildren > 0)
	{
		// If the node has childs, we build little pointers to each of them
		for( unsigned int a = 0; a < pNode->mNumChildren; a++)
		{
			// find a suitable coordinate system
			const aiMatrix4x4& childTransform = pNode->mChildren[a]->mTransformation;
			aiVector3D childpos( childTransform.a4, childTransform.b4, childTransform.c4);
			float distanceToChild = childpos.Length();
			if( distanceToChild < 0.0001f)
				continue;
			aiVector3D up = aiVector3D( childpos).Normalize();

			aiVector3D orth( 1.0f, 0.0f, 0.0f);
			if( abs( orth * up) > 0.99f)
				orth.Set( 0.0f, 1.0f, 0.0f);

			aiVector3D front = (up ^ orth).Normalize();
			aiVector3D side = (front ^ up).Normalize();

	  		unsigned int localVertexStart = mVertices.size();
			mVertices.push_back( -front * distanceToChild * 0.1f);
			mVertices.push_back( childpos);
			mVertices.push_back( -side * distanceToChild * 0.1f);
			mVertices.push_back( -side * distanceToChild * 0.1f);
			mVertices.push_back( childpos);
			mVertices.push_back( front * distanceToChild * 0.1f);
			mVertices.push_back( front * distanceToChild * 0.1f);
			mVertices.push_back( childpos);
			mVertices.push_back( side * distanceToChild * 0.1f);
			mVertices.push_back( side * distanceToChild * 0.1f);
			mVertices.push_back( childpos);
			mVertices.push_back( -front * distanceToChild * 0.1f);

			mFaces.push_back( Face( localVertexStart + 0, localVertexStart + 1, localVertexStart + 2));
			mFaces.push_back( Face( localVertexStart + 3, localVertexStart + 4, localVertexStart + 5));
			mFaces.push_back( Face( localVertexStart + 6, localVertexStart + 7, localVertexStart + 8));
			mFaces.push_back( Face( localVertexStart + 9, localVertexStart + 10, localVertexStart + 11));
		}
	} else
	{
		// if the node has no children, it's an end node. Put a little knob there instead
		aiVector3D ownpos( pNode->mTransformation.a4, pNode->mTransformation.b4, pNode->mTransformation.c4);
		float sizeEstimate = ownpos.Length() * 0.2f;

		mVertices.push_back( aiVector3D( -sizeEstimate, 0.0f, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, sizeEstimate, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, 0.0f, -sizeEstimate));
		mVertices.push_back( aiVector3D( 0.0f, sizeEstimate, 0.0f));
		mVertices.push_back( aiVector3D( sizeEstimate, 0.0f, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, 0.0f, -sizeEstimate));
		mVertices.push_back( aiVector3D( sizeEstimate, 0.0f, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, -sizeEstimate, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, 0.0f, -sizeEstimate));
		mVertices.push_back( aiVector3D( 0.0f, -sizeEstimate, 0.0f));
		mVertices.push_back( aiVector3D( -sizeEstimate, 0.0f, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, 0.0f, -sizeEstimate));

		mVertices.push_back( aiVector3D( -sizeEstimate, 0.0f, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, 0.0f, sizeEstimate));
		mVertices.push_back( aiVector3D( 0.0f, sizeEstimate, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, sizeEstimate, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, 0.0f, sizeEstimate));
		mVertices.push_back( aiVector3D( sizeEstimate, 0.0f, 0.0f));
		mVertices.push_back( aiVector3D( sizeEstimate, 0.0f, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, 0.0f, sizeEstimate));
		mVertices.push_back( aiVector3D( 0.0f, -sizeEstimate, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, -sizeEstimate, 0.0f));
		mVertices.push_back( aiVector3D( 0.0f, 0.0f, sizeEstimate));
		mVertices.push_back( aiVector3D( -sizeEstimate, 0.0f, 0.0f));

		mFaces.push_back( Face( vertexStartIndex + 0, vertexStartIndex + 1, vertexStartIndex + 2));
		mFaces.push_back( Face( vertexStartIndex + 3, vertexStartIndex + 4, vertexStartIndex + 5));
		mFaces.push_back( Face( vertexStartIndex + 6, vertexStartIndex + 7, vertexStartIndex + 8));
		mFaces.push_back( Face( vertexStartIndex + 9, vertexStartIndex + 10, vertexStartIndex + 11));
		mFaces.push_back( Face( vertexStartIndex + 12, vertexStartIndex + 13, vertexStartIndex + 14));
		mFaces.push_back( Face( vertexStartIndex + 15, vertexStartIndex + 16, vertexStartIndex + 17));
		mFaces.push_back( Face( vertexStartIndex + 18, vertexStartIndex + 19, vertexStartIndex + 20));
		mFaces.push_back( Face( vertexStartIndex + 21, vertexStartIndex + 22, vertexStartIndex + 23));
	}

	unsigned int numVertices = mVertices.size() - vertexStartIndex;
	if( numVertices > 0)
	{
		// create a bone affecting all the newly created vertices
		aiBone* bone = new aiBone;
		mBones.push_back( bone);
		bone->mName = pNode->mName;

		// calculate the bone offset matrix by concatenating the inverse transformations of all parents
		bone->mOffsetMatrix = aiMatrix4x4( pNode->mTransformation).Inverse();
		for( aiNode* parent = pNode->mParent; parent != NULL; parent = parent->mParent)
			bone->mOffsetMatrix = aiMatrix4x4( parent->mTransformation).Inverse() * bone->mOffsetMatrix;

		// add all the vertices to the bone's influences
		bone->mNumWeights = numVertices;
		bone->mWeights = new aiVertexWeight[numVertices];
		for( unsigned int a = 0; a < numVertices; a++)
			bone->mWeights[a] = aiVertexWeight( vertexStartIndex + a, 1.0f);

		// HACK: (thom) transform all vertices to the bone's local space. Should be done before adding
		// them to the array, but I'm tired now and I'm annoyed.
		aiMatrix4x4 boneToMeshTransform = aiMatrix4x4( bone->mOffsetMatrix).Inverse();
		for( unsigned int a = vertexStartIndex; a < mVertices.size(); a++)
			mVertices[a] = boneToMeshTransform * mVertices[a];
	}

	// and finally recurse into the children list
	for( unsigned int a = 0; a < pNode->mNumChildren; a++)
		CreateGeometry( pNode->mChildren[a]);
}

// ------------------------------------------------------------------------------------------------
// Creates the mesh from the internally accumulated stuff and returns it.
aiMesh* SkeletonMeshBuilder::CreateMesh()
{
	aiMesh* mesh = new aiMesh();

	// add points
	mesh->mNumVertices = mVertices.size();
	mesh->mVertices = new aiVector3D[mesh->mNumVertices];
	std::copy( mVertices.begin(), mVertices.end(), mesh->mVertices);

	// add faces
	mesh->mNumFaces = mFaces.size();
	mesh->mFaces = new aiFace[mesh->mNumFaces];
	for( unsigned int a = 0; a < mesh->mNumFaces; a++)
	{
		const Face& inface = mFaces[a];
		aiFace& outface = mesh->mFaces[a];
		outface.mNumIndices = 3;
		outface.mIndices = new unsigned int[3];
		outface.mIndices[0] = inface.mIndices[0];
		outface.mIndices[1] = inface.mIndices[1];
		outface.mIndices[2] = inface.mIndices[2];
	}

	// add the bones
	mesh->mNumBones = mBones.size();
	mesh->mBones = new aiBone*[mesh->mNumBones];
	std::copy( mBones.begin(), mBones.end(), mesh->mBones);

	// default
	mesh->mMaterialIndex = 0;

	return mesh;
}

// ------------------------------------------------------------------------------------------------
// Creates a dummy material and returns it.
aiMaterial* SkeletonMeshBuilder::CreateMaterial()
{
	Assimp::MaterialHelper* matHelper = new Assimp::MaterialHelper;

	// Name
	aiString matName( std::string( "Material"));
	matHelper->AddProperty( &matName, AI_MATKEY_NAME);

	return matHelper;
}
