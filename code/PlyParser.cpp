/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team

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
---------------------------------------------------------------------------
*/

/** @file Implementation of the PLY parser class */


#ifndef ASSIMP_BUILD_NO_PLY_IMPORTER

#include "PlyLoader.h"
#include "fast_atof.h"
#include "../include/assimp/DefaultLogger.hpp"
#include "ByteSwapper.h"


using namespace Assimp;

// ------------------------------------------------------------------------------------------------
PLY::EDataType PLY::Property::ParseDataType(const char* pCur,const char** pCurOut)
{
    ai_assert(NULL != pCur && NULL != pCurOut);
    PLY::EDataType eOut = PLY::EDT_INVALID;

    if (TokenMatch(pCur,"char",4) ||
        TokenMatch(pCur,"int8",4))
    {
        eOut = PLY::EDT_Char;
    }
    else if (TokenMatch(pCur,"uchar",5) ||
             TokenMatch(pCur,"uint8",5))
    {
        eOut = PLY::EDT_UChar;
    }
    else if (TokenMatch(pCur,"short",5) ||
             TokenMatch(pCur,"int16",5))
    {
        eOut = PLY::EDT_Short;
    }
    else if (TokenMatch(pCur,"ushort",6) ||
             TokenMatch(pCur,"uint16",6))
    {
        eOut = PLY::EDT_UShort;
    }
    else if (TokenMatch(pCur,"int32",5) || TokenMatch(pCur,"int",3))
    {
        eOut = PLY::EDT_Int;
    }
    else if (TokenMatch(pCur,"uint32",6) || TokenMatch(pCur,"uint",4))
    {
        eOut = PLY::EDT_UInt;
    }
    else if (TokenMatch(pCur,"float",5) || TokenMatch(pCur,"float32",7))
    {
        eOut = PLY::EDT_Float;
    }
    else if (TokenMatch(pCur,"double64",8) || TokenMatch(pCur,"double",6) ||
             TokenMatch(pCur,"float64",7))
    {
        eOut = PLY::EDT_Double;
    }
    if (PLY::EDT_INVALID == eOut)
    {
        DefaultLogger::get()->info("Found unknown data type in PLY file. This is OK");
    }
    *pCurOut = pCur;
    return eOut;
}

// ------------------------------------------------------------------------------------------------
PLY::ESemantic PLY::Property::ParseSemantic(const char* pCur,const char** pCurOut)
{
    ai_assert(NULL != pCur && NULL != pCurOut);

    PLY::ESemantic eOut = PLY::EST_INVALID;
    if (TokenMatch(pCur,"red",3))
    {
        eOut = PLY::EST_Red;
    }
    else if (TokenMatch(pCur,"green",5))
    {
        eOut = PLY::EST_Green;
    }
    else if (TokenMatch(pCur,"blue",4))
    {
        eOut = PLY::EST_Blue;
    }
    else if (TokenMatch(pCur,"alpha",5))
    {
        eOut = PLY::EST_Alpha;
    }
    else if (TokenMatch(pCur,"vertex_index",12) || TokenMatch(pCur,"vertex_indices",14))
    {
        eOut = PLY::EST_VertexIndex;
    }
    else if (TokenMatch(pCur,"material_index",14))
    {
        eOut = PLY::EST_MaterialIndex;
    }
    else if (TokenMatch(pCur,"ambient_red",11))
    {
        eOut = PLY::EST_AmbientRed;
    }
    else if (TokenMatch(pCur,"ambient_green",13))
    {
        eOut = PLY::EST_AmbientGreen;
    }
    else if (TokenMatch(pCur,"ambient_blue",12))
    {
        eOut = PLY::EST_AmbientBlue;
    }
    else if (TokenMatch(pCur,"ambient_alpha",13))
    {
        eOut = PLY::EST_AmbientAlpha;
    }
    else if (TokenMatch(pCur,"diffuse_red",11))
    {
        eOut = PLY::EST_DiffuseRed;
    }
    else if (TokenMatch(pCur,"diffuse_green",13))
    {
        eOut = PLY::EST_DiffuseGreen;
    }
    else if (TokenMatch(pCur,"diffuse_blue",12))
    {
        eOut = PLY::EST_DiffuseBlue;
    }
    else if (TokenMatch(pCur,"diffuse_alpha",13))
    {
        eOut = PLY::EST_DiffuseAlpha;
    }
    else if (TokenMatch(pCur,"specular_red",12))
    {
        eOut = PLY::EST_SpecularRed;
    }
    else if (TokenMatch(pCur,"specular_green",14))
    {
        eOut = PLY::EST_SpecularGreen;
    }
    else if (TokenMatch(pCur,"specular_blue",13))
    {
        eOut = PLY::EST_SpecularBlue;
    }
    else if (TokenMatch(pCur,"specular_alpha",14))
    {
        eOut = PLY::EST_SpecularAlpha;
    }
    else if (TokenMatch(pCur,"opacity",7))
    {
        eOut = PLY::EST_Opacity;
    }
    else if (TokenMatch(pCur,"specular_power",14))
    {
        eOut = PLY::EST_PhongPower;
    }
    else if (TokenMatch(pCur,"r",1))
    {
        eOut = PLY::EST_Red;
    }
    else if (TokenMatch(pCur,"g",1))
    {
        eOut = PLY::EST_Green;
    }
    else if (TokenMatch(pCur,"b",1))
    {
        eOut = PLY::EST_Blue;
    }
    // NOTE: Blender3D exports texture coordinates as s,t tuples
    else if (TokenMatch(pCur,"u",1) ||  TokenMatch(pCur,"s",1) || TokenMatch(pCur,"tx",2))
    {
        eOut = PLY::EST_UTextureCoord;
    }
    else if (TokenMatch(pCur,"v",1) ||  TokenMatch(pCur,"t",1) || TokenMatch(pCur,"ty",2))
    {
        eOut = PLY::EST_VTextureCoord;
    }
    else if (TokenMatch(pCur,"x",1))
    {
        eOut = PLY::EST_XCoord;
    }
    else if (TokenMatch(pCur,"y",1))
    {
        eOut = PLY::EST_YCoord;
    }
    else if (TokenMatch(pCur,"z",1))
    {
        eOut = PLY::EST_ZCoord;
    }
    else if (TokenMatch(pCur,"nx",2))
    {
        eOut = PLY::EST_XNormal;
    }
    else if (TokenMatch(pCur,"ny",2))
    {
        eOut = PLY::EST_YNormal;
    }
    else if (TokenMatch(pCur,"nz",2))
    {
        eOut = PLY::EST_ZNormal;
    }
    else
    {
        DefaultLogger::get()->info("Found unknown property semantic in file. This is ok");
        SkipLine(&pCur);
    }
    *pCurOut = pCur;
    return eOut;
}

// ------------------------------------------------------------------------------------------------
bool PLY::Property::ParseProperty (const char* pCur,
    const char** pCurOut,
    PLY::Property* pOut)
{
    ai_assert(NULL != pCur && NULL != pCurOut);

    // Forms supported:
    // "property float x"
    // "property list uchar int vertex_index"
    *pCurOut = pCur;

    // skip leading spaces
    if (!SkipSpaces(pCur,&pCur))return false;

    // skip the "property" string at the beginning
    if (!TokenMatch(pCur,"property",8))
    {
        // seems not to be a valid property entry
        return false;
    }
    // get next word
    if (!SkipSpaces(pCur,&pCur))return false;
    if (TokenMatch(pCur,"list",4))
    {
        pOut->bIsList = true;

        // seems to be a list.
        if(EDT_INVALID == (pOut->eFirstType = PLY::Property::ParseDataType(pCur, &pCur)))
        {
            // unable to parse list size data type
            SkipLine(pCur,&pCur);
            *pCurOut = pCur;
            return false;
        }
        if (!SkipSpaces(pCur,&pCur))return false;
        if(EDT_INVALID == (pOut->eType = PLY::Property::ParseDataType(pCur, &pCur)))
        {
            // unable to parse list data type
            SkipLine(pCur,&pCur);
            *pCurOut = pCur;
            return false;
        }
    }
    else
    {
        if(EDT_INVALID == (pOut->eType = PLY::Property::ParseDataType(pCur, &pCur)))
        {
            // unable to parse data type. Skip the property
            SkipLine(pCur,&pCur);
            *pCurOut = pCur;
            return false;
        }
    }

    if (!SkipSpaces(pCur,&pCur))return false;
    const char* szCur = pCur;
    pOut->Semantic = PLY::Property::ParseSemantic(pCur, &pCur);

    if (PLY::EST_INVALID == pOut->Semantic)
    {
        // store the name of the semantic
        uintptr_t iDiff = (uintptr_t)pCur - (uintptr_t)szCur;

        DefaultLogger::get()->info("Found unknown semantic in PLY file. This is OK");
        pOut->szName = std::string(szCur,iDiff);
    }

    SkipSpacesAndLineEnd(pCur,&pCur);
    *pCurOut = pCur;
    return true;
}

// ------------------------------------------------------------------------------------------------
PLY::EElementSemantic PLY::Element::ParseSemantic(const char* pCur,
    const char** pCurOut)
{
    ai_assert(NULL != pCur && NULL != pCurOut);
    PLY::EElementSemantic eOut = PLY::EEST_INVALID;
    if (TokenMatch(pCur,"vertex",6))
    {
        eOut = PLY::EEST_Vertex;
    }
    else if (TokenMatch(pCur,"face",4))
    {
        eOut = PLY::EEST_Face;
    }
#if 0
    // TODO: maybe implement this?
    else if (TokenMatch(pCur,"range_grid",10))
    {
        eOut = PLY::EEST_Face;
    }
#endif
    else if (TokenMatch(pCur,"tristrips",9))
    {
        eOut = PLY::EEST_TriStrip;
    }
    else if (TokenMatch(pCur,"edge",4))
    {
        eOut = PLY::EEST_Edge;
    }
    else if (TokenMatch(pCur,"material",8))
    {
        eOut = PLY::EEST_Material;
    }
    *pCurOut = pCur;
    return eOut;
}

// ------------------------------------------------------------------------------------------------
bool PLY::Element::ParseElement (const char* pCur,
    const char** pCurOut,
    PLY::Element* pOut)
{
    ai_assert(NULL != pCur && NULL != pCurOut && NULL != pOut);

    // Example format: "element vertex 8"
    *pCurOut = pCur;

    // skip leading spaces
    if (!SkipSpaces(&pCur))return false;

    // skip the "element" string at the beginning
    if (!TokenMatch(pCur,"element",7))
    {
        // seems not to be a valid property entry
        return false;
    }
    // get next word
    if (!SkipSpaces(&pCur))return false;

    // parse the semantic of the element
    const char* szCur = pCur;
    pOut->eSemantic = PLY::Element::ParseSemantic(pCur,&pCur);
    if (PLY::EEST_INVALID == pOut->eSemantic)
    {
        // if the exact semantic can't be determined, just store
        // the original string identifier
        uintptr_t iDiff = (uintptr_t)pCur - (uintptr_t)szCur;
        pOut->szName = std::string(szCur,iDiff);
    }

    if (!SkipSpaces(&pCur))return false;

    //parse the number of occurrences of this element
    pOut->NumOccur = strtoul10(pCur,&pCur);

    // go to the next line
    SkipSpacesAndLineEnd(pCur,&pCur);

    // now parse all properties of the element
    while(true)
    {
        // skip all comments
        PLY::DOM::SkipComments(pCur,&pCur);

        PLY::Property prop;
        if(!PLY::Property::ParseProperty(pCur,&pCur,&prop))break;
        pOut->alProperties.push_back(prop);
    }
    *pCurOut = pCur;
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::DOM::SkipComments (const char* pCur,
    const char** pCurOut)
{
    ai_assert(NULL != pCur && NULL != pCurOut);
    *pCurOut = pCur;

    // skip spaces
    if (!SkipSpaces(pCur,&pCur))return false;

    if (TokenMatch(pCur,"comment",7))
    {
	     if ( !IsLineEnd(pCur[-1]) )
	     {
	         SkipLine(pCur,&pCur);
	     }
        SkipComments(pCur,&pCur);
        *pCurOut = pCur;
        return true;
    }
    *pCurOut = pCur;
    return false;
}

// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseHeader (const char* pCur,const char** pCurOut,bool isBinary)
{
    ai_assert(NULL != pCur && NULL != pCurOut);
    DefaultLogger::get()->debug("PLY::DOM::ParseHeader() begin");

    // after ply and format line
    *pCurOut = pCur;

    // parse all elements
    while ((*pCur) != '\0')
    {
        // skip all comments
        PLY::DOM::SkipComments(pCur,&pCur);

        PLY::Element out;
        if(PLY::Element::ParseElement(pCur,&pCur,&out))
        {
            // add the element to the list of elements
            alElements.push_back(out);
        }
        else if (TokenMatch(pCur,"end_header",10))
        {
            // we have reached the end of the header
            break;
        }
        else
        {
            // ignore unknown header elements
            SkipLine(&pCur);
        }
    }
    if(!isBinary)
    { // it would occur an error, if binary data start with values as space or line end.
        SkipSpacesAndLineEnd(pCur,&pCur);
    }
    *pCurOut = pCur;

    DefaultLogger::get()->debug("PLY::DOM::ParseHeader() succeeded");
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseElementInstanceLists (
    const char* pCur,
    const char** pCurOut)
{
    ai_assert(NULL != pCur && NULL != pCurOut);

    DefaultLogger::get()->debug("PLY::DOM::ParseElementInstanceLists() begin");
    *pCurOut = pCur;

    alElementData.resize(alElements.size());

    std::vector<PLY::Element>::const_iterator i = alElements.begin();
    std::vector<PLY::ElementInstanceList>::iterator a = alElementData.begin();

    // parse all element instances
    for (;i != alElements.end();++i,++a)
    {
        (*a).alInstances.resize((*i).NumOccur);
        PLY::ElementInstanceList::ParseInstanceList(pCur,&pCur,&(*i),&(*a));
    }

    DefaultLogger::get()->debug("PLY::DOM::ParseElementInstanceLists() succeeded");
    *pCurOut = pCur;
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseElementInstanceListsBinary (
    const char* pCur,
    const char** pCurOut,
    bool p_bBE)
{
    ai_assert(NULL != pCur && NULL != pCurOut);

    DefaultLogger::get()->debug("PLY::DOM::ParseElementInstanceListsBinary() begin");
    *pCurOut = pCur;

    alElementData.resize(alElements.size());

    std::vector<PLY::Element>::const_iterator i = alElements.begin();
    std::vector<PLY::ElementInstanceList>::iterator a = alElementData.begin();

    // parse all element instances
    for (;i != alElements.end();++i,++a)
    {
        (*a).alInstances.resize((*i).NumOccur);
        PLY::ElementInstanceList::ParseInstanceListBinary(pCur,&pCur,&(*i),&(*a),p_bBE);
    }

    DefaultLogger::get()->debug("PLY::DOM::ParseElementInstanceListsBinary() succeeded");
    *pCurOut = pCur;
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseInstanceBinary (const char* pCur,DOM* p_pcOut,bool p_bBE)
{
    ai_assert(NULL != pCur && NULL != p_pcOut);

    DefaultLogger::get()->debug("PLY::DOM::ParseInstanceBinary() begin");

    if(!p_pcOut->ParseHeader(pCur,&pCur,true))
    {
        DefaultLogger::get()->debug("PLY::DOM::ParseInstanceBinary() failure");
        return false;
    }
    if(!p_pcOut->ParseElementInstanceListsBinary(pCur,&pCur,p_bBE))
    {
        DefaultLogger::get()->debug("PLY::DOM::ParseInstanceBinary() failure");
        return false;
    }
    DefaultLogger::get()->debug("PLY::DOM::ParseInstanceBinary() succeeded");
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::DOM::ParseInstance (const char* pCur,DOM* p_pcOut)
{
    ai_assert(NULL != pCur);
    ai_assert(NULL != p_pcOut);

    DefaultLogger::get()->debug("PLY::DOM::ParseInstance() begin");


    if(!p_pcOut->ParseHeader(pCur,&pCur,false))
    {
        DefaultLogger::get()->debug("PLY::DOM::ParseInstance() failure");
        return false;
    }
    if(!p_pcOut->ParseElementInstanceLists(pCur,&pCur))
    {
        DefaultLogger::get()->debug("PLY::DOM::ParseInstance() failure");
        return false;
    }
    DefaultLogger::get()->debug("PLY::DOM::ParseInstance() succeeded");
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstanceList::ParseInstanceList (
    const char* pCur,
    const char** pCurOut,
    const PLY::Element* pcElement,
    PLY::ElementInstanceList* p_pcOut)
{
    ai_assert(NULL != pCur && NULL != pCurOut && NULL != pcElement && NULL != p_pcOut);

    if (EEST_INVALID == pcElement->eSemantic || pcElement->alProperties.empty())
    {
        // if the element has an unknown semantic we can skip all lines
        // However, there could be comments
        for (unsigned int i = 0; i < pcElement->NumOccur;++i)
        {
            PLY::DOM::SkipComments(pCur,&pCur);
            SkipLine(pCur,&pCur);
        }
    }
    else
    {
        // be sure to have enough storage
        for (unsigned int i = 0; i < pcElement->NumOccur;++i)
        {
            PLY::DOM::SkipComments(pCur,&pCur);
            PLY::ElementInstance::ParseInstance(pCur, &pCur,pcElement,
                &p_pcOut->alInstances[i]);
        }
    }
    *pCurOut = pCur;
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstanceList::ParseInstanceListBinary (
    const char* pCur,
    const char** pCurOut,
    const PLY::Element* pcElement,
    PLY::ElementInstanceList* p_pcOut,
    bool p_bBE /* = false */)
{
    ai_assert(NULL != pCur && NULL != pCurOut && NULL != pcElement && NULL != p_pcOut);

    // we can add special handling code for unknown element semantics since
    // we can't skip it as a whole block (we don't know its exact size
    // due to the fact that lists could be contained in the property list
    // of the unknown element)
    for (unsigned int i = 0; i < pcElement->NumOccur;++i)
    {
        PLY::ElementInstance::ParseInstanceBinary(pCur, &pCur,pcElement,
            &p_pcOut->alInstances[i], p_bBE);
    }
    *pCurOut = pCur;
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstance::ParseInstance (
    const char* pCur,
    const char** pCurOut,
    const PLY::Element* pcElement,
    PLY::ElementInstance* p_pcOut)
{
    ai_assert(NULL != pCur && NULL != pCurOut && NULL != pcElement && NULL != p_pcOut);

    if (!SkipSpaces(pCur, &pCur))return false;

    // allocate enough storage
    p_pcOut->alProperties.resize(pcElement->alProperties.size());

    std::vector<PLY::PropertyInstance>::iterator i = p_pcOut->alProperties.begin();
    std::vector<PLY::Property>::const_iterator  a = pcElement->alProperties.begin();
    for (;i != p_pcOut->alProperties.end();++i,++a)
    {
        if(!(PLY::PropertyInstance::ParseInstance(pCur, &pCur,&(*a),&(*i))))
        {
            DefaultLogger::get()->warn("Unable to parse property instance. "
                "Skipping this element instance");

            // skip the rest of the instance
            SkipLine(pCur, &pCur);

            PLY::PropertyInstance::ValueUnion v = PLY::PropertyInstance::DefaultValue((*a).eType);
            (*i).avList.push_back(v);
        }
    }
    *pCurOut = pCur;
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::ElementInstance::ParseInstanceBinary (
    const char* pCur,
    const char** pCurOut,
    const PLY::Element* pcElement,
    PLY::ElementInstance* p_pcOut,
    bool p_bBE /* = false */)
{
    ai_assert(NULL != pCur && NULL != pCurOut && NULL != pcElement && NULL != p_pcOut);

    // allocate enough storage
    p_pcOut->alProperties.resize(pcElement->alProperties.size());

    std::vector<PLY::PropertyInstance>::iterator i =  p_pcOut->alProperties.begin();
    std::vector<PLY::Property>::const_iterator   a =  pcElement->alProperties.begin();
    for (;i != p_pcOut->alProperties.end();++i,++a)
    {
        if(!(PLY::PropertyInstance::ParseInstanceBinary(pCur, &pCur,&(*a),&(*i),p_bBE)))
        {
            DefaultLogger::get()->warn("Unable to parse binary property instance. "
                "Skipping this element instance");

            (*i).avList.push_back(PLY::PropertyInstance::DefaultValue((*a).eType));
        }
    }
    *pCurOut = pCur;
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseInstance (const char* pCur,const char** pCurOut,
    const PLY::Property* prop, PLY::PropertyInstance* p_pcOut)
{
    ai_assert(NULL != pCur && NULL != pCurOut && NULL !=  prop && NULL != p_pcOut);

    *pCurOut = pCur;

    // skip spaces at the beginning
    if (!SkipSpaces(pCur, &pCur))return false;

    if (prop->bIsList)
    {
        // parse the number of elements in the list
        PLY::PropertyInstance::ValueUnion v;
        PLY::PropertyInstance::ParseValue(pCur, &pCur,prop->eFirstType,&v);

        // convert to unsigned int
        unsigned int iNum = PLY::PropertyInstance::ConvertTo<unsigned int>(v,prop->eFirstType);

        // parse all list elements
        p_pcOut->avList.resize(iNum);
        for (unsigned int i = 0; i < iNum;++i)
        {
            if (!SkipSpaces(pCur, &pCur))return false;
            PLY::PropertyInstance::ParseValue(pCur, &pCur,prop->eType,&p_pcOut->avList[i]);
        }
    }
    else
    {
        // parse the property
        PLY::PropertyInstance::ValueUnion v;

        PLY::PropertyInstance::ParseValue(pCur, &pCur,prop->eType,&v);
        p_pcOut->avList.push_back(v);
    }
    SkipSpacesAndLineEnd(pCur, &pCur);
    *pCurOut = pCur;
    return true;
}

// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseInstanceBinary (
    const char*  pCur,
    const char** pCurOut,
    const PLY::Property* prop,
    PLY::PropertyInstance* p_pcOut,
    bool p_bBE)
{
    ai_assert(NULL != pCur && NULL != pCurOut && NULL != prop && NULL != p_pcOut);

    if (prop->bIsList)
    {
        // parse the number of elements in the list
        PLY::PropertyInstance::ValueUnion v;
        PLY::PropertyInstance::ParseValueBinary(pCur, &pCur,prop->eFirstType,&v,p_bBE);

        // convert to unsigned int
        unsigned int iNum = PLY::PropertyInstance::ConvertTo<unsigned int>(v,prop->eFirstType);

        // parse all list elements
        p_pcOut->avList.resize(iNum);
        for (unsigned int i = 0; i < iNum;++i){
            PLY::PropertyInstance::ParseValueBinary(pCur, &pCur,prop->eType,&p_pcOut->avList[i],p_bBE);
        }
    }
    else
    {
        // parse the property
        PLY::PropertyInstance::ValueUnion v;
        PLY::PropertyInstance::ParseValueBinary(pCur, &pCur,prop->eType,&v,p_bBE);
        p_pcOut->avList.push_back(v);
    }
    *pCurOut = pCur;
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
        out.fFloat = 0.f;
        return out;

    case EDT_Double:
        out.fDouble = 0.;
        return out;

    default: ;
    };
    out.iUInt = 0;
    return out;
}

// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseValue(
    const char* pCur,
    const char** pCurOut,
    PLY::EDataType eType,
    PLY::PropertyInstance::ValueUnion* out)
{
    ai_assert(NULL != pCur && NULL != pCurOut && NULL != out);

    bool ret = true;
    *pCurOut = pCur;
    switch (eType)
    {
    case EDT_UInt:
    case EDT_UShort:
    case EDT_UChar:

        out->iUInt = (uint32_t)strtoul10(pCur, &pCur);
        break;

    case EDT_Int:
    case EDT_Short:
    case EDT_Char:

        out->iInt = (int32_t)strtol10(pCur, &pCur);
        break;

    case EDT_Float:

        pCur = fast_atoreal_move<float>(pCur,out->fFloat);
        break;

    case EDT_Double:

        float f;
        pCur = fast_atoreal_move<float>(pCur,f);
        out->fDouble = (double)f;
        break;

    default:
        ret = false;
    }
    *pCurOut = pCur;
    return ret;
}

// ------------------------------------------------------------------------------------------------
bool PLY::PropertyInstance::ParseValueBinary(
    const char* pCur,
    const char** pCurOut,
    PLY::EDataType eType,
    PLY::PropertyInstance::ValueUnion* out,
    bool p_bBE)
{
    ai_assert(NULL != pCur && NULL != pCurOut && NULL != out);

    bool ret = true;
    switch (eType)
    {
    case EDT_UInt:
        out->iUInt = (uint32_t)*((uint32_t*)pCur);
        pCur += 4;

        // Swap endianness
        if (p_bBE)ByteSwap::Swap((int32_t*)&out->iUInt);
        break;

    case EDT_UShort:
        {
        uint16_t i = *((uint16_t*)pCur);

        // Swap endianness
        if (p_bBE)ByteSwap::Swap(&i);
        out->iUInt = (uint32_t)i;
        pCur += 2;
        break;
        }

    case EDT_UChar:
        {
        out->iUInt = (uint32_t)(*((uint8_t*)pCur));
        pCur ++;
        break;
        }

    case EDT_Int:
        out->iInt = *((int32_t*)pCur);
        pCur += 4;

        // Swap endianness
        if (p_bBE)ByteSwap::Swap(&out->iInt);
        break;

    case EDT_Short:
        {
        int16_t i = *((int16_t*)pCur);

        // Swap endianness
        if (p_bBE)ByteSwap::Swap(&i);
        out->iInt = (int32_t)i;
        pCur += 2;
        break;
        }

    case EDT_Char:
        out->iInt = (int32_t)*((int8_t*)pCur);
        pCur ++;
        break;

    case EDT_Float:
        {
        out->fFloat = *((float*)pCur);

        // Swap endianness
        if (p_bBE)ByteSwap::Swap((int32_t*)&out->fFloat);
        pCur += 4;
        break;
        }
    case EDT_Double:
        {
        out->fDouble = *((double*)pCur);

        // Swap endianness
        if (p_bBE)ByteSwap::Swap((int64_t*)&out->fDouble);
        pCur += 8;
        break;
        }
    default:
        ret = false;
    }
    *pCurOut = pCur;
    return ret;
}

#endif // !! ASSIMP_BUILD_NO_PLY_IMPORTER
