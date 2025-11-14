/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team
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

/** @file Android implementation of IOSystem using the standard C file functions.
 * Aimed to ease the access to android assets */

#if __ANDROID__ and __ANDROID_API__ > 9 and defined(AI_CONFIG_ANDROID_JNI_ASSIMP_MANAGER_SUPPORT)
#ifndef AI_BUNDLEDASSETIOSYSTEM_H_INC
#define AI_BUNDLEDASSETIOSYSTEM_H_INC

#include <android/asset_manager_jni.h>

#include <assimp/DefaultIOSystem.h>
#include <assimp/IOStream.hpp>

namespace Assimp {

class BundledAssetIOSystem : public Assimp::DefaultIOSystem {

public:
    AAssetManager* mApkAssetManager;

    BundledAssetIOSystem(JNIEnv* env, jobject assetManager) { mApkAssetManager = AAssetManager_fromJava(env, assetManager); }
    ~BundledAssetIOSystem() {};

    bool Exists( const char* pFile) const;

    Assimp::IOStream* Open( const char* pFile, const char* pMode = "rb");

    void Close( Assimp::IOStream* pFile);

private:

    class AssetIOStream : public Assimp::IOStream {
        AAsset * asset;

    public:
        AssetIOStream(AAsset *asset) { this->asset = asset; };
        ~AssetIOStream() { AAsset_close(asset); }

        size_t Read(void* pvBuffer, size_t pSize, size_t pCount) { return AAsset_read(asset, pvBuffer, pSize * pCount);}
        size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) { return 0; };
        aiReturn Seek(size_t pOffset, aiOrigin pOrigin) { return (AAsset_seek(asset, pOffset, pOrigin) >= 0 ? aiReturn_SUCCESS : aiReturn_FAILURE); }
        size_t Tell() const { return(AAsset_getLength(asset) - AAsset_getRemainingLength(asset)); };
        size_t FileSize() const  { return  AAsset_getLength(asset); }
        void Flush() { }
    };

};


} //!ns Assimp

#endif //AI_BUNDLEDASSETIOSYSTEM_H_INC
#endif //__ANDROID__ and __ANDROID_API__ > 9 and defined(AI_CONFIG_ANDROID_JNI_ASSIMP_MANAGER_SUPPORT)

