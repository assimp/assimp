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

#include "tydra/render-data.hh"
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
    bool ret{false};
    tinyusdz::USDLoadOptions options;
    tinyusdz::Stage stage;
    std::string warn, err;
    if (isUsdc(pFile)) {
        ret = LoadUSDCFromFile(pFile, &stage, &warn, &err, options);
    } else if(isUsda(pFile)) {
        ret = LoadUSDAFromFile(pFile, &stage, &warn, &err, options);
    } else if(isUsdz(pFile)) {
        ret = LoadUSDZFromFile(pFile, &stage, &warn, &err, options);
    } else if(isUsd(pFile)) {
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
        pScene->mMeshes[i]->mNumVertices = render_scene.meshes[i].points.size();
        pScene->mMeshes[i]->mVertices = new aiVector3D[pScene->mMeshes[i]->mNumVertices];
        for (size_t j = 0; j < pScene->mMeshes[i]->mNumVertices; ++j) {
            pScene->mMeshes[i]->mVertices[j].x = render_scene.meshes[i].points[j][0];
            pScene->mMeshes[i]->mVertices[j].y = render_scene.meshes[i].points[j][1];
            pScene->mMeshes[i]->mVertices[j].z = render_scene.meshes[i].points[j][2];
        }
        pScene->mRootNode->mMeshes[i] = static_cast<unsigned int>(i);
    }
}

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_USD_IMPORTER
