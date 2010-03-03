cmake_minimum_required( VERSION 2.6 )
PROJECT( Assimp )

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

# Libs
ADD_SUBDIRECTORY( code/ )
IF ( WIN32 )
	ADD_SUBDIRECTORY( test/ )
	ADD_SUBDIRECTORY( tools/assimp_view/ )
ENDIF ( WIN32 )

ADD_SUBDIRECTORY( tools/assimp_cmd/ )
