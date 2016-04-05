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

/** @file  FBXMeshGeometry.cpp
 *  @brief Assimp::FBX::MeshGeometry implementation
 */

#ifndef ASSIMP_BUILD_NO_FBX_IMPORTER

#include <functional>

#include "FBXMeshGeometry.h"
#include "FBXDocument.h"
#include "FBXImporter.h"
#include "FBXImportSettings.h"
#include "FBXDocumentUtil.h"


namespace Assimp {
namespace FBX {

using namespace Util;

// ------------------------------------------------------------------------------------------------
Geometry::Geometry(uint64_t id, const Element& element, const std::string& name, const Document& doc)
    : Object(id, element,name)
    , skin()
{
    const std::vector<const Connection*>& conns = doc.GetConnectionsByDestinationSequenced(ID(),"Deformer");
    for(const Connection* con : conns) {
        const Skin* const sk = ProcessSimpleConnection<Skin>(*con, false, "Skin -> Geometry", element);
        if(sk) {
            skin = sk;
            break;
        }
    }
}


// ------------------------------------------------------------------------------------------------
Geometry::~Geometry()
{

}

const Skin* Geometry::DeformerSkin() const {
    return skin;
}


// ------------------------------------------------------------------------------------------------
MeshGeometry::MeshGeometry(uint64_t id, const Element& element, const std::string& name, const Document& doc)
: Geometry(id, element,name, doc)
{
    const Scope* sc = element.Compound();
    if (!sc) {
        DOMError("failed to read Geometry object (class: Mesh), no data scope found");
    }

    // must have Mesh elements:
    const Element& Vertices = GetRequiredElement(*sc,"Vertices",&element);
    const Element& PolygonVertexIndex = GetRequiredElement(*sc,"PolygonVertexIndex",&element);

    // optional Mesh elements:
    const ElementCollection& Layer = sc->GetCollection("Layer");

    std::vector<aiVector3D> tempVerts;
    ParseVectorDataArray(tempVerts,Vertices);

    if(tempVerts.empty()) {
        FBXImporter::LogWarn("encountered mesh with no vertices");
        return;
    }

    std::vector<int> tempFaces;
    ParseVectorDataArray(tempFaces,PolygonVertexIndex);

    if(tempFaces.empty()) {
        FBXImporter::LogWarn("encountered mesh with no faces");
        return;
    }

    vertices.reserve(tempFaces.size());
    faces.reserve(tempFaces.size() / 3);

    mapping_offsets.resize(tempVerts.size());
    mapping_counts.resize(tempVerts.size(),0);
    mappings.resize(tempFaces.size());

    const size_t vertex_count = tempVerts.size();

    // generate output vertices, computing an adjacency table to
    // preserve the mapping from fbx indices to *this* indexing.
    unsigned int count = 0;
    for(int index : tempFaces) {
        const int absi = index < 0 ? (-index - 1) : index;
        if(static_cast<size_t>(absi) >= vertex_count) {
            DOMError("polygon vertex index out of range",&PolygonVertexIndex);
        }

        vertices.push_back(tempVerts[absi]);
        ++count;

        ++mapping_counts[absi];

        if (index < 0) {
            faces.push_back(count);
            count = 0;
        }
    }

    unsigned int cursor = 0;
    for (size_t i = 0, e = tempVerts.size(); i < e; ++i) {
        mapping_offsets[i] = cursor;
        cursor += mapping_counts[i];

        mapping_counts[i] = 0;
    }

    cursor = 0;
    for(int index : tempFaces) {
        const int absi = index < 0 ? (-index - 1) : index;
        mappings[mapping_offsets[absi] + mapping_counts[absi]++] = cursor++;
    }

    // if settings.readAllLayers is true:
    //  * read all layers, try to load as many vertex channels as possible
    // if settings.readAllLayers is false:
    //  * read only the layer with index 0, but warn about any further layers
    for (ElementMap::const_iterator it = Layer.first; it != Layer.second; ++it) {
        const TokenList& tokens = (*it).second->Tokens();

        const char* err;
        const int index = ParseTokenAsInt(*tokens[0], err);
        if(err) {
            DOMError(err,&element);
        }

        if(doc.Settings().readAllLayers || index == 0) {
            const Scope& layer = GetRequiredScope(*(*it).second);
            ReadLayer(layer);
        }
        else {
            FBXImporter::LogWarn("ignoring additional geometry layers");
        }
    }
}

// ------------------------------------------------------------------------------------------------
MeshGeometry::~MeshGeometry()
{

}

// ------------------------------------------------------------------------------------------------
const std::vector<aiVector3D>& MeshGeometry::GetVertices() const {
    return vertices;
}

// ------------------------------------------------------------------------------------------------
const std::vector<aiVector3D>& MeshGeometry::GetNormals() const {
    return normals;
}

// ------------------------------------------------------------------------------------------------
const std::vector<aiVector3D>& MeshGeometry::GetTangents() const {
    return tangents;
}

// ------------------------------------------------------------------------------------------------
const std::vector<aiVector3D>& MeshGeometry::GetBinormals() const {
    return binormals;
}

// ------------------------------------------------------------------------------------------------
const std::vector<unsigned int>& MeshGeometry::GetFaceIndexCounts() const {
    return faces;
}

// ------------------------------------------------------------------------------------------------
const std::vector<aiVector2D>& MeshGeometry::GetTextureCoords( unsigned int index ) const {
    static const std::vector<aiVector2D> empty;
    return index >= AI_MAX_NUMBER_OF_TEXTURECOORDS ? empty : uvs[ index ];
}

std::string MeshGeometry::GetTextureCoordChannelName( unsigned int index ) const {
    return index >= AI_MAX_NUMBER_OF_TEXTURECOORDS ? "" : uvNames[ index ];
}

const std::vector<aiColor4D>& MeshGeometry::GetVertexColors( unsigned int index ) const {
    static const std::vector<aiColor4D> empty;
    return index >= AI_MAX_NUMBER_OF_COLOR_SETS ? empty : colors[ index ];
}

const MatIndexArray& MeshGeometry::GetMaterialIndices() const {
    return materials;
}

// ------------------------------------------------------------------------------------------------
const unsigned int* MeshGeometry::ToOutputVertexIndex( unsigned int in_index, unsigned int& count ) const {
    if ( in_index >= mapping_counts.size() ) {
        return NULL;
    }

    ai_assert( mapping_counts.size() == mapping_offsets.size() );
    count = mapping_counts[ in_index ];

    ai_assert( count != 0 );
    ai_assert( mapping_offsets[ in_index ] + count <= mappings.size() );

    return &mappings[ mapping_offsets[ in_index ] ];
}

// ------------------------------------------------------------------------------------------------
unsigned int MeshGeometry::FaceForVertexIndex( unsigned int in_index ) const {
    ai_assert( in_index < vertices.size() );

    // in the current conversion pattern this will only be needed if
    // weights are present, so no need to always pre-compute this table
    if ( facesVertexStartIndices.empty() ) {
        facesVertexStartIndices.resize( faces.size() + 1, 0 );

        std::partial_sum( faces.begin(), faces.end(), facesVertexStartIndices.begin() + 1 );
        facesVertexStartIndices.pop_back();
    }

    ai_assert( facesVertexStartIndices.size() == faces.size() );
    const std::vector<unsigned int>::iterator it = std::upper_bound(
        facesVertexStartIndices.begin(),
        facesVertexStartIndices.end(),
        in_index
        );

    return static_cast< unsigned int >( std::distance( facesVertexStartIndices.begin(), it - 1 ) );
}

// ------------------------------------------------------------------------------------------------
void MeshGeometry::ReadLayer(const Scope& layer)
{
    const ElementCollection& LayerElement = layer.GetCollection("LayerElement");
    for (ElementMap::const_iterator eit = LayerElement.first; eit != LayerElement.second; ++eit) {
        const Scope& elayer = GetRequiredScope(*(*eit).second);

        ReadLayerElement(elayer);
    }
}


// ------------------------------------------------------------------------------------------------
void MeshGeometry::ReadLayerElement(const Scope& layerElement)
{
    const Element& Type = GetRequiredElement(layerElement,"Type");
    const Element& TypedIndex = GetRequiredElement(layerElement,"TypedIndex");

    const std::string& type = ParseTokenAsString(GetRequiredToken(Type,0));
    const int typedIndex = ParseTokenAsInt(GetRequiredToken(TypedIndex,0));

    const Scope& top = GetRequiredScope(element);
    const ElementCollection candidates = top.GetCollection(type);

    for (ElementMap::const_iterator it = candidates.first; it != candidates.second; ++it) {
        const int index = ParseTokenAsInt(GetRequiredToken(*(*it).second,0));
        if(index == typedIndex) {
            ReadVertexData(type,typedIndex,GetRequiredScope(*(*it).second));
            return;
        }
    }

    FBXImporter::LogError(Formatter::format("failed to resolve vertex layer element: ")
        << type << ", index: " << typedIndex);
}


// ------------------------------------------------------------------------------------------------
void MeshGeometry::ReadVertexData(const std::string& type, int index, const Scope& source)
{
    const std::string& MappingInformationType = ParseTokenAsString(GetRequiredToken(
        GetRequiredElement(source,"MappingInformationType"),0)
    );

    const std::string& ReferenceInformationType = ParseTokenAsString(GetRequiredToken(
        GetRequiredElement(source,"ReferenceInformationType"),0)
    );

    if (type == "LayerElementUV") {
        if(index >= AI_MAX_NUMBER_OF_TEXTURECOORDS) {
            FBXImporter::LogError(Formatter::format("ignoring UV layer, maximum number of UV channels exceeded: ")
                << index << " (limit is " << AI_MAX_NUMBER_OF_TEXTURECOORDS << ")" );
            return;
        }

        const Element* Name = source["Name"];
        uvNames[index] = "";
        if(Name) {
            uvNames[index] = ParseTokenAsString(GetRequiredToken(*Name,0));
        }

        ReadVertexDataUV(uvs[index],source,
            MappingInformationType,
            ReferenceInformationType
        );
    }
    else if (type == "LayerElementMaterial") {
        if (materials.size() > 0) {
            FBXImporter::LogError("ignoring additional material layer");
            return;
        }

        std::vector<int> temp_materials;

        ReadVertexDataMaterials(temp_materials,source,
            MappingInformationType,
            ReferenceInformationType
        );

        // sometimes, there will be only negative entries. Drop the material
        // layer in such a case (I guess it means a default material should
        // be used). This is what the converter would do anyway, and it
        // avoids losing the material if there are more material layers
        // coming of which at least one contains actual data (did observe
        // that with one test file).
        const size_t count_neg = std::count_if(temp_materials.begin(),temp_materials.end(),std::bind2nd(std::less<int>(),0));
        if(count_neg == temp_materials.size()) {
            FBXImporter::LogWarn("ignoring dummy material layer (all entries -1)");
            return;
        }

        std::swap(temp_materials, materials);
    }
    else if (type == "LayerElementNormal") {
        if (normals.size() > 0) {
            FBXImporter::LogError("ignoring additional normal layer");
            return;
        }

        ReadVertexDataNormals(normals,source,
            MappingInformationType,
            ReferenceInformationType
        );
    }
    else if (type == "LayerElementTangent") {
        if (tangents.size() > 0) {
            FBXImporter::LogError("ignoring additional tangent layer");
            return;
        }

        ReadVertexDataTangents(tangents,source,
            MappingInformationType,
            ReferenceInformationType
        );
    }
    else if (type == "LayerElementBinormal") {
        if (binormals.size() > 0) {
            FBXImporter::LogError("ignoring additional binormal layer");
            return;
        }

        ReadVertexDataBinormals(binormals,source,
            MappingInformationType,
            ReferenceInformationType
        );
    }
    else if (type == "LayerElementColor") {
        if(index >= AI_MAX_NUMBER_OF_COLOR_SETS) {
            FBXImporter::LogError(Formatter::format("ignoring vertex color layer, maximum number of color sets exceeded: ")
                << index << " (limit is " << AI_MAX_NUMBER_OF_COLOR_SETS << ")" );
            return;
        }

        ReadVertexDataColors(colors[index],source,
            MappingInformationType,
            ReferenceInformationType
        );
    }
}


// ------------------------------------------------------------------------------------------------
// Lengthy utility function to read and resolve a FBX vertex data array - that is, the
// output is in polygon vertex order. This logic is used for reading normals, UVs, colors,
// tangents ..
template <typename T>
void ResolveVertexDataArray(std::vector<T>& data_out, const Scope& source,
    const std::string& MappingInformationType,
    const std::string& ReferenceInformationType,
    const char* dataElementName,
    const char* indexDataElementName,
    size_t vertex_count,
    const std::vector<unsigned int>& mapping_counts,
    const std::vector<unsigned int>& mapping_offsets,
    const std::vector<unsigned int>& mappings)
{


    // handle permutations of Mapping and Reference type - it would be nice to
    // deal with this more elegantly and with less redundancy, but right
    // now it seems unavoidable.
    if (MappingInformationType == "ByVertice" && ReferenceInformationType == "Direct") {
		std::vector<T> tempData;
		ParseVectorDataArray(tempData, GetRequiredElement(source, dataElementName));

        data_out.resize(vertex_count);
		for (size_t i = 0, e = tempData.size(); i < e; ++i) {

            const unsigned int istart = mapping_offsets[i], iend = istart + mapping_counts[i];
            for (unsigned int j = istart; j < iend; ++j) {
				data_out[mappings[j]] = tempData[i];
            }
        }
    }
    else if (MappingInformationType == "ByVertice" && ReferenceInformationType == "IndexToDirect") {
		std::vector<T> tempData;
		ParseVectorDataArray(tempData, GetRequiredElement(source, dataElementName));

        data_out.resize(vertex_count);

        std::vector<int> uvIndices;
        ParseVectorDataArray(uvIndices,GetRequiredElement(source,indexDataElementName));

        for (size_t i = 0, e = uvIndices.size(); i < e; ++i) {

            const unsigned int istart = mapping_offsets[i], iend = istart + mapping_counts[i];
            for (unsigned int j = istart; j < iend; ++j) {
				if (static_cast<size_t>(uvIndices[i]) >= tempData.size()) {
                    DOMError("index out of range",&GetRequiredElement(source,indexDataElementName));
                }
				data_out[mappings[j]] = tempData[uvIndices[i]];
            }
        }
    }
    else if (MappingInformationType == "ByPolygonVertex" && ReferenceInformationType == "Direct") {
		std::vector<T> tempData;
		ParseVectorDataArray(tempData, GetRequiredElement(source, dataElementName));

		if (tempData.size() != vertex_count) {
            FBXImporter::LogError(Formatter::format("length of input data unexpected for ByPolygon mapping: ")
				<< tempData.size() << ", expected " << vertex_count
            );
            return;
        }

		data_out.swap(tempData);
    }
    else if (MappingInformationType == "ByPolygonVertex" && ReferenceInformationType == "IndexToDirect") {
		std::vector<T> tempData;
		ParseVectorDataArray(tempData, GetRequiredElement(source, dataElementName));

        data_out.resize(vertex_count);

        std::vector<int> uvIndices;
        ParseVectorDataArray(uvIndices,GetRequiredElement(source,indexDataElementName));

        if (uvIndices.size() != vertex_count) {
            FBXImporter::LogError("length of input data unexpected for ByPolygonVertex mapping");
            return;
        }

        unsigned int next = 0;
        for(int i : uvIndices) {
			if (static_cast<size_t>(i) >= tempData.size()) {
                DOMError("index out of range",&GetRequiredElement(source,indexDataElementName));
            }

			data_out[next++] = tempData[i];
        }
    }
    else {
        FBXImporter::LogError(Formatter::format("ignoring vertex data channel, access type not implemented: ")
            << MappingInformationType << "," << ReferenceInformationType);
    }
}

// ------------------------------------------------------------------------------------------------
void MeshGeometry::ReadVertexDataNormals(std::vector<aiVector3D>& normals_out, const Scope& source,
    const std::string& MappingInformationType,
    const std::string& ReferenceInformationType)
{
    ResolveVertexDataArray(normals_out,source,MappingInformationType,ReferenceInformationType,
        "Normals",
        "NormalsIndex",
        vertices.size(),
        mapping_counts,
        mapping_offsets,
        mappings);
}


// ------------------------------------------------------------------------------------------------
void MeshGeometry::ReadVertexDataUV(std::vector<aiVector2D>& uv_out, const Scope& source,
    const std::string& MappingInformationType,
    const std::string& ReferenceInformationType)
{
    ResolveVertexDataArray(uv_out,source,MappingInformationType,ReferenceInformationType,
        "UV",
        "UVIndex",
        vertices.size(),
        mapping_counts,
        mapping_offsets,
        mappings);
}


// ------------------------------------------------------------------------------------------------
void MeshGeometry::ReadVertexDataColors(std::vector<aiColor4D>& colors_out, const Scope& source,
    const std::string& MappingInformationType,
    const std::string& ReferenceInformationType)
{
    ResolveVertexDataArray(colors_out,source,MappingInformationType,ReferenceInformationType,
        "Colors",
        "ColorIndex",
        vertices.size(),
        mapping_counts,
        mapping_offsets,
        mappings);
}

// ------------------------------------------------------------------------------------------------
static const std::string TangentIndexToken = "TangentIndex";
static const std::string TangentsIndexToken = "TangentsIndex";

void MeshGeometry::ReadVertexDataTangents(std::vector<aiVector3D>& tangents_out, const Scope& source,
    const std::string& MappingInformationType,
    const std::string& ReferenceInformationType)
{
    const char * str = source.Elements().count( "Tangents" ) > 0 ? "Tangents" : "Tangent";
    const char * strIdx = source.Elements().count( "Tangents" ) > 0 ? TangentsIndexToken.c_str() : TangentIndexToken.c_str();
    ResolveVertexDataArray(tangents_out,source,MappingInformationType,ReferenceInformationType,
        str,
        strIdx,
        vertices.size(),
        mapping_counts,
        mapping_offsets,
        mappings);
}

// ------------------------------------------------------------------------------------------------
static const std::string BinormalIndexToken = "BinormalIndex";
static const std::string BinormalsIndexToken = "BinormalsIndex";

void MeshGeometry::ReadVertexDataBinormals(std::vector<aiVector3D>& binormals_out, const Scope& source,
    const std::string& MappingInformationType,
    const std::string& ReferenceInformationType)
{
    const char * str = source.Elements().count( "Binormals" ) > 0 ? "Binormals" : "Binormal";
    const char * strIdx = source.Elements().count( "Binormals" ) > 0 ? BinormalsIndexToken.c_str() : BinormalIndexToken.c_str();
    ResolveVertexDataArray(binormals_out,source,MappingInformationType,ReferenceInformationType,
        str,
        strIdx,
        vertices.size(),
        mapping_counts,
        mapping_offsets,
        mappings);
}


// ------------------------------------------------------------------------------------------------
void MeshGeometry::ReadVertexDataMaterials(std::vector<int>& materials_out, const Scope& source,
    const std::string& MappingInformationType,
    const std::string& ReferenceInformationType)
{
    const size_t face_count = faces.size();
    ai_assert(face_count);

    // materials are handled separately. First of all, they are assigned per-face
    // and not per polyvert. Secondly, ReferenceInformationType=IndexToDirect
    // has a slightly different meaning for materials.
    ParseVectorDataArray(materials_out,GetRequiredElement(source,"Materials"));

    if (MappingInformationType == "AllSame") {
        // easy - same material for all faces
        if (materials_out.empty()) {
            FBXImporter::LogError(Formatter::format("expected material index, ignoring"));
            return;
        }
        else if (materials_out.size() > 1) {
            FBXImporter::LogWarn(Formatter::format("expected only a single material index, ignoring all except the first one"));
            materials_out.clear();
        }

        materials.assign(vertices.size(),materials_out[0]);
    }
    else if (MappingInformationType == "ByPolygon" && ReferenceInformationType == "IndexToDirect") {
        materials.resize(face_count);

        if(materials_out.size() != face_count) {
            FBXImporter::LogError(Formatter::format("length of input data unexpected for ByPolygon mapping: ")
                << materials_out.size() << ", expected " << face_count
            );
            return;
        }
    }
    else {
        FBXImporter::LogError(Formatter::format("ignoring material assignments, access type not implemented: ")
            << MappingInformationType << "," << ReferenceInformationType);
    }
}

} // !FBX
} // !Assimp

#endif

