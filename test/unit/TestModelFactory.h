/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team



All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/
#pragma once

#include "UnitTestPCH.h"
#include <assimp/scene.h>
#include <assimp/mesh.h>
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

    static aiScene *createDefaultTestModel( float &opacity ) {
        aiScene *scene( new aiScene );
        scene->mNumMaterials = 1;
        scene->mMaterials = new aiMaterial*[scene->mNumMaterials];
        scene->mMaterials[ 0 ] = new aiMaterial;
        aiColor3D color( 1, 0, 0 );
        EXPECT_EQ( AI_SUCCESS, scene->mMaterials[ 0 ]->AddProperty( &color, 1, AI_MATKEY_COLOR_DIFFUSE ) );

        ::srand( static_cast< unsigned int >( ::time( NULL ) ) );
        opacity = float( rand() ) / float( RAND_MAX );
        EXPECT_EQ( AI_SUCCESS, scene->mMaterials[ 0 ]->AddProperty( &opacity, 1, AI_MATKEY_OPACITY ) );

        scene->mNumMeshes = 1;
        scene->mMeshes = new aiMesh*[scene->mNumMeshes];
        scene->mMeshes[ 0 ] = new aiMesh;
        scene->mMeshes[ 0 ]->mMaterialIndex = 0;
        scene->mMeshes[ 0 ]->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
        scene->mMeshes[ 0 ]->mNumVertices = 3;
        scene->mMeshes[ 0 ]->mVertices = new aiVector3D[ 3 ];
        scene->mMeshes[ 0 ]->mVertices[ 0 ] = aiVector3D( 1, 0, 0 );
        scene->mMeshes[ 0 ]->mVertices[ 1 ] = aiVector3D( 0, 1, 0 );
        scene->mMeshes[ 0 ]->mVertices[ 2 ] = aiVector3D( 0, 0, 1 );
        scene->mMeshes[ 0 ]->mNumFaces = 1;
        scene->mMeshes[ 0 ]->mFaces = new aiFace[scene->mMeshes[ 0 ]->mNumFaces];
        scene->mMeshes[ 0 ]->mFaces[ 0 ].mNumIndices = 3;
        scene->mMeshes[ 0 ]->mFaces[ 0 ].mIndices = new unsigned int[ 3 ];
        scene->mMeshes[ 0 ]->mFaces[ 0 ].mIndices[ 0 ] = 0;
        scene->mMeshes[ 0 ]->mFaces[ 0 ].mIndices[ 1 ] = 1;
        scene->mMeshes[ 0 ]->mFaces[ 0 ].mIndices[ 2 ] = 2;

        scene->mRootNode = new aiNode;
        scene->mRootNode->mNumMeshes = 1;
        scene->mRootNode->mMeshes = new unsigned int[1]{ 0 };

        return scene;
    }

    static void releaseDefaultTestModel( aiScene **scene ) {
        delete *scene;
        *scene = nullptr;
    }
};

}
