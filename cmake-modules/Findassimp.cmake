if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(ASSIMP_ARCHITECTURE "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(ASSIMP_ARCHITECTURE "32")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

if(WIN32)
	set(ASSIMP_ROOT_DIR CACHE PATH "ASSIMP root directory")

	# Find path of each library
	find_path(ASSIMP_INCLUDE_DIR
		NAMES
			assimp/anim.h
		HINTS
			${ASSIMP_ROOT_DIR}/include
	)

	if(MSVC12)
		set(ASSIMP_MSVC_VERSION "vc120")
	elseif(MSVC14)
		set(ASSIMP_MSVC_VERSION "vc140")
	endif(MSVC12)

	if(MSVC12 OR MSVC14)

		find_path(ASSIMP_LIBRARY_DIR
			NAMES
				assimp-${ASSIMP_MSVC_VERSION}-mt.lib
			HINTS
				${ASSIMP_ROOT_DIR}/lib${ASSIMP_ARCHITECTURE}
		)

		find_library(ASSIMP_LIBRARY_RELEASE				assimp-${ASSIMP_MSVC_VERSION}-mt.lib 			PATHS ${ASSIMP_LIBRARY_DIR})
		find_library(ASSIMP_LIBRARY_DEBUG				assimp-${ASSIMP_MSVC_VERSION}-mtd.lib			PATHS ${ASSIMP_LIBRARY_DIR})

		set(ASSIMP_LIBRARY
			optimized 	${ASSIMP_LIBRARY_RELEASE}
			debug		${ASSIMP_LIBRARY_DEBUG}
		)

		set(ASSIMP_LIBRARIES "ASSIMP_LIBRARY_RELEASE" "ASSIMP_LIBRARY_DEBUG")

		FUNCTION(ASSIMP_COPY_BINARIES TargetDirectory)
			ADD_CUSTOM_TARGET(AssimpCopyBinaries
				COMMAND ${CMAKE_COMMAND} -E copy ${ASSIMP_ROOT_DIR}/bin${ASSIMP_ARCHITECTURE}/assimp-${ASSIMP_MSVC_VERSION}-mtd.dll 	${TargetDirectory}/Debug/assimp-${ASSIMP_MSVC_VERSION}-mtd.dll
				COMMAND ${CMAKE_COMMAND} -E copy ${ASSIMP_ROOT_DIR}/bin${ASSIMP_ARCHITECTURE}/assimp-${ASSIMP_MSVC_VERSION}-mt.dll 		${TargetDirectory}/Release/assimp-${ASSIMP_MSVC_VERSION}-mt.dll
			COMMENT "Copying Assimp binaries to '${TargetDirectory}'"
			VERBATIM)
		ENDFUNCTION(ASSIMP_COPY_BINARIES)

		if (NOT TARGET ASSIMP)
			set(INCLUDE_DIRS ${ASSIMP_ROOT_DIR}/include)

			find_library(ASSIMP_LIB_DEBUG
				NAMES assimp-${ASSIMP_MSVC_VERSION}-mtd.lib
				PATHS ${ASSIMP_LIBRARY_DIR})

			find_file(ASSIMP_DLL_DEBUG
				NAMES assimp-${ASSIMP_MSVC_VERSION}-mtd.dll
				PATHS ${ASSIMP_ROOT_DIR}/bin${ASSIMP_ARCHITECTURE})

			find_library(ASSIMP_LIB_RELEASE
				NAMES assimp-${ASSIMP_MSVC_VERSION}-mt.lib
				PATHS ${ASSIMP_LIBRARY_DIR})

			find_file(ASSIMP_DLL_RELEASE
				NAMES assimp-${ASSIMP_MSVC_VERSION}-mt.dll
				PATHS ${ASSIMP_ROOT_DIR}/bin${ASSIMP_ARCHITECTURE})

			add_library(ASSIMP SHARED IMPORTED)
			set_target_properties(ASSIMP PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${INCLUDE_DIRS}"
				IMPORTED_IMPLIB_DEBUG ${ASSIMP_LIB_DEBUG}
				IMPORTED_IMPLIB_RELEASE ${ASSIMP_LIB_RELEASE}
				IMPORTED_LOCATION_DEBUG ${ASSIMP_DLL_DEBUG}
				IMPORTED_LOCATION_RELEASE ${ASSIMP_DLL_RELEASE}
			)
		endif()
	endif()

else(WIN32)

	find_path(
	  assimp_INCLUDE_DIRS
	  NAMES assimp/postprocess.h assimp/scene.h assimp/version.h assimp/config.h assimp/cimport.h
	  PATHS /usr/local/include
	  PATHS /usr/include/

	)

	find_library(
	  assimp_LIBRARIES
	  NAMES assimp
	  PATHS /usr/local/lib/
	  PATHS /usr/lib64/
	  PATHS /usr/lib/
	)

	if (assimp_INCLUDE_DIRS AND assimp_LIBRARIES)
	  SET(assimp_FOUND TRUE)
	ENDIF (assimp_INCLUDE_DIRS AND assimp_LIBRARIES)

	if (assimp_FOUND)
	  if (NOT assimp_FIND_QUIETLY)
		message(STATUS "Found asset importer library: ${assimp_LIBRARIES}")
	  endif (NOT assimp_FIND_QUIETLY)
	else (assimp_FOUND)
	  if (assimp_FIND_REQUIRED)
		message(FATAL_ERROR "Could not find asset importer library")
	  endif (assimp_FIND_REQUIRED)
	endif (assimp_FOUND)

endif(WIN32)
