# Open Asset Import Library (assimp)
# ----------------------------------------------------------------------
# Copyright (c) 2006-2022, assimp team
#
# All rights reserved.
#
# Redistribution and use of this software in source and binary forms,
# with or without modification, are permitted provided that the
# following conditions are met:
#
# * Redistributions of source code must retain the above
#   copyright notice, this list of conditions and the
#   following disclaimer.
#
# * Redistributions in binary form must reproduce the above
#   copyright notice, this list of conditions and the
#   following disclaimer in the documentation and/or other
#   materials provided with the distribution.
#
# * Neither the name of the assimp team, nor the names of its
#   contributors may be used to endorse or promote products
#   derived from this software without specific prior
#   written permission of the assimp team.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#----------------------------------------------------------------------
SET(CMAKE_POLICY_DEFAULT_CMP0012 NEW)
SET(CMAKE_POLICY_DEFAULT_CMP0074 NEW)
SET(CMAKE_POLICY_DEFAULT_CMP0092 NEW)

CMAKE_MINIMUM_REQUIRED( VERSION 3.10 )

# Disabled importers: m3d for 5.1
ADD_DEFINITIONS( -DASSIMP_BUILD_NO_M3D_IMPORTER)
ADD_DEFINITIONS( -DASSIMP_BUILD_NO_M3D_EXPORTER)
# Toggles the use of the hunter package manager
option(ASSIMP_HUNTER_ENABLED "Enable Hunter package manager support" OFF)

IF(ASSIMP_HUNTER_ENABLED)
  include("cmake-modules/HunterGate.cmake")
  HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.24.0.tar.gz"
    SHA1 "a3d7f4372b1dcd52faa6ff4a3bd5358e1d0e5efd"
  )

  add_definitions(-DASSIMP_USE_HUNTER)
ENDIF()

PROJECT(Assimp VERSION 5.2.0)

# All supported options ###############################################

OPTION( BUILD_SHARED_LIBS
  "Build package with shared libraries."
  ON
)
OPTION( ASSIMP_BUILD_FRAMEWORK
  "Build package as Mac OS X Framework bundle."
  OFF
)
OPTION( ASSIMP_DOUBLE_PRECISION
  "Set to ON to enable double precision processing"
  OFF
)
OPTION( ASSIMP_OPT_BUILD_PACKAGES
  "Set to ON to generate CPack configuration files and packaging targets"
  OFF
)
OPTION( ASSIMP_ANDROID_JNIIOSYSTEM
  "Android JNI IOSystem support is active"
  OFF
)
OPTION( ASSIMP_NO_EXPORT
  "Disable Assimp's export functionality."
  OFF
)
OPTION( ASSIMP_BUILD_ZLIB
  "Build your own zlib"
  OFF
)
OPTION( ASSIMP_BUILD_ASSIMP_TOOLS
  "If the supplementary tools for Assimp are built in addition to the library."
  ON
)
OPTION ( ASSIMP_BUILD_SAMPLES
  "If the official samples are built as well (needs Glut)."
  OFF
)
OPTION ( ASSIMP_BUILD_TESTS
  "If the test suite for Assimp is built in addition to the library."
  ON
)
OPTION ( ASSIMP_COVERALLS
  "Enable this to measure test coverage."
  OFF
)
OPTION( ASSIMP_INSTALL
  "Disable this if you want to use assimp as a submodule."
  ON
)
OPTION ( ASSIMP_ERROR_MAX
  "Enable all warnings."
  OFF
)
OPTION ( ASSIMP_ASAN
  "Enable AddressSanitizer."
  OFF
)
OPTION ( ASSIMP_UBSAN
  "Enable Undefined Behavior sanitizer."
  OFF
)
OPTION ( ASSIMP_BUILD_DOCS
  "Build documentation using Doxygen."
  OFF
)
OPTION( ASSIMP_INJECT_DEBUG_POSTFIX
  "Inject debug postfix in .a/.so/.dll lib names"
  ON
)

OPTION ( ASSIMP_IGNORE_GIT_HASH
   "Don't call git to get the hash."
   OFF
)

IF (WIN32)
  # Use subset of Windows.h
  ADD_DEFINITIONS( -DWIN32_LEAN_AND_MEAN )

  IF(MSVC)
    OPTION (ASSIMP_BUILD_ASSIMP_VIEW
      "If the Assimp view tool is built. (requires DirectX)"
      OFF )

    OPTION( ASSIMP_INSTALL_PDB
      "Install MSVC debug files."
      ON )
    IF(NOT (MSVC_VERSION LESS 1900))
      # Multibyte character set is deprecated since at least MSVC2015 (possibly earlier)
      ADD_DEFINITIONS( -DUNICODE -D_UNICODE )
    ENDIF()
  ENDIF()
ENDIF()

IF (IOS AND NOT ASSIMP_HUNTER_ENABLED)
  IF (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release")
  ENDIF ()
  ADD_DEFINITIONS(-DENABLE_BITCODE)
ENDIF ()


IF (ASSIMP_BUILD_FRAMEWORK)
  SET (BUILD_SHARED_LIBS ON)
  MESSAGE(STATUS "Framework bundle building enabled")
ENDIF()

IF(NOT BUILD_SHARED_LIBS)
  MESSAGE(STATUS "Shared libraries disabled")
  SET(LINK_SEARCH_START_STATIC TRUE)
ELSE()
  MESSAGE(STATUS "Shared libraries enabled")
ENDIF()

# Define here the needed parameters
SET (ASSIMP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
SET (ASSIMP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
SET (ASSIMP_VERSION_PATCH ${PROJECT_VERSION_PATCH})
SET (ASSIMP_VERSION ${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}.${ASSIMP_VERSION_PATCH})
SET (ASSIMP_SOVERSION 5)

SET( ASSIMP_PACKAGE_VERSION "0" CACHE STRING "the package-specific version used for uploading the sources" )
if(NOT ASSIMP_HUNTER_ENABLED)
  # Enable C++11 support globally
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_C_STANDARD 99)
endif()

IF(NOT ASSIMP_IGNORE_GIT_HASH)
  # Get the current working branch
  EXECUTE_PROCESS(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )

  # Get the latest abbreviated commit hash of the working branch
  EXECUTE_PROCESS(
    COMMAND git rev-parse --short=8 HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
ENDIF()

IF(NOT GIT_COMMIT_HASH)
  SET(GIT_COMMIT_HASH 0)
ENDIF()

IF(ASSIMP_DOUBLE_PRECISION)
  ADD_DEFINITIONS(-DASSIMP_DOUBLE_PRECISION)
ENDIF()

CONFIGURE_FILE(
  ${CMAKE_CURRENT_LIST_DIR}/revision.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/revision.h
)

CONFIGURE_FILE(
  ${CMAKE_CURRENT_LIST_DIR}/include/assimp/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/assimp/config.h
)

INCLUDE_DIRECTORIES( BEFORE
  ./
  code/
  include
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/include
)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules" )
SET(LIBASSIMP_COMPONENT     "libassimp${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}.${ASSIMP_VERSION_PATCH}" )
SET(LIBASSIMP-DEV_COMPONENT "libassimp${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}.${ASSIMP_VERSION_PATCH}-dev" )
SET(CPACK_COMPONENTS_ALL assimp-bin ${LIBASSIMP_COMPONENT} ${LIBASSIMP-DEV_COMPONENT} assimp-dev)
SET(ASSIMP_LIBRARY_SUFFIX "" CACHE STRING "Suffix to append to library names")

IF( UNIX )
  # Use GNUInstallDirs for Unix predefined directories
  INCLUDE(GNUInstallDirs)
  # Ensure that we do not run into issues like http://www.tcm.phy.cam.ac.uk/sw/inodes64.html on 32 bit linux
  IF( ${OPERATING_SYSTEM} MATCHES "Android")
  ELSE()
    IF ( CMAKE_SIZEOF_VOID_P EQUAL 4) # only necessary for 32-bit linux
      ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 )
    ENDIF()
  ENDIF()
ENDIF()

# Grouped compiler settings ########################################
IF ((CMAKE_C_COMPILER_ID MATCHES "GNU") AND NOT MINGW)
  IF(NOT ASSIMP_HUNTER_ENABLED)
    SET(CMAKE_CXX_STANDARD 11)
    SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
  ENDIF()
  # hide all not-exported symbols
  IF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "mips64" )
	SET(CMAKE_CXX_FLAGS "-mxgot -fvisibility=hidden -fno-strict-aliasing -Wall ${CMAKE_CXX_FLAGS}")
	SET(CMAKE_C_FLAGS "-fno-strict-aliasing ${CMAKE_C_FLAGS}")
	SET(LIBSTDC++_LIBRARIES -lstdc++)
  ELSE()
	SET(CMAKE_CXX_FLAGS "-fvisibility=hidden -fno-strict-aliasing -Wall ${CMAKE_CXX_FLAGS}")
	SET(CMAKE_C_FLAGS "-fno-strict-aliasing ${CMAKE_C_FLAGS}")
	SET(LIBSTDC++_LIBRARIES -lstdc++)
  ENDIF()
ELSEIF(MSVC)
  # enable multi-core compilation with MSVC
  IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang" ) # clang-cl
    ADD_COMPILE_OPTIONS(/bigobj /W4 /WX )
  ELSE() # msvc
    ADD_COMPILE_OPTIONS(/MP /bigobj /W4 /WX)
  ENDIF()
  # disable "elements of array '' will be default initialized" warning on MSVC2013
  IF(MSVC12)
    ADD_COMPILE_OPTIONS(/wd4351)	
  ENDIF()
  ADD_COMPILE_OPTIONS(/wd4244) #supress warning for double to float conversion if Double precission is activated
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG /Zi /Od")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
  SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG:FULL /PDBALTPATH:%_PDB% /OPT:REF /OPT:ICF")
ELSEIF (CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
  IF(NOT ASSIMP_HUNTER_ENABLED)
    SET(CMAKE_CXX_STANDARD 11)
    SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
  ENDIF()
  SET(CMAKE_CXX_FLAGS "-fvisibility=hidden -fno-strict-aliasing -Wall -Wno-long-long ${CMAKE_CXX_FLAGS}" )
  SET(CMAKE_C_FLAGS "-fno-strict-aliasing ${CMAKE_C_FLAGS}")
ELSEIF( MINGW )
  IF (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
    message(FATAL_ERROR "MinGW is too old to be supported. Please update MinGW and try again.")
  ELSEIF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.3)
    message(WARNING "MinGW is old, if you experience errors, update MinGW.")
  ENDIF()
  IF(NOT ASSIMP_HUNTER_ENABLED)
    SET(CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")
    SET(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
  ENDIF()
  SET(CMAKE_CXX_FLAGS "-fvisibility=hidden -fno-strict-aliasing -Wall -Wno-long-long -Wa,-mbig-obj -O3 ${CMAKE_CXX_FLAGS}")
  SET(CMAKE_C_FLAGS "-fno-strict-aliasing ${CMAKE_C_FLAGS}")
ENDIF()

IF ( IOS AND NOT ASSIMP_HUNTER_ENABLED)
  IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode -Og")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode -Og")
  ELSE()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode -O3")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode -O3")
    # Experimental for pdb generation
  ENDIF()
ENDIF()

IF (ASSIMP_COVERALLS)
  MESSAGE(STATUS "Coveralls enabled")
  INCLUDE(Coveralls)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
ENDIF()

IF (ASSIMP_ERROR_MAX)
  MESSAGE(STATUS "Turning on all warnings")
  IF (MSVC)
    ADD_COMPILE_OPTIONS(/W4) # NB: there is a /Wall option, pedantic mode
  ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  ENDIF()
ENDIF()

IF (ASSIMP_ASAN)
  MESSAGE(STATUS "AddressSanitizer enabled")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
ENDIF()

IF (ASSIMP_UBSAN)
  MESSAGE(STATUS "Undefined Behavior sanitizer enabled")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,shift,shift-exponent,integer-divide-by-zero,unreachable,vla-bound,null,return,signed-integer-overflow,bounds,float-divide-by-zero,float-cast-overflow,nonnull-attribute,returns-nonnull-attribute,bool,enum,vptr,pointer-overflow,builtin -fno-sanitize-recover=all")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined,shift,shift-exponent,integer-divide-by-zero,unreachable,vla-bound,null,return,signed-integer-overflow,bounds,float-divide-by-zero,float-cast-overflow,nonnull-attribute,returns-nonnull-attribute,bool,enum,vptr,pointer-overflow,builtin -fno-sanitize-recover=all")
ENDIF()

INCLUDE (FindPkgMacros)
INCLUDE (PrecompiledHeader)

# Set Assimp project output directory variables.
# Will respect top-level CMAKE_*_OUTPUT_DIRECTORY variables if any are set.
IF(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  SET(ASSIMP_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin" CACHE STRING "Path for runtime output files")
ELSE()
  SET(ASSIMP_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE STRING "Path for runtime output files")
ENDIF()

IF(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  SET(ASSIMP_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin" CACHE STRING "Path for library output files")
ELSE()
  SET(ASSIMP_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} CACHE STRING "Path for runtime output files")
ENDIF()

IF(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  SET(ASSIMP_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib" CACHE STRING "Path for library output files")
ELSE()
  SET(ASSIMP_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} CACHE STRING "Path for runtime output files")
ENDIF()

# Macro used to set the output directories of a target to the
# respective Assimp output directories.
MACRO(TARGET_USE_COMMON_OUTPUT_DIRECTORY target)
  set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${ASSIMP_RUNTIME_OUTPUT_DIRECTORY}
      LIBRARY_OUTPUT_DIRECTORY ${ASSIMP_LIBRARY_OUTPUT_DIRECTORY}
      ARCHIVE_OUTPUT_DIRECTORY ${ASSIMP_ARCHIVE_OUTPUT_DIRECTORY}
  )
ENDMACRO()

get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)

IF (ASSIMP_INJECT_DEBUG_POSTFIX AND (is_multi_config OR CMAKE_BUILD_TYPE STREQUAL "Debug"))
  SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Debug Postfix for lib, samples and tools")
ELSE()
  SET(CMAKE_DEBUG_POSTFIX "" CACHE STRING "Debug Postfix for lib, samples and tools")
ENDIF()

# Only generate this target if no higher-level project already has
IF (NOT TARGET uninstall AND ASSIMP_INSTALL)
  # add make uninstall capability
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules/cmake_uninstall.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
  ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
ENDIF()

# cmake configuration files
if(${BUILD_SHARED_LIBS})
  set(BUILD_LIB_TYPE SHARED)
else()
  set(BUILD_LIB_TYPE STATIC)
endif()

IF( UNIX )
  # Use GNUInstallDirs for Unix predefined directories
  INCLUDE(GNUInstallDirs)

  SET( ASSIMP_LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
  SET( ASSIMP_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
  SET( ASSIMP_BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
ELSE()
  # Cache these to allow the user to override them on non-Unix platforms
  SET( ASSIMP_LIB_INSTALL_DIR "lib" CACHE STRING
    "Path the built library files are installed to." )
  SET( ASSIMP_INCLUDE_INSTALL_DIR "include" CACHE STRING
    "Path the header files are installed to." )
  SET( ASSIMP_BIN_INSTALL_DIR "bin" CACHE STRING
    "Path the tool executables are installed to." )

  SET(CMAKE_INSTALL_FULL_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/${ASSIMP_INCLUDE_INSTALL_DIR})
  SET(CMAKE_INSTALL_FULL_LIBDIR ${CMAKE_INSTALL_PREFIX}/${ASSIMP_LIB_INSTALL_DIR})
  SET(CMAKE_INSTALL_FULL_BINDIR ${CMAKE_INSTALL_PREFIX}/${ASSIMP_BIN_INSTALL_DIR})
ENDIF()

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

IF(ASSIMP_HUNTER_ENABLED)
  set(CONFIG_INSTALL_DIR "lib/cmake/${PROJECT_NAME}")
  set(CMAKE_CONFIG_TEMPLATE_FILE "cmake-modules/assimp-hunter-config.cmake.in")
  set(NAMESPACE "${PROJECT_NAME}::")
  set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
  set(VERSION_CONFIG "${GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
  set(PROJECT_CONFIG "${GENERATED_DIR}/${PROJECT_NAME}Config.cmake")
ELSE()
  set(CONFIG_INSTALL_DIR "${ASSIMP_LIB_INSTALL_DIR}/cmake/assimp-${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}")
  set(CMAKE_CONFIG_TEMPLATE_FILE "cmake-modules/assimp-plain-config.cmake.in")
  string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
  set(NAMESPACE "${PROJECT_NAME_LOWERCASE}::")
  set(TARGETS_EXPORT_NAME "${PROJECT_NAME_LOWERCASE}Targets")
  set(VERSION_CONFIG "${GENERATED_DIR}/${PROJECT_NAME_LOWERCASE}ConfigVersion.cmake")
  set(PROJECT_CONFIG "${GENERATED_DIR}/${PROJECT_NAME_LOWERCASE}Config.cmake")
ENDIF()

set(INCLUDE_INSTALL_DIR "include")

# Include module with fuction 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Note: PROJECT_VERSION is used as a VERSION
write_basic_package_version_file("${VERSION_CONFIG}" COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    ${CMAKE_CONFIG_TEMPLATE_FILE}
    "${PROJECT_CONFIG}"
    INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
)

install(
    FILES "${PROJECT_CONFIG}" "${VERSION_CONFIG}"
    DESTINATION "${CONFIG_INSTALL_DIR}"
    COMPONENT ${LIBASSIMP-DEV_COMPONENT}
)

install(
    EXPORT "${TARGETS_EXPORT_NAME}"
    NAMESPACE "${NAMESPACE}"
    DESTINATION "${CONFIG_INSTALL_DIR}"
    COMPONENT ${LIBASSIMP-DEV_COMPONENT}
)

IF( ASSIMP_BUILD_DOCS )
  ADD_SUBDIRECTORY(doc)
ENDIF()

# Search for external dependencies, and build them from source if not found
# Search for zlib
IF(ASSIMP_HUNTER_ENABLED)
  hunter_add_package(ZLIB)
  find_package(ZLIB CONFIG REQUIRED)

  add_definitions(-DASSIMP_BUILD_NO_OWN_ZLIB)
  set(ZLIB_FOUND TRUE)
  set(ZLIB_LIBRARIES ZLIB::zlib)
  set(ASSIMP_BUILD_MINIZIP TRUE)
ELSE()
  # If the zlib is already found outside, add an export in case assimpTargets can't find it.
  IF( ZLIB_FOUND )
    INSTALL( TARGETS zlib zlibstatic
        EXPORT "${TARGETS_EXPORT_NAME}")
  ENDIF()

  IF ( NOT ASSIMP_BUILD_ZLIB )
    FIND_PACKAGE(ZLIB)
  ENDIF()

  IF( NOT ZLIB_FOUND )
    MESSAGE(STATUS "compiling zlib from sources")
    INCLUDE(CheckIncludeFile)
    INCLUDE(CheckTypeSize)
    INCLUDE(CheckFunctionExists)

    # Explicitly turn off ASM686 and AMD64 cmake options.
    # The AMD64 option causes a build failure on MSVC and the ASM builds seem to have problems:
    #		https://github.com/madler/zlib/issues/41#issuecomment-125848075
    # Also prevents these options from "polluting" the cmake options if assimp is being
    # included as a submodule.
    set( ASM686 FALSE CACHE INTERNAL "Override ZLIB flag to turn off assembly" FORCE )
    set( AMD64 FALSE CACHE INTERNAL "Override ZLIB flag to turn off assembly" FORCE )

    # compile from sources
    ADD_SUBDIRECTORY(contrib/zlib)
    SET(ZLIB_FOUND 1)
    SET(ZLIB_LIBRARIES zlibstatic)
    SET(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/contrib/zlib ${CMAKE_CURRENT_BINARY_DIR}/contrib/zlib)
    # need to ensure we don't link with system zlib or minizip as well.
    SET(ASSIMP_BUILD_MINIZIP 1)
  ELSE()
    ADD_DEFINITIONS(-DASSIMP_BUILD_NO_OWN_ZLIB)
    SET(ZLIB_LIBRARIES_LINKED -lz)
  ENDIF()
  INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
ENDIF()

IF( NOT IOS )
  IF( NOT ASSIMP_BUILD_MINIZIP )
    use_pkgconfig(UNZIP minizip)
  ENDIF()
ELSE ()
  IF( NOT BUILD_SHARED_LIBS )
    IF( NOT ASSIMP_BUILD_MINIZIP )
      use_pkgconfig(UNZIP minizip)
    ENDIF()
  ENDIF ()
ENDIF ()

IF ( ASSIMP_NO_EXPORT )
  ADD_DEFINITIONS( -DASSIMP_BUILD_NO_EXPORT)
  MESSAGE( STATUS "Build an import-only version of Assimp." )
ENDIF()

SET ( ASSIMP_BUILD_ARCHITECTURE "" CACHE STRING
  "describe the current architecture."
)
IF( ASSIMP_BUILD_ARCHITECTURE STREQUAL "")
ELSE()
  ADD_DEFINITIONS ( -D'ASSIMP_BUILD_ARCHITECTURE="${ASSIMP_BUILD_ARCHITECTURE}"' )
ENDIF()

# ${CMAKE_GENERATOR}
SET ( ASSIMP_BUILD_COMPILER "" CACHE STRING
  "describe the current compiler."
)
IF( ASSIMP_BUILD_COMPILER STREQUAL "")
ELSE()
  ADD_DEFINITIONS ( -D'ASSIMP_BUILD_COMPILER="${ASSIMP_BUILD_COMPILER}"' )
ENDIF()

MARK_AS_ADVANCED ( ASSIMP_BUILD_ARCHITECTURE ASSIMP_BUILD_COMPILER )

SET ( ASSIMP_BUILD_NONFREE_C4D_IMPORTER OFF CACHE BOOL
  "Build the C4D importer, which relies on the non-free Cineware SDK."
)

IF (ASSIMP_BUILD_NONFREE_C4D_IMPORTER)
  IF ( MSVC )
    SET(C4D_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Cineware/includes")

    # pick the correct prebuilt library
    IF(MSVC15)
      SET(C4D_LIB_POSTFIX "_2017")
    ELSEIF(MSVC14)
      SET(C4D_LIB_POSTFIX "_2015")
    ELSEIF(MSVC12)
      SET(C4D_LIB_POSTFIX "_2013")
    ELSEIF(MSVC11)
      SET(C4D_LIB_POSTFIX "_2012")
    ELSEIF(MSVC10)
      SET(C4D_LIB_POSTFIX "_2010")
    ELSE()
      MESSAGE( FATAL_ERROR
        "C4D is currently only supported with MSVC 10, 11, 12, 14"
      )
    ENDIF()

    SET(C4D_LIB_BASE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Cineware/libraries/win")

    SET(C4D_DEBUG_LIBRARIES
      "${C4D_LIB_BASE_PATH}/cinewarelib${C4D_LIB_POSTFIX}/cinewarelib_debug.lib"
      "${C4D_LIB_BASE_PATH}/jpeglib${C4D_LIB_POSTFIX}/jpeglib_debug.lib"
    )
    SET(C4D_RELEASE_LIBRARIES
      "${C4D_LIB_BASE_PATH}/cinewarelib${C4D_LIB_POSTFIX}/cinewarelib_release.lib"
      "${C4D_LIB_BASE_PATH}/jpeglib${C4D_LIB_POSTFIX}/jpeglib_release.lib"
    )

    # winsock and winmm are necessary (and undocumented) dependencies of Cineware SDK because
    # it can be used to communicate with a running Cinema 4D instance
    SET(C4D_EXTRA_LIBRARIES WSock32.lib Winmm.lib)
  ELSE ()
    MESSAGE( FATAL_ERROR
      "C4D is currently only available on Windows with Cineware SDK installed in contrib/Cineware"
    )
  ENDIF ()
ELSE ()
  ADD_DEFINITIONS( -DASSIMP_BUILD_NO_C4D_IMPORTER )
ENDIF ()

# Draco requires cmake 3.12
IF (DEFINED CMAKE_VERSION AND "${CMAKE_VERSION}" VERSION_LESS "3.12")
  message(NOTICE "draco requires cmake 3.12 or newer, cmake is ${CMAKE_VERSION} . Draco is disabled")
  SET ( ASSIMP_BUILD_DRACO OFF CACHE BOOL "Disabled: Draco requires newer cmake" FORCE )
ELSE()
  OPTION ( ASSIMP_BUILD_DRACO "If the Draco libraries are to be built. Primarily for glTF" OFF )
  IF ( ASSIMP_BUILD_DRACO )
    # Primarily for glTF v2
    # Enable Draco glTF feature set
    set(DRACO_GLTF ON CACHE BOOL "" FORCE)
    # Disable unnecessary or omitted components
    set(DRACO_JS_GLUE OFF CACHE BOOL "" FORCE)
    set(DRACO_WASM OFF CACHE BOOL "" FORCE)
    set(DRACO_MAYA_PLUGIN OFF CACHE BOOL "" FORCE)
    set(DRACO_UNITY_PLUGIN OFF CACHE BOOL "" FORCE)
    set(DRACO_TESTS OFF CACHE BOOL "" FORCE)

    IF(ASSIMP_HUNTER_ENABLED)
      hunter_add_package(draco)
      find_package(draco CONFIG REQUIRED)
      set(draco_LIBRARIES draco::draco)
    ELSE()
      # Draco 1.4.1 has many warnings and will not build with /WX or -Werror
      # See https://github.com/google/draco/issues/672
      # and https://github.com/google/draco/issues/673
      IF(MSVC)
        set(DRACO_CXX_FLAGS "/W0")
      ELSE()
        list(APPEND DRACO_CXX_FLAGS
          "-Wno-bool-compare"
          "-Wno-comment"
          "-Wno-maybe-uninitialized"
          "-Wno-sign-compare"
          "-Wno-unused-local-typedefs"
        )
        # Draco 1.4.1 does not explicitly export any symbols under GCC/clang
        list(APPEND DRACO_CXX_FLAGS
          "-fvisibility=default"
        )
      ENDIF()

      # Don't build or install all of Draco by default
      ADD_SUBDIRECTORY( "contrib/draco" EXCLUDE_FROM_ALL )

      if(MSVC OR WIN32)
        set(draco_LIBRARIES "draco")
      else()
        if(BUILD_SHARED_LIBS)
          set(draco_LIBRARIES "draco_shared")
        else()
          set(draco_LIBRARIES "draco_static")
        endif()
      endif()

      # Don't build the draco command-line tools by default
      set_target_properties(draco_encoder draco_decoder PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE
        )

      # Do build the draco shared library
      set_target_properties(${draco_LIBRARIES} PROPERTIES
        EXCLUDE_FROM_ALL FALSE
        EXCLUDE_FROM_DEFAULT_BUILD FALSE
        )

      TARGET_USE_COMMON_OUTPUT_DIRECTORY(${draco_LIBRARIES})
      TARGET_USE_COMMON_OUTPUT_DIRECTORY(draco_encoder)
      TARGET_USE_COMMON_OUTPUT_DIRECTORY(draco_decoder)

      set(draco_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/draco/src")

      # This is probably wrong
      INSTALL( TARGETS ${draco_LIBRARIES}
        EXPORT "${TARGETS_EXPORT_NAME}"
        LIBRARY DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
        RUNTIME DESTINATION ${ASSIMP_BIN_INSTALL_DIR}
        FRAMEWORK DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
        COMPONENT ${LIBASSIMP_COMPONENT}
        INCLUDES DESTINATION include
    )

    ENDIF()
  ENDIF()
ENDIF()

# Main assimp code
ADD_SUBDIRECTORY( code/ )
IF ( ASSIMP_BUILD_ASSIMP_TOOLS )
  # The viewer for windows only
  IF (WIN32)
    OPTION ( ASSIMP_BUILD_ASSIMP_VIEW "If the Assimp view tool is built. (requires DirectX)" OFF )
    IF ( ASSIMP_BUILD_ASSIMP_VIEW )
      ADD_SUBDIRECTORY( tools/assimp_view/ )
    ENDIF ()
  ELSE()
    MESSAGE("Building Assimp Viewer only supported on Windows.")
  ENDIF ()
  # The command line tool
  ADD_SUBDIRECTORY( tools/assimp_cmd/ )
ENDIF ()

IF ( ASSIMP_BUILD_SAMPLES )
  SET( SAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/samples )
  SET( SAMPLES_SHARED_CODE_DIR ${SAMPLES_DIR}/SharedCode )
  IF ( WIN32 )
    ADD_SUBDIRECTORY( samples/SimpleTexturedOpenGL/ )
    ADD_SUBDIRECTORY( samples/SimpleTexturedDirectx11 )
  ENDIF ()
  ADD_SUBDIRECTORY( samples/SimpleOpenGL/ )
ENDIF ()

IF ( ASSIMP_BUILD_TESTS )
  ADD_SUBDIRECTORY( test/ )
ENDIF ()

# Generate a pkg-config .pc for the Assimp library.
CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/assimp.pc.in" "${PROJECT_BINARY_DIR}/assimp.pc" @ONLY )
IF ( ASSIMP_INSTALL )
  INSTALL( FILES "${PROJECT_BINARY_DIR}/assimp.pc" DESTINATION ${ASSIMP_LIB_INSTALL_DIR}/pkgconfig/ COMPONENT ${LIBASSIMP-DEV_COMPONENT})
ENDIF()

IF ( ASSIMP_INSTALL )
  IF(CMAKE_CPACK_COMMAND AND UNIX AND ASSIMP_OPT_BUILD_PACKAGES)
    # Packing information
    SET(CPACK_PACKAGE_NAME                    "assimp{ASSIMP_VERSION_MAJOR}.{ASSIMP_VERSION_MINOR}")
    SET(CPACK_PACKAGE_CONTACT "" CACHE STRING "Package maintainer and PGP signer.")
    SET(CPACK_PACKAGE_VENDOR                  "https://github.com/assimp")
    SET(CPACK_PACKAGE_DISPLAY_NAME            "Assimp ${ASSIMP_VERSION}")
    SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY     " - Open Asset Import Library ${ASSIMP_VERSION}")
    SET(CPACK_PACKAGE_VERSION                 "${ASSIMP_VERSION}.${ASSIMP_PACKAGE_VERSION}" )
    SET(CPACK_PACKAGE_VERSION_MAJOR           "${ASSIMP_VERSION_MAJOR}")
    SET(CPACK_PACKAGE_VERSION_MINOR           "${ASSIMP_VERSION_MINOR}")
    SET(CPACK_PACKAGE_VERSION_PATCH           "${ASSIMP_VERSION_PATCH}")
    SET(CPACK_PACKAGE_INSTALL_DIRECTORY       "assimp${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}")
    SET(CPACK_RESOURCE_FILE_LICENSE           "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

    STRING(TOUPPER ${LIBASSIMP_COMPONENT}     "LIBASSIMP_COMPONENT_UPPER")
    STRING(TOUPPER ${LIBASSIMP-DEV_COMPONENT} "LIBASSIMP-DEV_COMPONENT_UPPER")

    SET(CPACK_COMPONENT_ASSIMP-BIN_DISPLAY_NAME                       "tools")
    SET(CPACK_COMPONENT_ASSIMP-BIN_DEPENDS                            "${LIBASSIMP_COMPONENT}" )
    SET(CPACK_COMPONENT_${LIBASSIMP_COMPONENT_UPPER}_DISPLAY_NAME     "libraries")
    SET(CPACK_COMPONENT_${LIBASSIMP-DEV_COMPONENT_UPPER}_DISPLAY_NAME "common headers and installs")
    SET(CPACK_COMPONENT_${LIBASSIMP-DEV_COMPONENT_UPPER}_DEPENDS $    "{LIBASSIMP_COMPONENT}" )
    SET(CPACK_COMPONENT_ASSIMP-DEV_DISPLAY_NAME                       "${CPACK_COMPONENT_${LIBASSIMP-DEV_COMPONENT}_DISPLAY_NAME}" )
    SET(CPACK_COMPONENT_ASSIMP-DEV_DEPENDS                            "${LIBASSIMP-DEV_COMPONENT}" )
    SET(CPACK_DEBIAN_BUILD_DEPENDS debhelper cmake zlib1g-dev pkg-config)

    # debian
    SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    SET(CPACK_DEBIAN_CMAKE_OPTIONS    "-DBUILD_ASSIMP_SAMPLES:BOOL=${ASSIMP_BUILD_SAMPLES}")
    SET(CPACK_DEBIAN_PACKAGE_SECTION  "libs" )
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS  "${CPACK_COMPONENTS_ALL}")
    SET(CPACK_DEBIAN_PACKAGE_SUGGESTS)
    SET(cPACK_DEBIAN_PACKAGE_NAME     "assimp")
    SET(CPACK_DEBIAN_PACKAGE_REMOVE_SOURCE_FILES contrib/gtest contrib/zlib workspaces test doc obj samples packaging)
    SET(CPACK_DEBIAN_PACKAGE_SOURCE_COPY svn export --force)
    SET(CPACK_DEBIAN_CHANGELOG)
    execute_process(COMMAND lsb_release -is
      OUTPUT_VARIABLE _lsb_distribution OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _lsb_release_failed)
    SET(CPACK_DEBIAN_DISTRIBUTION_NAME ${_lsb_distribution} CACHE STRING "Name of the distrubiton")
    STRING(TOLOWER ${CPACK_DEBIAN_DISTRIBUTION_NAME} CPACK_DEBIAN_DISTRIBUTION_NAME)
    IF( ${CPACK_DEBIAN_DISTRIBUTION_NAME} STREQUAL "ubuntu" )
      SET(CPACK_DEBIAN_DISTRIBUTION_RELEASES lucid maverick natty oneiric precise CACHE STRING "Release code-names of the distrubiton release")
    ENDIF()
    SET(DPUT_HOST "" CACHE STRING "PPA repository to upload the debian sources")
    INCLUDE(CPack)
    INCLUDE(DebSourcePPA)
  ENDIF()
ENDIF()

if(WIN32)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(BIN_DIR "${PROJECT_SOURCE_DIR}/bin64/")
    SET(LIB_DIR "${PROJECT_SOURCE_DIR}/lib64/")
  else()
    SET(BIN_DIR "${PROJECT_SOURCE_DIR}/bin32/")
    SET(LIB_DIR "${PROJECT_SOURCE_DIR}/lib32/")
  ENDIF()

  IF(MSVC_TOOLSET_VERSION)
    SET(MSVC_PREFIX "vc${MSVC_TOOLSET_VERSION}")
    SET(ASSIMP_MSVC_VERSION ${MSVC_PREFIX})
  ELSE()
    IF(MSVC12)
      SET(ASSIMP_MSVC_VERSION "vc120")
    ELSEIF(MSVC14)
      SET(ASSIMP_MSVC_VERSION "vc140")
    ELSEIF(MSVC15)
      SET(ASSIMP_MSVC_VERSION "vc141")
    ENDIF()
  ENDIF()

  IF(MSVC12 OR MSVC14 OR MSVC15 )
    ADD_CUSTOM_TARGET(UpdateAssimpLibsDebugSymbolsAndDLLs COMMENT "Copying Assimp Libraries ..." VERBATIM)
    IF(CMAKE_GENERATOR MATCHES "^Visual Studio")
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/Release/assimp-${ASSIMP_MSVC_VERSION}-mt.dll	${BIN_DIR}assimp-${ASSIMP_MSVC_VERSION}-mt.dll VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/Release/assimp-${ASSIMP_MSVC_VERSION}-mt.exp	${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mt.exp VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/Release/assimp-${ASSIMP_MSVC_VERSION}-mt.lib	${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mt.lib VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/Debug/assimp-${ASSIMP_MSVC_VERSION}-mtd.dll		${BIN_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.dll  VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/Debug/assimp-${ASSIMP_MSVC_VERSION}-mtd.exp		${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.exp VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/Debug/assimp-${ASSIMP_MSVC_VERSION}-mtd.ilk		${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.ilk VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/Debug/assimp-${ASSIMP_MSVC_VERSION}-mtd.lib		${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.lib VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/Debug/assimp-${ASSIMP_MSVC_VERSION}-mtd.pdb		${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.pdb VERBATIM)
    ELSE()
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/assimp-${ASSIMP_MSVC_VERSION}-mt.dll	${BIN_DIR}assimp-${ASSIMP_MSVC_VERSION}-mt.dll VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/assimp-${ASSIMP_MSVC_VERSION}-mt.exp	${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mt.exp VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/assimp-${ASSIMP_MSVC_VERSION}-mt.lib	${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mt.lib VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/assimp-${ASSIMP_MSVC_VERSION}-mtd.dll		${BIN_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.dll  VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/assimp-${ASSIMP_MSVC_VERSION}-mtd.exp		${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.exp VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/assimp-${ASSIMP_MSVC_VERSION}-mtd.ilk		${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.ilk VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/assimp-${ASSIMP_MSVC_VERSION}-mtd.lib		${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.lib VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/assimp-${ASSIMP_MSVC_VERSION}-mtd.pdb		${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.pdb VERBATIM)
      ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/code/assimp-${ASSIMP_MSVC_VERSION}-mtd.pdb		${LIB_DIR}assimp-${ASSIMP_MSVC_VERSION}-mtd.pdb VERBATIM)
    ENDIF()
  ENDIF()
ENDIF ()
