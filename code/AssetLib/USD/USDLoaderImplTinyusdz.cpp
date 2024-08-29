/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

/** @file  USDLoader.cpp
 *  @brief Implementation of the USD importer class
 */

#ifndef ASSIMP_BUILD_NO_USD_IMPORTER
#include <memory>
#include <sstream>

// internal headers
#include <assimp/ai_assert.h>
#include <assimp/anim.h>
#include <assimp/CreateAnimMesh.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/fast_atof.h>
#include <assimp/Importer.hpp>
#include <assimp/importerdesc.h>
#include <assimp/IOStreamBuffer.h>
#include <assimp/IOSystem.hpp>
#include <assimp/StringUtils.h>
#include <assimp/StreamReader.h>

#include "io-util.hh" // namespace tinyusdz::io
#include "tydra/scene-access.hh"
#include "tydra/shader-network.hh"
#include "USDLoaderImplTinyusdzHelper.h"
#include "USDLoaderImplTinyusdz.h"
#include "USDLoaderUtil.h"
#include "USDPreprocessor.h"

#include "../../../contrib/tinyusdz/assimp_tinyusdz_logging.inc"

namespace {
    static constexpr char TAG[] = "tinyusdz loader";
}

namespace Assimp {
using namespace std;

void USDImporterImplTinyusdz::InternReadFile(
        const std::string &pFile,
        aiScene *pScene,
        IOSystem *) {
    // Grab filename for logging purposes
    size_t pos = pFile.find_last_of('/');
    string basePath = pFile.substr(0, pos);
    string nameWExt = pFile.substr(pos + 1);
    stringstream ss;
    ss.str("");
    ss << "InternReadFile(): model" << nameWExt;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());

    bool ret{ false };
    tinyusdz::USDLoadOptions options;
    tinyusdz::Stage stage;
    std::string warn, err;
    bool is_usdz{ false };
    if (isUsdc(pFile)) {
        ret = LoadUSDCFromFile(pFile, &stage, &warn, &err, options);
        ss.str("");
        ss << "InternReadFile(): LoadUSDCFromFile() result: " << ret;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    } else if (isUsda(pFile)) {
        ret = LoadUSDAFromFile(pFile, &stage, &warn, &err, options);
        ss.str("");
        ss << "InternReadFile(): LoadUSDAFromFile() result: " << ret;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    } else if (isUsdz(pFile)) {
        ret = LoadUSDZFromFile(pFile, &stage, &warn, &err, options);
        is_usdz = true;
        ss.str("");
        ss << "InternReadFile(): LoadUSDZFromFile() result: " << ret;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    } else if (isUsd(pFile)) {
        ret = LoadUSDFromFile(pFile, &stage, &warn, &err, options);
        ss.str("");
        ss << "InternReadFile(): LoadUSDFromFile() result: " << ret;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    }
    if (warn.empty() && err.empty()) {
        ss.str("");
        ss << "InternReadFile(): load free of warnings/errors";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    } else {
        if (!warn.empty()) {
            ss.str("");
            ss << "InternReadFile(): WARNING reported: " << warn;
            TINYUSDZLOGW(TAG, "%s", ss.str().c_str());
        }
        if (!err.empty()) {
            ss.str("");
            ss << "InternReadFile(): ERROR reported: " << err;
            TINYUSDZLOGE(TAG, "%s", ss.str().c_str());
        }
    }
    if (!ret) {
        ss.str("");
        ss << "InternReadFile(): ERROR: load failed! ret: " << ret;
        TINYUSDZLOGE(TAG, "%s", ss.str().c_str());
        return;
    }
    tinyusdz::tydra::RenderScene render_scene;
    tinyusdz::tydra::RenderSceneConverter converter;
    tinyusdz::tydra::RenderSceneConverterEnv env(stage);
    std::string usd_basedir = tinyusdz::io::GetBaseDir(pFile);
    env.set_search_paths({ usd_basedir }); // {} needed to convert to vector of char

    // NOTE: Pointer address of usdz_asset must be valid until the call of RenderSceneConverter::ConvertToRenderScene.
    tinyusdz::USDZAsset usdz_asset;
    if (is_usdz) {
        if (!tinyusdz::ReadUSDZAssetInfoFromFile(pFile, &usdz_asset, &warn, &err)) {
            if (!warn.empty()) {
                ss.str("");
                ss << "InternReadFile(): ReadUSDZAssetInfoFromFile: WARNING reported: " << warn;
                TINYUSDZLOGW(TAG, "%s", ss.str().c_str());
            }
            if (!err.empty()) {
                ss.str("");
                ss << "InternReadFile(): ReadUSDZAssetInfoFromFile: ERROR reported: " << err;
                TINYUSDZLOGE(TAG, "%s", ss.str().c_str());
            }
            ss.str("");
            ss << "InternReadFile(): ReadUSDZAssetInfoFromFile: ERROR!";
            TINYUSDZLOGE(TAG, "%s", ss.str().c_str());
        } else {
            ss.str("");
            ss << "InternReadFile(): ReadUSDZAssetInfoFromFile: OK";
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        }

        tinyusdz::AssetResolutionResolver arr;
        if (!tinyusdz::SetupUSDZAssetResolution(arr, &usdz_asset)) {
            ss.str("");
            ss << "InternReadFile(): SetupUSDZAssetResolution: ERROR: load failed! ret: " << ret;
            TINYUSDZLOGE(TAG, "%s", ss.str().c_str());
        } else {
            ss.str("");
            ss << "InternReadFile(): SetupUSDZAssetResolution: OK";
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
            env.asset_resolver = arr;
        }
    }

    ret = converter.ConvertToRenderScene(env, &render_scene);
    if (!ret) {
        ss.str("");
        ss << "InternReadFile(): ConvertToRenderScene() failed!";
        TINYUSDZLOGE(TAG, "%s", ss.str().c_str());
        return;
    }

//    sanityCheckNodesRecursive(pScene->mRootNode);
    meshes(render_scene, pScene, nameWExt);
    materials(render_scene, pScene, nameWExt);
    textures(render_scene, pScene, nameWExt);
    textureImages(render_scene, pScene, nameWExt);
    buffers(render_scene, pScene, nameWExt);

    std::map<size_t, tinyusdz::tydra::Node> meshNodes;
    setupNodes(render_scene, pScene, meshNodes, nameWExt);

    setupBlendShapes(render_scene, pScene, nameWExt);
}

void USDImporterImplTinyusdz::meshes(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    stringstream ss;
    pScene->mNumMeshes = static_cast<unsigned int>(render_scene.meshes.size());
    pScene->mMeshes = new aiMesh *[pScene->mNumMeshes]();
    ss.str("");
    ss << "meshes(): pScene->mNumMeshes: " << pScene->mNumMeshes;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());

    // Export meshes
    for (size_t meshIdx = 0; meshIdx < pScene->mNumMeshes; meshIdx++) {
        pScene->mMeshes[meshIdx] = new aiMesh();
        pScene->mMeshes[meshIdx]->mName.Set(render_scene.meshes[meshIdx].prim_name);
        ss.str("");
        ss << "   mesh[" << meshIdx << "]: " <<
                render_scene.meshes[meshIdx].joint_and_weights.jointIndices.size() << " jointIndices, " <<
                render_scene.meshes[meshIdx].joint_and_weights.jointWeights.size() << " jointWeights, elementSize: " <<
                render_scene.meshes[meshIdx].joint_and_weights.elementSize;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        ss.str("");
        ss << "        skel_id: " << render_scene.meshes[meshIdx].skel_id;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        if (render_scene.meshes[meshIdx].material_id > -1) {
            pScene->mMeshes[meshIdx]->mMaterialIndex = render_scene.meshes[meshIdx].material_id;
        }
        verticesForMesh(render_scene, pScene, meshIdx, nameWExt);
        facesForMesh(render_scene, pScene, meshIdx, nameWExt);
        // Some models infer normals from faces, but others need them e.g.
        //   - apple "toy car" canopy normals will be wrong
        //   - human "untitled" model (tinyusdz issue #115) will be "splotchy"
        normalsForMesh(render_scene, pScene, meshIdx, nameWExt);
        materialsForMesh(render_scene, pScene, meshIdx, nameWExt);
        uvsForMesh(render_scene, pScene, meshIdx, nameWExt);
    }
}

void USDImporterImplTinyusdz::verticesForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
    UNUSED(nameWExt);
    pScene->mMeshes[meshIdx]->mNumVertices = static_cast<unsigned int>(render_scene.meshes[meshIdx].points.size());
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
    UNUSED(nameWExt);
    pScene->mMeshes[meshIdx]->mNumFaces = static_cast<unsigned int>(render_scene.meshes[meshIdx].faceVertexCounts().size());
    pScene->mMeshes[meshIdx]->mFaces = new aiFace[pScene->mMeshes[meshIdx]->mNumFaces]();
    size_t faceVertIdxOffset = 0;
    for (size_t faceIdx = 0; faceIdx < pScene->mMeshes[meshIdx]->mNumFaces; ++faceIdx) {
        pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices = render_scene.meshes[meshIdx].faceVertexCounts()[faceIdx];
        pScene->mMeshes[meshIdx]->mFaces[faceIdx].mIndices = new unsigned int[pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices];
        for (size_t j = 0; j < pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices; ++j) {
            pScene->mMeshes[meshIdx]->mFaces[faceIdx].mIndices[j] =
                    render_scene.meshes[meshIdx].faceVertexIndices()[j + faceVertIdxOffset];
        }
        faceVertIdxOffset += pScene->mMeshes[meshIdx]->mFaces[faceIdx].mNumIndices;
    }
}

void USDImporterImplTinyusdz::normalsForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
    UNUSED(nameWExt);
    pScene->mMeshes[meshIdx]->mNormals = new aiVector3D[pScene->mMeshes[meshIdx]->mNumVertices];
    const float *floatPtr = reinterpret_cast<const float *>(render_scene.meshes[meshIdx].normals.get_data().data());
    for (size_t vertIdx = 0, fpj = 0; vertIdx < pScene->mMeshes[meshIdx]->mNumVertices; ++vertIdx, fpj += 3) {
        pScene->mMeshes[meshIdx]->mNormals[vertIdx].x = floatPtr[fpj];
        pScene->mMeshes[meshIdx]->mNormals[vertIdx].y = floatPtr[fpj + 1];
        pScene->mMeshes[meshIdx]->mNormals[vertIdx].z = floatPtr[fpj + 2];
    }
}

void USDImporterImplTinyusdz::materialsForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
    UNUSED(render_scene); UNUSED(pScene); UNUSED(meshIdx); UNUSED(nameWExt);
}

void USDImporterImplTinyusdz::uvsForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
    UNUSED(nameWExt);
    const size_t uvSlotsCount = render_scene.meshes[meshIdx].texcoords.size();
    if (uvSlotsCount < 1) {
        return;
    }
    pScene->mMeshes[meshIdx]->mTextureCoords[0] = new aiVector3D[pScene->mMeshes[meshIdx]->mNumVertices];
    pScene->mMeshes[meshIdx]->mNumUVComponents[0] = 2; // U and V stored in "x", "y" of aiVector3D.
    for (unsigned int uvSlotIdx = 0; uvSlotIdx < uvSlotsCount; ++uvSlotIdx) {
        const auto uvsForSlot = render_scene.meshes[meshIdx].texcoords.at(uvSlotIdx);
        if (uvsForSlot.get_data().size() == 0) {
            continue;
        }
        const float *floatPtr = reinterpret_cast<const float *>(uvsForSlot.get_data().data());
        for (size_t vertIdx = 0, fpj = 0; vertIdx < pScene->mMeshes[meshIdx]->mNumVertices; ++vertIdx, fpj += 2) {
            pScene->mMeshes[meshIdx]->mTextureCoords[uvSlotIdx][vertIdx].x = floatPtr[fpj];
            pScene->mMeshes[meshIdx]->mTextureCoords[uvSlotIdx][vertIdx].y = floatPtr[fpj + 1];
        }
    }
}

static aiColor3D *ownedColorPtrFor(const std::array<float, 3> &color) {
    aiColor3D *colorPtr = new aiColor3D();
    colorPtr->r = color[0];
    colorPtr->g = color[1];
    colorPtr->b = color[2];
    return colorPtr;
}

static std::string nameForTextureWithId(
        const tinyusdz::tydra::RenderScene &render_scene,
        const int targetId) {
    stringstream ss;
    std::string texName;
    for (const auto &image : render_scene.images) {
        if (image.buffer_id == targetId) {
            texName = image.asset_identifier;
            ss.str("");
            ss << "nameForTextureWithId(): found texture " << texName << " with target id " << targetId;
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
            break;
        }
    }
    ss.str("");
    ss << "nameForTextureWithId(): ERROR!  Failed to find texture with target id " << targetId;
    TINYUSDZLOGE(TAG, "%s", ss.str().c_str());
    return texName;
}

static void assignTexture(
        const tinyusdz::tydra::RenderScene &render_scene,
        const tinyusdz::tydra::RenderMaterial &material,
        aiMaterial *mat,
        const int textureId,
        const int aiTextureType) {
    UNUSED(material);
    std::string name = nameForTextureWithId(render_scene, textureId);
    aiString *texName = new aiString();
    texName->Set(name);
    stringstream ss;
    ss.str("");
    ss << "assignTexture(): name: " << name;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    // TODO: verify hard-coded '0' index is correct
    mat->AddProperty(texName, _AI_MATKEY_TEXTURE_BASE, aiTextureType, 0);
}

void USDImporterImplTinyusdz::materials(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    const size_t numMaterials{render_scene.materials.size()};
    (void) numMaterials; // Ignore unused variable when -Werror enabled
    stringstream ss;
    ss.str("");
    ss << "materials(): model" << nameWExt << ", numMaterials: " << numMaterials;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    pScene->mNumMaterials = 0;
    if (render_scene.materials.empty()) {
        return;
    }
    pScene->mMaterials = new aiMaterial *[render_scene.materials.size()];
    for (const auto &material : render_scene.materials) {
        ss.str("");
        ss << "    material[" << pScene->mNumMaterials << "]: name: |" << material.name << "|, disp name: |" << material.display_name << "|";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
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

        ss.str("");
        if (material.surfaceShader.diffuseColor.is_texture()) {
            assignTexture(render_scene, material, mat, material.surfaceShader.diffuseColor.texture_id, aiTextureType_DIFFUSE);
            ss << "    material[" << pScene->mNumMaterials << "]: diff tex id " << material.surfaceShader.diffuseColor.texture_id << "\n";
        }
        if (material.surfaceShader.specularColor.is_texture()) {
            assignTexture(render_scene, material, mat, material.surfaceShader.specularColor.texture_id, aiTextureType_SPECULAR);
            ss << "    material[" << pScene->mNumMaterials << "]: spec tex id " << material.surfaceShader.specularColor.texture_id << "\n";
        }
        if (material.surfaceShader.normal.is_texture()) {
            assignTexture(render_scene, material, mat, material.surfaceShader.normal.texture_id, aiTextureType_NORMALS);
            ss << "    material[" << pScene->mNumMaterials << "]: normal tex id " << material.surfaceShader.normal.texture_id << "\n";
        }
        if (material.surfaceShader.emissiveColor.is_texture()) {
            assignTexture(render_scene, material, mat, material.surfaceShader.emissiveColor.texture_id, aiTextureType_EMISSIVE);
            ss << "    material[" << pScene->mNumMaterials << "]: emissive tex id " << material.surfaceShader.emissiveColor.texture_id << "\n";
        }
        if (material.surfaceShader.occlusion.is_texture()) {
            assignTexture(render_scene, material, mat, material.surfaceShader.occlusion.texture_id, aiTextureType_LIGHTMAP);
            ss << "    material[" << pScene->mNumMaterials << "]: lightmap (occlusion) tex id " << material.surfaceShader.occlusion.texture_id << "\n";
        }
        if (material.surfaceShader.metallic.is_texture()) {
            assignTexture(render_scene, material, mat, material.surfaceShader.metallic.texture_id, aiTextureType_METALNESS);
            ss << "    material[" << pScene->mNumMaterials << "]: metallic tex id " << material.surfaceShader.metallic.texture_id << "\n";
        }
        if (material.surfaceShader.roughness.is_texture()) {
            assignTexture(render_scene, material, mat, material.surfaceShader.roughness.texture_id, aiTextureType_DIFFUSE_ROUGHNESS);
            ss << "    material[" << pScene->mNumMaterials << "]: roughness tex id " << material.surfaceShader.roughness.texture_id << "\n";
        }
        if (material.surfaceShader.clearcoat.is_texture()) {
            assignTexture(render_scene, material, mat, material.surfaceShader.clearcoat.texture_id, aiTextureType_CLEARCOAT);
            ss << "    material[" << pScene->mNumMaterials << "]: clearcoat tex id " << material.surfaceShader.clearcoat.texture_id << "\n";
        }
        if (material.surfaceShader.opacity.is_texture()) {
            assignTexture(render_scene, material, mat, material.surfaceShader.opacity.texture_id, aiTextureType_OPACITY);
            ss << "    material[" << pScene->mNumMaterials << "]: opacity tex id " << material.surfaceShader.opacity.texture_id << "\n";
        }
        if (material.surfaceShader.displacement.is_texture()) {
            assignTexture(render_scene, material, mat, material.surfaceShader.displacement.texture_id, aiTextureType_DISPLACEMENT);
            ss << "    material[" << pScene->mNumMaterials << "]: displacement tex id " << material.surfaceShader.displacement.texture_id << "\n";
        }
        if (material.surfaceShader.clearcoatRoughness.is_texture()) {
            ss << "    material[" << pScene->mNumMaterials << "]: clearcoatRoughness tex id " << material.surfaceShader.clearcoatRoughness.texture_id << "\n";
        }
        if (material.surfaceShader.opacityThreshold.is_texture()) {
            ss << "    material[" << pScene->mNumMaterials << "]: opacityThreshold tex id " << material.surfaceShader.opacityThreshold.texture_id << "\n";
        }
        if (material.surfaceShader.ior.is_texture()) {
            ss << "    material[" << pScene->mNumMaterials << "]: ior tex id " << material.surfaceShader.ior.texture_id << "\n";
        }
        if (!ss.str().empty()) {
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        }

        pScene->mMaterials[pScene->mNumMaterials] = mat;
        ++pScene->mNumMaterials;
    }
}

void USDImporterImplTinyusdz::textures(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    UNUSED(pScene);
    const size_t numTextures{render_scene.textures.size()};
    UNUSED(numTextures); // Ignore unused variable when -Werror enabled
    stringstream ss;
    ss.str("");
    ss << "textures(): model" << nameWExt << ", numTextures: " << numTextures;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    size_t i{0};
    UNUSED(i);
    for (const auto &texture : render_scene.textures) {
        UNUSED(texture);
        ss.str("");
        ss << "    texture[" << i << "]: id: " << texture.texture_image_id << ", disp name: |" << texture.display_name << "|, varname_uv: " <<
                texture.varname_uv << ", prim_name: |" << texture.prim_name << "|, abs_path: |" << texture.abs_path << "|";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        ++i;
    }
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
    UNUSED(nameWExt);
    stringstream ss;
    aiTexture *tex = new aiTexture();
    size_t pos = image.asset_identifier.find_last_of('/');
    string embTexName{image.asset_identifier.substr(pos + 1)};
    tex->mFilename.Set(image.asset_identifier.c_str());
    tex->mHeight = image.height;
//    const size_t imageBytesCount{render_scene.buffers[image.buffer_id].data.size() / image.channels};
    tex->mWidth = image.width;
    if (tex->mHeight == 0) {
        pos = embTexName.find_last_of('.');
        strncpy(tex->achFormatHint, embTexName.substr(pos + 1).c_str(), 3);
        const size_t imageBytesCount{render_scene.buffers[image.buffer_id].data.size()};
        tex->pcData = (aiTexel *) new char[imageBytesCount];
        memcpy(tex->pcData, &render_scene.buffers[image.buffer_id].data[0], imageBytesCount);
    } else {
        string formatHint{"rgba8888"};
        strncpy(tex->achFormatHint, formatHint.c_str(), 8);
        const size_t imageTexelsCount{tex->mWidth * tex->mHeight};
        tex->pcData = (aiTexel *) new char[imageTexelsCount * image.channels];
        const float *floatPtr = reinterpret_cast<const float *>(&render_scene.buffers[image.buffer_id].data[0]);
        ss.str("");
        ss << "ownedEmbeddedTextureFor(): manual fill...";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        for (size_t i = 0, fpi = 0; i < imageTexelsCount; ++i, fpi += 4) {
            tex->pcData[i].b = static_cast<uint8_t>(floatPtr[fpi]     * 255);
            tex->pcData[i].g = static_cast<uint8_t>(floatPtr[fpi + 1] * 255);
            tex->pcData[i].r = static_cast<uint8_t>(floatPtr[fpi + 2] * 255);
            tex->pcData[i].a = static_cast<uint8_t>(floatPtr[fpi + 3] * 255);
        }
        ss.str("");
        ss << "ownedEmbeddedTextureFor(): imageTexelsCount: " << imageTexelsCount << ", channels: " << image.channels;
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    }
    return tex;
}

void USDImporterImplTinyusdz::textureImages(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    stringstream ss;
    const size_t numTextureImages{render_scene.images.size()};
    UNUSED(numTextureImages); // Ignore unused variable when -Werror enabled
    ss.str("");
    ss << "textureImages(): model" << nameWExt << ", numTextureImages: " << numTextureImages;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    pScene->mTextures = nullptr; // Need to iterate over images before knowing if valid textures available
    pScene->mNumTextures = 0;
    for (const auto &image : render_scene.images) {
        ss.str("");
        ss << "    image[" << pScene->mNumTextures << "]: |" << image.asset_identifier << "| w: " << image.width << ", h: " << image.height <<
           ", channels: " << image.channels << ", miplevel: " << image.miplevel << ", buffer id: " << image.buffer_id << "\n" <<
           "    buffers.size(): " << render_scene.buffers.size() << ", data empty? " << render_scene.buffers[image.buffer_id].data.empty();
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        if (image.buffer_id > -1 &&
            image.buffer_id < static_cast<long int>(render_scene.buffers.size()) &&
            !render_scene.buffers[image.buffer_id].data.empty()) {
            aiTexture *tex = ownedEmbeddedTextureFor(
                    render_scene,
                    image,
                    nameWExt);
            if (pScene->mTextures == nullptr) {
                ss.str("");
                ss << "    Init pScene->mTextures[" << render_scene.images.size() << "]";
                TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
                pScene->mTextures = new aiTexture *[render_scene.images.size()];
            }
            ss.str("");
            ss << "    pScene->mTextures[" << pScene->mNumTextures << "] name: |" << tex->mFilename.C_Str() <<
                    "|, w: " << tex->mWidth << ", h: " << tex->mHeight << ", hint: " << tex->achFormatHint;
            TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
            pScene->mTextures[pScene->mNumTextures++] = tex;
        }
    }
}

void USDImporterImplTinyusdz::buffers(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    const size_t numBuffers{render_scene.buffers.size()};
    UNUSED(pScene); UNUSED(numBuffers); // Ignore unused variable when -Werror enabled
    stringstream ss;
    ss.str("");
    ss << "buffers(): model" << nameWExt << ", numBuffers: " << numBuffers;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    size_t i = 0;
    for (const auto &buffer : render_scene.buffers) {
        ss.str("");
        ss << "    buffer[" << i << "]: count: " << buffer.data.size() << ", type: " << to_string(buffer.componentType);
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        ++i;
    }
}

void USDImporterImplTinyusdz::setupNodes(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        std::map<size_t, tinyusdz::tydra::Node> &meshNodes,
        const std::string &nameWExt) {
    stringstream ss;

    pScene->mRootNode = nodes(render_scene, meshNodes, nameWExt);
    pScene->mRootNode->mNumMeshes = pScene->mNumMeshes;
    pScene->mRootNode->mMeshes = new unsigned int[pScene->mRootNode->mNumMeshes];
    ss.str("");
    ss << "setupNodes(): pScene->mNumMeshes: " << pScene->mNumMeshes;
    if (pScene->mRootNode != nullptr) {
        ss << ", mRootNode->mNumMeshes: " << pScene->mRootNode->mNumMeshes;
    }
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());

    for (unsigned int meshIdx = 0; meshIdx < pScene->mNumMeshes; meshIdx++) {
        pScene->mRootNode->mMeshes[meshIdx] = meshIdx;
    }

}

aiNode *USDImporterImplTinyusdz::nodes(
        const tinyusdz::tydra::RenderScene &render_scene,
        std::map<size_t, tinyusdz::tydra::Node> &meshNodes,
        const std::string &nameWExt) {
    const size_t numNodes{render_scene.nodes.size()};
    (void) numNodes; // Ignore unused variable when -Werror enabled
    stringstream ss;
    ss.str("");
    ss << "nodes(): model" << nameWExt << ", numNodes: " << numNodes;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    return nodesRecursive(nullptr, render_scene.nodes[0], meshNodes);
}

using Assimp::tinyusdzNodeTypeFor;
using Assimp::tinyUsdzMat4ToAiMat4;
using tinyusdz::tydra::NodeType;
aiNode *USDImporterImplTinyusdz::nodesRecursive(
        aiNode *pNodeParent,
        const tinyusdz::tydra::Node &node,
        std::map<size_t, tinyusdz::tydra::Node> &meshNodes) {
    stringstream ss;
    aiNode *cNode = new aiNode();
    cNode->mParent = pNodeParent;
    cNode->mName.Set(node.prim_name);
    cNode->mTransformation = tinyUsdzMat4ToAiMat4(node.local_matrix.m);
    ss.str("");
    ss << "nodesRecursive(): node " << cNode->mName.C_Str() <<
            " type: |" << tinyusdzNodeTypeFor(node.nodeType) <<
            "|, disp " << node.display_name << ", abs " << node.abs_path;
    if (cNode->mParent != nullptr) {
        ss << " (parent " << cNode->mParent->mName.C_Str() << ")";
    }
    ss << " has " << node.children.size() << " children";
    if (node.id > -1) {
        ss << "\n    node mesh id: " << node.id << " (node type: " << tinyusdzNodeTypeFor(node.nodeType) << ")";
        meshNodes[node.id] = node;
    }
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    if (!node.children.empty()) {
        cNode->mNumChildren = static_cast<unsigned int>(node.children.size());
        cNode->mChildren = new aiNode *[cNode->mNumChildren];
    }

    size_t i{0};
    for (const auto &childNode: node.children) {
        cNode->mChildren[i] = nodesRecursive(cNode, childNode, meshNodes);
        ++i;
    }
    return cNode;
}

void USDImporterImplTinyusdz::sanityCheckNodesRecursive(
        aiNode *cNode) {
    stringstream ss;
    ss.str("");
    ss << "sanityCheckNodesRecursive(): node " << cNode->mName.C_Str();
    if (cNode->mParent != nullptr) {
        ss << " (parent " << cNode->mParent->mName.C_Str() << ")";
    }
    ss << " has " << cNode->mNumChildren << " children";
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    for (size_t i = 0; i < cNode->mNumChildren; ++i) {
        sanityCheckNodesRecursive(cNode->mChildren[i]);
    }
}

void USDImporterImplTinyusdz::setupBlendShapes(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt) {
    stringstream ss;
    ss.str("");
    ss << "setupBlendShapes(): iterating over " << pScene->mNumMeshes << " meshes for model" << nameWExt;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    for (size_t meshIdx = 0; meshIdx < pScene->mNumMeshes; meshIdx++) {
         blendShapesForMesh(render_scene, pScene, meshIdx, nameWExt);
    }
}

void USDImporterImplTinyusdz::blendShapesForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx,
        const std::string &nameWExt) {
    UNUSED(nameWExt);
    stringstream ss;
    const unsigned int numBlendShapeTargets{static_cast<unsigned int>(render_scene.meshes[meshIdx].targets.size())};
    UNUSED(numBlendShapeTargets); // Ignore unused variable when -Werror enabled
    ss.str("");
    ss << "    blendShapesForMesh(): mesh[" << meshIdx << "], numBlendShapeTargets: " << numBlendShapeTargets;
    TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
    if (numBlendShapeTargets > 0) {
        pScene->mMeshes[meshIdx]->mNumAnimMeshes = numBlendShapeTargets;
        pScene->mMeshes[meshIdx]->mAnimMeshes = new aiAnimMesh *[pScene->mMeshes[meshIdx]->mNumAnimMeshes];
    }
    auto mapIter = render_scene.meshes[meshIdx].targets.begin();
    size_t animMeshIdx{0};
    for (; mapIter != render_scene.meshes[meshIdx].targets.end(); ++mapIter) {
        const std::string name{mapIter->first};
        const tinyusdz::tydra::ShapeTarget shapeTarget{mapIter->second};
        pScene->mMeshes[meshIdx]->mAnimMeshes[animMeshIdx] = aiCreateAnimMesh(pScene->mMeshes[meshIdx]);
        ss.str("");
        ss << "        mAnimMeshes[" << animMeshIdx << "]: mNumVertices: " << pScene->mMeshes[meshIdx]->mAnimMeshes[animMeshIdx]->mNumVertices <<
                ", target: " << shapeTarget.pointIndices.size() << " pointIndices, " << shapeTarget.pointOffsets.size() <<
                " pointOffsets, " << shapeTarget.normalOffsets.size() << " normalOffsets";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        for (size_t iVert = 0; iVert < shapeTarget.pointOffsets.size(); ++iVert) {
            pScene->mMeshes[meshIdx]->mAnimMeshes[animMeshIdx]->mVertices[shapeTarget.pointIndices[iVert]] +=
                    tinyUsdzScaleOrPosToAssimp(shapeTarget.pointOffsets[iVert]);
        }
        for (size_t iVert = 0; iVert < shapeTarget.normalOffsets.size(); ++iVert) {
            pScene->mMeshes[meshIdx]->mAnimMeshes[animMeshIdx]->mNormals[shapeTarget.pointIndices[iVert]] +=
                    tinyUsdzScaleOrPosToAssimp(shapeTarget.normalOffsets[iVert]);
        }
        ss.str("");
        ss << "        target[" << animMeshIdx << "]: name: " << name << ", prim_name: " <<
                shapeTarget.prim_name << ", abs_path: " << shapeTarget.abs_path <<
                ", display_name: " << shapeTarget.display_name << ", " << shapeTarget.pointIndices.size() <<
                " pointIndices, " << shapeTarget.pointOffsets.size() << " pointOffsets, " <<
                shapeTarget.normalOffsets.size() << " normalOffsets, " << shapeTarget.inbetweens.size() <<
                " inbetweens";
        TINYUSDZLOGD(TAG, "%s", ss.str().c_str());
        ++animMeshIdx;
    }
}

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_USD_IMPORTER
