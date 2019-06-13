/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team



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

#include <assimp/types.h>

using namespace Assimp;

class utTypes : public ::testing::Test {
    // empty
};

TEST_F( utTypes, Color3dCpmpareOpTest ) {
    aiColor3D col1( 1, 2, 3 );
    aiColor3D col2( 4, 5, 6 );
    aiColor3D col3( col1 );
    
    EXPECT_FALSE( col1 == col2 );
    EXPECT_FALSE( col2 == col3 );
    EXPECT_TRUE( col1 == col3 );

    EXPECT_TRUE( col1 != col2 );
    EXPECT_TRUE( col2 != col3 );
    EXPECT_FALSE( col1 != col3 );
}

TEST_F( utTypes, Color3dIndexOpTest ) {
    aiColor3D col( 1, 2, 3 );
    const ai_real r = col[ 0 ];
    EXPECT_FLOAT_EQ( 1, r );

    const ai_real g = col[ 1 ];
    EXPECT_FLOAT_EQ( 2, g );

    const ai_real b = col[ 2 ];
    EXPECT_FLOAT_EQ( 3, b );
}
