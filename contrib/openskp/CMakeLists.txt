cmake_minimum_required(VERSION 3.21)

project(
  OpenSkp
  VERSION 1.3.0
  DESCRIPTION "Open-source SketchUp (.skp) binary file parser, writer, and converter - extract geometry, metadata, layers, and materials; create and edit .skp files; convert to GLB, OBJ, STL, PLY, DXF, and IFC4"
  HOMEPAGE_URL "https://openskp.com"
  LANGUAGES C CXX
)

include(CMakePackageConfigHelpers)
include(FetchContent)
include(GNUInstallDirs)

if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

# Build options

if(PROJECT_IS_TOP_LEVEL)
  set(_openskp_tests_default ON)
else()
  set(_openskp_tests_default OFF)
endif()

option(OPENSKP_BUILD_TESTS "Build OpenSkp tests" ${_openskp_tests_default})
option(OPENSKP_BUILD_EXAMPLES "Build OpenSkp examples" OFF)
option(OPENSKP_BUILD_WASM "Build OpenSkp WebAssembly / Emscripten module" OFF)

# Developer tooling

find_program(
  OPENSKP_CLANG_FORMAT_EXECUTABLE
  NAMES clang-format-18 clang-format
  DOC "clang-format executable used by the OpenSKP formatting targets"
)

file(
  GLOB_RECURSE OPENSKP_FORMAT_FILES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)

if(OPENSKP_CLANG_FORMAT_EXECUTABLE)
  add_custom_target(
    openskp-format
    COMMAND ${OPENSKP_CLANG_FORMAT_EXECUTABLE} -i ${OPENSKP_FORMAT_FILES}
    COMMENT "Formatting OpenSKP C++ sources"
    COMMAND_EXPAND_LISTS
    VERBATIM
  )

  add_custom_target(
    openskp-format-check
    COMMAND ${OPENSKP_CLANG_FORMAT_EXECUTABLE} --dry-run --Werror ${OPENSKP_FORMAT_FILES}
    COMMENT "Checking OpenSKP C++ source formatting"
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
else()
  message(STATUS "clang-format not found; openskp-format targets are unavailable")
endif()

# Private dependencies

FetchContent_Declare(
  miniz
  GIT_REPOSITORY https://github.com/richgel999/miniz.git
  GIT_TAG 77d0dce8627735138c51770d1799a1ef48f2117d
  GIT_SHALLOW FALSE
)
FetchContent_GetProperties(miniz)

if(NOT miniz_POPULATED)
  FetchContent_Populate(miniz)
endif()

set(OPENSKP_MINIZ_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/miniz_include")
file(MAKE_DIRECTORY "${OPENSKP_MINIZ_INCLUDE_DIR}")
configure_file(
  cmake/miniz_export.h.in
  "${OPENSKP_MINIZ_INCLUDE_DIR}/miniz_export.h"
  COPYONLY
)

FetchContent_Declare(
  tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG 488a70a3df62a4df1a736e9e56fb8836580c4888
  GIT_SHALLOW FALSE
)
FetchContent_GetProperties(tinygltf)

if(NOT tinygltf_POPULATED)
  FetchContent_Populate(tinygltf)
endif()

# Header-only C++ runtime for reading/writing the vendored ThatOpen
# Fragments (.frag) FlatBuffers schema (include/openskp/_fragments_fb/
# index.fbs, generated via `flatc --cpp`) - same role Python's
# `flatbuffers` pip package plays there. No sources to compile: this
# pulls in only its `include/flatbuffers/*.h` headers.
FetchContent_Declare(
  flatbuffers
  GIT_REPOSITORY https://github.com/google/flatbuffers.git
  GIT_TAG v25.12.19
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(flatbuffers)

if(NOT flatbuffers_POPULATED)
  FetchContent_Populate(flatbuffers)
endif()

# Library target

set(
  OPENSKP_SOURCES
  src/codegen.cpp
  src/core.cpp
  src/create.cpp
  src/dxf_export.cpp
  src/earcut.cpp
  src/edit.cpp
  src/errors.cpp
  src/face_groups.cpp
  src/fragments_export.cpp
  src/geometry.cpp
  src/glb.cpp
  src/ifc_export.cpp
  src/instanced_glb.cpp
  src/instanced_scene.cpp
  src/json_export.cpp
  src/legacy.cpp
  src/model.cpp
  src/obj_export.cpp
  src/observability.cpp
  src/pages_dimensions.cpp
  src/parser.cpp
  src/ply_export.cpp
  src/scene.cpp
  src/stl_export.cpp
  src/tlv.cpp
  src/transforms.cpp
  src/triangulator.cpp
  src/vff.cpp
)

set(
  OPENSKP_MINIZ_SOURCES
  "${miniz_SOURCE_DIR}/miniz.c"
  "${miniz_SOURCE_DIR}/miniz_tdef.c"
  "${miniz_SOURCE_DIR}/miniz_tinfl.c"
  "${miniz_SOURCE_DIR}/miniz_zip.c"
)

add_library(OpenSkp ${OPENSKP_SOURCES} ${OPENSKP_MINIZ_SOURCES})
add_library(OpenSkp::OpenSkp ALIAS OpenSkp)

target_compile_features(OpenSkp PUBLIC cxx_std_17)
target_include_directories(
  OpenSkp
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${OPENSKP_MINIZ_INCLUDE_DIR}"
    "${miniz_SOURCE_DIR}"
    "${tinygltf_SOURCE_DIR}"
    "${flatbuffers_SOURCE_DIR}/include"
)

set_target_properties(
  OpenSkp
  PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME OpenSkp
)

include(GenerateExportHeader)

set(OPENSKP_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
generate_export_header(
  OpenSkp
  EXPORT_FILE_NAME "${OPENSKP_GENERATED_INCLUDE_DIR}/openskp/export.hpp"
)
target_include_directories(
  OpenSkp
  PUBLIC "$<BUILD_INTERFACE:${OPENSKP_GENERATED_INCLUDE_DIR}>"
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(OpenSkp PUBLIC OPENSKP_STATIC_DEFINE)
endif()

if(MSVC)
  target_compile_definitions(OpenSkp PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(OPENSKP_BUILD_WASM AND NOT EMSCRIPTEN)
  message(FATAL_ERROR "Building WebAssembly target (OPENSKP_BUILD_WASM) requires the Emscripten toolchain (emcmake cmake ...).")
endif()

if(EMSCRIPTEN OR OPENSKP_BUILD_WASM)
  target_compile_options(OpenSkp PRIVATE -O3 -fexceptions)
  add_executable(openskp_wasm src/wasm_api.cpp)
  target_compile_options(openskp_wasm PRIVATE -O3 -fexceptions)
  target_link_libraries(openskp_wasm PRIVATE OpenSkp)
  target_include_directories(
    openskp_wasm
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${OPENSKP_MINIZ_INCLUDE_DIR}"
      "${miniz_SOURCE_DIR}"
      "${tinygltf_SOURCE_DIR}"
      "${flatbuffers_SOURCE_DIR}/include"
  )
  set_target_properties(
    openskp_wasm
    PROPERTIES
      OUTPUT_NAME "openskp"
      SUFFIX ".js"
  )
  target_link_options(
    openskp_wasm
    PRIVATE
      "--bind"
      "-sWASM=1"
      "-sALLOW_MEMORY_GROWTH=1"
      # Without an explicit ceiling, the WASM heap hits an internal
      # allocation failure - caught generically and surfaced as a
      # misleading "legacy .skp parse failed" - at a MUCH lower size than
      # the ~4GB a browser tab actually allows (openskp#305). A modest
      # initial size avoids repeated grow-and-copy on any real file.
      "-sMAXIMUM_MEMORY=4GB"
      "-sINITIAL_MEMORY=268435456"
      "-sMODULARIZE=1"
      "-sEXPORT_ES6=1"
      "-sEXPORT_NAME=OpenSkpModule"
      "-sENVIRONMENT=web,worker,node"
      "-sDYNAMIC_EXECUTION=0"
      "-fexceptions"
      "-O3"
  )
endif()

# Installation and CMake package

set(OPENSKP_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/OpenSkp")

install(
  TARGETS OpenSkp
  EXPORT OpenSkpTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/openskp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(
  FILES "${OPENSKP_GENERATED_INCLUDE_DIR}/openskp/export.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/openskp"
)
install(
  EXPORT OpenSkpTargets
  FILE OpenSkpTargets.cmake
  NAMESPACE OpenSkp::
  DESTINATION "${OPENSKP_INSTALL_CMAKE_DIR}"
)

configure_package_config_file(
  cmake/OpenSkpConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/OpenSkpConfig.cmake"
  INSTALL_DESTINATION "${OPENSKP_INSTALL_CMAKE_DIR}"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/OpenSkpConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/OpenSkpConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/OpenSkpConfigVersion.cmake"
  DESTINATION "${OPENSKP_INSTALL_CMAKE_DIR}"
)

# Optional tests and examples

if(OPENSKP_BUILD_TESTS)
  enable_testing()

  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723
    GIT_SHALLOW FALSE
  )
  FetchContent_MakeAvailable(googletest)

  add_subdirectory(tests)
endif()

if(OPENSKP_BUILD_EXAMPLES)
  add_executable(openskp_info examples/info.cpp)
  target_link_libraries(openskp_info PRIVATE OpenSkp::OpenSkp)

  add_executable(openskp_export_glb examples/export_glb.cpp)
  target_link_libraries(openskp_export_glb PRIVATE OpenSkp::OpenSkp)

  add_executable(openskp_bench examples/bench.cpp)
  target_link_libraries(openskp_bench PRIVATE OpenSkp::OpenSkp)

  add_executable(openskp_nametest examples/nametest.cpp)
  target_link_libraries(openskp_nametest PRIVATE OpenSkp::OpenSkp)

  add_executable(openskp_fragtest examples/fragtest.cpp)
  target_link_libraries(openskp_fragtest PRIVATE OpenSkp::OpenSkp)

  add_executable(openskp_parsetest examples/parsetest.cpp)
  target_link_libraries(openskp_parsetest PRIVATE OpenSkp::OpenSkp)

  add_executable(openskp_ifctest examples/ifctest.cpp)
  target_link_libraries(openskp_ifctest PRIVATE OpenSkp::OpenSkp)
endif()
