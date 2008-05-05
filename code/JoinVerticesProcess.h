/** @file Defines a post processing step to join identical vertices on all imported meshes.*/
#ifndef AI_JOINVERTICESPROCESS_H_INC
#define AI_CALCTANGENTSPROCESS_H_INC

#include "BaseProcess.h"
#include "../include/aiTypes.h"

struct aiMesh;

namespace Assimp
{

// ---------------------------------------------------------------------------
/** The JoinVerticesProcess unites identical vertices in all imported meshes. 
 * By default the importer returns meshes where each face addressed its own 
 * set of vertices even if that means that identical vertices are stored multiple
 * times. The JoinVerticesProcess finds these identical vertices and 
 * erases all but one of the copies. This usually reduces the number of vertices
 * in a mesh by a serious amount and is the standard form to render a mesh.
 */
class JoinVerticesProcess : public BaseProcess
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	JoinVerticesProcess();

	/** Destructor, private as well */
	~JoinVerticesProcess();

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
	/** Unites identical vertices in the given mesh.
	 * @param pMesh The mesh to process.
	 */
	void ProcessMesh( aiMesh* pMesh);

	/** Little helper function to calculate the quadratic difference of two colours. */
	float GetColorDifference( const aiColor4D& pColor1, const aiColor4D& pColor2) const
	{
		aiColor4D c( pColor1.r - pColor2.r, pColor1.g - pColor2.g, pColor1.b - pColor2.b, pColor1.a - pColor2.a);
		return c.r*c.r + c.g*c.g + c.b*c.b + c.a*c.a;
	}
};

} // end of namespace Assimp

#endif // AI_CALCTANGENTSPROCESS_H_INC