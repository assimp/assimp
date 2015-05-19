#include "UnitTestPCH.h"

#include <assimp/scene.h>
#include <BaseProcess.h>


using namespace std;
using namespace Assimp;

class SharedPPDataTest : public ::testing::Test
{
public:

    virtual void SetUp();
    virtual void TearDown();

protected:

    SharedPostProcessInfo* shared;
};

static bool destructed;

struct TestType
{
    ~TestType()
    {
        destructed = true;
    }
};


// ------------------------------------------------------------------------------------------------
void SharedPPDataTest::SetUp()
{
    shared = new SharedPostProcessInfo();
    destructed = false;
}

// ------------------------------------------------------------------------------------------------
void SharedPPDataTest::TearDown()
{

}

// ------------------------------------------------------------------------------------------------
TEST_F(SharedPPDataTest, testPODProperty)
{
    int i = 5;
    shared->AddProperty("test",i);
    int o;
    EXPECT_TRUE(shared->GetProperty("test",o));
    EXPECT_EQ(5, o);
    EXPECT_FALSE(shared->GetProperty("test2",o));
    EXPECT_EQ(5, o);

    float f = 12.f, m;
    shared->AddProperty("test",f);
    EXPECT_TRUE(shared->GetProperty("test",m));
    EXPECT_EQ(12.f, m);
}

// ------------------------------------------------------------------------------------------------
TEST_F(SharedPPDataTest, testPropertyPointer)
{
    int *i = new int[35];
    shared->AddProperty("test16",i);
    int* o;
    EXPECT_TRUE(shared->GetProperty("test16",o));
    EXPECT_EQ(i, o);
    shared->RemoveProperty("test16");
    EXPECT_FALSE(shared->GetProperty("test16",o));
}

// ------------------------------------------------------------------------------------------------
TEST_F(SharedPPDataTest, testPropertyDeallocation)
{
    TestType *out, * pip = new TestType();
    shared->AddProperty("quak",pip);
    EXPECT_TRUE(shared->GetProperty("quak",out));
    EXPECT_EQ(pip, out);

    delete shared;
    EXPECT_TRUE(destructed);
}
