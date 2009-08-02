include_directories (
	${AssetImporter_SOURCE_DIR}/include
	${AssetImporter_SOURCE_DIR}/code
)

# Make sure the linker can find the Hello library once it is built.
link_directories (${AssetImporter_BINARY_DIR} ${AssetImporter_BINARY_DIR}/lib)

SOURCE_GROUP( unit FILES 
	unit/CCompilerTest.c
	unit/BoostWorkaround/tupletest.cpp
)

SOURCE_GROUP( tests FILES 
	unit/Main.cpp
	unit/UnitTestPCH.cpp
	unit/UnitTestPCH.h
	unit/utFindDegenerates.cpp
	unit/utFindDegenerates.h
	unit/utFindInvalidData.cpp
	unit/utFindInvalidData.h
	unit/utFixInfacingNormals.cpp
	unit/utGenNormals.cpp
	unit/utGenNormals.h
	unit/utImporter.cpp
	unit/utImporter.h
	unit/utImproveCacheLocality.cpp
	unit/utJoinVertices.cpp
	unit/utJoinVertices.h
	unit/utLimitBoneWeights.cpp
	unit/utLimitBoneWeights.h
	unit/utMaterialSystem.cpp
	unit/utMaterialSystem.h
	unit/utPretransformVertices.cpp
	unit/utPretransformVertices.h
	unit/utRemoveComments.cpp
	unit/utRemoveComments.h
	unit/utRemoveComponent.cpp
	unit/utRemoveComponent.h
	unit/utRemoveRedundantMaterials.cpp
	unit/utRemoveRedundantMaterials.h
	unit/utScenePreprocessor.cpp
	unit/utScenePreprocessor.h
	unit/utSharedPPData.cpp
	unit/utSharedPPData.h
	unit/utSortByPType.cpp
	unit/utSortByPType.h
	unit/utSplitLargeMeshes.cpp
	unit/utSplitLargeMeshes.h
	unit/utTargetAnimation.cpp
	unit/utTargetAnimation.h
	unit/utTextureTransform.cpp
	unit/utTriangulate.cpp
	unit/utTriangulate.h
	unit/utVertexTriangleAdjacency.cpp
	unit/utVertexTriangleAdjacency.h

)

add_executable(  unit
	unit/CCompilerTest.c
	unit/Main.cpp
	unit/UnitTestPCH.cpp
	unit/UnitTestPCH.h
	unit/utFindDegenerates.cpp
	unit/utFindDegenerates.h
	unit/utFindInvalidData.cpp
	unit/utFindInvalidData.h
	unit/utFixInfacingNormals.cpp
	unit/utGenNormals.cpp
	unit/utGenNormals.h
	unit/utImporter.cpp
	unit/utImporter.h
	unit/utImproveCacheLocality.cpp
	unit/utJoinVertices.cpp
	unit/utJoinVertices.h
	unit/utLimitBoneWeights.cpp
	unit/utLimitBoneWeights.h
	unit/utMaterialSystem.cpp
	unit/utMaterialSystem.h
	unit/utPretransformVertices.cpp
	unit/utPretransformVertices.h
	unit/utRemoveComments.cpp
	unit/utRemoveComments.h
	unit/utRemoveComponent.cpp
	unit/utRemoveComponent.h
	unit/utRemoveRedundantMaterials.cpp
	unit/utRemoveRedundantMaterials.h
	unit/utScenePreprocessor.cpp
	unit/utScenePreprocessor.h
	unit/utSharedPPData.cpp
	unit/utSharedPPData.h
	unit/utSortByPType.cpp
	unit/utSortByPType.h
	unit/utSplitLargeMeshes.cpp
	unit/utSplitLargeMeshes.h
	unit/utTargetAnimation.cpp
	unit/utTargetAnimation.h
	unit/utTextureTransform.cpp
	unit/utTriangulate.cpp
	unit/utTriangulate.h
	unit/utVertexTriangleAdjacency.cpp
	unit/utVertexTriangleAdjacency.h

	unit/BoostWorkaround/tupletest.cpp
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
target_link_libraries ( unit assimp ${DX9_LIBRARIES} comctl32.lib Winmm.lib  ) 

