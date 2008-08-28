
/** @file File I/O wrappers for C++. Use interfaces instead of function
* pointers to be sure even the silliest men on earth can work with this
*/

#ifndef AI_IOSTREAM_H_INC
#define AI_IOSTREAM_H_INC

#include <string>
#include <stddef.h>

#include "aiTypes.h"
#include "aiFileIO.h"

#ifndef __cplusplus
#error This header requires C++ to be used.
#endif

namespace Assimp
{

// ---------------------------------------------------------------------------
/** Class to handle file I/O for C++
*
* Derive an own implementation from this interface to provide custom IO handling
* to the Importer. If you implement this interface, be sure to also provide an
* implementation for IOSystem that creates instances of your custom IO class.
*/
// ---------------------------------------------------------------------------
class ASSIMP_API IOStream 
{
protected:
	/** Constructor protected, use IOSystem::Open() to create an instance. */
	IOStream(void);

public:
	// -------------------------------------------------------------------
	/** Destructor. Deleting the object closes the underlying file, 
	 * alternatively you may use IOSystem::Close() to release the file. 
	 */
	virtual ~IOStream(void);

	// -------------------------------------------------------------------
	/** Read from the file
	*
	* See fread() for more details
	* This fails for write-only files
	*/
	// -------------------------------------------------------------------
    virtual size_t Read(
		void* pvBuffer, 
		size_t pSize, 
		size_t pCount) = 0;


	// -------------------------------------------------------------------
	/** Write to the file
	*
	* See fwrite() for more details
	* This fails for read-only files
	*/
	// -------------------------------------------------------------------
    virtual size_t Write(
		const void* pvBuffer, 
		size_t pSize,
		size_t pCount) = 0;


	// -------------------------------------------------------------------
	/** Set the read/write cursor of the file
	*
	* See fseek() for more details
	*/
	// -------------------------------------------------------------------
	virtual aiReturn Seek(
		size_t pOffset,
		aiOrigin pOrigin) = 0;


	// -------------------------------------------------------------------
	/** Get the current position of the read/write cursor
	*
	* See ftell() for more details
	*/
	// -------------------------------------------------------------------
    virtual size_t Tell(void) const = 0;


	// -------------------------------------------------------------------
	/**	Returns filesize
	*
	*	Returns the filesize
	*/
	// -------------------------------------------------------------------
	virtual size_t	FileSize() const = 0;
};
// ----------------------------------------------------------------------------
inline IOStream::IOStream()
{
	// empty
}

// ----------------------------------------------------------------------------
inline IOStream::~IOStream()
{
	// empty
}
// ----------------------------------------------------------------------------

} //!ns Assimp

#endif //!!AI_IOSTREAM_H_INC
