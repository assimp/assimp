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

if(DRACO_CMAKE_TOOLCHAINS_ARMV7_LINUX_GCC_CMAKE_)
  return()
endif()
set(DRACO_CMAKE_TOOLCHAINS_ARMV7_LINUX_GCC_CMAKE_ 1)

set(CMAKE_SYSTEM_NAME "Linux")

if("${CROSS}" STREQUAL "")
  # Default the cross compiler prefix to something known to work.
  set(CROSS arm-linux-gnueabihf-)
endif()

if(NOT ${CROSS} MATCHES hf-$)
  set(DRACO_EXTRA_TOOLCHAIN_FLAGS "-mfloat-abi=softfp")
endif()

set(CMAKE_C_COMPILER ${CROSS}gcc)
set(CMAKE_CXX_COMPILER ${CROSS}g++)
set(AS_EXECUTABLE ${CROSS}as)
set(CMAKE_C_COMPILER_ARG1
    "-march=armv7-a -mfpu=neon ${DRACO_EXTRA_TOOLCHAIN_FLAGS}")
set(CMAKE_CXX_COMPILER_ARG1
    "-march=armv7-a -mfpu=neon ${DRACO_EXTRA_TOOLCHAIN_FLAGS}")
set(CMAKE_SYSTEM_PROCESSOR "armv7")
