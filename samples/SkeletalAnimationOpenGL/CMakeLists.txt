FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLUT)

IF ( NOT GLUT_FOUND )
  MESSAGE( FATAL_ERROR "Please install glut." )
ENDIF ()

IF ( MSVC )
  ADD_DEFINITIONS( -D_SCL_SECURE_NO_WARNINGS )
  ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
  REMOVE_DEFINITIONS( -DUNICODE -D_UNICODE )
  IF ( MSVC_VERSION GREATER_EQUAL 1930 )
    ADD_DEFINITIONS( -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING=1 )
  ENDIF ()

  set(M_LIB "")
ELSE ()
  set(M_LIB "m")
ENDIF ()

ADD_COMPILE_DEFINITIONS(SHADER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/SkeletalAnimationOpenGL/")

INCLUDE_DIRECTORIES(
  ${Assimp_SOURCE_DIR}
  ${Assimp_SOURCE_DIR}/include
  ${Assimp_SOURCE_DIR}/code
  ${SAMPLES_SHARED_CODE_DIR}
  ${Assimp_SOURCE_DIR}/contrib/utf8cpp/source
  ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
  ${GLUT_INCLUDE_DIR}
)

LINK_DIRECTORIES(
  ${Assimp_BINARY_DIR}
  ${Assimp_BINARY_DIR}/lib
)

ADD_EXECUTABLE( assimp_skeletalanimationogl
  SkeletalAnimationOpenGL/main.cpp
  SkeletalAnimationOpenGL/app.cpp
  SkeletalAnimationOpenGL/mesh.cpp
  SkeletalAnimationOpenGL/math.cpp
  SkeletalAnimationOpenGL/model.cpp
  SkeletalAnimationOpenGL/model_loader.cpp
  glad/src/glad.c
)

TARGET_USE_COMMON_OUTPUT_DIRECTORY(assimp_skeletalanimationogl)

SET_PROPERTY(TARGET assimp_skeletalanimationogl PROPERTY DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

TARGET_LINK_LIBRARIES( assimp_skeletalanimationogl assimp ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${M_LIB} )
TARGET_PRECOMPILE_HEADERS( assimp_skeletalanimationogl PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_LIST_DIR}/SkeletalAnimationOpenGL/pch.hpp>
)
SET_TARGET_PROPERTIES( assimp_skeletalanimationogl PROPERTIES
  OUTPUT_NAME assimp_skeletalanimationogl
)

INSTALL( TARGETS assimp_skeletalanimationogl
  DESTINATION "${ASSIMP_BIN_INSTALL_DIR}" COMPONENT assimp-dev
)
