/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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
---------------------------------------------------------------------------
*/

/** @file Implementation of the PLY parser class */

#include "PlyLoader.h"
#include "MaterialSystem.h"
#include "fast_atof.h"
#include "StringComparison.h"
#include "ByteSwap.h"

#include "../include/IOStream.h"
#include "../include/IOSystem.h"
#include "../include/aiMesh.h"
#include "../include/aiScene.h"
#include "../include/aiAssert.h"
#include "../include/DefaultLogger.h"

#include <boost/scoped_ptr.hpp>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
void ValidateOut(const char* szCur, const char* szMax)
{
	if (szCur > szMax)
	{
		throw new ImportErrorException("Buffer overflow. PLY file contains invalid indices");
	}
	return;
}
// ------------------------------------------------------------------------------------------------
PLY::EDataType PLY::Property::ParseDataType(const char* p_szIn,const char** p_szOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);

	const char* szMax = *p_szOut;

	PLY::EDataType eOut = PLY::EDT_INVALID;

	if (0 == ASSIMP_strincmp(p_szIn,"char",4) ||
		0 == ASSIMP_strincmp(p_szIn,"int8",4))
	{
		p_szIn+=4;
		eOut = PLY::EDT_Char;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"uchar",5) ||
			 0 == ASSIMP_strincmp(p_szIn,"uint8",5))
	{
		p_szIn+=5;
		eOut = PLY::EDT_UChar;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"short",5) ||
			 0 == ASSIMP_strincmp(p_szIn,"int16",5))
	{
		p_szIn+=5;
		eOut = PLY::EDT_Short;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ushort",6) ||
			 0 == ASSIMP_strincmp(p_szIn,"uint16",6))
	{
		p_szIn+=6;
		eOut = PLY::EDT_UShort;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"int32",5))
	{
		p_szIn+=5;
		eOut = PLY::EDT_Int;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"uint32",6))
	{
		p_szIn+=6;
		eOut = PLY::EDT_UInt;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"int",3))
	{
		p_szIn+=3;
		eOut = PLY::EDT_Int;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"uint",4))
	{
		p_szIn+=4;
		eOut = PLY::EDT_UInt;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"float32",7))
	{
		p_szIn+=7;
		eOut = PLY::EDT_Float;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"float",5))
	{
		p_szIn+=5;
		eOut = PLY::EDT_Float;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"float64",7))
	{
		p_szIn+=7;
		eOut = PLY::EDT_Double;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"double64",8))
	{
		p_szIn+=8;
		eOut = PLY::EDT_Double;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"double",6))
	{
		p_szIn+=6;
		eOut = PLY::EDT_Double;
	}
	// either end of line or space, but no other characters allowed
	if (!(IsSpace(*p_szIn) || IsLineEnd(*p_szIn)))
	{
		eOut = PLY::EDT_INVALID;
	}
	if (PLY::EDT_INVALID == eOut)
	{
		DefaultLogger::get()->info("Found unknown data type in PLY file. This is OK");
	}
	*p_szOut = p_szIn;
	ValidateOut(p_szIn,szMax);
	return eOut;
}
// ------------------------------------------------------------------------------------------------
PLY::ESemantic PLY::Property::ParseSemantic(const char* p_szIn,const char** p_szOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);

	const char* szMax = *p_szOut;

	PLY::ESemantic eOut = PLY::EST_INVALID;
	if (0 == ASSIMP_strincmp(p_szIn,"red",3))
	{
		p_szIn+=3;
		eOut = PLY::EST_Red;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"green",4))
	{
		p_szIn+=5;
		eOut = PLY::EST_Green;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"blue",4))
	{
		p_szIn+=4;
		eOut = PLY::EST_Blue;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"alpha",5))
	{
		p_szIn+=5;
		eOut = PLY::EST_Alpha;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"vertex_index",12))
	{
		p_szIn+=12;
		eOut = PLY::EST_VertexIndex;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"vertex_indices",14))
	{
		p_szIn+=14;
		eOut = PLY::EST_VertexIndex;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"material_index",14))
	{
		p_szIn+=14;
		eOut = PLY::EST_MaterialIndex;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ambient_red",11))
	{
		p_szIn+=11;
		eOut = PLY::EST_AmbientRed;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ambient_green",13))
	{
		p_szIn+=13;
		eOut = PLY::EST_AmbientGreen;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ambient_blue",12))
	{
		p_szIn+=12;
		eOut = PLY::EST_AmbientBlue;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ambient_alpha",13))
	{
		p_szIn+=13;
		eOut = PLY::EST_AmbientAlpha;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"diffuse_red",11))
	{
		p_szIn+=11;
		eOut = PLY::EST_DiffuseRed;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"diffuse_green",13))
	{
		p_szIn+=13;
		eOut = PLY::EST_DiffuseGreen;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"diffuse_blue",12))
	{
		p_szIn+=12;
		eOut = PLY::EST_DiffuseBlue;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"diffuse_alpha",13))
	{
		p_szIn+=13;
		eOut = PLY::EST_DiffuseAlpha;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"specular_red",12))
	{
		p_szIn+=12;
		eOut = PLY::EST_SpecularRed;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"specular_green",14))
	{
		p_szIn+=14;
		eOut = PLY::EST_SpecularGreen;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"specular_blue",13))
	{
		p_szIn+=13;
		eOut = PLY::EST_SpecularBlue;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"specular_alpha",14))
	{
		p_szIn+=14;
		eOut = PLY::EST_SpecularAlpha;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"opacity",7))
	{
		p_szIn+=7;
		eOut = PLY::EST_Opacity;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"specular_power",6))
	{
		p_szIn+=7;
		eOut = PLY::EST_PhongPower;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"r",1))
	{
		p_szIn++;
		eOut = PLY::EST_Red;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"g",1))
	{
		p_szIn++;
		eOut = PLY::EST_Green;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"b",1))
	{
		p_szIn++;
		eOut = PLY::EST_Blue;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"tx",2))
	{
		p_szIn+=2;
		eOut = PLY::EST_UTextureCoord;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"ty",2))
	{
		p_szIn+=2;
		eOut = PLY::EST_VTextureCoord;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"u",1))
	{
		p_szIn++;
		eOut = PLY::EST_UTextureCoord;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"v",1))
	{
		p_szIn++;
		eOut = PLY::EST_VTextureCoord;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"x",1))
	{
		p_szIn++;
		eOut = PLY::EST_XCoord;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"y",1))
	{
		p_szIn++;
		eOut = PLY::EST_YCoord;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"z",1))
	{
		p_szIn++;
		eOut = PLY::EST_ZCoord;
	}
	else
	{
		DefaultLogger::get()->info("Found unknown property in file. This is ok");

		// ... find the next space or new line
		while (*p_szIn != ' ' && *p_szIn != '\t'  && 
			   *p_szIn != '\r' && *p_szIn != '\0' && *p_szIn != '\n')p_szIn++;
	}
	// either end of line or space, but no other characters allowed
	if (!(IsSpace(*p_szIn) || IsLineEnd(*p_szIn)))
	{
		eOut = PLY::EST_INVALID;
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return eOut;
}
// ------------------------------------------------------------------------------------------------
bool PLY::Property::ParseProperty (const char* p_szIn,
	const char** p_szOut,
	PLY::Property* pOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);

	// Forms supported:
	// "property float x"
	// "property list uchar int vertex_index"
	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	// skip leading spaces
	if (!SkipSpaces(p_szIn,&p_szIn))return false;

	// skip the "property" string at the beginning
	if (0 != ASSIMP_strincmp(p_szIn,"property",8) || !IsSpace(*(p_szIn+8)))
	{
		// seems not to be a valid property entry
		return false;
	}
	// get next word
	p_szIn += 9;
	if (!SkipSpaces(p_szIn,&p_szIn))return false;

	if (0 == ASSIMP_strincmp(p_szIn,"list",4) && IsSpace(*(p_szIn+4)))
	{
		pOut->bIsList = true;

		// seems to be a list.
		p_szIn += 5;
		const char* szPass = szMax;
		if(EDT_INVALID == (pOut->eFirstType = PLY::Property::ParseDataType(p_szIn, &szPass)))
		{
			// unable to parse list size data type
			SkipLine(p_szIn,&p_szIn);
			*p_szOut = p_szIn;
			return false;
		}
		p_szIn = szPass;
		if (!SkipSpaces(p_szIn,&p_szIn))return false;
		szPass = szMax;
		if(EDT_INVALID == (pOut->eType = PLY::Property::ParseDataType(p_szIn, &szPass)))
		{
			// unable to parse list data type
			SkipLine(p_szIn,&p_szIn);
			*p_szOut = p_szIn;
			return false;
		}
		p_szIn = szPass;
	}
	else
	{
		const char* szPass = szMax;
		if(EDT_INVALID == (pOut->eType = PLY::Property::ParseDataType(p_szIn, &szPass)))
		{
			// unable to parse data type. Skip the property
			SkipLine(p_szIn,&p_szIn);
			*p_szOut = p_szIn;
			return false;
		}
		p_szIn = szPass;
	}
	
	if (!SkipSpaces(p_szIn,&p_szIn))return false;
	const char* szCur = p_szIn;
	const char* szPass = szMax;
	pOut->Semantic = PLY::Property::ParseSemantic(p_szIn, &szPass);
	p_szIn = szPass;

	if (PLY::EST_INVALID == pOut->Semantic)
	{
		// store the name of the semantic
		uintptr_t iDiff = (uintptr_t)p_szIn - (uintptr_t)szCur;

		DefaultLogger::get()->info("Found unknown semantic in PLY file. This is OK");
		pOut->szName = std::string(szCur,iDiff);
	}

	SkipSpacesAndLineEnd(p_szIn,&p_szIn);
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
PLY::EElementSemantic PLY::Element::ParseSemantic(const char* p_szIn,
	const char** p_szOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);

	const char* szMax = *p_szOut;

	PLY::EElementSemantic eOut = PLY::EEST_INVALID;
	if (0 == ASSIMP_strincmp(p_szIn,"vertex",6))
	{
		p_szIn+=6;
		eOut = PLY::EEST_Vertex;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"face",4))
	{
		p_szIn+=4;
		eOut = PLY::EEST_Face;
	}
#if 0
	else if (0 == ASSIMP_strincmp(p_szIn,"range_grid",10))
	{
		p_szIn+=10;
		eOut = PLY::EEST_Face;
	}
#endif
	else if (0 == ASSIMP_strincmp(p_szIn,"tristrips",9))
	{
		p_szIn+=9;
		eOut = PLY::EEST_TriStrip;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"edge",4))
	{
		p_szIn+=4;
		eOut = PLY::EEST_Edge;
	}
	else if (0 == ASSIMP_strincmp(p_szIn,"material",8))
	{
		p_szIn+=8;
		eOut = PLY::EEST_Material;
	}

	// either end of line or space, but no other characters allowed
	if (!(IsSpace(*p_szIn) || IsLineEnd(*p_szIn)))
	{
		eOut = PLY::EEST_INVALID;
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return eOut;
}
// ------------------------------------------------------------------------------------------------
bool PLY::Element::ParseElement (const char* p_szIn, 
	const char** p_szOut,
	PLY::Element* pOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);
	ai_assert(NULL != pOut);

	// Example format: "element vertex 8"
	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	// skip leading spaces
	if (!SkipSpaces(p_szIn,&p_szIn))return false;

	// skip the "element" string at the beginning
	if (0 != ASSIMP_strincmp(p_szIn,"element",7) || !IsSpace(*(p_szIn+7)))
	{
		// seems not to be a valid property entry
		return false;
	}
	// get next word
	p_szIn += 8;
	if (!SkipSpaces(p_szIn,&p_szIn))return false;

	// parse the semantic of the element
	const char* szCur = p_szIn;
	const char* szPass = szMax;
	pOut->eSemantic = PLY::Element::ParseSemantic(p_szIn,&szPass);
	p_szIn = szPass;

	if (PLY::EEST_INVALID == pOut->eSemantic)
	{
		// store the name of the semantic
		uintptr_t iDiff = (uintptr_t)p_szIn - (uintptr_t)szCur;

		pOut->szName = std::string(szCur,iDiff);
	}

	if (!SkipSpaces(p_szIn,&p_szIn))return false;
	
	//parse the number of occurences of this element
	pOut->NumOccur = strtol10(p_szIn,&p_szIn);

	// go to the next line
	SkipSpacesAndLineEnd(p_szIn,&p_szIn);

	// now parse all properties of the element
	while(true)
	{
		// skip all comments
		PLY::DOM::SkipComments(p_szIn,&p_szIn);

		PLY::Property* prop = new PLY::Property();

		const char* szPass = szMax;
		if(!PLY::Property::ParseProperty(p_szIn,&szPass,prop))break;
		p_szIn = szPass;

		// add the property to the property list
		pOut->alProperties.push_back(prop);
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::SkipComments (const char* p_szIn,
	const char** p_szOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);

	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	// skip spaces
	if (!SkipSpaces(p_szIn,&p_szIn))return false;

	if (0 == ASSIMP_strincmp(p_szIn,"comment",7))
	{
		p_szIn += 7;

		SkipLine(p_szIn,&p_szIn);

		SkipComments(p_szIn,&p_szIn);
		*p_szOut = p_szIn;
		return true;
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return false;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseHeader (const char* p_szIn,const char** p_szOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);

	DefaultLogger::get()->debug("PLY::DOM::ParseHeader() begin");

	// after ply and format line
	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	// parse all elements
	while (true)
	{
		// skip all comments
		PLY::DOM::SkipComments(p_szIn,&p_szIn);

		PLY::Element* out = new PLY::Element();
		const char* szPass = szMax;
		if(PLY::Element::ParseElement(p_szIn,&szPass,out))
		{
			p_szIn = szPass;

			// add the element to the list of elements
			this->alElements.push_back(out);
		}
		else if (0 == ASSIMP_strincmp(p_szIn,"end_header",10) && IsSpaceOrNewLine(*(p_szIn+10)))
		{
			// we have reached the end of the header
			p_szIn += 11;
			break;
		}
		// ignore unknown header elements
	}
	SkipSpacesAndLineEnd(p_szIn,&p_szIn);
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;

	DefaultLogger::get()->debug("PLY::DOM::ParseHeader() succeeded");
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseElementInstanceLists (
	const char* p_szIn,
	const char** p_szOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);

	DefaultLogger::get()->debug("PLY::DOM::ParseElementInstanceLists() begin");

	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	this->alElementData.resize(this->alElements.size());

	std::vector<PLY::Element*>::const_iterator i = this->alElements.begin();
	std::vector<PLY::ElementInstanceList*>::iterator a = this->alElementData.begin();

	// parse all element instances
	for (;i != this->alElements.end();++i,++a)
	{
		*a = new PLY::ElementInstanceList((*i)); // reserve enough storage

		const char* szPass = szMax;
		PLY::ElementInstanceList::ParseInstanceList(p_szIn,&szPass,(*i),(*a));
		p_szIn = szPass;
	}

	DefaultLogger::get()->debug("PLY::DOM::ParseElementInstanceLists() succeeded");
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseElementInstanceListsBinary (
	const char* p_szIn,
	const char** p_szOut,
	bool p_bBE)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);

	DefaultLogger::get()->debug("PLY::DOM::ParseElementInstanceListsBinary() begin");

	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;
	
	this->alElementData.resize(this->alElements.size());

	std::vector<PLY::Element*>::const_iterator i = this->alElements.begin();
	std::vector<PLY::ElementInstanceList*>::iterator a = this->alElementData.begin();

	// parse all element instances
	for (;i != this->alElements.end();++i,++a)
	{
		*a = new PLY::ElementInstanceList((*i)); // reserve enough storage
		const char* szPass = szMax;
		PLY::ElementInstanceList::ParseInstanceListBinary(p_szIn,&szPass,(*i),(*a),p_bBE);
		p_szIn = szPass;
	}

	DefaultLogger::get()->debug("PLY::DOM::ParseElementInstanceListsBinary() succeeded");
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseInstanceBinary (const char* p_szIn,DOM* p_pcOut,bool p_bBE,unsigned int iSize)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_pcOut);

	DefaultLogger::get()->debug("PLY::DOM::ParseInstanceBinary() begin");

	const char* szMax = p_szIn + iSize;
	const char* szPass = szMax;
	if(!p_pcOut->ParseHeader(p_szIn,&szPass))
	{
		DefaultLogger::get()->debug("PLY::DOM::ParseInstanceBinary() failure");
		return false;
	}
	p_szIn = szPass;
	szPass = szMax;
	if(!p_pcOut->ParseElementInstanceListsBinary(p_szIn,&szPass,p_bBE))
	{
		DefaultLogger::get()->debug("PLY::DOM::ParseInstanceBinary() failure");
		return false;
	}
	DefaultLogger::get()->debug("PLY::DOM::ParseInstanceBinary() succeeded");
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseInstance (const char* p_szIn,DOM* p_pcOut,unsigned int iSize)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_pcOut);

	DefaultLogger::get()->debug("PLY::DOM::ParseInstance() begin");

	const char* szMax = p_szIn + iSize;
	const char* szPass = szMax;
	if(!p_pcOut->ParseHeader(p_szIn,&szPass))
	{
		DefaultLogger::get()->debug("PLY::DOM::ParseInstance() failure");
		return false;
	}
	p_szIn = szPass;
	szPass = szMax;
	if(!p_pcOut->ParseElementInstanceLists(p_szIn,&szPass))
	{
		DefaultLogger::get()->debug("PLY::DOM::ParseInstance() failure");
		return false;
	}
	DefaultLogger::get()->debug("PLY::DOM::ParseInstance() succeeded");
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstanceList::ParseInstanceList (
	const char* p_szIn,
	const char** p_szOut,
	const PLY::Element* pcElement, 
	PLY::ElementInstanceList* p_pcOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);
	ai_assert(NULL != pcElement);
	ai_assert(NULL != p_pcOut);

	const char* szMax = *p_szOut;

	if (EEST_INVALID == pcElement->eSemantic)
	{
		// if the element has an unknown semantic we can skip all lines
		// However, there could be comments
		for (unsigned int i = 0; i < pcElement->NumOccur;++i)
		{
			PLY::DOM::SkipComments(p_szIn,&p_szIn);
			SkipLine(p_szIn,&p_szIn);
		}
	}
	else
	{
		// be sure to have enough storage
		p_pcOut->alInstances.resize(pcElement->NumOccur);
		for (unsigned int i = 0; i < pcElement->NumOccur;++i)
		{
			PLY::DOM::SkipComments(p_szIn,&p_szIn);

			PLY::ElementInstance* out = new PLY::ElementInstance();

			const char* szPass = szMax;
			PLY::ElementInstance::ParseInstance(p_szIn, &szPass,pcElement, out);
			p_szIn = szPass;
			// add it to the list
			p_pcOut->alInstances[i] = out;
		}
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstanceList::ParseInstanceListBinary (
	const char* p_szIn,
	const char** p_szOut,
	const PLY::Element* pcElement,
	PLY::ElementInstanceList* p_pcOut,
	bool p_bBE /* = false */)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);
	ai_assert(NULL != pcElement);
	ai_assert(NULL != p_pcOut);

	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	// we can add special handling code for unknown element semantics since
	// we can't skip it as a whole block (we don't know its exact size
	// due to the fact that lists could be contained in the property list 
	// of the unknown element)
	for (unsigned int i = 0; i < pcElement->NumOccur;++i)
	{
		PLY::ElementInstance* out = new PLY::ElementInstance();
		const char* szPass = szMax;
		PLY::ElementInstance::ParseInstanceBinary(p_szIn, &p_szIn,pcElement, out, p_bBE);
		p_szIn = szPass;
		// add it to the list
		p_pcOut->alInstances[i] = out;
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstance::ParseInstance (
	const char* p_szIn,
	const char** p_szOut,
	const PLY::Element* pcElement,
	PLY::ElementInstance* p_pcOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);
	ai_assert(NULL != pcElement);
	ai_assert(NULL != p_pcOut);

	if (!SkipSpaces(p_szIn, &p_szIn))return false;

	// allocate enough storage
	p_pcOut->alProperties.resize(pcElement->alProperties.size());

	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	std::vector<PLY::PropertyInstance>::iterator i = p_pcOut->alProperties.begin();
	std::vector<PLY::Property*>::const_iterator a = pcElement->alProperties.begin();
	for (;i != p_pcOut->alProperties.end();++i,++a)
	{
		const char* szPass = szMax;
		if(!(PLY::PropertyInstance::ParseInstance(p_szIn, &szPass,(*a),&(*i))))
		{
			DefaultLogger::get()->warn("Unable to parse property instance. "
				"Skipping this element instance");

			// skip the rest of the instance
			SkipLine(p_szIn, &p_szIn);

			PLY::PropertyInstance::ValueUnion v = PLY::PropertyInstance::DefaultValue((*a)->eType);
			(*i).avList.push_back(v);
		}
		p_szIn = szPass;
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstance::ParseInstanceBinary (
	const char* p_szIn,
	const char** p_szOut,
	const PLY::Element* pcElement,
	PLY::ElementInstance* p_pcOut,
	bool p_bBE /* = false */)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);
	ai_assert(NULL != pcElement);
	ai_assert(NULL != p_pcOut);

	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	// allocate enough storage
	p_pcOut->alProperties.resize(pcElement->alProperties.size());

	std::vector<PLY::PropertyInstance>::iterator i =  p_pcOut->alProperties.begin();
	std::vector<PLY::Property*>::const_iterator a =  pcElement->alProperties.begin();
	for (;i != p_pcOut->alProperties.end();++i,++a)
	{
		const char* szPass = szMax;
		if(!(PLY::PropertyInstance::ParseInstanceBinary(p_szIn, &szPass,(*a),&(*i),p_bBE)))
		{
			DefaultLogger::get()->warn("Unable to parse binary property instance. "
				"Skipping this element instance");

			PLY::PropertyInstance::ValueUnion v = PLY::PropertyInstance::DefaultValue((*a)->eType);
			(*i).avList.push_back(v);
		}
		p_szIn = szPass;
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseInstance (const char* p_szIn,const char** p_szOut,
	const PLY::Property* prop, PLY::PropertyInstance* p_pcOut)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);
	ai_assert(NULL != prop);
	ai_assert(NULL != p_pcOut);

	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	// skip spaces at the beginning
	if (!SkipSpaces(p_szIn, &p_szIn))return false;

	if (prop->bIsList)
	{
		// parse the number of elements in the list
		PLY::PropertyInstance::ValueUnion v;

		const char* szPass = szMax;
		PLY::PropertyInstance::ParseValue(p_szIn, &szPass,prop->eFirstType,&v);
		p_szIn = szPass;

		// convert to unsigned int
		unsigned int iNum = PLY::PropertyInstance::ConvertTo<unsigned int>(v,prop->eFirstType);

		// parse all list elements
		for (unsigned int i = 0; i < iNum;++i)
		{
			if (!SkipSpaces(p_szIn, &p_szIn))return false;

			const char* szPass = szMax;
			PLY::PropertyInstance::ParseValue(p_szIn, &szPass,prop->eType,&v);
			p_szIn = szPass;
			p_pcOut->avList.push_back(v);
		}
	}
	else
	{
		// parse the property
		PLY::PropertyInstance::ValueUnion v;

		const char* szPass = szMax;
		PLY::PropertyInstance::ParseValue(p_szIn, &szPass,prop->eType,&v);
		p_szIn = szPass;
		p_pcOut->avList.push_back(v);
	}
	SkipSpacesAndLineEnd(p_szIn, &p_szIn);
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseInstanceBinary (const char* p_szIn,const char** p_szOut,
	const PLY::Property* prop, PLY::PropertyInstance* p_pcOut,bool p_bBE)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);
	ai_assert(NULL != prop);
	ai_assert(NULL != p_pcOut);

	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	if (prop->bIsList)
	{
		// parse the number of elements in the list
		PLY::PropertyInstance::ValueUnion v;
		const char* szPass = szMax;
		PLY::PropertyInstance::ParseValueBinary(p_szIn, &szPass,prop->eFirstType,&v,p_bBE);
		p_szIn = szPass;

		// convert to unsigned int
		unsigned int iNum = PLY::PropertyInstance::ConvertTo<unsigned int>(v,prop->eFirstType);

		// parse all list elements
		for (unsigned int i = 0; i < iNum;++i)
		{
			const char* szPass = szMax;
			PLY::PropertyInstance::ParseValueBinary(p_szIn, &szPass,prop->eType,&v,p_bBE);
			p_szIn = szPass;
			p_pcOut->avList.push_back(v);
		}
	}
	else
	{
		// parse the property
		PLY::PropertyInstance::ValueUnion v;
		const char* szPass = szMax;
		PLY::PropertyInstance::ParseValueBinary(p_szIn, &szPass,prop->eType,&v,p_bBE);
		p_szIn = szPass;
		p_pcOut->avList.push_back(v);
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
PLY::PropertyInstance::ValueUnion PLY::PropertyInstance::DefaultValue(
	PLY::EDataType eType)
{
	PLY::PropertyInstance::ValueUnion out;

	switch (eType)
	{
	case EDT_Float:
		out.fFloat = 0.0f;
		return out;

	case EDT_Double:
		out.fDouble = 0.0;
		return out;
	};
	out.iUInt = 0;
	return out;
}
// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseValue(const char* p_szIn,const char** p_szOut,
	PLY::EDataType eType,PLY::PropertyInstance::ValueUnion* out)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);
	ai_assert(NULL != out);

	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	switch (eType)
	{
	case EDT_UInt:
	case EDT_UShort:
	case EDT_UChar:

		// simply parse in a full uint
		out->iUInt = (uint32_t)strtol10(p_szIn, &p_szIn);

		break;

	case EDT_Int:
	case EDT_Short:
	case EDT_Char:

		{
		// simply parse in a full int
		// Take care of the sign at the beginning
		bool bMinus = false;
		if (*p_szIn == '-')
		{
			p_szIn++;
			bMinus = true;
		}
		out->iInt = (int32_t)strtol10(p_szIn, &p_szIn);
		if (bMinus)out->iInt *= -1;

		break;
		}

	case EDT_Float:

		// parse a simple float
		p_szIn = fast_atof_move(p_szIn,out->fFloat);
		break;

	case EDT_Double:

		// Parse a double float. .. TODO: support this
		float f;
		p_szIn = fast_atof_move(p_szIn,f);
		out->fDouble = (double)f;

	default:
		return false;
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseValueBinary(
	const char* p_szIn,
	const char** p_szOut,
	PLY::EDataType eType,
	PLY::PropertyInstance::ValueUnion* out, 
	bool p_bBE)
{
	ai_assert(NULL != p_szIn);
	ai_assert(NULL != p_szOut);
	ai_assert(NULL != out);

	const char* szMax = *p_szOut;
	*p_szOut = p_szIn;

	switch (eType)
	{
	case EDT_UInt:
		out->iUInt = (uint32_t)*((uint32_t*)p_szIn);
		p_szIn += 4;
		
		if (p_bBE)
		{
			ByteSwap::Swap((int32_t*)&out->iUInt);
		}
		break;

	case EDT_UShort:
		{
		uint16_t i = *((uint16_t*)p_szIn);
		if (p_bBE)
			{
			ByteSwap::Swap((int16_t*)&i);
			}
		out->iUInt = (uint32_t)i;
		p_szIn += 2;
		break;
		}

	case EDT_UChar:
		{
		uint8_t i = *((uint8_t*)p_szIn);
		out->iUInt = (uint32_t)i;
		p_szIn ++;
		break;
		}

	case EDT_Int:
		out->iInt = *((int32_t*)p_szIn);
		p_szIn += 4;
		
		if (p_bBE)
		{
			ByteSwap::Swap((int32_t*)&out->iInt);
		}
		break;

	case EDT_Short:
		{
		int16_t i = *((int16_t*)p_szIn);
		if (p_bBE)
			{
			ByteSwap::Swap((int16_t*)&i);
			}
		out->iInt = (int32_t)i;
		p_szIn += 2;
		break;
		}

	case EDT_Char:
		out->iInt = (int32_t)*((int8_t*)p_szIn);
		p_szIn ++;
		break;

	case EDT_Float:
		{
		int32_t* pf = (int32_t*)p_szIn;
		if (p_bBE)
		{
			ByteSwap::Swap((int32_t*)&pf);
		}
		p_szIn += 4;
		out->fFloat = *((float*)&pf);
		break;
		}
	case EDT_Double:
		{
		int64_t* pf = (int64_t*)p_szIn;
		if (p_bBE)
		{
			ByteSwap::Swap((int64_t*)&pf);
		}
		p_szIn += 8;
		out->fDouble = *((double*)&pf);
		break;
		}
	default:
		return false;
	}
	ValidateOut(p_szIn,szMax);
	*p_szOut = p_szIn;
	return true;
}
