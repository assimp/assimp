CMAKE_MINIMUM_REQUIRED( VERSION 3.8 )
PROJECT( OpenDDL-Parser )
# read version number from cpp file of the form: static const char *Version = "0.4.0";
file ( READ code/OpenDDLParser.cpp _ver )
string( REGEX MATCH "static const char [*]Version[ ]*=[ ]*\"[^\"]*\"" _ver_line "${_ver}" )
string( REGEX MATCH "[0-9]+\.[0-9]+\.[0-9]+" openddlparser_VERSION "${_ver_line}" )
SET ( PROJECT_VERSION "${openddlparser_VERSION}" )
if ( "${PROJECT_VERSION}" STREQUAL "" )
    message( FATAL_ERROR "Cannot find 'static const char *Version' in 'code/OpenDDLParser.cpp'" )
endif()
message(STATUS "openddlparser_VERSION: ${openddlparser_VERSION}")

option( DDL_DEBUG_OUTPUT        "Set to ON to use output debug texts"                                         OFF )
option( DDL_STATIC_LIBRARY		"Deprecated, use BUILD_SHARED_LIBS instead."                                  ON )
# for backwards compatibility use DDL_STATIC_LIBRARY as initial value for cmake variable BUILD_SHARED_LIBS
# https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html
if ( DDL_STATIC_LIBRARY )
    set ( build_shared_libs_default OFF )
else()
    set ( build_shared_libs_default ON )
endif()
option( DDL_BUILD_SHARED_LIBS   "Set to ON to build shared libary of OpenDDL Parser."                         ${build_shared_libs_default} )
option( COVERALLS               "Generate coveralls data"                                                     OFF )
option( DDL_DOCUMENTATION       "Set to ON to opt in generating API documentation with Doxygen"               OFF )
option( DDL_BUILD_TESTS         "Set to OFF to not build tests by default"                                    ON )
option( DDL_BUILD_PARSER_DEMO   "Set to OFF to opt out building parser demo"                                  ON )

if (MSVC)
    add_definitions(
        -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
        -DGTEST_HAS_TR1_TUPLE=0
        -D_CRT_SECURE_NO_WARNINGS
    )
endif()

add_definitions( -D_VARIADIC_MAX=10 )
add_definitions( -DGTEST_HAS_PTHREAD=0 )
if ( DDL_DEBUG_OUTPUT )
    add_definitions( -DDDL_DEBUG_HEADER_NAME)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib )
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )

if (MSVC)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc" )  # Force to always compile with W4
  if( CMAKE_CXX_FLAGS MATCHES "/W[0-4]" )
    string( REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
  else()
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4" )
  endif()
elseif( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic ${OPENDDL_CXXFLAGS}")
elseif ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic ${OPENDDL_CXXFLAGS} -Wwrite-strings")
endif()

if (COVERALLS)
    include(Coveralls)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
endif()

# Include the doc component.
if(DDL_DOCUMENTATION)
    find_package(Doxygen REQUIRED)
    CONFIGURE_FILE( doc/openddlparser_doc.in doc/doxygenfile @ONLY )
    add_custom_target(doc ALL
        ${DOXYGEN_EXECUTABLE} doc/doxygenfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM )
endif()

SET ( openddlparser_headers
    include/openddlparser/OpenDDLCommon.h
    include/openddlparser/OpenDDLExport.h
    include/openddlparser/OpenDDLParser.h
    include/openddlparser/OpenDDLParserUtils.h
    include/openddlparser/OpenDDLStream.h
    include/openddlparser/DDLNode.h
    include/openddlparser/Value.h
    include/openddlparser/TPoolAllocator.h
)
SET ( openddlparser_src
    code/OpenDDLCommon.cpp
    code/OpenDDLExport.cpp
    code/OpenDDLParser.cpp
    code/OpenDDLStream.cpp
    code/DDLNode.cpp
    code/Value.cpp
)

SOURCE_GROUP( code            FILES ${openddlparser_src} )

ADD_LIBRARY( openddlparser ${openddlparser_src})

target_include_directories(openddlparser PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_compile_features(openddlparser PUBLIC cxx_std_11)

if(DDL_BUILD_SHARED_LIBS)
    set_target_properties(openddlparser PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN TRUE
    )
else()
    target_compile_definitions(openddlparser PUBLIC OPENDDL_STATIC_LIBARY)
endif()

set_target_properties( openddlparser PROPERTIES PUBLIC_HEADER "${openddlparser_headers}")

if (DDL_BUILD_TESTS)
    find_package(Threads REQUIRED)

    SET ( GTEST_PATH contrib/gtest-1.7.0 )

    SET ( gtest_src
        ${GTEST_PATH}/src/gtest-death-test.cc
        ${GTEST_PATH}/src/gtest-filepath.cc
        ${GTEST_PATH}/src/gtest-port.cc
        ${GTEST_PATH}/src/gtest-printers.cc
        ${GTEST_PATH}/src/gtest-test-part.cc
        ${GTEST_PATH}/src/gtest-typed-test.cc
        ${GTEST_PATH}/src/gtest.cc
    )

    SET( openddlparser_unittest_src
        test/DDLNodeTest.cpp
        test/OpenDDLCommonTest.cpp
        test/OpenDDLExportTest.cpp
        test/OpenDDLParserTest.cpp
        test/OpenDDLParserUtilsTest.cpp
        test/OpenDDLStreamTest.cpp
        test/OpenDDLIntegrationTest.cpp
        test/ValueTest.cpp
        test/OpenDDLDefectsTest.cpp
        test/OssFuzzTest.cpp
        test/main.cpp
    )

    SOURCE_GROUP( code  FILES ${openddlparser_unittest_src} )
    SOURCE_GROUP( gtest FILES ${gtest_src} )

    ADD_EXECUTABLE( openddlparser_unittest
        ${gtest_src}
        ${openddlparser_unittest_src}
    )

    target_include_directories(openddlparser_unittest
        PRIVATE
            ${GTEST_PATH}
            ${GTEST_PATH}/include
    )
    target_link_libraries(openddlparser_unittest openddlparser Threads::Threads)
    target_compile_features(openddlparser_unittest PRIVATE cxx_std_11)
    target_compile_definitions(openddlparser_unittest PRIVATE OPENDDL_TEST_DATA="${PROJECT_SOURCE_DIR}/test/TestData")
endif ()

if (DDL_BUILD_PARSER_DEMO)
    SET( openddlparser_demo_src
        demo/main.cpp
    )

    if (COVERALLS)
        set(COVERAGE_SRCS     ${gtest_src} ${openddlparser_unittest_src} )

        # Create the coveralls target.
        coveralls_setup(
            "${COVERAGE_SRCS}" # The source files.
            ON                 # If we should upload.
            "${PROJECT_SOURCE_DIR}/cmake/") # (Optional) Alternate project cmake module path.
    endif()

    ADD_EXECUTABLE( openddlparser_demo
        ${openddlparser_demo_src}
    )

    target_link_libraries( openddlparser_demo openddlparser )
    target_compile_features(openddlparser_demo PRIVATE cxx_std_11)
endif ()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS openddlparser
        EXPORT  openddlparser-targets
        RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/openddlparser")

export(EXPORT openddlparser-targets
       FILE openddlparser-config.cmake
       NAMESPACE openddlparser::)

install(EXPORT openddlparser-targets
    FILE openddlparser-config.cmake
    NAMESPACE openddlparser::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/openddlparser")

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/openddlparser-config-version.cmake"
    COMPATIBILITY SameMajorVersion)

install(
    FILES "${CMAKE_BINARY_DIR}/openddlparser-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/openddlparser")
