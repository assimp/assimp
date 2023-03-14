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
