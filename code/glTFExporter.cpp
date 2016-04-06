/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team
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



#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_GLTF_EXPORTER

#include "glTFExporter.h"
#include "Exceptional.h"
#include "StringComparison.h"
#include "ByteSwapper.h"

#include <assimp/version.h>
#include <assimp/IOSystem.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/material.h>
#include <assimp/scene.h>

#include <memory>
#include <memory>

#include "glTFAssetWriter.h"

using namespace rapidjson;

using namespace Assimp;
using namespace glTF;

namespace Assimp {

    // ------------------------------------------------------------------------------------------------
    // Worker function for exporting a scene to GLTF. Prototyped and registered in Exporter.cpp
    void ExportSceneGLTF(const char* pFile, IOSystem* pIOSystem, const aiScene* pScene, const ExportProperties* pProperties)
    {
        // invoke the exporter
        glTFExporter exporter(pFile, pIOSystem, pScene, pProperties, false);
    }

    // ------------------------------------------------------------------------------------------------
    // Worker function for exporting a scene to GLB. Prototyped and registered in Exporter.cpp
    void ExportSceneGLB(const char* pFile, IOSystem* pIOSystem, const aiScene* pScene, const ExportProperties* pProperties)
    {
        // invoke the exporter
        glTFExporter exporter(pFile, pIOSystem, pScene, pProperties, true);
    }

} // end of namespace Assimp



glTFExporter::glTFExporter(const char* filename, IOSystem* pIOSystem, const aiScene* pScene,
                           const ExportProperties* pProperties, bool isBinary)
    : mFilename(filename)
    , mIOSystem(pIOSystem)
    , mScene(pScene)
    , mProperties(pProperties)
{
    std::unique_ptr<Asset> asset(new glTF::Asset(pIOSystem));
    mAsset = asset.get();

    if (isBinary) {
        asset->SetAsBinary();
    }

    ExportMetadata();

    //for (unsigned int i = 0; i < pScene->mNumAnimations; ++i) {}

    //for (unsigned int i = 0; i < pScene->mNumCameras; ++i) {}

    //for (unsigned int i = 0; i < pScene->mNumLights; ++i) {}


    ExportMaterials();

    ExportMeshes();

    //for (unsigned int i = 0; i < pScene->mNumTextures; ++i) {}


    if (mScene->mRootNode) {
        ExportNode(mScene->mRootNode);
    }

    ExportScene();


    glTF::AssetWriter writer(*mAsset);
    writer.WriteFile(filename);
}


static void CopyValue(const aiMatrix4x4& v, glTF::mat4& o)
{
    o[ 0] = v.a1; o[ 1] = v.b1; o[ 2] = v.c1; o[ 3] = v.d1;
    o[ 4] = v.a2; o[ 5] = v.b2; o[ 6] = v.c2; o[ 7] = v.d2;
    o[ 8] = v.a3; o[ 9] = v.b3; o[10] = v.c3; o[11] = v.d3;
    o[12] = v.a4; o[13] = v.b4; o[14] = v.c4; o[15] = v.d4;
}

inline Ref<Accessor> ExportData(Asset& a, std::string& meshName, Ref<Buffer>& buffer,
    unsigned int count, void* data, AttribType::Value typeIn, AttribType::Value typeOut, ComponentType compType, bool isIndices = false)
{
    if (!count || !data) return Ref<Accessor>();

    unsigned int numCompsIn = AttribType::GetNumComponents(typeIn);
    unsigned int numCompsOut = AttribType::GetNumComponents(typeOut);
    unsigned int bytesPerComp = ComponentTypeSize(compType);

    size_t offset = buffer->byteLength;
    size_t length = count * numCompsOut * bytesPerComp;
    buffer->Grow(length);

    // bufferView
    Ref<BufferView> bv = a.bufferViews.Create(a.FindUniqueID(meshName, "view"));
    bv->buffer = buffer;
    bv->byteOffset = 0;
    bv->byteLength = length; //! The target that the WebGL buffer should be bound to.
    bv->target = isIndices ? BufferViewTarget_ELEMENT_ARRAY_BUFFER : BufferViewTarget_ARRAY_BUFFER;

    // accessor
    Ref<Accessor> acc = a.accessors.Create(a.FindUniqueID(meshName, "accessor"));
    acc->bufferView = bv;
    acc->byteOffset = offset;
    acc->byteStride = 0;
    acc->componentType = compType;
    acc->count = count;
    acc->type = typeOut;

    // copy the data
    acc->WriteData(count, data, numCompsIn*bytesPerComp);

    return acc;
}

namespace {
    void GetMatScalar(const aiMaterial* mat, float& val, const char* propName, int type, int idx) {
        if (mat->Get(propName, type, idx, val) == AI_SUCCESS) {}
    }
}

void glTFExporter::GetMatColorOrTex(const aiMaterial* mat, glTF::TexProperty& prop, const char* propName, int type, int idx, aiTextureType tt)
{
    aiString tex;
    aiColor4D col;
    if (mat->GetTextureCount(tt) > 0) {
        if (mat->Get(AI_MATKEY_TEXTURE(tt, 0), tex) == AI_SUCCESS) {
            std::string path = tex.C_Str();

            if (path.size() > 0) {
                if (path[0] != '*') {
                    std::map<std::string, size_t>::iterator it = mTexturesByPath.find(path);
                    if (it != mTexturesByPath.end()) {
                        prop.texture = mAsset->textures.Get(it->second);
                    }
                }

                if (!prop.texture) {
                    std::string texId = mAsset->FindUniqueID("", "texture");
                    prop.texture = mAsset->textures.Create(texId);
                    mTexturesByPath[path] = prop.texture.GetIndex();

                    std::string imgId = mAsset->FindUniqueID("", "image");
                    prop.texture->source = mAsset->images.Create(imgId);

                    if (path[0] == '*') { // embedded
                        aiTexture* tex = mScene->mTextures[atoi(&path[1])];

                        uint8_t* data = reinterpret_cast<uint8_t*>(tex->pcData);
                        prop.texture->source->SetData(data, tex->mWidth, *mAsset);

                        if (tex->achFormatHint[0]) {
                            std::string mimeType = "image/";
                            mimeType += (memcmp(tex->achFormatHint, "jpg", 3) == 0) ? "jpeg" : tex->achFormatHint;
                            prop.texture->source->mimeType = mimeType;
                        }
                    }
                    else {
                        prop.texture->source->uri = path;
                    }
                }
            }
        }
    }

    if (mat->Get(propName, type, idx, col) == AI_SUCCESS) {
        prop.color[0] = col.r; prop.color[1] = col.g; prop.color[2] = col.b; prop.color[3] = col.a;
    }
}

void glTFExporter::ExportMaterials()
{
    aiString aiName;
    for (unsigned int i = 0; i < mScene->mNumMaterials; ++i) {
        const aiMaterial* mat = mScene->mMaterials[i];


        std::string name;
        if (mat->Get(AI_MATKEY_NAME, aiName) == AI_SUCCESS) {
            name = aiName.C_Str();
        }
        name = mAsset->FindUniqueID(name, "material");

        Ref<Material> m = mAsset->materials.Create(name);

        GetMatColorOrTex(mat, m->ambient, AI_MATKEY_COLOR_AMBIENT, aiTextureType_AMBIENT);
        GetMatColorOrTex(mat, m->diffuse, AI_MATKEY_COLOR_DIFFUSE, aiTextureType_DIFFUSE);
        GetMatColorOrTex(mat, m->specular, AI_MATKEY_COLOR_SPECULAR, aiTextureType_SPECULAR);
        GetMatColorOrTex(mat, m->emission, AI_MATKEY_COLOR_EMISSIVE, aiTextureType_EMISSIVE);

        GetMatScalar(mat, m->shininess, AI_MATKEY_SHININESS);
    }
}

void glTFExporter::ExportMeshes()
{
    for (unsigned int i = 0; i < mScene->mNumMeshes; ++i) {
        const aiMesh* aim = mScene->mMeshes[i];

        std::string meshId = mAsset->FindUniqueID(aim->mName.C_Str(), "mesh");
        Ref<Mesh> m = mAsset->meshes.Create(meshId);
        m->primitives.resize(1);
        Mesh::Primitive& p = m->primitives.back();

        p.material = mAsset->materials.Get(aim->mMaterialIndex);

        std::string bufferId = mAsset->FindUniqueID(meshId, "buffer");

        Ref<Buffer> b = mAsset->GetBodyBuffer();
        if (!b) {
            b = mAsset->buffers.Create(bufferId);
        }

        Ref<Accessor> v = ExportData(*mAsset, meshId, b, aim->mNumVertices, aim->mVertices, AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
        if (v) p.attributes.position.push_back(v);

        Ref<Accessor> n = ExportData(*mAsset, meshId, b, aim->mNumVertices, aim->mNormals, AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
        if (n) p.attributes.normal.push_back(n);

        for (int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
            if (aim->mNumUVComponents[i] > 0) {
                AttribType::Value type = (aim->mNumUVComponents[i] == 2) ? AttribType::VEC2 : AttribType::VEC3;
                Ref<Accessor> tc = ExportData(*mAsset, meshId, b, aim->mNumVertices, aim->mTextureCoords[i], AttribType::VEC3, type, ComponentType_FLOAT, true);
                if (tc) p.attributes.texcoord.push_back(tc);
            }
        }

        if (aim->mNumFaces > 0) {
            unsigned int nIndicesPerFace = aim->mFaces[0].mNumIndices;
            std::vector<uint16_t> indices;
            indices.resize(aim->mNumFaces * nIndicesPerFace);
            for (size_t i = 0; i < aim->mNumFaces; ++i) {
                for (size_t j = 0; j < nIndicesPerFace; ++j) {
                    indices[i*nIndicesPerFace + j] = uint16_t(aim->mFaces[i].mIndices[j]);
                }
            }
            p.indices = ExportData(*mAsset, meshId, b, indices.size(), &indices[0], AttribType::SCALAR, AttribType::SCALAR, ComponentType_UNSIGNED_SHORT);
        }

        switch (aim->mPrimitiveTypes) {
            case aiPrimitiveType_POLYGON:
                p.mode = PrimitiveMode_TRIANGLES; break; // TODO implement this
            case aiPrimitiveType_LINE:
                p.mode = PrimitiveMode_LINES; break;
            case aiPrimitiveType_POINT:
                p.mode = PrimitiveMode_POINTS; break;
            default: // aiPrimitiveType_TRIANGLE
                p.mode = PrimitiveMode_TRIANGLES;
        }
    }
}

size_t glTFExporter::ExportNode(const aiNode* n)
{
    Ref<Node> node = mAsset->nodes.Create(mAsset->FindUniqueID(n->mName.C_Str(), "node"));

    if (!n->mTransformation.IsIdentity()) {
        node->matrix.isPresent = true;
        CopyValue(n->mTransformation, node->matrix.value);
    }

    for (unsigned int i = 0; i < n->mNumMeshes; ++i) {
        node->meshes.push_back(mAsset->meshes.Get(n->mMeshes[i]));
    }

    for (unsigned int i = 0; i < n->mNumChildren; ++i) {
        size_t idx = ExportNode(n->mChildren[i]);
        node->children.push_back(mAsset->nodes.Get(idx));
    }

    return node.GetIndex();
}


void glTFExporter::ExportScene()
{
    const char* sceneName = "defaultScene";
    Ref<Scene> scene = mAsset->scenes.Create(sceneName);

    // root node will be the first one exported (idx 0)
    if (mAsset->nodes.Size() > 0) {
        scene->nodes.push_back(mAsset->nodes.Get(size_t(0)));
    }

    // set as the default scene
    mAsset->scene = scene;
}

void glTFExporter::ExportMetadata()
{
    glTF::AssetMetadata& asset = mAsset->asset;
    asset.version = 1;

    char buffer[256];
    ai_snprintf(buffer, 256, "Open Asset Import Library (assimp v%d.%d.%d)",
        aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionRevision());

    asset.generator = buffer;
}







#endif // ASSIMP_BUILD_NO_GLTF_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
