cmake_minimum_required(VERSION 2.8)
project(Poly2Tri)
enable_language(CXX)
set(SOURCE_FILES poly2tri/common/shapes.cc 
                 poly2tri/sweep/advancing_front.cc 
                 poly2tri/sweep/cdt.cc 
                 poly2tri/sweep/sweep.cc 
                 poly2tri/sweep/sweep_context.cc)

include_directories(${CMAKE_SOURCE_DIR}/poly2tri)
add_library(Poly2Tri ${SOURCE_FILES})

set(Poly2Tri_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(Poly2Tri_Library "Poly2Tri")
