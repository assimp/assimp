find_package(Threads REQUIRED)
include(ExternalProject)

if(MSYS OR MINGW)
  set(DISABLE_PTHREADS ON)
else()
  set(DISABLE_PTHREADS OFF)
endif()

if (MSVC)
  set(RELEASE_LIB_DIR ReleaseLibs)
  set(DEBUG_LIB_DIR DebugLibs)
elseif(XCODE_VERSION)
  set(RELEASE_LIB_DIR Release)
  set(DEBUG_LIB_DIR Debug)
else()
  set(RELEASE_LIB_DIR "")
  set(DEBUG_LIB_DIR "")
endif()

set(GTEST_CMAKE_ARGS
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-Dgtest_force_shared_crt=ON"
  "-Dgtest_disable_pthreads:BOOL=${DISABLE_PTHREADS}"
  "-DBUILD_GTEST=ON")
set(GTEST_RELEASE_LIB_DIR "")
set(GTEST_DEBUGLIB_DIR "")
if (MSVC)
  set(GTEST_CMAKE_ARGS ${GTEST_CMAKE_ARGS}
    "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=${DEBUG_LIB_DIR}"
    "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=${RELEASE_LIB_DIR}")
  set(GTEST_LIB_DIR)
endif()

set(GTEST_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/gtest")

# try to find git - if found, setup gtest
find_package(Git)
if(NOT GIT_FOUND)
  set(AddGTest_FOUND false CACHE BOOL "Was gtest setup correctly?")
else(NOT GIT_FOUND)
  set(AddGTest_FOUND true CACHE BOOL "Was gtest setup correctly?")

  ExternalProject_Add(gtest
    GIT_REPOSITORY https://github.com/google/googletest.git
    TIMEOUT 10
    PREFIX "${GTEST_PREFIX}"
    CMAKE_ARGS "${GTEST_CMAKE_ARGS}"
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
    # Disable install
    INSTALL_COMMAND ""
  )

  set(LIB_PREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}")
  set(LIB_SUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(GTEST_LOCATION "${GTEST_PREFIX}/src/gtest-build")
  set(GTEST_DEBUG_LIBRARIES
    "${LIB_PREFIX}gtest${LIB_SUFFIX}"
    "${CMAKE_THREAD_LIBS_INIT}")
  SET(GTEST_RELEASE_LIBRARIES
    "${LIB_PREFIX}gtest${LIB_SUFFIX}"
    "${CMAKE_THREAD_LIBS_INIT}")

  if(MSVC_VERSION EQUAL 1700)
    add_definitions(-D_VARIADIC_MAX=10)
  endif()

  ExternalProject_Get_Property(gtest source_dir)
  include_directories(${source_dir}/googletest/include)
  include_directories(${source_dir}/gtest/include)

  ExternalProject_Get_Property(gtest binary_dir)
  link_directories(${binary_dir}/googlemock/gtest)
  link_directories(${binary_dir}/googlemock/gtest/${RELEASE_LIB_DIR})
  link_directories(${binary_dir}/googlemock/gtest/${DEBUG_LIB_DIR})
endif(NOT GIT_FOUND)
