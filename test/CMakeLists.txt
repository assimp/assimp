cmake_minimum_required( VERSION 2.6 )

INCLUDE_DIRECTORIES(
	${Assimp_SOURCE_DIR}/include
	${Assimp_SOURCE_DIR}/code
	${Assimp_SOURCE_DIR}/contrib/cppunit-1.12.1/include
)

include(CheckIncludeFiles)
CHECK_INCLUDE_FILES("sys/param.h;sys/mount.h" HAVE_SYS_MOUNT_H)
CHECK_INCLUDE_FILES("stdarg.h;stddef.h;errno.h" CPPUNIT_STDC_HEADERS)
include(CheckIncludeFile)
CHECK_INCLUDE_FILE(malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILE(dlfcn.h CPPUNIT_HAVE_DLD)
CHECK_INCLUDE_FILE(dlfcn.h CPPUNIT_HAVE_DLFCN_H)
CHECK_INCLUDE_FILE(ieeefp.h CPPUNIT_HAVE_IEEEFP_H)
CHECK_INCLUDE_FILE(inttypes.h CPPUNIT_HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE(memory.h CPPUNIT_HAVE_MEMORY_H)
CHECK_INCLUDE_FILE(stdint.h CPPUNIT_HAVE_STDINT_H)
CHECK_INCLUDE_FILE(stdlib.h CPPUNIT_HAVE_STDLIB_H)
CHECK_INCLUDE_FILE(strings.h CPPUNIT_HAVE_STRINGS_H)
CHECK_INCLUDE_FILE(string.h CPPUNIT_HAVE_STRING_H)
CHECK_INCLUDE_FILE("sys/stat.h" CPPUNIT_HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE("sys/types.h" CPPUNIT_HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(unistd.h CPPUNIT_HAVE_UNISTD_H)
include(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(std::string::compare CPPUNIT_FUNC_STRING_COMPARE_STRING_FIRST)
CHECK_FUNCTION_EXISTS(type_info::name CPPUNIT_USE_TYPEINFO_NAME)
include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(dl dlopen "" CPPUNIT_HAVE_LIBDL)
CHECK_LIBRARY_EXISTS(dl dlerror "" CPPUNIT_HAVE_DLERROR)
CHECK_LIBRARY_EXISTS(m jnf "" CPPUNIT_HAVE_LIBM)
CHECK_LIBRARY_EXISTS(dld shl_load "" CPPUNIT_HAVE_SHL_LOAD)
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(strstream CPPUNIT_HAVE_STRSTREAM)
CHECK_INCLUDE_FILE_CXX(strstream CPPUNIT_HAVE_CLASS_STRSTREAM)
include(TestForSTDNamespace)
set(CPPUNIT_HAVE_NAMESPACES ${CMAKE_STD_NAMESPACE})
	#CHECK_INCLUDE_FILE_CXX(namespaces CPPUNIT_HAVE_NAMESPACES)
CHECK_INCLUDE_FILE_CXX(cmath CPPUNIT_HAVE_CMATH)
CHECK_INCLUDE_FILE_CXX(stringstream CPPUNIT_HAVE_SSTREAM)
include(CheckCXXSymbolExists)
CHECK_CXX_SYMBOL_EXISTS(finite cmath CPPUNIT_HAVE_FINITE)
CHECK_CXX_SYMBOL_EXISTS(isfinite cmath CPPUNIT_HAVE_ISFINITE)
# fake tests, needs better ones...
set(GCC_ABI_DEMANGLE 1)
set(CPPUNIT_HAVE_GCC_ABI_DEMANGLE ${GCC_ABI_DEMANGLE})
set(HAVE_RTTI 1)
set(CPPUNIT_HAVE_RTTI ${HAVE_RTTI})
#CHECK_INCLUDE_FILES(malloc.h CPPUNIT_HAVE_GCC_ABI_DEMANGLE)
#CHECK_INCLUDE_FILE_CXX(string CPPUNIT_HAVE_RTTI)
CONFIGURE_FILE(${Assimp_SOURCE_DIR}/contrib/config.h.in.cppunit ${Assimp_SOURCE_DIR}/contrib/cppunit-1.12.1/include/cppunit/config-auto.h)

# Add the temporary output directories to the library path to make sure the
# Assimp library can be found, even if it is not installed system-wide yet.
LINK_DIRECTORIES( ${Assimp_BINARY_DIR} ${AssetImporter_BINARY_DIR}/lib )

SOURCE_GROUP( unit FILES 
	unit/CCompilerTest.c
	unit/BoostWorkaround/tupletest.cpp
)

SET( CPPUNIT_SRCS
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
SOURCE_GROUP(cppunit FILES  ${CPPUNIT_SRCS})
if(NOT WIN32)
	find_library(CPPUNIT_LIBRARY cppunit)
	set(CPPUNIT_LIBRARY "dl")
endif(NOT WIN32)

SET( TEST_SRCS
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
)

SOURCE_GROUP(tests FILES  ${TEST_SRCS})


add_executable(unit EXCLUDE_FROM_ALL
	unit/CCompilerTest.c
	unit/Main.cpp
	unit/UnitTestPCH.cpp
	unit/UnitTestPCH.h
	${TEST_SRCS}
	${CPPUNIT_SRCS})

SET_PROPERTY(TARGET assimp PROPERTY DEBUG_POSTFIX ${ASSIMP_DEBUG_POSTFIX})

target_link_libraries(unit assimp ${CPPUNIT_LIBRARY})
