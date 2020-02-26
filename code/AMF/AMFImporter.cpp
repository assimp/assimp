/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

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

/// \file AMFImporter.cpp
/// \brief AMF-format files importer for Assimp: main algorithm implementation.
/// \date 2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_AMF_IMPORTER

// Header files, Assimp.
#include "AMFImporter.hpp"
#include "AMFImporter_Macro.hpp"

#include <assimp/DefaultIOSystem.h>
#include <assimp/fast_atof.h>

#include <memory>

namespace Assimp {

/// \var aiImporterDesc AMFImporter::Description
/// Constant which hold importer description
const aiImporterDesc AMFImporter::Description = {
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

AMFImporter::~AMFImporter() {
	if (mXmlParser != nullptr) {
		delete mXmlParser;
		mXmlParser = nullptr;
	}

	// Clear() is accounting if data already is deleted. So, just check again if all data is deleted.
	Clear();
}

void AMFImporter::ParseHelper_Decode_Base64(const std::string &pInputBase64, std::vector<uint8_t> &pOutputData) const {
	// With help from
	// René Nyffenegger http://www.adp-gmbh.ch/cpp/common/base64.html
	const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	uint8_t tidx = 0;
	uint8_t arr4[4], arr3[3];

	// check input data
	if (pInputBase64.size() % 4) throw DeadlyImportError("Base64-encoded data must have size multiply of four.");
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
	if (file.get() == nullptr) {
		throw DeadlyImportError("Failed to open AMF file " + pFile + ".");
	}

	mXmlParser = new XmlParser;
	XmlNode *root = mXmlParser->parse(file.get());
	if (nullptr == root) {
		throw DeadlyImportError("Failed to create XML reader for file" + pFile + ".");
	}

	// start reading
	// search for root tag <amf>

	if (!root->find_child("amf")) {
		throw DeadlyImportError("Root node \"amf\" not found.");
	}

	ParseNode_Root(*root);

	delete mXmlParser;
	mXmlParser = nullptr;
}

// <amf
// unit="" - The units to be used. May be "inch", "millimeter", "meter", "feet", or "micron".
// version="" - Version of file format.
// >
// </amf>
// Root XML element.
// Multi elements - No.
void AMFImporter::ParseNode_Root(XmlNode &root) {
	std::string unit, version;
	AMFNodeElementBase *ne(nullptr);

	// Read attributes for node <amf>.
	for (pugi::xml_attribute_iterator ait = root.attributes_begin(); ait != root.attributes_end(); ++ait) {
		if (ait->name() == "unit") {
			unit = ait->as_string();
		} else if (ait->name() == "version") {
			version = ait->as_string();
		}
	}

	// Check attributes
	if (!mUnit.empty()) {
		if ((mUnit != "inch") && (mUnit != "millimeter") && (mUnit != "meter") && (mUnit != "feet") && (mUnit != "micron")) {
			throw DeadlyImportError("Root node does not contain any units.");
		}
	}

	// create root node element.
	ne = new AMFRoot(nullptr);

	// set first "current" element
	mNodeElement_Cur = ne;

	// and assign attributes values
	((AMFRoot *)ne)->Unit = unit;
	((AMFRoot *)ne)->Version = version;

	// Check for child nodes
	for (pugi::xml_node child : node->children()) {
		if (child.name() == "object") {
			ParseNode_Object(child);
		} else if (child.name() == "material") {
			ParseNode_Material(child);
		} else if (child.name() == "texture") {
			ParseNode_Texture(child);
		} else if (child.name() == "constellation") {
			ParseNode_Constellation(child);
		} else if (child.name() == "metadata") {
			ParseNode_Metadata(child);
		}
	}
	mNodeElement_List.push_back(ne); // add to node element list because its a new object in graph.
}

// <constellation
// id="" - The Object ID of the new constellation being defined.
// >
// </constellation>
// A collection of objects or constellations with specific relative locations.
// Multi elements - Yes.
// Parent element - <amf>.
void AMFImporter::ParseNode_Constellation(XmlNode &root) {
	std::string id = root.attribute("id").as_string();

	// create and if needed - define new grouping object.
	AMFNodeElementBase *ne = new AMFConstellation(mNodeElement_Cur);

	AMFConstellation &als = *((AMFConstellation *)ne); // alias for convenience

	for (pugi::xml_node &child : root.children()) {
		if (child.name() == "instance") {
			ParseNode_Instance(child);
		} else if (child.name() == "metadata") {
			ParseNode_Metadata(child);
		}
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
void AMFImporter::ParseNode_Instance(XmlNode &root) {
	std::string objectid = root.attribute("objectid").as_string();

        // used object id must be defined, check that.
	if (objectid.empty()) {
		throw DeadlyImportError("\"objectid\" in <instance> must be defined.");
	}
	// create and define new grouping object.
	AMFNodeElementBase *ne = new AMFInstance(mNodeElement_Cur);

	AMFInstance &als = *((AMFInstance *)ne); // alias for convenience

	als.ObjectID = objectid;
	// Check for child nodes
	if (!mXmlParser->isEmptyElement()) {
		bool read_flag[6] = { false, false, false, false, false, false };

		als.Delta.Set(0, 0, 0);
		als.Rotation.Set(0, 0, 0);
		ParseHelper_Node_Enter(ne);
		MACRO_NODECHECK_LOOPBEGIN("instance");
		MACRO_NODECHECK_READCOMP_F("deltax", read_flag[0], als.Delta.x);
		MACRO_NODECHECK_READCOMP_F("deltay", read_flag[1], als.Delta.y);
		MACRO_NODECHECK_READCOMP_F("deltaz", read_flag[2], als.Delta.z);
		MACRO_NODECHECK_READCOMP_F("rx", read_flag[3], als.Rotation.x);
		MACRO_NODECHECK_READCOMP_F("ry", read_flag[4], als.Rotation.y);
		MACRO_NODECHECK_READCOMP_F("rz", read_flag[5], als.Rotation.z);
		MACRO_NODECHECK_LOOPEND("instance");
		ParseHelper_Node_Exit();
		// also convert degrees to radians.
		als.Rotation.x = AI_MATH_PI_F * als.Rotation.x / 180.0f;
		als.Rotation.y = AI_MATH_PI_F * als.Rotation.y / 180.0f;
		als.Rotation.z = AI_MATH_PI_F * als.Rotation.z / 180.0f;
	} // if(!mReader->isEmptyElement())
	else {
		mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
	} // if(!mReader->isEmptyElement()) else

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

	std::string id;
	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		if (ait->name() == "id") {
			id = ait->as_string();
		}
	}
	// Read attributes for node <object>.

	// create and if needed - define new geometry object.
	AMFNodeElementBase *ne = new AMFObject(mNodeElement_Cur);

	AMFObject &als = *((AMFObject *)ne); // alias for convenience

	if (!id.empty()) {
		als.ID = id;
	}

	// Check for child nodes
	for (pugi::xml_node_iterator it = node.children().begin(); it != node.children->end(); ++it) {
		bool col_read = false;
		if (it->name() == "mesh") {
			ParseNode_Mesh(*it);
		} else if (it->name() == "metadata") {
			ParseNode_Metadata(*it);
		} else if (it->name() == "color") {
			ParseNode_Color(*it);
		}
	}

	mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
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
void AMFImporter::ParseNode_Metadata() {
	std::string type, value;
	AMFNodeElementBase *ne(nullptr);

	// read attribute
	MACRO_ATTRREAD_LOOPBEG;
	MACRO_ATTRREAD_CHECK_RET("type", type, mXmlParser->getAttributeValue);
	MACRO_ATTRREAD_LOOPEND;
	// and value of node.
	value = mXmlParser->getNodeData();
	// Create node element and assign read data.
	ne = new AMFMetadata(mNodeElement_Cur);
	((AMFMetadata *)ne)->Type = type;
	((AMFMetadata *)ne)->Value = value;
	mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
	mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

/*********************************************************************************************************************************************/
/******************************************************** Functions: BaseImporter set ********************************************************/
/*********************************************************************************************************************************************/

bool AMFImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler, bool pCheckSig) const {
	const std::string extension = GetExtension(pFile);

	if (extension == "amf") {
		return true;
	}

	if (!extension.length() || pCheckSig) {
		const char *tokens[] = { "<amf" };

		return SearchFileHeaderForToken(pIOHandler, pFile, tokens, 1);
	}

	return false;
}

void AMFImporter::GetExtensionList(std::set<std::string> &extensionList) {
	extensionList.insert("amf");
}

const aiImporterDesc *AMFImporter::GetInfo() const {
	return &Description;
}

void AMFImporter::InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) {
	Clear();

	ParseFile(pFile, pIOHandler);

	Postprocess_BuildScene(pScene);
}

void AMFImporter::ParseNode_Mesh(XmlNode &node) {
	AMFNodeElementBase *ne;

	if (node.empty()) {
		return;
	}

	for (pugi::xml_node &child : node.children()) {
		if (child.name() == "vertices") {
			ParseNode_Vertices(child);
		}
	}
	// create new mesh object.
	ne = new AMFMesh(mNodeElement_Cur);
	// Check for child nodes
	if (!mXmlParser->isEmptyElement()) {
		bool vert_read = false;

		ParseHelper_Node_Enter(ne);
		MACRO_NODECHECK_LOOPBEGIN("mesh");
		if (XML_CheckNode_NameEqual("vertices")) {
			// Check if data already defined.
			if (vert_read) Throw_MoreThanOnceDefined("vertices", "Only one vertices set can be defined for <mesh>.");
			// read data and set flag about it
			vert_read = true;

			continue;
		}

		if (XML_CheckNode_NameEqual("volume")) {
			ParseNode_Volume();
			continue;
		}
		MACRO_NODECHECK_LOOPEND("mesh");
		ParseHelper_Node_Exit();
	} // if(!mReader->isEmptyElement())
	else {
		mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
	} // if(!mReader->isEmptyElement()) else

	mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

// <vertices>
// </vertices>
// The list of vertices to be used in defining triangles.
// Multi elements - No.
// Parent element - <mesh>.
void AMFImporter::ParseNode_Vertices(XmlNode &node) {
	AMFNodeElementBase *ne = new AMFVertices(mNodeElement_Cur);

	for (pugi::xml_node &child : node.children()) {
		if (child.name() == "vertices") {
			ParseNode_Vertex(child);
		}
	}
	// Check for child nodes

	mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

// <vertex>
// </vertex>
// A vertex to be referenced in triangles.
// Multi elements - Yes.
// Parent element - <vertices>.
void AMFImporter::ParseNode_Vertex() {
	AMFNodeElementBase *ne;

	// create new mesh object.
	ne = new AMFVertex(mNodeElement_Cur);
	// Check for child nodes
	if (!mXmlParser->isEmptyElement()) {
		bool col_read = false;
		bool coord_read = false;

		ParseHelper_Node_Enter(ne);
		MACRO_NODECHECK_LOOPBEGIN("vertex");
		if (XML_CheckNode_NameEqual("color")) {
			// Check if data already defined.
			if (col_read) Throw_MoreThanOnceDefined("color", "Only one color can be defined for <vertex>.");
			// read data and set flag about it
			ParseNode_Color();
			col_read = true;

			continue;
		}

		if (XML_CheckNode_NameEqual("coordinates")) {
			// Check if data already defined.
			if (coord_read) Throw_MoreThanOnceDefined("coordinates", "Only one coordinates set can be defined for <vertex>.");
			// read data and set flag about it
			ParseNode_Coordinates();
			coord_read = true;

			continue;
		}

		if (XML_CheckNode_NameEqual("metadata")) {
			ParseNode_Metadata();
			continue;
		}
		MACRO_NODECHECK_LOOPEND("vertex");
		ParseHelper_Node_Exit();
	} // if(!mReader->isEmptyElement())
	else {
		mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
	} // if(!mReader->isEmptyElement()) else

	mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

// <coordinates>
// </coordinates>
// Specifies the 3D location of this vertex.
// Multi elements - No.
// Parent element - <vertex>.
//
// Children elements:
//   <x>, <y>, <z>
//   Multi elements - No.
//   X, Y, or Z coordinate, respectively, of a vertex position in space.
void AMFImporter::ParseNode_Coordinates() {
	AMFNodeElementBase *ne;

	// create new color object.
	ne = new AMFCoordinates(mNodeElement_Cur);

	AMFCoordinates &als = *((AMFCoordinates *)ne); // alias for convenience

	// Check for child nodes
	if (!mXmlParser->isEmptyElement()) {
		bool read_flag[3] = { false, false, false };

		ParseHelper_Node_Enter(ne);
		MACRO_NODECHECK_LOOPBEGIN("coordinates");
		MACRO_NODECHECK_READCOMP_F("x", read_flag[0], als.Coordinate.x);
		MACRO_NODECHECK_READCOMP_F("y", read_flag[1], als.Coordinate.y);
		MACRO_NODECHECK_READCOMP_F("z", read_flag[2], als.Coordinate.z);
		MACRO_NODECHECK_LOOPEND("coordinates");
		ParseHelper_Node_Exit();
		// check that all components was defined
		if ((read_flag[0] && read_flag[1] && read_flag[2]) == 0) throw DeadlyImportError("Not all coordinate's components are defined.");

	} // if(!mReader->isEmptyElement())
	else {
		mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
	} // if(!mReader->isEmptyElement()) else

	mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

// <volume
// materialid="" - Which material to use.
// type=""       - What this volume describes can be “region” or “support”. If none specified, “object” is assumed. If support, then the geometric
//                 requirements 1-8 listed in section 5 do not need to be maintained.
// >
// </volume>
// Defines a volume from the established vertex list.
// Multi elements - Yes.
// Parent element - <mesh>.
void AMFImporter::ParseNode_Volume() {
	std::string materialid;
	std::string type;
	AMFNodeElementBase *ne;

	// Read attributes for node <color>.
	MACRO_ATTRREAD_LOOPBEG;
	MACRO_ATTRREAD_CHECK_RET("materialid", materialid, mXmlParser->getAttributeValue);
	MACRO_ATTRREAD_CHECK_RET("type", type, mXmlParser->getAttributeValue);
	MACRO_ATTRREAD_LOOPEND;

	// create new object.
	ne = new AMFVolume(mNodeElement_Cur);
	// and assign read data
	((AMFVolume *)ne)->MaterialID = materialid;
	((AMFVolume *)ne)->Type = type;
	// Check for child nodes
	if (!mXmlParser->isEmptyElement()) {
		bool col_read = false;

		ParseHelper_Node_Enter(ne);
		MACRO_NODECHECK_LOOPBEGIN("volume");
		if (XML_CheckNode_NameEqual("color")) {
			// Check if data already defined.
			if (col_read) Throw_MoreThanOnceDefined("color", "Only one color can be defined for <volume>.");
			// read data and set flag about it
			ParseNode_Color();
			col_read = true;

			continue;
		}

		if (XML_CheckNode_NameEqual("triangle")) {
			ParseNode_Triangle();
			continue;
		}
		if (XML_CheckNode_NameEqual("metadata")) {
			ParseNode_Metadata();
			continue;
		}
		MACRO_NODECHECK_LOOPEND("volume");
		ParseHelper_Node_Exit();
	} // if(!mReader->isEmptyElement())
	else {
		mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
	} // if(!mReader->isEmptyElement()) else

	mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

// <triangle>
// </triangle>
// Defines a 3D triangle from three vertices, according to the right-hand rule (counter-clockwise when looking from the outside).
// Multi elements - Yes.
// Parent element - <volume>.
//
// Children elements:
//   <v1>, <v2>, <v3>
//   Multi elements - No.
//   Index of the desired vertices in a triangle or edge.
void AMFImporter::ParseNode_Triangle() {
	AMFNodeElementBase *ne;

	// create new color object.
	ne = new AMFTriangle(mNodeElement_Cur);

	AMFTriangle &als = *((AMFTriangle *)ne); // alias for convenience

	// Check for child nodes
	if (!mXmlParser->isEmptyElement()) {
		bool col_read = false, tex_read = false;
		bool read_flag[3] = { false, false, false };

		ParseHelper_Node_Enter(ne);
		MACRO_NODECHECK_LOOPBEGIN("triangle");
		if (XML_CheckNode_NameEqual("color")) {
			// Check if data already defined.
			if (col_read) Throw_MoreThanOnceDefined("color", "Only one color can be defined for <triangle>.");
			// read data and set flag about it
			ParseNode_Color();
			col_read = true;

			continue;
		}

		if (XML_CheckNode_NameEqual("texmap")) // new name of node: "texmap".
		{
			// Check if data already defined.
			if (tex_read) Throw_MoreThanOnceDefined("texmap", "Only one texture coordinate can be defined for <triangle>.");
			// read data and set flag about it
			ParseNode_TexMap();
			tex_read = true;

			continue;
		} else if (XML_CheckNode_NameEqual("map")) // old name of node: "map".
		{
			// Check if data already defined.
			if (tex_read) Throw_MoreThanOnceDefined("map", "Only one texture coordinate can be defined for <triangle>.");
			// read data and set flag about it
			ParseNode_TexMap(true);
			tex_read = true;

			continue;
		}

		//		MACRO_NODECHECK_READCOMP_U32("v1", read_flag[0], als.V[0]);
		//		MACRO_NODECHECK_READCOMP_U32("v2", read_flag[1], als.V[1]);
		//		MACRO_NODECHECK_READCOMP_U32("v3", read_flag[2], als.V[2]);
		//		MACRO_NODECHECK_LOOPEND("triangle");
		ParseHelper_Node_Exit();
		// check that all components was defined
		if ((read_flag[0] && read_flag[1] && read_flag[2]) == 0) throw DeadlyImportError("Not all vertices of the triangle are defined.");

	} // if(!mReader->isEmptyElement())
	else {
		mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
	} // if(!mReader->isEmptyElement()) else

	mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_AMF_IMPORTER
