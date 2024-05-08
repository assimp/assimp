/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

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

----------------------------------------------------------------------
*/

#include <assimp/types.h>
#include <assimp/mesh.h>

namespace Assimp {

// ---------------------------------------------------------------------------
/// @brief This helper class supports some basic geometry algorithms.
// ---------------------------------------------------------------------------
class ASSIMP_API GeometryUtils {
public:
    /// @brief Will calculate the area of a triangle.
    /// @param a  The first vertex of the triangle.
    /// @param b  The first vertex of the triangle.
    /// @param c  The first vertex of the triangle.
    static ai_real heron( ai_real a, ai_real b, ai_real c );
    
    /// @brief Will compute the distance between 2 3D-vectors
    /// @param vA  Vector a.
    /// @param vB  Vector b.
    /// @return The distance.
    static ai_real distance3D( const aiVector3D &vA, const aiVector3D &vB );

    /// @brief Will calculate the area of a triangle described by a aiFace.
    /// @param face   The face
    /// @param mesh   The mesh containing the face
    /// @return The area.
    static ai_real calculateAreaOfTriangle( const aiFace& face, aiMesh* mesh );

    /// @brief Will calculate the intersection between a ray and a plane
    /// @param ray          The ray to test for
    /// @param planePos     A point on the plane
    /// @param planeNormal  The plane normal to describe its orientation
    /// @param pos          The position of the intersection.
    /// @return true is an intersection was detected, false if not.
    static bool PlaneIntersect(const aiRay& ray, const aiVector3D& planePos, const aiVector3D& planeNormal, aiVector3D& pos);

    /// @brief Will normalize an array of vectors.
    /// @param vectorArrayIn    The incoming arra of vectors.
    /// @param vectorArrayOut   The normalized vectors.
    /// @param numVectors       The array size.
    static void normalizeVectorArray(aiVector3D *vectorArrayIn, aiVector3D *vectorArrayOut, size_t numVectors);
};

} // namespace Assimp
