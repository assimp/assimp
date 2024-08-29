/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

#ifndef ASSIMP_BUILD_NO_AMF_IMPORTER

// Header files, Assimp.
#include "AMFImporter.hpp"

#include <assimp/DefaultIOSystem.h>
#include <assimp/fast_atof.h>
#include <assimp/StringUtils.h>

// Header files, stdlib.
#include <memory>

namespace Assimp {

static constexpr aiImporterDesc Description = {
    "Additive manufacturing file format(AMF) Importer",
    "smalcom",
    "",
    "See documentation in source code. Chapter: Limitations.",
    aiImporterFlags_SupportTextFlavour | aiImporterFlags_LimitedSupport | aiImporterFlags_Experimental,
    0,
    0,
    0,
    0,
    "amf"
};

void AMFImporter::Clear() {
    mNodeElement_Cur = nullptr;
    mUnit.clear();
    mMaterial_Converted.clear();
    mTexture_Converted.clear();
    // Delete all elements
    if (!mNodeElement_List.empty()) {
        for (AMFNodeElementBase *ne : mNodeElement_List) {
            delete ne;
        }

        mNodeElement_List.clear();
    }
}

AMFImporter::AMFImporter() AI_NO_EXCEPT :
        mNodeElement_Cur(nullptr),
        mXmlParser(nullptr) {
    // empty
}

AMFImporter::~AMFImporter() {
    delete mXmlParser;
    // Clear() is accounting if data already is deleted. So, just check again if all data is deleted.
    Clear();
}

/*********************************************************************************************************************************************/
/************************************************************ Functions: find set ************************************************************/
/*********************************************************************************************************************************************/

bool AMFImporter::Find_NodeElement(const std::string &pID, const AMFNodeElementBase::EType pType, AMFNodeElementBase **pNodeElement) const {
    for (AMFNodeElementBase *ne : mNodeElement_List) {
        if ((ne->ID == pID) && (ne->Type == pType)) {
            if (pNodeElement != nullptr) {
                *pNodeElement = ne;
            }

            return true;
        }
    } // for(CAMFImporter_NodeElement* ne: mNodeElement_List)

    return false;
}

bool AMFImporter::Find_ConvertedNode(const std::string &pID, NodeArray &nodeArray, aiNode **pNode) const {
    aiString node_name(pID.c_str());
    for (aiNode *node : nodeArray) {
        if (node->mName == node_name) {
            if (pNode != nullptr) {
                *pNode = node;
            }

            return true;
        }
    } // for(aiNode* node: pNodeList)

    return false;
}

bool AMFImporter::Find_ConvertedMaterial(const std::string &pID, const SPP_Material **pConvertedMaterial) const {
    for (const SPP_Material &mat : mMaterial_Converted) {
        if (mat.ID == pID) {
            if (pConvertedMaterial != nullptr) {
                *pConvertedMaterial = &mat;
            }

            return true;
        }
    } // for(const SPP_Material& mat: mMaterial_Converted)

    return false;
}

/*********************************************************************************************************************************************/
/************************************************************ Functions: throw set ***********************************************************/
/*********************************************************************************************************************************************/

void AMFImporter::Throw_CloseNotFound(const std::string &nodeName) {
    throw DeadlyImportError("Close tag for node <" + nodeName + "> not found. Seems file is corrupt.");
}

void AMFImporter::Throw_IncorrectAttr(const std::string &nodeName, const std::string &attrName) {
    throw DeadlyImportError("Node <" + nodeName + "> has incorrect attribute \"" + attrName + "\".");
}

void AMFImporter::Throw_IncorrectAttrValue(const std::string &nodeName, const std::string &attrName) {
    throw DeadlyImportError("Attribute \"" + attrName + "\" in node <" + nodeName + "> has incorrect value.");
}

void AMFImporter::Throw_MoreThanOnceDefined(const std::string &nodeName, const std::string &pNodeType, const std::string &pDescription) {
    throw DeadlyImportError("\"" + pNodeType + "\" node can be used only once in " + nodeName + ". Description: " + pDescription);
}

void AMFImporter::Throw_ID_NotFound(const std::string &pID) const {
    throw DeadlyImportError("Not found node with name \"", pID, "\".");
}

/*********************************************************************************************************************************************/
/************************************************************* Functions: XML set ************************************************************/
/*********************************************************************************************************************************************/

void AMFImporter::XML_CheckNode_MustHaveChildren(pugi::xml_node &node) {
    if (node.children().begin() == node.children().end()) {
        throw DeadlyImportError(std::string("Node <") + node.name() + "> must have children.");
    }
}

bool AMFImporter::XML_SearchNode(const std::string &nodeName) {
    return nullptr != mXmlParser->findNode(nodeName);
}

static bool ParseHelper_Decode_Base64_IsBase64(const char pChar) {
    return (isalnum((unsigned char)pChar) || (pChar == '+') || (pChar == '/'));
}

void AMFImporter::ParseHelper_Decode_Base64(const std::string &pInputBase64, std::vector<uint8_t> &pOutputData) const {
    // With help from
    // René Nyffenegger http://www.adp-gmbh.ch/cpp/common/base64.html
    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    uint8_t tidx = 0;
    uint8_t arr4[4], arr3[3];

    // check input data
    if (pInputBase64.size() % 4) {
        throw DeadlyImportError("Base64-encoded data must have size multiply of four.");
    }

    // prepare output place
    pOutputData.clear();
    pOutputData.reserve(pInputBase64.size() / 4 * 3);

    for (size_t in_len = pInputBase64.size(), in_idx = 0; (in_len > 0) && (pInputBase64[in_idx] != '='); in_len--) {
        if (ParseHelper_Decode_Base64_IsBase64(pInputBase64[in_idx])) {
            arr4[tidx++] = pInputBase64[in_idx++];
            if (tidx == 4) {
                for (tidx = 0; tidx < 4; tidx++)
                    arr4[tidx] = (uint8_t)base64_chars.find(arr4[tidx]);

                arr3[0] = (arr4[0] << 2) + ((arr4[1] & 0x30) >> 4);
                arr3[1] = ((arr4[1] & 0x0F) << 4) + ((arr4[2] & 0x3C) >> 2);
                arr3[2] = ((arr4[2] & 0x03) << 6) + arr4[3];
                for (tidx = 0; tidx < 3; tidx++)
                    pOutputData.push_back(arr3[tidx]);

                tidx = 0;
            } // if(tidx == 4)
        } // if(ParseHelper_Decode_Base64_IsBase64(pInputBase64[in_idx]))
        else {
            in_idx++;
        } // if(ParseHelper_Decode_Base64_IsBase64(pInputBase64[in_idx])) else
    }

    if (tidx) {
        for (uint8_t i = tidx; i < 4; i++)
            arr4[i] = 0;
        for (uint8_t i = 0; i < 4; i++)
            arr4[i] = (uint8_t)(base64_chars.find(arr4[i]));

        arr3[0] = (arr4[0] << 2) + ((arr4[1] & 0x30) >> 4);
        arr3[1] = ((arr4[1] & 0x0F) << 4) + ((arr4[2] & 0x3C) >> 2);
        arr3[2] = ((arr4[2] & 0x03) << 6) + arr4[3];
        for (uint8_t i = 0; i < (tidx - 1); i++)
            pOutputData.push_back(arr3[i]);
    }
}

void AMFImporter::ParseFile(const std::string &pFile, IOSystem *pIOHandler) {
    std::unique_ptr<IOStream> file(pIOHandler->Open(pFile, "rb"));

    // Check whether we can read from the file
    if (file == nullptr) {
        throw DeadlyImportError("Failed to open AMF file ", pFile, ".");
    }

    mXmlParser = new XmlParser();
    if (!mXmlParser->parse(file.get())) {
        delete mXmlParser;
        mXmlParser = nullptr;
        throw DeadlyImportError("Failed to create XML reader for file ", pFile, ".");
    }

    // Start reading, search for root tag <amf>
    if (!mXmlParser->hasNode("amf")) {
        throw DeadlyImportError("Root node \"amf\" not found.");
    }
    ParseNode_Root();
} // namespace Assimp

void AMFImporter::ParseHelper_Node_Enter(AMFNodeElementBase *node) {
    mNodeElement_Cur->Child.push_back(node); // add new element to current element child list.
    mNodeElement_Cur = node;
}

void AMFImporter::ParseHelper_Node_Exit() {
    if (mNodeElement_Cur != nullptr) mNodeElement_Cur = mNodeElement_Cur->Parent;
}

// <amf
// unit="" - The units to be used. May be "inch", "millimeter", "meter", "feet", or "micron".
// version="" - Version of file format.
// >
// </amf>
// Root XML element.
// Multi elements - No.
void AMFImporter::ParseNode_Root() {
    AMFNodeElementBase *ne = nullptr;
    XmlNode *root = mXmlParser->findNode("amf");
    if (nullptr == root) {
        throw DeadlyImportError("Root node \"amf\" not found.");
    }
    XmlNode node = *root;
    mUnit = ai_tolower(std::string(node.attribute("unit").as_string()));

    mVersion = node.attribute("version").as_string();

    // Read attributes for node <amf>.
    // Check attributes
    if (!mUnit.empty()) {
        if ((mUnit != "inch") && (mUnit != "millimeters") && (mUnit != "millimeter") && (mUnit != "meter") && (mUnit != "feet") && (mUnit != "micron")) {
            Throw_IncorrectAttrValue("unit", mUnit);
        }
    }

    // create root node element.
    ne = new AMFRoot(nullptr);

    mNodeElement_Cur = ne; // set first "current" element
    // and assign attribute's values
    ((AMFRoot *)ne)->Unit = mUnit;
    ((AMFRoot *)ne)->Version = mVersion;

    // Check for child nodes
    for (XmlNode &currentNode : node.children() ) {
        const std::string currentName = currentNode.name();
        if (currentName == "object") {
            ParseNode_Object(currentNode);
        } else if (currentName == "material") {
            ParseNode_Material(currentNode);
        } else if (currentName == "texture") {
            ParseNode_Texture(currentNode);
        } else if (currentName == "constellation") {
            ParseNode_Constellation(currentNode);
        } else if (currentName == "metadata") {
            ParseNode_Metadata(currentNode);
        }
        mNodeElement_Cur = ne;
    }
    mNodeElement_Cur = ne; // force restore "current" element
    mNodeElement_List.push_back(ne); // add to node element list because its a new object in graph.
}

// <constellation
// id="" - The Object ID of the new constellation being defined.
// >
// </constellation>
// A collection of objects or constellations with specific relative locations.
// Multi elements - Yes.
// Parent element - <amf>.
void AMFImporter::ParseNode_Constellation(XmlNode &node) {
    std::string id;
    id = node.attribute("id").as_string();

    // create and if needed - define new grouping object.
    AMFNodeElementBase *ne = new AMFConstellation(mNodeElement_Cur);

    AMFConstellation &als = *((AMFConstellation *)ne); // alias for convenience

    if (!id.empty()) {
        als.ID = id;
    }

    // Check for child nodes
    if (!node.empty()) {
        ParseHelper_Node_Enter(ne);
        for (XmlNode currentNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
            std::string name = currentNode.name();
            if (name == "instance") {
                ParseNode_Instance(currentNode);
            } else if (name == "metadata") {
                ParseNode_Metadata(currentNode);
            }
        }
        ParseHelper_Node_Exit();
    } else {
        mNodeElement_Cur->Child.push_back(ne);
    }
    mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

// <instance
// objectid="" - The Object ID of the new constellation being defined.
// >
// </instance>
// A collection of objects or constellations with specific relative locations.
// Multi elements - Yes.
// Parent element - <amf>.
void AMFImporter::ParseNode_Instance(XmlNode &node) {
    AMFNodeElementBase *ne(nullptr);

    // Read attributes for node <constellation>.
    std::string objectid = node.attribute("objectid").as_string();

    // used object id must be defined, check that.
    if (objectid.empty()) {
        throw DeadlyImportError("\"objectid\" in <instance> must be defined.");
    }
    // create and define new grouping object.
    ne = new AMFInstance(mNodeElement_Cur);
    AMFInstance &als = *((AMFInstance *)ne);
    als.ObjectID = objectid;

    if (!node.empty()) {
        ParseHelper_Node_Enter(ne);
        for (auto &currentNode : node.children()) {
            const std::string &currentName = currentNode.name();
            if (currentName == "deltax") {
                XmlParser::getValueAsReal(currentNode, als.Delta.x);
            } else if (currentName == "deltay") {
                XmlParser::getValueAsReal(currentNode, als.Delta.y);
            } else if (currentName == "deltaz") {
                XmlParser::getValueAsReal(currentNode, als.Delta.z);
            } else if (currentName == "rx") {
                XmlParser::getValueAsReal(currentNode, als.Delta.x);
            } else if (currentName == "ry") {
                XmlParser::getValueAsReal(currentNode, als.Delta.y);
            } else if (currentName == "rz") {
                XmlParser::getValueAsReal(currentNode, als.Delta.z);
            }
        }
        ParseHelper_Node_Exit();
    } else {
        mNodeElement_Cur->Child.push_back(ne);
    }

    mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

// <object
// id="" - A unique ObjectID for the new object being defined.
// >
// </object>
// An object definition.
// Multi elements - Yes.
// Parent element - <amf>.
void AMFImporter::ParseNode_Object(XmlNode &node) {
    AMFNodeElementBase *ne = nullptr;

    // Read attributes for node <object>.
    std::string id = node.attribute("id").as_string();

    // create and if needed - define new geometry object.
    ne = new AMFObject(mNodeElement_Cur);

    AMFObject &als = *((AMFObject *)ne); // alias for convenience

    if (!id.empty()) {
        als.ID = id;
    }

    // Check for child nodes
    if (!node.empty()) {
        ParseHelper_Node_Enter(ne);
        for (auto &currentNode : node.children()) {
            const std::string &currentName = currentNode.name();
            if (currentName == "color") {
                ParseNode_Color(currentNode);
            } else if (currentName == "mesh") {
                ParseNode_Mesh(currentNode);
            } else if (currentName == "metadata") {
                ParseNode_Metadata(currentNode);
            }
        }
        ParseHelper_Node_Exit();
    } else {
        mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
    }

    mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

// <metadata
// type="" - The type of the attribute.
// >
// </metadata>
// Specify additional information about an entity.
// Multi elements - Yes.
// Parent element - <amf>, <object>, <volume>, <material>, <vertex>.
//
// Reserved types are:
// "Name" - The alphanumeric label of the entity, to be used by the interpreter if interacting with the user.
// "Description" - A description of the content of the entity
// "URL" - A link to an external resource relating to the entity
// "Author" - Specifies the name(s) of the author(s) of the entity
// "Company" - Specifying the company generating the entity
// "CAD" - specifies the name of the originating CAD software and version
// "Revision" - specifies the revision of the entity
// "Tolerance" - specifies the desired manufacturing tolerance of the entity in entity's unit system
// "Volume" - specifies the total volume of the entity, in the entity's unit system, to be used for verification (object and volume only)
void AMFImporter::ParseNode_Metadata(XmlNode &node) {
    AMFNodeElementBase *ne = nullptr;

    std::string type = node.attribute("type").as_string(), value;
    XmlParser::getValueAsString(node, value);

    // read attribute
    ne = new AMFMetadata(mNodeElement_Cur);
    ((AMFMetadata *)ne)->Type = type;
    ((AMFMetadata *)ne)->Value = value;
    mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
    mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

bool AMFImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler, bool /*pCheckSig*/) const {
    static const char *tokens[] = { "<amf" };
    return SearchFileHeaderForToken(pIOHandler, pFile, tokens, AI_COUNT_OF(tokens));
}

const aiImporterDesc *AMFImporter::GetInfo() const {
    return &Description;
}

void AMFImporter::InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) {
    Clear(); // delete old graph.
    ParseFile(pFile, pIOHandler);
    Postprocess_BuildScene(pScene);
    // scene graph is ready, exit.
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_AMF_IMPORTER
