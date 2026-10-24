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

/** @file  USDLoader.h
 *  @brief Declaration of the USD importer class.
 */
#pragma once

#ifndef AI_USDLOADER_IMPL_TINYUSDZ_H_INCLUDED
#define AI_USDLOADER_IMPL_TINYUSDZ_H_INCLUDED

#if defined(ASSIMP_BUILD_USD_BETA)
#include "lightusd.hh"
#else
#include "tinyusdz.hh"
#endif //#if defined(ASSIMP_BUILD_USD_BETA)
#include "tydra/render-data.hh"
#include <assimp/BaseImporter.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <cstdint>
#include <vector>

#if !defined(ASSIMP_BUILD_USD_BETA)
namespace lightusd = tinyusdz;
#endif //#if defined(ASSIMP_BUILD_USD_BETA)

namespace Assimp {

/**
 * @brief USD Importer implementation using tinyusdz.
 */
    class USDImporterImplTinyusdz {
    public:
        /// @brief The class constructor.
        USDImporterImplTinyusdz() = default;

        /// @brief The class destructor.
        ~USDImporterImplTinyusdz() = default;

        void InternReadFile(
                const std::string &pFile,
                aiScene *pScene,
                IOSystem *pIOHandler);

    void animations(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene);

    void meshes(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            const std::string &nameWExt);

    void verticesForMesh(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx,
            const std::string &nameWExt);

    void facesForMesh(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx,
            const std::string &nameWExt);

    void normalsForMesh(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx,
            const std::string &nameWExt);

    void materialsForMesh(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx,
            const std::string &nameWExt);

    void uvsForMesh(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx,
            const std::string &nameWExt);

    void bones(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            const std::string &nameWExt);

    void bonesForMesh(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx,
            const std::string &nameWExt);

    void materials(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            const std::string &nameWExt);

    void textures(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            const std::string &nameWExt);

    void textureImages(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            const std::string &nameWExt);

    void buffers(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            const std::string &nameWExt);

    aiNode *nodesRecursive(
            aiNode *pNodeParent,
            const lightusd::tydra::Node &node,
            const std::vector<lightusd::tydra::SkelHierarchy> &skeletons);

    aiNode *skeletonNodesRecursive(
            aiNode *pNodeParent,
            const lightusd::tydra::SkelNode &joint);

    void sanityCheckNodesRecursive(
            aiNode *pNode);

    void setupBlendShapes(
        const lightusd::tydra::RenderScene &render_scene,
        aiScene *pScene,
        const std::string &nameWExt);

    void blendShapesForMesh(
            const lightusd::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx,
            const std::string &nameWExt);
    };

} // namespace Assimp

#endif // AI_USDLOADER_IMPL_TINYUSDZ_H_INCLUDED
