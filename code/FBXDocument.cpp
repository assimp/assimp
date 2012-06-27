/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
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

/** @file  FBXDocument.cpp
 *  @brief Implementation of the FBX DOM classes
 */
#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_FBX_IMPORTER

#include "FBXParser.h"
#include "FBXDocument.h"
#include "FBXUtil.h"
#include "FBXImporter.h"

namespace Assimp {
namespace FBX {

namespace {

// ------------------------------------------------------------------------------------------------
// signal DOM construction error, this is always unrecoverable. Throws DeadlyImportError.
void DOMError(const std::string& message, const Token& token)
{
	throw DeadlyImportError(Util::AddTokenText("FBX-DOM",message,&token));
}

// ------------------------------------------------------------------------------------------------
void DOMError(const std::string& message, const Element* element = NULL)
{
	if(element) {
		DOMError(message,element->KeyToken());
	}
	throw DeadlyImportError("FBX-DOM " + message);
}


// ------------------------------------------------------------------------------------------------
// extract required compound scope
const Scope& GetRequiredScope(const Element& el)
{
	const Scope* const s = el.Compound();
	if(!s) {
		DOMError("expected compound scope",&el);
	}

	return *s;
}


// ------------------------------------------------------------------------------------------------
// get token at a particular index
const Token& GetRequiredToken(const Element& el, unsigned int index)
{
	const TokenList& t = el.Tokens();
	if(index >= t.size()) {
		DOMError(Formatter::format( "missing token at index " ) << index,&el);
	}

	return *t[index];
}


// ------------------------------------------------------------------------------------------------
// wrapper around ParseTokenAsID() with DOMError handling
uint64_t ParseTokenAsID(const Token& t) 
{
	const char* err;
	const uint64_t i = ParseTokenAsID(t,err);
	if(err) {
		DOMError(err,t);
	}
	return i;
}


// ------------------------------------------------------------------------------------------------
// wrapper around ParseTokenAsDim() with DOMError handling
size_t ParseTokenAsDim(const Token& t)
{
	const char* err;
	const size_t i = ParseTokenAsDim(t,err);
	if(err) {
		DOMError(err,t);
	}
	return i;
}


// ------------------------------------------------------------------------------------------------
// wrapper around ParseTokenAsFloat() with DOMError handling
float ParseTokenAsFloat(const Token& t)
{
	const char* err;
	const float i = ParseTokenAsFloat(t,err);
	if(err) {
		DOMError(err,t);
	}
	return i;
}


// ------------------------------------------------------------------------------------------------
// wrapper around ParseTokenAsInt() with DOMError handling
int ParseTokenAsInt(const Token& t)
{
	const char* err;
	const int i = ParseTokenAsInt(t,err);
	if(err) {
		DOMError(err,t);
	}
	return i;
}


// ------------------------------------------------------------------------------------------------
// wrapper around ParseTokenAsString() with DOMError handling
std::string ParseTokenAsString(const Token& t)
{
	const char* err;
	const std::string& i = ParseTokenAsString(t,err);
	if(err) {
		DOMError(err,t);
	}
	return i;
}

// ------------------------------------------------------------------------------------------------
// extract a required element from a scope, abort if the element cannot be found
const Element& GetRequiredElement(const Scope& sc, const std::string& index, const Element* element = NULL) 
{
	const Element* el = sc[index];
	if(!el) {
		DOMError("did not find required element \"" + index + "\"",element);
	}
	return *el;
}


// ------------------------------------------------------------------------------------------------
// read an array of float3 tuples
void ReadVectorDataArray(std::vector<aiVector3D>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// may throw bad_alloc if the input is rubbish, but this need
	// not to be prevented - importing would fail but we wouldn't
	// crash since assimp handles this case properly.
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	if (a.Tokens().size() % 3 != 0) {
		DOMError("number of floats is not a multiple of three (3)",&el);
	}
	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		aiVector3D v;
		v.x = ParseTokenAsFloat(**it++);
		v.y = ParseTokenAsFloat(**it++);
		v.z = ParseTokenAsFloat(**it++);

		out.push_back(v);
	}
}


// ------------------------------------------------------------------------------------------------
// read an array of float2 tuples
void ReadVectorDataArray(std::vector<aiVector2D>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// see notes in ReadVectorDataArray() above
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	if (a.Tokens().size() % 2 != 0) {
		DOMError("number of floats is not a multiple of two (2)",&el);
	}
	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		aiVector2D v;
		v.x = ParseTokenAsFloat(**it++);
		v.y = ParseTokenAsFloat(**it++);

		out.push_back(v);
	}
}


// ------------------------------------------------------------------------------------------------
// read an array of ints
void ReadIntDataArray(std::vector<int>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// see notes in ReadVectorDataArray()
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		const int ival = ParseTokenAsInt(**it++);
		out.push_back(ival);
	}
}
} // end anon.


// ------------------------------------------------------------------------------------------------
LazyObject::LazyObject(const Element& element)
: element(element)
{

}

// ------------------------------------------------------------------------------------------------
LazyObject::~LazyObject()
{

}

// ------------------------------------------------------------------------------------------------
const Object* LazyObject::Get()
{
	if (object.get()) {
		return object.get();
	}

	const Token& key = element.KeyToken();
	const TokenList& tokens = element.Tokens();

	if(tokens.size() < 3) {
		DOMError("expected at least 3 tokens: id, name and class tag",&element);
	}

	const char* err;
	const std::string name = ParseTokenAsString(*tokens[1],err);
	if (err) {
		DOMError(err,&element);
	} 

	const std::string classtag = ParseTokenAsString(*tokens[2],err);
	if (err) {
		DOMError(err,&element);
	} 

	// this needs to be relatively fast since we do it a lot,
	// so avoid constructing strings all the time.
	const char* obtype = key.begin();
	if (!strncmp(obtype,"Geometry",static_cast<size_t>(key.end()-key.begin()))) {

		if (!strcmp(classtag.c_str(),"Mesh")) {
			object = new MeshGeometry(element,name);
		}
	}

	if (!object.get()) {
		//DOMError("failed to convert element to DOM object, class: " + classtag + ", name: " + name,&element);
	}

	return object.get();
}

// ------------------------------------------------------------------------------------------------
Object::Object(const Element& element, const std::string& name)
: element(element)
, name(name)
{

}

// ------------------------------------------------------------------------------------------------
Object::~Object()
{

}

// ------------------------------------------------------------------------------------------------
Geometry::Geometry(const Element& element, const std::string& name)
: Object(element,name)
{

}

// ------------------------------------------------------------------------------------------------
Geometry::~Geometry()
{

}

// ------------------------------------------------------------------------------------------------
MeshGeometry::MeshGeometry(const Element& element, const std::string& name)
: Geometry(element,name)
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
	const ElementCollection& LayerElementMaterial = sc->GetCollection("LayerElementMaterial");
	const ElementCollection& LayerElementUV = sc->GetCollection("LayerElementUV");
	const ElementCollection& LayerElementNormal = sc->GetCollection("LayerElementNormal");

	std::vector<aiVector3D> tempVerts;
	ReadVectorDataArray(tempVerts,Vertices);

	std::vector<int> tempFaces;
	ReadIntDataArray(tempFaces,PolygonVertexIndex);

	vertices.reserve(tempFaces.size());
	faces.reserve(tempFaces.size() / 3);

	mapping_offsets.resize(tempVerts.size());
	mapping_counts.resize(tempVerts.size(),0);
	mappings.resize(tempFaces.size());

	const size_t vertex_count = tempVerts.size();

	// generate output vertices, computing an adjacency table to
	// preserve the mapping from fbx indices to *this* indexing.
	unsigned int count = 0;
	BOOST_FOREACH(int index, tempFaces) {
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
	BOOST_FOREACH(int index, tempFaces) {
		const int absi = index < 0 ? (-index - 1) : index;
		mappings[mapping_offsets[absi] + mapping_counts[absi]++] = cursor;
	}

	// ignore all but the first layer, but warn about any further layers 
	for (ElementMap::const_iterator it = Layer.first; it != Layer.second; ++it) {
		const TokenList& tokens = (*it).second->Tokens();

		const char* err;
		const int index = ParseTokenAsInt(*tokens[0], err);
		if(err) {
			DOMError(err,&element);
		}

		if(index == 0) {
			const Scope& layer = GetRequiredScope(*(*it).second);
			ReadLayer(layer);
		}
		else {
			FBXImporter::LogWarn("ignoring additional geometry layers");
		}
	}
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
			FBXImporter::LogError(Formatter::format("ignoring UV layer, maximum UV number exceeded: ") 
				<< index << " (limit is " << AI_MAX_NUMBER_OF_TEXTURECOORDS << ")" );

			return;
		}

		std::vector<aiVector2D>& uv_out = uvs[index];

		std::vector<aiVector2D> tempUV;
		ReadVectorDataArray(tempUV,GetRequiredElement(source,"UV"));

		// handle permutations of Mapping and Reference type - it would be nice to
		// deal with this more elegantly and with less redundancy, but right
		// now it seems unavoidable.
		if (MappingInformationType == "ByVertice" && ReferenceInformationType == "Direct") {	
			uv_out.resize(vertices.size());
			for (size_t i = 0, e = tempUV.size(); i < e; ++i) {

				const unsigned int istart = mapping_offsets[i], iend = istart + mapping_counts[i];
				for (unsigned int j = istart; j < iend; ++j) {
					uv_out[mappings[j]] = tempUV[i];
				}
			}
		}
		else if (MappingInformationType == "ByVertice" && ReferenceInformationType == "IndexToDirect") {	
			uv_out.resize(vertices.size());

			std::vector<int> uvIndices;
			ReadIntDataArray(uvIndices,GetRequiredElement(source,"UVIndex"));

			for (size_t i = 0, e = uvIndices.size(); i < e; ++i) {

				const unsigned int istart = mapping_offsets[i], iend = istart + mapping_counts[i];
				for (unsigned int j = istart; j < iend; ++j) {
					if(static_cast<size_t>(uvIndices[i]) >= tempUV.size()) {
						DOMError("UV index out of range",&GetRequiredElement(source,"UVIndex"));
					}
					uv_out[mappings[j]] = tempUV[uvIndices[i]];
				}
			}
		}
		else if (MappingInformationType == "ByPolygonVertex" && ReferenceInformationType == "Direct") {	
			if (tempUV.size() != vertices.size()) {
				FBXImporter::LogError("size of input UV array unexpected for ByPolygonVertex mapping");
				return;
			}

			uv_out.swap(tempUV);
		}
		else if (MappingInformationType == "ByPolygonVertex" && ReferenceInformationType == "IndexToDirect") {	
			uv_out.resize(vertices.size());

			std::vector<int> uvIndices;
			ReadIntDataArray(uvIndices,GetRequiredElement(source,"UVIndex"));
			
			if (uvIndices.size() != vertices.size()) {
				FBXImporter::LogError("size of input UV array unexpected for ByPolygonVertex mapping");
				return;
			}

			unsigned int next = 0;
			BOOST_FOREACH(int i, uvIndices) {
				if(static_cast<size_t>(i) >= tempUV.size()) {
					DOMError("UV index out of range",&GetRequiredElement(source,"UVIndex"));
				}

				uv_out[next++] = tempUV[i];
			}
		}
		else {
			FBXImporter::LogError(Formatter::format("ignoring normals, unrecognized access type: ") 
				<< MappingInformationType << "," << ReferenceInformationType);
		}
	}
	else if (type == "LayerElementMaterial") {
		
		materials.resize(vertices.size());

		std::vector<int> tempMaterials;
		ReadIntDataArray(tempMaterials,GetRequiredElement(source,"Materials"));

		if (MappingInformationType == "AllSame") {
			// easy - same material for all faces
			materials.assign(vertices.size(),tempMaterials[0]);
		}
		else {
			FBXImporter::LogError(Formatter::format("ignoring material assignments, unrecognized access type: ") 
				<< MappingInformationType << "," << ReferenceInformationType);
		}
	}
	else if (type == "LayerElementNormal") {

		std::vector<aiVector3D> tempNormals;
		ReadVectorDataArray(normals,GetRequiredElement(source,"Normals"));

		normals.resize(vertices.size());

		if (MappingInformationType == "ByVertice" && ReferenceInformationType == "Direct") {	
			
			for (size_t i = 0, e = tempNormals.size(); i < e; ++i) {

				const unsigned int istart = mapping_offsets[i], iend = istart + mapping_counts[i];
				for (unsigned int j = istart; j < iend; ++j) {
					normals[mappings[j]] = tempNormals[i];
				}
			}
		}
		else if (MappingInformationType == "ByVertice" && ReferenceInformationType == "IndexToDirect") {	

			std::vector<int> normalIndices;
			ReadIntDataArray(normalIndices,GetRequiredElement(source,"NormalsIndex"));
			for (size_t i = 0, e = normalIndices.size(); i < e; ++i) {

				const unsigned int istart = mapping_offsets[i], iend = istart + mapping_counts[i];
				for (unsigned int j = istart; j < iend; ++j) {
					normals[mappings[j]] = tempNormals[normalIndices[i]];
				}
			}
		}
		else {
			FBXImporter::LogError(Formatter::format("ignoring normals, unrecognized access type: ") 
				<< MappingInformationType << "," << ReferenceInformationType);
		}
	}
}

// ------------------------------------------------------------------------------------------------
MeshGeometry::~MeshGeometry()
{

}

// ------------------------------------------------------------------------------------------------
Document::Document(const Parser& parser)
: parser(parser)
{

	const Scope& sc = parser.GetRootScope();
	const Element* const eobjects = sc["Objects"];
	if(!eobjects || !eobjects->Compound()) {
		DOMError("no Objects dictionary found");
	}

	const Scope* const sobjects = eobjects->Compound();
	BOOST_FOREACH(const ElementMap::value_type& el, sobjects->Elements()) {
		
		// extract ID 
		const TokenList& tok = el.second->Tokens();
		
		if (tok.empty()) {
			DOMError("expected ID after object key",el.second);
		}

		const char* err;

		const uint64_t id = ParseTokenAsID(*tok[0], err);
		if(err) {
			DOMError(err,el.second);
		}

		objects[id] = new LazyObject(*el.second);
		// DEBUG - evaluate all objects
		const Object* o = objects[id]->Get();
	}
}

// ------------------------------------------------------------------------------------------------
Document::~Document()
{
	
}

} // !FBX
} // !Assimp

#endif

