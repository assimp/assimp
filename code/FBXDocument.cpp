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

namespace {

	using namespace Assimp;
	using namespace Assimp::FBX;

	// ------------------------------------------------------------------------------------------------
	// signal DOM construction error, this is always unrecoverable. Throws DeadlyImportError.
	void DOMError(const std::string& message, Element* element = NULL)
	{
		throw DeadlyImportError(element ? Util::AddTokenText("FBX-DOM",message,element->KeyToken()) : ("FBX-DOM " + message));
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

	// XXX
	return NULL;
}

// ------------------------------------------------------------------------------------------------
Object::Object(const Element& element)
: element(element)
{

}

// ------------------------------------------------------------------------------------------------
Object::~Object()
{

}

// ------------------------------------------------------------------------------------------------
Geometry::Geometry(const Element& element)
: Object(element)
{

}

// ------------------------------------------------------------------------------------------------
Geometry::~Geometry()
{

}

// ------------------------------------------------------------------------------------------------
MeshGeometry::MeshGeometry(const Element& element)
: Geometry(element)
{

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

