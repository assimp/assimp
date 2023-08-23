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

#include "UnitTestPCH.h"

/// Ensure this test has asserts on, even if the build type doesn't have asserts by default.
#if !defined(ASSIMP_BUILD_DEBUG)
#define ASSIMP_BUILD_DEBUG
#endif

#include <assimp/ai_assert.h>
#include <assimp/AssertHandler.h>

namespace
{
    /// An exception which is thrown by the testAssertHandler
    struct TestAssertException
    {
        TestAssertException(const char* failedExpression, const char* file, int line)
            : m_failedExpression(failedExpression)
            , m_file(file)
            , m_line(line)
        {
        }

        std::string m_failedExpression;
        std::string m_file;
        int m_line;
    };

    /// Swap the default handler, which aborts, by one which throws.
    void testAssertHandler(const char* failedExpression, const char* file, int line)
    {
        throw TestAssertException(failedExpression, file, line);
    }

    /// Ensure that the default assert handler is restored after the test is finished.
    struct ReplaceHandlerScope
    {
        ReplaceHandlerScope()
        {
            Assimp::setAiAssertHandler(testAssertHandler);
        }

        ~ReplaceHandlerScope()
        {
            Assimp::setAiAssertHandler(Assimp::defaultAiAssertHandler);
        }
    };
}

TEST(utAssertHandler, replaceWithThrow)
{
    ReplaceHandlerScope scope;

    try
    {
        ai_assert((2 + 2 == 5) && "Sometimes people put messages here");
        EXPECT_TRUE(false);
    }
    catch(const TestAssertException& e)
    {
        EXPECT_STREQ(e.m_failedExpression.c_str(), "(2 + 2 == 5) && \"Sometimes people put messages here\"");
        EXPECT_STREQ(e.m_file.c_str(), __FILE__);
        EXPECT_GT(e.m_line, 0);
        EXPECT_LT(e.m_line, __LINE__);
    }
    catch(...)
    {
        EXPECT_TRUE(false);
    }
}
