#include "UnitTestPCH.h"

#include <assimp/scene.h>
#include <MaterialSystem.h>


using namespace std;
using namespace Assimp;

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
    this->pcMat->AddProperty(&pf,pMax,"testKey2");
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
    this->pcMat->AddProperty(&pf,pMax,"testKey4");
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
