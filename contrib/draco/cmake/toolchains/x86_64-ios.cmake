if(DRACO_CMAKE_TOOLCHAINS_X86_64_IOS_CMAKE_)
  return()
endif()
set(DRACO_CMAKE_TOOLCHAINS_X86_64_IOS_CMAKE_ 1)

if(XCODE)
  # TODO(tomfinegan): Handle arm builds in Xcode.
  message(FATAL_ERROR "This toolchain does not support Xcode.")
endif()

set(CMAKE_SYSTEM_PROCESSOR "x86_64")
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_OSX_SDK "iphonesimulator")

include("${CMAKE_CURRENT_LIST_DIR}/arm-ios-common.cmake")
