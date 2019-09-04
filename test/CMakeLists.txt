# Open Asset Import Library (assimp)
# ----------------------------------------------------------------------
#
# Copyright (c) 2006-2019, assimp team


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
cmake_minimum_required( VERSION 3.0 )

INCLUDE_DIRECTORIES(
	${Assimp_SOURCE_DIR}/contrib/gtest/include
	${Assimp_SOURCE_DIR}/contrib/gtest/
    ${Assimp_SOURCE_DIR}/test/unit
    ${Assimp_SOURCE_DIR}/include
    ${Assimp_SOURCE_DIR}/code
)
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
  unit/Common/utLineSplitter.cpp
)

SET( IMPORTERS
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
  unit/utColladaExportCamera.cpp
  unit/utColladaExportLight.cpp
  unit/utColladaImportExport.cpp
  unit/utCSMImportExport.cpp
  unit/utB3DImportExport.cpp
  unit/utMDCImportExport.cpp
  unit/utAssbinImportExport.cpp
  unit/ImportExport/utAssjsonImportExport.cpp
  unit/ImportExport/utCOBImportExport.cpp
  unit/ImportExport/utOgreImportExport.cpp
  unit/ImportExport/utQ3BSPFileImportExport.cpp
  unit/ImportExport/utOFFImportExport.cpp
  unit/ImportExport/utNFFImportExport.cpp
  unit/ImportExport/utXGLImportExport.cpp
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

SOURCE_GROUP( UnitTests\\Compiler     FILES  unit/CCompilerTest.c )
SOURCE_GROUP( UnitTests\\Common       FILES  ${COMMON} )
SOURCE_GROUP( UnitTests\\ImportExport FILES  ${IMPORTERS} )
SOURCE_GROUP( UnitTests\\Material     FILES  ${MATERIAL} )
SOURCE_GROUP( UnitTests\\Math         FILES  ${MATH} )
SOURCE_GROUP( UnitTests\\PostProcess  FILES  ${POST_PROCESSES})

add_executable( unit
    ../contrib/gtest/src/gtest-all.cc
    unit/CCompilerTest.c
    unit/Main.cpp
    ../code/Common/Version.cpp
	${COMMON}
	${IMPORTERS}
	${MATERIAL}
	${MATH}
    ${POST_PROCESSES}
)

add_definitions(-DASSIMP_TEST_MODELS_DIR="${CMAKE_CURRENT_LIST_DIR}/models")
add_definitions(-DASSIMP_TEST_MODELS_NONBSD_DIR="${CMAKE_CURRENT_LIST_DIR}/models-nonbsd")

SET_PROPERTY( TARGET assimp PROPERTY DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )

IF( WIN32 )
    SET( platform_libs )
ELSE( WIN32 )
    SET( platform_libs pthread )
ENDIF( WIN32 )

IF( WIN32 )
  ADD_CUSTOM_COMMAND(TARGET unit
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:assimp> $<TARGET_FILE_DIR:unit>
    MAIN_DEPENDENCY assimp)
ENDIF( WIN32 )

IF(MSVC)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(MSVC)

target_link_libraries( unit assimp ${platform_libs} )

add_subdirectory(headercheck)

add_test( unittests unit )

