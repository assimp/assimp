/** Implementation of the LimitBoneWeightsProcess post processing step */

#include <vector>
#include <assert.h>
#include "LimitBoneWeightsProcess.h"
#include "../include/aiPostProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
LimitBoneWeightsProcess::LimitBoneWeightsProcess()
{
	// TODO: (thom) make this configurable from somewhere?
	mMaxWeights = 4;
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
LimitBoneWeightsProcess::~LimitBoneWeightsProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool LimitBoneWeightsProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_LimitBoneWeights) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void LimitBoneWeightsProcess::Execute( aiScene* pScene)
{
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
		ProcessMesh( pScene->mMeshes[a]);
}

// ------------------------------------------------------------------------------------------------
// Unites identical vertices in the given mesh
void LimitBoneWeightsProcess::ProcessMesh( aiMesh* pMesh)
{
	if( !pMesh->HasBones())
		return;

	// collect all bone weights per vertex
	typedef std::vector< std::vector< Weight > > WeightsPerVertex;
	WeightsPerVertex vertexWeights( pMesh->mNumVertices);

	// collect all weights per vertex
	for( unsigned int a = 0; a < pMesh->mNumBones; a++)
	{
		const aiBone* bone = pMesh->mBones[a];
		for( unsigned int b = 0; b < bone->mNumWeights; b++)
		{
			const aiVertexWeight& w = bone->mWeights[b];
			vertexWeights[w.mVertexId].push_back( Weight( a, w.mWeight));
		}
	}

	// now cut the weight count if it exceeds the maximum
	for( WeightsPerVertex::iterator vit = vertexWeights.begin(); vit != vertexWeights.end(); ++vit)
	{
		if( vit->size() <= mMaxWeights)
			continue;

		// more than the defined maximum -> first sort by weight in descending order. That's 
		// why we defined the < operator in such a weird way.
		std::sort( vit->begin(), vit->end());

		// now kill everything beyond the maximum count
		vit->erase( vit->begin() + mMaxWeights, vit->end());

		// and renormalize the weights
		float sum = 0.0f;
		for( std::vector<Weight>::const_iterator it = vit->begin(); it != vit->end(); ++it)
			sum += it->mWeight;
		for( std::vector<Weight>::iterator it = vit->begin(); it != vit->end(); ++it)
			it->mWeight /= sum;
	}

	// rebuild the vertex weight array for all bones 
	typedef std::vector< std::vector< aiVertexWeight > > WeightsPerBone;
	WeightsPerBone boneWeights( pMesh->mNumBones);
	for( unsigned int a = 0; a < vertexWeights.size(); a++)
	{
		const std::vector<Weight>& vw = vertexWeights[a];
		for( std::vector<Weight>::const_iterator it = vw.begin(); it != vw.end(); ++it)
			boneWeights[it->mBone].push_back( aiVertexWeight( a, it->mWeight));
	}

	// and finally copy the vertex weight list over to the mesh's bones
	for( unsigned int a = 0; a < pMesh->mNumBones; a++)
	{
		const std::vector<aiVertexWeight>& bw = boneWeights[a];
		aiBone* bone = pMesh->mBones[a];
		// ignore the bone if no vertex weights were removed there
		if( bw.size() == bone->mNumWeights)
			continue;

		// copy the weight list. should always be less weights than before, so we don't need a new allocation
		assert( bw.size() < bone->mNumWeights);
		bone->mNumWeights = (unsigned int) bw.size();
		memcpy( bone->mWeights, &bw[0], bw.size() * sizeof( aiVertexWeight));
	}
}
