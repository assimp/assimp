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
#include "MathTest.h"

using namespace Assimp;

class AssimpAPITest_aiVector3D : public AssimpMathTest {
protected:
    virtual void SetUp() {
        result_c = result_cpp = aiVector3D();
        temp = random_vec3(); // Generates a random 3D vector != null vector.
    }

    aiVector3D result_c, result_cpp, temp;
};

TEST_F(AssimpAPITest_aiVector3D, aiVector3AreEqualTest) {
    result_c = result_cpp = random_vec3();
    EXPECT_EQ(result_cpp == result_c,
        (bool)aiVector3AreEqual(&result_cpp, &result_c));
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3AreEqualEpsilonTest) {
    result_c = result_cpp = random_vec3();
    EXPECT_EQ(result_cpp.Equal(result_c, Epsilon),
        (bool)aiVector3AreEqualEpsilon(&result_cpp, &result_c, Epsilon));
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3LessThanTest) {
    result_c = result_cpp = random_vec3();
    EXPECT_EQ(result_cpp < temp,
        (bool)aiVector3LessThan(&result_c, &temp));
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3AddTest) {
    result_c = result_cpp = random_vec3();
    result_cpp += temp;
    aiVector3Add(&result_c, &temp);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3SubtractTest) {
    result_c = result_cpp = random_vec3();
    result_cpp -= temp;
    aiVector3Subtract(&result_c, &temp);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3ScaleTest) {
    const float FACTOR = RandNonZero.next();
    result_c = result_cpp = random_vec3();
    result_cpp *= FACTOR;
    aiVector3Scale(&result_c, FACTOR);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3SymMulTest) {
    result_c = result_cpp = random_vec3();
    result_cpp = result_cpp.SymMul(temp);
    aiVector3SymMul(&result_c, &temp);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3DivideByScalarTest) {
    const float DIVISOR = RandNonZero.next();
    result_c = result_cpp = random_vec3();
    result_cpp /= DIVISOR;
    aiVector3DivideByScalar(&result_c, DIVISOR);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3DivideByVectorTest) {
    result_c = result_cpp = random_vec3();
    result_cpp = result_cpp / temp;
    aiVector3DivideByVector(&result_c, &temp);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3LengthTest) {
    result_c = result_cpp = random_vec3();
    EXPECT_EQ(result_cpp.Length(), aiVector3Length(&result_c));
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3SquareLengthTest) {
    result_c = result_cpp = random_vec3();
    EXPECT_EQ(result_cpp.SquareLength(), aiVector3SquareLength(&result_c));
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3NegateTest) {
    result_c = result_cpp = random_vec3();
    aiVector3Negate(&result_c);
    EXPECT_EQ(-result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3DotProductTest) {
    result_c = result_cpp = random_vec3();
    EXPECT_EQ(result_cpp * result_c,
        aiVector3DotProduct(&result_cpp, &result_c));
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3CrossProductTest) {
    result_c = result_cpp = random_vec3();
    result_cpp = result_cpp ^ temp;
    aiVector3CrossProduct(&result_c, &result_c, &temp);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3NormalizeTest) {
    result_c = result_cpp = random_vec3();
    aiVector3Normalize(&result_c);
    EXPECT_EQ(result_cpp.Normalize(), result_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3NormalizeSafeTest) {
    result_c = result_cpp = random_vec3();
    aiVector3NormalizeSafe(&result_c);
    EXPECT_EQ(result_cpp.NormalizeSafe(), result_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiVector3RotateByQuaternionTest) {
    aiVector3D v_c, v_cpp;
    v_c = v_cpp = random_vec3();
    const auto q = random_quat();
    aiVector3RotateByQuaternion(&v_c, &q);
    EXPECT_EQ(q.Rotate(v_cpp), v_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiTransformVecByMatrix3Test) {
    const auto m = random_mat3();
    aiVector3D v_c, v_cpp;
    v_c = v_cpp = random_vec3();
    v_cpp *= m;
    aiTransformVecByMatrix3(&v_c, &m);
    EXPECT_EQ(v_cpp, v_c);
}

TEST_F(AssimpAPITest_aiVector3D, aiTransformVecByMatrix4Test) {
    const auto m = random_mat4();
    aiVector3D v_c, v_cpp;
    v_c = v_cpp = random_vec3();
    v_cpp *= m;
    aiTransformVecByMatrix4(&v_c, &m);
    EXPECT_EQ(v_cpp, v_c);
}
