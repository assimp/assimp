cmake_minimum_required(VERSION 2.8)
project(rapidjson)
enable_language(CXX)

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE include/rapidjson)

set(rapidjson_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(rapidjson_LIBRARY "rapidjson")
