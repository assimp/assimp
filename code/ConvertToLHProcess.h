/** @file Defines a post processing step to convert all data to a left-handed coordinate system.*/
#ifndef AI_CONVERTTOLHPROCESS_H_INC
#define AI_CONVERTTOLHPROCESS_H_INC

#include "../include/aiTypes.h"
#include "BaseProcess.h"

struct aiMesh;
struct aiBoneAnim;

namespace Assimp
{

// ---------------------------------------------------------------------------
/** The ConvertToLHProcess converts all imported data to a left-handed coordinate
 * system. This implies inverting the Z axis for all transformation matrices
 * invert the orientation of all faces, and adapting skinning and animation 
 * data in a similar way.
 */
class ConvertToLHProcess : public BaseProcess
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	ConvertToLHProcess();

	/** Destructor, private as well */
	~ConvertToLHProcess();

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

	// -------------------------------------------------------------------
	/** Static helper function to convert a vector/matrix from DX coords to OGL coords.
	 * @param poMatrix The matrix to convert.
	 */
	static void ConvertToOGL( aiVector3D& poVector);
	static void ConvertToOGL( aiMatrix3x3& poMatrix);
	static void ConvertToOGL( aiMatrix4x4& poMatrix);

	// -------------------------------------------------------------------
	/** Static helper function to convert a vector/matrix from OGL coords back to DX coords.
	 * @param poMatrix The matrix to convert.
	 */
	static void ConvertToDX( aiVector3D& poVector);
	static void ConvertToDX( aiMatrix3x3& poMatrix);
	static void ConvertToDX( aiMatrix4x4& poMatrix);

protected:
	// -------------------------------------------------------------------
	/** Converts a single mesh to left handed coordinates. 
	 * This simply means the order of all faces is inverted.
	 * @param pMesh The mesh to convert.
	 */
	void ProcessMesh( aiMesh* pMesh);

	// -------------------------------------------------------------------
	/** Converts the given animation to LH coordinates. 
	 * The rotation and translation keys are transformed, the scale keys
	 * work in local space and can therefore be left untouched.
	 * @param pAnim The bone animation to transform
	 */
	void ProcessAnimation( aiBoneAnim* pAnim);

public:
	/** The transformation matrix to convert from DirectX coordinates to OpenGL coordinates. */
	static const aiMatrix3x3 sToOGLTransform;
	/** The transformation matrix to convert from OpenGL coordinates to DirectX coordinates. */
	static const aiMatrix3x3 sToDXTransform;
};

} // end of namespace Assimp

#endif // AI_CONVERTTOLHPROCESS_H_INC