#include "X3DGeoHelper.h"
#include "X3DImporter.hpp"

#include <assimp/vector3.h>
#include <assimp/Exceptional.h>
#include <assimp/StringUtils.h>

#include <vector>

namespace Assimp {

aiVector3D X3DGeoHelper::make_point2D(const float angle, const float radius) {
    return aiVector3D(radius * std::cos(angle), radius * std::sin(angle), 0);
}

void X3DGeoHelper::make_arc2D(const float pStartAngle, const float pEndAngle, const float pRadius, size_t numSegments, std::list<aiVector3D>& pVertices) {
    // check argument values ranges.
    if ( ( pStartAngle < -AI_MATH_TWO_PI_F ) || ( pStartAngle > AI_MATH_TWO_PI_F ) ) {
        throw DeadlyImportError( "GeometryHelper_Make_Arc2D.pStartAngle" );
    }
    if ( ( pEndAngle < -AI_MATH_TWO_PI_F ) || ( pEndAngle > AI_MATH_TWO_PI_F ) ) {
        throw DeadlyImportError( "GeometryHelper_Make_Arc2D.pEndAngle" );
    }
    if ( pRadius <= 0 ) {
        throw DeadlyImportError( "GeometryHelper_Make_Arc2D.pRadius" );
    }

    // calculate arc angle and check type of arc
    float angle_full = std::fabs(pEndAngle - pStartAngle);
    if ( ( angle_full > AI_MATH_TWO_PI_F ) || ( angle_full == 0.0f ) ) {
        angle_full = AI_MATH_TWO_PI_F;
    }

    // calculate angle for one step - angle to next point of line.
    float angle_step = angle_full / (float)numSegments;
    // make points
    for (size_t pi = 0; pi <= numSegments; pi++) {
        float tangle = pStartAngle + pi * angle_step;
        pVertices.push_back(make_point2D(tangle, pRadius));
    }// for(size_t pi = 0; pi <= numSegments; pi++)

    // if we making full circle then add last vertex equal to first vertex
    if(angle_full == AI_MATH_TWO_PI_F) pVertices.push_back(*pVertices.begin());
}

void X3DGeoHelper::extend_point_to_line(const std::list<aiVector3D>& pPoint, std::list<aiVector3D>& pLine) {
    std::list<aiVector3D>::const_iterator pit = pPoint.begin();
    std::list<aiVector3D>::const_iterator pit_last = pPoint.end();

    --pit_last;

    if ( pPoint.size() < 2 ) {
        throw DeadlyImportError( "GeometryHelper_Extend_PointToLine.pPoint.size() can not be less than 2." );
    }

    // add first point of first line.
    pLine.push_back(*pit++);
    // add internal points
    while(pit != pit_last) {
        pLine.push_back(*pit);// second point of previous line
        pLine.push_back(*pit);// first point of next line
        ++pit;
    }
    // add last point of last line
    pLine.push_back(*pit);
}

} // namespace Assimp
