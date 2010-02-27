cmake_minimum_required( VERSION 2.6 )
PROJECT( AssetImporter )

SET( CMAKE_CURRENT_SOURCE_DIR ${CMAKE_HOME_DIRECTORY}/code )
SET( CMAKE_CURRENT_BINARY_DIR ${CMAKE_HOME_DIRECTORY}/bin )

INCLUDE_DIRECTORIES( 
	include
)

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin ) 
SET( LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib" )

SET( INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" )
SET( LIBRARY_VERSION "1.0.0" )
SET( LIBRARY_SOVERSION "1" )

# Libs
add_subdirectory( code/ )
IF( WIN32 )
	add_subdirectory( test/ )
	add_subdirectory( tools/assimp_cmd/ )
	add_subdirectory( tools/assimp_view/ )
ENDIF( WIN32 )



