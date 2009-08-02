# Make sure the compiler can find include files from our Hello library.
include_directories (
	${AssetImporter_SOURCE_DIR}/include
	${AssetImporter_SOURCE_DIR}/code
)

# Make sure the linker can find the Hello library once it is built.
link_directories (${AssetImporter_BINARY_DIR} ${AssetImporter_BINARY_DIR}/lib)

# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx". The extensions are automatically found.
add_executable(  assimp_cmd
	../../contrib/zlib/adler32.c
	../../contrib/zlib/compress.c
	../../contrib/zlib/crc32.c
	../../contrib/zlib/crc32.h
	../../contrib/zlib/deflate.c
	../../contrib/zlib/deflate.h
	../../contrib/zlib/inffast.c
	../../contrib/zlib/inffast.h
	../../contrib/zlib/inffixed.h
	../../contrib/zlib/inflate.c
	../../contrib/zlib/inflate.h
	../../contrib/zlib/inftrees.c
	../../contrib/zlib/inftrees.h
	../../contrib/zlib/trees.c
	../../contrib/zlib/trees.h
	../../contrib/zlib/zconf.h
	../../contrib/zlib/zconf.in.h
	../../contrib/zlib/zlib.h
	../../contrib/zlib/zutil.c
	../../contrib/zlib/zutil.h
	assimp_cmd.rc
	CompareDump.cpp
	ImageExtractor.cpp
	Main.cpp
	Main.h
	resource.h
	WriteDumb.cpp
)

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
ENDIF( WIN32 )

# Link the executable to the Hello library.
target_link_libraries ( assimp_cmd assimp ${DX9_LIBRARIES} comctl32.lib Winmm.lib  ) 

