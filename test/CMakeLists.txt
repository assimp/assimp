cmake_minimum_required( VERSION 2.6 )

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

if(WIN32)
# cpppunit provides configuration headers for MSVC. Other plaforms require
# explicit creation at configure time.
if(NOT MSVC)
	# Generates include/cppunit/config-auto.h
	# This is originally done by autoconf

	include(CheckIncludeFile)
	include(CheckIncludeFileCXX)
	include(CheckCXXSourceCompiles)
	include(CheckCSourceCompiles)
	include(CheckLibraryExists)
	include(CheckFunctionExists)

	check_include_file_cxx(sstream CPPUNIT_HAVE_SSTREAM)
	check_include_file_cxx(strstream CPPUNIT_HAVE_STRSTREAM)
	set (CMAKE_REQUIRED_DEFINITIONS -DHAVE_STRSTREAM=CPPUNIT_HAVE_STRSTREAM)
	check_cxx_source_compiles(
	"#ifdef HAVE_STRSTREAM
	#	include <strstream>
	#else
	#	include <strstream.h>
	#endif
	int main() {
		std::ostrstream message;
		message << \"Hello\";
		return 0;
	}" CPPUNIT_HAVE_CLASS_STRSTREAM)

	check_include_file_cxx(cmath CPPUNIT_HAVE_CMATH)
	#Not used, dld library is obsolete anyway HAVE_DLD
	#Not used HAVE_DLERROR
	check_include_file(dlfcn.h CPPUNIT_HAVE_DLFCN_H)

	check_c_source_compiles(
	"#include <math.h>
	int main() {
		return finite(3);
	}" CPPUNIT_HAVE_FINITE)

	check_c_source_compiles(
	"#include <math.h>
	int main() {
		return _finite(3);
	}" CPPUNIT_HAVE__FINITE)

	#Not used CPPUNIT_HAVE_GCC_ABI_DEMANGLE
	#Not used HAVE_INTTYPES_H

	check_c_source_compiles(
	"#include <math.h>
	int main() {
		return isfinite(3);
	}" CPPUNIT_HAVE_ISFINITE)

	check_library_exists(dl dlopen "" CPPUNIT_HAVE_LIBDL)
	#Not used HAVE_MEMORY_H

	check_cxx_source_compiles(
	"namespace Outer {
		namespace Inner {
			int i = 0;
		}
	}
	using namespace Outer::Inner;
	int main() {
		return i;
	}" CPPUNIT_HAVE_NAMESPACES)

	check_cxx_source_compiles(
	"#include <typeinfo>
	class Base {
	public:
		Base() {}
		virtual int f() { return 0; }
	};
	class Derived : public Base {
	public:
		Derived() {}
		virtual int f() { return 1; }
	};
	int main() {
		Derived d;
		Base * ptr = &d;
		return typeid(*ptr) == typeid(Derived);
	}" CPPUNIT_HAVE_RTTI)

	check_library_exists(dl shl_load "" CPPUNIT_HAVE_SHL_LOAD)
	#Not used HAVE_STDINT_H
	#Not used HAVE_STDLIB_H
	#Not used HAVE_STRINGS_H
	#Not used HAVE_STRING_H
	#Not used HAVE_SYS_STAT_H
	#Not used HAVE_SYS_TYPES_H
	#Not used HAVE_UNISTD_H
	#Not used PACKAGE
	#Not used PACKAGE_BUGREPORT
	#Not used PACKAGE_NAME
	#Not used PACKAGE_STRING
	#Not used PACKAGE_TARNAME
	#Not used PACKAGE_VERSION
	#Not used STDC_HEADERS
	check_include_file_cxx(typeinfo CPPUNIT_USE_TYPEINFO_NAME)
	configure_file(unit/cppunit-config/config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/cppunit-1.12.1/include/cppunit/config-auto.h)
endif()

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
else()
	find_library(CPPUNIT_LIBRARY cppunit)
endif()

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


add_executable(  unit
	unit/CCompilerTest.c
	unit/Main.cpp
	unit/UnitTestPCH.cpp
	unit/UnitTestPCH.h
	${TEST_SRCS}
	${CPPUNIT_SRCS}
)

SET_PROPERTY(TARGET assimp PROPERTY DEBUG_POSTFIX ${ASSIMP_DEBUG_POSTFIX})

target_link_libraries ( unit assimp ${CPPUNIT_LIBRARY} )
