# Open Asset Import Library (assimp)
# ----------------------------------------------------------------------
#
# Copyright (c) 2006-2016, assimp team
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
cmake_minimum_required( VERSION 2.6 )

#INCLUDE( AddGTest )

INCLUDE_DIRECTORIES(
	../contrib/gtest/include
	../contrib/gtest/
    ${Assimp_SOURCE_DIR}/include
    ${Assimp_SOURCE_DIR}/code
)

# Add the temporary output directories to the library path to make sure the
# Assimp library can be found, even if it is not installed system-wide yet.
LINK_DIRECTORIES( ${Assimp_BINARY_DIR} ${AssetImporter_BINARY_DIR}/lib )

SOURCE_GROUP( unit FILES
  unit/CCompilerTest.c
)

SET( TEST_SRCS
  unit/AbstractImportExportBase.cpp
  unit/TestIOSystem.h
  unit/TestModelFactory.h
  unit/ut3DImportExport.cpp
  unit/ut3DSImportExport.cpp
  unit/utACImportExport.cpp
  unit/utAMFImportExport.cpp
  unit/utASEImportExport.cpp  
  unit/utAnim.cpp
  unit/AssimpAPITest.cpp
  unit/utB3DImportExport.cpp
  unit/utBatchLoader.cpp
  unit/utBlenderIntermediate.cpp
  unit/utBlendImportAreaLight.cpp
  unit/utBlenderImportExport.cpp
  unit/utBlendImportMaterials.cpp
  unit/utBVHImportExport.cpp
  unit/utColladaExportCamera.cpp
  unit/utColladaExportLight.cpp
  unit/utColladaImportExport.cpp
  unit/utCSMImportExport.cpp
  unit/utDefaultIOStream.cpp
  unit/utDXFImporterExporter.cpp
  unit/utFastAtof.cpp
  unit/utFBXImporterExporter.cpp
  unit/utFindDegenerates.cpp
  unit/utFindInvalidData.cpp
  unit/utFixInfacingNormals.cpp
  unit/utGenNormals.cpp
  unit/utglTFImportExport.cpp
  unit/utHMPImportExport.cpp
  unit/utIFCImportExport.cpp
  unit/utImporter.cpp
  unit/utImproveCacheLocality.cpp
  unit/utIOSystem.cpp
  unit/utIOStreamBuffer.cpp
  unit/utIssues.cpp
  unit/utJoinVertices.cpp
  unit/utLimitBoneWeights.cpp
  unit/utLWSImportExport.cpp
  unit/utMaterialSystem.cpp
  unit/utMatrix3x3.cpp
  unit/utMatrix4x4.cpp
  unit/utMetadata.cpp
  unit/SceneDiffer.h
  unit/SceneDiffer.cpp
  unit/utSIBImporter.cpp
  unit/utObjImportExport.cpp
  unit/utPretransformVertices.cpp
  unit/utPLYImportExport.cpp
  unit/utRemoveComments.cpp
  unit/utRemoveComponent.cpp
  unit/utRemoveRedundantMaterials.cpp
  unit/utScenePreprocessor.cpp
  unit/utSharedPPData.cpp
  unit/utStringUtils.cpp
  unit/utSMDImportExport.cpp
  unit/utSortByPType.cpp
  unit/utSplitLargeMeshes.cpp
  unit/utTargetAnimation.cpp
  unit/utTextureTransform.cpp
  unit/utTriangulate.cpp
  unit/utTypes.cpp
  unit/utVertexTriangleAdjacency.cpp
  unit/utVersion.cpp
  unit/utXImporterExporter.cpp
)

SOURCE_GROUP( tests FILES  ${TEST_SRCS} )

add_executable( unit
    ../contrib/gtest/src/gtest-all.cc
    unit/CCompilerTest.c
    unit/Main.cpp
    ../code/Version.cpp
    ${TEST_SRCS}
)

add_definitions(-DASSIMP_TEST_MODELS_DIR="${CMAKE_CURRENT_LIST_DIR}/models")
    
SET_PROPERTY( TARGET assimp PROPERTY DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )

IF( WIN32 )
    SET( platform_libs )
ELSE( WIN32 )
    SET( platform_libs pthread )
ENDIF( WIN32 )

IF(MSVC)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(MSVC)

target_link_libraries( unit assimp ${platform_libs} )

add_subdirectory(headercheck)
#if (ASSIMP_COVERALLS)
#    include(Coveralls)
#
#    set(COVERAGE_SRCS ${assimp_src} ${TEST_SRCS} )
#
#    # Create the coveralls target.
#    coveralls_setup(
#        "${COVERAGE_SRCS}" # The source files.
#        ON                 # If we should upload.
#        "${PROJECT_SOURCE_DIR}/cmake-modules/") # (Optional) Alternate project cmake module path.
#endif()

