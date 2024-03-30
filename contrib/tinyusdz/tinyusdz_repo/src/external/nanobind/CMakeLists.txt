cmake_minimum_required(VERSION 3.15...3.22)
project(nanobind)

# ---------------------------------------------------------------------------
# Only build tests by default if this is the top-level CMake project
# ---------------------------------------------------------------------------

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(NB_TEST_DEFAULT ON)
else()
  set(NB_TEST_DEFAULT OFF)
endif()

option(NB_TEST   "Compile nanobind tests?" ${NB_TEST_DEFAULT})

# ---------------------------------------------------------------------------
# Do a release build if nothing was specified
# ---------------------------------------------------------------------------

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "nanobind: setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

# ---------------------------------------------------------------------------
# Check whether all dependencies are present
# ---------------------------------------------------------------------------

if (NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ext/robin_map/include")
  message(FATAL_ERROR "The nanobind dependencies are missing! "
    "You probably did not clone the project with --recursive. It is possible to recover "
    "by invoking\n$ git submodule update --init --recursive")
endif()

# ---------------------------------------------------------------------------
# Compile with a few more compiler warnings turned on
# ---------------------------------------------------------------------------

if (MSVC)
  if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    add_compile_options(/W4)
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wno-unused-local-typedefs)
endif()

# ---------------------------------------------------------------------------
# Find the Python interpreter and development libraries
# ---------------------------------------------------------------------------

# TinyUSDZ fix: find_package(Python3) is invoked at project toplevel.

#set(Python_FIND_FRAMEWORK LAST) # Prefer Brew/Conda to Apple framework python
#find_package(Python 3.8 COMPONENTS Interpreter Development REQUIRED)

if ((Python3_VERSION_MAJOR LESS 3) OR ((Python3_VERSION_MAJOR EQUAL 3) AND (Python3_VERSION_MINOR LESS 8)))
  message(FATAL_ERROR "nanobind requires Python >= 3.8")
endif()

# ---------------------------------------------------------------------------
# Include nanobind cmake functionality
# ---------------------------------------------------------------------------

find_package(nanobind
  PATHS ${CMAKE_CURRENT_SOURCE_DIR}/cmake
  NO_DEFAULT_PATH)

if (NB_TEST)
  add_subdirectory(tests)
endif()
