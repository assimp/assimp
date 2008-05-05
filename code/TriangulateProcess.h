/** @file Defines a post processing step to triangulate all faces with more than three vertices.*/
#ifndef AI_TRIANGULATEPROCESS_H_INC
#define AI_TRIANGULATEPROCESS_H_INC

#include "BaseProcess.h"

struct aiMesh;

namespace Assimp
{

// ---------------------------------------------------------------------------
/** The TriangulateProcess splits up all faces with more than three indices
 * into triangles. You usually want this to happen because the graphics cards
 * need their data as triangles.
 */
class TriangulateProcess : public BaseProcess
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	TriangulateProcess();

	/** Destructor, private as well */
	~TriangulateProcess();

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
	/** Triangulates the given mesh.
	 * @param pMesh The mesh to triangulate.
	 */
	void TriangulateMesh( aiMesh* pMesh);
};

} // end of namespace Assimp

#endif // AI_TRIANGULATEPROCESS_H_INC