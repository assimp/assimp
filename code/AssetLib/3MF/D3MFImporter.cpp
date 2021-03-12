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

#ifndef ASSIMP_BUILD_NO_3MF_IMPORTER

#include "D3MFImporter.h"

#include <assimp/StringComparison.h>
#include <assimp/StringUtils.h>
#include <assimp/XmlParser.h>
#include <assimp/ZipArchiveIOSystem.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/IOSystem.hpp>
#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "3MFXmlTags.h"
#include "D3MFOpcPackage.h"
#include <assimp/fast_atof.h>

#include <iomanip>

namespace Assimp {
namespace D3MF {

enum class ResourceType {
    RT_Object,
    RT_BaseMaterials,
    RT_Unknown
}; // To be extended with other resource types (eg. material extension resources like Texture2d, Texture2dGroup...)

class Resource
{
public:
    Resource(int id) :
            mId(id) {}

    virtual ~Resource() {}

    int mId;

    virtual ResourceType getType() {
        return ResourceType::RT_Unknown;
    }
};

class BaseMaterials : public Resource {
public:
    BaseMaterials(int id) :
            Resource(id),
            mMaterials(),
            mMaterialIndex() {}

    std::vector<aiMaterial *> mMaterials;
    std::vector<unsigned int> mMaterialIndex;

    virtual ResourceType getType() {
        return ResourceType::RT_BaseMaterials;
    }
};

struct Component {
    int mObjectId;
    aiMatrix4x4 mTransformation;
};

class Object : public Resource {
public:
    std::vector<aiMesh*> mMeshes;
    std::vector<unsigned int> mMeshIndex;
    std::vector<Component> mComponents;
    std::string mName;

    Object(int id) :
            Resource(id),
            mName(std::string("Object_") + ai_to_string(id)) {}

    virtual ResourceType getType() {
        return ResourceType::RT_Object;
    }
};


class XmlSerializer {
public:

    XmlSerializer(XmlParser *xmlParser) :
            mResourcesDictionnary(),
            mMaterialCount(0),
            mMeshCount(0),
            mXmlParser(xmlParser) {
        // empty
    }

    ~XmlSerializer() {
        for (auto it = mResourcesDictionnary.begin(); it != mResourcesDictionnary.end(); it++) {
            delete it->second;
        }
    }

    void ImportXml(aiScene *scene) {
        if (nullptr == scene) {
            return;
        }

        scene->mRootNode = new aiNode("3MF");

        XmlNode node = mXmlParser->getRootNode().child("model");
        if (node.empty()) {
            return;
        }
        XmlNode resNode = node.child("resources");
        for (XmlNode currentNode = resNode.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
            const std::string &currentNodeName = currentNode.name();
            if (currentNodeName == D3MF::XmlTag::object) {
                ReadObject(currentNode);;
            } else if (currentNodeName == D3MF::XmlTag::basematerials) {
                ReadBaseMaterials(currentNode);
            } else if (currentNodeName == D3MF::XmlTag::meta) {
                ReadMetadata(currentNode);
            }
        }

        XmlNode buildNode = node.child("build");
        for (XmlNode currentNode = buildNode.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
            const std::string &currentNodeName = currentNode.name();
            if (currentNodeName == D3MF::XmlTag::item) {
                int objectId = -1;
                std::string transformationMatrixStr;
                aiMatrix4x4 transformationMatrix;
                getNodeAttribute(currentNode, D3MF::XmlTag::objectid, objectId);
                bool hasTransform = getNodeAttribute(currentNode, D3MF::XmlTag::transform, transformationMatrixStr);

                auto it = mResourcesDictionnary.find(objectId);
                if (it != mResourcesDictionnary.end() && it->second->getType() == ResourceType::RT_Object) {
                    Object *obj = static_cast<Object *>(it->second);
                    if (hasTransform) {
                        transformationMatrix = parseTransformMatrix(transformationMatrixStr);
                    }

                    addObjectToNode(scene->mRootNode, obj, transformationMatrix);
                }
            }
        }


        // import the metadata
        if (!mMetaData.empty()) {
            const size_t numMeta(mMetaData.size());
            scene->mMetaData = aiMetadata::Alloc(static_cast<unsigned int>(numMeta));
            for (size_t i = 0; i < numMeta; ++i) {
                aiString val(mMetaData[i].value);
                scene->mMetaData->Set(static_cast<unsigned int>(i), mMetaData[i].name, val);
            }
        }

        // import the meshes
        scene->mNumMeshes = static_cast<unsigned int>(mMeshCount);
        if (scene->mNumMeshes != 0) {
            scene->mMeshes = new aiMesh *[scene->mNumMeshes]();
            for (auto it = mResourcesDictionnary.begin(); it != mResourcesDictionnary.end(); it++) {
                if (it->second->getType() == ResourceType::RT_Object) {
                    Object *obj = static_cast<Object*>(it->second);
                    for (unsigned int i = 0; i < obj->mMeshes.size(); ++i) {
                        scene->mMeshes[obj->mMeshIndex[i]] = obj->mMeshes[i];
                    }
                }
            }
        }
        

        // import the materials
        scene->mNumMaterials = static_cast<unsigned int>(mMaterialCount);
        if (scene->mNumMaterials != 0) {
            scene->mMaterials = new aiMaterial *[scene->mNumMaterials];
            for (auto it = mResourcesDictionnary.begin(); it != mResourcesDictionnary.end(); it++) {
                if (it->second->getType() == ResourceType::RT_BaseMaterials) {
                    BaseMaterials *baseMaterials = static_cast<BaseMaterials *>(it->second);
                    for (unsigned int i = 0; i < baseMaterials->mMaterials.size(); ++i) {
                        scene->mMaterials[baseMaterials->mMaterialIndex[i]] = baseMaterials->mMaterials[i];
                    }
                }
            }
        }
    }

private:

    void addObjectToNode(aiNode* parent, Object* obj, aiMatrix4x4 nodeTransform) {
        aiNode *sceneNode = new aiNode(obj->mName);
        sceneNode->mNumMeshes = static_cast<unsigned int>(obj->mMeshes.size());
        sceneNode->mMeshes = new unsigned int[sceneNode->mNumMeshes];
        std::copy(obj->mMeshIndex.begin(), obj->mMeshIndex.end(), sceneNode->mMeshes);

        sceneNode->mTransformation = nodeTransform;

        parent->addChildren(1, &sceneNode);

        for (size_t i = 0; i < obj->mComponents.size(); ++i) {
            Component c = obj->mComponents[i];
            auto it = mResourcesDictionnary.find(c.mObjectId);
            if (it != mResourcesDictionnary.end() && it->second->getType() == ResourceType::RT_Object) {
                addObjectToNode(sceneNode, static_cast<Object*>(it->second), c.mTransformation);
            }
            
        }
    }

    bool getNodeAttribute(const XmlNode& node, const std::string& attribute, std::string& value) {
        pugi::xml_attribute objectAttribute = node.attribute(attribute.c_str());
        if (!objectAttribute.empty()) {
            value = objectAttribute.as_string();
            return true;
        } else {
            return false;
        }
    }

    bool getNodeAttribute(const XmlNode &node, const std::string &attribute, int &value) {
        std::string strValue;
        bool ret = getNodeAttribute(node, attribute, strValue);
        if (ret) {
            value = std::atoi(strValue.c_str());
            return true;
        } else {
            return false;
        }
    }

    aiMatrix4x4 parseTransformMatrix(std::string matrixStr) {
        // split the string
        std::vector<float> numbers;
        std::string currentNumber;
        for (size_t i = 0; i < matrixStr.size(); ++i) {
            const char c = matrixStr[i];
            if (c == ' ') {
                if (currentNumber.size() > 0) {
                    float f = std::stof(currentNumber);
                    numbers.push_back(f);
                    currentNumber.clear();
                }
            } else {
                currentNumber.push_back(c);
            }
        }
        if (currentNumber.size() > 0) {
            float f = std::stof(currentNumber);
            numbers.push_back(f);
        }

        aiMatrix4x4 transformMatrix;
        transformMatrix.a1 = numbers[0];
        transformMatrix.b1 = numbers[1];
        transformMatrix.c1 = numbers[2];
        transformMatrix.d1 = 0;

        transformMatrix.a2 = numbers[3];
        transformMatrix.b2 = numbers[4];
        transformMatrix.c2 = numbers[5];
        transformMatrix.d2 = 0;

        transformMatrix.a3 = numbers[6];
        transformMatrix.b3 = numbers[7];
        transformMatrix.c3 = numbers[8];
        transformMatrix.d3 = 0;

        transformMatrix.a4 = numbers[9];
        transformMatrix.b4 = numbers[10];
        transformMatrix.c4 = numbers[11];
        transformMatrix.d4 = 1;
        return transformMatrix;
    }

    void ReadObject(XmlNode &node) {
        int id = -1, pid = -1, pindex = -1;
        bool hasId = getNodeAttribute(node, D3MF::XmlTag::id, id);
        //bool hasType = getNodeAttribute(node, D3MF::XmlTag::type, type); not used currently
        bool hasPid = getNodeAttribute(node, D3MF::XmlTag::pid, pid);
        bool hasPindex = getNodeAttribute(node, D3MF::XmlTag::pindex, pindex);

        std::string idStr = ai_to_string(id);

        if (!hasId) {
            return;
        }

        Object *obj = new Object(id);

        for (XmlNode currentNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
            const std::string &currentName = currentNode.name();
            if (currentName == D3MF::XmlTag::mesh) {
                auto mesh = ReadMesh(currentNode);
                mesh->mName.Set(idStr);

                if (hasPid) {
                    auto it = mResourcesDictionnary.find(pid);
                    if (hasPindex && it != mResourcesDictionnary.end() && it->second->getType() == ResourceType::RT_BaseMaterials) {
                        BaseMaterials *materials = static_cast<BaseMaterials *>(it->second);
                        mesh->mMaterialIndex = materials->mMaterialIndex[pindex];
                    }
                }

                obj->mMeshes.push_back(mesh);
                obj->mMeshIndex.push_back(mMeshCount);
                mMeshCount++;
            } else if (currentName == D3MF::XmlTag::components) {
                for (XmlNode currentSubNode = currentNode.first_child(); currentSubNode; currentSubNode = currentSubNode.next_sibling()) {
                    if (currentSubNode.name() == D3MF::XmlTag::component) {
                        int objectId = -1;
                        std::string componentTransformStr;
                        aiMatrix4x4 componentTransform;
                        if (getNodeAttribute(currentSubNode, D3MF::XmlTag::transform, componentTransformStr)) {
                            componentTransform = parseTransformMatrix(componentTransformStr);
                        }

                        if (getNodeAttribute(currentSubNode, D3MF::XmlTag::objectid, objectId))
                            obj->mComponents.push_back({ objectId, componentTransform });
                    }
                }
            }
        }

        mResourcesDictionnary.insert(std::make_pair(id, obj));
    }

    aiMesh *ReadMesh(XmlNode &node) {
        aiMesh *mesh = new aiMesh();

        for (XmlNode currentNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
            const std::string &currentName = currentNode.name();
            if (currentName == D3MF::XmlTag::vertices) {
                ImportVertices(currentNode, mesh);
            } else if (currentName == D3MF::XmlTag::triangles) {
                ImportTriangles(currentNode, mesh);
            }

        }

        return mesh;
    }

    void ReadMetadata(XmlNode &node) {
        pugi::xml_attribute attribute = node.attribute(D3MF::XmlTag::meta_name.c_str());
        const std::string name = attribute.as_string();
        const std::string value = node.value();
        if (name.empty()) {
            return;
        }

        MetaEntry entry;
        entry.name = name;
        entry.value = value;
        mMetaData.push_back(entry);
    }

    void ImportVertices(XmlNode &node, aiMesh *mesh) {
        std::vector<aiVector3D> vertices;
        for (XmlNode currentNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
            const std::string &currentName = currentNode.name();
            if (currentName == D3MF::XmlTag::vertex) {
                vertices.push_back(ReadVertex(currentNode));
            }
        }

        mesh->mNumVertices = static_cast<unsigned int>(vertices.size());
        mesh->mVertices = new aiVector3D[mesh->mNumVertices];
        std::copy(vertices.begin(), vertices.end(), mesh->mVertices);
    }

    aiVector3D ReadVertex(XmlNode &node) {
        aiVector3D vertex;
        vertex.x = ai_strtof(node.attribute(D3MF::XmlTag::x.c_str()).as_string(), nullptr);
        vertex.y = ai_strtof(node.attribute(D3MF::XmlTag::y.c_str()).as_string(), nullptr);
        vertex.z = ai_strtof(node.attribute(D3MF::XmlTag::z.c_str()).as_string(), nullptr);

        return vertex;
    }

    void ImportTriangles(XmlNode &node, aiMesh *mesh) {
        std::vector<aiFace> faces;
        for (XmlNode currentNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
            const std::string &currentName = currentNode.name();
            if (currentName == D3MF::XmlTag::triangle) {
                aiFace face = ReadTriangle(currentNode);
                faces.push_back(face);

                int pid = 0, p1;
                bool hasPid = getNodeAttribute(currentNode, D3MF::XmlTag::pid, pid);
                bool hasP1 = getNodeAttribute(currentNode, D3MF::XmlTag::p1, p1);

                if (hasPid && hasP1) {
                    auto it = mResourcesDictionnary.find(pid);
                    if (it != mResourcesDictionnary.end())
                    {
                        if (it->second->getType() == ResourceType::RT_BaseMaterials) {
                            BaseMaterials *baseMaterials = static_cast<BaseMaterials *>(it->second);
                            mesh->mMaterialIndex = baseMaterials->mMaterialIndex[p1];
                        }
                        // TODO: manage the separation into several meshes if the triangles of the mesh do not all refer to the same material
                    }
                }
            }
        }

        mesh->mNumFaces = static_cast<unsigned int>(faces.size());
        mesh->mFaces = new aiFace[mesh->mNumFaces];
        mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

        std::copy(faces.begin(), faces.end(), mesh->mFaces);
    }

    aiFace ReadTriangle(XmlNode &node) {
        aiFace face;

        face.mNumIndices = 3;
        face.mIndices = new unsigned int[face.mNumIndices];
        face.mIndices[0] = static_cast<unsigned int>(std::atoi(node.attribute(D3MF::XmlTag::v1.c_str()).as_string()));
        face.mIndices[1] = static_cast<unsigned int>(std::atoi(node.attribute(D3MF::XmlTag::v2.c_str()).as_string()));
        face.mIndices[2] = static_cast<unsigned int>(std::atoi(node.attribute(D3MF::XmlTag::v3.c_str()).as_string()));

        return face;
    }

    void ReadBaseMaterials(XmlNode &node) {
        int id = -1;
        if (getNodeAttribute(node, D3MF::XmlTag::basematerials_id, id)) {
            BaseMaterials* baseMaterials = new BaseMaterials(id);

            for (XmlNode currentNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling())
            {
                if (currentNode.name() == D3MF::XmlTag::basematerials_base) {
                    baseMaterials->mMaterialIndex.push_back(mMaterialCount);
                    baseMaterials->mMaterials.push_back(readMaterialDef(currentNode, id));
                    mMaterialCount++;
                }
            }

            mResourcesDictionnary.insert(std::make_pair(id, baseMaterials));
        }
    }

    bool parseColor(const char *color, aiColor4D &diffuse) {
        if (nullptr == color) {
            return false;
        }

        //format of the color string: #RRGGBBAA or #RRGGBB (3MF Core chapter 5.1.1)
        const size_t len(strlen(color));
        if (9 != len && 7 != len) {
            return false;
        }

        const char *buf(color);
        if ('#' != buf[0]) {
            return false;
        }

        char r[3] = { buf[1], buf[2], '\0' };
        diffuse.r = static_cast<ai_real>(strtol(r, nullptr, 16)) / ai_real(255.0);

        char g[3] = { buf[3], buf[4], '\0' };
        diffuse.g = static_cast<ai_real>(strtol(g, nullptr, 16)) / ai_real(255.0);

        char b[3] = { buf[5], buf[6], '\0' };
        diffuse.b = static_cast<ai_real>(strtol(b, nullptr, 16)) / ai_real(255.0);

        if (7 == len)
            return true;

        char a[3] = { buf[7], buf[8], '\0' };
        diffuse.a = static_cast<ai_real>(strtol(a, nullptr, 16)) / ai_real(255.0);

        return true;
    }

    void assignDiffuseColor(XmlNode &node, aiMaterial *mat) {
        const char *color = node.attribute(D3MF::XmlTag::basematerials_displaycolor.c_str()).as_string();
        aiColor4D diffuse;
        if (parseColor(color, diffuse)) {
            mat->AddProperty<aiColor4D>(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
        }
    }

    aiMaterial *readMaterialDef(XmlNode &node, unsigned int basematerialsId) {
        aiMaterial *material = new aiMaterial();
        material->mNumProperties = 0;
        std::string name;
        bool hasName = getNodeAttribute(node, D3MF::XmlTag::basematerials_name, name);

        std::string stdMaterialName;
        std::string strId(ai_to_string(basematerialsId));
        stdMaterialName += "id";
        stdMaterialName += strId;
        stdMaterialName += "_";
        if (hasName) {
            stdMaterialName += std::string(name);
        } else {
            stdMaterialName += "basemat_";
            stdMaterialName += ai_to_string(mMaterialCount - basematerialsId);
        }

        aiString assimpMaterialName(stdMaterialName);
        material->AddProperty(&assimpMaterialName, AI_MATKEY_NAME);

        assignDiffuseColor(node, material);

        return material;
    }

private:
    struct MetaEntry {
        std::string name;
        std::string value;
    };
    std::vector<MetaEntry> mMetaData;
    std::map<unsigned int, Resource*> mResourcesDictionnary;
    unsigned int mMaterialCount, mMeshCount;
    XmlParser *mXmlParser;
};

} //namespace D3MF

static const aiImporterDesc desc = {
    "3mf Importer",
    "",
    "",
    "http://3mf.io/",
    aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_SupportCompressedFlavour,
    0,
    0,
    0,
    0,
    "3mf"
};

D3MFImporter::D3MFImporter() :
        BaseImporter() {
    // empty
}

D3MFImporter::~D3MFImporter() {
    // empty
}

bool D3MFImporter::CanRead(const std::string &filename, IOSystem *pIOHandler, bool checkSig) const {
    const std::string extension(GetExtension(filename));
    if (extension == desc.mFileExtensions) {
        return true;
    } else if (!extension.length() || checkSig) {
        if (nullptr == pIOHandler) {
            return false;
        }
        if (!ZipArchiveIOSystem::isZipArchive(pIOHandler, filename)) {
            return false;
        }
        D3MF::D3MFOpcPackage opcPackage(pIOHandler, filename);
        return opcPackage.validate();
    }

    return false;
}

void D3MFImporter::SetupProperties(const Importer * /*pImp*/) {
    // empty
}

const aiImporterDesc *D3MFImporter::GetInfo() const {
    return &desc;
}

void D3MFImporter::InternReadFile(const std::string &filename, aiScene *pScene, IOSystem *pIOHandler) {
    D3MF::D3MFOpcPackage opcPackage(pIOHandler, filename);

    XmlParser xmlParser;
    if (xmlParser.parse(opcPackage.RootStream())) {
        D3MF::XmlSerializer xmlSerializer(&xmlParser);
        xmlSerializer.ImportXml(pScene);
    }
}

} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_3MF_IMPORTER
