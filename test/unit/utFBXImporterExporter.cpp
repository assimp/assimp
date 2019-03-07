/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team



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

#include "UnitTestPCH.h"
#include "SceneDiffer.h"
#include "AbstractImportExportBase.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/types.h>

using namespace Assimp;

class utFBXImporterExporter : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/FBX/spider.fbx", aiProcess_ValidateDataStructure );
        return nullptr != scene;
    }
};

TEST_F( utFBXImporterExporter, importXFromFileTest ) {
    EXPECT_TRUE( importerTest() );
}

TEST_F( utFBXImporterExporter, importBareBoxWithoutColorsAndTextureCoords ) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/FBX/box.fbx", aiProcess_ValidateDataStructure );
    EXPECT_NE( nullptr, scene );
    EXPECT_EQ(scene->mNumMeshes, 1);
    aiMesh* mesh = scene->mMeshes[0];
    EXPECT_EQ(mesh->mNumFaces, 12);
    EXPECT_EQ(mesh->mNumVertices, 36);
}

TEST_F( utFBXImporterExporter, importPhongMaterial ) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile( ASSIMP_TEST_MODELS_DIR "/FBX/phong_cube.fbx", aiProcess_ValidateDataStructure );
    EXPECT_NE( nullptr, scene );
    EXPECT_EQ( (unsigned int)1, scene->mNumMaterials );
    const aiMaterial *mat = scene->mMaterials[0];
    EXPECT_NE( nullptr, mat );
    float f; aiColor3D c;
    // phong_cube.fbx has all properties defined
    EXPECT_EQ( mat->Get(AI_MATKEY_COLOR_DIFFUSE, c), aiReturn_SUCCESS );
    EXPECT_EQ( c, aiColor3D(0.5, 0.25, 0.25) );
    EXPECT_EQ( mat->Get(AI_MATKEY_COLOR_SPECULAR, c), aiReturn_SUCCESS );
    EXPECT_EQ( c, aiColor3D(0.25, 0.25, 0.5) );
    EXPECT_EQ( mat->Get(AI_MATKEY_SHININESS_STRENGTH, f), aiReturn_SUCCESS );
    EXPECT_EQ( f, 0.5 );
    EXPECT_EQ( mat->Get(AI_MATKEY_SHININESS, f), aiReturn_SUCCESS );
    EXPECT_EQ( f, 10.0 );
    EXPECT_EQ( mat->Get(AI_MATKEY_COLOR_AMBIENT, c), aiReturn_SUCCESS );
    EXPECT_EQ( c, aiColor3D(0.125, 0.25, 0.25) );
    EXPECT_EQ( mat->Get(AI_MATKEY_COLOR_EMISSIVE, c), aiReturn_SUCCESS );
    EXPECT_EQ( c, aiColor3D(0.25, 0.125, 0.25) );
    EXPECT_EQ( mat->Get(AI_MATKEY_COLOR_TRANSPARENT, c), aiReturn_SUCCESS );
    EXPECT_EQ( c, aiColor3D(0.75, 0.5, 0.25) );
    EXPECT_EQ( mat->Get(AI_MATKEY_OPACITY, f), aiReturn_SUCCESS );
    EXPECT_EQ( f, 0.5 );
}

TEST_F(utFBXImporterExporter, importUnitScaleFactor) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/FBX/global_settings.fbx", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
    EXPECT_NE(nullptr, scene->mMetaData);

    double factor(0.0);
    scene->mMetaData->Get("UnitScaleFactor", factor);
    EXPECT_DOUBLE_EQ(500.0, factor);
}
