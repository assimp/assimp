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

#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# source & headers
set(CPU_SOURCE_FILES
    cpuEvaluator.cpp
    cpuKernel.cpp
    cpuPatchTable.cpp
    cpuVertexBuffer.cpp
)

set(GPU_SOURCE_FILES )

set(INC_FILES )

set(PRIVATE_HEADER_FILES
    cpuKernel.h
)

set(PUBLIC_HEADER_FILES
    bufferDescriptor.h
    cpuEvaluator.h
    cpuPatchTable.h
    cpuVertexBuffer.h
    mesh.h
    nonCopyable.h
    opengl.h
    types.h
)

list(APPEND KERNEL_FILES
    patchBasisCommonTypes.h
    patchBasisCommon.h
    patchBasisCommonEval.h
)

set(DOXY_HEADER_FILES ${PUBLIC_HEADER_FILES})

#-------------------------------------------------------------------------------
set(OPENMP_PUBLIC_HEADERS
    ompEvaluator.h
    ompKernel.h
)

if(OPENMP_FOUND )
    list(APPEND CPU_SOURCE_FILES
        ompEvaluator.cpp
        ompKernel.cpp
    )

    list(APPEND PUBLIC_HEADER_FILES ${OPENMP_PUBLIC_HEADERS})

    if (CMAKE_COMPILER_IS_GNUCXX)
        list(APPEND PLATFORM_CPU_LIBRARIES gomp)
    endif()
endif()

list(APPEND DOXY_HEADER_FILES ${OPENMP_PUBLIC_HEADERS})

#-------------------------------------------------------------------------------
set(TBB_PUBLIC_HEADERS
    tbbEvaluator.h
    tbbKernel.h
)

if( TBB_FOUND )
    include_directories("${TBB_INCLUDE_DIR}")

    list(APPEND CPU_SOURCE_FILES
        tbbEvaluator.cpp
        tbbKernel.cpp
    )

    list(APPEND PUBLIC_HEADER_FILES ${TBB_PUBLIC_HEADERS})

    if (CMAKE_COMPILER_IS_GNUCXX)
        list(APPEND PLATFORM_CPU_LIBRARIES gomp)
    endif()

    list(APPEND PLATFORM_CPU_LIBRARIES
        ${TBB_LIBRARIES}
    )
endif()

list(APPEND DOXY_HEADER_FILES ${TBB_PUBLIC_HEADERS})

#-------------------------------------------------------------------------------
# GL code & dependencies
set(GL_PUBLIC_HEADERS
    cpuGLVertexBuffer.h
    glLegacyGregoryPatchTable.h
    glPatchTable.h
    glVertexBuffer.h
    glMesh.h
    glslPatchShaderSource.h
)

if( (NOT NO_OPENGL) AND (OPENGL_FOUND OR OPENGLES_FOUND) )
    list(APPEND GPU_SOURCE_FILES
        cpuGLVertexBuffer.cpp
        glLegacyGregoryPatchTable.cpp
        glPatchTable.cpp
        glVertexBuffer.cpp
        glslPatchShaderSource.cpp
    )
    list(APPEND PUBLIC_HEADER_FILES ${GL_PUBLIC_HEADERS})
    if ( OPENGL_FOUND )
        list(APPEND KERNEL_FILES
            glslPatchCommon.glsl
            glslPatchCommonTess.glsl
            glslPatchBoxSplineTriangle.glsl
            glslPatchBSpline.glsl
            glslPatchGregory.glsl
            glslPatchGregoryBasis.glsl
            glslPatchGregoryTriangle.glsl
            glslPatchLegacy.glsl
        )
    endif()
endif()

list(APPEND DOXY_HEADER_FILES ${GL_PUBLIC_HEADERS})

#-------------------------------------------------------------------------------
# OpenGL 4.2 dependencies
# note : (GLSL transform feedback kernels require GL 4.2)
set(GL_4_2_PUBLIC_HEADERS
    glXFBEvaluator.h
)

if( OPENGL_4_2_FOUND )
    list(APPEND GPU_SOURCE_FILES
        glXFBEvaluator.cpp
    )
    list(APPEND PUBLIC_HEADER_FILES ${GL_4_2_PUBLIC_HEADERS})
    list(APPEND KERNEL_FILES
        glslXFBKernel.glsl
    )
    list(APPEND PLATFORM_GPU_LIBRARIES
        ${OPENGL_LOADER_LIBRARIES}
    )
endif()

list(APPEND DOXY_HEADER_FILES ${GL_4_2_PUBLIC_HEADERS})


#-------------------------------------------------------------------------------
# OpenGL 4.3 dependencies
# note : (GLSL compute shader kernels require GL 4.3)
set(GL_4_3_PUBLIC_HEADERS
    glComputeEvaluator.h
)

if( OPENGL_4_3_FOUND )
    list(APPEND GPU_SOURCE_FILES
        glComputeEvaluator.cpp
    )
    list(APPEND PUBLIC_HEADER_FILES ${GL_4_3_PUBLIC_HEADERS})
    list(APPEND KERNEL_FILES
        glslComputeKernel.glsl
    )
    list(APPEND PLATFORM_GPU_LIBRARIES
        ${OPENGL_LOADER_LIBRARIES}
    )
endif()

list(APPEND DOXY_HEADER_FILES ${GL_4_3_PUBLIC_HEADERS})

#-------------------------------------------------------------------------------
# DX11 code & dependencies
set(DXSDK_PUBLIC_HEADERS
    cpuD3D11VertexBuffer.h
    d3d11ComputeEvaluator.h
    d3d11LegacyGregoryPatchTable.h
    d3d11PatchTable.h
    d3d11VertexBuffer.h
    d3d11Mesh.h
    hlslPatchShaderSource.h
)
if( DXSDK_FOUND )
    list(APPEND GPU_SOURCE_FILES
        cpuD3D11VertexBuffer.cpp
        d3d11ComputeEvaluator.cpp
        d3d11LegacyGregoryPatchTable.cpp
        d3d11PatchTable.cpp
        d3d11VertexBuffer.cpp
        hlslPatchShaderSource.cpp
    )
    list(APPEND PUBLIC_HEADER_FILES ${DXSDK_PUBLIC_HEADERS})
    list(APPEND KERNEL_FILES
        hlslComputeKernel.hlsl
        hlslPatchCommon.hlsl
        hlslPatchCommonTess.hlsl
        hlslPatchBoxSplineTriangle.hlsl
        hlslPatchBSpline.hlsl
        hlslPatchGregory.hlsl
        hlslPatchGregoryBasis.hlsl
        hlslPatchGregoryTriangle.hlsl
        hlslPatchLegacy.hlsl
    )
    list(APPEND PLATFORM_GPU_LIBRARIES
        ${DXSDK_LIBRARIES}
    )
endif()

list(APPEND DOXY_HEADER_FILES ${DXSDK_PUBLIC_HEADERS})

#-------------------------------------------------------------------------------
# Metal code & dependencies
set(METAL_PUBLIC_HEADERS
    mtlVertexBuffer.h
    mtlComputeEvaluator.h
    mtlLegacyGregoryPatchTable.h
    mtlPatchTable.h
    mtlMesh.h
    mtlPatchShaderSource.h
    mtlCommon.h
)


if( METAL_FOUND )
    set(METAL_SOURCE_FILES
        mtlVertexBuffer.mm
        mtlComputeEvaluator.mm
        mtlLegacyGregoryPatchTable.mm
        mtlPatchTable.mm
        mtlVertexBuffer.mm
        mtlPatchShaderSource.mm
    )

    set_source_files_properties(
        ${METAL_SOURCE_FILES}
        PROPERTIES
        COMPILE_FLAGS
        "-fobjc-arc")

    list(APPEND GPU_SOURCE_FILES ${METAL_SOURCE_FILES})

    

    list(APPEND PUBLIC_HEADER_FILES ${METAL_PUBLIC_HEADERS})
    list(APPEND KERNEL_FILES
       mtlComputeKernel.metal
       mtlPatchCommon.metal
       mtlPatchCommonTess.metal
       mtlPatchBoxSplineTriangle.metal
       mtlPatchBSpline.metal
       mtlPatchGregory.metal
       mtlPatchGregoryBasis.metal
       mtlPatchGregoryTriangle.metal
       mtlPatchLegacy.metal
    )
    list(APPEND PLATFORM_GPU_LIBRARIES
        ${METAL_LIBRARIES}
    )
endif()

list(APPEND DOXY_HEADER_FILES ${METAL_PUBLIC_HEADERS})

#-------------------------------------------------------------------------------
# OpenCL code & dependencies
set(OPENCL_PUBLIC_HEADERS
    clEvaluator.h
    clPatchTable.h
    clVertexBuffer.h
    opencl.h
)

if ( OPENCL_FOUND )
    list(APPEND GPU_SOURCE_FILES
        clEvaluator.cpp
        clPatchTable.cpp
        clVertexBuffer.cpp
    )
    list(APPEND PUBLIC_HEADER_FILES ${OPENCL_PUBLIC_HEADERS})
    list(APPEND KERNEL_FILES
        clKernel.cl
    )
    list(APPEND PLATFORM_GPU_LIBRARIES
        ${OPENCL_LIBRARIES}
    )
    include_directories( "${OPENCL_INCLUDE_DIRS}" )
    if ( OPENGL_FOUND )
        list(APPEND GPU_SOURCE_FILES clGLVertexBuffer.cpp)
        list(APPEND PUBLIC_HEADER_FILES clGLVertexBuffer.h)
    endif()

    if ( OPENCL_D3D11_INTEROP_FOUND )
        list(APPEND GPU_SOURCE_FILES
             clD3D11VertexBuffer.cpp
        )
        list(APPEND PUBLIC_HEADER_FILES
             clD3D11VertexBuffer.h
        )
    endif()
endif()

list(APPEND DOXY_HEADER_FILES ${OPENCL_PUBLIC_HEADERS})

#-------------------------------------------------------------------------------
# CUDA code & dependencies
set(CUDA_PUBLIC_HEADERS
    cudaEvaluator.h
    cudaPatchTable.h
    cudaVertexBuffer.h
)

if( CUDA_FOUND )
    list(APPEND GPU_SOURCE_FILES
        cudaEvaluator.cpp
        cudaPatchTable.cpp
        cudaVertexBuffer.cpp
    )
    list(APPEND PUBLIC_HEADER_FILES ${CUDA_PUBLIC_HEADERS})
    list(APPEND CUDA_KERNEL_FILES
        osd/cudaKernel.cu
    )
    set (CUDA_KERNEL_FILES ${CUDA_KERNEL_FILES} PARENT_SCOPE)
    if ( OPENGL_FOUND )
        list(APPEND GPU_SOURCE_FILES
            cudaGLVertexBuffer.cpp
        )
        list(APPEND PUBLIC_HEADER_FILES
            cudaGLVertexBuffer.h
        )
    endif()

    if( DXSDK_FOUND )
        list(APPEND GPU_SOURCE_FILES
            cudaD3D11VertexBuffer.cpp
        )
        list(APPEND PUBLIC_HEADER_FILES
            cudaD3D11VertexBuffer.h
        )
    endif()
endif()

list(APPEND DOXY_HEADER_FILES ${CUDA_PUBLIC_HEADERS})

#-------------------------------------------------------------------------------

osd_stringify("${KERNEL_FILES}" INC_FILES)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

#-------------------------------------------------------------------------------
source_group("Kernels" FILES ${KERNEL_FILES})

source_group("Inc" FILES ${INC_FILES})

# Compile objs first for both the CPU and GPU libs -----
add_library(osd_cpu_obj
    OBJECT
        ${CPU_SOURCE_FILES}
        ${PRIVATE_HEADER_FILES}
        ${PUBLIC_HEADER_FILES}
)

set_target_properties(osd_cpu_obj
    PROPERTIES
        FOLDER "opensubdiv"
)

if( GPU_SOURCE_FILES ) 
    add_library(osd_gpu_obj
        OBJECT
            ${GPU_SOURCE_FILES}
            ${PRIVATE_HEADER_FILES}
            ${PUBLIC_HEADER_FILES}
            ${INC_FILES}
    )
    set_target_properties(osd_gpu_obj
        PROPERTIES
            FOLDER "opensubdiv"
    )
endif()

osd_add_doxy_headers( "${DOXY_HEADER_FILES}" )

install(
    FILES
        ${PUBLIC_HEADER_FILES}
    DESTINATION
        "${CMAKE_INCDIR_BASE}/osd"
    PERMISSIONS
        OWNER_READ
        GROUP_READ
        WORLD_READ )

if (ANDROID)
    install(
        FILES
            Android.mk
        DESTINATION
            "${LIBRARY_OUTPUT_PATH_ROOT}"
        PERMISSIONS
            OWNER_READ
            GROUP_READ
            WORLD_READ )
endif()

#-------------------------------------------------------------------------------
