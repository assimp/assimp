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

class AssimpAPITest_aiMatrix4x4 : public AssimpMathTest {
protected:
    virtual void SetUp() {
        result_c = result_cpp = aiMatrix4x4();
    }

    /* Generates a predetermined transformation matrix to use
       for the aiDecompose functions to prevent running into
       division by zero. */
    aiMatrix4x4 get_predetermined_transformation_matrix_for_decomposition() const {
        aiMatrix4x4 t, r;
        aiMatrix4x4::Translation(aiVector3D(14,-25,-8), t);
        aiMatrix4x4::Rotation(Math::aiPi<float>() / 4.0f, aiVector3D(1).Normalize(), r);
        return t * r;
    }

    aiMatrix4x4 result_c, result_cpp;
};

TEST_F(AssimpAPITest_aiMatrix4x4, aiIdentityMatrix4Test) {
    // Force a non-identity matrix.
    result_c = aiMatrix4x4(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
    aiIdentityMatrix4(&result_c);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4FromMatrix3Test) {
    aiMatrix3x3 m = random_mat3();
    result_cpp = aiMatrix4x4(m);
    aiMatrix4FromMatrix3(&result_c, &m);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4FromScalingQuaternionPositionTest) {
    const aiVector3D s = random_vec3();
    const aiQuaternion q = random_quat();
    const aiVector3D t = random_vec3();
    result_cpp = aiMatrix4x4(s, q, t);
    aiMatrix4FromScalingQuaternionPosition(&result_c, &s, &q, &t);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4AddTest) {
    const aiMatrix4x4 temp = random_mat4();
    result_c = result_cpp = random_mat4();
    result_cpp = result_cpp + temp;
    aiMatrix4Add(&result_c, &temp);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4AreEqualTest) {
    result_c = result_cpp = random_mat4();
    EXPECT_EQ(result_cpp == result_c,
        (bool)aiMatrix4AreEqual(&result_cpp, &result_c));
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4AreEqualEpsilonTest) {
    result_c = result_cpp = random_mat4();
    EXPECT_EQ(result_cpp.Equal(result_c, Epsilon),
        (bool)aiMatrix4AreEqualEpsilon(&result_cpp, &result_c, Epsilon));
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMultiplyMatrix4Test) {
    const auto m = random_mat4();
    result_c = result_cpp = random_mat4();
    result_cpp *= m;
    aiMultiplyMatrix4(&result_c, &m);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiTransposeMatrix4Test) {
    result_c = result_cpp = random_mat4();
    result_cpp.Transpose();
    aiTransposeMatrix4(&result_c);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4InverseTest) {
    // Use a predetermined matrix to prevent arbitrary
    // cases where it could have a null determinant.
    result_c = result_cpp = aiMatrix4x4(
        6, 10, 15, 3,
        14, 2, 12, 8,
        9, 13, 5, 16,
        4, 7, 11, 1);
    result_cpp.Inverse();
    aiMatrix4Inverse(&result_c);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4DeterminantTest) {
    result_c = result_cpp = random_mat4();
    EXPECT_EQ(result_cpp.Determinant(),
        aiMatrix4Determinant(&result_c));
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4IsIdentityTest) {
    EXPECT_EQ(result_cpp.IsIdentity(),
        (bool)aiMatrix4IsIdentity(&result_c));
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiDecomposeMatrixTest) {
    aiVector3D scaling_c, scaling_cpp,
        position_c, position_cpp;
    aiQuaternion rotation_c, rotation_cpp;

    result_c = result_cpp = get_predetermined_transformation_matrix_for_decomposition();
    result_cpp.Decompose(scaling_cpp, rotation_cpp, position_cpp);
    aiDecomposeMatrix(&result_c, &scaling_c, &rotation_c, &position_c);
    EXPECT_EQ(scaling_cpp, scaling_c);
    EXPECT_EQ(position_cpp, position_c);
    EXPECT_EQ(rotation_cpp, rotation_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4DecomposeIntoScalingEulerAnglesPositionTest) {
    aiVector3D scaling_c, scaling_cpp,
        rotation_c, rotation_cpp,
        position_c, position_cpp;

    result_c = result_cpp = get_predetermined_transformation_matrix_for_decomposition();
    result_cpp.Decompose(scaling_cpp, rotation_cpp, position_cpp);
    aiMatrix4DecomposeIntoScalingEulerAnglesPosition(&result_c, &scaling_c, &rotation_c, &position_c);
    EXPECT_EQ(scaling_cpp, scaling_c);
    EXPECT_EQ(position_cpp, position_c);
    EXPECT_EQ(rotation_cpp, rotation_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4DecomposeIntoScalingAxisAnglePositionTest) {
    aiVector3D scaling_c, scaling_cpp,
        axis_c, axis_cpp,
        position_c, position_cpp;
    ai_real angle_c, angle_cpp;

    result_c = result_cpp = get_predetermined_transformation_matrix_for_decomposition();
    result_cpp.Decompose(scaling_cpp, axis_cpp, angle_cpp, position_cpp);
    aiMatrix4DecomposeIntoScalingAxisAnglePosition(&result_c, &scaling_c, &axis_c, &angle_c, &position_c);
    EXPECT_EQ(scaling_cpp, scaling_c);
    EXPECT_EQ(axis_cpp, axis_c);
    EXPECT_EQ(angle_cpp, angle_c);
    EXPECT_EQ(position_cpp, position_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4DecomposeNoScalingTest) {
    aiVector3D position_c, position_cpp;
    aiQuaternion rotation_c, rotation_cpp;

    result_c = result_cpp = get_predetermined_transformation_matrix_for_decomposition();
    result_cpp.DecomposeNoScaling(rotation_cpp, position_cpp);
    aiMatrix4DecomposeNoScaling(&result_c, &rotation_c, &position_c);
    EXPECT_EQ(position_cpp, position_c);
    EXPECT_EQ(rotation_cpp, rotation_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4FromEulerAnglesTest) {
    const float x(RandPI.next()),
        y(RandPI.next()),
        z(RandPI.next());
    result_cpp.FromEulerAnglesXYZ(x, y, z);
    aiMatrix4FromEulerAngles(&result_c, x, y, z);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4RotationXTest) {
    const float angle(RandPI.next());
    aiMatrix4x4::RotationX(angle, result_cpp);
    aiMatrix4RotationX(&result_c, angle);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4RotationYTest) {
    const float angle(RandPI.next());
    aiMatrix4x4::RotationY(angle, result_cpp);
    aiMatrix4RotationY(&result_c, angle);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4RotationZTest) {
    const float angle(RandPI.next());
    aiMatrix4x4::RotationZ(angle, result_cpp);
    aiMatrix4RotationZ(&result_c, angle);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4FromRotationAroundAxisTest) {
    const float angle(RandPI.next());
    const auto axis = random_unit_vec3();
    aiMatrix4x4::Rotation(angle, axis, result_cpp);
    aiMatrix4FromRotationAroundAxis(&result_c, &axis, angle);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4TranslationTest) {
    const auto axis = random_vec3();
    aiMatrix4x4::Translation(axis, result_cpp);
    aiMatrix4Translation(&result_c, &axis);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4ScalingTest) {
    const auto scaling = random_vec3();
    aiMatrix4x4::Scaling(scaling, result_cpp);
    aiMatrix4Scaling(&result_c, &scaling);
    EXPECT_EQ(result_cpp, result_c);
}

TEST_F(AssimpAPITest_aiMatrix4x4, aiMatrix4FromToTest) {
    // Use predetermined vectors to prevent running into division by zero.
    const auto from = aiVector3D(1,2,1).Normalize(), to = aiVector3D(-1,1,1).Normalize();
    aiMatrix4x4::FromToMatrix(from, to, result_cpp);
    aiMatrix4FromTo(&result_c, &from, &to);
    EXPECT_EQ(result_cpp, result_c);
}
