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

#include "assimp_view.h"
#include "Tangents.h"
#include "winerror.h"

namespace AssimpView {

    int Tangents::createBuffers(IDirect3DDevice9 *piDevice, AssetHelper::MeshHelper *meshHelper) {
        if (mMesh == nullptr || meshHelper == nullptr) {
            return 1;
        }

        // create vertex buffer
        const UINT size = sizeof(AssetHelper::LineVertex) * mMesh->mNumVertices * 2;
        if (FAILED(piDevice->CreateVertexBuffer(size, D3DUSAGE_WRITEONLY,
                                                AssetHelper::LineVertex::GetFVF(),
                                                D3DPOOL_DEFAULT, &meshHelper->piTangents, nullptr))) {
            CLogDisplay::Instance().AddEntry("Failed to create vertex buffer for the tangent list", D3DCOLOR_ARGB(0xFF, 0xFF, 0, 0));
            return 2;
        }

        if (mMesh->HasTangentsAndBitangents()) {
            AssetHelper::LineVertex *pbData2{ nullptr };
            meshHelper->piTangents->Lock(0, 0, (void **)&pbData2, 0);
            for (unsigned int x = 0; x < mMesh->mNumVertices; ++x) {
                pbData2->vPosition = mMesh->mVertices[x];
                ++pbData2;

                aiVector3D vTangent = mMesh->mTangents[x];
                vTangent.Normalize();

                vTangent.x /= g_mWorld.a1 * 4;
                vTangent.y /= g_mWorld.b2 * 4;
                vTangent.z /= g_mWorld.c3 * 4;

                pbData2->vPosition = mMesh->mVertices[x] + vTangent;

                ++pbData2;  
            }
        }
        meshHelper->piTangents->Unlock();
        
        return 0;
    }

} // namespace AssimpView
