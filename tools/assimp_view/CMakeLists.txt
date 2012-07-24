FIND_PACKAGE(DirectX REQUIRED)

# Make sure the compiler can find include files from our Hello library.
include_directories (
	${Assimp_SOURCE_DIR}/include
	${Assimp_SOURCE_DIR}/code
	${DirectX_INCLUDE_DIR}
)

# Make sure the linker can find the Hello library once it is built.
link_directories (${Assimp_BINARY_DIR} ${AssetImporter_BINARY_DIR}/lib)

# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx". The extensions are automatically found.
add_executable(  assimp_viewer WIN32
	AnimEvaluator.cpp
	Background.cpp
	Display.cpp
	HelpDialog.cpp
	Input.cpp
	LogDisplay.cpp
	LogWindow.cpp
	Material.cpp
	MeshRenderer.cpp
	MessageProc.cpp
	Normals.cpp
	SceneAnimator.cpp
	Shaders.cpp
	assimp_view.cpp
	stdafx.cpp
	assimp_view.rc
	banner.bmp
	banner_pure.bmp
	base_anim.bmp
	base_display.bmp
	base_inter.bmp
	base_rendering.bmp
	base_stats.bmp
	fx.bmp
	n.bmp
	root.bmp
	tx.bmp
	txi.bmp
)

SET_PROPERTY(TARGET assimp_viewer PROPERTY DEBUG_POSTFIX ${ASSIMP_DEBUG_POSTFIX})

IF( WIN32 )
	ADD_DEFINITIONS( -D_SCL_SECURE_NO_WARNINGS )
	ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
ENDIF( WIN32 )

# Link the executable to the Hello library.
target_link_libraries ( assimp_viewer assimp ${DirectX_LIBRARY} ${DirectX_D3DX9_LIBRARY} comctl32.lib Winmm.lib  ) 

INSTALL( TARGETS assimp_viewer
	DESTINATION "${ASSIMP_BIN_INSTALL_DIR}" COMPONENT assimp-dev
)
