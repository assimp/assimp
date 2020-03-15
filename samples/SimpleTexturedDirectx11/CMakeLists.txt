FIND_PACKAGE(DirectX)

IF ( MSVC )
  SET(M_LIB)
ENDIF ()

if ( MSVC )
  ADD_DEFINITIONS( -D_SCL_SECURE_NO_WARNINGS )
  ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
  REMOVE_DEFINITIONS( -DUNICODE -D_UNICODE )
endif ()

ADD_COMPILE_DEFINITIONS(SHADER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/SimpleTexturedDirectx11/")

INCLUDE_DIRECTORIES(
  ${Assimp_SOURCE_DIR}/include
  ${Assimp_SOURCE_DIR}/code
  ${SAMPLES_SHARED_CODE_DIR}
)

LINK_DIRECTORIES(
  ${Assimp_BINARY_DIR}
  ${Assimp_BINARY_DIR}/lib
)

ADD_EXECUTABLE( assimp_simpletextureddirectx11 WIN32
  SimpleTexturedDirectx11/Mesh.h 
  SimpleTexturedDirectx11/ModelLoader.cpp
  SimpleTexturedDirectx11/ModelLoader.h
  #SimpleTexturedDirectx11/PixelShader.hlsl
  SimpleTexturedDirectx11/TextureLoader.cpp
  SimpleTexturedDirectx11/TextureLoader.h 
  #SimpleTexturedDirectx11/VertexShader.hlsl  
  SimpleTexturedDirectx11/main.cpp
  SimpleTexturedDirectx11/SafeRelease.hpp
  ${SAMPLES_SHARED_CODE_DIR}/UTFConverter.cpp
  ${SAMPLES_SHARED_CODE_DIR}/UTFConverter.h
)

SET_PROPERTY(TARGET assimp_simpletextureddirectx11 PROPERTY DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

TARGET_LINK_LIBRARIES( assimp_simpletextureddirectx11 assimp comctl32.lib winmm.lib )
SET_TARGET_PROPERTIES( assimp_simpletextureddirectx11 PROPERTIES
  OUTPUT_NAME assimp_simpletextureddirectx11
)

INSTALL( TARGETS assimp_simpletextureddirectx11
  DESTINATION "${ASSIMP_BIN_INSTALL_DIR}" COMPONENT assimp-dev
)
