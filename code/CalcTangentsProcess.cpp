/** @file Implementation of the post processing step to calculate tangents and bitangents
 * for all imported meshes
 */

#include <vector>
#include <assert.h>
#include "CalcTangentsProcess.h"
#include "SpatialSort.h"
#include "../include/aiPostProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
CalcTangentsProcess::CalcTangentsProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
CalcTangentsProcess::~CalcTangentsProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool CalcTangentsProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_CalcTangentSpace) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void CalcTangentsProcess::Execute( aiScene* pScene)
{
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
		ProcessMesh( pScene->mMeshes[a]);
}

// ------------------------------------------------------------------------------------------------
// Calculates tangents and bitangents for the given mesh
void CalcTangentsProcess::ProcessMesh( aiMesh* pMesh)
{
	// we assume that the mesh is still in the verbose vertex format where each face has its own set
	// of vertices and no vertices are shared between faces. Sadly I don't know any quick test to 
	// assert() it here.
    //assert( must be verbose, dammit);

	// what we can check, though, is if the mesh has normals and texture coord. That's a requirement
	if( pMesh->mNormals == NULL || pMesh->mTextureCoords[0] == NULL)
		return;

	// calculate the position bounds so we have a reliable epsilon to check position differences against 
	aiVector3D minVec( 1e10f, 1e10f, 1e10f), maxVec( -1e10f, -1e10f, -1e10f);
	for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
	{
		minVec.x = std::min( minVec.x, pMesh->mVertices[a].x);
		minVec.y = std::min( minVec.y, pMesh->mVertices[a].y);
		minVec.z = std::min( minVec.z, pMesh->mVertices[a].z);
		maxVec.x = std::max( maxVec.x, pMesh->mVertices[a].x);
		maxVec.y = std::max( maxVec.y, pMesh->mVertices[a].y);
		maxVec.z = std::max( maxVec.z, pMesh->mVertices[a].z);
	}

	// calculate epsilons border
	const float epsilon = 1e-5f;
	const float posEpsilon = (maxVec - minVec).Length() * epsilon;
	const float angleEpsilon = 0.9999f;

	// create space for the tangents and bitangents
	pMesh->mTangents = new aiVector3D[pMesh->mNumVertices];
	pMesh->mBitangents = new aiVector3D[pMesh->mNumVertices];

	const aiVector3D* meshPos = pMesh->mVertices;
	const aiVector3D* meshNorm = pMesh->mNormals;
	const aiVector3D* meshTex = pMesh->mTextureCoords[0];
	aiVector3D* meshTang = pMesh->mTangents;
	aiVector3D* meshBitang = pMesh->mBitangents;
	
	// calculate the tangent and bitangent for every face
	for( unsigned int a = 0; a < pMesh->mNumFaces; a++)
	{
		const aiFace& face = pMesh->mFaces[a];

		// triangle or polygon... we always use only the first three indices. A polygon
		// is supposed to be planar anyways....
		// FIXME: (thom) create correct calculation for multi-vertex polygons maybe?
		const unsigned int p0 = face.mIndices[0], p1 = face.mIndices[1], p2 = face.mIndices[2];

		// position differences p1->p2 and p1->p3
		aiVector3D v = meshPos[p1] - meshPos[p0], w = meshPos[p2] - meshPos[p0];

		// texture offset p1->p2 and p1->p3
		float sx = meshTex[p1].x - meshTex[p0].x, sy = meshTex[p1].y - meshTex[p0].y;
        float tx = meshTex[p2].x - meshTex[p0].x, ty = meshTex[p2].y - meshTex[p0].y;
		float dirCorrection = (tx * sy - ty * sx) < 0.0f ? -1.0f : 1.0f;

		// tangent points in the direction where to positive X axis of the texture coords would point in model space
		// bitangents points along the positive Y axis of the texture coords, respectively
		aiVector3D tangent, bitangent;
		tangent.x = (w.x * sy - v.x * ty) * dirCorrection;
        tangent.y = (w.y * sy - v.y * ty) * dirCorrection;
        tangent.z = (w.z * sy - v.z * ty) * dirCorrection;
        bitangent.x = (w.x * sx - v.x * tx) * dirCorrection;
        bitangent.y = (w.y * sx - v.y * tx) * dirCorrection;
        bitangent.z = (w.z * sx - v.z * tx) * dirCorrection;

		// store for every vertex of that face
		for( unsigned int b = 0; b < face.mNumIndices; b++)
		{
			unsigned int p = face.mIndices[b];

			// project tangent and bitangent into the plane formed by the vertex' normal
			aiVector3D localTangent = tangent - meshNorm[p] * (tangent * meshNorm[p]);
			aiVector3D localBitangent = bitangent - meshNorm[p] * (bitangent * meshNorm[p]);
			localTangent.Normalize(); localBitangent.Normalize();

			// and write it into the mesh.
			meshTang[p] = localTangent;
			meshBitang[p] = localBitangent;
		}
    }

	// create a helper to quickly find locally close vertices among the vertex array
	SpatialSort vertexFinder( meshPos, pMesh->mNumVertices, sizeof( aiVector3D));
	std::vector<unsigned int> verticesFound;

	// in the second pass we now smooth out all tangents and bitangents at the same local position 
	// if they are not too far off.
	std::vector<bool> vertexDone( pMesh->mNumVertices, false);
	for( unsigned int a = 0; a < pMesh->mNumVertices; a++)
	{
		if( vertexDone[a])
			continue;

		const aiVector3D& origPos = pMesh->mVertices[a];
		const aiVector3D& origNorm = pMesh->mNormals[a];
		const aiVector3D& origTang = pMesh->mTangents[a];
		const aiVector3D& origBitang = pMesh->mBitangents[a];
		std::vector<unsigned int> closeVertices;
		closeVertices.push_back( a);

		// find all vertices close to that position
		vertexFinder.FindPositions( origPos, posEpsilon, verticesFound);

		// look among them for other vertices sharing the same normal and a close-enough tangent/bitangent
		static const float MAX_DIFF_ANGLE = 0.701f;
		for( unsigned int b = 0; b < verticesFound.size(); b++)
		{
			unsigned int idx = verticesFound[b];
			if( vertexDone[idx])
				continue;
			if( meshNorm[idx] * origNorm < angleEpsilon)
				continue;
			if( meshTang[idx] * origTang < MAX_DIFF_ANGLE)
				continue;
			if( meshBitang[idx] * origBitang < MAX_DIFF_ANGLE)
				continue;

			// it's similar enough -> add it to the smoothing group
			closeVertices.push_back( idx);
			vertexDone[idx] = true;
		}

		// smooth the tangents and bitangents of all vertices that were found to be close enough
		aiVector3D smoothTangent( 0, 0, 0), smoothBitangent( 0, 0, 0);
		for( unsigned int b = 0; b < closeVertices.size(); b++)
		{
			smoothTangent += meshTang[ closeVertices[b] ];
			smoothBitangent += meshBitang[ closeVertices[b] ];
		}
		smoothTangent.Normalize();
		smoothBitangent.Normalize();

		// and write it back into all affected tangents
		for( unsigned int b = 0; b < closeVertices.size(); b++)
		{
			meshTang[ closeVertices[b] ] = smoothTangent;
			meshBitang[ closeVertices[b] ] = smoothBitangent;
		}
	}
}
