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

#ifndef ASSIMP_BUILD_NO_3MF_IMPORTER

#include "D3MFImporter.h"
#include "3MFXmlTags.h"
#include "D3MFOpcPackage.h"
#include "XmlSerializer.h"

#include <assimp/StringComparison.h>
#include <assimp/StringUtils.h>
#include <assimp/XmlParser.h>
#include <assimp/ZipArchiveIOSystem.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/fast_atof.h>

#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iomanip>
#include <cstring>

namespace Assimp {

using namespace D3MF;

static const aiImporterDesc desc = {
    "3mf Importer",
    "",
    "",
    "http://3mf.io/",
    aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_SupportCompressedFlavour,
    0,
    0,
    0,
    0,
    "3mf"
};

D3MFImporter::D3MFImporter() = default;

D3MFImporter::~D3MFImporter() = default;

bool D3MFImporter::CanRead(const std::string &filename, IOSystem *pIOHandler, bool /*checkSig*/) const {
    if (!ZipArchiveIOSystem::isZipArchive(pIOHandler, filename)) {
        return false;
    }
    D3MF::D3MFOpcPackage opcPackage(pIOHandler, filename);
    return opcPackage.validate();
}

void D3MFImporter::SetupProperties(const Importer*) {
    // empty
}

const aiImporterDesc *D3MFImporter::GetInfo() const {
    return &desc;
}

void D3MFImporter::InternReadFile(const std::string &filename, aiScene *pScene, IOSystem *pIOHandler) {
    D3MFOpcPackage opcPackage(pIOHandler, filename);

    XmlParser xmlParser;
    if (xmlParser.parse(opcPackage.RootStream())) {
        XmlSerializer xmlSerializer(&xmlParser);
        xmlSerializer.ImportXml(pScene);

        const std::vector<aiTexture*> &tex =  opcPackage.GetEmbeddedTextures();
        if (!tex.empty()) {
            pScene->mNumTextures = static_cast<unsigned int>(tex.size());
            pScene->mTextures = new aiTexture *[pScene->mNumTextures];
            for (unsigned int i = 0; i < pScene->mNumTextures; ++i) {
                pScene->mTextures[i] = tex[i];
            }
        }
    }
}

} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_3MF_IMPORTER
