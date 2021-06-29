if(DRACO_CMAKE_TOOLCHAINS_ANDROID_NDK_COMMON_CMAKE_)
  return()
endif()
set(DRACO_CMAKE_TOOLCHAINS_ANDROID_NDK_COMMON_CMAKE_ 1)

# Toolchain files do not have access to cached variables:
# https://gitlab.kitware.com/cmake/cmake/issues/16170. Set an intermediate
# environment variable when loaded the first time.
if(DRACO_ANDROID_NDK_PATH)
  set(ENV{DRACO_ANDROID_NDK_PATH} "${DRACO_ANDROID_NDK_PATH}")
else()
  set(DRACO_ANDROID_NDK_PATH "$ENV{DRACO_ANDROID_NDK_PATH}")
endif()

set(CMAKE_SYSTEM_NAME Android)

if(NOT CMAKE_ANDROID_STL_TYPE)
  set(CMAKE_ANDROID_STL_TYPE c++_static)
endif()

if(NOT CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION)
  set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION clang)
endif()
