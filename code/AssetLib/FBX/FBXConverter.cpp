/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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

/** @file  FBXConverter.cpp
 *  @brief Implementation of the FBX DOM -> aiScene converter
 */

#ifndef ASSIMP_BUILD_NO_FBX_IMPORTER

#include "FBXConverter.h"
#include "FBXDocument.h"
#include "FBXImporter.h"
#include "FBXMeshGeometry.h"
#include "FBXParser.h"
#include "FBXProperties.h"
#include "FBXUtil.h"

#include <assimp/MathFunctions.h>
#include <assimp/StringComparison.h>
#include <assimp/scene.h>
#include <assimp/CreateAnimMesh.h>
#include <assimp/StringUtils.h>
#include <assimp/commonMetaData.h>

#include <stdlib.h>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <memory>
#include <sstream>

namespace Assimp {
namespace FBX {

using namespace Util;

static constexpr char MAGIC_NODE_TAG[] = "_$AssimpFbx$";

#define CONVERT_FBX_TIME(time) static_cast<double>(time) / 46186158000LL

static void correctRootTransform(const aiScene *scene) {
    if (scene == nullptr) {
        return;
    }

    if (scene->mMetaData == nullptr) {
        return;
    }

    int32_t UpAxis = 1, UpAxisSign = 1, FrontAxis = 2, FrontAxisSign = 1, CoordAxis = 0, CoordAxisSign = 1;
    double UnitScaleFactor = 1.0;
    for (unsigned MetadataIndex = 0; MetadataIndex < scene->mMetaData->mNumProperties; ++MetadataIndex) {
        if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "UpAxis") == 0) {
            scene->mMetaData->Get<int32_t>(MetadataIndex, UpAxis);
        }
        if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "UpAxisSign") == 0) {
            scene->mMetaData->Get<int32_t>(MetadataIndex, UpAxisSign);
        }
        if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "FrontAxis") == 0) {
            scene->mMetaData->Get<int32_t>(MetadataIndex, FrontAxis);
        }
        if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "FrontAxisSign") == 0) {
            scene->mMetaData->Get<int32_t>(MetadataIndex, FrontAxisSign);
        }
        if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "CoordAxis") == 0) {
            scene->mMetaData->Get<int32_t>(MetadataIndex, CoordAxis);
        }
        if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "CoordAxisSign") == 0) {
            scene->mMetaData->Get<int32_t>(MetadataIndex, CoordAxisSign);
        }
        if (strcmp(scene->mMetaData->mKeys[MetadataIndex].C_Str(), "UnitScaleFactor") == 0) {
            scene->mMetaData->Get<double>(MetadataIndex, UnitScaleFactor);
        }
    }

    aiVector3D upVec, forwardVec, rightVec;
    upVec[UpAxis] = UpAxisSign * static_cast<float>(UnitScaleFactor);
    forwardVec[FrontAxis] = FrontAxisSign * static_cast<float>(UnitScaleFactor);
    rightVec[CoordAxis] = CoordAxisSign * (float)UnitScaleFactor;

    aiMatrix4x4 mat(rightVec.x, rightVec.y, rightVec.z, 0.0f,
                    upVec.x, upVec.y, upVec.z, 0.0f,
                    forwardVec.x, forwardVec.y, forwardVec.z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
    scene->mRootNode->mTransformation *= mat;
}

FBXConverter::FBXConverter(aiScene *out, const Document &doc, bool removeEmptyBones) :
        defaultMaterialIndex(),
        anim_fps(),
        mSceneOut(out),
        doc(doc),
        mRemoveEmptyBones(removeEmptyBones) {
    // animations need to be converted first since this will
    // populate the node_anim_chain_bits map, which is needed
    // to determine which nodes need to be generated.
    ConvertAnimations();
    // Embedded textures in FBX could be connected to nothing but to itself,
    // for instance Texture -> Video connection only but not to the main graph,
    // The idea here is to traverse all objects to find these Textures and convert them,
    // so later during material conversion it will find converted texture in the textures_converted array.
    if (doc.Settings().readTextures) {
        ConvertOrphanedEmbeddedTextures();
    }
    ConvertRootNode();

    if (doc.Settings().readAllMaterials) {
        // unfortunately this means we have to evaluate all objects
        for (const ObjectMap::value_type &v : doc.Objects()) {

            const Object *ob = v.second->Get();
            if (!ob) {
                continue;
            }

            auto mat = dynamic_cast<const Material *>(ob);
            if (mat) {

                if (materials_converted.find(mat) == materials_converted.end()) {
                    ConvertMaterial(*mat, nullptr);
                }
            }
        }
    }

    ConvertGlobalSettings();
    TransferDataToScene();

    // if we didn't read any meshes set the AI_SCENE_FLAGS_INCOMPLETE
    // to make sure the scene passes assimp's validation. FBX files
    // need not contain geometry (i.e. camera animations, raw armatures).
    if (out->mNumMeshes == 0) {
        out->mFlags |= AI_SCENE_FLAGS_INCOMPLETE;
    } else {
        // Apply the FBX axis metadata unless requested not to
        if (!doc.Settings().ignoreUpDirection)
            correctRootTransform(mSceneOut);
    }
}

FBXConverter::~FBXConverter() {
    std::for_each(mMeshes.begin(), mMeshes.end(), Util::delete_fun<aiMesh>());
    std::for_each(materials.begin(), materials.end(), Util::delete_fun<aiMaterial>());
    std::for_each(animations.begin(), animations.end(), Util::delete_fun<aiAnimation>());
    std::for_each(lights.begin(), lights.end(), Util::delete_fun<aiLight>());
    std::for_each(cameras.begin(), cameras.end(), Util::delete_fun<aiCamera>());
    std::for_each(textures.begin(), textures.end(), Util::delete_fun<aiTexture>());
}

void FBXConverter::ConvertRootNode() {
    mSceneOut->mRootNode = new aiNode();
    std::string unique_name;
    GetUniqueName("RootNode", unique_name);
    mSceneOut->mRootNode->mName.Set(unique_name);

    // root has ID 0
    ConvertNodes(0L, mSceneOut->mRootNode, mSceneOut->mRootNode);
}

static std::string getAncestorBaseName(const aiNode *node) {
    const char *nodeName = nullptr;
    size_t length = 0;
    while (node && (!nodeName || length == 0)) {
        nodeName = node->mName.C_Str();
        length = node->mName.length;
        node = node->mParent;
    }

    if (!nodeName || length == 0) {
        return {};
    }
    // could be std::string_view if c++17 available
    return std::string(nodeName, length);
}

std::string FBXConverter::MakeUniqueNodeName(const Model *const model, const aiNode &parent) {
    std::string original_name = FixNodeName(model->Name());
    if (original_name.empty()) {
        original_name = getAncestorBaseName(&parent);
    }
    std::string unique_name;
    GetUniqueName(original_name, unique_name);
    return unique_name;
}

/// This struct manages nodes which may or may not end up in the node hierarchy.
/// When a node becomes a child of another node, that node becomes its owner and mOwnership should be released.
struct FBXConverter::PotentialNode {
    PotentialNode() : mOwnership(new aiNode), mNode(mOwnership.get()) {
        // empty
    }
    
    explicit PotentialNode(const std::string& name) : mOwnership(new aiNode(name)), mNode(mOwnership.get()) {
        // empty
    }
    
    aiNode* operator->() {
        return mNode;
    }

    std::unique_ptr<aiNode> mOwnership;
    aiNode* mNode;
};

/// todo: pre-build node hierarchy
/// todo: get bone from stack
/// todo: make map of aiBone* to aiNode*
/// then update convert clusters to the new format
void FBXConverter::ConvertNodes(uint64_t id, aiNode *parent, aiNode *root_node, const aiMatrix4x4& parent_transform) {
    const std::vector<const Connection *> &conns = doc.GetConnectionsByDestinationSequenced(id, "Model");

    std::vector<PotentialNode> nodes;
    nodes.reserve(conns.size());

    std::vector<PotentialNode> nodes_chain;
    std::vector<PotentialNode> post_nodes_chain;

    for (const Connection *con : conns) {
        // ignore object-property links
        if (con->PropertyName().length()) {
            // really important we document why this is ignored.
            FBXImporter::LogInfo("ignoring property link - no docs on why this is ignored");
            continue; //?
        }

        // convert connection source object into Object base class
        const Object *const object = con->SourceObject();
        if (nullptr == object) {
            FBXImporter::LogError("failed to convert source object for Model link");
            continue;
        }

        // FBX Model::Cube, Model::Bone001, etc elements
        // This detects if we can cast the object into this model structure.
        const Model *const model = dynamic_cast<const Model *>(object);

        if (nullptr != model) {
            nodes_chain.clear();
            post_nodes_chain.clear();
            aiMatrix4x4 new_abs_transform = parent_transform;
            std::string node_name = FixNodeName(model->Name());
            // even though there is only a single input node, the design of
            // assimp (or rather: the complicated transformation chain that
            // is employed by fbx) means that we may need multiple aiNode's
            // to represent a fbx node's transformation.

            // generate node transforms - this includes pivot data
            // if need_additional_node is true then you t
            const bool need_additional_node = GenerateTransformationNodeChain(*model, node_name, nodes_chain, post_nodes_chain);

            // assert that for the current node we must have at least a single transform
            ai_assert(nodes_chain.size());

            if (need_additional_node) {
                nodes_chain.emplace_back(node_name);
            }

            //setup metadata on newest node
            SetupNodeMetadata(*model, *nodes_chain.back().mNode);

            // link all nodes in a row
            aiNode *last_parent = parent;
            for (PotentialNode& child : nodes_chain) {
                ai_assert(child.mNode);

                if (last_parent != parent) {
                    last_parent->mNumChildren = 1;
                    last_parent->mChildren = new aiNode *[1];
                    last_parent->mChildren[0] = child.mOwnership.release();
                }

                child->mParent = last_parent;
                last_parent = child.mNode;

                new_abs_transform *= child->mTransformation;
            }

            // attach geometry
            ConvertModel(*model, nodes_chain.back().mNode, root_node, new_abs_transform);

            // check if there will be any child nodes
            const std::vector<const Connection *> &child_conns = doc.GetConnectionsByDestinationSequenced(model->ID(), "Model");

            // if so, link the geometric transform inverse nodes
            // before we attach any child nodes
            if (child_conns.size()) {
                for (PotentialNode& postnode : post_nodes_chain) {
                    ai_assert(postnode.mNode);

                    if (last_parent != parent) {
                        last_parent->mNumChildren = 1;
                        last_parent->mChildren = new aiNode *[1];
                        last_parent->mChildren[0] = postnode.mOwnership.release();
                    }

                    postnode->mParent = last_parent;
                    last_parent = postnode.mNode;

                    new_abs_transform *= postnode->mTransformation;
                }
            } else {
                // free the nodes we allocated as we don't need them
                post_nodes_chain.clear();
            }

            // recursion call - child nodes
            ConvertNodes(model->ID(), last_parent, root_node, new_abs_transform);

            if (doc.Settings().readLights) {
                ConvertLights(*model, node_name);
            }

            if (doc.Settings().readCameras) {
                ConvertCameras(*model, node_name);
            }

            nodes.push_back(std::move(nodes_chain.front()));
            nodes_chain.clear();
        }
    }

    if (nodes.empty()) {
        parent->mNumChildren = 0;
        parent->mChildren = nullptr;
    } else {
        parent->mChildren = new aiNode *[nodes.size()]();
        parent->mNumChildren = static_cast<unsigned int>(nodes.size());
        for (unsigned int i = 0; i < nodes.size(); ++i) {
            parent->mChildren[i] = nodes[i].mOwnership.release();
        }
    }
}

void FBXConverter::ConvertLights(const Model &model, const std::string &orig_name) {
    const std::vector<const NodeAttribute *> &node_attrs = model.GetAttributes();
    for (const NodeAttribute *attr : node_attrs) {
        const Light *const light = dynamic_cast<const Light *>(attr);
        if (light) {
            ConvertLight(*light, orig_name);
        }
    }
}

void FBXConverter::ConvertCameras(const Model &model, const std::string &orig_name) {
    const std::vector<const NodeAttribute *> &node_attrs = model.GetAttributes();
    for (const NodeAttribute *attr : node_attrs) {
        const Camera *const cam = dynamic_cast<const Camera *>(attr);
        if (cam) {
            ConvertCamera(*cam, orig_name);
        }
    }
}

void FBXConverter::ConvertLight(const Light &light, const std::string &orig_name) {
    lights.push_back(new aiLight());
    aiLight *const out_light = lights.back();

    out_light->mName.Set(orig_name);

    const float intensity = light.Intensity() / 100.0f;
    const aiVector3D &col = light.Color();

    out_light->mColorDiffuse = aiColor3D(col.x, col.y, col.z);
    out_light->mColorDiffuse.r *= intensity;
    out_light->mColorDiffuse.g *= intensity;
    out_light->mColorDiffuse.b *= intensity;

    out_light->mColorSpecular = out_light->mColorDiffuse;

    //lights are defined along negative y direction
    out_light->mPosition = aiVector3D(0.0f);
    out_light->mDirection = aiVector3D(0.0f, -1.0f, 0.0f);
    out_light->mUp = aiVector3D(0.0f, 0.0f, -1.0f);

    switch (light.LightType()) {
        case Light::Type_Point:
            out_light->mType = aiLightSource_POINT;
            break;

        case Light::Type_Directional:
            out_light->mType = aiLightSource_DIRECTIONAL;
            break;

        case Light::Type_Spot:
            out_light->mType = aiLightSource_SPOT;
            out_light->mAngleOuterCone = AI_DEG_TO_RAD(light.OuterAngle());
            out_light->mAngleInnerCone = AI_DEG_TO_RAD(light.InnerAngle());
            break;

        case Light::Type_Area:
            FBXImporter::LogWarn("cannot represent area light, set to UNDEFINED");
            out_light->mType = aiLightSource_UNDEFINED;
            break;

        case Light::Type_Volume:
            FBXImporter::LogWarn("cannot represent volume light, set to UNDEFINED");
            out_light->mType = aiLightSource_UNDEFINED;
            break;
        default:
            FBXImporter::LogError("Not handled light type: ", light.LightType());
            break;
    }

    float decay = light.DecayStart();
    switch (light.DecayType()) {
        case Light::Decay_None:
            out_light->mAttenuationConstant = decay;
            out_light->mAttenuationLinear = 0.0f;
            out_light->mAttenuationQuadratic = 0.0f;
            break;
        case Light::Decay_Linear:
            out_light->mAttenuationConstant = 0.0f;
            out_light->mAttenuationLinear = 2.0f / decay;
            out_light->mAttenuationQuadratic = 0.0f;
            break;
        case Light::Decay_Quadratic:
            out_light->mAttenuationConstant = 0.0f;
            out_light->mAttenuationLinear = 0.0f;
            out_light->mAttenuationQuadratic = 2.0f / (decay * decay);
            break;
        case Light::Decay_Cubic:
            FBXImporter::LogWarn("cannot represent cubic attenuation, set to Quadratic");
            out_light->mAttenuationQuadratic = 1.0f;
            break;
        default:
            FBXImporter::LogError("Not handled light decay type: ", light.DecayType());
            break;
    }
}

void FBXConverter::ConvertCamera(const Camera &cam, const std::string &orig_name) {
    cameras.push_back(new aiCamera());
    aiCamera *const out_camera = cameras.back();

    out_camera->mName.Set(orig_name);

    out_camera->mAspect = cam.AspectWidth() / cam.AspectHeight();

    // NOTE: Camera mPosition, mLookAt and mUp must be set to default here.
    // All transformations to the camera will be handled by its node in the scenegraph.
    out_camera->mPosition = aiVector3D(0.0f);
    out_camera->mLookAt = aiVector3D(1.0f, 0.0f, 0.0f);
    out_camera->mUp = aiVector3D(0.0f, 1.0f, 0.0f);

    // NOTE: Some software (maya) does not put FieldOfView in FBX, so we compute
    // mHorizontalFOV from FocalLength and FilmWidth with unit conversion.

    // TODO: This is not a complete solution for how FBX cameras can be stored.
    // TODO: Incorporate non-square pixel aspect ratio.
    // TODO: FBX aperture mode might be storing vertical FOV in need of conversion with aspect ratio.

    float fov_deg = cam.FieldOfView();
    // If FOV not specified in file, compute using FilmWidth and FocalLength.
    if (fov_deg == kFovUnknown) {
        float film_width_inches = cam.FilmWidth();
        float focal_length_mm = cam.FocalLength();
        ASSIMP_LOG_VERBOSE_DEBUG("FBX FOV unspecified. Computing from FilmWidth (", film_width_inches, "inches) and FocalLength (", focal_length_mm, "mm).");
        double half_fov_rad = std::atan2(film_width_inches * 25.4 * 0.5, focal_length_mm);
        out_camera->mHorizontalFOV = static_cast<float>(half_fov_rad);
    } else {
        // FBX fov is full-view degrees. We want half-view radians.
        out_camera->mHorizontalFOV = AI_DEG_TO_RAD(fov_deg) * 0.5f;
    }

    out_camera->mClipPlaneNear = cam.NearPlane();
    out_camera->mClipPlaneFar = cam.FarPlane();
}

void FBXConverter::GetUniqueName(const std::string &name, std::string &uniqueName) {
    uniqueName = name;
    auto it_pair = mNodeNames.insert({ name, 0 }); // duplicate node name instance count
    unsigned int &i = it_pair.first->second;
    while (!it_pair.second) {
        ++i;
        std::ostringstream ext;
        ext << name << std::setfill('0') << std::setw(3) << i;
        uniqueName = ext.str();
        it_pair = mNodeNames.insert({ uniqueName, 0 });
    }
}

const char *FBXConverter::NameTransformationComp(TransformationComp comp) {
    switch (comp) {
        case TransformationComp_Translation:
            return "Translation";
        case TransformationComp_RotationOffset:
            return "RotationOffset";
        case TransformationComp_RotationPivot:
            return "RotationPivot";
        case TransformationComp_PreRotation:
            return "PreRotation";
        case TransformationComp_Rotation:
            return "Rotation";
        case TransformationComp_PostRotation:
            return "PostRotation";
        case TransformationComp_RotationPivotInverse:
            return "RotationPivotInverse";
        case TransformationComp_ScalingOffset:
            return "ScalingOffset";
        case TransformationComp_ScalingPivot:
            return "ScalingPivot";
        case TransformationComp_Scaling:
            return "Scaling";
        case TransformationComp_ScalingPivotInverse:
            return "ScalingPivotInverse";
        case TransformationComp_GeometricScaling:
            return "GeometricScaling";
        case TransformationComp_GeometricRotation:
            return "GeometricRotation";
        case TransformationComp_GeometricTranslation:
            return "GeometricTranslation";
        case TransformationComp_GeometricScalingInverse:
            return "GeometricScalingInverse";
        case TransformationComp_GeometricRotationInverse:
            return "GeometricRotationInverse";
        case TransformationComp_GeometricTranslationInverse:
            return "GeometricTranslationInverse";
        case TransformationComp_MAXIMUM: // this is to silence compiler warnings
        default:
            break;
    }

    ai_assert(false);

    return nullptr;
}

const char *FBXConverter::NameTransformationCompProperty(TransformationComp comp) {
    switch (comp) {
        case TransformationComp_Translation:
            return "Lcl Translation";
        case TransformationComp_RotationOffset:
            return "RotationOffset";
        case TransformationComp_RotationPivot:
            return "RotationPivot";
        case TransformationComp_PreRotation:
            return "PreRotation";
        case TransformationComp_Rotation:
            return "Lcl Rotation";
        case TransformationComp_PostRotation:
            return "PostRotation";
        case TransformationComp_RotationPivotInverse:
            return "RotationPivotInverse";
        case TransformationComp_ScalingOffset:
            return "ScalingOffset";
        case TransformationComp_ScalingPivot:
            return "ScalingPivot";
        case TransformationComp_Scaling:
            return "Lcl Scaling";
        case TransformationComp_ScalingPivotInverse:
            return "ScalingPivotInverse";
        case TransformationComp_GeometricScaling:
            return "GeometricScaling";
        case TransformationComp_GeometricRotation:
            return "GeometricRotation";
        case TransformationComp_GeometricTranslation:
            return "GeometricTranslation";
        case TransformationComp_GeometricScalingInverse:
            return "GeometricScalingInverse";
        case TransformationComp_GeometricRotationInverse:
            return "GeometricRotationInverse";
        case TransformationComp_GeometricTranslationInverse:
            return "GeometricTranslationInverse";
        case TransformationComp_MAXIMUM:
            break;
    }

    ai_assert(false);

    return nullptr;
}

aiVector3D FBXConverter::TransformationCompDefaultValue(TransformationComp comp) {
    // XXX a neat way to solve the never-ending special cases for scaling
    // would be to do everything in log space!
    return comp == TransformationComp_Scaling ? aiVector3D(1.f, 1.f, 1.f) : aiVector3D();
}

void FBXConverter::GetRotationMatrix(Model::RotOrder mode, const aiVector3D &rotation, aiMatrix4x4 &out) {
    if (mode == Model::RotOrder_SphericXYZ) {
        FBXImporter::LogError("Unsupported RotationMode: SphericXYZ");
        out = aiMatrix4x4();
        return;
    }

    const float angle_epsilon = Math::getEpsilon<float>();

    out = aiMatrix4x4();

    bool is_id[3] = { true, true, true };

    aiMatrix4x4 temp[3];
    const auto rot = AI_DEG_TO_RAD(rotation);
    if (std::fabs(rot.z) > angle_epsilon) {
        aiMatrix4x4::RotationZ(rot.z, temp[2]);
        is_id[2] = false;
    }
    if (std::fabs(rot.y) > angle_epsilon) {
        aiMatrix4x4::RotationY(rot.y, temp[1]);
        is_id[1] = false;
    }
    if (std::fabs(rot.x) > angle_epsilon) {
        aiMatrix4x4::RotationX(rot.x, temp[0]);
        is_id[0] = false;
    }

    int order[3] = { -1, -1, -1 };

    // note: rotation order is inverted since we're left multiplying as is usual in assimp
    switch (mode) {
        case Model::RotOrder_EulerXYZ:
            order[0] = 2;
            order[1] = 1;
            order[2] = 0;
            break;

        case Model::RotOrder_EulerXZY:
            order[0] = 1;
            order[1] = 2;
            order[2] = 0;
            break;

        case Model::RotOrder_EulerYZX:
            order[0] = 0;
            order[1] = 2;
            order[2] = 1;
            break;

        case Model::RotOrder_EulerYXZ:
            order[0] = 2;
            order[1] = 0;
            order[2] = 1;
            break;

        case Model::RotOrder_EulerZXY:
            order[0] = 1;
            order[1] = 0;
            order[2] = 2;
            break;

        case Model::RotOrder_EulerZYX:
            order[0] = 0;
            order[1] = 1;
            order[2] = 2;
            break;

        default:
            ai_assert(false);
            break;
    }

    ai_assert(order[0] >= 0);
    ai_assert(order[0] <= 2);
    ai_assert(order[1] >= 0);
    ai_assert(order[1] <= 2);
    ai_assert(order[2] >= 0);
    ai_assert(order[2] <= 2);

    if (!is_id[order[0]]) {
        out = temp[order[0]];
    }

    if (!is_id[order[1]]) {
        out = out * temp[order[1]];
    }

    if (!is_id[order[2]]) {
        out = out * temp[order[2]];
    }
}

bool FBXConverter::NeedsComplexTransformationChain(const Model &model) {
    const PropertyTable &props = model.Props();

    const auto zero_epsilon = Math::getEpsilon<ai_real>();
    const aiVector3D all_ones(1.0f, 1.0f, 1.0f);
    for (size_t i = 0; i < TransformationComp_MAXIMUM; ++i) {
        const TransformationComp comp = static_cast<TransformationComp>(i);

        if (comp == TransformationComp_Rotation || comp == TransformationComp_Scaling || comp == TransformationComp_Translation) {
            continue;
        }

        bool scale_compare = (comp == TransformationComp_GeometricScaling || comp == TransformationComp_Scaling);

        bool ok = true;
        const aiVector3D &v = PropertyGet<aiVector3D>(props, NameTransformationCompProperty(comp), ok);
        if (ok && scale_compare) {
            if ((v - all_ones).SquareLength() > zero_epsilon) {
                return true;
            }
        } else if (ok) {
            if (v.SquareLength() > zero_epsilon) {
                return true;
            }
        }
    }

    return false;
}

std::string FBXConverter::NameTransformationChainNode(const std::string &name, TransformationComp comp) {
    return name + std::string(MAGIC_NODE_TAG) + "_" + NameTransformationComp(comp);
}

bool FBXConverter::GenerateTransformationNodeChain(const Model &model, const std::string &name, std::vector<PotentialNode> &output_nodes,
        std::vector<PotentialNode> &post_output_nodes) {
    const PropertyTable &props = model.Props();
    const Model::RotOrder rot = model.RotationOrder();

    bool ok;

    aiMatrix4x4 chain[TransformationComp_MAXIMUM];

    ai_assert(TransformationComp_MAXIMUM < 32);
    std::uint32_t chainBits = 0;
    // A node won't need a node chain if it only has these.
    const std::uint32_t chainMaskSimple = (1 << TransformationComp_Translation) + (1 << TransformationComp_Scaling) + (1 << TransformationComp_Rotation);
    // A node will need a node chain if it has any of these.
    const std::uint32_t chainMaskComplex = ((1 << (TransformationComp_MAXIMUM)) - 1) - chainMaskSimple;

    std::fill_n(chain, static_cast<unsigned int>(TransformationComp_MAXIMUM), aiMatrix4x4());

    // generate transformation matrices for all the different transformation components
    const float zero_epsilon = Math::getEpsilon<float>();
    const aiVector3D all_ones(1.0f, 1.0f, 1.0f);

    const aiVector3D &PreRotation = PropertyGet<aiVector3D>(props, "PreRotation", ok);
    if (ok && PreRotation.SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_PreRotation);

        GetRotationMatrix(Model::RotOrder::RotOrder_EulerXYZ, PreRotation, chain[TransformationComp_PreRotation]);
    }

    const aiVector3D &PostRotation = PropertyGet<aiVector3D>(props, "PostRotation", ok);
    if (ok && PostRotation.SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_PostRotation);

        GetRotationMatrix(Model::RotOrder::RotOrder_EulerXYZ, PostRotation, chain[TransformationComp_PostRotation]);
    }

    const aiVector3D &RotationPivot = PropertyGet<aiVector3D>(props, "RotationPivot", ok);
    if (ok && RotationPivot.SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_RotationPivot) | (1 << TransformationComp_RotationPivotInverse);

        aiMatrix4x4::Translation(RotationPivot, chain[TransformationComp_RotationPivot]);
        aiMatrix4x4::Translation(-RotationPivot, chain[TransformationComp_RotationPivotInverse]);
    }

    const aiVector3D &RotationOffset = PropertyGet<aiVector3D>(props, "RotationOffset", ok);
    if (ok && RotationOffset.SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_RotationOffset);

        aiMatrix4x4::Translation(RotationOffset, chain[TransformationComp_RotationOffset]);
    }

    const aiVector3D &ScalingOffset = PropertyGet<aiVector3D>(props, "ScalingOffset", ok);
    if (ok && ScalingOffset.SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_ScalingOffset);

        aiMatrix4x4::Translation(ScalingOffset, chain[TransformationComp_ScalingOffset]);
    }

    const aiVector3D &ScalingPivot = PropertyGet<aiVector3D>(props, "ScalingPivot", ok);
    if (ok && ScalingPivot.SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_ScalingPivot) | (1 << TransformationComp_ScalingPivotInverse);

        aiMatrix4x4::Translation(ScalingPivot, chain[TransformationComp_ScalingPivot]);
        aiMatrix4x4::Translation(-ScalingPivot, chain[TransformationComp_ScalingPivotInverse]);
    }

    const aiVector3D &Translation = PropertyGet<aiVector3D>(props, "Lcl Translation", ok);
    if (ok && Translation.SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_Translation);

        aiMatrix4x4::Translation(Translation, chain[TransformationComp_Translation]);
    }

    const aiVector3D &Scaling = PropertyGet<aiVector3D>(props, "Lcl Scaling", ok);
    if (ok && (Scaling - all_ones).SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_Scaling);

        aiMatrix4x4::Scaling(Scaling, chain[TransformationComp_Scaling]);
    }

    const aiVector3D &Rotation = PropertyGet<aiVector3D>(props, "Lcl Rotation", ok);
    if (ok && Rotation.SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_Rotation);

        GetRotationMatrix(rot, Rotation, chain[TransformationComp_Rotation]);
    }

    const aiVector3D &GeometricScaling = PropertyGet<aiVector3D>(props, "GeometricScaling", ok);
    if (ok && (GeometricScaling - all_ones).SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_GeometricScaling);
        aiMatrix4x4::Scaling(GeometricScaling, chain[TransformationComp_GeometricScaling]);
        aiVector3D GeometricScalingInverse = GeometricScaling;
        bool canscale = true;
        for (unsigned int i = 0; i < 3; ++i) {
            if (std::fabs(GeometricScalingInverse[i]) > zero_epsilon) {
                GeometricScalingInverse[i] = 1.0f / GeometricScaling[i];
            } else {
                FBXImporter::LogError("cannot invert geometric scaling matrix with a 0.0 scale component");
                canscale = false;
                break;
            }
        }
        if (canscale) {
            chainBits = chainBits | (1 << TransformationComp_GeometricScalingInverse);
            aiMatrix4x4::Scaling(GeometricScalingInverse, chain[TransformationComp_GeometricScalingInverse]);
        }
    }

    const aiVector3D &GeometricRotation = PropertyGet<aiVector3D>(props, "GeometricRotation", ok);
    if (ok && GeometricRotation.SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_GeometricRotation) | (1 << TransformationComp_GeometricRotationInverse);
        GetRotationMatrix(rot, GeometricRotation, chain[TransformationComp_GeometricRotation]);
        GetRotationMatrix(rot, GeometricRotation, chain[TransformationComp_GeometricRotationInverse]);
        chain[TransformationComp_GeometricRotationInverse].Inverse();
    }

    const aiVector3D &GeometricTranslation = PropertyGet<aiVector3D>(props, "GeometricTranslation", ok);
    if (ok && GeometricTranslation.SquareLength() > zero_epsilon) {
        chainBits = chainBits | (1 << TransformationComp_GeometricTranslation) | (1 << TransformationComp_GeometricTranslationInverse);
        aiMatrix4x4::Translation(GeometricTranslation, chain[TransformationComp_GeometricTranslation]);
        aiMatrix4x4::Translation(-GeometricTranslation, chain[TransformationComp_GeometricTranslationInverse]);
    }

    // now, if we have more than just Translation, Scaling and Rotation,
    // we need to generate a full node chain to accommodate for assimp's
    // lack to express pivots and offsets.
    if ((chainBits & chainMaskComplex) && doc.Settings().preservePivots) {
        FBXImporter::LogInfo("generating full transformation chain for node: ", name);

        // query the anim_chain_bits dictionary to find out which chain elements
        // have associated node animation channels. These can not be dropped
        // even if they have identity transform in bind pose.
        NodeAnimBitMap::const_iterator it = node_anim_chain_bits.find(name);
        const unsigned int anim_chain_bitmask = (it == node_anim_chain_bits.end() ? 0 : (*it).second);

        unsigned int bit = 0x1;
        for (size_t i = 0; i < TransformationComp_MAXIMUM; ++i, bit <<= 1) {
            const TransformationComp comp = static_cast<TransformationComp>(i);

            if ((chainBits & bit) == 0 && (anim_chain_bitmask & bit) == 0) {
                continue;
            }

            if (comp == TransformationComp_PostRotation) {
                chain[i] = chain[i].Inverse();
            }

            PotentialNode nd;
            nd->mName.Set(NameTransformationChainNode(name, comp));
            nd->mTransformation = chain[i];

            // geometric inverses go in a post-node chain
            if (comp == TransformationComp_GeometricScalingInverse ||
                    comp == TransformationComp_GeometricRotationInverse ||
                    comp == TransformationComp_GeometricTranslationInverse) {
                post_output_nodes.emplace_back(std::move(nd));
            } else {
                output_nodes.emplace_back(std::move(nd));
            }
        }

        ai_assert(output_nodes.size());
        return true;
    }

    // else, we can just multiply the matrices together
    PotentialNode nd;

    // name passed to the method is already unique
    nd->mName.Set(name);
    // for (const auto &transform : chain) {
    // skip inverse chain for no preservePivots
    for (unsigned int i = TransformationComp_Translation; i < TransformationComp_MAXIMUM; i++) {
      nd->mTransformation = nd->mTransformation * chain[i];
    }
    output_nodes.push_back(std::move(nd));
    return false;
}

void FBXConverter::SetupNodeMetadata(const Model &model, aiNode &nd) {
    const PropertyTable &props = model.Props();
    DirectPropertyMap unparsedProperties = props.GetUnparsedProperties();

    // create metadata on node
    const std::size_t numStaticMetaData = 2;
    aiMetadata *data = aiMetadata::Alloc(static_cast<unsigned int>(unparsedProperties.size() + numStaticMetaData));
    nd.mMetaData = data;
    int index = 0;

    // find user defined properties (3ds Max)
    data->Set(index++, "UserProperties", aiString(PropertyGet<std::string>(props, "UDP3DSMAX", "")));
    // preserve the info that a node was marked as Null node in the original file.
    data->Set(index++, "IsNull", model.IsNull() ? true : false);

    // add unparsed properties to the node's metadata
    for (const DirectPropertyMap::value_type &prop : unparsedProperties) {
        // Interpret the property as a concrete type
        if (const TypedProperty<bool> *interpretedBool = prop.second->As<TypedProperty<bool>>()) {
            data->Set(index++, prop.first, interpretedBool->Value());
        } else if (const TypedProperty<int> *interpretedInt = prop.second->As<TypedProperty<int>>()) {
            data->Set(index++, prop.first, interpretedInt->Value());
        } else if (const TypedProperty<uint32_t> *interpretedUInt = prop.second->As<TypedProperty<uint32_t>>()) {
            data->Set(index++, prop.first, interpretedUInt->Value());
        } else if (const TypedProperty<uint64_t> *interpretedUint64 = prop.second->As<TypedProperty<uint64_t>>()) {
            data->Set(index++, prop.first, interpretedUint64->Value());
        } else if (const TypedProperty<int64_t> *interpretedint64 = prop.second->As<TypedProperty<int64_t>>()) {
            data->Set(index++, prop.first, interpretedint64->Value());
        } else if (const TypedProperty<float> *interpretedFloat = prop.second->As<TypedProperty<float>>()) {
            data->Set(index++, prop.first, interpretedFloat->Value());
        } else if (const TypedProperty<std::string> *interpretedString = prop.second->As<TypedProperty<std::string>>()) {
            data->Set(index++, prop.first, aiString(interpretedString->Value()));
        } else if (const TypedProperty<aiVector3D> *interpretedVec3 = prop.second->As<TypedProperty<aiVector3D>>()) {
            data->Set(index++, prop.first, interpretedVec3->Value());
        } else {
            ai_assert(false);
        }
    }
}

void FBXConverter::ConvertModel(const Model &model, aiNode *parent, aiNode *root_node, const aiMatrix4x4 &absolute_transform) {
    const std::vector<const Geometry *> &geos = model.GetGeometry();

    std::vector<unsigned int> meshes;
    meshes.reserve(geos.size());

    for (const Geometry *geo : geos) {
        const MeshGeometry *const mesh = dynamic_cast<const MeshGeometry *>(geo);
        const LineGeometry *const line = dynamic_cast<const LineGeometry *>(geo);
        if (mesh) {
            const std::vector<unsigned int> &indices = ConvertMesh(*mesh, model, parent, root_node, absolute_transform);
            std::copy(indices.begin(), indices.end(), std::back_inserter(meshes));
        } else if (line) {
            const std::vector<unsigned int> &indices = ConvertLine(*line, root_node);
            std::copy(indices.begin(), indices.end(), std::back_inserter(meshes));
        } else if (geo) {
            FBXImporter::LogWarn("ignoring unrecognized geometry: ", geo->Name());
        } else {
            FBXImporter::LogWarn("skipping null geometry");
        }
    }

    if (meshes.size()) {
        parent->mMeshes = new unsigned int[meshes.size()]();
        parent->mNumMeshes = static_cast<unsigned int>(meshes.size());

        std::swap_ranges(meshes.begin(), meshes.end(), parent->mMeshes);
    }
}

std::vector<unsigned int>
FBXConverter::ConvertMesh(const MeshGeometry &mesh, const Model &model, aiNode *parent, aiNode *root_node, const aiMatrix4x4 &absolute_transform) {
    std::vector<unsigned int> temp;

    MeshMap::const_iterator it = meshes_converted.find(&mesh);
    if (it != meshes_converted.end()) {
        std::copy((*it).second.begin(), (*it).second.end(), std::back_inserter(temp));
        return temp;
    }

    const std::vector<aiVector3D> &vertices = mesh.GetVertices();
    const std::vector<unsigned int> &faces = mesh.GetFaceIndexCounts();
    if (vertices.empty() || faces.empty()) {
        FBXImporter::LogWarn("ignoring empty geometry: ", mesh.Name());
        return temp;
    }

    // one material per mesh maps easily to aiMesh. Multiple material
    // meshes need to be split.
    const MatIndexArray &mindices = mesh.GetMaterialIndices();
    if (doc.Settings().readMaterials && !mindices.empty()) {
        const MatIndexArray::value_type base = mindices[0];
        for (MatIndexArray::value_type index : mindices) {
            if (index != base) {
                return ConvertMeshMultiMaterial(mesh, model, absolute_transform, parent, root_node);
            }
        }
    }

    // faster code-path, just copy the data
    temp.push_back(ConvertMeshSingleMaterial(mesh, model, absolute_transform, parent, root_node));
    return temp;
}

std::vector<unsigned int> FBXConverter::ConvertLine(const LineGeometry &line, aiNode *root_node) {
    std::vector<unsigned int> temp;

    const std::vector<aiVector3D> &vertices = line.GetVertices();
    const std::vector<int> &indices = line.GetIndices();
    if (vertices.empty() || indices.empty()) {
        FBXImporter::LogWarn("ignoring empty line: ", line.Name());
        return temp;
    }

    aiMesh *const out_mesh = SetupEmptyMesh(line, root_node);
    out_mesh->mPrimitiveTypes |= aiPrimitiveType_LINE;

    // copy vertices
    out_mesh->mNumVertices = static_cast<unsigned int>(vertices.size());
    out_mesh->mVertices = new aiVector3D[out_mesh->mNumVertices];
    std::copy(vertices.begin(), vertices.end(), out_mesh->mVertices);

    //Number of line segments (faces) is "Number of Points - Number of Endpoints"
    //N.B.: Endpoints in FbxLine are denoted by negative indices.
    //If such an Index is encountered, add 1 and multiply by -1 to get the real index.
    unsigned int epcount = 0;
    for (unsigned i = 0; i < indices.size(); i++) {
        if (indices[i] < 0) {
            epcount++;
        }
    }
    unsigned int pcount = static_cast<unsigned int>(indices.size());
    unsigned int scount = out_mesh->mNumFaces = pcount - epcount;

    aiFace *fac = out_mesh->mFaces = new aiFace[scount]();
    for (unsigned int i = 0; i < pcount; ++i) {
        if (indices[i] < 0) continue;
        aiFace &f = *fac++;
        f.mNumIndices = 2; //2 == aiPrimitiveType_LINE
        f.mIndices = new unsigned int[2];
        f.mIndices[0] = indices[i];
        int segid = indices[(i + 1 == pcount ? 0 : i + 1)]; //If we have reached he last point, wrap around
        f.mIndices[1] = (segid < 0 ? (segid + 1) * -1 : segid); //Convert EndPoint Index to normal Index
    }
    temp.push_back(static_cast<unsigned int>(mMeshes.size() - 1));
    return temp;
}

aiMesh *FBXConverter::SetupEmptyMesh(const Geometry &mesh, aiNode *parent) {
    aiMesh *const out_mesh = new aiMesh();
    mMeshes.push_back(out_mesh);
    meshes_converted[&mesh].push_back(static_cast<unsigned int>(mMeshes.size() - 1));

    // set name
    std::string name = mesh.Name();
    if (name.substr(0, 10) == "Geometry::") {
        name = name.substr(10);
    }

    if (name.length()) {
        out_mesh->mName.Set(name);
    } else {
        out_mesh->mName = parent->mName;
    }

    return out_mesh;
}

static aiSkeleton *createAiSkeleton(SkeletonBoneContainer &sbc) {
    if (sbc.meshArray.empty() || sbc.skeletonBoneToMeshLookup.empty()) {
        return nullptr;
    }

    aiSkeleton *skeleton = new aiSkeleton;
    for (auto *mesh : sbc.meshArray) {
        auto it = sbc.skeletonBoneToMeshLookup.find(mesh);
        if (it == sbc.skeletonBoneToMeshLookup.end()) {
            continue;
        }
        SkeletonBoneArray *ba = it->second;
        if (ba == nullptr) {
            continue;
        }

        skeleton->mNumBones = static_cast<unsigned int>(ba->size());
        skeleton->mBones = new aiSkeletonBone*[skeleton->mNumBones];
        size_t index = 0;
        for (auto bone : (* ba)) {
            skeleton->mBones[index] = bone;
            ++index;
        }
    }

    return skeleton;
}

unsigned int FBXConverter::ConvertMeshSingleMaterial(const MeshGeometry &mesh, const Model &model, const aiMatrix4x4 &absolute_transform,
        aiNode *parent, aiNode *) {
    const MatIndexArray &mindices = mesh.GetMaterialIndices();
    aiMesh *const out_mesh = SetupEmptyMesh(mesh, parent);

    const std::vector<aiVector3D> &vertices = mesh.GetVertices();
    const std::vector<unsigned int> &faces = mesh.GetFaceIndexCounts();

    // copy vertices
    out_mesh->mNumVertices = static_cast<unsigned int>(vertices.size());
    out_mesh->mVertices = new aiVector3D[vertices.size()];

    std::copy(vertices.begin(), vertices.end(), out_mesh->mVertices);

    // generate dummy faces
    out_mesh->mNumFaces = static_cast<unsigned int>(faces.size());
    aiFace *fac = out_mesh->mFaces = new aiFace[faces.size()]();

    unsigned int cursor = 0;
    for (unsigned int pcount : faces) {
        aiFace &f = *fac++;
        f.mNumIndices = pcount;
        f.mIndices = new unsigned int[pcount];
        switch (pcount) {
            case 1:
                out_mesh->mPrimitiveTypes |= aiPrimitiveType_POINT;
                break;
            case 2:
                out_mesh->mPrimitiveTypes |= aiPrimitiveType_LINE;
                break;
            case 3:
                out_mesh->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
                break;
            default:
                out_mesh->mPrimitiveTypes |= aiPrimitiveType_POLYGON;
                break;
        }
        for (unsigned int i = 0; i < pcount; ++i) {
            f.mIndices[i] = cursor++;
        }
    }

    // copy normals
    const std::vector<aiVector3D> &normals = mesh.GetNormals();
    if (normals.size()) {
        ai_assert(normals.size() == vertices.size());

        out_mesh->mNormals = new aiVector3D[vertices.size()];
        std::copy(normals.begin(), normals.end(), out_mesh->mNormals);
    }

    // copy tangents - assimp requires both tangents and bitangents (binormals)
    // to be present, or neither of them. Compute binormals from normals
    // and tangents if needed.
    const std::vector<aiVector3D> &tangents = mesh.GetTangents();
    const std::vector<aiVector3D> *binormals = &mesh.GetBinormals();

    if (tangents.size()) {
        std::vector<aiVector3D> tempBinormals;
        if (!binormals->size()) {
            if (normals.size()) {
                tempBinormals.resize(normals.size());
                for (unsigned int i = 0; i < tangents.size(); ++i) {
                    tempBinormals[i] = normals[i] ^ tangents[i];
                }

                binormals = &tempBinormals;
            } else {
                binormals = nullptr;
            }
        }

        if (binormals) {
            ai_assert(tangents.size() == vertices.size());
            ai_assert(binormals->size() == vertices.size());

            out_mesh->mTangents = new aiVector3D[vertices.size()];
            std::copy(tangents.begin(), tangents.end(), out_mesh->mTangents);

            out_mesh->mBitangents = new aiVector3D[vertices.size()];
            std::copy(binormals->begin(), binormals->end(), out_mesh->mBitangents);
        }
    }

    // copy texture coords
    for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
        const std::vector<aiVector2D> &uvs = mesh.GetTextureCoords(i);
        if (uvs.empty()) {
            break;
        }

        aiVector3D *out_uv = out_mesh->mTextureCoords[i] = new aiVector3D[vertices.size()];
        for (const aiVector2D &v : uvs) {
            *out_uv++ = aiVector3D(v.x, v.y, 0.0f);
        }

        out_mesh->SetTextureCoordsName(i, aiString(mesh.GetTextureCoordChannelName(i)));

        out_mesh->mNumUVComponents[i] = 2;
    }

    // copy vertex colors
    for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
        const std::vector<aiColor4D> &colors = mesh.GetVertexColors(i);
        if (colors.empty()) {
            break;
        }

        out_mesh->mColors[i] = new aiColor4D[vertices.size()];
        std::copy(colors.begin(), colors.end(), out_mesh->mColors[i]);
    }

    if (!doc.Settings().readMaterials || mindices.empty()) {
        FBXImporter::LogError("no material assigned to mesh, setting default material");
        out_mesh->mMaterialIndex = GetDefaultMaterial();
    } else {
        ConvertMaterialForMesh(out_mesh, model, mesh, mindices[0]);
    }

    if (doc.Settings().readWeights && mesh.DeformerSkin() != nullptr && !doc.Settings().useSkeleton) {
        ConvertWeights(out_mesh, mesh, absolute_transform, parent, NO_MATERIAL_SEPARATION, nullptr);
    } else if (doc.Settings().readWeights && mesh.DeformerSkin() != nullptr && doc.Settings().useSkeleton) {
        SkeletonBoneContainer sbc;
        ConvertWeightsToSkeleton(out_mesh, mesh, absolute_transform, parent, NO_MATERIAL_SEPARATION, nullptr, sbc);
        aiSkeleton *skeleton = createAiSkeleton(sbc);
        if (skeleton != nullptr) {
            mSkeletons.emplace_back(skeleton);
        }
    }

    std::vector<aiAnimMesh *> animMeshes;
    for (const BlendShape *blendShape : mesh.GetBlendShapes()) {
        for (const BlendShapeChannel *blendShapeChannel : blendShape->BlendShapeChannels()) {
            const auto& shapeGeometries = blendShapeChannel->GetShapeGeometries();
            for (const ShapeGeometry *shapeGeometry : shapeGeometries) {
                const auto &curNormals = shapeGeometry->GetNormals();
                aiAnimMesh *animMesh = aiCreateAnimMesh(out_mesh, true, !curNormals.empty());
                const auto &curVertices = shapeGeometry->GetVertices();
                const auto &curIndices = shapeGeometry->GetIndices();
                //losing channel name if using shapeGeometry->Name()
                // if blendShapeChannel Name is empty or doesn't have a ".", add geoMetryName;
                auto aniName = FixAnimMeshName(blendShapeChannel->Name());
                auto geoMetryName = FixAnimMeshName(shapeGeometry->Name());
                if (aniName.empty()) {
                    aniName = geoMetryName;
                }
                else if (aniName.find('.') == aniName.npos) {
                    aniName += "." + geoMetryName;
                }
                animMesh->mName.Set(aniName);
                for (size_t j = 0; j < curIndices.size(); j++) {
                    const unsigned int curIndex = curIndices.at(j);
                    aiVector3D vertex = curVertices.at(j);
                    aiVector3D normal = curNormals.empty() ? aiVector3D() : curNormals.at(j);
                    unsigned int count = 0;
                    const unsigned int *outIndices = mesh.ToOutputVertexIndex(curIndex, count);
                    for (unsigned int k = 0; k < count; k++) {
                        unsigned int index = outIndices[k];
                        animMesh->mVertices[index] += vertex;
                        if (animMesh->mNormals != nullptr) {
                            animMesh->mNormals[index] += normal;
                            animMesh->mNormals[index].NormalizeSafe();
                        }
                    }
                }
                animMesh->mWeight = shapeGeometries.size() > 1 ? blendShapeChannel->DeformPercent() / 100.0f : 1.0f;
                animMeshes.push_back(animMesh);
            }
        }
    }
    const size_t numAnimMeshes = animMeshes.size();
    if (numAnimMeshes > 0) {
        out_mesh->mNumAnimMeshes = static_cast<unsigned int>(numAnimMeshes);
        out_mesh->mAnimMeshes = new aiAnimMesh *[numAnimMeshes];
        for (size_t i = 0; i < numAnimMeshes; i++) {
            out_mesh->mAnimMeshes[i] = animMeshes.at(i);
        }
    }
    return static_cast<unsigned int>(mMeshes.size() - 1);
}

std::vector<unsigned int>
FBXConverter::ConvertMeshMultiMaterial(const MeshGeometry &mesh, const Model &model, const aiMatrix4x4 &absolute_transform, aiNode *parent,
        aiNode *root_node) {
    const MatIndexArray &mindices = mesh.GetMaterialIndices();
    ai_assert(mindices.size());

    std::set<MatIndexArray::value_type> had;
    std::vector<unsigned int> indices;

    for (MatIndexArray::value_type index : mindices) {
        if (had.find(index) == had.end()) {

            indices.push_back(ConvertMeshMultiMaterial(mesh, model, absolute_transform, index, parent, root_node));
            had.insert(index);
        }
    }

    return indices;
}

unsigned int FBXConverter::ConvertMeshMultiMaterial(const MeshGeometry &mesh, const Model &model, const aiMatrix4x4 &absolute_transform,
        MatIndexArray::value_type index, aiNode *parent, aiNode *) {
    aiMesh *const out_mesh = SetupEmptyMesh(mesh, parent);

    const MatIndexArray &mindices = mesh.GetMaterialIndices();
    const std::vector<aiVector3D> &vertices = mesh.GetVertices();
    const std::vector<unsigned int> &faces = mesh.GetFaceIndexCounts();

    const bool process_weights = doc.Settings().readWeights && mesh.DeformerSkin() != nullptr;

    unsigned int count_faces = 0;
    unsigned int count_vertices = 0;

    // count faces
    std::vector<unsigned int>::const_iterator itf = faces.begin();
    for (MatIndexArray::const_iterator it = mindices.begin(),
                                       end = mindices.end();
            it != end; ++it, ++itf) {
        if ((*it) != index) {
            continue;
        }
        ++count_faces;
        count_vertices += *itf;
    }

    ai_assert(count_faces);
    ai_assert(count_vertices);

    // mapping from output indices to DOM indexing, needed to resolve weights or blendshapes
    std::vector<unsigned int> reverseMapping;
    std::map<unsigned int, unsigned int> translateIndexMap;
    if (process_weights || mesh.GetBlendShapes().size() > 0) {
        reverseMapping.resize(count_vertices);
    }

    // allocate output data arrays, but don't fill them yet
    out_mesh->mNumVertices = count_vertices;
    out_mesh->mVertices = new aiVector3D[count_vertices];

    out_mesh->mNumFaces = count_faces;
    aiFace *fac = out_mesh->mFaces = new aiFace[count_faces]();

    // allocate normals
    const std::vector<aiVector3D> &normals = mesh.GetNormals();
    if (normals.size()) {
        ai_assert(normals.size() == vertices.size());
        out_mesh->mNormals = new aiVector3D[count_vertices];
    }

    // allocate tangents, binormals.
    const std::vector<aiVector3D> &tangents = mesh.GetTangents();
    const std::vector<aiVector3D> *binormals = &mesh.GetBinormals();
    std::vector<aiVector3D> tempBinormals;

    if (tangents.size()) {
        if (!binormals->size()) {
            if (normals.size()) {
                // XXX this computes the binormals for the entire mesh, not only
                // the part for which we need them.
                tempBinormals.resize(normals.size());
                for (unsigned int i = 0; i < tangents.size(); ++i) {
                    tempBinormals[i] = normals[i] ^ tangents[i];
                }

                binormals = &tempBinormals;
            } else {
                binormals = nullptr;
            }
        }

        if (binormals) {
            ai_assert(tangents.size() == vertices.size());
            ai_assert(binormals->size() == vertices.size());

            out_mesh->mTangents = new aiVector3D[count_vertices];
            out_mesh->mBitangents = new aiVector3D[count_vertices];
        }
    }

    // allocate texture coords
    unsigned int num_uvs = 0;
    for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i, ++num_uvs) {
        const std::vector<aiVector2D> &uvs = mesh.GetTextureCoords(i);
        if (uvs.empty()) {
            break;
        }

        out_mesh->mTextureCoords[i] = new aiVector3D[count_vertices];
        out_mesh->mNumUVComponents[i] = 2;
    }

    // allocate vertex colors
    unsigned int num_vcs = 0;
    for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i, ++num_vcs) {
        const std::vector<aiColor4D> &colors = mesh.GetVertexColors(i);
        if (colors.empty()) {
            break;
        }

        out_mesh->mColors[i] = new aiColor4D[count_vertices];
    }

    unsigned int cursor = 0, in_cursor = 0;

    itf = faces.begin();
    for (MatIndexArray::const_iterator it = mindices.begin(), end = mindices.end(); it != end; ++it, ++itf) {
        const unsigned int pcount = *itf;
        if ((*it) != index) {
            in_cursor += pcount;
            continue;
        }

        aiFace &f = *fac++;

        f.mNumIndices = pcount;
        f.mIndices = new unsigned int[pcount];
        switch (pcount) {
            case 1:
                out_mesh->mPrimitiveTypes |= aiPrimitiveType_POINT;
                break;
            case 2:
                out_mesh->mPrimitiveTypes |= aiPrimitiveType_LINE;
                break;
            case 3:
                out_mesh->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
                break;
            default:
                out_mesh->mPrimitiveTypes |= aiPrimitiveType_POLYGON;
                break;
        }
        for (unsigned int i = 0; i < pcount; ++i, ++cursor, ++in_cursor) {
            f.mIndices[i] = cursor;

            if (reverseMapping.size()) {
                reverseMapping[cursor] = in_cursor;
                translateIndexMap[in_cursor] = cursor;
            }

            out_mesh->mVertices[cursor] = vertices[in_cursor];

            if (out_mesh->mNormals) {
                out_mesh->mNormals[cursor] = normals[in_cursor];
            }

            if (out_mesh->mTangents) {
                out_mesh->mTangents[cursor] = tangents[in_cursor];
                out_mesh->mBitangents[cursor] = (*binormals)[in_cursor];
            }

            for (unsigned int j = 0; j < num_uvs; ++j) {
                const std::vector<aiVector2D> &uvs = mesh.GetTextureCoords(j);
                out_mesh->mTextureCoords[j][cursor] = aiVector3D(uvs[in_cursor].x, uvs[in_cursor].y, 0.0f);
            }

            for (unsigned int j = 0; j < num_vcs; ++j) {
                const std::vector<aiColor4D> &cols = mesh.GetVertexColors(j);
                out_mesh->mColors[j][cursor] = cols[in_cursor];
            }
        }
    }

    ConvertMaterialForMesh(out_mesh, model, mesh, index);

    if (process_weights) {
        ConvertWeights(out_mesh, mesh, absolute_transform, parent, index, &reverseMapping);
    }

    std::vector<aiAnimMesh *> animMeshes;
    for (const BlendShape *blendShape : mesh.GetBlendShapes()) {
        for (const BlendShapeChannel *blendShapeChannel : blendShape->BlendShapeChannels()) {
            const auto& shapeGeometries = blendShapeChannel->GetShapeGeometries();
            for (const ShapeGeometry *shapeGeometry : shapeGeometries) {
                const auto& curNormals = shapeGeometry->GetNormals();
                aiAnimMesh *animMesh = aiCreateAnimMesh(out_mesh, true, !curNormals.empty());
                const auto& curVertices = shapeGeometry->GetVertices();
                const auto& curIndices = shapeGeometry->GetIndices();
                animMesh->mName.Set(FixAnimMeshName(shapeGeometry->Name()));
                for (size_t j = 0; j < curIndices.size(); j++) {
                    unsigned int curIndex = curIndices.at(j);
                    aiVector3D vertex = curVertices.at(j);
                    aiVector3D normal = curNormals.empty() ? aiVector3D() : curNormals.at(j);
                    unsigned int count = 0;
                    const unsigned int *outIndices = mesh.ToOutputVertexIndex(curIndex, count);
                    for (unsigned int k = 0; k < count; k++) {
                        unsigned int outIndex = outIndices[k];
                        if (translateIndexMap.find(outIndex) == translateIndexMap.end())
                            continue;
                        unsigned int transIndex = translateIndexMap[outIndex];
                        animMesh->mVertices[transIndex] += vertex;
                        if (animMesh->mNormals != nullptr) {
                            animMesh->mNormals[transIndex] += normal;
                            animMesh->mNormals[transIndex].NormalizeSafe();
                        }
                    }
                }
                animMesh->mWeight = shapeGeometries.size() > 1 ? blendShapeChannel->DeformPercent() / 100.0f : 1.0f;
                animMeshes.push_back(animMesh);
            }
        }
    }

    const size_t numAnimMeshes = animMeshes.size();
    if (numAnimMeshes > 0) {
        out_mesh->mNumAnimMeshes = static_cast<unsigned int>(numAnimMeshes);
        out_mesh->mAnimMeshes = new aiAnimMesh *[numAnimMeshes];
        for (size_t i = 0; i < numAnimMeshes; i++) {
            out_mesh->mAnimMeshes[i] = animMeshes.at(i);
        }
    }

    return static_cast<unsigned int>(mMeshes.size() - 1);
}

static void copyBoneToSkeletonBone(aiMesh *mesh, aiBone *bone, aiSkeletonBone *skeletonBone ) {
    skeletonBone->mNumnWeights = bone->mNumWeights;
    skeletonBone->mWeights = bone->mWeights;
    skeletonBone->mOffsetMatrix = bone->mOffsetMatrix;
    skeletonBone->mMeshId = mesh;
#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
    skeletonBone->mNode = bone->mNode;
#endif
    skeletonBone->mParent = -1;
}

void FBXConverter::ConvertWeightsToSkeleton(aiMesh *out, const MeshGeometry &geo, const aiMatrix4x4 &absolute_transform, aiNode *parent, unsigned int materialIndex,
        std::vector<unsigned int> *outputVertStartIndices, SkeletonBoneContainer &skeletonContainer) {

    if (skeletonContainer.skeletonBoneToMeshLookup.find(out) != skeletonContainer.skeletonBoneToMeshLookup.end()) {
        return;
    }

    ConvertWeights(out, geo, absolute_transform, parent, materialIndex, outputVertStartIndices);
    skeletonContainer.meshArray.emplace_back(out);
    SkeletonBoneArray *ba = new SkeletonBoneArray;
    for (size_t i = 0; i < out->mNumBones; ++i) {
        aiBone *bone = out->mBones[i];
        if (bone == nullptr) {
            continue;
        }
        aiSkeletonBone *skeletonBone = new aiSkeletonBone;
        copyBoneToSkeletonBone(out, bone, skeletonBone);
        ba->emplace_back(skeletonBone);
    }
    skeletonContainer.skeletonBoneToMeshLookup[out] = ba;
}

void FBXConverter::ConvertWeights(aiMesh *out, const MeshGeometry &geo, const aiMatrix4x4 &absolute_transform,
        aiNode *parent, unsigned int materialIndex,
        std::vector<unsigned int> *outputVertStartIndices) {
    ai_assert(geo.DeformerSkin());

    const Skin &sk = *geo.DeformerSkin();

    std::vector<aiBone*> bones;
    const bool no_mat_check = materialIndex == NO_MATERIAL_SEPARATION;
    ai_assert(no_mat_check || outputVertStartIndices);

    try {
        std::vector<size_t> count_out_indices;
        std::vector<size_t> index_out_indices;
        std::vector<size_t> out_indices;

        // iterate over the sub deformers
        for (const Cluster *cluster : sk.Clusters()) {
            ai_assert(cluster);

            const WeightIndexArray &indices = cluster->GetIndices();

            const MatIndexArray &mats = geo.GetMaterialIndices();

            const size_t no_index_sentinel = std::numeric_limits<size_t>::max();

            count_out_indices.clear();
            index_out_indices.clear();
            out_indices.clear();

            // now check if *any* of these weights is contained in the output mesh,
            // taking notes so we don't need to do it twice.
            for (WeightIndexArray::value_type index : indices) {

                unsigned int count = 0;
                const unsigned int *const out_idx = geo.ToOutputVertexIndex(index, count);
                // ToOutputVertexIndex only returns nullptr if index is out of bounds
                // which should never happen
                ai_assert(out_idx != nullptr);

                index_out_indices.push_back(no_index_sentinel);
                count_out_indices.push_back(0);

                for (unsigned int i = 0; i < count; ++i) {
                    if (no_mat_check || static_cast<size_t>(mats[geo.FaceForVertexIndex(out_idx[i])]) == materialIndex) {

                        if (index_out_indices.back() == no_index_sentinel) {
                            index_out_indices.back() = out_indices.size();
                        }

                        if (no_mat_check) {
                            out_indices.push_back(out_idx[i]);
                        } else {
                            // this extra lookup is in O(logn), so the entire algorithm becomes O(nlogn)
                            const std::vector<unsigned int>::iterator it = std::lower_bound(
                                    outputVertStartIndices->begin(),
                                    outputVertStartIndices->end(),
                                    out_idx[i]);

                            out_indices.push_back(std::distance(outputVertStartIndices->begin(), it));
                        }

                        ++count_out_indices.back();
                    }
                }
            }

            // if we found at least one, generate the output bones
            // XXX this could be heavily simplified by collecting the bone
            // data in a single step.
            ConvertCluster(bones, cluster, out_indices, index_out_indices,
                    count_out_indices, absolute_transform, parent);
        }

        bone_map.clear();
    } catch (std::exception &) {
        std::for_each(bones.begin(), bones.end(), Util::delete_fun<aiBone>());
        throw;
    }

    if (bones.empty()) {
        out->mBones = nullptr;
        out->mNumBones = 0;
        return;
    }

    out->mBones = new aiBone *[bones.size()]();
    out->mNumBones = static_cast<unsigned int>(bones.size());
    std::swap_ranges(bones.begin(), bones.end(), out->mBones);
}

void FBXConverter::ConvertCluster(std::vector<aiBone*> &local_mesh_bones, const Cluster *cluster,
        std::vector<size_t> &out_indices, std::vector<size_t> &index_out_indices,
        std::vector<size_t> &count_out_indices, const aiMatrix4x4 &absolute_transform,
        aiNode *) {
    ai_assert(cluster != nullptr); // make sure cluster valid

    std::string deformer_name = cluster->TargetNode()->Name();
    aiString bone_name = aiString(FixNodeName(deformer_name));

    aiBone *bone = nullptr;

    if (bone_map.count(deformer_name)) {
        ASSIMP_LOG_VERBOSE_DEBUG("retrieved bone from lookup ", bone_name.C_Str(), ". Deformer:", deformer_name);
        bone = bone_map[deformer_name];
    } else {
        ASSIMP_LOG_VERBOSE_DEBUG("created new bone ", bone_name.C_Str(), ". Deformer: ", deformer_name);
        bone = new aiBone();
        bone->mName = bone_name;

        //bone->mOffsetMatrix = cluster->Transform();
        // store local transform link for post processing

        bone->mOffsetMatrix = cluster->TransformLink();
        bone->mOffsetMatrix.Inverse();

        const aiMatrix4x4 matrix = (aiMatrix4x4)absolute_transform;

        bone->mOffsetMatrix = bone->mOffsetMatrix * matrix; // * mesh_offset

        //
        // Now calculate the aiVertexWeights
        //

        aiVertexWeight *cursor = nullptr;

        bone->mNumWeights = static_cast<unsigned int>(out_indices.size());
        cursor = bone->mWeights = new aiVertexWeight[out_indices.size()];

        const size_t no_index_sentinel = std::numeric_limits<size_t>::max();
        const WeightArray &weights = cluster->GetWeights();

        const size_t c = index_out_indices.size();
        for (size_t i = 0; i < c; ++i) {
            const size_t index_index = index_out_indices[i];

            if (index_index == no_index_sentinel) {
                continue;
            }

            const size_t cc = count_out_indices[i];
            for (size_t j = 0; j < cc; ++j) {
                // cursor runs from first element relative to the start
                // or relative to the start of the next indexes.
                aiVertexWeight &out_weight = *cursor++;

                out_weight.mVertexId = static_cast<unsigned int>(out_indices[index_index + j]);
                out_weight.mWeight = weights[i];
            }
        }

        bone_map.insert(std::pair<const std::string, aiBone *>(deformer_name, bone));
    }

    ASSIMP_LOG_DEBUG("bone research: Indices size: ", out_indices.size());

    // lookup must be populated in case something goes wrong
    // this also allocates bones to mesh instance outside
    local_mesh_bones.push_back(bone);
}

void FBXConverter::ConvertMaterialForMesh(aiMesh *out, const Model &model, const MeshGeometry &geo,
        MatIndexArray::value_type materialIndex) {
    // locate source materials for this mesh
    const std::vector<const Material *> &mats = model.GetMaterials();
    if (static_cast<unsigned int>(materialIndex) >= mats.size() || materialIndex < 0) {
        FBXImporter::LogError("material index out of bounds, setting default material");
        out->mMaterialIndex = GetDefaultMaterial();
        return;
    }

    const Material *const mat = mats[materialIndex];
    MaterialMap::const_iterator it = materials_converted.find(mat);
    if (it != materials_converted.end()) {
        out->mMaterialIndex = (*it).second;
        return;
    }

    out->mMaterialIndex = ConvertMaterial(*mat, &geo);
    materials_converted[mat] = out->mMaterialIndex;
}

unsigned int FBXConverter::GetDefaultMaterial() {
    if (defaultMaterialIndex) {
        return defaultMaterialIndex - 1;
    }

    auto out_mat = new aiMaterial();
    materials.push_back(out_mat);

    const auto diffuse = aiColor3D(0.8f, 0.8f, 0.8f);
    out_mat->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);

    aiString s;
    s.Set(AI_DEFAULT_MATERIAL_NAME);

    out_mat->AddProperty(&s, AI_MATKEY_NAME);

    defaultMaterialIndex = static_cast<unsigned int>(materials.size());
    return defaultMaterialIndex - 1;
}

unsigned int FBXConverter::ConvertMaterial(const Material &material, const MeshGeometry *const mesh) {
    const PropertyTable &props = material.Props();

    // generate empty output material
    auto out_mat = new aiMaterial();
    materials_converted[&material] = static_cast<unsigned int>(materials.size());

    materials.push_back(out_mat);

    aiString str;

    // strip Material:: prefix
    std::string name = material.Name();
    if (name.substr(0, 10) == "Material::") {
        name = name.substr(10);
    }

    // set material name if not empty - this could happen
    // and there should be no key for it in this case.
    if (name.length()) {
        str.Set(name);
        out_mat->AddProperty(&str, AI_MATKEY_NAME);
    }

    // Set the shading mode as best we can: The FBX specification only mentions Lambert and Phong, and only Phong is mentioned in Assimp's aiShadingMode enum.
    if (material.GetShadingModel() == "phong") {
        aiShadingMode shadingMode = aiShadingMode_Phong;
        out_mat->AddProperty<aiShadingMode>(&shadingMode, 1, AI_MATKEY_SHADING_MODEL);
    }

    // shading stuff and colors
    SetShadingPropertiesCommon(out_mat, props);
    SetShadingPropertiesRaw(out_mat, props, material.Textures(), mesh);

    // texture assignments
    SetTextureProperties(out_mat, material.Textures(), mesh);
    SetTextureProperties(out_mat, material.LayeredTextures(), mesh);

    return static_cast<unsigned int>(materials.size() - 1);
}

unsigned int FBXConverter::ConvertVideo(const Video &video) {
    // generate empty output texture
    auto out_tex = new aiTexture();
    textures.push_back(out_tex);

    // assuming the texture is compressed
    out_tex->mWidth = static_cast<unsigned int>(video.ContentLength()); // total data size
    out_tex->mHeight = 0; // fixed to 0

    // steal the data from the Video to avoid an additional copy
    out_tex->pcData = reinterpret_cast<aiTexel *>(const_cast<Video &>(video).RelinquishContent());

    // try to extract a hint from the file extension
    const std::string &filename = video.RelativeFilename().empty() ? video.FileName() : video.RelativeFilename();
    std::string ext = BaseImporter::GetExtension(filename);

    if (ext == "jpeg") {
        ext = "jpg";
    }

    if (ext.size() <= 3) {
        memcpy(out_tex->achFormatHint, ext.c_str(), ext.size());
    }

    out_tex->mFilename.Set(filename.c_str());

    return static_cast<unsigned int>(textures.size() - 1);
}

aiString FBXConverter::GetTexturePath(const Texture *tex) {
    aiString path;
    path.Set(tex->RelativeFilename());

    const Video *media = tex->Media();
    if (media != nullptr) {
        bool textureReady = false; //tells if our texture is ready (if it was loaded or if it was found)
        unsigned int index=0;

        VideoMap::const_iterator it = textures_converted.find(media);
        if (it != textures_converted.end()) {
            index = (*it).second;
            textureReady = true;
        } else {
            if (media->ContentLength() > 0) {
                index = ConvertVideo(*media);
                textures_converted[media] = index;
                textureReady = true;
            }
        }

        // setup texture reference string (copied from ColladaLoader::FindFilenameForEffectTexture), if the texture is ready
        if (doc.Settings().useLegacyEmbeddedTextureNaming) {
            if (textureReady) {
                // TODO: check the possibility of using the flag "AI_CONFIG_IMPORT_FBX_EMBEDDED_TEXTURES_LEGACY_NAMING"
                // In FBX files textures are now stored internally by Assimp with their filename included
                // Now Assimp can lookup through the loaded textures after all data is processed
                // We need to load all textures before referencing them, as FBX file format order may reference a texture before loading it
                // This may occur on this case too, it has to be studied
                path.data[0] = '*';
                path.length = 1 + ASSIMP_itoa10(path.data + 1, AI_MAXLEN - 1, index);
            }
        }
    }

    return path;
}

void FBXConverter::TrySetTextureProperties(aiMaterial *out_mat, const TextureMap &_textures,
        const std::string &propName,
        aiTextureType target, const MeshGeometry *const mesh) {
    TextureMap::const_iterator it = _textures.find(propName);
    if (it == _textures.end()) {
        return;
    }

    const Texture *const tex = (*it).second;
    if (tex != nullptr) {
        aiString path = GetTexturePath(tex);
        out_mat->AddProperty(&path, _AI_MATKEY_TEXTURE_BASE, target, 0);

        aiUVTransform uvTrafo;
        // XXX handle all kinds of UV transformations
        uvTrafo.mScaling = tex->UVScaling();
        uvTrafo.mTranslation = tex->UVTranslation();
        uvTrafo.mRotation = tex->UVRotation();
        out_mat->AddProperty(&uvTrafo, 1, _AI_MATKEY_UVTRANSFORM_BASE, target, 0);

        const PropertyTable &props = tex->Props();

        int uvIndex = 0;

        bool ok;
        const std::string &uvSet = PropertyGet<std::string>(props, "UVSet", ok);
        if (ok) {
            // "default" is the name which usually appears in the FbxFileTexture template
            if (uvSet != "default" && uvSet.length()) {
                // this is a bit awkward - we need to find a mesh that uses this
                // material and scan its UV channels for the given UV name because
                // assimp references UV channels by index, not by name.

                // XXX: the case that UV channels may appear in different orders
                // in meshes is unhandled. A possible solution would be to sort
                // the UV channels alphabetically, but this would have the side
                // effect that the primary (first) UV channel would sometimes
                // be moved, causing trouble when users read only the first
                // UV channel and ignore UV channel assignments altogether.

                const unsigned int matIndex = static_cast<unsigned int>(std::distance(materials.begin(),
                        std::find(materials.begin(), materials.end(), out_mat)));

                uvIndex = -1;
                if (!mesh) {
                    for (const MeshMap::value_type &v : meshes_converted) {
                        const MeshGeometry *const meshGeom = dynamic_cast<const MeshGeometry *>(v.first);
                        if (!meshGeom) {
                            continue;
                        }

                        const MatIndexArray &mats = meshGeom->GetMaterialIndices();
                        MatIndexArray::const_iterator curIt = std::find(mats.begin(), mats.end(), (int) matIndex);
                        if (curIt == mats.end()) {
                            continue;
                        }

                        int index = -1;
                        for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                            if (meshGeom->GetTextureCoords(i).empty()) {
                                break;
                            }
                            const std::string &name = meshGeom->GetTextureCoordChannelName(i);
                            if (name == uvSet) {
                                index = static_cast<int>(i);
                                break;
                            }
                        }
                        if (index == -1) {
                            FBXImporter::LogWarn("did not find UV channel named ", uvSet, " in a mesh using this material");
                            continue;
                        }

                        if (uvIndex == -1) {
                            uvIndex = index;
                        } else {
                            FBXImporter::LogWarn("the UV channel named ", uvSet,
                                                 " appears at different positions in meshes, results will be wrong");
                        }
                    }
                } else {
                    int index = -1;
                    for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                        if (mesh->GetTextureCoords(i).empty()) {
                            break;
                        }
                        const std::string &name = mesh->GetTextureCoordChannelName(i);
                        if (name == uvSet) {
                            index = static_cast<int>(i);
                            break;
                        }
                    }
                    if (index == -1) {
                        FBXImporter::LogWarn("did not find UV channel named ", uvSet, " in a mesh using this material");
                    }

                    if (uvIndex == -1) {
                        uvIndex = index;
                    }
                }

                if (uvIndex == -1) {
                    FBXImporter::LogWarn("failed to resolve UV channel ", uvSet, ", using first UV channel");
                    uvIndex = 0;
                }
            }
        }

        out_mat->AddProperty(&uvIndex, 1, _AI_MATKEY_UVWSRC_BASE, target, 0);
    }
}

void FBXConverter::TrySetTextureProperties(aiMaterial *out_mat, const LayeredTextureMap &layeredTextures,
        const std::string &propName,
        aiTextureType target, const MeshGeometry *const mesh) {
    LayeredTextureMap::const_iterator it = layeredTextures.find(propName);
    if (it == layeredTextures.end()) {
        return;
    }

    int texCount = (*it).second->textureCount();

    // Set the blend mode for layered textures
    int blendmode = (*it).second->GetBlendMode();
    out_mat->AddProperty(&blendmode, 1, _AI_MATKEY_TEXOP_BASE, target, 0);

    for (int texIndex = 0; texIndex < texCount; texIndex++) {

        const Texture *const tex = (*it).second->getTexture(texIndex);

        aiString path = GetTexturePath(tex);
        out_mat->AddProperty(&path, _AI_MATKEY_TEXTURE_BASE, target, texIndex);

        aiUVTransform uvTrafo;
        // XXX handle all kinds of UV transformations
        uvTrafo.mScaling = tex->UVScaling();
        uvTrafo.mTranslation = tex->UVTranslation();
        uvTrafo.mRotation = tex->UVRotation();
        out_mat->AddProperty(&uvTrafo, 1, _AI_MATKEY_UVTRANSFORM_BASE, target, texIndex);

        const PropertyTable &props = tex->Props();

        int uvIndex = 0;

        bool ok;
        const std::string &uvSet = PropertyGet<std::string>(props, "UVSet", ok);
        if (ok) {
            // "default" is the name which usually appears in the FbxFileTexture template
            if (uvSet != "default" && uvSet.length()) {
                // this is a bit awkward - we need to find a mesh that uses this
                // material and scan its UV channels for the given UV name because
                // assimp references UV channels by index, not by name.

                // XXX: the case that UV channels may appear in different orders
                // in meshes is unhandled. A possible solution would be to sort
                // the UV channels alphabetically, but this would have the side
                // effect that the primary (first) UV channel would sometimes
                // be moved, causing trouble when users read only the first
                // UV channel and ignore UV channel assignments altogether.

                const unsigned int matIndex = static_cast<unsigned int>(std::distance(materials.begin(),
                        std::find(materials.begin(), materials.end(), out_mat)));

                uvIndex = -1;
                if (!mesh) {
                    for (const MeshMap::value_type &v : meshes_converted) {
                        const MeshGeometry *const meshGeom = dynamic_cast<const MeshGeometry *>(v.first);
                        if (!meshGeom) {
                            continue;
                        }

                        const MatIndexArray &mats = meshGeom->GetMaterialIndices();
                        MatIndexArray::const_iterator curIt = std::find(mats.begin(), mats.end(), (int) matIndex);
                        if ( curIt == mats.end()) {
                            continue;
                        }

                        int index = -1;
                        for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                            if (meshGeom->GetTextureCoords(i).empty()) {
                                break;
                            }
                            const std::string &name = meshGeom->GetTextureCoordChannelName(i);
                            if (name == uvSet) {
                                index = static_cast<int>(i);
                                break;
                            }
                        }
                        if (index == -1) {
                            FBXImporter::LogWarn("did not find UV channel named ", uvSet, " in a mesh using this material");
                            continue;
                        }

                        if (uvIndex == -1) {
                            uvIndex = index;
                        } else {
                            FBXImporter::LogWarn("the UV channel named ", uvSet,
                                                 " appears at different positions in meshes, results will be wrong");
                        }
                    }
                } else {
                    int index = -1;
                    for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                        if (mesh->GetTextureCoords(i).empty()) {
                            break;
                        }
                        const std::string &name = mesh->GetTextureCoordChannelName(i);
                        if (name == uvSet) {
                            index = static_cast<int>(i);
                            break;
                        }
                    }
                    if (index == -1) {
                        FBXImporter::LogWarn("did not find UV channel named ", uvSet, " in a mesh using this material");
                    }

                    if (uvIndex == -1) {
                        uvIndex = index;
                    }
                }

                if (uvIndex == -1) {
                    FBXImporter::LogWarn("failed to resolve UV channel ", uvSet, ", using first UV channel");
                    uvIndex = 0;
                }
            }
        }

        out_mat->AddProperty(&uvIndex, 1, _AI_MATKEY_UVWSRC_BASE, target, texIndex);
    }
}

void FBXConverter::SetTextureProperties(aiMaterial *out_mat, const TextureMap &_textures, const MeshGeometry *const mesh) {
    TrySetTextureProperties(out_mat, _textures, "DiffuseColor", aiTextureType_DIFFUSE, mesh);
    TrySetTextureProperties(out_mat, _textures, "AmbientColor", aiTextureType_AMBIENT, mesh);
    TrySetTextureProperties(out_mat, _textures, "EmissiveColor", aiTextureType_EMISSIVE, mesh);
    TrySetTextureProperties(out_mat, _textures, "SpecularColor", aiTextureType_SPECULAR, mesh);
    TrySetTextureProperties(out_mat, _textures, "SpecularFactor", aiTextureType_SPECULAR, mesh);
    TrySetTextureProperties(out_mat, _textures, "TransparentColor", aiTextureType_OPACITY, mesh);
    TrySetTextureProperties(out_mat, _textures, "ReflectionColor", aiTextureType_REFLECTION, mesh);
    TrySetTextureProperties(out_mat, _textures, "DisplacementColor", aiTextureType_DISPLACEMENT, mesh);
    TrySetTextureProperties(out_mat, _textures, "NormalMap", aiTextureType_NORMALS, mesh);
    TrySetTextureProperties(out_mat, _textures, "Bump", aiTextureType_HEIGHT, mesh);
    TrySetTextureProperties(out_mat, _textures, "ShininessExponent", aiTextureType_SHININESS, mesh);
    TrySetTextureProperties(out_mat, _textures, "TransparencyFactor", aiTextureType_OPACITY, mesh);
    TrySetTextureProperties(out_mat, _textures, "EmissiveFactor", aiTextureType_EMISSIVE, mesh);
    TrySetTextureProperties(out_mat, _textures, "ReflectionFactor", aiTextureType_METALNESS, mesh);
    //Maya counterparts
    TrySetTextureProperties(out_mat, _textures, "Maya|DiffuseTexture", aiTextureType_DIFFUSE, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|NormalTexture", aiTextureType_NORMALS, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|SpecularTexture", aiTextureType_SPECULAR, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|FalloffTexture", aiTextureType_OPACITY, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|ReflectionMapTexture", aiTextureType_REFLECTION, mesh);

    // Maya PBR
    TrySetTextureProperties(out_mat, _textures, "Maya|baseColor", aiTextureType_BASE_COLOR, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|normalCamera", aiTextureType_NORMAL_CAMERA, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|emissionColor", aiTextureType_EMISSION_COLOR, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|metalness", aiTextureType_METALNESS, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|diffuseRoughness", aiTextureType_DIFFUSE_ROUGHNESS, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|base", aiTextureType_MAYA_BASE, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|specular", aiTextureType_MAYA_SPECULAR, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|specularColor", aiTextureType_MAYA_SPECULAR_COLOR, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|specularRoughness", aiTextureType_MAYA_SPECULAR_ROUGHNESS, mesh);

    // Maya stingray
    TrySetTextureProperties(out_mat, _textures, "Maya|TEX_color_map", aiTextureType_BASE_COLOR, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|TEX_normal_map", aiTextureType_NORMAL_CAMERA, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|TEX_emissive_map", aiTextureType_EMISSION_COLOR, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|TEX_metallic_map", aiTextureType_METALNESS, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|TEX_roughness_map", aiTextureType_DIFFUSE_ROUGHNESS, mesh);
    TrySetTextureProperties(out_mat, _textures, "Maya|TEX_ao_map", aiTextureType_AMBIENT_OCCLUSION, mesh);

    // 3DSMax Physical material
    TrySetTextureProperties(out_mat, _textures, "3dsMax|Parameters|base_color_map", aiTextureType_BASE_COLOR, mesh);
    TrySetTextureProperties(out_mat, _textures, "3dsMax|Parameters|bump_map", aiTextureType_NORMAL_CAMERA, mesh);
    TrySetTextureProperties(out_mat, _textures, "3dsMax|Parameters|emission_map", aiTextureType_EMISSION_COLOR, mesh);
    TrySetTextureProperties(out_mat, _textures, "3dsMax|Parameters|metalness_map", aiTextureType_METALNESS, mesh);
    TrySetTextureProperties(out_mat, _textures, "3dsMax|Parameters|roughness_map", aiTextureType_DIFFUSE_ROUGHNESS, mesh);

    // 3DSMax PBR materials
    TrySetTextureProperties(out_mat, _textures, "3dsMax|main|base_color_map", aiTextureType_BASE_COLOR, mesh);
    TrySetTextureProperties(out_mat, _textures, "3dsMax|main|norm_map", aiTextureType_NORMAL_CAMERA, mesh);
    TrySetTextureProperties(out_mat, _textures, "3dsMax|main|emit_color_map", aiTextureType_EMISSION_COLOR, mesh);
    TrySetTextureProperties(out_mat, _textures, "3dsMax|main|ao_map", aiTextureType_AMBIENT_OCCLUSION, mesh);
    TrySetTextureProperties(out_mat, _textures, "3dsMax|main|opacity_map", aiTextureType_OPACITY, mesh);
    // Metalness/Roughness material type
    TrySetTextureProperties(out_mat, _textures, "3dsMax|main|metalness_map", aiTextureType_METALNESS, mesh);
    // Specular/Gloss material type
    TrySetTextureProperties(out_mat, _textures, "3dsMax|main|specular_map", aiTextureType_SPECULAR, mesh);

    // Glossiness vs roughness in 3ds Max Pbr Materials
    int useGlossiness;
    if (out_mat->Get("$raw.3dsMax|main|useGlossiness", aiTextureType_NONE, 0, useGlossiness) == aiReturn_SUCCESS) {
        // These textures swap meaning if ((useGlossiness == 1) != (material type is Specular/Gloss))
        if (useGlossiness == 1) {
            TrySetTextureProperties(out_mat, _textures, "3dsMax|main|roughness_map", aiTextureType_SHININESS, mesh);
            TrySetTextureProperties(out_mat, _textures, "3dsMax|main|glossiness_map", aiTextureType_SHININESS, mesh);
        }
        else if (useGlossiness == 2) {
            TrySetTextureProperties(out_mat, _textures, "3dsMax|main|roughness_map", aiTextureType_DIFFUSE_ROUGHNESS, mesh);
            TrySetTextureProperties(out_mat, _textures, "3dsMax|main|glossiness_map", aiTextureType_DIFFUSE_ROUGHNESS, mesh);
        }
        else {
            FBXImporter::LogWarn("A 3dsMax Pbr Material must have a useGlossiness value to correctly interpret roughness and glossiness textures.");
        }
    }
}

void FBXConverter::SetTextureProperties(aiMaterial *out_mat, const LayeredTextureMap &layeredTextures, const MeshGeometry *const mesh) {
    TrySetTextureProperties(out_mat, layeredTextures, "DiffuseColor", aiTextureType_DIFFUSE, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "AmbientColor", aiTextureType_AMBIENT, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "EmissiveColor", aiTextureType_EMISSIVE, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "SpecularColor", aiTextureType_SPECULAR, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "SpecularFactor", aiTextureType_SPECULAR, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "TransparentColor", aiTextureType_OPACITY, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "ReflectionColor", aiTextureType_REFLECTION, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "DisplacementColor", aiTextureType_DISPLACEMENT, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "NormalMap", aiTextureType_NORMALS, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "Bump", aiTextureType_HEIGHT, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "ShininessExponent", aiTextureType_SHININESS, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "EmissiveFactor", aiTextureType_EMISSIVE, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "TransparencyFactor", aiTextureType_OPACITY, mesh);
    TrySetTextureProperties(out_mat, layeredTextures, "ReflectionFactor", aiTextureType_METALNESS, mesh);
}

aiColor3D FBXConverter::GetColorPropertyFactored(const PropertyTable &props, const std::string &colorName,
        const std::string &factorName, bool &result, bool useTemplate) {
    result = true;

    bool ok;
    aiVector3D BaseColor = PropertyGet<aiVector3D>(props, colorName, ok, useTemplate);
    if (!ok) {
        result = false;
        return aiColor3D(0.0f, 0.0f, 0.0f);
    }

    // if no factor name, return the colour as is
    if (factorName.empty()) {
        return aiColor3D(BaseColor.x, BaseColor.y, BaseColor.z);
    }

    // otherwise it should be multiplied by the factor, if found.
    float factor = PropertyGet<float>(props, factorName, ok, useTemplate);
    if (ok) {
        BaseColor *= factor;
    }
    return aiColor3D(BaseColor.x, BaseColor.y, BaseColor.z);
}

aiColor3D FBXConverter::GetColorPropertyFromMaterial(const PropertyTable &props, const std::string &baseName,
        bool &result) {
    return GetColorPropertyFactored(props, baseName + "Color", baseName + "Factor", result, true);
}

aiColor3D FBXConverter::GetColorProperty(const PropertyTable &props, const std::string &colorName,
        bool &result, bool useTemplate) {
    result = true;
    bool ok;
    const aiVector3D &ColorVec = PropertyGet<aiVector3D>(props, colorName, ok, useTemplate);
    if (!ok) {
        result = false;
        return aiColor3D(0.0f, 0.0f, 0.0f);
    }
    return aiColor3D(ColorVec.x, ColorVec.y, ColorVec.z);
}

void FBXConverter::SetShadingPropertiesCommon(aiMaterial *out_mat, const PropertyTable &props) {
    // Set shading properties.
    // Modern FBX Files have two separate systems for defining these,
    // with only the more comprehensive one described in the property template.
    // Likely the other values are a legacy system,
    // which is still always exported by the official FBX SDK.
    //
    // Blender's FBX import and export mostly ignore this legacy system,
    // and as we only support recent versions of FBX anyway, we can do the same.
    bool ok;

    const aiColor3D &Diffuse = GetColorPropertyFromMaterial(props, "Diffuse", ok);
    if (ok) {
        out_mat->AddProperty(&Diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
    }

    const aiColor3D &Emissive = GetColorPropertyFromMaterial(props, "Emissive", ok);
    if (ok) {
        out_mat->AddProperty(&Emissive, 1, AI_MATKEY_COLOR_EMISSIVE);
    } else {
        const aiColor3D &emissiveColor = GetColorProperty(props, "Maya|emissive", ok);
        if (ok) {
            out_mat->AddProperty(&emissiveColor, 1, AI_MATKEY_COLOR_EMISSIVE);
        }
     }

    const aiColor3D &Ambient = GetColorPropertyFromMaterial(props, "Ambient", ok);
    if (ok) {
        out_mat->AddProperty(&Ambient, 1, AI_MATKEY_COLOR_AMBIENT);
    }

    // we store specular factor as SHININESS_STRENGTH, so just get the color
    const aiColor3D &Specular = GetColorProperty(props, "SpecularColor", ok, true);
    if (ok) {
        out_mat->AddProperty(&Specular, 1, AI_MATKEY_COLOR_SPECULAR);
    }

    // and also try to get SHININESS_STRENGTH
    const float SpecularFactor = PropertyGet<float>(props, "SpecularFactor", ok, true);
    if (ok) {
        out_mat->AddProperty(&SpecularFactor, 1, AI_MATKEY_SHININESS_STRENGTH);
    }

    // and the specular exponent
    const float ShininessExponent = PropertyGet<float>(props, "ShininessExponent", ok);
    if (ok) {
        out_mat->AddProperty(&ShininessExponent, 1, AI_MATKEY_SHININESS);
         // Match Blender behavior to extract roughness when only shininess is present
        const float roughness = 1.0f - (sqrt(ShininessExponent) / 10.0f);
        out_mat->AddProperty(&roughness, 1, AI_MATKEY_ROUGHNESS_FACTOR);
    }

    // TransparentColor / TransparencyFactor... gee thanks FBX :rolleyes:
    const aiColor3D &Transparent = GetColorPropertyFactored(props, "TransparentColor", "TransparencyFactor", ok);
    float CalculatedOpacity = 1.0f;
    if (ok) {
        out_mat->AddProperty(&Transparent, 1, AI_MATKEY_COLOR_TRANSPARENT);
        // as calculated by FBX SDK 2017:
        CalculatedOpacity = 1.0f - ((Transparent.r + Transparent.g + Transparent.b) / 3.0f);
    }

    // try to get the transparency factor
    const float TransparencyFactor = PropertyGet<float>(props, "TransparencyFactor", ok);
    if (ok) {
        out_mat->AddProperty(&TransparencyFactor, 1, AI_MATKEY_TRANSPARENCYFACTOR);
    }

    // use of TransparencyFactor is inconsistent.
    // Maya always stores it as 1.0,
    // so we can't use it to set AI_MATKEY_OPACITY.
    // Blender is more sensible and stores it as the alpha value.
    // However both the FBX SDK and Blender always write an additional
    // legacy "Opacity" field, so we can try to use that.
    //
    // If we can't find it,
    // we can fall back to the value which the FBX SDK calculates
    // from transparency colour (RGB) and factor (F) as
    // 1.0 - F*((R+G+B)/3).
    //
    // There's no consistent way to interpret this opacity value,
    // so it's up to clients to do the correct thing.
    const float Opacity = PropertyGet<float>(props, "Opacity", ok);
    if (ok) {
        out_mat->AddProperty(&Opacity, 1, AI_MATKEY_OPACITY);
    } else if (CalculatedOpacity != 1.0) {
        out_mat->AddProperty(&CalculatedOpacity, 1, AI_MATKEY_OPACITY);
    }

    // reflection color and factor are stored separately
    const aiColor3D &Reflection = GetColorProperty(props, "ReflectionColor", ok, true);
    if (ok) {
        out_mat->AddProperty(&Reflection, 1, AI_MATKEY_COLOR_REFLECTIVE);
    }

    float ReflectionFactor = PropertyGet<float>(props, "ReflectionFactor", ok, true);
    if (ok) {
        out_mat->AddProperty(&ReflectionFactor, 1, AI_MATKEY_REFLECTIVITY);
    }

    const float BumpFactor = PropertyGet<float>(props, "BumpFactor", ok);
    if (ok) {
        out_mat->AddProperty(&BumpFactor, 1, AI_MATKEY_BUMPSCALING);
    }

    const float DispFactor = PropertyGet<float>(props, "DisplacementFactor", ok);
    if (ok) {
        out_mat->AddProperty(&DispFactor, 1, "$mat.displacementscaling", 0, 0);
    }

    // PBR material information
    const aiColor3D &baseColor = GetColorProperty(props, "Maya|base_color", ok);
    if (ok) {
        out_mat->AddProperty(&baseColor, 1, AI_MATKEY_BASE_COLOR);
    }

    const float useColorMap = PropertyGet<float>(props, "Maya|use_color_map", ok);
    if (ok) {
        out_mat->AddProperty(&useColorMap, 1, AI_MATKEY_USE_COLOR_MAP);
    }

    const float useMetallicMap = PropertyGet<float>(props, "Maya|use_metallic_map", ok);
    if (ok) {
        out_mat->AddProperty(&useMetallicMap, 1, AI_MATKEY_USE_METALLIC_MAP);
    }

    const float metallicFactor = PropertyGet<float>(props, "Maya|metallic", ok);
    if (ok) {
        out_mat->AddProperty(&metallicFactor, 1, AI_MATKEY_METALLIC_FACTOR);
    }

    const float useRoughnessMap = PropertyGet<float>(props, "Maya|use_roughness_map", ok);
    if (ok) {
        out_mat->AddProperty(&useRoughnessMap, 1, AI_MATKEY_USE_ROUGHNESS_MAP);
    }

    const float roughnessFactor = PropertyGet<float>(props, "Maya|roughness", ok);
    if (ok) {
        out_mat->AddProperty(&roughnessFactor, 1, AI_MATKEY_ROUGHNESS_FACTOR);
    }

    const float useEmissiveMap = PropertyGet<float>(props, "Maya|use_emissive_map", ok);
    if (ok) {
        out_mat->AddProperty(&useEmissiveMap, 1, AI_MATKEY_USE_EMISSIVE_MAP);
    }

    const float emissiveIntensity = PropertyGet<float>(props, "Maya|emissive_intensity", ok);
    if (ok) {
        out_mat->AddProperty(&emissiveIntensity, 1, AI_MATKEY_EMISSIVE_INTENSITY);
    }

    const float useAOMap = PropertyGet<float>(props, "Maya|use_ao_map", ok);
    if (ok) {
        out_mat->AddProperty(&useAOMap, 1, AI_MATKEY_USE_AO_MAP);
    }
}

void FBXConverter::SetShadingPropertiesRaw(aiMaterial *out_mat, const PropertyTable &props, const TextureMap &_textures, const MeshGeometry *const mesh) {
    // Add all the unparsed properties with a "$raw." prefix

    const std::string prefix = "$raw.";

    for (const DirectPropertyMap::value_type &prop : props.GetUnparsedProperties()) {

        std::string name = prefix + prop.first;

        if (const TypedProperty<aiVector3D> *interpretedVec3 = prop.second->As<TypedProperty<aiVector3D>>()) {
            out_mat->AddProperty(&interpretedVec3->Value(), 1, name.c_str(), 0, 0);
        } else if (const TypedProperty<aiColor3D> *interpretedCol3 = prop.second->As<TypedProperty<aiColor3D>>()) {
            out_mat->AddProperty(&interpretedCol3->Value(), 1, name.c_str(), 0, 0);
        } else if (const TypedProperty<aiColor4D> *interpretedCol4 = prop.second->As<TypedProperty<aiColor4D>>()) {
            out_mat->AddProperty(&interpretedCol4->Value(), 1, name.c_str(), 0, 0);
        } else if (const TypedProperty<float> *interpretedFloat = prop.second->As<TypedProperty<float>>()) {
            out_mat->AddProperty(&interpretedFloat->Value(), 1, name.c_str(), 0, 0);
        } else if (const TypedProperty<int> *interpretedInt = prop.second->As<TypedProperty<int>>()) {
            out_mat->AddProperty(&interpretedInt->Value(), 1, name.c_str(), 0, 0);
        } else if (const TypedProperty<bool> *interpretedBool = prop.second->As<TypedProperty<bool>>()) {
            int value = interpretedBool->Value() ? 1 : 0;
            out_mat->AddProperty(&value, 1, name.c_str(), 0, 0);
        } else if (const TypedProperty<std::string> *interpretedString = prop.second->As<TypedProperty<std::string>>()) {
            const aiString value = aiString(interpretedString->Value());
            out_mat->AddProperty(&value, name.c_str(), 0, 0);
        }
    }

    // Add the textures' properties

    for (TextureMap::const_iterator it = _textures.begin(); it != _textures.end(); ++it) {

        std::string name = prefix + it->first;

        const Texture *const tex = it->second;
        if (tex != nullptr) {
            aiString path;
            path.Set(tex->RelativeFilename());

            const Video *media = tex->Media();
            if (media != nullptr && media->ContentLength() > 0) {
                unsigned int index;

                VideoMap::const_iterator videoIt = textures_converted.find(media);
                if (videoIt != textures_converted.end()) {
                    index = videoIt->second;
                } else {
                    index = ConvertVideo(*media);
                    textures_converted[media] = index;
                }

                // setup texture reference string (copied from ColladaLoader::FindFilenameForEffectTexture)
                path.data[0] = '*';
                path.length = 1 + ASSIMP_itoa10(path.data + 1, AI_MAXLEN - 1, index);
            }

            out_mat->AddProperty(&path, (name + "|file").c_str(), aiTextureType_UNKNOWN, 0);

            aiUVTransform uvTrafo;
            // XXX handle all kinds of UV transformations
            uvTrafo.mScaling = tex->UVScaling();
            uvTrafo.mTranslation = tex->UVTranslation();
            uvTrafo.mRotation = tex->UVRotation();
            out_mat->AddProperty(&uvTrafo, 1, (name + "|uvtrafo").c_str(), aiTextureType_UNKNOWN, 0);

            int uvIndex = 0;

            bool uvFound = false;
            const std::string &uvSet = PropertyGet<std::string>(tex->Props(), "UVSet", uvFound);
            if (uvFound) {
                // "default" is the name which usually appears in the FbxFileTexture template
                if (uvSet != "default" && uvSet.length()) {
                    // this is a bit awkward - we need to find a mesh that uses this
                    // material and scan its UV channels for the given UV name because
                    // assimp references UV channels by index, not by name.

                    // XXX: the case that UV channels may appear in different orders
                    // in meshes is unhandled. A possible solution would be to sort
                    // the UV channels alphabetically, but this would have the side
                    // effect that the primary (first) UV channel would sometimes
                    // be moved, causing trouble when users read only the first
                    // UV channel and ignore UV channel assignments altogether.

                    std::vector<aiMaterial *>::iterator materialIt = std::find(materials.begin(), materials.end(), out_mat);
                    const unsigned int matIndex = static_cast<unsigned int>(std::distance(materials.begin(), materialIt));

                    uvIndex = -1;
                    if (!mesh) {
                        for (const MeshMap::value_type &v : meshes_converted) {
                            const MeshGeometry *const meshGeom = dynamic_cast<const MeshGeometry *>(v.first);
                            if (!meshGeom) {
                                continue;
                            }

                            const MatIndexArray &mats = meshGeom->GetMaterialIndices();
                            if (std::find(mats.begin(), mats.end(), (int)matIndex) == mats.end()) {
                                continue;
                            }

                            int index = -1;
                            for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                                if (meshGeom->GetTextureCoords(i).empty()) {
                                    break;
                                }
                                const std::string &curName = meshGeom->GetTextureCoordChannelName(i);
                                if (curName == uvSet) {
                                    index = static_cast<int>(i);
                                    break;
                                }
                            }
                            if (index == -1) {
                                FBXImporter::LogWarn("did not find UV channel named ", uvSet, " in a mesh using this material");
                                continue;
                            }

                            if (uvIndex == -1) {
                                uvIndex = index;
                            } else {
                                FBXImporter::LogWarn("the UV channel named ", uvSet, " appears at different positions in meshes, results will be wrong");
                            }
                        }
                    } else {
                        int index = -1;
                        for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                            if (mesh->GetTextureCoords(i).empty()) {
                                break;
                            }
                            const std::string &curName = mesh->GetTextureCoordChannelName(i);
                            if (curName == uvSet) {
                                index = static_cast<int>(i);
                                break;
                            }
                        }
                        if (index == -1) {
                            FBXImporter::LogWarn("did not find UV channel named ", uvSet, " in a mesh using this material");
                        }

                        if (uvIndex == -1) {
                            uvIndex = index;
                        }
                    }

                    if (uvIndex == -1) {
                        FBXImporter::LogWarn("failed to resolve UV channel ", uvSet, ", using first UV channel");
                        uvIndex = 0;
                    }
                }
            }

            out_mat->AddProperty(&uvIndex, 1, (name + "|uvwsrc").c_str(), aiTextureType_UNKNOWN, 0);
        }
    }
}

double FBXConverter::FrameRateToDouble(FileGlobalSettings::FrameRate fp, double customFPSVal) {
    switch (fp) {
        case FileGlobalSettings::FrameRate_DEFAULT:
            return 1.0;

        case FileGlobalSettings::FrameRate_120:
            return 120.0;

        case FileGlobalSettings::FrameRate_100:
            return 100.0;

        case FileGlobalSettings::FrameRate_60:
            return 60.0;

        case FileGlobalSettings::FrameRate_50:
            return 50.0;

        case FileGlobalSettings::FrameRate_48:
            return 48.0;

        case FileGlobalSettings::FrameRate_30:
        case FileGlobalSettings::FrameRate_30_DROP:
            return 30.0;

        case FileGlobalSettings::FrameRate_NTSC_DROP_FRAME:
        case FileGlobalSettings::FrameRate_NTSC_FULL_FRAME:
            return 29.9700262;

        case FileGlobalSettings::FrameRate_PAL:
            return 25.0;

        case FileGlobalSettings::FrameRate_CINEMA:
            return 24.0;

        case FileGlobalSettings::FrameRate_1000:
            return 1000.0;

        case FileGlobalSettings::FrameRate_CINEMA_ND:
            return 23.976;

        case FileGlobalSettings::FrameRate_CUSTOM:
            return customFPSVal;

        case FileGlobalSettings::FrameRate_MAX: // this is to silence compiler warnings
            break;
    }

    ai_assert(false);

    return -1.0f;
}

void FBXConverter::ConvertAnimations() {
    // first of all determine framerate
    const FileGlobalSettings::FrameRate fps = doc.GlobalSettings().TimeMode();
    const float custom = doc.GlobalSettings().CustomFrameRate();
    anim_fps = FrameRateToDouble(fps, custom);

    const std::vector<const AnimationStack *> &curAnimations = doc.AnimationStacks();
    for (const AnimationStack *stack : curAnimations) {
        ConvertAnimationStack(*stack);
    }
}

std::string FBXConverter::FixNodeName(const std::string &name) {
    // strip Model:: prefix, avoiding ambiguities (i.e. don't strip if
    // this causes ambiguities, well possible between empty identifiers,
    // such as "Model::" and ""). Make sure the behaviour is consistent
    // across multiple calls to FixNodeName().
    if (name.substr(0, 7) == "Model::") {
        std::string temp = name.substr(7);
        return temp;
    }

    return name;
}

std::string FBXConverter::FixAnimMeshName(const std::string &name) {
    if (name.length()) {
        size_t indexOf = name.find_first_of("::");
        if (indexOf != std::string::npos && indexOf < name.size() - 2) {
            return name.substr(indexOf + 2);
        }
    }
    return name.length() ? name : "AnimMesh";
}

void FBXConverter::ConvertAnimationStack(const AnimationStack &st) {
    const AnimationLayerList &layers = st.Layers();
    if (layers.empty()) {
        return;
    }

    aiAnimation *const anim = new aiAnimation();
    animations.push_back(anim);

    // strip AnimationStack:: prefix
    std::string name = st.Name();
    if (name.substr(0, 16) == "AnimationStack::") {
        name = name.substr(16);
    } else if (name.substr(0, 11) == "AnimStack::") {
        name = name.substr(11);
    }

    anim->mName.Set(name);

    // need to find all nodes for which we need to generate node animations -
    // it may happen that we need to merge multiple layers, though.
    NodeMap node_map;

    // reverse mapping from curves to layers, much faster than querying
    // the FBX DOM for it.
    LayerMap layer_map;

    const char *prop_whitelist[] = {
        "Lcl Scaling",
        "Lcl Rotation",
        "Lcl Translation",
        "DeformPercent"
    };

    std::map<std::string, morphAnimData *> morphAnimDatas;

    for (const AnimationLayer *layer : layers) {
        ai_assert(layer);
        const AnimationCurveNodeList &nodes = layer->Nodes(prop_whitelist, 4);
        for (const AnimationCurveNode *node : nodes) {
            ai_assert(node);
            const Model *const model = dynamic_cast<const Model *>(node->Target());
            if (model) {
                const std::string &curName = FixNodeName(model->Name());
                node_map[curName].push_back(node);
                layer_map[node] = layer;
                continue;
            }
            const BlendShapeChannel *const bsc = dynamic_cast<const BlendShapeChannel *>(node->Target());
            if (bsc) {
                ProcessMorphAnimDatas(&morphAnimDatas, bsc, node);
            }
        }
    }

    // generate node animations
    std::vector<aiNodeAnim *> node_anims;

    double min_time = 1e10;
    double max_time = -1e10;

    int64_t start_time = st.LocalStart();
    int64_t stop_time = st.LocalStop();
    bool has_local_startstop = start_time != 0 || stop_time != 0;
    if (!has_local_startstop) {
        // no time range given, so accept every keyframe and use the actual min/max time
        // the numbers are INT64_MIN/MAX, the 20000 is for safety because GenerateNodeAnimations uses an epsilon of 10000
        start_time = -9223372036854775807ll + 20000;
        stop_time = 9223372036854775807ll - 20000;
    }

    try {
        for (const NodeMap::value_type &kv : node_map) {
            GenerateNodeAnimations(node_anims,
                    kv.first,
                    kv.second,
                    layer_map,
                    start_time, stop_time,
                    max_time,
                    min_time);
        }
    } catch (std::exception &) {
        std::for_each(node_anims.begin(), node_anims.end(), Util::delete_fun<aiNodeAnim>());
        throw;
    }

    if (node_anims.size() || morphAnimDatas.size()) {
        if (node_anims.size()) {
            anim->mChannels = new aiNodeAnim *[node_anims.size()]();
            anim->mNumChannels = static_cast<unsigned int>(node_anims.size());
            std::swap_ranges(node_anims.begin(), node_anims.end(), anim->mChannels);
        }
        if (morphAnimDatas.size()) {
            unsigned int numMorphMeshChannels = static_cast<unsigned int>(morphAnimDatas.size());
            anim->mMorphMeshChannels = new aiMeshMorphAnim *[numMorphMeshChannels];
            anim->mNumMorphMeshChannels = numMorphMeshChannels;
            unsigned int i = 0;
            for (const auto &morphAnimIt : morphAnimDatas) {
                morphAnimData *animData = morphAnimIt.second;
                unsigned int numKeys = static_cast<unsigned int>(animData->size());
                aiMeshMorphAnim *meshMorphAnim = new aiMeshMorphAnim();
                meshMorphAnim->mName.Set(morphAnimIt.first);
                meshMorphAnim->mNumKeys = numKeys;
                meshMorphAnim->mKeys = new aiMeshMorphKey[numKeys];
                unsigned int j = 0;
                for (auto &animIt : *animData) {
                    morphKeyData *keyData = animIt.second;
                    unsigned int numValuesAndWeights = static_cast<unsigned int>(keyData->values.size());
                    meshMorphAnim->mKeys[j].mNumValuesAndWeights = numValuesAndWeights;
                    meshMorphAnim->mKeys[j].mValues = new unsigned int[numValuesAndWeights];
                    meshMorphAnim->mKeys[j].mWeights = new double[numValuesAndWeights];
                    meshMorphAnim->mKeys[j].mTime = CONVERT_FBX_TIME(animIt.first) * anim_fps;
                    for (unsigned int k = 0; k < numValuesAndWeights; k++) {
                        meshMorphAnim->mKeys[j].mValues[k] = keyData->values.at(k);
                        meshMorphAnim->mKeys[j].mWeights[k] = keyData->weights.at(k);
                    }
                    j++;
                }
                anim->mMorphMeshChannels[i++] = meshMorphAnim;
            }
        }
    } else {
        // empty animations would fail validation, so drop them
        delete anim;
        animations.pop_back();
        FBXImporter::LogInfo("ignoring empty AnimationStack (using IK?): ", name);
        return;
    }

    double start_time_fps = has_local_startstop ? (CONVERT_FBX_TIME(start_time) * anim_fps) : min_time;
    double stop_time_fps = has_local_startstop ? (CONVERT_FBX_TIME(stop_time) * anim_fps) : max_time;

    // adjust relative timing for animation
    for (unsigned int c = 0; c < anim->mNumChannels; c++) {
        aiNodeAnim *channel = anim->mChannels[c];
        for (uint32_t i = 0; i < channel->mNumPositionKeys; i++) {
            channel->mPositionKeys[i].mTime -= start_time_fps;
        }
        for (uint32_t i = 0; i < channel->mNumRotationKeys; i++) {
            channel->mRotationKeys[i].mTime -= start_time_fps;
        }
        for (uint32_t i = 0; i < channel->mNumScalingKeys; i++) {
            channel->mScalingKeys[i].mTime -= start_time_fps;
        }
    }
    for (unsigned int c = 0; c < anim->mNumMorphMeshChannels; c++) {
        aiMeshMorphAnim *channel = anim->mMorphMeshChannels[c];
        for (uint32_t i = 0; i < channel->mNumKeys; i++) {
            channel->mKeys[i].mTime -= start_time_fps;
        }
    }

    // for some mysterious reason, mDuration is simply the maximum key -- the
    // validator always assumes animations to start at zero.
    anim->mDuration = stop_time_fps - start_time_fps;
    anim->mTicksPerSecond = anim_fps;
}

// ------------------------------------------------------------------------------------------------
void FBXConverter::ProcessMorphAnimDatas(std::map<std::string, morphAnimData *> *morphAnimDatas, const BlendShapeChannel *bsc, const AnimationCurveNode *node) {
    std::vector<const Connection *> bscConnections = doc.GetConnectionsBySourceSequenced(bsc->ID(), "Deformer");
    for (const Connection *bscConnection : bscConnections) {
        auto bs = dynamic_cast<const BlendShape *>(bscConnection->DestinationObject());
        if (bs) {
            auto channelIt = std::find(bs->BlendShapeChannels().begin(), bs->BlendShapeChannels().end(), bsc);
            if (channelIt != bs->BlendShapeChannels().end()) {
                auto channelIndex = static_cast<unsigned int>(std::distance(bs->BlendShapeChannels().begin(), channelIt));
                std::vector<const Connection *> bsConnections = doc.GetConnectionsBySourceSequenced(bs->ID(), "Geometry");
                for (const Connection *bsConnection : bsConnections) {
                    auto geo = dynamic_cast<const Geometry *>(bsConnection->DestinationObject());
                    if (geo) {
                        std::vector<const Connection *> geoConnections = doc.GetConnectionsBySourceSequenced(geo->ID(), "Model");
                        for (const Connection *geoConnection : geoConnections) {
                            auto model = dynamic_cast<const Model *>(geoConnection->DestinationObject());
                            if (model) {
                                auto geoIt = std::find(model->GetGeometry().begin(), model->GetGeometry().end(), geo);
                                auto geoIndex = static_cast<unsigned int>(std::distance(model->GetGeometry().begin(), geoIt));
                                auto name = aiString(FixNodeName(model->Name() + "*"));
                                name.length = 1 + ASSIMP_itoa10(name.data + name.length, AI_MAXLEN - 1, geoIndex);
                                morphAnimData *animData;
                                auto animIt = morphAnimDatas->find(name.C_Str());
                                if (animIt == morphAnimDatas->end()) {
                                    animData = new morphAnimData();
                                    morphAnimDatas->insert(std::make_pair(name.C_Str(), animData));
                                } else {
                                    animData = animIt->second;
                                }
                                for (std::pair<std::string, const AnimationCurve *> curvesIt : node->Curves()) {
                                    if (curvesIt.first == "d|DeformPercent") {
                                        const AnimationCurve *animationCurve = curvesIt.second;
                                        const KeyTimeList &keys = animationCurve->GetKeys();
                                        const KeyValueList &values = animationCurve->GetValues();
                                        unsigned int k = 0;
                                        for (auto key : keys) {
                                            morphKeyData *keyData;
                                            auto keyIt = animData->find(key);
                                            if (keyIt == animData->end()) {
                                                keyData = new morphKeyData();
                                                animData->insert(std::make_pair(key, keyData));
                                            } else {
                                                keyData = keyIt->second;
                                            }
                                            keyData->values.push_back(channelIndex);
                                            keyData->weights.push_back(values.at(k) / 100.0f);
                                            k++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
#ifdef ASSIMP_BUILD_DEBUG
// ------------------------------------------------------------------------------------------------
// sanity check whether the input is ok
static void validateAnimCurveNodes(const std::vector<const AnimationCurveNode *> &curves,
        bool strictMode) {
    const Object *target(nullptr);
    for (const AnimationCurveNode *node : curves) {
        if (!target) {
            target = node->Target();
        }
        if (node->Target() != target) {
            FBXImporter::LogWarn("Node target is nullptr type.");
        }
        if (strictMode) {
            ai_assert(node->Target() == target);
        }
    }
}
#endif // ASSIMP_BUILD_DEBUG

// ------------------------------------------------------------------------------------------------
void FBXConverter::GenerateNodeAnimations(std::vector<aiNodeAnim *> &node_anims,
        const std::string &fixed_name,
        const std::vector<const AnimationCurveNode *> &curves,
        const LayerMap &layer_map,
        int64_t start, int64_t stop,
        double &max_time,
        double &min_time) {

    NodeMap node_property_map;
    ai_assert(curves.size());

#ifdef ASSIMP_BUILD_DEBUG
    validateAnimCurveNodes(curves, doc.Settings().strictMode);
#endif
    const AnimationCurveNode *curve_node = nullptr;
    for (const AnimationCurveNode *node : curves) {
        ai_assert(node);

        if (node->TargetProperty().empty()) {
            FBXImporter::LogWarn("target property for animation curve not set: ", node->Name());
            continue;
        }

        curve_node = node;
        if (node->Curves().empty()) {
            FBXImporter::LogWarn("no animation curves assigned to AnimationCurveNode: ", node->Name());
            continue;
        }

        node_property_map[node->TargetProperty()].push_back(node);
    }

    ai_assert(curve_node);
    ai_assert(curve_node->TargetAsModel());

    const Model &target = *curve_node->TargetAsModel();

    // check for all possible transformation components
    NodeMap::const_iterator chain[TransformationComp_MAXIMUM];

    bool has_any = false;
    bool has_complex = false;

    for (size_t i = 0; i < TransformationComp_MAXIMUM; ++i) {
        const TransformationComp comp = static_cast<TransformationComp>(i);

        // inverse pivots don't exist in the input, we just generate them
        if (comp == TransformationComp_RotationPivotInverse || comp == TransformationComp_ScalingPivotInverse) {
            chain[i] = node_property_map.end();
            continue;
        }

        chain[i] = node_property_map.find(NameTransformationCompProperty(comp));
        if (chain[i] != node_property_map.end()) {

            // check if this curves contains redundant information by looking
            // up the corresponding node's transformation chain.
            if (doc.Settings().optimizeEmptyAnimationCurves &&
                    IsRedundantAnimationData(target, comp, (chain[i]->second))) {

                FBXImporter::LogVerboseDebug("dropping redundant animation channel for node ", target.Name());
                continue;
            }

            has_any = true;

            if (comp != TransformationComp_Rotation && comp != TransformationComp_Scaling && comp != TransformationComp_Translation) {
                has_complex = true;
            }
        }
    }

    if (!has_any) {
        FBXImporter::LogWarn("ignoring node animation, did not find any transformation key frames");
        return;
    }

    // this needs to play nicely with GenerateTransformationNodeChain() which will
    // be invoked _later_ (animations come first). If this node has only rotation,
    // scaling and translation _and_ there are no animated other components either,
    // we can use a single node and also a single node animation channel.
    if (!doc.Settings().preservePivots || (!has_complex && !NeedsComplexTransformationChain(target))) {
        aiNodeAnim* const nd = GenerateSimpleNodeAnim(fixed_name, target, chain,
                node_property_map.end(),
                start, stop,
                max_time,
                min_time
        );

        ai_assert(nd);
        if (nd->mNumPositionKeys == 0 && nd->mNumRotationKeys == 0 && nd->mNumScalingKeys == 0) {
            delete nd;
        } else {
            node_anims.push_back(nd);
        }
        return;
    }

    // otherwise, things get gruesome and we need separate animation channels
    // for each part of the transformation chain. Remember which channels
    // we generated and pass this information to the node conversion
    // code to avoid nodes that have identity transform, but non-identity
    // animations, being dropped.
    unsigned int flags = 0, bit = 0x1;
    for (size_t i = 0; i < TransformationComp_MAXIMUM; ++i, bit <<= 1) {
        const TransformationComp comp = static_cast<TransformationComp>(i);

        if (chain[i] != node_property_map.end()) {
            flags |= bit;

            ai_assert(comp != TransformationComp_RotationPivotInverse);
            ai_assert(comp != TransformationComp_ScalingPivotInverse);

            const std::string &chain_name = NameTransformationChainNode(fixed_name, comp);

            aiNodeAnim *na = nullptr;
            switch (comp) {
                case TransformationComp_Rotation:
                case TransformationComp_PreRotation:
                case TransformationComp_PostRotation:
                case TransformationComp_GeometricRotation:
                    na = GenerateRotationNodeAnim(chain_name,
                            target,
                            (*chain[i]).second,
                            layer_map,
                            start, stop,
                            max_time,
                            min_time);

                    break;

                case TransformationComp_RotationOffset:
                case TransformationComp_RotationPivot:
                case TransformationComp_ScalingOffset:
                case TransformationComp_ScalingPivot:
                case TransformationComp_Translation:
                case TransformationComp_GeometricTranslation:
                    na = GenerateTranslationNodeAnim(chain_name,
                            target,
                            (*chain[i]).second,
                            layer_map,
                            start, stop,
                            max_time,
                            min_time);

                    // pivoting requires us to generate an implicit inverse channel to undo the pivot translation
                    if (comp == TransformationComp_RotationPivot) {
                        const std::string &invName = NameTransformationChainNode(fixed_name,
                                TransformationComp_RotationPivotInverse);

                        aiNodeAnim *const inv = GenerateTranslationNodeAnim(invName,
                                target,
                                (*chain[i]).second,
                                layer_map,
                                start, stop,
                                max_time,
                                min_time,
                                true);

                        ai_assert(inv);
                        if (inv->mNumPositionKeys == 0 && inv->mNumRotationKeys == 0 && inv->mNumScalingKeys == 0) {
                            delete inv;
                        } else {
                            node_anims.push_back(inv);
                        }

                        ai_assert(TransformationComp_RotationPivotInverse > i);
                        flags |= bit << (TransformationComp_RotationPivotInverse - i);
                    } else if (comp == TransformationComp_ScalingPivot) {
                        const std::string &invName = NameTransformationChainNode(fixed_name,
                                TransformationComp_ScalingPivotInverse);

                        aiNodeAnim *const inv = GenerateTranslationNodeAnim(invName,
                                target,
                                (*chain[i]).second,
                                layer_map,
                                start, stop,
                                max_time,
                                min_time,
                                true);

                        ai_assert(inv);
                        if (inv->mNumPositionKeys == 0 && inv->mNumRotationKeys == 0 && inv->mNumScalingKeys == 0) {
                            delete inv;
                        } else {
                            node_anims.push_back(inv);
                        }

                        ai_assert(TransformationComp_RotationPivotInverse > i);
                        flags |= bit << (TransformationComp_RotationPivotInverse - i);
                    }

                    break;

                case TransformationComp_Scaling:
                case TransformationComp_GeometricScaling:
                    na = GenerateScalingNodeAnim(chain_name,
                            target,
                            (*chain[i]).second,
                            layer_map,
                            start, stop,
                            max_time,
                            min_time);

                    break;

                default:
                    ai_assert(false);
            }

            ai_assert(na);
            if (na->mNumPositionKeys == 0 && na->mNumRotationKeys == 0 && na->mNumScalingKeys == 0) {
                delete na;
            } else {
                node_anims.push_back(na);
            }
            continue;
        }
    }

    node_anim_chain_bits[fixed_name] = flags;
}

bool FBXConverter::IsRedundantAnimationData(const Model &target,
        TransformationComp comp,
        const std::vector<const AnimationCurveNode *> &curves) {
    ai_assert(curves.size());

    // look for animation nodes with
    //  * sub channels for all relevant components set
    //  * one key/value pair per component
    //  * combined values match up the corresponding value in the bind pose node transformation
    // only such nodes are 'redundant' for this function.

    if (curves.size() > 1) {
        return false;
    }

    const AnimationCurveNode &nd = *curves.front();
    const AnimationCurveMap &sub_curves = nd.Curves();

    const AnimationCurveMap::const_iterator dx = sub_curves.find("d|X");
    const AnimationCurveMap::const_iterator dy = sub_curves.find("d|Y");
    const AnimationCurveMap::const_iterator dz = sub_curves.find("d|Z");

    if (dx == sub_curves.end() || dy == sub_curves.end() || dz == sub_curves.end()) {
        return false;
    }

    const KeyValueList &vx = (*dx).second->GetValues();
    const KeyValueList &vy = (*dy).second->GetValues();
    const KeyValueList &vz = (*dz).second->GetValues();

    if (vx.size() != 1 || vy.size() != 1 || vz.size() != 1) {
        return false;
    }

    const aiVector3D dyn_val = aiVector3D(vx[0], vy[0], vz[0]);
    const aiVector3D &static_val = PropertyGet<aiVector3D>(target.Props(),
            NameTransformationCompProperty(comp),
            TransformationCompDefaultValue(comp));

    const float epsilon = Math::getEpsilon<float>();
    return (dyn_val - static_val).SquareLength() < epsilon;
}

aiNodeAnim *FBXConverter::GenerateRotationNodeAnim(const std::string &name,
        const Model &target,
        const std::vector<const AnimationCurveNode *> &curves,
        const LayerMap &layer_map,
        int64_t start, int64_t stop,
        double &max_time,
        double &min_time) {
    std::unique_ptr<aiNodeAnim> na(new aiNodeAnim());
    na->mNodeName.Set(name);

    ConvertRotationKeys(na.get(), curves, layer_map, start, stop, max_time, min_time, target.RotationOrder());

    // dummy scaling key
    na->mScalingKeys = new aiVectorKey[1];
    na->mNumScalingKeys = 1;

    na->mScalingKeys[0].mTime = 0.;
    na->mScalingKeys[0].mValue = aiVector3D(1.0f, 1.0f, 1.0f);

    // dummy position key
    na->mPositionKeys = new aiVectorKey[1];
    na->mNumPositionKeys = 1;

    na->mPositionKeys[0].mTime = 0.;
    na->mPositionKeys[0].mValue = aiVector3D();

    return na.release();
}

aiNodeAnim *FBXConverter::GenerateScalingNodeAnim(const std::string &name,
        const Model & /*target*/,
        const std::vector<const AnimationCurveNode *> &curves,
        const LayerMap &layer_map,
        int64_t start, int64_t stop,
        double &max_time,
        double &min_time) {
    std::unique_ptr<aiNodeAnim> na(new aiNodeAnim());
    na->mNodeName.Set(name);

    ConvertScaleKeys(na.get(), curves, layer_map, start, stop, max_time, min_time);

    // dummy rotation key
    na->mRotationKeys = new aiQuatKey[1];
    na->mNumRotationKeys = 1;

    na->mRotationKeys[0].mTime = 0.;
    na->mRotationKeys[0].mValue = aiQuaternion();

    // dummy position key
    na->mPositionKeys = new aiVectorKey[1];
    na->mNumPositionKeys = 1;

    na->mPositionKeys[0].mTime = 0.;
    na->mPositionKeys[0].mValue = aiVector3D();

    return na.release();
}

aiNodeAnim *FBXConverter::GenerateTranslationNodeAnim(const std::string &name,
        const Model & /*target*/,
        const std::vector<const AnimationCurveNode *> &curves,
        const LayerMap &layer_map,
        int64_t start, int64_t stop,
        double &max_time,
        double &min_time,
        bool inverse) {
    std::unique_ptr<aiNodeAnim> na(new aiNodeAnim());
    na->mNodeName.Set(name);

    ConvertTranslationKeys(na.get(), curves, layer_map, start, stop, max_time, min_time);

    if (inverse) {
        for (unsigned int i = 0; i < na->mNumPositionKeys; ++i) {
            na->mPositionKeys[i].mValue *= -1.0f;
        }
    }

    // dummy scaling key
    na->mScalingKeys = new aiVectorKey[1];
    na->mNumScalingKeys = 1;

    na->mScalingKeys[0].mTime = 0.;
    na->mScalingKeys[0].mValue = aiVector3D(1.0f, 1.0f, 1.0f);

    // dummy rotation key
    na->mRotationKeys = new aiQuatKey[1];
    na->mNumRotationKeys = 1;

    na->mRotationKeys[0].mTime = 0.;
    na->mRotationKeys[0].mValue = aiQuaternion();

    return na.release();
}

aiNodeAnim* FBXConverter::GenerateSimpleNodeAnim(const std::string& name,
        const Model& target,
        NodeMap::const_iterator chain[TransformationComp_MAXIMUM],
        NodeMap::const_iterator iterEnd,
        int64_t start, int64_t stop,
        double& maxTime,
        double& minTime)
{
    std::unique_ptr<aiNodeAnim> na(new aiNodeAnim());
    na->mNodeName.Set(name);

    const PropertyTable &props = target.Props();

    // collect unique times and keyframe lists
    KeyFrameListList keyframeLists[TransformationComp_MAXIMUM];
    KeyTimeList keytimes;

    for (size_t i = 0; i < TransformationComp_MAXIMUM; ++i) {
        if (chain[i] == iterEnd)
            continue;

        if (i == TransformationComp_Rotation || i == TransformationComp_PreRotation
                || i == TransformationComp_PostRotation || i == TransformationComp_GeometricRotation) {
            keyframeLists[i] = GetRotationKeyframeList((*chain[i]).second, start, stop);
        } else {
            keyframeLists[i] = GetKeyframeList((*chain[i]).second, start, stop);
        }

        for (KeyFrameListList::const_iterator it = keyframeLists[i].begin(); it != keyframeLists[i].end(); ++it) {
            const KeyTimeList& times = *std::get<0>(*it);
            keytimes.insert(keytimes.end(), times.begin(), times.end());
        }

        // remove duplicates
        std::sort(keytimes.begin(), keytimes.end());

        auto last = std::unique(keytimes.begin(), keytimes.end());
        keytimes.erase(last, keytimes.end());
    }

    const Model::RotOrder rotOrder = target.RotationOrder();
    const size_t keyCount = keytimes.size();

    aiVector3D defTranslate = PropertyGet(props, "Lcl Translation", aiVector3D(0.f, 0.f, 0.f));
    aiVector3D defRotation = PropertyGet(props, "Lcl Rotation", aiVector3D(0.f, 0.f, 0.f));
    aiVector3D defScale = PropertyGet(props, "Lcl Scaling", aiVector3D(1.f, 1.f, 1.f));

    aiVectorKey* outTranslations = new aiVectorKey[keyCount];
    aiQuatKey* outRotations = new aiQuatKey[keyCount];
    aiVectorKey* outScales = new aiVectorKey[keyCount];

    if (keyframeLists[TransformationComp_Translation].size() > 0) {
        InterpolateKeys(outTranslations, keytimes, keyframeLists[TransformationComp_Translation], defTranslate, maxTime, minTime);
    } else {
        for (size_t i = 0; i < keyCount; ++i) {
            outTranslations[i].mTime = CONVERT_FBX_TIME(keytimes[i]) * anim_fps;
            outTranslations[i].mValue = defTranslate;
        }
    }

    if (keyframeLists[TransformationComp_Rotation].size() > 0) {
        InterpolateKeys(outRotations, keytimes, keyframeLists[TransformationComp_Rotation], defRotation, maxTime, minTime, rotOrder);
    } else {
        aiQuaternion defQuat = EulerToQuaternion(defRotation, rotOrder);
        for (size_t i = 0; i < keyCount; ++i) {
            outRotations[i].mTime = CONVERT_FBX_TIME(keytimes[i]) * anim_fps;
            outRotations[i].mValue = defQuat;
        }
    }

    if (keyframeLists[TransformationComp_Scaling].size() > 0) {
        InterpolateKeys(outScales, keytimes, keyframeLists[TransformationComp_Scaling], defScale, maxTime, minTime);
    } else {
        for (size_t i = 0; i < keyCount; ++i) {
            outScales[i].mTime = CONVERT_FBX_TIME(keytimes[i]) * anim_fps;
            outScales[i].mValue = defScale;
        }
    }

    bool ok = false;

    const auto zero_epsilon = ai_epsilon;

    const aiVector3D& preRotation = PropertyGet<aiVector3D>(props, "PreRotation", ok);
    if (ok && preRotation.SquareLength() > zero_epsilon) {
        const aiQuaternion preQuat = EulerToQuaternion(preRotation, Model::RotOrder_EulerXYZ);
        for (size_t i = 0; i < keyCount; ++i) {
            outRotations[i].mValue = preQuat * outRotations[i].mValue;
        }
    }

    const aiVector3D& postRotation = PropertyGet<aiVector3D>(props, "PostRotation", ok);
    if (ok && postRotation.SquareLength() > zero_epsilon) {
        const aiQuaternion postQuat = EulerToQuaternion(postRotation, Model::RotOrder_EulerXYZ);
        for (size_t i = 0; i < keyCount; ++i) {
            outRotations[i].mValue = outRotations[i].mValue * postQuat;
        }
    }

    // convert TRS to SRT
    for (size_t i = 0; i < keyCount; ++i) {
        aiQuaternion& r = outRotations[i].mValue;
        aiVector3D& s = outScales[i].mValue;
        aiVector3D& t = outTranslations[i].mValue;

        aiMatrix4x4 mat, temp;
        aiMatrix4x4::Translation(t, mat);
        mat *= aiMatrix4x4(r.GetMatrix());
        mat *= aiMatrix4x4::Scaling(s, temp);

        mat.Decompose(s, r, t);
    }

    na->mNumScalingKeys = static_cast<unsigned int>(keyCount);
    na->mNumRotationKeys = na->mNumScalingKeys;
    na->mNumPositionKeys = na->mNumScalingKeys;

    na->mScalingKeys = outScales;
    na->mRotationKeys = outRotations;
    na->mPositionKeys = outTranslations;

    return na.release();
}

FBXConverter::KeyFrameListList FBXConverter::GetKeyframeList(const std::vector<const AnimationCurveNode *> &nodes, int64_t start, int64_t stop) {
    KeyFrameListList inputs;
    inputs.reserve(nodes.size() * 3);

    //give some breathing room for rounding errors
    int64_t adj_start = start - 10000;
    int64_t adj_stop = stop + 10000;

    for (const AnimationCurveNode *node : nodes) {
        ai_assert(node);

        const AnimationCurveMap &curves = node->Curves();
        for (const AnimationCurveMap::value_type &kv : curves) {

            unsigned int mapto;
            if (kv.first == "d|X") {
                mapto = 0;
            } else if (kv.first == "d|Y") {
                mapto = 1;
            } else if (kv.first == "d|Z") {
                mapto = 2;
            } else {
                FBXImporter::LogWarn("ignoring scale animation curve, did not recognize target component");
                continue;
            }

            const AnimationCurve *const curve = kv.second;
            ai_assert(curve->GetKeys().size() == curve->GetValues().size());
            ai_assert(curve->GetKeys().size());

            //get values within the start/stop time window
            std::shared_ptr<KeyTimeList> Keys(new KeyTimeList());
            std::shared_ptr<KeyValueList> Values(new KeyValueList());
            const size_t count = curve->GetKeys().size();
            Keys->reserve(count);
            Values->reserve(count);
            for (size_t n = 0; n < count; n++) {
                int64_t k = curve->GetKeys().at(n);
                if (k >= adj_start && k <= adj_stop) {
                    Keys->push_back(k);
                    Values->push_back(curve->GetValues().at(n));
                }
            }

            inputs.emplace_back(Keys, Values, mapto);
        }
    }
    return inputs; // pray for NRVO :-)
}

FBXConverter::KeyFrameListList FBXConverter::GetRotationKeyframeList(const std::vector<const AnimationCurveNode *> &nodes,
                                                                     int64_t start, int64_t stop) {
    KeyFrameListList inputs;
    inputs.reserve(nodes.size() * 3);

    // give some breathing room for rounding errors
    const int64_t adj_start = start - 10000;
    const int64_t adj_stop = stop + 10000;

    for (const AnimationCurveNode *node : nodes) {
        ai_assert(node);

        const AnimationCurveMap &curves = node->Curves();
        for (const AnimationCurveMap::value_type &kv : curves) {

            unsigned int mapto;
            if (kv.first == "d|X") {
                mapto = 0;
            } else if (kv.first == "d|Y") {
                mapto = 1;
            } else if (kv.first == "d|Z") {
                mapto = 2;
            } else {
                FBXImporter::LogWarn("ignoring scale animation curve, did not recognize target component");
                continue;
            }

            const AnimationCurve *const curve = kv.second;
            ai_assert(curve->GetKeys().size() == curve->GetValues().size());
            ai_assert(curve->GetKeys().size());

            // get values within the start/stop time window
            std::shared_ptr<KeyTimeList> Keys(new KeyTimeList());
            std::shared_ptr<KeyValueList> Values(new KeyValueList());
            const size_t count = curve->GetKeys().size();

            int64_t tp = curve->GetKeys().at(0);
            float vp = curve->GetValues().at(0);
            Keys->push_back(tp);
            Values->push_back(vp);
            if (count > 1) {
                int64_t tc = curve->GetKeys().at(1);
                float vc = curve->GetValues().at(1);
                for (size_t n = 1; n < count; n++) {
                    while (std::abs(vc - vp) >= 180.0f) {
                        double step = std::floor(double(tc - tp) / std::abs(vc - vp) * 179.0f);
                        int64_t tnew = tp + int64_t(step);
                        float vnew = vp + (vc - vp) * float(step / (tc - tp));
                        if (tnew >= adj_start && tnew <= adj_stop) {
                            Keys->push_back(tnew);
                            Values->push_back(vnew);
                        } else {
                            // Something broke
                            break;
                        }
                        tp = tnew;
                        vp = vnew;
                    }
                    if (tc >= adj_start && tc <= adj_stop) {
                        Keys->push_back(tc);
                        Values->push_back(vc);
                    }
                    if (n + 1 < count) {
                        tp = tc;
                        vp = vc;
                        tc = curve->GetKeys().at(n + 1);
                        vc = curve->GetValues().at(n + 1);
                    }
                }
            }
            inputs.emplace_back(Keys, Values, mapto);
        }
    }
    return inputs;
}

KeyTimeList FBXConverter::GetKeyTimeList(const KeyFrameListList &inputs) {
    ai_assert(!inputs.empty());

    // reserve some space upfront - it is likely that the key-frame lists
    // have matching time values, so max(of all key-frame lists) should
    // be a good estimate.
    KeyTimeList keys;

    size_t estimate = 0;
    for (const KeyFrameList &kfl : inputs) {
        estimate = std::max(estimate, std::get<0>(kfl)->size());
    }

    keys.reserve(estimate);

    std::vector<unsigned int> next_pos;
    next_pos.resize(inputs.size(), 0);

    const size_t count = inputs.size();
    while (true) {

        int64_t min_tick = std::numeric_limits<int64_t>::max();
        for (size_t i = 0; i < count; ++i) {
            const KeyFrameList &kfl = inputs[i];

            if (std::get<0>(kfl)->size() > next_pos[i] && std::get<0>(kfl)->at(next_pos[i]) < min_tick) {
                min_tick = std::get<0>(kfl)->at(next_pos[i]);
            }
        }

        if (min_tick == std::numeric_limits<int64_t>::max()) {
            break;
        }
        keys.push_back(min_tick);

        for (size_t i = 0; i < count; ++i) {
            const KeyFrameList &kfl = inputs[i];

            while (std::get<0>(kfl)->size() > next_pos[i] && std::get<0>(kfl)->at(next_pos[i]) == min_tick) {
                ++next_pos[i];
            }
        }
    }

    return keys;
}

void FBXConverter::InterpolateKeys(aiVectorKey *valOut, const KeyTimeList &keys, const KeyFrameListList &inputs,
        const aiVector3D &def_value,
        double &max_time,
        double &min_time) {
    ai_assert(!keys.empty());
    ai_assert(nullptr != valOut);

    std::vector<unsigned int> next_pos;
    const size_t count(inputs.size());

    next_pos.resize(inputs.size(), 0);

    for (KeyTimeList::value_type time : keys) {
        ai_real result[3] = { def_value.x, def_value.y, def_value.z };

        for (size_t i = 0; i < count; ++i) {
            const KeyFrameList &kfl = inputs[i];

            const size_t ksize = std::get<0>(kfl)->size();
            if (ksize == 0) {
                continue;
            }
            if (ksize > next_pos[i] && std::get<0>(kfl)->at(next_pos[i]) == time) {
                ++next_pos[i];
            }

            const size_t id0 = next_pos[i] > 0 ? next_pos[i] - 1 : 0;
            const size_t id1 = next_pos[i] == ksize ? ksize - 1 : next_pos[i];

            // use lerp for interpolation
            const KeyValueList::value_type valueA = std::get<1>(kfl)->at(id0);
            const KeyValueList::value_type valueB = std::get<1>(kfl)->at(id1);

            const KeyTimeList::value_type timeA = std::get<0>(kfl)->at(id0);
            const KeyTimeList::value_type timeB = std::get<0>(kfl)->at(id1);

            const ai_real factor = timeB == timeA ? ai_real(0.) : static_cast<ai_real>((time - timeA)) / (timeB - timeA);
            const ai_real interpValue = static_cast<ai_real>(valueA + (valueB - valueA) * factor);

            result[std::get<2>(kfl)] = interpValue;
        }

        // magic value to convert fbx times to seconds
        valOut->mTime = CONVERT_FBX_TIME(time) * anim_fps;

        min_time = std::min(min_time, valOut->mTime);
        max_time = std::max(max_time, valOut->mTime);

        valOut->mValue.x = result[0];
        valOut->mValue.y = result[1];
        valOut->mValue.z = result[2];

        ++valOut;
    }
}

void FBXConverter::InterpolateKeys(aiQuatKey *valOut, const KeyTimeList &keys, const KeyFrameListList &inputs,
        const aiVector3D &def_value,
        double &maxTime,
        double &minTime,
        Model::RotOrder order) {
    ai_assert(!keys.empty());
    ai_assert(nullptr != valOut);

    std::unique_ptr<aiVectorKey[]> temp(new aiVectorKey[keys.size()]);
    InterpolateKeys(temp.get(), keys, inputs, def_value, maxTime, minTime);

    aiMatrix4x4 m;

    aiQuaternion lastq;

    for (size_t i = 0, c = keys.size(); i < c; ++i) {

        valOut[i].mTime = temp[i].mTime;

        GetRotationMatrix(order, temp[i].mValue, m);
        aiQuaternion quat = aiQuaternion(aiMatrix3x3(m));

        // take shortest path by checking the inner product
        // http://www.3dkingdoms.com/weekly/weekly.php?a=36
        if (quat.x * lastq.x + quat.y * lastq.y + quat.z * lastq.z + quat.w * lastq.w < 0) {
            quat.Conjugate();
            quat.w = -quat.w;
        }
        lastq = quat;

        valOut[i].mValue = quat;
    }
}

aiQuaternion FBXConverter::EulerToQuaternion(const aiVector3D &rot, Model::RotOrder order) {
    aiMatrix4x4 m;
    GetRotationMatrix(order, rot, m);

    return aiQuaternion(aiMatrix3x3(m));
}

void FBXConverter::ConvertScaleKeys(aiNodeAnim *na, const std::vector<const AnimationCurveNode *> &nodes, const LayerMap & /*layers*/,
        int64_t start, int64_t stop,
        double &maxTime,
        double &minTime) {
    ai_assert(nodes.size());

    // XXX for now, assume scale should be blended geometrically (i.e. two
    // layers should be multiplied with each other). There is a FBX
    // property in the layer to specify the behaviour, though.

    const KeyFrameListList &inputs = GetKeyframeList(nodes, start, stop);
    const KeyTimeList &keys = GetKeyTimeList(inputs);

    na->mNumScalingKeys = static_cast<unsigned int>(keys.size());
    na->mScalingKeys = new aiVectorKey[keys.size()];
    if (keys.size() > 0) {
        InterpolateKeys(na->mScalingKeys, keys, inputs, aiVector3D(1.0f, 1.0f, 1.0f), maxTime, minTime);
    }
}

void FBXConverter::ConvertTranslationKeys(aiNodeAnim *na, const std::vector<const AnimationCurveNode *> &nodes,
        const LayerMap & /*layers*/,
        int64_t start, int64_t stop,
        double &maxTime,
        double &minTime) {
    ai_assert(nodes.size());

    // XXX see notes in ConvertScaleKeys()
    const KeyFrameListList &inputs = GetKeyframeList(nodes, start, stop);
    const KeyTimeList &keys = GetKeyTimeList(inputs);

    na->mNumPositionKeys = static_cast<unsigned int>(keys.size());
    na->mPositionKeys = new aiVectorKey[keys.size()];
    if (keys.size() > 0)
        InterpolateKeys(na->mPositionKeys, keys, inputs, aiVector3D(0.0f, 0.0f, 0.0f), maxTime, minTime);
}

void FBXConverter::ConvertRotationKeys(aiNodeAnim *na, const std::vector<const AnimationCurveNode *> &nodes,
        const LayerMap & /*layers*/,
        int64_t start, int64_t stop,
        double &maxTime,
        double &minTime,
        Model::RotOrder order) {
    ai_assert(nodes.size());

    // XXX see notes in ConvertScaleKeys()
    const std::vector<KeyFrameList> &inputs = GetRotationKeyframeList(nodes, start, stop);
    const KeyTimeList &keys = GetKeyTimeList(inputs);

    na->mNumRotationKeys = static_cast<unsigned int>(keys.size());
    na->mRotationKeys = new aiQuatKey[keys.size()];
    if (!keys.empty()) {
        InterpolateKeys(na->mRotationKeys, keys, inputs, aiVector3D(0.0f, 0.0f, 0.0f), maxTime, minTime, order);
    }
}

void FBXConverter::ConvertGlobalSettings() {
    if (nullptr == mSceneOut) {
        return;
    }

    const bool hasGenerator = !doc.Creator().empty();

    mSceneOut->mMetaData = aiMetadata::Alloc(16 + (hasGenerator ? 1 : 0));
    mSceneOut->mMetaData->Set(0, "UpAxis", doc.GlobalSettings().UpAxis());
    mSceneOut->mMetaData->Set(1, "UpAxisSign", doc.GlobalSettings().UpAxisSign());
    mSceneOut->mMetaData->Set(2, "FrontAxis", doc.GlobalSettings().FrontAxis());
    mSceneOut->mMetaData->Set(3, "FrontAxisSign", doc.GlobalSettings().FrontAxisSign());
    mSceneOut->mMetaData->Set(4, "CoordAxis", doc.GlobalSettings().CoordAxis());
    mSceneOut->mMetaData->Set(5, "CoordAxisSign", doc.GlobalSettings().CoordAxisSign());
    mSceneOut->mMetaData->Set(6, "OriginalUpAxis", doc.GlobalSettings().OriginalUpAxis());
    mSceneOut->mMetaData->Set(7, "OriginalUpAxisSign", doc.GlobalSettings().OriginalUpAxisSign());
    //const double unitScaleFactor = (double)doc.GlobalSettings().UnitScaleFactor();
    mSceneOut->mMetaData->Set(8, "UnitScaleFactor", doc.GlobalSettings().UnitScaleFactor());
    mSceneOut->mMetaData->Set(9, "OriginalUnitScaleFactor", doc.GlobalSettings().OriginalUnitScaleFactor());
    mSceneOut->mMetaData->Set(10, "AmbientColor", doc.GlobalSettings().AmbientColor());
    mSceneOut->mMetaData->Set(11, "FrameRate", (int)doc.GlobalSettings().TimeMode());
    mSceneOut->mMetaData->Set(12, "TimeSpanStart", doc.GlobalSettings().TimeSpanStart());
    mSceneOut->mMetaData->Set(13, "TimeSpanStop", doc.GlobalSettings().TimeSpanStop());
    mSceneOut->mMetaData->Set(14, "CustomFrameRate", doc.GlobalSettings().CustomFrameRate());
    mSceneOut->mMetaData->Set(15, AI_METADATA_SOURCE_FORMAT_VERSION, aiString(ai_to_string(doc.FBXVersion())));
    if (hasGenerator) {
        mSceneOut->mMetaData->Set(16, AI_METADATA_SOURCE_GENERATOR, aiString(doc.Creator()));
    }
}

void FBXConverter::TransferDataToScene() {
    ai_assert(!mSceneOut->mMeshes);
    ai_assert(!mSceneOut->mNumMeshes);

    // note: the trailing () ensures initialization with nullptr - not
    // many C++ users seem to know this, so pointing it out to avoid
    // confusion why this code works.

    if (!mMeshes.empty()) {
        mSceneOut->mMeshes = new aiMesh *[mMeshes.size()]();
        mSceneOut->mNumMeshes = static_cast<unsigned int>(mMeshes.size());

        std::swap_ranges(mMeshes.begin(), mMeshes.end(), mSceneOut->mMeshes);
    }

    if (!materials.empty()) {
        mSceneOut->mMaterials = new aiMaterial *[materials.size()]();
        mSceneOut->mNumMaterials = static_cast<unsigned int>(materials.size());

        std::swap_ranges(materials.begin(), materials.end(), mSceneOut->mMaterials);
    }

    if (!animations.empty()) {
        mSceneOut->mAnimations = new aiAnimation *[animations.size()]();
        mSceneOut->mNumAnimations = static_cast<unsigned int>(animations.size());

        std::swap_ranges(animations.begin(), animations.end(), mSceneOut->mAnimations);
    }

    if (!lights.empty()) {
        mSceneOut->mLights = new aiLight *[lights.size()]();
        mSceneOut->mNumLights = static_cast<unsigned int>(lights.size());

        std::swap_ranges(lights.begin(), lights.end(), mSceneOut->mLights);
    }

    if (!cameras.empty()) {
        mSceneOut->mCameras = new aiCamera *[cameras.size()]();
        mSceneOut->mNumCameras = static_cast<unsigned int>(cameras.size());

        std::swap_ranges(cameras.begin(), cameras.end(), mSceneOut->mCameras);
    }

    if (!textures.empty()) {
        mSceneOut->mTextures = new aiTexture *[textures.size()]();
        mSceneOut->mNumTextures = static_cast<unsigned int>(textures.size());

        std::swap_ranges(textures.begin(), textures.end(), mSceneOut->mTextures);
    }

    if (!mSkeletons.empty()) {
        mSceneOut->mSkeletons = new aiSkeleton *[mSkeletons.size()];
        mSceneOut->mNumSkeletons = static_cast<unsigned int>(mSkeletons.size());
        std::swap_ranges(mSkeletons.begin(), mSkeletons.end(), mSceneOut->mSkeletons);
    }
}

void FBXConverter::ConvertOrphanedEmbeddedTextures() {
    // in C++14 it could be:
    // for (auto&& [id, object] : objects)
    for (auto &&id_and_object : doc.Objects()) {
        auto &&id = std::get<0>(id_and_object);
        auto &&object = std::get<1>(id_and_object);
        // If an object doesn't have parent
        if (doc.ConnectionsBySource().count(id) == 0) {
            const Texture *realTexture = nullptr;
            try {
                const auto &element = object->GetElement();
                const Token &key = element.KeyToken();
                const char *obtype = key.begin();
                const size_t length = static_cast<size_t>(key.end() - key.begin());
                if (strncmp(obtype, "Texture", length) == 0) {
                    if (const Texture *texture = static_cast<const Texture *>(object->Get())) {
                        if (texture->Media() && texture->Media()->ContentLength() > 0) {
                            realTexture = texture;
                        }
                    }
                }
            } catch (...) {
                // do nothing
            }
            if (realTexture) {
                const Video *media = realTexture->Media();
                unsigned int index = ConvertVideo(*media);
                textures_converted[media] = index;
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
void ConvertToAssimpScene(aiScene *out, const Document &doc, bool removeEmptyBones) {
    FBXConverter converter(out, doc, removeEmptyBones);
}

} // namespace FBX
} // namespace Assimp

#endif
