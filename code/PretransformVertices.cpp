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
	if (0xdeadbeef == pcMesh->mNumUVComponents[0])
		return pcMesh->mNumUVComponents[1];

	unsigned int iRet = 0;

	// normals
	if (pcMesh->HasNormals())iRet |= 0x1;
	// tangents and bitangents
	if (pcMesh->HasTangentsAndBitangents())iRet |= 0x2;

	// texture coordinates
	unsigned int p = 0;
	ai_assert(4 >= AI_MAX_NUMBER_OF_TEXTURECOORDS);
	while (pcMesh->HasTextureCoords(p))
	{
		iRet |= (0x100 << p++);
		if (3 == pcMesh->mNumUVComponents[p])
			iRet |= (0x1000 << p++);
	}
	// vertex colors
	p = 0;
	while (pcMesh->HasVertexColors(p))iRet |= (0x10000 << p++);

	// store the value for later use
	pcMesh->mNumUVComponents[0] = 0xdeadbeef;
	pcMesh->mNumUVComponents[1] = iRet;

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
	return;
}

#define AI_PTVS_VERTEX 0x0
#define AI_PTVS_FACE 0x1

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
			if (iVFormat & 0x1)
			{
				aiMatrix4x4 mWorldIT = pcNode->mTransformation;
				mWorldIT.Inverse().Transpose();

				// copy normals, transform them to worldspace
				for (unsigned int n = 0; n < pcMesh->mNumVertices;++n)
				{
					pcMeshOut->mNormals[aiCurrent[AI_PTVS_VERTEX]+n] = 
						mWorldIT * pcMesh->mNormals[n];
				}
			}
			if (iVFormat & 0x2)
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
			while (iVFormat & (0x10000 << p))
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
			}
			aiCurrent[AI_PTVS_VERTEX] += pcMesh->mNumVertices;
			aiCurrent[AI_PTVS_FACE] += pcMesh->mNumFaces;
		}
	}
	for (unsigned int i = 0;i < pcNode->mNumChildren;++i)
	{
		CollectData(pcScene,pcNode->mChildren[i],iMat,
			iVFormat,pcMeshOut,aiCurrent);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
// Get a list of all vertex formats that occur for a given material index
// The output list contains duplicate elements
void GetVFormatList( aiScene* pcScene, aiNode* pcNode, unsigned int iMat,
					std::list<unsigned int>& aiOut)
{
	for (unsigned int i = 0; i < pcNode->mNumMeshes;++i)
	{
		aiMesh* pcMesh = pcScene->mMeshes[ pcNode->mMeshes[i] ]; 
		if (iMat == pcMesh->mMaterialIndex)
		{
			aiOut.push_back(GetMeshVFormat(pcMesh));
		}
	}
	for (unsigned int i = 0;i < pcNode->mNumChildren;++i)
	{
		GetVFormatList(pcScene,pcNode->mChildren[i],iMat,aiOut);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
// Compute the absolute transformation matrices of each node
void ComputeAbsoluteTransform( aiNode* pcNode )
{
	if (pcNode->mParent)
	{
		pcNode->mTransformation =  pcNode->mTransformation*pcNode->mParent->mTransformation;
	}

	for (unsigned int i = 0;i < pcNode->mNumChildren;++i)
	{
		ComputeAbsoluteTransform(pcNode->mChildren[i]);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void PretransformVertices::Execute( aiScene* pScene)
{
	DefaultLogger::get()->debug("PretransformVerticesProcess begin");

	// first compute absolute transformation matrices for all nodes
	ComputeAbsoluteTransform(pScene->mRootNode);

	// now build a list of output meshes
	std::vector<aiMesh*> apcOutMeshes;
	apcOutMeshes.reserve(pScene->mNumMaterials*2);
	std::list<unsigned int> aiVFormats;
	for (unsigned int i = 0; i < pScene->mNumMaterials;++i)
	{
		// get the list of all vertex formats for this material
		aiVFormats.clear();
		GetVFormatList(pScene,pScene->mRootNode,i,aiVFormats);
		aiVFormats.sort(std::less<unsigned int>());
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
				if ((*j) & 0x1)pcMesh->mNormals = new aiVector3D[iVertices];
				if ((*j) & 0x2)
				{
					pcMesh->mTangents = new aiVector3D[iVertices];
					pcMesh->mBitangents = new aiVector3D[iVertices];
				}
				iFaces = 0;
				while ((*j) & (0x100 << iFaces))
				{
					pcMesh->mTextureCoords[iFaces] = new aiVector3D[iVertices];
					if ((*j) & (0x1000 << iFaces))pcMesh->mNumUVComponents[iFaces] = 3;
					else pcMesh->mNumUVComponents[iFaces] = 2;
					iFaces++;
				}
				iFaces = 0;
				while ((*j) & (0x10000 << iFaces))
					pcMesh->mColors[iFaces] = new aiColor4D[iVertices];

				// fill the mesh ...
				unsigned int aiTemp[2] = {0,0};
				CollectData(pScene,pScene->mRootNode,i,*j,pcMesh,aiTemp);
			}
		}
	}

	// remove all animations from the scene
	for (unsigned int i = 0; i < pScene->mNumAnimations;++i)
		delete pScene->mAnimations[i];
	pScene->mAnimations = NULL;
	pScene->mNumAnimations = 0;

	// now delete all meshes in the scene and build a new mesh list
	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		delete pScene->mMeshes[i];
	if (apcOutMeshes.size() != pScene->mNumMeshes)
	{
		delete[] pScene->mMeshes;
		pScene->mNumMeshes = (unsigned int)apcOutMeshes.size();
		pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
	}
	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		pScene->mMeshes[i] = apcOutMeshes[i];

	// now delete all nodes in the scene and build a new
	// flat node graph with a root node and some level 1 children
	delete pScene->mRootNode;
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mName.Set("<dummy_root>");

	if (1 == pScene->mNumMeshes)
	{
		pScene->mRootNode->mNumMeshes = 1;
		pScene->mRootNode->mMeshes = new unsigned int[1];
		pScene->mRootNode->mMeshes[0] = 0;
	}
	else
	{
		pScene->mRootNode->mNumChildren = pScene->mNumMeshes;
		pScene->mRootNode->mChildren = new aiNode*[pScene->mNumMeshes];
		for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		{
			aiNode* pcNode = pScene->mRootNode->mChildren[i] = new aiNode();
			pcNode->mName.length = sprintf(pcNode->mName.data,"dummy_%i",i);

			pcNode->mNumMeshes = 1;
			pcNode->mMeshes = new unsigned int[1];
			pcNode->mMeshes[0] = i;
			pcNode->mParent = pScene->mRootNode;
		}
	}

	DefaultLogger::get()->debug("PretransformVerticesProcess finished. All "
		"vertices are in worldspace now");
	return;
}

