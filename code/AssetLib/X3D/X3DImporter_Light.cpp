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
/// \file   X3DImporter_Light.cpp
/// \brief  Parsing data from nodes of "Lighting" set of X3D.
/// date   2015-2016
/// author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DImporter.hpp"
#include "X3DImporter_Macro.hpp"
#include "X3DXmlHelper.h"
#include <assimp/StringUtils.h>

namespace Assimp {

// <DirectionalLight
// DEF=""               ID
// USE=""               IDREF
// ambientIntensity="0" SFFloat [inputOutput]
// color="1 1 1"        SFColor [inputOutput]
// direction="0 0 -1"   SFVec3f [inputOutput]
// global="false"       SFBool  [inputOutput]
// intensity="1"        SFFloat [inputOutput]
// on="true"            SFBool  [inputOutput]
// />
void X3DImporter::readDirectionalLight(XmlNode &node) {
    std::string def, use;
    float ambientIntensity = 0;
    aiColor3D color(1, 1, 1);
    aiVector3D direction(0, 0, -1);
    bool global = false;
    float intensity = 1;
    bool on = true;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getFloatAttribute(node, "ambientIntensity", ambientIntensity);
    X3DXmlHelper::getColor3DAttribute(node, "color", color);
    X3DXmlHelper::getVector3DAttribute(node, "direction", direction);
    XmlParser::getBoolAttribute(node, "global", global);
    XmlParser::getFloatAttribute(node, "intensity", intensity);
    XmlParser::getBoolAttribute(node, "on", on);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_DirectionalLight, ne);
    } else {
        if (on) {
            // create and if needed - define new geometry object.
            ne = new X3DNodeElementLight(X3DElemType::ENET_DirectionalLight, mNodeElementCur);
            if (!def.empty())
                ne->ID = def;
            else
                ne->ID = "DirectionalLight_" + ai_to_string((size_t)ne); // make random name

            ((X3DNodeElementLight *)ne)->AmbientIntensity = ambientIntensity;
            ((X3DNodeElementLight *)ne)->Color = color;
            ((X3DNodeElementLight *)ne)->Direction = direction;
            ((X3DNodeElementLight *)ne)->Global = global;
            ((X3DNodeElementLight *)ne)->Intensity = intensity;
            // Assimp want a node with name similar to a light. "Why? I don't no." )
            ParseHelper_Group_Begin(false);

            mNodeElementCur->ID = ne->ID; // assign name to node and return to light element.
            ParseHelper_Node_Exit();
            // check for child nodes
            if (!isNodeEmpty(node))
                childrenReadMetadata(node, ne, "DirectionalLight");
            else
                mNodeElementCur->Children.push_back(ne); // add made object as child to current element

            NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
        } // if(on)
    } // if(!use.empty()) else
}

// <PointLight
// DEF=""               ID
// USE=""               IDREF
// ambientIntensity="0" SFFloat [inputOutput]
// attenuation="1 0 0"  SFVec3f [inputOutput]
// color="1 1 1"        SFColor [inputOutput]
// global="true"        SFBool  [inputOutput]
// intensity="1"        SFFloat [inputOutput]
// location="0 0 0"     SFVec3f [inputOutput]
// on="true"            SFBool  [inputOutput]
// radius="100"         SFFloat [inputOutput]
// />
void X3DImporter::readPointLight(XmlNode &node) {
    std::string def, use;
    float ambientIntensity = 0;
    aiVector3D attenuation(1, 0, 0);
    aiColor3D color(1, 1, 1);
    bool global = true;
    float intensity = 1;
    aiVector3D location(0, 0, 0);
    bool on = true;
    float radius = 100;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getFloatAttribute(node, "ambientIntensity", ambientIntensity);
    X3DXmlHelper::getVector3DAttribute(node, "attenuation", attenuation);
    X3DXmlHelper::getColor3DAttribute(node, "color", color);
    XmlParser::getBoolAttribute(node, "global", global);
    XmlParser::getFloatAttribute(node, "intensity", intensity);
    X3DXmlHelper::getVector3DAttribute(node, "location", location);
    XmlParser::getBoolAttribute(node, "on", on);
    XmlParser::getFloatAttribute(node, "radius", radius);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_PointLight, ne);
    } else {
        if (on) {
            // create and if needed - define new geometry object.
            ne = new X3DNodeElementLight(X3DElemType::ENET_PointLight, mNodeElementCur);
            if (!def.empty()) ne->ID = def;

            ((X3DNodeElementLight *)ne)->AmbientIntensity = ambientIntensity;
            ((X3DNodeElementLight *)ne)->Attenuation = attenuation;
            ((X3DNodeElementLight *)ne)->Color = color;
            ((X3DNodeElementLight *)ne)->Global = global;
            ((X3DNodeElementLight *)ne)->Intensity = intensity;
            ((X3DNodeElementLight *)ne)->Location = location;
            ((X3DNodeElementLight *)ne)->Radius = radius;
            // Assimp want a node with name similar to a light. "Why? I don't no." )
            ParseHelper_Group_Begin(false);
            // make random name
            if (ne->ID.empty()) ne->ID = "PointLight_" + ai_to_string((size_t)ne);

            mNodeElementCur->ID = ne->ID; // assign name to node and return to light element.
            ParseHelper_Node_Exit();
            // check for child nodes
            if (!isNodeEmpty(node))
                childrenReadMetadata(node, ne, "PointLight");
            else
                mNodeElementCur->Children.push_back(ne); // add made object as child to current element

            NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
        } // if(on)
    } // if(!use.empty()) else
}

// <SpotLight
// DEF=""                 ID
// USE=""                 IDREF
// ambientIntensity="0"   SFFloat [inputOutput]
// attenuation="1 0 0"    SFVec3f [inputOutput]
// beamWidth="0.7854"     SFFloat [inputOutput]
// color="1 1 1"          SFColor [inputOutput]
// cutOffAngle="1.570796" SFFloat [inputOutput]
// direction="0 0 -1"     SFVec3f [inputOutput]
// global="true"          SFBool  [inputOutput]
// intensity="1"          SFFloat [inputOutput]
// location="0 0 0"       SFVec3f [inputOutput]
// on="true"              SFBool  [inputOutput]
// radius="100"           SFFloat [inputOutput]
// />
void X3DImporter::readSpotLight(XmlNode &node) {
    std::string def, use;
    float ambientIntensity = 0;
    aiVector3D attenuation(1, 0, 0);
    float beamWidth = 0.7854f;
    aiColor3D color(1, 1, 1);
    float cutOffAngle = 1.570796f;
    aiVector3D direction(0, 0, -1);
    bool global = true;
    float intensity = 1;
    aiVector3D location(0, 0, 0);
    bool on = true;
    float radius = 100;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_CHECKUSEDEF_RET(node, def, use);
    XmlParser::getFloatAttribute(node, "ambientIntensity", ambientIntensity);
    X3DXmlHelper::getVector3DAttribute(node, "attenuation", attenuation);
    XmlParser::getFloatAttribute(node, "beamWidth", beamWidth);
    X3DXmlHelper::getColor3DAttribute(node, "color", color);
    XmlParser::getFloatAttribute(node, "cutOffAngle", cutOffAngle);
    X3DXmlHelper::getVector3DAttribute(node, "direction", direction);
    XmlParser::getBoolAttribute(node, "global", global);
    XmlParser::getFloatAttribute(node, "intensity", intensity);
    X3DXmlHelper::getVector3DAttribute(node, "location", location);
    XmlParser::getBoolAttribute(node, "on", on);
    XmlParser::getFloatAttribute(node, "radius", radius);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        ne = MACRO_USE_CHECKANDAPPLY(node, def, use, ENET_SpotLight, ne);
    } else {
        if (on) {
            // create and if needed - define new geometry object.
            ne = new X3DNodeElementLight(X3DElemType::ENET_SpotLight, mNodeElementCur);
            if (!def.empty()) ne->ID = def;

            if (beamWidth > cutOffAngle) beamWidth = cutOffAngle;

            ((X3DNodeElementLight *)ne)->AmbientIntensity = ambientIntensity;
            ((X3DNodeElementLight *)ne)->Attenuation = attenuation;
            ((X3DNodeElementLight *)ne)->BeamWidth = beamWidth;
            ((X3DNodeElementLight *)ne)->Color = color;
            ((X3DNodeElementLight *)ne)->CutOffAngle = cutOffAngle;
            ((X3DNodeElementLight *)ne)->Direction = direction;
            ((X3DNodeElementLight *)ne)->Global = global;
            ((X3DNodeElementLight *)ne)->Intensity = intensity;
            ((X3DNodeElementLight *)ne)->Location = location;
            ((X3DNodeElementLight *)ne)->Radius = radius;

            // Assimp want a node with name similar to a light. "Why? I don't no." )
            ParseHelper_Group_Begin(false);
            // make random name
            if (ne->ID.empty()) ne->ID = "SpotLight_" + ai_to_string((size_t)ne);

            mNodeElementCur->ID = ne->ID; // assign name to node and return to light element.
            ParseHelper_Node_Exit();
            // check for child nodes
            if (!isNodeEmpty(node))
                childrenReadMetadata(node, ne, "SpotLight");
            else
                mNodeElementCur->Children.push_back(ne); // add made object as child to current element

            NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
        } // if(on)
    } // if(!use.empty()) else
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
