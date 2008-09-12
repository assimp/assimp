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

/** @file Defines the C-API to the Asset Import Library. */
#ifndef AI_ASSIMP_H_INC
#define AI_ASSIMP_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct aiScene;
struct aiFileIO;
struct aiString;


// ---------------------------------------------------------------------------
/** Reads the given file and returns its content.
* 
* If the call succeeds, the imported data is returned in an aiScene structure. 
* The data is intended to be read-only, it stays property of the ASSIMP 
* library and will be stable until aiReleaseImport() is called. After you're 
* done with it, call aiReleaseImport() to free the resources associated with 
* this file. If the import fails, NULL is returned instead. Call 
* aiGetErrorString() to retrieve a human-readable error text.
* @param pFile Path and filename of the file to be imported, 
*   expected to be a null-terminated c-string. NULL is not a valid value.
* @param pFlags Optional post processing steps to be executed after 
*   a successful import. Provide a bitwise combination of the 
*   #aiPostProcessSteps flags.
* @return Pointer to the imported data or NULL if the import failed. 
*/
// ---------------------------------------------------------------------------
ASSIMP_API const C_STRUCT aiScene* aiImportFile( const char* pFile, 
	unsigned int pFlags);


// ---------------------------------------------------------------------------
/** Reads the given file using user-defined I/O functions and returns 
*   its content.
* 
* If the call succeeds, the imported data is returned in an aiScene structure. 
* The data is intended to be read-only, it stays property of the ASSIMP 
* library and will be stable until aiReleaseImport() is called. After you're 
* done with it, call aiReleaseImport() to free the resources associated with 
* this file. If the import fails, NULL is returned instead. Call 
* aiGetErrorString() to retrieve a human-readable error text.
* @param pFile Path and filename of the file to be imported, 
*   expected to be a null-terminated c-string. NULL is not a valid value.
* @param pFlags Optional post processing steps to be executed after 
*   a successful import. Provide a bitwise combination of the
*   #aiPostProcessSteps flags.
* @param pFS aiFileIO structure. Will be used to open the model file itself
*   and any other files the loader needs to open.
* @return Pointer to the imported data or NULL if the import failed.  
*/
// ---------------------------------------------------------------------------
ASSIMP_API const C_STRUCT aiScene* aiImportFileEx( 
	const char* pFile, unsigned int pFlags,
	C_STRUCT aiFileIO* pFS);


// ---------------------------------------------------------------------------
/** Releases all resources associated with the given import process.
*
* Call this function after you're done with the imported data.
* @param pScene The imported data to release. NULL is a valid value.
*/
// ---------------------------------------------------------------------------
ASSIMP_API void aiReleaseImport( const C_STRUCT aiScene* pScene);


// ---------------------------------------------------------------------------
/** Returns the error text of the last failed import process. 
*
* @return A textual description of the error that occured at the last
* import process. NULL if there was no error.
*/
// ---------------------------------------------------------------------------
ASSIMP_API const char* aiGetErrorString();


// ---------------------------------------------------------------------------
/** Returns whether a given file extension is supported by ASSIMP
*
* @param szExtension Extension for which the function queries support.
* Must include a leading dot '.'. Example: ".3ds", ".md3"
* @return 1 if the extension is supported, 0 otherwise
*/
// ---------------------------------------------------------------------------
ASSIMP_API int aiIsExtensionSupported(const char* szExtension);


// ---------------------------------------------------------------------------
/** Get a full list of all file extensions generally supported by ASSIMP.
 *
 * If a file extension is contained in the list this does, of course, not
 * mean that ASSIMP is able to load all files with this extension.
 * @param szOut String to receive the extension list.
 * Format of the list: "*.3ds;*.obj;*.dae". NULL is not a valid parameter.
*/
// ---------------------------------------------------------------------------
ASSIMP_API void aiGetExtensionList(C_STRUCT aiString* szOut);


// ---------------------------------------------------------------------------
/** Get the storage required by an imported asset
 * \param pIn Input asset.
 * \param in Data structure to be filled. 
 */
// ---------------------------------------------------------------------------
ASSIMP_API void aiGetMemoryRequirements(const C_STRUCT aiScene* pIn,
	C_STRUCT aiMemoryInfo* in);


// ---------------------------------------------------------------------------
/** Set an integer property. This is the C-version of 
 *  #Importer::SetPropertyInteger(). In the C-API properties are shared by
 *  all imports. It is not possible to specify them per asset.
 *
 * \param szName Name of the configuration property to be set. All constants
 *   are defined in the aiConfig.h header file.
 * \param value New value for the property
 */
// ---------------------------------------------------------------------------
ASSIMP_API void aiSetImportPropertyInteger(const char* szName, int value);

// ---------------------------------------------------------------------------
/**  @see aiSetImportPropertyInteger()
 */
ASSIMP_API void aiSetImportPropertyFloat(const char* szName, float value);

// ---------------------------------------------------------------------------
/**  @see aiSetImportPropertyInteger()
 */
ASSIMP_API void aiSetImportPropertyString(const char* szName,
	const C_STRUCT aiString* st);


#ifdef __cplusplus
}
#endif

#endif // AI_ASSIMP_H_INC
