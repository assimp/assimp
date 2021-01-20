/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

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

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

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

/** @file  cimport.h
 *  @brief Defines the C-API to the Open Asset Import Library.
 */
#pragma once
#ifndef AI_ASSIMP_H_INC
#define AI_ASSIMP_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/importerdesc.h>
#include <assimp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct aiScene;
struct aiFileIO;

typedef void (*aiLogStreamCallback)(const char * /* message */, char * /* user */);

// --------------------------------------------------------------------------------
/** C-API: Represents a log stream. A log stream receives all log messages and
 *  streams them _somewhere_.
 *  @see aiGetPredefinedLogStream
 *  @see aiAttachLogStream
 *  @see aiDetachLogStream */
// --------------------------------------------------------------------------------
struct aiLogStream {
    /** callback to be called */
    aiLogStreamCallback callback;

    /** user data to be passed to the callback */
    char *user;
};

// --------------------------------------------------------------------------------
/** C-API: Represents an opaque set of settings to be used during importing.
 *  @see aiCreatePropertyStore
 *  @see aiReleasePropertyStore
 *  @see aiImportFileExWithProperties
 *  @see aiSetPropertyInteger
 *  @see aiSetPropertyFloat
 *  @see aiSetPropertyString
 *  @see aiSetPropertyMatrix
 */
// --------------------------------------------------------------------------------
struct aiPropertyStore {
    char sentinel;
};

/** Our own C boolean type */
typedef int aiBool;

#define AI_FALSE 0
#define AI_TRUE 1

// --------------------------------------------------------------------------------
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
ASSIMP_API const C_STRUCT aiScene *aiImportFile(
        const char *pFile,
        unsigned int pFlags);

// --------------------------------------------------------------------------------
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
 *   and any other files the loader needs to open.  Pass NULL to use the default
 *   implementation.
 * @return Pointer to the imported data or NULL if the import failed.
 * @note Include <aiFileIO.h> for the definition of #aiFileIO.
 */
ASSIMP_API const C_STRUCT aiScene *aiImportFileEx(
        const char *pFile,
        unsigned int pFlags,
        C_STRUCT aiFileIO *pFS);

// --------------------------------------------------------------------------------
/** Same as #aiImportFileEx, but adds an extra parameter containing importer settings.
 *
 * @param pFile Path and filename of the file to be imported,
 *   expected to be a null-terminated c-string. NULL is not a valid value.
 * @param pFlags Optional post processing steps to be executed after
 *   a successful import. Provide a bitwise combination of the
 *   #aiPostProcessSteps flags.
 * @param pFS aiFileIO structure. Will be used to open the model file itself
 *   and any other files the loader needs to open.  Pass NULL to use the default
 *   implementation.
 * @param pProps #aiPropertyStore instance containing import settings.
 * @return Pointer to the imported data or NULL if the import failed.
 * @note Include <aiFileIO.h> for the definition of #aiFileIO.
 * @see aiImportFileEx
 */
ASSIMP_API const C_STRUCT aiScene *aiImportFileExWithProperties(
        const char *pFile,
        unsigned int pFlags,
        C_STRUCT aiFileIO *pFS,
        const C_STRUCT aiPropertyStore *pProps);

// --------------------------------------------------------------------------------
/** Reads the given file from a given memory buffer,
 *
 * If the call succeeds, the contents of the file are returned as a pointer to an
 * aiScene object. The returned data is intended to be read-only, the importer keeps
 * ownership of the data and will destroy it upon destruction. If the import fails,
 * NULL is returned.
 * A human-readable error description can be retrieved by calling aiGetErrorString().
 * @param pBuffer Pointer to the file data
 * @param pLength Length of pBuffer, in bytes
 * @param pFlags Optional post processing steps to be executed after
 *   a successful import. Provide a bitwise combination of the
 *   #aiPostProcessSteps flags. If you wish to inspect the imported
 *   scene first in order to fine-tune your post-processing setup,
 *   consider to use #aiApplyPostProcessing().
 * @param pHint An additional hint to the library. If this is a non empty string,
 *   the library looks for a loader to support the file extension specified by pHint
 *   and passes the file to the first matching loader. If this loader is unable to
 *   completely the request, the library continues and tries to determine the file
 *   format on its own, a task that may or may not be successful.
 *   Check the return value, and you'll know ...
 * @return A pointer to the imported data, NULL if the import failed.
 *
 * @note This is a straightforward way to decode models from memory
 * buffers, but it doesn't handle model formats that spread their
 * data across multiple files or even directories. Examples include
 * OBJ or MD3, which outsource parts of their material info into
 * external scripts. If you need full functionality, provide
 * a custom IOSystem to make Assimp find these files and use
 * the regular aiImportFileEx()/aiImportFileExWithProperties() API.
 */
ASSIMP_API const C_STRUCT aiScene *aiImportFileFromMemory(
        const char *pBuffer,
        unsigned int pLength,
        unsigned int pFlags,
        const char *pHint);

// --------------------------------------------------------------------------------
/** Same as #aiImportFileFromMemory, but adds an extra parameter containing importer settings.
 *
 * @param pBuffer Pointer to the file data
 * @param pLength Length of pBuffer, in bytes
 * @param pFlags Optional post processing steps to be executed after
 *   a successful import. Provide a bitwise combination of the
 *   #aiPostProcessSteps flags. If you wish to inspect the imported
 *   scene first in order to fine-tune your post-processing setup,
 *   consider to use #aiApplyPostProcessing().
 * @param pHint An additional hint to the library. If this is a non empty string,
 *   the library looks for a loader to support the file extension specified by pHint
 *   and passes the file to the first matching loader. If this loader is unable to
 *   completely the request, the library continues and tries to determine the file
 *   format on its own, a task that may or may not be successful.
 *   Check the return value, and you'll know ...
 * @param pProps #aiPropertyStore instance containing import settings.
 * @return A pointer to the imported data, NULL if the import failed.
 *
 * @note This is a straightforward way to decode models from memory
 * buffers, but it doesn't handle model formats that spread their
 * data across multiple files or even directories. Examples include
 * OBJ or MD3, which outsource parts of their material info into
 * external scripts. If you need full functionality, provide
 * a custom IOSystem to make Assimp find these files and use
 * the regular aiImportFileEx()/aiImportFileExWithProperties() API.
 * @see aiImportFileFromMemory
 */
ASSIMP_API const C_STRUCT aiScene *aiImportFileFromMemoryWithProperties(
        const char *pBuffer,
        unsigned int pLength,
        unsigned int pFlags,
        const char *pHint,
        const C_STRUCT aiPropertyStore *pProps);

// --------------------------------------------------------------------------------
/** Apply post-processing to an already-imported scene.
 *
 * This is strictly equivalent to calling #aiImportFile()/#aiImportFileEx with the
 * same flags. However, you can use this separate function to inspect the imported
 * scene first to fine-tune your post-processing setup.
 * @param pScene Scene to work on.
 * @param pFlags Provide a bitwise combination of the #aiPostProcessSteps flags.
 * @return A pointer to the post-processed data. Post processing is done in-place,
 *   meaning this is still the same #aiScene which you passed for pScene. However,
 *   _if_ post-processing failed, the scene could now be NULL. That's quite a rare
 *   case, post processing steps are not really designed to 'fail'. To be exact,
 *   the #aiProcess_ValidateDataStructure flag is currently the only post processing step
 *   which can actually cause the scene to be reset to NULL.
 */
ASSIMP_API const C_STRUCT aiScene *aiApplyPostProcessing(
        const C_STRUCT aiScene *pScene,
        unsigned int pFlags);

// --------------------------------------------------------------------------------
/** Get one of the predefine log streams. This is the quick'n'easy solution to
 *  access Assimp's log system. Attaching a log stream can slightly reduce Assimp's
 *  overall import performance.
 *
 *  Usage is rather simple (this will stream the log to a file, named log.txt, and
 *  the stdout stream of the process:
 *  @code
 *    struct aiLogStream c;
 *    c = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"log.txt");
 *    aiAttachLogStream(&c);
 *    c = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
 *    aiAttachLogStream(&c);
 *  @endcode
 *
 *  @param pStreams One of the #aiDefaultLogStream enumerated values.
 *  @param file Solely for the #aiDefaultLogStream_FILE flag: specifies the file to write to.
 *    Pass NULL for all other flags.
 *  @return The log stream. callback is set to NULL if something went wrong.
 */
ASSIMP_API C_STRUCT aiLogStream aiGetPredefinedLogStream(
        C_ENUM aiDefaultLogStream pStreams,
        const char *file);

// --------------------------------------------------------------------------------
/** Attach a custom log stream to the libraries' logging system.
 *
 *  Attaching a log stream can slightly reduce Assimp's overall import
 *  performance. Multiple log-streams can be attached.
 *  @param stream Describes the new log stream.
 *  @note To ensure proper destruction of the logging system, you need to manually
 *    call aiDetachLogStream() on every single log stream you attach.
 *    Alternatively (for the lazy folks) #aiDetachAllLogStreams is provided.
 */
ASSIMP_API void aiAttachLogStream(
        const C_STRUCT aiLogStream *stream);

// --------------------------------------------------------------------------------
/** Enable verbose logging. Verbose logging includes debug-related stuff and
 *  detailed import statistics. This can have severe impact on import performance
 *  and memory consumption. However, it might be useful to find out why a file
 *  didn't read correctly.
 *  @param d AI_TRUE or AI_FALSE, your decision.
 */
ASSIMP_API void aiEnableVerboseLogging(aiBool d);

// --------------------------------------------------------------------------------
/** Detach a custom log stream from the libraries' logging system.
 *
 *  This is the counterpart of #aiAttachLogStream. If you attached a stream,
 *  don't forget to detach it again.
 *  @param stream The log stream to be detached.
 *  @return AI_SUCCESS if the log stream has been detached successfully.
 *  @see aiDetachAllLogStreams
 */
ASSIMP_API C_ENUM aiReturn aiDetachLogStream(
        const C_STRUCT aiLogStream *stream);

// --------------------------------------------------------------------------------
/** Detach all active log streams from the libraries' logging system.
 *  This ensures that the logging system is terminated properly and all
 *  resources allocated by it are actually freed. If you attached a stream,
 *  don't forget to detach it again.
 *  @see aiAttachLogStream
 *  @see aiDetachLogStream
 */
ASSIMP_API void aiDetachAllLogStreams(void);

// --------------------------------------------------------------------------------
/** Releases all resources associated with the given import process.
 *
 * Call this function after you're done with the imported data.
 * @param pScene The imported data to release. NULL is a valid value.
 */
ASSIMP_API void aiReleaseImport(
        const C_STRUCT aiScene *pScene);

// --------------------------------------------------------------------------------
/** Returns the error text of the last failed import process.
 *
 * @return A textual description of the error that occurred at the last
 * import process. NULL if there was no error. There can't be an error if you
 * got a non-NULL #aiScene from #aiImportFile/#aiImportFileEx/#aiApplyPostProcessing.
 */
ASSIMP_API const char *aiGetErrorString(void);

// --------------------------------------------------------------------------------
/** Returns whether a given file extension is supported by ASSIMP
 *
 * @param szExtension Extension for which the function queries support for.
 * Must include a leading dot '.'. Example: ".3ds", ".md3"
 * @return AI_TRUE if the file extension is supported.
 */
ASSIMP_API aiBool aiIsExtensionSupported(
        const char *szExtension);

// --------------------------------------------------------------------------------
/** Get a list of all file extensions supported by ASSIMP.
 *
 * If a file extension is contained in the list this does, of course, not
 * mean that ASSIMP is able to load all files with this extension.
 * @param szOut String to receive the extension list.
 * Format of the list: "*.3ds;*.obj;*.dae". NULL is not a valid parameter.
 */
ASSIMP_API void aiGetExtensionList(
        C_STRUCT aiString *szOut);

// --------------------------------------------------------------------------------
/** Get the approximated storage required by an imported asset
 * @param pIn Input asset.
 * @param in Data structure to be filled.
 */
ASSIMP_API void aiGetMemoryRequirements(
        const C_STRUCT aiScene *pIn,
        C_STRUCT aiMemoryInfo *in);

// --------------------------------------------------------------------------------
/** Create an empty property store. Property stores are used to collect import
 *  settings.
 * @return New property store. Property stores need to be manually destroyed using
 *   the #aiReleasePropertyStore API function.
 */
ASSIMP_API C_STRUCT aiPropertyStore *aiCreatePropertyStore(void);

// --------------------------------------------------------------------------------
/** Delete a property store.
 * @param p Property store to be deleted.
 */
ASSIMP_API void aiReleasePropertyStore(C_STRUCT aiPropertyStore *p);

// --------------------------------------------------------------------------------
/** Set an integer property.
 *
 *  This is the C-version of #Assimp::Importer::SetPropertyInteger(). In the C
 *  interface, properties are always shared by all imports. It is not possible to
 *  specify them per import.
 *
 * @param store Store to modify. Use #aiCreatePropertyStore to obtain a store.
 * @param szName Name of the configuration property to be set. All supported
 *   public properties are defined in the config.h header file (AI_CONFIG_XXX).
 * @param value New value for the property
 */
ASSIMP_API void aiSetImportPropertyInteger(
        C_STRUCT aiPropertyStore *store,
        const char *szName,
        int value);

// --------------------------------------------------------------------------------
/** Set a floating-point property.
 *
 *  This is the C-version of #Assimp::Importer::SetPropertyFloat(). In the C
 *  interface, properties are always shared by all imports. It is not possible to
 *  specify them per import.
 *
 * @param store Store to modify. Use #aiCreatePropertyStore to obtain a store.
 * @param szName Name of the configuration property to be set. All supported
 *   public properties are defined in the config.h header file (AI_CONFIG_XXX).
 * @param value New value for the property
 */
ASSIMP_API void aiSetImportPropertyFloat(
        C_STRUCT aiPropertyStore *store,
        const char *szName,
        ai_real value);

// --------------------------------------------------------------------------------
/** Set a string property.
 *
 *  This is the C-version of #Assimp::Importer::SetPropertyString(). In the C
 *  interface, properties are always shared by all imports. It is not possible to
 *  specify them per import.
 *
 * @param store Store to modify. Use #aiCreatePropertyStore to obtain a store.
 * @param szName Name of the configuration property to be set. All supported
 *   public properties are defined in the config.h header file (AI_CONFIG_XXX).
 * @param st New value for the property
 */
ASSIMP_API void aiSetImportPropertyString(
        C_STRUCT aiPropertyStore *store,
        const char *szName,
        const C_STRUCT aiString *st);

// --------------------------------------------------------------------------------
/** Set a matrix property.
 *
 *  This is the C-version of #Assimp::Importer::SetPropertyMatrix(). In the C
 *  interface, properties are always shared by all imports. It is not possible to
 *  specify them per import.
 *
 * @param store Store to modify. Use #aiCreatePropertyStore to obtain a store.
 * @param szName Name of the configuration property to be set. All supported
 *   public properties are defined in the config.h header file (AI_CONFIG_XXX).
 * @param mat New value for the property
 */
ASSIMP_API void aiSetImportPropertyMatrix(
        C_STRUCT aiPropertyStore *store,
        const char *szName,
        const C_STRUCT aiMatrix4x4 *mat);

// --------------------------------------------------------------------------------
/** Construct a quaternion from a 3x3 rotation matrix.
 *  @param quat Receives the output quaternion.
 *  @param mat Matrix to 'quaternionize'.
 *  @see aiQuaternion(const aiMatrix3x3& pRotMatrix)
 */
ASSIMP_API void aiCreateQuaternionFromMatrix(
        C_STRUCT aiQuaternion *quat,
        const C_STRUCT aiMatrix3x3 *mat);

// --------------------------------------------------------------------------------
/** Decompose a transformation matrix into its rotational, translational and
 *  scaling components.
 *
 * @param mat Matrix to decompose
 * @param scaling Receives the scaling component
 * @param rotation Receives the rotational component
 * @param position Receives the translational component.
 * @see aiMatrix4x4::Decompose (aiVector3D&, aiQuaternion&, aiVector3D&) const;
 */
ASSIMP_API void aiDecomposeMatrix(
        const C_STRUCT aiMatrix4x4 *mat,
        C_STRUCT aiVector3D *scaling,
        C_STRUCT aiQuaternion *rotation,
        C_STRUCT aiVector3D *position);

// --------------------------------------------------------------------------------
/** Transpose a 4x4 matrix.
 *  @param mat Pointer to the matrix to be transposed
 */
ASSIMP_API void aiTransposeMatrix4(
        C_STRUCT aiMatrix4x4 *mat);

// --------------------------------------------------------------------------------
/** Transpose a 3x3 matrix.
 *  @param mat Pointer to the matrix to be transposed
 */
ASSIMP_API void aiTransposeMatrix3(
        C_STRUCT aiMatrix3x3 *mat);

// --------------------------------------------------------------------------------
/** Transform a vector by a 3x3 matrix
 *  @param vec Vector to be transformed.
 *  @param mat Matrix to transform the vector with.
 */
ASSIMP_API void aiTransformVecByMatrix3(
        C_STRUCT aiVector3D *vec,
        const C_STRUCT aiMatrix3x3 *mat);

// --------------------------------------------------------------------------------
/** Transform a vector by a 4x4 matrix
 *  @param vec Vector to be transformed.
 *  @param mat Matrix to transform the vector with.
 */
ASSIMP_API void aiTransformVecByMatrix4(
        C_STRUCT aiVector3D *vec,
        const C_STRUCT aiMatrix4x4 *mat);

// --------------------------------------------------------------------------------
/** Multiply two 4x4 matrices.
 *  @param dst First factor, receives result.
 *  @param src Matrix to be multiplied with 'dst'.
 */
ASSIMP_API void aiMultiplyMatrix4(
        C_STRUCT aiMatrix4x4 *dst,
        const C_STRUCT aiMatrix4x4 *src);

// --------------------------------------------------------------------------------
/** Multiply two 3x3 matrices.
 *  @param dst First factor, receives result.
 *  @param src Matrix to be multiplied with 'dst'.
 */
ASSIMP_API void aiMultiplyMatrix3(
        C_STRUCT aiMatrix3x3 *dst,
        const C_STRUCT aiMatrix3x3 *src);

// --------------------------------------------------------------------------------
/** Get a 3x3 identity matrix.
 *  @param mat Matrix to receive its personal identity
 */
ASSIMP_API void aiIdentityMatrix3(
        C_STRUCT aiMatrix3x3 *mat);

// --------------------------------------------------------------------------------
/** Get a 4x4 identity matrix.
 *  @param mat Matrix to receive its personal identity
 */
ASSIMP_API void aiIdentityMatrix4(
        C_STRUCT aiMatrix4x4 *mat);

// --------------------------------------------------------------------------------
/** Returns the number of import file formats available in the current Assimp build.
 * Use aiGetImportFormatDescription() to retrieve infos of a specific import format.
 */
ASSIMP_API size_t aiGetImportFormatCount(void);

// --------------------------------------------------------------------------------
/** Returns a description of the nth import file format. Use #aiGetImportFormatCount()
 * to learn how many import formats are supported.
 * @param pIndex Index of the import format to retrieve information for. Valid range is
 *    0 to #aiGetImportFormatCount()
 * @return A description of that specific import format. NULL if pIndex is out of range.
 */
ASSIMP_API const C_STRUCT aiImporterDesc *aiGetImportFormatDescription(size_t pIndex);

// --------------------------------------------------------------------------------
/** Check if 2D vectors are equal.
 *  @param a First vector to compare
 *  @param b Second vector to compare
 *  @return 1 if the vectors are equal
 *  @return 0 if the vectors are not equal
 */
ASSIMP_API int aiVector2AreEqual(
        const C_STRUCT aiVector2D *a,
        const C_STRUCT aiVector2D *b);

// --------------------------------------------------------------------------------
/** Check if 2D vectors are equal using epsilon.
 *  @param a First vector to compare
 *  @param b Second vector to compare
 *  @param epsilon Epsilon
 *  @return 1 if the vectors are equal
 *  @return 0 if the vectors are not equal
 */
ASSIMP_API int aiVector2AreEqualEpsilon(
        const C_STRUCT aiVector2D *a,
        const C_STRUCT aiVector2D *b,
        const float epsilon);

// --------------------------------------------------------------------------------
/** Add 2D vectors.
 *  @param dst First addend, receives result.
 *  @param src Vector to be added to 'dst'.
 */
ASSIMP_API void aiVector2Add(
        C_STRUCT aiVector2D *dst,
        const C_STRUCT aiVector2D *src);

// --------------------------------------------------------------------------------
/** Subtract 2D vectors.
 *  @param dst Minuend, receives result.
 *  @param src Vector to be subtracted from 'dst'.
 */
ASSIMP_API void aiVector2Subtract(
        C_STRUCT aiVector2D *dst,
        const C_STRUCT aiVector2D *src);

// --------------------------------------------------------------------------------
/** Multiply a 2D vector by a scalar.
 *  @param dst Vector to be scaled by \p s
 *  @param s Scale factor
 */
ASSIMP_API void aiVector2Scale(
        C_STRUCT aiVector2D *dst,
        const float s);

// --------------------------------------------------------------------------------
/** Multiply each component of a 2D vector with
 *  the components of another vector.
 *  @param dst First vector, receives result
 *  @param other Second vector
 */
ASSIMP_API void aiVector2SymMul(
        C_STRUCT aiVector2D *dst,
        const C_STRUCT aiVector2D *other);

// --------------------------------------------------------------------------------
/** Divide a 2D vector by a scalar.
 *  @param dst Vector to be divided by \p s
 *  @param s Scalar divisor
 */
ASSIMP_API void aiVector2DivideByScalar(
        C_STRUCT aiVector2D *dst,
        const float s);

// --------------------------------------------------------------------------------
/** Divide each component of a 2D vector by
 *  the components of another vector.
 *  @param dst Vector as the dividend
 *  @param v Vector as the divisor
 */
ASSIMP_API void aiVector2DivideByVector(
        C_STRUCT aiVector2D *dst,
        C_STRUCT aiVector2D *v);

// --------------------------------------------------------------------------------
/** Get the length of a 2D vector.
 *  @return v Vector to evaluate
 */
ASSIMP_API float aiVector2Length(
        const C_STRUCT aiVector2D *v);

// --------------------------------------------------------------------------------
/** Get the squared length of a 2D vector.
 *  @return v Vector to evaluate
 */
ASSIMP_API float aiVector2SquareLength(
        const C_STRUCT aiVector2D *v);

// --------------------------------------------------------------------------------
/** Negate a 2D vector.
 *  @param dst Vector to be negated
 */
ASSIMP_API void aiVector2Negate(
        C_STRUCT aiVector2D *dst);

// --------------------------------------------------------------------------------
/** Get the dot product of 2D vectors.
 *  @param a First vector
 *  @param b Second vector
 *  @return The dot product of vectors
 */
ASSIMP_API float aiVector2DotProduct(
        const C_STRUCT aiVector2D *a,
        const C_STRUCT aiVector2D *b);

// --------------------------------------------------------------------------------
/** Normalize a 2D vector.
 *  @param v Vector to normalize
 */
ASSIMP_API void aiVector2Normalize(
        C_STRUCT aiVector2D *v);

// --------------------------------------------------------------------------------
/** Check if 3D vectors are equal.
 *  @param a First vector to compare
 *  @param b Second vector to compare
 *  @return 1 if the vectors are equal
 *  @return 0 if the vectors are not equal
 */
ASSIMP_API int aiVector3AreEqual(
        const C_STRUCT aiVector3D *a,
        const C_STRUCT aiVector3D *b);

// --------------------------------------------------------------------------------
/** Check if 3D vectors are equal using epsilon.
 *  @param a First vector to compare
 *  @param b Second vector to compare
 *  @param epsilon Epsilon
 *  @return 1 if the vectors are equal
 *  @return 0 if the vectors are not equal
 */
ASSIMP_API int aiVector3AreEqualEpsilon(
        const C_STRUCT aiVector3D *a,
        const C_STRUCT aiVector3D *b,
        const float epsilon);

// --------------------------------------------------------------------------------
/** Check if vector \p a is less than vector \p b.
 *  @param a First vector to compare
 *  @param b Second vector to compare
 *  @param epsilon Epsilon
 *  @return 1 if \p a is less than \p b
 *  @return 0 if \p a is equal or greater than \p b
 */
ASSIMP_API int aiVector3LessThan(
        const C_STRUCT aiVector3D *a,
        const C_STRUCT aiVector3D *b);

// --------------------------------------------------------------------------------
/** Add 3D vectors.
 *  @param dst First addend, receives result.
 *  @param src Vector to be added to 'dst'.
 */
ASSIMP_API void aiVector3Add(
        C_STRUCT aiVector3D *dst,
        const C_STRUCT aiVector3D *src);

// --------------------------------------------------------------------------------
/** Subtract 3D vectors.
 *  @param dst Minuend, receives result.
 *  @param src Vector to be subtracted from 'dst'.
 */
ASSIMP_API void aiVector3Subtract(
        C_STRUCT aiVector3D *dst,
        const C_STRUCT aiVector3D *src);

// --------------------------------------------------------------------------------
/** Multiply a 3D vector by a scalar.
 *  @param dst Vector to be scaled by \p s
 *  @param s Scale factor
 */
ASSIMP_API void aiVector3Scale(
        C_STRUCT aiVector3D *dst,
        const float s);

// --------------------------------------------------------------------------------
/** Multiply each component of a 3D vector with
 *  the components of another vector.
 *  @param dst First vector, receives result
 *  @param other Second vector
 */
ASSIMP_API void aiVector3SymMul(
        C_STRUCT aiVector3D *dst,
        const C_STRUCT aiVector3D *other);

// --------------------------------------------------------------------------------
/** Divide a 3D vector by a scalar.
 *  @param dst Vector to be divided by \p s
 *  @param s Scalar divisor
 */
ASSIMP_API void aiVector3DivideByScalar(
        C_STRUCT aiVector3D *dst,
        const float s);

// --------------------------------------------------------------------------------
/** Divide each component of a 3D vector by
 *  the components of another vector.
 *  @param dst Vector as the dividend
 *  @param v Vector as the divisor
 */
ASSIMP_API void aiVector3DivideByVector(
        C_STRUCT aiVector3D *dst,
        C_STRUCT aiVector3D *v);

// --------------------------------------------------------------------------------
/** Get the length of a 3D vector.
 *  @return v Vector to evaluate
 */
ASSIMP_API float aiVector3Length(
        const C_STRUCT aiVector3D *v);

// --------------------------------------------------------------------------------
/** Get the squared length of a 3D vector.
 *  @return v Vector to evaluate
 */
ASSIMP_API float aiVector3SquareLength(
        const C_STRUCT aiVector3D *v);

// --------------------------------------------------------------------------------
/** Negate a 3D vector.
 *  @param dst Vector to be negated
 */
ASSIMP_API void aiVector3Negate(
        C_STRUCT aiVector3D *dst);

// --------------------------------------------------------------------------------
/** Get the dot product of 3D vectors.
 *  @param a First vector
 *  @param b Second vector
 *  @return The dot product of vectors
 */
ASSIMP_API float aiVector3DotProduct(
        const C_STRUCT aiVector3D *a,
        const C_STRUCT aiVector3D *b);

// --------------------------------------------------------------------------------
/** Get cross product of 3D vectors.
 *  @param dst Vector to receive the result.
 *  @param a First vector
 *  @param b Second vector
 *  @return The dot product of vectors
 */
ASSIMP_API void aiVector3CrossProduct(
        C_STRUCT aiVector3D *dst,
        const C_STRUCT aiVector3D *a,
        const C_STRUCT aiVector3D *b);

// --------------------------------------------------------------------------------
/** Normalize a 3D vector.
 *  @param v Vector to normalize
 */
ASSIMP_API void aiVector3Normalize(
        C_STRUCT aiVector3D *v);

// --------------------------------------------------------------------------------
/** Check for division by zero and normalize a 3D vector.
 *  @param v Vector to normalize
 */
ASSIMP_API void aiVector3NormalizeSafe(
        C_STRUCT aiVector3D *v);

// --------------------------------------------------------------------------------
/** Rotate a 3D vector by a quaternion.
 *  @param v The vector to rotate by \p q
 *  @param q Quaternion to use to rotate \p v
 */
ASSIMP_API void aiVector3RotateByQuaternion(
        C_STRUCT aiVector3D *v,
        const C_STRUCT aiQuaternion *q);

// --------------------------------------------------------------------------------
/** Construct a 3x3 matrix from a 4x4 matrix.
 *  @param dst Receives the output matrix
 *  @param mat The 4x4 matrix to use
 */
ASSIMP_API void aiMatrix3FromMatrix4(
        C_STRUCT aiMatrix3x3 *dst,
        const C_STRUCT aiMatrix4x4 *mat);

// --------------------------------------------------------------------------------
/** Construct a 3x3 matrix from a quaternion.
 *  @param mat Receives the output matrix
 *  @param q The quaternion matrix to use
 */
ASSIMP_API void aiMatrix3FromQuaternion(
        C_STRUCT aiMatrix3x3 *mat,
        const C_STRUCT aiQuaternion *q);

// --------------------------------------------------------------------------------
/** Check if 3x3 matrices are equal.
 *  @param a First matrix to compare
 *  @param b Second matrix to compare
 *  @return 1 if the matrices are equal
 *  @return 0 if the matrices are not equal
 */
ASSIMP_API int aiMatrix3AreEqual(
        const C_STRUCT aiMatrix3x3 *a,
        const C_STRUCT aiMatrix3x3 *b);

// --------------------------------------------------------------------------------
/** Check if 3x3 matrices are equal.
 *  @param a First matrix to compare
 *  @param b Second matrix to compare
 *  @param epsilon Epsilon
 *  @return 1 if the matrices are equal
 *  @return 0 if the matrices are not equal
 */
ASSIMP_API int aiMatrix3AreEqualEpsilon(
        const C_STRUCT aiMatrix3x3 *a,
        const C_STRUCT aiMatrix3x3 *b,
        const float epsilon);

// --------------------------------------------------------------------------------
/** Invert a 3x3 matrix.
 *  @param mat Matrix to invert
 */
ASSIMP_API void aiMatrix3Inverse(
        C_STRUCT aiMatrix3x3 *mat);

// --------------------------------------------------------------------------------
/** Get the determinant of a 3x3 matrix.
 *  @param mat Matrix to get the determinant from
 */
ASSIMP_API float aiMatrix3Determinant(
        const C_STRUCT aiMatrix3x3 *mat);

// --------------------------------------------------------------------------------
/** Get a 3x3 rotation matrix around the Z axis.
 *  @param mat Receives the output matrix 
 *  @param angle Rotation angle, in radians
 */
ASSIMP_API void aiMatrix3RotationZ(
        C_STRUCT aiMatrix3x3 *mat,
        const float angle);

// --------------------------------------------------------------------------------
/** Returns a 3x3 rotation matrix for a rotation around an arbitrary axis.
 *  @param mat Receives the output matrix 
 *  @param axis Rotation axis, should be a normalized vector
 *  @param angle Rotation angle, in radians
 */
ASSIMP_API void aiMatrix3FromRotationAroundAxis(
        C_STRUCT aiMatrix3x3 *mat,
        const C_STRUCT aiVector3D *axis,
        const float angle);

// --------------------------------------------------------------------------------
/** Get a 3x3 translation matrix.
 *  @param mat Receives the output matrix 
 *  @param translation The translation vector
 */
ASSIMP_API void aiMatrix3Translation(
        C_STRUCT aiMatrix3x3 *mat,
        const C_STRUCT aiVector2D *translation);

// --------------------------------------------------------------------------------
/** Create a 3x3 matrix that rotates one vector to another vector.
 *  @param mat Receives the output matrix 
 *  @param from Vector to rotate from
 *  @param to Vector to rotate to
 */
ASSIMP_API void aiMatrix3FromTo(
        C_STRUCT aiMatrix3x3 *mat,
        const C_STRUCT aiVector3D *from,
        const C_STRUCT aiVector3D *to);

// --------------------------------------------------------------------------------
/** Construct a 4x4 matrix from a 3x3 matrix.
 *  @param dst Receives the output matrix
 *  @param mat The 3x3 matrix to use
 */
ASSIMP_API void aiMatrix4FromMatrix3(
        C_STRUCT aiMatrix4x4 *dst,
        const C_STRUCT aiMatrix3x3 *mat);

// --------------------------------------------------------------------------------
/** Construct a 4x4 matrix from scaling, rotation and position.
 *  @param mat Receives the output matrix.
 *  @param scaling The scaling for the x,y,z axes
 *  @param rotation The rotation as a hamilton quaternion
 *  @param position The position for the x,y,z axes
 */
ASSIMP_API void aiMatrix4FromScalingQuaternionPosition(
        C_STRUCT aiMatrix4x4 *mat,
        const C_STRUCT aiVector3D *scaling,
        const C_STRUCT aiQuaternion *rotation,
        const C_STRUCT aiVector3D *position);

// --------------------------------------------------------------------------------
/** Add 4x4 matrices.
 *  @param dst First addend, receives result.
 *  @param src Matrix to be added to 'dst'.
 */
ASSIMP_API void aiMatrix4Add(
        C_STRUCT aiMatrix4x4 *dst,
        const C_STRUCT aiMatrix4x4 *src);

// --------------------------------------------------------------------------------
/** Check if 4x4 matrices are equal.
 *  @param a First matrix to compare
 *  @param b Second matrix to compare
 *  @return 1 if the matrices are equal
 *  @return 0 if the matrices are not equal
 */
ASSIMP_API int aiMatrix4AreEqual(
        const C_STRUCT aiMatrix4x4 *a,
        const C_STRUCT aiMatrix4x4 *b);

// --------------------------------------------------------------------------------
/** Check if 4x4 matrices are equal.
 *  @param a First matrix to compare
 *  @param b Second matrix to compare
 *  @param epsilon Epsilon
 *  @return 1 if the matrices are equal
 *  @return 0 if the matrices are not equal
 */
ASSIMP_API int aiMatrix4AreEqualEpsilon(
        const C_STRUCT aiMatrix4x4 *a,
        const C_STRUCT aiMatrix4x4 *b,
        const float epsilon);

// --------------------------------------------------------------------------------
/** Invert a 4x4 matrix.
 *  @param result Matrix to invert
 */
ASSIMP_API void aiMatrix4Inverse(
        C_STRUCT aiMatrix4x4 *mat);

// --------------------------------------------------------------------------------
/** Get the determinant of a 4x4 matrix.
 *  @param mat Matrix to get the determinant from
 *  @return The determinant of the matrix
 */
ASSIMP_API float aiMatrix4Determinant(
        const C_STRUCT aiMatrix4x4 *mat);

// --------------------------------------------------------------------------------
/** Returns true of the matrix is the identity matrix.
 *  @param mat Matrix to get the determinant from
 *  @return 1 if \p mat is an identity matrix.
 *  @return 0 if \p mat is not an identity matrix.
 */
ASSIMP_API int aiMatrix4IsIdentity(
        const C_STRUCT aiMatrix4x4 *mat);

// --------------------------------------------------------------------------------
/** Decompose a transformation matrix into its scaling,
 *  rotational as euler angles, and translational components.
 *
 * @param mat Matrix to decompose
 * @param scaling Receives the output scaling for the x,y,z axes
 * @param rotation Receives the output rotation as a Euler angles
 * @param position Receives the output position for the x,y,z axes
 */
ASSIMP_API void aiMatrix4DecomposeIntoScalingEulerAnglesPosition(
        const C_STRUCT aiMatrix4x4 *mat,
        C_STRUCT aiVector3D *scaling,
        C_STRUCT aiVector3D *rotation,
        C_STRUCT aiVector3D *position);

// --------------------------------------------------------------------------------
/** Decompose a transformation matrix into its scaling,
 *  rotational split into an axis and rotational angle,
 *  and it's translational components.
 *
 * @param mat Matrix to decompose
 * @param rotation Receives the rotational component
 * @param axis Receives the output rotation axis
 * @param angle Receives the output rotation angle
 * @param position Receives the output position for the x,y,z axes.
 */
ASSIMP_API void aiMatrix4DecomposeIntoScalingAxisAnglePosition(
        const C_STRUCT aiMatrix4x4 *mat,
        C_STRUCT aiVector3D *scaling,
        C_STRUCT aiVector3D *axis,
        ai_real *angle,
        C_STRUCT aiVector3D *position);

// --------------------------------------------------------------------------------
/** Decompose a transformation matrix into its rotational and
 *  translational components.
 *
 * @param mat Matrix to decompose
 * @param rotation Receives the rotational component
 * @param position Receives the translational component.
 */
ASSIMP_API void aiMatrix4DecomposeNoScaling(
        const C_STRUCT aiMatrix4x4 *mat,
        C_STRUCT aiQuaternion *rotation,
        C_STRUCT aiVector3D *position);

// --------------------------------------------------------------------------------
/** Creates a 4x4 matrix from a set of euler angles.
 *  @param mat Receives the output matrix 
 *  @param x Rotation angle for the x-axis, in radians
 *  @param y Rotation angle for the y-axis, in radians
 *  @param z Rotation angle for the z-axis, in radians
 */
ASSIMP_API void aiMatrix4FromEulerAngles(
        C_STRUCT aiMatrix4x4 *mat,
        float x, float y, float z);

// --------------------------------------------------------------------------------
/** Get a 4x4 rotation matrix around the X axis.
 *  @param mat Receives the output matrix
 *  @param angle Rotation angle, in radians
 */
ASSIMP_API void aiMatrix4RotationX(
        C_STRUCT aiMatrix4x4 *mat,
        const float angle);

// --------------------------------------------------------------------------------
/** Get a 4x4 rotation matrix around the Y axis.
 *  @param mat Receives the output matrix
 *  @param angle Rotation angle, in radians
 */
ASSIMP_API void aiMatrix4RotationY(
        C_STRUCT aiMatrix4x4 *mat,
        const float angle);

// --------------------------------------------------------------------------------
/** Get a 4x4 rotation matrix around the Z axis.
 *  @param mat Receives the output matrix
 *  @param angle Rotation angle, in radians
 */
ASSIMP_API void aiMatrix4RotationZ(
        C_STRUCT aiMatrix4x4 *mat,
        const float angle);

// --------------------------------------------------------------------------------
/** Returns a 4x4 rotation matrix for a rotation around an arbitrary axis.
 *  @param mat Receives the output matrix
 *  @param axis Rotation axis, should be a normalized vector
 *  @param angle Rotation angle, in radians
 */
ASSIMP_API void aiMatrix4FromRotationAroundAxis(
        C_STRUCT aiMatrix4x4 *mat,
        const C_STRUCT aiVector3D *axis,
        const float angle);

// --------------------------------------------------------------------------------
/** Get a 4x4 translation matrix.
 *  @param mat Receives the output matrix
 *  @param translation The translation vector
 */
ASSIMP_API void aiMatrix4Translation(
        C_STRUCT aiMatrix4x4 *mat,
        const C_STRUCT aiVector3D *translation);

// --------------------------------------------------------------------------------
/** Get a 4x4 scaling matrix.
 *  @param mat Receives the output matrix
 *  @param scaling The scaling vector
 */
ASSIMP_API void aiMatrix4Scaling(
        C_STRUCT aiMatrix4x4 *mat,
        const C_STRUCT aiVector3D *scaling);

// --------------------------------------------------------------------------------
/** Create a 4x4 matrix that rotates one vector to another vector.
 *  @param mat Receives the output matrix
 *  @param from Vector to rotate from
 *  @param to Vector to rotate to
 */
ASSIMP_API void aiMatrix4FromTo(
        C_STRUCT aiMatrix4x4 *mat,
        const C_STRUCT aiVector3D *from,
        const C_STRUCT aiVector3D *to);

// --------------------------------------------------------------------------------
/** Create a Quaternion from euler angles.
 *  @param q Receives the output quaternion 
 *  @param x Rotation angle for the x-axis, in radians
 *  @param y Rotation angle for the y-axis, in radians
 *  @param z Rotation angle for the z-axis, in radians
 */
ASSIMP_API void aiQuaternionFromEulerAngles(
        C_STRUCT aiQuaternion *q,
        float x, float y, float z);

// --------------------------------------------------------------------------------
/** Create a Quaternion from an axis angle pair.
 *  @param q Receives the output quaternion
 *  @param axis The orientation axis
 *  @param angle The rotation angle, in radians
 */
ASSIMP_API void aiQuaternionFromAxisAngle(
        C_STRUCT aiQuaternion *q,
        const C_STRUCT aiVector3D *axis,
        const float angle);

// --------------------------------------------------------------------------------
/** Create a Quaternion from a normalized quaternion stored
 *  in a 3D vector.
 *  @param q Receives the output quaternion
 *  @param normalized The vector that stores the quaternion
 */
ASSIMP_API void aiQuaternionFromNormalizedQuaternion(
        C_STRUCT aiQuaternion *q,
        const C_STRUCT aiVector3D *normalized);

// --------------------------------------------------------------------------------
/** Check if quaternions are equal.
 *  @param a First quaternion to compare
 *  @param b Second quaternion to compare
 *  @return 1 if the quaternions are equal
 *  @return 0 if the quaternions are not equal
 */
ASSIMP_API int aiQuaternionAreEqual(
        const C_STRUCT aiQuaternion *a,
        const C_STRUCT aiQuaternion *b);

// --------------------------------------------------------------------------------
/** Check if quaternions are equal using epsilon.
 *  @param a First quaternion to compare
 *  @param b Second quaternion to compare
 *  @param epsilon Epsilon
 *  @return 1 if the quaternions are equal
 *  @return 0 if the quaternions are not equal
 */
ASSIMP_API int aiQuaternionAreEqualEpsilon(
        const C_STRUCT aiQuaternion *a,
        const C_STRUCT aiQuaternion *b,
        const float epsilon);

// --------------------------------------------------------------------------------
/** Normalize a quaternion.
 *  @param q Quaternion to normalize
 */
ASSIMP_API void aiQuaternionNormalize(
        C_STRUCT aiQuaternion *q);

// --------------------------------------------------------------------------------
/** Compute quaternion conjugate.
 *  @param q Quaternion to compute conjugate,
 *           receives the output quaternion
 */
ASSIMP_API void aiQuaternionConjugate(
        C_STRUCT aiQuaternion *q);

// --------------------------------------------------------------------------------
/** Multiply quaternions.
 *  @param dst First quaternion, receives the output quaternion
 *  @param q Second quaternion
 */
ASSIMP_API void aiQuaternionMultiply(
        C_STRUCT aiQuaternion *dst,
        const C_STRUCT aiQuaternion *q);

// --------------------------------------------------------------------------------
/** Performs a spherical interpolation between two quaternions.
 * @param dst Receives the quaternion resulting from the interpolation.
 * @param start Quaternion when factor == 0
 * @param end Quaternion when factor == 1
 * @param factor Interpolation factor between 0 and 1
 */
ASSIMP_API void aiQuaternionInterpolate(
        C_STRUCT aiQuaternion *dst,
        const C_STRUCT aiQuaternion *start,
        const C_STRUCT aiQuaternion *end,
        const float factor);

#ifdef __cplusplus
}
#endif

#endif // AI_ASSIMP_H_INC
