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

#ifndef ASSIMP_PCH_INCLUDED
#define ASSIMP_PCH_INCLUDED

#define ASSIMP_INTERNAL_BUILD

// Compile config
#include "../include/aiDefines.h"

// Undefine the min/max macros defined by some platform headers
#undef min
#undef max

// Concatenate two tokens after evaluating them
#define AI_CONCAT(a,b) a ## b

// Helper macro that sets a pointer to NULL in debug builds
#if (defined _DEBUG)
#	define AI_DEBUG_INVALIDATE_PTR(x) x = NULL;
#else
#	define AI_DEBUG_INVALIDATE_PTR(x)
#endif

// If we have at least VC8 some C string manipulation functions
// are mapped to their safe _s counterparts (e.g. _itoa_s).
#if _MSC_VER >= 1400 && !(defined _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
#	define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#endif

// Actually that's not required for MSVC (it is included somewhere in 
// the STL ..) but it is necessary for build with STLport.
#include <ctype.h>

// Runtime/STL headers
#include <vector>
#include <list>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <stack>
#include <queue>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <new>

// Public ASSIMP headers
#include "../include/DefaultLogger.h"
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiScene.h"
#include "../include/aiPostProcess.h"
#include "../include/assimp.hpp"

// Internal utility headers
#include "BaseImporter.h"
#include "MaterialSystem.h"
#include "StringComparison.h"
#include "StreamReader.h"
#include "qnan.h"

// boost headers - take them from the workaround dir if possible
#ifdef ASSIMP_BUILD_BOOST_WORKAROUND

#	include "../include/BoostWorkaround/boost/scoped_ptr.hpp"
#	include "../include/BoostWorkaround/boost/scoped_array.hpp"
#	include "../include/BoostWorkaround/boost/format.hpp"
#	include "../include/BoostWorkaround/boost/foreach.hpp"
#	include "../include/BoostWorkaround/boost/static_assert.hpp"

#else

#	include <boost/scoped_ptr.hpp>
#	include <boost/scoped_array.hpp>
#	include <boost/format.hpp>
#	include <boost/foreach.hpp>
#	include <boost/static_assert.hpp>

#endif // ! ASSIMP_BUILD_BOOST_WORKAROUND
#endif // !! ASSIMP_PCH_INCLUDED


