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
#include <assimp/StringUtils.h>

class utStringUtils : public ::testing::Test {
    // empty
};

TEST_F(utStringUtils, to_string_Test ) {
    std::string res = ai_to_string( 1 );
    EXPECT_EQ( res, "1" );

    res = ai_to_string( 1.0f );
    EXPECT_EQ( res, "1" );
}

TEST_F(utStringUtils, ai_strtofTest ) {
    float res = ai_strtof( nullptr, nullptr );
    EXPECT_FLOAT_EQ( res, 0.0f );

    std::string testStr1 = "200.0";
    res = ai_strtof( testStr1.c_str(), nullptr );
    EXPECT_FLOAT_EQ( res, 200.0f );

    std::string testStr2 = "200.0 xxx";
    const char *begin( testStr2.c_str() );
    const char *end( begin + 6 );
    res = ai_strtof( begin, end );
    EXPECT_FLOAT_EQ( res, 200.0f );
}

TEST_F(utStringUtils, ai_rgba2hexTest) {
    std::string result;
    result = ai_rgba2hex(255, 255, 255, 255, true);
    EXPECT_EQ(result, "#ffffffff");
    result = ai_rgba2hex(0, 0, 0, 0, false);
    EXPECT_EQ(result, "00000000");
}
