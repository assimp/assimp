/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team

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

// ------------------------------------------------------------------------------------------------
void SharedPPDataTest::SetUp()
{
    shared = new SharedPostProcessInfo();
}

// ------------------------------------------------------------------------------------------------
void SharedPPDataTest::TearDown()
{
	delete shared;
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

static bool destructed;

struct TestType
{
    ~TestType()
    {
        destructed = true;
    }
};
// ------------------------------------------------------------------------------------------------
TEST_F(SharedPPDataTest, testPropertyDeallocation)
{
	SharedPostProcessInfo* localShared = new SharedPostProcessInfo();
	destructed = false;

	TestType *out, * pip = new TestType();
	localShared->AddProperty("quak",pip);
    EXPECT_TRUE(localShared->GetProperty("quak",out));
    EXPECT_EQ(pip, out);

    delete localShared;
    EXPECT_TRUE(destructed);
}
