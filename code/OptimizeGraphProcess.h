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

/** Defines a post processing step to refactor the output node graph to 
    be more compact */
#ifndef AI_OPTIMIZEGRAPHPROCESS_H_INC
#define AI_OPTIMIZEGRAPHPROCESS_H_INC

#include "BaseProcess.h"

struct aiMesh;
struct aiNode;
struct aiBone;
class OptimizeGraphProcessTest;

namespace Assimp	{

// NOTE: If you change these limits, don't forget to change the
// corresponding values in all Assimp ports

// **********************************************************
// Java: ConfigProperty.java, 
//  ConfigProperty.OG_MIN_NUM_FACES
//  ConfigProperty.JOIN_INEQUAL_TRANSFORMS
// **********************************************************

#if (!defined AI_OG_MAX_DEPTH)
#	define AI_OG_MAX_DEPTH	0x4
#endif // !! AI_LMW_MAX_WEIGHTS

#if (!defined AI_OG_MIN_NUM_FACES)
#	define AI_OG_MIN_NUM_FACES 0xffffffff
#endif // !! AI_LMW_MAX_WEIGHTS

#if (!defined AI_OG_JOIN_INEQUAL_TRANSFORMS)
#	define AI_OG_JOIN_INEQUAL_TRANSFORMS false
#endif // !! AI_LMW_MAX_WEIGHTS


// ---------------------------------------------------------------------------
struct NodeIndexEntry : public std::pair<unsigned int, unsigned int>
{
	// binary operator < for use with std::sort
	bool operator< (const NodeIndexEntry& other)
	{
		return second < other.second;
	}

	// pointer to the original node
	aiNode* pNode;
};
typedef std::vector<NodeIndexEntry> NodeIndexList;

// ---------------------------------------------------------------------------
/** This post processing step reformats the output node graph to be more
 *  compact. There are several options, e.g. maximum hierachy depth or
 *  minimum mesh size. Some files store every face as a new mesh, so
 *  some Assimp steps (such as FixInfacingNormals or SmoothNormals) don't
 *  work properly on these meshes. This step joins such small meshes and
 *  nodes. Animations are kept during the step.
 *  @note Use the PretransformVertices step to remove the node graph
 *    completely (and all animations, too).
*/
class ASSIMP_API OptimizeGraphProcess : public BaseProcess
{
	friend class Importer;
	friend class ::OptimizeGraphProcessTest;

protected:
	/** Constructor to be privately used by Importer */
	OptimizeGraphProcess();

	/** Destructor, private as well */
	~OptimizeGraphProcess();


	typedef std::pair< unsigned int, aiNode* > MeshRefCount;
	typedef unsigned int MeshHash;

public:

	// -------------------------------------------------------------------
	/** Returns whether the processing step is present in the given flag.
	* @param pFlags The processing flags the importer was called with. 
	*   A bitwise combination of #aiPostProcessSteps.
	* @return true if the process is present in this flag fields, 
	*   false if not.
	*/
	bool IsActive( unsigned int pFlags) const;

	// -------------------------------------------------------------------
	/** Called prior to ExecuteOnScene().
	* The function is a request to the process to update its configuration
	* basing on the Importer's configuration property list.
	*/
	void SetupProperties(const Importer* pImp);


	// set the configMinNumfaces property
	inline void SetMinNumFaces(unsigned int n)
	{
		configMinNumFaces = n;
	}


protected:

	// -------------------------------------------------------------------
	/** Executes the post processing step on the given imported data.
	* At the moment a process is not supposed to fail.
	* @param pScene The imported data to work at.
	*/
	void Execute( aiScene* pScene);


	// -------------------------------------------------------------------
	/** Removes animation nodes from the tree. 
	 * @param node Current node
	 * @param out Receives a list of replacement nodes for *this* node -
	 *   if *this* node should be kept, it must be added to the list.
	*/
	void RemoveAnimationNodes (aiNode* node,std::vector<aiNode*>& out);


	// -------------------------------------------------------------------
	/** Finds and marks all locked nodes in the tree.
	 * A node is locked if it is referenced by animations.
	 * @param node ROot node to start with
	 * @note A locked node has the MSB set in its mNumChildren member
	 */
	void FindLockedNodes(aiNode* node);


	// -------------------------------------------------------------------
	/** Searches for locked meshes. A mesh is locked if it is referenced
	 *  by more than one node in the hierarchy.
	 * @param node Root node to start with
	 */
	void FindLockedMeshes(aiNode* node);
	void FindLockedMeshes(aiNode* node, MeshRefCount* pRefCount);


	// -------------------------------------------------------------------
	/** Unlocks all nodes in the output tree.
	 * @param node Root node to start with
	 */
	void UnlockNodes(aiNode* node);


	// -------------------------------------------------------------------
	/** Unlocks all meshes in the output tree.
	 */
	void UnlockMeshes();


	// -------------------------------------------------------------------
	/** Apply the final optimization algorithm to the tree. 
	 * See the Execute-method for a detailled description of the algorithm.
	 * @param node Root node to start with
	 */
	void ApplyOptimizations(aiNode* node);


	// -------------------------------------------------------------------
	/** Binary search for the first element that is >= min.
	 * @param sortedArray Input array
	 * @param min Treshold
	 */
	unsigned int BinarySearch(NodeIndexList& sortedArray,
		unsigned int min, unsigned int& index, unsigned int iStart);


	// -------------------------------------------------------------------
	/** Compute stable hashes for all meshes and fill mMeshHashes 
	 *  with the results.
	 */
	void ComputeMeshHashes();


	// -------------------------------------------------------------------
	/** Optimizes the meshes in a single node aftr the joining process.
	 * @param pNode Node to be optimized. Childs noded won't be processed
	 *   automatically.
	 */
	void ApplyNodeMeshesOptimization(aiNode* pNode);



	// -------------------------------------------------------------------
	/** Build the output mesh list.
	 */
	void BuildOutputMeshList();


	// -------------------------------------------------------------------
	/** Transform meshes from one coordinate space into another.
	 * @param quak Input space. All meshes referenced by this node -
	 *   assuming none of them is locked - are transformed in the
	 *   local coordinate space of pNode
	 * @param pNode Destination coordinate space
	 */
	void TransformMeshes(aiNode* quak,aiNode* pNode);

private:

	/** Configuration option: specifies the minimum number of faces
	    a node should have. The steps tries to join meshes with less
		vertices that are on the same hierarchy level. 
		If this value is set to a very large value (e.g. 0xffffffff)
		all meshes on the same hierarchy level are joined - if they
		aren't animation nodes and if they have the same world matrices
	 */
	unsigned int configMinNumFaces;


	/** Configuration option: specifies whether nodes with inequal
	    world matrices are joined if they are on the same hierarchy
	    level and if it seems to make sense.
	 */
	bool configJoinInequalTransforms;

	/** Working data */
	aiScene* pScene;

	/** List of hash identifiers for all meshes. 
	    The hashes are build from both the meshes vertex format
		and the material indices. Bones are not taken into account.
	*/
	std::vector<MeshHash> mMeshHashes;

	/** List of output meshes.
	 */
	std::vector<aiMesh*> mOutputMeshes;
};

} // end of namespace Assimp

#endif // AI_LIMITBONEWEIGHTSPROCESS_H_INC
