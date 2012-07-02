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
#include "FBXImportSettings.h"
#include "FBXDocumentUtil.h"
#include "FBXProperties.h"

namespace Assimp {
namespace FBX {
namespace Util {

// ------------------------------------------------------------------------------------------------
// signal DOM construction error, this is always unrecoverable. Throws DeadlyImportError.
void DOMError(const std::string& message, const Token& token)
{
	throw DeadlyImportError(Util::AddTokenText("FBX-DOM",message,&token));
}

// ------------------------------------------------------------------------------------------------
void DOMError(const std::string& message, const Element* element /*= NULL*/)
{
	if(element) {
		DOMError(message,element->KeyToken());
	}
	throw DeadlyImportError("FBX-DOM " + message);
}


// ------------------------------------------------------------------------------------------------
// print warning, do return
void DOMWarning(const std::string& message, const Token& token)
{
	if(DefaultLogger::get()) {
		DefaultLogger::get()->warn(Util::AddTokenText("FBX-DOM",message,&token));
	}
}

// ------------------------------------------------------------------------------------------------
void DOMWarning(const std::string& message, const Element* element /*= NULL*/)
{
	if(element) {
		DOMWarning(message,element->KeyToken());
		return;
	}
	if(DefaultLogger::get()) {
		DefaultLogger::get()->warn("FBX-DOM: " + message);
	}
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
const Element& GetRequiredElement(const Scope& sc, const std::string& index, const Element* element /*= NULL*/) 
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
// read an array of color4 tuples
void ReadVectorDataArray(std::vector<aiColor4D>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	//  see notes in ReadVectorDataArray() above
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	if (a.Tokens().size() % 4 != 0) {
		DOMError("number of floats is not a multiple of four (4)",&el);
	}
	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		aiColor4D v;
		v.r = ParseTokenAsFloat(**it++);
		v.g = ParseTokenAsFloat(**it++);
		v.b = ParseTokenAsFloat(**it++);
		v.a = ParseTokenAsFloat(**it++);

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
void ReadVectorDataArray(std::vector<int>& out, const Element& el)
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


// ------------------------------------------------------------------------------------------------
// read an array of uints
void ReadVectorDataArray(std::vector<unsigned int>& out, const Element& el)
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
		if(ival < 0) {
			DOMError("encountered negative integer index");
		}
		out.push_back(static_cast<unsigned int>(ival));
	}
}
} // !Util

using namespace Util;

// ------------------------------------------------------------------------------------------------
LazyObject::LazyObject(const Element& element, const Document& doc)
: doc(doc)
, element(element)
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

	// this needs to be relatively fast since it happens a lot,
	// so avoid constructing strings all the time.
	const char* obtype = key.begin();
	const size_t length = static_cast<size_t>(key.end()-key.begin());
	if (!strncmp(obtype,"Geometry",length)) {
		if (!strcmp(classtag.c_str(),"Mesh")) {
			object.reset(new MeshGeometry(element,name,doc.Settings()));
		}
	}
	else if (!strncmp(obtype,"Material",length)) {
		object.reset(new Material(element,doc,name));
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
Document::Document(const Parser& parser, const ImportSettings& settings)
: parser(parser)
, settings(settings)
{
	ReadPropertyTemplates();
	ReadObjects();
}

// ------------------------------------------------------------------------------------------------
Document::~Document()
{
	BOOST_FOREACH(ObjectMap::value_type& v, objects) {
		delete v.second;
	}
}

// ------------------------------------------------------------------------------------------------
void Document::ReadObjects()
{
	// read ID objects from "Objects" section
	const Scope& sc = parser.GetRootScope();
	const Element* const eobjects = sc["Objects"];
	if(!eobjects || !eobjects->Compound()) {
		DOMError("no Objects dictionary found");
	}

	const Scope& sobjects = *eobjects->Compound();
	BOOST_FOREACH(const ElementMap::value_type& el, sobjects.Elements()) {
		
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

		objects[id] = new LazyObject(*el.second, *this);
		// DEBUG - evaluate all objects
		const Object* o = objects[id]->Get();
	}
}

// ------------------------------------------------------------------------------------------------
void Document::ReadPropertyTemplates()
{
	const Scope& sc = parser.GetRootScope();
	// read property templates from "Definitions" section
	const Element* const edefs = sc["Definitions"];
	if(!edefs || !edefs->Compound()) {
		DOMWarning("no Definitions dictionary found");
		return;
	}

	const Scope& sdefs = *edefs->Compound();
	const ElementCollection otypes = sdefs.GetCollection("ObjectType");
	for(ElementMap::const_iterator it = otypes.first; it != otypes.second; ++it) {
		const Element& el = *(*it).second;
		const Scope* sc = el.Compound();
		if(!sc) {
			DOMWarning("expected nested scope in ObjectType, ignoring",&el);
			continue;
		}

		const TokenList& tok = el.Tokens();
		if(tok.empty()) {
			DOMWarning("expected name for ObjectType element, ignoring",&el);
			continue;
		}

		const std::string& oname = ParseTokenAsString(*tok[0]);

		const ElementCollection templs = sc->GetCollection("PropertyTemplate");
		for(ElementMap::const_iterator it = templs.first; it != templs.second; ++it) {
			const Element& el = *(*it).second;
			const Scope* sc = el.Compound();
			if(!sc) {
				DOMWarning("expected nested scope in PropertyTemplate, ignoring",&el);
				continue;
			}

			const TokenList& tok = el.Tokens();
			if(tok.empty()) {
				DOMWarning("expected name for PropertyTemplate element, ignoring",&el);
				continue;
			}

			const std::string& pname = ParseTokenAsString(*tok[0]);

			const Element* Properties70 = (*sc)["Properties70"];
			if(Properties70) {
				boost::shared_ptr<const PropertyTable> props = boost::make_shared<const PropertyTable>(
					*Properties70,boost::shared_ptr<const PropertyTable>(NULL)
				);

				templates[oname+"."+pname] = props;
			}
		}
	}
}

} // !FBX
} // !Assimp

#endif

