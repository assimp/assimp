#include "X3DGeoHelper.h"
#include "X3DImporter.hpp"
#include "X3DImporter_Macro.hpp"

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

#define MACRO_FACE_ADD_QUAD_FA(pCCW, pOut, pIn, pP1, pP2, pP3, pP4) \
	do { \
        if(pCCW) { \
            pOut.push_back(pIn[pP1]); \
            pOut.push_back(pIn[pP2]); \
            pOut.push_back(pIn[pP3]); \
            pOut.push_back(pIn[pP4]); \
        } else { \
            pOut.push_back(pIn[pP4]); \
            pOut.push_back(pIn[pP3]); \
            pOut.push_back(pIn[pP2]); \
            pOut.push_back(pIn[pP1]); \
        } \
	} while(false)

#define MESH_RectParallelepiped_CREATE_VERT \
aiVector3D vert_set[8]; \
float x1, x2, y1, y2, z1, z2, hs; \
 \
	hs = pSize.x / 2, x1 = -hs, x2 = hs; \
	hs = pSize.y / 2, y1 = -hs, y2 = hs; \
	hs = pSize.z / 2, z1 = -hs, z2 = hs; \
	vert_set[0].Set(x2, y1, z2); \
	vert_set[1].Set(x2, y2, z2); \
	vert_set[2].Set(x2, y2, z1); \
	vert_set[3].Set(x2, y1, z1); \
	vert_set[4].Set(x1, y1, z2); \
	vert_set[5].Set(x1, y2, z2); \
	vert_set[6].Set(x1, y2, z1); \
	vert_set[7].Set(x1, y1, z1)

void X3DGeoHelper::rect_parallel_epiped(const aiVector3D& pSize, std::list<aiVector3D>& pVertices) {
    MESH_RectParallelepiped_CREATE_VERT;
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 3, 2, 1, 0);// front
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 6, 7, 4, 5);// back
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 7, 3, 0, 4);// left
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 2, 6, 5, 1);// right
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 0, 1, 5, 4);// top
    MACRO_FACE_ADD_QUAD_FA(true, pVertices, vert_set, 7, 6, 2, 3);// bottom
}

#undef MESH_RectParallelepiped_CREATE_VERT

void X3DGeoHelper::coordIdx_str2faces_arr(const std::vector<int32_t>& pCoordIdx, std::vector<aiFace>& pFaces, unsigned int& pPrimitiveTypes) {
    std::vector<int32_t> f_data(pCoordIdx);
    std::vector<unsigned int> inds;
    unsigned int prim_type = 0;

    if ( f_data.back() != ( -1 ) ) {
        f_data.push_back( -1 );
    }

    // reserve average size.
    pFaces.reserve(f_data.size() / 3);
    inds.reserve(4);
    //PrintVectorSet("build. ci", pCoordIdx);
    for(std::vector<int32_t>::iterator it = f_data.begin(); it != f_data.end(); ++it) {
        // when face is got count how many indices in it.
        if(*it == (-1)) {
            aiFace tface;
            size_t ts;

            ts = inds.size();
            switch (ts) {
            case 0:
                goto mg_m_err;
            case 1:
                prim_type |= aiPrimitiveType_POINT;
                break;
            case 2:
                prim_type |= aiPrimitiveType_LINE;
                break;
            case 3:
                prim_type |= aiPrimitiveType_TRIANGLE;
                break;
            default:
                prim_type |= aiPrimitiveType_POLYGON;
                break;
            }

            tface.mNumIndices = static_cast<unsigned int>(ts);
            tface.mIndices = new unsigned int[ts];
            memcpy(tface.mIndices, inds.data(), ts * sizeof(unsigned int));
            pFaces.push_back(tface);
            inds.clear();
        }// if(*it == (-1))
        else {
            inds.push_back(*it);
        }// if(*it == (-1)) else
    }// for(std::list<int32_t>::iterator it = f_data.begin(); it != f_data.end(); it++)
    //PrintVectorSet("build. faces", pCoordIdx);

    pPrimitiveTypes = prim_type;

    return;

mg_m_err:
    for(size_t i = 0, i_e = pFaces.size(); i < i_e; i++)
        delete [] pFaces.at(i).mIndices;

    pFaces.clear();
}

aiMesh* X3DGeoHelper::make_mesh(const std::vector<int32_t>& pCoordIdx, const std::list<aiVector3D>& pVertices) {
    std::vector<aiFace> faces;
    unsigned int prim_type = 0;

    // create faces array from input string with vertices indices.
    X3DGeoHelper::coordIdx_str2faces_arr(pCoordIdx, faces, prim_type);
    if ( !faces.size() ) {
        throw DeadlyImportError( "Failed to create mesh, faces list is empty." );
    }

    //
    // Create new mesh and copy geometry data.
    //
    aiMesh *tmesh = new aiMesh;
    size_t ts = faces.size();
    // faces
    tmesh->mFaces = new aiFace[ts];
    tmesh->mNumFaces = static_cast<unsigned int>(ts);
    for(size_t i = 0; i < ts; i++)
        tmesh->mFaces[i] = faces.at(i);

    // vertices
    std::list<aiVector3D>::const_iterator vit = pVertices.begin();

    ts = pVertices.size();
    tmesh->mVertices = new aiVector3D[ts];
    tmesh->mNumVertices = static_cast<unsigned int>(ts);
    for ( size_t i = 0; i < ts; i++ ) {
        tmesh->mVertices[ i ] = *vit++;
    }

    // set primitives type and return result.
    tmesh->mPrimitiveTypes = prim_type;

    return tmesh;
}

} // namespace Assimp
