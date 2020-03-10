cmake_minimum_required(VERSION 2.8)
project(Clipper)
enable_language(CXX)
set(SOURCE_FILES clipper.cpp)

include_directories(${CMAKE_SOURCE_DIR)
add_library(Clipper ${SOURCE_FILES})

set(Clipper_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(Clipper_LIBRARY "Clipper")
