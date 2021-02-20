if(DRACO_CMAKE_TOOLCHAINS_ARM64_ANDROID_NDK_LIBCPP_CMAKE_)
  return()
endif()
set(DRACO_CMAKE_TOOLCHAINS_ARM64_ANDROID_NDK_LIBCPP_CMAKE_ 1)

include("${CMAKE_CURRENT_LIST_DIR}/android-ndk-common.cmake")

if(NOT ANDROID_PLATFORM)
  set(ANROID_PLATFORM android-21)
endif()

if(NOT ANDROID_ABI)
  set(ANDROID_ABI arm64-v8a)
endif()

include("${DRACO_ANDROID_NDK_PATH}/build/cmake/android.toolchain.cmake")
