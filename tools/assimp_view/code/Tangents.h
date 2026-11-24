/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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
#ifndef ASSIMPVIEW_TANGENTS_H_INC
#define ASSIMPVIEW_TANGENTS_H_INC

#include "AssetHelper.h"

struct aiMesh;
struct IDirect3DDevice9;

namespace AssimpView {

// ---------------------------------------------------------------------------------
/// @brief Class to generate tangents for a given aiMesh and create a vertex 
///        buffer for it, which can be used to render the tangents in the scene.
// ---------------------------------------------------------------------------------
class Tangents {
public: 
    /// @brief Constructor. Takes a pointer to an aiMesh for which tangents should be generated.
    /// @param mesh Pointer to the aiMesh for which tangents should be generated.
    explicit Tangents(const aiMesh *mesh) : mMesh(mesh) {
        // empty
    }

    /// @brief Destructor.
    ~Tangents() = default;

    /// @brief Creates vertex buffers for the generated tangents.
    /// @param piDevice Pointer to the Direct3D device.
    /// @param meshHelper Pointer to the mesh helper.
    /// @returns Result of the operation.
    int createBuffers(IDirect3DDevice9 *piDevice, AssetHelper::MeshHelper *meshHelper);

private:
    const aiMesh *mMesh{ nullptr };
};

} // namespace AssimpView

#endif // !defined ASSIMPVIEW_TANGENTS_H_INCASSIMPVIEW_TANGENTS_H_INC  
