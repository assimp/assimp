


// This is a dummy unit - it is used to generate the precompiled header file.
// + it contains version management functions

#include "AssimpPCH.h"

// ################################################################################
// Legal information string
// Note that this string must be contained in the compiled image.
static const char* LEGAL_INFORMATION =

"Open Asset Import Library (ASSIMP).\n"
"A free C/C++ library to import various 3D file formats into applications\n\n"

"(c) ASSIMP Development Team, 2008-2009\n"
"License: modified BSD license (http://assimp.sourceforge.net/main_license.html)\n"
"Website: http://assimp.sourceforge.net\n"
;
// ################################################################################

// ------------------------------------------------------------------------------------------------
ASSIMP_API const char*  aiGetLegalString  ()
{
	return LEGAL_INFORMATION;
}

// ------------------------------------------------------------------------------------------------
ASSIMP_API unsigned int aiGetVersionMinor ()
{
	return 5;
}

// ------------------------------------------------------------------------------------------------
ASSIMP_API unsigned int aiGetVersionMajor ()
{
	return 0;
}

// ------------------------------------------------------------------------------------------------
ASSIMP_API unsigned int aiGetVersionRevision ()
{
	// TODO: find a way to update the revision number automatically
	return 254;
}

