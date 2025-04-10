/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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
#include <assimp/ParsingUtils.h>
#include <assimp/fast_atof.h>
#include <array>

using namespace Assimp;

class utParsingUtils : public ::testing::Test {};

TEST_F(utParsingUtils, parseFloatsStringTest) {
    const std::array<float, 16> floatArray = {
        1.0f, 0.0f,         0.0f,        0.0f,
        0.0f, 7.54979e-8f, -1.0f,        0.0f,
        0.0f, 1.0f,         7.54979e-8f, 0.0f,
        0.0f, 0.0f,         0.0f,        1.0f};
    const std::string floatArrayAsStr = "1 0 0 0 0 7.54979e-8 -1 0 0 1 7.54979e-8 0 0 0 0 1";
    const char *content = floatArrayAsStr.c_str();
    const char *end = content + floatArrayAsStr.size();
    for (float i : floatArray) {
        float value = 0.0f;
        SkipSpacesAndLineEnd(&content, end);
        content = fast_atoreal_move<ai_real>(content, value);
        EXPECT_FLOAT_EQ(value, i);
    }
}
