INCLUDE_DIRECTORIES(
	${Assimp_SOURCE_DIR}/include
	${Assimp_SOURCE_DIR}/code
	${Assimp_SOURCE_DIR}/samples/glut/
)

LINK_DIRECTORIES( 
	${Assimp_BINARY_DIR} 
	${Assimp_BINARY_DIR}/lib 
	${Assimp_SOURCE_DIR}/samples/glut/
)

ADD_EXECUTABLE( assimp_simpleogl
	Sample_SimpleOpenGL.c
)

TARGET_LINK_LIBRARIES( assimp_simpleogl assimp )
SET_TARGET_PROPERTIES( assimp_simpleogl PROPERTIES
	OUTPUT_NAME assimp_simpleogl
)

INSTALL( TARGETS assimp_simpleogl
	DESTINATION "${BIN_INSTALL_DIR}"
) 
