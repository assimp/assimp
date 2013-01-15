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

/** @file  IFCMaterial.cpp
 *  @brief Implementation of conversion routines to convert IFC materials to aiMaterial
 */

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_IFC_IMPORTER
#include "IFCUtil.h"

namespace Assimp {
	namespace IFC {

// ------------------------------------------------------------------------------------------------
int ConvertShadingMode(const std::string& name)
{
	if (name == "BLINN") {
		return aiShadingMode_Blinn;
	}
	else if (name == "FLAT" || name == "NOTDEFINED") {
		return aiShadingMode_NoShading;
	}
	else if (name == "PHONG") {
		return aiShadingMode_Phong;
	}
	IFCImporter::LogWarn("shading mode "+name+" not recognized by Assimp, using Phong instead");
	return aiShadingMode_Phong;
}

// ------------------------------------------------------------------------------------------------
void FillMaterial(aiMaterial* mat,const IFC::IfcSurfaceStyle* surf,ConversionData& conv) 
{
	aiString name;
	name.Set((surf->Name? surf->Name.Get() : "IfcSurfaceStyle_Unnamed"));
	mat->AddProperty(&name,AI_MATKEY_NAME);

	// now see which kinds of surface information are present
	BOOST_FOREACH(boost::shared_ptr< const IFC::IfcSurfaceStyleElementSelect > sel2, surf->Styles) {
		if (const IFC::IfcSurfaceStyleShading* shade = sel2->ResolveSelectPtr<IFC::IfcSurfaceStyleShading>(conv.db)) {
			aiColor4D col_base,col;

			ConvertColor(col_base, shade->SurfaceColour);
			mat->AddProperty(&col_base,1, AI_MATKEY_COLOR_DIFFUSE);

			if (const IFC::IfcSurfaceStyleRendering* ren = shade->ToPtr<IFC::IfcSurfaceStyleRendering>()) {

				if (ren->Transparency) {
					const float t = 1.f-static_cast<float>(ren->Transparency.Get());
					mat->AddProperty(&t,1, AI_MATKEY_OPACITY);
				}

				if (ren->DiffuseColour) {
					ConvertColor(col, *ren->DiffuseColour.Get(),conv,&col_base);
					mat->AddProperty(&col,1, AI_MATKEY_COLOR_DIFFUSE);
				}

				if (ren->SpecularColour) {
					ConvertColor(col, *ren->SpecularColour.Get(),conv,&col_base);
					mat->AddProperty(&col,1, AI_MATKEY_COLOR_SPECULAR);
				}

				if (ren->TransmissionColour) {
					ConvertColor(col, *ren->TransmissionColour.Get(),conv,&col_base);
					mat->AddProperty(&col,1, AI_MATKEY_COLOR_TRANSPARENT);
				}

				if (ren->ReflectionColour) {
					ConvertColor(col, *ren->ReflectionColour.Get(),conv,&col_base);
					mat->AddProperty(&col,1, AI_MATKEY_COLOR_REFLECTIVE);
				}

				const int shading = (ren->SpecularHighlight && ren->SpecularColour)?ConvertShadingMode(ren->ReflectanceMethod):static_cast<int>(aiShadingMode_Gouraud);
				mat->AddProperty(&shading,1, AI_MATKEY_SHADING_MODEL);

				if (ren->SpecularHighlight) {
					if(const EXPRESS::REAL* rt = ren->SpecularHighlight.Get()->ToPtr<EXPRESS::REAL>()) {
						// at this point we don't distinguish between the two distinct ways of
						// specifying highlight intensities. leave this to the user.
						const float e = static_cast<float>(*rt);
						mat->AddProperty(&e,1,AI_MATKEY_SHININESS);
					}
					else {
						IFCImporter::LogWarn("unexpected type error, SpecularHighlight should be a REAL");
					}
				}
			}
		} /*
		else if (const IFC::IfcSurfaceStyleWithTextures* tex = sel2->ResolveSelectPtr<IFC::IfcSurfaceStyleWithTextures>(conv.db)) {
			// XXX
		} */
	}

}

// ------------------------------------------------------------------------------------------------
unsigned int ProcessMaterials(const IFC::IfcRepresentationItem& item, ConversionData& conv)
{
	if (conv.materials.empty()) {
		aiString name;
		std::auto_ptr<aiMaterial> mat(new aiMaterial());

		name.Set("<IFCDefault>");
		mat->AddProperty(&name,AI_MATKEY_NAME);

		const aiColor4D col = aiColor4D(0.6f,0.6f,0.6f,1.0f);
		mat->AddProperty(&col,1, AI_MATKEY_COLOR_DIFFUSE);

		conv.materials.push_back(mat.release());
	}

	STEP::DB::RefMapRange range = conv.db.GetRefs().equal_range(item.GetID());
	for(;range.first != range.second; ++range.first) {
		if(const IFC::IfcStyledItem* const styled = conv.db.GetObject((*range.first).second)->ToPtr<IFC::IfcStyledItem>()) {
			BOOST_FOREACH(const IFC::IfcPresentationStyleAssignment& as, styled->Styles) {
				BOOST_FOREACH(boost::shared_ptr<const IFC::IfcPresentationStyleSelect> sel, as.Styles) {

					if (const IFC::IfcSurfaceStyle* const surf =  sel->ResolveSelectPtr<IFC::IfcSurfaceStyle>(conv.db)) {
						const std::string side = static_cast<std::string>(surf->Side);
						if (side != "BOTH") {
							IFCImporter::LogWarn("ignoring surface side marker on IFC::IfcSurfaceStyle: " + side);
						}

						std::auto_ptr<aiMaterial> mat(new aiMaterial());

						FillMaterial(mat.get(),surf,conv);

						conv.materials.push_back(mat.release());
						return conv.materials.size()-1;
					}
				}
			}
		}
	}
	return 0;
}

} // ! IFC
} // ! Assimp

#endif
