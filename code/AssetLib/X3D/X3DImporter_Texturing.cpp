/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team

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
/// \file   X3DImporter_Texturing.cpp
/// \brief  Parsing data from nodes of "Texturing" set of X3D.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DImporter.hpp"
#include "X3DImporter_Macro.hpp"
#include "X3DXmlHelper.h"

namespace Assimp {

// <ImageTexture
// DEF=""         ID
// USE=""         IDREF
// repeatS="true" SFBool
// repeatT="true" SFBool
// url=""         MFString
// />
// When the url field contains no values ([]), texturing is disabled.
void X3DImporter::readImageTexture(XmlNode &node) {
    std::string use, def;
    bool repeatS = true;
    bool repeatT = true;
    std::list<std::string> url;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getBoolAttribute(node, "repeatS", repeatS);
    XmlParser::getBoolAttribute(node, "repeatT", repeatT);
    X3DXmlHelper::getStringListAttribute(node, "url", url);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_ImageTexture, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementImageTexture(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        ((X3DNodeElementImageTexture *)ne)->RepeatS = repeatS;
        ((X3DNodeElementImageTexture *)ne)->RepeatT = repeatT;
        // Attribute "url" can contain list of strings. But we need only one - first.
        if (!url.empty())
            ((X3DNodeElementImageTexture *)ne)->URL = url.front();
        else
            ((X3DNodeElementImageTexture *)ne)->URL = "";

        // check for X3DMetadataObject childs.
        if (!isNodeEmpty(node))
            childrenReadMetadata(node, ne, "ImageTexture");
        else
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <TextureCoordinate
// DEF=""      ID
// USE=""      IDREF
// point=""    MFVec3f [inputOutput]
// />
void X3DImporter::readTextureCoordinate(XmlNode &node) {
    std::string use, def;
    std::list<aiVector2D> point;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    X3DXmlHelper::getVector2DListAttribute(node, "point", point);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_TextureCoordinate, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementTextureCoordinate(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        ((X3DNodeElementTextureCoordinate *)ne)->Value = point;
        // check for X3DMetadataObject childs.
        if (!isNodeEmpty(node))
            childrenReadMetadata(node, ne, "TextureCoordinate");
        else
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <TextureTransform
// DEF=""            ID
// USE=""            IDREF
// center="0 0"      SFVec2f [inputOutput]
// rotation="0"      SFFloat [inputOutput]
// scale="1 1"       SFVec2f [inputOutput]
// translation="0 0" SFVec2f [inputOutput]
// />
void X3DImporter::readTextureTransform(XmlNode &node) {
    std::string use, def;
    aiVector2D center(0, 0);
    float rotation = 0;
    aiVector2D scale(1, 1);
    aiVector2D translation(0, 0);
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    X3DXmlHelper::getVector2DAttribute(node, "center", center);
    XmlParser::getFloatAttribute(node, "rotation", rotation);
    X3DXmlHelper::getVector2DAttribute(node, "scale", scale);
    X3DXmlHelper::getVector2DAttribute(node, "translation", translation);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_TextureTransform, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementTextureTransform(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        ((X3DNodeElementTextureTransform *)ne)->Center = center;
        ((X3DNodeElementTextureTransform *)ne)->Rotation = rotation;
        ((X3DNodeElementTextureTransform *)ne)->Scale = scale;
        ((X3DNodeElementTextureTransform *)ne)->Translation = translation;
        // check for X3DMetadataObject childs.
        if (!isNodeEmpty(node))
            childrenReadMetadata(node, ne, "TextureTransform");
        else
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
