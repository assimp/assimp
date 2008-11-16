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

/** @file Implementation of the "PretransformVertices" post processing step 
*/

#include "AssimpPCH.h"
#include "PretransformVertices.h"


using namespace Assimp;

// some array offsets
#define AI_PTVS_VERTEX 0x0
#define AI_PTVS_FACE 0x1

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
PretransformVertices::PretransformVertices()
{
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
PretransformVertices::~PretransformVertices()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool PretransformVertices::IsActive( unsigned int pFlags) const
{
	return	(pFlags & aiProcess_PreTransformVertices) != 0;
}

// ------------------------------------------------------------------------------------------------
// Count the number of nodes
unsigned int CountNodes( aiNode* pcNode )
{
	unsigned int iRet = 1;
	for (unsigned int i = 0;i < pcNode->mNumChildren;++i)
	{
		iRet += CountNodes(pcNode->mChildren[i]);
	}
	return iRet;
}

// ------------------------------------------------------------------------------------------------
// Get a bitwise combination identifying the vertex format of a mesh
unsigned int GetMeshVFormat(aiMesh* pcMesh)
{
	// the vertex format is stored in aiMesh::mBones for later retrieval.
	// there isn't a good reason to compute it a few hundred times
	// from scratch. The pointer is unused as animations are lost
	// during PretransformVertices.
	if (pcMesh->mBones)
		return (unsigned int)(unsigned long)pcMesh->mBones;

	ai_assert(NULL != pcMesh->mVertices);

	// FIX: the hash may never be 0. Otherwise a comparison against
	// nullptr could be successful
	unsigned int iRet = 1;

	// normals
	if (pcMesh->HasNormals())iRet |= 0x2;
	// tangents and bitangents
	if (pcMesh->HasTangentsAndBitangents())iRet |= 0x4;

	// texture coordinates
	unsigned int p = 0;
	ai_assert(8 >= AI_MAX_NUMBER_OF_TEXTURECOORDS);
	while (pcMesh->HasTextureCoords(p))
	{
		iRet |= (0x100 << p);
		if (3 == pcMesh->mNumUVComponents[p])
			iRet |= (0x10000 << p);

		++p;
	}
	// vertex colors
	p = 0;
	ai_assert(8 >= AI_MAX_NUMBER_OF_COLOR_SETS);
	while (pcMesh->HasVertexColors(p))iRet |= (0x1000000 << p++);

	// store the value for later use
	pcMesh->mBones = (aiBone**)(unsigned long)iRet;
	return iRet;
}

// ------------------------------------------------------------------------------------------------
// Count the number of vertices in the whole scene and a given
// material index
void CountVerticesAndFaces( aiScene* pcScene, aiNode* pcNode, unsigned int iMat,
	unsigned int iVFormat, unsigned int* piFaces, unsigned int* piVertices)
{
	for (unsigned int i = 0; i < pcNode->mNumMeshes;++i)
	{
		aiMesh* pcMesh = pcScene->mMeshes[ pcNode->mMeshes[i] ]; 
		if (iMat == pcMesh->mMaterialIndex && iVFormat == GetMeshVFormat(pcMesh))
		{
			*piVertices += pcMesh->mNumVertices;
			*piFaces += pcMesh->mNumFaces;
		}
	}
	for (unsigned int i = 0;i < pcNode->mNumChildren;++i)
	{
		CountVerticesAndFaces(pcScene,pcNode->mChildren[i],iMat,
			iVFormat,piFaces,piVertices);
	}
}

// ------------------------------------------------------------------------------------------------
// Collect vertex/face data
void CollectData( aiScene* pcScene, aiNode* pcNode, unsigned int iMat,
	unsigned int iVFormat, aiMesh* pcMeshOut, 
	unsigned int aiCurrent[2])
{
	for (unsigned int i = 0; i < pcNode->mNumMeshes;++i)
	{
		aiMesh* pcMesh = pcScene->mMeshes[ pcNode->mMeshes[i] ]; 
		if (iMat == pcMesh->mMaterialIndex && iVFormat == GetMeshVFormat(pcMesh))
		{
			// copy positions, transform them to worldspace
			for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
			{
				pcMeshOut->mVertices[aiCurrent[AI_PTVS_VERTEX]+n] = 
					pcNode->mTransformation * pcMesh->mVertices[n];
			}
			if (iVFormat & 0x2)
			{
				aiMatrix4x4 mWorldIT = pcNode->mTransformation;
				mWorldIT.Inverse().Transpose();

				// TODO: implement Inverse() for aiMatrix3x3
				aiMatrix3x3 m = aiMatrix3x3(mWorldIT);
				
				// copy normals, transform them to worldspace
				for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
				{
					pcMeshOut->mNormals[aiCurrent[AI_PTVS_VERTEX]+n] = 
						m * pcMesh->mNormals[n];
				}
			}
			if (iVFormat & 0x4)
			{
				// copy tangents
				memcpy(pcMeshOut->mTangents + aiCurrent[AI_PTVS_VERTEX],
					pcMesh->mTangents,
					pcMesh->mNumVertices * sizeof(aiVector3D));
				// copy bitangents
				memcpy(pcMeshOut->mBitangents + aiCurrent[AI_PTVS_VERTEX],
					pcMesh->mBitangents,
					pcMesh->mNumVertices * sizeof(aiVector3D));
			}
			unsigned int p = 0;
			while (iVFormat & (0x100 << p))
			{
				// copy texture coordinates
				memcpy(pcMeshOut->mTextureCoords[p] + aiCurrent[AI_PTVS_VERTEX],
					pcMesh->mTextureCoords[p],
					pcMesh->mNumVertices * sizeof(aiVector3D));
				++p;
			}
			p = 0;
			while (iVFormat & (0x1000000 << p))
			{
				// copy vertex colors
				memcpy(pcMeshOut->mColors[p] + aiCurrent[AI_PTVS_VERTEX],
					pcMesh->mColors[p],
					pcMesh->mNumVertices * sizeof(aiColor4D));
				++p;
			}
			// now we need to copy all faces
			// since we will delete the source mesh afterwards,
			// we don't need to reallocate the array of indices
			for (unsigned int planck = 0;planck<pcMesh->mNumFaces;++planck)
			{
				pcMeshOut->mFaces[aiCurrent[AI_PTVS_FACE]+planck].mNumIndices =
					pcMesh->mFaces[planck].mNumIndices; 

				unsigned int* pi = pcMeshOut->mFaces[aiCurrent[AI_PTVS_FACE]+planck].
					mIndices = pcMesh->mFaces[planck].mIndices; 

				// offset all vrtex indices
				for (unsigned int hahn = 0; hahn < pcMesh->mFaces[planck].mNumIndices;++hahn)
				{
					pi[hahn] += aiCurrent[AI_PTVS_VERTEX];
				}

				// just make sure the array won't be deleted by the
				// aiFace destructor ...
				pcMesh->mFaces[planck].mIndices = NULL;

				// FIX: update the mPrimitiveTypes member of the mesh
				switch (pcMesh->mFaces[planck].mNumIndices)
				{
				case 0x1:
					pcMeshOut->mPrimitiveTypes |= aiPrimitiveType_POINT;
					break;
				case 0x2:
					pcMeshOut->mPrimitiveTypes |= aiPrimitiveType_LINE;
					break;
				case 0x3:
					pcMeshOut->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
					break;
				default:
					pcMeshOut->mPrimitiveTypes |= aiPrimitiveType_POLYGON;
					break;
				};
			}
			aiCurrent[AI_PTVS_VERTEX] += pcMesh->mNumVertices;
			aiCurrent[AI_PTVS_FACE]   += pcMesh->mNumFaces;
		}
	}
	for (unsigned int i = 0;i < pcNode->mNumChildren;++i)
	{
		CollectData(pcScene,pcNode->mChildren[i],iMat,
			iVFormat,pcMeshOut,aiCurrent);
	}
}

// ------------------------------------------------------------------------------------------------
// Get a list of all vertex formats that occur for a given material index
// The output list contains duplicate elements
void GetVFormatList( aiScene* pcScene, unsigned int iMat,
	std::list<unsigned int>& aiOut)
{
	for (unsigned int i = 0; i < pcScene->mNumMeshes;++i)
	{
		aiMesh* pcMesh = pcScene->mMeshes[ i ]; 
		if (iMat == pcMesh->mMaterialIndex)
		{
			aiOut.push_back(GetMeshVFormat(pcMesh));
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Compute the absolute transformation matrices of each node
void ComputeAbsoluteTransform( aiNode* pcNode )
{
	if (pcNode->mParent)
	{
		pcNode->mTransformation = pcNode->mParent->mTransformation*pcNode->mTransformation;
	}

	for (unsigned int i = 0;i < pcNode->mNumChildren;++i)
	{
		ComputeAbsoluteTransform(pcNode->mChildren[i]);
	}
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void PretransformVertices::Execute( aiScene* pScene)
{
	DefaultLogger::get()->debug("PretransformVerticesProcess begin");

	const unsigned int iOldMeshes = pScene->mNumMeshes;
	const unsigned int iOldAnimationChannels = pScene->mNumAnimations;
	const unsigned int iOldNodes = CountNodes(pScene->mRootNode);

	// first compute absolute transformation matrices for all nodes
	ComputeAbsoluteTransform(pScene->mRootNode);

	// delete aiMesh::mBones for all meshes. The bones are
	// removed during this step and we need the pointer as
	// temporary storage
	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
	{
		aiMesh* mesh = pScene->mMeshes[i];

		for (unsigned int a = 0; a < mesh->mNumBones;++a)
			delete mesh->mBones[a];

		delete[] mesh->mBones;
		mesh->mBones = NULL;
	}

	// now build a list of output meshes
	std::vector<aiMesh*> apcOutMeshes;
	apcOutMeshes.reserve(pScene->mNumMaterials<<1u);
	std::list<unsigned int> aiVFormats;
	for (unsigned int i = 0; i < pScene->mNumMaterials;++i)
	{
		// get the list of all vertex formats for this material
		aiVFormats.clear();
		GetVFormatList(pScene,i,aiVFormats);
		aiVFormats.sort();
		aiVFormats.unique();
		for (std::list<unsigned int>::const_iterator
			j =  aiVFormats.begin();
			j != aiVFormats.end();++j)
		{
			unsigned int iVertices = 0;
			unsigned int iFaces = 0; 
			CountVerticesAndFaces(pScene,pScene->mRootNode,i,*j,&iFaces,&iVertices);
			if (iFaces && iVertices)
			{
				apcOutMeshes.push_back(new aiMesh());
				aiMesh* pcMesh = apcOutMeshes.back();
				pcMesh->mNumFaces = iFaces;
				pcMesh->mNumVertices = iVertices;
				pcMesh->mFaces = new aiFace[iFaces];
				pcMesh->mVertices = new aiVector3D[iVertices];
				pcMesh->mMaterialIndex = i;
				if ((*j) & 0x2)pcMesh->mNormals = new aiVector3D[iVertices];
				if ((*j) & 0x4)
				{
					pcMesh->mTangents    = new aiVector3D[iVertices];
					pcMesh->mBitangents  = new aiVector3D[iVertices];
				}
				iFaces = 0;
				while ((*j) & (0x100 << iFaces))
				{
					pcMesh->mTextureCoords[iFaces] = new aiVector3D[iVertices];
					if ((*j) & (0x10000 << iFaces))pcMesh->mNumUVComponents[iFaces] = 3;
					else pcMesh->mNumUVComponents[iFaces] = 2;
					iFaces++;
				}
				iFaces = 0;
				while ((*j) & (0x1000000 << iFaces))
					pcMesh->mColors[iFaces++] = new aiColor4D[iVertices];

				// fill the mesh ...
				unsigned int aiTemp[2] = {0,0};
				CollectData(pScene,pScene->mRootNode,i,*j,pcMesh,aiTemp);
			}
		}
	}

	// remove all animations from the scene
	for (unsigned int i = 0; i < pScene->mNumAnimations;++i)
		delete pScene->mAnimations[i];
	delete[] pScene->mAnimations;

	pScene->mAnimations    = NULL;
	pScene->mNumAnimations = 0;

	// now delete all meshes in the scene and build a new mesh list
	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
	{
		pScene->mMeshes[i]->mBones = NULL;
		delete pScene->mMeshes[i];

		// invalidate the contents of the old mesh array. We will most
		// likely have less output meshes now, so the last entries of 
		// the mesh array are not overridden. We set them to NULL to 
		// make sure the developer gets notified when his application
		// attempts to access these fields ...
		pScene->mMeshes[i] = NULL;
	}

	// If no meshes are referenced in the node graph it is
	// possible that we get no output meshes. However, this 
	// is OK if we had no input meshes, too
	if (apcOutMeshes.empty())
	{
		if (pScene->mNumMeshes)
		{
			throw new ImportErrorException("No output meshes: all meshes are orphaned "
				"and have no node references");
		}
	}
	else
	{
		// It is impossible that we have more output meshes than 
		// input meshes, so we can easily reuse the old mesh array
		pScene->mNumMeshes = (unsigned int)apcOutMeshes.size();
		for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
			pScene->mMeshes[i] = apcOutMeshes[i];
	}

	// --- we need to keep all cameras and lights 
	for (unsigned int i = 0; i < pScene->mNumCameras;++i)
	{
		aiCamera* cam = pScene->mCameras[i];
		const aiNode* nd = pScene->mRootNode->FindNode(cam->mName);
		ai_assert(NULL != nd);

		// multiply all properties of the camera with the absolute
		// transformation of the corresponding node
		cam->mPosition = nd->mTransformation * cam->mPosition;
		cam->mLookAt   = aiMatrix3x3( nd->mTransformation ) * cam->mLookAt;
		cam->mUp       = aiMatrix3x3( nd->mTransformation ) * cam->mUp;
	}

	for (unsigned int i = 0; i < pScene->mNumLights;++i)
	{
		aiLight* l = pScene->mLights[i];
		const aiNode* nd = pScene->mRootNode->FindNode(l->mName);
		ai_assert(NULL != nd);

		// multiply all properties of the camera with the absolute
		// transformation of the corresponding node
		l->mPosition   = nd->mTransformation * l->mPosition;
		l->mDirection  = aiMatrix3x3( nd->mTransformation ) * l->mDirection;
	}

	// now delete all nodes in the scene and build a new
	// flat node graph with a root node and some level 1 children
	delete pScene->mRootNode;
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mName.Set("<dummy_root>");

	if (1 == pScene->mNumMeshes && !pScene->mNumLights && !pScene->mNumCameras)
	{
		pScene->mRootNode->mNumMeshes = 1;
		pScene->mRootNode->mMeshes = new unsigned int[1];
		pScene->mRootNode->mMeshes[0] = 0;
	}
	else
	{
		pScene->mRootNode->mNumChildren = pScene->mNumMeshes+pScene->mNumLights+pScene->mNumCameras;
		aiNode** nodes = pScene->mRootNode->mChildren = new aiNode*[pScene->mRootNode->mNumChildren];

		// generate mesh nodes
		for (unsigned int i = 0; i < pScene->mNumMeshes;++i,++nodes)
		{
			aiNode* pcNode = *nodes = new aiNode();
			pcNode->mParent = pScene->mRootNode;
			pcNode->mName.length = ::sprintf(pcNode->mName.data,"mesh_%i",i);

			// setup mesh indices
			pcNode->mNumMeshes = 1;
			pcNode->mMeshes = new unsigned int[1];
			pcNode->mMeshes[0] = i;
		}
		// generate light nodes
		for (unsigned int i = 0; i < pScene->mNumLights;++i,++nodes)
		{
			aiNode* pcNode = *nodes = new aiNode();
			pcNode->mParent = pScene->mRootNode;
			pcNode->mName.length = ::sprintf(pcNode->mName.data,"light_%i",i);
			pScene->mLights[i]->mName = pcNode->mName;
		}
		// generate camera nodes
		for (unsigned int i = 0; i < pScene->mNumCameras;++i,++nodes)
		{
			aiNode* pcNode = *nodes = new aiNode();
			pcNode->mParent = pScene->mRootNode;
			pcNode->mName.length = ::sprintf(pcNode->mName.data,"cam_%i",i);
			pScene->mCameras[i]->mName = pcNode->mName;
		}
	}

	// print statistics
	if (!DefaultLogger::isNullLogger())
	{
		char buffer[4096];

		DefaultLogger::get()->debug("PretransformVerticesProcess finished");

		::sprintf(buffer,"Removed %i nodes and %i animation channels (%i output nodes)",
			iOldNodes,iOldAnimationChannels,CountNodes(pScene->mRootNode));
		DefaultLogger::get()->info(buffer);

		::sprintf(buffer,"Kept %i lights and %i cameras",
			pScene->mNumLights,pScene->mNumCameras);
		DefaultLogger::get()->info(buffer);

		::sprintf(buffer,"Moved %i meshes to WCS (number of output meshes: %i)",
			iOldMeshes,pScene->mNumMeshes);
		DefaultLogger::get()->info(buffer);
	}

	return;
}

