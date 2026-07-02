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
#include "Tangets.h"

namespace AssimpView {

    void Tangents::render(IDirect3DDevice9 *piDevice) {
        if (mScene == nullptr) {
            return;
        }   


        for (unsigned int i=0; i<mScene->mNumMeshes; i++) {
            const aiMesh *mesh = mScene->mMeshes[i];
            if (mesh == nullptr) {
                continue;
            }

            // create vertex buffer
            if (FAILED(piDevice->CreateVertexBuffer(sizeof(AssetHelper::LineVertex) *
                        mesh->mNumVertices * 2,
                        D3DUSAGE_WRITEONLY,
                        AssetHelper::LineVertex::GetFVF(),
                        D3DPOOL_DEFAULT, &pcMesh->piVBNormals, nullptr))) {
                CLogDisplay::Instance().AddEntry("Failed to create vertex buffer for the normal list", D3DCOLOR_ARGB(0xFF, 0xFF, 0, 0));
                return 2;
            }

            for (unsigned int x = 0; x < mesh->mNumVertices; ++x) {
                AssetHelper::LineVertex *pbData2;
                if (mesh->HasTangentsAndBitangents()) {
                    pbData2->vPosition = mesh->mVertices[x];
                    ++pbData2;

                    aiVector3D vTangent = mesh->mTangents[x];
                    vTangent.Normalize();

                    // scalo with the inverse of the world scaling to make sure
                    // the normals have equal length in each case
                    // TODO: Check whether this works in every case, I don't think so
                    vTangent.x /= g_mWorld.a1 * 4;
                    vTangent.y /= g_mWorld.b2 * 4;
                    vTangent.z /= g_mWorld.c3 * 4;

                    pbData2->vPosition = pcSource->mVertices[x] + vTangent;

                    ++pbData2;
                
                    
                }
            }
            pcMesh->piVBNormals->Unlock();
        }
    }

} // namespace AssimpView