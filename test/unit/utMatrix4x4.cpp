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

using namespace Assimp;

class utMatrix4x4Test : public ::testing::Test {

};

TEST_F( utMatrix4x4Test, badIndexOperatorTest ) {
    aiMatrix4x4 m;
    float *a0 = m[ 4 ];
    EXPECT_EQ( NULL, a0 );
}

TEST_F( utMatrix4x4Test, indexOperatorTest ) {
    aiMatrix4x4 m;
    float *a0 = m[ 0 ];
    EXPECT_FLOAT_EQ( 1.0f, *a0 );
    float *a1 = a0+1;
    EXPECT_FLOAT_EQ( 0.0f, *a1 );
    float *a2 = a0 + 2;
    EXPECT_FLOAT_EQ( 0.0f, *a2 );
    float *a3 = a0 + 3;
    EXPECT_FLOAT_EQ( 0.0f, *a3 );

    float *a4 = m[ 1 ];
    EXPECT_FLOAT_EQ( 0.0f, *a4 );
    float *a5 = a4 + 1;
    EXPECT_FLOAT_EQ( 1.0f, *a5 );
    float *a6 = a4 + 2;
    EXPECT_FLOAT_EQ( 0.0f, *a6 );
    float *a7 = a4 + 3;
    EXPECT_FLOAT_EQ( 0.0f, *a7 );

    float *a8 = m[ 2 ];
    EXPECT_FLOAT_EQ( 0.0f, *a8 );
    float *a9 = a8 + 1;
    EXPECT_FLOAT_EQ( 0.0f, *a9 );
    float *a10 = a8 + 2;
    EXPECT_FLOAT_EQ( 1.0f, *a10 );
    float *a11 = a8 + 3;
    EXPECT_FLOAT_EQ( 0.0f, *a11 );

    float *a12 = m[ 3 ];
    EXPECT_FLOAT_EQ( 0.0f, *a12 );
    float *a13 = a12 + 1;
    EXPECT_FLOAT_EQ( 0.0f, *a13 );
    float *a14 = a12 + 2;
    EXPECT_FLOAT_EQ( 0.0f, *a14 );
    float *a15 = a12 + 3;
    EXPECT_FLOAT_EQ( 1.0f, *a15 );
}