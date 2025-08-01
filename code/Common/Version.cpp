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

// Actually just a dummy, used by the compiler to build the pre-compiled header.

#include "ScenePrivate.h"
#include <assimp/scene.h>
#include <assimp/version.h>
#include <assimp/revision.h>

// --------------------------------------------------------------------------------
// Legal information string - don't remove this.
static constexpr char LEGAL_INFORMATION[] =
        "Open Asset Import Library (Assimp).\n"
        "A free C/C++ library to import various 3D file formats into applications\n\n"
        "(c) 2006-2025, Assimp team\n"
        "License under the terms and conditions of the 3-clause BSD license\n"
        "https://www.assimp.org\n";

// ------------------------------------------------------------------------------------------------
// Get legal string
ASSIMP_API const char *aiGetLegalString() {
    return LEGAL_INFORMATION;
}

// ------------------------------------------------------------------------------------------------
// Get Assimp patch version
ASSIMP_API unsigned int aiGetVersionPatch() {
    return VER_PATCH;
}

// ------------------------------------------------------------------------------------------------
// Get Assimp minor version
ASSIMP_API unsigned int aiGetVersionMinor() {
    return VER_MINOR;
}

// ------------------------------------------------------------------------------------------------
// Get Assimp major version
ASSIMP_API unsigned int aiGetVersionMajor() {
    return VER_MAJOR;
}

// ------------------------------------------------------------------------------------------------
// Get flags used for compilation
ASSIMP_API unsigned int aiGetCompileFlags() {

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
#ifdef ASSIMP_DOUBLE_PRECISION
    flags |= ASSIMP_CFLAGS_DOUBLE_SUPPORT;
#endif

    return flags;
}

// ------------------------------------------------------------------------------------------------
ASSIMP_API unsigned int aiGetVersionRevision() {
    return GitVersion;
}

// ------------------------------------------------------------------------------------------------
ASSIMP_API const char *aiGetBranchName() {
    return GitBranch;
}

