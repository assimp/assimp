/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

/** @file  FBXImporter.h
*  @brief Declaration of the FBX main importer class
*/
#ifndef INCLUDED_AI_FBX_MESHGEOMETRY_H
#define INCLUDED_AI_FBX_MESHGEOMETRY_H

#include "FBXParser.h"
#include "FBXDocument.h"

namespace Assimp {
namespace FBX {

/**
 *  @brief DOM base class for all kinds of FBX geometry
 */
class Geometry : public Object {
public:
    /// @brief The class constructor with all parameters.
    /// @param id       The id.
    /// @param element  The element instance
    /// @param name     The name instance
    /// @param doc      The document instance
    Geometry( uint64_t id, const Element& element, const std::string& name, const Document& doc );

    /// @brief The class destructor, default.
    virtual ~Geometry() = default;

    /// @brief Get the Skin attached to this geometry or nullptr.
    /// @return The deformer skip instance as a pointer, nullptr if none.
    const Skin* DeformerSkin() const;

    /// @brief Get the BlendShape attached to this geometry or nullptr
    /// @return The blendshape arrays.
    const std::unordered_set<const BlendShape*>& GetBlendShapes() const;

private:
    const Skin* skin;
    std::unordered_set<const BlendShape*> blendShapes;

};

typedef std::vector<int> MatIndexArray;

/**
 *  @brief DOM class for FBX geometry of type "Mesh"
 */
class MeshGeometry : public Geometry {
public:
    /// @brief The class constructor
    /// @param id       The id.
    /// @param element  The element instance
    /// @param name     The name instance
    /// @param doc      The document instance
    MeshGeometry( uint64_t id, const Element& element, const std::string& name, const Document& doc );

    /// @brief The class destructor, default.
    virtual ~MeshGeometry() = default;

    /// brief Get a vector of all vertex points, non-unique.
    /// @return The vertices vector.
    const std::vector<aiVector3D>& GetVertices() const;

    /// @brief Get a vector of all vertex normals or an empty array if no normals are specified.
    /// @return The normal vector.
    const std::vector<aiVector3D>& GetNormals() const;

    /// @brief Get a vector of all vertex tangents or an empty array if no tangents are specified.
    /// @return The vertex tangents vector.
    const std::vector<aiVector3D>& GetTangents() const;

    /// @brief Get a vector of all vertex bi-normals or an empty array if no bi-normals are specified.
    /// @return The binomal vector.
    const std::vector<aiVector3D>& GetBinormals() const;

    /// @brief Return list of faces - each entry denotes a face and specifies how many vertices it has.
    ///        Vertices are taken from the vertex data arrays in sequential order.
    /// @return The face indices vector.
    const std::vector<unsigned int>& GetFaceIndexCounts() const;

    /// @brief Get a UV coordinate slot, returns an empty array if the requested slot does not exist.
    /// @param index    The requested texture coordinate slot.
    /// @return The texture coordinates.
    const std::vector<aiVector2D>& GetTextureCoords( unsigned int index ) const;

    /// @brief Get a UV coordinate slot, returns an empty array if the requested slot does not exist.
    /// @param index    The requested texture coordinate slot.
    /// @return The texture coordinate channel name.
    std::string GetTextureCoordChannelName( unsigned int index ) const;

    /// @brief Get a vertex color coordinate slot, returns an empty array if the requested slot does not exist.
    /// @param index    The requested texture coordinate slot.
    /// @return The vertex color vector.
    const std::vector<aiColor4D>& GetVertexColors( unsigned int index ) const;

    /// @brief Get per-face-vertex material assignments.
    /// @return The Material indices Array.
    const MatIndexArray& GetMaterialIndices() const;

    /// @brief Convert from a fbx file vertex index (for example from a #Cluster weight) or nullptr if the vertex index is not valid.
    /// @param in_index   The requested input index.
    /// @param count      The number of indices.
    /// @return The indices.
    const unsigned int* ToOutputVertexIndex( unsigned int in_index, unsigned int& count ) const;

    /// @brief Determine the face to which a particular output vertex index belongs.
    ///        This mapping is always unique.
    /// @param in_index   The requested input index.
    /// @return The face-to-vertex index.
    unsigned int FaceForVertexIndex( unsigned int in_index ) const;

private:
    void ReadLayer( const Scope& layer );
    void ReadLayerElement( const Scope& layerElement );
    void ReadVertexData( const std::string& type, int index, const Scope& source );

    void ReadVertexDataUV( std::vector<aiVector2D>& uv_out, const Scope& source,
        const std::string& MappingInformationType,
        const std::string& ReferenceInformationType );

    void ReadVertexDataNormals( std::vector<aiVector3D>& normals_out, const Scope& source,
        const std::string& MappingInformationType,
        const std::string& ReferenceInformationType );

    void ReadVertexDataColors( std::vector<aiColor4D>& colors_out, const Scope& source,
        const std::string& MappingInformationType,
        const std::string& ReferenceInformationType );

    void ReadVertexDataTangents( std::vector<aiVector3D>& tangents_out, const Scope& source,
        const std::string& MappingInformationType,
        const std::string& ReferenceInformationType );

    void ReadVertexDataBinormals( std::vector<aiVector3D>& binormals_out, const Scope& source,
        const std::string& MappingInformationType,
        const std::string& ReferenceInformationType );

    void ReadVertexDataMaterials( MatIndexArray& materials_out, const Scope& source,
        const std::string& MappingInformationType,
        const std::string& ReferenceInformationType );

private:
    // cached data arrays
    MatIndexArray m_materials;
    std::vector<aiVector3D> m_vertices;
    std::vector<unsigned int> m_faces;
    mutable std::vector<unsigned int> m_facesVertexStartIndices;
    std::vector<aiVector3D> m_tangents;
    std::vector<aiVector3D> m_binormals;
    std::vector<aiVector3D> m_normals;

    std::string m_uvNames[ AI_MAX_NUMBER_OF_TEXTURECOORDS ];
    std::vector<aiVector2D> m_uvs[ AI_MAX_NUMBER_OF_TEXTURECOORDS ];
    std::vector<aiColor4D> m_colors[ AI_MAX_NUMBER_OF_COLOR_SETS ];

    std::vector<unsigned int> m_mapping_counts;
    std::vector<unsigned int> m_mapping_offsets;
    std::vector<unsigned int> m_mappings;
};

/**
*  DOM class for FBX geometry of type "Shape"
*/
class ShapeGeometry : public Geometry
{
public:
    /** The class constructor */
    ShapeGeometry(uint64_t id, const Element& element, const std::string& name, const Document& doc);

    /** The class destructor */
    virtual ~ShapeGeometry();

    /** Get a list of all vertex points, non-unique*/
    const std::vector<aiVector3D>& GetVertices() const;

    /** Get a list of all vertex normals or an empty array if
    *  no normals are specified. */
    const std::vector<aiVector3D>& GetNormals() const;

    /** Return list of vertex indices. */
    const std::vector<unsigned int>& GetIndices() const;

private:
    std::vector<aiVector3D> m_vertices;
    std::vector<aiVector3D> m_normals;
    std::vector<unsigned int> m_indices;
};
/**
*  DOM class for FBX geometry of type "Line"
*/
class LineGeometry : public Geometry
{
public:
    /** The class constructor */
    LineGeometry(uint64_t id, const Element& element, const std::string& name, const Document& doc);

    /** The class destructor */
    virtual ~LineGeometry();

    /** Get a list of all vertex points, non-unique*/
    const std::vector<aiVector3D>& GetVertices() const;

    /** Return list of vertex indices. */
    const std::vector<int>& GetIndices() const;

private:
    std::vector<aiVector3D> m_vertices;
    std::vector<int> m_indices;
};

}
}

#endif // INCLUDED_AI_FBX_MESHGEOMETRY_H

