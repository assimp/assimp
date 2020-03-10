cmake_minimum_required(VERSION 2.8)
project(stb_image)
enable_language(CXX)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE "#{CMAKE_CURRENT_SOURCE_DIR}")

set(stb_image_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(stb_image_LIBRARY "stb_image")
