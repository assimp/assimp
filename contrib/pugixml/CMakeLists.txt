cmake_minimum_required(VERSION 3.5)

# Policy configuration; this *MUST* be specified before project is defined
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW) # Enables use of MSVC_RUNTIME_LIBRARY
endif()

project(pugixml VERSION 1.14 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(CMakeDependentOption)
include(GNUInstallDirs)
include(CTest)

cmake_dependent_option(PUGIXML_USE_VERSIONED_LIBDIR
  "Use a private subdirectory to install the headers and libraries" OFF
  "CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF)

cmake_dependent_option(PUGIXML_USE_POSTFIX
  "Use separate postfix for each configuration to make sure you can install multiple build outputs" OFF
  "CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF)

cmake_dependent_option(PUGIXML_STATIC_CRT
  "Use static MSVC RT libraries" OFF
  "MSVC" OFF)

cmake_dependent_option(PUGIXML_BUILD_TESTS
  "Build pugixml tests" OFF
  "BUILD_TESTING;CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF)

# Custom build defines
set(PUGIXML_BUILD_DEFINES CACHE STRING "Build defines for custom options")
separate_arguments(PUGIXML_BUILD_DEFINES)

# Technically not needed for this file. This is builtin CMAKE global variable.
option(BUILD_SHARED_LIBS "Build shared instead of static library" OFF) 

# Expose option to build PUGIXML as static as well when the global BUILD_SHARED_LIBS variable is set
cmake_dependent_option(PUGIXML_BUILD_SHARED_AND_STATIC_LIBS
  "Build both shared and static libraries" OFF
  "BUILD_SHARED_LIBS" OFF)

# Expose options from the pugiconfig.hpp
option(PUGIXML_WCHAR_MODE "Enable wchar_t mode" OFF)
option(PUGIXML_COMPACT "Enable compact mode" OFF)

# Advanced options from pugiconfig.hpp
option(PUGIXML_NO_XPATH "Disable XPath" OFF)
option(PUGIXML_NO_STL "Disable STL" OFF)
option(PUGIXML_NO_EXCEPTIONS "Disable Exceptions" OFF)
mark_as_advanced(PUGIXML_NO_XPATH PUGIXML_NO_STL PUGIXML_NO_EXCEPTIONS)

set(PUGIXML_PUBLIC_DEFINITIONS
  $<$<BOOL:${PUGIXML_WCHAR_MODE}>:PUGIXML_WCHAR_MODE>
  $<$<BOOL:${PUGIXML_COMPACT}>:PUGIXML_COMPACT>
  $<$<BOOL:${PUGIXML_NO_XPATH}>:PUGIXML_NO_XPATH>
  $<$<BOOL:${PUGIXML_NO_STL}>:PUGIXML_NO_STL>
  $<$<BOOL:${PUGIXML_NO_EXCEPTIONS}>:PUGIXML_NO_EXCEPTIONS>)

# This is used to backport a CMake 3.15 feature, but is also forwards compatible
if (NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  set(CMAKE_MSVC_RUNTIME_LIBRARY
    MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${PUGIXML_STATIC_CRT}>>:DLL>)
endif()

if (NOT DEFINED CMAKE_CXX_STANDARD_REQUIRED)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
endif()

if (PUGIXML_USE_POSTFIX)
  set(CMAKE_RELWITHDEBINFO_POSTFIX _r)
  set(CMAKE_MINSIZEREL_POSTFIX _m)
  set(CMAKE_DEBUG_POSTFIX _d)
endif()

if (CMAKE_VERSION VERSION_LESS 3.15)
  set(msvc-rt $<TARGET_PROPERTY:MSVC_RUNTIME_LIBRARY>)

  set(msvc-rt-mtd-shared $<STREQUAL:${msvc-rt},MultiThreadedDebugDLL>)
  set(msvc-rt-mtd-static $<STREQUAL:${msvc-rt},MultiThreadedDebug>)
  set(msvc-rt-mt-shared $<STREQUAL:${msvc-rt},MultiThreadedDLL>)
  set(msvc-rt-mt-static $<STREQUAL:${msvc-rt},MultiThreaded>)
  unset(msvc-rt)

  set(msvc-rt-mtd-shared $<${msvc-rt-mtd-shared}:-MDd>)
  set(msvc-rt-mtd-static $<${msvc-rt-mtd-static}:-MTd>)
  set(msvc-rt-mt-shared $<${msvc-rt-mt-shared}:-MD>)
  set(msvc-rt-mt-static $<${msvc-rt-mt-static}:-MT>)
endif()

set(versioned-dir $<$<BOOL:${PUGIXML_USE_VERSIONED_LIBDIR}>:/pugixml-${PROJECT_VERSION}>)

set(libs)

if (BUILD_SHARED_LIBS)
  add_library(pugixml-shared SHARED
    ${PROJECT_SOURCE_DIR}/scripts/pugixml_dll.rc
    ${PROJECT_SOURCE_DIR}/src/pugixml.cpp)
  add_library(pugixml::shared ALIAS pugixml-shared)
  list(APPEND libs pugixml-shared)
  string(CONCAT pugixml.msvc $<OR:
    $<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},MSVC>,
    $<CXX_COMPILER_ID:MSVC>
  >)

  set_property(TARGET pugixml-shared PROPERTY EXPORT_NAME shared)
  target_include_directories(pugixml-shared
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
  target_compile_definitions(pugixml-shared
    PUBLIC
      ${PUGIXML_BUILD_DEFINES}
      ${PUGIXML_PUBLIC_DEFINITIONS}
    PRIVATE
      PUGIXML_API=$<IF:${pugixml.msvc},__declspec\(dllexport\),__attribute__\(\(visibility\("default"\)\)\)>
    )
  target_compile_options(pugixml-shared
    PRIVATE
      ${msvc-rt-mtd-shared}
      ${msvc-rt-mtd-static}
      ${msvc-rt-mt-shared}
      ${msvc-rt-mt-static})
endif()

if (NOT BUILD_SHARED_LIBS OR PUGIXML_BUILD_SHARED_AND_STATIC_LIBS)
  add_library(pugixml-static STATIC
    ${PROJECT_SOURCE_DIR}/src/pugixml.cpp)
  add_library(pugixml::static ALIAS pugixml-static)
  list(APPEND libs pugixml-static)

  set_property(TARGET pugixml-static PROPERTY EXPORT_NAME static)
  target_include_directories(pugixml-static
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
  target_compile_definitions(pugixml-static
    PUBLIC
      ${PUGIXML_BUILD_DEFINES}
      ${PUGIXML_PUBLIC_DEFINITIONS})
  target_compile_options(pugixml-static
    PRIVATE
      ${msvc-rt-mtd-shared}
      ${msvc-rt-mtd-static}
      ${msvc-rt-mt-shared}
      ${msvc-rt-mt-static})
endif()

if (BUILD_SHARED_LIBS)
  set(pugixml-alias pugixml-shared)
else()
  set(pugixml-alias pugixml-static)
endif()
add_library(pugixml INTERFACE)
target_link_libraries(pugixml INTERFACE ${pugixml-alias})
add_library(pugixml::pugixml ALIAS pugixml)

set_target_properties(${libs}
  PROPERTIES
    MSVC_RUNTIME_LIBRARY ${CMAKE_MSVC_RUNTIME_LIBRARY}
    EXCLUDE_FROM_ALL ON
    POSITION_INDEPENDENT_CODE ON
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION}
    OUTPUT_NAME pugixml)

set_target_properties(${libs}
  PROPERTIES
    EXCLUDE_FROM_ALL OFF)
set(install-targets pugixml ${libs})

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/scripts/pugixml-config.cmake.in"
  "${PROJECT_BINARY_DIR}/pugixml-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  NO_SET_AND_CHECK_MACRO)

write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/pugixml-config-version.cmake"
  COMPATIBILITY SameMajorVersion)

if (PUGIXML_USE_POSTFIX)
  if(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    set(LIB_POSTFIX ${CMAKE_RELWITHDEBINFO_POSTFIX})
  elseif(CMAKE_BUILD_TYPE MATCHES MinSizeRel)
    set(LIB_POSTFIX ${CMAKE_MINSIZEREL_POSTFIX})
  elseif(CMAKE_BUILD_TYPE MATCHES Debug)
    set(LIB_POSTFIX ${CMAKE_DEBUG_POSTFIX})
  endif()
endif()

configure_file(scripts/pugixml.pc.in pugixml.pc @ONLY)

if (NOT DEFINED PUGIXML_RUNTIME_COMPONENT)
  set(PUGIXML_RUNTIME_COMPONENT Runtime)
endif()

if (NOT DEFINED PUGIXML_LIBRARY_COMPONENT)
  set(PUGIXML_LIBRARY_COMPONENT Library)
endif()

if (NOT DEFINED PUGIXML_DEVELOPMENT_COMPONENT)
  set(PUGIXML_DEVELOPMENT_COMPONENT Development)
endif()

set(namelink-component)
if (NOT CMAKE_VERSION VERSION_LESS 3.12)
  set(namelink-component NAMELINK_COMPONENT ${PUGIXML_DEVELOPMENT_COMPONENT})
endif()
install(TARGETS ${install-targets}
  EXPORT pugixml-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${PUGIXML_RUNTIME_COMPONENT}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${PUGIXML_LIBRARY_COMPONENT} ${namelink-component}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${PUGIXML_DEVELOPMENT_COMPONENT}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}${versioned-dir})

install(EXPORT pugixml-targets
  NAMESPACE pugixml::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pugixml COMPONENT ${PUGIXML_DEVELOPMENT_COMPONENT})

export(EXPORT pugixml-targets
  NAMESPACE pugixml::)

install(FILES
  "${PROJECT_BINARY_DIR}/pugixml-config-version.cmake"
  "${PROJECT_BINARY_DIR}/pugixml-config.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pugixml COMPONENT ${PUGIXML_DEVELOPMENT_COMPONENT})

install(FILES ${PROJECT_BINARY_DIR}/pugixml.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT ${PUGIXML_DEVELOPMENT_COMPONENT})

install(
  FILES
    "${PROJECT_SOURCE_DIR}/src/pugiconfig.hpp"
    "${PROJECT_SOURCE_DIR}/src/pugixml.hpp"
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}${versioned-dir} COMPONENT ${PUGIXML_DEVELOPMENT_COMPONENT})

if (PUGIXML_BUILD_TESTS)
  set(fuzz-pattern "tests/fuzz_*.cpp")
  set(test-pattern "tests/*.cpp")
  if (CMAKE_VERSION VERSION_GREATER 3.11)
    list(INSERT fuzz-pattern 0 CONFIGURE_DEPENDS)
    list(INSERT test-pattern 0 CONFIGURE_DEPENDS)
  endif()
  file(GLOB test-sources ${test-pattern})
  file(GLOB fuzz-sources ${fuzz-pattern})
  list(REMOVE_ITEM test-sources ${fuzz-sources})

  add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)

  add_executable(pugixml-check ${test-sources})
  add_test(NAME pugixml::test
    COMMAND pugixml-check
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  add_dependencies(check pugixml-check)
  target_link_libraries(pugixml-check
    PRIVATE
      pugixml::pugixml)
endif()
