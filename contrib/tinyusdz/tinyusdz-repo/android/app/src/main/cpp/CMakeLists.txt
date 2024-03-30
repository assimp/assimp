project(tinyusz_android)

cmake_minimum_required(VERSION 3.4.1)

option(TINYUSDZ_USE_USDOBJ "Build with built-in .obj support" On)

set(TINYUSDZ_SOURCES
        ${PROJECT_SOURCE_DIR}/../../../../../src/tinyusdz.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/asset-resolution.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/composition.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/prim-types.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/ascii-parser.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/ascii-parser-basetype.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/ascii-parser-timesamples.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/ascii-parser-timesamples-array.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/usda-reader.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/usdc-reader.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/usdc-writer.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/crate-reader.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/crate-format.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/crate-pprint.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/io-util.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/pprinter.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/tiny-format.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/value-types.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/value-pprint.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/primvar.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/prim-reconstruct.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/prim-composition.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/image-loader.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/usda-writer.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/usdGeom.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/usdSkel.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/usdShade.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/usdMtlx.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/usdLux.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/xform.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/stage.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/str-util.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/path-util.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/image-util.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/image-writer.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/linear-algebra.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/tydra/facial.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/tydra/scene-access.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/tydra/render-data.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/tydra/prim-apply.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/tydra/shader-network.cc
        )
if (TINYUSDZ_USE_USDOBJ)
    list(APPEND TINYUSDZ_SOURCES
            ${PROJECT_SOURCE_DIR}/../../../../../src/usdObj.cc)
    list(APPEND TINYUSDZ_SOURCES
            ${PROJECT_SOURCE_DIR}/../../../../../src/external/tiny_obj_loader.cc)
endif ()

set(TINYUSDZ_DEP_SOURCES
        ${PROJECT_SOURCE_DIR}/../../../../../src/integerCoding.cpp
        ${PROJECT_SOURCE_DIR}/../../../../../src/lz4-compression.cc
        ${PROJECT_SOURCE_DIR}/../../../../../src/lz4/lz4.c
        #${PROJECT_SOURCE_DIR}/../../../../../src/external/string_id/database.cpp
        #${PROJECT_SOURCE_DIR}/../../../../../src/external/string_id/string_id.cpp
        #${PROJECT_SOURCE_DIR}/../../../../../src/external/string_id/error.cpp
        ${PROJECT_SOURCE_DIR}/../../../../../src/external/fpng.cpp
        #${PROJECT_SOURCE_DIR}/../../../../../src/external/staticstruct.cc
        #${PROJECT_SOURCE_DIR}/../../../../../src/external/tinyxml2/tinyxml2.cpp
        )

# Reuse files from sdlviewer
set(USDVIEW_SOURCES
        ${PROJECT_SOURCE_DIR}/../../../../../examples/sdlviewer/simple-render.cc
        ${PROJECT_SOURCE_DIR}/../../../../../examples/common/matrix.cc
        ${PROJECT_SOURCE_DIR}/../../../../../examples/common/trackball.cc)

# Build the libhello-oboe library
add_library(
        hello-tinyusdz SHARED jni-tinyusdz.cc render-ctx.cc ${TINYUSDZ_SOURCES}
        ${TINYUSDZ_DEP_SOURCES} ${USDVIEW_SOURCES})

target_link_libraries(hello-tinyusdz jnigraphics android log)

target_include_directories(
        hello-tinyusdz
        PRIVATE ${PROJECT_SOURCE_DIR}/../../../../../src/
        # nanort, nanosg, etc
        ${PROJECT_SOURCE_DIR}/../../../../../examples/common/
        # sdlviewer example
        ${PROJECT_SOURCE_DIR}/../../../../../examples/sdlviewer/)

if (TINYUSDZ_USE_USDOBJ)
    target_compile_definitions(hello-tinyusdz PRIVATE "TINYUSDZ_USE_USDOBJ")
endif ()

# Required to load .usd files from Android asset for demo purpose
# When you embed TinyUSDZ to your own app, you are better to load .usd files from a memory and turn this define off.
target_compile_definitions(hello-tinyusdz
        PRIVATE "TINYUSDZ_ANDROID_LOAD_FROM_ASSETS")

# Enable optimization flags: if having problems with source level debugging,
# disable -Ofast ( and debug ), re-enable after done debugging.
target_compile_options(hello-tinyusdz PRIVATE -Wall -Werror
        "$<$<CONFIG:RELEASE>:-Ofast>")
