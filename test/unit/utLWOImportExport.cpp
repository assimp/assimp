/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

class utLWOImportExport : public AbstractImportExportBase {
public:
    virtual bool importerTest() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/boxuv.lwo", aiProcess_ValidateDataStructure);

        EXPECT_EQ(1u, scene->mNumMeshes);
        EXPECT_NE(nullptr, scene->mMeshes[0]);
        EXPECT_EQ(24u, scene->mMeshes[0]->mNumVertices);

        //This test model is using n-gons, so 6 faces instead of 12 tris
        EXPECT_EQ(6u, scene->mMeshes[0]->mNumFaces);
        EXPECT_EQ(aiPrimitiveType_POLYGON, scene->mMeshes[0]->mPrimitiveTypes);
        EXPECT_EQ(true, scene->mMeshes[0]->HasTextureCoords(0));

        return nullptr != scene;
    }
};

TEST_F(utLWOImportExport, importLWObox_uv) {
    EXPECT_TRUE(importerTest());
}

TEST_F(utLWOImportExport, importLWOformatdetection) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/formatDetection", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOempty) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/invalid/empty.lwo", aiProcess_ValidateDataStructure);

    EXPECT_EQ(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWObox_2uv_1unused) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/box_2uv_1unused.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWObox_2vc_1unused) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/box_2vc_1unused.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOconcave_polygon) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/concave_polygon.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOconcave_self_intersecting) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/concave_self_intersecting.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOhierarchy) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/hierarchy.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOhierarchy_smoothed) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/hierarchy_smoothed.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_cylindrical_x) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_cylindrical_x.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_cylindrical_x_scale_222_wrap_21) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_cylindrical_x_scale_222_wrap_21.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_cylindrical_y) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_cylindrical_y.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_cylindrical_y_scale_111) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_cylindrical_y_scale_111.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_cylindrical_y_scale_111_wrap_21) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_cylindrical_y_scale_111_wrap_21.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_cylindrical_z) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_cylindrical_z.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_planar_x) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_planar_x.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_planar_y) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_planar_y.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_planar_z) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_planar_z.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_planar_z_scale_111) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_planar_z_scale_111.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_spherical_x) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_spherical_x.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_spherical_x_scale_222_wrap_22) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_spherical_x_scale_222_wrap_22.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_spherical_y) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_spherical_y.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_spherical_) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_spherical_z.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_spherical_z_wrap_22) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_spherical_z_wrap_22.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOearth_uv_cylindrical_y) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/MappingModes/earth_uv_cylindrical_y.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOModoExport_vertNormals) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/ModoExport_vertNormals.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOnonplanar_polygon) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/nonplanar_polygon.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOCellShader) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/shader_test/CellShader.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOfastFresne) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/shader_test/fastFresnel.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOrealFresnel) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/shader_test/realFresnel.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOSuperCellShader) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/shader_test/SuperCellShader.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOsphere_with_gradient) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/sphere_with_gradient.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOsphere_with_mat_gloss_10pc) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/sphere_with_mat_gloss_10pc.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOSubdivision) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/Subdivision.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOtransparency) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/transparency.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOUglyVertexColors) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/UglyVertexColors.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOuvtest) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWO2/uvtest.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOBConcavePolygon) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWOB/ConcavePolygon.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOBbluewithcylindrictex) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWOB/MappingModes/bluewithcylindrictexz.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOBsphere_with_mat_gloss_10pc) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWOB/sphere_with_mat_gloss_10pc.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}


TEST_F(utLWOImportExport, importLWOBsphere_with_mat_gloss_50pc) {
    ::Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MODELS_DIR "/LWO/LWOB/sphere_with_mat_gloss_50pc.lwo", aiProcess_ValidateDataStructure);

    EXPECT_NE(nullptr, scene);
}
