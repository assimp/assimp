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

#pragma once
#ifndef OBJ_FILEDATA_H_INC
#define OBJ_FILEDATA_H_INC

#include <assimp/mesh.h>
#include <assimp/types.h>
#include <map>
#include <vector>
#include "Common/Maybe.h"

namespace Assimp::ObjFile {

struct Object;
struct Face;
struct Material;

// ------------------------------------------------------------------------------------------------
/// @struct Face
/// @brief  Data structure for a simple obj-face, describes discredit,l.ation and materials
// ------------------------------------------------------------------------------------------------
struct Face {
    using IndexArray = std::vector<unsigned int>;

    aiPrimitiveType mPrimitiveType{aiPrimitiveType_TRIANGLE};   ///< Primitive type
    IndexArray mVertices{};                                     ///< Vertex indices
    IndexArray mNormals{};                                      ///< Normal indices
    IndexArray mTextureCoords{};                                ///< Texture coordinates indices
    Material *mMaterial{nullptr};                               ///< Pointer to assigned material

    /// @brief  Default constructor with default primitive type.
    /// @param [in] pt  Primitive type.
    explicit Face(aiPrimitiveType pt = aiPrimitiveType_POLYGON) :
            mPrimitiveType(pt) {
        // empty
    }

    ~Face() = default;
};

// ------------------------------------------------------------------------------------------------
/// @struct Object
/// @brief  Stores all objects of an obj-file object definition
// ------------------------------------------------------------------------------------------------
struct Object {
    /// @brief  Object type.
    enum ObjectType {
        ObjType,
        GroupType
    };

    std::string mObjName{};                 ///< Object name
    aiMatrix4x4 mTransformation{};          ///< Transformation matrix, stored in OpenGL format
    std::vector<Object *> mSubObjects{};    ///< All sub-objects referenced by this object
    std::vector<unsigned int> mMeshes{};    ///< Assigned meshes

    Object() = default;
    ~Object() {
        for (auto it = mSubObjects.begin(); it != mSubObjects.end(); ++it) {
            delete *it;
        }
    }
};

// ------------------------------------------------------------------------------------------------
/// @struct Material
/// @brief  Data structure to store all material specific data
// ------------------------------------------------------------------------------------------------
struct Material {
    aiString MaterialName;          ///< Name of material description
    aiString texture;               ///< Texture names
    aiString textureSpecular;       ///< Specular texture name
    aiString textureAmbient;        ///< Ambient texture name
    aiString textureEmissive;       ///< Emissive texture name
    aiString textureBump;           ///< Bump texture name
    aiString textureNormal;         ///< Normal texture name
    aiString textureReflection[6];  ///< Reflection texture names
    aiString textureSpecularity;    ///< Specularity texture names
    aiString textureOpacity;        ///< Opacity texture names
    aiString textureDisp;           ///< Displaycement  texture names
    aiString textureRoughness;      ///< Roughness texture names
    aiString textureMetallic;       ///< Metallic texture names
    aiString textureSheen;          ///< Sheen texture names
    aiString textureRMA;            ///< RMA  texture names

    /// @brief  Texture types enum.
    enum TextureType {
        TextureDiffuseType = 0,
        TextureSpecularType,
        TextureAmbientType,
        TextureEmissiveType,
        TextureBumpType,
        TextureNormalType,
        TextureReflectionSphereType,
        TextureReflectionCubeTopType,
        TextureReflectionCubeBottomType,
        TextureReflectionCubeFrontType,
        TextureReflectionCubeBackType,
        TextureReflectionCubeLeftType,
        TextureReflectionCubeRightType,
        TextureSpecularityType,
        TextureOpacityType,
        TextureDispType,
        TextureRoughnessType,
        TextureMetallicType,
        TextureSheenType,
        TextureRMAType,
        TextureTypeCount
    };
    bool clamp[TextureTypeCount];

    ///< Ambient color
    aiColor3D ambient{};
    ///< Diffuse color
    aiColor3D diffuse = aiColor3D(0.6f, 0.6f, 0.6f);
    ///< Specular color
    aiColor3D specular{};
    ///< Emissive color
    aiColor3D emissive{};
    ///< Alpha value
    ai_real alpha{1};
    ///< Shineness factor
    ai_real shineness = ai_real(0.0);
    ///< Illumination model
    int illumination_model{1};
    ///< Index of refraction
    ai_real ior{1};
    ///< Transparency color
    aiColor3D transparent = aiColor3D(1.0f, 1.0f, 1.0f);
    ///< Ambient occlusion
    Maybe<ai_real> ambient_occlusion{};
    ///< PBR Roughness
    Maybe<ai_real> roughness{};
    ///< PBR Metallic
    Maybe<ai_real> metallic{};
    ///< PBR Metallic
    Maybe<aiColor3D> sheen{};
    ///< PBR Sheen: an additional grazing component, primarily intended for cloth.
    Maybe<ai_real> sheen_grazing{};
    ///< PBR Sheen Tint: amount to tint sheen towards base color.
    Maybe<ai_real> sheen_tint{};
    ///< PBR Clearcoat
    Maybe<ai_real> clearcoat{};
    ///< PBR Clearcoat Thickness
    Maybe<ai_real> clearcoat_thickness{};
    ///< PBR Clearcoat Roughness
    Maybe<ai_real> clearcoat_roughness{};
    ///< PBR clearcoatGloss: controls clearcoat glossiness (0 = a “satin” appearance, 1 = a “gloss” appearance).
    Maybe<ai_real> clearcoat_gloss{};
    ///< PBR Anisotropy
    ai_real anisotropy{0};
    ///< PBR Anisotropy Rotation
    Maybe<ai_real> anisotropy_rotation{};
    ///< PBR Subsurface Scattering
    Maybe<ai_real> subsurface_scattering{};
    ///< PBR Specular Tint: a concession for artistic control that tints incident specular towards the base color.
    Maybe<ai_real> specular_tint{};
    /// See: https://disneyanimation.com/publications/physically-based-shading-at-disney/
    ///<  bump map multiplier (normal map scalar)(-bm)
    ai_real bump_multiplier{1};

    /// Constructor
    Material() {
        std::fill_n(clamp, static_cast<unsigned int>(TextureTypeCount), false);
    }

    /// Destructor
    ~Material() = default;
};

// ------------------------------------------------------------------------------------------------
/// @struct Mesh
/// @brief  Data structure to store a mesh
// ------------------------------------------------------------------------------------------------
struct Mesh {
    static constexpr unsigned int NoMaterial = ~0u;

    std::string mName;                                              ///< The name for the mesh
    std::vector<Face*> mFaces;                                      ///< Array with a pointer to all stored faces
    Material *mMaterial{nullptr};                                   ///< Assigned material
    unsigned int mNumIndices{0};                                    ///< Number of stored indices.
    unsigned int mUVCoordinates[AI_MAX_NUMBER_OF_TEXTURECOORDS]{};  ///< Number of UV components
    unsigned int mMaterialIndex{NoMaterial};                        ///< Material index.
    bool mHasNormals{false};                                        ///< True, if normals are stored.

    /// @brief Constructor
    /// @paran [in] name    The mesh name
    explicit Mesh(const std::string &name) : mName(name) {
        memset(mUVCoordinates, 0, sizeof(unsigned int) * AI_MAX_NUMBER_OF_TEXTURECOORDS);
    }

    /// @brief Destructor
    ~Mesh() {
        for (auto it = mFaces.begin(); it != mFaces.end(); ++it) {
            delete *it;
        }
    }
};

// ------------------------------------------------------------------------------------------------
/// @struct Model
/// @brief  Data structure to store all obj-specific model data
// ------------------------------------------------------------------------------------------------
struct Model {
    using GroupMap = std::map<std::string, std::vector<unsigned int> *>;
    using GroupMapIt = std::map<std::string, std::vector<unsigned int> *>::iterator;
    using ConstGroupMapIt = std::map<std::string, std::vector<unsigned int> *>::const_iterator;

    std::string mModelName{};                               ///< Model name
    std::vector<Object *> mObjects{};                       ///< List ob assigned objects
    ObjFile::Object *mCurrentObject{nullptr};               ///< Pointer to the current object
    ObjFile::Material *mCurrentMaterial{nullptr};           ///< Pointer to the current material
    ObjFile::Material *mDefaultMaterial{nullptr};           ///< Pointer to default material
    std::vector<std::string> mMaterialLib{};                ///< Vector with all generated materials
    std::vector<aiVector3D> mVertices{};                    ///< Vector with all generated vertices
    std::vector<aiVector3D> mNormals{};                     ///< vector with all generated normals
    std::vector<aiVector3D> mVertexColors{};                ///< vector with all vertex colors
    GroupMap mGroups{};                                     ///< Group map
    std::vector<unsigned int> *mGroupFaceIDs{nullptr};      ///< Group to face id assignment
    std::string mActiveGroup{};                             ///< Active group
    std::vector<aiVector3D> mTextureCoord{};                ///< Vector with generated texture coordinates
    unsigned int mTextureCoordDim{0};                       ///< Maximum dimension of texture coordinates
    Mesh *mCurrentMesh{nullptr};                            ///< Current mesh instance
    std::vector<Mesh *> mMeshes{};                          ///< Vector with stored meshes
    std::map<std::string, Material*> mMaterialMap{};        ///< Material map

    /// @brief  The default class constructor
    Model() = default;

    /// @brief  The class destructor
    ~Model() {
        for (auto & it : mObjects) {
            delete it;
        }
        for (auto & Meshe : mMeshes) {
            delete Meshe;
        }
        for (auto & Group : mGroups) {
            delete Group.second;
        }
        for (auto & it : mMaterialMap) {
            delete it.second;
        }
    }
};

} // Namespace Assimp::ObjFile

#endif // OBJ_FILEDATA_H_INC
