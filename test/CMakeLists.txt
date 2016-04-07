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
  unit/utFastAtof.cpp
  unit/utFindDegenerates.cpp
  unit/utFindInvalidData.cpp
  unit/utFixInfacingNormals.cpp
  unit/utGenNormals.cpp
  unit/utImporter.cpp
  unit/utImproveCacheLocality.cpp
  unit/utIOSystem.cpp
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
  unit/utSortByPType.cpp
  unit/utSplitLargeMeshes.cpp
  unit/utTargetAnimation.cpp
  unit/utTextureTransform.cpp
  unit/utTriangulate.cpp
  unit/utVertexTriangleAdjacency.cpp
  unit/utColladaExportCamera.cpp
  unit/utColladaExportLight.cpp
  unit/utIssues.cpp
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
