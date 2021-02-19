if(DRACO_CMAKE_TOOLCHAINS_ANDROID_CMAKE_)
  return()
endif() # DRACO_CMAKE_TOOLCHAINS_ANDROID_CMAKE_

# Additional ANDROID_* settings are available, see:
# https://developer.android.com/ndk/guides/cmake#variables

if(NOT ANDROID_PLATFORM)
  set(ANDROID_PLATFORM android-21)
endif()

# Choose target architecture with:
#
# -DANDROID_ABI={armeabi-v7a,armeabi-v7a with NEON,arm64-v8a,x86,x86_64}
if(NOT ANDROID_ABI)
  set(ANDROID_ABI arm64-v8a)
endif()

# Force arm mode for 32-bit targets (instead of the default thumb) to improve
# performance.
if(NOT ANDROID_ARM_MODE)
  set(ANDROID_ARM_MODE arm)
endif()

# Toolchain files do not have access to cached variables:
# https://gitlab.kitware.com/cmake/cmake/issues/16170. Set an intermediate
# environment variable when loaded the first time.
if(DRACO_ANDROID_NDK_PATH)
  set(ENV{DRACO_ANDROID_NDK_PATH} "${DRACO_ANDROID_NDK_PATH}")
else()
  set(DRACO_ANDROID_NDK_PATH "$ENV{DRACO_ANDROID_NDK_PATH}")
endif()

if(NOT DRACO_ANDROID_NDK_PATH)
  message(FATAL_ERROR "DRACO_ANDROID_NDK_PATH not set.")
  return()
endif()

include("${DRACO_ANDROID_NDK_PATH}/build/cmake/android.toolchain.cmake")
