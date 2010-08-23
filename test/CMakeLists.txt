INCLUDE_DIRECTORIES(
	${Assimp_SOURCE_DIR}/include
	${Assimp_SOURCE_DIR}/code
	${Assimp_SOURCE_DIR}/contrib/cppunit-1.12.1/include
)

# Add the temporary output directories to the library path to make sure the
# Assimp library can be found, even if it is not installed system-wide yet.
LINK_DIRECTORIES( ${Assimp_BINARY_DIR} ${AssetImporter_BINARY_DIR}/lib )

SOURCE_GROUP( unit FILES 
	unit/CCompilerTest.c
	unit/BoostWorkaround/tupletest.cpp
)

SOURCE_GROUP( cppunit FILES 
	../contrib/cppunit-1.12.1/src/cppunit/AdditionalMessage.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Asserter.cpp
	../contrib/cppunit-1.12.1/src/cppunit/BeOsDynamicLibraryManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/BriefTestProgressListener.cpp
	../contrib/cppunit-1.12.1/src/cppunit/CompilerOutputter.cpp
	../contrib/cppunit-1.12.1/src/cppunit/DefaultProtector.cpp
	../contrib/cppunit-1.12.1/src/cppunit/DefaultProtector.h
	../contrib/cppunit-1.12.1/src/cppunit/DllMain.cpp
	../contrib/cppunit-1.12.1/src/cppunit/DynamicLibraryManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/DynamicLibraryManagerException.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Exception.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Message.cpp
	../contrib/cppunit-1.12.1/src/cppunit/PlugInManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/PlugInParameters.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Protector.cpp
	../contrib/cppunit-1.12.1/src/cppunit/ProtectorChain.cpp
	../contrib/cppunit-1.12.1/src/cppunit/ProtectorChain.h
	../contrib/cppunit-1.12.1/src/cppunit/ProtectorContext.h
	../contrib/cppunit-1.12.1/src/cppunit/RepeatedTest.cpp
	../contrib/cppunit-1.12.1/src/cppunit/ShlDynamicLibraryManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/SourceLine.cpp
	../contrib/cppunit-1.12.1/src/cppunit/StringTools.cpp
	../contrib/cppunit-1.12.1/src/cppunit/SynchronizedObject.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Test.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestAssert.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestCase.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestCaseDecorator.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestComposite.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestDecorator.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestFactoryRegistry.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestFailure.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestLeaf.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestNamer.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestPath.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestPlugInDefaultImpl.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestResult.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestResultCollector.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestRunner.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestSetUp.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestSuccessListener.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestSuite.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestSuiteBuilderContext.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TextOutputter.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TextTestProgressListener.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TextTestResult.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TextTestRunner.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TypeInfoHelper.cpp
	../contrib/cppunit-1.12.1/src/cppunit/UnixDynamicLibraryManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Win32DynamicLibraryManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/XmlDocument.cpp
	../contrib/cppunit-1.12.1/src/cppunit/XmlElement.cpp
	../contrib/cppunit-1.12.1/src/cppunit/XmlOutputter.cpp
	../contrib/cppunit-1.12.1/src/cppunit/XmlOutputterHook.cpp
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
	unit/utNoBoostTest.cpp
	unit/utNoBoostTest.h
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
	unit/utNoBoostTest.cpp
	unit/utNoBoostTest.h
	unit/BoostWorkaround/tupletest.cpp	
	../contrib/cppunit-1.12.1/src/cppunit/AdditionalMessage.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Asserter.cpp
	../contrib/cppunit-1.12.1/src/cppunit/BeOsDynamicLibraryManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/BriefTestProgressListener.cpp
	../contrib/cppunit-1.12.1/src/cppunit/CompilerOutputter.cpp
	../contrib/cppunit-1.12.1/src/cppunit/DefaultProtector.cpp
	../contrib/cppunit-1.12.1/src/cppunit/DefaultProtector.h
	../contrib/cppunit-1.12.1/src/cppunit/DllMain.cpp
	../contrib/cppunit-1.12.1/src/cppunit/DynamicLibraryManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/DynamicLibraryManagerException.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Exception.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Message.cpp
	../contrib/cppunit-1.12.1/src/cppunit/PlugInManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/PlugInParameters.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Protector.cpp
	../contrib/cppunit-1.12.1/src/cppunit/ProtectorChain.cpp
	../contrib/cppunit-1.12.1/src/cppunit/ProtectorChain.h
	../contrib/cppunit-1.12.1/src/cppunit/ProtectorContext.h
	../contrib/cppunit-1.12.1/src/cppunit/RepeatedTest.cpp
	../contrib/cppunit-1.12.1/src/cppunit/ShlDynamicLibraryManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/SourceLine.cpp
	../contrib/cppunit-1.12.1/src/cppunit/StringTools.cpp
	../contrib/cppunit-1.12.1/src/cppunit/SynchronizedObject.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Test.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestAssert.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestCase.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestCaseDecorator.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestComposite.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestDecorator.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestFactoryRegistry.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestFailure.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestLeaf.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestNamer.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestPath.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestPlugInDefaultImpl.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestResult.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestResultCollector.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestRunner.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestSetUp.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestSuccessListener.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestSuite.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TestSuiteBuilderContext.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TextOutputter.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TextTestProgressListener.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TextTestResult.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TextTestRunner.cpp
	../contrib/cppunit-1.12.1/src/cppunit/TypeInfoHelper.cpp
	../contrib/cppunit-1.12.1/src/cppunit/UnixDynamicLibraryManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/Win32DynamicLibraryManager.cpp
	../contrib/cppunit-1.12.1/src/cppunit/XmlDocument.cpp
	../contrib/cppunit-1.12.1/src/cppunit/XmlElement.cpp
	../contrib/cppunit-1.12.1/src/cppunit/XmlOutputter.cpp
	../contrib/cppunit-1.12.1/src/cppunit/XmlOutputterHook.cpp
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

# TODO: Port to non-Windows platforms.
target_link_libraries ( unit assimp ${DX9_LIBRARIES} comctl32.lib Winmm.lib ) 
