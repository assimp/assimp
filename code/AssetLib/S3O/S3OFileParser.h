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

/** @file  S3OFileParser.h
 *  @brief S3O File format parser
 */
#ifndef AI_S3OFILEPARSER_H_INC
#define AI_S3OFILEPARSER_H_INC
#ifndef ASSIMP_BUILD_NO_S3O_IMPORTER

#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/Exceptional.h>

#include <vector>

namespace Assimp {

struct IOSystem;

class ASSIMP_API S3OFileParser {
public:
    S3OFileParser(const std::string& pFile, aiScene *pScene, IOSystem* pIOHandler);
    ~S3OFileParser();

    /// @brief  The actual parser.
    void Parse();

protected:
    // -----------------------------------------------------------------------------------
    template <typename T>
    T Read(std::size_t pOffset) {
        throw DeadlyImportError("BAD Implementation");
    }

    void LoadNode(size_t offset, aiNode *parent);

private:
    std::vector<char> mBuffer;
    const std::string& mFile;
    IOSystem *mIOHandler;
    aiScene *mScene;

    std::vector<aiMesh *> mMeshMap;
};

} // namespace Assimp

#endif // !! ASSIMP_BUILD_NO_S3O_IMPORTER

#endif // AI_S3OFILEPARSER_H_INC