# looks like cmake is a bit braindead, selects c and c++
# but not ld and ar for crosscompile, so we force it
INCLUDE(CMakeForceCompiler)

# this one sets internal to crosscompile (in theory)
SET(CMAKE_SYSTEM_NAME Windows)

# extreme way of settings...
# which C and C++ compiler to use
SET(_CMAKE_TOOLCHAIN_PREFIX "x86_64-w64-mingw32-")
SET(CMAKE_RC_COMPILER "${_CMAKE_TOOLCHAIN_PREFIX}windres")

# specify the force cross compiler else compiler-test fails.
# strange thing that compiler ID is ignored???
CMAKE_FORCE_C_COMPILER(${_CMAKE_TOOLCHAIN_PREFIX}gcc MinGW)
CMAKE_FORCE_CXX_COMPILER(${_CMAKE_TOOLCHAIN_PREFIX}g++ MinGW)

# where is the target (so called staging) environment
SET(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
#
# search for programs in the build host directories (default BOTH)
#SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
