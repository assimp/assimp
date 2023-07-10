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
#ifndef OBJ_FILEDATA_H_INC
#define OBJ_FILEDATA_H_INC

#include <assimp/mesh.h>
#include <assimp/types.h>
#include <map>
#include <vector>
#include "Common/Maybe.h"

namespace Assimp {
namespace ObjFile {

struct Object;
struct Face;
struct Material;

// ------------------------------------------------------------------------------------------------
//! \struct Face
//! \brief  Data structure for a simple obj-face, describes discredit,l.ation and materials
// ------------------------------------------------------------------------------------------------
struct Face {
    using IndexArray = std::vector<unsigned int>;

    //! Primitive type
    aiPrimitiveType mPrimitiveType;
    //! Vertex indices
    IndexArray m_vertices;
    //! Normal indices
    IndexArray m_normals;
    //! Texture coordinates indices
    IndexArray m_texturCoords;
    //! Pointer to assigned material
    Material *m_pMaterial;

    //! \brief  Default constructor
    Face(aiPrimitiveType pt = aiPrimitiveType_POLYGON) :
            mPrimitiveType(pt), m_vertices(), m_normals(), m_texturCoords(), m_pMaterial(nullptr) {
        // empty
    }

    //! \brief  Destructor
    ~Face() = default;
};

// ------------------------------------------------------------------------------------------------
//! \struct Object
//! \brief  Stores all objects of an obj-file object definition
// ------------------------------------------------------------------------------------------------
struct Object {
    enum ObjectType {
        ObjType,
        GroupType
    };

    //! Object name
    std::string m_strObjName;
    //! Transformation matrix, stored in OpenGL format
    aiMatrix4x4 m_Transformation;
    //! All sub-objects referenced by this object
    std::vector<Object *> m_SubObjects;
    /// Assigned meshes
    std::vector<unsigned int> m_Meshes;

    //! \brief  Default constructor
    Object() = default;

    //! \brief  Destructor
    ~Object() {
        for (std::vector<Object *>::iterator it = m_SubObjects.begin(); it != m_SubObjects.end(); ++it) {
            delete *it;
        }
    }
};

// ------------------------------------------------------------------------------------------------
//! \struct Material
//! \brief  Data structure to store all material specific data
// ------------------------------------------------------------------------------------------------
struct Material {
    //! Name of material description
    aiString MaterialName;
    //! Texture names
    aiString texture;
    aiString textureSpecular;
    aiString textureAmbient;
    aiString textureEmissive;
    aiString textureBump;
    aiString textureNormal;
    aiString textureReflection[6];
    aiString textureSpecularity;
    aiString textureOpacity;
    aiString textureDisp;
    aiString textureRoughness;
    aiString textureMetallic;
    aiString textureSheen;
    aiString textureRMA;

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

    //! Ambient color
    aiColor3D ambient;
    //! Diffuse color
    aiColor3D diffuse;
    //! Specular color
    aiColor3D specular;
    //! Emissive color
    aiColor3D emissive;
    //! Alpha value
    ai_real alpha;
    //! Shineness factor
    ai_real shineness;
    //! Illumination model
    int illumination_model;
    //! Index of refraction
    ai_real ior;
    //! Transparency color
    aiColor3D transparent;

    //! PBR Roughness
    Maybe<ai_real> roughness;
    //! PBR Metallic
    Maybe<ai_real> metallic;
    //! PBR Metallic
    Maybe<aiColor3D> sheen;
    //! PBR Clearcoat Thickness
    Maybe<ai_real> clearcoat_thickness;
    //! PBR Clearcoat Rougness
    Maybe<ai_real> clearcoat_roughness;
    //! PBR Anisotropy
    ai_real anisotropy;

    //! bump map multipler (normal map scalar)(-bm)
    ai_real bump_multiplier;

    //! Constructor
    Material() :
            diffuse(ai_real(0.6), ai_real(0.6), ai_real(0.6)),
            alpha(ai_real(1.0)),
            shineness(ai_real(0.0)),
            illumination_model(1),
            ior(ai_real(1.0)),
            transparent(ai_real(1.0), ai_real(1.0), ai_real(1.0)),
            roughness(),
            metallic(),
            sheen(),
            clearcoat_thickness(),
            clearcoat_roughness(),
            anisotropy(ai_real(0.0)),
            bump_multiplier(ai_real(1.0)) {
        std::fill_n(clamp, static_cast<unsigned int>(TextureTypeCount), false);
    }

    // Destructor
    ~Material() = default;
};

// ------------------------------------------------------------------------------------------------
//! \struct Mesh
//! \brief  Data structure to store a mesh
// ------------------------------------------------------------------------------------------------
struct Mesh {
    static const unsigned int NoMaterial = ~0u;
    /// The name for the mesh
    std::string m_name;
    /// Array with pointer to all stored faces
    std::vector<Face*> m_Faces;
    /// Assigned material
    Material *m_pMaterial;
    /// Number of stored indices.
    unsigned int m_uiNumIndices;
    /// Number of UV
    unsigned int m_uiUVCoordinates[AI_MAX_NUMBER_OF_TEXTURECOORDS];
    /// Material index.
    unsigned int m_uiMaterialIndex;
    /// True, if normals are stored.
    bool m_hasNormals;

    /// Constructor
    explicit Mesh(const std::string &name) :
            m_name(name),
            m_pMaterial(nullptr),
            m_uiNumIndices(0),
            m_uiMaterialIndex(NoMaterial),
            m_hasNormals(false) {
        memset(m_uiUVCoordinates, 0, sizeof(unsigned int) * AI_MAX_NUMBER_OF_TEXTURECOORDS);
    }

    /// Destructor
    ~Mesh() {
        for (std::vector<Face *>::iterator it = m_Faces.begin();
                it != m_Faces.end(); ++it) {
            delete *it;
        }
    }
};

// ------------------------------------------------------------------------------------------------
//! \struct Model
//! \brief  Data structure to store all obj-specific model data
// ------------------------------------------------------------------------------------------------
struct Model {
    using GroupMap = std::map<std::string, std::vector<unsigned int> *>;
    using GroupMapIt = std::map<std::string, std::vector<unsigned int> *>::iterator;
    using ConstGroupMapIt = std::map<std::string, std::vector<unsigned int> *>::const_iterator;

    //! Model name
    std::string mModelName;
    //! List ob assigned objects
    std::vector<Object *> mObjects;
    //! Pointer to current object
    ObjFile::Object *mCurrentObject;
    //! Pointer to current material
    ObjFile::Material *mCurrentMaterial;
    //! Pointer to default material
    ObjFile::Material *mDefaultMaterial;
    //! Vector with all generated materials
    std::vector<std::string> mMaterialLib;
    //! Vector with all generated vertices
    std::vector<aiVector3D> mVertices;
    //! vector with all generated normals
    std::vector<aiVector3D> mNormals;
    //! vector with all vertex colors
    std::vector<aiVector3D> mVertexColors;
    //! Group map
    GroupMap mGroups;
    //! Group to face id assignment
    std::vector<unsigned int> *mGroupFaceIDs;
    //! Active group
    std::string mActiveGroup;
    //! Vector with generated texture coordinates
    std::vector<aiVector3D> mTextureCoord;
    //! Maximum dimension of texture coordinates
    unsigned int mTextureCoordDim;
    //! Current mesh instance
    Mesh *mCurrentMesh;
    //! Vector with stored meshes
    std::vector<Mesh *> mMeshes;
    //! Material map
    std::map<std::string, Material*> mMaterialMap;

    //! \brief  The default class constructor
    Model() :
            mModelName(),
            mCurrentObject(nullptr),
            mCurrentMaterial(nullptr),
            mDefaultMaterial(nullptr),
            mGroupFaceIDs(nullptr),
            mActiveGroup(),
            mTextureCoordDim(0),
            mCurrentMesh(nullptr) {
        // empty
    }

    //! \brief  The class destructor
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

// ------------------------------------------------------------------------------------------------

} // Namespace ObjFile
} // Namespace Assimp

#endif // OBJ_FILEDATA_H_INC
