# Assume this cmake is called from tinyusdz root(../../)
set(EXAMPLE_TARGET "c_api_example")

set(TINYUSDZ_API_TUTORIAL_SOURCES
    c-api-example-main.c
    )

add_executable(${EXAMPLE_TARGET} ${TINYUSDZ_API_TUTORIAL_SOURCES})
add_sanitizers(${EXAMPLE_TARGET})

target_include_directories(${EXAMPLE_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${EXAMPLE_TARGET} c-tinyusd_static)

set_target_properties(${EXAMPLE_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
