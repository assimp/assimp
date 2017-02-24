# This module can find the International Components for Unicode (ICU) libraries
#
# Requirements:
# - CMake >= 2.8.3 (for new version of find_package_handle_standard_args)
#
# The following variables will be defined for your use:
#   - ICU_FOUND             : were all of your specified components found?
#   - ICU_INCLUDE_DIRS      : ICU include directory
#   - ICU_LIBRARIES         : ICU libraries
#   - ICU_VERSION           : complete version of ICU (x.y.z)
#   - ICU_VERSION_MAJOR     : major version of ICU
#   - ICU_VERSION_MINOR     : minor version of ICU
#   - ICU_VERSION_PATCH     : patch version of ICU
#   - ICU_<COMPONENT>_FOUND : were <COMPONENT> found? (FALSE for non specified component if it is not a dependency)
#
# For windows or non standard installation, define ICU_ROOT_DIR variable to point to the root installation of ICU. Two ways:
#   - run cmake with -DICU_ROOT_DIR=<PATH>
#   - define an environment variable with the same name before running cmake
# With cmake-gui, before pressing "Configure":
#   1) Press "Add Entry" button
#   2) Add a new entry defined as:
#     - Name: ICU_ROOT_DIR
#     - Type: choose PATH in the selection list
#     - Press "..." button and select the root installation of ICU
#
# Example Usage:
#
#   1. Copy this file in the root of your project source directory
#   2. Then, tell CMake to search this non-standard module in your project directory by adding to your CMakeLists.txt:
#     set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
#   3. Finally call find_package() once, here are some examples to pick from
#
#   Require ICU 4.4 or later
#     find_package(ICU 4.4 REQUIRED)
#
#   if(ICU_FOUND)
#      add_executable(myapp myapp.c)
#      include_directories(${ICU_INCLUDE_DIRS})
#      target_link_libraries(myapp ${ICU_LIBRARIES})
#      # with CMake >= 3.0.0, the last two lines can be replaced by the following
#      target_link_libraries(myapp ICU::ICU)
#   endif(ICU_FOUND)

########## <ICU finding> ##########

find_package(PkgConfig QUIET)

########## Private ##########
if(NOT DEFINED ICU_PUBLIC_VAR_NS)
    set(ICU_PUBLIC_VAR_NS "ICU")                          # Prefix for all ICU relative public variables
endif(NOT DEFINED ICU_PUBLIC_VAR_NS)
if(NOT DEFINED ICU_PRIVATE_VAR_NS)
    set(ICU_PRIVATE_VAR_NS "_${ICU_PUBLIC_VAR_NS}")       # Prefix for all ICU relative internal variables
endif(NOT DEFINED ICU_PRIVATE_VAR_NS)
if(NOT DEFINED PC_ICU_PRIVATE_VAR_NS)
    set(PC_ICU_PRIVATE_VAR_NS "_PC${ICU_PRIVATE_VAR_NS}") # Prefix for all pkg-config relative internal variables
endif(NOT DEFINED PC_ICU_PRIVATE_VAR_NS)

set(${ICU_PRIVATE_VAR_NS}_HINTS )
# <deprecated>
# for future removal
if(DEFINED ENV{ICU_ROOT})
    list(APPEND ${ICU_PRIVATE_VAR_NS}_HINTS "$ENV{ICU_ROOT}")
    message(AUTHOR_WARNING "ENV{ICU_ROOT} is deprecated in favor of ENV{ICU_ROOT_DIR}")
endif(DEFINED ENV{ICU_ROOT})
if (DEFINED ICU_ROOT)
    list(APPEND ${ICU_PRIVATE_VAR_NS}_HINTS "${ICU_ROOT}")
    message(AUTHOR_WARNING "ICU_ROOT is deprecated in favor of ICU_ROOT_DIR")
endif(DEFINED ICU_ROOT)
# </deprecated>
if(DEFINED ENV{ICU_ROOT_DIR})
    list(APPEND ${ICU_PRIVATE_VAR_NS}_HINTS "$ENV{ICU_ROOT_DIR}")
endif(DEFINED ENV{ICU_ROOT_DIR})
if (DEFINED ICU_ROOT_DIR)
    list(APPEND ${ICU_PRIVATE_VAR_NS}_HINTS "${ICU_ROOT_DIR}")
endif(DEFINED ICU_ROOT_DIR)

set(${ICU_PRIVATE_VAR_NS}_COMPONENTS )
# <icu component name> <library name 1> ... <library name N>
macro(_icu_declare_component _NAME)
    list(APPEND ${ICU_PRIVATE_VAR_NS}_COMPONENTS ${_NAME})
    set("${ICU_PRIVATE_VAR_NS}_COMPONENTS_${_NAME}" ${ARGN})
endmacro(_icu_declare_component)

_icu_declare_component(data icudata)
_icu_declare_component(uc   icuuc)         # Common and Data libraries
_icu_declare_component(i18n icui18n icuin) # Internationalization library
_icu_declare_component(io   icuio ustdio)  # Stream and I/O Library
_icu_declare_component(le   icule)         # Layout library
_icu_declare_component(lx   iculx)         # Paragraph Layout library

########## Public ##########
set(${ICU_PUBLIC_VAR_NS}_FOUND FALSE)
set(${ICU_PUBLIC_VAR_NS}_LIBRARIES )
set(${ICU_PUBLIC_VAR_NS}_INCLUDE_DIRS )
set(${ICU_PUBLIC_VAR_NS}_C_FLAGS "")
set(${ICU_PUBLIC_VAR_NS}_CXX_FLAGS "")
set(${ICU_PUBLIC_VAR_NS}_CPP_FLAGS "")
set(${ICU_PUBLIC_VAR_NS}_C_SHARED_FLAGS "")
set(${ICU_PUBLIC_VAR_NS}_CXX_SHARED_FLAGS "")
set(${ICU_PUBLIC_VAR_NS}_CPP_SHARED_FLAGS "")

foreach(${ICU_PRIVATE_VAR_NS}_COMPONENT ${${ICU_PRIVATE_VAR_NS}_COMPONENTS})
    string(TOUPPER "${${ICU_PRIVATE_VAR_NS}_COMPONENT}" ${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT)
    set("${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_FOUND" FALSE) # may be done in the _icu_declare_component macro
endforeach(${ICU_PRIVATE_VAR_NS}_COMPONENT)

# Check components
if(NOT ${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS) # uc required at least
    set(${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS uc)
else(NOT ${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS)
    list(APPEND ${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS uc)
    list(REMOVE_DUPLICATES ${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS)
    foreach(${ICU_PRIVATE_VAR_NS}_COMPONENT ${${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS})
        if(NOT DEFINED ${ICU_PRIVATE_VAR_NS}_COMPONENTS_${${ICU_PRIVATE_VAR_NS}_COMPONENT})
            message(FATAL_ERROR "Unknown ICU component: ${${ICU_PRIVATE_VAR_NS}_COMPONENT}")
        endif(NOT DEFINED ${ICU_PRIVATE_VAR_NS}_COMPONENTS_${${ICU_PRIVATE_VAR_NS}_COMPONENT})
    endforeach(${ICU_PRIVATE_VAR_NS}_COMPONENT)
endif(NOT ${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS)

# if pkg-config is available check components dependencies and append `pkg-config icu-<component> --variable=prefix` to hints
if(PKG_CONFIG_FOUND)
    set(${ICU_PRIVATE_VAR_NS}_COMPONENTS_DUP ${${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS})
    foreach(${ICU_PRIVATE_VAR_NS}_COMPONENT ${${ICU_PRIVATE_VAR_NS}_COMPONENTS_DUP})
        pkg_check_modules(${PC_ICU_PRIVATE_VAR_NS} "icu-${${ICU_PRIVATE_VAR_NS}_COMPONENT}" QUIET)

        if(${PC_ICU_PRIVATE_VAR_NS}_FOUND)
            list(APPEND ${ICU_PRIVATE_VAR_NS}_HINTS ${${PC_ICU_PRIVATE_VAR_NS}_PREFIX})
            foreach(${PC_ICU_PRIVATE_VAR_NS}_LIBRARY ${${PC_ICU_PRIVATE_VAR_NS}_LIBRARIES})
                string(REGEX REPLACE "^icu" "" ${PC_ICU_PRIVATE_VAR_NS}_STRIPPED_LIBRARY ${${PC_ICU_PRIVATE_VAR_NS}_LIBRARY})
                if(NOT ${PC_ICU_PRIVATE_VAR_NS}_STRIPPED_LIBRARY STREQUAL "data")
                    list(FIND ${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS ${${PC_ICU_PRIVATE_VAR_NS}_STRIPPED_LIBRARY} ${ICU_PRIVATE_VAR_NS}_COMPONENT_INDEX)
                    if(${ICU_PRIVATE_VAR_NS}_COMPONENT_INDEX EQUAL -1)
                        message(WARNING "Missing component dependency: ${${PC_ICU_PRIVATE_VAR_NS}_STRIPPED_LIBRARY}. Add it to your find_package(ICU) line as COMPONENTS to fix this warning.")
                        list(APPEND ${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS ${${PC_ICU_PRIVATE_VAR_NS}_STRIPPED_LIBRARY})
                    endif(${ICU_PRIVATE_VAR_NS}_COMPONENT_INDEX EQUAL -1)
                endif(NOT ${PC_ICU_PRIVATE_VAR_NS}_STRIPPED_LIBRARY STREQUAL "data")
            endforeach(${PC_ICU_PRIVATE_VAR_NS}_LIBRARY)
        endif(${PC_ICU_PRIVATE_VAR_NS}_FOUND)
    endforeach(${ICU_PRIVATE_VAR_NS}_COMPONENT)
endif(PKG_CONFIG_FOUND)
# list(APPEND ${ICU_PRIVATE_VAR_NS}_HINTS ENV ICU_ROOT_DIR)
# message("${ICU_PRIVATE_VAR_NS}_HINTS = ${${ICU_PRIVATE_VAR_NS}_HINTS}")

# Includes
find_path(
    ${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR
    NAMES unicode/utypes.h utypes.h
    HINTS ${${ICU_PRIVATE_VAR_NS}_HINTS}
    PATH_SUFFIXES "include"
    DOC "Include directories for ICU"
)

if(${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR)
    ########## <part to keep synced with tests/version/CMakeLists.txt> ##########
    if(EXISTS "${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR}/unicode/uvernum.h") # ICU >= 4.4
        file(READ "${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR}/unicode/uvernum.h" ${ICU_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS)
    elseif(EXISTS "${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR}/unicode/uversion.h") # ICU [2;4.4[
        file(READ "${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR}/unicode/uversion.h" ${ICU_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS)
    elseif(EXISTS "${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR}/unicode/utypes.h") # ICU [1.4;2[
        file(READ "${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR}/unicode/utypes.h" ${ICU_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS)
    elseif(EXISTS "${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR}/utypes.h") # ICU 1.3
        file(READ "${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR}/utypes.h" ${ICU_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS)
    else()
        message(FATAL_ERROR "ICU version header not found")
    endif()

    if(${ICU_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS MATCHES ".*# *define *ICU_VERSION *\"([0-9]+)\".*") # ICU 1.3
        # [1.3;1.4[ as #define ICU_VERSION "3" (no patch version, ie all 1.3.X versions will be detected as 1.3.0)
        set(${ICU_PUBLIC_VAR_NS}_VERSION_MAJOR "1")
        set(${ICU_PUBLIC_VAR_NS}_VERSION_MINOR "${CMAKE_MATCH_1}")
        set(${ICU_PUBLIC_VAR_NS}_VERSION_PATCH "0")
    elseif(${ICU_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS MATCHES ".*# *define *U_ICU_VERSION_MAJOR_NUM *([0-9]+).*")
        #
        # Since version 4.9.1, ICU release version numbering was totaly changed, see:
        # - http://site.icu-project.org/download/49
        # - http://userguide.icu-project.org/design#TOC-Version-Numbers-in-ICU
        #
        set(${ICU_PUBLIC_VAR_NS}_VERSION_MAJOR "${CMAKE_MATCH_1}")
        string(REGEX REPLACE ".*# *define *U_ICU_VERSION_MINOR_NUM *([0-9]+).*" "\\1" ${ICU_PUBLIC_VAR_NS}_VERSION_MINOR "${${ICU_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS}")
        string(REGEX REPLACE ".*# *define *U_ICU_VERSION_PATCHLEVEL_NUM *([0-9]+).*" "\\1" ${ICU_PUBLIC_VAR_NS}_VERSION_PATCH "${${ICU_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS}")
    elseif(${ICU_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS MATCHES ".*# *define *U_ICU_VERSION *\"(([0-9]+)(\\.[0-9]+)*)\".*") # ICU [1.4;1.8[
        # [1.4;1.8[ as #define U_ICU_VERSION "1.4.1.2" but it seems that some 1.4.[12](?:\.\d)? have releasing error and appears as 1.4.0
        set(${ICU_PRIVATE_VAR_NS}_FULL_VERSION "${CMAKE_MATCH_1}") # copy CMAKE_MATCH_1, no longer valid on the following if
        if(${ICU_PRIVATE_VAR_NS}_FULL_VERSION MATCHES "^([0-9]+)\\.([0-9]+)$")
            set(${ICU_PUBLIC_VAR_NS}_VERSION_MAJOR "${CMAKE_MATCH_1}")
            set(${ICU_PUBLIC_VAR_NS}_VERSION_MINOR "${CMAKE_MATCH_2}")
            set(${ICU_PUBLIC_VAR_NS}_VERSION_PATCH "0")
        elseif(${ICU_PRIVATE_VAR_NS}_FULL_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
            set(${ICU_PUBLIC_VAR_NS}_VERSION_MAJOR "${CMAKE_MATCH_1}")
            set(${ICU_PUBLIC_VAR_NS}_VERSION_MINOR "${CMAKE_MATCH_2}")
            set(${ICU_PUBLIC_VAR_NS}_VERSION_PATCH "${CMAKE_MATCH_3}")
        endif()
    else()
        message(FATAL_ERROR "failed to detect ICU version")
    endif()
    set(${ICU_PUBLIC_VAR_NS}_VERSION "${${ICU_PUBLIC_VAR_NS}_VERSION_MAJOR}.${${ICU_PUBLIC_VAR_NS}_VERSION_MINOR}.${${ICU_PUBLIC_VAR_NS}_VERSION_PATCH}")
    ########## </part to keep synced with tests/version/CMakeLists.txt> ##########
endif(${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR)

# Check libraries
if(MSVC)
    include(SelectLibraryConfigurations)
endif(MSVC)
foreach(${ICU_PRIVATE_VAR_NS}_COMPONENT ${${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS})
    string(TOUPPER "${${ICU_PRIVATE_VAR_NS}_COMPONENT}" ${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT)
    if(MSVC)
        set(${ICU_PRIVATE_VAR_NS}_POSSIBLE_RELEASE_NAMES )
        set(${ICU_PRIVATE_VAR_NS}_POSSIBLE_DEBUG_NAMES )
        foreach(${ICU_PRIVATE_VAR_NS}_BASE_NAME ${${ICU_PRIVATE_VAR_NS}_COMPONENTS_${${ICU_PRIVATE_VAR_NS}_COMPONENT}})
            list(APPEND ${ICU_PRIVATE_VAR_NS}_POSSIBLE_RELEASE_NAMES "${${ICU_PRIVATE_VAR_NS}_BASE_NAME}")
            list(APPEND ${ICU_PRIVATE_VAR_NS}_POSSIBLE_DEBUG_NAMES "${${ICU_PRIVATE_VAR_NS}_BASE_NAME}d")
            list(APPEND ${ICU_PRIVATE_VAR_NS}_POSSIBLE_RELEASE_NAMES "${${ICU_PRIVATE_VAR_NS}_BASE_NAME}${${ICU_PUBLIC_VAR_NS}_VERSION_MAJOR}${${ICU_PUBLIC_VAR_NS}_VERSION_MINOR}")
            list(APPEND ${ICU_PRIVATE_VAR_NS}_POSSIBLE_DEBUG_NAMES "${${ICU_PRIVATE_VAR_NS}_BASE_NAME}${${ICU_PUBLIC_VAR_NS}_VERSION_MAJOR}${${ICU_PUBLIC_VAR_NS}_VERSION_MINOR}d")
        endforeach(${ICU_PRIVATE_VAR_NS}_BASE_NAME)

        find_library(
            ${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY_RELEASE
            NAMES ${${ICU_PRIVATE_VAR_NS}_POSSIBLE_RELEASE_NAMES}
            HINTS ${${ICU_PRIVATE_VAR_NS}_HINTS}
            DOC "Release library for ICU ${${ICU_PRIVATE_VAR_NS}_COMPONENT} component"
        )
        find_library(
            ${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY_DEBUG
            NAMES ${${ICU_PRIVATE_VAR_NS}_POSSIBLE_DEBUG_NAMES}
            HINTS ${${ICU_PRIVATE_VAR_NS}_HINTS}
            DOC "Debug library for ICU ${${ICU_PRIVATE_VAR_NS}_COMPONENT} component"
        )

        select_library_configurations("${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}")
        list(APPEND ${ICU_PUBLIC_VAR_NS}_LIBRARY ${${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY})
    else(MSVC)
        find_library(
            ${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY
            NAMES ${${ICU_PRIVATE_VAR_NS}_COMPONENTS_${${ICU_PRIVATE_VAR_NS}_COMPONENT}}
            PATHS ${${ICU_PRIVATE_VAR_NS}_HINTS}
            DOC "Library for ICU ${${ICU_PRIVATE_VAR_NS}_COMPONENT} component"
        )

        if(${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY)
            set("${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_FOUND" TRUE)
            list(APPEND ${ICU_PUBLIC_VAR_NS}_LIBRARY ${${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY})
        endif(${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY)
    endif(MSVC)
endforeach(${ICU_PRIVATE_VAR_NS}_COMPONENT)

# Try to find out compiler flags
find_program(${ICU_PUBLIC_VAR_NS}_CONFIG_EXECUTABLE icu-config HINTS ${${ICU_PRIVATE_VAR_NS}_HINTS})
if(${ICU_PUBLIC_VAR_NS}_CONFIG_EXECUTABLE)
    execute_process(COMMAND ${${ICU_PUBLIC_VAR_NS}_CONFIG_EXECUTABLE} --cflags OUTPUT_VARIABLE ${ICU_PUBLIC_VAR_NS}_C_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${${ICU_PUBLIC_VAR_NS}_CONFIG_EXECUTABLE} --cxxflags OUTPUT_VARIABLE ${ICU_PUBLIC_VAR_NS}_CXX_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${${ICU_PUBLIC_VAR_NS}_CONFIG_EXECUTABLE} --cppflags OUTPUT_VARIABLE ${ICU_PUBLIC_VAR_NS}_CPP_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND ${${ICU_PUBLIC_VAR_NS}_CONFIG_EXECUTABLE} --cflags-dynamic OUTPUT_VARIABLE ${ICU_PUBLIC_VAR_NS}_C_SHARED_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${${ICU_PUBLIC_VAR_NS}_CONFIG_EXECUTABLE} --cxxflags-dynamic OUTPUT_VARIABLE ${ICU_PUBLIC_VAR_NS}_CXX_SHARED_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${${ICU_PUBLIC_VAR_NS}_CONFIG_EXECUTABLE} --cppflags-dynamic OUTPUT_VARIABLE ${ICU_PUBLIC_VAR_NS}_CPP_SHARED_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(${ICU_PUBLIC_VAR_NS}_CONFIG_EXECUTABLE)

# Check find_package arguments
include(FindPackageHandleStandardArgs)
if(${ICU_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${ICU_PUBLIC_VAR_NS}_FIND_QUIETLY)
    find_package_handle_standard_args(
        ${ICU_PUBLIC_VAR_NS}
        REQUIRED_VARS ${ICU_PUBLIC_VAR_NS}_LIBRARY ${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR
        VERSION_VAR ${ICU_PUBLIC_VAR_NS}_VERSION
    )
else(${ICU_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${ICU_PUBLIC_VAR_NS}_FIND_QUIETLY)
    find_package_handle_standard_args(${ICU_PUBLIC_VAR_NS} "Could NOT find ICU" ${ICU_PUBLIC_VAR_NS}_LIBRARY ${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR)
endif(${ICU_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${ICU_PUBLIC_VAR_NS}_FIND_QUIETLY)

if(${ICU_PUBLIC_VAR_NS}_FOUND)
    # <deprecated>
    # for compatibility with previous versions, alias old ICU_(MAJOR|MINOR|PATCH)_VERSION to ICU_VERSION_$1
    set(${ICU_PUBLIC_VAR_NS}_MAJOR_VERSION ${${ICU_PUBLIC_VAR_NS}_VERSION_MAJOR})
    set(${ICU_PUBLIC_VAR_NS}_MINOR_VERSION ${${ICU_PUBLIC_VAR_NS}_VERSION_MINOR})
    set(${ICU_PUBLIC_VAR_NS}_PATCH_VERSION ${${ICU_PUBLIC_VAR_NS}_VERSION_PATCH})
    # </deprecated>
    set(${ICU_PUBLIC_VAR_NS}_LIBRARIES ${${ICU_PUBLIC_VAR_NS}_LIBRARY})
    set(${ICU_PUBLIC_VAR_NS}_INCLUDE_DIRS ${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR})

    if(NOT CMAKE_VERSION VERSION_LESS "3.0.0")
        if(NOT TARGET ICU::ICU)
            add_library(ICU::ICU INTERFACE IMPORTED)
        endif(NOT TARGET ICU::ICU)
        set_target_properties(ICU::ICU PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR}")
        foreach(${ICU_PRIVATE_VAR_NS}_COMPONENT ${${ICU_PUBLIC_VAR_NS}_FIND_COMPONENTS})
            string(TOUPPER "${${ICU_PRIVATE_VAR_NS}_COMPONENT}" ${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT)
            add_library("ICU::${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}" UNKNOWN IMPORTED)
            if(${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY_RELEASE)
                set_property(TARGET "ICU::${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}" APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
                set_target_properties("ICU::${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}" PROPERTIES IMPORTED_LOCATION_RELEASE "${${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY_RELEASE}")
            endif(${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY_RELEASE)
            if(${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY_DEBUG)
                set_property(TARGET "ICU::${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}" APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
                set_target_properties("ICU::${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}" PROPERTIES IMPORTED_LOCATION_DEBUG "${${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY_DEBUG}")
            endif(${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY_DEBUG)
            if(${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY)
                set_target_properties("ICU::${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}" PROPERTIES IMPORTED_LOCATION "${${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY}")
            endif(${ICU_PUBLIC_VAR_NS}_${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_LIBRARY)
            set_property(TARGET ICU::ICU APPEND PROPERTY INTERFACE_LINK_LIBRARIES "ICU::${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}")
#             set_target_properties("ICU::${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}" PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR}")
        endforeach(${ICU_PRIVATE_VAR_NS}_COMPONENT)
    endif(NOT CMAKE_VERSION VERSION_LESS "3.0.0")
endif(${ICU_PUBLIC_VAR_NS}_FOUND)

mark_as_advanced(
    ${ICU_PUBLIC_VAR_NS}_INCLUDE_DIR
    ${ICU_PUBLIC_VAR_NS}_LIBRARY
)

########## </ICU finding> ##########

########## <resource bundle support> ##########

########## Private ##########
function(_icu_extract_locale_from_rb _BUNDLE_SOURCE _RETURN_VAR_NAME)
    file(READ "${_BUNDLE_SOURCE}" _BUNDLE_CONTENTS)
    string(REGEX REPLACE "//[^\n]*\n" "" _BUNDLE_CONTENTS_WITHOUT_COMMENTS ${_BUNDLE_CONTENTS})
    string(REGEX REPLACE "[ \t\n]" "" _BUNDLE_CONTENTS_WITHOUT_COMMENTS_AND_SPACES ${_BUNDLE_CONTENTS_WITHOUT_COMMENTS})
    string(REGEX MATCH "^([a-zA-Z_-]+)(:table)?{" LOCALE_FOUND ${_BUNDLE_CONTENTS_WITHOUT_COMMENTS_AND_SPACES})
    set("${_RETURN_VAR_NAME}" "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction(_icu_extract_locale_from_rb)

########## Public ##########

#
# Prototype:
#   icu_generate_resource_bundle([NAME <name>] [PACKAGE] [DESTINATION <location>] [FILES <list of files>])
#
# Common arguments:
#   - NAME <name>                      : name of output package and to create dummy targets
#   - FILES <file 1> ... <file N>      : list of resource bundles sources
#   - DEPENDS <target1> ... <target N> : required to package as library (shared or static), a list of cmake parent targets to link to
#                                        Note: only (PREVIOUSLY DECLARED) add_executable and add_library as dependencies
#   - DESTINATION <location>           : optional, directory where to install final binary file(s)
#   - FORMAT <name>                    : optional, one of none (ICU4C binary format, default), java (plain java) or xliff (XML), see below
#
# Arguments depending on FORMAT:
#   - none (default):
#       * PACKAGE     : if present, package all resource bundles together. Default is to stop after building individual *.res files
#       * TYPE <name> : one of :
#           + common or archive (default) : archive all ressource bundles into a single .dat file
#           + library or dll              : assemble all ressource bundles into a separate and loadable library (.dll/.so)
#           + static                      : integrate all ressource bundles to targets designed by DEPENDS parameter (as a static library)
#       * NO_SHARED_FLAGS                 : only with TYPE in ['library', 'dll', 'static'], do not append ICU_C(XX)_SHARED_FLAGS to targets given as DEPENDS argument
#   - JAVA:
#       * BUNDLE <name> : required, prefix for generated classnames
#   - XLIFF:
#       (none)
#

#
# For an archive, the idea is to generate the following dependencies:
#
#   root.txt => root.res \
#                        |
#   en.txt   => en.res   |
#                        | => pkglist.txt => application.dat
#   fr.txt   => fr.res   |
#                        |
#   and so on            /
#
# Lengend: 'A => B' means B depends on A
#
# Steps (correspond to arrows):
#   1) genrb (from .txt to .res)
#   2) generate a file text (pkglist.txt) with all .res files to put together
#   3) build final archive (from *.res/pkglist.txt to .dat)
#

function(icu_generate_resource_bundle)

    ##### <check for pkgdata/genrb availability> #####
    find_program(${ICU_PUBLIC_VAR_NS}_GENRB_EXECUTABLE genrb HINTS ${${ICU_PRIVATE_VAR_NS}_HINTS})
    find_program(${ICU_PUBLIC_VAR_NS}_PKGDATA_EXECUTABLE pkgdata HINTS ${${ICU_PRIVATE_VAR_NS}_HINTS})

    if(NOT ${ICU_PUBLIC_VAR_NS}_GENRB_EXECUTABLE)
        message(FATAL_ERROR "genrb not found")
    endif(NOT ${ICU_PUBLIC_VAR_NS}_GENRB_EXECUTABLE)
    if(NOT ${ICU_PUBLIC_VAR_NS}_PKGDATA_EXECUTABLE)
        message(FATAL_ERROR "pkgdata not found")
    endif(NOT ${ICU_PUBLIC_VAR_NS}_PKGDATA_EXECUTABLE)
    ##### </check for pkgdata/genrb availability> #####

    ##### <constants> #####
    set(TARGET_SEPARATOR "+")
    set(__FUNCTION__ "icu_generate_resource_bundle")
    set(PACKAGE_TARGET_PREFIX "ICU${TARGET_SEPARATOR}PKG")
    set(RESOURCE_TARGET_PREFIX "ICU${TARGET_SEPARATOR}RB")
    ##### </constants> #####

    ##### <hash constants> #####
    # filename extension of built resource bundle (without dot)
    set(BUNDLES__SUFFIX "res")
    set(BUNDLES_JAVA_SUFFIX "java")
    set(BUNDLES_XLIFF_SUFFIX "xlf")
    # alias: none (default) = common = archive ; dll = library ; static
    set(PKGDATA__ALIAS "")
    set(PKGDATA_COMMON_ALIAS "")
    set(PKGDATA_ARCHIVE_ALIAS "")
    set(PKGDATA_DLL_ALIAS "LIBRARY")
    set(PKGDATA_LIBRARY_ALIAS "LIBRARY")
    set(PKGDATA_STATIC_ALIAS "STATIC")
    # filename prefix of built package
    set(PKGDATA__PREFIX "")
    set(PKGDATA_LIBRARY_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")
    set(PKGDATA_STATIC_PREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}")
    # filename extension of built package (with dot)
    set(PKGDATA__SUFFIX ".dat")
    set(PKGDATA_LIBRARY_SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(PKGDATA_STATIC_SUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")
    # pkgdata option mode specific
    set(PKGDATA__OPTIONS "-m" "common")
    set(PKGDATA_STATIC_OPTIONS "-m" "static")
    set(PKGDATA_LIBRARY_OPTIONS "-m" "library")
    # cmake library type for output package
    set(PKGDATA_LIBRARY__TYPE "")
    set(PKGDATA_LIBRARY_STATIC_TYPE STATIC)
    set(PKGDATA_LIBRARY_LIBRARY_TYPE SHARED)
    ##### </hash constants> #####

    include(CMakeParseArguments)
    cmake_parse_arguments(
        PARSED_ARGS # output variable name
        # options (true/false) (default value: false)
        "PACKAGE;NO_SHARED_FLAGS"
        # univalued parameters (default value: "")
        "NAME;DESTINATION;TYPE;FORMAT;BUNDLE"
        # multivalued parameters (default value: "")
        "FILES;DEPENDS"
        ${ARGN}
    )

    # assert(${PARSED_ARGS_NAME} != "")
    if(NOT PARSED_ARGS_NAME)
        message(FATAL_ERROR "${__FUNCTION__}(): no name given, NAME parameter missing")
    endif(NOT PARSED_ARGS_NAME)

    # assert(length(PARSED_ARGS_FILES) > 0)
    list(LENGTH PARSED_ARGS_FILES PARSED_ARGS_FILES_LEN)
    if(PARSED_ARGS_FILES_LEN LESS 1)
        message(FATAL_ERROR "${__FUNCTION__}() expects at least 1 resource bundle as FILES argument, 0 given")
    endif(PARSED_ARGS_FILES_LEN LESS 1)

    string(TOUPPER "${PARSED_ARGS_FORMAT}" UPPER_FORMAT)
    # assert(${UPPER_FORMAT} in ['', 'java', 'xlif'])
    if(NOT DEFINED BUNDLES_${UPPER_FORMAT}_SUFFIX)
        message(FATAL_ERROR "${__FUNCTION__}(): unknown FORMAT '${PARSED_ARGS_FORMAT}'")
    endif(NOT DEFINED BUNDLES_${UPPER_FORMAT}_SUFFIX)

    if(UPPER_FORMAT STREQUAL "JAVA")
        # assert(${PARSED_ARGS_BUNDLE} != "")
        if(NOT PARSED_ARGS_BUNDLE)
            message(FATAL_ERROR "${__FUNCTION__}(): java bundle name expected, BUNDLE parameter missing")
        endif(NOT PARSED_ARGS_BUNDLE)
    endif(UPPER_FORMAT STREQUAL "JAVA")

    if(PARSED_ARGS_PACKAGE)
        # assert(${PARSED_ARGS_FORMAT} == "")
        if(PARSED_ARGS_FORMAT)
            message(FATAL_ERROR "${__FUNCTION__}(): packaging is only supported for binary format, not xlif neither java outputs")
        endif(PARSED_ARGS_FORMAT)

        string(TOUPPER "${PARSED_ARGS_TYPE}" UPPER_MODE)
        # assert(${UPPER_MODE} in ['', 'common', 'archive', 'dll', library'])
        if(NOT DEFINED PKGDATA_${UPPER_MODE}_ALIAS)
            message(FATAL_ERROR "${__FUNCTION__}(): unknown TYPE '${PARSED_ARGS_TYPE}'")
        else(NOT DEFINED PKGDATA_${UPPER_MODE}_ALIAS)
            set(TYPE "${PKGDATA_${UPPER_MODE}_ALIAS}")
        endif(NOT DEFINED PKGDATA_${UPPER_MODE}_ALIAS)

        # Package name: strip file extension if present
        get_filename_component(PACKAGE_NAME_WE ${PARSED_ARGS_NAME} NAME_WE)
        # Target name to build package
        set(PACKAGE_TARGET_NAME "${PACKAGE_TARGET_PREFIX}${TARGET_SEPARATOR}${PACKAGE_NAME_WE}")
        # Target name to build intermediate list file
        set(PACKAGE_LIST_TARGET_NAME "${PACKAGE_TARGET_NAME}${TARGET_SEPARATOR}PKGLIST")
        # Directory (absolute) to set as "current directory" for genrb (does not include package directory, -p)
        # We make our "cook" there to prevent any conflict
        if(DEFINED CMAKE_PLATFORM_ROOT_BIN) # CMake < 2.8.10
            set(RESOURCE_GENRB_CHDIR_DIR "${CMAKE_PLATFORM_ROOT_BIN}/${PACKAGE_TARGET_NAME}.dir/")
        else(DEFINED CMAKE_PLATFORM_ROOT_BIN) # CMake >= 2.8.10
            set(RESOURCE_GENRB_CHDIR_DIR "${CMAKE_PLATFORM_INFO_DIR}/${PACKAGE_TARGET_NAME}.dir/")
        endif(DEFINED CMAKE_PLATFORM_ROOT_BIN)
        # Directory (absolute) where resource bundles are built: concatenation of RESOURCE_GENRB_CHDIR_DIR and package name
        set(RESOURCE_OUTPUT_DIR "${RESOURCE_GENRB_CHDIR_DIR}/${PACKAGE_NAME_WE}/")
        # Output (relative) path for built package
        if(MSVC AND TYPE STREQUAL PKGDATA_LIBRARY_ALIAS)
            set(PACKAGE_OUTPUT_PATH "${RESOURCE_GENRB_CHDIR_DIR}/${PACKAGE_NAME_WE}/${PKGDATA_${TYPE}_PREFIX}${PACKAGE_NAME_WE}${PKGDATA_${TYPE}_SUFFIX}")
        else(MSVC AND TYPE STREQUAL PKGDATA_LIBRARY_ALIAS)
            set(PACKAGE_OUTPUT_PATH "${RESOURCE_GENRB_CHDIR_DIR}/${PKGDATA_${TYPE}_PREFIX}${PACKAGE_NAME_WE}${PKGDATA_${TYPE}_SUFFIX}")
        endif(MSVC AND TYPE STREQUAL PKGDATA_LIBRARY_ALIAS)
        # Output (absolute) path for the list file
        set(PACKAGE_LIST_OUTPUT_PATH "${RESOURCE_GENRB_CHDIR_DIR}/pkglist.txt")

        file(MAKE_DIRECTORY "${RESOURCE_OUTPUT_DIR}")
    else(PARSED_ARGS_PACKAGE)
        set(RESOURCE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/")
#         set(RESOURCE_GENRB_CHDIR_DIR "UNUSED")
    endif(PARSED_ARGS_PACKAGE)

    set(TARGET_RESOURCES )
    set(COMPILED_RESOURCES_PATH )
    set(COMPILED_RESOURCES_BASENAME )
    foreach(RESOURCE_SOURCE ${PARSED_ARGS_FILES})
        _icu_extract_locale_from_rb(${RESOURCE_SOURCE} RESOURCE_NAME_WE)
        get_filename_component(SOURCE_BASENAME ${RESOURCE_SOURCE} NAME)
        get_filename_component(ABSOLUTE_SOURCE ${RESOURCE_SOURCE} ABSOLUTE)

        if(UPPER_FORMAT STREQUAL "XLIFF")
            if(RESOURCE_NAME_WE STREQUAL "root")
                set(XLIFF_LANGUAGE "en")
            else(RESOURCE_NAME_WE STREQUAL "root")
                string(REGEX REPLACE "[^a-z].*$" "" XLIFF_LANGUAGE "${RESOURCE_NAME_WE}")
            endif(RESOURCE_NAME_WE STREQUAL "root")
        endif(UPPER_FORMAT STREQUAL "XLIFF")

        ##### <templates> #####
        set(RESOURCE_TARGET_NAME "${RESOURCE_TARGET_PREFIX}${TARGET_SEPARATOR}${PARSED_ARGS_NAME}${TARGET_SEPARATOR}${RESOURCE_NAME_WE}")

        set(RESOURCE_OUTPUT__PATH "${RESOURCE_NAME_WE}.res")
        if(RESOURCE_NAME_WE STREQUAL "root")
            set(RESOURCE_OUTPUT_JAVA_PATH "${PARSED_ARGS_BUNDLE}.java")
        else(RESOURCE_NAME_WE STREQUAL "root")
            set(RESOURCE_OUTPUT_JAVA_PATH "${PARSED_ARGS_BUNDLE}_${RESOURCE_NAME_WE}.java")
        endif(RESOURCE_NAME_WE STREQUAL "root")
        set(RESOURCE_OUTPUT_XLIFF_PATH "${RESOURCE_NAME_WE}.xlf")

        set(GENRB__OPTIONS "")
        set(GENRB_JAVA_OPTIONS "-j" "-b" "${PARSED_ARGS_BUNDLE}")
        set(GENRB_XLIFF_OPTIONS "-x" "-l" "${XLIFF_LANGUAGE}")
        ##### </templates> #####

        # build <locale>.txt from <locale>.res
        if(PARSED_ARGS_PACKAGE)
            add_custom_command(
                OUTPUT "${RESOURCE_OUTPUT_DIR}${RESOURCE_OUTPUT_${UPPER_FORMAT}_PATH}"
                COMMAND ${CMAKE_COMMAND} -E chdir ${RESOURCE_GENRB_CHDIR_DIR} ${${ICU_PUBLIC_VAR_NS}_GENRB_EXECUTABLE} ${GENRB_${UPPER_FORMAT}_OPTIONS} -d ${PACKAGE_NAME_WE} ${ABSOLUTE_SOURCE}
                DEPENDS ${RESOURCE_SOURCE}
            )
        else(PARSED_ARGS_PACKAGE)
            add_custom_command(
                OUTPUT "${RESOURCE_OUTPUT_DIR}${RESOURCE_OUTPUT_${UPPER_FORMAT}_PATH}"
                COMMAND ${${ICU_PUBLIC_VAR_NS}_GENRB_EXECUTABLE} ${GENRB_${UPPER_FORMAT}_OPTIONS} -d ${RESOURCE_OUTPUT_DIR} ${ABSOLUTE_SOURCE}
                DEPENDS ${RESOURCE_SOURCE}
            )
        endif(PARSED_ARGS_PACKAGE)
        # dummy target (ICU+RB+<name>+<locale>) for each locale to build the <locale>.res file from its <locale>.txt by the add_custom_command above
        add_custom_target(
            "${RESOURCE_TARGET_NAME}" ALL
            COMMENT ""
            DEPENDS "${RESOURCE_OUTPUT_DIR}${RESOURCE_OUTPUT_${UPPER_FORMAT}_PATH}"
            SOURCES ${RESOURCE_SOURCE}
        )

        if(PARSED_ARGS_DESTINATION AND NOT PARSED_ARGS_PACKAGE)
            install(FILES "${RESOURCE_OUTPUT_DIR}${RESOURCE_OUTPUT_${UPPER_FORMAT}_PATH}" DESTINATION ${PARSED_ARGS_DESTINATION} PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
        endif(PARSED_ARGS_DESTINATION AND NOT PARSED_ARGS_PACKAGE)

        list(APPEND TARGET_RESOURCES "${RESOURCE_TARGET_NAME}")
        list(APPEND COMPILED_RESOURCES_PATH "${RESOURCE_OUTPUT_DIR}${RESOURCE_OUTPUT_${UPPER_FORMAT}_PATH}")
        list(APPEND COMPILED_RESOURCES_BASENAME "${RESOURCE_NAME_WE}.${BUNDLES_${UPPER_FORMAT}_SUFFIX}")
    endforeach(RESOURCE_SOURCE)
    # convert semicolon separated list to a space separated list
    # NOTE: if the pkglist.txt file starts (or ends?) with a whitespace, pkgdata add an undefined symbol (named <package>_) for it
    string(REPLACE ";" " " COMPILED_RESOURCES_BASENAME "${COMPILED_RESOURCES_BASENAME}")

    if(PARSED_ARGS_PACKAGE)
        # create a text file (pkglist.txt) with the list of the *.res to package together
        add_custom_command(
            OUTPUT "${PACKAGE_LIST_OUTPUT_PATH}"
            COMMAND ${CMAKE_COMMAND} -E echo "${COMPILED_RESOURCES_BASENAME}" > "${PACKAGE_LIST_OUTPUT_PATH}"
            DEPENDS ${COMPILED_RESOURCES_PATH}
        )
        # run pkgdata from pkglist.txt
        add_custom_command(
            OUTPUT "${PACKAGE_OUTPUT_PATH}"
            COMMAND ${CMAKE_COMMAND} -E chdir ${RESOURCE_GENRB_CHDIR_DIR} ${${ICU_PUBLIC_VAR_NS}_PKGDATA_EXECUTABLE} -F ${PKGDATA_${TYPE}_OPTIONS} -s ${PACKAGE_NAME_WE} -p ${PACKAGE_NAME_WE} ${PACKAGE_LIST_OUTPUT_PATH}
            DEPENDS "${PACKAGE_LIST_OUTPUT_PATH}"
            VERBATIM
        )
        if(PKGDATA_LIBRARY_${TYPE}_TYPE)
            # assert(${PARSED_ARGS_DEPENDS} != "")
            if(NOT PARSED_ARGS_DEPENDS)
                message(FATAL_ERROR "${__FUNCTION__}(): static and library mode imply a list of targets to link to, DEPENDS parameter missing")
            endif(NOT PARSED_ARGS_DEPENDS)
            add_library(${PACKAGE_TARGET_NAME} ${PKGDATA_LIBRARY_${TYPE}_TYPE} IMPORTED)
            if(MSVC)
                string(REGEX REPLACE "${PKGDATA_LIBRARY_SUFFIX}\$" "${CMAKE_IMPORT_LIBRARY_SUFFIX}" PACKAGE_OUTPUT_LIB "${PACKAGE_OUTPUT_PATH}")
                set_target_properties(${PACKAGE_TARGET_NAME} PROPERTIES IMPORTED_LOCATION ${PACKAGE_OUTPUT_PATH} IMPORTED_IMPLIB ${PACKAGE_OUTPUT_LIB})
            else(MSVC)
                set_target_properties(${PACKAGE_TARGET_NAME} PROPERTIES IMPORTED_LOCATION ${PACKAGE_OUTPUT_PATH})
            endif(MSVC)
            foreach(DEPENDENCY ${PARSED_ARGS_DEPENDS})
                target_link_libraries(${DEPENDENCY} ${PACKAGE_TARGET_NAME})
                if(NOT PARSED_ARGS_NO_SHARED_FLAGS)
                    get_property(ENABLED_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
                    list(LENGTH "${ENABLED_LANGUAGES}" ENABLED_LANGUAGES_LENGTH)
                    if(ENABLED_LANGUAGES_LENGTH GREATER 1)
                        message(WARNING "Project has more than one language enabled, skip automatic shared flags appending")
                    else(ENABLED_LANGUAGES_LENGTH GREATER 1)
                        set_property(TARGET "${DEPENDENCY}" APPEND PROPERTY COMPILE_FLAGS "${${ICU_PUBLIC_VAR_NS}_${ENABLED_LANGUAGES}_SHARED_FLAGS}")
                    endif(ENABLED_LANGUAGES_LENGTH GREATER 1)
                endif(NOT PARSED_ARGS_NO_SHARED_FLAGS)
            endforeach(DEPENDENCY)
            # http://www.mail-archive.com/cmake-commits@cmake.org/msg01135.html
            set(PACKAGE_INTERMEDIATE_TARGET_NAME "${PACKAGE_TARGET_NAME}${TARGET_SEPARATOR}DUMMY")
            # dummy intermediate target (ICU+PKG+<name>+DUMMY) to link the package to the produced library by running pkgdata (see add_custom_command above)
            add_custom_target(
                ${PACKAGE_INTERMEDIATE_TARGET_NAME}
                COMMENT ""
                DEPENDS "${PACKAGE_OUTPUT_PATH}"
            )
            add_dependencies("${PACKAGE_TARGET_NAME}" "${PACKAGE_INTERMEDIATE_TARGET_NAME}")
        else(PKGDATA_LIBRARY_${TYPE}_TYPE)
            # dummy target (ICU+PKG+<name>) to run pkgdata (see add_custom_command above)
            add_custom_target(
                "${PACKAGE_TARGET_NAME}" ALL
                COMMENT ""
                DEPENDS "${PACKAGE_OUTPUT_PATH}"
            )
        endif(PKGDATA_LIBRARY_${TYPE}_TYPE)
        # dummy target (ICU+PKG+<name>+PKGLIST) to build the file pkglist.txt
        add_custom_target(
            "${PACKAGE_LIST_TARGET_NAME}" ALL
            COMMENT ""
            DEPENDS "${PACKAGE_LIST_OUTPUT_PATH}"
        )
        # package => pkglist.txt
        add_dependencies("${PACKAGE_TARGET_NAME}" "${PACKAGE_LIST_TARGET_NAME}")
        # pkglist.txt => *.res
        add_dependencies("${PACKAGE_LIST_TARGET_NAME}" ${TARGET_RESOURCES})

        if(PARSED_ARGS_DESTINATION)
            install(FILES "${PACKAGE_OUTPUT_PATH}" DESTINATION ${PARSED_ARGS_DESTINATION} PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
        endif(PARSED_ARGS_DESTINATION)
    endif(PARSED_ARGS_PACKAGE)

endfunction(icu_generate_resource_bundle)

########## </resource bundle support> ##########

########## <debug> ##########

if(${ICU_PUBLIC_VAR_NS}_DEBUG)

    function(icudebug _VARNAME)
        if(DEFINED ${ICU_PUBLIC_VAR_NS}_${_VARNAME})
            message("${ICU_PUBLIC_VAR_NS}_${_VARNAME} = ${${ICU_PUBLIC_VAR_NS}_${_VARNAME}}")
        else(DEFINED ${ICU_PUBLIC_VAR_NS}_${_VARNAME})
            message("${ICU_PUBLIC_VAR_NS}_${_VARNAME} = <UNDEFINED>")
        endif(DEFINED ${ICU_PUBLIC_VAR_NS}_${_VARNAME})
    endfunction(icudebug)

    # IN (args)
    icudebug("FIND_COMPONENTS")
    icudebug("FIND_REQUIRED")
    icudebug("FIND_QUIETLY")
    icudebug("FIND_VERSION")

    # OUT
    # Found
    icudebug("FOUND")
    # Flags
    icudebug("C_FLAGS")
    icudebug("CPP_FLAGS")
    icudebug("CXX_FLAGS")
    icudebug("C_SHARED_FLAGS")
    icudebug("CPP_SHARED_FLAGS")
    icudebug("CXX_SHARED_FLAGS")
    # Linking
    icudebug("INCLUDE_DIRS")
    icudebug("LIBRARIES")
    # Version
    icudebug("VERSION_MAJOR")
    icudebug("VERSION_MINOR")
    icudebug("VERSION_PATCH")
    icudebug("VERSION")
    # <COMPONENT>_(FOUND|LIBRARY)
    set(${ICU_PRIVATE_VAR_NS}_COMPONENT_VARIABLES "FOUND" "LIBRARY" "LIBRARY_RELEASE" "LIBRARY_DEBUG")
    foreach(${ICU_PRIVATE_VAR_NS}_COMPONENT ${${ICU_PRIVATE_VAR_NS}_COMPONENTS})
        string(TOUPPER "${${ICU_PRIVATE_VAR_NS}_COMPONENT}" ${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT)
        foreach(${ICU_PRIVATE_VAR_NS}_COMPONENT_VARIABLE ${${ICU_PRIVATE_VAR_NS}_COMPONENT_VARIABLES})
            icudebug("${${ICU_PRIVATE_VAR_NS}_UPPER_COMPONENT}_${${ICU_PRIVATE_VAR_NS}_COMPONENT_VARIABLE}")
        endforeach(${ICU_PRIVATE_VAR_NS}_COMPONENT_VARIABLE)
    endforeach(${ICU_PRIVATE_VAR_NS}_COMPONENT)

endif(${ICU_PUBLIC_VAR_NS}_DEBUG)

########## </debug> ##########
