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

SET_PROPERTY(TARGET assimp_cmd PROPERTY DEBUG_POSTFIX ${DEBUG_POSTFIX})

TARGET_LINK_LIBRARIES( assimp_cmd assimp ${ZLIB_LIBRARIES})
SET_TARGET_PROPERTIES( assimp_cmd PROPERTIES
	OUTPUT_NAME assimp
)

INSTALL( TARGETS assimp_cmd
	DESTINATION "${BIN_INSTALL_DIR}" COMPONENT assimp-bin
) 
