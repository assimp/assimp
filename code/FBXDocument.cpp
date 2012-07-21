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

#include <functional>

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

// XXX: tacke code duplication in the various ReadVectorDataArray() overloads below.
// could use a type traits based solution.

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
// read an array of floats
void ReadVectorDataArray(std::vector<float>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// see notes in ReadVectorDataArray()
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		const float ival = ParseTokenAsFloat(**it++);
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


// ------------------------------------------------------------------------------------------------
// read an array of uint64_ts
void ReadVectorDataArray(std::vector<uint64_t>& out, const Element& el)
{
	out.clear();
	const TokenList& tok = el.Tokens();
	const size_t dim = ParseTokenAsDim(*tok[0]);

	// see notes in ReadVectorDataArray()
	out.reserve(dim);

	const Scope& scope = GetRequiredScope(el);
	const Element& a = GetRequiredElement(scope,"a",&el);

	for (TokenList::const_iterator it = a.Tokens().begin(), end = a.Tokens().end(); it != end; ) {
		const uint64_t ival = ParseTokenAsID(**it++);
		
		out.push_back(ival);
	}
}


// ------------------------------------------------------------------------------------------------
// fetch a property table and the corresponding property template 
boost::shared_ptr<const PropertyTable> GetPropertyTable(const Document& doc, 
	const std::string& templateName, 
	const Element &element, 
	const Scope& sc)
{
	const Element* const Properties70 = sc["Properties70"];
	boost::shared_ptr<const PropertyTable> templateProps = boost::shared_ptr<const PropertyTable>(
		static_cast<const PropertyTable*>(NULL));

	if(templateName.length()) {
		PropertyTemplateMap::const_iterator it = doc.Templates().find(templateName); 
		if(it != doc.Templates().end()) {
			templateProps = (*it).second;
		}
	}

	if(!Properties70) {
		DOMWarning("material property table (Properties70) not found",&element);
		if(templateProps) {
			return templateProps;
		}
		else {
			return boost::make_shared<const PropertyTable>();
		}
	}
	return boost::make_shared<const PropertyTable>(*Properties70,templateProps);
}
} // !Util

using namespace Util;

// ------------------------------------------------------------------------------------------------
LazyObject::LazyObject(uint64_t id, const Element& element, const Document& doc)
: doc(doc)
, element(element)
, id(id)
, flags()
{

}

// ------------------------------------------------------------------------------------------------
LazyObject::~LazyObject()
{

}

// ------------------------------------------------------------------------------------------------
const Object* LazyObject::Get(bool dieOnError)
{
	if(IsBeingConstructed() || FailedToConstruct()) {
		return NULL;
	}

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

	// prevent recursive calls
	flags |= BEING_CONSTRUCTED;

	try {
		// this needs to be relatively fast since it happens a lot,
		// so avoid constructing strings all the time.
		const char* obtype = key.begin();
		const size_t length = static_cast<size_t>(key.end()-key.begin());
		if (!strncmp(obtype,"Geometry",length)) {
			if (!strcmp(classtag.c_str(),"Mesh")) {
				object.reset(new MeshGeometry(id,element,name,doc));
			}
		}
		else if (!strncmp(obtype,"Model",length)) {
			object.reset(new Model(id,element,doc,name));
		}
		else if (!strncmp(obtype,"Material",length)) {
			object.reset(new Material(id,element,doc,name));
		}
		else if (!strncmp(obtype,"Texture",length)) {
			object.reset(new Texture(id,element,doc,name));
		}
		else if (!strncmp(obtype,"AnimationStack",length)) {
			object.reset(new AnimationStack(id,element,name,doc));
		}
		else if (!strncmp(obtype,"AnimationLayer",length)) {
			object.reset(new AnimationLayer(id,element,name,doc));
		}
		// note: order matters for these two
		else if (!strncmp(obtype,"AnimationCurve",length)) {
			object.reset(new AnimationCurve(id,element,name,doc));
		}
		else if (!strncmp(obtype,"AnimationCurveNode",length)) {
			object.reset(new AnimationCurveNode(id,element,name,doc));
		}	
	}
	catch(std::exception& ex) {
		flags &= ~BEING_CONSTRUCTED;
		flags |= FAILED_TO_CONSTRUCT;

		if(dieOnError || doc.Settings().strictMode) {
			throw;
		}

		// note: the error message is already formatted, so raw logging is ok
		if(!DefaultLogger::isNullLogger()) {
			DefaultLogger::get()->error(ex.what());
		}
		return NULL;
	}

	if (!object.get()) {
		//DOMError("failed to convert element to DOM object, class: " + classtag + ", name: " + name,&element);
	}

	flags &= ~BEING_CONSTRUCTED;
	return object.get();
}

// ------------------------------------------------------------------------------------------------
Object::Object(uint64_t id, const Element& element, const std::string& name)
: element(element)
, name(name)
, id(id)
{

}

// ------------------------------------------------------------------------------------------------
Object::~Object()
{

}


// ------------------------------------------------------------------------------------------------
Geometry::Geometry(uint64_t id, const Element& element, const std::string& name)
: Object(id, element,name)
{

}


// ------------------------------------------------------------------------------------------------
Geometry::~Geometry()
{

}


// ------------------------------------------------------------------------------------------------
Document::Document(const Parser& parser, const ImportSettings& settings)
: settings(settings)
, parser(parser)
{
	// cannot use array default initialization syntax because vc8 fails on it
	for (unsigned int i = 0; i < 7; ++i) {
		creationTimeStamp[i] = 0;
	}

	ReadHeader();
	ReadPropertyTemplates();

	// this order is important, connections need parsed objects to check
	// whether connections are ok or not. Objects may not be evaluated yet,
	// though, since this may require valid connections.
	ReadObjects();
	ReadConnections();
}


// ------------------------------------------------------------------------------------------------
Document::~Document()
{
	BOOST_FOREACH(ObjectMap::value_type& v, objects) {
		delete v.second;
	}
}


// ------------------------------------------------------------------------------------------------
void Document::ReadHeader()
{
	// read ID objects from "Objects" section
	const Scope& sc = parser.GetRootScope();
	const Element* const ehead = sc["FBXHeaderExtension"];
	if(!ehead || !ehead->Compound()) {
		DOMError("no FBXHeaderExtension dictionary found");
	}

	const Scope& shead = *ehead->Compound();
	fbxVersion = ParseTokenAsInt(GetRequiredToken(GetRequiredElement(shead,"FBXVersion",ehead),0));

	if(Settings().strictMode) {
		if(fbxVersion < 7200 || fbxVersion > 7300) {
			DOMError("unsupported format version, supported are only FBX 2012 and FBX 2013"\
				" in ASCII format (turn off strict mode to try anyhow) ");
		}
	}

	const Element* const ecreator = shead["Creator"];
	if(ecreator) {
		creator = ParseTokenAsString(GetRequiredToken(*ecreator,0));
	}

	const Element* const etimestamp = shead["CreationTimeStamp"];
	if(etimestamp && etimestamp->Compound()) {
		const Scope& stimestamp = *etimestamp->Compound();
		creationTimeStamp[0] = ParseTokenAsInt(GetRequiredToken(GetRequiredElement(stimestamp,"Year"),0));
		creationTimeStamp[1] = ParseTokenAsInt(GetRequiredToken(GetRequiredElement(stimestamp,"Month"),0));
		creationTimeStamp[2] = ParseTokenAsInt(GetRequiredToken(GetRequiredElement(stimestamp,"Day"),0));
		creationTimeStamp[3] = ParseTokenAsInt(GetRequiredToken(GetRequiredElement(stimestamp,"Hour"),0));
		creationTimeStamp[4] = ParseTokenAsInt(GetRequiredToken(GetRequiredElement(stimestamp,"Minute"),0));
		creationTimeStamp[5] = ParseTokenAsInt(GetRequiredToken(GetRequiredElement(stimestamp,"Second"),0));
		creationTimeStamp[6] = ParseTokenAsInt(GetRequiredToken(GetRequiredElement(stimestamp,"Millisecond"),0));
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

		objects[id] = new LazyObject(id, *el.second, *this);

		// grab all animation stacks upfront since there is no listing of them
		if(el.first == "AnimationStack") {
			animationStacks.push_back(id);
		}
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
					*Properties70,boost::shared_ptr<const PropertyTable>(static_cast<const PropertyTable*>(NULL))
				);

				templates[oname+"."+pname] = props;
			}
		}
	}
}



// ------------------------------------------------------------------------------------------------
void Document::ReadConnections()
{
	const Scope& sc = parser.GetRootScope();
	// read property templates from "Definitions" section
	const Element* const econns = sc["Connections"];
	if(!econns || !econns->Compound()) {
		DOMError("no Connections dictionary found");
	}

	uint64_t insertionOrder = 0l;

	const Scope& sconns = *econns->Compound();
	const ElementCollection conns = sconns.GetCollection("C");
	for(ElementMap::const_iterator it = conns.first; it != conns.second; ++it) {
		const Element& el = *(*it).second;
		const std::string& type = ParseTokenAsString(GetRequiredToken(el,0));
		const uint64_t src = ParseTokenAsID(GetRequiredToken(el,1));
		const uint64_t dest = ParseTokenAsID(GetRequiredToken(el,2));

		// OO = object-object connection
		// OP = object-property connection, in which case the destination property follows the object ID
		const std::string& prop = (type == "OP" ? ParseTokenAsString(GetRequiredToken(el,3)) : "");

		if(objects.find(src) == objects.end()) {
			DOMWarning("source object for connection does not exist",&el);
			continue;
		}

		// dest may be 0 (root node)
		if(dest && objects.find(dest) == objects.end()) {
			DOMWarning("destination object for connection does not exist",&el);
			continue;
		}

		// add new connection
		const Connection* const c = new Connection(insertionOrder++,src,dest,prop,*this);
		src_connections.insert(ConnectionMap::value_type(src,c));  
		dest_connections.insert(ConnectionMap::value_type(dest,c));  
	}
}


// ------------------------------------------------------------------------------------------------
const std::vector<const AnimationStack*>& Document::AnimationStacks() const
{
	if (!animationStacksResolved.empty() || !animationStacks.size()) {
		return animationStacksResolved;
	}

	animationStacksResolved.reserve(animationStacks.size());
	BOOST_FOREACH(uint64_t id, animationStacks) {
		LazyObject* const lazy = GetObject(id);
		const AnimationStack* stack;
		if(!lazy || !(stack = lazy->Get<AnimationStack>())) {
			DOMWarning("failed to read AnimationStack object");
			continue;
		}
		animationStacksResolved.push_back(stack);
	}

	return animationStacksResolved;
}


// ------------------------------------------------------------------------------------------------
LazyObject* Document::GetObject(uint64_t id) const
{
	ObjectMap::const_iterator it = objects.find(id);
	return it == objects.end() ? NULL : (*it).second;
}

#define MAX_CLASSNAMES 6

// ------------------------------------------------------------------------------------------------
std::vector<const Connection*> Document::GetConnectionsSequenced(uint64_t id, const ConnectionMap& conns) const
{
	std::vector<const Connection*> temp;

	const std::pair<ConnectionMap::const_iterator,ConnectionMap::const_iterator> range = 
		conns.equal_range(id);

	temp.reserve(std::distance(range.first,range.second));
	for (ConnectionMap::const_iterator it = range.first; it != range.second; ++it) {
		temp.push_back((*it).second);
	}

	std::sort(temp.begin(), temp.end(), std::mem_fun(&Connection::Compare));

	return temp; // NRVO should handle this
}


// ------------------------------------------------------------------------------------------------
std::vector<const Connection*> Document::GetConnectionsSequenced(uint64_t id, bool is_src, const ConnectionMap& conns, const char* const* classnames, size_t count) const
{
	ai_assert(classnames);
	ai_assert(count != 0 && count <= MAX_CLASSNAMES);

	size_t lenghts[MAX_CLASSNAMES];

	const size_t c = count;
	for (size_t i = 0; i < c; ++i) {
		lenghts[i] = strlen(classnames[i]);
	}

	std::vector<const Connection*> temp;

	const std::pair<ConnectionMap::const_iterator,ConnectionMap::const_iterator> range = 
		conns.equal_range(id);

	temp.reserve(std::distance(range.first,range.second));
	for (ConnectionMap::const_iterator it = range.first; it != range.second; ++it) {
		const Token& key = (is_src 
			? (*it).second->LazyDestinationObject()
			: (*it).second->LazySourceObject()
		).GetElement().KeyToken();

		const char* obtype = key.begin();

		for (size_t i = 0; i < c; ++i) {
			ai_assert(classnames[i]);
			if(std::distance(key.begin(),key.end()) == lenghts[i] && !strncmp(classnames[i],obtype,lenghts[i])) {
				obtype = NULL;
				break;
			}
		}

		if(obtype) {
			continue;
		}

		temp.push_back((*it).second);
	}

	std::sort(temp.begin(), temp.end(), std::mem_fun(&Connection::Compare));
	return temp; // NRVO should handle this
}


// ------------------------------------------------------------------------------------------------
std::vector<const Connection*> Document::GetConnectionsBySourceSequenced(uint64_t source) const
{
	return GetConnectionsSequenced(source, ConnectionsBySource());
}



// ------------------------------------------------------------------------------------------------
std::vector<const Connection*> Document::GetConnectionsBySourceSequenced(uint64_t dest, const char* classname) const
{
	const char* arr[] = {classname};
	return GetConnectionsBySourceSequenced(dest, arr,1);
}



// ------------------------------------------------------------------------------------------------
std::vector<const Connection*> Document::GetConnectionsBySourceSequenced(uint64_t source, const char* const* classnames, size_t count) const
{
	return GetConnectionsSequenced(source, true, ConnectionsBySource(),classnames, count);
}


// ------------------------------------------------------------------------------------------------
std::vector<const Connection*> Document::GetConnectionsByDestinationSequenced(uint64_t dest, const char* classname) const
{
	const char* arr[] = {classname};
	return GetConnectionsByDestinationSequenced(dest, arr,1);
}


// ------------------------------------------------------------------------------------------------
std::vector<const Connection*> Document::GetConnectionsByDestinationSequenced(uint64_t dest) const
{
	return GetConnectionsSequenced(dest, ConnectionsByDestination());
}


// ------------------------------------------------------------------------------------------------
std::vector<const Connection*> Document::GetConnectionsByDestinationSequenced(uint64_t dest, const char* const* classnames, size_t count) const
{
	return GetConnectionsSequenced(dest, false, ConnectionsByDestination(),classnames, count);
}


// ------------------------------------------------------------------------------------------------
Connection::Connection(uint64_t insertionOrder,  uint64_t src, uint64_t dest, const std::string& prop, const Document& doc)
: insertionOrder(insertionOrder)
, prop(prop)
, src(src)
, dest(dest)
, doc(doc)
{
	ai_assert(doc.Objects().find(src) != doc.Objects().end());
	// dest may be 0 (root node)
	ai_assert(!dest || doc.Objects().find(dest) != doc.Objects().end());
}


// ------------------------------------------------------------------------------------------------
Connection::~Connection()
{

}


// ------------------------------------------------------------------------------------------------
LazyObject& Connection::LazySourceObject() const
{
	LazyObject* const lazy = doc.GetObject(src);
	ai_assert(lazy);
	return *lazy;
}


// ------------------------------------------------------------------------------------------------
LazyObject& Connection::LazyDestinationObject() const
{
	LazyObject* const lazy = doc.GetObject(dest);
	ai_assert(lazy);
	return *lazy;
}


// ------------------------------------------------------------------------------------------------
const Object* Connection::SourceObject() const
{
	LazyObject* const lazy = doc.GetObject(src);
	ai_assert(lazy);
	return lazy->Get();
}


// ------------------------------------------------------------------------------------------------
const Object* Connection::DestinationObject() const
{
	LazyObject* const lazy = doc.GetObject(dest);
	ai_assert(lazy);
	return lazy->Get();
}

} // !FBX
} // !Assimp

#endif

