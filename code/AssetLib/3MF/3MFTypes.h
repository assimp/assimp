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
#pragma once

#include <assimp/vector3.h>
#include <assimp/matrix4x4.h>
#include <assimp/ParsingUtils.h>
#include <vector>
#include <string>

struct aiMaterial;
struct aiMesh;

namespace Assimp {
namespace D3MF {

enum class ResourceType {
    RT_Object,
    RT_BaseMaterials,
    RT_EmbeddedTexture2D,
    RT_Texture2DGroup,
    RT_ColorGroup,
    RT_Unknown
}; // To be extended with other resource types (eg. material extension resources like Texture2d, Texture2dGroup...)

class Resource {
public:
    int mId;

    Resource(int id) :
            mId(id) {
        // empty
    }

    virtual ~Resource() = default;

    virtual ResourceType getType() const {
        return ResourceType::RT_Unknown;
    }
};

class EmbeddedTexture : public Resource {
public:
    std::string mPath;
    std::string mContentType;
    std::string mTilestyleU;
    std::string mTilestyleV;
    std::vector<char> mBuffer;

    EmbeddedTexture(int id) :
            Resource(id),
            mPath(),
            mContentType(),
            mTilestyleU(),
            mTilestyleV() {
        // empty
    }

    ~EmbeddedTexture() override = default;

    ResourceType getType() const override {
        return ResourceType::RT_EmbeddedTexture2D;
    }
};

class Texture2DGroup : public Resource {
public:
    std::vector<aiVector2D> mTex2dCoords;
    int mTexId;
    Texture2DGroup(int id) :
            Resource(id),
            mTexId(-1) {
        // empty
    }

    ~Texture2DGroup() override = default;

    ResourceType getType() const override {
        return ResourceType::RT_Texture2DGroup;
    }
};

class ColorGroup : public Resource {
public:
    std::vector<aiColor4D> mColors;
    ColorGroup(int id) :
            Resource(id){
        // empty
    }

    ~ColorGroup() override = default;

    ResourceType getType() const override {
        return ResourceType::RT_ColorGroup;
    }
};

class BaseMaterials : public Resource {
public:
    std::vector<unsigned int> mMaterialIndex;

    BaseMaterials(int id) :
            Resource(id),
            mMaterialIndex() {
        // empty
    }

    ~BaseMaterials() override = default;

    ResourceType getType() const override {
        return ResourceType::RT_BaseMaterials;
    }
};

struct Component {
    int mObjectId;
    aiMatrix4x4 mTransformation;
};

class Object : public Resource {
public:
    std::vector<aiMesh *> mMeshes;
    std::vector<unsigned int> mMeshIndex;
    std::vector<Component> mComponents;
    std::string mName;

    Object(int id) :
            Resource(id),
            mName(std::string("Object_") + ai_to_string(id)) {
        // empty
    }

    ~Object() override = default;

    ResourceType getType() const override {
        return ResourceType::RT_Object;
    }
};

} // namespace D3MF
} // namespace Assimp
