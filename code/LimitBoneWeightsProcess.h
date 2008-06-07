/** Defines a post processing step to limit the number of bones affecting a single vertex. */
#ifndef AI_LIMITBONEWEIGHTSPROCESS_H_INC
#define AI_LIMITBONEWEIGHTSPROCESS_H_INC

#include "BaseProcess.h"

struct aiMesh;

namespace Assimp
{

// ---------------------------------------------------------------------------
/** This post processing step limits the number of bones affecting a vertex
* to a certain maximum value. If a vertex is affected by more than that number
* of bones, the bone weight with the least influence on this vertex are removed.
* The other weights on this bone are then renormalized to assure the sum weight
* to be 1.
*/
class LimitBoneWeightsProcess : public BaseProcess
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	LimitBoneWeightsProcess();

	/** Destructor, private as well */
	~LimitBoneWeightsProcess();

public:
	// -------------------------------------------------------------------
	/** Returns whether the processing step is present in the given flag field.
	* @param pFlags The processing flags the importer was called with. A bitwise
	*   combination of #aiPostProcessSteps.
	* @return true if the process is present in this flag fields, false if not.
	*/
	bool IsActive( unsigned int pFlags) const;

	// -------------------------------------------------------------------
	/** Executes the post processing step on the given imported data.
	* At the moment a process is not supposed to fail.
	* @param pScene The imported data to work at.
	*/
	void Execute( aiScene* pScene);

protected:
	// -------------------------------------------------------------------
	/** Limits the bone weight count for all vertices in the given mesh.
	* @param pMesh The mesh to process.
	*/
	void ProcessMesh( aiMesh* pMesh);

protected:
	/** Describes a bone weight on a vertex */
	struct Weight
	{
		unsigned int mBone; ///< Index of the bone
		float mWeight;      ///< Weight of that bone on this vertex
		Weight() { }
		Weight( unsigned int pBone, float pWeight) { mBone = pBone; mWeight = pWeight; }
		/** Comparision operator to sort bone weights by descending weight */
		bool operator < (const Weight& pWeight) const { return mWeight > pWeight.mWeight; }
	};

	/** Maximum number of bones influencing any single vertex. */
	unsigned int mMaxWeights;
};

} // end of namespace Assimp

#endif // AI_LIMITBONEWEIGHTSPROCESS_H_INC
