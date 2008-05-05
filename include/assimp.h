/** @file Defines the C-API to the Asset Import Library. */
#ifndef AI_ASSIMP_H_INC
#define AI_ASSIMP_H_INC

#ifdef __cplusplus
extern "C" {
#endif

struct aiScene;
struct aiFileIO;
//enum aiOrigin;

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
*        expected to be a null-terminated c-string.
* @param pFlags Optional post processing steps to be executed after 
*   a successful import. Provide a bitwise combination of the #aiPostProcessSteps
*   flags.
* @return Pointer to the imported data or NULL if the import failed. 
*/
// ---------------------------------------------------------------------------
const aiScene* aiImportFile( const char* pFile, unsigned int pFlags);


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
* @param pFile aiFileIO structure.  All functions pointers must be
*        initialized. aiFileIO::OpenFunc() and aiFileIO::CloseFunc()
*        will be used to open other files in the fs if the asset to be 
*        loaded depends on them.
* @return Pointer to the imported data or NULL if the import failed. 
*/
// ---------------------------------------------------------------------------
const aiScene* aiImportFileEx( const aiFileIO* pFile);



// ---------------------------------------------------------------------------
/** Releases all resources associated with the given import process.
*
* Call this function after you're done with the imported data.
* @param pScene The imported data to release.
*/
// ---------------------------------------------------------------------------
void aiReleaseImport( const aiScene* pScene);


// ---------------------------------------------------------------------------
/** Returns the error text of the last failed import process. 
*
* @return A textual description of the error that occured at the last
* import process. NULL if there was no error.
*/
// ---------------------------------------------------------------------------
const char* aiGetErrorString();

#ifdef __cplusplus
}
#endif

#endif // AI_ASSIMP_H_INC