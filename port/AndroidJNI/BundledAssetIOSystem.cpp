/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

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

/** @file Android extension of DefaultIOSystem using the standard C file functions */

#include <assimp/ai_assert.h>
#include <android/asset_manager.h>
#include <assimp/DefaultIOStream.h>

#if __ANDROID__ and __ANDROID_API__ > 9 and defined(AI_CONFIG_ANDROID_JNI_ASSIMP_MANAGER_SUPPORT)

#include <assimp/port/AndroidJNI/BundledAssetIOSystem.h>

using namespace Assimp;

/** Tests for the existence of a file at the given path. */
bool BundledAssetIOSystem::Exists(const char* pFile) const {
    ai_assert(NULL != mApkAssetManager);
    AAsset *  asset = AAssetManager_open(mApkAssetManager, pFile, AASSET_MODE_UNKNOWN);
    if (!asset) { return false; }
    if (asset) AAsset_close(asset);
    return true;
}

// -------------------------------------------------------------------
/** Open a new file with a given path. */
Assimp::IOStream* BundledAssetIOSystem::Open(const char* pFile, const char* pMode) {
    ai_assert(NULL != mApkAssetManager);
    AAsset *  asset = AAssetManager_open(mApkAssetManager, pFile, AASSET_MODE_UNKNOWN);
    if (!asset) { return NULL; }

    return new AssetIOStream(asset);
}

// -------------------------------------------------------------------
/** Closes the given file and releases all resources associated with it. */
void BundledAssetIOSystem::Close(Assimp::IOStream* pFile) {
    delete reinterpret_cast<AssetIOStream *>(pFile);
}

#endif // __ANDROID__ and __ANDROID_API__ > 9 and defined(AI_CONFIG_ANDROID_JNI_ASSIMP_MANAGER_SUPPORT)

