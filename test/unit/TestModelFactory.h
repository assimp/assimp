#pragma once

#include "UnitTestPCH.h"
#include <assimp/scene.h>
#include <assimp/material.h>

namespace Assimp {

class TestModelFacttory {
public:
    TestModelFacttory() {
        // empty
    }

    ~TestModelFacttory() {
        // empty
    }

    static aiScene *createDefaultTestModel( float &opacity  ) {
        aiScene *scene( new aiScene );
        scene->mNumMaterials = 1;
        scene->mMaterials = new aiMaterial*;
        scene->mMaterials[ 0 ] = new aiMaterial;
        aiColor3D color( 1, 0, 0 );
        EXPECT_EQ( AI_SUCCESS, scene->mMaterials[ 0 ]->AddProperty( &color, 1, AI_MATKEY_COLOR_DIFFUSE ) );

        ::srand( static_cast< unsigned int >( ::time( NULL ) ) );
        opacity = float( rand() ) / float( RAND_MAX );
        EXPECT_EQ( AI_SUCCESS, scene->mMaterials[ 0 ]->AddProperty( &opacity, 1, AI_MATKEY_OPACITY ) );

        scene->mNumMeshes = 1;
        scene->mMeshes = new aiMesh*;
        scene->mMeshes[ 0 ] = new aiMesh;
        scene->mMeshes[ 0 ]->mMaterialIndex = 0;
        scene->mMeshes[ 0 ]->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
        scene->mMeshes[ 0 ]->mNumVertices = 3;
        scene->mMeshes[ 0 ]->mVertices = new aiVector3D[ 3 ];
        scene->mMeshes[ 0 ]->mVertices[ 0 ] = aiVector3D( 1, 0, 0 );
        scene->mMeshes[ 0 ]->mVertices[ 1 ] = aiVector3D( 0, 1, 0 );
        scene->mMeshes[ 0 ]->mVertices[ 2 ] = aiVector3D( 0, 0, 1 );
        scene->mMeshes[ 0 ]->mNumFaces = 1;
        scene->mMeshes[ 0 ]->mFaces = new aiFace;
        scene->mMeshes[ 0 ]->mFaces[ 0 ].mNumIndices = 3;
        scene->mMeshes[ 0 ]->mFaces[ 0 ].mIndices = new unsigned int[ 3 ];
        scene->mMeshes[ 0 ]->mFaces[ 0 ].mIndices[ 0 ] = 0;
        scene->mMeshes[ 0 ]->mFaces[ 0 ].mIndices[ 1 ] = 1;
        scene->mMeshes[ 0 ]->mFaces[ 0 ].mIndices[ 2 ] = 2;

        scene->mRootNode = new aiNode;
        scene->mRootNode->mNumMeshes = 1;
        scene->mRootNode->mMeshes = new unsigned int( 0 );

        return scene;
    }
};

}
