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

#pragma once

#include <map>

#include "AssetHelper.h"

namespace AssimpView {

//-------------------------------------------------------------------------------
/// @brief Helper class to create, access and destroy materials
//-------------------------------------------------------------------------------
class CMaterialManager {
    friend class CDisplay;

public:
    /// @brief Singleton accessors
    /// @return Reference to the singleton instance of CMaterialManager
    inline static CMaterialManager &Instance() {
        return s_cInstance;
    }

    /// @brief  Delete all resources of a given material
    ///         be called before CreateMaterial() to prevent memory leaking
    /// @param pcIn Pointer to the material to delete
    void DeleteMaterial(AssetHelper::MeshHelper *pcIn);

    /// @brief  Create the material for a mesh.
    ///
    /// The function checks whether an identical shader is already in use.
    /// A shader is considered to be identical if it has the same input
    /// signature and takes the same number of texture channels.
    int CreateMaterial(AssetHelper::MeshHelper *pcMesh, const aiMesh *pcSource);

    ///	@brief  Setup the material for a given mesh.
    /// @param  pcMesh   Mesh to be rendered
    /// @param  pcProj   Projection matrix
    /// @param  aiMe     Current world matrix
    /// @param  pcCam    Camera matrix
    /// @param  vPos     Position of the camera
    /// @return 0 if successful.
    int SetupMaterial(AssetHelper::MeshHelper *pcMesh,
            const aiMatrix4x4 &pcProj,
            const aiMatrix4x4 &aiMe,
            const aiMatrix4x4 &pcCam,
            const aiVector3D &vPos);

    /// @brief End the material for a given mesh
    /// @param pcMesh Mesh object
    /// @return 0 if successful.
    int EndMaterial(AssetHelper::MeshHelper *pcMesh);

    /// @brief Recreate all specular materials depending on the current
    /// @brief specularity settings
    ///
    /// Diffuse-only materials are ignored.
    /// Must be called after specular highlights have been toggled
    /// @return 0 if successful.
    int UpdateSpecularMaterials();

    /// @brief find a valid path to a texture file
    /// @param p_szString Pointer to the string containing the texture name
    /// @return 0 if successful.
    int FindValidPath(aiString *p_szString);

    /// @brief Load a texture into memory and create a native D3D texture resource
    /// @param p_ppiOut Pointer to the output texture pointer
    /// @param szPath Pointer to the string containing the texture path
    /// @return 0 if successful.
    int LoadTexture(IDirect3DTexture9 **p_ppiOut, aiString *szPath);

    /// @brief Get the number of different shaders generated for the current asset
    /// @return The shader count
    inline unsigned int GetShaderCount() {
        return this->m_iShaderCount;
    }

    //------------------------------------------------------------------
    // Reset the state of the class
    // Called whenever a new asset is loaded
    inline void Reset() {
        m_iShaderCount = 0;
        for (auto & sCachedTexture : sCachedTextures) {
            sCachedTexture.second->Release();
        }
        sCachedTextures.clear();
    }

private:
    // The default constructor
    CMaterialManager() :
            m_iShaderCount(0),
            sDefaultTexture() {
        // empty
    }

    // Destructor, private.
    ~CMaterialManager() {
        if (sDefaultTexture) {
            sDefaultTexture->Release();
        }
        Reset();
    }

    //------------------------------------------------------------------
    // find a valid path to a texture file
    //
    // Handle 8.3 syntax correctly, search the environment of the
    // executable and the asset for a texture with a name very similar
    // to a given one
    bool TryLongerPath(char *szTemp, aiString *p_szString);

    //------------------------------------------------------------------
    // Setup the default texture for a texture channel
    //
    // Generates a default checker pattern for a texture
    int SetDefaultTexture(IDirect3DTexture9 **p_ppiOut);

    //------------------------------------------------------------------
    // Convert a height map to a normal map if necessary
    //
    // The function tries to detect the type of a texture automatically.
    // However, this won't work in every case.
    void HMtoNMIfNecessary(IDirect3DTexture9 *piTexture,
            IDirect3DTexture9 **piTextureOut,
            bool bWasOriginallyHM = true);

    //------------------------------------------------------------------
    // Search for non-opaque pixels in a texture
    //
    // A pixel is considered to be non-opaque if its alpha value is
    // less than 255
    //------------------------------------------------------------------
    bool HasAlphaPixels(IDirect3DTexture9 *piTexture);

private:
    static CMaterialManager s_cInstance;

    // Specifies the number of different shaders generated for
    // the current asset. This number is incremented by CreateMaterial()
    // each time a shader isn't found in cache and needs to be created
    unsigned int m_iShaderCount;
    IDirect3DTexture9 *sDefaultTexture;
    using TextureCache = std::map<std::string, IDirect3DTexture9 *>;
    TextureCache sCachedTextures;
};

} // namespace AssimpView
