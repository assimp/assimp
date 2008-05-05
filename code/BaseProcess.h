/** @file Base class of all import post processing steps */
#ifndef AI_BASEPROCESS_H_INC
#define AI_BASEPROCESS_H_INC

struct aiScene;

namespace Assimp
{

// ---------------------------------------------------------------------------
/** The BaseProcess defines a common interface for all post processing steps.
 * A post processing step is run after a successful import if the caller
 * specified the corresponding flag when calling ReadFile(). Enum #aiPostProcessSteps
 * defines which flags are available. 
 * After a successful import the Importer iterates over its internal array of processes
 * and calls IsActive() on each process to evaluate if the step should be executed.
 * If the function returns true, the class' Execute() function is called subsequently.
 */
class BaseProcess
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	BaseProcess();

	/** Destructor, private as well */
	virtual ~BaseProcess();

public:
	// -------------------------------------------------------------------
	/** Returns whether the processing step is present in the given flag field.
	 * @param pFlags The processing flags the importer was called with. A bitwise
	 *   combination of #aiPostProcessSteps.
	 * @return true if the process is present in this flag fields, false if not.
	*/
	virtual bool IsActive( unsigned int pFlags) const = 0;

	// -------------------------------------------------------------------
	/** Executes the post processing step on the given imported data.
	* At the moment a process is not supposed to fail.
	* @param pScene The imported data to work at.
	*/
	virtual void Execute( aiScene* pScene) = 0;
};

/** Constructor, dummy implementation to keep the compiler from complaining */
inline BaseProcess::BaseProcess()
{
}

/** Destructor */
inline BaseProcess::~BaseProcess()
{
}

} // end of namespace Assimp

#endif // AI_BASEPROCESS_H_INC