

// #ifndef ASSIMP_BUILD_SINGLETHREADED
// #    include <boost/thread.hpp>
// #endif

// We need to be sure to have the same STL settings as Assimp

#include <assimp/cimport.h>
#include <gtest/gtest.h>
#include <memory>
#include <math.h>

#undef min
#undef max
