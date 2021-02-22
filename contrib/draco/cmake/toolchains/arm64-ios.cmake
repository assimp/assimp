if(DRACO_CMAKE_TOOLCHAINS_ARM64_IOS_CMAKE_)
  return()
endif()
set(DRACO_CMAKE_TOOLCHAINS_ARM64_IOS_CMAKE_ 1)

if(XCODE)
  # TODO(tomfinegan): Handle arm builds in Xcode.
  message(FATAL_ERROR "This toolchain does not support Xcode.")
endif()

set(CMAKE_SYSTEM_PROCESSOR "arm64")
set(CMAKE_OSX_ARCHITECTURES "arm64")

include("${CMAKE_CURRENT_LIST_DIR}/arm-ios-common.cmake")
