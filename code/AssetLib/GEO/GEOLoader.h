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

/** @file  GEOLoader.h
 *  @brief Declaration of the GEO importer class.
 */
#pragma once
#ifndef AI_GEOLOADER_H_INCLUDED
#define AI_GEOLOADER_H_INCLUDED

#include <assimp/BaseImporter.h>
#include <assimp/types.h>
#include <assimp/mesh.h>
#include <vector>

namespace Assimp {

// ---------------------------------------------------------------------------
/** Importer class for the Videoscape File Format (.geo)
 */
class GEOImporter : public BaseImporter {
public:
    GEOImporter();
    ~GEOImporter() override;

    // -------------------------------------------------------------------
    /** Returns whether the class can handle the format of the given file.
     * See BaseImporter::CanRead() for details. */
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler,
            bool checkSig) const override;

protected:
    // -------------------------------------------------------------------
    /** Return importer meta information.
     * See #BaseImporter::GetInfo for the details
     */
    const aiImporterDesc *GetInfo() const override;

    // -------------------------------------------------------------------
    void SetupProperties(const Importer *pImp) override;

    // -------------------------------------------------------------------
    /** Imports the given file into the given scene structure.
     * See BaseImporter::InternReadFile() for details
     */
    void InternReadFile(const std::string &pFile, aiScene *pScene,
            IOSystem *pIOHandler) override;
    void InternReadcF(unsigned int count);
    void InternReadncF(unsigned int count);
    void InternReadLamp(unsigned int count);
    void InternReadFbS(unsigned int count);
    void InternReadcV(unsigned int count);
    void InternReadncV(unsigned int count);
    void InternReadFinish();
    void InternReadColor(unsigned int pos);
    void LookupColor(long index, aiColor4D &cOut);

private:
    enum Flavor {
        Mesh_with_coloured_faces = 1,
        Mesh_with_coloured_vertices,
        Lamp,
        Gouraud_curves_or_NURBS_surfaces
    };
    Flavor flav;
    bool rgbH;
    aiScene *pScene;
    const char *buffer;
    const char *sz;
    char line[4096];
    std::vector<aiVector3D> tempPositions;
    std::vector<aiColor4D> tempColors;
    aiMesh *mesh;
    aiFace *faces;
    long lastcolor = 0; // TODO: reserved to detect material change.
    long color = 0;
    aiVector3D *verts = nullptr;
    [[maybe_unused]] aiColor4D *colOut = nullptr;
};

} // namespace Assimp

#endif // AI_GEOLOADER_H_INCLUDED
