#!/bin/bash

CMAKE_BIN=cmake

rm -rf build-aarch64-cross

$CMAKE_BIN \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_VERBOSE_MAKEFILE=On \
-DCMAKE_INSTALL_PREFIX=$HOME/local/tinyusdz-ios \
-DCMAKE_C_COMPILER=clang \
-DCMAKE_CXX_COMPILER=clang++ \
-DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk \
-DCMAKE_OSX_ARCHITECTURES=arm64e \
-DCMAKE_TOOLCHAIN_FILE=./cmake/ios.toolchain.cmake \
-Bbuild-aarch64-cross .
