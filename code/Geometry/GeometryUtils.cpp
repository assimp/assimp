/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

#include "GeometryUtils.h"

#include <assimp/vector3.h>

namespace Assimp {

ai_real GeometryUtils::heron( ai_real a, ai_real b, ai_real c ) {
    ai_real s = (a + b + c) / 2;
    ai_real area = pow((s * ( s - a ) * ( s - b ) * ( s - c ) ), (ai_real)0.5 );
    return area;
}

ai_real GeometryUtils::distance3D( const aiVector3D &vA, aiVector3D &vB ) {
    const ai_real lx = ( vB.x - vA.x );
    const ai_real ly = ( vB.y - vA.y );
    const ai_real lz = ( vB.z - vA.z );
    ai_real a = lx*lx + ly*ly + lz*lz;
    ai_real d = pow( a, (ai_real)0.5 );

    return d;
}

ai_real GeometryUtils::calculateAreaOfTriangle( const aiFace& face, aiMesh* mesh ) {
    ai_real area = 0;

    aiVector3D vA( mesh->mVertices[ face.mIndices[ 0 ] ] );
    aiVector3D vB( mesh->mVertices[ face.mIndices[ 1 ] ] );
    aiVector3D vC( mesh->mVertices[ face.mIndices[ 2 ] ] );

    ai_real a( distance3D( vA, vB ) );
    ai_real b( distance3D( vB, vC ) );
    ai_real c( distance3D( vC, vA ) );
    area = heron( a, b, c );

    return area;
}

} // namespace Assimp
