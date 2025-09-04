/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team
Copyright (c) 2019 bzt

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

/** @file M3DExporter.h
*   @brief Declares the exporter class to write a scene to a Model 3D file
*/
#ifndef AI_M3DEXPORTER_H_INC
#define AI_M3DEXPORTER_H_INC

#ifndef ASSIMP_BUILD_NO_M3D_IMPORTER
#ifndef ASSIMP_BUILD_NO_M3D_EXPORTER

#include <assimp/types.h>
#include <assimp/StreamWriter.h> // StreamWriterLE
#include <assimp/Exceptional.h> // DeadlyExportError

#include <memory> // shared_ptr

struct aiScene;
struct aiNode;
struct aiMaterial;
struct aiFace;

namespace Assimp {
    class IOSystem;
    class IOStream;
    class ExportProperties;

    class M3DWrapper;

    // ---------------------------------------------------------------------
    /** Helper class to export a given scene to an M3D file. */
    // ---------------------------------------------------------------------
    class M3DExporter {
    public:
        /// Constructor for a specific scene to export
        M3DExporter(const aiScene* pScene, const ExportProperties* pProperties);
        // call this to do the actual export
        void doExport(const char* pFile, IOSystem* pIOSystem, bool toAscii);

    private:
        const aiScene* mScene; // the scene to export
        const ExportProperties* mProperties; // currently unused
        std::shared_ptr<IOStream> outfile; // file to write to

        // helper to do the recursive walking
        void NodeWalk(const M3DWrapper &m3d, const aiNode* pNode, aiMatrix4x4 m);
    };
}

#endif // #ifndef ASSIMP_BUILD_NO_M3D_IMPORTER
#endif // ASSIMP_BUILD_NO_M3D_EXPORTER

#endif // AI_M3DEXPORTER_H_INC
