cmake_minimum_required(VERSION 3.19)

project(tinyusdz_wasi C CXX)
set(PROJECT_NAME tinyusdz_wasi)

set(MAIN_SOURCE main.cc)

option(WASI_SDK_PATH "wasi-sdk path")

set(CMAKE_SYSROOT "${WASI_SDK_PATH}/share/wasi-sysroot")

# C++ exceptions is not availble.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

# wasm specific settings
# https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wasm_app.md
# - Stack size: increased to 64KB.
#   Default is 16KB, which is not enough for some USDZ files with deep hierarchies.
#   TODO: Rewrite recursive function call with for-loop to save stack size.
#   (e.g. ReconstructPrimRecursively))
# - Initial memory: 128KB(Must be larger than stack size
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -z stack-size=65536 -Wl,--initial-memory=131072") 

list(
  APPEND
  TINYUSDZ_SOURCES
  ../../src/tinyusdz.cc
  ../../src/io-util.cc
  ../../src/prim-types.cc
  ../../src/pprinter.cc
  ../../src/path-util.cc
  ../../src/str-util.cc
  ../../src/value-pprint.cc
  ../../src/value-types.cc
  ../../src/primvar.cc
  ../../src/prim-types.cc
  ../../src/crate-format.cc
  ../../src/crate-reader.cc
  ../../src/crate-writer.cc
  ../../src/crate-pprint.cc
  ../../src/ascii-parser.cc
  ../../src/ascii-parser-basetype.cc
  ../../src/ascii-parser-timesamples.cc
  ../../src/ascii-parser-timesamples-array.cc
  ../../src/usda-reader.cc
  ../../src/usda-writer.cc
  ../../src/usdc-reader.cc
  ../../src/usdc-writer.cc
  ../../src/image-loader.cc
  ../../src/prim-reconstruct.cc
  ../../src/prim-composition.cc
  ../../src/tiny-format.cc
  ../../src/xform.cc
  ../../src/usdGeom.cc
  ../../src/usdLux.cc
  ../../src/usdShade.cc
  ../../src/usdSkel.cc
  ../../src/usdMtlx.cc
  ../../src/usdObj.cc
  ../../src/tydra/render-data.cc
  ../../src/tydra/scene-access.cc
  ../../src/tydra/shader-network.cc
  ../../src/stage.cc
)

set(TINYUSDZ_DEP_SOURCES
  ../../src/integerCoding.cpp
  ../../src/lz4-compression.cc
  ../../src/lz4/lz4.c
  ../../src/external/fpng.cpp
  #../../src/external/string_id/string_id.cpp
  #../../src/external/string_id/error.cpp
  #../../src/external/string_id/database.cpp
  #../../src/external/staticstruct.cc
  #../../src/external/tinyxml2/tinyxml2.cpp
  )
  #../../src/external/ryu/ryu/s2d.c
  #../../src/external/ryu/ryu/s2f.c)



add_executable(${PROJECT_NAME} ${MAIN_SOURCE} ${TINYUSDZ_SOURCES} ${TINYUSDZ_DEP_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ../../src/external/ryu)
target_include_directories(${PROJECT_NAME} PRIVATE ../../src/external/fast_float/include)
target_include_directories(${PROJECT_NAME} PRIVATE ../../src/)

#set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXCEPTIONS OFF)
