# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

OPTION(ASSIMP_HEADERCHECK "adds the target headercheck which compiles every header file, default disabled because it adds many targets" OFF)
if(ASSIMP_HEADERCHECK)
include_directories(
  ${Assimp_SOURCE_DIR}/code/BoostWorkaround
)

FILE(GLOB_RECURSE headers
  ${Assimp_SOURCE_DIR}/code/*.h
)

set(headerchecklibs "")

FOREACH( HEADER ${headers} )

  #pseudo unique name
  string(REPLACE "${Spring_SOURCE_DIR}" "" PLIST "${HEADER}")
  string(REPLACE "/" ";" PLIST "${PLIST}")
  SET(TEST_NAME "headercheck_")

  foreach(name ${PLIST})
    SET(TEST_NAME "${TEST_NAME}_${name}")
  endforeach()

  SET(TEST_FILE ${CMAKE_CURRENT_BINARY_DIR}/src-generated/${TEST_NAME}.cpp)

  # create test file
  configure_file( headercheck.cpp.in ${TEST_FILE} )
  # add library
  add_library( ${TEST_NAME} EXCLUDE_FROM_ALL ${TEST_FILE} )
  list(APPEND headerchecklibs ${TEST_NAME})
ENDFOREACH( HEADER )

add_custom_target(headercheck DEPENDS ${headerchecklibs})

endif()
