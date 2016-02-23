/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team
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
#ifndef AI_PLYLOADER_H_INCLUDED
#define AI_PLYLOADER_H_INCLUDED

#include "BaseImporter.h"
#include "../include/assimp/types.h"
#include "PlyParser.h"
#include <vector>

struct aiNode;
struct aiMaterial;
struct aiMesh;

namespace Assimp    {


using namespace PLY;

// ---------------------------------------------------------------------------
/** Importer class to load the stanford PLY file format
*/
class PLYImporter : public BaseImporter
{
public:
    PLYImporter();
    ~PLYImporter();


public:

    // -------------------------------------------------------------------
    /** Returns whether the class can handle the format of the given file.
     * See BaseImporter::CanRead() for details.
     */
    bool CanRead( const std::string& pFile, IOSystem* pIOHandler,
        bool checkSig) const;

protected:

    // -------------------------------------------------------------------
    /** Return importer meta information.
     * See #BaseImporter::GetInfo for the details
     */
    const aiImporterDesc* GetInfo () const;

    // -------------------------------------------------------------------
    /** Imports the given file into the given scene structure.
    * See BaseImporter::InternReadFile() for details
    */
    void InternReadFile( const std::string& pFile, aiScene* pScene,
        IOSystem* pIOHandler);

protected:


    // -------------------------------------------------------------------
    /** Extract vertices from the DOM
    */
    void LoadVertices(std::vector<aiVector3D>* pvOut,
        bool p_bNormals = false);

    // -------------------------------------------------------------------
    /** Extract vertex color channels from the DOM
    */
    void LoadVertexColor(std::vector<aiColor4D>* pvOut);

    // -------------------------------------------------------------------
    /** Extract texture coordinate channels from the DOM
    */
    void LoadTextureCoordinates(std::vector<aiVector2D>* pvOut);

    // -------------------------------------------------------------------
    /** Extract a face list from the DOM
    */
    void LoadFaces(std::vector<PLY::Face>* pvOut);

    // -------------------------------------------------------------------
    /** Extract a material list from the DOM
    */
    void LoadMaterial(std::vector<aiMaterial*>* pvOut);


    // -------------------------------------------------------------------
    /** Validate material indices, replace default material identifiers
    */
    void ReplaceDefaultMaterial(std::vector<PLY::Face>* avFaces,
        std::vector<aiMaterial*>* avMaterials);


    // -------------------------------------------------------------------
    /** Convert all meshes into our ourer representation
    */
    void ConvertMeshes(std::vector<PLY::Face>* avFaces,
        const std::vector<aiVector3D>* avPositions,
        const std::vector<aiVector3D>* avNormals,
        const std::vector<aiColor4D>* avColors,
        const std::vector<aiVector2D>* avTexCoords,
        const std::vector<aiMaterial*>* avMaterials,
        std::vector<aiMesh*>* avOut);


    // -------------------------------------------------------------------
    /** Static helper to parse a color from four single channels in
    */
    static void GetMaterialColor(
        const std::vector<PLY::PropertyInstance>& avList,
        unsigned int aiPositions[4],
        PLY::EDataType aiTypes[4],
        aiColor4D* clrOut);


    // -------------------------------------------------------------------
    /** Static helper to parse a color channel value. The input value
    *  is normalized to 0-1.
    */
    static float NormalizeColorValue (
        PLY::PropertyInstance::ValueUnion val,
        PLY::EDataType eType);


    /** Buffer to hold the loaded file */
    unsigned char* mBuffer;

    /** Document object model representation extracted from the file */
    PLY::DOM* pcDOM;
};

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC
