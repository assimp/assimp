/** Small helper classes to optimise finding vertizes close to a given location */
#ifndef AI_D3DSSPATIALSORT_H_INC
#define AI_D3DSSPATIALSORT_H_INC

#include <vector>
#include "../include/aiVector3D.h"
#include "3DSHelper.h"

namespace Assimp
{

using namespace Dot3DS;

// ------------------------------------------------------------------------------------------------
/** Specialized version of SpatialSort to support smoothing groups
 *  This is used in the .3ds loader
 */
class D3DSSpatialSorter
{
public:

	D3DSSpatialSorter() {/* This is unintialized. This is evil. This is OK. */}

	/** Constructs a spatially sorted representation from the given position array.
	 * Supply the positions in its layout in memory, the class will only refer to them
	 * by index.
	 * @param pPositions Pointer to the first position vector of the array.
	 * @param pNumPositions Number of vectors to expect in that array.
	 * @param pElementOffset Offset in bytes from the beginning of one vector in memory to the beginning of the next vector.
	 * @note Smoothing groups are ignored
	 */
	D3DSSpatialSorter( const aiVector3D* pPositions,
		unsigned int pNumPositions, unsigned int pElementOffset);

	/** Construction from a given face array, handling smoothing groups properly
	 * @param p_pcMesh Input mesh.
	 */
	D3DSSpatialSorter( const Dot3DS::Mesh* p_pcMesh);


	/** Destructor */
	~D3DSSpatialSorter();

	/** Returns an iterator for all positions close to the given position.
	 * @param pPosition The position to look for vertices.
	 * @param pSG Only included vertices with at least one shared smooth group
	 * @param pRadius Maximal distance from the position a vertex may have to be counted in.
	 * @param poResults The container to store the indices of the found positions. Will be emptied
	 *   by the call so it may contain anything.
	 * @return An iterator to iterate over all vertices in the given area.
	 */
	void FindPositions( const aiVector3D& pPosition, uint32_t pSG,
		float pRadius, std::vector<unsigned int>& poResults) const;

protected:
	/** Normal of the sorting plane, normalized. The center is always at (0, 0, 0) */
	aiVector3D mPlaneNormal;

	/** An entry in a spatially sorted position array. Consists of a vertex index,
	 * its position and its precalculated distance from the reference plane */
	struct Entry
	{
		unsigned int mIndex; ///< The vertex referred by this entry
		aiVector3D mPosition; ///< Position
		uint32_t mSmoothGroups;
		float mDistance; ///< Distance of this vertex to the sorting plane

		Entry() { /** intentionally not initialized.*/ }
		Entry( unsigned int pIndex, const aiVector3D& pPosition, float pDistance,uint32_t pSG) 
			: 
			mPosition( pPosition), mIndex( pIndex), mDistance( pDistance),mSmoothGroups (pSG)
			{ 	}

		bool operator < (const Entry& e) const { return mDistance < e.mDistance; }
	};

	// all positions, sorted by distance to the sorting plane
	std::vector<Entry> mPositions;
};

} // end of namespace Assimp

#endif // AI_SPATIALSORT_H_INC