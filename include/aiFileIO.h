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

/** @file Defines generic routines to access memory-mapped files
 */

#ifndef AI_FILEIO_H_INC
#define AI_FILEIO_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif


struct aiFileIO;
struct aiFile;

typedef aiFile* (*aiFileOpenProc)(C_STRUCT aiFileIO*, const char*, const char*);
typedef void (*aiFileCloseProc)(C_STRUCT aiFileIO*, C_STRUCT aiFile*);
typedef size_t (*aiFileWriteProc)(C_STRUCT aiFile*, const char*, size_t, size_t);
typedef size_t (*aiFileReadProc)(C_STRUCT aiFile*, char*, size_t,size_t);
typedef size_t (*aiFileTellProc)(C_STRUCT aiFile*);

// ---------------------------------------------------------------------------
/** Define seek origins in fseek()-style.
*/
// ---------------------------------------------------------------------------
enum aiOrigin
{
	aiOrigin_SET = 0x0,		//!< Set position
	aiOrigin_CUR = 0x1,		//!< Current position
	aiOrigin_END = 0x2		//!< End of file
};

typedef aiReturn (*aiFileSeek)(aiFile*, size_t, aiOrigin);
typedef char* aiUserData;

// ---------------------------------------------------------------------------
/** Defines how C-Assimp accesses files. Provided are functions to open
 *  and close files.
*/
// ---------------------------------------------------------------------------
struct aiFileIO
{
	//! Function used to open a new file
	aiFileOpenProc OpenProc;

	//! Function used to close an existing file
	aiFileCloseProc CloseProc;

	//! User-defined data
	aiUserData UserData;
};

// ---------------------------------------------------------------------------
/** Data structure to wrap a set of fXXXX (e.g fopen) replacement functions
*
* The functions behave the same way as their appropriate fXXXX 
* counterparts in the CRT.
*/
// ---------------------------------------------------------------------------
struct aiFile
{
	//! Function used to read from a file
	aiFileReadProc ReadProc;

	//! Function used to write to a file
	aiFileWriteProc WriteProc;

	//! Function used to retrieve the current
	//! position of the file cursor (ftell())
	aiFileTellProc TellProc;

	//! Function used to retrieve the size of the file, in bytes
	aiFileTellProc FileSizeProc;

	//! Function used to set the current position
	//! of the file cursor (fseek())
	aiFileSeek SeekProc;

	//! User-defined data
	aiUserData UserData;
};


#ifdef __cplusplus
}
#endif


#endif // AI_FILEIO_H_INC
