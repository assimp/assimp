/** @file Filesystem wrapper for C++. Inherit this class to supply custom file handling
 * logic to the Import library.
*/

#ifndef AI_IOSYSTEM_H_INC
#define AI_IOSYSTEM_H_INC

#ifndef __cplusplus
#error This header requires C++ to be used.
#endif

#include <string>

#include "aiDefines.h"

namespace Assimp
{

class IOStream;

// ---------------------------------------------------------------------------
/** Interface to the file system.
*
* Derive an own implementation from this interface to supply custom file handling
* to the importer library. If you implement this interface, you also want to
* supply a custom implementation for IOStream.
*/
class ASSIMP_API IOSystem
{
public:
	/** Constructor. Create an instance of your derived class and assign it to 
	 * the #Importer instance by calling Importer::SetIOHandler().
	 */
	IOSystem();

	/** Destructor. */
	virtual ~IOSystem();

	// -------------------------------------------------------------------
	/** Tests for the existence of a file at the given path. 
	*
	* @param pFile Path to the file
	* @return true if there is a file with this path, else false.
	*/
	virtual bool Exists( const std::string& pFile) const = 0;

	// -------------------------------------------------------------------
	/**	Returns the system specific directory separator
	*	@return	System specific directory separator
	*/
	virtual std::string getOsSeparator() const = 0;

	// -------------------------------------------------------------------
	/** Open a new file with a given path. When the access to the file
	* is finished, call Close() to release all associated resources.
	*
	* @param pFile Path to the file
	* @param pMode Desired file I/O mode. Required are: "wb", "w", "wt",
	*        "rb", "r", "rt".
	*
	* @return New IOStream interface allowing the lib to access
	*         the underlying file. 
	* @note When implementing this class to provide custom IO handling, 
	* you propably have to supply an own implementation of IOStream as well. 
	*/
	virtual IOStream* Open(
		const std::string& pFile,
		const std::string& pMode = std::string("rb")) = 0;

	// -------------------------------------------------------------------
	/** Closes the given file and releases all resources associated with it.
	 * @param pFile The file instance previously created by Open().
	 */
	virtual void Close( IOStream* pFile) = 0;


	// -------------------------------------------------------------------
	/** Compares two paths and check whether the point to identical files.
	 *  
	 * The dummy implementation of this virtual performs a 
	 * case-insensitive comparison of the absolute path strings.
	 * @param one First file
	 * @param second Second file
	 * @return true if the paths point to the same file. The file needn't
	 *   be existing, however.
	 */
	virtual bool ComparePaths (const std::string& one, 
		const std::string& second);
};

// ----------------------------------------------------------------------------
inline IOSystem::IOSystem() 
{
	// empty
}

// ----------------------------------------------------------------------------
inline IOSystem::~IOSystem() 
{
	// empty
}
// ----------------------------------------------------------------------------


} //!ns Assimp

#endif //AI_IOSYSTEM_H_INC
