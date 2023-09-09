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
