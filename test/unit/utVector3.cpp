/*-------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2017, assimp team

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
-------------------------------------------------------------------------*/
#include "UnitTestPCH.h"
#include <assimp/vector3.h>

using namespace ::Assimp;

class utVector3 : public ::testing::Test {
    // empty
};

TEST_F(utVector3, CreationTest) {
    aiVector3D v0;
    aiVector3D v1( 1.0f, 2.0f, 3.0f );
    EXPECT_FLOAT_EQ (1.0f, v1[ 0 ] );
    EXPECT_FLOAT_EQ( 2.0f, v1[ 1 ] );
    EXPECT_FLOAT_EQ( 3.0f, v1[ 2 ] );
    aiVector3D v2( 1 );
    EXPECT_FLOAT_EQ( 1.0f, v2[ 0 ] );
    EXPECT_FLOAT_EQ( 1.0f, v2[ 1 ] );
    EXPECT_FLOAT_EQ( 1.0f, v2[ 2 ] );
    aiVector3D v3( v1 );
    EXPECT_FLOAT_EQ( v1[ 0 ], v3[ 0 ] );
    EXPECT_FLOAT_EQ( v1[ 1 ], v3[ 1 ] );
    EXPECT_FLOAT_EQ( v1[ 2 ], v3[ 2 ] );
}

TEST_F( utVector3, BracketOpTest ) {
    aiVector3D v(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ( 1.0f, v[ 0 ] );
    EXPECT_FLOAT_EQ( 2.0f, v[ 1 ] ); 
    EXPECT_FLOAT_EQ( 3.0f, v[ 2 ] );
}
