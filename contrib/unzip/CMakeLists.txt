cmake_minimum_required(VERSION 2.8)
project(unzip)
set(SOURCE_FILES 
  ioapi.c
  unzip.c
)

include_directories(${CMAKE_SOURCE_DIR})
add_library(unzip ${SOURCE_FILES})

set(unzip_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(unzip_LIBRARY "unzip")
