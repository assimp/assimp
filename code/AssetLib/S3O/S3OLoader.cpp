/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2023, assimp team


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

/** @file  S3OImporter.cpp
 *  @brief Implementation of the S3O importer class
 *
 */
#ifndef ASSIMP_BUILD_NO_S3O_IMPORTER

#include "S3OLoader.h"
#include "S3OHelper.hpp"
#include "S3OFileParser.h"

using namespace Assimp;

static const aiImporterDesc desc = {
    "S3O Importer",
    "",
    "",
    "",
    aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_Experimental,
    0,
    0,
    0,
    0,
    "s3o"
};

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
S3OImporter::S3OImporter() {
    // empty
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
S3OImporter::~S3OImporter() = default;

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file.
bool S3OImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler, bool /*checkSig*/) const {
    static const char *tokens[] = { S3OToken };

    return CheckMagicToken(pIOHandler, pFile, tokens, 1, 0, AI_COUNT_OF(tokens));
}

// ------------------------------------------------------------------------------------------------
// Setup configuration properties
void S3OImporter::SetupProperties(const Importer * /*pImp*/) {
    // nothing to be done for the moment
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure.
void S3OImporter::InternReadFile(const std::string &pFile,
        aiScene *pScene, IOSystem *pIOHandler) {

    auto parser = new S3OFileParser(pFile, pScene, pIOHandler);
    parser->Parse();
}

// ------------------------------------------------------------------------------------------------
// Loader registry entry
const aiImporterDesc *S3OImporter::GetInfo() const {
    return &desc;
}

#endif // !! ASSIMP_BUILD_NO_S3O_IMPORTER
