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

namespace {

	using namespace Assimp;
	using namespace Assimp::FBX;

	// ------------------------------------------------------------------------------------------------
	// signal DOM construction error, this is always unrecoverable. Throws DeadlyImportError.
	void DOMError(const std::string& message, const Element* element = NULL)
	{
		throw DeadlyImportError(element ? Util::AddTokenText("FBX-DOM",message,&element->KeyToken()) : ("FBX-DOM " + message));
	}


	// ------------------------------------------------------------------------------------------------
	// extract a required element from a scope, abort if the element cannot be found
	const Element& GetFixedElementFromScope(const Scope& sc, const std::string& index, const Element* element = NULL) 
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

		const char* err;
		const size_t dim = ParseTokenAsDim(*tok[0],err);
		if(err) {
			DOMError(err,&el);
		}

		// may throw bad_alloc if the input is rubbish, but this need
		// not to be prevented - importing would fail but we wouldn't
		// crash since assimp handles this case properly.
		out.reserve(dim);

		const Scope* const scope = el.Compound();
		if(!scope) {
			DOMError("expected vector3 data",&el);
		}

		const Element& a = GetFixedElementFromScope(*scope,"a",&el);
		if (a.Tokens().size() % 3 != 0) {
			DOMError("number of floats is not a multiple of three",&el);
		}
		for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
			aiVector3D v;
			v.x = ParseTokenAsFloat(**it++,err);
			if(err) {
				DOMError(err,&el);
			}

			v.y = ParseTokenAsFloat(**it++,err);
			if(err) {
				DOMError(err,&el);
			}

			v.z = ParseTokenAsFloat(**it++,err);
			if(err) {
				DOMError(err,&el);
			}

			out.push_back(v);
		}
	}


	// ------------------------------------------------------------------------------------------------
	// read an array of ints
	void ReadIntDataArray(std::vector<int>& out, const Element& el)
	{
		out.clear();
		const TokenList& tok = el.Tokens();

		const char* err;
		const size_t dim = ParseTokenAsDim(*tok[0],err);
		if(err) {
			DOMError(err,&el);
		}

		// see notes in ReadVectorDataArray()
		out.reserve(dim);

		const Scope* const scope = el.Compound();
		if(!scope) {
			DOMError("expected int data block",&el);
		}

		const Element& a = GetFixedElementFromScope(*scope,"a",&el);
		for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
			const int ival = ParseTokenAsInt(**it++,err);
			if(err) {
				DOMError(err,&el);
			}

			out.push_back(ival);
		}
	}

}

namespace Assimp {
namespace FBX {

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
	// so avoid constructing strings all the time. strcmp()
	// may scan beyond the bounds of the token, but the 
	// next character is always a colon so false positives
	// are not possible.
	const char* obtype = key.begin();
	if (!strcmp(obtype,"Geometry")) {

		if (!strcmp(classtag.c_str(),"Mesh")) {
			object = new MeshGeometry(element,name);
		}
	}

	if (!object.get()) {
		DOMError("failed to convert element to DOM object, class: " + classtag + ", name: " + name,&element);
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
	const Element& Vertices = GetFixedElementFromScope(*sc,"Vertices",&element);
	const Element& PolygonVertexIndex = GetFixedElementFromScope(*sc,"PolygonVertexIndex",&element);

	// optional Mesh elements:
	const ElementCollection& Layer = sc->GetCollection("Layer");
	const ElementCollection& LayerElementMaterial = sc->GetCollection("LayerElementMaterial");
	const ElementCollection& LayerElementUV = sc->GetCollection("LayerElementUV");
	const ElementCollection& LayerElementNormal = sc->GetCollection("LayerElementNormal");

	ReadVectorDataArray(vertices,Vertices);

	std::vector<int> tempFaces;
	ReadIntDataArray(tempFaces,PolygonVertexIndex);

	// ignore all but the first layer, but warn about any further layers 
	for (ElementMap::const_iterator it = Layer.first; it != Layer.second; ++it) {
		const TokenList& tokens = (*it).second->Tokens();

		const char* err;
		const int index = ParseTokenAsInt(*tokens[0], err);
		if(err) {
			DOMError(err,&element);
		}

		if(index == 0) {
			const Scope* const layer = (*it).second->Compound();
			if (layer) {
				DOMError("expected layer scope",&element);
			}

			// XXX read layer data
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
	}
}

// ------------------------------------------------------------------------------------------------
Document::~Document()
{
	
}

} // !FBX
} // !Assimp

#endif

