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
 *  @brief Assimp::FBX::Material and Assimp::FBX::Texture implementation
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
Material::Material(uint64_t id, const Element& element, const Document& doc, const std::string& name)
: Object(id,element,name)
{
	const Scope& sc = GetRequiredScope(element);
	
	const Element* const ShadingModel = sc["ShadingModel"];
	const Element* const MultiLayer = sc["MultiLayer"];

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

	props = GetPropertyTable(doc,templateName,element,sc);

	// resolve texture links
	const std::vector<const Connection*>& conns = doc.GetConnectionsByDestinationSequenced(ID());
	BOOST_FOREACH(const Connection* con, conns) {

		// texture link to properties, not objects
		if (!con->PropertyName().length()) {
			continue;
		}

		const Object* const ob = con->SourceObject();
		if(!ob) {
			DOMWarning("failed to read source object for texture link, ignoring",&element);
			continue;
		}

		const Texture* const tex = dynamic_cast<const Texture*>(ob);
		if(!tex) {
			DOMWarning("source object for texture link is not a texture, ignoring",&element);
			continue;
		}

		const std::string& prop = con->PropertyName();
		if (textures.find(prop) != textures.end()) {
			DOMWarning("duplicate texture link: " + prop,&element);
		}

		textures[prop] = tex;
	}
}


// ------------------------------------------------------------------------------------------------
Material::~Material()
{
}


// ------------------------------------------------------------------------------------------------
Texture::Texture(uint64_t id, const Element& element, const Document& doc, const std::string& name)
: Object(id,element,name)
, uvScaling(1.0f,1.0f)
{
	const Scope& sc = GetRequiredScope(element);

	const Element* const Type = sc["Type"];
	const Element* const FileName = sc["FileName"];
	const Element* const RelativeFilename = sc["RelativeFilename"];
	const Element* const ModelUVTranslation = sc["ModelUVTranslation"];
	const Element* const ModelUVScaling = sc["ModelUVScaling"];
	const Element* const Texture_Alpha_Source = sc["Texture_Alpha_Source"];
	const Element* const Cropping = sc["Cropping"];

	if(Type) {
		type = ParseTokenAsString(GetRequiredToken(*Type,0));
	}

	if(FileName) {
		fileName = ParseTokenAsString(GetRequiredToken(*FileName,0));
	}

	if(RelativeFilename) {
		relativeFileName = ParseTokenAsString(GetRequiredToken(*RelativeFilename,0));
	}

	if(ModelUVTranslation) {
		uvTrans = aiVector2D(ParseTokenAsFloat(GetRequiredToken(*ModelUVTranslation,0)),
			ParseTokenAsFloat(GetRequiredToken(*ModelUVTranslation,1))
		);
	}

	if(ModelUVScaling) {
		uvScaling = aiVector2D(ParseTokenAsFloat(GetRequiredToken(*ModelUVScaling,0)),
			ParseTokenAsFloat(GetRequiredToken(*ModelUVScaling,1))
		);
	}

	if(Cropping) {
		crop[0] = ParseTokenAsInt(GetRequiredToken(*Cropping,0));
		crop[1] = ParseTokenAsInt(GetRequiredToken(*Cropping,1));
		crop[2] = ParseTokenAsInt(GetRequiredToken(*Cropping,2));
		crop[3] = ParseTokenAsInt(GetRequiredToken(*Cropping,3));
	}
	else {
		// vc8 doesn't support the crop() syntax in initialization lists
		// (and vc9 WARNS about the new (i.e. compliant) behaviour).
		crop[0] = crop[1] = crop[2] = crop[3] = 0;
	}

	if(Texture_Alpha_Source) {
		alphaSource = ParseTokenAsString(GetRequiredToken(*Texture_Alpha_Source,0));
	}

	props = GetPropertyTable(doc,"Texture.FbxFileTexture",element,sc);
}


Texture::~Texture()
{

}

} //!FBX
} //!Assimp

#endif
