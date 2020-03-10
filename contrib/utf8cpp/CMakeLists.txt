cmake_minimum_required(VERSION 2.8)
project(utf8cpp)
enable_language(CXX)

add_library(utf8cpp INTERFACE)
target_include_directories(utf8cpp INTERFACE source)

set(utf8cpp_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(utf8cpp_LIBRARY "utf8cpp")
