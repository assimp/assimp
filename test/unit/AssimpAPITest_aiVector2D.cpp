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
#include "MathTest.h"

using namespace Assimp;

class AssimpAPITest_aiVector2D : public AssimpMathTest {
protected:
    virtual void SetUp() {
        result_c = result_cpp = aiVector2D();
        temp = random_vec2(); // Generates a random 2D vector != null vector.
    }

    aiVector2D result_c, result_cpp, temp;
};

TEST_F(AssimpAPITest_aiVector2D, aiVector2AreEqualTest) {
    result_c = result_cpp = random_vec2();
    EXPECT_EQ(result_cpp == result_c,
        (bool)aiVector2AreEqual(&result_cpp, &result_c));
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2AreEqualEpsilonTest) {
    result_c = result_cpp = random_vec2();
    EXPECT_EQ(result_cpp.Equal(result_c, Epsilon),
        (bool)aiVector2AreEqualEpsilon(&result_cpp, &result_c, Epsilon));
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2AddTest) {
    result_c = result_cpp = random_vec2();
    result_cpp += temp;
    aiVector2Add(&result_c, &temp);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2SubtractTest) {
    result_c = result_cpp = random_vec2();
    result_cpp -= temp;
    aiVector2Subtract(&result_c, &temp);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2ScaleTest) {
    const float FACTOR = RandNonZero.next();
    result_c = result_cpp = random_vec2();
    result_cpp *= FACTOR;
    aiVector2Scale(&result_c, FACTOR);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2SymMulTest) {
    result_c = result_cpp = random_vec2();
    result_cpp = result_cpp.SymMul(temp);
    aiVector2SymMul(&result_c, &temp);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2DivideByScalarTest) {
    const float DIVISOR = RandNonZero.next();
    result_c = result_cpp = random_vec2();
    result_cpp /= DIVISOR;
    aiVector2DivideByScalar(&result_c, DIVISOR);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2DivideByVectorTest) {
    result_c = result_cpp = random_vec2();
    result_cpp = result_cpp / temp;
    aiVector2DivideByVector(&result_c, &temp);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2LengthTest) {
    result_c = result_cpp = random_vec2();
    EXPECT_EQ(result_cpp.Length(), aiVector2Length(&result_c));
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2SquareLengthTest) {
    result_c = result_cpp = random_vec2();
    EXPECT_EQ(result_cpp.SquareLength(), aiVector2SquareLength(&result_c));
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2NegateTest) {
    result_c = result_cpp = random_vec2();
    aiVector2Negate(&result_c);
    EXPECT_EQ(-result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2DotProductTest) {
    result_c = result_cpp = random_vec2();
    EXPECT_EQ(result_cpp * result_c,
        aiVector2DotProduct(&result_cpp, &result_c));
}

TEST_F(AssimpAPITest_aiVector2D, aiVector2NormalizeTest) {
    result_c = result_cpp = random_vec2();
    aiVector2Normalize(&result_c);
    EXPECT_EQ(result_cpp.Normalize(), result_c);
}
