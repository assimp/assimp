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
#include <array>

using namespace Assimp;

class AssimpAPITest_aiMatrix3x3 : public AssimpMathTest {
protected:
    void SetUp() override {
        result_c = result_cpp = aiMatrix3x3();
    }

    aiMatrix3x3 result_c, result_cpp;
};

TEST_F(AssimpAPITest_aiMatrix3x3, aiIdentityMatrix3Test) {
    // Force a non-identity matrix.
    result_c = aiMatrix3x3(0,0,0,0,0,0,0,0,0);
    aiIdentityMatrix3(&result_c);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMatrix3FromMatrix4Test) {
    const auto m = random_mat4();
    result_cpp = aiMatrix3x3(m);
    aiMatrix3FromMatrix4(&result_c, &m);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMatrix3FromQuaternionTest) {
    const auto q = random_quat();
    result_cpp = q.GetMatrix();
    aiMatrix3FromQuaternion(&result_c, &q);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMatrix3AreEqualTest) {
    result_c = result_cpp = random_mat3();
    EXPECT_EQ(result_cpp == result_c,
        (bool)aiMatrix3AreEqual(&result_cpp, &result_c));
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMatrix3AreEqualEpsilonTest) {
    result_c = result_cpp = random_mat3();
    EXPECT_EQ(result_cpp.Equal(result_c, Epsilon),
        (bool)aiMatrix3AreEqualEpsilon(&result_cpp, &result_c, Epsilon));
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMultiplyMatrix3Test) {
    const auto m = random_mat3();
    result_c = result_cpp = random_mat3();
    result_cpp *= m;
    aiMultiplyMatrix3(&result_c, &m);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiTransposeMatrix3Test) {
    result_c = result_cpp = random_mat3();
    result_cpp.Transpose();
    aiTransposeMatrix3(&result_c);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMatrix3InverseTest) {
    // Use a predetermined matrix to prevent arbitrary
    // cases where it could have a null determinant.
    result_c = result_cpp = aiMatrix3x3(
        5, 2, 7,
        4, 6, 9,
        1, 8, 3);
    result_cpp.Inverse();
    aiMatrix3Inverse(&result_c);
    EXPECT_EQ(result_cpp, result_c);
}

inline void AI_EXPECT_REAL_EQ(ai_real val1, ai_real val2) {
#ifdef ASSIMP_DOUBLE_PRECISION
    EXPECT_DOUBLE_EQ((val1), (val2));
#else
    EXPECT_FLOAT_EQ((val1), (val2));
#endif
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMatrix3DeterminantTest) {
    result_c = result_cpp = random_mat3();
    const ai_real det_1 = result_cpp.Determinant();
    const ai_real det_2 = aiMatrix3Determinant(&result_c);
    AI_EXPECT_REAL_EQ(det_1, det_2);
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMatrix3RotationZTest) {
    const float angle(RandPI.next());
    aiMatrix3x3::RotationZ(angle, result_cpp);
    aiMatrix3RotationZ(&result_c, angle);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMatrix3FromRotationAroundAxisTest) {
    const float angle(RandPI.next());
    const auto axis = random_unit_vec3();
    aiMatrix3x3::Rotation(angle, axis, result_cpp);
    aiMatrix3FromRotationAroundAxis(&result_c, &axis, angle);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMatrix3TranslationTest) {
    const auto axis = random_vec2();
    aiMatrix3x3::Translation(axis, result_cpp);
    aiMatrix3Translation(&result_c, &axis);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix3x3, aiMatrix3FromToTest) {
    // Use predetermined vectors to prevent running into division by zero.
    const auto from = aiVector3D(1,2,1).Normalize(), to = aiVector3D(-1,1,1).Normalize();
    aiMatrix3x3::FromToMatrix(from, to, result_cpp);
    aiMatrix3FromTo(&result_c, &from, &to);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix3x3, operatorTest) {
    std::array<ai_real, 9> value = { 1, 2, 3, 4, 5, 6, 7, 8,9};
    result_cpp = aiMatrix3x3( value[0], value[1], value[2], value[3],
                              value[4], value[5], value[6], value[7],
                              value[8]);
    size_t idx=0;
    for (unsigned int i = 0; i < 3; ++i) {
       for (unsigned int j = 0; j < 3; ++j) {
            ai_real curValue = result_cpp[i][j];
            EXPECT_EQ(curValue, value[idx]);
            idx++;
       }
    }
}
