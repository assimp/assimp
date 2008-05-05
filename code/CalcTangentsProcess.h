/** @file Defines a post processing step to calculate tangents and bitangents on all imported meshes.*/
#ifndef AI_CALCTANGENTSPROCESS_H_INC
#define AI_CALCTANGENTSPROCESS_H_INC

#include "BaseProcess.h"

struct aiMesh;

namespace Assimp
{

// ---------------------------------------------------------------------------
/** The CalcTangentsProcess calculates the tangent and bitangent for any vertex
 * of all meshes. It is expected to be run before the JoinVerticesProcess runs
 * because the joining of vertices also considers tangents and bitangents for uniqueness.

 */
class CalcTangentsProcess : public BaseProcess
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	CalcTangentsProcess();

	/** Destructor, private as well */
	~CalcTangentsProcess();

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
	/** Calculates tangents and bitangents for the given mesh
	 * @param pMesh The mesh to process.
	 */
	void ProcessMesh( aiMesh* pMesh);
};

} // end of namespace Assimp

#endif // AI_CALCTANGENTSPROCESS_H_INC