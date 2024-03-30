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

if (NOT NO_LIB)

    #---------------------------------------------------------------------------
    include_directories(
        "${PROJECT_SOURCE_DIR}/opensubdiv"
    )

    #---------------------------------------------------------------------------
    # platform dependent tweaks
    if(APPLE OR UNIX)
        set(PLATFORM_COMPILE_FLAGS -fPIC)
    elseif(WIN32)

    endif()

    add_definitions(
        ${PLATFORM_COMPILE_FLAGS}
    )

    #---------------------------------------------------------------------------
    if( OPENMP_FOUND )
        if (CMAKE_COMPILER_IS_GNUCXX)
            list(APPEND PLATFORM_CPU_LIBRARIES gomp)
        endif()
    endif()

    if( TBB_FOUND )
        include_directories("${TBB_INCLUDE_DIR}")
        list(APPEND PLATFORM_CPU_LIBRARIES
            ${TBB_LIBRARIES}
        )
    endif()

    if( OPENGL_FOUND )
        include_directories(${OPENGL_LOADER_INCLUDE_DIRS})
        list(APPEND PLATFORM_GPU_LIBRARIES
            ${OPENGL_LOADER_LIBRARIES}
        )
        set(OPENGL_LOADER_OBJS $<TARGET_OBJECTS:glLoader_obj>)
    elseif( OPENGLES_FOUND )
        include_directories("${OPENGLES_INCLUDE_DIR}")
        list(APPEND PLATFORM_GPU_LIBRARIES
            ${OPENGLES_LIBRARIES}
        )
    endif()

    if( DXSDK_FOUND )
        include_directories( "${DXSDK_INCLUDE_DIR}" )
        list(APPEND PLATFORM_GPU_LIBRARIES
            ${DXSDK_LIBRARIES}
        )
    endif()

    if( METAL_FOUND )
        include_directories( "${METAL_INCLUDE_DIR}" )
        list(APPEND PLATFORM_GPU_LIBRARIES
            ${METAL_LIBRARIES}
        )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    endif()

    if ( OPENCL_FOUND )
        include_directories( "${OPENCL_INCLUDE_DIRS}" )
        list(APPEND PLATFORM_GPU_LIBRARIES
            ${OPENCL_LIBRARIES}
        )
    endif()

    if( CUDA_FOUND )
        include_directories( "${CUDA_INCLUDE_DIRS}" )
    endif()



    #---------------------------------------------------------------------------

    add_subdirectory(hbr)

    add_subdirectory(sdc)

    add_subdirectory(vtr)

    add_subdirectory(far)

    add_subdirectory(bfr)

    add_subdirectory(osd)

    #---------------------------------------------------------------------------

    install( FILES version.h
        DESTINATION
            "${CMAKE_INCDIR_BASE}"
        PERMISSIONS
            OWNER_READ
            GROUP_READ
            WORLD_READ )

    #---------------------------------------------------------------------------

    # Build static libs ------------------------------------
    add_library(osd_static_cpu
        STATIC
        version.cpp
        $<TARGET_OBJECTS:sdc_obj>
        $<TARGET_OBJECTS:vtr_obj>
        $<TARGET_OBJECTS:far_obj>
        $<TARGET_OBJECTS:bfr_obj>
        $<TARGET_OBJECTS:osd_cpu_obj>
    )

    set_target_properties(osd_static_cpu
        PROPERTIES
            OUTPUT_NAME osdCPU
            EXPORT_NAME osdCPU_static
            CLEAN_DIRECT_OUTPUT 1
            FOLDER "opensubdiv"
    )

    target_link_libraries(osd_static_cpu
        ${PLATFORM_CPU_LIBRARIES}
    )

    install( TARGETS osd_static_cpu EXPORT opensubdiv-targets DESTINATION "${CMAKE_LIBDIR_BASE}" )

    if( OSD_GPU )
        # this macro uses FindCUDA.cmake to compile .cu kernel files
        # the target then adds the other obj dependencies and include files
        osd_add_possibly_cuda_library(osd_static_gpu "opensubdiv"
                STATIC
                version.cpp
                $<TARGET_OBJECTS:osd_gpu_obj>
                ${OPENGL_LOADER_OBJS}
                ${CUDA_KERNEL_FILES}
        )
        set_target_properties(osd_static_gpu
            PROPERTIES
                OUTPUT_NAME osdGPU
                EXPORT_NAME osdGPU_static
                CLEAN_DIRECT_OUTPUT 1)

        target_link_libraries(osd_static_gpu
            ${PLATFORM_CPU_LIBRARIES} ${PLATFORM_GPU_LIBRARIES}
        )

        install( TARGETS osd_static_gpu EXPORT opensubdiv-targets DESTINATION "${CMAKE_LIBDIR_BASE}" )
    endif()


    # Build dynamic libs  ----------------------------------
    if (BUILD_SHARED_LIBS AND NOT WIN32 AND NOT IOS)

        # generate dynamic-link targets

        #---------------------------------------------------
        add_library(osd_dynamic_cpu
            SHARED
            version.cpp
            $<TARGET_OBJECTS:sdc_obj>
            $<TARGET_OBJECTS:vtr_obj>
            $<TARGET_OBJECTS:far_obj>
            $<TARGET_OBJECTS:bfr_obj>
            $<TARGET_OBJECTS:osd_cpu_obj>
        )

        if (NOT ANDROID)
            set_target_properties(osd_dynamic_cpu
                PROPERTIES
                    OUTPUT_NAME osdCPU
                    EXPORT_NAME osdCPU
                    CLEAN_DIRECT_OUTPUT 1
                    SOVERSION ${OSD_SONAME}
                )
        else()
            set_target_properties(osd_dynamic_cpu
                PROPERTIES
                    OUTPUT_NAME osdCPU
                    EXPORT_NAME osdCPU
                    CLEAN_DIRECT_OUTPUT 1
                )
        endif()

        target_link_libraries(osd_dynamic_cpu
            ${PLATFORM_CPU_LIBRARIES}
        )

        install( TARGETS osd_dynamic_cpu EXPORT opensubdiv-targets LIBRARY DESTINATION "${CMAKE_LIBDIR_BASE}" )

        #---------------------------------------------------
        if( OSD_GPU )
            osd_add_possibly_cuda_library(osd_dynamic_gpu "opensubdiv"
                SHARED
                version.cpp
                $<TARGET_OBJECTS:osd_gpu_obj>
                ${OPENGL_LOADER_OBJS}
                ${CUDA_KERNEL_FILES}
            )

            if (NOT ANDROID)
                set_target_properties(osd_dynamic_gpu
                    PROPERTIES
                        OUTPUT_NAME osdGPU
                        EXPORT_NAME osdGPU
                        CLEAN_DIRECT_OUTPUT 1
                        SOVERSION ${OSD_SONAME}
                    )
            else()
                set_target_properties(osd_dynamic_gpu
                    PROPERTIES
                        OUTPUT_NAME osdGPU
                        EXPORT_NAME osdGPU
                        CLEAN_DIRECT_OUTPUT 1
                    )
            endif()

            target_link_libraries(osd_dynamic_gpu
                osd_dynamic_cpu
                ${PLATFORM_CPU_LIBRARIES} ${PLATFORM_GPU_LIBRARIES}
            )

            install( TARGETS osd_dynamic_gpu EXPORT opensubdiv-targets LIBRARY DESTINATION "${CMAKE_LIBDIR_BASE}" )
        endif()

    endif()

    # Build frameworks  ----------------------------------
    if(APPLE AND NOT NO_MACOS_FRAMEWORK)
        get_directory_property(OSD_HEADER_FILES DIRECTORY ${PROJECT_SOURCE_DIR}/opensubdiv/osd DEFINITION PUBLIC_HEADER_FILES)
        get_directory_property(FAR_HEADER_FILES DIRECTORY ${PROJECT_SOURCE_DIR}/opensubdiv/far DEFINITION PUBLIC_HEADER_FILES)
        get_directory_property(SDC_HEADER_FILES DIRECTORY ${PROJECT_SOURCE_DIR}/opensubdiv/sdc DEFINITION PUBLIC_HEADER_FILES)
        get_directory_property(HBR_HEADER_FILES DIRECTORY ${PROJECT_SOURCE_DIR}/opensubdiv/hbr DEFINITION PUBLIC_HEADER_FILES)
        get_directory_property(VTR_HEADER_FILES DIRECTORY ${PROJECT_SOURCE_DIR}/opensubdiv/vtr DEFINITION PUBLIC_HEADER_FILES)
        get_directory_property(BFR_HEADER_FILES DIRECTORY ${PROJECT_SOURCE_DIR}/opensubdiv/bfr DEFINITION PUBLIC_HEADER_FILES)


        foreach(file ${OSD_HEADER_FILES})
            list(APPEND PUBLIC_HEADER_FILES "osd/${file}")
        endforeach(file)

        foreach(file ${FAR_HEADER_FILES})
            list(APPEND PUBLIC_HEADER_FILES "far/${file}")
        endforeach(file)

        foreach(file ${SDC_HEADER_FILES})
            list(APPEND PUBLIC_HEADER_FILES "sdc/${file}")
        endforeach(file)

        foreach(file ${HBR_HEADER_FILES})
            list(APPEND PUBLIC_HEADER_FILES "hbr/${file}")
        endforeach(file)

        foreach(file ${VTR_HEADER_FILES})
            list(APPEND PUBLIC_HEADER_FILES "vtr/${file}")
        endforeach(file)

        foreach(file ${BFR_HEADER_FILES})
            list(APPEND PUBLIC_HEADER_FILES "bfr/${file}")
        endforeach(file)

        list(APPEND PUBLIC_HEADER_FILES "version.h")

        #static framework
        add_library(osd_static_framework
            STATIC
            version.cpp
            $<TARGET_OBJECTS:sdc_obj>
            $<TARGET_OBJECTS:vtr_obj>
            $<TARGET_OBJECTS:far_obj>
            $<TARGET_OBJECTS:bfr_obj>
            $<TARGET_OBJECTS:osd_cpu_obj>
            $<TARGET_OBJECTS:osd_gpu_obj>
            ${OPENGL_LOADER_OBJS}
        )

        set_target_properties(
            osd_static_framework
            PROPERTIES
            FRAMEWORK true
            INSTALL_NAME_DIR "@rpath/OpenSubdiv.framework/OpenSubdiv"
            INSTALL_RPATH "@executable_path/Frameworks;@loader_path/Frameworks"
            OUTPUT_NAME OpenSubdiv_static
            EXPORT_NAME OpenSubdiv_static
            CLEAN_DIRECT_OUTPUT true
        )

        target_link_libraries(osd_static_framework
            ${PLATFORM_CPU_LIBRARIES} ${PLATFORM_GPU_LIBRARIES}
        )

        install( TARGETS osd_static_framework EXPORT opensubdiv-targets
             LIBRARY DESTINATION "${CMAKE_LIBDIR_BASE}"
             FRAMEWORK DESTINATION "${CMAKE_LIBDIR_BASE}"
             PUBLIC_HEADER  DESTINATION "${CMAKE_INCDIR_BASE}"
             ARCHIVE DESTINATION "${CMAKE_LIBDIR_BASE}")

        foreach(file ${PUBLIC_HEADER_FILES})
            add_custom_command(TARGET osd_static_framework POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/opensubdiv/${file}"
                "$<TARGET_FILE_DIR:osd_static_framework>/Headers/${file}"
            )
        endforeach(file)

        add_custom_command(TARGET osd_static_framework POST_BUILD
            COMMAND ln -sf
            "Versions/Current/Headers"
            "$<TARGET_FILE_DIR:osd_static_framework>/../../Headers"
        )

        #shared framework
        if (BUILD_SHARED_LIBS)
            add_library(osd_dynamic_framework
                SHARED
                version.cpp
                $<TARGET_OBJECTS:sdc_obj>
                $<TARGET_OBJECTS:vtr_obj>
                $<TARGET_OBJECTS:far_obj>
                $<TARGET_OBJECTS:bfr_obj>
                $<TARGET_OBJECTS:osd_cpu_obj>
                $<TARGET_OBJECTS:osd_gpu_obj>
                ${OPENGL_LOADER_OBJS}
            )

            set_target_properties(
                osd_dynamic_framework
                PROPERTIES
                RPATH true
                FRAMEWORK true
                INSTALL_NAME_DIR "@rpath/OpenSubdiv.framework/OpenSubdiv"
                INSTALL_RPATH "@executable_path/Frameworks;@loader_path/Frameworks"
                OUTPUT_NAME OpenSubdiv
                EXPORT_NAME OpenSubdiv
                CLEAN_DIRECT_OUTPUT true
            )

            target_link_libraries(osd_dynamic_framework
                ${PLATFORM_CPU_LIBRARIES} ${PLATFORM_GPU_LIBRARIES}
            )

            install( TARGETS osd_dynamic_framework EXPORT opensubdiv-targets
                 FRAMEWORK DESTINATION "${CMAKE_LIBDIR_BASE}"
                 LIBRARY DESTINATION "${CMAKE_LIBDIR_BASE}"
                 PUBLIC_HEADER  DESTINATION "${CMAKE_INCDIR_BASE}"
                 PRIVATE_HEADER  DESTINATION "${CMAKE_INCDIR_BASE}"
                 )

            foreach(file ${PUBLIC_HEADER_FILES})
                add_custom_command(TARGET osd_dynamic_framework POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_SOURCE_DIR}/opensubdiv/${file}"
                    "$<TARGET_FILE_DIR:osd_dynamic_framework>/Headers/${file}"
                )
            endforeach(file)

            add_custom_command(TARGET osd_dynamic_framework POST_BUILD
                COMMAND ln -sf
                "Versions/Current/Headers"
                "$<TARGET_FILE_DIR:osd_dynamic_framework>/../../Headers"
            )
        endif()

    endif()

    # Expose preprocessor macro to the interface so that client know which feature are enabled
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.0)
        foreach(osd_target osd_static_cpu osd_static_gpu osd_dynamic_cpu osd_dynamic_gpu osd_static_framework osd_dynamic_framework)
            if(TARGET ${osd_target})
                if(CLEW_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_CLEW)
                endif()
                if(CUDA_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_CUDA)
                endif()
                if(DXSDK_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_DX11SDK)
                endif()
                if(METAL_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_METAL)
                endif()
                if(OPENCL_CL_D3D11_EXT_H_FOUND OR OPENCL_CL_D3D11_H_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_OPENCL_DX_INTEROP)
                endif()
                if(OPENCL_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_OPENCL)
                endif()
                if(OPENGL_4_2_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
                endif()
                if(OPENGL_4_3_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_GLSL_COMPUTE)
                endif()
                if(NOT NO_OPENGL AND OPENGL_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_OPENGL)
                endif()
                if(NOT NO_OPENGL AND OPENGLES_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_OPENGLES)
                endif()
                if(OPENMP_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_OPENMP)
                endif()
                if(OPENSUBDIV_GREGORY_EVAL_TRUE_DERIVATIVES)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_GREGORY_EVAL_TRUE_DERIVATIVES)
                endif()
                if(PTEX_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_PTEX)
                endif()
                if(TBB_FOUND)
                    target_compile_definitions(${osd_target} INTERFACE OPENSUBDIV_HAS_TBB)
                endif()
            endif()
        endforeach()
    endif()

endif()

#-------------------------------------------------------------------------------
