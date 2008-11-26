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

/** Implementation of the OptimizeGraphProcess post-processing step*/

#include "AssimpPCH.h"

#include "OptimizeGraphProcess.h"
#include "Hash.h"


using namespace Assimp;

// MSB for type unsigned int
#define AI_OG_UINT_MSB (1u<<((sizeof(unsigned int)*8u)-1u))
#define AI_OG_UINT_MSB_2 (AI_OG_UINT_MSB>>1)

// check whether a node/a mesh is locked
#define AI_OG_IS_NODE_LOCKED(nd) (nd->mNumChildren & AI_OG_UINT_MSB)
#define AI_OG_IS_MESH_LOCKED(ms) (ms->mNumBones & AI_OG_UINT_MSB)

// check whether a node has locked meshes in its list
#define AI_OG_HAS_NODE_LOCKED_MESHES(nd) (nd->mNumChildren & AI_OG_UINT_MSB_2)

// unmask the two upper bits of an unsigned int
#define AI_OG_UNMASK(p) (p & (~(AI_OG_UINT_MSB|AI_OG_UINT_MSB_2)))

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
OptimizeGraphProcess::OptimizeGraphProcess()
{
	configMinNumFaces = AI_OG_MIN_NUM_FACES;
	configJoinInequalTransforms = AI_OG_JOIN_INEQUAL_TRANSFORMS;
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
OptimizeGraphProcess::~OptimizeGraphProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool OptimizeGraphProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_OptimizeGraph) != 0;
}

// ------------------------------------------------------------------------------------------------
// Setup properties of the step
void OptimizeGraphProcess::SetupProperties(const Importer* pImp)
{
	// join nods with inequal transformations?
	configJoinInequalTransforms = pImp->GetPropertyInteger(AI_CONFIG_PP_OG_JOIN_INEQUAL_TRANSFORMS,
		AI_OG_JOIN_INEQUAL_TRANSFORMS) != 0 ? true : false;

	// minimum face number per node
	configMinNumFaces = pImp->GetPropertyInteger(AI_CONFIG_PP_OG_MIN_NUM_FACES,
		AI_OG_MIN_NUM_FACES);
}

// ------------------------------------------------------------------------------------------------
void OptimizeGraphProcess::FindLockedNodes(aiNode* node)
{
	ai_assert(NULL != node);

	// process animations
	for (unsigned int i = 0; i < pScene->mNumAnimations;++i)
	{
		aiAnimation* pani = pScene->mAnimations[i];
		for (unsigned int a = 0; a < pani->mNumChannels;++a)
		{
			aiNodeAnim* pba = pani->mChannels[a];
			if (pba->mNodeName == node->mName)
			{
				// this node is locked, it is referenced by an animation channel
				node->mNumChildren |= AI_OG_UINT_MSB;
			}
		}
	}

	// process cameras
	for (unsigned int i = 0; i < pScene->mNumCameras;++i)
	{
		aiCamera* p = pScene->mCameras[i];
		if (p->mName == node->mName)
		{
			// this node is locked, it is referenced by a camera
			node->mNumChildren |= AI_OG_UINT_MSB;
		}
	}

	// process lights
	for (unsigned int i = 0; i < pScene->mNumLights;++i)
	{
		aiLight* p = pScene->mLights[i];
		if (p->mName == node->mName)
		{
			// this node is locked, it is referenced by a light
			node->mNumChildren |= AI_OG_UINT_MSB;
		}
	}

	// call all children
	for (unsigned int i = 0; i < node->mNumChildren;++i)
		FindLockedNodes(node->mChildren[i]);
}

// ------------------------------------------------------------------------------------------------
void OptimizeGraphProcess::FindLockedMeshes(aiNode* node, MeshRefCount* pRefCount)
{
	ai_assert(NULL != node && NULL != pRefCount);
	for (unsigned int i = 0;i < node->mNumMeshes;++i)
	{
		unsigned int m = node->mMeshes[i];
		if (pRefCount[m].first)
		{
			// we have already one reference - lock the first node
			// that had a referenced to this mesh too if it has only
			// one mesh assigned. If there are multiple meshes,
			// the others could still be used for optimizations.
			if (pRefCount[m].second)
			{
				pRefCount[m].second->mNumChildren |= (pRefCount[m].second->mNumMeshes <= 1 
					? AI_OG_UINT_MSB : AI_OG_UINT_MSB_2);

				pRefCount[m].second = NULL;
			}
			pScene->mMeshes[m]->mNumBones |= AI_OG_UINT_MSB;

			// lock this node
			node->mNumChildren |= (node->mNumMeshes <= 1 
				? AI_OG_UINT_MSB : AI_OG_UINT_MSB_2);
		}
		else pRefCount[m].second = node;
		++pRefCount[m].first;
	}
	// call all children
	for (unsigned int i = 0; i < node->mNumChildren;++i)
		FindLockedMeshes(node->mChildren[i],pRefCount);
}

// ------------------------------------------------------------------------------------------------
void OptimizeGraphProcess::FindLockedMeshes(aiNode* node)
{
	ai_assert(NULL != node);
	MeshRefCount* pRefCount = new MeshRefCount[pScene->mNumMeshes];
	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		pRefCount[i] = MeshRefCount();

	// execute the algorithm
	FindLockedMeshes(node,pRefCount);

	delete[] pRefCount;
}

// ------------------------------------------------------------------------------------------------
void OptimizeGraphProcess::UnlockNodes(aiNode* node)
{
	ai_assert(NULL != node);
	node->mNumChildren &= ~(AI_OG_UINT_MSB|AI_OG_UINT_MSB_2);

	// call all children
	for (unsigned int i = 0; i < node->mNumChildren;++i)
		UnlockNodes(node->mChildren[i]);
}

// ------------------------------------------------------------------------------------------------
void OptimizeGraphProcess::UnlockMeshes()
{
	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
		pScene->mMeshes[i]->mNumBones &= ~AI_OG_UINT_MSB;
}

// ------------------------------------------------------------------------------------------------
void OptimizeGraphProcess::ComputeMeshHashes()
{
	mMeshHashes.resize(pScene->mNumMeshes);
	for (unsigned int i = 0; i < pScene->mNumMeshes;++i)
	{
		unsigned int iRet = 0;
		aiMesh* pcMesh = pScene->mMeshes[i];

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

			// NOTE: meshes with numUVComp != 3 && != 2 aren't handled correctly here
			ai_assert(pcMesh->mNumUVComponents[p] == 3 || pcMesh->mNumUVComponents[p] == 2);
			if (3 == pcMesh->mNumUVComponents[p])
				iRet |= (0x1000 << p++);
		}
		// vertex colors
		p = 0;
		ai_assert(4 >= AI_MAX_NUMBER_OF_COLOR_SETS);
		while (pcMesh->HasVertexColors(p))iRet |= (0x10000 << p++);
		mMeshHashes[i] = iRet;

		// material index -store it in the upper 1 1/2 bytes, so
		// are able to encode 2^12 material indices.

		iRet |= (pcMesh->mMaterialIndex << 20u);
	}
}

// ------------------------------------------------------------------------------------------------
inline unsigned int OptimizeGraphProcess::BinarySearch(NodeIndexList& sortedArray, 
	unsigned int min, unsigned int& index, unsigned int iStart)
{
	unsigned int first = iStart,last = (unsigned int)sortedArray.size()-1;
	while (first <= last) 
	{
		unsigned int mid = (first + last) / 2;  
		unsigned int id = sortedArray[mid].second;

		if (min > id) 
			first = mid + 1;  
		else if (min <= id) 
		{
			last = mid - 1;
			if (!mid || min > sortedArray[last].second)
			{
				index = sortedArray[last].first;
				return mid;
			}
		}
	}
	return (unsigned int)sortedArray.size();
}


// ------------------------------------------------------------------------------------------------
void OptimizeGraphProcess::ApplyNodeMeshesOptimization(aiNode* pNode)
{
	ai_assert(NULL != pNode);

	// find all meshes which are compatible and could therefore be joined.
	// we can't join meshes that are locked 
	std::vector<aiMesh*> apcMeshes(pNode->mNumMeshes);
	unsigned int iNumMeshes;

	for (unsigned int m = 0, ttt = 0; m < pNode->mNumMeshes;++m)
	{
		iNumMeshes = 0;

		unsigned int nm = pNode->mMeshes[m];
		if (0xffffffff == nm || AI_OG_IS_MESH_LOCKED(pScene->mMeshes[nm]))continue;

		for (unsigned int q = m+1; q < pNode->mNumMeshes;++q)
		{
			register unsigned int nq = pNode->mMeshes[q];

			// skip locked meshes
			if (AI_OG_IS_MESH_LOCKED(pScene->mMeshes[nq]))continue;

			// compare the mesh hashes
			if (mMeshHashes[nm] == mMeshHashes[nq])
			{
				apcMeshes[iNumMeshes++] = pScene->mMeshes[nq];
				pNode->mMeshes[q] = 0xffffffff;
			}
		}
		aiMesh* out;
		if (iNumMeshes > 0)
		{
			apcMeshes[iNumMeshes++] = pScene->mMeshes[nm];
//			JoinMeshes(apcMeshes,out,iNumMeshes);
		}
		else out = pScene->mMeshes[nm];

		pNode->mMeshes[ttt++] = (unsigned int)mOutputMeshes.size();
		mOutputMeshes.push_back(out);
	}
}

// ------------------------------------------------------------------------------------------------
void OptimizeGraphProcess::TransformMeshes(aiNode* quak,aiNode* pNode)
{
	for (unsigned int pl = 0; pl < quak->mNumMeshes;++pl)
	{
		aiMesh* mariusIsHot = pScene->mMeshes[quak->mMeshes[pl]];
		aiMatrix4x4 mMatTransform = pNode->mTransformation;

		// transformation: first back to the parent's local space,
		// later into the local space of the destination child node
		mMatTransform.Inverse();
		mMatTransform = quak->mTransformation * mMatTransform;

		// transform all vertices
		for (unsigned int oo =0; oo < mariusIsHot->mNumVertices;++oo)
			mariusIsHot->mVertices[oo] = mMatTransform * mariusIsHot->mVertices[oo];

		// transform all normal vectors 
		if (mariusIsHot->HasNormals())
		{
			mMatTransform.Inverse().Transpose();
			for (unsigned int oo =0; oo < mariusIsHot->mNumVertices;++oo)
				mariusIsHot->mNormals[oo] = mMatTransform * mariusIsHot->mNormals[oo];
		}
	}
}

// ------------------------------------------------------------------------------------------------
void OptimizeGraphProcess::ApplyOptimizations(aiNode* node)
{
	ai_assert(NULL != node);

	unsigned int iJoinedIndex = 0;

	// first: node index; second: number of faces in node
	NodeIndexList aiBelowTreshold;
	aiBelowTreshold.reserve(node->mNumChildren);
	
	for (unsigned int i = 0; i < node->mNumChildren;++i)
	{
		aiNode* pChild = node->mChildren[i];
		if (AI_OG_IS_NODE_LOCKED(pChild) || !pChild->mNumMeshes)continue;

		// find out how many faces this node is referencing
		unsigned int iFaceCnt = 0;
		for (unsigned int a = 0; a < pChild->mNumMeshes;++a)
			iFaceCnt += pScene->mMeshes[pChild->mMeshes[a]]->mNumFaces;
		
		// are we below the treshold?
		if (iFaceCnt < configMinNumFaces)
		{
			aiBelowTreshold.push_back(NodeIndexEntry());
			NodeIndexEntry& p = aiBelowTreshold.back();
			p.first = i;
			p.second = iFaceCnt;
			p.pNode = pChild;
		}
	}

	if (!aiBelowTreshold.empty())
	{
		// some typedefs for the data structures we'll need
		typedef std::pair<unsigned int, unsigned int> JoinListEntry;
		std::vector<JoinListEntry> aiJoinList(aiBelowTreshold.size());
		std::vector<unsigned int>  aiTempList(aiBelowTreshold.size());

		unsigned int iNumJoins, iNumTemp;

		// sort the list by size
		std::sort(aiBelowTreshold.begin(),aiBelowTreshold.end());

		unsigned int iStart = 0;
		for (NodeIndexList::const_iterator it = aiBelowTreshold.begin(),end = aiBelowTreshold.end();
			it != end; /*++it */++iStart)
		{
			aiNode* pNode = node->mChildren[(*it).first];

			// get the hash of the mesh
			const unsigned int iMeshVFormat = mMeshHashes[pNode->mMeshes[0]];

			// we search for a node with more faces than this ... find
			// the one that fits best and continue until we've reached 
			// treshold size. 
			int iDiff = configMinNumFaces-(*it).second;
			for (;;)
			{
				// do a binary search and start the iteration there
				unsigned int index;
				unsigned int start = BinarySearch(aiBelowTreshold,iDiff,index,iStart);

				if (index == (*it).first)start++;

				if (start >= aiBelowTreshold.size())
				{
					// there is no node with enough faces. take the first
					start = 0;
				}

				// todo: implement algorithm to find the best possible combination ...
				iNumTemp = 0;

				while( start < aiBelowTreshold.size())
				{
					// check whether the node has akready been processed before
					const NodeIndexEntry& entry = aiBelowTreshold[start];
					if (!entry.pNode)continue;

					const aiNode* pip = node->mChildren[entry.first];
					if (configJoinInequalTransforms )
					{
						// we need to check whether this node has locked meshes
						// in this case we can't add it here - the meshes will
						// be transformed from one to another coordinate space

						if (!AI_OG_HAS_NODE_LOCKED_MESHES(pip) || pip->mTransformation == pNode->mTransformation)
							aiTempList[iNumTemp++] = start;
					}
					else if (node->mChildren[entry.first]->mTransformation == pNode->mTransformation)
					{
						aiTempList[iNumTemp++] = start;
						break;
					}
					++start;
				}

				if (iNumTemp)
				{
					// search for a node which has a mesh with
					//  - the same material index
					//  - the same vertex layout
					unsigned int d = iNumJoins = 0; 
					for (unsigned int m = 0; m < iNumTemp;++m)
					{
						register unsigned int mn = aiTempList[m];
						aiNode* pip = aiBelowTreshold[mn].pNode;

						for (unsigned int tt = 0; tt < pip->mNumMeshes;++tt)
						{
							register unsigned int mm = pip->mMeshes[tt];

							if (mMeshHashes  [ mm ] == iMeshVFormat)
							{
								d = mn;
								goto break_out;
							}
						}
					}
break_out:
					aiJoinList[iNumJoins++] = JoinListEntry( aiBelowTreshold[d].first, d );
					iDiff -= aiBelowTreshold[d].second;
				}
				// did we reach the target treshold?
				if (iDiff <= 0)break;
			}

			// did we found any nodes to be joined with *this* one?
			if (iNumJoins)
			{
				unsigned int iNumTotalChilds = pNode->mNumChildren;
				unsigned int iNumTotalMeshes = pNode->mNumMeshes;
				std::vector<JoinListEntry>::const_iterator wend = aiJoinList.begin()+iNumJoins;

				// get output array bounds
				for (std::vector<JoinListEntry>::const_iterator wit = aiJoinList.begin();
					wit != wend;++wit )
				{
					aiNode*& quak = node->mChildren[(*wit).first];
					iNumTotalChilds += AI_OG_UNMASK( quak->mNumChildren );
					iNumTotalMeshes += quak->mNumMeshes;
				}

				// build the output child list
				if (iNumTotalChilds != pNode->mNumChildren)
				{
					aiNode** ppc = pNode->mChildren;
					delete[] pNode->mChildren;
					pNode->mChildren = new aiNode*[iNumTotalChilds];
					::memcpy(pNode->mChildren,ppc, sizeof(void*)* AI_OG_UNMASK( pNode->mNumChildren ));

					for (std::vector<JoinListEntry>::const_iterator wit = aiJoinList.begin();
						wit != wend;++wit )
					{
						aiNode*& quak = node->mChildren[(*wit).first];
						::memcpy(pNode->mChildren+pNode->mNumChildren,
							quak->mChildren, sizeof(void*)*quak->mNumChildren);

						 pNode->mNumChildren += AI_OG_UNMASK( quak->mNumChildren );
					}
				}

				// build the output mesh list
				unsigned int* ppc = pNode->mMeshes;
				delete[] pNode->mMeshes;
				pNode->mMeshes = new unsigned int[iNumTotalMeshes];
				::memcpy(pNode->mMeshes,ppc, sizeof(void*)*pNode->mNumMeshes);

				for (std::vector<JoinListEntry>::const_iterator wit = aiJoinList.begin();
					wit != wend;++wit )
				{
					aiNode*& quak = node->mChildren[(*wit).first];
					::memcpy(pNode->mMeshes+pNode->mNumMeshes,
						quak->mMeshes, sizeof(unsigned int)*quak->mNumMeshes);

					// if the node has a transformation matrix that is not equal to ours,
					// we'll need to transform all vertices of the mesh into our
					// local coordinate space.
					if (configJoinInequalTransforms && quak->mTransformation != pNode->mTransformation)
						TransformMeshes(quak,pNode);

					pNode->mNumMeshes += quak->mNumMeshes;

					// remove the joined nodes from all lists.
					aiBelowTreshold[(*wit).second].pNode = NULL;
					if ((*wit).second == iStart+1)++iStart;
				}

				// now generate an output name for the joined nodes
				if (1 == iNumTotalChilds)
				{
					pNode->mName.length = ::sprintf( pNode->mName.data, "<Joined_%i_%i>",
						iJoinedIndex++,iNumJoins+1);
				}
			}

			// now optimize the meshes in this node
			ApplyNodeMeshesOptimization(pNode);

			// note - this has been optimized away. The search in the binary
			// list starts with iStart, which is incremented each iteration
			++it; // = aiBelowTreshold.erase(it);
		}
	}

	// call all children recursively
	for (unsigned int i = 0; i < node->mNumChildren;++i)
		ApplyOptimizations(node->mChildren[i]);
}

// ------------------------------------------------------------------------------------------------
void OptimizeGraphProcess::BuildOutputMeshList()
{
	// all meshes should have been deleted before if they are
	// not contained in the new mesh list

	if (pScene->mNumMeshes < mOutputMeshes.size())
	{
		delete[] pScene->mMeshes; 
		pScene->mMeshes = new aiMesh*[mOutputMeshes.size()];
	}
	pScene->mNumMeshes = (unsigned int)mOutputMeshes.size();
	::memcpy(pScene->mMeshes,&mOutputMeshes[0],pScene->mNumMeshes*sizeof(void*));
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void OptimizeGraphProcess::Execute( aiScene* pScene)
{
	throw new ImportErrorException("This step is disabled in this beta");
	this->pScene = pScene;
	/*

	a) the term "mesh node" stands for a node with numMeshes > 0
	b) the term "animation node" stands for a node with numMeshes == 0,
	   regardless whether the node is referenced by animation channels,
	   lights or cameras

	Algorithm:

	1. Compute hashes for all meshes that we're able to check whether
	   two meshes are compatible.
	3. Find out which nodes may not be moved, so to speak are "locked" - a
	   locked node will never be joined with neighbors.
	   - A node lock is indicated by a set MSB in the aiNode::mNumChildren member
    4. Find out which meshes are locked - they are referenced by
	   more than one node. They will never be joined. Mark all 
	   nodes referencing such a mesh as "locked", too.
       - A mesh lock is indicated by a set MSB in the aiMesh::mNumBones member
    5. For each unlocked node count the face numbers of all assigned meshes 
	   - if it is below the pre-defined treshold add the node to a list.
	   For each node in the list - try to find enough joinable nodes to
	   have enough faces all together. 
		  Two nodes are joined if:
		  - none of them is locked
		  - (optional) their world matrices are identical
		  - nodes whose meshes share the same material indices are prefered
		  Two meshes in one node are joined if:
		  - their material indices are identical
		  - none of them is locked
		  - they share the same vertex format
    6. Build the final mesh list
	7. For all meshes and all nodes - remove locks.

	*/

	throw new ImportErrorException("OG step is still undeer development and not yet finished");

	// STEP 1
	ComputeMeshHashes();

	// STEP 2
	FindLockedNodes(pScene->mRootNode);

	// STEP 3
	FindLockedMeshes(pScene->mRootNode);

	// STEP 4
	ApplyOptimizations(pScene->mRootNode);

	// STEP 5
	BuildOutputMeshList();

	// STEP 6
	UnlockNodes(pScene->mRootNode);
	UnlockMeshes();
}
