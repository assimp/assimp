CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )
PROJECT( OpenDDL-Parser )
SET ( OPENDDL_PARSER_VERSION_MAJOR 0 )
SET ( OPENDDL_PARSER_VERSION_MINOR 1 )
SET ( OPENDDL_PARSER_VERSION_PATCH 0 )
SET ( OPENDDL_PARSER_VERSION ${CPPCORE_VERSION_MAJOR}.${CPPCORE_VERSION_MINOR}.${CPPCORE_VERSION_PATCH} )
SET ( PROJECT_VERSION "${OPENDDL_PARSER_VERSION}" )

if( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
    find_package(Threads)
else()
    add_definitions( -D_CRT_SECURE_NO_WARNINGS )
endif()

add_definitions( -DOPENDDLPARSER_BUILD )
add_definitions( -DOPENDDL_NO_USE_CPP11 )
add_definitions( -D_VARIADIC_MAX=10 )

INCLUDE_DIRECTORIES(
    ./
    include/
    contrib/gtest-1.7.0/include
    contrib/gtest-1.7.0/
)

link_directories(
    ./
)

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )

if( WIN32 AND NOT CYGWIN )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc" )  # Force to always compile with W4
  if( CMAKE_CXX_FLAGS MATCHES "/W[0-4]" )
    string( REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
  else()
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4" )
  endif()
elseif( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -std=c++0x")
elseif ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -std=c++11")
endif()

SET ( openddl_parser_src
    code/OpenDDLParser.cpp
    code/DDLNode.cpp
    code/Value.cpp
    include/openddlparser/OpenDDLParser.h
    include/openddlparser/OpenDDLParserUtils.h
    include/openddlparser/OpenDDLCommon.h
    include/openddlparser/DDLNode.h
    include/openddlparser/Value.h
    README.md
)
 
SOURCE_GROUP( code            FILES ${openddl_parser_src} )

ADD_LIBRARY( openddl_parser SHARED
    ${openddl_parser_src}
)
