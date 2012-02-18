INCLUDE_DIRECTORIES(
	${Assimp_SOURCE_DIR}/include
	${Assimp_SOURCE_DIR}/code
	${Assimp_SOURCE_DIR}/samples/glut/
	${Assimp_SOURCE_DIR}/samples/DevIL/include/
)

LINK_DIRECTORIES( 
	${Assimp_BINARY_DIR} 
	${Assimp_BINARY_DIR}/lib/
	${Assimp_SOURCE_DIR}/samples/glut/
	${Assimp_SOURCE_DIR}/samples/DevIL/lib/
)

ADD_EXECUTABLE( assimp_simpletexturedogl WIN32
	SimpleTexturedOpenGL/include/boost_includes.h
	SimpleTexturedOpenGL/src/model_loading.cpp
)

SET_PROPERTY(TARGET assimp_simpletexturedogl PROPERTY DEBUG_POSTFIX ${DEBUG_POSTFIX})

IF( WIN32 )
	SET( PSDK_PATH "C:/Program Files/Microsoft Platform SDK/Bin" )
	SET( PSDK_INC "C:/Program Files/Microsoft Platform SDK/Include" )
	
	FIND_LIBRARY( WIN32_COMCTRL comctl32.lib 
		PATHS 
			"C:/Programme/Microsoft Platform SDK for Windows Server 2003 R2/Lib" 	
		DOC "Path to psdk"
	) 
ENDIF( WIN32 )


TARGET_LINK_LIBRARIES( assimp_simpletexturedogl assimp opengl32.lib glu32.lib comctl32.lib devil.lib )

SET_TARGET_PROPERTIES( assimp_simpletexturedogl PROPERTIES
	OUTPUT_NAME assimp_simpletexturedogl
)

INSTALL( TARGETS assimp_simpletexturedogl
	DESTINATION "${BIN_INSTALL_DIR}" COMPONENT assimp-dev
) 
