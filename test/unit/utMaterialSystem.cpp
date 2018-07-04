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
#include "UnitTestPCH.h"

#include <assimp/scene.h>
#include <MaterialSystem.h>

using namespace ::std;
using namespace ::Assimp;

class MaterialSystemTest : public ::testing::Test
{
public:
    virtual void SetUp() { this->pcMat = new aiMaterial(); }
    virtual void TearDown() { delete this->pcMat; }

protected:
    aiMaterial* pcMat;
};

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testFloatProperty)
{
    float pf = 150392.63f;
    this->pcMat->AddProperty(&pf,1,"testKey1");
    pf = 0.0f;

    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey1",0,0,pf));
    EXPECT_EQ(150392.63f, pf);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testFloatArrayProperty)
{
    float pf[] = {0.0f,1.0f,2.0f,3.0f};
    unsigned int pMax = sizeof(pf) / sizeof(float);
    this->pcMat->AddProperty(pf,pMax,"testKey2");
    pf[0] = pf[1] = pf[2] = pf[3] = 12.0f;

    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey2",0,0,pf,&pMax));
    EXPECT_EQ(sizeof(pf) / sizeof(float), pMax);
    EXPECT_TRUE(!pf[0] && 1.0f == pf[1] && 2.0f == pf[2] && 3.0f == pf[3] );
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testIntProperty)
{
    int pf = 15039263;
    this->pcMat->AddProperty(&pf,1,"testKey3");
    pf = 12;

    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey3",0,0,pf));
    EXPECT_EQ(15039263, pf);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testIntArrayProperty)
{
    int pf[] = {0,1,2,3};
    unsigned int pMax = sizeof(pf) / sizeof(int);
    this->pcMat->AddProperty(pf,pMax,"testKey4");
    pf[0] = pf[1] = pf[2] = pf[3] = 12;

    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey4",0,0,pf,&pMax));
    EXPECT_EQ(sizeof(pf) / sizeof(int), pMax);
    EXPECT_TRUE(!pf[0] && 1 == pf[1] && 2 == pf[2] && 3 == pf[3] );
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testColorProperty)
{
    aiColor4D clr;
    clr.r = 2.0f;clr.g = 3.0f;clr.b = 4.0f;clr.a = 5.0f;
    this->pcMat->AddProperty(&clr,1,"testKey5");
    clr.b = 1.0f;
    clr.a = clr.g = clr.r = 0.0f;

    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey5",0,0,clr));
    EXPECT_TRUE(clr.r == 2.0f && clr.g == 3.0f && clr.b == 4.0f && clr.a == 5.0f);
}

// ------------------------------------------------------------------------------------------------
TEST_F(MaterialSystemTest, testStringProperty)
{
    aiString s;
    s.Set("Hello, this is a small test");
    this->pcMat->AddProperty(&s,"testKey6");
    s.Set("358358");
    EXPECT_EQ(AI_SUCCESS, pcMat->Get("testKey6",0,0,s));
    EXPECT_STREQ("Hello, this is a small test", s.data);
}
