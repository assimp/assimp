/** @file Implementation of the post processing step to convert all imported data
 * to a left-handed coordinate system.
 */
#include "ConvertToLHProcess.h"
#include "../include/DefaultLogger.h"
#include "../include/aiPostProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiAnim.h"
#include "../include/aiScene.h"


using namespace Assimp;

// The transformation matrix to convert from DirectX coordinates to OpenGL coordinates.
const aiMatrix3x3 Assimp::ConvertToLHProcess::sToOGLTransform(
	1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f
	);
// The transformation matrix to convert from OpenGL coordinates to DirectX coordinates.
const aiMatrix3x3 Assimp::ConvertToLHProcess::sToDXTransform(
	1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f
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
		if (pFlags & aiProcess_PreTransformVertices)
			this->bTransformVertices = true;
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
	if (this->bTransformVertices)
	{
		this->bTransformVertices = false;
		aiMatrix4x4 mTransform;
		this->ConvertToDX(mTransform);
		for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		{
			aiMesh* pcMesh = pScene->mMeshes[i];
			for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
			{
				pcMesh->mVertices[n] = mTransform * pcMesh->mVertices[n];
			}
			if (pcMesh->HasNormals())
			{
				mTransform.Inverse().Transpose();
				for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
				{
					pcMesh->mNormals[n] = mTransform * pcMesh->mNormals[n];
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

	// transform all animation channels affecting the root node as well
	for( unsigned int a = 0; a < pScene->mNumAnimations; a++)
	{
		aiAnimation* anim = pScene->mAnimations[a];
		for( unsigned int b = 0; b < anim->mNumBones; b++)
		{
			aiBoneAnim* boneAnim = anim->mBones[b];
			if( strcmp( boneAnim->mBoneName.data, pScene->mRootNode->mName.data) == 0)
				ProcessAnimation( boneAnim);
		}
	}
	DefaultLogger::get()->debug("ConvertToLHProcess finished");
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
void ConvertToLHProcess::ProcessAnimation( aiBoneAnim* pAnim)
{
	// position keys
	for( unsigned int a = 0; a < pAnim->mNumPositionKeys; a++)
		ConvertToDX( pAnim->mPositionKeys[a].mValue);

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
	poMatrix *= sToOGLTransform;
}

// ------------------------------------------------------------------------------------------------
void ConvertToLHProcess::ConvertToOGL( aiMatrix4x4& poMatrix)
{
	poMatrix *= aiMatrix4x4( sToOGLTransform);
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
	poMatrix *= sToDXTransform;
}

// ------------------------------------------------------------------------------------------------
void ConvertToLHProcess::ConvertToDX( aiMatrix4x4& poMatrix)
{
	aiMatrix4x4 temp(sToDXTransform);
	poMatrix *= temp;
}
