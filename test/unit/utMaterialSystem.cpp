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
#include "UnitTestPCH.h"

#include "Material/MaterialSystem.h"
#include <assimp/scene.h>

using namespace ::std;
using namespace ::Assimp;

class MaterialSystemTest : public ::testing::Test {
public:
    virtual void SetUp() { this->pcMat = new aiMaterial(); }
    virtual void TearDown() { delete this->pcMat; }

protected:
    aiMaterial *pcMat;
};

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testFloatProperty) {
    float pf = 150392.63f;
    this->pcMat->AddProperty(&pf, 1, "testKey1");
    pf = 0.0f;

    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey1", 0, 0, pf));
    EXPECT_EQ(150392.63f, pf);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testFloatArrayProperty) {
    float pf[] = { 0.0f, 1.0f, 2.0f, 3.0f };
    unsigned int pMax = sizeof(pf) / sizeof(float);
    this->pcMat->AddProperty(pf, pMax, "testKey2");
    pf[0] = pf[1] = pf[2] = pf[3] = 12.0f;

    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey2", 0, 0, pf, &pMax));
    EXPECT_EQ(sizeof(pf) / sizeof(float), static_cast<size_t>(pMax));
    EXPECT_TRUE(!pf[0] && 1.0f == pf[1] && 2.0f == pf[2] && 3.0f == pf[3]);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testIntProperty) {
    int pf = 15039263;
    this->pcMat->AddProperty(&pf, 1, "testKey3");
    pf = 12;

    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey3", 0, 0, pf));
    EXPECT_EQ(15039263, pf);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testIntArrayProperty) {
    int pf[] = { 0, 1, 2, 3 };
    unsigned int pMax = sizeof(pf) / sizeof(int);
    this->pcMat->AddProperty(pf, pMax, "testKey4");
    pf[0] = pf[1] = pf[2] = pf[3] = 12;

    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey4", 0, 0, pf, &pMax));
    EXPECT_EQ(sizeof(pf) / sizeof(int), pMax);
    EXPECT_TRUE(!pf[0] && 1 == pf[1] && 2 == pf[2] && 3 == pf[3]);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testColorProperty) {
    aiColor4D clr;
    clr.r = 2.0f;
    clr.g = 3.0f;
    clr.b = 4.0f;
    clr.a = 5.0f;
    this->pcMat->AddProperty(&clr, 1, "testKey5");
    clr.b = 1.0f;
    clr.a = clr.g = clr.r = 0.0f;

    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey5", 0, 0, clr));
    EXPECT_TRUE(clr.r == 2.0f && clr.g == 3.0f && clr.b == 4.0f && clr.a == 5.0f);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testStringProperty) {
    aiString s;
    s.Set("Hello, this is a small test");
    this->pcMat->AddProperty(&s, "testKey6");
    s.Set("358358");
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey6", 0, 0, s));
    EXPECT_STREQ("Hello, this is a small test", s.data);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testDefaultMaterialName) {
    aiString name = pcMat->GetName();
    const int retValue(strncmp(name.C_Str(), AI_DEFAULT_MATERIAL_NAME, name.length));
    EXPECT_EQ(0, retValue);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testBoolProperty) {
    const bool valTrue = true;
    const bool valFalse = false;
    EXPECT_EQ(AI_SUCCESS, pcMat->AddProperty(&valTrue, 1, "bool_true"));
    EXPECT_EQ(AI_SUCCESS, pcMat->AddProperty(&valFalse, 1, "bool_false"));

    bool read = false;
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("bool_true", 0, 0, read));
    EXPECT_TRUE(read) << "read true bool";
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("bool_false", 0, 0, read));
    EXPECT_FALSE(read) << "read false bool";
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testCastIntProperty) {
    int value = 10;
    EXPECT_EQ(AI_SUCCESS, pcMat->AddProperty(&value, 1, "integer"));
    value = 0;
    EXPECT_EQ(AI_SUCCESS, pcMat->AddProperty(&value, 1, "zero"));
    value = -1;
    EXPECT_EQ(AI_SUCCESS, pcMat->AddProperty(&value, 1, "negative"));

    // To float
    float valFloat = 0.0f;
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("integer", 0, 0, valFloat));
    EXPECT_EQ(10.0f, valFloat);
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("zero", 0, 0, valFloat));
    EXPECT_EQ(0.0f, valFloat);
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("negative", 0, 0, valFloat));
    EXPECT_EQ(-1.0f, valFloat);

    // To bool
    bool valBool = false;
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("integer", 0, 0, valBool));
    EXPECT_EQ(true, valBool);
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("zero", 0, 0, valBool));
    EXPECT_EQ(false, valBool);
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("negative", 0, 0, valBool));
    EXPECT_EQ(true, valBool);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testCastFloatProperty) {
    float value = 150392.63f;
    EXPECT_EQ(AI_SUCCESS, pcMat->AddProperty(&value, 1, "float"));
    value = 0;
    EXPECT_EQ(AI_SUCCESS, pcMat->AddProperty(&value, 1, "zero"));

    // To int
    int valInt = 0.0f;
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("float", 0, 0, valInt));
    EXPECT_EQ(150392, valInt);
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("zero", 0, 0, valInt));
    EXPECT_EQ(0, valInt);

    // To bool
    bool valBool = false;
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("float", 0, 0, valBool));
    EXPECT_EQ(true, valBool);
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("zero", 0, 0, valBool));
    EXPECT_EQ(false, valBool);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testCastSmallFloatProperty) {
    float value = 0.0078125f;
    EXPECT_EQ(AI_SUCCESS, pcMat->AddProperty(&value, 1, "float"));
    value = 0;
    EXPECT_EQ(AI_SUCCESS, pcMat->AddProperty(&value, 1, "zero"));

    // To int
    int valInt = 0.0f;
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("float", 0, 0, valInt));
    EXPECT_EQ(0, valInt);
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("zero", 0, 0, valInt));
    EXPECT_EQ(0, valInt);

    // To bool
    bool valBool = false;
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("float", 0, 0, valBool));
    EXPECT_EQ(true, valBool);
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("zero", 0, 0, valBool));
    EXPECT_EQ(false, valBool);
}

// ------------------------------------------------------------------------------------------------
#if defined(_MSC_VER)
// Refuse to compile on Windows if any enum values are not explicitly handled in the switch
// TODO: Move this into assimp/Compiler as a macro and add clang/gcc versions so other code can use it
__pragma(warning(push));
__pragma(warning(error : 4061)); // enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label
__pragma(warning(error : 4062)); // enumerator 'identifier' in switch of enum 'enumeration' is not handled
#endif

TEST_F(MaterialSystemTest, testMaterialTextureTypeEnum) {
    // Verify that AI_TEXTURE_TYPE_MAX equals the largest 'real' value in the enum

    int32_t maxTextureType = 0;
    static constexpr int32_t bigNumber = 255;
    EXPECT_GT(bigNumber, AI_TEXTURE_TYPE_MAX) << "AI_TEXTURE_TYPE_MAX too large for valid enum test, increase bigNumber";

    // Loop until a value larger than any enum
    for (int32_t i = 0; i < bigNumber; ++i) {
        aiTextureType texType = static_cast<aiTextureType>(i);
        switch (texType) {
        default: break;
#ifndef SWIG
        case _aiTextureType_Force32Bit: break;
#endif
            // All the real values
        case aiTextureType_NONE:
        case aiTextureType_DIFFUSE:
        case aiTextureType_SPECULAR:
        case aiTextureType_AMBIENT:
        case aiTextureType_EMISSIVE:
        case aiTextureType_HEIGHT:
        case aiTextureType_NORMALS:
        case aiTextureType_SHININESS:
        case aiTextureType_OPACITY:
        case aiTextureType_DISPLACEMENT:
        case aiTextureType_LIGHTMAP:
        case aiTextureType_REFLECTION:
        case aiTextureType_BASE_COLOR:
        case aiTextureType_NORMAL_CAMERA:
        case aiTextureType_EMISSION_COLOR:
        case aiTextureType_METALNESS:
        case aiTextureType_DIFFUSE_ROUGHNESS:
        case aiTextureType_AMBIENT_OCCLUSION:
        case aiTextureType_SHEEN:
        case aiTextureType_CLEARCOAT:
        case aiTextureType_TRANSMISSION:
        case aiTextureType_MAYA_BASE:
        case aiTextureType_MAYA_SPECULAR:
        case aiTextureType_MAYA_SPECULAR_COLOR:
        case aiTextureType_MAYA_SPECULAR_ROUGHNESS:
        case aiTextureType_ANISOTROPY:
        case aiTextureType_GLTF_METALLIC_ROUGHNESS:
        case aiTextureType_UNKNOWN:
            if (i > maxTextureType)
                maxTextureType = i;
            break;
        }
    }

    EXPECT_EQ(maxTextureType, AI_TEXTURE_TYPE_MAX) << "AI_TEXTURE_TYPE_MAX macro must be equal to the largest valid aiTextureType_XXX";
}

#if defined(_MSC_VER)
__pragma (warning(pop))
#endif
