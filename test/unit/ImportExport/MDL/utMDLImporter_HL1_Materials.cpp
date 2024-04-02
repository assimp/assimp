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

/** @file utMDLImporter_HL1_Materials.cpp
 *  @brief Half-Life 1 MDL loader materials tests.
 */

#include "AbstractImportExportBase.h"
#include "AssetLib/MDL/HalfLife/HL1ImportDefinitions.h"
#include "MDLHL1TestFiles.h"
#include "UnitTestPCH.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

using namespace Assimp;

class utMDLImporter_HL1_Materials : public ::testing::Test {

public:
    /* Given an MDL model with a texture flagged as flatshade,
       verify that the imported model has a flat shading model. */
    void flatShadeTexture() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "chrome_sphere.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);
        ASSERT_NE(nullptr, scene->mMaterials);

        aiShadingMode shading_mode = aiShadingMode_Flat;
        scene->mMaterials[0]->Get(AI_MATKEY_SHADING_MODEL, shading_mode);
        EXPECT_EQ(aiShadingMode_Flat, shading_mode);
    }

    /* Given an MDL model with a chrome texture, verify that
       the imported model has a chrome material. */
    void chromeTexture() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "chrome_sphere.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);
        ASSERT_NE(nullptr, scene->mMaterials);

        int chrome;
        scene->mMaterials[0]->Get(AI_MDL_HL1_MATKEY_CHROME(aiTextureType_DIFFUSE, 0), chrome);
        EXPECT_EQ(1, chrome);
    }

    /* Given an MDL model with an additive texture, verify that
       the imported model has an additive material. */
    void additiveBlendTexture() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "blend_additive.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);
        ASSERT_NE(nullptr, scene->mMaterials);

        aiBlendMode blend_mode = aiBlendMode_Default;
        scene->mMaterials[0]->Get(AI_MATKEY_BLEND_FUNC, blend_mode);
        EXPECT_EQ(aiBlendMode_Additive, blend_mode);
    }

    /* Given an MDL model with a color masked texture, verify that
       the imported model has a color masked material. Ensure too
       that the transparency color is the correct one. */
    void textureWithColorMask() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(ASSIMP_TEST_MDL_HL1_MODELS_DIR "alpha_test.mdl", aiProcess_ValidateDataStructure);
        ASSERT_NE(nullptr, scene);
        ASSERT_NE(nullptr, scene->mMaterials);

        int texture_flags = 0;
        scene->mMaterials[0]->Get(AI_MATKEY_TEXFLAGS_DIFFUSE(0), texture_flags);
        EXPECT_EQ(aiTextureFlags_UseAlpha, texture_flags);

        // The model has only one texture, a 256 color bitmap with
        // a palette. Pure blue is the last color in the palette,
        // and should be the transparency color.
        aiColor3D transparency_color = {};
        scene->mMaterials[0]->Get(AI_MATKEY_COLOR_TRANSPARENT, transparency_color);
        EXPECT_EQ(aiColor3D(0, 0, 255), transparency_color);
    }
};

TEST_F(utMDLImporter_HL1_Materials, flatShadeTexture) {
    flatShadeTexture();
}

TEST_F(utMDLImporter_HL1_Materials, chromeTexture) {
    chromeTexture();
}

TEST_F(utMDLImporter_HL1_Materials, additiveBlendTexture) {
    additiveBlendTexture();
}

TEST_F(utMDLImporter_HL1_Materials, textureWithColorMask) {
    textureWithColorMask();
}
