# 3.13 = target_link_directories
cmake_minimum_required(VERSION 3.13)

set(EXAMPLE_BUILD_TARGET "pxrusd")

project(${EXAMPLE_BUILD_TARGET} C CXX)

# options
set(EXAMPLE_USD_LIB_PREFIX "" CACHE STRING "Prefix to pxrUSD lib name.For example, some(?) pxrUSD build configuration adds 'usd_' prefix")
set(EXAMPLE_USD_INC_DIR "dist/include" CACHE PATH "Path to pxrUSD include dir")
set(EXAMPLE_USD_LIB_DIR "dist/lib" CACHE PATH "Path to pxrUSD lib dir")
set(EXAMPLE_USD_MONOLITHIC OFF CACHE BOOL "Is pxrUSD monolithic build?")
# USD is being built with Python(libboost_python) cannot be detected by pxrConfig.cmake or other settings file, so need to supply Python info manually. 
set(EXAMPLE_LINK_BOOST_PYTHON OFF CACHE BOOL "Link libboost_python*")

# cmake modules

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/sanitizers)
find_package(Sanitizers) # Address sanitizer (-DSANITIZE_ADDRESS=ON)

find_package(Threads REQUIRED)

# Require strict C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# TODO(LTE): Only enable PIC when compiling python module(dll)?.
set(CMAKE_POSITION_INDEPENDENT_CODE On)

if (${EXAMPLE_LINK_BOOST_PYTHON})

   if(EXAMPLE_PREFER_LOCAL_PYTHON_INSTALLATION)
       #message(STATUS "Local Python")
       set(Python3_FIND_FRAMEWORK NEVER) # Do not search framework python
       set(Python3_FIND_STRATEGY LOCATION)
       set(Python3_FIND_REGISTRY NEVER) # Windows only
   else()
       set(Python_FIND_FRAMEWORK LAST
       )# Prefer Brew/Conda to Apple framework python
  endif()

  find_package(
     Python3
     COMPONENTS Development.Embed
     REQUIRED)

  message(STATUS ${Python3_INCLUDE_DIRS})
  message(STATUS ${Python3_LIBRARIES})
endif()


list(APPEND CMAKE_MODULE_PATH ${EXAMPLE_USD_DIR})
message(STATUS "USD_INCLUDE_DIRS" ${EXAMPLE_USD_INC_DIR})
message(STATUS "USD_LIB_DIRS" ${EXAMPLE_USD_LIB_DIRS})
message(STATUS "USD_MONOLITHIC" ${EXAMPLE_USD_MONOLITHIC})


#
# === core source files ===
#
set(EXAMPLE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cc
  )

add_executable(${EXAMPLE_BUILD_TARGET} ${EXAMPLE_SOURCES})
add_sanitizers(${EXAMPLE_BUILD_TARGET})


# <tinyusdz>/src
target_include_directories(${EXAMPLE_BUILD_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

# [pxrusd]
target_include_directories(
  ${EXAMPLE_BUILD_TARGET}
  PUBLIC ${EXAMPLE_USD_INC_DIR})

target_link_directories(${EXAMPLE_BUILD_TARGET} PRIVATE ${EXAMPLE_USD_LIB_DIR})

if (EXAMPLE_USD_MONOLITHIC)
  target_link_libraries(${EXAMPLE_BUILD_TARGET} PRIVATE usd_ms)
else()
  target_link_libraries(${EXAMPLE_BUILD_TARGET} PRIVATE
	${EXAMPLE_USD_LIB_PREFIX}usd
	${EXAMPLE_USD_LIB_PREFIX}usdGeom
	${EXAMPLE_USD_LIB_PREFIX}usdUtils
	${EXAMPLE_USD_LIB_PREFIX}sdf
	${EXAMPLE_USD_LIB_PREFIX}tf
	${EXAMPLE_USD_LIB_PREFIX}gf
	${EXAMPLE_USD_LIB_PREFIX}vt)
endif()

if (${EXAMPLE_LINK_BOOST_PYTHON})
  # [python]
  target_link_libraries(${EXAMPLE_BUILD_TARGET} PUBLIC boost_python3${Python3_VERSION_MINOR} Python3::Python)
endif()

target_link_libraries(${EXAMPLE_BUILD_TARGET} PUBLIC ${EXAMPLE_EXT_LIBRARIES} ${CMAKE_DL_LIBS} Threads::Threads)

# Increase warning level for clang.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

  include(CheckCXXCompilerFlag)

  # -Wno-delete-non-abstract-non-virtual-dtor
  # -Wno-delete-non-virtual-dtor
  #  workaround for pbrtParser https://github.com/ingowald/pbrt-parser/issues/39
  #check_cxx_compiler_flag("-Wno-delete-non-abstract-non-virtual-dtor" HAS_NO_DELETE_NON_ABSTRACT_NON_VIRTUAL_DTOR)
  #if (HAS_NO_DELETE_NON_ABSTRACT_NON_VIRTUAL_DTOR)
  #  string(APPEND OPTIONAL_CXX_FLAGS " -Wno-delete-non-abstract-non-virtual-dtor ")
  #endif ()

  #check_cxx_compiler_flag("-Wno-delete-non-virtual-dtor" HAS_NO_DELETE_NON_VIRTUAL_DTOR)
  #if (HAS_NO_DELETE_NON_VIRTUAL_DTOR)
  #  string(APPEND OPTIONAL_CXX_FLAGS " -Wno-delete-non-virtual-dtor ")
  #endif ()

  #set(CLANG_COMPILE_FLAGS "-Weverything -Werror -Wno-padded -Wno-c++98-compat-pedantic -Wno-documentation -Wno-documentation-unknown-command -Wno-reserved-id-macro -Wno-unused-variable  -Wno-exit-time-destructors -Wno-global-constructors ")

  # TODO(LTE): Remove 'Wno-unused-functions'
  set(CLANG_COMPILE_FLAGS "-Weverything -Werror -Wno-padded -Wno-c++98-compat-pedantic -Wno-documentation -Wno-documentation-unknown-command -Wno-reserved-id-macro -Wno-unused-function")

  string(APPEND CLANG_COMPILE_FLAGS ${OPTIONAL_CXX_FLAGS})

  if(MINGW)
    # TODO(LTE): increase warning level for llvm-mingw.
  else()
    set_source_files_properties(
      ${EXAMPLE_SOURCES} ${EXAMPLE_GUI_SOURCES}
      PROPERTIES
      COMPILE_FLAGS
      ${CLANG_COMPILE_FLAGS}
      )
  endif()
endif()

# postbuild
if (WIN32)
  if (LLVM_MINGW_ROOT_ABSPATH)

    # Copy libunwind.dll and libc++.dll
    add_custom_command(TARGET ${EXAMPLE_BUILD_TARGET} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
            "${LLVM_MINGW_ROOT_ABSPATH}/x86_64-w64-mingw32/bin/libc++.dll"      # src
            $<TARGET_FILE_DIR:${EXAMPLE_BUILD_TARGET}>)           # dest

    add_custom_command(TARGET ${EXAMPLE_BUILD_TARGET} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
            "${LLVM_MINGW_ROOT_ABSPATH}/x86_64-w64-mingw32/bin/libunwind.dll"      # src
            $<TARGET_FILE_DIR:${EXAMPLE_BUILD_TARGET}>)           # dest

    # TODO(LTE): Copy asan dll only when the app is build with asan support
    add_custom_command(TARGET ${EXAMPLE_BUILD_TARGET} POST_BUILD        # Adds a post-build event to MyTest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
            "${LLVM_MINGW_ROOT_ABSPATH}/x86_64-w64-mingw32/bin/libclang_rt.asan_dynamic-x86_64.dll"      # src
            $<TARGET_FILE_DIR:${EXAMPLE_BUILD_TARGET}>)           # dest

  endif (LLVM_MINGW_ROOT_ABSPATH)

endif (WIN32)

# [VisualStudio]
if(WIN32)
  # Set ${EXAMPLE_BUILD_TARGET} as a startup project for VS IDE
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${EXAMPLE_BUILD_TARGET})

  # For easier debugging in VS IDE(cmake 3.8.0 or later required) Set working
  # directory to ${EXAMPLE_BUILD_TARGET} git repo root.
  if(CMAKE_VERSION VERSION_GREATER 3.8.0)
    set_target_properties(${EXAMPLE_BUILD_TARGET}
                          PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                                     "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
endif()
