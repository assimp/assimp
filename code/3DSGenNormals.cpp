/** @file Implementation of the 3ds importer class */
#include "3DSLoader.h"
#include "MaterialSystem.h"
#include <algorithm>
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "3DSSpatialSort.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
void Dot3DSImporter::GenNormals(Dot3DS::Mesh* sMesh)
{
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// First generate face normals
	sMesh->mNormals.resize(sMesh->mPositions.size(),aiVector3D());
	for( unsigned int a = 0; a < sMesh->mFaces.size(); a++)
	{
		const Dot3DS::Face& face = sMesh->mFaces[a];

		// assume it is a triangle
		aiVector3D* pV1 = &sMesh->mPositions[face.i1];
		aiVector3D* pV2 = &sMesh->mPositions[face.i2];
		aiVector3D* pV3 = &sMesh->mPositions[face.i3];

		aiVector3D pDelta1 = *pV2 - *pV1;
		aiVector3D pDelta2 = *pV3 - *pV1;
		aiVector3D vNor = pDelta1 ^ pDelta2;
		
		//float fLength = vNor.Length();
		//if (0.0f != fLength)vNor /= fLength;


		sMesh->mNormals[face.i1] = vNor;
		sMesh->mNormals[face.i2] = vNor;
		sMesh->mNormals[face.i3] = vNor;
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// calculate the position bounds so we have a reliable epsilon to 
	// check position differences against 
	// @Schrompf: This is the 6th time this snippet is repeated!
	aiVector3D minVec( 1e10f, 1e10f, 1e10f), maxVec( -1e10f, -1e10f, -1e10f);
	for( unsigned int a = 0; a < sMesh->mPositions.size(); a++)
	{
		minVec.x = std::min( minVec.x, sMesh->mPositions[a].x);
		minVec.y = std::min( minVec.y, sMesh->mPositions[a].y);
		minVec.z = std::min( minVec.z, sMesh->mPositions[a].z);
		maxVec.x = std::max( maxVec.x, sMesh->mPositions[a].x);
		maxVec.y = std::max( maxVec.y, sMesh->mPositions[a].y);
		maxVec.z = std::max( maxVec.z, sMesh->mPositions[a].z);
	}
	const float posEpsilon = (maxVec - minVec).Length() * 1e-5f;


	std::vector<aiVector3D> avNormals;
	avNormals.resize(sMesh->mNormals.size());
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// now generate the spatial sort tree
	D3DSSpatialSorter sSort(sMesh);

	for( std::vector<Dot3DS::Face>::iterator
		i =  sMesh->mFaces.begin();
		i != sMesh->mFaces.end();++i)
		{
		std::vector<unsigned int> poResult;
		// need to repeat the code for all three vertices of the triangle

		// vertex 1
		sSort.FindPositions(sMesh->mPositions[(*i).i1],(*i).iSmoothGroup,
			posEpsilon,poResult);

		aiVector3D vNormals;
		float fDiv = 0.0f;
		for (std::vector<unsigned int>::const_iterator
			a =  poResult.begin();
			a != poResult.end();++a)
			{
			vNormals += sMesh->mNormals[(*a)];
			fDiv += 1.0f;
			}
		vNormals.x /= fDiv;
		vNormals.y /= fDiv;
		vNormals.z /= fDiv;
		vNormals.Normalize();
		avNormals[(*i).i1] = vNormals;
		poResult.clear();

		// vertex 2
		sSort.FindPositions(sMesh->mPositions[(*i).i2],(*i).iSmoothGroup,
			posEpsilon,poResult);

		vNormals = aiVector3D();
		fDiv = 0.0f;
		for (std::vector<unsigned int>::const_iterator
			a =  poResult.begin();
			a != poResult.end();++a)
			{
			vNormals += sMesh->mNormals[(*a)];
			fDiv += 1.0f;
			}
		vNormals.x /= fDiv;
		vNormals.y /= fDiv;
		vNormals.z /= fDiv;
		vNormals.Normalize();
		avNormals[(*i).i2] = vNormals;
		poResult.clear();

		// vertex 3
		sSort.FindPositions(sMesh->mPositions[(*i).i3],(*i).iSmoothGroup,
			posEpsilon ,poResult);

		vNormals = aiVector3D();
		fDiv = 0.0f;
		for (std::vector<unsigned int>::const_iterator
			a =  poResult.begin();
			a != poResult.end();++a)
			{
			vNormals += sMesh->mNormals[(*a)];
			fDiv += 1.0f;
			}
		vNormals.x /= fDiv;
		vNormals.y /= fDiv;
		vNormals.z /= fDiv;
		vNormals.Normalize();
		avNormals[(*i).i3] = vNormals;
		}
	sMesh->mNormals = avNormals;
	return;
}