/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team


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
#include "FBXParser.h"
#include "FBXMeshGeometry.h"
#include "FBXDocument.h"
#include "FBXUtil.h"
#include "FBXProperties.h"
#include "FBXImporter.h"

#include <assimp/StringComparison.h>
#include <assimp/MathFunctions.h>

#include <assimp/scene.h>

#include <assimp/CreateAnimMesh.h>

#include <tuple>
#include <memory>
#include <iterator>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <iostream>
#include <string>
#include <float.h>

#define FBX_ONE_SECOND 46186158000L


namespace Assimp {
    namespace FBX {

        using namespace Util;

        #define MAGIC_NODE_TAG "_$AssimpFbx$"

        #define CONVERT_FBX_TIME_TO_SECONDS(time) static_cast<double>(time) / 46186158000L
        #define CONVERT_FBX_TIME_TO_FRAMES(time, frames_per_second) CONVERT_FBX_TIME_TO_SECONDS(time) * frames_per_second
        FBXConverter::FBXConverter(aiScene* out, const Document& doc, bool removeEmptyBones )
        : defaultMaterialIndex()
        , lights()
        , cameras()
        , textures()
        , materials_converted()
        , textures_converted()
        , meshes_converted()
        , node_anim_chain_bits()
        , mNodeNames()
        , anim_fps()
        , out(out)
        , doc(doc) {
            // animations need to be converted first since this will
            // populate the node_anim_chain_bits map, which is needed
            // to determine which nodes need to be generated.
            ConvertAnimations();
            ConvertRootNode();

            if (doc.Settings().readAllMaterials) {
                // unfortunately this means we have to evaluate all objects
                for (const ObjectMap::value_type& v : doc.Objects()) {

                    const Object* ob = v.second->Get();
                    if (!ob) {
                        continue;
                    }

                    const Material* mat = dynamic_cast<const Material*>(ob);
                    if (mat) {

                        if (materials_converted.find(mat) == materials_converted.end()) {
                            ConvertMaterial(*mat, 0);
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
            }
        }


        FBXConverter::~FBXConverter() {
            std::for_each(meshes.begin(), meshes.end(), Util::delete_fun<aiMesh>());
            std::for_each(materials.begin(), materials.end(), Util::delete_fun<aiMaterial>());
            std::for_each(animations.begin(), animations.end(), Util::delete_fun<aiAnimation>());
            std::for_each(lights.begin(), lights.end(), Util::delete_fun<aiLight>());
            std::for_each(cameras.begin(), cameras.end(), Util::delete_fun<aiCamera>());
            std::for_each(textures.begin(), textures.end(), Util::delete_fun<aiTexture>());
        }

        void FBXConverter::ConvertRootNode() {
            out->mRootNode = new aiNode();
            std::string unique_name;
            GetUniqueName("RootNode", unique_name);
            out->mRootNode->mName.Set(unique_name);

            // root has ID 0
            ConvertNodes(0L, *out->mRootNode);
        }

        static std::string getAncestorBaseName(const aiNode* node)
        {
            const char* nodeName = nullptr;
            size_t length = 0;
            while (node && (!nodeName || length == 0))
            {
                nodeName = node->mName.C_Str();
                length = node->mName.length;
                node = node->mParent;
            }

            if (!nodeName || length == 0)
            {
                return {};
            }
            // could be std::string_view if c++17 available
            return std::string(nodeName, length);
        }

        // Make unique name
        std::string FBXConverter::MakeUniqueNodeName(const Model* const model, const aiNode& parent)
        {
            std::string original_name = FixNodeName(model->Name());
            if (original_name.empty())
            {
                original_name = getAncestorBaseName(&parent);
            }
            std::string unique_name;
            GetUniqueName(original_name, unique_name);
            return unique_name;
        }

        void FBXConverter::ConvertNodes(uint64_t id, aiNode& parent, const aiMatrix4x4& parent_transform) {
            const std::vector<const Connection*>& conns = doc.GetConnectionsByDestinationSequenced(id, "Model");

            std::vector<aiNode*> nodes;
            nodes.reserve(conns.size());

            std::vector<aiNode*> nodes_chain;
            std::vector<aiNode*> post_nodes_chain;

            try {
                for (const Connection* con : conns) {

                    // ignore object-property links
                    if (con->PropertyName().length()) {
                        continue;
                    }

                    const Object* const object = con->SourceObject();
                    if (nullptr == object) {
                        FBXImporter::LogWarn("failed to convert source object for Model link");
                        continue;
                    }

                    const Model* const model = dynamic_cast<const Model*>(object);

                    if (nullptr != model) {
                        nodes_chain.clear();
                        post_nodes_chain.clear();

                        aiMatrix4x4 new_abs_transform = parent_transform;

                        std::string unique_name = MakeUniqueNodeName(model, parent);

                        // even though there is only a single input node, the design of
                        // assimp (or rather: the complicated transformation chain that
                        // is employed by fbx) means that we may need multiple aiNode's
                        // to represent a fbx node's transformation.
                        const bool need_additional_node = GenerateTransformationNodeChain(*model, unique_name, nodes_chain, post_nodes_chain);

                        ai_assert(nodes_chain.size());

                        if (need_additional_node) {
                            nodes_chain.push_back(new aiNode(unique_name));
                        }

                        //setup metadata on newest node
                        SetupNodeMetadata(*model, *nodes_chain.back());

                        // link all nodes in a row
                        aiNode* last_parent = &parent;
                        for (aiNode* prenode : nodes_chain) {
                            ai_assert(prenode);

                            if (last_parent != &parent) {
                                last_parent->mNumChildren = 1;
                                last_parent->mChildren = new aiNode*[1];
                                last_parent->mChildren[0] = prenode;
                            }

                            prenode->mParent = last_parent;
                            last_parent = prenode;

                            new_abs_transform *= prenode->mTransformation;
                        }

                        // attach geometry
                        ConvertModel(*model, *nodes_chain.back(), new_abs_transform);

                        // check if there will be any child nodes
                        const std::vector<const Connection*>& child_conns
                            = doc.GetConnectionsByDestinationSequenced(model->ID(), "Model");

                        // if so, link the geometric transform inverse nodes
                        // before we attach any child nodes
                        if (child_conns.size()) {
                            for (aiNode* postnode : post_nodes_chain) {
                                ai_assert(postnode);

                                if (last_parent != &parent) {
                                    last_parent->mNumChildren = 1;
                                    last_parent->mChildren = new aiNode*[1];
                                    last_parent->mChildren[0] = postnode;
                                }

                                postnode->mParent = last_parent;
                                last_parent = postnode;

                                new_abs_transform *= postnode->mTransformation;
                            }
                        }
                        else {
                            // free the nodes we allocated as we don't need them
                            Util::delete_fun<aiNode> deleter;
                            std::for_each(
                                post_nodes_chain.begin(),
                                post_nodes_chain.end(),
                                deleter
                            );
                        }

                        // attach sub-nodes (if any)
                        ConvertNodes(model->ID(), *last_parent, new_abs_transform);

                        if (doc.Settings().readLights) {
                            ConvertLights(*model, unique_name);
                        }

                        if (doc.Settings().readCameras) {
                            ConvertCameras(*model, unique_name);
                        }

                        nodes.push_back(nodes_chain.front());
                        nodes_chain.clear();
                    }
                }

                if (nodes.size()) {
                    parent.mChildren = new aiNode*[nodes.size()]();
                    parent.mNumChildren = static_cast<unsigned int>(nodes.size());

                    std::swap_ranges(nodes.begin(), nodes.end(), parent.mChildren);
                }
            }
            catch (std::exception&) {
                Util::delete_fun<aiNode> deleter;
                std::for_each(nodes.begin(), nodes.end(), deleter);
                std::for_each(nodes_chain.begin(), nodes_chain.end(), deleter);
                std::for_each(post_nodes_chain.begin(), post_nodes_chain.end(), deleter);
            }
        }


        void FBXConverter::ConvertLights(const Model& model, const std::string &orig_name) {
            const std::vector<const NodeAttribute*>& node_attrs = model.GetAttributes();
            for (const NodeAttribute* attr : node_attrs) {
                const Light* const light = dynamic_cast<const Light*>(attr);
                if (light) {
                    ConvertLight(*light, orig_name);
                }
            }
        }

        void FBXConverter::ConvertCameras(const Model& model, const std::string &orig_name) {
            const std::vector<const NodeAttribute*>& node_attrs = model.GetAttributes();
            for (const NodeAttribute* attr : node_attrs) {
                const Camera* const cam = dynamic_cast<const Camera*>(attr);
                if (cam) {
                    ConvertCamera(*cam, orig_name);
                }
            }
        }

        void FBXConverter::ConvertLight(const Light& light, const std::string &orig_name) {
            lights.push_back(new aiLight());
            aiLight* const out_light = lights.back();

            out_light->mName.Set(orig_name);

            const float intensity = light.Intensity() / 100.0f;
            const aiVector3D& col = light.Color();

            out_light->mColorDiffuse = aiColor3D(col.x, col.y, col.z);
            out_light->mColorDiffuse.r *= intensity;
            out_light->mColorDiffuse.g *= intensity;
            out_light->mColorDiffuse.b *= intensity;

            out_light->mColorSpecular = out_light->mColorDiffuse;

            //lights are defined along negative y direction
            out_light->mPosition = aiVector3D(0.0f);
            out_light->mDirection = aiVector3D(0.0f, -1.0f, 0.0f);
            out_light->mUp = aiVector3D(0.0f, 0.0f, -1.0f);

            switch (light.LightType())
            {
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
                ai_assert(false);
            }

            float decay = light.DecayStart();
            switch (light.DecayType())
            {
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
                ai_assert(false);
                break;
            }
        }

        void FBXConverter::ConvertCamera(const Camera& cam, const std::string &orig_name)
        {
            cameras.push_back(new aiCamera());
            aiCamera* const out_camera = cameras.back();

            out_camera->mName.Set(orig_name);

            out_camera->mAspect = cam.AspectWidth() / cam.AspectHeight();

            out_camera->mPosition = aiVector3D(0.0f);
            out_camera->mLookAt = aiVector3D(1.0f, 0.0f, 0.0f);
            out_camera->mUp = aiVector3D(0.0f, 1.0f, 0.0f);

            out_camera->mHorizontalFOV = AI_DEG_TO_RAD(cam.FieldOfView());

            out_camera->mClipPlaneNear = cam.NearPlane();
            out_camera->mClipPlaneFar = cam.FarPlane();

            out_camera->mHorizontalFOV = AI_DEG_TO_RAD(cam.FieldOfView());
            out_camera->mClipPlaneNear = cam.NearPlane();
            out_camera->mClipPlaneFar = cam.FarPlane();
        }

        void FBXConverter::GetUniqueName(const std::string &name, std::string &uniqueName)
        {
            uniqueName = name;
            auto it_pair = mNodeNames.insert({ name, 0 }); // duplicate node name instance count
            unsigned int& i = it_pair.first->second;
            while (!it_pair.second)
            {
                i++;
                std::ostringstream ext;
                ext << name << std::setfill('0') << std::setw(3) << i;
                uniqueName = ext.str();
                it_pair = mNodeNames.insert({ uniqueName, 0 });
            }
        }

        const char* FBXConverter::NameTransformationComp(TransformationComp comp) {
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

        const char* FBXConverter::NameTransformationCompProperty(TransformationComp comp) {
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
            case TransformationComp_MAXIMUM: // this is to silence compiler warnings
                break;
            }

            ai_assert(false);

            return nullptr;
        }

        aiVector3D FBXConverter::TransformationCompDefaultValue(TransformationComp comp)
        {
            // XXX a neat way to solve the never-ending special cases for scaling
            // would be to do everything in log space!
            return comp == TransformationComp_Scaling ? aiVector3D(1.f, 1.f, 1.f) : aiVector3D();
        }

        void FBXConverter::GetRotationMatrix(Model::RotOrder mode, const aiVector3D& rotation, aiMatrix4x4& out)
        {
            if (mode == Model::RotOrder_SphericXYZ) {
                FBXImporter::LogError("Unsupported RotationMode: SphericXYZ");
                out = aiMatrix4x4();
                return;
            }

            const float angle_epsilon = Math::getEpsilon<float>();

            out = aiMatrix4x4();

            bool is_id[3] = { true, true, true };

            aiMatrix4x4 temp[3];
            if (std::fabs(rotation.z) > angle_epsilon) {
                aiMatrix4x4::RotationZ(AI_DEG_TO_RAD(rotation.z), temp[2]);
                is_id[2] = false;
            }
            if (std::fabs(rotation.y) > angle_epsilon) {
                aiMatrix4x4::RotationY(AI_DEG_TO_RAD(rotation.y), temp[1]);
                is_id[1] = false;
            }
            if (std::fabs(rotation.x) > angle_epsilon) {
                aiMatrix4x4::RotationX(AI_DEG_TO_RAD(rotation.x), temp[0]);
                is_id[0] = false;
            }

            int order[3] = { -1, -1, -1 };

            // note: rotation order is inverted since we're left multiplying as is usual in assimp
            switch (mode)
            {
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

        bool FBXConverter::NeedsComplexTransformationChain(const Model& model)
        {
            const PropertyTable& props = model.Props();
            bool ok;

            const float zero_epsilon = 1e-6f;
            const aiVector3D all_ones(1.0f, 1.0f, 1.0f);
            for (size_t i = 0; i < TransformationComp_MAXIMUM; ++i) {
                const TransformationComp comp = static_cast<TransformationComp>(i);

                if (comp == TransformationComp_Rotation || comp == TransformationComp_Scaling || comp == TransformationComp_Translation) {
                    continue;
                }

                bool scale_compare = (comp == TransformationComp_GeometricScaling || comp == TransformationComp_Scaling);

                const aiVector3D& v = PropertyGet<aiVector3D>(props, NameTransformationCompProperty(comp), ok);
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

        std::string FBXConverter::NameTransformationChainNode(const std::string& name, TransformationComp comp)
        {
            return name + std::string(MAGIC_NODE_TAG) + "_" + NameTransformationComp(comp);
        }

        bool FBXConverter::GenerateTransformationNodeChain(const Model& model, const std::string& name, std::vector<aiNode*>& output_nodes,
            std::vector<aiNode*>& post_output_nodes) {
            const PropertyTable& props = model.Props();
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

            const aiVector3D& PreRotation = PropertyGet<aiVector3D>(props, "PreRotation", ok);
            if (ok && PreRotation.SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_PreRotation);

                GetRotationMatrix(Model::RotOrder::RotOrder_EulerXYZ, PreRotation, chain[TransformationComp_PreRotation]);
            }

            const aiVector3D& PostRotation = PropertyGet<aiVector3D>(props, "PostRotation", ok);
            if (ok && PostRotation.SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_PostRotation);

                GetRotationMatrix(Model::RotOrder::RotOrder_EulerXYZ, PostRotation, chain[TransformationComp_PostRotation]);
            }

            const aiVector3D& RotationPivot = PropertyGet<aiVector3D>(props, "RotationPivot", ok);
            if (ok && RotationPivot.SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_RotationPivot) | (1 << TransformationComp_RotationPivotInverse);

                aiMatrix4x4::Translation(RotationPivot, chain[TransformationComp_RotationPivot]);
                aiMatrix4x4::Translation(-RotationPivot, chain[TransformationComp_RotationPivotInverse]);
            }

            const aiVector3D& RotationOffset = PropertyGet<aiVector3D>(props, "RotationOffset", ok);
            if (ok && RotationOffset.SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_RotationOffset);

                aiMatrix4x4::Translation(RotationOffset, chain[TransformationComp_RotationOffset]);
            }

            const aiVector3D& ScalingOffset = PropertyGet<aiVector3D>(props, "ScalingOffset", ok);
            if (ok && ScalingOffset.SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_ScalingOffset);

                aiMatrix4x4::Translation(ScalingOffset, chain[TransformationComp_ScalingOffset]);
            }

            const aiVector3D& ScalingPivot = PropertyGet<aiVector3D>(props, "ScalingPivot", ok);
            if (ok && ScalingPivot.SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_ScalingPivot) | (1 << TransformationComp_ScalingPivotInverse);

                aiMatrix4x4::Translation(ScalingPivot, chain[TransformationComp_ScalingPivot]);
                aiMatrix4x4::Translation(-ScalingPivot, chain[TransformationComp_ScalingPivotInverse]);
            }

            const aiVector3D& Translation = PropertyGet<aiVector3D>(props, "Lcl Translation", ok);
            if (ok && Translation.SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_Translation);

                aiMatrix4x4::Translation(Translation, chain[TransformationComp_Translation]);
            }

            const aiVector3D& Scaling = PropertyGet<aiVector3D>(props, "Lcl Scaling", ok);
            if (ok && (Scaling - all_ones).SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_Scaling);

                aiMatrix4x4::Scaling(Scaling, chain[TransformationComp_Scaling]);
            }

            // use me rotation code
            const aiVector3D& Rotation = PropertyGet<aiVector3D>(props, "Lcl Rotation", ok);
            if (ok && Rotation.SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_Rotation);

                GetRotationMatrix(rot, Rotation, chain[TransformationComp_Rotation]);
            }

            const aiVector3D& GeometricScaling = PropertyGet<aiVector3D>(props, "GeometricScaling", ok);
            if (ok && (GeometricScaling - all_ones).SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_GeometricScaling);
                aiMatrix4x4::Scaling(GeometricScaling, chain[TransformationComp_GeometricScaling]);
                aiVector3D GeometricScalingInverse = GeometricScaling;
                bool canscale = true;
                for (unsigned int i = 0; i < 3; ++i) {
                    if (std::fabs(GeometricScalingInverse[i]) > zero_epsilon) {
                        GeometricScalingInverse[i] = 1.0f / GeometricScaling[i];
                    }
                    else {
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

            const aiVector3D& GeometricRotation = PropertyGet<aiVector3D>(props, "GeometricRotation", ok);
            if (ok && GeometricRotation.SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_GeometricRotation) | (1 << TransformationComp_GeometricRotationInverse);
                GetRotationMatrix(rot, GeometricRotation, chain[TransformationComp_GeometricRotation]);
                GetRotationMatrix(rot, GeometricRotation, chain[TransformationComp_GeometricRotationInverse]);
                chain[TransformationComp_GeometricRotationInverse].Inverse();
            }

            const aiVector3D& GeometricTranslation = PropertyGet<aiVector3D>(props, "GeometricTranslation", ok);
            if (ok && GeometricTranslation.SquareLength() > zero_epsilon) {
                chainBits = chainBits | (1 << TransformationComp_GeometricTranslation) | (1 << TransformationComp_GeometricTranslationInverse);
                aiMatrix4x4::Translation(GeometricTranslation, chain[TransformationComp_GeometricTranslation]);
                aiMatrix4x4::Translation(-GeometricTranslation, chain[TransformationComp_GeometricTranslationInverse]);
            }

            // is_complex needs to be consistent with NeedsComplexTransformationChain()
            // or the interplay between this code and the animation converter would
            // not be guaranteed.
            ai_assert(NeedsComplexTransformationChain(model) == ((chainBits & chainMaskComplex) != 0));

            // now, if we have more than just Translation, Scaling and Rotation,
            // we need to generate a full node chain to accommodate for assimp's
            // lack to express pivots and offsets.
            if ((chainBits & chainMaskComplex) && doc.Settings().preservePivots) {
                FBXImporter::LogInfo("generating full transformation chain for node: " + name);

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

                    aiNode* nd = new aiNode();
                    nd->mName.Set(NameTransformationChainNode(name, comp));
                    nd->mTransformation = chain[i];

                    // geometric inverses go in a post-node chain
                    if (comp == TransformationComp_GeometricScalingInverse ||
                        comp == TransformationComp_GeometricRotationInverse ||
                        comp == TransformationComp_GeometricTranslationInverse
                        ) {
                        post_output_nodes.push_back(nd);
                    }
                    else {
                        output_nodes.push_back(nd);
                    }
                }

                ai_assert(output_nodes.size());
                return true;
            }

            // else, we can just multiply the matrices together
            aiNode* nd = new aiNode();
            output_nodes.push_back(nd);

            // name passed to the method is already unique
            nd->mName.Set(name);

            for (const auto &transform : chain) {
                nd->mTransformation = nd->mTransformation * transform;
            }
            return false;
        }

        void FBXConverter::SetupNodeMetadata(const Model& model, aiNode& nd)
        {
            const PropertyTable& props = model.Props();
            DirectPropertyMap unparsedProperties = props.GetUnparsedProperties();

            // create metadata on node
            const std::size_t numStaticMetaData = 2;
            aiMetadata* data = aiMetadata::Alloc(static_cast<unsigned int>(unparsedProperties.size() + numStaticMetaData));
            nd.mMetaData = data;
            int index = 0;

            // find user defined properties (3ds Max)
            data->Set(index++, "UserProperties", aiString(PropertyGet<std::string>(props, "UDP3DSMAX", "")));
            // preserve the info that a node was marked as Null node in the original file.
            data->Set(index++, "IsNull", model.IsNull() ? true : false);

            // add unparsed properties to the node's metadata
            for (const DirectPropertyMap::value_type& prop : unparsedProperties) {
                // Interpret the property as a concrete type
                if (const TypedProperty<bool>* interpreted = prop.second->As<TypedProperty<bool> >()) {
                    data->Set(index++, prop.first, interpreted->Value());
                }
                else if (const TypedProperty<int>* interpreted = prop.second->As<TypedProperty<int> >()) {
                    data->Set(index++, prop.first, interpreted->Value());
                }
                else if (const TypedProperty<uint64_t>* interpreted = prop.second->As<TypedProperty<uint64_t> >()) {
                    data->Set(index++, prop.first, interpreted->Value());
                }
                else if (const TypedProperty<float>* interpreted = prop.second->As<TypedProperty<float> >()) {
                    data->Set(index++, prop.first, interpreted->Value());
                }
                else if (const TypedProperty<std::string>* interpreted = prop.second->As<TypedProperty<std::string> >()) {
                    data->Set(index++, prop.first, aiString(interpreted->Value()));
                }
                else if (const TypedProperty<aiVector3D>* interpreted = prop.second->As<TypedProperty<aiVector3D> >()) {
                    data->Set(index++, prop.first, interpreted->Value());
                }
                else {
                    ai_assert(false);
                }
            }
        }

        void FBXConverter::ConvertModel(const Model& model, aiNode& nd, const aiMatrix4x4& node_global_transform)
        {
            const std::vector<const Geometry*>& geos = model.GetGeometry();

            std::vector<unsigned int> meshes;
            meshes.reserve(geos.size());

            for (const Geometry* geo : geos) {

                const MeshGeometry* const mesh = dynamic_cast<const MeshGeometry*>(geo);
                const LineGeometry* const line = dynamic_cast<const LineGeometry*>(geo);
                if (mesh) {
                    const std::vector<unsigned int>& indices = ConvertMesh(*mesh, model, node_global_transform, nd);
                    std::copy(indices.begin(), indices.end(), std::back_inserter(meshes));
                }
                else if (line) {
                    const std::vector<unsigned int>& indices = ConvertLine(*line, model, node_global_transform, nd);
                    std::copy(indices.begin(), indices.end(), std::back_inserter(meshes));
                }
                else {
                    FBXImporter::LogWarn("ignoring unrecognized geometry: " + geo->Name());
                }
            }

            if (meshes.size()) {
                nd.mMeshes = new unsigned int[meshes.size()]();
                nd.mNumMeshes = static_cast<unsigned int>(meshes.size());

                std::swap_ranges(meshes.begin(), meshes.end(), nd.mMeshes);
            }
        }

        std::vector<unsigned int> FBXConverter::ConvertMesh(const MeshGeometry& mesh, const Model& model,
            const aiMatrix4x4& node_global_transform, aiNode& nd)
        {
            std::vector<unsigned int> temp;

            MeshMap::const_iterator it = meshes_converted.find(&mesh);
            if (it != meshes_converted.end()) {
                std::copy((*it).second.begin(), (*it).second.end(), std::back_inserter(temp));
                return temp;
            }

            const std::vector<aiVector3D>& vertices = mesh.GetVertices();
            const std::vector<unsigned int>& faces = mesh.GetFaceIndexCounts();
            if (vertices.empty() || faces.empty()) {
                FBXImporter::LogWarn("ignoring empty geometry: " + mesh.Name());
                return temp;
            }

            // one material per mesh maps easily to aiMesh. Multiple material
            // meshes need to be split.
            const MatIndexArray& mindices = mesh.GetMaterialIndices();
            if (doc.Settings().readMaterials && !mindices.empty()) {
                const MatIndexArray::value_type base = mindices[0];
                for (MatIndexArray::value_type index : mindices) {
                    if (index != base) {
                        return ConvertMeshMultiMaterial(mesh, model, node_global_transform, nd);
                    }
                }
            }

            // faster code-path, just copy the data
            temp.push_back(ConvertMeshSingleMaterial(mesh, model, node_global_transform, nd));
            return temp;
        }

        std::vector<unsigned int> FBXConverter::ConvertLine(const LineGeometry& line, const Model& model,
            const aiMatrix4x4& node_global_transform, aiNode& nd)
        {
            std::vector<unsigned int> temp;

            const std::vector<aiVector3D>& vertices = line.GetVertices();
            const std::vector<int>& indices = line.GetIndices();
            if (vertices.empty() || indices.empty()) {
                FBXImporter::LogWarn("ignoring empty line: " + line.Name());
                return temp;
            }

            aiMesh* const out_mesh = SetupEmptyMesh(line, nd);
            out_mesh->mPrimitiveTypes |= aiPrimitiveType_LINE;

            // copy vertices
            out_mesh->mNumVertices = static_cast<unsigned int>(vertices.size());
            out_mesh->mVertices = new aiVector3D[out_mesh->mNumVertices];
            std::copy(vertices.begin(), vertices.end(), out_mesh->mVertices);

            //Number of line segments (faces) is "Number of Points - Number of Endpoints"
            //N.B.: Endpoints in FbxLine are denoted by negative indices.
            //If such an Index is encountered, add 1 and multiply by -1 to get the real index.
            unsigned int epcount = 0;
            for (unsigned i = 0; i < indices.size(); i++)
            {
                if (indices[i] < 0) {
                    epcount++;
                }
            }
            unsigned int pcount = static_cast<unsigned int>( indices.size() );
            unsigned int scount = out_mesh->mNumFaces = pcount - epcount;

            aiFace* fac = out_mesh->mFaces = new aiFace[scount]();
            for (unsigned int i = 0; i < pcount; ++i) {
                if (indices[i] < 0) continue;
                aiFace& f = *fac++;
                f.mNumIndices = 2; //2 == aiPrimitiveType_LINE 
                f.mIndices = new unsigned int[2];
                f.mIndices[0] = indices[i];
                int segid = indices[(i + 1 == pcount ? 0 : i + 1)];   //If we have reached he last point, wrap around
                f.mIndices[1] = (segid < 0 ? (segid + 1)*-1 : segid); //Convert EndPoint Index to normal Index
            }
            temp.push_back(static_cast<unsigned int>(meshes.size() - 1));
            return temp;
        }

        aiMesh* FBXConverter::SetupEmptyMesh(const Geometry& mesh, aiNode& nd)
        {
            aiMesh* const out_mesh = new aiMesh();
            meshes.push_back(out_mesh);
            meshes_converted[&mesh].push_back(static_cast<unsigned int>(meshes.size() - 1));

            // set name
            std::string name = mesh.Name();
            if (name.substr(0, 10) == "Geometry::") {
                name = name.substr(10);
            }

            if (name.length()) {
                out_mesh->mName.Set(name);
            }
            else
            {
                out_mesh->mName = nd.mName;
            }

            return out_mesh;
        }

        unsigned int FBXConverter::ConvertMeshSingleMaterial(const MeshGeometry& mesh, const Model& model,
            const aiMatrix4x4& node_global_transform, aiNode& nd)
        {
            const MatIndexArray& mindices = mesh.GetMaterialIndices();
            aiMesh* const out_mesh = SetupEmptyMesh(mesh, nd);

            const std::vector<aiVector3D>& vertices = mesh.GetVertices();
            const std::vector<unsigned int>& faces = mesh.GetFaceIndexCounts();

            // copy vertices
            out_mesh->mNumVertices = static_cast<unsigned int>(vertices.size());
            out_mesh->mVertices = new aiVector3D[vertices.size()];

            std::copy(vertices.begin(), vertices.end(), out_mesh->mVertices);

            // generate dummy faces
            out_mesh->mNumFaces = static_cast<unsigned int>(faces.size());
            aiFace* fac = out_mesh->mFaces = new aiFace[faces.size()]();

            unsigned int cursor = 0;
            for (unsigned int pcount : faces) {
                aiFace& f = *fac++;
                f.mNumIndices = pcount;
                f.mIndices = new unsigned int[pcount];
                switch (pcount)
                {
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
            const std::vector<aiVector3D>& normals = mesh.GetNormals();
            if (normals.size()) {
                ai_assert(normals.size() == vertices.size());

                out_mesh->mNormals = new aiVector3D[vertices.size()];
                std::copy(normals.begin(), normals.end(), out_mesh->mNormals);
            }

            // copy tangents - assimp requires both tangents and bitangents (binormals)
            // to be present, or neither of them. Compute binormals from normals
            // and tangents if needed.
            const std::vector<aiVector3D>& tangents = mesh.GetTangents();
            const std::vector<aiVector3D>* binormals = &mesh.GetBinormals();

            if (tangents.size()) {
                std::vector<aiVector3D> tempBinormals;
                if (!binormals->size()) {
                    if (normals.size()) {
                        tempBinormals.resize(normals.size());
                        for (unsigned int i = 0; i < tangents.size(); ++i) {
                            tempBinormals[i] = normals[i] ^ tangents[i];
                        }

                        binormals = &tempBinormals;
                    }
                    else {
                        binormals = NULL;
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
                const std::vector<aiVector2D>& uvs = mesh.GetTextureCoords(i);
                if (uvs.empty()) {
                    break;
                }

                aiVector3D* out_uv = out_mesh->mTextureCoords[i] = new aiVector3D[vertices.size()];
                for (const aiVector2D& v : uvs) {
                    *out_uv++ = aiVector3D(v.x, v.y, 0.0f);
                }

                out_mesh->mNumUVComponents[i] = 2;
            }

            // copy vertex colors
            for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
                const std::vector<aiColor4D>& colors = mesh.GetVertexColors(i);
                if (colors.empty()) {
                    break;
                }

                out_mesh->mColors[i] = new aiColor4D[vertices.size()];
                std::copy(colors.begin(), colors.end(), out_mesh->mColors[i]);
            }

            if (!doc.Settings().readMaterials || mindices.empty()) {
                FBXImporter::LogError("no material assigned to mesh, setting default material");
                out_mesh->mMaterialIndex = GetDefaultMaterial();
            }
            else {
                ConvertMaterialForMesh(out_mesh, model, mesh, mindices[0]);
            }

            if (doc.Settings().readWeights && mesh.DeformerSkin() != NULL) {
                ConvertWeights(out_mesh, model, mesh, node_global_transform, NO_MATERIAL_SEPARATION);
            }

            std::vector<aiAnimMesh*> animMeshes;
            for (const BlendShape* blendShape : mesh.GetBlendShapes()) {
                for (const BlendShapeChannel* blendShapeChannel : blendShape->BlendShapeChannels()) {
                    const std::vector<const ShapeGeometry*>& shapeGeometries = blendShapeChannel->GetShapeGeometries();
                    for (size_t i = 0; i < shapeGeometries.size(); i++) {
                        aiAnimMesh *animMesh = aiCreateAnimMesh(out_mesh);
                        const ShapeGeometry* shapeGeometry = shapeGeometries.at(i);
                        const std::vector<aiVector3D>& vertices = shapeGeometry->GetVertices();
                        const std::vector<aiVector3D>& normals = shapeGeometry->GetNormals();
                        const std::vector<unsigned int>& indices = shapeGeometry->GetIndices();
                        animMesh->mName.Set(FixAnimMeshName(shapeGeometry->Name()));
                        for (size_t j = 0; j < indices.size(); j++) {
                            unsigned int index = indices.at(j);
                            aiVector3D vertex = vertices.at(j);
                            aiVector3D normal = normals.at(j);
                            unsigned int count = 0;
                            const unsigned int* outIndices = mesh.ToOutputVertexIndex(index, count);
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
                out_mesh->mAnimMeshes = new aiAnimMesh*[numAnimMeshes];
                for (size_t i = 0; i < numAnimMeshes; i++) {
                    out_mesh->mAnimMeshes[i] = animMeshes.at(i);
                }
            }
            return static_cast<unsigned int>(meshes.size() - 1);
        }

        std::vector<unsigned int> FBXConverter::ConvertMeshMultiMaterial(const MeshGeometry& mesh, const Model& model,
            const aiMatrix4x4& node_global_transform, aiNode& nd)
        {
            const MatIndexArray& mindices = mesh.GetMaterialIndices();
            ai_assert(mindices.size());

            std::set<MatIndexArray::value_type> had;
            std::vector<unsigned int> indices;

            for (MatIndexArray::value_type index : mindices) {
                if (had.find(index) == had.end()) {

                    indices.push_back(ConvertMeshMultiMaterial(mesh, model, index, node_global_transform, nd));
                    had.insert(index);
                }
            }

            return indices;
        }

        unsigned int FBXConverter::ConvertMeshMultiMaterial(const MeshGeometry& mesh, const Model& model,
            MatIndexArray::value_type index,
            const aiMatrix4x4& node_global_transform,
            aiNode& nd)
        {
            aiMesh* const out_mesh = SetupEmptyMesh(mesh, nd);

            const MatIndexArray& mindices = mesh.GetMaterialIndices();
            const std::vector<aiVector3D>& vertices = mesh.GetVertices();
            const std::vector<unsigned int>& faces = mesh.GetFaceIndexCounts();

            const bool process_weights = doc.Settings().readWeights && mesh.DeformerSkin() != NULL;

            unsigned int count_faces = 0;
            unsigned int count_vertices = 0;

            // count faces
            std::vector<unsigned int>::const_iterator itf = faces.begin();
            for (MatIndexArray::const_iterator it = mindices.begin(),
                end = mindices.end(); it != end; ++it, ++itf)
            {
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
            aiFace* fac = out_mesh->mFaces = new aiFace[count_faces]();


            // allocate normals
            const std::vector<aiVector3D>& normals = mesh.GetNormals();
            if (normals.size()) {
                ai_assert(normals.size() == vertices.size());
                out_mesh->mNormals = new aiVector3D[vertices.size()];
            }

            // allocate tangents, binormals.
            const std::vector<aiVector3D>& tangents = mesh.GetTangents();
            const std::vector<aiVector3D>* binormals = &mesh.GetBinormals();
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
                    }
                    else {
                        binormals = NULL;
                    }
                }

                if (binormals) {
                    ai_assert(tangents.size() == vertices.size() && binormals->size() == vertices.size());

                    out_mesh->mTangents = new aiVector3D[vertices.size()];
                    out_mesh->mBitangents = new aiVector3D[vertices.size()];
                }
            }

            // allocate texture coords
            unsigned int num_uvs = 0;
            for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i, ++num_uvs) {
                const std::vector<aiVector2D>& uvs = mesh.GetTextureCoords(i);
                if (uvs.empty()) {
                    break;
                }

                out_mesh->mTextureCoords[i] = new aiVector3D[vertices.size()];
                out_mesh->mNumUVComponents[i] = 2;
            }

            // allocate vertex colors
            unsigned int num_vcs = 0;
            for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i, ++num_vcs) {
                const std::vector<aiColor4D>& colors = mesh.GetVertexColors(i);
                if (colors.empty()) {
                    break;
                }

                out_mesh->mColors[i] = new aiColor4D[vertices.size()];
            }

            unsigned int cursor = 0, in_cursor = 0;

            itf = faces.begin();
            for (MatIndexArray::const_iterator it = mindices.begin(), end = mindices.end(); it != end; ++it, ++itf)
            {
                const unsigned int pcount = *itf;
                if ((*it) != index) {
                    in_cursor += pcount;
                    continue;
                }

                aiFace& f = *fac++;

                f.mNumIndices = pcount;
                f.mIndices = new unsigned int[pcount];
                switch (pcount)
                {
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
                        const std::vector<aiVector2D>& uvs = mesh.GetTextureCoords(j);
                        out_mesh->mTextureCoords[j][cursor] = aiVector3D(uvs[in_cursor].x, uvs[in_cursor].y, 0.0f);
                    }

                    for (unsigned int j = 0; j < num_vcs; ++j) {
                        const std::vector<aiColor4D>& cols = mesh.GetVertexColors(j);
                        out_mesh->mColors[j][cursor] = cols[in_cursor];
                    }
                }
            }

            ConvertMaterialForMesh(out_mesh, model, mesh, index);

            if (process_weights) {
                ConvertWeights(out_mesh, model, mesh, node_global_transform, index, &reverseMapping);
            }

            std::vector<aiAnimMesh*> animMeshes;
            for (const BlendShape* blendShape : mesh.GetBlendShapes()) {
                for (const BlendShapeChannel* blendShapeChannel : blendShape->BlendShapeChannels()) {
                    const std::vector<const ShapeGeometry*>& shapeGeometries = blendShapeChannel->GetShapeGeometries();
                    for (size_t i = 0; i < shapeGeometries.size(); i++) {
                        aiAnimMesh* animMesh = aiCreateAnimMesh(out_mesh);
                        const ShapeGeometry* shapeGeometry = shapeGeometries.at(i);
                        const std::vector<aiVector3D>& vertices = shapeGeometry->GetVertices();
                        const std::vector<aiVector3D>& normals = shapeGeometry->GetNormals();
                        const std::vector<unsigned int>& indices = shapeGeometry->GetIndices();
                        animMesh->mName.Set(FixAnimMeshName(shapeGeometry->Name()));
                        for (size_t j = 0; j < indices.size(); j++) {
                            unsigned int index = indices.at(j);
                            aiVector3D vertex = vertices.at(j);
                            aiVector3D normal = normals.at(j);
                            unsigned int count = 0;
                            const unsigned int* outIndices = mesh.ToOutputVertexIndex(index, count);
                            for (unsigned int k = 0; k < count; k++) {
                                unsigned int outIndex = outIndices[k];
                                if (translateIndexMap.find(outIndex) == translateIndexMap.end())
                                    continue;
                                unsigned int index = translateIndexMap[outIndex];
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
                out_mesh->mAnimMeshes = new aiAnimMesh*[numAnimMeshes];
                for (size_t i = 0; i < numAnimMeshes; i++) {
                    out_mesh->mAnimMeshes[i] = animMeshes.at(i);
                }
            }

            return static_cast<unsigned int>(meshes.size() - 1);
        }

        void FBXConverter::ConvertWeights(aiMesh* out, const Model& model, const MeshGeometry& geo,
            const aiMatrix4x4& node_global_transform,
            unsigned int materialIndex,
            std::vector<unsigned int>* outputVertStartIndices)
        {
            ai_assert(geo.DeformerSkin());

            std::vector<size_t> out_indices;
            std::vector<size_t> index_out_indices;
            std::vector<size_t> count_out_indices;

            const Skin& sk = *geo.DeformerSkin();

            std::vector<aiBone*> bones;
            bones.reserve(sk.Clusters().size());

            const bool no_mat_check = materialIndex == NO_MATERIAL_SEPARATION;
            ai_assert(no_mat_check || outputVertStartIndices);

            try {

                for (const Cluster* cluster : sk.Clusters()) {
                    ai_assert(cluster);

                    const WeightIndexArray& indices = cluster->GetIndices();

                    const MatIndexArray& mats = geo.GetMaterialIndices();

                    const size_t no_index_sentinel = std::numeric_limits<size_t>::max();

                    count_out_indices.clear();
                    index_out_indices.clear();
                    out_indices.clear();

                    // now check if *any* of these weights is contained in the output mesh,
                    // taking notes so we don't need to do it twice.
                    for (WeightIndexArray::value_type index : indices) {

                        unsigned int count = 0;
                        const unsigned int* const out_idx = geo.ToOutputVertexIndex(index, count);
                        // ToOutputVertexIndex only returns NULL if index is out of bounds
                        // which should never happen
                        ai_assert(out_idx != NULL);

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
                                        out_idx[i]
                                    );

                                    out_indices.push_back(std::distance(outputVertStartIndices->begin(), it));
                                }

                                ++count_out_indices.back();                               
                            }
                        }
                    }
                    
                    // if we found at least one, generate the output bones
                    // XXX this could be heavily simplified by collecting the bone
                    // data in a single step.
                    ConvertCluster(bones, model, *cluster, out_indices, index_out_indices,
                            count_out_indices, node_global_transform);
                }
            }
            catch (std::exception&) {
                std::for_each(bones.begin(), bones.end(), Util::delete_fun<aiBone>());
                throw;
            }

            if (bones.empty()) {
                return;
            }

            out->mBones = new aiBone*[bones.size()]();
            out->mNumBones = static_cast<unsigned int>(bones.size());

            std::swap_ranges(bones.begin(), bones.end(), out->mBones);
        }

        void FBXConverter::ConvertCluster(std::vector<aiBone*>& bones, const Model& /*model*/, const Cluster& cl,
            std::vector<size_t>& out_indices,
            std::vector<size_t>& index_out_indices,
            std::vector<size_t>& count_out_indices,
            const aiMatrix4x4& node_global_transform)
        {

            aiBone* const bone = new aiBone();
            bones.push_back(bone);

            bone->mName = FixNodeName(cl.TargetNode()->Name());

            bone->mOffsetMatrix = cl.TransformLink();
            bone->mOffsetMatrix.Inverse();

            bone->mOffsetMatrix = bone->mOffsetMatrix * node_global_transform;

            bone->mNumWeights = static_cast<unsigned int>(out_indices.size());
            aiVertexWeight* cursor = bone->mWeights = new aiVertexWeight[out_indices.size()];

            const size_t no_index_sentinel = std::numeric_limits<size_t>::max();
            const WeightArray& weights = cl.GetWeights();

            const size_t c = index_out_indices.size();
            for (size_t i = 0; i < c; ++i) {
                const size_t index_index = index_out_indices[i];

                if (index_index == no_index_sentinel) {
                    continue;
                }

                const size_t cc = count_out_indices[i];
                for (size_t j = 0; j < cc; ++j) {
                    aiVertexWeight& out_weight = *cursor++;

                    out_weight.mVertexId = static_cast<unsigned int>(out_indices[index_index + j]);
                    out_weight.mWeight = weights[i];
                }
            }
        }

        void FBXConverter::ConvertMaterialForMesh(aiMesh* out, const Model& model, const MeshGeometry& geo,
            MatIndexArray::value_type materialIndex)
        {
            // locate source materials for this mesh
            const std::vector<const Material*>& mats = model.GetMaterials();
            if (static_cast<unsigned int>(materialIndex) >= mats.size() || materialIndex < 0) {
                FBXImporter::LogError("material index out of bounds, setting default material");
                out->mMaterialIndex = GetDefaultMaterial();
                return;
            }

            const Material* const mat = mats[materialIndex];
            MaterialMap::const_iterator it = materials_converted.find(mat);
            if (it != materials_converted.end()) {
                out->mMaterialIndex = (*it).second;
                return;
            }

            out->mMaterialIndex = ConvertMaterial(*mat, &geo);
            materials_converted[mat] = out->mMaterialIndex;
        }

        unsigned int FBXConverter::GetDefaultMaterial()
        {
            if (defaultMaterialIndex) {
                return defaultMaterialIndex - 1;
            }

            aiMaterial* out_mat = new aiMaterial();
            materials.push_back(out_mat);

            const aiColor3D diffuse = aiColor3D(0.8f, 0.8f, 0.8f);
            out_mat->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);

            aiString s;
            s.Set(AI_DEFAULT_MATERIAL_NAME);

            out_mat->AddProperty(&s, AI_MATKEY_NAME);

            defaultMaterialIndex = static_cast<unsigned int>(materials.size());
            return defaultMaterialIndex - 1;
        }


        unsigned int FBXConverter::ConvertMaterial(const Material& material, const MeshGeometry* const mesh)
        {
            const PropertyTable& props = material.Props();

            // generate empty output material
            aiMaterial* out_mat = new aiMaterial();
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
            if (material.GetShadingModel() == "phong")
            {
                aiShadingMode shadingMode = aiShadingMode_Phong;
                out_mat->AddProperty<aiShadingMode>(&shadingMode, 1, AI_MATKEY_SHADING_MODEL);               
            }

            // shading stuff and colors
            SetShadingPropertiesCommon(out_mat, props);
            SetShadingPropertiesRaw( out_mat, props, material.Textures(), mesh );

            // texture assignments
            SetTextureProperties(out_mat, material.Textures(), mesh);
            SetTextureProperties(out_mat, material.LayeredTextures(), mesh);

            return static_cast<unsigned int>(materials.size() - 1);
        }

        unsigned int FBXConverter::ConvertVideo(const Video& video)
        {
            // generate empty output texture
            aiTexture* out_tex = new aiTexture();
            textures.push_back(out_tex);

            // assuming the texture is compressed
            out_tex->mWidth = static_cast<unsigned int>(video.ContentLength()); // total data size
            out_tex->mHeight = 0; // fixed to 0

            // steal the data from the Video to avoid an additional copy
            out_tex->pcData = reinterpret_cast<aiTexel*>(const_cast<Video&>(video).RelinquishContent());

            // try to extract a hint from the file extension
            const std::string& filename = video.RelativeFilename().empty() ? video.FileName() : video.RelativeFilename();
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

        aiString FBXConverter::GetTexturePath(const Texture* tex)
        {
            aiString path;
            path.Set(tex->RelativeFilename());

            const Video* media = tex->Media();
            if (media != nullptr) {
                bool textureReady = false; //tells if our texture is ready (if it was loaded or if it was found)
                unsigned int index;

                VideoMap::const_iterator it = textures_converted.find(media);
                if (it != textures_converted.end()) {
                    index = (*it).second;
                    textureReady = true;
                }
                else {
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
                        path.length = 1 + ASSIMP_itoa10(path.data + 1, MAXLEN - 1, index);
                    }
                }
            }

            return path;
        }

        void FBXConverter::TrySetTextureProperties(aiMaterial* out_mat, const TextureMap& textures,
                const std::string& propName,
                aiTextureType target, const MeshGeometry* const mesh) {
            TextureMap::const_iterator it = textures.find(propName);
            if (it == textures.end()) {
                return;
            }

            const Texture* const tex = (*it).second;
            if (tex != 0)
            {
                aiString path = GetTexturePath(tex);
                out_mat->AddProperty(&path, _AI_MATKEY_TEXTURE_BASE, target, 0);

                aiUVTransform uvTrafo;
                // XXX handle all kinds of UV transformations
                uvTrafo.mScaling = tex->UVScaling();
                uvTrafo.mTranslation = tex->UVTranslation();
                out_mat->AddProperty(&uvTrafo, 1, _AI_MATKEY_UVTRANSFORM_BASE, target, 0);

                const PropertyTable& props = tex->Props();

                int uvIndex = 0;

                bool ok;
                const std::string& uvSet = PropertyGet<std::string>(props, "UVSet", ok);
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
                            std::find(materials.begin(), materials.end(), out_mat)
                        ));


                        uvIndex = -1;
                        if (!mesh)
                        {
                            for (const MeshMap::value_type& v : meshes_converted) {
                                const MeshGeometry* const meshGeom = dynamic_cast<const MeshGeometry*> (v.first);
                                if (!meshGeom) {
                                    continue;
                                }

                                const MatIndexArray& mats = meshGeom->GetMaterialIndices();
                                if (std::find(mats.begin(), mats.end(), matIndex) == mats.end()) {
                                    continue;
                                }

                                int index = -1;
                                for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                                    if (meshGeom->GetTextureCoords(i).empty()) {
                                        break;
                                    }
                                    const std::string& name = meshGeom->GetTextureCoordChannelName(i);
                                    if (name == uvSet) {
                                        index = static_cast<int>(i);
                                        break;
                                    }
                                }
                                if (index == -1) {
                                    FBXImporter::LogWarn("did not find UV channel named " + uvSet + " in a mesh using this material");
                                    continue;
                                }

                                if (uvIndex == -1) {
                                    uvIndex = index;
                                }
                                else {
                                    FBXImporter::LogWarn("the UV channel named " + uvSet +
                                        " appears at different positions in meshes, results will be wrong");
                                }
                            }
                        }
                        else
                        {
                            int index = -1;
                            for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                                if (mesh->GetTextureCoords(i).empty()) {
                                    break;
                                }
                                const std::string& name = mesh->GetTextureCoordChannelName(i);
                                if (name == uvSet) {
                                    index = static_cast<int>(i);
                                    break;
                                }
                            }
                            if (index == -1) {
                                FBXImporter::LogWarn("did not find UV channel named " + uvSet + " in a mesh using this material");
                            }

                            if (uvIndex == -1) {
                                uvIndex = index;
                            }
                        }

                        if (uvIndex == -1) {
                            FBXImporter::LogWarn("failed to resolve UV channel " + uvSet + ", using first UV channel");
                            uvIndex = 0;
                        }
                    }
                }

                out_mat->AddProperty(&uvIndex, 1, _AI_MATKEY_UVWSRC_BASE, target, 0);
            }
        }

        void FBXConverter::TrySetTextureProperties(aiMaterial* out_mat, const LayeredTextureMap& layeredTextures,
            const std::string& propName,
            aiTextureType target, const MeshGeometry* const mesh) {
            LayeredTextureMap::const_iterator it = layeredTextures.find(propName);
            if (it == layeredTextures.end()) {
                return;
            }

            int texCount = (*it).second->textureCount();

            // Set the blend mode for layered textures
            int blendmode = (*it).second->GetBlendMode();
            out_mat->AddProperty(&blendmode, 1, _AI_MATKEY_TEXOP_BASE, target, 0);

            for (int texIndex = 0; texIndex < texCount; texIndex++) {

                const Texture* const tex = (*it).second->getTexture(texIndex);

                aiString path = GetTexturePath(tex);
                out_mat->AddProperty(&path, _AI_MATKEY_TEXTURE_BASE, target, texIndex);

                aiUVTransform uvTrafo;
                // XXX handle all kinds of UV transformations
                uvTrafo.mScaling = tex->UVScaling();
                uvTrafo.mTranslation = tex->UVTranslation();
                out_mat->AddProperty(&uvTrafo, 1, _AI_MATKEY_UVTRANSFORM_BASE, target, texIndex);

                const PropertyTable& props = tex->Props();

                int uvIndex = 0;

                bool ok;
                const std::string& uvSet = PropertyGet<std::string>(props, "UVSet", ok);
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
                            std::find(materials.begin(), materials.end(), out_mat)
                        ));

                        uvIndex = -1;
                        if (!mesh)
                        {
                            for (const MeshMap::value_type& v : meshes_converted) {
                                const MeshGeometry* const meshGeom = dynamic_cast<const MeshGeometry*> (v.first);
                                if (!meshGeom) {
                                    continue;
                                }

                                const MatIndexArray& mats = meshGeom->GetMaterialIndices();
                                if (std::find(mats.begin(), mats.end(), matIndex) == mats.end()) {
                                    continue;
                                }

                                int index = -1;
                                for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                                    if (meshGeom->GetTextureCoords(i).empty()) {
                                        break;
                                    }
                                    const std::string& name = meshGeom->GetTextureCoordChannelName(i);
                                    if (name == uvSet) {
                                        index = static_cast<int>(i);
                                        break;
                                    }
                                }
                                if (index == -1) {
                                    FBXImporter::LogWarn("did not find UV channel named " + uvSet + " in a mesh using this material");
                                    continue;
                                }

                                if (uvIndex == -1) {
                                    uvIndex = index;
                                }
                                else {
                                    FBXImporter::LogWarn("the UV channel named " + uvSet +
                                        " appears at different positions in meshes, results will be wrong");
                                }
                            }
                        }
                        else
                        {
                            int index = -1;
                            for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                                if (mesh->GetTextureCoords(i).empty()) {
                                    break;
                                }
                                const std::string& name = mesh->GetTextureCoordChannelName(i);
                                if (name == uvSet) {
                                    index = static_cast<int>(i);
                                    break;
                                }
                            }
                            if (index == -1) {
                                FBXImporter::LogWarn("did not find UV channel named " + uvSet + " in a mesh using this material");
                            }

                            if (uvIndex == -1) {
                                uvIndex = index;
                            }
                        }

                        if (uvIndex == -1) {
                            FBXImporter::LogWarn("failed to resolve UV channel " + uvSet + ", using first UV channel");
                            uvIndex = 0;
                        }
                    }
                }

                out_mat->AddProperty(&uvIndex, 1, _AI_MATKEY_UVWSRC_BASE, target, texIndex);
            }
        }

        void FBXConverter::SetTextureProperties(aiMaterial* out_mat, const TextureMap& textures, const MeshGeometry* const mesh)
        {
            TrySetTextureProperties(out_mat, textures, "DiffuseColor", aiTextureType_DIFFUSE, mesh);
            TrySetTextureProperties(out_mat, textures, "AmbientColor", aiTextureType_AMBIENT, mesh);
            TrySetTextureProperties(out_mat, textures, "EmissiveColor", aiTextureType_EMISSIVE, mesh);
            TrySetTextureProperties(out_mat, textures, "SpecularColor", aiTextureType_SPECULAR, mesh);
            TrySetTextureProperties(out_mat, textures, "SpecularFactor", aiTextureType_SPECULAR, mesh);
            TrySetTextureProperties(out_mat, textures, "TransparentColor", aiTextureType_OPACITY, mesh);
            TrySetTextureProperties(out_mat, textures, "ReflectionColor", aiTextureType_REFLECTION, mesh);
            TrySetTextureProperties(out_mat, textures, "DisplacementColor", aiTextureType_DISPLACEMENT, mesh);
            TrySetTextureProperties(out_mat, textures, "NormalMap", aiTextureType_NORMALS, mesh);
            TrySetTextureProperties(out_mat, textures, "Bump", aiTextureType_HEIGHT, mesh);
            TrySetTextureProperties(out_mat, textures, "ShininessExponent", aiTextureType_SHININESS, mesh);
			TrySetTextureProperties( out_mat, textures, "TransparencyFactor", aiTextureType_OPACITY, mesh );
			TrySetTextureProperties( out_mat, textures, "EmissiveFactor", aiTextureType_EMISSIVE, mesh );
            //Maya counterparts
            TrySetTextureProperties(out_mat, textures, "Maya|DiffuseTexture", aiTextureType_DIFFUSE, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|NormalTexture", aiTextureType_NORMALS, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|SpecularTexture", aiTextureType_SPECULAR, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|FalloffTexture", aiTextureType_OPACITY, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|ReflectionMapTexture", aiTextureType_REFLECTION, mesh);
            
            // Maya PBR
            TrySetTextureProperties(out_mat, textures, "Maya|baseColor|file", aiTextureType_BASE_COLOR, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|normalCamera|file", aiTextureType_NORMAL_CAMERA, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|emissionColor|file", aiTextureType_EMISSION_COLOR, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|metalness|file", aiTextureType_METALNESS, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|diffuseRoughness|file", aiTextureType_DIFFUSE_ROUGHNESS, mesh);
            
            // Maya stingray
            TrySetTextureProperties(out_mat, textures, "Maya|TEX_color_map|file", aiTextureType_BASE_COLOR, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|TEX_normal_map|file", aiTextureType_NORMAL_CAMERA, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|TEX_emissive_map|file", aiTextureType_EMISSION_COLOR, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|TEX_metallic_map|file", aiTextureType_METALNESS, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|TEX_roughness_map|file", aiTextureType_DIFFUSE_ROUGHNESS, mesh);
            TrySetTextureProperties(out_mat, textures, "Maya|TEX_ao_map|file", aiTextureType_AMBIENT_OCCLUSION, mesh);            
        }

        void FBXConverter::SetTextureProperties(aiMaterial* out_mat, const LayeredTextureMap& layeredTextures, const MeshGeometry* const mesh)
        {
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
			TrySetTextureProperties( out_mat, layeredTextures, "EmissiveFactor", aiTextureType_EMISSIVE, mesh );
			TrySetTextureProperties( out_mat, layeredTextures, "TransparencyFactor", aiTextureType_OPACITY, mesh );
        }

        aiColor3D FBXConverter::GetColorPropertyFactored(const PropertyTable& props, const std::string& colorName,
            const std::string& factorName, bool& result, bool useTemplate)
        {
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

        aiColor3D FBXConverter::GetColorPropertyFromMaterial(const PropertyTable& props, const std::string& baseName,
            bool& result)
        {
            return GetColorPropertyFactored(props, baseName + "Color", baseName + "Factor", result, true);
        }

        aiColor3D FBXConverter::GetColorProperty(const PropertyTable& props, const std::string& colorName,
            bool& result, bool useTemplate)
        {
            result = true;
            bool ok;
            const aiVector3D& ColorVec = PropertyGet<aiVector3D>(props, colorName, ok, useTemplate);
            if (!ok) {
                result = false;
                return aiColor3D(0.0f, 0.0f, 0.0f);
            }
            return aiColor3D(ColorVec.x, ColorVec.y, ColorVec.z);
        }

        void FBXConverter::SetShadingPropertiesCommon(aiMaterial* out_mat, const PropertyTable& props)
        {
            // Set shading properties.
            // Modern FBX Files have two separate systems for defining these,
            // with only the more comprehensive one described in the property template.
            // Likely the other values are a legacy system,
            // which is still always exported by the official FBX SDK.
            //
            // Blender's FBX import and export mostly ignore this legacy system,
            // and as we only support recent versions of FBX anyway, we can do the same.
            bool ok;

            const aiColor3D& Diffuse = GetColorPropertyFromMaterial(props, "Diffuse", ok);
            if (ok) {
                out_mat->AddProperty(&Diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
            }

            const aiColor3D& Emissive = GetColorPropertyFromMaterial(props, "Emissive", ok);
            if (ok) {
                out_mat->AddProperty(&Emissive, 1, AI_MATKEY_COLOR_EMISSIVE);
            }

            const aiColor3D& Ambient = GetColorPropertyFromMaterial(props, "Ambient", ok);
            if (ok) {
                out_mat->AddProperty(&Ambient, 1, AI_MATKEY_COLOR_AMBIENT);
            }

            // we store specular factor as SHININESS_STRENGTH, so just get the color
            const aiColor3D& Specular = GetColorProperty(props, "SpecularColor", ok, true);
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
            }

            // TransparentColor / TransparencyFactor... gee thanks FBX :rolleyes:
            const aiColor3D& Transparent = GetColorPropertyFactored(props, "TransparentColor", "TransparencyFactor", ok);
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
            }
            else if (CalculatedOpacity != 1.0) {
                out_mat->AddProperty(&CalculatedOpacity, 1, AI_MATKEY_OPACITY);
            }

            // reflection color and factor are stored separately
            const aiColor3D& Reflection = GetColorProperty(props, "ReflectionColor", ok, true);
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
}


void FBXConverter::SetShadingPropertiesRaw(aiMaterial* out_mat, const PropertyTable& props, const TextureMap& textures, const MeshGeometry* const mesh)
{
    // Add all the unparsed properties with a "$raw." prefix

    const std::string prefix = "$raw.";

    for (const DirectPropertyMap::value_type& prop : props.GetUnparsedProperties()) {

        std::string name = prefix + prop.first;

        if (const TypedProperty<aiVector3D>* interpreted = prop.second->As<TypedProperty<aiVector3D> >())
        {
            out_mat->AddProperty(&interpreted->Value(), 1, name.c_str(), 0, 0);
        }
        else if (const TypedProperty<aiColor3D>* interpreted = prop.second->As<TypedProperty<aiColor3D> >())
        {
            out_mat->AddProperty(&interpreted->Value(), 1, name.c_str(), 0, 0);
        }
        else if (const TypedProperty<aiColor4D>* interpreted = prop.second->As<TypedProperty<aiColor4D> >())
        {
            out_mat->AddProperty(&interpreted->Value(), 1, name.c_str(), 0, 0);
        }
        else if (const TypedProperty<float>* interpreted = prop.second->As<TypedProperty<float> >())
        {
            out_mat->AddProperty(&interpreted->Value(), 1, name.c_str(), 0, 0);
        }
        else if (const TypedProperty<int>* interpreted = prop.second->As<TypedProperty<int> >())
        {
            out_mat->AddProperty(&interpreted->Value(), 1, name.c_str(), 0, 0);
        }
        else if (const TypedProperty<bool>* interpreted = prop.second->As<TypedProperty<bool> >())
        {
            int value = interpreted->Value() ? 1 : 0;
            out_mat->AddProperty(&value, 1, name.c_str(), 0, 0);
        }
        else if (const TypedProperty<std::string>* interpreted = prop.second->As<TypedProperty<std::string> >())
        {
            const aiString value = aiString(interpreted->Value());
            out_mat->AddProperty(&value, name.c_str(), 0, 0);
        }
    }

    // Add the textures' properties

    for (TextureMap::const_iterator it = textures.begin(); it != textures.end(); it++) {

        std::string name = prefix + it->first;

        const Texture* const tex = (*it).second;
        if (tex != nullptr)
        {
            aiString path;
            path.Set(tex->RelativeFilename());

            const Video* media = tex->Media();
            if (media != nullptr && media->ContentLength() > 0) {
                unsigned int index;

                VideoMap::const_iterator it = textures_converted.find(media);
                if (it != textures_converted.end()) {
                    index = (*it).second;
                }
                else {
                    index = ConvertVideo(*media);
                    textures_converted[media] = index;
                }

                // setup texture reference string (copied from ColladaLoader::FindFilenameForEffectTexture)
                path.data[0] = '*';
                path.length = 1 + ASSIMP_itoa10(path.data + 1, MAXLEN - 1, index);
            }

            out_mat->AddProperty(&path, (name + "|file").c_str(), aiTextureType_UNKNOWN, 0);

            aiUVTransform uvTrafo;
            // XXX handle all kinds of UV transformations
            uvTrafo.mScaling = tex->UVScaling();
            uvTrafo.mTranslation = tex->UVTranslation();
            out_mat->AddProperty(&uvTrafo, 1, (name + "|uvtrafo").c_str(), aiTextureType_UNKNOWN, 0);
 
            int uvIndex = 0;

            bool uvFound = false;
            const std::string& uvSet = PropertyGet<std::string>(tex->Props(), "UVSet", uvFound);
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

                    std::vector<aiMaterial*>::iterator materialIt = std::find(materials.begin(), materials.end(), out_mat);
                    const unsigned int matIndex = static_cast<unsigned int>(std::distance(materials.begin(), materialIt));

                    uvIndex = -1;
                    if (!mesh)
                    {
                        for (const MeshMap::value_type& v : meshes_converted) {
                            const MeshGeometry* const meshGeom = dynamic_cast<const MeshGeometry*>(v.first);
                            if (!meshGeom) {
                                continue;
                            }

                            const MatIndexArray& mats = meshGeom->GetMaterialIndices();
                            if (std::find(mats.begin(), mats.end(), matIndex) == mats.end()) {
                                continue;
                            }

                            int index = -1;
                            for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                                if (meshGeom->GetTextureCoords(i).empty()) {
                                    break;
                                }
                                const std::string& name = meshGeom->GetTextureCoordChannelName(i);
                                if (name == uvSet) {
                                    index = static_cast<int>(i);
                                    break;
                                }
                            }
                            if (index == -1) {
                                FBXImporter::LogWarn("did not find UV channel named " + uvSet + " in a mesh using this material");
                                continue;
                            }

                            if (uvIndex == -1) {
                                uvIndex = index;
                            }
                            else {
                                FBXImporter::LogWarn("the UV channel named " + uvSet + " appears at different positions in meshes, results will be wrong");
                            }
                        }
                    }
                    else
                    {
                        int index = -1;
                        for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                            if (mesh->GetTextureCoords(i).empty()) {
                                break;
                            }
                            const std::string& name = mesh->GetTextureCoordChannelName(i);
                            if (name == uvSet) {
                                index = static_cast<int>(i);
                                break;
                            }
                        }
                        if (index == -1) {
                            FBXImporter::LogWarn("did not find UV channel named " + uvSet + " in a mesh using this material");
                        }

                        if (uvIndex == -1) {
                            uvIndex = index;
                        }
                    }

                    if (uvIndex == -1) {
                        FBXImporter::LogWarn("failed to resolve UV channel " + uvSet + ", using first UV channel");
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


        void FBXConverter::ConvertAnimations()
        {
            // first of all determine framerate
            const FileGlobalSettings::FrameRate fps = doc.GlobalSettings().TimeMode();
            const float custom = doc.GlobalSettings().CustomFrameRate();
            anim_fps = FrameRateToDouble(fps, custom);

            const std::vector<const AnimationStack*>& animations = doc.AnimationStacks();
            for (const AnimationStack* stack : animations) {
                ConvertAnimationStack(*stack);
            }
        }

        std::string FBXConverter::FixNodeName(const std::string& name) {
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

        std::string FBXConverter::FixAnimMeshName(const std::string& name) {
            if (name.length()) {
                size_t indexOf = name.find_first_of("::");
                if (indexOf != std::string::npos && indexOf < name.size() - 2) {
                    return name.substr(indexOf + 2);
                }
            }
            return name.length() ? name : "AnimMesh";
        }

        void FBXConverter::ConvertAnimationStack(const AnimationStack& st)
        {
            const AnimationLayerList& layers = st.Layers();
            if (layers.empty()) {
                return;
            }

            aiAnimation* const anim = new aiAnimation();
            animations.push_back(anim);

            // strip AnimationStack:: prefix
            std::string name = st.Name();
            if (name.substr(0, 16) == "AnimationStack::") {
                name = name.substr(16);
            }
            else if (name.substr(0, 11) == "AnimStack::") {
                name = name.substr(11);
            }

            anim->mName.Set(name);

            // need to find all nodes for which we need to generate node animations -
            // it may happen that we need to merge multiple layers, though.
            NodeMap node_map;

            // reverse mapping from curves to layers, much faster than querying
            // the FBX DOM for it.
            LayerMap layer_map;

            const char* prop_whitelist[] = {
                "Lcl Scaling",
                "Lcl Rotation",
                "Lcl Translation",
                "DeformPercent"
            };

            std::map<std::string, morphAnimData*> morphAnimDatas;

            for (const AnimationLayer* layer : layers) {
                ai_assert(layer);
                const AnimationCurveNodeList& nodes = layer->Nodes(prop_whitelist, 4);
                for (const AnimationCurveNode* node : nodes) {
                    ai_assert(node);
                    const Model* const model = dynamic_cast<const Model*>(node->Target());
                    if (model) {
                        const std::string& name = FixNodeName(model->Name());
                        node_map[name].push_back(node);
                        layer_map[node] = layer;
                        continue;
                    }
                    const BlendShapeChannel* const bsc = dynamic_cast<const BlendShapeChannel*>(node->Target());
                    if (bsc) {
                        ProcessMorphAnimDatas(&morphAnimDatas, bsc, node);
                    }
                }
            }

            // generate node animations
            std::vector<aiNodeAnim*> node_anims;

            int64_t start_time = st.LocalStart();
            int64_t stop_time = st.LocalStop();
            bool has_local_startstop = start_time != 0 || stop_time != 0;

            ASSIMP_LOG_DEBUG_F("Has local start stop? %s\n", has_local_startstop ? "yes" : "no" );

            // Goal we need the number of frames passed
            double start_time_frame_number = has_local_startstop ? (CONVERT_FBX_TIME_TO_SECONDS(start_time) * anim_fps) : 0;
            double stop_time_frame_number = has_local_startstop ? (CONVERT_FBX_TIME_TO_SECONDS(stop_time) * anim_fps): DBL_MAX;

            try {
                for (const NodeMap::value_type& kv : node_map) {
                    GenerateNodeAnimations(node_anims,
                        kv.first,
                        kv.second,
                        layer_map,
                        start_time_frame_number,
                        stop_time_frame_number);
                }
            }
            catch (std::exception&) {
                std::for_each(node_anims.begin(), node_anims.end(), Util::delete_fun<aiNodeAnim>());
                throw;
            }

            if (node_anims.size() || morphAnimDatas.size()) {
                if (node_anims.size()) {
                    anim->mChannels = new aiNodeAnim*[node_anims.size()]();
                    anim->mNumChannels = static_cast<unsigned int>(node_anims.size());
                    std::swap_ranges(node_anims.begin(), node_anims.end(), anim->mChannels);
                }
                if (morphAnimDatas.size()) {
                    unsigned int numMorphMeshChannels = static_cast<unsigned int>(morphAnimDatas.size());
                    anim->mMorphMeshChannels = new aiMeshMorphAnim*[numMorphMeshChannels];
                    anim->mNumMorphMeshChannels = numMorphMeshChannels;
                    unsigned int i = 0;
                    for (auto morphAnimIt : morphAnimDatas) {
                        morphAnimData* animData = morphAnimIt.second;
                        unsigned int numKeys = static_cast<unsigned int>(animData->size());
                        aiMeshMorphAnim* meshMorphAnim = new aiMeshMorphAnim();
                        meshMorphAnim->mName.Set(morphAnimIt.first);
                        meshMorphAnim->mNumKeys = numKeys;
                        meshMorphAnim->mKeys = new aiMeshMorphKey[numKeys];
                        unsigned int j = 0;
                        for (auto animIt : *animData) {
                            morphKeyData* keyData = animIt.second;
                            unsigned int numValuesAndWeights = static_cast<unsigned int>(keyData->values.size());
                            meshMorphAnim->mKeys[j].mNumValuesAndWeights = numValuesAndWeights;
                            meshMorphAnim->mKeys[j].mValues = new unsigned int[numValuesAndWeights];
                            meshMorphAnim->mKeys[j].mWeights = new double[numValuesAndWeights];

                            meshMorphAnim->mKeys[j].mTime = CONVERT_FBX_TIME_TO_FRAMES(animIt.first, anim_fps);
                            for (unsigned int k = 0; k < numValuesAndWeights; k++) {
                                meshMorphAnim->mKeys[j].mValues[k] = keyData->values.at(k);
                                meshMorphAnim->mKeys[j].mWeights[k] = keyData->weights.at(k);
                            }
                            j++;
                        }
                        anim->mMorphMeshChannels[i++] = meshMorphAnim;
                    }
                }
            }
            else {
                // empty animations would fail validation, so drop them
                delete anim;
                animations.pop_back();
                FBXImporter::LogInfo("ignoring empty AnimationStack (using IK?): " + name);
                return;
            }


            // // adjust relative timing for animation
            // for (unsigned int c = 0; c < anim->mNumChannels; c++) {
            //     aiNodeAnim* channel = anim->mChannels[c];
            //     for (uint32_t i = 0; i < channel->mNumPositionKeys; i++) {
            //         channel->mPositionKeys[i].mTime -= start_time_fps;
            //     }
            //     for (uint32_t i = 0; i < channel->mNumRotationKeys; i++) {
            //         channel->mRotationKeys[i].mTime -= start_time_fps;
            //     }
            //     for (uint32_t i = 0; i < channel->mNumScalingKeys; i++) {
            //         channel->mScalingKeys[i].mTime -= start_time_fps;
            //     }
            // }
            // for (unsigned int c = 0; c < anim->mNumMorphMeshChannels; c++) {
            //     aiMeshMorphAnim* channel = anim->mMorphMeshChannels[c];
            //     for (uint32_t i = 0; i < channel->mNumKeys; i++) {
            //         channel->mKeys[i].mTime -= start_time_fps;
            //     }
            // }

            // for some mysterious reason, mDuration is simply the maximum key -- the
            // validator always assumes animations to start at zero.
            // todo: this is broken

            // Basis use FBX method
            // anim->mDuration = stop_time - start_time;
            // anim->mTicksPerSecond = FBX_ONE_SECOND;

            anim->mDuration = stop_time_frame_number;
            anim->mTicksPerSecond = anim_fps;
        }

        // ------------------------------------------------------------------------------------------------
        void FBXConverter::ProcessMorphAnimDatas(std::map<std::string, morphAnimData*>* morphAnimDatas, const BlendShapeChannel* bsc, const AnimationCurveNode* node) {
            std::vector<const Connection*> bscConnections = doc.GetConnectionsBySourceSequenced(bsc->ID(), "Deformer");
            for (const Connection* bscConnection : bscConnections) {
                auto bs = dynamic_cast<const BlendShape*>(bscConnection->DestinationObject());
                if (bs) {
                    auto channelIt = std::find(bs->BlendShapeChannels().begin(), bs->BlendShapeChannels().end(), bsc);
                    if (channelIt != bs->BlendShapeChannels().end()) {
                        auto channelIndex = static_cast<unsigned int>(std::distance(bs->BlendShapeChannels().begin(), channelIt));
                        std::vector<const Connection*> bsConnections = doc.GetConnectionsBySourceSequenced(bs->ID(), "Geometry");
                        for (const Connection* bsConnection : bsConnections) {
                            auto geo = dynamic_cast<const Geometry*>(bsConnection->DestinationObject());
                            if (geo) {
                                std::vector<const Connection*> geoConnections = doc.GetConnectionsBySourceSequenced(geo->ID(), "Model");
                                for (const Connection* geoConnection : geoConnections) {
                                    auto model = dynamic_cast<const Model*>(geoConnection->DestinationObject());
                                    if (model) {
                                        auto geoIt = std::find(model->GetGeometry().begin(), model->GetGeometry().end(), geo);
                                        auto geoIndex = static_cast<unsigned int>(std::distance(model->GetGeometry().begin(), geoIt));
                                        auto name = aiString(FixNodeName(model->Name() + "*"));
                                        name.length = 1 + ASSIMP_itoa10(name.data + name.length, MAXLEN - 1, geoIndex);
                                        morphAnimData* animData;
                                        auto animIt = morphAnimDatas->find(name.C_Str());
                                        if (animIt == morphAnimDatas->end()) {
                                            animData = new morphAnimData();
                                            morphAnimDatas->insert(std::make_pair(name.C_Str(), animData));
                                        }
                                        else {
                                            animData = animIt->second;
                                        }
                                        
                                        for (std::pair<std::string, const AnimationCurve*> curvesIt : node->Curves()) {
                                            if (curvesIt.first == "d|DeformPercent") {
                                                const AnimationCurve* animationCurve = curvesIt.second;
                                                
                                                unsigned int k = 0;
                                                for (auto key_value_pair : animationCurve->GetKeyframeData()) {
                                                    
                                                    morphKeyData* keyData;

                                                    // does our anim key already exist? if not create it!
                                                    auto keyIt = animData->find(key_value_pair.first);
                                                    if (keyIt == animData->end()) {
                                                        keyData = new morphKeyData();
                                                        animData->insert(std::make_pair(key_value_pair.first, keyData));
                                                    }
                                                    else {
                                                        keyData = keyIt->second;
                                                    }
                                                    keyData->values.push_back(channelIndex);
                                                    keyData->weights.push_back(key_value_pair.second / 100.0f);
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
        static void validateAnimCurveNodes(const std::vector<const AnimationCurveNode*>& curves,
            bool strictMode) {
            const Object* target(NULL);
            for (const AnimationCurveNode* node : curves) {
                if( node->Curves().size() == 0 || node->Target() == nullptr) continue; // prevents dumb data becoming part of the equation
                if (!target) {
                    target = node->Target();
                }
                if (node->Target() != target) {
                    FBXImporter::LogWarn("Node target is nullptr type.");
                }                

                if (strictMode) {
                    if(target == nullptr) continue;
                }
            }
        }
#endif // ASSIMP_BUILD_DEBUG



    // can be expanded for other use cases
    FBXConverter::FBX_PROPERTY_TYPE GetFBXPropertyType( const std::string& property_name )
    {
        try
        {
            if(property_name.compare("d|X") == 0)
            {
                return FBXConverter::X_AXIS;                           
            }
            else if(property_name.compare("d|Y") == 0)
            {
                return FBXConverter::Y_AXIS;
            }
            else if(property_name.compare("d|Z") == 0)
            {
                return FBXConverter::Z_AXIS;          
            }
            else if(property_name.compare("Lcl Translation") == 0)
            {
                return FBXConverter::Translation;
            }
            else if(property_name.compare("Lcl Rotation") == 0)
            {
               return FBXConverter::Rotation;
            }
            else if(property_name.compare("Lcl Scaling") == 0)
            {
                return FBXConverter::Scale;
            }
            
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        
        return FBXConverter::UNKNOWN;
    }

        

        // ------------------------------------------------------------------------------------------------
        void FBXConverter::GenerateNodeAnimations(std::vector<aiNodeAnim*>& node_anims,
            const std::string& fixed_name,
            const std::vector<const AnimationCurveNode*>& curves,
            const LayerMap& layer_map,
            double& start_time,
            double& end_time)
        {

            NodeMap node_property_map;
            ai_assert(curves.size());

#ifdef ASSIMP_BUILD_DEBUG
            validateAnimCurveNodes(curves, true);
#endif

            // make new animation to hold this animations keyframes
            aiNodeAnim * nodeAnim = new aiNodeAnim();
            nodeAnim->mNumPositionKeys = 0;
            nodeAnim->mNumRotationKeys = 0;
            nodeAnim->mNumScalingKeys = 0;

            // simple position, scale and rotation keys
            // each will have a target, and if they have any duplicate target they will write to the value of the key.
            std::map<const int64_t, aiVectorKey*> position_keys;
            std::map<const int64_t, aiVectorKey*> rotation_keys;
            std::map<const int64_t, aiVectorKey*> scale_keys;

            for( const AnimationCurveNode* curve : curves)
            {
                ASSIMP_LOG_DEBUG_F("[Header] Curve from fbx: ID %d Name: %s\n", curve->ID(), curve->Name().c_str());
                
                const Object *target = curve->Target();
                std::string property_type = curve->TargetProperty();

                ASSIMP_LOG_DEBUG_F("-- Target property: %s\n", curve->TargetProperty().c_str());
                
                // I believe that an invalid target could 
                // still be of use in certain cases so we 
                // must keep this data for now.
                if(target == nullptr)
                {
                    ASSIMP_LOG_DEBUG_F("-- [WARNING-Serious] Invalid node target: %d\n", curve->ID());
                    continue; // skip this to make sure we handle this case.
                }
                else
                {
                    ASSIMP_LOG_DEBUG_F("-- Valid node target: %d\n", curve->ID());
                }


                // std::map<std::string, const AnimationCurve*>
                // please note this is an std::map
                AnimationCurveMap map = curve->Curves();
                AnimationCurveMap::iterator itr;


                for( itr = map.begin(); itr != map.end(); ++itr)
                {
                    std::string subPropertyName = itr->first;
                    const AnimationCurve* subCurve = itr->second;
                    ASSIMP_LOG_DEBUG_F("-- SubCurve %s vs sub curve name %s\n", subPropertyName.c_str(), subCurve->Name().c_str());
                    
                    // todo make into dict
                    const std::map<int64_t, float>& keyframes = subCurve->GetKeyframeData();
                    ASSIMP_LOG_DEBUG_F("keyframe count: %ld\n", keyframes.size());
                    // subCurve->GetKeys()
                    // filter node

                    // note target match is wrong.
                    switch (GetFBXPropertyType(property_type))
                    {
                        case Translation:                        
                            for( std::pair<const int64_t, float> keyframe_data : keyframes )
                            {
                                aiVectorKey *key;
                                if( position_keys.count( keyframe_data.first ) )
                                {
                                    key = position_keys[keyframe_data.first];
                                    ASSIMP_LOG_DEBUG_F("Found pre-existing pos key for sub curve %ld\n", keyframe_data.first);
                                }
                                else
                                {
                                    key = new aiVectorKey();
                                    position_keys.insert( std::pair<const int64_t, aiVectorKey*>(keyframe_data.first, key) );
                                    ASSIMP_LOG_DEBUG_F("Created pos key for sub curve %ld\n", keyframe_data.first);
                                }                         
                               
                                key->mTime = CONVERT_FBX_TIME_TO_FRAMES(keyframe_data.first, anim_fps);

                                switch ( GetFBXPropertyType( subPropertyName ) )
                                {
                                    case X_AXIS:
                                        key->mValue.x = keyframe_data.second;
                                    break;
                                    case Y_AXIS:
                                        key->mValue.y = keyframe_data.second;
                                    break;                                    
                                    case Z_AXIS:
                                        key->mValue.z = keyframe_data.second;
                                    break;
                                }
                            }
                        break;
                        case Rotation:
                        for( std::pair<const int64_t, float> keyframe_data : keyframes )
                            {
                                aiVectorKey *key;
                                if( rotation_keys.count( keyframe_data.first ) )
                                {
                                    key = rotation_keys[keyframe_data.first];
                                    ASSIMP_LOG_DEBUG_F("Found pre-existing rot key for sub curve %ld\n", keyframe_data.first);
                                }
                                else
                                {

                                    key = new aiVectorKey();
                                    rotation_keys.insert( std::pair<const int64_t, aiVectorKey*>(keyframe_data.first, key) );
                                    ASSIMP_LOG_DEBUG_F("Created rot key for sub curve %ld\n", keyframe_data.first);
                                }

                                // set key frame time
                                key->mTime = CONVERT_FBX_TIME_TO_FRAMES(keyframe_data.first, anim_fps);

                                switch ( GetFBXPropertyType( subPropertyName ) )
                                {
                                    case X_AXIS:
                                        key->mValue.x = keyframe_data.second;
                                    break;
                                    case Y_AXIS:
                                        key->mValue.y = keyframe_data.second;
                                    break;                                    
                                    case Z_AXIS:
                                        key->mValue.z = keyframe_data.second;
                                    break;
                                }
                            }
                        break;
                        case Scale:
                            for( std::pair<const int64_t, float> keyframe_data : keyframes )
                            {
                                aiVectorKey *key;
                                if( scale_keys.count( keyframe_data.first ) )
                                {
                                    key = scale_keys[keyframe_data.first];
                                    ASSIMP_LOG_DEBUG_F("Found pre-existing scale key for sub curve %ld\n", keyframe_data.first);
                                }
                                else
                                {
                                    key = new aiVectorKey();
                                    scale_keys.insert( std::pair<const int64_t, aiVectorKey*>(keyframe_data.first, key) );
                                    ASSIMP_LOG_DEBUG_F("Created scale key for sub curve %ld\n", keyframe_data.first);
                                }                                


                                key->mTime = CONVERT_FBX_TIME_TO_FRAMES(keyframe_data.first, anim_fps);

                                switch ( GetFBXPropertyType( subPropertyName ) )
                                {
                                    case X_AXIS:
                                        key->mValue.x = keyframe_data.second;
                                    break;
                                    case Y_AXIS:
                                        key->mValue.y = keyframe_data.second;
                                    break;                                    
                                    case Z_AXIS:
                                        key->mValue.z = keyframe_data.second;
                                    break;
                                }
                            }
                        break;
                        default:
                        break;
                    }
                }

                std::cout << std::endl;               

                // todo: container which can contain arbitrary keyframe data?
                // goal: convert FBX data to assimp aiAnimation.
                // goal: only include what is needed for the animation
            }

            std::map<const int64_t, aiQuatKey*> real_rotation_keys;
            // goal convert rotation keys into quaternion keys from euler (which is in FBX)
            for( std::pair<const int64_t, aiVectorKey*> rot_key : rotation_keys)
            {
                auto quat_key = new aiQuatKey();
                quat_key->mValue = EulerToQuaternion(rot_key.second->mValue, Model::RotOrder::RotOrder_EulerXYZ);
                quat_key->mTime = rot_key.second->mTime;
                real_rotation_keys.insert( std::pair<const int64_t, aiQuatKey*> (rot_key.first, quat_key ));  
                delete rot_key.second; // clear allocation          
            }

            // this list is no longer required free it.
            rotation_keys.clear();

            ai_assert(nodeAnim); // if new fails then we need to report this asap and crash?
            nodeAnim->mNodeName = fixed_name;
            nodeAnim->mNumPositionKeys = position_keys.size();
            nodeAnim->mNumRotationKeys = real_rotation_keys.size();
            nodeAnim->mNumScalingKeys = scale_keys.size();

            /*
            /* Finalization stage
             */

            nodeAnim->mPositionKeys = new aiVectorKey[nodeAnim->mNumPositionKeys];
            int ordered_insert = 0;
            for (std::pair<const int64_t, aiVectorKey*> pos_key : position_keys)
            {   
                // why does this happen? 
                // we need to remove keys the artist purposefully didn't include in the track :)
                // 120 frame number
                // 119.001 duration
                if( pos_key.second->mTime >= start_time && pos_key.second->mTime <= end_time )
                {                
                    nodeAnim->mPositionKeys[ordered_insert] = *pos_key.second;
                    ++ordered_insert;
                }
                delete pos_key.second;
            }

            nodeAnim->mScalingKeys = new aiVectorKey[nodeAnim->mNumScalingKeys];
            ordered_insert = 0;
            for (std::pair<const int64_t, aiVectorKey*> scale_key : scale_keys)
            {
                if( scale_key.second->mTime >= start_time && scale_key.second->mTime <= end_time )
                { 
                    nodeAnim->mScalingKeys[ordered_insert] = *scale_key.second;
                    ++ordered_insert;
                }
                delete scale_key.second;
            }

            nodeAnim->mRotationKeys = new aiQuatKey[nodeAnim->mNumRotationKeys];
            ordered_insert = 0;
            for (std::pair<const int64_t, aiQuatKey*> rot_key : real_rotation_keys)
            {
                if(rot_key.second->mTime >= start_time && rot_key.second->mTime <= end_time)
                { 
                    nodeAnim->mRotationKeys[ordered_insert] = *rot_key.second;
                    ++ordered_insert;
                }
                delete rot_key.second;
            }

            if(real_rotation_keys.size() > 0 || position_keys.size() > 0 || scale_keys.size() > 0)
            {
                node_anims.push_back( nodeAnim );
            }
            else
            {
                delete nodeAnim;
            }            
        }

        aiQuaternion FBXConverter::EulerToQuaternion(const aiVector3D& rot, Model::RotOrder order)
        {
            aiMatrix4x4 m;
            GetRotationMatrix(order, rot, m);

            return aiQuaternion(aiMatrix3x3(m));
        }

        void FBXConverter::ConvertGlobalSettings() {
            if (nullptr == out) {
                return;
            }

            out->mMetaData = aiMetadata::Alloc(15);
            out->mMetaData->Set(0, "UpAxis", doc.GlobalSettings().UpAxis());
            out->mMetaData->Set(1, "UpAxisSign", doc.GlobalSettings().UpAxisSign());
            out->mMetaData->Set(2, "FrontAxis", doc.GlobalSettings().FrontAxis());
            out->mMetaData->Set(3, "FrontAxisSign", doc.GlobalSettings().FrontAxisSign());
            out->mMetaData->Set(4, "CoordAxis", doc.GlobalSettings().CoordAxis());
            out->mMetaData->Set(5, "CoordAxisSign", doc.GlobalSettings().CoordAxisSign());
            out->mMetaData->Set(6, "OriginalUpAxis", doc.GlobalSettings().OriginalUpAxis());
            out->mMetaData->Set(7, "OriginalUpAxisSign", doc.GlobalSettings().OriginalUpAxisSign());
            out->mMetaData->Set(8, "UnitScaleFactor", (double)doc.GlobalSettings().UnitScaleFactor());
            out->mMetaData->Set(9, "OriginalUnitScaleFactor", doc.GlobalSettings().OriginalUnitScaleFactor());
            out->mMetaData->Set(10, "AmbientColor", doc.GlobalSettings().AmbientColor());
            out->mMetaData->Set(11, "FrameRate", (int)doc.GlobalSettings().TimeMode());
            out->mMetaData->Set(12, "TimeSpanStart", doc.GlobalSettings().TimeSpanStart());
            out->mMetaData->Set(13, "TimeSpanStop", doc.GlobalSettings().TimeSpanStop());
            out->mMetaData->Set(14, "CustomFrameRate", doc.GlobalSettings().CustomFrameRate());
        }

        void FBXConverter::TransferDataToScene()
        {
            ai_assert(!out->mMeshes);
            ai_assert(!out->mNumMeshes);

            // note: the trailing () ensures initialization with NULL - not
            // many C++ users seem to know this, so pointing it out to avoid
            // confusion why this code works.

            if (meshes.size()) {
                out->mMeshes = new aiMesh*[meshes.size()]();
                out->mNumMeshes = static_cast<unsigned int>(meshes.size());

                std::swap_ranges(meshes.begin(), meshes.end(), out->mMeshes);
            }

            if (materials.size()) {
                out->mMaterials = new aiMaterial*[materials.size()]();
                out->mNumMaterials = static_cast<unsigned int>(materials.size());

                std::swap_ranges(materials.begin(), materials.end(), out->mMaterials);
            }

            if (animations.size()) {
                out->mAnimations = new aiAnimation*[animations.size()]();
                out->mNumAnimations = static_cast<unsigned int>(animations.size());

                std::swap_ranges(animations.begin(), animations.end(), out->mAnimations);
            }

            if (lights.size()) {
                out->mLights = new aiLight*[lights.size()]();
                out->mNumLights = static_cast<unsigned int>(lights.size());

                std::swap_ranges(lights.begin(), lights.end(), out->mLights);
            }

            if (cameras.size()) {
                out->mCameras = new aiCamera*[cameras.size()]();
                out->mNumCameras = static_cast<unsigned int>(cameras.size());

                std::swap_ranges(cameras.begin(), cameras.end(), out->mCameras);
            }

            if (textures.size()) {
                out->mTextures = new aiTexture*[textures.size()]();
                out->mNumTextures = static_cast<unsigned int>(textures.size());

                std::swap_ranges(textures.begin(), textures.end(), out->mTextures);
            }
        }

        // ------------------------------------------------------------------------------------------------
        void ConvertToAssimpScene(aiScene* out, const Document& doc, bool removeEmptyBones)
        {
            FBXConverter converter(out, doc, removeEmptyBones);
        }

    } // !FBX
} // !Assimp

#endif
