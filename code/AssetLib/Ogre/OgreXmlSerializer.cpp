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

#include "OgreXmlSerializer.h"
#include "OgreBinarySerializer.h"
#include "OgreParsingUtils.h"

#include <assimp/TinyFormatter.h>
#include <assimp/DefaultLogger.hpp>

#include <memory>

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

// Define as 1 to get verbose logging.
#define OGRE_XML_SERIALIZER_DEBUG 0

namespace Assimp {
namespace Ogre {

//AI_WONT_RETURN void ThrowAttibuteError(const XmlParser *reader, const std::string &name, const std::string &error = "") AI_WONT_RETURN_SUFFIX;

AI_WONT_RETURN void ThrowAttibuteError(const std::string &nodeName, const std::string &name, const std::string &error) {
    if (!error.empty()) {
        throw DeadlyImportError(error, " in node '", nodeName, "' and attribute '", name, "'");
    } else {
        throw DeadlyImportError("Attribute '", name, "' does not exist in node '", nodeName, "'");
    }
}

template <>
int32_t OgreXmlSerializer::ReadAttribute<int32_t>(XmlNode &xmlNode, const char *name) const {
    if (!XmlParser::hasAttribute(xmlNode, name)) {
        ThrowAttibuteError(xmlNode.name(), name, "Not found");
    }
    pugi::xml_attribute attr = xmlNode.attribute(name);
    return static_cast<int32_t>(attr.as_int());
}

template <>
uint32_t OgreXmlSerializer::ReadAttribute<uint32_t>(XmlNode &xmlNode, const char *name) const {
    if (!XmlParser::hasAttribute(xmlNode, name)) {
        ThrowAttibuteError(xmlNode.name(), name, "Not found");
    }

    // @note This is hackish. But we are never expecting unsigned values that go outside the
    //       int32_t range. Just monitor for negative numbers and kill the import.
    int32_t temp = ReadAttribute<int32_t>(xmlNode, name);
    if (temp < 0) {
        ThrowAttibuteError(xmlNode.name(), name, "Found a negative number value where expecting a uint32_t value");
    }

    return static_cast<uint32_t>(temp);
}

template <>
uint16_t OgreXmlSerializer::ReadAttribute<uint16_t>(XmlNode &xmlNode, const char *name) const {
    if (!XmlParser::hasAttribute(xmlNode, name)) {
        ThrowAttibuteError(xmlNode.name(), name, "Not found");
    }

    return static_cast<uint16_t>(xmlNode.attribute(name).as_int());
}

template <>
float OgreXmlSerializer::ReadAttribute<float>(XmlNode &xmlNode, const char *name) const {
    if (!XmlParser::hasAttribute(xmlNode, name)) {
        ThrowAttibuteError(xmlNode.name(), name, "Not found");
    }

    return xmlNode.attribute(name).as_float();
}

template <>
std::string OgreXmlSerializer::ReadAttribute<std::string>(XmlNode &xmlNode, const char *name) const {
    if (!XmlParser::hasAttribute(xmlNode, name)) {
        ThrowAttibuteError(xmlNode.name(), name, "Not found");
    }

    return xmlNode.attribute(name).as_string();
}

template <>
bool OgreXmlSerializer::ReadAttribute<bool>(XmlNode &xmlNode, const char *name) const {
    std::string value = ai_tolower(ReadAttribute<std::string>(xmlNode, name));
    if (ASSIMP_stricmp(value, "true") == 0) {
        return true;
    } else if (ASSIMP_stricmp(value, "false") == 0) {
        return false;
    }

    ThrowAttibuteError(xmlNode.name(), name, "Boolean value is expected to be 'true' or 'false', encountered '" + value + "'");
    return false;
}

// Mesh XML constants

// <mesh>
static const char *nnMesh = "mesh";
static const char *nnSharedGeometry = "sharedgeometry";
static const char *nnSubMeshes = "submeshes";
static const char *nnSubMesh = "submesh";
//static const char *nnSubMeshNames = "submeshnames";
static const char *nnSkeletonLink = "skeletonlink";
//static const char *nnLOD = "levelofdetail";
//static const char *nnExtremes = "extremes";
//static const char *nnPoses = "poses";
static const char *nnAnimations = "animations";

// <submesh>
static const char *nnFaces = "faces";
static const char *nnFace = "face";
static const char *nnGeometry = "geometry";
//static const char *nnTextures = "textures";

// <mesh/submesh>
static const char *nnBoneAssignments = "boneassignments";

// <sharedgeometry/geometry>
static const char *nnVertexBuffer = "vertexbuffer";

// <vertexbuffer>
//static const char *nnVertex = "vertex";
static const char *nnPosition = "position";
static const char *nnNormal = "normal";
static const char *nnTangent = "tangent";
//static const char *nnBinormal = "binormal";
static const char *nnTexCoord = "texcoord";
//static const char *nnColorDiffuse = "colour_diffuse";
//static const char *nnColorSpecular = "colour_specular";

// <boneassignments>
static const char *nnVertexBoneAssignment = "vertexboneassignment";

// Skeleton XML constants

// <skeleton>
static const char *nnSkeleton = "skeleton";
static const char *nnBones = "bones";
static const char *nnBoneHierarchy = "bonehierarchy";
//static const char *nnAnimationLinks = "animationlinks";

// <bones>
static const char *nnBone = "bone";
static const char *nnRotation = "rotation";
static const char *nnAxis = "axis";
static const char *nnScale = "scale";

// <bonehierarchy>
static const char *nnBoneParent = "boneparent";

// <animations>
static const char *nnAnimation = "animation";
static const char *nnTracks = "tracks";

// <tracks>
static const char *nnTrack = "track";
static const char *nnKeyFrames = "keyframes";
static const char *nnKeyFrame = "keyframe";
static const char *nnTranslate = "translate";
static const char *nnRotate = "rotate";

// Common XML constants
static const char *anX = "x";
static const char *anY = "y";
static const char *anZ = "z";

// Mesh

OgreXmlSerializer::OgreXmlSerializer(XmlParser *parser) :
        mParser(parser) {
    // empty
}

MeshXml *OgreXmlSerializer::ImportMesh(XmlParser *parser) {
    if (nullptr == parser) {
        return nullptr;
    }

    OgreXmlSerializer serializer(parser);

    MeshXml *mesh = new MeshXml();
    serializer.ReadMesh(mesh);

    return mesh;
}

void OgreXmlSerializer::ReadMesh(MeshXml *mesh) {
    XmlNode root = mParser->getRootNode();
    if (nullptr == root) {
        throw DeadlyImportError("Root node is <" + std::string(root.name()) + "> expecting <mesh>");
    }

    XmlNode startNode = root.child(nnMesh);
    if (startNode.empty()) {
        throw DeadlyImportError("Root node is <" + std::string(root.name()) + "> expecting <mesh>");
    }
    for (XmlNode currentNode : startNode.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == nnSharedGeometry) {
            mesh->sharedVertexData = new VertexDataXml();
            ReadGeometry(currentNode, mesh->sharedVertexData);
        } else if (currentName == nnSubMeshes) {
            for (XmlNode &subMeshesNode : currentNode.children()) {
                const std::string &currentSMName = subMeshesNode.name();
                if (currentSMName == nnSubMesh) {
                    ReadSubMesh(subMeshesNode, mesh);
                }
            }
        } else if (currentName == nnBoneAssignments) {
            ReadBoneAssignments(currentNode, mesh->sharedVertexData);
        } else if (currentName == nnSkeletonLink) {
        }
    }

    ASSIMP_LOG_VERBOSE_DEBUG("Reading Mesh");
}

void OgreXmlSerializer::ReadGeometry(XmlNode &node, VertexDataXml *dest) {
    dest->count = ReadAttribute<uint32_t>(node, "vertexcount");
    ASSIMP_LOG_VERBOSE_DEBUG_F("  - Reading geometry of ", dest->count, " vertices");

    for (XmlNode currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == nnVertexBuffer) {
            ReadGeometryVertexBuffer(currentNode, dest);
        }
    }
}

void OgreXmlSerializer::ReadGeometryVertexBuffer(XmlNode &node, VertexDataXml *dest) {
    bool positions = (XmlParser::hasAttribute(node, "positions") && ReadAttribute<bool>(node, "positions"));
    bool normals = (XmlParser::hasAttribute(node, "normals") && ReadAttribute<bool>(node, "normals"));
    bool tangents = (XmlParser::hasAttribute(node, "tangents") && ReadAttribute<bool>(node, "tangents"));
    uint32_t uvs = (XmlParser::hasAttribute(node, "texture_coords") ? ReadAttribute<uint32_t>(node, "texture_coords") : 0);

    // Not having positions is a error only if a previous vertex buffer did not have them.
    if (!positions && !dest->HasPositions()) {
        throw DeadlyImportError("Vertex buffer does not contain positions!");
    }

    if (positions) {
        ASSIMP_LOG_VERBOSE_DEBUG("    - Contains positions");
        dest->positions.reserve(dest->count);
    }
    if (normals) {
        ASSIMP_LOG_VERBOSE_DEBUG("    - Contains normals");
        dest->normals.reserve(dest->count);
    }
    if (tangents) {
        ASSIMP_LOG_VERBOSE_DEBUG("    - Contains tangents");
        dest->tangents.reserve(dest->count);
    }
    if (uvs > 0) {
        ASSIMP_LOG_VERBOSE_DEBUG_F("    - Contains ", uvs, " texture coords");
        dest->uvs.resize(uvs);
        for (size_t i = 0, len = dest->uvs.size(); i < len; ++i) {
            dest->uvs[i].reserve(dest->count);
        }
    }

    for (XmlNode currentNode : node.children("vertex")) {
        for (XmlNode vertexNode : currentNode.children()) {
            const std::string &currentName = vertexNode.name();
            if (positions && currentName == nnPosition) {
                aiVector3D pos;
                pos.x = ReadAttribute<float>(vertexNode, anX);
                pos.y = ReadAttribute<float>(vertexNode, anY);
                pos.z = ReadAttribute<float>(vertexNode, anZ);
                dest->positions.push_back(pos);
            } else if (normals && currentName == nnNormal) {
                aiVector3D normal;
                normal.x = ReadAttribute<float>(vertexNode, anX);
                normal.y = ReadAttribute<float>(vertexNode, anY);
                normal.z = ReadAttribute<float>(vertexNode, anZ);
                dest->normals.push_back(normal);
            } else if (tangents && currentName == nnTangent) {
                aiVector3D tangent;
                tangent.x = ReadAttribute<float>(vertexNode, anX);
                tangent.y = ReadAttribute<float>(vertexNode, anY);
                tangent.z = ReadAttribute<float>(vertexNode, anZ);
                dest->tangents.push_back(tangent);
            } else if (uvs > 0 && currentName == nnTexCoord) {
                for (auto &curUvs : dest->uvs) {
                    aiVector3D uv;
                    uv.x = ReadAttribute<float>(vertexNode, "u");
                    uv.y = (ReadAttribute<float>(vertexNode, "v") * -1) + 1; // Flip UV from Ogre to Assimp form
                    curUvs.push_back(uv);
                }
            }
        }
    }

    // Sanity checks
    if (dest->positions.size() != dest->count) {
        throw DeadlyImportError("Read only ", dest->positions.size(), " positions when should have read ", dest->count);
    }
    if (normals && dest->normals.size() != dest->count) {
        throw DeadlyImportError("Read only ", dest->normals.size(), " normals when should have read ", dest->count);
    }
    if (tangents && dest->tangents.size() != dest->count) {
        throw DeadlyImportError("Read only ", dest->tangents.size(), " tangents when should have read ", dest->count);
    }
    for (unsigned int i = 0; i < dest->uvs.size(); ++i) {
        if (dest->uvs[i].size() != dest->count) {
            throw DeadlyImportError("Read only ", dest->uvs[i].size(),
                    " uvs for uv index ", i, " when should have read ", dest->count);
        }
    }
}

void OgreXmlSerializer::ReadSubMesh(XmlNode &node, MeshXml *mesh) {
    static const char *anMaterial = "material";
    static const char *anUseSharedVertices = "usesharedvertices";
    static const char *anCount = "count";
    static const char *anV1 = "v1";
    static const char *anV2 = "v2";
    static const char *anV3 = "v3";
    static const char *anV4 = "v4";

    SubMeshXml *submesh = new SubMeshXml();

    if (XmlParser::hasAttribute(node, anMaterial)) {
        submesh->materialRef = ReadAttribute<std::string>(node, anMaterial);
    }
    if (XmlParser::hasAttribute(node, anUseSharedVertices)) {
        submesh->usesSharedVertexData = ReadAttribute<bool>(node, anUseSharedVertices);
    }

    ASSIMP_LOG_VERBOSE_DEBUG_F("Reading SubMesh ", mesh->subMeshes.size());
    ASSIMP_LOG_VERBOSE_DEBUG_F("  - Material: '", submesh->materialRef, "'");
    ASSIMP_LOG_VERBOSE_DEBUG_F("  - Uses shared geometry: ", (submesh->usesSharedVertexData ? "true" : "false"));

    // TODO: maybe we have always just 1 faces and 1 geometry and always in this order. this loop will only work correct, when the order
    // of faces and geometry changed, and not if we have more than one of one
    /// @todo Fix above comment with better read logic below

    bool quadWarned = false;

    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == nnFaces) {
            submesh->indexData->faceCount = ReadAttribute<uint32_t>(currentNode, anCount);
            submesh->indexData->faces.reserve(submesh->indexData->faceCount);
            for (XmlNode currentChildNode : currentNode.children()) {
                const std::string &currentChildName = currentChildNode.name();
                if (currentChildName == nnFace) {
                    aiFace face;
                    face.mNumIndices = 3;
                    face.mIndices = new unsigned int[3];
                    face.mIndices[0] = ReadAttribute<uint32_t>(currentChildNode, anV1);
                    face.mIndices[1] = ReadAttribute<uint32_t>(currentChildNode, anV2);
                    face.mIndices[2] = ReadAttribute<uint32_t>(currentChildNode, anV3);
                    /// @todo Support quads if Ogre even supports them in XML (I'm not sure but I doubt it)
                    if (!quadWarned && XmlParser::hasAttribute(currentChildNode, anV4)) {
                        ASSIMP_LOG_WARN("Submesh <face> has quads with <v4>, only triangles are supported at the moment!");
                        quadWarned = true;
                    }
                    submesh->indexData->faces.push_back(face);
                }
            }
            if (submesh->indexData->faces.size() == submesh->indexData->faceCount) {
                ASSIMP_LOG_VERBOSE_DEBUG_F("  - Faces ", submesh->indexData->faceCount);
            } else {
                throw DeadlyImportError("Read only ", submesh->indexData->faces.size(), " faces when should have read ", submesh->indexData->faceCount);
            }
        } else if (currentName == nnGeometry) {
            if (submesh->usesSharedVertexData) {
                throw DeadlyImportError("Found <geometry> in <submesh> when use shared geometry is true. Invalid mesh file.");
            }

            submesh->vertexData = new VertexDataXml();
            ReadGeometry(currentNode, submesh->vertexData);
        } else if (currentName == nnBoneAssignments) {
            ReadBoneAssignments(currentNode, submesh->vertexData);
        }
    }

    submesh->index = static_cast<unsigned int>(mesh->subMeshes.size());
    mesh->subMeshes.push_back(submesh);
}

void OgreXmlSerializer::ReadBoneAssignments(XmlNode &node, VertexDataXml *dest) {
    if (!dest) {
        throw DeadlyImportError("Cannot read bone assignments, vertex data is null.");
    }

    static const char *anVertexIndex = "vertexindex";
    static const char *anBoneIndex = "boneindex";
    static const char *anWeight = "weight";

    std::set<uint32_t> influencedVertices;
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == nnVertexBoneAssignment) {
            VertexBoneAssignment ba;
            ba.vertexIndex = ReadAttribute<uint32_t>(currentNode, anVertexIndex);
            ba.boneIndex = ReadAttribute<uint16_t>(currentNode, anBoneIndex);
            ba.weight = ReadAttribute<float>(currentNode, anWeight);

            dest->boneAssignments.push_back(ba);
            influencedVertices.insert(ba.vertexIndex);
        }
    }

    /** Normalize bone weights.
        Some exporters won't care if the sum of all bone weights
        for a single vertex equals 1 or not, so validate here. */
    const float epsilon = 0.05f;
    for (const uint32_t vertexIndex : influencedVertices) {
        float sum = 0.0f;
        for (VertexBoneAssignmentList::const_iterator baIter = dest->boneAssignments.begin(), baEnd = dest->boneAssignments.end(); baIter != baEnd; ++baIter) {
            if (baIter->vertexIndex == vertexIndex)
                sum += baIter->weight;
        }
        if ((sum < (1.0f - epsilon)) || (sum > (1.0f + epsilon))) {
            for (auto &boneAssign : dest->boneAssignments) {
                if (boneAssign.vertexIndex == vertexIndex)
                    boneAssign.weight /= sum;
            }
        }
    }

    ASSIMP_LOG_VERBOSE_DEBUG_F("  - ", dest->boneAssignments.size(), " bone assignments");
}

// Skeleton

bool OgreXmlSerializer::ImportSkeleton(Assimp::IOSystem *pIOHandler, MeshXml *mesh) {
    if (!mesh || mesh->skeletonRef.empty())
        return false;

    // Highly unusual to see in read world cases but support
    // XML mesh referencing a binary skeleton file.
    if (EndsWith(mesh->skeletonRef, ".skeleton", false)) {
        if (OgreBinarySerializer::ImportSkeleton(pIOHandler, mesh))
            return true;

        /** Last fallback if .skeleton failed to be read. Try reading from
            .skeleton.xml even if the XML file referenced a binary skeleton.
            @note This logic was in the previous version and I don't want to break
            old code that might depends on it. */
        mesh->skeletonRef = mesh->skeletonRef + ".xml";
    }

    XmlParserPtr xmlParser = OpenXmlParser(pIOHandler, mesh->skeletonRef);
    if (!xmlParser.get())
        return false;

    Skeleton *skeleton = new Skeleton();
    OgreXmlSerializer serializer(xmlParser.get());
    XmlNode root = xmlParser->getRootNode();
    serializer.ReadSkeleton(root, skeleton);
    mesh->skeleton = skeleton;
    return true;
}

bool OgreXmlSerializer::ImportSkeleton(Assimp::IOSystem *pIOHandler, Mesh *mesh) {
    if (!mesh || mesh->skeletonRef.empty()) {
        return false;
    }

    XmlParserPtr xmlParser = OpenXmlParser(pIOHandler, mesh->skeletonRef);
    if (!xmlParser.get()) {
        return false;
    }

    Skeleton *skeleton = new Skeleton();
    OgreXmlSerializer serializer(xmlParser.get());
    XmlNode root = xmlParser->getRootNode();

    serializer.ReadSkeleton(root, skeleton);
    mesh->skeleton = skeleton;

    return true;
}

XmlParserPtr OgreXmlSerializer::OpenXmlParser(Assimp::IOSystem *pIOHandler, const std::string &filename) {
    if (!EndsWith(filename, ".skeleton.xml", false)) {
        ASSIMP_LOG_ERROR_F("Imported Mesh is referencing to unsupported '", filename, "' skeleton file.");
        return XmlParserPtr();
    }

    if (!pIOHandler->Exists(filename)) {
        ASSIMP_LOG_ERROR_F("Failed to find skeleton file '", filename, "' that is referenced by imported Mesh.");
        return XmlParserPtr();
    }

    std::unique_ptr<IOStream> file(pIOHandler->Open(filename));
    if (!file.get()) {
        throw DeadlyImportError("Failed to open skeleton file ", filename);
    }

    XmlParserPtr xmlParser = std::make_shared<XmlParser>();
    if (!xmlParser->parse(file.get())) {
        throw DeadlyImportError("Failed to create XML reader for skeleton file " + filename);
    }
    return xmlParser;
}

void OgreXmlSerializer::ReadSkeleton(XmlNode &node, Skeleton *skeleton) {
    if (node.name() != nnSkeleton) {
        throw DeadlyImportError("Root node is <" + std::string(node.name()) + "> expecting <skeleton>");
    }

    ASSIMP_LOG_VERBOSE_DEBUG("Reading Skeleton");

    // Optional blend mode from root node
    if (XmlParser::hasAttribute(node, "blendmode")) {
        skeleton->blendMode = (ai_tolower(ReadAttribute<std::string>(node, "blendmode")) == "cumulative" ? Skeleton::ANIMBLEND_CUMULATIVE : Skeleton::ANIMBLEND_AVERAGE);
    }

    for (XmlNode &currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == nnBones) {
            ReadBones(currentNode, skeleton);
        } else if (currentName == nnBoneHierarchy) {
            ReadBoneHierarchy(currentNode, skeleton);
        } else if (currentName == nnAnimations) {
            ReadAnimations(currentNode, skeleton);
        }
    }
}

void OgreXmlSerializer::ReadAnimations(XmlNode &node, Skeleton *skeleton) {
    if (skeleton->bones.empty()) {
        throw DeadlyImportError("Cannot read <animations> for a Skeleton without bones");
    }

    ASSIMP_LOG_VERBOSE_DEBUG("  - Animations");

    for (XmlNode &currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == nnAnimation) {
            Animation *anim = new Animation(skeleton);
            anim->name = ReadAttribute<std::string>(currentNode, "name");
            anim->length = ReadAttribute<float>(currentNode, "length");
            for (XmlNode &currentChildNode : currentNode.children()) {
                const std::string currentChildName = currentNode.name();
                if (currentChildName == nnTracks) {
                    ReadAnimationTracks(currentChildNode, anim);
                    skeleton->animations.push_back(anim);
                } else {
                    throw DeadlyImportError("No <tracks> found in <animation> ", anim->name);
                }
            }
        }
    }
}

void OgreXmlSerializer::ReadAnimationTracks(XmlNode &node, Animation *dest) {
    for (XmlNode &currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == nnTrack) {
            VertexAnimationTrack track;
            track.type = VertexAnimationTrack::VAT_TRANSFORM;
            track.boneName = ReadAttribute<std::string>(currentNode, "bone");
            for (XmlNode &currentChildNode : currentNode.children()) {
                const std::string currentChildName = currentNode.name();
                if (currentChildName == nnKeyFrames) {
                    ReadAnimationKeyFrames(currentChildNode, dest, &track);
                    dest->tracks.push_back(track);
                } else {
                    throw DeadlyImportError("No <keyframes> found in <track> ", dest->name);
                }
            }
        }
    }
}

void OgreXmlSerializer::ReadAnimationKeyFrames(XmlNode &node, Animation *anim, VertexAnimationTrack *dest) {
    const aiVector3D zeroVec(0.f, 0.f, 0.f);
    for (XmlNode &currentNode : node.children()) {
        TransformKeyFrame keyframe;
        const std::string currentName = currentNode.name();
        if (currentName == nnKeyFrame) {
            keyframe.timePos = ReadAttribute<float>(currentNode, "time");
            for (XmlNode &currentChildNode : currentNode.children()) {
                const std::string currentChildName = currentNode.name();
                if (currentChildName == nnTranslate) {
                    keyframe.position.x = ReadAttribute<float>(currentChildNode, anX);
                    keyframe.position.y = ReadAttribute<float>(currentChildNode, anY);
                    keyframe.position.z = ReadAttribute<float>(currentChildNode, anZ);
                } else if (currentChildName == nnRotate) {
                    float angle = ReadAttribute<float>(currentChildNode, "angle");
                    for (XmlNode &currentChildChildNode : currentNode.children()) {
                        const std::string currentChildChildName = currentNode.name();
                        if (currentChildChildName == nnAxis) {
                            aiVector3D axis;
                            axis.x = ReadAttribute<float>(currentChildChildNode, anX);
                            axis.y = ReadAttribute<float>(currentChildChildNode, anY);
                            axis.z = ReadAttribute<float>(currentChildChildNode, anZ);
                            if (axis.Equal(zeroVec)) {
                                axis.x = 1.0f;
                                if (angle != 0) {
                                    ASSIMP_LOG_WARN_F("Found invalid a key frame with a zero rotation axis in animation: ", anim->name);
                                }
                            }
                            keyframe.rotation = aiQuaternion(axis, angle);
                        }
                    }
                } else if (currentChildName == nnScale) {
                    keyframe.scale.x = ReadAttribute<float>(currentChildNode, anX);
                    keyframe.scale.y = ReadAttribute<float>(currentChildNode, anY);
                    keyframe.scale.z = ReadAttribute<float>(currentChildNode, anZ);
                }
            }
        }
        dest->transformKeyFrames.push_back(keyframe);
    }
}

void OgreXmlSerializer::ReadBoneHierarchy(XmlNode &node, Skeleton *skeleton) {
    if (skeleton->bones.empty()) {
        throw DeadlyImportError("Cannot read <bonehierarchy> for a Skeleton without bones");
    }

    for (XmlNode &currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == nnBoneParent) {
            const std::string name = ReadAttribute<std::string>(currentNode, "bone");
            const std::string parentName = ReadAttribute<std::string>(currentNode, "parent");

            Bone *bone = skeleton->BoneByName(name);
            Bone *parent = skeleton->BoneByName(parentName);

            if (bone && parent) {
                parent->AddChild(bone);
            } else {
                throw DeadlyImportError("Failed to find bones for parenting: Child ", name, " for parent ", parentName);
            }
        }
    }

    // Calculate bone matrices for root bones. Recursively calculates their children.
    for (size_t i = 0, len = skeleton->bones.size(); i < len; ++i) {
        Bone *bone = skeleton->bones[i];
        if (!bone->IsParented())
            bone->CalculateWorldMatrixAndDefaultPose(skeleton);
    }
}

static bool BoneCompare(Bone *a, Bone *b) {
    ai_assert(nullptr != a);
    ai_assert(nullptr != b);

    return (a->id < b->id);
}

void OgreXmlSerializer::ReadBones(XmlNode &node, Skeleton *skeleton) {
    ASSIMP_LOG_VERBOSE_DEBUG("  - Bones");

    for (XmlNode &currentNode : node.children()) {
        const std::string currentName = currentNode.name();
        if (currentName == nnBone) {
            Bone *bone = new Bone();
            bone->id = ReadAttribute<uint16_t>(currentNode, "id");
            bone->name = ReadAttribute<std::string>(currentNode, "name");
            for (XmlNode &currentChildNode : currentNode.children()) {
                const std::string currentChildName = currentNode.name();
                if (currentChildName == nnRotation) {
                    bone->position.x = ReadAttribute<float>(currentChildNode, anX);
                    bone->position.y = ReadAttribute<float>(currentChildNode, anY);
                    bone->position.z = ReadAttribute<float>(currentChildNode, anZ);
                } else if (currentChildName == nnScale) {
                    float angle = ReadAttribute<float>(currentChildNode, "angle");
                    for (XmlNode currentChildChildNode : currentChildNode.children()) {
                        const std::string &currentChildChildName = currentChildChildNode.name();
                        if (currentChildChildName == nnAxis) {
                            aiVector3D axis;
                            axis.x = ReadAttribute<float>(currentChildChildNode, anX);
                            axis.y = ReadAttribute<float>(currentChildChildNode, anY);
                            axis.z = ReadAttribute<float>(currentChildChildNode, anZ);

                            bone->rotation = aiQuaternion(axis, angle);
                        } else {
                            throw DeadlyImportError("No axis specified for bone rotation in bone ", bone->id);
                        }
                    }
                } else if (currentChildName == nnScale) {
                    if (XmlParser::hasAttribute(currentChildNode, "factor")) {
                        float factor = ReadAttribute<float>(currentChildNode, "factor");
                        bone->scale.Set(factor, factor, factor);
                    } else {
                        if (XmlParser::hasAttribute(currentChildNode, anX))
                            bone->scale.x = ReadAttribute<float>(currentChildNode, anX);
                        if (XmlParser::hasAttribute(currentChildNode, anY))
                            bone->scale.y = ReadAttribute<float>(currentChildNode, anY);
                        if (XmlParser::hasAttribute(currentChildNode, anZ))
                            bone->scale.z = ReadAttribute<float>(currentChildNode, anZ);
                    }
                }
            }
            skeleton->bones.push_back(bone);
        }
    }

    // Order bones by Id
    std::sort(skeleton->bones.begin(), skeleton->bones.end(), BoneCompare);

    // Validate that bone indexes are not skipped.
    /** @note Left this from original authors code, but not sure if this is strictly necessary
        as per the Ogre skeleton spec. It might be more that other (later) code in this imported does not break. */
    for (size_t i = 0, len = skeleton->bones.size(); i < len; ++i) {
        Bone *b = skeleton->bones[i];
        ASSIMP_LOG_VERBOSE_DEBUG_F("    ", b->id, " ", b->name);

        if (b->id != static_cast<uint16_t>(i)) {
            throw DeadlyImportError("Bone ids are not in sequence starting from 0. Missing index ", i);
        }
    }
}

} // namespace Ogre
} // namespace Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
