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

INCLUDE( AddGTest )

INCLUDE_DIRECTORIES(
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
  unit/AssimpAPITest.cpp
  unit/utBlendImportAreaLight.cpp
  unit/utBlendImportMaterials.cpp
  unit/utColladaExportCamera.cpp
  unit/utColladaExportLight.cpp
  unit/utFastAtof.cpp
  unit/utFindDegenerates.cpp
  unit/utFindInvalidData.cpp
  unit/utFixInfacingNormals.cpp
  unit/utGenNormals.cpp
  unit/utImporter.cpp
  unit/utImproveCacheLocality.cpp
  unit/utIOSystem.cpp
  unit/utIssues.cpp
  unit/utJoinVertices.cpp
  unit/utLimitBoneWeights.cpp
  unit/utMaterialSystem.cpp
  unit/utMatrix3x3.cpp
  unit/utMatrix4x4.cpp
  unit/utPretransformVertices.cpp
  unit/utRemoveComments.cpp
  unit/utRemoveComponent.cpp
  unit/utRemoveRedundantMaterials.cpp
  unit/utScenePreprocessor.cpp
  unit/utSharedPPData.cpp
  unit/utStringUtils.cpp
  unit/utSortByPType.cpp
  unit/utSplitLargeMeshes.cpp
  unit/utTargetAnimation.cpp
  unit/utTextureTransform.cpp
  unit/utTriangulate.cpp
  unit/utVertexTriangleAdjacency.cpp
)

SOURCE_GROUP( tests FILES  ${TEST_SRCS} )

if(AddGTest_FOUND)
    add_executable( unit
      unit/CCompilerTest.c
      unit/Main.cpp
      ../code/Version.cpp
      ${TEST_SRCS}
    )

    add_definitions(-DASSIMP_TEST_MODELS_DIR="${CMAKE_CURRENT_LIST_DIR}/models")

    SET_PROPERTY( TARGET assimp PROPERTY DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )

    add_dependencies( unit gtest )
    target_link_libraries( unit assimp
      debug ${GTEST_DEBUG_LIBRARIES}
      optimized ${GTEST_RELEASE_LIBRARIES}
    )
endif(AddGTest_FOUND)
add_subdirectory(headercheck)
