
// Actually just a dummy, used by the compiler to build the precompiled header.

#include "AssimpPCH.h"
#include "./../include/aiVersion.h"

// --------------------------------------------------------------------------------
// Legal information string - dont't remove from image!
static const char* LEGAL_INFORMATION =

"Open Asset Import Library (ASSIMP).\n"
"A free C/C++ library to import various 3D file formats into applications\n\n"

"(c) ASSIMP Development Team, 2008-2009\n"
"License: 3-clause BSD license\n"
"Website: http://assimp.sourceforge.net\n"
;

// ------------------------------------------------------------------------------------------------
// Get legal string
ASSIMP_API const char*  aiGetLegalString  ()	{
	return LEGAL_INFORMATION;
}

// ------------------------------------------------------------------------------------------------
// Get Assimp minor version
ASSIMP_API unsigned int aiGetVersionMinor ()	{
	return 5;
}

// ------------------------------------------------------------------------------------------------
// Get Assimp major version
ASSIMP_API unsigned int aiGetVersionMajor ()	{
	return 0;
}

// ------------------------------------------------------------------------------------------------
// Get flags used for compilation
ASSIMP_API unsigned int aiGetCompileFlags ()	{

	unsigned int flags = 0;

#ifdef ASSIMP_BUILD_BOOST_WORKAROUND
	flags |= ASSIMP_CFLAGS_NOBOOST;
#endif
#ifdef ASSIMP_BUILD_SINGLETHREADED
	flags |= ASSIMP_CFLAGS_SINGLETHREADED;
#endif
#ifdef ASSIMP_BUILD_DEBUG
	flags |= ASSIMP_CFLAGS_DEBUG;
#endif
#ifdef ASSIMP_BUILD_DLL_EXPORT
	flags |= ASSIMP_CFLAGS_SHARED;
#endif
#ifdef _STLPORT_VERSION
	flags |= ASSIMP_CFLAGS_STLPORT;
#endif

	return flags;
}

// include current build revision
#include "../mkutil/revision.h"

// ------------------------------------------------------------------------------------------------
ASSIMP_API unsigned int aiGetVersionRevision ()
{
	return SVNRevision;
}

