INCLUDE(CMakeForceCompiler)

SET (CMAKE_CROSSCOMPILING   TRUE)
SET (CMAKE_SYSTEM_NAME      "Darwin")
SET (CMAKE_SYSTEM_PROCESSOR "armv7")

SET (SDKVER     "5.0")
SET (DEVROOT    "/Developer/Platforms/iPhoneOS.platform/Developer")
SET (SDKROOT    "/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS${SDKVER}.sdk")
SET (CC         "${DEVROOT}/usr/bin/llvm-gcc")
SET (CXX        "${DEVROOT}/usr/bin/llvm-g++")

CMAKE_FORCE_C_COMPILER          (${CC} LLVM)
CMAKE_FORCE_CXX_COMPILER        (${CXX} LLVM)

SET (CMAKE_FIND_ROOT_PATH               "${SDKROOT}" "${DEVROOT}")
SET (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM  NEVER)
SET (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY  ONLY)
SET (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE  ONLY)