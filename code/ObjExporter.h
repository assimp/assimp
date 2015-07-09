/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2015, assimp team
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

/** @file ObjExporter.h
 * Declares the exporter class to write a scene to a Collada file
 */
#ifndef AI_OBJEXPORTER_H_INC
#define AI_OBJEXPORTER_H_INC

#include <assimp/types.h>
#include <sstream>
#include <vector>
#include <map>

struct aiScene;
struct aiNode;
struct aiMesh;

namespace Assimp
{

// ------------------------------------------------------------------------------------------------
/** Helper class to export a given scene to an OBJ file. */
// ------------------------------------------------------------------------------------------------
class ObjExporter
{
public:
    /// Constructor for a specific scene to export
    ObjExporter(const char* filename, const aiScene* pScene);

public:

    std::string GetMaterialLibName();
    std::string GetMaterialLibFileName();

public:

    /// public stringstreams to write all output into
    std::ostringstream mOutput, mOutputMat;

private:

    // intermediate data structures
    struct FaceVertex
    {
        FaceVertex()
            : vp(),vn(),vt()
        {
        }

        // one-based, 0 means: 'does not exist'
        unsigned int vp,vn,vt;
    };

    struct Face {
        char kind;
        std::vector<FaceVertex> indices;
    };

    struct MeshInstance {

        std::string name, matname;
        std::vector<Face> faces;
    };

    void WriteHeader(std::ostringstream& out);

    void WriteMaterialFile();
    void WriteGeometryFile();

    std::string GetMaterialName(unsigned int index);

    void AddMesh(const aiString& name, const aiMesh* m, const aiMatrix4x4& mat);
    void AddNode(const aiNode* nd, const aiMatrix4x4& mParent);

private:

    const std::string filename;
    const aiScene* const pScene;

    std::vector<aiVector3D> vp, vn, vt;


    struct aiVectorCompare
    {
        bool operator() (const aiVector3D& a, const aiVector3D& b) const
        {
            if(a.x < b.x) return true;
            if(a.x > b.x) return false;
            if(a.y < b.y) return true;
            if(a.y > b.y) return false;
            if(a.z < b.z) return true;
            return false;
        }
    };

    class vecIndexMap
    {
        int mNextIndex;
        typedef std::map<aiVector3D, int, aiVectorCompare> dataType;
        dataType vecMap;
    public:

        vecIndexMap():mNextIndex(1)
        {}

        int getIndex(const aiVector3D& vec);
        void getVectors( std::vector<aiVector3D>& vecs );
    };

    vecIndexMap vpMap, vnMap, vtMap;
    std::vector<MeshInstance> meshes;

    // this endl() doesn't flush() the stream
    const std::string endl;
};

}

#endif
