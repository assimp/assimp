cmake_minimum_required( VERSION 2.6 )

INCLUDE_DIRECTORIES(
	${Assimp_SOURCE_DIR}/include
	${Assimp_SOURCE_DIR}/code
)

LINK_DIRECTORIES( ${Assimp_BINARY_DIR} ${Assimp_BINARY_DIR}/lib )

ADD_EXECUTABLE( assimp_cmd
	assimp_cmd.rc
	CompareDump.cpp
	ImageExtractor.cpp
	Main.cpp
	Main.h
	resource.h
	WriteDumb.cpp
	Info.cpp
	Export.cpp
)

SET_PROPERTY(TARGET assimp_cmd PROPERTY DEBUG_POSTFIX ${ASSIMP_DEBUG_POSTFIX})

IF( WIN32 )
	ADD_CUSTOM_COMMAND(TARGET assimp_cmd
		PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:assimp> $<TARGET_FILE_DIR:assimp_cmd>
		MAIN_DEPENDENCY assimp)
ENDIF( WIN32 )

TARGET_LINK_LIBRARIES( assimp_cmd assimp ${ZLIB_LIBRARIES})
SET_TARGET_PROPERTIES( assimp_cmd PROPERTIES
	OUTPUT_NAME assimp
)

INSTALL( TARGETS assimp_cmd
	DESTINATION "${ASSIMP_BIN_INSTALL_DIR}" COMPONENT assimp-bin
) 
