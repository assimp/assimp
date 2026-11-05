/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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
#include "XmlSerializer.h"
#include "D3MFOpcPackage.h"
#include "3MFXmlTags.h"
#include "3MFTypes.h"
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>

#include <set>
#include <utility>

namespace Assimp {
namespace D3MF {

static constexpr int IdNotSet = -1;

namespace {

static constexpr size_t ColRGBA_Len = 9;
static constexpr size_t ColRGB_Len = 7;

// format of the color string: #RRGGBBAA or #RRGGBB (3MF Core chapter 5.1.1)
bool validateColorString(const std::string color) {
    const size_t len = color.size();
    if (ColRGBA_Len != len && ColRGB_Len != len) {
        return false;
    }

    return true;
}

aiFace ReadTriangle(XmlNode &node, int &texId0, int &texId1, int &texId2) {
    aiFace face;

    face.mNumIndices = 3;
    face.mIndices = new unsigned int[face.mNumIndices];
    face.mIndices[0] = static_cast<unsigned int>(std::atoi(node.attribute(XmlTag::v1).as_string()));
    face.mIndices[1] = static_cast<unsigned int>(std::atoi(node.attribute(XmlTag::v2).as_string()));
    face.mIndices[2] = static_cast<unsigned int>(std::atoi(node.attribute(XmlTag::v3).as_string()));

    texId0 = texId1 = texId2 = IdNotSet;
    XmlParser::getIntAttribute(node, XmlTag::p1, texId0);
    XmlParser::getIntAttribute(node, XmlTag::p2, texId1);
    XmlParser::getIntAttribute(node, XmlTag::p3, texId2);

    return face;
}

aiVector3D ReadVertex(XmlNode &node) {
    aiVector3D vertex;
    vertex.x = ai_strtof(node.attribute(XmlTag::x).as_string(), nullptr);
    vertex.y = ai_strtof(node.attribute(XmlTag::y).as_string(), nullptr);
    vertex.z = ai_strtof(node.attribute(XmlTag::z).as_string(), nullptr);

    return vertex;
}

bool getNodeAttribute(const XmlNode &node, const std::string &attribute, std::string &value) {
    pugi::xml_attribute objectAttribute = node.attribute(attribute.c_str());
    if (!objectAttribute.empty()) {
        value = objectAttribute.as_string();
        return true;
    }

    return false;
}

bool getNodeAttribute(const XmlNode &node, const std::string &attribute, int &value) {
    std::string strValue;
    const bool ret = getNodeAttribute(node, attribute, strValue);
    if (ret) {
        value = std::atoi(strValue.c_str());
        return true;
    }

    return false;
}

aiMatrix4x4 parseTransformMatrix(const std::string& matrixStr) {
    // split the string
    std::vector<float> numbers;
    std::string currentNumber;
    for (char c : matrixStr) {
        if (c == ' ') {
            if (!currentNumber.empty()) {
                numbers.push_back(ai_strtof(currentNumber.c_str(), nullptr));
                currentNumber.clear();
            }
        } else {
            currentNumber.push_back(c);
        }
    }
    if (!currentNumber.empty()) {
        numbers.push_back(ai_strtof(currentNumber.c_str(), nullptr));
    }

    // A 3MF transform is a row-major 4x3 affine matrix (3MF Core 3.3); the implicit
    // fourth column is (0,0,0,1). Anything shorter is malformed, so fall back to identity.
    constexpr size_t AffineMatrixValueCount = 12;
    if (numbers.size() < AffineMatrixValueCount) {
        return aiMatrix4x4();
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

bool parseColor(const std::string &color, aiColor4D &diffuse) {
    if (color.empty()) {
        return false;
    }

    if (!validateColorString(color)) {
        return false;
    }

    if ('#' != color[0]) {
        return false;
    }

    char r[3] = { color[1], color[2], '\0' };
    diffuse.r = static_cast<ai_real>(strtol(r, nullptr, 16)) / ai_real(255.0);

    char g[3] = { color[3], color[4], '\0' };
    diffuse.g = static_cast<ai_real>(strtol(g, nullptr, 16)) / ai_real(255.0);

    char b[3] = { color[5], color[6], '\0' };
    diffuse.b = static_cast<ai_real>(strtol(b, nullptr, 16)) / ai_real(255.0);
    const size_t len = color.size();
    if (ColRGB_Len == len) {
        return true;
    }

    char a[3] = { color[7], color[8], '\0' };
    diffuse.a = static_cast<ai_real>(strtol(a, nullptr, 16)) / ai_real(255.0);

    return true;
}

void assignDiffuseColor(XmlNode &node, aiMaterial *mat) {
    const char *color = node.attribute(XmlTag::basematerials_displaycolor).as_string();
    aiColor4D diffuse;
    if (parseColor(color, diffuse)) {
        mat->AddProperty<aiColor4D>(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
    }
}

} // namespace

XmlSerializer::XmlSerializer(XmlParser &xmlParser, D3MFOpcPackage *package) :
        mResourcesByFile(),
        mCurrentResources(nullptr),
        mMeshCount(0),
        mXmlParser(xmlParser),
        mPackage(package) {
    // empty
}

XmlSerializer::~XmlSerializer() {
    for (auto &file : mResourcesByFile) {
        for (auto &it : file.second) {
            delete it.second;
        }
    }
}

void XmlSerializer::ReadModel(XmlNode &modelNode, const std::string &fileKey) {
    mCurrentResources = &mResourcesByFile[fileKey];

    XmlNode resNode = modelNode.child(XmlTag::resources);
    for (auto &currentNode : resNode.children()) {
        const std::string currentNodeName = currentNode.name();
        if (currentNodeName == XmlTag::texture_2d) {
            ReadEmbeddecTexture(currentNode);
        } else if (currentNodeName == XmlTag::texture_group) {
            ReadTextureGroup(currentNode);
        } else if (currentNodeName == XmlTag::object) {
            ReadObject(currentNode);
        } else if (currentNodeName == XmlTag::basematerials) {
            ReadBaseMaterials(currentNode);
        } else if (currentNodeName == XmlTag::meta) {
            ReadMetadata(currentNode);
        } else if (currentNodeName == XmlTag::colorgroup) {
            ReadColorGroup(currentNode);
        }
    }
}

void XmlSerializer::LoadModelFile(const std::string &path) {
    if (path.empty() || nullptr == mPackage) {
        return;
    }
    // Already loaded (e.g. the root file, or referenced from multiple places).
    if (mResourcesByFile.find(path) != mResourcesByFile.end()) {
        return;
    }

    IOStream *stream = mPackage->OpenPart(path);
    if (nullptr == stream) {
        ASSIMP_LOG_WARN("3MF: cannot open referenced model part: ", path);
        return;
    }

    XmlParser parser;
    if (parser.parse(stream)) {
        XmlNode modelNode = parser.getRootNode().child(XmlTag::model);
        if (!modelNode.empty()) {
            // ReadObject copies all data into Object/aiMesh, so the parser may be
            // discarded once the resources of this part have been read.
            ReadModel(modelNode, path);
        }
    }
    mPackage->CloseStream(stream);
}

void XmlSerializer::ImportXml(aiScene *scene) {
    if (nullptr == scene) {
        return;
    }

    scene->mRootNode = new aiNode(XmlTag::RootTag);
    XmlNode node = mXmlParser.getRootNode().child(XmlTag::model);
    if (node.empty()) {
        return;
    }

    const std::string rootKey = (nullptr != mPackage) ? mPackage->RootPath() : std::string();
    ReadModel(node, rootKey);

    XmlNode buildNode = node.child(XmlTag::build);

    // Production extension: build items and root-object components may reference objects
    // in separate model parts via a path attribute. Load every referenced part now, before
    // materials are copied into the scene, so material indices defined there stay valid.
    std::set<std::string> referencedFiles;
    if (!buildNode.empty()) {
        for (auto &currentNode : buildNode.children()) {
            if (std::string(currentNode.name()) == XmlTag::item) {
                std::string path;
                if (getNodeAttribute(currentNode, D3MF::XmlTag::p_path, path)) {
                    referencedFiles.insert(D3MFOpcPackage::NormalizePath(path));
                }
            }
        }
    }
    auto rootIt = mResourcesByFile.find(rootKey);
    if (rootIt != mResourcesByFile.end()) {
        for (auto &it : rootIt->second) {
            if (it.second->getType() == ResourceType::RT_Object) {
                Object *obj = static_cast<Object *>(it.second);
                for (const Component &c : obj->mComponents) {
                    if (!c.mPath.empty()) {
                        referencedFiles.insert(c.mPath);
                    }
                }
            }
        }
    }
    for (const std::string &file : referencedFiles) {
        LoadModelFile(file);
    }

    StoreMaterialsInScene(scene);

    if (!buildNode.empty()) {
        for (auto &currentNode : buildNode.children()) {
            const std::string currentNodeName = currentNode.name();
            if (currentNodeName == XmlTag::item) {
                int objectId = IdNotSet;
                std::string transformationMatrixStr;
                std::string itemPath;
                aiMatrix4x4 transformationMatrix;
                getNodeAttribute(currentNode, D3MF::XmlTag::objectid, objectId);
                bool hasTransform = getNodeAttribute(currentNode, D3MF::XmlTag::transform, transformationMatrixStr);
                bool hasPath = getNodeAttribute(currentNode, D3MF::XmlTag::p_path, itemPath);

                const std::string fileKey = hasPath ? D3MFOpcPackage::NormalizePath(itemPath) : rootKey;
                auto fileIt = mResourcesByFile.find(fileKey);
                if (fileIt == mResourcesByFile.end()) {
                    continue;
                }
                auto it = fileIt->second.find(objectId);
                if (it != fileIt->second.end() && it->second->getType() == ResourceType::RT_Object) {
                    Object *obj = static_cast<Object *>(it->second);
                    if (hasTransform) {
                        transformationMatrix = parseTransformMatrix(transformationMatrixStr);
                    }

                    addObjectToNode(scene->mRootNode, obj, transformationMatrix, fileKey);
                }
            }
        }
    }

    // import the metadata
    if (!mMetaData.empty()) {
        const size_t numMeta = mMetaData.size();
        scene->mMetaData = aiMetadata::Alloc(static_cast<unsigned int>(numMeta));
        for (size_t i = 0; i < numMeta; ++i) {
            aiString val(mMetaData[i].value);
            scene->mMetaData->Set(static_cast<unsigned int>(i), mMetaData[i].name, val);
        }
    }

    // import the meshes from every model part, materials are already stored
    scene->mNumMeshes = static_cast<unsigned int>(mMeshCount);
    if (scene->mNumMeshes != 0) {
        scene->mMeshes = new aiMesh *[scene->mNumMeshes]();
        for (auto &file : mResourcesByFile) {
            for (auto &it : file.second) {
                if (it.second->getType() == ResourceType::RT_Object) {
                    Object *obj = static_cast<Object *>(it.second);
                    ai_assert(nullptr != obj);
                    for (unsigned int i = 0; i < obj->mMeshes.size(); ++i) {
                        scene->mMeshes[obj->mMeshIndex[i]] = obj->mMeshes[i];
                    }
                }
            }
        }
    }
}

void XmlSerializer::addObjectToNode(aiNode *parent, Object *obj, const aiMatrix4x4 &nodeTransform, const std::string &fileKey) {
    ai_assert(nullptr != obj);

    aiNode *sceneNode = new aiNode(obj->mName);
    sceneNode->mNumMeshes = static_cast<unsigned int>(obj->mMeshes.size());
    sceneNode->mMeshes = new unsigned int[sceneNode->mNumMeshes];
    std::copy(obj->mMeshIndex.begin(), obj->mMeshIndex.end(), sceneNode->mMeshes);

    sceneNode->mTransformation = nodeTransform;
    if (nullptr != parent) {
        parent->addChildren(1, &sceneNode);
    }

    for (const Assimp::D3MF::Component &c : obj->mComponents) {
        // A component without a path references an object in the same model part;
        // with a path (production extension, already normalized) it references another part.
        const std::string &childKey = c.mPath.empty() ? fileKey : c.mPath;
        auto fileIt = mResourcesByFile.find(childKey);
        if (fileIt == mResourcesByFile.end()) {
            continue;
        }
        auto it = fileIt->second.find(c.mObjectId);
        if (it != fileIt->second.end() && it->second->getType() == ResourceType::RT_Object) {
            addObjectToNode(sceneNode, static_cast<Object *>(it->second), c.mTransformation, childKey);
        }
    }
}

void XmlSerializer::ReadObject(XmlNode &node) {
    int id = IdNotSet, pid = IdNotSet, pindex = IdNotSet;
    bool hasId = getNodeAttribute(node, XmlTag::id, id);
    if (!hasId) {
        return;
    }

    bool hasPid = getNodeAttribute(node, XmlTag::pid, pid);
    bool hasPindex = getNodeAttribute(node, XmlTag::pindex, pindex);

    Object *obj = new Object(id);
    for (XmlNode &currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == D3MF::XmlTag::mesh) {
            auto mesh = ReadMesh(currentNode);
            mesh->mName.Set(ai_to_string(id));

            if (hasPid) {
                auto it = mCurrentResources->find(pid);
                if (hasPindex && it != mCurrentResources->end()) {
                    if (it->second->getType() == ResourceType::RT_BaseMaterials) {
                        BaseMaterials *materials = static_cast<BaseMaterials *>(it->second);
                        if (pindex >= 0 && static_cast<size_t>(pindex) < materials->mMaterialIndex.size()) {
                            mesh->mMaterialIndex = materials->mMaterialIndex[pindex];
                        }
                    } else if (it->second->getType() == ResourceType::RT_Texture2DGroup) {
                        Texture2DGroup *group = static_cast<Texture2DGroup *>(it->second);
                        const bool pindexValid = pindex >= 0 && static_cast<size_t>(pindex) < group->mTex2dCoords.size();
                        if (mesh->mTextureCoords[0] == nullptr) {
                            mesh->mNumUVComponents[0] = 2;
                            for (unsigned int i = 1; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                                mesh->mNumUVComponents[i] = 0;
                            }

                            const std::string name = ai_to_string(group->mTexId);
                            for (size_t i = 0; i < mMaterials.size(); ++i) {
                                if (name == mMaterials[i]->GetName().C_Str()) {
                                    mesh->mMaterialIndex = static_cast<unsigned int>(i);
                                }
                            }

                            mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
                            if (pindexValid) {
                                for (unsigned int vertex_idx = 0; vertex_idx < mesh->mNumVertices; vertex_idx++) {
                                    mesh->mTextureCoords[0][vertex_idx] =
                                            aiVector3D(group->mTex2dCoords[pindex].x, group->mTex2dCoords[pindex].y, 0.0f);
                                }
                            }
                        } else if (pindexValid) {
                            for (unsigned int vertex_idx = 0; vertex_idx < mesh->mNumVertices; vertex_idx++) {
                                if (mesh->mTextureCoords[0][vertex_idx].z < 0) {
                                    // use default
                                    mesh->mTextureCoords[0][vertex_idx] =
                                            aiVector3D(group->mTex2dCoords[pindex].x, group->mTex2dCoords[pindex].y, 0.0f);
                                }
                            }
                        }
                    }else if (it->second->getType() == ResourceType::RT_ColorGroup) {
                        if (mesh->mColors[0] == nullptr) {
                            mesh->mColors[0] = new aiColor4D[mesh->mNumVertices];

                            ColorGroup *group = static_cast<ColorGroup *>(it->second);
                            if (pindex >= 0 && static_cast<size_t>(pindex) < group->mColors.size()) {
                                for (unsigned int vertex_idx = 0; vertex_idx < mesh->mNumVertices; vertex_idx++) {
                                    mesh->mColors[0][vertex_idx] = group->mColors[pindex];
                                }
                            }
                        }
                    }
                }
            }

            obj->mMeshes.push_back(mesh);
            obj->mMeshIndex.push_back(mMeshCount);
            mMeshCount++;
        } else if (currentName == D3MF::XmlTag::components) {
            for (XmlNode &currentSubNode : currentNode.children()) {
                const std::string subNodeName = currentSubNode.name();
                if (subNodeName == D3MF::XmlTag::component) {
                    int objectId = IdNotSet;
                    std::string componentTransformStr;
                    aiMatrix4x4 componentTransform;
                    if (getNodeAttribute(currentSubNode, D3MF::XmlTag::transform, componentTransformStr)) {
                        componentTransform = parseTransformMatrix(componentTransformStr);
                    }

                    // Production extension: a component may reference an object in another model
                    // part. Normalize the path once here so every later lookup can compare directly.
                    std::string componentPath;
                    if (getNodeAttribute(currentSubNode, D3MF::XmlTag::p_path, componentPath)) {
                        componentPath = D3MFOpcPackage::NormalizePath(componentPath);
                    }

                    if (getNodeAttribute(currentSubNode, D3MF::XmlTag::objectid, objectId)) {
                        obj->mComponents.push_back({ objectId, componentTransform, componentPath });
                    }
                }
            }
        }
    }

    mCurrentResources->insert(std::make_pair(id, obj));
}

aiMesh *XmlSerializer::ReadMesh(XmlNode &node) {
    if (node.empty()) {
        return nullptr;
    }

    aiMesh *mesh = new aiMesh();
    for (XmlNode &currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == XmlTag::vertices) {
            ImportVertices(currentNode, mesh);
        } else if (currentName == XmlTag::triangles) {
            ImportTriangles(currentNode, mesh);
        }
    }

    return mesh;
}

void XmlSerializer::ReadMetadata(XmlNode &node) {
    pugi::xml_attribute attribute = node.attribute(D3MF::XmlTag::meta_name);
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

void XmlSerializer::ImportVertices(XmlNode &node, aiMesh *mesh) {
    ai_assert(nullptr != mesh);

    std::vector<aiVector3D> vertices;
    for (XmlNode &currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == XmlTag::vertex) {
            vertices.push_back(ReadVertex(currentNode));
        }
    }

    mesh->mNumVertices = static_cast<unsigned int>(vertices.size());
    mesh->mVertices = new aiVector3D[mesh->mNumVertices];
    std::copy(vertices.begin(), vertices.end(), mesh->mVertices);
}

void XmlSerializer::ImportTriangles(XmlNode &node, aiMesh *mesh) {
    std::vector<aiFace> faces;
    for (XmlNode &currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == XmlTag::triangle) {
            int pid = IdNotSet;
            bool hasPid = getNodeAttribute(currentNode, D3MF::XmlTag::pid, pid);

            int pindex[3];
            aiFace face = ReadTriangle(currentNode, pindex[0], pindex[1], pindex[2]);
            if (hasPid && (pindex[0] != IdNotSet || pindex[1] != IdNotSet || pindex[2] != IdNotSet)) {
                auto it = mCurrentResources->find(pid);
                if (it != mCurrentResources->end()) {
                    if (it->second->getType() == ResourceType::RT_BaseMaterials) {
                        BaseMaterials *baseMaterials = static_cast<BaseMaterials *>(it->second);

                        auto update_material = [&](int idx) {
                            if (pindex[idx] != IdNotSet && static_cast<size_t>(pindex[idx]) < baseMaterials->mMaterialIndex.size()) {
                                mesh->mMaterialIndex = baseMaterials->mMaterialIndex[pindex[idx]];
                            }
                        };

                        update_material(0);
                        update_material(1);
                        update_material(2);

                    } else if (it->second->getType() == ResourceType::RT_Texture2DGroup) {
                        // Load texture coordinates into mesh, when any
                        Texture2DGroup *group = static_cast<Texture2DGroup *>(it->second); // fix bug
                        if (mesh->mTextureCoords[0] == nullptr) {
                            mesh->mNumUVComponents[0] = 2;
                            for (unsigned int i = 1; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                                mesh->mNumUVComponents[i] = 0;
                            }

                            const std::string name = ai_to_string(group->mTexId);
                            for (size_t i = 0; i < mMaterials.size(); ++i) {
                                if (name == mMaterials[i]->GetName().C_Str()) {
                                    mesh->mMaterialIndex = static_cast<unsigned int>(i);
                                }
                            }
                            mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
                            for (unsigned int vertex_index = 0; vertex_index < mesh->mNumVertices; vertex_index++) {
                                mesh->mTextureCoords[0][vertex_index].z = IdNotSet;//mark not set
                            }
                        }

                        auto update_texture = [&](int idx) {
                            if (pindex[idx] != IdNotSet && static_cast<size_t>(pindex[idx]) < group->mTex2dCoords.size()) {
                                size_t vertex_index = face.mIndices[idx];
                                if (vertex_index < mesh->mNumVertices) {
                                    mesh->mTextureCoords[0][vertex_index] =
                                            aiVector3D(group->mTex2dCoords[pindex[idx]].x, group->mTex2dCoords[pindex[idx]].y, 0.0f);
                                }
                            }
                        };

                        update_texture(0);
                        update_texture(1);
                        update_texture(2);

                    } else if (it->second->getType() == ResourceType::RT_ColorGroup) {
                        // Load vertex color into mesh, when any
                        ColorGroup *group = static_cast<ColorGroup *>(it->second);
                        if (mesh->mColors[0] == nullptr) {
                            mesh->mColors[0] = new aiColor4D[mesh->mNumVertices];
                        }

                        auto update_color = [&](int idx) {
                            if (pindex[idx] != IdNotSet && static_cast<size_t>(pindex[idx]) < group->mColors.size()) {
                                size_t vertex_index = face.mIndices[idx];
                                if (vertex_index < mesh->mNumVertices) {
                                    mesh->mColors[0][vertex_index] = group->mColors[pindex[idx]];
                                }
                            }
                        };

                        update_color(0);
                        update_color(1);
                        update_color(2);
                    }
                }
            }

            faces.push_back(face);
        }
    }

    mesh->mNumFaces = static_cast<unsigned int>(faces.size());
    mesh->mFaces = new aiFace[mesh->mNumFaces];
    mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

    std::copy(faces.begin(), faces.end(), mesh->mFaces);
}

void XmlSerializer::ReadBaseMaterials(XmlNode &node) {
    int id = IdNotSet;
    if (getNodeAttribute(node, D3MF::XmlTag::id, id)) {
        BaseMaterials *baseMaterials = new BaseMaterials(id);

        for (XmlNode &currentNode : node.children()) {
            const std::string currentName = currentNode.name();
            if (currentName == XmlTag::basematerials_base) {
                baseMaterials->mMaterialIndex.push_back(static_cast<unsigned int>(mMaterials.size()));
                mMaterials.push_back(readMaterialDef(currentNode, id));
            }
        }

        mCurrentResources->insert(std::make_pair(id, baseMaterials));
    }
}

void XmlSerializer::ReadEmbeddecTexture(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    std::string value;
    EmbeddedTexture *tex2D = nullptr;
    if (XmlParser::getStdStrAttribute(node, XmlTag::id, value)) {
        tex2D = new EmbeddedTexture(atoi(value.c_str()));
    }
    if (nullptr == tex2D) {
        return;
    }

    if (XmlParser::getStdStrAttribute(node, XmlTag::path, value)) {
        tex2D->mPath = value;
    }
    if (XmlParser::getStdStrAttribute(node, XmlTag::texture_content_type, value)) {
        tex2D->mContentType = value;
    }
    if (XmlParser::getStdStrAttribute(node, XmlTag::texture_tilestyleu, value)) {
        tex2D->mTilestyleU = value;
    }
    if (XmlParser::getStdStrAttribute(node, XmlTag::texture_tilestylev, value)) {
        tex2D->mTilestyleV = value;
    }
    mEmbeddedTextures.emplace_back(tex2D);
    StoreEmbeddedTexture(tex2D);
}

void XmlSerializer::StoreEmbeddedTexture(EmbeddedTexture *tex) {
    aiMaterial *mat = new aiMaterial;
    aiString s;
    s.Set(ai_to_string(tex->mId).c_str());
    mat->AddProperty(&s, AI_MATKEY_NAME);
    const std::string name = "*" + tex->mPath;
    s.Set(name);
    mat->AddProperty(&s, AI_MATKEY_TEXTURE_DIFFUSE(0));

    aiColor3D col;
    mat->AddProperty<aiColor3D>(&col, 1, AI_MATKEY_COLOR_DIFFUSE);
    mat->AddProperty<aiColor3D>(&col, 1, AI_MATKEY_COLOR_AMBIENT);
    mat->AddProperty<aiColor3D>(&col, 1, AI_MATKEY_COLOR_EMISSIVE);
    mat->AddProperty<aiColor3D>(&col, 1, AI_MATKEY_COLOR_SPECULAR);
    mMaterials.emplace_back(mat);
}

void XmlSerializer::ReadTextureCoords2D(XmlNode &node, Texture2DGroup *tex2DGroup) {
    if (node.empty() || nullptr == tex2DGroup) {
        return;
    }

    int id = IdNotSet;
    if (XmlParser::getIntAttribute(node, "texid", id)) {
        tex2DGroup->mTexId = id;
    }

    double value = 0.0;
    for (XmlNode currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        aiVector2D texCoord;
        if (currentName == XmlTag::texture_2d_coord) {
            XmlParser::getDoubleAttribute(currentNode, XmlTag::texture_cuurd_u, value);
            texCoord.x = (ai_real)value;
            XmlParser::getDoubleAttribute(currentNode, XmlTag::texture_cuurd_v, value);
            texCoord.y = (ai_real)value;
            tex2DGroup->mTex2dCoords.push_back(texCoord);
        }
    }
}

void XmlSerializer::ReadTextureGroup(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    int id = IdNotSet;
    if (!XmlParser::getIntAttribute(node, XmlTag::id, id)) {
        return;
    }

    Texture2DGroup *group = new Texture2DGroup(id);
    ReadTextureCoords2D(node, group);
    mCurrentResources->insert(std::make_pair(id, group));
}

aiMaterial *XmlSerializer::readMaterialDef(XmlNode &node, unsigned int basematerialsId) {
    aiMaterial *material = new aiMaterial();
    material->mNumProperties = 0;
    std::string name;
    bool hasName = getNodeAttribute(node, D3MF::XmlTag::basematerials_name, name);

    std::string stdMaterialName;
    const std::string strId(ai_to_string(basematerialsId));
    stdMaterialName += "id";
    stdMaterialName += strId;
    stdMaterialName += "_";
    if (hasName) {
        stdMaterialName += name;
    } else {
        stdMaterialName += "basemat_";
        stdMaterialName += ai_to_string(mMaterials.size());
    }

    aiString assimpMaterialName(stdMaterialName);
    material->AddProperty(&assimpMaterialName, AI_MATKEY_NAME);

    assignDiffuseColor(node, material);

    return material;
}

void XmlSerializer::ReadColor(XmlNode &node, ColorGroup *colorGroup) {
    if (node.empty() || nullptr == colorGroup) {
        return;
    }

    for (XmlNode currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == XmlTag::color_item) {
            const char *color = currentNode.attribute(XmlTag::color_value).as_string();
            aiColor4D color_value;
            if (parseColor(color, color_value)) {
                colorGroup->mColors.push_back(color_value);
            }
        }
    }
}

void XmlSerializer::ReadColorGroup(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    int id = IdNotSet;
    if (!XmlParser::getIntAttribute(node, XmlTag::id, id)) {
        return;
    }

    ColorGroup *group = new ColorGroup(id);
    ReadColor(node, group);
    mCurrentResources->insert(std::make_pair(id, group));
}

void XmlSerializer::StoreMaterialsInScene(aiScene *scene) {
    if (nullptr == scene) {
        return;
    }

    scene->mNumMaterials = static_cast<unsigned int>(mMaterials.size());
    if (scene->mNumMaterials == 0) {
        return;
    }

    scene->mMaterials = new aiMaterial *[scene->mNumMaterials];
    for (size_t i = 0; i < mMaterials.size(); ++i) {
        scene->mMaterials[i] = mMaterials[i];
    }
}

} // namespace D3MF
} // namespace Assimp
