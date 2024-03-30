#
#   Copyright 2013 Pixar
#
#   Licensed under the Apache License, Version 2.0 (the "Apache License")
#   with the following modification; you may not use this file except in
#   compliance with the Apache License and the following modification to it:
#   Section 6. Trademarks. is deleted and replaced with:
#
#   6. Trademarks. This License does not grant permission to use the trade
#      names, trademarks, service marks, or product names of the Licensor
#      and its affiliates, except as required to comply with Section 4(c) of
#      the License and to reproduce the content of the NOTICE file.
#
#   You may obtain a copy of the Apache License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the Apache License with the above modification is
#   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#   KIND, either express or implied. See the Apache License for the specific
#   language governing permissions and limitations under the Apache License.
#

cmake_minimum_required(VERSION 3.12)

project(OpenSubdiv)

# Turn on folder support
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#-------------------------------------------------------------------------------
# Obtain OpenSubdiv API version from version.h file
if(EXISTS "${OpenSubdiv_SOURCE_DIR}/opensubdiv/version.h")
    file(STRINGS "${OpenSubdiv_SOURCE_DIR}/opensubdiv/version.h"
        OpenSubdiv_VERSION REGEX "^#define OPENSUBDIV_VERSION .*$")
    string(REPLACE "#define OPENSUBDIV_VERSION " "" OpenSubdiv_VERSION ${OpenSubdiv_VERSION})
else()
    message(FATAL_ERROR, "Cannot locate opensubdiv/version.h in ${OpenSubdiv_SOURCE_DIR}")
endif()

# Evaluate 'soname' from OSD version

    # replace '_' with '.'
    string(REGEX REPLACE "(_)" "." OSD_SONAME ${OpenSubdiv_VERSION})

    # remove starting 'v' character
    string(REGEX REPLACE "^v" "" OSD_SONAME ${OSD_SONAME})

    add_definitions(
        -DOPENSUBDIV_VERSION_STRING="${OSD_SONAME}"
    )

#-------------------------------------------------------------------------------

message(STATUS "Compiling ${PROJECT_NAME} version ${OpenSubdiv_VERSION}")
message(STATUS "Using cmake version ${CMAKE_VERSION}")

#-------------------------------------------------------------------------------
# Determine if the project is built as a subproject (using add_subdirectory)
# or if it is the main project.
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

# Specify the default install path
if (NOT DEFINED CMAKE_INSTALL_PREFIX)
    SET( CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/" )
endif()

if (NOT DEFINED CMAKE_INCDIR_BASE)
    set( CMAKE_INCDIR_BASE include/opensubdiv )
endif()

if (NOT DEFINED CMAKE_BINDIR_BASE)
    set( CMAKE_BINDIR_BASE bin )
endif()

if (NOT DEFINED CMAKE_LIBDIR_BASE)
    set( CMAKE_LIBDIR_BASE lib )
endif()

if (NOT DEFINED CMAKE_FRAMEWORKDIR_BASE)
    set( CMAKE_FRAMEWORKDIR_BASE Frameworks )
endif()

if (NOT DEFINED CMAKE_PLUGINDIR_BASE)
    set( CMAKE_PLUGINDIR_BASE plugin )
endif()

if (NOT DEFINED CMAKE_DOCDIR_BASE)
    set( CMAKE_DOCDIR_BASE share/doc/opensubdiv )
else()
    if (IS_ABSOLUTE ${CMAKE_DOCDIR_BASE})
        set( CMAKE_DOCDIR_BASE "${CMAKE_DOCDIR_BASE}" )
    else()
        set( CMAKE_DOCDIR_BASE "${CMAKE_INSTALL_PREFIX}/${CMAKE_DOCDIR_BASE}" )
    endif()
endif()

# Allow install path to be overridden for cross-compile builds
if(LIBRARY_OUTPUT_PATH_ROOT)
    SET( CMAKE_INSTALL_PREFIX "${LIBRARY_OUTPUT_PATH_ROOT}/" )
endif()

if (MAIN_PROJECT)
    # Set the directory where the executables will be stored.
    set(EXECUTABLE_OUTPUT_PATH
        "${PROJECT_BINARY_DIR}/bin"
        CACHE PATH
        "Directory where executables will be stored"
    )

    # Set the directory where the libraries will be stored.
    set(LIBRARY_OUTPUT_PATH
        "${PROJECT_BINARY_DIR}/lib"
        CACHE PATH
        "Directory where all libraries will be stored"
    )
endif()

# Specify the list of directories to search for cmake modules.
set(CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

#-------------------------------------------------------------------------------
# OpenSubdiv trips bugs in some older gcc versions
if (CMAKE_COMPILER_IS_GNUCC)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(WARNING "g++ 4.8 or newer recommended")
    endif()
endif()

# Detect Clang (until a cmake version provides built-in variables)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_COMPILER_IS_CLANGCC 1)
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")
    set(CMAKE_COMPILER_IS_ICC 1)
endif()


if (NOT CMAKE_COMPILER_IS_ICC)
    # Currently icc has a bug that asserts when linking rpaths containing long
    # sequences of ':' that this command causes. The consequence is that examples
    # built and installed using icc will not have an rpath pointing to the built
    # OSD library which they depend on and will have to set LD_LIBRARY_PATH instead.
    list(APPEND CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# ensure that ARC is shown as enabled in the Xcode UI
if(CMAKE_GENERATOR STREQUAL "Xcode")
    set (CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
endif()


set(OSD_COMPILER_FLAGS)

# Disable spurious warnings in gcc builds and clang
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANGCC OR CMAKE_COMPILER_IS_ICC )

    # Turn on all warnings
    if(CMAKE_COMPILER_IS_ICC)
        list(APPEND OSD_COMPILER_FLAGS -w2 -wd1572 -wd1418 -wd981 -wd383 -wd193 -wd444)
    else()
        list(APPEND OSD_COMPILER_FLAGS -Wall -Wextra)
    endif()

    # HBR uses the offsetof macro on a templated struct, which appears
    # to spuriously set off this warning in both gcc and Clang
    list(APPEND OSD_COMPILER_FLAGS -Wno-invalid-offsetof)

    # HBR uses unions as an optimization for its memory allocation.
    # Type casting between union members breaks strict aliasing rules from
    # gcc 4.4.1 versions onwards. We disable the warning but keep aliasing
    # optimization.
    list(APPEND OSD_COMPILER_FLAGS -Wno-strict-aliasing)

    # FAR and OSD have templated virtual function implementations that trigger
    # a lot of hidden virtual function overloads (some of them spurious).
    # Disable those for now in Clang.
    if(CMAKE_COMPILER_IS_CLANGCC)
        list(APPEND OSD_COMPILER_FLAGS -Wno-overloaded-virtual)
    endif()

    # Intel's icc compiler requires some libraries linked
    if(CMAKE_COMPILER_IS_ICC)

        foreach (ICC_LIB iomp5 irng intlc)

            if(CMAKE_SIZEOF_VOID_P MATCHES "8")
                list(APPEND ICC_LIB_ARCH "intel64")
            elseif(CMAKE_SIZEOF_VOID_P MATCHES "4")
                list(APPEND ICC_LIB_ARCH "ia32")
            endif()

            find_library( ICC_${ICC_LIB}
                NAMES
                    ${ICC_LIB}
                HINTS
                    ${ICC_LOCATION}
                PATHS
                    /opt/intel/lib/
                PATH_SUFFIXES
                    ${ICC_LIB_ARCH}
                    lib/${ICC_LIB_ARCH}
            )

            if (ICC_${ICC_LIB})
                list(APPEND ICC_LIBRARIES ${ICC_${ICC_LIB}})
            else()
                message( FATAL_ERROR "${ICC_${ICC_LIB}} library not found - required by icc" )
            endif()

        endforeach()
    endif()

elseif(MSVC)

    # Turn on all warnings
    list(APPEND OSD_COMPILER_FLAGS /Wall)

    list(APPEND OSD_COMPILER_FLAGS
                    /W3     # Use warning level recommended for production purposes.
                    /WX     # Treat all compiler warnings as errors.

                    # warning C4005: macro redefinition
                    /wd4005

                    # these warnings are being triggered from inside VC's header files
                    # warning C4350: behavior change: 'member1' called instead of 'member2'
                    /wd4350
                    # warning C4548: expression before comma has no effect; expected expression with side-effect
                    /wd4548

                    # Make sure WinDef.h does not define min and max macros which
                    # will conflict with std::min() and std::max().
                    /DNOMINMAX

                    # Make sure the constants in <math.h> get defined.
                    /D_USE_MATH_DEFINES

                    # Do not enforce MSVC's safe CRT replacements.
                    /D_CRT_SECURE_NO_WARNINGS

                    # Disable checked iterators and iterator debugging.  Visual Studio
                    # 2008 does not implement std::vector::data(), so we need to take the
                    # address of std::vector::operator[](0) to get the memory location of
                    # a vector's underlying data storage.  This does not work for an empty
                    # vector if checked iterators or iterator debugging is enabled.

                    # XXXX manuelk : we can't force SECURE_SCL to 0 or client code has
                    # problems linking against OSD if their build is not also
                    # overriding SSCL to the same value.
                    # See : http://msdn.microsoft.com/en-us/library/vstudio/hh697468.aspx
                    #/D_SECURE_SCL=0
                    #/D_HAS_ITERATOR_DEBUGGING=0
    )

    option(MSVC_STATIC_CRT "Statically link MSVC CRT" OFF)

    if(MSVC_STATIC_CRT)
        message(STATUS "Using static MSVC CRT")
        # http://stackoverflow.com/a/32128977/486990
        add_compile_options(
            "$<$<CONFIG:Debug>:/MTd>"
            "$<$<CONFIG:RelWithDebInfo>:/MT>"
            "$<$<CONFIG:Release>:/MT>"
            "$<$<CONFIG:MinSizeRel>:/MT>"
        )
    else()
        # Turn off a duplicate LIBCMT linker warning
        set(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt.lib")
        set(CMAKE_SHARED_LINKER_FLAGS
            "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:libcmt.lib")
    endif()

endif()

if(${SIMD} MATCHES "AVX")
    list(APPEND OSD_COMPILER_FLAGS -xAVX)
endif()

add_definitions(${OSD_COMPILER_FLAGS})

#-------------------------------------------------------------------------------

# Ignore rules that will re-run cmake (this will avoid constant
# reloading of the generated Visual Studio project).
set(CMAKE_SUPPRESS_REGENERATION TRUE)

option(PTEX_LOCATION "Path to Ptex" "")
option(GLEW_LOCATION "Path to GLEW" "")
option(GLFW_LOCATION "Path to GLFW" "")

option(NO_LIB "Disable the opensubdiv libs build (caveat emptor)" OFF)
option(NO_EXAMPLES "Disable examples build" OFF)
option(NO_TUTORIALS "Disable tutorials build" OFF)
option(NO_REGRESSION "Disable regression tests build" OFF)
option(NO_PTEX "Disable PTex support" OFF)
option(NO_DOC "Disable documentation build" OFF)
option(NO_OMP "Disable OpenMP backend" OFF)
option(NO_TBB "Disable TBB backend" OFF)
option(NO_CUDA "Disable CUDA backend" OFF)
option(NO_OPENCL "Disable OpenCL backend" OFF)
option(NO_CLEW "Disable CLEW wrapper library" OFF)
option(NO_OPENGL "Disable OpenGL support")
option(NO_METAL "Disable Metal support" OFF)
option(NO_DX "Disable DirectX support")
option(NO_TESTS "Disable all tests")
option(NO_GLTESTS "Disable GL tests")
option(NO_GLEW "Disable use of GLEW" ON)
option(NO_GLFW "Disable components depending on GLFW" OFF)
option(NO_GLFW_X11 "Disable GLFW components depending on X11" OFF)
option(NO_MACOS_FRAMEWORK "Disable generation of framework on macOS" OFF)

option(OPENSUBDIV_GREGORY_EVAL_TRUE_DERIVATIVES "Enable true derivative evaluation for Gregory basis patches" OFF)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Save the current value of BUILD_SHARED_LIBS and restore it after
# processing Find* modules, since some of the Find* modules invoked
# below may wind up stomping over this value.
set(build_shared_libs "${BUILD_SHARED_LIBS}")

set(OSD_GPU FALSE)

# Check for dependencies
if(NOT NO_OMP)
    find_package(OpenMP)
endif()
if(NOT NO_TBB)
    find_package(TBB 4.0)
endif()
if (NOT NO_OPENGL)
    find_package(OpenGL)
endif()
find_package(OpenGLES)
if(NOT NO_OPENCL)
    if(NOT NO_CLEW)
        find_package(CLEW)
    endif()
    if (NOT CLEW_FOUND)
        find_package(OpenCL 1.1)
    else()
        set(OPENCL_FOUND TRUE)
    endif()
endif()
if(NOT NO_CUDA)
    find_package(CUDA 4.0)
endif()
if(NOT NO_GLFW AND NOT NO_OPENGL AND NOT ANDROID AND NOT IOS)
    find_package(GLFW 3.0.0)
endif()
if(NOT NO_PTEX)
   find_package(PTex 2.0)
   find_package(ZLIB 1.2)
endif()
if(APPLE AND NOT NO_METAL)
    find_package(Metal)
endif()
if (OPENGL_FOUND AND NOT IOS)
    add_definitions(
        -DOPENSUBDIV_HAS_OPENGL
    )
    if (NOT NO_GLEW)
        if (APPLE)
            find_package(GLEW)
        else()
            find_package(GLEW REQUIRED)
        endif()
    endif()
    if(GLEW_FOUND)
        add_definitions( -DOSD_USES_GLEW )
    else()
        add_definitions( -DOSD_USES_INTERNAL_GLAPILOADER )
    endif()
endif()

if (WIN32 AND NOT NO_DX)
   find_package(DXSDK)
endif()

if (NOT NO_DOC)
    find_package(Doxygen 1.8.4)
    find_package(Docutils 0.9)
else()
    set(DOXYGEN_EXECUTABLE )
endif()

# Python is used optionally to process source and documentation source files.
find_package(Python COMPONENTS Interpreter)

set(BUILD_SHARED_LIBS "${build_shared_libs}")

# Warn about missing dependencies that will cause parts of OpenSubdiv to be
# disabled.  Also, add preprocessor defines that can be used in the source
# code to determine if a specific dependency is present or not.

if(OPENMP_FOUND)
    add_definitions(
        -DOPENSUBDIV_HAS_OPENMP
        ${OpenMP_CXX_FLAGS}
    )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_CXX_FLAGS}")
else()
    if (NOT NO_OMP)
        message(WARNING
            "OpenMP was not found : support for OMP parallel compute kernels "
            "will be disabled in Osd.  If your compiler supports OpenMP "
            "directives, please refer to the FindOpenMP.cmake shared module "
            "in your cmake installation.")
    endif()
endif()

if(TBB_FOUND)
    add_definitions(
        -DOPENSUBDIV_HAS_TBB
        ${TBB_CXX_FLAGS}
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TBB_CXX_FLAGS}")
else()
    if (NOT NO_TBB)
        message(WARNING
            "TBB was not found : support for TBB parallel compute kernels "
            "will be disabled in Osd.  If your compiler supports TBB "
            "directives, please refer to the FindTBB.cmake shared module "
            "in your cmake installation.")
    endif()
endif()

if( METAL_FOUND AND NOT NO_METAL)
    set(OSD_GPU TRUE)
    if (CMAKE_COMPILER_IS_CLANGCC)
        # When building with Metal support enabled, we need to explicitly
        # request that the C++ code be compiled with C++11 since some example
        # code for Metal relies on functionality only available in C++11.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
endif()

if( OPENGL_FOUND AND NOT NO_OPENGL)
    set(OSD_GPU TRUE)
endif()

if(GLFW_FOUND AND (GLFW_VERSION VERSION_EQUAL 3.0 OR GLFW_VERSION VERSION_GREATER 3.0))
    add_definitions( -DGLFW_VERSION_3 )
endif()

macro(osd_detect_gl_version header)

   if (EXISTS "${header}")
       file(STRINGS "${header}" VERSION_4_2 REGEX "^#define GL_VERSION_4_2.*$")
       if (VERSION_4_2)
           set(OPENGL_4_2_FOUND TRUE)
       else ()
           message(WARNING "OpenGL 4.2 dependent features not enabled")
       endif ()

       file(STRINGS "${header}" VERSION_4_3 REGEX "^#define GL_VERSION_4_3.*$")
       if (VERSION_4_3)
           SET(OPENGL_4_3_FOUND TRUE)
       else ()
           message(WARNING "OpenGL 4.3 dependent features not enabled")
       endif ()
   endif ()

endmacro()

if(GLEW_FOUND AND GLEW_INCLUDE_DIR)

    osd_detect_gl_version(${GLEW_INCLUDE_DIR}/GL/glew.h)
    set(OPENGL_LOADER_INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/glLoader)
    set(OPENGL_LOADER_LIBRARIES
        ${GLEW_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${CMAKE_DL_LIBS})

elseif(OPENGL_FOUND)

    osd_detect_gl_version(${PROJECT_SOURCE_DIR}/glLoader/glApi.h)
    set(OPENGL_LOADER_INCLUDE_DIRS
        ${OPENGL_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/glLoader)
    set(OPENGL_LOADER_LIBRARIES
        ${OPENGL_gl_LIBRARY}
        ${CMAKE_DL_LIBS})

endif()

# note : (GLSL transform feedback kernels require GL 4.2)
if(OPENGL_4_2_FOUND)
    add_definitions(
        -DOPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK
    )
else()
    if (NOT NO_OPENGL)
        message(WARNING
            "OpenGL 4.2 was not found : support for GLSL transform feedback kernels "
            "will be disabled in Osd.  If you have an OpenGL SDK installed "
            "(version 4.2 or above), please refer to the FindOpenGL.cmake "
            "shared module in your cmake installation.")
    endif()
endif()

# note : (GLSL compute shader kernels require GL 4.3)
if(OPENGL_4_3_FOUND)
    add_definitions(
        -DOPENSUBDIV_HAS_GLSL_COMPUTE
    )
else()
    if (NOT NO_OPENGL)
        message(WARNING
            "OpenGL 4.3 was not found : support for GLSL compute shader kernels "
            "will be disabled in Osd.  If you have an OpenGL SDK installed "
            "(version 4.3 or above), please refer to the FindOpenGL.cmake "
            "shared module in your cmake installation.")
    endif()
endif()

if(OPENGLES_FOUND)
    add_definitions(
        -DOPENSUBDIV_HAS_OPENGLES
    )
    set(OSD_GPU TRUE)
endif()

if(OPENCL_FOUND)
    add_definitions(
        -DOPENSUBDIV_HAS_OPENCL
    )
    if(CLEW_FOUND)
        add_definitions(
            -DOPENSUBDIV_HAS_CLEW
        )
        set(OPENCL_INCLUDE_DIRS ${CLEW_INCLUDE_DIR})
        set(OPENCL_LIBRARIES ${CLEW_LIBRARY} ${CMAKE_DL_LIBS})
    else()
        if (NOT NO_CLEW)
            message(WARNING
                "OpenCL was found, but CLEW was not. "
                "Building with OpenCL support enabled, but the built binary "
                "will not be portable to systems without OpenCL installed.")
        endif()
    endif()

    if (DXSDK_FOUND AND NOT NO_DX)
        if (OPENCL_CL_D3D11_H_FOUND)
            set(OPENCL_D3D11_INTEROP_FOUND "YES")
            add_definitions(
                -DOPENSUBDIV_HAS_OPENCL_DX_INTEROP
                -DOPENSUBDIV_HAS_CL_D3D11_H
            )
        endif()
        if (OPENCL_CL_D3D11_EXT_H_FOUND)
            set(OPENCL_D3D11_INTEROP_FOUND "YES")
            add_definitions(
                -DOPENSUBDIV_HAS_OPENCL_DX_INTEROP
                -DOPENSUBDIV_HAS_CL_D3D11_EXT_H
            )
        endif()
    endif()
    set(OSD_GPU TRUE)
else()
    if (NOT NO_OPENCL)
        message(WARNING
            "OpenCL was not found : support for OpenCL parallel compute kernels "
            "will be disabled in Osd.  If you have the OpenCL SDK installed, "
            "please refer to the FindOpenCL.cmake in ${PROJECT_SOURCE_DIR}/cmake.")
    endif()
endif()

if(CUDA_FOUND)
    add_definitions(
        -DOPENSUBDIV_HAS_CUDA
        -DCUDA_ENABLE_DEPRECATED=0
    )
    set(OSD_GPU TRUE)

    if (UNIX)
        list( APPEND CUDA_NVCC_FLAGS -Xcompiler -fPIC )
        if (NOT DEFINED OSD_CUDA_NVCC_FLAGS)
            if (CUDA_VERSION_MAJOR LESS 6)
                set( OSD_CUDA_NVCC_FLAGS --gpu-architecture compute_11 )
            else()
                set( OSD_CUDA_NVCC_FLAGS --gpu-architecture compute_20 )
            endif()
        endif()
    endif()

    if (DEFINED OSD_CUDA_NVCC_FLAGS)
        list( APPEND CUDA_NVCC_FLAGS ${OSD_CUDA_NVCC_FLAGS})
    endif()

else()
    if (NOT NO_CUDA)
        message(WARNING
            "CUDA was not found : support for CUDA parallel compute kernels "
            "will be disabled in Osd.  If you have the CUDA SDK installed, please "
            "refer to the FindCUDA.cmake shared module in your cmake installation.")
    endif()
endif()

if(PTEX_FOUND)
    add_definitions(
        -DOPENSUBDIV_HAS_PTEX
        -DPTEX_STATIC
    )
else()
    if(NOT NO_PTEX)
        message(WARNING
            "Ptex was not found : the OpenSubdiv Ptex example will not be "
            "available.  If you do have Ptex installed and see this message, "
            "please add your Ptex path to FindPTex.cmake in "
            "${PROJECT_SOURCE_DIR}/cmake or set it through the PTEX_LOCATION "
            "cmake command line argument or environment variable.")
    endif()
endif()

if( OPENSUBDIV_GREGORY_EVAL_TRUE_DERIVATIVES )
    add_definitions(-DOPENSUBDIV_GREGORY_EVAL_TRUE_DERIVATIVES)
endif()

# Link examples & regressions against Osd
if( BUILD_SHARED_LIBS )
    if( OSD_GPU )
        set( OSD_LINK_TARGET osd_dynamic_gpu osd_dynamic_cpu )
    else()
        set( OSD_LINK_TARGET osd_dynamic_cpu )
    endif()
else()
    if( OSD_GPU )
        set( OSD_LINK_TARGET osd_static_gpu osd_static_cpu )
    else()
        set( OSD_LINK_TARGET osd_static_cpu )
    endif()
endif()

if (WIN32)
    if ("${GLEW_LIBRARY}" MATCHES "glew32s(d|)")
        # Link against the static version of GLEW
        add_definitions(
            -DGLEW_STATIC
        )
    endif()

    if (DXSDK_FOUND AND NOT NO_DX)
        add_definitions(
            -DOPENSUBDIV_HAS_DX11SDK
        )
        set(OSD_GPU TRUE)
    elseif(NOT NO_DX)
        message(WARNING
            "DirectX11 SDK was not found. "
            "If you do have DXSDK installed and see this message, "
            "please add your sdk path to FindDirectX.cmake in "
            "${PROJECT_SOURCE_DIR}/cmake or set it through the "
            "DXSDK_LOCATION cmake command line argument or "
            "environment variable."
        )
    endif()

    # Link examples & regressions statically against Osd for
    # Windows until all the kinks can be worked out.
    if( OSD_GPU )
        set( OSD_LINK_TARGET osd_static_cpu osd_static_gpu )
    else()
        set( OSD_LINK_TARGET osd_static_cpu )
    endif()

endif()


#-------------------------------------------------------------------------------
# General-use macros

# Macro for processing public headers into the build area for doxygen processing

add_custom_target( public_headers )

macro(osd_add_doxy_headers headers)
    if (NOT NO_DOC AND DOXYGEN_FOUND)
        file(RELATIVE_PATH path "${OpenSubdiv_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" )

        string(REPLACE  "/" "_" targetpath ${path})

        foreach (header ${headers})

            set(infile "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
            set(outfile "${OpenSubdiv_BINARY_DIR}/public_headers/${path}/${header}")
            set(targetname "${targetpath}_${header}")

            add_custom_command(
                OUTPUT
                    "${outfile}"
                COMMAND
                    ${CMAKE_COMMAND}
                ARGS
                    -E copy ${infile} ${outfile}
                DEPENDS
                    ${infile}
            )

            add_custom_target(${targetname} DEPENDS "${outfile}")


            list(APPEND headerfiles ${targetname} )
        endforeach()

        add_dependencies( public_headers ${headerfiles} )
    endif()
endmacro()

# Kernel Stringification
# We want to use preprocessor include directives to include GLSL, OpenCL, etc.
# kernel source files in cpp files, but since the sources contain newline
# characters we would need raw string literals from C++11 to do this directly.
# To avoid depending on C++11 we instead use a small tool called "stringify"
# to generate source files that are suitable for direct inclusion.

# We provide a Python implementation for configurability, e.g. to
# use when cross compiling or building multi-architecture binaries.
# We also provide a C++ binary implementation so that Python is not
# required (for backward compatibility).
if (OPENGL_FOUND OR OPENCL_FOUND OR DXSDK_FOUND OR METAL_FOUND)
    if(Python_Interpreter_FOUND)
        set(OSD_STRINGIFY_TOOL ${CMAKE_CURRENT_SOURCE_DIR}/tools/stringify/stringify.py)
        set(OSD_STRINGIFY ${Python_EXECUTABLE} ${OSD_STRINGIFY_TOOL})
    else()
        set(OSD_STRINGIFY_TOOL stringify)
        set(OSD_STRINGIFY ${OSD_STRINGIFY_TOOL})
        set(OSD_USES_STRINGIFY_TOOL_BINARY TRUE)
    endif()
endif()

function(osd_stringify src_files varname)

    set(inc_files "")

    foreach(src_file ${src_files})

        string(REGEX REPLACE ".*[.](.*)" "\\1" extension "${src_file}")

        string(REGEX REPLACE "(.*)[.].*" "\\1.gen.h" inc_file "${src_file}")
        list(APPEND inc_files "${CMAKE_CURRENT_BINARY_DIR}/${inc_file}")

        add_custom_command(
            OUTPUT
                "${CMAKE_CURRENT_BINARY_DIR}/${inc_file}"
            COMMAND
                ${OSD_STRINGIFY} "${CMAKE_CURRENT_SOURCE_DIR}/${src_file}" "${CMAKE_CURRENT_BINARY_DIR}/${inc_file}"
            DEPENDS
                ${OSD_STRINGIFY_TOOL} "${CMAKE_CURRENT_SOURCE_DIR}/${src_file}"
        )

    endforeach()
    set(${varname} ${inc_files} PARENT_SCOPE)
endfunction()

# Macro wrapper for adding a non-cuda dependent executable
macro(osd_add_executable target folder)

    add_executable(${target} ${ARGN})

    set_target_properties(${target} PROPERTIES FOLDER ${folder})

    if(CMAKE_COMPILER_IS_ICC)
        target_link_libraries(${target} ${ICC_LIBRARIES})
    endif()

    if(APPLE)
        set_property (TARGET ${target} APPEND_STRING PROPERTY
              COMPILE_FLAGS " -fobjc-arc ")
    endif()
endmacro()


# Macro for adding a cuda executable if cuda is found and a regular
# executable otherwise.
macro(osd_add_possibly_cuda_executable target folder)
    if(CUDA_FOUND)
        cuda_add_executable(${target} ${ARGN})
    else()
        add_executable(${target} ${ARGN})
    endif()

    set_target_properties(${target} PROPERTIES FOLDER ${folder})

    if(CMAKE_COMPILER_IS_ICC)
        target_link_libraries(${target} ${ICC_LIBRARIES})
    endif()

    if(APPLE)
        set_property (TARGET ${target} APPEND_STRING PROPERTY
              COMPILE_FLAGS " -fobjc-arc ")
    endif()
endmacro()


# Macro for adding a cuda library if cuda is found and a regular
# library otherwise.
macro(osd_add_possibly_cuda_library target folder)
    if(CUDA_FOUND)
        cuda_add_library(${target} ${ARGN})
    else()
        add_library(${target} ${ARGN})
    endif()
    set_target_properties(${target} PROPERTIES FOLDER ${folder})

    if(APPLE)
        set_property (TARGET ${target} APPEND_STRING PROPERTY
              COMPILE_FLAGS " -fobjc-arc ")
    endif()
endmacro()


# Macro for adding a (potentially cuda) GLFW executable.
macro(osd_add_glfw_executable target folder)

    osd_add_possibly_cuda_executable(${target} ${folder} ${ARGN})

    if(APPLE)
        set_property (TARGET ${target} APPEND_STRING PROPERTY
              COMPILE_FLAGS " -fobjc-arc ")
    endif()

endmacro()

#-------------------------------------------------------------------------------
# Build targets


# if you want to build examples against installed OpenSubdiv header files,
# use OPENSUBDIV_INCLUDE_DIR.

# example: if you have already installed opensubdiv libs in this cmake setup,
# set (OPENSUBDIV_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INCDIR_BASE})

if (NOT OPENSUBDIV_INCLUDE_DIR)
    set(OPENSUBDIV_INCLUDE_DIR "${PROJECT_SOURCE_DIR}")
endif()

if (NOT NO_TESTS)
    enable_testing()
endif()

if (NOT NO_OPENGL)
    add_subdirectory(glLoader)
endif()

if (OSD_USES_STRINGIFY_TOOL_BINARY)
    add_subdirectory(tools/stringify)
endif()

add_subdirectory(opensubdiv)

if (NOT ANDROID) # XXXdyu
    add_subdirectory(regression)
endif()

if (NOT NO_EXAMPLES)
    add_subdirectory(examples)
endif()

if (NOT NO_TUTORIALS)
    add_subdirectory(tutorials)
endif()

if (NOT NO_DOC)
    add_subdirectory(documentation)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.2)
    #
    # CMake Config.
    #
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    set(OPENSUBDIV_CONFIG_PATH "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

    configure_package_config_file(
        opensubdiv-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/OpenSubdivConfig.cmake
        INSTALL_DESTINATION ${OPENSUBDIV_CONFIG_PATH}
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/OpenSubdivConfigVersion.cmake
        VERSION ${OSD_SONAME}
        COMPATIBILITY SameMajorVersion
    )

    install(EXPORT opensubdiv-targets
        NAMESPACE OpenSubdiv::
        FILE OpenSubdivTargets.cmake
        DESTINATION ${OPENSUBDIV_CONFIG_PATH})

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/OpenSubdivConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/OpenSubdivConfigVersion.cmake
        DESTINATION ${OPENSUBDIV_CONFIG_PATH}
    )
endif()
