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

/** @file  UnrealLoader.h
 *  @brief Declaration of the .3d (UNREAL) importer class.
 */
#ifndef INCLUDED_AI_3D_LOADER_H
#define INCLUDED_AI_3D_LOADER_H

#include <assimp/BaseImporter.h>

namespace Assimp {

// ---------------------------------------------------------------------------
/**
 *  @brief Importer class to load UNREAL files (*.3d)
 */
class UnrealImporter : public BaseImporter {
public:
    /**
     *  @brief The class constructor.
     */
    UnrealImporter();

    /**
     *  @brief The class destructor.
     */
    ~UnrealImporter() override = default;

    // -------------------------------------------------------------------
    /** @brief Returns whether we can handle the format of the given file
     *
     *  See BaseImporter::CanRead() for details.
     **/
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool checkSig) const override;

protected:
    // -------------------------------------------------------------------
    /** @brief Called by Importer::GetExtensionList()
     *
     * @see #BaseImporter::GetInfo for the details
     */
    const aiImporterDesc *GetInfo() const override;

    // -------------------------------------------------------------------
    /** @brief Setup properties for the importer
     *
     * See BaseImporter::SetupProperties() for details
     */
    void SetupProperties(const Importer *pImp) override;

    // -------------------------------------------------------------------
    /** @brief Imports the given file into the given scene structure.
     *
     * See BaseImporter::InternReadFile() for details
     */
    void InternReadFile(const std::string &pFile, aiScene *pScene,
            IOSystem *pIOHandler) override;

private:
    //! frame to be loaded
    uint32_t mConfigFrameID;

    //! process surface flags
    bool mConfigHandleFlags;

}; // !class UnrealImporter

} // end of namespace Assimp

#endif // AI_UNREALIMPORTER_H_INC
