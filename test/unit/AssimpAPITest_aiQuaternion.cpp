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

class AssimpAPITest_aiQuaternion : public AssimpMathTest {
protected:
    virtual void SetUp() {
        result_c = result_cpp = aiQuaternion();
    }

    aiQuaternion result_c, result_cpp;
};

TEST_F(AssimpAPITest_aiQuaternion, aiCreateQuaternionFromMatrixTest) {
    // Use a predetermined transformation matrix
    // to prevent running into division by zero.
    aiMatrix3x3 m, r;
    aiMatrix3x3::Translation(aiVector2D(14,-25), m);
    aiMatrix3x3::RotationZ(Math::aiPi<float>() / 4.0f, r);
    m = m * r;

    result_cpp = aiQuaternion(m);
    aiCreateQuaternionFromMatrix(&result_c, &m);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiQuaternion, aiQuaternionFromEulerAnglesTest) {
    const float x(RandPI.next()),
        y(RandPI.next()),
        z(RandPI.next());
    result_cpp = aiQuaternion(x, y, z);
    aiQuaternionFromEulerAngles(&result_c, x, y, z);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiQuaternion, aiQuaternionFromAxisAngleTest) {
    const float angle(RandPI.next());
    const aiVector3D axis(random_unit_vec3());
    result_cpp = aiQuaternion(axis, angle);
    aiQuaternionFromAxisAngle(&result_c, &axis, angle);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiQuaternion, aiQuaternionFromNormalizedQuaternionTest) {
    const auto qvec3 = random_unit_vec3();
    result_cpp = aiQuaternion(qvec3);
    aiQuaternionFromNormalizedQuaternion(&result_c, &qvec3);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiQuaternion, aiQuaternionAreEqualTest) {
    result_c = result_cpp = random_quat();
    EXPECT_EQ(result_cpp == result_c,
        (bool)aiQuaternionAreEqual(&result_cpp, &result_c));
}

TEST_F(AssimpAPITest_aiQuaternion, aiQuaternionAreEqualEpsilonTest) {
    result_c = result_cpp = random_quat();
    EXPECT_EQ(result_cpp.Equal(result_c, Epsilon),
        (bool)aiQuaternionAreEqualEpsilon(&result_cpp, &result_c, Epsilon));
}

TEST_F(AssimpAPITest_aiQuaternion, aiQuaternionNormalizeTest) {
    result_c = result_cpp = random_quat();
    aiQuaternionNormalize(&result_c);
    EXPECT_EQ(result_cpp.Normalize(), result_c);
}

TEST_F(AssimpAPITest_aiQuaternion, aiQuaternionConjugateTest) {
    result_c = result_cpp = random_quat();
    aiQuaternionConjugate(&result_c);
    EXPECT_EQ(result_cpp.Conjugate(), result_c);
}

TEST_F(AssimpAPITest_aiQuaternion, aiQuaternionMultiplyTest) {
    const aiQuaternion temp = random_quat();
    result_c = result_cpp = random_quat();
    result_cpp = result_cpp * temp;
    aiQuaternionMultiply(&result_c, &temp);

    EXPECT_FLOAT_EQ(result_cpp.x, result_c.x);
    EXPECT_FLOAT_EQ(result_cpp.y, result_c.y);
    EXPECT_FLOAT_EQ(result_cpp.z, result_c.z);
    EXPECT_FLOAT_EQ(result_cpp.w, result_c.w);
}

TEST_F(AssimpAPITest_aiQuaternion, aiQuaternionInterpolateTest) {
    // Use predetermined quaternions to prevent division by zero
    // during slerp calculations.
    const float INTERPOLATION(0.5f);
    const auto q1 = aiQuaternion(aiVector3D(-1,1,1).Normalize(), Math::aiPi<float>() / 4.0f);
    const auto q2 = aiQuaternion(aiVector3D(1,2,1).Normalize(), Math::aiPi<float>() / 2.0f);
    aiQuaternion::Interpolate(result_cpp, q1, q2, INTERPOLATION);
    aiQuaternionInterpolate(&result_c, &q1, &q2, INTERPOLATION);

    EXPECT_FLOAT_EQ(result_cpp.x, result_c.x);
    EXPECT_FLOAT_EQ(result_cpp.y, result_c.y);
    EXPECT_FLOAT_EQ(result_cpp.z, result_c.z);
    EXPECT_FLOAT_EQ(result_cpp.w, result_c.w);
}
