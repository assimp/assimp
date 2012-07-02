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

/** @file  FBXMaterial.cpp
 *  @brief Assimp::FBX::Material implementation
 */
#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_FBX_IMPORTER

#include "FBXParser.h"
#include "FBXDocument.h"
#include "FBXImporter.h"
#include "FBXImportSettings.h"
#include "FBXDocumentUtil.h"
#include "FBXProperties.h"

namespace Assimp {
namespace FBX {

	using namespace Util;

// ------------------------------------------------------------------------------------------------
Material::Material(const Element& element, const Document& doc, const std::string& name)
: Object(element,name)
{
	const Scope& sc = GetRequiredScope(element);
	
	const Element* const ShadingModel = sc["ShadingModel"];
	const Element* const MultiLayer = sc["MultiLayer"];
	const Element* const Properties70 = sc["Properties70"];

	if(MultiLayer) {
		multilayer = !!ParseTokenAsInt(GetRequiredToken(*MultiLayer,0));
	}

	if(ShadingModel) {
		shading = ParseTokenAsString(GetRequiredToken(*ShadingModel,0));
	}
	else {
		DOMWarning("shading mode not specified, assuming phong",&element);
		shading = "phong";
	}

	std::string templateName;

	const char* const sh = shading.c_str();
	if(!strcmp(sh,"phong")) {
		templateName = "Material.FbxSurfacePhong";
	}
	else if(!strcmp(sh,"lambert")) {
		templateName = "Material.FbxSurfaceLambert";
	}
	else {
		DOMWarning("shading mode not recognized: " + shading,&element);
	}

	boost::shared_ptr<const PropertyTable> templateProps = boost::shared_ptr<const PropertyTable>(NULL);
	if(templateName.length()) {
		PropertyTemplateMap::const_iterator it = doc.Templates().find(templateName); 
		if(it != doc.Templates().end()) {
			templateProps = (*it).second;
		}
	}

	if(!Properties70) {
		DOMWarning("material property table (Properties70) not found",&element);
		if(templateProps) {
			props = templateProps;
		}
		else {
			props = boost::make_shared<const PropertyTable>();
		}
	}
	else {
		props = boost::make_shared<const PropertyTable>(*Properties70,templateProps);
	}
}


// ------------------------------------------------------------------------------------------------
Material::~Material()
{
}

} //!FBX
} //!Assimp

#endif
