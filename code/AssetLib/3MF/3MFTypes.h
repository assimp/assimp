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
    RT_Unknown
}; // To be extended with other resource types (eg. material extension resources like Texture2d, Texture2dGroup...)

class Resource {
public:
    int mId;

    Resource(int id) :
            mId(id) {
        // empty
    }

    virtual ~Resource() {
        // empty
    }

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

    ~EmbeddedTexture() = default;

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

    ~Texture2DGroup() = default;

    ResourceType getType() const override {
        return ResourceType::RT_Texture2DGroup;
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

    ~BaseMaterials() = default;

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

    ~Object() = default;

    ResourceType getType() const override {
        return ResourceType::RT_Object;
    }
};

} // namespace D3MF
} // namespace Assimp
