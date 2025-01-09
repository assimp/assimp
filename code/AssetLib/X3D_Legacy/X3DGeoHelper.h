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
};

} // namespace Assimp
