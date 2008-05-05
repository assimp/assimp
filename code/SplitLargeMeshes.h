/** @file Defines a post processing step to split large meshes into submeshes*/
#ifndef AI_SPLITLARGEMESHES_H_INC
#define AI_SPLITLARGEMESHES_H_INC

#include <vector>
#include "BaseProcess.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"

namespace Assimp
{

#define AI_SLM_MAX_VERTICES	1000000

// ---------------------------------------------------------------------------
/** Postprocessing filter to split large meshes into submeshes
*/
class SplitLargeMeshesProcess : public BaseProcess
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	SplitLargeMeshesProcess();

	/** Destructor, private as well */
	~SplitLargeMeshesProcess();

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


private:

	//! Apply the algorithm to a given mesh
	void SplitMesh (unsigned int a, aiMesh* pcMesh,
		std::vector<std::pair<aiMesh*, unsigned int> >& avList);

	//! Update a node in the asset after a few of its meshes
	//! have been split
	void UpdateNode(aiNode* pcNode,
		const std::vector<std::pair<aiMesh*, unsigned int> >& avList);
};

} // end of namespace Assimp

#endif // !!AI_SPLITLARGEMESHES_H_INC