# Copyright 2021 The Draco Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

if(DRACO_CMAKE_TOOLCHAINS_ARMV7_ANDROID_NDK_LIBCPP_CMAKE_)
  return()
endif()
set(DRACO_CMAKE_TOOLCHAINS_ARMV7_ANDROID_NDK_LIBCPP_CMAKE_ 1)

include("${CMAKE_CURRENT_LIST_DIR}/android-ndk-common.cmake")

if(NOT ANDROID_PLATFORM)
  set(ANDROID_PLATFORM android-18)
endif()

if(NOT ANDROID_ABI)
  set(ANDROID_ABI armeabi-v7a)
endif()

include("${DRACO_ANDROID_NDK_PATH}/build/cmake/android.toolchain.cmake")
