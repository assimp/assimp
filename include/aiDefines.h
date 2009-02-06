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

/** @file aiDefines.h
 *  @brief Assimp build configuration setup. See the notes in the comment
 *  blocks to find out how you can customize your Assimp build.
 */

#ifndef INCLUDED_AI_DEFINES_H
#define INCLUDED_AI_DEFINES_H

	//////////////////////////////////////////////////////////////////////////
	// Define ASSIMP_BUILD_NO_XX_IMPORTER to disable a specific
	// file format loader. The loader is be excluded from the
	// build in this case. 'XX' stands for the most common file
	// extension of the file format. E.g.: 
    // ASSIMP_BUILD_NO_X_IMPORTER disables the X loader.
	//
	// Other configuration switches:
	//    ASSIMP_BUILD_NO_COMPRESSED_X 
	//      - Disable support for compressed X files, removes the
	//        dependency from the zlib inflate algorithm.
	//
#ifndef ASSIMP_BUILD_NO_COMPRESSED_X
#	define ASSIMP_BUILD_NEED_Z_INFLATE
#endif

	//////////////////////////////////////////////////////////////////////////
	// Define ASSIMP_BUILD_NO_XX_PROCESS to disable a specific
	// post-processing step. The spe will be excluded from the
	// build in this case. 'XX' stands for the name of the loader.
	// List of defines:
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
	// ENTITYMESHBUILDER

// Compiler specific includes and definitions
#if (defined _MSC_VER)
#	undef ASSIMP_API

	// Include our workaround stdint.h - VC doesn't have one
#	include "./../include/Compiler/pstdint.h"

	//////////////////////////////////////////////////////////////////////////
	// Define ASSIMP_BUILD_DLL_EXPORT to build a DLL of the library
#	if (defined ASSIMP_BUILD_DLL_EXPORT)
#		define ASSIMP_API __declspec(dllexport)
#		pragma warning (disable : 4251)

	//////////////////////////////////////////////////////////////////////////
	// Define ASSIMP_DLL before including Assimp to use ASSIMP in
	// an external DLL (otherwise a static library is used)
#	elif (defined ASSIMP_DLL)
#		define ASSIMP_API __declspec(dllimport)
#	else
#		define ASSIMP_API 
#	endif

	// Force the compiler to inline a function, if supported
#	define AI_FORCE_INLINE __forceinline

#else
#	define ASSIMP_API
#	define AI_FORCE_INLINE inline
#endif // (defined _MSC_VER)

#ifdef __cplusplus
	// No explicit 'struct' and 'enum' tags for C++, we don't want to 
	// confuse the AI (:-)) of our IDE.
#	define C_STRUCT
#	define C_ENUM
#else
	//////////////////////////////////////////////////////////////////////////
	// To build the documentation, make sure ASSIMP_DOXYGEN_BUILD
	// is defined by Doxygen's preprocessor. The corresponding
	// entries in the DoxyFile look like this:
#if 0
	ENABLE_PREPROCESSING   = YES
	MACRO_EXPANSION        = YES
	EXPAND_ONLY_PREDEF     = YES
	SEARCH_INCLUDES        = YES
	INCLUDE_PATH           = 
	INCLUDE_FILE_PATTERNS  = 
	PREDEFINED             = ASSIMP_DOXYGEN_BUILD=1
	EXPAND_AS_DEFINED      = C_STRUCT C_ENUM
	SKIP_FUNCTION_MACROS   = YES
#endif
	//////////////////////////////////////////////////////////////////////////
	// Doxygen gets confused if we use c-struct typedefs to avoid
	// the explicit 'struct' notation. This trick here has the same
	// effect as the TYPEDEF_HIDES_STRUCT option, but we don't need
	// to typedef all structs/enums.
#	if (defined ASSIMP_DOXYGEN_BUILD)
#		define C_STRUCT 
#		define C_ENUM   
#	else
#		define C_STRUCT struct
#		define C_ENUM   enum
#	endif
#endif

#if (defined(__BORLANDC__) || defined (__BCPLUSPLUS__))

#error Currently Borland is unsupported. Feel free to port Assimp.

// "W8059 Packgröße der Struktur geändert"

#endif
	//////////////////////////////////////////////////////////////////////////
	// Define ASSIMP_BUILD_BOOST_WORKAROUND to compile assimp
	// without boost. This is done by using a few workaround
	// classes. However, some assimp features are not available
	// in this case. This implies the ASSIMP_BUILD_SINGLETHREADED option.
#ifdef ASSIMP_BUILD_BOOST_WORKAROUND

	// threading support requires boost
#ifndef ASSIMP_BUILD_SINGLETHREADED
#	define ASSIMP_BUILD_SINGLETHREADED
#endif

#endif

	//////////////////////////////////////////////////////////////////////////
	// Define ASSIMP_BUILD_SINGLETHREADED to compile assimp
	// without threading support. The library doesn't utilize
	// threads then, and is itself not threadsafe.
	// If this flag is specified, boost::threads is *not* required.

	// TODO
#ifndef ASSIMP_BUILD_SINGLETHREADED
#	define ASSIMP_BUILD_SINGLETHREADED
#endif

#ifndef ASSIMP_BUILD_SINGLETHREADED
#	define AI_C_THREADSAFE
#endif // !! ASSIMP_BUILD_SINGLETHREADED

#if (defined _DEBUG || defined DEBUG) // one of the two should be defined ..
#	define ASSIMP_BUILD_DEBUG
#endif

// Make sure NULL is defined
#ifndef NULL
#	define NULL 0
#endif

// Use our own definition of PI here
#define AI_MATH_PI		(3.1415926538)
#define AI_MATH_TWO_PI	(AI_MATH_PI * 2.0)
#define AI_MATH_HALF_PI	(AI_MATH_PI * 0.5)

// Tiny macro to convert from radians to degrees and the reverse
#define AI_DEG_TO_RAD(x) (x*0.0174532925f)
#define AI_RAD_TO_DEG(x) (x*57.2957795f)

#endif // !! INCLUDED_AI_DEFINES_H
