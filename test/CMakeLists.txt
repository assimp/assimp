# Open Asset Import Library (assimp)
# ----------------------------------------------------------------------
# Copyright (c) 2006-2024, assimp team
#
# All rights reserved.
#
# Redistribution and use of this software in source and binary forms,
# with or without modification, are permitted provided that the
# following conditions are met:
#
# * Redistributions of source code must retain the above
#   copyright notice, this list of conditions and the
#   following disclaimer.
#
# * Redistributions in binary form must reproduce the above
#   copyright notice, this list of conditions and the
#   following disclaimer in the documentation and/or other
#   materials provided with the distribution.
#
# * Neither the name of the assimp team, nor the names of its
#   contributors may be used to endorse or promote products
#   derived from this software without specific prior
#   written permission of the assimp team.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#----------------------------------------------------------------------
cmake_minimum_required( VERSION 3.10 )

INCLUDE_DIRECTORIES(
    ${Assimp_SOURCE_DIR}/test/unit
    ${Assimp_SOURCE_DIR}/include
    ${Assimp_SOURCE_DIR}/code
)

if(NOT ASSIMP_HUNTER_ENABLED)
  INCLUDE_DIRECTORIES(
    ${Assimp_SOURCE_DIR}/contrib/googletest/googletest/include
    ${Assimp_SOURCE_DIR}/contrib/googletest/googletest/
    ${Assimp_SOURCE_DIR}/contrib/pugixml/src
  )
endif()

if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
endif()

# Add the temporary output directories to the library path to make sure the
# Assimp library can be found, even if it is not installed system-wide yet.
LINK_DIRECTORIES( ${Assimp_BINARY_DIR} ${AssetImporter_BINARY_DIR}/lib )

SET( COMMON
  unit/utSimd.cpp
  unit/utIOSystem.cpp
  unit/utIOStreamBuffer.cpp
  unit/utIssues.cpp
  unit/utAnim.cpp
  unit/AssimpAPITest.cpp
  unit/AssimpAPITest_aiMatrix3x3.cpp
  unit/AssimpAPITest_aiMatrix4x4.cpp
  unit/AssimpAPITest_aiQuaternion.cpp
  unit/AssimpAPITest_aiVector2D.cpp
  unit/AssimpAPITest_aiVector3D.cpp
  unit/MathTest.cpp
  unit/MathTest.h
  unit/RandomNumberGeneration.h
  unit/utBatchLoader.cpp
  unit/utDefaultIOStream.cpp
  unit/utFastAtof.cpp
  unit/utMetadata.cpp
  unit/SceneDiffer.h
  unit/SceneDiffer.cpp
  unit/UTLogStream.h
  unit/AbstractImportExportBase.cpp
  unit/TestIOSystem.h
  unit/TestModelFactory.h
  unit/utTypes.cpp
  unit/utVersion.cpp
  unit/utProfiler.cpp
  unit/utSharedPPData.cpp
  unit/utStringUtils.cpp
  unit/Common/utMaybe.cpp
  unit/Common/utMesh.cpp
  unit/Common/utStandardShapes.cpp
  unit/Common/uiScene.cpp
  unit/Common/utLineSplitter.cpp
  unit/Common/utSpatialSort.cpp
  unit/Common/utAssertHandler.cpp
  unit/Common/utXmlParser.cpp
  unit/Common/utBase64.cpp
  unit/Common/utHash.cpp
  unit/Common/utBaseProcess.cpp
  unit/Common/utLogger.cpp
)

SET(Geometry 
    unit/Geometry/utGeometryUtils.cpp
)

SET( IMPORTERS
  unit/ImportExport/Assxml/utAssxmlImportExport.cpp
  unit/utLWSImportExport.cpp
  unit/utLWOImportExport.cpp
  unit/utSMDImportExport.cpp
  unit/utglTFImportExport.cpp
  unit/utglTF2ImportExport.cpp
  unit/utHMPImportExport.cpp
  unit/utIFCImportExport.cpp
  unit/utFBXImporterExporter.cpp
  unit/utImporter.cpp
  unit/ImportExport/utExporter.cpp
  unit/ut3DImportExport.cpp
  unit/ut3DSImportExport.cpp
  unit/utACImportExport.cpp
  unit/utAMFImportExport.cpp
  unit/utASEImportExport.cpp
  unit/utD3MFImportExport.cpp
  unit/utQ3DImportExport.cpp
  unit/utSTLImportExport.cpp
  unit/utXImporterExporter.cpp
  unit/utX3DImportExport.cpp
  unit/utDXFImporterExporter.cpp
  unit/utPMXImporter.cpp
  unit/utPLYImportExport.cpp
  unit/utObjImportExport.cpp
  unit/utObjTools.cpp
  unit/utOpenGEXImportExport.cpp
  unit/utSIBImporter.cpp
  unit/utBlenderIntermediate.cpp
  unit/utBlendImportAreaLight.cpp
  unit/utBlenderImportExport.cpp
  unit/utBlendImportMaterials.cpp
  unit/utBlenderWork.cpp
  unit/utBVHImportExport.cpp
  unit/utColladaExport.cpp
  unit/utColladaImportExport.cpp
  unit/utCSMImportExport.cpp
  unit/utB3DImportExport.cpp
  #unit/utM3DImportExport.cpp
  unit/utMDCImportExport.cpp
  unit/utAssbinImportExport.cpp
  unit/utUSDImport.cpp
  unit/ImportExport/utAssjsonImportExport.cpp
  unit/ImportExport/utCOBImportExport.cpp
  unit/ImportExport/utOgreImportExport.cpp
  unit/ImportExport/utQ3BSPFileImportExport.cpp
  unit/ImportExport/utOFFImportExport.cpp
  unit/ImportExport/utNFFImportExport.cpp
  unit/ImportExport/utXGLImportExport.cpp
  unit/ImportExport/utMD2Importer.cpp
  #unit/ImportExport/utMD3Importer.cpp
  unit/ImportExport/utMD5Importer.cpp
  unit/ImportExport/utMDLImporter.cpp
  unit/ImportExport/MDL/MDLHL1TestFiles.h
  unit/ImportExport/MDL/utMDLImporter_HL1_ImportSettings.cpp
  unit/ImportExport/MDL/utMDLImporter_HL1_Materials.cpp
  unit/ImportExport/MDL/utMDLImporter_HL1_Nodes.cpp
  unit/ImportExport/RAW/utRAWImportExport.cpp
  unit/ImportExport/Terragen/utTerragenImportExport.cpp
  unit/ImportExport/Pbrt/utPbrtImportExport.cpp
)

SET( MATERIAL
  unit/utMaterialSystem.cpp
)

SET( MATH
  unit/utMatrix3x3.cpp
  unit/utMatrix4x4.cpp
  unit/utVector3.cpp
)

SET( POST_PROCESSES
  unit/utImproveCacheLocality.cpp
  unit/utFixInfacingNormals.cpp
  unit/utGenNormals.cpp
  unit/utTriangulate.cpp
  unit/utTextureTransform.cpp
  unit/utRemoveRedundantMaterials.cpp
  unit/utRemoveVCProcess.cpp
  unit/utScaleProcess.cpp
  unit/utArmaturePopulate.cpp
  unit/utJoinVertices.cpp
  unit/utRemoveComments.cpp
  unit/utRemoveComponent.cpp
  unit/utVertexTriangleAdjacency.cpp
  unit/utJoinVertices.cpp
  unit/utSplitLargeMeshes.cpp
  unit/utFindDegenerates.cpp
  unit/utFindInvalidData.cpp
  unit/utLimitBoneWeights.cpp
  unit/utPretransformVertices.cpp
  unit/utScenePreprocessor.cpp
  unit/utTargetAnimation.cpp
  unit/utSortByPType.cpp
  unit/utSceneCombiner.cpp
  unit/utGenBoundingBoxesProcess.cpp
)

SOURCE_GROUP( UnitTests\\Compiler      FILES unit/CCompilerTest.c )
SOURCE_GROUP( UnitTests\\Common        FILES ${COMMON} )
SOURCE_GROUP( UnitTests\\GeometryTools FILES ${Geometry} )
SOURCE_GROUP( UnitTests\\ImportExport  FILES ${IMPORTERS} )
SOURCE_GROUP( UnitTests\\Material      FILES ${MATERIAL} )
SOURCE_GROUP( UnitTests\\Math          FILES ${MATH} )
SOURCE_GROUP( UnitTests\\PostProcess   FILES ${POST_PROCESSES})

add_executable( unit
    unit/CCompilerTest.c
    unit/Main.cpp
    ../code/Common/Version.cpp
	../code/Common/Base64.cpp
	${COMMON}
  ${Geometry}
	${IMPORTERS}
	${MATERIAL}
	${MATH}
  ${POST_PROCESSES}
)

if(ASSIMP_HUNTER_ENABLED)
    hunter_add_package(GTest)
    find_package(GTest CONFIG REQUIRED)
    target_link_libraries(unit GTest::gtest_main GTest::gmock)
else()
    target_sources(unit PUBLIC ${Assimp_SOURCE_DIR}/contrib/googletest/googletest/src/gtest-all.cc)
endif()

# RapidJSON
IF(ASSIMP_HUNTER_ENABLED)
  hunter_add_package(RapidJSON)
  find_package(RapidJSON CONFIG REQUIRED)
ELSE()
  INCLUDE_DIRECTORIES("../contrib/rapidjson/include")
  ADD_DEFINITIONS( -DRAPIDJSON_HAS_STDSTRING=1)
  option( ASSIMP_RAPIDJSON_NO_MEMBER_ITERATOR "Suppress rapidjson warning on MSVC (NOTE: breaks android build)" ON )
  if(ASSIMP_RAPIDJSON_NO_MEMBER_ITERATOR)
    ADD_DEFINITIONS( -DRAPIDJSON_NOMEMBERITERATORCLASS )
  endif()
ENDIF()

IF (ASSIMP_BUILD_DRACO)
  ADD_DEFINITIONS( -DASSIMP_ENABLE_DRACO )
ENDIF()

TARGET_USE_COMMON_OUTPUT_DIRECTORY(unit)

add_definitions(-DASSIMP_TEST_MODELS_DIR="${CMAKE_CURRENT_LIST_DIR}/models")
add_definitions(-DASSIMP_TEST_MODELS_NONBSD_DIR="${CMAKE_CURRENT_LIST_DIR}/models-nonbsd")

SET_PROPERTY( TARGET assimp PROPERTY DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )

IF( WIN32 )
    SET( platform_libs )
ELSEIF(ANDROID)
    SET( platform_libs )
ELSE()
    SET( platform_libs pthread )
ENDIF()

IF(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

IF (ASSIMP_WARNINGS_AS_ERRORS)
  IF (MSVC)
    TARGET_COMPILE_OPTIONS(unit PRIVATE /W4 /WX)
  ELSE()
    TARGET_COMPILE_OPTIONS(unit PRIVATE -Wall -Werror)
  ENDIF()
ENDIF()

target_link_libraries( unit assimp ${platform_libs} )

add_subdirectory(headercheck)

add_test( unittests unit )
