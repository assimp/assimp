FIND_PATH(
	assimp_INCLUDE_DIRS
	NAMES postprocess.h scene.h version.h config.h cimport.h
	PATHS /usr/local/include/
)

FIND_LIBRARY(
	assimp_LIBRARIES
	NAMES assimp
	PATHS /usr/local/lib/
)

IF (assimp_INCLUDE_DIRS AND assimp_LIBRARIES)
    SET(assimp_FOUND TRUE)
ENDIF (assimp_INCLUDE_DIRS AND assimp_LIBRARIES)

IF (assimp_FOUND)
    IF (NOT assimp_FIND_QUIETLY)
        MESSAGE(STATUS "Found asset importer library: ${assimp_LIBRARIES}")
    ENDIF (NOT assimp_FIND_QUIETLY)
ELSE (assimp_FOUND)
    IF (assimp_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find asset importer library")
    ENDIF (assimp_FIND_REQUIRED)
ENDIF (assimp_FOUND)
