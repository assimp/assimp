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

using namespace Assimp;

class utMatrix4x4 : public ::testing::Test {
};

TEST_F(utMatrix4x4, badIndexOperatorTest) {
    aiMatrix4x4 m;
    ai_real *a0 = m[4];
    EXPECT_EQ(nullptr, a0);
}

TEST_F(utMatrix4x4, indexOperatorTest) {
    aiMatrix4x4 m;
    ai_real *a0 = m[0];
    EXPECT_FLOAT_EQ(1.0, *a0);
    ai_real *a1 = a0 + 1;
    EXPECT_FLOAT_EQ(0.0, *a1);
    ai_real *a2 = a0 + 2;
    EXPECT_FLOAT_EQ(0.0, *a2);
    ai_real *a3 = a0 + 3;
    EXPECT_FLOAT_EQ(0.0, *a3);

    ai_real *a4 = m[1];
    EXPECT_FLOAT_EQ(0.0, *a4);
    ai_real *a5 = a4 + 1;
    EXPECT_FLOAT_EQ(1.0, *a5);
    ai_real *a6 = a4 + 2;
    EXPECT_FLOAT_EQ(0.0, *a6);
    ai_real *a7 = a4 + 3;
    EXPECT_FLOAT_EQ(0.0, *a7);

    ai_real *a8 = m[2];
    EXPECT_FLOAT_EQ(0.0, *a8);
    ai_real *a9 = a8 + 1;
    EXPECT_FLOAT_EQ(0.0, *a9);
    ai_real *a10 = a8 + 2;
    EXPECT_FLOAT_EQ(1.0, *a10);
    ai_real *a11 = a8 + 3;
    EXPECT_FLOAT_EQ(0.0, *a11);

    ai_real *a12 = m[3];
    EXPECT_FLOAT_EQ(0.0, *a12);
    ai_real *a13 = a12 + 1;
    EXPECT_FLOAT_EQ(0.0, *a13);
    ai_real *a14 = a12 + 2;
    EXPECT_FLOAT_EQ(0.0, *a14);
    ai_real *a15 = a12 + 3;
    EXPECT_FLOAT_EQ(1.0, *a15);
}
