cmake_minimum_required(VERSION 3.16)

set(TINYUSDZ_TARGET "tinyusdz") # TODO: drop `z`
set(TINYUSDZ_TARGET_NS "tinyusdz::tinyusdz") # dll lib. may not be built.
set(TINYUSDZ_TARGET_STATIC_NS "tinyusdz::tinyusdz_static") # static lib is always built.
set(TINYUSDZ_TARGET_STATIC "tinyusdz_static") # alias to ${TINYUSDZ_TARGET_STATIC_NS}
set(BUILD_TARGET_C "c-tinyusd")
set(BUILD_TARGET_PY "pytinyusd")
#set(BUILD_TARGET_BLENDER_PY "tinyusd_blender")
set(TINYUSDZ_TEST_TARGET "test_tinyusdz")
set(TINYUSDZ_BENCHMARK_TARGET "benchmark_tinyusdz")

project(${TINYUSDZ_TARGET} C CXX)

# PROJECT_IS_TOP_LEVEL is available from cmake 3.21
if (CMAKE_VERSION VERSION_LESS 3.21)
  if ("${CMAKE_PROJECT_NAME}" STREQUAL "${TINYUSDZ_TARGET}")
    set(PROJECT_IS_TOP_LEVEL ON)
  endif()
endif ()


if (EMSCRIPTEN)
  set(TINYUSDZ_DEFAULT_NO_WERROR ON)
  set(TINYUSDZ_DEFAULT_PRODUCTION_BUILD On)
  set(TINYUSDZ_DEFAULT_WITH_BUILTIN_IMAGE_LOADER On)
  set(TINYUSDZ_DEFAULT_BUILD_TESTS Off)
  set(TINYUSDZ_DEFAULT_EXAMPLES Off)
  set(TINYUSDZ_DEFAULT_WITH_C_API Off)
  set(TINYUSDZ_DEFAULT_WITH_PXR_COMPAT_API Off)
  # TODO: deprecate in next major version
  set(TINYUSDZ_DEFAULT_WITH_USDA_PARSER Off)
  set(TINYUSDZ_DEFAULT_WITH_USDC_PARSER Off)
elseif(NOT PROJECT_IS_TOP_LEVEL)
  # assume tinyusdz is added from add_subdirectory()
  # disable tools, tests and examples build by default.
  set(TINYUSDZ_DEFAULT_NO_WERROR OFF)
  set(TINYUSDZ_DEFAULT_PRODUCTION_BUILD Off)
  set(TINYUSDZ_DEFAULT_WITH_BUILTIN_IMAGE_LOADER On)
  set(TINYUSDZ_DEFAULT_BUILD_TESTS Off)
  set(TINYUSDZ_DEFAULT_EXAMPLES Off)
  set(TINYUSDZ_DEFAULT_WITH_C_API Off)
  set(TINYUSDZ_DEFAULT_WITH_PXR_COMPAT_API On)
  # TODO: deprecate in next major version
  set(TINYUSDZ_DEFAULT_WITH_USDA_PARSER Off)
  set(TINYUSDZ_DEFAULT_WITH_USDC_PARSER Off)
else()
  set(TINYUSDZ_DEFAULT_NO_WERROR OFF)
  set(TINYUSDZ_DEFAULT_PRODUCTION_BUILD Off)
  set(TINYUSDZ_DEFAULT_WITH_BUILTIN_IMAGE_LOADER On)
  set(TINYUSDZ_DEFAULT_BUILD_TESTS On)
  set(TINYUSDZ_DEFAULT_BUILD_EXAMPLES On)
  set(TINYUSDZ_DEFAULT_WITH_C_API Off)
  set(TINYUSDZ_DEFAULT_WITH_PXR_COMPAT_API On)
  # TODO: deprecate in next major version
  set(TINYUSDZ_DEFAULT_WITH_USDA_PARSER Off)
  set(TINYUSDZ_DEFAULT_WITH_USDC_PARSER Off)

  # For Visual Studio
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

# options
option(TINYUSDZ_USE_CCACHE "Use ccache for faster recompile." ON)
option(TINYUSDZ_BUILD_SHARED_LIBS "Build as dll?" ${BUILD_SHARED_LIBS})
option(TINYUSDZ_ENABLE_THREAD "Build with C++11 std::thread support?(threading support is not implemented yet)" OFF)
option(TINYUSDZ_WITH_C_API "Enable C API." ${TINYUSDZ_DEFAULT_WITH_C_API})
option(TINYUSDZ_BUILD_TESTS "Build tests" ${TINYUSDZ_DEFAULT_BUILD_TESTS})
option(TINYUSDZ_BUILD_BENCHMARKS
       "Build some bechmark tests(e.g. internal C++ data structures)" OFF)
option(TINYUSDZ_BUILD_EXAMPLES
       "Build examples(but not all examples area built in `examples` folder)"
       ${TINYUSDZ_DEFAULT_BUILD_EXAMPLES})
option(TINYUSDZ_WITH_BUILTIN_IMAGE_LOADER
       "Build with built-in image loader(stb_image and fpng). When disabled, app need to provide image loader callback to load images."
       ${TINYUSDZ_DEFAULT_WITH_BUILTIN_IMAGE_LOADER})

if(MSVC)
  # it looks it is hard to disable C++ exception for MSVC, so enable exception by default
  set(TINYUSDZ_CXX_EXCEPTIONS_DEFAULT On)
else()
  set(TINYUSDZ_CXX_EXCEPTIONS_DEFAULT Off)
endif()

# Enable /MP flags(multi processor build) for Visual Studio
# Based on VTK's Cmake
# No need to set this if you use Ninja Generator
if(MSVC)
  # default on.
  # disable MP or reduce cores if you have less memory
  set(TINYUSDZ_CXX_MP_FLAG ON CACHE BOOL "Build with /MP flag enabled")
  set(TINYUSDZ_PROCESSOR_COUNT "$ENV{NUMBER_OF_PROCESSORS}")
  set(TINYUSDZ_CXX_MP_NUM_PROCESSORS ${TINYUSDZ_PROCESSOR_COUNT} CACHE STRING "The maximum number of processes for the /MP flag")


  message(STATUS "Use procesors : " ${TINYUSDZ_PROCESSOR_COUNT})

  # /MP flag is set as target_compile_options, so do not add /MP to global CMAKE_CXX_FLAGS variable.
  #if (CMAKE_CXX_MP_FLAG)
  #  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP${CMAKE_CXX_MP_NUM_PROCESSORS}")
  #  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP${CMAKE_CXX_MP_NUM_PROCESSORS}")
  #endif ()
endif()

option(TINYUSDZ_CXX_EXCEPTIONS
       "Enable/Disable C++ exceptions(default: Off for posix, on for MSVC)"
       ${TINYUSDZ_CXX_EXCEPTIONS_DEFAULT})

option(TINYUSDZ_WITH_USDMTLX "Build with MaterialX support" ON)
option(TINYUSDZ_WITH_JSON "Build with JSON serialization support" OFF)
option(TINYUSDZ_WITH_USDOBJ "Build with usdObj support(import wavefront .obj)"
       ON)
option(TINYUSDZ_WITH_USDFBX "Build with usdFbx support(import FBX .fbx)"
       OFF)
option(TINYUSDZ_WITH_USDVOX
       "Build with usdVox support(import MagicaVoxel .vox)" ON)
option(
  TINYUSDZ_WITH_OPENSUBDIV
  "Build with OpenSubdiv(osdCPU. if required, set `osd_DIR` to specify the path to your own OpenSubdiv)"
  OFF)
# Use embedded version of OpenSubdiv code by default
set(osd_DIR ${PROJECT_SOURCE_DIR}/src/osd)

option(TINYUSDZ_WITH_AUDIO "Build with Audio support(MP3 and WAV)" ON)
option(TINYUSDZ_WITH_ALAC_AUDIO "Build with ALAC(as M4A) Audio support" OFF)

option(TINYUSDZ_WITH_PYTHON "Build with Python binding through pybind11" OFF)

if (TINYUSDZ_WITH_PYTHON)
  # force enable C API + DLL build
  set(TINYUSDZ_WITH_C_API ON CACHE INTERNAL "" FORCE)
  set(TINYUSDZ_BUILD_SHARED_LIBS ON CACHE INTERNAL "" FORCE)
endif()


option(
  TINYUSDZ_PREFER_LOCAL_PYTHON_INSTALLATION
  "Prefer locally-installed Python interpreter than system or conda/brew installed Python. Please specify your Python interpreter with `Python3_EXECUTABLE` cmake option"
  OFF)

option(TINYUSDZ_WITH_PXR_COMPAT_API "Build with pxr compatible API" ${TINYUSDZ_DEFAULT_WITH_PXR_COMPAT_API})

# deprecated. to be removed
#option(
#  TINYUSDZ_WITH_BLENDER_ADDON
#  "Build with Python module for Blender(`TINYUSDZ_WITH_PYTHON` is force set to ON"
#  OFF)

option(TINYUSDZ_USE_SYSTEM_ZLIB
       "Use system's zlib instead of miniz for TinyEXR/TIFF" OFF)


option(
  TINYUSDZ_PRODUCTION_BUILD
  "Build for production release(e.g. disables debug print, do not include full filepath in error messages)"
  OFF)


option(
  TINYUSDZ_DEBUG_PRINT
  "Enable debug print(For debugging). Disabled when `TINYUSDZ_PRODUCTION_BUILD` is On"
  OFF)

# -- modules --
option(TINYUSDZ_WITH_MODULE_USDA_READER "Build with USDA reader feature" ON)
option(TINYUSDZ_WITH_MODULE_USDA_WRITER "Build with USDA writer feature" ON)

# USDC(Crate binary) reader is mandatory in most situation, so this option is primarily for developer build.
option(TINYUSDZ_WITH_MODULE_USDC_READER "Build with USDC reader feature" ON)
option(TINYUSDZ_WITH_MODULE_USDC_WRITER "Build with USDC writer feature" ON)
# -------------

# -- STB --

#
# In default TinyUSDZ embeds stb_image and stb_image_write to load/save JPG/PNG/BMP images.
# (When TINYUSDZ_WITH_BUILTIN_IMAGE_LOADER is set On)
#

# Set ON if you use stb_image implementation outside of TinyUSDZ library to avoid multiple symbol def error.
option(TINYUSDZ_NO_STB_IMAGE_IMPLEMENTATION
       "Do not define STB_IMAGE_IMPLEMENTATION" OFF)

# Set ON if you use stb_image_write implementation outside of TinyUSDZ library in your app to avoid multiple symbol def error.
option(TINYUSDZ_NO_STB_IMAGE_WRITE_IMPLEMENTATION
       "Do not define STB_IMAGE_WRITGE_IMPLEMENTATION" OFF)
# ----------

# -- Wuffs --

#
# Use wuffs to decode jpeg/png/bmp instead of using stb_image.
# (wuffs is well(I guess) fuzz tested image decoder, so has better security than stb_image)
#
option(TINYUSDZ_USE_WUFFS_IMAGE_LOADER
       "Use wuffs to load jpg/png/bmp images" OFF)

# Set ON if you use wuffs implementation outside of TinyUSDZ library in your app to avoid multiple symbol def error.
option(TINYUSDZ_NO_WUFFS_IMPLEMENTATION
       "Do not define WUFFS_IMPLEMENTATION" OFF)
# -----------

#

# -- TIFF --
option(TINYUSDZ_WITH_TIFF
       "Build with TIFF texture(includes 32bit floating point TIFF) support"
       OFF)
# ----------

# -- EXR --
option(TINYUSDZ_WITH_EXR "Build with EXR HDR texture support" ON)
# ---------

# -- ColorIO --
option(TINYUSDZ_WITH_COLORIO
       "Build with Color IO Baked LUT support(through tinycolorio)" ON)
# ---------

# -- optional tool --
# TODO: deperecate. Use tusdcat
option(TINYUSDZ_WITH_TOOL_USDA_PARSER "Build with USDA parser program" Off)
option(TINYUSDZ_WITH_TOOL_USDC_PARSER "Build with USDC parser program" Off)
# --------------

# -- For developers --
option(TINYUSDZ_COMPILE_TIME_TRACE
       "Add -ftime-trace to profile compilation time(clang only)" OFF)
option(TINYUSDZ_CUSTOM_COMPILE_FLAGS
       "Use hard-coded custom compile flags(described in CMakeLists.txt). For Developer only)" OFF)
# ---

# cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/sanitizers)
find_package(Sanitizers) # Address sanitizer (-DSANITIZE_ADDRESS=ON)

# [cifuzz]
option(TINYUSDZ_TEST_CIFUZZ "Run cifuzz test" OFF)

if (TINYUSDZ_TEST_CIFUZZ)
  find_package(cifuzz NO_SYSTEM_ENVIRONMENT_PATH)
  enable_fuzz_testing()
endif()

if (TINYUSDZ_ENABLE_THREAD)
  find_package(Threads REQUIRED)
  # prefer adding "-pthread" compile flag
  set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()

if(TINYUSDZ_WITH_EXR OR TINYUSDZ_WITH_TIFF)
  if(TINYUSDZ_USE_SYSTEM_ZLIB)
    find_package(ZLIB REQUIRED)
  endif()
endif()


# Currently TinyUSDZ does not claim CXX version when TinyUSDZ is added as add_subdirectory()
# of toplevel(root) project.
# TODO: Property inherit CXX compiler settings from toplevel(root) project.


if (PROJECT_IS_TOP_LEVEL)
  message(STATUS "TinyUSDZ is being built as toplevel project so set CXX standard here.")
  if(TINYUSDZ_WITH_PYTHON)
    #set(CMAKE_CXX_STANDARD 17) # nanobind requires C++17

    # for pybind11
    set(CMAKE_CXX_STANDARD 14)
  else()
    # Require strict C++14 mode(e.g. `-std=c++14`)
    set(CMAKE_CXX_STANDARD 14)
  endif()
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# [ccache]
if(TINYUSDZ_USE_CCACHE)
  if(MSVC)
    # No ccache support
  else()
    find_program(CCACHE_EXE ccache)
    if(CCACHE_EXE)

      message(STATUS "Use ccache : " ${CCACHE_EXE})
      # CMAKE_C_COMPILER_LAUNCHER = available from cmake 3.4+)
      if(CMAKE_C_COMPILER_LAUNCHER)
        set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_C_COMPILER_LAUNCHER}"
                                      "${CCACHE_EXE}")
      else()
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXE}")
      endif()

      if(CMAKE_CXX_COMPILER_LAUNCHER)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_CXX_COMPILER_LAUNCHER}"
                                        "${CCACHE_EXE}")
      else()
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXE}")
      endif()

    endif(CCACHE_EXE)
  endif()
endif()


if(TINYUSDZ_WITH_PYTHON)

  # For the time beging, we stick with pybind11, since PyPI manylinux2014(probably mostly used architectrue as of 2022/Aug) does not support C++17.
  # We may switch to nanobind at some point(probably around 2024?)

  # We build monolithic python module, so build libtinyusdz as a static library
  # with PIC enabled.
  # NOTE: TinyUSDZ now builds static lib with PIC enabled by default, so commented out here.
  # set(CMAKE_POSITION_INDEPENDENT_CODE On)

  #[nanobind]
  #set(NB_SHARED
  #    OFF
  #    CACHE INTERNAL "")
  #set(NB_STATIC
  #    ON
  #    CACHE INTERNAL "")

  # workaround: find_package() does not work well inside of nanobind(`Python3::Module` target won't defined), so call it here.
  # and use `FindPython3`, not `FindPython`
  if(TINYUSDZ_PREFER_LOCAL_PYTHON_INSTALLATION)
    #message(STATUS "Local Python")
    set(Python3_FIND_FRAMEWORK NEVER) # Do not search framework python
    set(Python3_FIND_STRATEGY LOCATION)
    set(Python3_FIND_REGISTRY NEVER) # Windows only
  else()
    set(Python_FIND_FRAMEWORK LAST
    )# Prefer Brew/Conda to Apple framework python
  endif()

  find_package(
    Python3
    COMPONENTS Interpreter Development.Module
    REQUIRED)

  #message(STATUS "Python include dirs: " ${Python_INCLUDE_DIRS})
  message(STATUS "Python include dirs: " ${Python3_INCLUDE_DIRS})

  #add_subdirectory(${PROJECT_SOURCE_DIR}/src/external/nanobind nanobind_build)

  add_subdirectory(${PROJECT_SOURCE_DIR}/src/external/pybind11)
endif()

set(TINYUSDZ_SOURCES
    ${PROJECT_SOURCE_DIR}/src/asset-resolution.cc
    ${PROJECT_SOURCE_DIR}/src/tinyusdz.cc
    ${PROJECT_SOURCE_DIR}/src/xform.cc
    ${PROJECT_SOURCE_DIR}/src/performance.cc
    ${PROJECT_SOURCE_DIR}/src/ascii-parser.cc
    ${PROJECT_SOURCE_DIR}/src/ascii-parser-basetype.cc
    ${PROJECT_SOURCE_DIR}/src/ascii-parser-timesamples.cc
    ${PROJECT_SOURCE_DIR}/src/ascii-parser-timesamples-array.cc
    ${PROJECT_SOURCE_DIR}/src/audio-loader.cc
    ${PROJECT_SOURCE_DIR}/src/usda-reader.cc
    ${PROJECT_SOURCE_DIR}/src/usdc-reader.cc
    ${PROJECT_SOURCE_DIR}/src/usda-writer.cc
    ${PROJECT_SOURCE_DIR}/src/usdc-writer.cc
    ${PROJECT_SOURCE_DIR}/src/composition.cc
    ${PROJECT_SOURCE_DIR}/src/crate-reader.cc
    ${PROJECT_SOURCE_DIR}/src/crate-format.cc
    ${PROJECT_SOURCE_DIR}/src/crate-writer.cc
    ${PROJECT_SOURCE_DIR}/src/crate-pprint.cc
    ${PROJECT_SOURCE_DIR}/src/path-util.cc
    ${PROJECT_SOURCE_DIR}/src/prim-reconstruct.cc
    ${PROJECT_SOURCE_DIR}/src/prim-composition.cc
    ${PROJECT_SOURCE_DIR}/src/prim-types.cc
    ${PROJECT_SOURCE_DIR}/src/primvar.cc
    ${PROJECT_SOURCE_DIR}/src/str-util.cc
    ${PROJECT_SOURCE_DIR}/src/value-pprint.cc
    ${PROJECT_SOURCE_DIR}/src/value-types.cc
    ${PROJECT_SOURCE_DIR}/src/tiny-format.cc
    ${PROJECT_SOURCE_DIR}/src/io-util.cc
    ${PROJECT_SOURCE_DIR}/src/image-loader.cc
    ${PROJECT_SOURCE_DIR}/src/image-writer.cc
    ${PROJECT_SOURCE_DIR}/src/image-util.cc
    ${PROJECT_SOURCE_DIR}/src/linear-algebra.cc
    ${PROJECT_SOURCE_DIR}/src/usdGeom.cc
    ${PROJECT_SOURCE_DIR}/src/usdSkel.cc
    ${PROJECT_SOURCE_DIR}/src/usdShade.cc
    ${PROJECT_SOURCE_DIR}/src/usdLux.cc
    # usdMtlX has less dependency(pugixml), so add it to core component
    ${PROJECT_SOURCE_DIR}/src/usdMtlx.cc
    ${PROJECT_SOURCE_DIR}/src/usdObj.cc
    ${PROJECT_SOURCE_DIR}/src/image-loader.cc
    ${PROJECT_SOURCE_DIR}/src/pprinter.cc
    ${PROJECT_SOURCE_DIR}/src/stage.cc
    ${PROJECT_SOURCE_DIR}/src/tydra/facial.cc
    ${PROJECT_SOURCE_DIR}/src/tydra/prim-apply.cc
    ${PROJECT_SOURCE_DIR}/src/tydra/scene-access.cc
    ${PROJECT_SOURCE_DIR}/src/tydra/shader-network.cc
    ${PROJECT_SOURCE_DIR}/src/tydra/render-data.cc)

if(TINYUSDZ_WITH_PXR_COMPAT_API)
  list(APPEND TINYUSDZ_SOURCES ${PROJECT_SOURCE_DIR}/src/pxr-compat.cc)
endif()

set(TINYUSDZ_C_API_SOURCES ${PROJECT_SOURCE_DIR}/src/c-tinyusd.cc)

set(TINYUSDZ_DEP_SOURCES
    ${PROJECT_SOURCE_DIR}/src/integerCoding.cpp
    ${PROJECT_SOURCE_DIR}/src/lz4-compression.cc
    ${PROJECT_SOURCE_DIR}/src/lz4/lz4.c
    ${PROJECT_SOURCE_DIR}/src/external/pystring.cpp
)

if (TINYUSDZ_WITH_BUILTIN_IMAGE_LOADER)
  list(APPEND TINYUSDZ_DEP_SOURCES  ${PROJECT_SOURCE_DIR}/src/external/fpng.cpp)
  # Disable SSE for a while, since -mpclmul required to use SSE feature of fpng.
  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/external/fpng.cpp
                              PROPERTIES COMPILE_DEFINITIONS "FPNG_NO_SSE=1")
endif()

if(TINYUSDZ_WITH_USDMTLX)
  list(APPEND TINYUSDZ_DEP_SOURCES ${PROJECT_SOURCE_DIR}/src/external/pugixml.cpp)
endif()

if(TINYUSDZ_WITH_JSON)
  list(APPEND TINYUSDZ_DEP_SOURCES ${PROJECT_SOURCE_DIR}/src/json-to-usd.cc)
  list(APPEND TINYUSDZ_DEP_SOURCES ${PROJECT_SOURCE_DIR}/src/usd-to-json.cc)
endif()

if(TINYUSDZ_WITH_USDFBX)
  list(APPEND TINYUSDZ_SOURCES
    ${PROJECT_SOURCE_DIR}/src/usdFbx.cc
  )
  list(APPEND TINYUSDZ_DEP_SOURCES
       ${PROJECT_SOURCE_DIR}/src/external/OpenFBX/src/ofbx.cpp)
endif()

if(TINYUSDZ_WITH_USDOBJ)
  list(APPEND TINYUSDZ_DEP_SOURCES
       ${PROJECT_SOURCE_DIR}/src/external/tiny_obj_loader.cc)
endif()

if(TINYUSDZ_WITH_USDVOX)
  list(APPEND TINYUSDZ_SOURCES ${PROJECT_SOURCE_DIR}/src/usdVox.cc)
endif()

set(TINYUSDZ_PYTHON_BINDING_SOURCES
    ${PROJECT_SOURCE_DIR}/src/python-bindings.cc)

#set(TINYUSDZ_BLENDER_PYTHON_BINDING_SOURCES
#    ${PROJECT_SOURCE_DIR}/src/blender/bindings.cc)

if(TINYUSDZ_WITH_TIFF)
  if(TINYUSDZ_USE_SYSTEM_ZLIB)
    set_source_files_properties(
      ${PROJECT_SOURCE_DIR}/src/external/tiny_dng_loader.cc
      PROPERTIES COMPILE_DEFINITIONS "TINY_DNG_LOADER_USE_SYSTEM_ZLIB=1")
  else()
    set_source_files_properties(
      ${PROJECT_SOURCE_DIR}/src/external/tiny_dng_loader.cc
      PROPERTIES INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/external)
  endif()

  list(APPEND TINYUSDZ_DEP_SOURCES
       ${PROJECT_SOURCE_DIR}/src/external/tinyexr.cc)
endif(TINYUSDZ_WITH_TIFF)

if(TINYUSDZ_WITH_EXR)
  if(TINYUSDZ_USE_SYSTEM_ZLIB)
    set_source_files_properties(
      ${PROJECT_SOURCE_DIR}/src/external/tinyexr.cc
      PROPERTIES COMPILE_DEFINITIONS "TINYEXR_USE_MINIZ=0")
  else()
    set_source_files_properties(
      ${PROJECT_SOURCE_DIR}/src/external/tinyexr.cc
      PROPERTIES INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/external)
  endif()

  list(APPEND TINYUSDZ_DEP_SOURCES
       ${PROJECT_SOURCE_DIR}/src/external/tinyexr.cc)
endif(TINYUSDZ_WITH_EXR)

if(TINYUSDZ_WITH_TIFF OR TINYUSDZ_WITH_EXR)

  if(NOT TINYUSDZ_USE_SYSTEM_ZLIB)
    list(APPEND TINYUSDZ_DEP_SOURCES ${PROJECT_SOURCE_DIR}/src/external/miniz.c)

    # TODO: Set this only for clang, gcc
    set_source_files_properties(
      ${PROJECT_SOURCE_DIR}/src/external/miniz.c
      PROPERTIES COMPILE_DEFINITIONS "_LARGEFILE64_SOURCE=1")
  endif()

endif()

if(TINYUSDZ_WITH_ALAC_AUDIO)
  list(APPEND TINYUSDZ_DEP_SOURCES
       ${PROJECT_SOURCE_DIR}/src/external/alac/codec/EndianPortable.c
       ${PROJECT_SOURCE_DIR}/src/external/alac/codec/ALACBitUtilities.c
       ${PROJECT_SOURCE_DIR}/src/external/alac/codec/ALACDecoder.cpp
       ${PROJECT_SOURCE_DIR}/src/external/alac/codec/ALACEncoder.cpp
       ${PROJECT_SOURCE_DIR}/src/external/alac/codec/ag_dec.c
       ${PROJECT_SOURCE_DIR}/src/external/alac/codec/ag_enc.c
       ${PROJECT_SOURCE_DIR}/src/external/alac/codec/dp_dec.c
       ${PROJECT_SOURCE_DIR}/src/external/alac/codec/dp_enc.c
       ${PROJECT_SOURCE_DIR}/src/external/alac/codec/matrix_dec.c
       ${PROJECT_SOURCE_DIR}/src/external/alac/codec/matrix_enc.c
  )
endif(TINYUSDZ_WITH_ALAC_AUDIO)

if(TINYUSDZ_WITH_OPENSUBDIV)

  # https://stackoverflow.com/questions/41700463/push-pop-a-cmake-variable
  function(ADD_OSD_LIB)

    #set(NO_TUTORIALS
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_EXAMPLES
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_REGRESSION
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_DOC
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_OMP
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_TBB
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_CUDA
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_OPENCL
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_OPENGL
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_TESTS
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_GLTESTS
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_GLFW
    #    ON
    #    CACHE INTERNAL "" FORCE)
    #set(NO_PTEX
    #    ON
    #    CACHE INTERNAL "" FORCE)

    # CMakeLists of OSD package is problematic, so provide our own one.
    list(
      APPEND
      OSD_FAR_SOURCES
      ${osd_DIR}/opensubdiv/far/bilinearPatchBuilder.cpp
      ${osd_DIR}/opensubdiv/far/catmarkPatchBuilder.cpp
      ${osd_DIR}/opensubdiv/far/error.cpp
      ${osd_DIR}/opensubdiv/far/loopPatchBuilder.cpp
      ${osd_DIR}/opensubdiv/far/patchBasis.cpp
      ${osd_DIR}/opensubdiv/far/patchBuilder.cpp
      ${osd_DIR}/opensubdiv/far/patchDescriptor.cpp
      ${osd_DIR}/opensubdiv/far/patchMap.cpp
      ${osd_DIR}/opensubdiv/far/patchTable.cpp
      ${osd_DIR}/opensubdiv/far/patchTableFactory.cpp
      ${osd_DIR}/opensubdiv/far/ptexIndices.cpp
      ${osd_DIR}/opensubdiv/far/stencilTable.cpp
      ${osd_DIR}/opensubdiv/far/stencilTableFactory.cpp
      ${osd_DIR}/opensubdiv/far/stencilBuilder.cpp
      ${osd_DIR}/opensubdiv/far/topologyDescriptor.cpp
      ${osd_DIR}/opensubdiv/far/topologyRefiner.cpp
      ${osd_DIR}/opensubdiv/far/topologyRefinerFactory.cpp)

    # CPU only
    list(
      APPEND
      OSD_OSD_SOURCES
      ${osd_DIR}/opensubdiv/osd/cpuEvaluator.cpp
      ${osd_DIR}/opensubdiv/osd/cpuKernel.cpp
      ${osd_DIR}/opensubdiv/osd/cpuPatchTable.cpp
      ${osd_DIR}/opensubdiv/osd/cpuVertexBuffer.cpp)

    list(APPEND OSD_SDC_SOURCES ${osd_DIR}/opensubdiv/sdc/typeTraits.cpp
         ${osd_DIR}/opensubdiv/sdc/crease.cpp)

    list(
      APPEND
      OSD_VTR_SOURCES
      ${osd_DIR}/opensubdiv/vtr/fvarLevel.cpp
      ${osd_DIR}/opensubdiv/vtr/fvarRefinement.cpp
      ${osd_DIR}/opensubdiv/vtr/level.cpp
      ${osd_DIR}/opensubdiv/vtr/quadRefinement.cpp
      ${osd_DIR}/opensubdiv/vtr/refinement.cpp
      ${osd_DIR}/opensubdiv/vtr/sparseSelector.cpp
      ${osd_DIR}/opensubdiv/vtr/triRefinement.cpp)

    #add_library(osd_cpu_static ${OSD_FAR_SOURCES} ${OSD_SDC_SOURCES} ${OSD_VTR_SOURCES} ${OSD_OSD_SOURCES})
    #target_include_directories(osd_cpu_static PRIVATE ${osd_DIR})

    #if (TINYUSDZ_BUILD_SHARED_LIBS)
    #  add_library(osd_cpu_shared SHARED ${OSD_FAR_SOURCES} ${OSD_SDC_SOURCES} ${OSD_VTR_SOURCES} ${OSD_OSD_SOURCES})
    #  target_include_directories(osd_cpu_shared PRIVATE ${osd_DIR})
    #endif()

    #if (MSVC)
    #  if (TINYUSDZ_CXX_MP_FLAG)
    #    target_compile_options(osd_cpu_static PRIVATE /MP${CMAKE_CXX_MP_NUM_PROCESSORS})
    #    if (TINYUSDZ_BUILD_SHARED_LIBS)
    #      target_compile_options(osd_cpu_shared PRIVATE /MP${CMAKE_CXX_MP_NUM_PROCESSORS})
    #    endif()
    #  endif()
    #endif()

    # Build as Object Library(non archival collections)
    add_library(osd_cpu ${OSD_FAR_SOURCES} ${OSD_SDC_SOURCES} ${OSD_VTR_SOURCES} ${OSD_OSD_SOURCES})
    target_include_directories(osd_cpu PRIVATE ${osd_DIR})
    set_target_properties(osd_cpu PROPERTIES POSITION_INDEPENDENT_CODE ON)
    
    # primarily for Visual Studio Solution Explorer.
    set_target_properties(osd_cpu PROPERTIES FOLDER "osd")
    

    #set(TINYUSDZ_OSD_SOURCES ${OSD_FAR_SOURCES} ${OSD_SDC_SOURCES} ${OSD_VTR_SOURCES} ${OSD_OSD_SOURCES})

  endfunction()

  add_osd_lib()
  #list(APPEND TINYUSDZ_EXT_LIBRARIES $<TARGET_OBJECTS:osd_cpu>)

  list(APPEND TINYUSDZ_SOURCES ${PROJECT_SOURCE_DIR}/src/subdiv.cc)

endif(TINYUSDZ_WITH_OPENSUBDIV)

if(TINYUSDZ_WITH_TIFF OR TINYUSDZ_WITH_EXR)
  if(TINYUSDZ_USE_SYSTEM_ZLIB)
    list(APPEND TINYUSDZ_EXT_LIBRARIES ZLIB::ZLIB)
  endif()
endif()

# Increase warning level for clang.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

  set(TUSDZ_COMPILE_FLAGS
      "-Weverything -Wno-poison-system-directories -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-documentation -Wno-unused-member-function"
  )

  option(TINYUSDZ_NO_WERROR "Don't set -Werror when building the tinyusdz library" ${TINYUSDZ_DEFAULT_NO_WERROR})
  if (NOT TINYUSDZ_NO_WERROR)
    set(TUSDZ_COMPILE_FLAGS "${TUSDZ_COMPILE_FLAGS} -Werror")
  endif()

  # clang17 seems enable -Wunsafe-buffer-usage when `-Weverything` is defined,
  # resulting compilation error for some raw array access without bound check.
  # https://reviews.llvm.org/D138940
  # https://github.com/llvm/llvm-project/issues/64646
  # we need to suppress it as a work around.
  if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER 16)
    set(TUSDZ_COMPILE_FLAGS "${TUSDZ_COMPILE_FLAGS} -Wno-unsafe-buffer-usage")
  endif()

  if(TINYUSDZ_COMPILE_TIME_TRACE)
    set(TUSDZ_COMPILE_FLAGS "${TUSDZ_COMPILE_FLAGS} -ftime-trace ")
  endif()

  set_source_files_properties(${TINYUSDZ_SOURCES}
                              PROPERTIES COMPILE_FLAGS ${TUSDZ_COMPILE_FLAGS})

  if (TINYUSDZ_WITH_C_API)
    set_source_files_properties(${TINYUSDZ_C_API_SOURCES}
                                PROPERTIES COMPILE_FLAGS ${TUSDZ_COMPILE_FLAGS})
  endif()

endif()

if(TINYUSDZ_CUSTOM_COMPILE_FLAGS)
  #
  # Overwrite existing TUSDZ_COMPILE_FLAGS
  #

  #if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(TUSDZ_CUSTOM_COMPILE_FLAGS "-fstack-usage ")
    set_source_files_properties(${TINYUSDZ_SOURCES}
                              PROPERTIES COMPILE_FLAGS ${TUSDZ_CUSTOM_COMPILE_FLAGS})

    if (TINYUSDZ_WITH_C_API)
      set_source_files_properties(${TINYUSDZ_C_API_SOURCES}
                                  PROPERTIES COMPILE_FLAGS ${TUSDZ_CUSTOM_COMPILE_FLAGS})
    endif()
  #endif()
endif()

#
# -- target
#

#
# Disabled Object library for a while, since it has some issue on MSVC target.
## To save compile time, use Object library(non-archival collections) for SHARED and Static lib.
## NOTE: Static lib is always being built with PIC enabled.
##
##
#add_library(tinyusdz_object OBJECT ${TINYUSDZ_SOURCES} ${TINYUSDZ_DEP_SOURCES} ${TINYUSDZ_EXT_SOURCES})
#set_target_properties(tinyusdz_object PROPERTIES FOLDER "tinyusdz")
#set_target_properties(tinyusdz_object PROPERTIES POSITION_INDEPENDENT_CODE ON)

# TinyUSDZ should compile well without /bigobj flag.
# Uncommend if you encounter the following errror: fatal  error C1128: number of sections exceeded object file format limit: compile with /bigobj
#if(MSVC)
#  target_compile_options(tinyusdz_object PRIVATE /bigobj)
#elif(WIN32)
#  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#    # guess mingw-gcc
#    target_compile_options(tinyusdz_object PRIVATE "-Wa, -mbig-obj")
#  endif()
#endif()

#if (MSVC)
#  if (TINYUSDZ_CXX_MP_FLAG)
#    target_compile_options(tinyusdz_object PRIVATE /MP${CMAKE_CXX_MP_NUM_PROCESSORS})
#  endif()
#endif()
#
#if(TINYUSDZ_DEBUG_PRINT)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_DEBUG_PRINT")
#endif()
#
#if(TINYUSDZ_PRODUCTION_BUILD)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_PRODUCTION_BUILD")
#endif()
#
## default = enable module, so invert definition
#if(NOT TINYUSDZ_WITH_MODULE_USDA_READER)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_DISABLE_MODULE_USDA_READER")
#endif()
#
#if(NOT TINYUSDZ_WITH_MODULE_USDA_WRITER)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_DISABLE_MODULE_USDA_WRITER")
#endif()
#
#if(NOT TINYUSDZ_WITH_MODULE_USDC_READER)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_DISABLE_MODULE_USDC_READER")
#endif()
#
#if(NOT TINYUSDZ_WITH_MODULE_USDC_WRITER)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_DISABLE_MODULE_USDC_WRITER")
#endif()
#
#if(NOT TINYUSDZ_WITH_BUILTIN_IMAGE_LOADER)
#  target_compile_definitions(tinyusdz_object PRIVATE "TINYUSDZ_NO_BUILTIN_IMAGE_LOADER")
#endif()
#
#if(TINYUSDZ_NO_STB_IMAGE_IMPLEMENTATION)
#  target_compile_definitions(tinyusdz_object PRIVATE "TINYUSDZ_NO_STB_IMAGE_IMPLEMENTATION")
#endif()
#
#if(TINYUSDZ_NO_STB_IMAGE_WRITE_IMPLEMENTATION)
#  target_compile_definitions(tinyusdz_object PRIVATE "TINYUSDZ_NO_STB_IMAGE_WRITE_IMPLEMENTATION")
#endif()
#
#target_include_directories(tinyusdz_object
#                           PRIVATE ${PROJECT_SOURCE_DIR}/src)

#if(TINYUSDZ_WITH_JSON)
##target_include_directories(
##  ${TINYUSDZ_LIB_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/src/external/jsonhpp/)
#  target_include_directories(
#    ${TINYUSDZ_LIB_TARGET}
#    PRIVATE ${PROJECT_SOURCE_DIR}/src/external/jsonhpp/)
#endif()

#if(TINYUSDZ_WITH_USDMTLX)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_USE_USDMTLX")
#endif()
#
#if(TINYUSDZ_WITH_USDFBX)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_USE_USDFBX")
#endif()
#
#if(TINYUSDZ_WITH_USDOBJ)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_USE_USDOBJ")
#endif()
#
#if(TINYUSDZ_WITH_USDVOX)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_USE_USDVOX")
#endif()

#target_link_libraries(tinyusdz_object ${TINYUSDZ_EXT_LIBRARIES}
#                      ${CMAKE_DL_LIBS})
#
#if (TINYUSDZ_ENABLE_THREAD)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_ENABLE_THREAD")
#  target_link_libraries(tinyusdz_object Threads::Threads)
#endif()
#
#
#if(IOS)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_BUILD_IOS")
#endif()
#
#if(TINYUSDZ_WITH_TIFF)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_WITH_TIFF")
#endif(TINYUSDZ_WITH_TIFF)
#
#if(TINYUSDZ_WITH_EXR)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_WITH_EXR")
#endif(TINYUSDZ_WITH_EXR)
#
#if(TINYUSDZ_WITH_COLORIO)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_WITH_COLORIO")
#endif(TINYUSDZ_WITH_COLORIO)
#
#if(TINYUSDZ_WITH_AUDIO)
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_WITH_AUDIO")
#endif(TINYUSDZ_WITH_AUDIO)
#
#if(TINYUSDZ_WITH_ALAC_AUDIO)
#  target_compile_definitions(tinuusdz_object
#                             PRIVATE "TINYUSDZ_WITH_ALAC_AUDIO")
#endif(TINYUSDZ_WITH_ALAC_AUDIO)
#
#if(TINYUSDZ_WITH_OPENSUBDIV)
#  target_include_directories(tinyusdz_object PRIVATE ${osd_DIR})
#  target_compile_definitions(tinyusdz_object
#                             PRIVATE "TINYUSDZ_WITH_OPENSUBDIV")
#endif(TINYUSDZ_WITH_OPENSUBDIV)
#
#if(NOT TINYUSDZ_CXX_EXCEPTIONS)
#  if(MSVC)
#    target_compile_options(tinyusdz_object PUBLIC /EHs-c-)
#  else()
#    target_compile_options(tinyusdz_object PUBLIC -fno-exceptions)
#  endif()
#endif()
#
# TODO: USE set_target_properties(... PROPERTIES OUTPUT_NAME outname)?
#add_library(${TINYUSDZ_TARGET_STATIC} STATIC $<TARGET_OBJECTS:tinyusdz_object>)
#add_sanitizers(${TINYUSDZ_TARGET_STATIC})

add_library(${TINYUSDZ_TARGET_STATIC} STATIC ${TINYUSDZ_SOURCES} ${TINYUSDZ_DEP_SOURCES} ${TINYUSDZ_EXT_SOURCES})
set_target_properties(${TINYUSDZ_TARGET_STATIC} PROPERTIES POSITION_INDEPENDENT_CODE ON)
#set_target_properties(tinyusdz_object PROPERTIES FOLDER "tinyusdz")

# Target with namespace
add_library(${TINYUSDZ_TARGET_STATIC_NS} ALIAS ${TINYUSDZ_TARGET_STATIC})


if(TINYUSDZ_BUILD_SHARED_LIBS)
  #add_library(
  #  ${TINYUSDZ_TARGET} SHARED $<TARGET_OBJECTS:tinyusdz_object>)
  add_library(
    ${TINYUSDZ_TARGET} SHARED ${TINYUSDZ_SOURCES} ${TINYUSDZ_DEP_SOURCES} ${TINYUSDZ_EXT_SOURCES})
  #add_sanitizers(${TINYUSDZ_TARGET})

  # for dllexport
  target_compile_definitions(${TINYUSDZ_TARGET} PRIVATE "TINYUSDZ_COMPILE_LIBRARY")
  target_compile_definitions(${TINYUSDZ_TARGET} PRIVATE "TINYUSDZ_SHARED_LIBRARY")

  set(TINYUSDZ_LIBS ${TINYUSDZ_TARGET_STATIC} ${TINYUSDZ_TARGET})
else()
  # static only
  set(TINYUSDZ_LIBS ${TINYUSDZ_TARGET_STATIC})
endif()

if(WIN32 AND TINYUSDZ_WITH_PXR_COMPAT_API)
  if(TINYUSDZ_BUILD_SHARED_LIBS)
    target_compile_definitions(${TINYUSDZ_TARGET} PRIVATE "PXR_DYNAMIC")
    target_compile_definitions(${TINYUSDZ_TARGET} PRIVATE "USD_EXPORTS")
  endif()

endif()

#
# CMake's Object libary have some issue on MSVC build, so build STATIC and SHARED lib indepdendently.
#

#
# -- common target props
#
foreach(TINYUSDZ_LIB_TARGET ${TINYUSDZ_LIBS})

  # Although TinyUSDZ should compile well without /bigobj flag,
  # corrent TinyYSDZ need to use bigobj format otherwise we'll get: fatal  error C1128: number of sections exceeded object file format limit: compile with /bigobj
  if(MSVC)
    target_compile_options(${TINYUSDZ_LIB_TARGET} PRIVATE /bigobj)
  elseif(WIN32)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      # guess mingw-gcc
      target_compile_options(${TINYUSDZ_LIB_TARGET} PRIVATE "-Wa, -mbig-obj")
    endif()
  endif()

  if (MSVC)
    if (TINYUSDZ_CXX_MP_FLAG)
      target_compile_options(${TINYUSDZ_LIB_TARGET} PRIVATE /MP${CMAKE_CXX_MP_NUM_PROCESSORS})
    endif()
  endif()

  if(TINYUSDZ_DEBUG_PRINT)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_DEBUG_PRINT")
  endif()

  if(TINYUSDZ_PRODUCTION_BUILD)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_PRODUCTION_BUILD")
  endif()

  # default = enable module, so invert definition
  if(NOT TINYUSDZ_WITH_MODULE_USDA_READER)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_DISABLE_MODULE_USDA_READER")
  endif()

  if(NOT TINYUSDZ_WITH_MODULE_USDA_WRITER)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_DISABLE_MODULE_USDA_WRITER")
  endif()

  if(NOT TINYUSDZ_WITH_MODULE_USDC_READER)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_DISABLE_MODULE_USDC_READER")
  endif()

  if(NOT TINYUSDZ_WITH_MODULE_USDC_WRITER)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_DISABLE_MODULE_USDC_WRITER")
  endif()

  if(TINYUSDZ_NO_STB_IMAGE_IMPLEMENTATION)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET} PRIVATE "TINYUSDZ_NO_STB_IMAGE_IMPLEMENTATION")
  endif()

  if(TINYUSDZ_NO_STB_IMAGE_WRITE_IMPLEMENTATION)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET} PRIVATE "TINYUSDZ_NO_STB_IMAGE_WRITE_IMPLEMENTATION")
  endif()

  target_include_directories(${TINYUSDZ_LIB_TARGET}
                             PRIVATE ${PROJECT_SOURCE_DIR}/src)

  #if(TINYUSDZ_WITH_JSON)
  ##target_include_directories(
  ##  ${TINYUSDZ_LIB_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/src/external/jsonhpp/)
  #  target_include_directories(
  #    ${TINYUSDZ_LIB_TARGET}
  #    PRIVATE ${PROJECT_SOURCE_DIR}/src/external/jsonhpp/)
  #endif()

  if(TINYUSDZ_WITH_USDMTLX)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_USE_USDMTLX")
  endif()

  if(TINYUSDZ_WITH_USDFBX)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_USE_USDFBX")
  endif()

  if(TINYUSDZ_WITH_USDOBJ)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_USE_USDOBJ")
  endif()

  if(TINYUSDZ_WITH_USDVOX)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_USE_USDVOX")
  endif()

  target_link_libraries(${TINYUSDZ_LIB_TARGET} ${TINYUSDZ_EXT_LIBRARIES}
                        ${CMAKE_DL_LIBS})

  if (TINYUSDZ_ENABLE_THREAD)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_ENABLE_THREAD")
    target_link_libraries(${TINYUSDZ_LIB_TARGET} Threads::Threads)
  endif()


  if(IOS)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_BUILD_IOS")
  endif()

  if(TINYUSDZ_WITH_TIFF)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_WITH_TIFF")
  endif(TINYUSDZ_WITH_TIFF)

  if(TINYUSDZ_WITH_EXR)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_WITH_EXR")
  endif(TINYUSDZ_WITH_EXR)

  if(TINYUSDZ_WITH_COLORIO)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_WITH_COLORIO")
  endif(TINYUSDZ_WITH_COLORIO)

  if(TINYUSDZ_WITH_AUDIO)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_WITH_AUDIO")
  endif(TINYUSDZ_WITH_AUDIO)

  if(TINYUSDZ_WITH_ALAC_AUDIO)
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_WITH_ALAC_AUDIO")
  endif(TINYUSDZ_WITH_ALAC_AUDIO)

  if(TINYUSDZ_WITH_OPENSUBDIV)
    target_include_directories(${TINYUSDZ_LIB_TARGET} PRIVATE ${osd_DIR})
    target_compile_definitions(${TINYUSDZ_LIB_TARGET}
                               PRIVATE "TINYUSDZ_WITH_OPENSUBDIV")
  endif(TINYUSDZ_WITH_OPENSUBDIV)

  if(NOT TINYUSDZ_CXX_EXCEPTIONS)
    if(MSVC)
      target_compile_options(${TINYUSDZ_LIB_TARGET} PRIVATE /EHs-c-)
    else()
      target_compile_options(${TINYUSDZ_LIB_TARGET} PRIVATE -fno-exceptions)
    endif()
  endif()

endforeach()




if(TINYUSDZ_WITH_C_API)
  if(TINYUSDZ_BUILD_SHARED_LIBS)
    add_library(${BUILD_TARGET_C} SHARED ${TINYUSDZ_C_API_SOURCES})
    # Link with static(object) version of libtinyusdz to make dll monolitic
    # TODO: Build TINYUSDZ_TARGET_STATIC as Object?
    target_link_libraries(${BUILD_TARGET_C} PRIVATE ${TINYUSDZ_TARGET_STATIC})
    target_compile_definitions(${BUILD_TARGET_C} PRIVATE "TINYUSDZ_COMPILE_LIBRARY")
    target_compile_definitions(${BUILD_TARGET_C} PRIVATE "TINYUSDZ_SHARED_LIBRARY")
    target_include_directories(
      ${BUILD_TARGET_C} PRIVATE ${PROJECT_SOURCE_DIR}/src)
  endif() 

  add_library(${BUILD_TARGET_C}_static STATIC ${TINYUSDZ_C_API_SOURCES})
  target_link_libraries(${BUILD_TARGET_C}_static PRIVATE ${TINYUSDZ_TARGET_STATIC})
  target_include_directories(
    ${BUILD_TARGET_C}_static PRIVATE ${PROJECT_SOURCE_DIR}/src)
endif()

if(TINYUSDZ_WITH_PYTHON)

  # TODO: Fully Remove pybind11-based python binding
  ## For the time beging, we stick with pybind11, since PyPI manylinux2014(probably mostly used architectrue as of 2022/Aug) does not support C++17.
  ## We may switch to nanobind at some point(probably around 2024?)

  ## build monolithic .dll
  ##nanobind_add_module(${BUILD_TARGET_PY} ${TINYUSDZ_PYTHON_BINDING_SOURCES})

  #pybind11_add_module(${BUILD_TARGET_PY} ${TINYUSDZ_PYTHON_BINDING_SOURCES})
  #add_sanitizers(${BUILD_TARGET_PY})
  #target_include_directories(
  #  ${BUILD_TARGET_PY} PRIVATE ${PROJECT_SOURCE_DIR}/src)

  #target_link_libraries(${BUILD_TARGET_PY} PRIVATE ${TINYUSDZ_TARGET_STATIC})

  ## Use 'c' prefix
  ## TODO: Use `cpp` prefix?
  #set_target_properties(${BUILD_TARGET_PY} PROPERTIES OUTPUT_NAME "ctinyusdz")

  ## copy python binding .so file to python/
  ## For developer
  ## NOTE: `POST_BUILD` command is not triggered when building python module using
  ## `python setup.py build`
  #add_custom_command(
  #  TARGET ${BUILD_TARGET_PY}
  #  POST_BUILD
  #  COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${BUILD_TARGET_PY}>"
  #          "${CMAKE_SOURCE_DIR}/python/tinyusdz/$<TARGET_FILE_NAME:${BUILD_TARGET_PY}>"
  #  COMMENT "copying python module file to python/tinyusdz"
  #  VERBATIM)
  ## For pypi packaging
  #install(TARGETS ${BUILD_TARGET_PY} LIBRARY DESTINATION tinyusdz)


  # copy C tinyusd .so file to python/tinyusdz/
  # For developer
  # NOTE: `POST_BUILD` command is not triggered when building python module using
  # `python setup.py build`
  add_custom_command(
    TARGET ${BUILD_TARGET_C}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${BUILD_TARGET_C}>"
            "${CMAKE_SOURCE_DIR}/python/tinyusdz/$<TARGET_FILE_NAME:${BUILD_TARGET_C}>"
    COMMENT "Copying libc-tinyusd.so/c-tinyusd.dll file to <tinyusdz>/python/tinyusdz"
    VERBATIM)

  # TODO: Run ctypesgen?

  #
  # target for setup.py
  # TODO: Use custom target and supply `cmake_install_target` in setup.py?
  # https://scikit-build.readthedocs.io/en/latest/usage.html#scikit-build-options
  #
  install(TARGETS ${BUILD_TARGET_C} LIBRARY DESTINATION tinyusdz)


endif()

# TODO: deprecate in next major version
if(TINYUSDZ_WITH_TOOL_USDA_PARSER AND TINYUSDZ_WITH_MODULE_USDA_READER)
  add_executable(
    usda_parser ${CMAKE_CURRENT_SOURCE_DIR}/examples/usda_parser/parser-main.cc)
  add_sanitizers(usda_parser)
  if(TINYUSDZ_PRODUCTION_BUILD)
    target_compile_definitions(usda_parser
                               PRIVATE "TINYUSDZ_PRODUCTION_BUILD")
  endif()
  target_include_directories(usda_parser PRIVATE ${PROJECT_SOURCE_DIR}/src/)
  target_link_libraries(usda_parser PRIVATE ${TINYUSDZ_TARGET_STATIC})
endif()

# TODO: deprecate in next major version
if(TINYUSDZ_WITH_TOOL_USDC_PARSER AND TINYUSDZ_WITH_MODULE_USDC_READER)
  add_executable(
    usdc_parser ${CMAKE_CURRENT_SOURCE_DIR}/examples/usdc_parser/usdc-parser-main.cc)
  add_sanitizers(usdc_parser)
  if(TINYUSDZ_PRODUCTION_BUILD)
    target_compile_definitions(usdc_parser
                               PRIVATE "TINYUSDZ_PRODUCTION_BUILD")
  endif()
  target_include_directories(usdc_parser PRIVATE ${PROJECT_SOURCE_DIR}/src/)
  target_link_libraries(usdc_parser PRIVATE ${TINYUSDZ_TARGET_STATIC})
endif()

#if(TINYUSDZ_WITH_BLENDER_ADDON)
#  pybind11_add_module(${BUILD_TARGET_BLENDER_PY}
#                      ${TINYUSDZ_BLENDER_PYTHON_BINDING_SOURCES})
#  add_sanitizers(${BUILD_TARGET_BLENDER_PY})
#
#  # For Python.h header files. Assume Blender 2.93 LTS(Python 3.9)
#  target_include_directories(
#    ${BUILD_TARGET_BLENDER_PY}
#    PRIVATE ${PROJECT_SOURCE_DIR}/src/blender/python3.9/Include)
#
#endif()

if(TINYUSDZ_BUILD_EXAMPLES)
  add_subdirectory(examples/tusdcat)
  add_subdirectory(examples/tydra_api)
  add_subdirectory(examples/tydra_to_renderscene)
  add_subdirectory(examples/api_tutorial)
  add_subdirectory(examples/file_format)
  add_subdirectory(examples/asset_resolution)
  if (TINYUSDZ_WITH_JSON)
    add_subdirectory(examples/usd_to_json)
  endif()
  if(TINYUSDZ_WITH_MODULE_USDA_WRITER)
    add_subdirectory(examples/save_usda)
  endif()
  if(TINYUSDZ_WITH_C_API)
    add_subdirectory(examples/c_api_example)
  endif()
endif(TINYUSDZ_BUILD_EXAMPLES)

if(TINYUSDZ_BUILD_BENCHMARKS)

  #
  # Standalone benchmark exe.
  #
  set(TINYUSDZ_BENCH_SOURCES ${PROJECT_SOURCE_DIR}/benchmarks/benchmark-main.cc)

  add_executable(${TINYUSDZ_BENCHMARK_TARGET} ${TINYUSDZ_BENCH_SOURCES})
  add_sanitizers(${TINYUSDZ_BENCHMARK_TARGET})

  target_include_directories(
    ${TINYUSDZ_BENCHMARK_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/src
                                         ${PROJECT_SOURCE_DIR}/bencharks)
  target_link_libraries(${TINYUSDZ_BENCHMARK_TARGET}
                        PRIVATE ${TINYUSDZ_TARGET_STATIC})

  if(TINYUSDZ_WITH_OPENSUBDIV)
    target_compile_definitions(${TINYUSDZ_BENCHMARK_TARGET}
                               PRIVATE "TINYUSDZ_USE_OPENSUBDIV")
  endif(TINYUSDZ_WITH_OPENSUBDIV)

endif(TINYUSDZ_BUILD_BENCHMARKS)

# [VisualStudio]
if(WIN32)
  # Set ${TINYUSDZ_TARGET_STATIC} as a startup project for VS IDE
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${TINYUSDZ_TARGET_STATIC})

  # For easier debugging in VS IDE(cmake 3.8.0 or later required) Set working
  # directory to ${TINYUSDZ_TARGET_STATIC} git repo root.
  if(CMAKE_VERSION VERSION_GREATER 3.8.0)
    set_target_properties(
      ${TINYUSDZ_TARGET_STATIC} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                                           "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
endif()

if(TINYUSDZ_BUILD_TESTS)

  enable_testing()

  #
  # Standalone tester
  #
  set(TINYUSDZ_TEST_SOURCES ${PROJECT_SOURCE_DIR}/tests/test-main.cc)

  add_executable(${TINYUSDZ_TEST_TARGET} ${TINYUSDZ_TEST_SOURCES})
  add_sanitizers(${TINYUSDZ_TEST_TARGET})

  target_include_directories(
    ${TINYUSDZ_TEST_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/src
                                    ${PROJECT_SOURCE_DIR}/tests)
  target_link_libraries(${TINYUSDZ_TEST_TARGET}
                        PRIVATE ${TINYUSDZ_TARGET_STATIC})

  if(TINYUSDZ_WITH_OPENSUBDIV)
    target_compile_definitions(${TINYUSDZ_TEST_TARGET}
                               PRIVATE "TINYUSDZ_USE_OPENSUBDIV")
  endif(TINYUSDZ_WITH_OPENSUBDIV)

  #
  # unit tester
  #
  add_subdirectory(${PROJECT_SOURCE_DIR}/tests/unit)

  #
  # Compression tester
  #
  add_subdirectory(${PROJECT_SOURCE_DIR}/tests/decompress-int)


  #
  # TODO: Add feature to specify Python interpreter at ctest run
  #

  #
  # USDA parser tester
  #
  if(TINYUSDZ_WITH_MODULE_USDA_READER)
    add_test(
      NAME usda-parser-unit-test
      COMMAND python3 ${PROJECT_SOURCE_DIR}/tests/usda/unit-runner.py --app
              "$<TARGET_FILE:test_tinyusdz>" --basedir "${PROJECT_SOURCE_DIR}/tests/usda"
      )
  endif()

  #
  # USDA parser tester
  #
  if(TINYUSDZ_WITH_MODULE_USDC_READER)
    add_test(
      NAME usdc-parser-unit-test
      COMMAND python3 ${PROJECT_SOURCE_DIR}/tests/usdc/unit-runner.py --app
              "$<TARGET_FILE:test_tinyusdz>" --basedir "${PROJECT_SOURCE_DIR}/tests/usdc"
      )
  endif()

endif(TINYUSDZ_BUILD_TESTS)

# [VisualStudio]
if(WIN32)
  # Set ${TINYUSDZ_TARGET_STATIC} as a startup project for VS IDE
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${TINYUSDZ_TARGET_STATIC})

  # For easier debugging in VS IDE(cmake 3.8.0 or later required) Set working
  # directory to ${TINYUSDZ_TARGET_STATIC} git repo root.
  if(CMAKE_VERSION VERSION_GREATER 3.8.0)
    set_target_properties(
      ${TINYUSDZ_TARGET_STATIC} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                                           "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
endif()
