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

#include <assimp/RemoveComments.h>

using namespace std;
using namespace Assimp;

// ------------------------------------------------------------------------------------------------
TEST(RemoveCommentsTest, testSingleLineComments) {
    const char *szTest = "int i = 0; \n"
                         "if (4 == //)\n"
                         "\ttrue) { // do something here \n"
                         "\t// hello ... and bye //\n";

    const size_t len(::strlen(szTest) + 1);
    char *szTest2 = new char[len];
    ::strncpy(szTest2, szTest, len);

    const char *szTestResult = "int i = 0; \n"
                               "if (4 ==    \n"
                               "\ttrue) {                      \n"
                               "\t                       \n";

    CommentRemover::RemoveLineComments("//", szTest2, ' ');
    EXPECT_STREQ(szTestResult, szTest2);

    delete[] szTest2;
}

// ------------------------------------------------------------------------------------------------
TEST(RemoveCommentsTest, testMultiLineComments) {
    const char *szTest =
            "/* comment to be removed */\n"
            "valid text /* \n "
            " comment across multiple lines */"
            " / * Incomplete comment */ /* /* multiple comments */ */";

    const char *szTestResult =
            "                           \n"
            "valid text      "
            "                                 "
            " / * Incomplete comment */                            */";

    const size_t len(::strlen(szTest) + 1);
    char *szTest2 = new char[len];
    ::strncpy(szTest2, szTest, len);

    CommentRemover::RemoveMultiLineComments("/*", "*/", szTest2, ' ');
    EXPECT_STREQ(szTestResult, szTest2);

    delete[] szTest2;
}
