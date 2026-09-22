/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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

/** @file  PLYLoader.h
 *  @brief Declaration of the .ply importer class.
 *
 *  Format notes (mesh faces vs. 3DGS f_dc_ / f_rest_): see doc/PLY.md
 */
#pragma once
#ifndef AI_PLYLOADER_H_INCLUDED
#define AI_PLYLOADER_H_INCLUDED

#include "PlyParser.h"
#include <assimp/BaseImporter.h>
#include <assimp/gaussian.h>
#include <assimp/types.h>
#include <vector>

struct aiNode;
struct aiMaterial;
struct aiMesh;

namespace Assimp {

using namespace PLY;

// ---------------------------------------------------------------------------
/// @brief Importer class to load the stanford PLY file format
// ---------------------------------------------------------------------------
class PLYImporter final : public BaseImporter {
public:
    /// @brief Default constructor
    PLYImporter() = default;

    /// @brief Destructor
    ~PLYImporter() override;

    // -------------------------------------------------------------------
    /// Returns whether the class can handle the format of the given file.
    /// @see BaseImporter::CanRead() for details.
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool checkSig) const override;

    // -------------------------------------------------------------------
    /// Extract a vertex from the DOM
    void LoadVertex(const PLY::Element *pcElement, const PLY::ElementInstance *instElement, unsigned int pos);

    // -------------------------------------------------------------------
    /// @brief Extract a face from the DOM
    void LoadFace(const PLY::Element *pcElement, const PLY::ElementInstance *instElement, unsigned int pos);

    /// @brief Will create a triangle from a triangle strip.
    void createFromeTriStrip(const Assimp::PLY::ElementInstance *instElement, unsigned int iProperty, Assimp::PLY::EDataType eType);

protected:
    const aiImporterDesc *GetInfo() const override;

    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) override;

    void LoadMaterial(std::vector<aiMaterial *> *pvOut, std::string &defaultTexture, const bool pointsOnly);

private:
    /// Detect 3DGS vertex layout and allocate side-channel storage once.
    void PrepareGaussianIfNeeded(const PLY::Element *pcElement);

    /// Fill one Gaussian from the current vertex instance (if active).
    void LoadGaussianVertex(const PLY::ElementInstance *instElement, unsigned int pos);

    unsigned char *mBuffer{ nullptr };
    PLY::DOM *pcDOM{ nullptr };
    aiMesh *mGeneratedMesh{ nullptr };

    bool mGaussianChecked{ false };
    bool mGaussianActive{ false };
    aiGaussianSplat *mGaussianSplat{ nullptr };

    // Property indices into Element::alProperties / ElementInstance::alProperties
    int mGsDc[3]{ -1, -1, -1 };
    int mGsScale[3]{ -1, -1, -1 };
    int mGsRot[4]{ -1, -1, -1, -1 };
    int mGsOpacity{ -1 };
    std::vector<int> mGsRest; // index per f_rest_i, -1 if missing
};

} // end of namespace Assimp

#endif // AI_PLYLOADER_H_INCLUDED
