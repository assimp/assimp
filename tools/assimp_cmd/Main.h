/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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

/** @file  Main.h
 *  @brief Utility declarations for assimp_cmd
 */

#ifndef AICMD_MAIN_INCLUDED
#define AICMD_MAIN_INCLUDED

#ifndef _CRT_SECURE_NO_WARNINGS
#   define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits>

#include <assimp/postprocess.h>
#include <assimp/version.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include "../code/Common/Compression.h"

#ifndef ASSIMP_BUILD_NO_EXPORT
#	include <assimp/Exporter.hpp>
#endif

#ifndef SIZE_MAX
#	define SIZE_MAX (std::numeric_limits<size_t>::max())
#endif

using namespace Assimp;

// Global assimp importer instance
extern Assimp::Importer* globalImporter;

#ifndef ASSIMP_BUILD_NO_EXPORT
// Global assimp exporter instance
extern Assimp::Exporter* globalExporter;
#endif

// ------------------------------------------------------------------------------
/// Defines common import parameters
struct ImportData {
	ImportData()
		:	ppFlags	(0)
		,	showLog (false)
		,	verbose (false)
		,	log	    (false)
        ,   rot     (aiVector3D(0.f, 0.f, 0.f))
	{}

	/// Post-processing flags
	unsigned int ppFlags;

	// Log to std::err?
	bool showLog;

	// Log file
	std::string logFile;

	// Verbose log mode?
	bool verbose;

	// Need to log?
	bool log;

	// Export With Rotation
	aiVector3D rot;
};

// ------------------------------------------------------------------------------
/// @brief General error codes used among assimp_cmd's utilities.
enum AssimpCmdError {
	Success = 0,
	InvalidNumberOfArguments,
	UnrecognizedCommand,
	FailedToLoadInputFile,
	FailedToOpenOutputFile,
	NoFileFormatSpecified,
	UnknownFileFormat,
	NoFileExtensionSpecified,
	UnknownFileExtension,
	ExceptionWasRaised,

	// Add new error codes here...

	LastAssimpCmdError, // Must be last.
};

// ------------------------------------------------------------------------------
/** Process standard arguments
 *
 *  @param fill Filled by function
 *  @param params Command line parameters to be processed
 *  @param num NUmber of params
 *  @return An #AssimpCmdError value. */
int ProcessStandardArguments(ImportData& fill,
	const char* const* params,
	unsigned int num);

// ------------------------------------------------------------------------------
/** Import a specific model file
 *  @param imp Import configuration to be used
 *  @param path Path to the file to be read */
const aiScene* ImportModel(
	const ImportData& imp,
	const std::string& path);

#ifndef ASSIMP_BUILD_NO_EXPORT

// ------------------------------------------------------------------------------
/** Export a specific model file
 *  @param imp Import configuration to be used
 *  @param path Path to the file to be written
 *  @param format Format id*/
bool ExportModel(const aiScene* pOut,
	const ImportData& imp,
	const std::string& path,
	const char* pID);

#endif

// ------------------------------------------------------------------------------
/** assimp_dump utility
 *  @param params Command line parameters to 'assimp dump'
 *  @param Number of params
 *  @return An #AssimpCmdError value.*/
int Assimp_Dump (
	const char* const* params,
	unsigned int num);

// ------------------------------------------------------------------------------
/// @brief Error codes used by the 'Export' utility.
enum AssimpCmdExportError {
	FailedToImportModel = AssimpCmdError::LastAssimpCmdError,
	FailedToExportModel,

	// Add new error codes here...

	LastAssimpCmdExportError, // Must be last.
};

// ------------------------------------------------------------------------------
/** assimp_export utility
 *  @param params Command line parameters to 'assimp export'
 *  @param Number of params
 *  @return Either an #AssimpCmdError or #AssimpCmdExportError value. */
int Assimp_Export (
	const char* const* params,
	unsigned int num);

// ------------------------------------------------------------------------------
/// @brief Error codes used by the 'Image Extractor' utility.
enum AssimpCmdExtractError {
	TextureIndexIsOutOfRange = AssimpCmdError::LastAssimpCmdError,
	NoAvailableTextureEncoderFound,
	FailedToExportCompressedTexture,

	// Add new error codes here...

	LastAssimpCmdExtractError, // Must be last.
};

// ------------------------------------------------------------------------------
/** assimp_extract utility
 *  @param params Command line parameters to 'assimp extract'
 *  @param Number of params
 *  @return Either an #AssimpCmdError or #AssimpCmdExtractError value. */
int Assimp_Extract (
	const char* const* params,
	unsigned int num);

// ------------------------------------------------------------------------------
/// @brief Error codes used by the 'Compare Dump' utility.
enum AssimpCmdCompareDumpError {
	FailedToLoadExpectedInputFile = AssimpCmdError::LastAssimpCmdError,
	FileComparaisonFailure,
	UnknownFailure,

	// Add new error codes here...

	LastAssimpCmdCompareDumpError, // Must be last.
};

// ------------------------------------------------------------------------------
/** assimp_cmpdump utility
 *  @param params Command line parameters to 'assimp cmpdump'
 *  @param Number of params
 *  @return Either an #AssimpCmdError or #AssimpCmdCompareDumpError. */
int Assimp_CompareDump (
	const char* const* params,
	unsigned int num);

/// @brief Error codes used by the 'Info' utility.
enum AssimpCmdInfoError {
	InvalidCombinaisonOfArguments = AssimpCmdError::LastAssimpCmdError,

	// Add new error codes here...

	LastAssimpCmdInfoError, // Must be last.
};

// ------------------------------------------------------------------------------
/** @brief assimp info utility
 *  @param params Command line parameters to 'assimp info'
 *  @param Number of params
 *  @return Either an #AssimpCmdError or #AssimpCmdInfoError value. */
int Assimp_Info (
	const char* const* params,
	unsigned int num);

// ------------------------------------------------------------------------------
/** @brief assimp testbatchload utility
 *  @param params Command line parameters to 'assimp testbatchload'
 *  @param Number of params
 *  @return An #AssimpCmdError value. */
int Assimp_TestBatchLoad (
	const char* const* params,
	unsigned int num);


#endif // !! AICMD_MAIN_INCLUDED
