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
    static aiVector3D make_point2D(float angle, float radius);
    static void make_arc2D(float pStartAngle, float pEndAngle, float pRadius, size_t numSegments, std::list<aiVector3D> &pVertices);
    static void extend_point_to_line(const std::list<aiVector3D> &pPoint, std::list<aiVector3D> &pLine);
    static void polylineIdx_to_lineIdx(const std::list<int32_t> &pPolylineCoordIdx, std::list<int32_t> &pLineCoordIdx);
    static void rect_parallel_epiped(const aiVector3D &pSize, std::list<aiVector3D> &pVertices);
    static void coordIdx_str2faces_arr(const std::vector<int32_t> &pCoordIdx, std::vector<aiFace> &pFaces, unsigned int &pPrimitiveTypes);
    static void coordIdx_str2lines_arr(const std::vector<int32_t> &pCoordIdx, std::vector<aiFace> &pFaces);
    static void add_color(aiMesh &pMesh, const std::list<aiColor3D> &pColors, const bool pColorPerVertex);
    static void add_color(aiMesh &pMesh, const std::list<aiColor4D> &pColors, const bool pColorPerVertex);
    static void add_color(aiMesh &pMesh, const std::vector<int32_t> &pCoordIdx, const std::vector<int32_t> &pColorIdx,
            const std::list<aiColor3D> &pColors, const bool pColorPerVertex);
    static void add_color(aiMesh &pMesh, const std::vector<int32_t> &pCoordIdx, const std::vector<int32_t> &pColorIdx,
            const std::list<aiColor4D> &pColors, const bool pColorPerVertex);
    static void add_normal(aiMesh &pMesh, const std::vector<int32_t> &pCoordIdx, const std::vector<int32_t> &pNormalIdx,
            const std::list<aiVector3D> &pNormals, const bool pNormalPerVertex);
    static void add_normal(aiMesh &pMesh, const std::list<aiVector3D> &pNormals, const bool pNormalPerVertex);
    static void add_tex_coord(aiMesh &pMesh, const std::vector<int32_t> &pCoordIdx, const std::vector<int32_t> &pTexCoordIdx,
            const std::list<aiVector2D> &pTexCoords);
    static void add_tex_coord(aiMesh &pMesh, const std::list<aiVector2D> &pTexCoords);
    static aiMesh *make_mesh(const std::vector<int32_t> &pCoordIdx, const std::list<aiVector3D> &pVertices);
    static aiMesh *make_line_mesh(const std::vector<int32_t> &pCoordIdx, const std::list<aiVector3D> &pVertices);
};

} // namespace Assimp
