cmake_minimum_required(VERSION 3.10)

include_directories(./)
include_directories(./../../)
add_library( # Defines the name of the library.
             android_jniiosystem

             # Implements a static library.
             STATIC

             # relative path to source file(s).
             AndroidJNIIOSystem.cpp
             BundledAssetIOSystem.cpp
)
TARGET_LINK_LIBRARIES(android_jniiosystem android log)
INSTALL(TARGETS android_jniiosystem EXPORT "${TARGETS_EXPORT_NAME}")
