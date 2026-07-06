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

#if (!defined AV_MESH_RENDERER_H_INCLUDED)
#define AV_MESH_RENDERER_H_INCLUDED

namespace AssimpView {


    //-------------------------------------------------------------------------------
    /// @brief Helper class tp render meshes.
    //-------------------------------------------------------------------------------
    class CMeshRenderer {
    public:
        /// @brief Destructor
        ~CMeshRenderer() = default;

        //------------------------------------------------------------------
        /// @brief Singleton accessors
        static CMeshRenderer s_cInstance;

        /// @brief Get the singleton instance of the mesh renderer
        /// @return Reference to the singleton instance of the mesh renderer
        inline static CMeshRenderer& Instance() {
            return s_cInstance;
        }

        //------------------------------------------------------------------
        /// @brief Draw a mesh in the global mesh list using the current pipeline state
        /// @param iIndex Index of the mesh to be drawn
        /// @return Result of the operation
        int DrawUnsorted( unsigned int iIndex );

        //------------------------------------------------------------------
        /// @brief Draw a mesh in the global mesh list using the current pipeline state
        /// @param iIndex Index of the mesh to be drawn
        /// @param mWorld World matrix for the node
        /// @return Result of the operation
        int DrawSorted( unsigned int iIndex, const aiMatrix4x4& mWorld );

    private:
        // default constructor
        CMeshRenderer() = default;
    };
}

#endif //!! include guard
