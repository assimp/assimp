INCLUDE_DIRECTORIES(
	${Assimp_SOURCE_DIR}/include
	${Assimp_SOURCE_DIR}/code
)

LINK_DIRECTORIES( ${Assimp_BINARY_DIR} ${Assimp_BINARY_DIR}/lib )

ADD_EXECUTABLE( assimp_cmd
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
	Info.cpp
)

TARGET_LINK_LIBRARIES( assimp_cmd assimp )
SET_TARGET_PROPERTIES( assimp_cmd PROPERTIES
	OUTPUT_NAME assimp
)

INSTALL( TARGETS assimp_cmd
	DESTINATION "${BIN_INSTALL_DIR}"
) 
