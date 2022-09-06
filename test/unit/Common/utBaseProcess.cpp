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

#include "TestIOSystem.h"
#include "UnitTestPCH.h"

#include "Common/BaseProcess.h"
#include "Common/AssertHandler.h"

using namespace Assimp;

class BaseProcessTest : public ::testing::Test {
public:
    static void test_handler( const char*, const char*, int ) {
        HandlerWasCalled = true;
    }

    void SetUp() override {
        HandlerWasCalled = false;
        setAiAssertHandler(test_handler);
    }

    void TearDown() override {
        setAiAssertHandler(nullptr);
    }

    static bool handlerWasCalled() {
        return HandlerWasCalled;
    }

private:
    static bool HandlerWasCalled;
};

bool BaseProcessTest::HandlerWasCalled = false;

class TestingBaseProcess : public BaseProcess {
public:
    TestingBaseProcess() : BaseProcess() {
        // empty
    }

    ~TestingBaseProcess() override = default;

    bool IsActive( unsigned int ) const override {
        return true;
    }

    void Execute(aiScene*) override {

    }
};
TEST_F( BaseProcessTest, constructTest ) {
    bool ok = true;
    try {
        TestingBaseProcess process;
    } catch (...) {
        ok = false;
    }
    EXPECT_TRUE(ok);
}

TEST_F( BaseProcessTest, executeOnSceneTest ) {
    TestingBaseProcess process;
    process.ExecuteOnScene(nullptr);
#ifdef ASSIMP_BUILD_DEBUG
    EXPECT_TRUE(BaseProcessTest::handlerWasCalled());
#else
    EXPECT_FALSE(BaseProcessTest::handlerWasCalled());
#endif

}
