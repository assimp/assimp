# Make sure the compiler can find include files from our Hello library.
include_directories (
	${Assimp_SOURCE_DIR}/include
	${Assimp_SOURCE_DIR}/code
	${DX9_INCLUDE_PATH}
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

SET_PROPERTY(TARGET assimp_viewer PROPERTY DEBUG_POSTFIX ${DEBUG_POSTFIX})

IF( WIN32 )
	SET( PSDK_PATH "C:/Program Files/Microsoft Platform SDK/Bin" )
	SET( PSDK_INC "C:/Program Files/Microsoft Platform SDK/Include" )

	FIND_PATH(DX9_INCLUDE_PATH d3d9.h
		PATHS
			"$ENV{DXSDK_DIR}/Include"
			"$ENV{PROGRAMFILES}/Microsoft DirectX SDK/Include"
		DOC "The directory where D3D9.h resides")


	FIND_LIBRARY(D3D9_LIBRARY d3d9.lib
		PATHS
			"$ENV{DXSDK_DIR}/Lib/x86"
			"$ENV{PROGRAMFILES}/Microsoft DirectX SDK/Lib/x86"
		DOC "The directory where d3d9.lib resides")

	FIND_LIBRARY(D3DX9_LIBRARY d3dx9.lib
		PATHS
			"$ENV{DXSDK_DIR}/Lib/x86"
			"$ENV{PROGRAMFILES}/Microsoft DirectX SDK/Lib/x86"
		DOC "The directory where d3dx9.lib resides")

	SET( DX9_LIBRARIES ${D3D9_LIBRARY} ${D3DX9_LIBRARY} )
	
	FIND_LIBRARY( WIN32_COMCTRL comctl32.lib 
		PATHS 
			"C:/Programme/Microsoft Platform SDK for Windows Server 2003 R2/Lib" 	
		DOC "Path to psdk"
	)
	ADD_DEFINITIONS( -D_SCL_SECURE_NO_WARNINGS )
	ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
ENDIF( WIN32 )

# Link the executable to the Hello library.
target_link_libraries ( assimp_viewer assimp ${DX9_LIBRARIES} comctl32.lib Winmm.lib  ) 

INSTALL( TARGETS assimp_viewer
	DESTINATION "${BIN_INSTALL_DIR}" COMPONENT assimp-dev
) 
