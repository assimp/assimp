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

#include "simd.h"

#include <chrono>

using namespace ::Assimp;

class utSimd : public ::testing::Test {
protected:
    // empty
};

TEST_F( utSimd, SSE2SupportedTest ) {
    bool isSupported;

    isSupported = CPUSupportsSSE2();
    if ( isSupported ) {
        std::cout << "Supported" << std::endl;
    } else {
        std::cout << "Not supported" << std::endl;
    }
}

TEST_F(utSimd, AddTest) {
    auto start = std::chrono::high_resolution_clock::now();
    const size_t NumIterations = 1000000;
    aiVector3D vec1(1,1,1), vec2(2,2,2), vec3;
    for (size_t i = 0; i < NumIterations; ++i) {
        vec3 = vec1 + vec2;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Elapsed time: " << elapsed.count() << " s\n";

    float4 v1, v2, res;
    v1.a[0] = 1.0f;
    v1.a[1] = 1.0f;
    v1.a[2] = 1.0f;
    v1.a[3] = 1.0f;

    v2.a[0] = 2.0f;
    v2.a[1] = 2.0f;
    v2.a[2] = 2.0f;
    v2.a[3] = 2.0f;

    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < NumIterations; ++i) {
        simd_add_op(v1, v2, res);
    }
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    std::cout << "Elapsed time: " << elapsed.count() << " s\n";
}

TEST_F(utSimd, NormalizeTest) {
    float4 v[100], res[100];
    for (size_t i = 0; i < 100; ++i) {
        v[i].a[0] = (i + 1) * 1.0f;
        v[i].a[1] = (i + 1) * 2.0f;
        v[i].a[2] = (i + 1) * 3.0f;
        v[i].a[3] = 0.0f;
    }

    simd_normalise_vectors_op(v, res, 100);
    for (size_t i = 0; i < 100; ++i) {
        aiVector3D vec3(res[i].a[0], res[i].a[1], res[i].a[2]);
        ai_real l = vec3.Length();
        EXPECT_NEAR(l, 1, 0.1);
    }
}
