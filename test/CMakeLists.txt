# Open Asset Import Library (assimp)
# ----------------------------------------------------------------------
# Copyright (c) 2006-2026, assimp team
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

# Add the temporary output directories to the library path to make sure the
# Assimp library can be found, even if it is not installed system-wide yet.
LINK_DIRECTORIES( ${Assimp_BINARY_DIR} ${AssetImporter_BINARY_DIR}/lib )

set(TOOLS
    unit/Tools/TestTools.h
)

set( COMMON
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
  unit/Common/utParsingUtils.cpp
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

set(Geometry 
    unit/Geometry/utGeometryUtils.cpp
)

set( IMPORTERS
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
  unit/utCSMImportExport.cpp
  unit/utB3DImportExport.cpp
  #unit/utM3DImportExport.cpp
  unit/utMDCImportExport.cpp
  unit/utAssbinImportExport.cpp
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

if(ASSIMP_BUILD_USD_IMPORTER)
  list( APPEND IMPORTERS
    unit/utUSDImport.cpp
  )
endif()

if(ASSIMP_BUILD_COLLADA_EXPORTER)
  list( APPEND IMPORTERS
    unit/utColladaExport.cpp
  )
endif()

if(ASSIMP_BUILD_COLLADA_IMPORTER)
  list( APPEND IMPORTERS
    unit/utColladaImportExport.cpp
  )
endif()

set( MATERIAL
  unit/utMaterialSystem.cpp
)

set( MATH
  unit/utMatrix3x3.cpp
  unit/utMatrix4x4.cpp
  unit/utVector3.cpp
)

set( POST_PROCESSES
  unit/Postprocessing/utImproveCacheLocality.cpp
  unit/Postprocessing/utFixInfacingNormals.cpp
  unit/Postprocessing/utGenNormals.cpp
  unit/Postprocessing/utTriangulate.cpp
  unit/Postprocessing/utTextureTransform.cpp
  unit/Postprocessing/utRemoveRedundantMaterials.cpp
  unit/Postprocessing/utRemoveVCProcess.cpp
  unit/Postprocessing/utScaleProcess.cpp
  unit/Postprocessing/utArmaturePopulate.cpp
  unit/Postprocessing/utJoinVertices.cpp
  unit/Postprocessing/utRemoveComments.cpp
  unit/Postprocessing/utRemoveComponent.cpp
  unit/Postprocessing/utVertexTriangleAdjacency.cpp
  unit/Postprocessing/utJoinVertices.cpp
  unit/Postprocessing/utSplitLargeMeshes.cpp
  unit/Postprocessing/utFindDegenerates.cpp
  unit/Postprocessing/utFindInvalidData.cpp
  unit/Postprocessing/utLimitBoneWeights.cpp
  unit/Postprocessing/utPretransformVertices.cpp
  unit/Postprocessing/utScenePreprocessor.cpp
  unit/Postprocessing/utTargetAnimation.cpp
  unit/Postprocessing/utSortByPType.cpp
  unit/Postprocessing/utSceneCombiner.cpp
  unit/Postprocessing/utGenBoundingBoxesProcess.cpp
  unit/Postprocessing/utGenerateMikkTSpaceTangents.cpp
)

source_group( UnitTests\\Compiler      FILES unit/CCompilerTest.c )
source_group( UnitTests\\Common        FILES ${COMMON} )
source_group( UnitTests\\GeometryTools FILES ${Geometry} )
source_group( UnitTests\\ImportExport  FILES ${IMPORTERS} )
source_group( UnitTests\\Material      FILES ${MATERIAL} )
source_group( UnitTests\\Math          FILES ${MATH} )
source_group( UnitTests\\Tools         FILES ${TOOLS} )
source_group( UnitTests\\PostProcess   FILES ${POST_PROCESSES})

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
    ${TOOLS}
)

if(ASSIMP_HUNTER_ENABLED)
    hunter_add_package(GTest)
    find_package(GTest CONFIG REQUIRED)
    target_link_libraries(unit GTest::gtest_main GTest::gmock)
else()
    target_sources(unit PUBLIC ${Assimp_SOURCE_DIR}/contrib/googletest/googletest/src/gtest-all.cc)
endif()

# RapidJSON
if(ASSIMP_HUNTER_ENABLED)
  hunter_add_package(RapidJSON)
  find_package(RapidJSON CONFIG REQUIRED)
else()
  INCLUDE_DIRECTORIES("../contrib/rapidjson/include")
  ADD_DEFINITIONS( -DRAPIDJSON_HAS_STDSTRING=1)
  option( ASSIMP_RAPIDJSON_NO_MEMBER_ITERATOR "Suppress rapidjson warning on MSVC (NOTE: breaks android build)" ON )
  if(ASSIMP_RAPIDJSON_NO_MEMBER_ITERATOR)
    ADD_DEFINITIONS( -DRAPIDJSON_NOMEMBERITERATORCLASS )
  endif()
endif()

if (ASSIMP_BUILD_DRACO)
  ADD_DEFINITIONS( -DASSIMP_ENABLE_DRACO )
endif()

TARGET_USE_COMMON_OUTPUT_DIRECTORY(unit)

add_definitions(-DASSIMP_TEST_MODELS_DIR="${CMAKE_CURRENT_LIST_DIR}/models")
add_definitions(-DASSIMP_TEST_MODELS_NONBSD_DIR="${CMAKE_CURRENT_LIST_DIR}/models-nonbsd")

set_property( TARGET assimp PROPERTY DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )

if( WIN32 )
    SET( platform_libs )
elseif(ANDROID)
    SET( platform_libs )
else()
    SET( platform_libs pthread )
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
endif()

if (ASSIMP_WARNINGS_AS_ERRORS)
  if (MSVC)
    TARGET_COMPILE_OPTIONS(unit PRIVATE /W4 /WX)
  else()
    TARGET_COMPILE_OPTIONS(unit PRIVATE -Wall -Werror)
  endif()
endif()

target_link_libraries( unit assimp ${platform_libs} )

add_subdirectory(headercheck)

add_test( unittests unit )
