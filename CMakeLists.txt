cmake_minimum_required( VERSION 2.6 )
PROJECT( Assimp )

# Define here the needed parameters
set (ASSIMP_SV_REVISION 1264)
set (ASSIMP_VERSION_MAJOR 3)
set (ASSIMP_VERSION_MINOR 0)
set (ASSIMP_VERSION_PATCH ${ASSIMP_SV_REVISION}) # subversion revision?
set (ASSIMP_VERSION ${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}.${ASSIMP_VERSION_PATCH})
set (ASSIMP_SOVERSION 3)
SET ( PROJECT_VERSION "${ASSIMP_VERSION}" )

set(PACKAGE_VERSION "0" CACHE STRING "the package-specific version used for uploading the sources")

option(OPT_BUILD_PACKAGES "Set to ON to generate CPack configuration files and packaging targets" OFF)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")
set(LIBASSIMP_COMPONENT libassimp${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}-r${ASSIMP_SV_REVISION})
set(LIBASSIMP-DEV_COMPONENT libassimp${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}-r${ASSIMP_SV_REVISION}-dev)
set(CPACK_COMPONENTS_ALL assimp-bin ${LIBASSIMP_COMPONENT} ${LIBASSIMP-DEV_COMPONENT} assimp-dev)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-fPIC) # this is a very important switch and some libraries seem now to have it....
  ## hide all not-exported symbols
  add_definitions( -fvisibility=hidden )
endif()

INCLUDE (FindPkgConfig)
INCLUDE_DIRECTORIES( include )

# If this is an in-source build (CMAKE_SOURCE_DIR == CMAKE_BINARY_DIR),
# write the library/executable files to the respective directories in the
# source tree. During an out-of-source build, however, do not litter this
# directory, since that is probably what the user wanted to avoid.
IF ( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )
	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
	SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )
ENDIF ( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )

# Cache these to allow the user to override them manually.
SET( LIB_INSTALL_DIR "lib" CACHE PATH
	"Path the built library files are installed to." )
SET( INCLUDE_INSTALL_DIR "include" CACHE PATH
	"Path the header files are installed to." )
SET( BIN_INSTALL_DIR "bin" CACHE PATH
	"Path the tool executables are installed to." )

SET(DEBUG_POSTFIX "D" CACHE STRING "Debug Postfitx for lib, samples and tools")

# Generate a pkg-config .pc for the Assimp library.
CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/assimp.pc.in" "${PROJECT_BINARY_DIR}/assimp.pc" @ONLY )
INSTALL( FILES "${PROJECT_BINARY_DIR}/assimp.pc" DESTINATION ${LIB_INSTALL_DIR}/pkgconfig/ COMPONENT ${LIBASSIMP-DEV_COMPONENT})

# cmake configuration files
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/assimp-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/assimp-config.cmake" @ONLY IMMEDIATE)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/assimp-config-version.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/assimp-config-version.cmake" @ONLY IMMEDIATE)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/assimp-config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/assimp-config-version.cmake" DESTINATION "${LIB_INSTALL_DIR}/cmake/assimp-${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}" COMPONENT ${LIBASSIMP-DEV_COMPONENT})

# add make uninstall capability
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules/cmake_uninstall.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Allow the user to build a static library
SET ( BUILD_STATIC_LIB OFF CACHE BOOL
	"Build a static (.a) version of the library"
)

# Globally enbale Boost resp. the Boost workaround – it is also needed by the
# tools which include the Assimp headers.
SET ( ENABLE_BOOST_WORKAROUND OFF CACHE BOOL
	"If a simple implementation of the used Boost functions is used. Slightly reduces functionality, but enables builds without Boost available."
)
IF ( ENABLE_BOOST_WORKAROUND )
	INCLUDE_DIRECTORIES( code/BoostWorkaround )
	ADD_DEFINITIONS( -DASSIMP_BUILD_BOOST_WORKAROUND )
	MESSAGE( STATUS "Building a non-boost version of Assimp." )
ELSE ( ENABLE_BOOST_WORKAROUND )
	SET( Boost_DETAILED_FAILURE_MSG ON )
	SET( Boost_ADDITIONAL_VERSIONS "1.47" "1.47.0" "1.48.0" "1.48" "1.49" "1.49.0" "1.50")	
	FIND_PACKAGE( Boost )
	IF ( NOT Boost_FOUND )
		MESSAGE( FATAL_ERROR
			"Boost libraries (http://www.boost.org/) not found. "
			"You can build a non-boost version of Assimp with slightly reduced "
			"functionality by specifying -DENABLE_BOOST_WORKAROUND=ON."
		)
	ENDIF ( NOT Boost_FOUND )

	INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIRS} )
ENDIF ( ENABLE_BOOST_WORKAROUND )


SET ( NO_EXPORT OFF CACHE BOOL
	"Disable Assimp's export functionality." 
)

# Search for external dependencies, and build them from source if not found
# Search for zlib
find_package(ZLIB)
if( NOT ZLIB_FOUND )
  message(STATUS "compiling zlib from souces")
  include(CheckIncludeFile)
  include(CheckTypeSize)
  include(CheckFunctionExists)
  # compile from sources
  add_subdirectory(contrib/zlib)
  set(ZLIB_FOUND 1)
  set(ZLIB_LIBRARIES zlib)
  set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zlib)
else(NOT ZLIB_FOUND)
  ADD_DEFINITIONS(-DASSIMP_BUILD_NO_OWN_ZLIB)
endif(NOT ZLIB_FOUND)
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})

# Search for unzip
if (PKG_CONFIG_FOUND)
	PKG_CHECK_MODULES(UNZIP minizip)
endif (PKG_CONFIG_FOUND)

IF ( NO_EXPORT )
	ADD_DEFINITIONS( -DASSIMP_BUILD_NO_EXPORT)
	MESSAGE( STATUS "Build an import-only version of Assimp." )
ENDIF( NO_EXPORT )


SET ( ASSIMP_BUILD_ARCHITECTURE "" CACHE STRING 
	"describe the current architecture."
)
IF    ( ASSIMP_BUILD_ARCHITECTURE STREQUAL "")
ELSE  ( ASSIMP_BUILD_ARCHITECTURE STREQUAL "")
	ADD_DEFINITIONS ( -D'ASSIMP_BUILD_ARCHITECTURE="${ASSIMP_BUILD_ARCHITECTURE}"' )
ENDIF ( ASSIMP_BUILD_ARCHITECTURE STREQUAL "")

SET ( ASSIMP_BUILD_COMPILER "" CACHE STRING 
	"describe the current compiler."
)
IF    ( ASSIMP_BUILD_COMPILER STREQUAL "")
ELSE  ( ASSIMP_BUILD_COMPILER STREQUAL "")
	ADD_DEFINITIONS ( -D'ASSIMP_BUILD_COMPILER="${ASSIMP_BUILD_COMPILER}"' )
ENDIF ( ASSIMP_BUILD_COMPILER STREQUAL "")

MARK_AS_ADVANCED ( ASSIMP_BUILD_ARCHITECTURE ASSIMP_BUILD_COMPILER )


ADD_SUBDIRECTORY( code/ )
SET ( BUILD_ASSIMP_TOOLS ON CACHE BOOL
	"If the supplementary tools for Assimp are built in addition to the library."
)
IF ( BUILD_ASSIMP_TOOLS )
	IF ( WIN32 )
		ADD_SUBDIRECTORY( tools/assimp_view/ )
	ENDIF ( WIN32 )
	ADD_SUBDIRECTORY( tools/assimp_cmd/ )
ENDIF ( BUILD_ASSIMP_TOOLS )

SET ( BUILD_ASSIMP_SAMPLES OFF CACHE BOOL
	"If the official samples are built as well (needs Glut)."
)

IF ( BUILD_ASSIMP_SAMPLES)
	IF ( WIN32 )
		ADD_SUBDIRECTORY( samples/SimpleTexturedOpenGL/ )
	ENDIF ( WIN32 )
	ADD_SUBDIRECTORY( samples/SimpleOpenGL/ )
ENDIF ( BUILD_ASSIMP_SAMPLES )

SET ( BUILD_TESTS OFF CACHE BOOL
	"If the test suite for Assimp is built in addition to the library."
)
IF ( BUILD_TESTS )
	IF ( WIN32 )
		ADD_SUBDIRECTORY( test/ )
	ELSE ( WIN32 )
		MESSAGE( WARNING "The Assimp test suite is currently Windows-only." )
	ENDIF ( WIN32 )
ENDIF ( BUILD_TESTS )

if(CMAKE_CPACK_COMMAND AND UNIX AND OPT_BUILD_PACKAGES)
  # Packing information
  set(CPACK_PACKAGE_NAME assimp{ASSIMP_VERSION_MAJOR})
  set(CPACK_PACKAGE_CONTACT "" CACHE STRING "Package maintainer and PGP signer.")
  set(CPACK_PACKAGE_VENDOR "http://assimp.sourceforge.net/")
  set(CPACK_PACKAGE_DISPLAY_NAME "Assimp ${ASSIMP_VERSION}")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY " - Open Asset Import Library ${ASSIMP_VERSION}")
  set(CPACK_PACKAGE_VERSION ${ASSIMP_VERSION}.${PACKAGE_VERSION})
  set(CPACK_PACKAGE_VERSION_MAJOR ${ASSIMP_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${ASSIMP_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${ASSIMP_VERSION_PATCH})
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "assimp${ASSIMP_VERSION_MAJOR}.${ASSIMP_VERSION_MINOR}")
  #set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/description)
  set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)

  string(TOUPPER ${LIBASSIMP_COMPONENT} LIBASSIMP_COMPONENT_UPPER)
  string(TOUPPER ${LIBASSIMP-DEV_COMPONENT} LIBASSIMP-DEV_COMPONENT_UPPER)

  set(CPACK_COMPONENT_ASSIMP-BIN_DISPLAY_NAME "tools")
  set(CPACK_COMPONENT_ASSIMP-BIN_DEPENDS ${LIBASSIMP_COMPONENT})
  set(CPACK_COMPONENT_${LIBASSIMP_COMPONENT_UPPER}_DISPLAY_NAME "libraries")
  set(CPACK_COMPONENT_${LIBASSIMP-DEV_COMPONENT_UPPER}_DISPLAY_NAME "common headers and installs")
  set(CPACK_COMPONENT_${LIBASSIMP-DEV_COMPONENT_UPPER}_DEPENDS ${LIBASSIMP_COMPONENT})
  set(CPACK_COMPONENT_ASSIMP-DEV_DISPLAY_NAME ${CPACK_COMPONENT_${LIBASSIMP-DEV_COMPONENT}_DISPLAY_NAME})
  set(CPACK_COMPONENT_ASSIMP-DEV_DEPENDS ${LIBASSIMP-DEV_COMPONENT})
  set(CPACK_DEBIAN_BUILD_DEPENDS debhelper cmake libboost-dev libboost-thread-dev libboost-math-dev zlib1g-dev pkg-config)

  # debian
  set(CPACK_DEBIAN_PACKAGE_PRIORITY optional)
  set(CPACK_DEBIAN_CMAKE_OPTIONS "-DBUILD_ASSIMP_SAMPLES:BOOL=${BUILD_ASSIMP_SAMPLES}")
  set(CPACK_DEBIAN_PACKAGE_SECTION libs)
  set(CPACK_DEBIAN_PACKAGE_DEPENDS ${CPACK_COMPONENTS_ALL})
  set(CPACK_DEBIAN_PACKAGE_SUGGESTS)
  set(CPACK_DEBIAN_PACKAGE_NAME assimp)
  set(CPACK_DEBIAN_PACKAGE_REMOVE_SOURCE_FILES contrib/cppunit-1.12.1 contrib/cppunit_note.txt contrib/zlib workspaces test doc obj samples packaging)
  set(CPACK_DEBIAN_PACKAGE_SOURCE_COPY svn export --force)
  set(CPACK_DEBIAN_CHANGELOG)
  execute_process(COMMAND lsb_release -is
    OUTPUT_VARIABLE _lsb_distribution OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _lsb_release_failed)
  set(CPACK_DEBIAN_DISTRIBUTION_NAME ${_lsb_distribution} CACHE STRING "Name of the distrubiton")
  string(TOLOWER ${CPACK_DEBIAN_DISTRIBUTION_NAME} CPACK_DEBIAN_DISTRIBUTION_NAME)
  if( ${CPACK_DEBIAN_DISTRIBUTION_NAME} STREQUAL "ubuntu" )
    set(CPACK_DEBIAN_DISTRIBUTION_RELEASES lucid maverick natty oneiric precise CACHE STRING "Release code-names of the distrubiton release")
  endif()
  set(DPUT_HOST "" CACHE STRING "PPA repository to upload the debian sources")
  include(CPack)
  include(DebSourcePPA)
endif()
