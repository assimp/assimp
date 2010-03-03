# Make sure the compiler can find include files from our Hello library.
include_directories (
	${Assimp_SOURCE_DIR}/include
	${Assimp_SOURCE_DIR}/code
)

# Make sure the linker can find the Hello library once it is built.
link_directories (${Assimp_BINARY_DIR} ${AssetImporter_BINARY_DIR}/lib)

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


# Link the executable to the assimp library and rename it to assimp as well :-)
target_link_libraries ( assimp_cmd assimp  )
INSTALL(PROGRAMS ../../bin/assimp_cmd 
	DESTINATION "${BIN_INSTALL_DIR}"
	RENAME assimp
) 

