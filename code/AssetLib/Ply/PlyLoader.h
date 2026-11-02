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
 */
#pragma once
#ifndef AI_PLYLOADER_H_INCLUDED
#define AI_PLYLOADER_H_INCLUDED

#include "PlyParser.h"
#include <assimp/BaseImporter.h>
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
    /// The function will also take care of the correct winding order of the triangles.
    /// @param pcElement The element containing the face data
    /// @param instElement The element instance containing the face data
    /// @param pos The position of the face in the element instance. This is needed to correctly assign the face to the mesh.
    void LoadFace(const PLY::Element *pcElement, const PLY::ElementInstance *instElement, unsigned int pos);

    /// @brief Will create a triangle from a triangle strip. 
    /// The function will use the first three vertices of the strip to create the first triangle, then the second,
    /// third and fourth vertex to create the second triangle and so on. The function will also take care of the correct 
    /// winding order of the triangles.
    /// @param instElement The element instance containing the triangle strip data
    /// @param iProperty   The index of the property containing the triangle strip data
    /// @param eType       The data type of the triangle strip data
    void createFromeTriStrip(const Assimp::PLY::ElementInstance *instElement, unsigned int iProperty, Assimp::PLY::EDataType eType);

protected:
    // -------------------------------------------------------------------
    const aiImporterDesc *GetInfo() const override;

    // -------------------------------------------------------------------
    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) override;

    // -------------------------------------------------------------------
    /// @brief  Extract a material list from the DOM    
    /// @param pvOut          The output material list. The function will fill the list with the extracted materials.
    /// @param defaultTexture The default texture to use for the materials. This is needed to correctly assign the texture to the materials.
    /// @param pointsOnly     Whether the file contains only points. This is needed to correctly assign the material properties.
    void LoadMaterial(std::vector<aiMaterial *> *pvOut, std::string &defaultTexture, const bool pointsOnly);

private:
    unsigned char *mBuffer{nullptr};
    PLY::DOM *pcDOM{nullptr};
    aiMesh *mGeneratedMesh{nullptr};
};

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC
