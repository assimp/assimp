
/* --------------------------------------------------------------------------------
 *
 * Open Asset Import Library (ASSIMP) (http://assimp.sourceforge.net)
 * Assimp2Java bridge 
 *
 * Copyright (c) 2006-2009, ASSIMP Development Team
 * All rights reserved. See the LICENSE file for more information.
 *
 * --------------------------------------------------------------------------------
 */

#ifdef DEBUG
#	define JASSIMP_DEBUG_CHECKS
#endif

// Assimp's public headers don't include it anymore, but we need it for uint64_t
#include "Compiler/pstdint.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>

#include <vector>
#include <jni.h>

#include <aiScene.h>
#include "assimp.hpp"

#include "jbridge_Environment.h"
#include "jbridge_Logger.h"

