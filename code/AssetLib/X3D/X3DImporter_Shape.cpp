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
/// \file   X3DImporter_Shape.cpp
/// \brief  Parsing data from nodes of "Shape" set of X3D.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DImporter.hpp"
#include "X3DImporter_Macro.hpp"
#include "X3DXmlHelper.h"

namespace Assimp {

void X3DImporter::readShape(XmlNode &node) {
    std::string use, def;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_Shape, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementShape(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                // check for appearance node
                if (currentChildName == "Appearance") readAppearance(currentChildNode);
                // check for X3DGeometryNodes
                else if (currentChildName == "Arc2D")
                    readArc2D(currentChildNode);
                else if (currentChildName == "ArcClose2D")
                    readArcClose2D(currentChildNode);
                else if (currentChildName == "Circle2D")
                    readCircle2D(currentChildNode);
                else if (currentChildName == "Disk2D")
                    readDisk2D(currentChildNode);
                else if (currentChildName == "Polyline2D")
                    readPolyline2D(currentChildNode);
                else if (currentChildName == "Polypoint2D")
                    readPolypoint2D(currentChildNode);
                else if (currentChildName == "Rectangle2D")
                    readRectangle2D(currentChildNode);
                else if (currentChildName == "TriangleSet2D")
                    readTriangleSet2D(currentChildNode);
                else if (currentChildName == "Box")
                    readBox(currentChildNode);
                else if (currentChildName == "Cone")
                    readCone(currentChildNode);
                else if (currentChildName == "Cylinder")
                    readCylinder(currentChildNode);
                else if (currentChildName == "ElevationGrid")
                    readElevationGrid(currentChildNode);
                else if (currentChildName == "Extrusion")
                    readExtrusion(currentChildNode);
                else if (currentChildName == "IndexedFaceSet")
                    readIndexedFaceSet(currentChildNode);
                else if (currentChildName == "Sphere")
                    readSphere(currentChildNode);
                else if (currentChildName == "IndexedLineSet")
                    readIndexedLineSet(currentChildNode);
                else if (currentChildName == "LineSet")
                    readLineSet(currentChildNode);
                else if (currentChildName == "PointSet")
                    readPointSet(currentChildNode);
                else if (currentChildName == "IndexedTriangleFanSet")
                    readIndexedTriangleFanSet(currentChildNode);
                else if (currentChildName == "IndexedTriangleSet")
                    readIndexedTriangleSet(currentChildNode);
                else if (currentChildName == "IndexedTriangleStripSet")
                    readIndexedTriangleStripSet(currentChildNode);
                else if (currentChildName == "TriangleFanSet")
                    readTriangleFanSet(currentChildNode);
                else if (currentChildName == "TriangleSet")
                    readTriangleSet(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("Shape", currentChildNode);
            }

            ParseHelper_Node_Exit();
        } // if (!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Appearance
// DEF="" ID
// USE="" IDREF
// >
// <!-- AppearanceChildContentModel -->
// "Child-node content model corresponding to X3DAppearanceChildNode. Appearance can contain FillProperties, LineProperties, Material, any Texture node and
// any TextureTransform node, in any order. No more than one instance of these nodes is allowed. Appearance may also contain multiple shaders (ComposedShader,
// PackagedShader, ProgramShader).
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model."
// </Appearance>
void X3DImporter::readAppearance(XmlNode &node) {
    std::string use, def;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_Appearance, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementAppearance(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        // check for child nodes
        if (!isNodeEmpty(node)) {
            ParseHelper_Node_Enter(ne);
            for (auto currentChildNode : node.children()) {
                const std::string &currentChildName = currentChildNode.name();
                if (currentChildName == "Material")
                    readMaterial(currentChildNode);
                else if (currentChildName == "ImageTexture")
                    readImageTexture(currentChildNode);
                else if (currentChildName == "TextureTransform")
                    readTextureTransform(currentChildNode);
                // check for X3DMetadataObject
                else if (!checkForMetadataNode(currentChildNode))
                    skipUnsupportedNode("Appearance", currentChildNode);
            }
            ParseHelper_Node_Exit();
        } // if(!isNodeEmpty(node))
        else {
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Material
// DEF=""                     ID
// USE=""                     IDREF
// ambientIntensity="0.2"     SFFloat [inputOutput]
// diffuseColor="0.8 0.8 0.8" SFColor [inputOutput]
// emissiveColor="0 0 0"      SFColor [inputOutput]
// shininess="0.2"            SFFloat [inputOutput]
// specularColor="0 0 0"      SFColor [inputOutput]
// transparency="0"           SFFloat [inputOutput]
// />
void X3DImporter::readMaterial(XmlNode &node) {
    std::string use, def;
    float ambientIntensity = 0.2f;
    float shininess = 0.2f;
    float transparency = 0;
    aiColor3D diffuseColor(0.8f, 0.8f, 0.8f);
    aiColor3D emissiveColor(0, 0, 0);
    aiColor3D specularColor(0, 0, 0);
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getFloatAttribute(node, "ambientIntensity", ambientIntensity);
    XmlParser::getFloatAttribute(node, "shininess", shininess);
    XmlParser::getFloatAttribute(node, "transparency", transparency);
    X3DXmlHelper::getColor3DAttribute(node, "diffuseColor", diffuseColor);
    X3DXmlHelper::getColor3DAttribute(node, "emissiveColor", emissiveColor);
    X3DXmlHelper::getColor3DAttribute(node, "specularColor", specularColor);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_Material, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new X3DNodeElementMaterial(mNodeElementCur);
        if (!def.empty()) ne->ID = def;

        ((X3DNodeElementMaterial *)ne)->AmbientIntensity = ambientIntensity;
        ((X3DNodeElementMaterial *)ne)->Shininess = shininess;
        ((X3DNodeElementMaterial *)ne)->Transparency = transparency;
        ((X3DNodeElementMaterial *)ne)->DiffuseColor = diffuseColor;
        ((X3DNodeElementMaterial *)ne)->EmissiveColor = emissiveColor;
        ((X3DNodeElementMaterial *)ne)->SpecularColor = specularColor;
        // check for child nodes
        if (!isNodeEmpty(node))
            childrenReadMetadata(node, ne, "Material");
        else
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
