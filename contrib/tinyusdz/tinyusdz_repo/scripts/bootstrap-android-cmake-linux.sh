#!/bin/bash

# Edit path to Android NDK

# Use ANDROID_SDK_HOME environment
#ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk-bundle

# It looks ANDROID_NDK_LATEST_HOME is set in Github CI.
ANDROID_NDK_ROOT=$ANDROID_NDK_LATEST_HOME

# default installation directory of NDK through old? Android Studio
# ANDROID_NDK_ROOT=$HOME/Android/Sdk/ndk-bundle

# Set your own path
#ANDROID_NDK_ROOT=$HOME/local/android-ndk-r16b/

# CMake 3.6 or later required.
# TODO: Use cmake in Android Sdk.
CMAKE_BIN=cmake

rm -rf build-android
mkdir build-android
cd build-android

# Sometimes CI failed to find `ninja`, so disable Ninja generator for a while.

$CMAKE_BIN -G Ninja -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_NATIVE_API_LEVEL=24 \
  -DANDROID_ARM_MODE=arm \
  -DANDROID_ARM_NEON=TRUE \
  -DANDROID_STL=c++_shared \
  -DTINYUSDZ_BUILD_TESTS=On \
  -DTINYUSDZ_BUILD_EXAMPLES=On \
  ..

cd ..
