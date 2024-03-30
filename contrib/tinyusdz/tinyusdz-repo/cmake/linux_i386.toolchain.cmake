set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR "i386")
set(CMAKE_C_COMPILER_TARGET i386-linux-gnu)

# Assume debian/ubuntu
#set(CMAKE_FIND_ROOT_PATH /usr/lib/i386-linux-gnu/)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# https://stackoverflow.com/questions/41557927/using-usr-lib-i386-linux-gnu-instead-of-usr-lib-x86-64-linux-gnu-to-find-libra
set(FIND_LIBRARY_USE_LIB64_PATHS OFF)
