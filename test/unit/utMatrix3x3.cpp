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
#include <iostream>

using namespace ::Assimp;

class utMatrix3x3Test : public ::testing::Test {
    // empty
};

TEST_F( utMatrix3x3Test, FromToMatrixTest ) {
    aiVector3D res;
    aiMatrix3x3 trafo;

    const double PRECISION = 0.000001;

    // axes test
    aiVector3D axes[] =
        { aiVector3D(1, 0, 0)
        , aiVector3D(0, 1, 0)
        , aiVector3D(0, 0, 1)
        };

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            aiMatrix3x3::FromToMatrix( axes[i], axes[j], trafo );
            res = trafo * axes[i];

            ASSERT_NEAR( axes[j].x, res.x, PRECISION );
            ASSERT_NEAR( axes[j].y, res.y, PRECISION );
            ASSERT_NEAR( axes[j].z, res.z, PRECISION );
        }
    }

    // random test
    const int NUM_SAMPLES = 10000;

    aiVector3D from, to;

    for (int i = 0; i < NUM_SAMPLES; ++i) {
        from = aiVector3D
            ( 1.f * rand() / RAND_MAX
            , 1.f * rand() / RAND_MAX
            , 1.f * rand() / RAND_MAX
            ).Normalize();
        to = aiVector3D
            ( 1.f * rand() / RAND_MAX
            , 1.f * rand() / RAND_MAX
            , 1.f * rand() / RAND_MAX
            ).Normalize();

        aiMatrix3x3::FromToMatrix( from, to, trafo );
        res = trafo * from;

        ASSERT_NEAR( to.x, res.x, PRECISION );
        ASSERT_NEAR( to.y, res.y, PRECISION );
        ASSERT_NEAR( to.z, res.z, PRECISION );
    }
}
