
/** @file Defines generic routines to access memory-mapped files
 *
 */

#ifndef AI_FILEIO_H_INC
#define AI_FILEIO_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif


struct aiFileIO;
//enum aiOrigin;
typedef aiFileIO (*aiFileOpenProc)(aiFileIO*, const char*, const char*);
typedef aiReturn (*aiFileCloseProc)(aiFileIO*);
typedef unsigned long (*aiFileReadWriteProc)(aiFileIO*, char*, unsigned int, unsigned int);
typedef unsigned long (*aiFileTellProc)(aiFileIO*);

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

typedef aiReturn (*aiFileSeek)(aiFileIO*, unsigned long, aiOrigin);

typedef char* aiUserData;

// ---------------------------------------------------------------------------
/** Data structure to wrap a set of fXXXX (e.g fopen) replacement functions
*
* The functions behave the same way as their appropriate fXXXX 
* counterparts in the CRT.
*/
// ---------------------------------------------------------------------------
struct aiFileIO
{
	aiUserData UserData;

	aiFileOpenProc OpenFunc;
	aiFileCloseProc CloseFunc;
	aiFileReadWriteProc ReadFunc;
	aiFileReadWriteProc WriteFunc;
	aiFileTellProc TellProc;
	aiFileSeek SeekProc;
};


#ifdef __cplusplus
}
#endif


#endif // AI_FILEIO_H_INC
