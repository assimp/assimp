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

/** @file  USDLoader.h
 *  @brief Declaration of the USD importer class.
 */
#pragma once
#ifndef AI_USDLOADER_IMPL_TINYUSDZ_H_INCLUDED
#define AI_USDLOADER_IMPL_TINYUSDZ_H_INCLUDED

#include <assimp/BaseImporter.h>
#include <assimp/types.h>
#include <vector>
#include <cstdint>
#include "tinyusdz.hh"
#include "tydra/render-data.hh"

namespace Assimp {
class USDImporterImplTinyusdz {
public:
    USDImporterImplTinyusdz() = default;
    ~USDImporterImplTinyusdz() = default;

    void InternReadFile(
            const std::string &pFile,
            aiScene *pScene,
            IOSystem *pIOHandler);

    void verticesForMesh(
            const tinyusdz::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx);

    void facesForMesh(
            const tinyusdz::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx);

    void normalsForMesh(
            const tinyusdz::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx);

    void materialsForMesh(
            const tinyusdz::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx);

    void uvsForMesh(
            const tinyusdz::tydra::RenderScene &render_scene,
            aiScene *pScene,
            size_t meshIdx);
};
} // namespace Assimp
#endif // AI_USDLOADER_IMPL_TINYUSDZ_H_INCLUDED