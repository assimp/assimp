/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

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

#include "AMFImporter.hpp"
#include <assimp/ParsingUtils.h>

namespace Assimp {

// <mesh>
// </mesh>
// A 3D mesh hull.
// Multi elements - Yes.
// Parent element - <object>.
void AMFImporter::ParseNode_Mesh(XmlNode &node) {
    AMFNodeElementBase *ne = nullptr;

    // Check for child nodes
    if (0 != ASSIMP_stricmp(node.name(), "mesh")) {
        return;
    }
    // create new mesh object.
    ne = new AMFMesh(mNodeElement_Cur);
    bool found_verts = false, found_volumes = false;
    if (!node.empty()) {
        ParseHelper_Node_Enter(ne);
        pugi::xml_node vertNode = node.child("vertices");
        if (!vertNode.empty()) {
            ParseNode_Vertices(vertNode);
            found_verts = true;
        }

        pugi::xml_node volumeNode = node.child("volume");
        if (!volumeNode.empty()) {
            ParseNode_Volume(volumeNode);
            found_volumes = true;
        }
        ParseHelper_Node_Exit();
    } 

    if (!found_verts && !found_volumes) {
        mNodeElement_Cur->Child.push_back(ne);
    } // if(!mReader->isEmptyElement()) else

    // and to node element list because its a new object in graph.
    mNodeElement_List.push_back(ne);
}

// <vertices>
// </vertices>
// The list of vertices to be used in defining triangles.
// Multi elements - No.
// Parent element - <mesh>.
void AMFImporter::ParseNode_Vertices(XmlNode &node) {
    AMFNodeElementBase *ne = nullptr;

    // create new mesh object.
    ne = new AMFVertices(mNodeElement_Cur);
    // Check for child nodes
    if (node.empty()) {
        mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
        return;
    }
    ParseHelper_Node_Enter(ne);
    for (XmlNode &currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "vertex") {
            ParseNode_Vertex(currentNode);
        }
    }
    ParseHelper_Node_Exit();
    mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

// <vertex>
// </vertex>
// A vertex to be referenced in triangles.
// Multi elements - Yes.
// Parent element - <vertices>.
void AMFImporter::ParseNode_Vertex(XmlNode &node) {
    AMFNodeElementBase *ne = nullptr;

    // create new mesh object.
    ne = new AMFVertex(mNodeElement_Cur);

    // Check for child nodes
    pugi::xml_node colorNode = node.child("color");
    bool col_read = false;
    bool coord_read = false;
    if (!node.empty()) {
        ParseHelper_Node_Enter(ne);
        if (!colorNode.empty()) {
            ParseNode_Color(colorNode);
            col_read = true;
        }
        pugi::xml_node coordNode = node.child("coordinates");
        if (!coordNode.empty()) {
            ParseNode_Coordinates(coordNode);
            coord_read = true;
        }
        ParseHelper_Node_Exit();
    }

    if (!coord_read && !col_read) {
        mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
    }

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
void AMFImporter::ParseNode_Coordinates(XmlNode &node) {
    AMFNodeElementBase *ne = nullptr;
    if (!node.empty()) {
        ne = new AMFCoordinates(mNodeElement_Cur);
        ParseHelper_Node_Enter(ne);
        for (XmlNode &currentNode : node.children()) {
            // create new color object.
            AMFCoordinates &als = *((AMFCoordinates *)ne); // alias for convenience
            const std::string &currentName = ai_tolower(currentNode.name());
            if (currentName == "x") {
                XmlParser::getValueAsFloat(currentNode, als.Coordinate.x);
            } else if (currentName == "y") {
                XmlParser::getValueAsFloat(currentNode, als.Coordinate.y);
            } else if (currentName == "z") {
                XmlParser::getValueAsFloat(currentNode, als.Coordinate.z);
            }
        }
        ParseHelper_Node_Exit();

    } else {
        mNodeElement_Cur->Child.push_back(new AMFCoordinates(mNodeElement_Cur));
    }

    mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

// <volume
// materialid="" - Which material to use.
// type=""       - What this volume describes can be "region" or "support". If none specified, "object" is assumed. If support, then the geometric
//                 requirements 1-8 listed in section 5 do not need to be maintained.
// >
// </volume>
// Defines a volume from the established vertex list.
// Multi elements - Yes.
// Parent element - <mesh>.
void AMFImporter::ParseNode_Volume(XmlNode &node) {
    std::string materialid;
    std::string type;
    AMFNodeElementBase *ne = new AMFVolume(mNodeElement_Cur);

    // Read attributes for node <color>.
    // and assign read data
   
    ((AMFVolume *)ne)->MaterialID = node.attribute("materialid").as_string();
     
    ((AMFVolume *)ne)->Type = type;
    // Check for child nodes
    bool col_read = false;
    if (!node.empty()) {
        ParseHelper_Node_Enter(ne);
        for (auto &currentNode : node.children()) {
            const std::string currentName = currentNode.name();
            if (currentName == "color") {
                if (col_read) Throw_MoreThanOnceDefined(currentName, "color", "Only one color can be defined for <volume>.");
                ParseNode_Color(currentNode);
                col_read = true;
            } else if (currentName == "triangle") {
                ParseNode_Triangle(currentNode);
            } else if (currentName == "metadata") {
                ParseNode_Metadata(currentNode);
            } else if (currentName == "volume") {
                ParseNode_Metadata(currentNode);
            }
        }
        ParseHelper_Node_Exit();
    } else {
        mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
    }

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
void AMFImporter::ParseNode_Triangle(XmlNode &node) {
    AMFNodeElementBase *ne = new AMFTriangle(mNodeElement_Cur);

    // create new triangle object.

    AMFTriangle &als = *((AMFTriangle *)ne); // alias for convenience

    bool col_read = false;
    if (!node.empty()) {
        ParseHelper_Node_Enter(ne);
        std::string v;
        for (auto &currentNode : node.children()) {
            const std::string currentName = currentNode.name();
            if (currentName == "color") {
                if (col_read) Throw_MoreThanOnceDefined(currentName, "color", "Only one color can be defined for <triangle>.");
                ParseNode_Color(currentNode);
                col_read = true;
            } else if (currentName == "texmap") {
                ParseNode_TexMap(currentNode);
            } else if (currentName == "map") {
                ParseNode_TexMap(currentNode, true);
            } else if (currentName == "v1") {
                XmlParser::getValueAsString(currentNode, v);
                als.V[0] = std::atoi(v.c_str());
            } else if (currentName == "v2") {
                XmlParser::getValueAsString(currentNode, v);
                als.V[1] = std::atoi(v.c_str());
            } else if (currentName == "v3") {
                XmlParser::getValueAsString(currentNode, v);
                als.V[2] = std::atoi(v.c_str());
            }
        }
        ParseHelper_Node_Exit();
    } else {
        mNodeElement_Cur->Child.push_back(ne); // Add element to child list of current element
    }

    mNodeElement_List.push_back(ne); // and to node element list because its a new object in graph.
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_AMF_IMPORTER
