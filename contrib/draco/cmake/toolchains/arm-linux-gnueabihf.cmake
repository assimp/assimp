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

if(DRACO_CMAKE_TOOLCHAINS_ARM_LINUX_GNUEABIHF_CMAKE_)
  return()
endif() # DRACO_CMAKE_TOOLCHAINS_ARM_LINUX_GNUEABIHF_CMAKE_
set(DRACO_CMAKE_TOOLCHAINS_ARM_LINUX_GNUEABIHF_CMAKE_ 1)

set(CMAKE_SYSTEM_NAME "Linux")

if("${CROSS}" STREQUAL "")
  set(CROSS arm-linux-gnueabihf-)
endif()

set(CMAKE_CXX_COMPILER ${CROSS}g++)
set(CMAKE_CXX_FLAGS_INIT "-march=armv7-a -marm")
set(CMAKE_SYSTEM_PROCESSOR "armv7")
set(DRACO_NEON_INTRINSICS_FLAG "-mfpu=neon")
