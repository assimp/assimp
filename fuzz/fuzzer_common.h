/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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
#pragma once

#include <assimp/Importer.hpp>
#include <assimp/BaseImporter.h>
#include <assimp/importerdesc.h>
#include <cstring>
#include <vector>

namespace AssimpFuzz {

// Unregisters all loaders except the ones matching the given extension.
// Returns true if at least one loader was kept.
inline bool ForceFormat(Assimp::Importer& importer, const char* targetExtension) {
    size_t count = importer.GetImporterCount();
    std::vector<Assimp::BaseImporter*> toRemove;
    bool found = false;

    for (size_t i = 0; i < count; ++i) {
        const aiImporterDesc* desc = importer.GetImporterInfo(i);
        Assimp::BaseImporter* imp = importer.GetImporter(i);
        
        if (!desc || !imp) continue;

        // Check if the importer supports the target extension
        // mFileExtensions is a space-separated list (e.g., "obj mod")
        // We wrap target in spaces or check bounds to be precise, 
        // but for fuzzing, a simple strstr is usually sufficient 
        // if the target string is unique enough (e.g. "gltf", "obj").
        // A more robust check:
        
        bool isTarget = false;
        const char* extList = desc->mFileExtensions;
        if (!extList) {
            toRemove.push_back(imp);
            continue;
        }
        const size_t targetLen = strlen(targetExtension);

        const char* p = extList;
        while ((p = strstr(p, targetExtension)) != nullptr) {
            // Check boundaries
            const char prev = (p == extList) ? ' ' : *(p - 1);
            const char next = *(p + targetLen);
            
            if (prev == ' ' && (next == ' ' || next == '\0')) {
                isTarget = true;
                break;
            }
            p++;
        }

        if (isTarget) {
            found = true;
        } else {
            toRemove.push_back(imp);
        }
    }

    for (auto* imp : toRemove) {
        importer.UnregisterLoader(imp);
        delete imp;  // Free the unregistered importer to prevent memory leaks
    }

    return found;
}

}
