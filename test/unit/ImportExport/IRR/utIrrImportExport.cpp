/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

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
#include "AbstractImportExportBase.h"
#include "UnitTestPCH.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

class utIrrImportExport : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/box.irr", aiProcess_ValidateDataStructure);
        // Only one box thus only one mesh
        return nullptr != scene && scene->mNumMeshes == 1;
    }
};


TEST_F(utIrrImportExport, importSimpleIrrTest) {
    EXPECT_TRUE(importerTest());
}


TEST_F(utIrrImportExport, importAnimMesh) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/animMesh.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importAnimMeshUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/animMesh_UTF16LE.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importBox) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/box.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importBoxUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/box_UTF16LE.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importCellar) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRRMesh/cellar.irrmesh", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importCellarUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRRMesh/cellar_UTF16LE.irrmesh", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importDawfInCellar) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/dawfInCellar_ChildOfCellar.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importDawfInCellarUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/dawfInCellar_ChildOfCellar_UTF16LE.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importSGIrrTest) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/dawfInCellar_SameHierarchy.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(scene->mNumMeshes, 4u);
    EXPECT_EQ(scene->mNumMaterials, 5u);
    ASSERT_GT(scene->mNumMeshes, 0u);
    EXPECT_GT(scene->mMeshes[0]->mNumVertices, 0u);
}


TEST_F(utIrrImportExport, importSGIrrTestUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/dawfInCellar_SameHierarchy_UTF16LE.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
    EXPECT_EQ(scene->mNumMeshes, 4u);
    EXPECT_EQ(scene->mNumMaterials, 5u);
    ASSERT_GT(scene->mNumMeshes, 0u);
    EXPECT_GT(scene->mMeshes[0]->mNumVertices, 0u);
}


TEST_F(utIrrImportExport, importANewDwarf) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/EpisodeI_ANewDwarf.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importANewDwarfUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/EpisodeI_ANewDwarf_UTF16LE.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importDwarfStrikesBack) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/EpisodeII_TheDwarfesStrikeBack.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importDwarfStrikesBackUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/EpisodeII_TheDwarfesStrikeBack_UTF16LE.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importInstancing) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/instancing.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importMultipleAnimators) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/multipleAnimators.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importMultipleAnimatorsUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/multipleAnimators_UTF16LE.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importScenegraphAnim) {
    Assimp::Importer importer;
    // FIXME: this fails but probably shouldn't
    // Validation failed: Empty node animation channel
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/scenegraphAnim.irr", aiProcess_ValidateDataStructure);
    ASSERT_EQ(nullptr, scene);

    Assimp::Importer importer2;
    const aiScene *scene2 = importer2.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/scenegraphAnim.irr", 0);
    ASSERT_NE(nullptr, scene2);
}


TEST_F(utIrrImportExport, importScenegraphAnimUTF16) {
    Assimp::Importer importer;
    // FIXME: this fails but probably shouldn't
    // Validation failed: Empty node animation channel
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/scenegraphAnim_UTF16LE.irr", aiProcess_ValidateDataStructure);
    ASSERT_EQ(nullptr, scene);

    Assimp::Importer importer2;
    const aiScene *scene2 = importer2.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/scenegraphAnim_UTF16LE.irr", 0);
    ASSERT_NE(nullptr, scene2);
}


TEST_F(utIrrImportExport, importScenegraphAnimMod) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/scenegraphAnimMod.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importScenegraphAnimModUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/scenegraphAnimMod_UTF16LE.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importSphere) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/sphere.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importSphereUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRR/sphere_UTF16LE.irr", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importSpider) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRRMesh/spider.irrmesh", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importSpiderUTF166) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/IRRMesh/spider_UTF16LE.irrmesh", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importSkybox) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/IRR/skybox.xml", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}


TEST_F(utIrrImportExport, importSkyboxUTF16) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/IRR/skybox_UTF16LE.xml", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}
