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
/// \file   X3DImporter_Rendering.cpp
/// \brief  Parsing data from nodes of "Rendering" set of X3D.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DImporter.hpp"
#include "X3DImporter_Macro.hpp"
#include "X3DXmlHelper.h"

namespace Assimp {

// <Color
// DEF=""           ID
// USE=""           IDREF
// color="" MFColor [inputOutput]
// />
void X3DImporter::readColor(XmlNode &node) {
    std::string use, def;
    std::list<aiColor3D> color;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    X3DXmlHelper::getColor3DListAttribute(node, "color", color);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_Color, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementColor(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        ((X3DNodeElementColor *)ne)->Value = color;
        // check for X3DMetadataObject childs.
        if (!isNodeEmpty(node))
            childrenReadMetadata(node, ne, "Color");
        else
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <ColorRGBA
// DEF=""               ID
// USE=""               IDREF
// color="" MFColorRGBA [inputOutput]
// />
void X3DImporter::readColorRGBA(XmlNode &node) {
    std::string use, def;
    std::list<aiColor4D> color;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    X3DXmlHelper::getColor4DListAttribute(node, "color", color);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_ColorRGBA, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementColorRGBA(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        ((X3DNodeElementColorRGBA *)ne)->Value = color;
        // check for X3DMetadataObject childs.
        if (!isNodeEmpty(node))
            childrenReadMetadata(node, ne, "ColorRGBA");
        else
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Coordinate
// DEF=""      ID
// USE=""      IDREF
// point=""    MFVec3f [inputOutput]
// />
void X3DImporter::readCoordinate(XmlNode &node) {
    std::string use, def;
    std::list<aiVector3D> point;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    X3DXmlHelper::getVector3DListAttribute(node, "point", point);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_Coordinate, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementCoordinate(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        ((X3DNodeElementCoordinate *)ne)->Value = point;
        // check for X3DMetadataObject childs.
        if (!isNodeEmpty(node))
            childrenReadMetadata(node, ne, "Coordinate");
        else
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <IndexedLineSet
// DEF=""                ID
// USE=""                IDREF
// colorIndex=""         MFInt32 [initializeOnly]
// colorPerVertex="true" SFBool  [initializeOnly]
// coordIndex=""         MFInt32 [initializeOnly]
// >
//    <!-- ColorCoordinateContentModel -->
// ColorCoordinateContentModel is the child-node content model corresponding to IndexedLineSet, LineSet and PointSet. ColorCoordinateContentModel can
// contain any-order Coordinate node with Color (or ColorRGBA) node. No more than one instance of any single node type is allowed.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </IndexedLineSet>
void X3DImporter::readIndexedLineSet(XmlNode &node) {
    std::string use, def;
    std::vector<int32_t> colorIndex;
    bool colorPerVertex = true;
    std::vector<int32_t> coordIndex;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    X3DXmlHelper::getInt32ArrayAttribute(node, "colorIndex", colorIndex);
    XmlParser::getBoolAttribute(node, "colorPerVertex", colorPerVertex);
    X3DXmlHelper::getInt32ArrayAttribute(node, "coordIndex", coordIndex);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_IndexedLineSet, ne);
    } else {
        // check data
        if ((coordIndex.size() < 2) || ((coordIndex.back() == (-1)) && (coordIndex.size() < 3)))
            throw DeadlyImportError("IndexedLineSet must contain not empty \"coordIndex\" attribute.");

        // create and if needed - define new geometry object.
        ne = new X3DNodeElementIndexedSet(X3DElemType::ENET_IndexedLineSet, mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        X3DNodeElementIndexedSet &ne_alias = *((X3DNodeElementIndexedSet *)ne);

        ne_alias.ColorIndex = colorIndex;
        ne_alias.ColorPerVertex = colorPerVertex;
        ne_alias.CoordIndex = coordIndex;
        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                // check for Color and Coordinate nodes
                if (currentChildName == "Color")
                    readColor(currentChildNode);
                else if (currentChildName == "ColorRGBA")
                    readColorRGBA(currentChildNode);
                else if (currentChildName == "Coordinate")
                    readCoordinate(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("IndexedLineSet", currentChildNode);
            }
            ParseHelper_Node_Exit();
        } // if(!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <IndexedTriangleFanSet
// DEF=""                 ID
// USE=""                 IDREF
// ccw="true"             SFBool  [initializeOnly]
// colorPerVertex="true"  SFBool  [initializeOnly]
// index=""               MFInt32 [initializeOnly]
// normalPerVertex="true" SFBool  [initializeOnly]
// solid="true"           SFBool  [initializeOnly]
// >
//    <!-- ComposedGeometryContentModel -->
// ComposedGeometryContentModel is the child-node content model corresponding to X3DComposedGeometryNodes. It can contain Color (or ColorRGBA), Coordinate,
// Normal and TextureCoordinate, in any order. No more than one instance of these nodes is allowed. Multiple VertexAttribute (FloatVertexAttribute,
// Matrix3VertexAttribute, Matrix4VertexAttribute) nodes can also be contained.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </IndexedTriangleFanSet>
void X3DImporter::readIndexedTriangleFanSet(XmlNode &node) {
    std::string use, def;
    bool ccw = true;
    bool colorPerVertex = true;
    std::vector<int32_t> index;
    bool normalPerVertex = true;
    bool solid = true;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getBoolAttribute(node, "ccw", ccw);
    XmlParser::getBoolAttribute(node, "colorPerVertex", colorPerVertex);
    X3DXmlHelper::getInt32ArrayAttribute(node, "index", index);
    XmlParser::getBoolAttribute(node, "normalPerVertex", normalPerVertex);
    XmlParser::getBoolAttribute(node, "solid", solid);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_IndexedTriangleFanSet, ne);
    } else {
        // check data
        if (index.size() == 0) throw DeadlyImportError("IndexedTriangleFanSet must contain not empty \"index\" attribute.");

        // create and if needed - define new geometry object.
        ne = new X3DNodeElementIndexedSet(X3DElemType::ENET_IndexedTriangleFanSet, mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        X3DNodeElementIndexedSet &ne_alias = *((X3DNodeElementIndexedSet *)ne);

        ne_alias.CCW = ccw;
        ne_alias.ColorPerVertex = colorPerVertex;
        ne_alias.NormalPerVertex = normalPerVertex;
        ne_alias.Solid = solid;

        ne_alias.CoordIndex.clear();
        int counter = 0;
        int32_t idx[3];
        for (std::vector<int32_t>::const_iterator idx_it = index.begin(); idx_it != index.end(); ++idx_it) {
            idx[2] = *idx_it;
            if (idx[2] < 0) {
                counter = 0;
            } else {
                if (counter >= 2) {
                    if (ccw) {
                        ne_alias.CoordIndex.push_back(idx[0]);
                        ne_alias.CoordIndex.push_back(idx[1]);
                        ne_alias.CoordIndex.push_back(idx[2]);
                    } else {
                        ne_alias.CoordIndex.push_back(idx[0]);
                        ne_alias.CoordIndex.push_back(idx[2]);
                        ne_alias.CoordIndex.push_back(idx[1]);
                    }
                    ne_alias.CoordIndex.push_back(-1);
                    idx[1] = idx[2];
                } else {
                    idx[counter] = idx[2];
                }
                ++counter;
            }
        } // for(std::list<int32_t>::const_iterator idx_it = index.begin(); idx_it != ne_alias.index.end(); idx_it++)

        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                // check for X3DComposedGeometryNodes
                if (currentChildName == "Color")
                    readColor(currentChildNode);
                else if (currentChildName == "ColorRGBA")
                    readColorRGBA(currentChildNode);
                else if (currentChildName == "Coordinate")
                    readCoordinate(currentChildNode);
                else if (currentChildName == "Normal")
                    readNormal(currentChildNode);
                else if (currentChildName == "TextureCoordinate")
                    readTextureCoordinate(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("IndexedTriangleFanSet", currentChildNode);
            }
            ParseHelper_Node_Exit();
        } // if(!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <IndexedTriangleSet
// DEF=""                 ID
// USE=""                 IDREF
// ccw="true"             SFBool  [initializeOnly]
// colorPerVertex="true"  SFBool  [initializeOnly]
// index=""               MFInt32 [initializeOnly]
// normalPerVertex="true" SFBool  [initializeOnly]
// solid="true"           SFBool  [initializeOnly]
// >
//    <!-- ComposedGeometryContentModel -->
// ComposedGeometryContentModel is the child-node content model corresponding to X3DComposedGeometryNodes. It can contain Color (or ColorRGBA), Coordinate,
// Normal and TextureCoordinate, in any order. No more than one instance of these nodes is allowed. Multiple VertexAttribute (FloatVertexAttribute,
// Matrix3VertexAttribute, Matrix4VertexAttribute) nodes can also be contained.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </IndexedTriangleSet>
void X3DImporter::readIndexedTriangleSet(XmlNode &node) {
    std::string use, def;
    bool ccw = true;
    bool colorPerVertex = true;
    std::vector<int32_t> index;
    bool normalPerVertex = true;
    bool solid = true;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getBoolAttribute(node, "ccw", ccw);
    XmlParser::getBoolAttribute(node, "colorPerVertex", colorPerVertex);
    X3DXmlHelper::getInt32ArrayAttribute(node, "index", index);
    XmlParser::getBoolAttribute(node, "normalPerVertex", normalPerVertex);
    XmlParser::getBoolAttribute(node, "solid", solid);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_IndexedTriangleSet, ne);
    } else {
        // check data
        if (index.size() == 0) throw DeadlyImportError("IndexedTriangleSet must contain not empty \"index\" attribute.");

        // create and if needed - define new geometry object.
        ne = new X3DNodeElementIndexedSet(X3DElemType::ENET_IndexedTriangleSet, mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        X3DNodeElementIndexedSet &ne_alias = *((X3DNodeElementIndexedSet *)ne);

        ne_alias.CCW = ccw;
        ne_alias.ColorPerVertex = colorPerVertex;
        ne_alias.NormalPerVertex = normalPerVertex;
        ne_alias.Solid = solid;

        ne_alias.CoordIndex.clear();
        int counter = 0;
        int32_t idx[3];
        for (std::vector<int32_t>::const_iterator idx_it = index.begin(); idx_it != index.end(); ++idx_it) {
            idx[counter++] = *idx_it;
            if (counter > 2) {
                counter = 0;
                if (ccw) {
                    ne_alias.CoordIndex.push_back(idx[0]);
                    ne_alias.CoordIndex.push_back(idx[1]);
                    ne_alias.CoordIndex.push_back(idx[2]);
                } else {
                    ne_alias.CoordIndex.push_back(idx[0]);
                    ne_alias.CoordIndex.push_back(idx[2]);
                    ne_alias.CoordIndex.push_back(idx[1]);
                }
                ne_alias.CoordIndex.push_back(-1);
            }
        } // for(std::list<int32_t>::const_iterator idx_it = index.begin(); idx_it != ne_alias.index.end(); idx_it++)

        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                // check for X3DComposedGeometryNodes
                if (currentChildName == "Color")
                    readColor(currentChildNode);
                else if (currentChildName == "ColorRGBA")
                    readColorRGBA(currentChildNode);
                else if (currentChildName == "Coordinate")
                    readCoordinate(currentChildNode);
                else if (currentChildName == "Normal")
                    readNormal(currentChildNode);
                else if (currentChildName == "TextureCoordinate")
                    readTextureCoordinate(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("IndexedTriangleSet", currentChildNode);
            }
            ParseHelper_Node_Exit();
        } // if(!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <IndexedTriangleStripSet
// DEF=""                 ID
// USE=""                 IDREF
// ccw="true"             SFBool  [initializeOnly]
// colorPerVertex="true"  SFBool  [initializeOnly]
// index=""               MFInt32 [initializeOnly]
// normalPerVertex="true" SFBool  [initializeOnly]
// solid="true"           SFBool  [initializeOnly]
// >
//    <!-- ComposedGeometryContentModel -->
// ComposedGeometryContentModel is the child-node content model corresponding to X3DComposedGeometryNodes. It can contain Color (or ColorRGBA), Coordinate,
// Normal and TextureCoordinate, in any order. No more than one instance of these nodes is allowed. Multiple VertexAttribute (FloatVertexAttribute,
// Matrix3VertexAttribute, Matrix4VertexAttribute) nodes can also be contained.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </IndexedTriangleStripSet>
void X3DImporter::readIndexedTriangleStripSet(XmlNode &node) {
    std::string use, def;
    bool ccw = true;
    bool colorPerVertex = true;
    std::vector<int32_t> index;
    bool normalPerVertex = true;
    bool solid = true;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getBoolAttribute(node, "ccw", ccw);
    XmlParser::getBoolAttribute(node, "colorPerVertex", colorPerVertex);
    X3DXmlHelper::getInt32ArrayAttribute(node, "index", index);
    XmlParser::getBoolAttribute(node, "normalPerVertex", normalPerVertex);
    XmlParser::getBoolAttribute(node, "solid", solid);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_IndexedTriangleStripSet, ne);
    } else {
        // check data
        if (index.empty()) {
            throw DeadlyImportError("IndexedTriangleStripSet must contain not empty \"index\" attribute.");
        }

        // create and if needed - define new geometry object.
        ne = new X3DNodeElementIndexedSet(X3DElemType::ENET_IndexedTriangleStripSet, mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        X3DNodeElementIndexedSet &ne_alias = *((X3DNodeElementIndexedSet *)ne);

        ne_alias.CCW = ccw;
        ne_alias.ColorPerVertex = colorPerVertex;
        ne_alias.NormalPerVertex = normalPerVertex;
        ne_alias.Solid = solid;

        ne_alias.CoordIndex.clear();
        int counter = 0;
        int32_t idx[3];
        for (std::vector<int32_t>::const_iterator idx_it = index.begin(); idx_it != index.end(); ++idx_it) {
            idx[2] = *idx_it;
            if (idx[2] < 0) {
                counter = 0;
            } else {
                if (counter >= 2) {
                    if (ccw) {
                        ne_alias.CoordIndex.push_back(idx[0]);
                        ne_alias.CoordIndex.push_back(idx[1]);
                        ne_alias.CoordIndex.push_back(idx[2]);
                    } else {
                        ne_alias.CoordIndex.push_back(idx[0]);
                        ne_alias.CoordIndex.push_back(idx[2]);
                        ne_alias.CoordIndex.push_back(idx[1]);
                    }
                    ne_alias.CoordIndex.push_back(-1);
                }
                idx[counter & 1] = idx[2];
                ++counter;
            }
        } // for(std::list<int32_t>::const_iterator idx_it = index.begin(); idx_it != ne_alias.index.end(); idx_it++)

        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                // check for X3DComposedGeometryNodes
                if (currentChildName == "Color")
                    readColor(currentChildNode);
                else if (currentChildName == "ColorRGBA")
                    readColorRGBA(currentChildNode);
                else if (currentChildName == "Coordinate")
                    readCoordinate(currentChildNode);
                else if (currentChildName == "Normal")
                    readNormal(currentChildNode);
                else if (currentChildName == "TextureCoordinate")
                    readTextureCoordinate(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("IndexedTriangleStripSet", currentChildNode);
            }
            ParseHelper_Node_Exit();
        } // if(!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <LineSet
// DEF=""         ID
// USE=""         IDREF
// vertexCount="" MFInt32 [initializeOnly]
// >
//    <!-- ColorCoordinateContentModel -->
// ColorCoordinateContentModel is the child-node content model corresponding to IndexedLineSet, LineSet and PointSet. ColorCoordinateContentModel can
// contain any-order Coordinate node with Color (or ColorRGBA) node. No more than one instance of any single node type is allowed.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </LineSet>
void X3DImporter::readLineSet(XmlNode &node) {
    std::string use, def;
    std::vector<int32_t> vertexCount;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    X3DXmlHelper::getInt32ArrayAttribute(node, "vertexCount", vertexCount);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_LineSet, ne);
    } else {
        // check data
        if (vertexCount.empty()) {
            throw DeadlyImportError("LineSet must contain not empty \"vertexCount\" attribute.");
        }

        // create and if needed - define new geometry object.
        ne = new X3DNodeElementSet(X3DElemType::ENET_LineSet, mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        X3DNodeElementSet &ne_alias = *((X3DNodeElementSet *)ne);

        ne_alias.VertexCount = vertexCount;
        // create CoordIdx
        size_t coord_num = 0;

        ne_alias.CoordIndex.clear();
        for (std::vector<int32_t>::const_iterator vc_it = ne_alias.VertexCount.begin(); vc_it != ne_alias.VertexCount.end(); ++vc_it) {
            if (*vc_it < 2) throw DeadlyImportError("LineSet. vertexCount shall be greater than or equal to two.");

            for (int32_t i = 0; i < *vc_it; i++)
                ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num++)); // add vertices indices

            ne_alias.CoordIndex.push_back(-1); // add face delimiter.
        }

        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                // check for X3DComposedGeometryNodes
                if (currentChildName == "Color")
                    readColor(currentChildNode);
                else if (currentChildName == "ColorRGBA")
                    readColorRGBA(currentChildNode);
                else if (currentChildName == "Coordinate")
                    readCoordinate(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("LineSet", currentChildNode);
            }
            ParseHelper_Node_Exit();
        } // if(!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <PointSet
// DEF="" ID
// USE="" IDREF
// >
//    <!-- ColorCoordinateContentModel -->
// ColorCoordinateContentModel is the child-node content model corresponding to IndexedLineSet, LineSet and PointSet. ColorCoordinateContentModel can
// contain any-order Coordinate node with Color (or ColorRGBA) node. No more than one instance of any single node type is allowed.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </PointSet>
void X3DImporter::readPointSet(XmlNode &node) {
    std::string use, def;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_PointSet, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementIndexedSet(X3DElemType::ENET_PointSet, mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                // check for X3DComposedGeometryNodes
                if (currentChildName == "Color")
                    readColor(currentChildNode);
                else if (currentChildName == "ColorRGBA")
                    readColorRGBA(currentChildNode);
                else if (currentChildName == "Coordinate")
                    readCoordinate(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("PointSet", currentChildNode);
            }
            ParseHelper_Node_Exit();
        } // if(!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <TriangleFanSet
// DEF=""                 ID
// USE=""                 IDREF
// ccw="true"             SFBool  [initializeOnly]
// colorPerVertex="true"  SFBool  [initializeOnly]
// fanCount=""            MFInt32 [inputOutput]
// normalPerVertex="true" SFBool  [initializeOnly]
// solid="true"           SFBool  [initializeOnly]
// >
//    <!-- ComposedGeometryContentModel -->
// ComposedGeometryContentModel is the child-node content model corresponding to X3DComposedGeometryNodes. It can contain Color (or ColorRGBA), Coordinate,
// Normal and TextureCoordinate, in any order. No more than one instance of these nodes is allowed. Multiple VertexAttribute (FloatVertexAttribute,
// Matrix3VertexAttribute, Matrix4VertexAttribute) nodes can also be contained.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </TriangleFanSet>
void X3DImporter::readTriangleFanSet(XmlNode &node) {
    std::string use, def;
    bool ccw = true;
    bool colorPerVertex = true;
    std::vector<int32_t> fanCount;
    bool normalPerVertex = true;
    bool solid = true;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getBoolAttribute(node, "ccw", ccw);
    XmlParser::getBoolAttribute(node, "colorPerVertex", colorPerVertex);
    X3DXmlHelper::getInt32ArrayAttribute(node, "fanCount", fanCount);
    XmlParser::getBoolAttribute(node, "normalPerVertex", normalPerVertex);
    XmlParser::getBoolAttribute(node, "solid", solid);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_TriangleFanSet, ne);
    } else {
        // check data
        if (fanCount.empty()) {
            throw DeadlyImportError("TriangleFanSet must contain not empty \"fanCount\" attribute.");
        }

        // create and if needed - define new geometry object.
        ne = new X3DNodeElementSet(X3DElemType::ENET_TriangleFanSet, mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        X3DNodeElementSet &ne_alias = *((X3DNodeElementSet *)ne);

        ne_alias.CCW = ccw;
        ne_alias.ColorPerVertex = colorPerVertex;
        ne_alias.VertexCount = fanCount;
        ne_alias.NormalPerVertex = normalPerVertex;
        ne_alias.Solid = solid;
        // create CoordIdx
        size_t coord_num_first, coord_num_prev;

        ne_alias.CoordIndex.clear();
        // assign indices for first triangle
        coord_num_first = 0;
        coord_num_prev = 1;
        for (std::vector<int32_t>::const_iterator vc_it = ne_alias.VertexCount.begin(); vc_it != ne_alias.VertexCount.end(); ++vc_it) {
            if (*vc_it < 3) throw DeadlyImportError("TriangleFanSet. fanCount shall be greater than or equal to three.");

            for (int32_t vc = 2; vc < *vc_it; vc++) {
                if (ccw) {
                    // 2 1
                    //  0
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num_first)); // first vertex is a center and always is [0].
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num_prev++));
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num_prev));
                } else {
                    // 1 2
                    //  0
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num_first)); // first vertex is a center and always is [0].
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num_prev + 1));
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num_prev++));
                } // if(ccw) else

                ne_alias.CoordIndex.push_back(-1); // add face delimiter.
            } // for(int32_t vc = 2; vc < *vc_it; vc++)

            coord_num_prev++; // that index will be center of next fan
            coord_num_first = coord_num_prev++; // forward to next point - second point of fan
        } // for(std::list<int32_t>::const_iterator vc_it = ne_alias.VertexCount.begin(); vc_it != ne_alias.VertexCount.end(); vc_it++)
        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                // check for X3DComposedGeometryNodes
                if (currentChildName == "Color")
                    readColor(currentChildNode);
                else if (currentChildName == "ColorRGBA")
                    readColorRGBA(currentChildNode);
                else if (currentChildName == "Coordinate")
                    readCoordinate(currentChildNode);
                else if (currentChildName == "Normal")
                    readNormal(currentChildNode);
                else if (currentChildName == "TextureCoordinate")
                    readTextureCoordinate(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("TriangleFanSet", currentChildNode);
            }
            ParseHelper_Node_Exit();
        } // if(!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <TriangleSet
// DEF=""                 ID
// USE=""                 IDREF
// ccw="true"             SFBool  [initializeOnly]
// colorPerVertex="true"  SFBool  [initializeOnly]
// normalPerVertex="true" SFBool  [initializeOnly]
// solid="true"           SFBool  [initializeOnly]
// >
//    <!-- ComposedGeometryContentModel -->
// ComposedGeometryContentModel is the child-node content model corresponding to X3DComposedGeometryNodes. It can contain Color (or ColorRGBA), Coordinate,
// Normal and TextureCoordinate, in any order. No more than one instance of these nodes is allowed. Multiple VertexAttribute (FloatVertexAttribute,
// Matrix3VertexAttribute, Matrix4VertexAttribute) nodes can also be contained.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </TriangleSet>
void X3DImporter::readTriangleSet(XmlNode &node) {
    std::string use, def;
    bool ccw = true;
    bool colorPerVertex = true;
    bool normalPerVertex = true;
    bool solid = true;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getBoolAttribute(node, "ccw", ccw);
    XmlParser::getBoolAttribute(node, "colorPerVertex", colorPerVertex);
    XmlParser::getBoolAttribute(node, "normalPerVertex", normalPerVertex);
    XmlParser::getBoolAttribute(node, "solid", solid);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_TriangleSet, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementIndexedSet(X3DElemType::ENET_TriangleSet, mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        X3DNodeElementSet &ne_alias = *((X3DNodeElementSet *)ne);

        ne_alias.CCW = ccw;
        ne_alias.ColorPerVertex = colorPerVertex;
        ne_alias.NormalPerVertex = normalPerVertex;
        ne_alias.Solid = solid;
        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                // check for X3DComposedGeometryNodes
                if (currentChildName == "Color")
                    readColor(currentChildNode);
                else if (currentChildName == "ColorRGBA")
                    readColorRGBA(currentChildNode);
                else if (currentChildName == "Coordinate")
                    readCoordinate(currentChildNode);
                else if (currentChildName == "Normal")
                    readNormal(currentChildNode);
                else if (currentChildName == "TextureCoordinate")
                    readTextureCoordinate(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("TriangleSet", currentChildNode);
            }
            ParseHelper_Node_Exit();
        } // if(!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <TriangleStripSet
// DEF=""                 ID
// USE=""                 IDREF
// ccw="true"             SFBool  [initializeOnly]
// colorPerVertex="true"  SFBool  [initializeOnly]
// normalPerVertex="true" SFBool  [initializeOnly]
// solid="true"           SFBool  [initializeOnly]
// stripCount=""          MFInt32 [inputOutput]
// >
//    <!-- ComposedGeometryContentModel -->
// ComposedGeometryContentModel is the child-node content model corresponding to X3DComposedGeometryNodes. It can contain Color (or ColorRGBA), Coordinate,
// Normal and TextureCoordinate, in any order. No more than one instance of these nodes is allowed. Multiple VertexAttribute (FloatVertexAttribute,
// Matrix3VertexAttribute, Matrix4VertexAttribute) nodes can also be contained.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </TriangleStripSet>
void X3DImporter::readTriangleStripSet(XmlNode &node) {
    std::string use, def;
    bool ccw = true;
    bool colorPerVertex = true;
    std::vector<int32_t> stripCount;
    bool normalPerVertex = true;
    bool solid = true;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getBoolAttribute(node, "ccw", ccw);
    XmlParser::getBoolAttribute(node, "colorPerVertex", colorPerVertex);
    X3DXmlHelper::getInt32ArrayAttribute(node, "stripCount", stripCount);
    XmlParser::getBoolAttribute(node, "normalPerVertex", normalPerVertex);
    XmlParser::getBoolAttribute(node, "solid", solid);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_TriangleStripSet, ne);
    } else {
        // check data
        if (stripCount.size() == 0) throw DeadlyImportError("TriangleStripSet must contain not empty \"stripCount\" attribute.");

        // create and if needed - define new geometry object.
        ne = new X3DNodeElementSet(X3DElemType::ENET_TriangleStripSet, mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        X3DNodeElementSet &ne_alias = *((X3DNodeElementSet *)ne);

        ne_alias.CCW = ccw;
        ne_alias.ColorPerVertex = colorPerVertex;
        ne_alias.VertexCount = stripCount;
        ne_alias.NormalPerVertex = normalPerVertex;
        ne_alias.Solid = solid;
        // create CoordIdx
        size_t coord_num0, coord_num1, coord_num2; // indices of current triangle
        bool odd_tri; // sequence of current triangle
        size_t coord_num_sb; // index of first point of strip

        ne_alias.CoordIndex.clear();
        coord_num_sb = 0;
        for (std::vector<int32_t>::const_iterator vc_it = ne_alias.VertexCount.begin(); vc_it != ne_alias.VertexCount.end(); ++vc_it) {
            if (*vc_it < 3) throw DeadlyImportError("TriangleStripSet. stripCount shall be greater than or equal to three.");

            // set initial values for first triangle
            coord_num0 = coord_num_sb;
            coord_num1 = coord_num_sb + 1;
            coord_num2 = coord_num_sb + 2;
            odd_tri = true;

            for (int32_t vc = 2; vc < *vc_it; vc++) {
                if (ccw) {
                    // 0 2
                    //  1
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num0));
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num1));
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num2));
                } else {
                    // 0 1
                    //  2
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num0));
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num2));
                    ne_alias.CoordIndex.push_back(static_cast<int32_t>(coord_num1));
                } // if(ccw) else

                ne_alias.CoordIndex.push_back(-1); // add face delimiter.
                // prepare values for next triangle
                if (odd_tri) {
                    coord_num0 = coord_num2;
                    coord_num2++;
                } else {
                    coord_num1 = coord_num2;
                    coord_num2 = coord_num1 + 1;
                }

                odd_tri = !odd_tri;
                coord_num_sb = coord_num2; // that index will be start of next strip
            } // for(int32_t vc = 2; vc < *vc_it; vc++)
        } // for(std::list<int32_t>::const_iterator vc_it = ne_alias.VertexCount.begin(); vc_it != ne_alias.VertexCount.end(); vc_it++)
        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                // check for X3DComposedGeometryNodes
                if (currentChildName == "Color")
                    readColor(currentChildNode);
                else if (currentChildName == "ColorRGBA")
                    readColorRGBA(currentChildNode);
                else if (currentChildName == "Coordinate")
                    readCoordinate(currentChildNode);
                else if (currentChildName == "Normal")
                    readNormal(currentChildNode);
                else if (currentChildName == "TextureCoordinate")
                    readTextureCoordinate(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("TriangleStripSet", currentChildNode);
            }
            ParseHelper_Node_Exit();
        } // if(!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Normal
// DEF=""      ID
// USE=""      IDREF
// vector=""   MFVec3f [inputOutput]
// />
void X3DImporter::readNormal(XmlNode &node) {
    std::string use, def;
    std::list<aiVector3D> vector;
    X3DNodeElementBase *ne=nullptr;

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    X3DXmlHelper::getVector3DListAttribute(node, "vector", vector);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_Normal, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementNormal(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        ((X3DNodeElementNormal *)ne)->Value = vector;
        // check for X3DMetadataObject childs.
        if (!isNodeEmpty(node))
            childrenReadMetadata(node, ne, "Normal");
        else
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
