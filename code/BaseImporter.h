/** @file Definition of the base class for all importer worker classes. */
#ifndef AI_BASEIMPORTER_H_INC
#define AI_BASEIMPORTER_H_INC

#include <string>

struct aiScene;

namespace Assimp
{

class IOSystem;

// ---------------------------------------------------------------------------
/** Simple exception class to be thrown if an error occurs while importing. */
class ImportErrorException 
{
public:
	/** Constructor with arguments */
	ImportErrorException( const std::string& pErrorText)
	{
		mErrorText = pErrorText;
	}

	/** Returns the error text provided when throwing the exception */
	const std::string& GetErrorText() const { return mErrorText; }

private:
	std::string mErrorText;
};

// ---------------------------------------------------------------------------
/** The BaseImporter defines a common interface for all importer worker 
 *  classes.
 *
 * The interface defines two functions: CanRead() is used to check if the 
 * importer can handle the format of the given file. If an implementation of 
 * this function returns true, the importer then calls ReadFile() which 
 * imports the given file. ReadFile is not overridable, it just calls InternReadFile() 
 * and catches any ImportErrorException that might occur.
 */
class BaseImporter
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	BaseImporter();

	/** Destructor, private as well */
	virtual ~BaseImporter();

public:
	// -------------------------------------------------------------------
	/** Returns whether the class can handle the format of the given file.
	* @param pFile Path and file name of the file to be examined.
	* @param pIOHandler The IO handler to use for accessing any file.
	* @return true if the class can read this file, false if not.
	*/
	virtual bool CanRead( const std::string& pFile, IOSystem* pIOHandler) const = 0;

	// -------------------------------------------------------------------
	/** Imports the given file and returns the imported data.
	* If the import succeeds, ownership of the data is transferred to the caller. 
	* If the import failes, NULL is returned. The function takes care that any
	* partially constructed data is destroyed beforehand.
	*
	* @param pFile Path of the file to be imported. 
	* @param pIOHandler IO-Handler used to open this and possible other files.
	* @return The imported data or NULL if failed. If it failed a human-readable
	*   error description can be retrieved by calling GetErrorText()
	*
	* @note This function is not intended to be overridden. Implement InternReadFile()
	*   to do the import. If an exception is thrown somewhere in InternReadFile(),
	*   this function will catch it and transform it into a suitable response to the caller.
	*/
	aiScene* ReadFile( const std::string& pFile, IOSystem* pIOHandler);

	// -------------------------------------------------------------------
	/** Returns the error description of the last error that occured. 
	 * @return A description of the last error that occured. An empty string if no error.
	 */
	const std::string& GetErrorText() const { return mErrorText; }

protected:
	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. The function is 
	 * expected to throw an ImportErrorException if there is an error. If it
	 * terminates normally, the data in aiScene is expected to be correct.
	 * Override this function to implement the actual importing.
	 * 
	 * @param pFile Path of the file to be imported.
	 * @param pScene The scene object to hold the imported data.
	 * @param pIOHandler The IO handler to use for any file access.
	 */
	virtual void InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler) = 0;

protected:
	/** Error description in case there was one. */
	std::string mErrorText;
};

} // end of namespace Assimp

#endif // AI_BASEIMPORTER_H_INC