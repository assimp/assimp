/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2017, assimp team


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

#include <assimp/cexport.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class BlendImportAreaLight : public ::testing::Test {
public:

    virtual void SetUp()
    {
        im = new Assimp::Importer();
    }

    virtual void TearDown()
    {
        delete im;
    }

protected:

    Assimp::Importer* im;
};

// ------------------------------------------------------------------------------------------------
TEST_F(BlendImportAreaLight, testImportLight)
{
    const aiScene* pTest = im->ReadFile(ASSIMP_TEST_MODELS_DIR "/BLEND/AreaLight_269.blend", aiProcess_ValidateDataStructure);
    ASSERT_TRUE(pTest != NULL);
    ASSERT_TRUE(pTest->HasLights());

    std::vector< std::pair<std::string, size_t> > lightNames;

    for (size_t i = 0; i < pTest->mNumLights; i++) {
        lightNames.push_back(std::make_pair(pTest->mLights[i]->mName.C_Str(), i));
    }

    std::sort(lightNames.begin(), lightNames.end());

    std::vector<aiLight> lights;

    for (size_t i = 0; i < pTest->mNumLights; ++i) {
        lights.push_back(*pTest->mLights[lightNames[i].second]);
    }

    ASSERT_STREQ(lights[0].mName.C_Str(), "Bar");
    ASSERT_STREQ(lights[1].mName.C_Str(), "Baz");
    ASSERT_STREQ(lights[2].mName.C_Str(), "Foo");

    ASSERT_EQ(lights[0].mType, aiLightSource_AREA);
    ASSERT_EQ(lights[1].mType, aiLightSource_POINT);
    ASSERT_EQ(lights[2].mType, aiLightSource_AREA);

    EXPECT_FLOAT_EQ(lights[0].mSize.x, 0.5f);
    EXPECT_FLOAT_EQ(lights[0].mSize.y, 2.0f);
    EXPECT_FLOAT_EQ(lights[2].mSize.x, 1.0f);
    EXPECT_FLOAT_EQ(lights[2].mSize.y, 1.0f);

    EXPECT_FLOAT_EQ(lights[0].mColorDiffuse.r, 42.0f);
    EXPECT_FLOAT_EQ(lights[0].mColorDiffuse.g, 42.0f);
    EXPECT_FLOAT_EQ(lights[0].mColorDiffuse.b, 42.0f);
    EXPECT_FLOAT_EQ(lights[2].mColorDiffuse.r, 1.0f);
    EXPECT_FLOAT_EQ(lights[2].mColorDiffuse.g, 1.0f);
    EXPECT_FLOAT_EQ(lights[2].mColorDiffuse.b, 1.0f);

    EXPECT_FLOAT_EQ(lights[0].mDirection.x, 0.0f);
    EXPECT_FLOAT_EQ(lights[0].mDirection.y, 0.0f);
    EXPECT_FLOAT_EQ(lights[0].mDirection.z, -1.0f);
    EXPECT_FLOAT_EQ(lights[2].mDirection.x, 0.0f);
    EXPECT_FLOAT_EQ(lights[2].mDirection.y, 0.0f);
    EXPECT_FLOAT_EQ(lights[2].mDirection.z, -1.0f);
}
