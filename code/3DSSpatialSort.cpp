/** @file Implementation of the helper class to quickly find vertices close to a given position */
#include <algorithm>
#include "3DSSpatialSort.h"

using namespace Assimp;
using namespace Assimp::Dot3DS;

// ------------------------------------------------------------------------------------------------
// Constructs a spatially sorted representation from the given position array.
D3DSSpatialSorter::D3DSSpatialSorter( const aiVector3D* pPositions,
									 unsigned int pNumPositions, unsigned int pElementOffset)
	{
	// define the reference plane. We choose some arbitrary vector away from all basic axises 
	// in the hope that no model spreads all its vertices along this plane.
	mPlaneNormal.Set( 0.8523f, 0.34321f, 0.5736f);
	mPlaneNormal.Normalize();

	// store references to all given positions along with their distance to the reference plane
	mPositions.reserve( pNumPositions);
	for( unsigned int a = 0; a < pNumPositions; a++)
		{
		const char* tempPointer = reinterpret_cast<const char*> (pPositions);
		const aiVector3D* vec = reinterpret_cast<const aiVector3D*> (tempPointer + a * pElementOffset);

		// store position by index and distance
		float distance = *vec * mPlaneNormal;
		mPositions.push_back( Entry( a, *vec, distance,0));
		}

	// now sort the array ascending by distance.
	std::sort( mPositions.begin(), mPositions.end());
	}
// ------------------------------------------------------------------------------------------------
D3DSSpatialSorter::D3DSSpatialSorter( const Dot3DS::Mesh* p_pcMesh)
	{
	// define the reference plane. We choose some arbitrary vector away from all basic axises 
	// in the hope that no model spreads all its vertices along this plane.
	mPlaneNormal.Set( 0.8523f, 0.34321f, 0.5736f);
	mPlaneNormal.Normalize();

	// store references to all given positions along with their distance to the reference plane
	mPositions.reserve( p_pcMesh->mPositions.size());
	for( std::vector<Dot3DS::Face>::const_iterator
		i =  p_pcMesh->mFaces.begin();
		i != p_pcMesh->mFaces.end();++i)
		{
		// store position by index and distance
		float distance = p_pcMesh->mPositions[(*i).i1] * mPlaneNormal;
		mPositions.push_back( Entry( (*i).i1, p_pcMesh->mPositions[(*i).i1], 
			distance, (*i).iSmoothGroup));

		// triangle vertex 2
		distance = p_pcMesh->mPositions[(*i).i2] * mPlaneNormal;
		mPositions.push_back( Entry( (*i).i2, p_pcMesh->mPositions[(*i).i2], 
			distance, (*i).iSmoothGroup));

		// triangle vertex 3
		distance = p_pcMesh->mPositions[(*i).i3] * mPlaneNormal;
		mPositions.push_back( Entry( (*i).i3, p_pcMesh->mPositions[(*i).i3], 
			distance, (*i).iSmoothGroup));
		}

	// now sort the array ascending by distance.
	std::sort( this->mPositions.begin(), this->mPositions.end());
	}
// ------------------------------------------------------------------------------------------------
// Destructor
D3DSSpatialSorter::~D3DSSpatialSorter()
	{
	// nothing to do here, everything destructs automatically
	}

// ------------------------------------------------------------------------------------------------
// Returns an iterator for all positions close to the given position.
void D3DSSpatialSorter::FindPositions( const aiVector3D& pPosition, 
									  uint32_t pSG,float pRadius, std::vector<unsigned int>& poResults) const
	{
	float dist = pPosition * mPlaneNormal;
	float minDist = dist - pRadius, maxDist = dist + pRadius;

	// clear the array in this strange fashion because a simple clear() would also deallocate
	// the array which we want to avoid
	poResults.erase( poResults.begin(), poResults.end());

	// quick check for positions outside the range
	if( mPositions.size() == 0)
		return;
	if( maxDist < mPositions.front().mDistance)
		return;
	if( minDist > mPositions.back().mDistance)
		return;

	// do a binary search for the minimal distance to start the iteration there
	unsigned int index = mPositions.size() / 2;
	unsigned int binaryStepSize = mPositions.size() / 4;
	while( binaryStepSize > 1)
		{
		if( mPositions[index].mDistance < minDist)
			index += binaryStepSize;
		else
			index -= binaryStepSize;

		binaryStepSize /= 2;
		}

	// depending on the direction of the last step we need to single step a bit back or forth
	// to find the actual beginning element of the range
	while( index > 0 && mPositions[index].mDistance > minDist)
		index--;
	while( index < (mPositions.size() - 1) && mPositions[index].mDistance < minDist)
		index++;

	// Mow start iterating from there until the first position lays outside of the distance range.
	// Add all positions inside the distance range within the given radius to the result aray

	if (0 == pSG)
		{
		std::vector<Entry>::const_iterator it = mPositions.begin() + index;
		float squareEpsilon = pRadius * pRadius;
		while( it->mDistance < maxDist)
			{
			if((it->mPosition - pPosition).SquareLength() < squareEpsilon)
				{
				poResults.push_back( it->mIndex);
				}
			++it;
			if( it == mPositions.end())
				break;
			}
		}
	else
		{
		std::vector<Entry>::const_iterator it = mPositions.begin() + index;
		float squareEpsilon = pRadius * pRadius;
		while( it->mDistance < maxDist)
			{
			if((it->mPosition - pPosition).SquareLength() < squareEpsilon &&
				(it->mSmoothGroups & pSG || 0 == it->mSmoothGroups))
				{
				poResults.push_back( it->mIndex);
				}
			++it;
			if( it == mPositions.end())
				break;
			}
		}
	}

