/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

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
* implement IOSystem and IOStream and supply an instance of your custom 
* IOSystem implementation by calling SetIOHandler() before calling ReadFile().
* If you do not assign a custion IO handler, a default handler using the 
* standard C++ IO logic will be used.
*
* @note One Importer instance is not thread-safe. If you use multiple
* threads for loading each thread should manage its own Importer instance.
*/
class Importer
{
	// used internally
	friend class BaseProcess;

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
	/** Supplies a custom IO handler to the importer to use to open and
	 * access files. If you need the importer to use custion IO logic to 
	 * access the files, you need to provide a custom implementation of 
	 * IOSystem and IOFile to the importer. Then create an instance of 
	 * your custion IOSystem implementation and supply it by this function.
	 *
	 * The Importer takes ownership of the object and will destroy it 
	 * afterwards. The previously assigned handler will be deleted.
	 *
	 * @param pIOHandler The IO handler to be used in all file accesses 
	 * of the Importer. NULL resets it to the default handler.
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


	// -------------------------------------------------------------------
	/** Returns whether a given file extension is supported by ASSIMP
	*
	* @param szExtension Extension for which the function queries support.
	* Must include a leading dot '.'. Example: ".3ds", ".md3"
	* @return true if the extension is supported, false otherwise
	*/
	bool IsExtensionSupported(const std::string& szExtension);


	// -------------------------------------------------------------------
	/** Get a full list of all file extensions generally supported by ASSIMP.
	*
	* If a file extension is contained in the list this does, of course, not
	* mean that ASSIMP is able to load all files with this extension.
	* @param szOut String to receive the extension list.
	* Format of the list: "*.3ds;*.obj;*.dae". NULL is not a valid parameter.
	*/
	void GetExtensionList(std::string& szOut);


	// -------------------------------------------------------------------
	/** Returns the scene loaded by the last successful call to ReadFile()
	*
	* @return Current scene or NULL if there is currently no scene loaded
	*/
	inline const aiScene* GetScene()
		{return this->mScene;}

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