/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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

/** @file  USDLoader.cpp
 *  @brief Implementation of the USD importer class
 */

#ifndef ASSIMP_BUILD_NO_USD_IMPORTER
#include <memory>

// internal headers
#include <assimp/ai_assert.h>
#include <assimp/anim.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/fast_atof.h>
#include <assimp/Importer.hpp>
#include <assimp/importerdesc.h>
#include <assimp/IOStreamBuffer.h>
#include <assimp/IOSystem.hpp>
#include <assimp/scene.h>
#include <assimp/StringUtils.h>
#include <assimp/StreamReader.h>

#include "USDLoader.h"
#include "USDLoaderUtil.h"
#include "USDPreprocessor.h"

static constexpr aiImporterDesc desc = {
    "USD Object Importer",
    "",
    "",
    "https://en.wikipedia.org/wiki/Universal_Scene_Description/",
    aiImporterFlags_SupportTextFlavour | aiImporterFlags_SupportBinaryFlavour,
    0,
    0,
    0,
    0,
    "usd usda usdc usdz"
};

namespace Assimp {
using namespace std;

// Constructor to be privately used by Importer
USDImporter::USDImporter() :
    impl(USDImporterImplTinyusdz()) {
}

// ------------------------------------------------------------------------------------------------

bool USDImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler, bool) const {
    // Based on token
    static constexpr uint32_t usdcTokens[] = { AI_MAKE_MAGIC("PXR-USDC") };
    bool canRead = CheckMagicToken(pIOHandler, pFile, usdcTokens, AI_COUNT_OF(usdcTokens));
    if (canRead) {
        return canRead;
    }

    // Based on extension
    // TODO: confirm OK to replace this w/SimpleExtensionCheck() below
    canRead = isUsd(pFile) || isUsda(pFile) || isUsdc(pFile) || isUsdz(pFile);
    if (canRead) {
        return canRead;
    }
    canRead = SimpleExtensionCheck(pFile, "usd", "usda", "usdc", "usdz");
    return canRead;
}

const aiImporterDesc *USDImporter::GetInfo() const {
    return &desc;
}

void USDImporter::InternReadFile(
        const std::string &pFile,
        aiScene *pScene,
        IOSystem *pIOHandler) {
    impl.InternReadFile(
            pFile,
            pScene,
            pIOHandler);
}

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_USD_IMPORTER

