#include <assimp/types.h>
#include <assimp/mesh.h>

namespace Assimp {

class GeometryUtils {
public:
    static ai_real heron( ai_real a, ai_real b, ai_real c );
    static ai_real distance3D( const aiVector3D &vA, aiVector3D &vB );
    static ai_real calculateAreaOfTriangle( const aiFace& face, aiMesh* mesh );
};

} // namespace Assimp
