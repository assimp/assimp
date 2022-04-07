/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team



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
#include <assimp/Importer.hpp>

using namespace Assimp;

class utBlenderImporterExporter : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/box.blend", aiProcess_ValidateDataStructure);
        return nullptr != scene;
    }
};

TEST_F(utBlenderImporterExporter, importBlenFromFileTest) {
    EXPECT_TRUE(importerTest());
}

TEST(utBlenderImporter, import4cubes) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/4Cubes4Mats_248.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, import269_regress1) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/blender_269_regress1.blend", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utBlenderImporter, importBlenderDefault248) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/BlenderDefault_248.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importBlenderDefault250) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/BlenderDefault_250.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importBlenderDefault250Compressed) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/BlenderDefault_250_Compressed.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importBlenderDefault262) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/BlenderDefault_262.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importBlenderDefault269) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/BlenderDefault_269.blend", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utBlenderImporter, importBlenderDefault271) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/BlenderDefault_271.blend", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utBlenderImporter, importBlenderDefault293) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/BlenderDefault_276.blend", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}

TEST(utBlenderImporter, importCubeHierarchy_248) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/CubeHierarchy_248.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importHuman) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/HUMAN.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importMirroredCube_252) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/MirroredCube_252.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importNoisyTexturedCube_VoronoiGlob_248) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/NoisyTexturedCube_VoronoiGlob_248.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importSmoothVsSolidCube_248) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/SmoothVsSolidCube_248.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importSuzanne_248) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/Suzanne_248.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importSuzanneSubdiv_252) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/SuzanneSubdiv_252.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importTexturedCube_ImageGlob_248) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/TexturedCube_ImageGlob_248.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importTexturedPlane_ImageUv_248) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/TexturedPlane_ImageUv_248.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importTexturedPlane_ImageUvPacked_248) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/TexturedPlane_ImageUvPacked_248.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importTorusLightsCams_250_compressed) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/TorusLightsCams_250_compressed.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, import_yxa_1) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/yxa_1.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importBob) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/BLEND/Bob.blend", aiProcess_ValidateDataStructure);
    // FIXME: this is probably not right, loading this should succeed
    ASSERT_EQ(nullptr, scene);
}

TEST(utBlenderImporter, importFleurOptonl) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_NONBSD_DIR "/BLEND/fleurOptonl.blend", aiProcess_ValidateDataStructure);
    ASSERT_NE(nullptr, scene);
}
