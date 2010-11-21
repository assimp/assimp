cmake_minimum_required( VERSION 2.6 )
PROJECT( Assimp )
SET ( PROJECT_VERSION "2.0" )

INCLUDE_DIRECTORIES( include )

# If this is an in-source build (CMAKE_SOURCE_DIR == CMAKE_BINARY_DIR),
# write the library/executable files to the respective directories in the
# source tree. During an out-of-source build, however, do not litter this
# directory, since that is probably what the user wanted to avoid.
IF ( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )
	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )
ENDIF ( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )

# Cache these to allow the user to override them manually.
SET( LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH
	"Path the built library files are installed to." )
SET( INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH
	"Path the header files are installed to." )
SET( BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH
	"Path the tool executables are installed to." )

# Generate a pkg-config .pc for the Assimp library.
CONFIGURE_FILE( "${CMAKE_SOURCE_DIR}/assimp.pc.in" "${CMAKE_BINARY_DIR}/assimp.pc" @ONLY )
INSTALL( FILES "${CMAKE_BINARY_DIR}/assimp.pc" DESTINATION ${LIB_INSTALL_DIR}/pkgconfig/ )

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
	FIND_PACKAGE( Boost 1.35 )

	IF ( NOT Boost_FOUND )
		MESSAGE( FATAL_ERROR
			"Boost libraries (http://www.boost.org/) not found. "
			"You can build a non-boost version of Assimp with slightly reduced "
			"functionality by specifying -DENABLE_BOOST_WORKAROUND=ON."
		)
	ENDIF ( NOT Boost_FOUND )

	INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIRS} )
ENDIF ( ENABLE_BOOST_WORKAROUND )


ADD_SUBDIRECTORY( code/ )
SET ( BUILD_ASSIMP_TOOLS ON CACHE BOOL
	"If the supplementary tools for Assimp are built in addition to the library."
)
IF ( BUILD_ASSIMP_TOOLS )
	IF ( WIN32 )
		ADD_SUBDIRECTORY( tools/assimp_view/ )
		ADD_SUBDIRECTORY( samples/SimpleOpenGL/ )
		ADD_SUBDIRECTORY( samples/SimpleTexturedOpenGL/ )
	ENDIF ( WIN32 )
	ADD_SUBDIRECTORY( tools/assimp_cmd/ )
ENDIF ( BUILD_ASSIMP_TOOLS )

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
