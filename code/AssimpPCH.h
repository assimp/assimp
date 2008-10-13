

#ifndef ASSIMP_PCH_INCLUDED
#define ASSIMP_PCH_INCLUDED

// STL headers
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

// public ASSIMP headers
#include "../include/DefaultLogger.h"
#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiScene.h"
#include "../include/aiPostProcess.h"
#include "../include/assimp.hpp"

// internal headers that are nearly always required
#include "MaterialSystem.h"
#include "StringComparison.h"
#include "ByteSwap.h"
#include "qnan.h"

// boost headers - take them from the workaround dir if possible
#ifdef ASSIMP_BUILD_BOOST_WORKAROUND

#	include "../include/BoostWorkaround/boost/scoped_ptr.hpp"
#	include "../include/BoostWorkaround/boost/format.hpp"
#else

#	include <boost/scoped_ptr.hpp>
#	include <boost/format.hpp>

#endif

#endif // !! ASSIMP_PCH_INCLUDED

