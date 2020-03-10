cmake_minimum_required(VERSION 2.8)
project(Open3DGC)
enable_language(CXX)
set(SOURCE_FILES 
o3dgcArithmeticCodec.cpp
o3dgcDynamicVectorDecoder.cpp
o3dgcDynamicVectorEncoder.cpp
o3dgcTools.cpp
o3dgcTriangleFans.cpp
)
include_directories(${CMAKE_SOURCE_DIR)
add_library(Open3DGC ${SOURCE_FILES})
