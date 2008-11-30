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

/** @file Defines the C++-API to the Open Asset Import Library. */
#ifndef __AI_ASSIMP_HPP_INC__
#define __AI_ASSIMP_HPP_INC__

#ifndef __cplusplus
#	error This header requires C++ to be used. Use Assimp's C-API (assimp.h) \
          to access the library from C code.
#endif

#include <map>
#include <vector>

// Public ASSIMP headers
#include "aiTypes.h"
#include "aiConfig.h"
#include "aiAssert.h"

namespace Assimp
{
	// Public interface
	class Importer;
	class IOStream;
	class IOSystem;

	// Plugin development
	class BaseImporter;
	class BaseProcess;
	class SharedPostProcessInfo;
	class BatchLoader;
}

#define AI_PROPERTY_WAS_NOT_EXISTING 0xffffffff

struct aiScene;
struct aiFileIO;
extern "C" ASSIMP_API const aiScene* aiImportFileEx( const char*, unsigned int, aiFileIO*);

namespace Assimp
{

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
class ASSIMP_API Importer
{
	// used internally
	friend class BaseProcess;
	friend class BatchLoader;
	friend const aiScene* ::aiImportFileEx( const char*, unsigned int, aiFileIO*);

public:

	typedef unsigned int KeyType;
	typedef std::map<KeyType, int>  		IntPropertyMap;
	typedef std::map<KeyType, float>		FloatPropertyMap;
	typedef std::map<KeyType, std::string>	StringPropertyMap;

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
	/** Registers a new loader.
	 *
	 * @param pImp Importer to be added. The Importer instance takes 
	 *   ownership of the pointer, so it will be automatically deleted
	 *   with the Importer instance.
	 * @return AI_SUCCESS if the loader has been added. The registration
	 *   fails if there is already a loader for a specific file extension.
	 */
	aiReturn RegisterLoader(BaseImporter* pImp);


	// -------------------------------------------------------------------
	/** Unregisters a loader.
	 *
	 * @param pImp Importer to be unregistered.
	 * @return AI_SUCCESS if the loader has been removed. The function
	 *   fails if the loader is currently in use (this could happen
	 *   if the #Importer instance is used by more than one thread) or
	 *   if it has not yet been registered.
	 */
	aiReturn UnregisterLoader(BaseImporter* pImp);

#if 0
	// -------------------------------------------------------------------
	/** Registers a new post-process step.
	 *
	 * @param pImp Post-process step to be added. The Importer instance 
	 *   takes ownership of the pointer, so it will be automatically 
	 *   deleted with the Importer instance.
	 * @return AI_SUCCESS if the step has been added.
	 */
	aiReturn RegisterPPStep(BaseProcess* pImp);


	// -------------------------------------------------------------------
	/** Unregisters a post-process step.
	 *
	 * @param pImp Step to be unregistered. 
	 * @return AI_SUCCESS if the step has been removed. The function
	 *   fails if the step is currently in use (this could happen
	 *   if the #Importer instance is used by more than one thread) or
	 *   if it has not yet been registered.
	 */
	aiReturn UnregisterPPStep(BaseProcess* pImp);
#endif

	// -------------------------------------------------------------------
	/** Set an integer configuration property.
	 * @param szName Name of the property. All supported properties
	 *   are defined in the aiConfig.g header (all constants share the
	 *   prefix AI_CONFIG_XXX).
	 * @param iValue New value of the property
	 * @param bWasExisting Optional pointer to receive true if the
	 *   property was set before. The new value replaced the old value
	 *   in this case.
	 * @note Property of different types (float, int, string ..) are kept
	 *   on different stacks, so calling SetPropertyInteger() for a 
	 *   floating-point property has no effect - the loader will call
	 *   GetPropertyFloat() to read the property, but it won't be there.
	 */
	void SetPropertyInteger(const char* szName, int iValue, 
		bool* bWasExisting = NULL);

	// -------------------------------------------------------------------
	/** Set a floating-point configuration property.
	 * @see SetPropertyInteger()
	 */
	void SetPropertyFloat(const char* szName, float fValue, 
		bool* bWasExisting = NULL);

	// -------------------------------------------------------------------
	/** Set a string configuration property.
	 * @see SetPropertyInteger()
	 */
	void SetPropertyString(const char* szName, const std::string& sValue, 
		bool* bWasExisting = NULL);


	// -------------------------------------------------------------------
	/** Get a configuration property.
	 * @param szName Name of the property. All supported properties
	 *   are defined in the aiConfig.g header (all constants share the
	 *   prefix AI_CONFIG_XXX).
	 * @param iErrorReturn Value that is returned if the property 
	 *   is not found. 
	 * @return Current value of the property
	 * @note Property of different types (float, int, string ..) are kept
	 *   on different lists, so calling SetPropertyInteger() for a 
	 *   floating-point property has no effect - the loader will call
	 *   GetPropertyFloat() to read the property, but it won't be there.
	 */
	int GetPropertyInteger(const char* szName, 
		int iErrorReturn = 0xffffffff) const;

	// -------------------------------------------------------------------
	/** Get a floating-point configuration property
	 * @see GetPropertyInteger()
	 */
	float GetPropertyFloat(const char* szName, 
		float fErrorReturn = 10e10f) const;

	// -------------------------------------------------------------------
	/** Get a string configuration property
	 *
	 *  The return value remains valid until the property is modified.
	 * @see GetPropertyInteger()
	 */
	const std::string& GetPropertyString(const char* szName,
		const std::string& sErrorReturn = "") const;


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
	 *   of the Importer. NULL resets it to the default handler.
	 */
	void SetIOHandler( IOSystem* pIOHandler);


	// -------------------------------------------------------------------
	/** Retrieves the IO handler that is currently set.
	 * You can use IsDefaultIOHandler() to check whether the returned
	 * interface is the default IO handler provided by ASSIMP. The default
	 * handler is active as long the application doesn't supply its own
	 * custom IO handler via SetIOHandler().
	 * @return A valid IOSystem interface
	 */
	IOSystem* GetIOHandler();


	// -------------------------------------------------------------------
	/** Checks whether a default IO handler is active 
	 * A default handler is active as long the application doesn't 
	 * supply its own custom IO handler via SetIOHandler().
	 * @return true by default
	 */
	bool IsDefaultIOHandler();


	// -------------------------------------------------------------------
	/** Reads the given file and returns its contents if successful. 
	* 
	* If the call succeeds, the contents of the file are returned as a 
	* pointer to an aiScene object. The returned data is intended to be 
	* read-only, the importer object keeps ownership of the data and will
   	* destroy it upon destruction. If the import failes, NULL is returned.
	* A human-readable error description can be retrieved by calling 
	* GetErrorString(). The previous scene will be deleted during this call.
	* @param pFile Path and filename to the file to be imported.
	* @param pFlags Optional post processing steps to be executed after 
	*   a successful import. Provide a bitwise combination of the 
	*   #aiPostProcessSteps flags.
	* @return A pointer to the imported data, NULL if the import failed.
	*   The pointer to the scene remains in possession of the Importer
	*   instance. Use GetOrphanedScene() to take ownership of it.
	*/
	const aiScene* ReadFile( const std::string& pFile, unsigned int pFlags);


	// -------------------------------------------------------------------
	/** Returns an error description of an error that occured in ReadFile(). 
	*
	* Returns an empty string if no error occured.
	* @return A description of the last error, an empty string if no 
	*   error occured.
	*/
	inline const std::string& GetErrorString() const;


	// -------------------------------------------------------------------
	/** Returns whether a given file extension is supported by ASSIMP.
	*
	* @param szExtension Extension to be checked.
	*   Must include a trailing dot '.'. Example: ".3ds", ".md3".
	*   Cases-insensitive.
	* @return true if the extension is supported, false otherwise
	*/
	bool IsExtensionSupported(const std::string& szExtension);


	// -------------------------------------------------------------------
	/** Get a full list of all file extensions supported by ASSIMP.
	*
	* If a file extension is contained in the list this does, of course, not
	* mean that ASSIMP is able to load all files with this extension.
	* @param szOut String to receive the extension list.
	*   Format of the list: "*.3ds;*.obj;*.dae". 
	*/
	void GetExtensionList(std::string& szOut);


	// -------------------------------------------------------------------
	/** Find the loader corresponding to a specific file extension.
	*
	*  This is quite similar to IsExtensionSupported() except a
	*  BaseImporter instance is returned.
	*  @param szExtension Extension to be checke, cases insensitive,
	*    must include a trailing dot.
	*  @return NULL if there is no loader for the extension.
	*/
	BaseImporter* FindLoader (const std::string& szExtension);


	// -------------------------------------------------------------------
	/** Returns the scene loaded by the last successful call to ReadFile()
	 *
	 * @return Current scene or NULL if there is currently no scene loaded
	 */
	inline const aiScene* GetScene();


	// -------------------------------------------------------------------
	/** Returns the scene loaded by the last successful call to ReadFile()
	 *  and releases the scene from the ownership of the Importer 
	 *  instance. The application is now resposible for deleting the
	 *  scene. Any further calls to GetScene() or GetOrphanedScene()
	 *  will return NULL - until a new scene has been loaded via ReadFile().
	 *
	 * @return Current scene or NULL if there is currently no scene loaded
	 */
	inline const aiScene* GetOrphanedScene();


	// -------------------------------------------------------------------
	/** Returns the storage allocated by ASSIMP to hold the asset data
	 * in memory.
	 * \param in Data structure to be filled. 
	*/
	void GetMemoryRequirements(aiMemoryInfo& in) const;


	// -------------------------------------------------------------------
	/** Enables the "extra verbose" mode. In this mode the data 
	* structure is validated after each post-process step to make sure
	* all steps behave consequently in the same manner when modifying
	* data structures.
	*/
	inline void SetExtraVerbose(bool bDo);

private:

	/** Empty copy constructor. */
	Importer(const Importer &other);

protected:

	/** IO handler to use for all file accesses. */
	IOSystem* mIOHandler;
	bool mIsDefaultHandler;

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

	/** List of integer properties */
	IntPropertyMap mIntProperties;

	/** List of floating-point properties */
	FloatPropertyMap mFloatProperties;

	/** List of string properties */
	StringPropertyMap mStringProperties;

	/** Used for testing - extra verbose mode causes the
	    validateDataStructure-Step to be executed before
		and after every single postprocess step */
	bool bExtraVerbose;

	/** Used by post-process steps to share data */
	SharedPostProcessInfo* mPPShared;
};

// ---------------------------------------------------------------------------
// inline methods for Importer
inline const std::string& Importer::GetErrorString() const 
{ 
	return mErrorString;
}

inline void Importer::SetExtraVerbose(bool bDo)
{
	bExtraVerbose = bDo;
}

inline const aiScene* Importer::GetOrphanedScene()
{
	aiScene* scene = mScene;
	mScene = NULL;
	return scene;
}

inline const aiScene* Importer::GetScene()
{
	return mScene;
}


} // End of namespace Assimp

#endif // __AI_ASSIMP_HPP_INC
