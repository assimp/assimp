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

#ifndef AI_D3MFLOADER_H_INCLUDED
#define AI_D3MFLOADER_H_INCLUDED

#ifndef ASSIMP_BUILD_NO_3MF_IMPORTER

#include <assimp/BaseImporter.h>

namespace Assimp {

// ---------------------------------------------------------------------------
/// @brief  The 3MF-importer class.
///
/// Implements the basic topology import and embedded textures.
// ---------------------------------------------------------------------------
class D3MFImporter : public BaseImporter {
public:
    /// @brief The default class constructor.
    D3MFImporter();

    ///	@brief  The class destructor.
    ~D3MFImporter() override;

    /// @brief Performs the data format detection.
    /// @param pFile        The filename to check.
    /// @param pIOHandler   The used IO-System.
    /// @param checkSig     true for signature checking.
    /// @return true for can be loaded, false for not.
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool checkSig) const override;

    /// @brief  Not used
    /// @param pImp Not used
    void SetupProperties(const Importer *pImp) override;

    /// @brief The importer description getter.
    /// @return The info
    const aiImporterDesc *GetInfo() const override;

protected:
    /// @brief Internal read function, performs the file parsing.
    /// @param pFile        The filename
    /// @param pScene       The scene to load in.
    /// @param pIOHandler   The io-system
    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) override;
};

} // Namespace Assimp

#endif // #ifndef ASSIMP_BUILD_NO_3MF_IMPORTER

#endif // AI_D3MFLOADER_H_INCLUDED
