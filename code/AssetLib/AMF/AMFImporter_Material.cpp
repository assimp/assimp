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

/// \file AMFImporter_Material.cpp
/// \brief Parsing data from material nodes.
/// \date 2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_AMF_IMPORTER

#include "AMFImporter.hpp"

namespace Assimp {

// <color
// profile="" - The ICC color space used to interpret the three color channels <r>, <g> and <b>.
// >
// </color>
// A color definition.
// Multi elements - No.
// Parent element - <material>, <object>, <volume>, <vertex>, <triangle>.
//
// "profile" can be one of "sRGB", "AdobeRGB", "Wide-Gamut-RGB", "CIERGB", "CIELAB", or "CIEXYZ".
// Children elements:
//   <r>, <g>, <b>, <a>
//   Multi elements - No.
//   Red, Greed, Blue and Alpha (transparency) component of a color in sRGB space, values ranging from 0 to 1. The
//   values can be specified as constants, or as a formula depending on the coordinates.
void AMFImporter::ParseNode_Color(XmlNode &node) {
    if (node.empty()) {
        return;
    }

    const std::string &profile = node.attribute("profile").as_string();
    bool read_flag[4] = { false, false, false, false };
    AMFNodeElementBase *ne = new AMFColor(mNodeElement_Cur);
    AMFColor &als = *((AMFColor *)ne); // alias for convenience
    ParseHelper_Node_Enter(ne);
    for (pugi::xml_node &child : node.children()) {
        // create new color object.
        als.Profile = profile;

        const std::string &name = child.name();
        if ( name == "r") {
			read_flag[0] = true;
            XmlParser::getValueAsFloat(child, als.Color.r);
        } else if (name == "g") {
			read_flag[1] = true;
            XmlParser::getValueAsFloat(child, als.Color.g);
        } else if (name == "b") {
			read_flag[2] = true;
            XmlParser::getValueAsFloat(child, als.Color.b);
        } else if (name == "a") {
			read_flag[3] = true;
            XmlParser::getValueAsFloat(child, als.Color.a);
        }
        // check if <a> is absent. Then manually add "a == 1".
        if (!read_flag[3]) {
            als.Color.a = 1;
        }
    }
    als.Composed = false;
    mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
    ParseHelper_Node_Exit();
    // check that all components was defined
	if (!(read_flag[0] && read_flag[1] && read_flag[2])) {
		throw DeadlyImportError("Not all color components are defined.");
	}
}

// <material
// id="" - A unique material id. material ID "0" is reserved to denote no material (void) or sacrificial material.
// >
// </material>
// An available material.
// Multi elements - Yes.
// Parent element - <amf>.
void AMFImporter::ParseNode_Material(XmlNode &node) {
    // create new object and assign read data
	std::string id = node.attribute("id").as_string();
	AMFNodeElementBase *ne = new AMFMaterial(mNodeElement_Cur);
	((AMFMaterial*)ne)->ID = id;

    // Check for child nodes
	if (!node.empty()) {
        ParseHelper_Node_Enter(ne);
        for (pugi::xml_node &child : node.children()) {
            const std::string name = child.name();
            if (name == "color") {
				ParseNode_Color(child);
            } else if (name == "metadata") {
				ParseNode_Metadata(child);
			}
		}
        ParseHelper_Node_Exit();
	} else {
		mNodeElement_Cur->Child.push_back(ne);// Add element to child list of current element
	}

	mNodeElement_List.push_back(ne);// and to node element list because its a new object in graph.
}

// <texture
// id=""     - Assigns a unique texture id for the new texture.
// width=""  - Width (horizontal size, x) of the texture, in pixels.
// height="" - Height (lateral size, y) of the texture, in pixels.
// depth=""  - Depth (vertical size, z) of the texture, in pixels.
// type=""   - Encoding of the data in the texture. Currently allowed values are "grayscale" only. In grayscale mode, each pixel is represented by one byte
//   in the range of 0-255. When the texture is referenced using the tex function, these values are converted into a single floating point number in the
//   range of 0-1 (see Annex 2). A full color graphics will typically require three textures, one for each of the color channels. A graphic involving
//   transparency may require a fourth channel.
// tiled=""  - If true then texture repeated when UV-coordinates is greater than 1.
// >
// </triangle>
// Specifies an texture data to be used as a map. Lists a sequence of Base64 values specifying values for pixels from left to right then top to bottom,
// then layer by layer.
// Multi elements - Yes.
// Parent element - <amf>.
void AMFImporter::ParseNode_Texture(XmlNode &node) {
    const std::string id = node.attribute("id").as_string();
	const uint32_t width = node.attribute("width").as_uint();
    const uint32_t height = node.attribute("height").as_uint();
    uint32_t depth = node.attribute("depth").as_uint();
    const std::string type = node.attribute("type").as_string();
	bool tiled = node.attribute("tiled").as_bool();

    if (node.empty()) {
		return;
    }

    // create new texture object.
    AMFNodeElementBase *ne = new AMFTexture(mNodeElement_Cur);

	AMFTexture& als = *((AMFTexture*)ne);// alias for convenience

    std::string enc64_data;
    XmlParser::getValueAsString(node, enc64_data);
    // Check for child nodes

	// check that all components was defined
    if (id.empty()) {
		throw DeadlyImportError("ID for texture must be defined.");
    }
    if (width < 1) {
		throw DeadlyImportError("Invalid width for texture.");
    }
    if (height < 1) {
		throw DeadlyImportError("Invalid height for texture.");
	}
    if (type != "grayscale") {
		throw DeadlyImportError("Invalid type for texture.");
    }
    if (enc64_data.empty()) {
        throw DeadlyImportError("Texture data not defined.");
    }
	// copy data
	als.ID = id;
	als.Width = width;
	als.Height = height;
	als.Depth = depth;
	als.Tiled = tiled;
	ParseHelper_Decode_Base64(enc64_data, als.Data);
    if (depth == 0) {
        depth = (uint32_t)(als.Data.size() / (width * height));
    }
    // check data size
    if ((width * height * depth) != als.Data.size()) {
        throw DeadlyImportError("Texture has incorrect data size.");
    }

	mNodeElement_Cur->Child.push_back(ne);// Add element to child list of current element
	mNodeElement_List.push_back(ne);// and to node element list because its a new object in graph.
}

// <texmap
// rtexid="" - Texture ID for red color component.
// gtexid="" - Texture ID for green color component.
// btexid="" - Texture ID for blue color component.
// atexid="" - Texture ID for alpha color component. Optional.
// >
// </texmap>, old name: <map>
// Specifies texture coordinates for triangle.
// Multi elements - No.
// Parent element - <triangle>.
// Children elements:
//   <utex1>, <utex2>, <utex3>, <vtex1>, <vtex2>, <vtex3>. Old name: <u1>, <u2>, <u3>, <v1>, <v2>, <v3>.
//   Multi elements - No.
//   Texture coordinates for every vertex of triangle.
void AMFImporter::ParseNode_TexMap(XmlNode &node, const bool pUseOldName) {
	// Read attributes for node <color>.
    AMFNodeElementBase *ne = new AMFTexMap(mNodeElement_Cur);
    AMFTexMap &als = *((AMFTexMap *)ne); //
    std::string rtexid, gtexid, btexid, atexid;
    if (!node.empty()) {
        for (pugi::xml_attribute &attr : node.attributes()) {
            const std::string &currentAttr = attr.name();
            if (currentAttr == "rtexid") {
                rtexid = attr.as_string();
            } else if (currentAttr == "gtexid") {
                gtexid = attr.as_string();
            } else if (currentAttr == "btexid") {
                btexid = attr.as_string();
            } else if (currentAttr == "atexid") {
                atexid = attr.as_string();
            }
        }
    }

	// create new texture coordinates object, alias for convenience
	// check data
	if (rtexid.empty() && gtexid.empty() && btexid.empty()) {
		throw DeadlyImportError("ParseNode_TexMap. At least one texture ID must be defined.");
	}

	// Check for children nodes
	if (node.children().begin() == node.children().end()) {
		throw DeadlyImportError("Invalid children definition.");
	}
	// read children nodes
	bool read_flag[6] = { false, false, false, false, false, false };

	if (!pUseOldName) {
        ParseHelper_Node_Enter(ne);
        for ( XmlNode &currentNode : node.children()) {
            const std::string &name = currentNode.name();
            if (name == "utex1") {
				read_flag[0] = true;
                XmlParser::getValueAsFloat(node, als.TextureCoordinate[0].x);
            } else if (name == "utex2") {
				read_flag[1] = true;
                XmlParser::getValueAsFloat(node, als.TextureCoordinate[1].x);
            } else if (name == "utex3") {
				read_flag[2] = true;
                XmlParser::getValueAsFloat(node, als.TextureCoordinate[2].x);
            } else if (name == "vtex1") {
				read_flag[3] = true;
                XmlParser::getValueAsFloat(node, als.TextureCoordinate[0].y);
            } else if (name == "vtex2") {
				read_flag[4] = true;
                XmlParser::getValueAsFloat(node, als.TextureCoordinate[1].y);
            } else if (name == "vtex3") {
				read_flag[5] = true;
                XmlParser::getValueAsFloat(node, als.TextureCoordinate[2].y);
			}
		}
        ParseHelper_Node_Exit();

	} else {
		for (pugi::xml_attribute &attr : node.attributes()) {
            const std::string name = attr.name();
            if (name == "u") {
				read_flag[0] = true;
				als.TextureCoordinate[0].x = attr.as_float();
            } else if (name == "u2") {
				read_flag[1] = true;
				als.TextureCoordinate[1].x = attr.as_float();
            } else if (name == "u3") {
				read_flag[2] = true;
				als.TextureCoordinate[2].x = attr.as_float();
            } else if (name == "v1") {
				read_flag[3] = true;
				als.TextureCoordinate[0].y = attr.as_float();
            } else if (name == "v2") {
				read_flag[4] = true;
				als.TextureCoordinate[1].y = attr.as_float();
            } else if (name == "v3") {
				read_flag[5] = true;
				als.TextureCoordinate[0].y = attr.as_float();
			}
		}
	}

	// check that all components was defined
	if (!(read_flag[0] && read_flag[1] && read_flag[2] && read_flag[3] && read_flag[4] && read_flag[5])) {
		throw DeadlyImportError("Not all texture coordinates are defined.");
	}

	// copy attributes data
	als.TextureID_R = rtexid;
	als.TextureID_G = gtexid;
	als.TextureID_B = btexid;
	als.TextureID_A = atexid;

	mNodeElement_List.push_back(ne);
}

}// namespace Assimp

#endif // !ASSIMP_BUILD_NO_AMF_IMPORTER
