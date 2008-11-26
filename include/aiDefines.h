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

/** Assimo build configuration setup. See the notes in the comment
 *  blocks to find out how you can customize the Assimp build
 */

#ifndef AI_DEFINES_H_INC
#define AI_DEFINES_H_INC


	// ************************************************************
	// Define ASSIMP_BUILD_NO_XX_LOADER to disable a specific
	// file format loader. The loader will be excluded from the
	// build in this case. 'XX' stands for the file extension
	// of the loader, e.g. ASSIMP_BUILD_NO_X_LOADER will disable
	// the X loader.
	// ************************************************************

	// ************************************************************
	// Define ASSIMP_BUILD_NO_XX_PROCESS to disable a specific
	// post-processing step. The spe will be excluded from the
	// build in this case. 'XX' stands for the name of the loader.
	// Name list:
	//
	// CALCTANGENTS
	// JOINVERTICES
	// CONVERTTOLH
	// TRIANGULATE
	// GENFACENORMALS
	// GENVERTEXNORMALS
	// REMOVEVC
	// SPLITLARGEMESHES
	// PRETRANSFORMVERTICES
	// LIMITBONEWEIGHTS
	// VALIDATEDS
	// IMPROVECACHELOCALITY
	// FIXINFACINGNORMALS
	// REMOVE_REDUNDANTMATERIALS
	// OPTIMIZEGRAPH
	// SORTBYPTYPE
	// FINDINVALIDDATA
	// TRANSFORMTEXCOORDS
	// GENUVCOORDS
	// ************************************************************


	// ************************************************************
	// Define AI_C_THREADSAFE if you want a thread-safe C-API
	// This feature requires boost.
	// ************************************************************


// compiler specific includes and definitions
#if (defined _MSC_VER)

#	undef ASSIMP_API

	
	// ************************************************************
	// Define ASSIMP_BUILD_DLL_EXPORT to build a DLL of the library
	// ************************************************************
#	if (defined ASSIMP_BUILD_DLL_EXPORT)

#		if (defined ASSIMP_API)
#			error ASSIMP_API is defined, although it shouldn't
#		endif

#		define ASSIMP_API __declspec(dllexport)
#		pragma warning (disable : 4251)

	// ************************************************************
	// Define ASSIMP_DLL before including Assimp to use ASSIMP in
	// an external DLL (otherwise a static library is used)
	// ************************************************************
#	elif (defined ASSIMP_DLL)
#		define ASSIMP_API __declspec(dllimport)
#	else
#		define ASSIMP_API 
#	endif

#	define AI_FORCE_INLINE __forceinline


#else
#	define ASSIMP_API
#	define AI_FORCE_INLINE inline
#endif // (defined _MSC_VER)

#ifdef __cplusplus
#	define C_STRUCT
#else
	// ************************************************************
	// To build the documentation, make sure ASSIMP_DOXYGEN_BUILD
	// is defined by Doxygen's preprocessor. The corresponding
	// entries in the DoxyFile look like this:
	// ************************************************************
#if 0
	ENABLE_PREPROCESSING   = YES
	MACRO_EXPANSION        = YES
	EXPAND_ONLY_PREDEF     = YES
	SEARCH_INCLUDES        = YES
	INCLUDE_PATH           = 
	INCLUDE_FILE_PATTERNS  = 
	PREDEFINED             = ASSIMP_DOXYGEN_BUILD=1
	EXPAND_AS_DEFINED      = C_STRUCT
	SKIP_FUNCTION_MACROS   = YES
#endif
	// ************************************************************
#	if (defined ASSIMP_DOXYGEN_BUILD)
#		define C_STRUCT
#	else
#		define C_STRUCT struct
#	endif
#endif

#if (defined(__BORLANDC__) || defined (__BCPLUSPLUS__))

// "W8059 Packgröße der Struktur geändert"
// TODO: find a way to deactivate this warning automatically
// maybe there is a pragma to do exactly this?

#endif

// include our workaround stdint.h from the C98 standard to make
// sure the types it declares are consistently available 
#include "./../include/Compiler/pstdint.h"

	// ************************************************************
	// Define ASSIMP_BUILD_BOOST_WORKAROUND to compile assimp
	// without boost. This is done by using a few workaround
	// classes. However, some assimp features are not available
	// in this case. 
	// ************************************************************
#ifdef ASSIMP_BUILD_BOOST_WORKAROUND

	// threading support in the C-API requires boost
#	ifdef AI_C_THREADSAFE
#		error Unable to activate C-API threading support. Boost is required for this.
#	endif

#endif

// helper macro that sets a pointer to NULL in debug builds
#if (!defined AI_DEBUG_INVALIDATE_PTR)
#	if (defined _DEBUG)
#		define AI_DEBUG_INVALIDATE_PTR(x) x = NULL;
#	else
#		define AI_DEBUG_INVALIDATE_PTR(x)
#	endif
#endif 

// Use our own definition of PI here
#define AI_MATH_PI		(3.1415926538)
#define AI_MATH_TWO_PI	(AI_MATH_PI * 2.0)
#define AI_MATH_HALF_PI	(AI_MATH_PI * 0.5)

// macrod to convert from radians to degrees and the reverse
#define AI_DEG_TO_RAD(x) (x*0.0174532925f)
#define AI_RAD_TO_DEG(x) (x*57.2957795f)

#endif // !! AI_DEFINES_H_INC
