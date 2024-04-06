/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

                          All rights reserved.

                          Redistribution and use of this software in source and binary forms,
        with or without modification, are permitted provided that the following
        conditions are met:

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
                ---------------------------------------------------------------------------
                */

/** @file  USDLoader.cpp
 *  @brief Implementation of the USD importer class
 */

#ifndef ASSIMP_BUILD_NO_USD_IMPORTER
#include <memory>
#include <sstream>

// internal headers
#include <assimp/ai_assert.h>
#include <assimp/anim.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/fast_atof.h>
#include <assimp/Importer.hpp>
#include <assimp/importerdesc.h>
#include <assimp/IOStreamBuffer.h>
#include <assimp/IOSystem.hpp>
#include <assimp/scene.h>
#include <assimp/StringUtils.h>
#include <assimp/StreamReader.h>

#include "tydra/scene-access.hh"
#include "tydra/shader-network.hh"
#include "USDLoaderImplTinyusdz.h"
#include "USDLoaderUtil.h"

namespace {
    const char *const TAG = "USDLoaderImplTinyusdz (C++)";
}

namespace Assimp {
using namespace std;

void USDImporterImplTinyusdz::InternReadFile(
        const std::string &pFile,
        aiScene *pScene,
        IOSystem *pIOHandler) {
    // Grab filename for logging purposes
    size_t pos = pFile.find_last_of('/');
    string nameWExt = pFile.substr(pos + 1);
    (void) TAG; // Ignore unused variable when -Werror enabled

    bool ret{ false };
    tinyusdz::USDLoadOptions options;
    tinyusdz::Stage stage;
    std::string warn, err;
    if (isUsdc(pFile)) {
        ret = LoadUSDCFromFile(pFile, &stage, &warn, &err, options);
    } else if (isUsda(pFile)) {
        ret = LoadUSDAFromFile(pFile, &stage, &warn, &err, options);
    } else if (isUsdz(pFile)) {
        ret = LoadUSDZFromFile(pFile, &stage, &warn, &err, options);
    } else if (isUsd(pFile)) {
        ret = LoadUSDFromFile(pFile, &stage, &warn, &err, options);
    }
    if (!ret) {
        return;
    }
    tinyusdz::tydra::RenderScene render_scene;
    tinyusdz::tydra::RenderSceneConverter converter;
    ret = converter.ConvertToRenderScene(stage, &render_scene);
    if (!ret) {
        return;
    }
    pScene->mNumMeshes = render_scene.meshes.size();
    pScene->mMeshes = new aiMesh *[pScene->mNumMeshes]();
    // Create root node
    pScene->mRootNode = new aiNode();
    pScene->mRootNode->mNumMeshes = pScene->mNumMeshes;
    pScene->mRootNode->mMeshes = new unsigned int[pScene->mRootNode->mNumMeshes];

    // Export meshes
    for (size_t meshIdx = 0; meshIdx < pScene->mNumMeshes; meshIdx++) {
        pScene->mMeshes[meshIdx] = new aiMesh();
        pScene->mMeshes[meshIdx]->mName.Set(render_scene.meshes[meshIdx].element_name);
        if (!render_scene.meshes[meshIdx].materialIds.empty()) {
            pScene->mMeshes[meshIdx]->mMaterialIndex = render_scene.meshes[meshIdx].materialIds[0];
        }
        verticesForMesh(render_scene, pScene, meshIdx, nameWExt);
        facesForMesh(render_scene, pScene, meshIdx, nameWExt);
        normalsForMesh(render_scene, pScene, meshIdx, nameWExt);
        materialsForMesh(render_scene, pScene, meshIdx, nameWExt);
        uvsForMesh(render_scene, pScene, meshIdx, nameWExt);
        pScene->mRootNode->mMeshes[meshIdx] = static_cast<unsigned int>(meshIdx);
    }
    nodes(render_scene, pScene, nameWExt);
    materials(render_scene, pScene, nameWExt);
    textures(render_scene, pScene, nameWExt);
    textureImages(render_scene, pScene, nameWExt);
    buffers(render_scene, pScene, nameWExt);
    animations(render_scene, pScene, nameWExt);
}

void USDImporterImplTinyusdz::verticesForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
    pScene->mMeshes[meshIdx]->mNumVertices = render_scene.meshes[meshIdx].points.size();
    pScene->mMeshes[meshIdx]->mVertices = new aiVector3D[pScene->mMeshes[meshIdx]->mNumVertices];
    for (size_t j = 0; j < pScene->mMeshes[meshIdx]->mNumVertices; ++j) {
        pScene->mMeshes[meshIdx]->mVertices[j].x = render_scene.meshes[meshIdx].points[j][0];
        pScene->mMeshes[meshIdx]->mVertices[j].y = render_scene.meshes[meshIdx].points[j][1];
        pScene->mMeshes[meshIdx]->mVertices[j].z = render_scene.meshes[meshIdx].points[j][2];
    }
}

void USDImporterImplTinyusdz::facesForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
    pScene->mMeshes[meshIdx]->mNumFaces = render_scene.meshes[meshIdx].faceVertexCounts.size();
    pScene->mMeshes[meshIdx]->mFaces = new aiFace[pScene->mMeshes[meshIdx]->mNumFaces]();
    size_t faceVertIdxOffset = 0;
    for (size_t faceIdx = 0; faceIdx < pScene->mMeshes[meshIdx]->mNumFaces; ++faceIdx) {
        pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices = render_scene.meshes[meshIdx].faceVertexCounts[faceIdx];
        pScene->mMeshes[meshIdx]->mFaces[faceIdx].mIndices = new unsigned int[pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices];
        for (size_t j = 0; j < pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices; ++j) {
            pScene->mMeshes[meshIdx]->mFaces[faceIdx].mIndices[j] =
                    render_scene.meshes[meshIdx].faceVertexIndices[j + faceVertIdxOffset];
        }
        faceVertIdxOffset += pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices;
    }
}

void USDImporterImplTinyusdz::normalsForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
    pScene->mMeshes[meshIdx]->mNormals = new aiVector3D[pScene->mMeshes[meshIdx]->mNumVertices];
    size_t faceVertIdxOffset = 0;
    for (size_t faceIdx = 0; faceIdx < pScene->mMeshes[meshIdx]->mNumFaces; ++faceIdx) {
        size_t vertIdx;
        for (size_t j = 0; j < pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices; ++j) {
            vertIdx = pScene->mMeshes[meshIdx]->mFaces[faceIdx].mIndices[j];
            pScene->mMeshes[meshIdx]->mNormals[vertIdx].x = render_scene.meshes[meshIdx].facevaryingNormals[faceVertIdxOffset + j][0];
            pScene->mMeshes[meshIdx]->mNormals[vertIdx].y = render_scene.meshes[meshIdx].facevaryingNormals[faceVertIdxOffset + j][1];
            pScene->mMeshes[meshIdx]->mNormals[vertIdx].z = render_scene.meshes[meshIdx].facevaryingNormals[faceVertIdxOffset + j][2];
        }
        faceVertIdxOffset += pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices;
    }
}

void USDImporterImplTinyusdz::materialsForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
}

void USDImporterImplTinyusdz::uvsForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
    const size_t uvSlotsCount = render_scene.meshes[meshIdx].facevaryingTexcoords.size();
    if (uvSlotsCount < 1) {
        return;
    }
    const auto uvsForSlot0 = render_scene.meshes[meshIdx].facevaryingTexcoords.at(0);
    pScene->mMeshes[meshIdx]->mNumUVComponents[0] = uvSlotsCount;
    pScene->mMeshes[meshIdx]->mTextureCoords[0] = new aiVector3D[pScene->mMeshes[meshIdx]->mNumVertices];
    for (size_t uvSlotIdx = 0; uvSlotIdx < pScene->mMeshes[meshIdx]->mNumUVComponents[0]; ++uvSlotIdx) {
        const auto uvsForSlot = render_scene.meshes[meshIdx].facevaryingTexcoords.at(uvSlotIdx);
        size_t faceVertIdxOffset = 0;
        for (size_t faceIdx = 0; faceIdx < pScene->mMeshes[meshIdx]->mNumFaces; ++faceIdx) {
            for (size_t j = 0; j < pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices; ++j) {
                size_t vertIdx = pScene->mMeshes[meshIdx]->mFaces[faceIdx].mIndices[j];
                pScene->mMeshes[meshIdx]->mTextureCoords[uvSlotIdx][vertIdx].x = uvsForSlot[faceVertIdxOffset + j][0];
                pScene->mMeshes[meshIdx]->mTextureCoords[uvSlotIdx][vertIdx].y = uvsForSlot[faceVertIdxOffset + j][1];
            }
            faceVertIdxOffset += pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices;
        }
    }
}

void USDImporterImplTinyusdz::nodes(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    const size_t numNodes{render_scene.nodes.size()};
    (void) numNodes; // Ignore unused variable when -Werror enabled
}

static aiColor3D *ownedColorPtrFor(const std::array<float, 3> &color) {
    aiColor3D *colorPtr = new aiColor3D();
    colorPtr->r = color[0];
    colorPtr->g = color[1];
    colorPtr->b = color[2];
    return colorPtr;
}

void USDImporterImplTinyusdz::materials(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    const size_t numMaterials{render_scene.materials.size()};
    (void) numMaterials; // Ignore unused variable when -Werror enabled
    pScene->mMaterials = 0;
    if (render_scene.materials.empty()) {
        return;
    }
    pScene->mMaterials = new aiMaterial *[render_scene.materials.size()];
    for (const auto &material : render_scene.materials) {
        aiMaterial *mat = new aiMaterial;

        aiString *materialName = new aiString();
        materialName->Set(material.name);
        mat->AddProperty(materialName, AI_MATKEY_NAME);

        mat->AddProperty(
                ownedColorPtrFor(material.surfaceShader.diffuseColor.value),
                1, AI_MATKEY_COLOR_DIFFUSE);
        mat->AddProperty(
                ownedColorPtrFor(material.surfaceShader.specularColor.value),
                1, AI_MATKEY_COLOR_SPECULAR);
        mat->AddProperty(
                ownedColorPtrFor(material.surfaceShader.emissiveColor.value),
                1, AI_MATKEY_COLOR_EMISSIVE);

        pScene->mMaterials[pScene->mNumMaterials] = mat;
        ++pScene->mNumMaterials;
    }
}

void USDImporterImplTinyusdz::textures(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    const size_t numTextures{render_scene.textures.size()};
    (void) numTextures; // Ignore unused variable when -Werror enabled
}

/**
 * "owned" as in, used "new" to allocate and aiScene now responsible for "delete"
 *
 * @param render_scene  renderScene object
 * @param image         textureImage object
 * @param nameWExt      filename w/ext (use to extract file type hint)
 * @return              aiTexture ptr
 */
static aiTexture *ownedEmbeddedTextureFor(
        const tinyusdz::tydra::RenderScene &render_scene,
        const tinyusdz::tydra::TextureImage &image,
        const std::string &nameWExt) {
    aiTexture *tex = new aiTexture();
    tex->mFilename.Set(image.asset_identifier.c_str());
    size_t pos = nameWExt.find_last_of('.');
    strncpy(tex->achFormatHint, nameWExt.substr(pos + 1).c_str(), 3);
    tex->mHeight = 0;
    tex->mWidth = render_scene.buffers[image.buffer_id].data.size();
    tex->pcData = (aiTexel *) new char[tex->mWidth];
    memcpy(tex->pcData, &render_scene.buffers[image.buffer_id].data[0], tex->mWidth);
    return tex;
}

void USDImporterImplTinyusdz::textureImages(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    const size_t numTextureImages{render_scene.images.size()};
    (void) numTextureImages; // Ignore unused variable when -Werror enabled
    pScene->mTextures = nullptr; // Need to iterate over images before knowing if valid textures available
    pScene->mNumTextures = 0;
    for (const auto &image : render_scene.images) {
        if (image.buffer_id > -1 &&
            image.buffer_id < render_scene.buffers.size() &&
            !render_scene.buffers[image.buffer_id].data.empty()) {
            aiTexture *tex = ownedEmbeddedTextureFor(
                    render_scene,
                    image,
                    nameWExt);
            if (pScene->mTextures == nullptr) {
                pScene->mTextures = new aiTexture *[render_scene.images.size()];
            }
            pScene->mTextures[pScene->mNumTextures++] = tex;
        }
    }
}

void USDImporterImplTinyusdz::buffers(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    const size_t numBuffers{render_scene.buffers.size()};
    (void) numBuffers; // Ignore unused variable when -Werror enabled
}

void USDImporterImplTinyusdz::animations(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    const size_t numAnimations{render_scene.animations.size()};
    (void) numAnimations; // Ignore unused variable when -Werror enabled
}

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_USD_IMPORTER
