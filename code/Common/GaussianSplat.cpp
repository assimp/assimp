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

/** @file GaussianSplat.cpp
 *  @brief ABI-safe getters / storage for 3D Gaussian Splatting side data.
 */

#include "ScenePrivate.h"

#include <assimp/gaussian.h>
#include <assimp/scene.h>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
ASSIMP_API int aiSceneHasGaussianSplat(const aiScene *pScene) {
    if (pScene == nullptr || pScene->mPrivate == nullptr) {
        return 0;
    }
    const ScenePrivateData *priv = ScenePriv(pScene);
    for (const aiGaussianSplat *g : priv->mGaussianSplats) {
        if (g != nullptr) {
            return 1;
        }
    }
    return 0;
}

// ------------------------------------------------------------------------------------------------
ASSIMP_API const aiGaussianSplat *aiGetGaussianSplat(const aiScene *pScene, unsigned int meshIndex) {
    if (pScene == nullptr || pScene->mPrivate == nullptr) {
        return nullptr;
    }
    if (meshIndex >= pScene->mNumMeshes) {
        return nullptr;
    }
    const ScenePrivateData *priv = ScenePriv(pScene);
    if (meshIndex >= priv->mGaussianSplats.size()) {
        return nullptr;
    }
    return priv->mGaussianSplats[meshIndex];
}

// ------------------------------------------------------------------------------------------------
void Assimp::AttachGaussianSplat(aiScene *scene, unsigned int meshIndex, aiGaussianSplat *splat) {
    if (scene == nullptr || scene->mPrivate == nullptr || splat == nullptr) {
        delete splat;
        return;
    }
    ScenePrivateData *priv = ScenePriv(scene);
    if (meshIndex >= priv->mGaussianSplats.size()) {
        priv->mGaussianSplats.resize(static_cast<size_t>(meshIndex) + 1u, nullptr);
    }
    delete priv->mGaussianSplats[meshIndex];
    priv->mGaussianSplats[meshIndex] = splat;
}
