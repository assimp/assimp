# Assume this project is a root project
cmake_minimum_required(VERSION 3.16)

set(BUILD_TARGET "usdz_glview")

project(${BUILD_TARGET} CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OpenGL_GL_PREFERENCE GLVND) # CMP0072
find_package(OpenGL REQUIRED)

if(WIN32)
    # nothing.
elseif(APPLE)
    find_library(COCOA Cocoa REQUIRED)
else()
    find_package(X11 REQUIRED)
endif()

set(SOURCES
    main.cc
    shader.hh
    shader.cc
    #render.cc
    #render-config.cc

)

get_filename_component(TINYUSDZ_ABS_PATH "${PROJECT_SOURCE_DIR}/../../" ABSOLUTE)
get_filename_component(EXAMPLE_COMMON_ABS_PATH "${PROJECT_SOURCE_DIR}/../common/" ABSOLUTE)


# TinyUSDZ root
add_subdirectory(${TINYUSDZ_ABS_PATH} tinyusdz)


set(GUI_SOURCES
    ${PROJECT_SOURCE_DIR}/../common/trackball.cc
    ${PROJECT_SOURCE_DIR}/../common/matrix.cc
    ${PROJECT_SOURCE_DIR}/../common/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/../common/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/../common/imgui/imgui_tables.cpp
    ${PROJECT_SOURCE_DIR}/../common/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/../common/imgui/imgui_impl_glfw.cpp
    ${PROJECT_SOURCE_DIR}/../common/imgui/imgui_impl_opengl3.cpp
    ${PROJECT_SOURCE_DIR}/../common/glad/include/glad/glad.h
    ${PROJECT_SOURCE_DIR}/../common/glad/src/glad.c
    )

# local [glfw3]
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
#option(GLFW_VULKAN_STATIC "Assume the Vulkan loader is linked with the application" OFF)
add_subdirectory("${EXAMPLE_COMMON_ABS_PATH}/glfw-3.3.6" glfw-3.3.6 EXCLUDE_FROM_ALL)

add_executable(${BUILD_TARGET} ${SOURCES} ${GUI_SOURCES})

# Assume add_sanitizer() is imported in TinyUSDZ's CMakeList.txt
add_sanitizers(${BUILD_TARGET})

# local glad
target_include_directories(${BUILD_TARGET} PUBLIC "${EXAMPLE_COMMON_ABS_PATH}/glad/include")

target_include_directories(${BUILD_TARGET} PUBLIC ${OPENGL_INCLUDE_DIR})

if (UNIX)
  target_include_directories(${BUILD_TARGET} PUBLIC ${X11_INCLUDE_DIR})
endif()

target_include_directories(${BUILD_TARGET} PUBLIC "${EXAMPLE_COMMON_ABS_PATH}")
target_include_directories(${BUILD_TARGET} PUBLIC "${EXAMPLE_COMMON_ABS_PATH}/imgui")

# tinyusdz dir
target_include_directories(${BUILD_TARGET} PUBLIC "${TINYUSDZ_ABS_PATH}/src/")

target_link_libraries(
    ${BUILD_TARGET}
    ${OPENGL_LIBRARIES}
    glfw
    tinyusdz::tinyusdz_static
    ${CMAKE_DL_LIBS}
)

if(WIN32)
    # nothing.
elseif(APPLE)
    target_link_libraries(
        ${BUILD_TARGET}
        ${COCOA}
    )
else()
    target_link_libraries(
        ${BUILD_TARGET}
        ${X11_LIBRARIES}
    )
endif()

source_group("Source Files" FILES ${SOURCES})
