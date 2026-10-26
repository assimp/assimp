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

/** @file 3DSExporter.h
 * 3DS Exporter Main Header
 */
#ifndef AI_3DSEXPORTER_H_INC
#define AI_3DSEXPORTER_H_INC

#include <map>
#include <memory>

#include <assimp/StreamWriter.h>
#include <assimp/material.h>

struct aiScene;
struct aiNode;
struct aiMaterial;
struct aiMesh;

namespace Assimp {

// ------------------------------------------------------------------------------------------------
/**
 *  @brief  Helper class to export a given scene to a 3DS file.
 */
// ------------------------------------------------------------------------------------------------
class Discreet3DSExporter final {
public:
    /// Constructor for a specific scene to export
    /// @param outfile Output file stream
    /// @param pScene Scene to export
    Discreet3DSExporter(std::shared_ptr<IOStream> &outfile, const aiScene* pScene);

    /// Destructor
    ~Discreet3DSExporter() = default;

private:
    /// Writes all meshes in the scene to the output file
    void WriteMeshes();

    /// Writes all materials in the scene to the output file
    void WriteMaterials();

    /// Writes texture information for a material
    /// @param mat Material to write texture for
    /// @param type Texture type (e.g., aiTextureType_DIFFUSE)
    /// @param chunk_flags 3DS chunk flags for the texture
    /// @return true if texture was written successfully, false otherwise
    bool WriteTexture(const aiMaterial& mat, aiTextureType type, uint16_t chunk_flags);

    /// Writes face material chunk for a mesh
    /// @param mesh Mesh to write face material for
    void WriteFaceMaterialChunk(const aiMesh& mesh);

    /// Writes the node hierarchy to the output file
    /// @param node Current node to process
    /// @param level Current hierarchy level
    /// @param sibling_level Sibling level for hierarchy tracking
    /// @return Next sequence number for hierarchy
    int WriteHierarchy(const aiNode& node, int level, int sibling_level);

    /// Writes a string to the output stream
    /// @param s String to write
    void WriteString(const std::string& s);

    /// Writes an aiString to the output stream
    /// @param s aiString to write
    void WriteString(const aiString& s);

    /// Writes a color to the output stream
    /// @param color Color to write
    void WriteColor(const aiColor3D& color);

    /// Writes a percentage value as float to the output stream
    /// @param f Percentage value to write
    void WritePercentChunk(float f);

    /// Writes a percentage value as double to the output stream
    /// @param f Percentage value to write
    void WritePercentChunk(double f);

private:
    /// The scene to be exported
    const aiScene* const scene;

    /// Stream writer for output
    StreamWriterLE writer;

    /// World transformations for each node
    std::map<const aiNode*, aiMatrix4x4> trafos;

    /// Meshes organized by node
    using MeshesByNodeMap = std::multimap<const aiNode*, unsigned int>;
    MeshesByNodeMap meshes;
};

} // Namespace Assimp

#endif // AI_3DSEXPORTER_H_INC
