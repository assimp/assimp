/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team
Copyright (c) 2024-2026, Ahsan Mehmood (OpenSKP)

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

#ifndef ASSIMP_BUILD_NO_SKP_IMPORTER

#include "SkpImporter.h"

#include <assimp/StringComparison.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>

#include <openskp/errors.hpp>
#include <openskp/instanced_scene.hpp>
#include <openskp/parser.hpp>

#include <algorithm>
#include <cstring>
#include <memory>

// RESOURCES:
// https://github.com/iamahsanmehmood/openskp - the .skp reader/writer this
// importer delegates all parsing to. See its docs/BINARY_FORMAT.md for the
// on-disk format this importer does not itself need to understand.

namespace {

static constexpr aiImporterDesc desc = {
    "SketchUp Importer (OpenSKP)",
    "",
    "",
    "Delegates all parsing to OpenSKP (MIT, https://github.com/iamahsanmehmood/openskp); "
    "reads both the legacy pre-2021 and modern 2021+ .skp container formats.",
    aiImporterFlags_SupportBinaryFlavour,
    0,
    0,
    0,
    0,
    "skp"
};

// Every .skp file - legacy (pre-2021) and VFF (2021+) alike - starts with
// this 4-byte marker (confirmed against OpenSKP's own openskp::valid_header,
// packages/cpp/src/vff.cpp - not part of OpenSKP's public API, so mirrored
// here rather than linked against directly).
constexpr unsigned char kSkpMagic[4] = { 0xff, 0xfe, 0xff, 0x0e };

aiColor4D toColor4D(const std::array<double, 4> &c) {
    return aiColor4D(static_cast<ai_real>(c[0]), static_cast<ai_real>(c[1]), static_cast<ai_real>(c[2]),
            static_cast<ai_real>(c[3]));
}

// OpenSKP's InstancedNode::matrix is a 16-element COLUMN-major matrix
// (metres, Y-up, glTF's own convention - see instanced_scene.hpp), while
// aiMatrix4x4's a1..d4 fields are ROW-major (translation lives at a4/b4/c4,
// confirmed against aiMatrix4x4::Translation()). The two are transposes of
// each other read in the same flat element order.
aiMatrix4x4 toMatrix4x4(const std::array<double, 16> &m) {
    aiMatrix4x4 out;
    out.a1 = static_cast<ai_real>(m[0]);
    out.b1 = static_cast<ai_real>(m[1]);
    out.c1 = static_cast<ai_real>(m[2]);
    out.d1 = static_cast<ai_real>(m[3]);
    out.a2 = static_cast<ai_real>(m[4]);
    out.b2 = static_cast<ai_real>(m[5]);
    out.c2 = static_cast<ai_real>(m[6]);
    out.d2 = static_cast<ai_real>(m[7]);
    out.a3 = static_cast<ai_real>(m[8]);
    out.b3 = static_cast<ai_real>(m[9]);
    out.c3 = static_cast<ai_real>(m[10]);
    out.d3 = static_cast<ai_real>(m[11]);
    out.a4 = static_cast<ai_real>(m[12]);
    out.b4 = static_cast<ai_real>(m[13]);
    out.c4 = static_cast<ai_real>(m[14]);
    out.d4 = static_cast<ai_real>(m[15]);
    return out;
}

} // namespace

namespace Assimp {

bool SkpImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler, bool /*checkSig*/) const {
    std::unique_ptr<IOStream> pStream(pIOHandler->Open(pFile, "rb"));
    if (!pStream) {
        return false;
    }
    unsigned char data[4];
    if (4 != pStream->Read(data, 1, 4)) {
        return false;
    }
    return 0 == std::memcmp(data, kSkpMagic, 4);
}

const aiImporterDesc *SkpImporter::GetInfo() const {
    return &desc;
}

void SkpImporter::InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) {
    std::unique_ptr<IOStream> pStream(pIOHandler->Open(pFile, "rb"));
    if (!pStream) {
        throw DeadlyImportError("Failed to open file ", pFile, ".");
    }

    const size_t fileSize = pStream->FileSize();
    if (fileSize < 4) {
        throw DeadlyImportError(".skp file ", pFile, " is too small to be valid.");
    }

    std::vector<std::uint8_t> buffer(fileSize);
    if (fileSize != pStream->Read(buffer.data(), 1, fileSize)) {
        throw DeadlyImportError("Failed to read the file ", pFile, ".");
    }

    openskp::InstancedScene scene;
    try {
        scene = openskp::build_instanced_scene(std::move(buffer));
    } catch (const openskp::SkpParseError &e) {
        throw DeadlyImportError("OpenSKP failed to parse ", pFile, ": ", e.what());
    }

    if (scene.mesh_resources.empty()) {
        throw DeadlyImportError(pFile, " contains no importable geometry.");
    }

    const std::vector<int> materialTextureIndex = importMaterialsAndTextures(scene, pScene);
    const auto meshIndicesByResource = importMeshes(scene, pScene, materialTextureIndex);

    pScene->mRootNode = importNode(scene.scene_hierarchy, meshIndicesByResource, nullptr);
    pScene->mRootNode->mName.Set(pFile);
}

std::vector<int> SkpImporter::importMaterialsAndTextures(const openskp::InstancedScene &scene, aiScene *pScene) {
    // scene.textures is already deduplicated by source bytes (see
    // InstancedScene::textures's own doc comment); embed every one exactly
    // once, then have each material reference the embedded index it needs.
    if (!scene.textures.empty()) {
        pScene->mNumTextures = static_cast<unsigned int>(scene.textures.size());
        pScene->mTextures = new aiTexture *[pScene->mNumTextures];
        for (std::size_t i = 0; i < scene.textures.size(); ++i) {
            const openskp::SceneTexture &src = scene.textures[i];
            if (src.data.size() > AI_MAX_ALLOC(aiTexel)) {
                throw DeadlyImportError("SketchUp: embedded texture too large, would overflow");
            }
            aiTexture *tex = new aiTexture();
            tex->mWidth = static_cast<unsigned int>(src.data.size());
            tex->mHeight = 0; // compressed (PNG/JPEG) payload, not raw texels - see aiTexture::mHeight's doc comment
            // Allocated as aiTexel[] (not unsigned char[] cast to aiTexel*) so
            // ~aiTexture()'s `delete[] pcData` matches the allocation type -
            // same pattern AssbinLoader uses for its own compressed textures
            // (`new aiTexel[tex->mWidth]`). mWidth stays the exact byte count
            // per aiTexture::mWidth's own doc comment; since aiTexel is 4
            // bytes, indexing mWidth aiTexel elements over-allocates up to 4x
            // the real byte count - the same known inefficiency Assbin's own
            // code accepts, harmless since only the first mWidth bytes are
            // ever read back out.
            tex->pcData = new aiTexel[tex->mWidth];
            std::memcpy(tex->pcData, src.data.data(), src.data.size());
            tex->mFilename.Set(src.filename.c_str());

            // achFormatHint: file extension without the dot, lowercase, <=3 chars used in practice.
            std::string hint = src.mime_type.rfind("image/", 0) == 0 ? src.mime_type.substr(6) : src.mime_type;
            if (hint == "jpeg") hint = "jpg";
            std::memset(tex->achFormatHint, 0, sizeof(tex->achFormatHint));
            const std::size_t hintLen = std::min(hint.size(), sizeof(tex->achFormatHint) - 1);
            std::memcpy(tex->achFormatHint, hint.data(), hintLen);

            pScene->mTextures[i] = tex;
        }
    }

    pScene->mNumMaterials = static_cast<unsigned int>(scene.gltf_materials.size());
    pScene->mMaterials = pScene->mNumMaterials ? new aiMaterial *[pScene->mNumMaterials] : nullptr;

    std::vector<int> materialTextureIndex(scene.gltf_materials.size(), -1);
    for (std::size_t i = 0; i < scene.gltf_materials.size(); ++i) {
        const openskp::GltfMaterial &src = scene.gltf_materials[i];
        aiMaterial *mat = new aiMaterial();

        aiString name(src.name.empty() ? ("SkpMaterial_" + std::to_string(i)) : src.name);
        mat->AddProperty(&name, AI_MATKEY_NAME);

        const aiColor4D base = toColor4D(src.pbr_metallic_roughness.base_color_factor);
        mat->AddProperty(&base, 1, AI_MATKEY_BASE_COLOR);
        mat->AddProperty(&base, 1, AI_MATKEY_COLOR_DIFFUSE); // legacy-pipeline compatibility, same as glTF2Importer

        const ai_real opacity = static_cast<ai_real>(src.pbr_metallic_roughness.base_color_factor[3]);
        mat->AddProperty(&opacity, 1, AI_MATKEY_OPACITY);

        const ai_real metallic = static_cast<ai_real>(src.pbr_metallic_roughness.metallic_factor);
        mat->AddProperty(&metallic, 1, AI_MATKEY_METALLIC_FACTOR);
        const ai_real roughness = static_cast<ai_real>(src.pbr_metallic_roughness.roughness_factor);
        mat->AddProperty(&roughness, 1, AI_MATKEY_ROUGHNESS_FACTOR);

        const int twoSided = src.double_sided ? 1 : 0;
        mat->AddProperty(&twoSided, 1, AI_MATKEY_TWOSIDED);

        if (src.pbr_metallic_roughness.base_color_texture.has_value()) {
            const int texIdx = static_cast<int>(*src.pbr_metallic_roughness.base_color_texture);
            materialTextureIndex[i] = texIdx;

            aiString uri;
            uri.data[0] = '*';
            uri.length = 1 + ASSIMP_itoa10(uri.data + 1, AI_MAXLEN - 1, texIdx);
            mat->AddProperty(&uri, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
            mat->AddProperty(&uri, AI_MATKEY_TEXTURE(aiTextureType_BASE_COLOR, 0));
        }

        pScene->mMaterials[i] = mat;
    }

    return materialTextureIndex;
}

std::unordered_map<std::string, std::vector<unsigned int>> SkpImporter::importMeshes(
        const openskp::InstancedScene &scene, aiScene *pScene, const std::vector<int> & /*materialTextureIndex*/) {
    std::size_t totalPrimitives = 0;
    for (const auto &resource : scene.mesh_resources) {
        totalPrimitives += resource.primitives.size();
    }

    pScene->mNumMeshes = static_cast<unsigned int>(totalPrimitives);
    pScene->mMeshes = totalPrimitives ? new aiMesh *[totalPrimitives] : nullptr;

    std::unordered_map<std::string, std::vector<unsigned int>> meshIndicesByResource;
    unsigned int meshIndex = 0;

    for (const auto &resource : scene.mesh_resources) {
        std::vector<unsigned int> &indices = meshIndicesByResource[resource.id];
        indices.reserve(resource.primitives.size());

        for (const openskp::LocalPrimitive &prim : resource.primitives) {
            aiMesh *mesh = new aiMesh();
            mesh->mName.Set(resource.definition_name.empty() ? resource.id : resource.definition_name);
            mesh->mMaterialIndex = static_cast<unsigned int>(prim.material_index);
            mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

            const std::size_t vertexCount = prim.positions.size() / 3;
            mesh->mNumVertices = static_cast<unsigned int>(vertexCount);
            mesh->mVertices = new aiVector3D[vertexCount];
            for (std::size_t v = 0; v < vertexCount; ++v) {
                mesh->mVertices[v] = aiVector3D(static_cast<ai_real>(prim.positions[3 * v]),
                        static_cast<ai_real>(prim.positions[3 * v + 1]), static_cast<ai_real>(prim.positions[3 * v + 2]));
            }

            if (prim.normals.size() == prim.positions.size()) {
                mesh->mNormals = new aiVector3D[vertexCount];
                for (std::size_t v = 0; v < vertexCount; ++v) {
                    mesh->mNormals[v] = aiVector3D(static_cast<ai_real>(prim.normals[3 * v]),
                            static_cast<ai_real>(prim.normals[3 * v + 1]), static_cast<ai_real>(prim.normals[3 * v + 2]));
                }
            }

            if (prim.uvs.size() == vertexCount * 2) {
                mesh->mTextureCoords[0] = new aiVector3D[vertexCount];
                mesh->mNumUVComponents[0] = 2;
                for (std::size_t v = 0; v < vertexCount; ++v) {
                    mesh->mTextureCoords[0][v] = aiVector3D(
                            static_cast<ai_real>(prim.uvs[2 * v]), static_cast<ai_real>(prim.uvs[2 * v + 1]), 0);
                }
            }

            const std::size_t faceCount = prim.indices.size() / 3;
            mesh->mNumFaces = static_cast<unsigned int>(faceCount);
            mesh->mFaces = new aiFace[faceCount];
            for (std::size_t f = 0; f < faceCount; ++f) {
                aiFace &face = mesh->mFaces[f];
                face.mNumIndices = 3;
                face.mIndices = new unsigned int[3];
                face.mIndices[0] = prim.indices[3 * f];
                face.mIndices[1] = prim.indices[3 * f + 1];
                face.mIndices[2] = prim.indices[3 * f + 2];
            }

            pScene->mMeshes[meshIndex] = mesh;
            indices.push_back(meshIndex);
            ++meshIndex;
        }
    }

    return meshIndicesByResource;
}

aiNode *SkpImporter::importNode(const openskp::InstancedNode &node,
        const std::unordered_map<std::string, std::vector<unsigned int>> &meshIndicesByResource, aiNode *parent) {
    aiNode *out = new aiNode(node.name);
    out->mParent = parent;
    out->mTransformation = toMatrix4x4(node.matrix);

    if (node.mesh_resource_id.has_value()) {
        auto it = meshIndicesByResource.find(*node.mesh_resource_id);
        if (it != meshIndicesByResource.end() && !it->second.empty()) {
            out->mNumMeshes = static_cast<unsigned int>(it->second.size());
            out->mMeshes = new unsigned int[out->mNumMeshes];
            std::copy(it->second.begin(), it->second.end(), out->mMeshes);
        }
    }

    if (!node.children.empty()) {
        out->mNumChildren = static_cast<unsigned int>(node.children.size());
        out->mChildren = new aiNode *[out->mNumChildren];
        for (std::size_t i = 0; i < node.children.size(); ++i) {
            out->mChildren[i] = importNode(node.children[i], meshIndicesByResource, out);
        }
    }

    return out;
}

} // namespace Assimp

#endif // ASSIMP_BUILD_NO_SKP_IMPORTER
