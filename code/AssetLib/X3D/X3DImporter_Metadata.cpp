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
/// \file   X3DImporter_Metadata.cpp
/// \brief  Parsing data from nodes of "Metadata" set of X3D.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DImporter.hpp"
#include "X3DImporter_Macro.hpp"
#include "X3DXmlHelper.h"

namespace Assimp {

bool X3DImporter::checkForMetadataNode(XmlNode &node) {
    const std::string &name = node.name();
    if (name == "MetadataBoolean") {
        readMetadataBoolean(node);
    } else if (name == "MetadataDouble") {
        readMetadataDouble(node);
    } else if (name == "MetadataFloat") {
        readMetadataFloat(node);
    } else if (name == "MetadataInteger") {
        readMetadataInteger(node);
    } else if (name == "MetadataSet") {
        readMetadataSet(node);
    } else if (name == "MetadataString") {
        readMetadataString(node);
    } else
        return false;
    return true;
}

void X3DImporter::childrenReadMetadata(XmlNode &node, X3DNodeElementBase *pParentElement, const std::string &pNodeName) {
    ParseHelper_Node_Enter(pParentElement);
    for (auto childNode : node.children()) {
        if (!checkForMetadataNode(childNode)) skipUnsupportedNode(pNodeName, childNode);
    }
    ParseHelper_Node_Exit();
}

/// \def MACRO_METADATA_FINDCREATE(pDEF_Var, pUSE_Var, pReference, pValue, pNE, pMetaName)
/// Find element by "USE" or create new one.
/// \param [in] pNode - pugi xml node to read.
/// \param [in] pDEF_Var - variable name with "DEF" value.
/// \param [in] pUSE_Var - variable name with "USE" value.
/// \param [in] pReference - variable name with "reference" value.
/// \param [in] pValue - variable name with "value" value.
/// \param [in, out] pNE - pointer to node element.
/// \param [in] pMetaClass - Class of node.
/// \param [in] pMetaName - Name of node.
/// \param [in] pType - type of element to find.
#define MACRO_METADATA_FINDCREATE(pNode, pDEF_Var, pUSE_Var, pReference, pValue, pNE, pMetaClass, pMetaName, pType)                            \
    /* if "USE" defined then find already defined element. */                                                                                  \
    if (!pUSE_Var.empty()) {                                                                                                                   \
        ne = MACRO_USE_CHECKANDAPPLY(pNode, pDEF_Var, pUSE_Var, pType, pNE);                                                                        \
    } else {                                                                                                                                   \
        pNE = new pMetaClass(mNodeElementCur);                                                                                                 \
        if (!pDEF_Var.empty()) pNE->ID = pDEF_Var;                                                                                             \
                                                                                                                                               \
        ((pMetaClass *)pNE)->Reference = pReference;                                                                                           \
        ((pMetaClass *)pNE)->Value = pValue;                                                                                                   \
        /* also metadata node can contain childs */                                                                                            \
        if (!isNodeEmpty(pNode))                                                                                                               \
            childrenReadMetadata(pNode, pNE, pMetaName); /* in that case node element will be added to child elements list of current node. */ \
        else                                                                                                                                   \
            mNodeElementCur->Children.push_back(pNE); /* else - add element to child list manually */                                          \
                                                                                                                                               \
        NodeElement_List.push_back(pNE); /* add new element to elements list. */                                                               \
    } /* if(!pUSE_Var.empty()) else */                                                                                                         \
                                                                                                                                               \
    do {                                                                                                                                       \
    } while (false)

// <MetadataBoolean
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString [inputOutput]
// reference="" SFString [inputOutput]
// value=""     MFBool   [inputOutput]
// />
void X3DImporter::readMetadataBoolean(XmlNode &node) {
    std::string def, use;
    std::string name, reference;
    std::vector<bool> value;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getStdStrAttribute(node, "name", name);
    XmlParser::getStdStrAttribute(node, "reference", reference);
    X3DXmlHelper::getBooleanArrayAttribute(node, "value", value);

    MACRO_METADATA_FINDCREATE(node, def, use, reference, value, ne, X3DNodeElementMetaBoolean, "MetadataBoolean", ENET_MetaBoolean);
}

// <MetadataDouble
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString [inputOutput]
// reference="" SFString [inputOutput]
// value=""     MFDouble [inputOutput]
// />
void X3DImporter::readMetadataDouble(XmlNode &node) {
    std::string def, use;
    std::string name, reference;
    std::vector<double> value;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getStdStrAttribute(node, "name", name);
    XmlParser::getStdStrAttribute(node, "reference", reference);
    X3DXmlHelper::getDoubleArrayAttribute(node, "value", value);

    MACRO_METADATA_FINDCREATE(node, def, use, reference, value, ne, X3DNodeElementMetaDouble, "MetadataDouble", ENET_MetaDouble);
}

// <MetadataFloat
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString [inputOutput]
// reference="" SFString [inputOutput]
// value=""     MFFloat  [inputOutput]
// />
void X3DImporter::readMetadataFloat(XmlNode &node) {
    std::string def, use;
    std::string name, reference;
    std::vector<float> value;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getStdStrAttribute(node, "name", name);
    XmlParser::getStdStrAttribute(node, "reference", reference);
    X3DXmlHelper::getFloatArrayAttribute(node, "value", value);

    MACRO_METADATA_FINDCREATE(node, def, use, reference, value, ne, X3DNodeElementMetaFloat, "MetadataFloat", ENET_MetaFloat);
}

// <MetadataInteger
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString  [inputOutput]
// reference="" SFString  [inputOutput]
// value=""     MFInteger [inputOutput]
// />
void X3DImporter::readMetadataInteger(XmlNode &node) {
    std::string def, use;
    std::string name, reference;
    std::vector<int32_t> value;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getStdStrAttribute(node, "name", name);
    XmlParser::getStdStrAttribute(node, "reference", reference);
    X3DXmlHelper::getInt32ArrayAttribute(node, "value", value);

    MACRO_METADATA_FINDCREATE(node, def, use, reference, value, ne, X3DNodeElementMetaInt, "MetadataInteger", ENET_MetaInteger);
}

// <MetadataSet
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString [inputOutput]
// reference="" SFString [inputOutput]
// />
void X3DImporter::readMetadataSet(XmlNode &node) {
    std::string def, use;
    std::string name, reference;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getStdStrAttribute(node, "name", name);
    XmlParser::getStdStrAttribute(node, "reference", reference);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_MetaSet, ne);
    } else {
        ne = new X3DNodeElementMetaSet(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        ((X3DNodeElementMetaSet *)ne)->Reference = reference;
        // also metadata node can contain children
        if (!isNodeEmpty(node))
            childrenReadMetadata(node, ne, "MetadataSet");
        else
            mNodeElementCur->Children.push_back(ne); // made object as child to current element

        NodeElement_List.push_back(ne); // add new element to elements list.
    } // if(!use.empty()) else
}

// <MetadataString
// DEF=""       ID
// USE=""       IDREF
// name=""      SFString [inputOutput]
// reference="" SFString [inputOutput]
// value=""     MFString [inputOutput]
// />
void X3DImporter::readMetadataString(XmlNode &node) {
    std::string def, use;
    std::string name, reference;
    std::vector<std::string> value;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getStdStrAttribute(node, "name", name);
    XmlParser::getStdStrAttribute(node, "reference", reference);
    X3DXmlHelper::getStringArrayAttribute(node, "value", value);

    MACRO_METADATA_FINDCREATE(node, def, use, reference, value, ne, X3DNodeElementMetaString, "MetadataString", ENET_MetaString);
}

}// namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
