/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

#include "EmbedTexturesProcess.h"
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/ParsingUtils.h>
#include "ProcessHelper.h"

#include <fstream>

using namespace Assimp;

EmbedTexturesProcess::EmbedTexturesProcess() :
        BaseProcess() {
    // empty
}

EmbedTexturesProcess::~EmbedTexturesProcess() {
    // empty
}

bool EmbedTexturesProcess::IsActive(unsigned int pFlags) const {
    return (pFlags & aiProcess_EmbedTextures) != 0;
}

void EmbedTexturesProcess::SetupProperties(const Importer* pImp) {
    mRootPath = pImp->GetPropertyString("sourceFilePath");
    mRootPath = mRootPath.substr(0, mRootPath.find_last_of("\\/") + 1u);
    mIOHandler = pImp->GetIOHandler();
}

void EmbedTexturesProcess::Execute(aiScene* pScene) {
    if (pScene == nullptr || pScene->mRootNode == nullptr || mIOHandler == nullptr){
        return;
    }

    aiString path;
    uint32_t embeddedTexturesCount = 0u;
    for (auto matId = 0u; matId < pScene->mNumMaterials; ++matId) {
        auto material = pScene->mMaterials[matId];

        for (auto ttId = 1u; ttId < AI_TEXTURE_TYPE_MAX; ++ttId) {
            auto tt = static_cast<aiTextureType>(ttId);
            auto texturesCount = material->GetTextureCount(tt);

            for (auto texId = 0u; texId < texturesCount; ++texId) {
                material->GetTexture(tt, texId, &path);
                if (path.data[0] == '*') continue; // Already embedded

                // Indeed embed
                if (addTexture(pScene, path.data)) {
                    auto embeddedTextureId = pScene->mNumTextures - 1u;
                    path.length = ::ai_snprintf(path.data, 1024, "*%u", embeddedTextureId);
                    material->AddProperty(&path, AI_MATKEY_TEXTURE(tt, texId));
                    embeddedTexturesCount++;
                }
            }
        }
    }

    ASSIMP_LOG_INFO("EmbedTexturesProcess finished. Embedded ", embeddedTexturesCount, " textures." );
}

bool EmbedTexturesProcess::addTexture(aiScene *pScene, const std::string &path) const {
    std::streampos imageSize = 0;
    std::string    imagePath = path;

    // Test path directly
    if (!mIOHandler->Exists(imagePath)) {
        ASSIMP_LOG_WARN("EmbedTexturesProcess: Cannot find image: ", imagePath, ". Will try to find it in root folder.");

        // Test path in root path
        imagePath = mRootPath + path;
        if (!mIOHandler->Exists(imagePath)) {
            // Test path basename in root path
            imagePath = mRootPath + path.substr(path.find_last_of("\\/") + 1u);
            if (!mIOHandler->Exists(imagePath)) {
                ASSIMP_LOG_ERROR("EmbedTexturesProcess: Unable to embed texture: ", path, ".");
                return false;
            }
        }
    }
    IOStream* pFile = mIOHandler->Open(imagePath);
    if (pFile == nullptr) {
        ASSIMP_LOG_ERROR("EmbedTexturesProcess: Unable to embed texture: ", path, ".");
        return false;
    }
    imageSize = pFile->FileSize();

    aiTexel* imageContent = new aiTexel[ 1ul + static_cast<unsigned long>( imageSize ) / sizeof(aiTexel)];
    pFile->Seek(0, aiOrigin_SET);
    pFile->Read(reinterpret_cast<char*>(imageContent), static_cast<size_t>(imageSize), 1);
    mIOHandler->Close(pFile);

    // Enlarging the textures table
    unsigned int textureId = pScene->mNumTextures++;
    auto oldTextures = pScene->mTextures;
    pScene->mTextures = new aiTexture*[pScene->mNumTextures];
    ::memmove(pScene->mTextures, oldTextures, sizeof(aiTexture*) * (pScene->mNumTextures - 1u));
    delete [] oldTextures;

    // Add the new texture
    auto pTexture = new aiTexture;
    pTexture->mHeight = 0; // Means that this is still compressed
    pTexture->mWidth = static_cast<uint32_t>(imageSize);
    pTexture->pcData = imageContent;

    auto extension = path.substr(path.find_last_of('.') + 1u);
    extension = ai_tolower(extension);
    if (extension == "jpeg") {
        extension = "jpg";
    }

    size_t len = extension.size();
    if (len > HINTMAXTEXTURELEN -1 ) {
        len = HINTMAXTEXTURELEN - 1;
    }
    ::strncpy(pTexture->achFormatHint, extension.c_str(), len);
    pScene->mTextures[textureId] = pTexture;

    return true;
}
