#pragma once

#include <assimp/vector2.h>
#include <assimp/vector3.h>
#include <assimp/color4.h>
#include <assimp/types.h>

#include <list>
#include <vector>

struct aiFace;
struct aiMesh;

namespace Assimp {

class X3DGeoHelper {
public:
    static aiVector3D make_point2D(const float angle, const float radius);
    static void make_arc2D(const float pStartAngle, const float pEndAngle, const float pRadius, size_t pNumSegments, std::list<aiVector3D>& pVertices);
    static void extend_point_to_line(const std::list<aiVector3D>& pPoint, std::list<aiVector3D>& pLine);
    static void rect_parallel_epiped(const aiVector3D& pSize, std::list<aiVector3D>& pVertices);
    static void coordIdx_str2faces_arr(const std::vector<int32_t>& pCoordIdx, std::vector<aiFace>& pFaces, unsigned int& pPrimitiveTypes);
    static aiMesh* make_mesh(const std::vector<int32_t>& pCoordIdx, const std::list<aiVector3D>& pVertices);
};

} // namespace Assimp
