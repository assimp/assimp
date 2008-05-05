/** @file Implementation of the SplitLargeMeshes postprocessing step
*/
#include "SplitLargeMeshes.h"
#include "../include/aiPostProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"

using namespace Assimp;

// Constructor to be privately used by Importer
SplitLargeMeshesProcess::SplitLargeMeshesProcess()
	{
	}

// Destructor, private as well
SplitLargeMeshesProcess::~SplitLargeMeshesProcess()
	{
	// nothing to do here
	}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool SplitLargeMeshesProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_SplitLargeMeshes) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void SplitLargeMeshesProcess::Execute( aiScene* pScene)
{
	std::vector<std::pair<aiMesh*, unsigned int> > avList;

	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
		this->SplitMesh(a, pScene->mMeshes[a],avList);

	if (avList.size() != pScene->mNumMeshes)
	{
		// it seems something has been splitted. rebuild the mesh list
		delete[] pScene->mMeshes;
		pScene->mNumMeshes = avList.size();
		pScene->mMeshes = new aiMesh*[avList.size()];

		for (unsigned int i = 0; i < avList.size();++i)
			pScene->mMeshes[i] = avList[i].first;

		// now we need to update all nodes
		this->UpdateNode(pScene->mRootNode,avList);
	}
	return;
}
// ------------------------------------------------------------------------------------------------
// Update a node after some meshes have been split
void SplitLargeMeshesProcess::UpdateNode(aiNode* pcNode,
	const std::vector<std::pair<aiMesh*, unsigned int> >& avList)
{
	// for every index in out list build a new entry
	// TODO: Currently O(n^2)

	std::vector<unsigned int> aiEntries;
	aiEntries.reserve(pcNode->mNumMeshes + 1);
	for (unsigned int i = 0; i < pcNode->mNumMeshes;++i)
	{
		for (unsigned int a = 0; a < avList.size();++a)
		{
			if (avList[a].second == pcNode->mMeshes[i])
			{
				aiEntries.push_back(a);
			}
		}
	}

	// now build the new list
	delete pcNode->mMeshes;
	pcNode->mNumMeshes = aiEntries.size();
	pcNode->mMeshes = new unsigned int[pcNode->mNumMeshes];

	for (unsigned int b = 0; b < pcNode->mNumMeshes;++b)
		pcNode->mMeshes[b] = aiEntries[b];

	// recusively update all other nodes
	for (unsigned int i = 0; i < pcNode->mNumChildren;++i)
	{
		this->UpdateNode ( pcNode->mChildren[i], avList );
	}
	return;
}
// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void SplitLargeMeshesProcess::SplitMesh(
	unsigned int a,
	aiMesh* pMesh,
	std::vector<std::pair<aiMesh*, unsigned int> >& avList)
{
	// TODO: Mesh splitting is currently not supported for meshes
	// containing bones

	if (pMesh->mNumVertices > AI_SLM_MAX_VERTICES && 0 == pMesh->mNumBones)
	{
		// we need to split this mesh into sub meshes
		// determine the size of a submesh
		const unsigned int iSubMeshes = (pMesh->mNumVertices / AI_SLM_MAX_VERTICES) + 1;

		const unsigned int iOutFaceNum = pMesh->mNumFaces / iSubMeshes;
		const unsigned int iOutVertexNum = iOutFaceNum * 3;

		// now generate all submeshes
		for (unsigned int i = 0; i < iSubMeshes;++i)
		{
			aiMesh* pcMesh			= new aiMesh;			
			pcMesh->mNumFaces		= iOutFaceNum;
			pcMesh->mMaterialIndex	= pMesh->mMaterialIndex;

			if (i == iSubMeshes-1)
			{
				pcMesh->mNumFaces = iOutFaceNum + (
					pMesh->mNumFaces - iOutFaceNum * iSubMeshes);
			}
			// copy the list of faces
			pcMesh->mFaces = new aiFace[pcMesh->mNumFaces];

			const unsigned int iBase = iOutFaceNum * i;

			// get the total number of indices
			unsigned int iCnt = 0;
			for (unsigned int p = iBase; p < pcMesh->mNumFaces + iBase;++p)
			{
				iCnt += pMesh->mFaces[p].mNumIndices;
			}
			pcMesh->mNumVertices = iCnt;

			// allocate storage
			if (pMesh->mVertices != NULL)
				pcMesh->mVertices = new aiVector3D[iCnt];

			if (pMesh->HasNormals())
				pcMesh->mNormals = new aiVector3D[iCnt];

			if (pMesh->HasTangentsAndBitangents())
			{
				pcMesh->mTangents = new aiVector3D[iCnt];
				pcMesh->mBitangents = new aiVector3D[iCnt];
			}

			// texture coordinates
			for (unsigned int c = 0;  c < AI_MAX_NUMBER_OF_TEXTURECOORDS;++c)
			{
				pcMesh->mNumUVComponents[c] = pMesh->mNumUVComponents[c];
				if (pMesh->HasTextureCoords( c))
				{
					pcMesh->mTextureCoords[c] = new aiVector3D[iCnt];
				}
			}

			// vertex colors
			for (unsigned int c = 0;  c < AI_MAX_NUMBER_OF_COLOR_SETS;++c)
			{
				if (pMesh->HasVertexColors( c))
				{
					pcMesh->mColors[c] = new aiColor4D[iCnt];
				}
			}

			// (we will also need to copy the array of indices)
			for (unsigned int p = 0; p < pcMesh->mNumFaces;++p)
			{
				pcMesh->mFaces[p].mNumIndices = 3;

				// allocate a new array
				unsigned int* pi = pMesh->mFaces[p + iBase].mIndices;
				pcMesh->mFaces[p].mIndices = new unsigned int[3];

				// and copy the contents of the old array, offset by current base
				for (unsigned int v = 0; v < 3;++v)
				{
					unsigned int iIndex = pMesh->mFaces[p+iBase].mIndices[v];
					unsigned int iIndexOut = p*3 + v;
					pcMesh->mFaces[p].mIndices[v] = iIndexOut;

					// copy positions
					if (pMesh->mVertices != NULL)
					{
						pcMesh->mVertices[iIndexOut] = pMesh->mVertices[iIndex];
					}

					// copy normals
					if (pMesh->HasNormals())
					{
						pcMesh->mNormals[iIndexOut] = pMesh->mNormals[iIndex];
					}

					// copy tangents/bitangents
					if (pMesh->HasTangentsAndBitangents())
					{
						pcMesh->mTangents[iIndexOut] = pMesh->mTangents[iIndex];
						pcMesh->mBitangents[iIndexOut] = pMesh->mBitangents[iIndex];
					}

					// texture coordinates
					for (unsigned int c = 0;  c < AI_MAX_NUMBER_OF_TEXTURECOORDS;++c)
					{
						if (pMesh->HasTextureCoords( c))
						{
							pcMesh->mTextureCoords[c][iIndexOut] = pMesh->mTextureCoords[c][iIndex];
						}
					}
					// vertex colors 
					for (unsigned int c = 0;  c < AI_MAX_NUMBER_OF_COLOR_SETS;++c)
					{
						if (pMesh->HasVertexColors( c))
						{
							pcMesh->mColors[c][iIndexOut] = pMesh->mColors[c][iIndex];
						}
					}
				}
			}

			// add the newly created mesh to the list
			avList.push_back(std::pair<aiMesh*, unsigned int>(pcMesh,a));
		}

		// now delete the old mesh data
		delete pMesh;
	}
	else avList.push_back(std::pair<aiMesh*, unsigned int>(pMesh,a));
	return;
}