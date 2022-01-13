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
#pragma once

#include <assimp/XmlParser.h>
#include <assimp/mesh.h>
#include <vector>
#include <map>

struct aiNode;
struct aiMesh;
struct aiMaterial;

namespace Assimp {
namespace D3MF {

class Resource;
class D3MFOpcPackage;
class Object;
class Texture2DGroup;
class EmbeddedTexture;

class XmlSerializer {
public:
    XmlSerializer(XmlParser *xmlParser);
    ~XmlSerializer();
    void ImportXml(aiScene *scene);

private:
    void addObjectToNode(aiNode *parent, Object *obj, aiMatrix4x4 nodeTransform);
    void ReadObject(XmlNode &node);
    aiMesh *ReadMesh(XmlNode &node);
    void ReadMetadata(XmlNode &node);
    void ImportVertices(XmlNode &node, aiMesh *mesh);
    void ImportTriangles(XmlNode &node, aiMesh *mesh);
    void ReadBaseMaterials(XmlNode &node);
    void ReadEmbeddecTexture(XmlNode &node);
    void StoreEmbeddedTexture(EmbeddedTexture *tex);
    void ReadTextureCoords2D(XmlNode &node, Texture2DGroup *tex2DGroup);
    void ReadTextureGroup(XmlNode &node);
    aiMaterial *readMaterialDef(XmlNode &node, unsigned int basematerialsId);
    void StoreMaterialsInScene(aiScene *scene);

private:
    struct MetaEntry {
        std::string name;
        std::string value;
    };
    std::vector<MetaEntry> mMetaData;
    std::vector<EmbeddedTexture *> mEmbeddedTextures;
    std::vector<aiMaterial *> mMaterials;
    std::map<unsigned int, Resource *> mResourcesDictionnary;
    unsigned int mMeshCount;
    XmlParser *mXmlParser;
};

} // namespace D3MF
} // namespace Assimp
