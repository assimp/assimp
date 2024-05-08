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

#ifndef ASSIMP_MATH_TEST_H
#define ASSIMP_MATH_TEST_H

#include "UnitTestPCH.h"
#include <assimp/types.h>
#include "RandomNumberGeneration.h"

namespace Assimp {

/** Custom test class providing several math related utilities. */
class AssimpMathTest : public ::testing::Test {
public:
    /** Return a random non-null 2D vector. */
    inline static aiVector2D random_vec2() {
        return aiVector2D(RandNonZero.next(), RandNonZero.next());
    }

    /** Return a random non-null 3D vector. */
    inline static aiVector3D random_vec3() {
        return aiVector3D(RandNonZero.next(), RandNonZero.next(),RandNonZero.next());
    }

    /** Return a random unit 3D vector. */
    inline static aiVector3D random_unit_vec3() {
        return random_vec3().NormalizeSafe();
    }

    /** Return a quaternion with random orientation and
      * rotation angle around axis. */
    inline static aiQuaternion random_quat() {
        return aiQuaternion(random_unit_vec3(), RandPI.next());
    }

    /** Return a random non-null 3x3 matrix. */
    inline static aiMatrix3x3 random_mat3() {
        return aiMatrix3x3(
            RandNonZero.next(), RandNonZero.next(),RandNonZero.next(),
            RandNonZero.next(), RandNonZero.next(),RandNonZero.next(),
            RandNonZero.next(), RandNonZero.next(),RandNonZero.next());
    }

    /** Return a random non-null 4x4 matrix. */
    inline static aiMatrix4x4 random_mat4() {
        return aiMatrix4x4(
            RandNonZero.next(), RandNonZero.next(),RandNonZero.next(), RandNonZero.next(),
            RandNonZero.next(), RandNonZero.next(),RandNonZero.next(), RandNonZero.next(),
            RandNonZero.next(), RandNonZero.next(),RandNonZero.next(), RandNonZero.next(),
            RandNonZero.next(), RandNonZero.next(),RandNonZero.next(), RandNonZero.next());
    }

    /** Epsilon value to use in tests. */
    static const float Epsilon;

    /** Random number generators. */
    static RandomUniformFloatGenerator RandNonZero, RandPI;
};

}

#endif // ASSIMP_MATH_TEST_H
