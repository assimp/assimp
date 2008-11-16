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

/** @file Implementation of the post processing step to convert all imported data
 * to a left-handed coordinate system.
 */

#include "AssimpPCH.h"
#include "ConvertToLHProcess.h"


using namespace Assimp;

// The transformation matrix to convert from DirectX coordinates to OpenGL coordinates.
const aiMatrix3x3 Assimp::ConvertToLHProcess::sToOGLTransform(
	1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f
	);
// The transformation matrix to convert from OpenGL coordinates to DirectX coordinates.
const aiMatrix3x3 Assimp::ConvertToLHProcess::sToDXTransform(
	1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f
	);

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ConvertToLHProcess::ConvertToLHProcess()
{
	bTransformVertices = false;
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
ConvertToLHProcess::~ConvertToLHProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool ConvertToLHProcess::IsActive( unsigned int pFlags) const
{
	if (pFlags & aiProcess_ConvertToLeftHanded)
	{
		bTransformVertices = (0 != (pFlags & aiProcess_PreTransformVertices) ? true : false);
		return true;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void ConvertToLHProcess::Execute( aiScene* pScene)
{
	// Check for an existent root node to proceed
	if (NULL == pScene->mRootNode)
	{
		DefaultLogger::get()->error("ConvertToLHProcess fails, there is no root node");
		return;
	}

	DefaultLogger::get()->debug("ConvertToLHProcess begin");

	// transform vertex by vertex or change the root transform?
	// We can't do the coordinate system transformation earlier 
	// in the pipeline - most steps assume that we're in OGL
	// space. So we need to transform all vertices a second time
	// here.
	if (bTransformVertices)
	{
		aiMatrix4x4 mTransform;
		ConvertToDX(mTransform);
		for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		{
			aiMesh* pcMesh = pScene->mMeshes[i];

			// transform all vertices
			for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
				pcMesh->mVertices[n]  = mTransform * pcMesh->mVertices[n];
		
			// transform all normals
			if (pcMesh->HasNormals())
			{
				for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
					pcMesh->mNormals[n]  = mTransform * pcMesh->mNormals[n];
			}
			// transform all tangents and all bitangents
			if (pcMesh->HasTangentsAndBitangents())
			{
				for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
				{
					pcMesh->mTangents[n]    = mTransform * pcMesh->mTangents[n];
					pcMesh->mBitangents[n]  = mTransform * pcMesh->mBitangents[n];
				}
			}
		}
	}
	else
	{
		// transform the root node of the scene, the other nodes will follow then
		ConvertToDX( pScene->mRootNode->mTransformation);
	}

	// transform all meshes accordingly
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
		ProcessMesh( pScene->mMeshes[a]);

	// process all materials - we need to adjust UV transformations
	for( unsigned int a = 0; a < pScene->mNumMaterials; a++)
		ProcessMaterial( pScene->mMaterials[a]);

	// transform all animation channels affecting the root node as well
	for( unsigned int a = 0; a < pScene->mNumAnimations; a++)
	{
		aiAnimation* anim = pScene->mAnimations[a];
		for( unsigned int b = 0; b < anim->mNumChannels; b++)
		{
			aiNodeAnim* nodeAnim = anim->mChannels[b];
			if( strcmp( nodeAnim->mNodeName.data, pScene->mRootNode->mName.data) == 0)
				ProcessAnimation( nodeAnim);
		}
	}
	DefaultLogger::get()->debug("ConvertToLHProcess finished");
}

// ------------------------------------------------------------------------------------------------
// Converts a single material to left handed coordinates. 
void ConvertToLHProcess::ProcessMaterial (aiMaterial* mat)
{
	for (unsigned int a = 0; a < mat->mNumProperties;++a)
	{
		aiMaterialProperty* prop = mat->mProperties[a];
		if (!::strcmp( prop->mKey.data, "$tex.uvtrafo"))
		{
			ai_assert( prop->mDataLength >= sizeof(aiUVTransform));
			aiUVTransform* uv = (aiUVTransform*)prop->mData;

			// just flip it, that's everything
			uv->mTranslation.y *= -1.f;
			uv->mRotation *= -1.f;
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Converts a single mesh to left handed coordinates. 
void ConvertToLHProcess::ProcessMesh( aiMesh* pMesh)
{
	// invert the order of all faces in this mesh
	for( unsigned int a = 0; a < pMesh->mNumFaces; a++)
	{
		aiFace& face = pMesh->mFaces[a];
		for( unsigned int b = 0; b < face.mNumIndices / 2; b++)
			std::swap( face.mIndices[b], face.mIndices[ face.mNumIndices - 1 - b]);
	}

	// mirror texture y coordinate
	for( unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++)
	{
		if( pMesh->HasTextureCoords( a))
		{
			for( unsigned int b = 0; b < pMesh->mNumVertices; b++)
				pMesh->mTextureCoords[a][b].y = 1.0f - pMesh->mTextureCoords[a][b].y;
		}
	}

	// mirror bitangents as well as they're derived from the texture coords
	if( pMesh->HasTangentsAndBitangents())
	{
		for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
			pMesh->mBitangents[a] = -pMesh->mBitangents[a];
	}
}

// ------------------------------------------------------------------------------------------------
// Converts the given animation to LH coordinates. 
void ConvertToLHProcess::ProcessAnimation( aiNodeAnim* pAnim)
{
	// position keys
	for( unsigned int a = 0; a < pAnim->mNumPositionKeys; a++)
		ConvertToDX( pAnim->mPositionKeys[a].mValue);

	return;
	// rotation keys
	for( unsigned int a = 0; a < pAnim->mNumRotationKeys; a++)
	{
		aiMatrix3x3 rotmat = pAnim->mRotationKeys[a].mValue.GetMatrix();
		ConvertToDX( rotmat);
		pAnim->mRotationKeys[a].mValue = aiQuaternion( rotmat);
	}
}

// ------------------------------------------------------------------------------------------------
// Static helper function to convert a vector/matrix from DX to OGL coords
void ConvertToLHProcess::ConvertToOGL( aiVector3D& poVector)
{
	poVector = sToOGLTransform * poVector;
}

// ------------------------------------------------------------------------------------------------
void ConvertToLHProcess::ConvertToOGL( aiMatrix3x3& poMatrix)
{
	poMatrix = sToOGLTransform * poMatrix;
}

// ------------------------------------------------------------------------------------------------
void ConvertToLHProcess::ConvertToOGL( aiMatrix4x4& poMatrix)
{
	poMatrix = aiMatrix4x4( sToOGLTransform) * poMatrix;
}

// ------------------------------------------------------------------------------------------------
// Static helper function to convert a vector/matrix from OGL back to DX coords
void ConvertToLHProcess::ConvertToDX( aiVector3D& poVector)
{
	poVector = sToDXTransform * poVector;
}

// ------------------------------------------------------------------------------------------------
void ConvertToLHProcess::ConvertToDX( aiMatrix3x3& poMatrix)
{
	poMatrix = sToDXTransform * poMatrix;
}

// ------------------------------------------------------------------------------------------------
void ConvertToLHProcess::ConvertToDX( aiMatrix4x4& poMatrix)
{
	poMatrix = aiMatrix4x4(sToDXTransform) * poMatrix;
}
