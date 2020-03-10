cmake_minimum_required(VERSION 2.8)
project(Poly2Tri)
enable_language(CXX)
set(SOURCE_FILES poly2tri/common/shapes.cc 
                 poly2tri/sweep/advancing_front.cc 
                 poly2tri/sweep/cdt.cc 
                 poly2tri/sweep/sweep.cc 
                 poly2tri/sweep/sweep_context.cc)

include_directories(${CMAKE_SOURCE_DIR}/poly2tri)
add_library(Poly2Tri SHARED ${SOURCE_FILES})
add_library(Poly2Tristatic STATIC ${SOURCE_FILES})
