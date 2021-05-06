/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

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

/** @file XGLLoader.h
 *  @brief Declaration of the .xgl/.zgl
 */
#ifndef AI_XGLLOADER_H_INCLUDED
#define AI_XGLLOADER_H_INCLUDED

#include <assimp/BaseImporter.h>
#include <assimp/XmlParser.h>
#include <assimp/LogAux.h>
#include <assimp/light.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/light.h>
#include <assimp/Importer.hpp>
#include <assimp/XmlParser.h>

#include <map>
#include <memory>

struct aiNode;

namespace Assimp {

// ---------------------------------------------------------------------------
/** XGL/ZGL importer.
 *
 * Spec: http://vizstream.aveva.com/release/vsplatform/XGLSpec.htm
 */
class XGLImporter : public BaseImporter, public LogFunctions<XGLImporter> {
public:
    XGLImporter();
    ~XGLImporter();

    // -------------------------------------------------------------------
    /** Returns whether the class can handle the format of the given file.
     *  See BaseImporter::CanRead() for details.    */
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler,
            bool checkSig) const;

protected:
    // -------------------------------------------------------------------
    /** Return importer meta information.
     * See #BaseImporter::GetInfo for the details  */
    const aiImporterDesc *GetInfo() const;

    // -------------------------------------------------------------------
    /** Imports the given file into the given scene structure.
     * See BaseImporter::InternReadFile() for details */
    void InternReadFile(const std::string &pFile, aiScene *pScene,
            IOSystem *pIOHandler);

private:
    struct TempScope {
        TempScope() :
                light() {
            // empty
        }

        ~TempScope() {
            for (aiMesh *m : meshes_linear) {
                delete m;
            }

            for (aiMaterial *m : materials_linear) {
                delete m;
            }

            delete light;
        }

        void dismiss() {
            light = nullptr;
            meshes_linear.clear();
            materials_linear.clear();
            meshes.clear();
            materials.clear();
        }

        std::multimap<unsigned int, aiMesh *> meshes;
        std::map<unsigned int, aiMaterial *> materials;

        std::vector<aiMesh *> meshes_linear;
        std::vector<aiMaterial *> materials_linear;

        aiLight *light;
    };

    struct SortMeshByMaterialId {
        SortMeshByMaterialId(const TempScope &scope) :
                scope(scope) {
            // empty
        }
        bool operator()(unsigned int a, unsigned int b) const {
            return scope.meshes_linear[a]->mMaterialIndex < scope.meshes_linear[b]->mMaterialIndex;
        };

        const TempScope &scope;
    };

    struct TempMesh {
        std::map<unsigned int, aiVector3D> points;
        std::map<unsigned int, aiVector3D> normals;
        std::map<unsigned int, aiVector2D> uvs;
    };

    struct TempMaterialMesh {
        TempMaterialMesh() :
                pflags(),
                matid() {
            // empty
        }

        std::vector<aiVector3D> positions, normals;
        std::vector<aiVector2D> uvs;

        std::vector<unsigned int> vcounts;
        unsigned int pflags;
        unsigned int matid;
    };

    struct TempFace {
        TempFace() :
                has_uv(),
                has_normal() {
            // empty
        }

        aiVector3D pos;
        aiVector3D normal;
        aiVector2D uv;
        bool has_uv;
        bool has_normal;
    };

private:
    void Cleanup();

    std::string GetElementName();
    bool ReadElement();
    bool ReadElementUpToClosing(const char *closetag);
    bool SkipToText();
    unsigned int ReadIDAttr(XmlNode &node);

    void ReadWorld(XmlNode &node, TempScope &scope);
    void ReadLighting(XmlNode &node, TempScope &scope);
    aiLight *ReadDirectionalLight(XmlNode &node);
    aiNode *ReadObject(XmlNode &node, TempScope &scope, bool skipFirst = false/*, const char *closetag = "object"*/);
    bool ReadMesh(XmlNode &node, TempScope &scope);
    void ReadMaterial(XmlNode &node, TempScope &scope);
    aiVector2D ReadVec2(XmlNode &node);
    aiVector3D ReadVec3(XmlNode &node);
    aiColor3D ReadCol3(XmlNode &node);
    aiMatrix4x4 ReadTrafo(XmlNode &node);
    unsigned int ReadIndexFromText(XmlNode &node);
    float ReadFloat(XmlNode &node);

    aiMesh *ToOutputMesh(const TempMaterialMesh &m);
    void ReadFaceVertex(XmlNode &node, const TempMesh &t, TempFace &out);
    unsigned int ResolveMaterialRef(XmlNode &node, TempScope &scope);

private:
    XmlParser *mXmlParser;
    aiScene *m_scene;
};

} // end of namespace Assimp

#endif // AI_IRRMESHIMPORTER_H_INC
