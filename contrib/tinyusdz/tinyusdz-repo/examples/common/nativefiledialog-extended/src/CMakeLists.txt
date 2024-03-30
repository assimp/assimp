set(TARGET_NAME nfd)

set(PUBLIC_HEADER_FILES
  include/nfd.h
  include/nfd.hpp)

set(SOURCE_FILES ${PUBLIC_HEADER_FILES})

if(nfd_PLATFORM STREQUAL PLATFORM_WIN32)
  list(APPEND SOURCE_FILES nfd_win.cpp)
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_LINUX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  message("Using GTK version: ${GTK3_VERSION}")
  list(APPEND SOURCE_FILES nfd_gtk.cpp)
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_MACOS)
  find_library(APPKIT_LIBRARY AppKit)
  list(APPEND SOURCE_FILES nfd_cocoa.m)
endif()

# Define the library
add_library(${TARGET_NAME} STATIC
  ${SOURCE_FILES})

# Allow includes from include/
target_include_directories(${TARGET_NAME}
  PUBLIC include/)

if(nfd_PLATFORM STREQUAL PLATFORM_LINUX)
  target_include_directories(${TARGET_NAME}
    PRIVATE ${GTK3_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME}
    PRIVATE ${GTK3_LIBRARIES})
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_MACOS)
  target_link_libraries(${TARGET_NAME}
    PRIVATE ${APPKIT_LIBRARY})
endif()

if(nfd_COMPILER STREQUAL COMPILER_MSVC)
  string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY STATIC_LIBRARY_OPTIONS /NODEFAULTLIB)
endif()

if(nfd_COMPILER STREQUAL COMPILER_GNU)
  target_compile_options(${TARGET_NAME} PRIVATE -nostdlib -fno-exceptions -fno-rtti)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")

install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include)
