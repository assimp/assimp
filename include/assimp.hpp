/** @file Defines the CPP-API to the Asset Import Library. */
#ifndef AI_ASSIMP_HPP_INC
#define AI_ASSIMP_HPP_INC

#ifndef __cplusplus
#error This header requires C++ to be used.
#endif

#include <string>
#include <vector>

struct aiScene;

namespace Assimp
{

class BaseImporter;
class BaseProcess;
class IOStream;
class IOSystem;

// ---------------------------------------------------------------------------
/** The Importer class forms an C++ interface to the functionality of the 
*   Asset Import library.
*
* Create an object of this class and call ReadFile() to import a file. 
* If the import succeeds, the function returns a pointer to the imported data. 
* The data remains property of the object, it is intended to be accessed 
* read-only. The imported data will be destroyed along with the Importer 
* object. If the import failes, ReadFile() returns a NULL pointer. In this
* case you can retrieve a human-readable error description be calling 
* GetErrorString().
*
* If you need the Importer to do custom file handling to access the files,
* implement IOSystem and IOStream and supply an instance of your custom IOSystem
* implementation by calling SetIOHandler() before calling ReadFile(). If you
* do not assign a custion IO handler, a default handler using the standard C++
* IO logic will be used.
*/
class Importer
{
public:

	// -------------------------------------------------------------------
	/** Constructor. Creates an empty importer object. 
	 * 
	 * Call ReadFile() to start the import process.
	 */
	Importer();

	// -------------------------------------------------------------------
	/** Destructor. The object kept ownership of the imported data,
	 * which now will be destroyed along with the object. 
	 */
	~Importer();

	// -------------------------------------------------------------------
	/** Supplies a custom IO handler to the importer to open and access files.
	 * If you need the importer to use custion IO logic to access the files,
	 * you need to provide a custom implementation of IOSystem and IOFile
	 * to the importer. Then create an instance of your custion IOSystem
	 * implementation and supply it by this function.
	 *
	 * The Importer takes ownership of the object and will destroy it afterwards.
	 * The previously assigned handler will be deleted.
	 *
	 * @param pIOHandler The IO handler to be used in all file accesses of the Importer.
	 */
	void SetIOHandler( IOSystem* pIOHandler);

	// -------------------------------------------------------------------
	/** Reads the given file and returns its contents if successful. 
	* 
	* If the call succeeds, the contents of the file are returned as a 
	* pointer to an aiScene object. The returned data is intended to be 
	* read-only, the importer object keeps ownership of the data and will
    * destroy it upon destruction. If the import failes, NULL is returned.
	* A human-readable error description can be retrieved by calling 
	* GetErrorString().
	* @param pFile Path and filename to the file to be imported.
	* @param pFlags Optional post processing steps to be executed after 
	*   a successful import. Provide a bitwise combination of the #aiPostProcessSteps
	*   flags.
	* @return A pointer to the imported data, NULL if the import failed.
	*/
	const aiScene* ReadFile( const std::string& pFile, unsigned int pFlags);

	// -------------------------------------------------------------------
	/** Returns an error description of an error that occured in ReadFile(). 
	*
	* Returns an empty string if no error occured.
	* @return A description of the last error, an empty string if no 
	*         error occured.
	*/
	inline const std::string& GetErrorString() const 
		{ return mErrorString; }

private:
	/** Empty copy constructor. */
	Importer(const Importer &other);

protected:
	/** IO handler to use for all file accesses. */
	IOSystem* mIOHandler;

	/** Format-specific importer worker objects - 
	 * one for each format we can read. */
	std::vector<BaseImporter*> mImporter;

	/** Post processing steps we can apply at the imported data. */
	std::vector<BaseProcess*> mPostProcessingSteps;

	/** The imported data, if ReadFile() was successful,
	 * NULL otherwise. */
	aiScene* mScene;

	/** The error description, if there was one. */
	std::string mErrorString;
};

} // End of namespace Assimp

#endif // AI_ASSIMP_HPP_INC