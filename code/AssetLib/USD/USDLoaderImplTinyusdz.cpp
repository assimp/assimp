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

namespace Assimp {
using namespace std;

void USDImporterImplTinyusdz::InternReadFile(
        const std::string &pFile,
        aiScene *pScene,
        IOSystem *pIOHandler) {
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
    for (size_t i = 0; i < pScene->mNumMeshes; i++) {
        pScene->mMeshes[i] = new aiMesh();
        verticesForMesh(render_scene, pScene, i);
        facesForMesh(render_scene, pScene, i);
        normalsForMesh(render_scene, pScene, i);
        uvsForMesh(render_scene, pScene, i);
        pScene->mRootNode->mMeshes[i] = static_cast<unsigned int>(i);
    }
}

void USDImporterImplTinyusdz::verticesForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx) {
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
        size_t meshIdx) {
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
        size_t meshIdx) {
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

void USDImporterImplTinyusdz::uvsForMesh(
        const tinyusdz::tydra::RenderScene &render_scene,
        aiScene *pScene,
        size_t meshIdx) {
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

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_USD_IMPORTER
