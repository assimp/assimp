/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team

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
/// \file   X3DImporter.cpp
/// \brief  X3D-format files importer for Assimp: main algorithm implementation.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef ASSIMP_BUILD_NO_X3D_IMPORTER

#include "X3DImporter.hpp"

#include <assimp/StringUtils.h>
#include <assimp/ParsingUtils.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/fast_atof.h>

// Header files, stdlib.
#include <iterator>
#include <memory>

namespace Assimp {

/// Constant which holds the importer description
const aiImporterDesc X3DImporter::Description = {
    "Extensible 3D(X3D) Importer",
    "smalcom",
    "",
    "See documentation in source code. Chapter: Limitations.",
    aiImporterFlags_SupportTextFlavour | aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_LimitedSupport | aiImporterFlags_Experimental,
    0,
    0,
    0,
    0,
    "x3d x3db"
};

struct WordIterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = const char *;
    using difference_type = ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    static const char *whitespace;
    const char *mStart, *mEnd;

    WordIterator(const char *start, const char *end) :
            mStart(start),
            mEnd(end) {
        mStart = start + ::strspn(start, whitespace);
        if (mStart >= mEnd) {
            mStart = 0;
        }
    }
    WordIterator() :
            mStart(0),
            mEnd(0) {}
    WordIterator(const WordIterator &other) :
            mStart(other.mStart),
            mEnd(other.mEnd) {}
    WordIterator &operator=(const WordIterator &other) {
        mStart = other.mStart;
        mEnd = other.mEnd;
        return *this;
    }

    bool operator==(const WordIterator &other) const { return mStart == other.mStart; }

    bool operator!=(const WordIterator &other) const { return mStart != other.mStart; }

    WordIterator &operator++() {
        mStart += strcspn(mStart, whitespace);
        mStart += strspn(mStart, whitespace);
        if (mStart >= mEnd) {
            mStart = 0;
        }
        return *this;
    }
    WordIterator operator++(int) {
        WordIterator result(*this);
        ++(*this);
        return result;
    }
    const char *operator*() const { return mStart; }
};

const char *WordIterator::whitespace = ", \t\r\n";

void skipUnsupportedNode(const std::string &pParentNodeName, XmlNode &node) {
    static const size_t Uns_Skip_Len = 192;
    static const char *Uns_Skip[Uns_Skip_Len] = {
        // CAD geometry component
        "CADAssembly", "CADFace", "CADLayer", "CADPart", "IndexedQuadSet", "QuadSet",
        // Core
        "ROUTE", "ExternProtoDeclare", "ProtoDeclare", "ProtoInstance", "ProtoInterface", "WorldInfo",
        // Distributed interactive simulation (DIS) component
        "DISEntityManager", "DISEntityTypeMapping", "EspduTransform", "ReceiverPdu", "SignalPdu", "TransmitterPdu",
        // Cube map environmental texturing component
        "ComposedCubeMapTexture", "GeneratedCubeMapTexture", "ImageCubeMapTexture",
        // Environmental effects component
        "Background", "Fog", "FogCoordinate", "LocalFog", "TextureBackground",
        // Environmental sensor component
        "ProximitySensor", "TransformSensor", "VisibilitySensor",
        // Followers component
        "ColorChaser", "ColorDamper", "CoordinateChaser", "CoordinateDamper", "OrientationChaser", "OrientationDamper", "PositionChaser", "PositionChaser2D",
        "PositionDamper", "PositionDamper2D", "ScalarChaser", "ScalarDamper", "TexCoordChaser2D", "TexCoordDamper2D",
        // Geospatial component
        "GeoCoordinate", "GeoElevationGrid", "GeoLocation", "GeoLOD", "GeoMetadata", "GeoOrigin", "GeoPositionInterpolator", "GeoProximitySensor",
        "GeoTouchSensor", "GeoTransform", "GeoViewpoint",
        // Humanoid Animation (H-Anim) component
        "HAnimDisplacer", "HAnimHumanoid", "HAnimJoint", "HAnimSegment", "HAnimSite",
        // Interpolation component
        "ColorInterpolator", "CoordinateInterpolator", "CoordinateInterpolator2D", "EaseInEaseOut", "NormalInterpolator", "OrientationInterpolator",
        "PositionInterpolator", "PositionInterpolator2D", "ScalarInterpolator", "SplinePositionInterpolator", "SplinePositionInterpolator2D",
        "SplineScalarInterpolator", "SquadOrientationInterpolator",
        // Key device sensor component
        "KeySensor", "StringSensor",
        // Layering component
        "Layer", "LayerSet", "Viewport",
        // Layout component
        "Layout", "LayoutGroup", "LayoutLayer", "ScreenFontStyle", "ScreenGroup",
        // Navigation component
        "Billboard", "Collision", "LOD", "NavigationInfo", "OrthoViewpoint", "Viewpoint", "ViewpointGroup",
        // Networking component
        "EXPORT", "IMPORT", "Anchor", "LoadSensor",
        // NURBS component
        "Contour2D", "ContourPolyline2D", "CoordinateDouble", "NurbsCurve", "NurbsCurve2D", "NurbsOrientationInterpolator", "NurbsPatchSurface",
        "NurbsPositionInterpolator", "NurbsSet", "NurbsSurfaceInterpolator", "NurbsSweptSurface", "NurbsSwungSurface", "NurbsTextureCoordinate",
        "NurbsTrimmedSurface",
        // Particle systems component
        "BoundedPhysicsModel", "ConeEmitter", "ExplosionEmitter", "ForcePhysicsModel", "ParticleSystem", "PointEmitter", "PolylineEmitter", "SurfaceEmitter",
        "VolumeEmitter", "WindPhysicsModel",
        // Picking component
        "LinePickSensor", "PickableGroup", "PointPickSensor", "PrimitivePickSensor", "VolumePickSensor",
        // Pointing device sensor component
        "CylinderSensor", "PlaneSensor", "SphereSensor", "TouchSensor",
        // Rendering component
        "ClipPlane",
        // Rigid body physics
        "BallJoint", "CollidableOffset", "CollidableShape", "CollisionCollection", "CollisionSensor", "CollisionSpace", "Contact", "DoubleAxisHingeJoint",
        "MotorJoint", "RigidBody", "RigidBodyCollection", "SingleAxisHingeJoint", "SliderJoint", "UniversalJoint",
        // Scripting component
        "Script",
        // Programmable shaders component
        "ComposedShader", "FloatVertexAttribute", "Matrix3VertexAttribute", "Matrix4VertexAttribute", "PackagedShader", "ProgramShader", "ShaderPart",
        "ShaderProgram",
        // Shape component
        "FillProperties", "LineProperties", "TwoSidedMaterial",
        // Sound component
        "AudioClip", "Sound",
        // Text component
        "FontStyle", "Text",
        // Texturing3D Component
        "ComposedTexture3D", "ImageTexture3D", "PixelTexture3D", "TextureCoordinate3D", "TextureCoordinate4D", "TextureTransformMatrix3D", "TextureTransform3D",
        // Texturing component
        "MovieTexture", "MultiTexture", "MultiTextureCoordinate", "MultiTextureTransform", "PixelTexture", "TextureCoordinateGenerator", "TextureProperties",
        // Time component
        "TimeSensor",
        // Event Utilities component
        "BooleanFilter", "BooleanSequencer", "BooleanToggle", "BooleanTrigger", "IntegerSequencer", "IntegerTrigger", "TimeTrigger",
        // Volume rendering component
        "BlendedVolumeStyle", "BoundaryEnhancementVolumeStyle", "CartoonVolumeStyle", "ComposedVolumeStyle", "EdgeEnhancementVolumeStyle", "IsoSurfaceVolumeData",
        "OpacityMapVolumeStyle", "ProjectionVolumeStyle", "SegmentedVolumeData", "ShadedVolumeStyle", "SilhouetteEnhancementVolumeStyle", "ToneMappedVolumeStyle",
        "VolumeData"
    };

    const std::string nn = node.name();
    bool found = false;
    bool close_found = false;

    for (size_t i = 0; i < Uns_Skip_Len; i++) {
        if (nn == Uns_Skip[i]) {
            found = true;
            if (node.empty()) {
                close_found = true;
                break;
            }
        }
    }

    if (!found) throw DeadlyImportError("Unknown node \"" + nn + "\" in " + pParentNodeName + ".");

    LogInfo("Skipping node \"" + nn + "\" in " + pParentNodeName + ".");
}

X3DImporter::X3DImporter() :
        mNodeElementCur(nullptr),
        mScene(nullptr) {
    // empty
}

X3DImporter::~X3DImporter() {
    // Clear() is accounting if data already is deleted. So, just check again if all data is deleted.
    Clear();
}

void X3DImporter::Clear() {
    mNodeElementCur = nullptr;
    // Delete all elements
    if (!NodeElement_List.empty()) {
        for (std::list<X3DNodeElementBase *>::iterator it = NodeElement_List.begin(); it != NodeElement_List.end(); ++it) {
            delete *it;
        }
        NodeElement_List.clear();
    }
}

void X3DImporter::ParseFile(const std::string &file, IOSystem *pIOHandler) {
    ai_assert(nullptr != pIOHandler);

    static const std::string mode = "rb";
    std::unique_ptr<IOStream> fileStream(pIOHandler->Open(file, mode));
    if (!fileStream.get()) {
        throw DeadlyImportError("Failed to open file " + file + ".");
    }

    XmlParser theParser;
    if (!theParser.parse(fileStream.get())) {
        return;
    }

    XmlNode *node = theParser.findNode("X3D");
    if (nullptr == node) {
        return;
    }

    for (auto &currentNode : node->children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "head") {
            readMetadata(currentNode);
        } else if (currentName == "Scene") {
            readScene(currentNode);
        }
    }
}

bool X3DImporter::CanRead(const std::string &pFile, IOSystem * /*pIOHandler*/, bool checkSig) const {
    if (checkSig) {
        std::string::size_type pos = pFile.find_last_of(".x3d");
        if (pos != std::string::npos) {
            return true;
        }
    }

    return false;
}

void X3DImporter::InternReadFile( const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler ) {
    std::shared_ptr<IOStream> stream(pIOHandler->Open(pFile, "rb"));
    if (!stream) {
        throw DeadlyImportError("Could not open file for reading");
    }
    std::string::size_type slashPos = pFile.find_last_of("\\/");

    pIOHandler->PushDirectory(slashPos == std::string::npos ? std::string() : pFile.substr(0, slashPos + 1));
    ParseFile(pFile, pIOHandler);
    pIOHandler->PopDirectory();

    //
    mScene = pScene;
    pScene->mRootNode = new aiNode(pFile);
    pScene->mRootNode->mParent = nullptr;
    pScene->mFlags |= AI_SCENE_FLAGS_ALLOW_SHARED;

    // search for root node element
    mNodeElementCur = NodeElement_List.front();
    while (mNodeElementCur->Parent != nullptr) {
        mNodeElementCur = mNodeElementCur->Parent;
    }

    { // fill aiScene with objects.
        std::list<aiMesh *> mesh_list;
        std::list<aiMaterial *> mat_list;
        std::list<aiLight *> light_list;

        // create nodes tree
        Postprocess_BuildNode(*mNodeElementCur, *pScene->mRootNode, mesh_list, mat_list, light_list);
        // copy needed data to scene
        if (!mesh_list.empty()) {
            std::list<aiMesh *>::const_iterator it = mesh_list.begin();

            pScene->mNumMeshes = static_cast<unsigned int>(mesh_list.size());
            pScene->mMeshes = new aiMesh *[pScene->mNumMeshes];
            for (size_t i = 0; i < pScene->mNumMeshes; i++)
                pScene->mMeshes[i] = *it++;
        }

        if (!mat_list.empty()) {
            std::list<aiMaterial *>::const_iterator it = mat_list.begin();

            pScene->mNumMaterials = static_cast<unsigned int>(mat_list.size());
            pScene->mMaterials = new aiMaterial *[pScene->mNumMaterials];
            for (size_t i = 0; i < pScene->mNumMaterials; i++)
                pScene->mMaterials[i] = *it++;
        }

        if (!light_list.empty()) {
            std::list<aiLight *>::const_iterator it = light_list.begin();

            pScene->mNumLights = static_cast<unsigned int>(light_list.size());
            pScene->mLights = new aiLight *[pScene->mNumLights];
            for (size_t i = 0; i < pScene->mNumLights; i++)
                pScene->mLights[i] = *it++;
        }
    }
}

const aiImporterDesc *X3DImporter::GetInfo() const {
    return &Description;
}

struct meta_entry {
    std::string name;
    std::string value;
};

void X3DImporter::readMetadata(XmlNode &node) {
    std::vector<meta_entry> metaArray;
    for (auto currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "meta") {
            meta_entry entry;
            if (XmlParser::getStdStrAttribute(currentNode, "name", entry.name)) {
                XmlParser::getStdStrAttribute(currentNode, "content", entry.value);
                metaArray.emplace_back(entry);
            }
        }
    }
    mScene->mMetaData = aiMetadata::Alloc(static_cast<unsigned int>(metaArray.size()));
    unsigned int i = 0;
    for (auto currentMeta : metaArray) {
        mScene->mMetaData->Set(i, currentMeta.name, currentMeta.value);
        ++i;
    }
}

void X3DImporter::readScene(XmlNode &node) {
    for (auto currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "Viewpoint") {
            readViewpoint(currentNode);
        }
    }
}

void X3DImporter::readViewpoint(XmlNode &node) {
    for (auto currentNode : node.children()) {
        //const std::string &currentName = currentNode.name();
    }
}

void readMetadataBoolean(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaBoolean *boolean = nullptr;
    if (XmlParser::getStdStrAttribute(node, "value", val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        boolean = new X3DNodeElementMetaBoolean(parent);
        for (size_t i = 0; i < values.size(); ++i) {
            bool current_boolean = false;
            if (values[i] == "true") {
                current_boolean = true;
            }
            boolean->Value.emplace_back(current_boolean);
        }
    }
}

void readMetadataDouble(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaDouble *doubleNode = nullptr;
    if (XmlParser::getStdStrAttribute(node, "value", val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        doubleNode = new X3DNodeElementMetaDouble(parent);
        for (size_t i = 0; i < values.size(); ++i) {
            double current_double = static_cast<double>(fast_atof(values[i].c_str()));
            doubleNode->Value.emplace_back(current_double);
        }
    }
}

void readMetadataFloat(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaFloat *floatNode = nullptr;
    if (XmlParser::getStdStrAttribute(node, "value", val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        floatNode = new X3DNodeElementMetaFloat(parent);
        for (size_t i = 0; i < values.size(); ++i) {
            float current_float = static_cast<float>(fast_atof(values[i].c_str()));
            floatNode->Value.emplace_back(current_float);
        }
    }
}

void readMetadataInteger(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaInt *intNode = nullptr;
    if (XmlParser::getStdStrAttribute(node, "value", val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        intNode = new X3DNodeElementMetaInt(parent);
        for (size_t i = 0; i < values.size(); ++i) {
            int current_int = static_cast<int>(std::atoi(values[i].c_str()));
            intNode->Value.emplace_back(current_int);
        }
    }
}

void readMetadataSet(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaSet *setNode = new X3DNodeElementMetaSet(parent);
    if (XmlParser::getStdStrAttribute(node, "name", val)) {
        setNode->Name = val;
    }

    if (XmlParser::getStdStrAttribute(node, "reference", val)) {
        setNode->Reference = val;
    }
}

void readMetadataString(XmlNode &node, X3DNodeElementBase *parent) {
    std::string val;
    X3DNodeElementMetaString *strNode = nullptr;
    if (XmlParser::getStdStrAttribute(node, "value", val)) {
        std::vector<std::string> values;
        tokenize<std::string>(val, values, " ");
        strNode = new X3DNodeElementMetaString(parent);
        for (size_t i = 0; i < values.size(); ++i) {
            strNode->Value.emplace_back(values[i]);
        }
    }
}

static void parseColor(const std::string &value, aiColor3D &color) {
    std::vector<std::string> values;
    tokenize<std::string>(value, values, " ");
    for (unsigned int i = 0; i < values.size(); ++i) {
        color[i] = static_cast<ai_real>(atof(values[i].c_str()));
    }
}

static void parseVector3(const std::string &value, aiVector3D &vector) {
    std::vector<std::string> values;
    tokenize<std::string>(value, values, " ");
    for (unsigned int i = 0; i < values.size(); ++i) {
        vector[i] = static_cast<ai_real>(atof(values[i].c_str()));
    }
}

static void getUseAndDef(XmlNode &node, std::string &use, std::string &def) {
    XmlParser::getStdStrAttribute(node, "use", use);
    XmlParser::getStdStrAttribute(node, "def", def);
}

void X3DImporter::ParseDirectionalLight(XmlNode &node) {
    std::string def, use, id, value;
    float ambientIntensity = 0;
    aiColor3D color(1, 1, 1);
    aiVector3D direction(0, 0, -1);
    bool global = false;
    float intensity = 1;
    bool on = true;
    X3DNodeElementBase *ne = nullptr;

    getUseAndDef(node, use, def);
    XmlParser::getStdStrAttribute(node, "id", id);
    if (XmlParser::getStdStrAttribute(node, "color", value)) {
        parseColor(value, color);
    }

    if (XmlParser::getStdStrAttribute(node, "direction", value)) {
        parseVector3(value, direction);
    }

    XmlParser::getFloatAttribute(node, "ambientIntensity", ambientIntensity);
    XmlParser::getBoolAttribute(node, "global", global);
    XmlParser::getFloatAttribute(node, "intensity", intensity);
    XmlParser::getBoolAttribute(node, "on", on);

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        //MACRO_USE_CHECKANDAPPLY(def, use, ENET_DirectionalLight, ne);
    } else {
        if (on) {
            // create and if needed - define new geometry object.
            ne = new X3DNodeNodeElementLight(X3DElemType::ENET_DirectionalLight, mNodeElementCur);
            if (!def.empty())
                ne->ID = def;
            else
                ne->ID = "DirectionalLight_" + ai_to_string((size_t)ne); // make random name

            ((X3DNodeNodeElementLight *)ne)->AmbientIntensity = ambientIntensity;
            ((X3DNodeNodeElementLight *)ne)->Color = color;
            ((X3DNodeNodeElementLight *)ne)->Direction = direction;
            ((X3DNodeNodeElementLight *)ne)->Global = global;
            ((X3DNodeNodeElementLight *)ne)->Intensity = intensity;

            mNodeElementCur->ID = ne->ID; // assign name to node and return to light element.
            mNodeElementCur->Children.push_back(ne); // add made object as child to current element
            readMetadata(node);
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
void X3DImporter::ParseNode_Lighting_PointLight(XmlNode &node) {
    std::string def, use, value;
    getUseAndDef(node, use, def);

    float ambientIntensity = 0;
    aiVector3D attenuation(1, 0, 0);
    aiVector3D attenuation(1, 0, 0);
    aiColor3D color(1, 1, 1);
    bool global = true;
    float intensity = 1;
    aiVector3D location(0, 0, 0);
    bool on = true;
    float radius = 100;
    X3DNodeElementBase *ne = nullptr;

    //MACRO_ATTRREAD_LOOPBEG;
    //MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    XmlParser::getFloatAttribute(node, "ambientIntensity", ambientIntensity);
    if (XmlParser::getStdStrAttribute(node, "attenuation", value)) {
        parseVector3(value, attenuation);
    }
    if (XmlParser::getStdStrAttribute(node, "color", value)) {
        parseColor(value, color);
    }
    XmlParser::getBoolAttribute(node, "global", global);
    XmlParser::getFloatAttribute(node, "intensity", intensity);
    if (XmlParser::getStdStrAttribute(node, "location", value)) {
        parseVector3(value, location);
    }
    XmlParser::getBoolAttribute(node, "on", on);
    XmlParser::getFloatAttribute(node, "radius", radius);
    //MACRO_ATTRREAD_CHECK_RET("ambientIntensity", ambientIntensity, XML_ReadNode_GetAttrVal_AsFloat);
    
    //MACRO_ATTRREAD_CHECK_REF("attenuation", attenuation, XML_ReadNode_GetAttrVal_AsVec3f);
    //MACRO_ATTRREAD_CHECK_REF("color", color, XML_ReadNode_GetAttrVal_AsCol3f);
    //MACRO_ATTRREAD_CHECK_RET("global", global, XML_ReadNode_GetAttrVal_AsBool);
    //MACRO_ATTRREAD_CHECK_RET("intensity", intensity, XML_ReadNode_GetAttrVal_AsFloat);
    //MACRO_ATTRREAD_CHECK_REF("location", location, XML_ReadNode_GetAttrVal_AsVec3f);
    //MACRO_ATTRREAD_CHECK_RET("on", on, XML_ReadNode_GetAttrVal_AsBool);
    //MACRO_ATTRREAD_CHECK_RET("radius", radius, XML_ReadNode_GetAttrVal_AsFloat);
    //MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        mNodeElementCur->Children.push_back(ne);
        //MACRO_USE_CHECKANDAPPLY(def, use, ENET_PointLight, ne);
    } else {
        if (on) {
            // create and if needed - define new geometry object.
            ne = new X3DNodeNodeElementLight(X3DElemType::ENET_PointLight, mNodeElementCur);
            if (!def.empty()) ne->ID = def;

            ((X3DNodeNodeElementLight *)ne)->AmbientIntensity = ambientIntensity;
            ((X3DNodeNodeElementLight *)ne)->Attenuation = attenuation;
            ((X3DNodeNodeElementLight *)ne)->Color = color;
            ((X3DNodeNodeElementLight *)ne)->Global = global;
            ((X3DNodeNodeElementLight *)ne)->Intensity = intensity;
            ((X3DNodeNodeElementLight *)ne)->Location = location;
            ((X3DNodeNodeElementLight *)ne)->Radius = radius;
            // Assimp want a node with name similar to a light. "Why? I don't no." )
          //  ParseHelper_Group_Begin(false);
            // make random name
            if (ne->ID.empty()) {
                ne->ID = "PointLight_" + ai_to_string((size_t)ne);
            }

            mNodeElementCur->ID = ne->ID; // assign name to node and return to light element.
            //ParseHelper_Node_Exit();
            // check for child nodes
            //if (!mReader->isEmptyElement())
            readMetadata(node);
                //ParseNode_Metadata(ne, "PointLight");
            //else
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
void X3DImporter::ParseNode_Lighting_SpotLight(XmlNode & node) {
    std::string def, use, value;
    getUseAndDef(node, use, def);

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
    X3DNodeElementBase *ne = nullptr;

    //MACRO_ATTRREAD_LOOPBEG;
    //MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    XmlParser::getFloatAttribute(node, "ambientIntensity", ambientIntensity);
    //MACRO_ATTRREAD_CHECK_RET("ambientIntensity", ambientIntensity, XML_ReadNode_GetAttrVal_AsFloat);
    if (XmlParser::getStdStrAttribute(node, "attenuation", value)) {
        parseVector3(value, attenuation);
    }
    XmlParser::getFloatAttribute(node, "beamWidth", beamWidth);
    //MACRO_ATTRREAD_CHECK_REF("attenuation", attenuation, XML_ReadNode_GetAttrVal_AsVec3f);
    //MACRO_ATTRREAD_CHECK_RET("beamWidth", beamWidth, XML_ReadNode_GetAttrVal_AsFloat);
    if (XmlParser::getStdStrAttribute(node, "color", value)) {
        parseColor(value, color);
    }
    XmlParser::getFloatAttribute(node, "cutOffAngle", cutOffAngle);
    if (XmlParser::getStdStrAttribute(node, "direction", value)) {
        parseVector3(value, direction);
    }
    XmlParser::getBoolAttribute(node, "global", global);
    XmlParser::getFloatAttribute(node, "intensity", intensity);
    if (XmlParser::getStdStrAttribute(node, "location", value)) {
        parseVector3(value, location);
    }
    XmlParser::getBoolAttribute(node, "on", on);
    XmlParser::getFloatAttribute(node, "radius", radius);
    //    MACRO_ATTRREAD_CHECK_REF("color", color, XML_ReadNode_GetAttrVal_AsCol3f);
    //MACRO_ATTRREAD_CHECK_RET("cutOffAngle", cutOffAngle, XML_ReadNode_GetAttrVal_AsFloat);
    //MACRO_ATTRREAD_CHECK_REF("direction", direction, XML_ReadNode_GetAttrVal_AsVec3f);
    //MACRO_ATTRREAD_CHECK_RET("global", global, XML_ReadNode_GetAttrVal_AsBool);
    //MACRO_ATTRREAD_CHECK_RET("intensity", intensity, XML_ReadNode_GetAttrVal_AsFloat);
    //MACRO_ATTRREAD_CHECK_REF("location", location, XML_ReadNode_GetAttrVal_AsVec3f);
    //MACRO_ATTRREAD_CHECK_RET("on", on, XML_ReadNode_GetAttrVal_AsBool);
    //MACRO_ATTRREAD_CHECK_RET("radius", radius, XML_ReadNode_GetAttrVal_AsFloat);
    //MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        mNodeElementCur->Children.push_back(ne);
        //MACRO_USE_CHECKANDAPPLY(def, use, ENET_SpotLight, ne);
    } else {
        if (on) {
            // create and if needed - define new geometry object.
            ne = new X3DNodeNodeElementLight(X3DElemType::ENET_SpotLight, mNodeElementCur);
            if (!def.empty())
                ne->ID = def;

            if (beamWidth > cutOffAngle)
                beamWidth = cutOffAngle;

            ((X3DNodeNodeElementLight *)ne)->AmbientIntensity = ambientIntensity;
            ((X3DNodeNodeElementLight *)ne)->Attenuation = attenuation;
            ((X3DNodeNodeElementLight *)ne)->BeamWidth = beamWidth;
            ((X3DNodeNodeElementLight *)ne)->Color = color;
            ((X3DNodeNodeElementLight *)ne)->CutOffAngle = cutOffAngle;
            ((X3DNodeNodeElementLight *)ne)->Direction = direction;
            ((X3DNodeNodeElementLight *)ne)->Global = global;
            ((X3DNodeNodeElementLight *)ne)->Intensity = intensity;
            ((X3DNodeNodeElementLight *)ne)->Location = location;
            ((X3DNodeNodeElementLight *)ne)->Radius = radius;

            // Assimp want a node with name similar to a light. "Why? I don't no." )
//            ParseHelper_Group_Begin(false);
            // make random name
            if (ne->ID.empty()) ne->ID = "SpotLight_" + ai_to_string((size_t)ne);

            mNodeElementCur->ID = ne->ID; // assign name to node and return to light element.
            //ParseHelper_Node_Exit();
            // check for child nodes
//            if (!mReader->isEmptyElement())
                readMetadata(node);

  //              ParseNode_Metadata(ne, "SpotLight");
    //        else
                mNodeElementCur->Children.push_back(ne); // add made object as child to current element

            NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
        } // if(on)
    } // if(!use.empty()) else
}

void X3DImporter::ParseNode_Grouping_Group() {
    std::string def, use;

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        X3DNodeElementBase *ne = nullptr;

        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Group, ne);
    } else {
        ParseHelper_Group_Begin(); // create new grouping element and go deeper if node has children.
        // at this place new group mode created and made current, so we can name it.
        if (!def.empty()) mNodeElementCur->ID = def;
        // in grouping set of nodes check X3DMetadataObject is not needed, because it is done in <Scene> parser function.

        // for empty element exit from node in that place
        if (mReader->isEmptyElement()) ParseHelper_Node_Exit();
    } // if(!use.empty()) else
}

void X3DImporter::ParseNode_Grouping_GroupEnd() {
    ParseHelper_Node_Exit(); // go up in scene graph
}

// <StaticGroup
// DEF=""              ID
// USE=""              IDREF
// bboxCenter="0 0 0"  SFVec3f [initializeOnly]
// bboxSize="-1 -1 -1" SFVec3f [initializeOnly]
// >
//    <!-- ChildContentModel -->
// ChildContentModel is the child-node content model corresponding to X3DChildNode, combining all profiles. ChildContentModel can contain most nodes,
// other Grouping nodes, Prototype declarations and ProtoInstances in any order and any combination. When the assigned profile is less than Full, the
// precise palette of legal nodes that are available depends on assigned profile and components.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </StaticGroup>
// The StaticGroup node contains children nodes which cannot be modified. StaticGroup children are guaranteed to not change, send events, receive events or
// contain any USE references outside the StaticGroup.
void X3DImporter::ParseNode_Grouping_StaticGroup() {
    std::string def, use;

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        X3DNodeElementBase *ne = nullptr;

        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Group, ne);
    } else {
        ParseHelper_Group_Begin(true); // create new grouping element and go deeper if node has children.
        // at this place new group mode created and made current, so we can name it.
        if (!def.empty()) mNodeElementCur->ID = def;
        // in grouping set of nodes check X3DMetadataObject is not needed, because it is done in <Scene> parser function.

        // for empty element exit from node in that place
        if (mReader->isEmptyElement()) ParseHelper_Node_Exit();
    } // if(!use.empty()) else
}

void X3DImporter::ParseNode_Grouping_StaticGroupEnd() {
    ParseHelper_Node_Exit(); // go up in scene graph
}

// <Switch
// DEF=""              ID
// USE=""              IDREF
// bboxCenter="0 0 0"  SFVec3f [initializeOnly]
// bboxSize="-1 -1 -1" SFVec3f [initializeOnly]
// whichChoice="-1"    SFInt32 [inputOutput]
// >
//    <!-- ChildContentModel -->
// ChildContentModel is the child-node content model corresponding to X3DChildNode, combining all profiles. ChildContentModel can contain most nodes,
// other Grouping nodes, Prototype declarations and ProtoInstances in any order and any combination. When the assigned profile is less than Full, the
// precise palette of legal nodes that are available depends on assigned profile and components.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </Switch>
// The Switch grouping node traverses zero or one of the nodes specified in the children field. The whichChoice field specifies the index of the child
// to traverse, with the first child having index 0. If whichChoice is less than zero or greater than the number of nodes in the children field, nothing
// is chosen.
void X3DImporter::ParseNode_Grouping_Switch() {
    std::string def, use;
    int32_t whichChoice = -1;

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("whichChoice", whichChoice, XML_ReadNode_GetAttrVal_AsI32);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        CX3DImporter_NodeElement *ne;

        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Group, ne);
    } else {
        ParseHelper_Group_Begin(); // create new grouping element and go deeper if node has children.
        // at this place new group mode created and made current, so we can name it.
        if (!def.empty()) NodeElement_Cur->ID = def;

        // also set values specific to this type of group
        ((CX3DNodeElementGroup *)NodeElement_Cur)->UseChoice = true;
        ((CX3DNodeElementGroup *)NodeElement_Cur)->Choice = whichChoice;
        // in grouping set of nodes check X3DMetadataObject is not needed, because it is done in <Scene> parser function.

        // for empty element exit from node in that place
        if (mReader->isEmptyElement()) ParseHelper_Node_Exit();
    } // if(!use.empty()) else
}

void X3DImporter::ParseNode_Grouping_SwitchEnd() {
    // just exit from node. Defined choice will be accepted at postprocessing stage.
    ParseHelper_Node_Exit(); // go up in scene graph
}

// <Transform
// DEF=""                     ID
// USE=""                     IDREF
// bboxCenter="0 0 0"         SFVec3f    [initializeOnly]
// bboxSize="-1 -1 -1"        SFVec3f    [initializeOnly]
// center="0 0 0"             SFVec3f    [inputOutput]
// rotation="0 0 1 0"         SFRotation [inputOutput]
// scale="1 1 1"              SFVec3f    [inputOutput]
// scaleOrientation="0 0 1 0" SFRotation [inputOutput]
// translation="0 0 0"        SFVec3f    [inputOutput]
// >
//    <!-- ChildContentModel -->
// ChildContentModel is the child-node content model corresponding to X3DChildNode, combining all profiles. ChildContentModel can contain most nodes,
// other Grouping nodes, Prototype declarations and ProtoInstances in any order and any combination. When the assigned profile is less than Full, the
// precise palette of legal nodes that are available depends on assigned profile and components.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </Transform>
// The Transform node is a grouping node that defines a coordinate system for its children that is relative to the coordinate systems of its ancestors.
// Given a 3-dimensional point P and Transform node, P is transformed into point P' in its parent's coordinate system by a series of intermediate
// transformations. In matrix transformation notation, where C (center), SR (scaleOrientation), T (translation), R (rotation), and S (scale) are the
// equivalent transformation matrices,
//   P' = T * C * R * SR * S * -SR * -C * P
void X3DImporter::ParseNode_Grouping_Transform() {
    aiVector3D center(0, 0, 0);
    float rotation[4] = { 0, 0, 1, 0 };
    aiVector3D scale(1, 1, 1); // A value of zero indicates that any child geometry shall not be displayed
    float scale_orientation[4] = { 0, 0, 1, 0 };
    aiVector3D translation(0, 0, 0);
    aiMatrix4x4 matr, tmatr;
    std::string use, def;

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_REF("center", center, XML_ReadNode_GetAttrVal_AsVec3f);
    MACRO_ATTRREAD_CHECK_REF("scale", scale, XML_ReadNode_GetAttrVal_AsVec3f);
    MACRO_ATTRREAD_CHECK_REF("translation", translation, XML_ReadNode_GetAttrVal_AsVec3f);
    if (an == "rotation") {
        std::vector<float> tvec;

        XML_ReadNode_GetAttrVal_AsArrF(idx, tvec);
        if (tvec.size() != 4) throw DeadlyImportError("<Transform>: rotation vector must have 4 elements.");

        memcpy(rotation, tvec.data(), sizeof(rotation));

        continue;
    }

    if (an == "scaleOrientation") {
        std::vector<float> tvec;
        XML_ReadNode_GetAttrVal_AsArrF(idx, tvec);
        if (tvec.size() != 4) {
            throw DeadlyImportError("<Transform>: scaleOrientation vector must have 4 elements.");
        }

        ::memcpy(scale_orientation, tvec.data(), sizeof(scale_orientation));

        continue;
    }

    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        CX3DImporter_NodeElement *ne(nullptr);

        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Group, ne);
    } else {
        ParseHelper_Group_Begin(); // create new grouping element and go deeper if node has children.
        // at this place new group mode created and made current, so we can name it.
        if (!def.empty()) {
            NodeElement_Cur->ID = def;
        }

        //
        // also set values specific to this type of group
        //
        // calculate transformation matrix
        aiMatrix4x4::Translation(translation, matr); // T
        aiMatrix4x4::Translation(center, tmatr); // C
        matr *= tmatr;
        aiMatrix4x4::Rotation(rotation[3], aiVector3D(rotation[0], rotation[1], rotation[2]), tmatr); // R
        matr *= tmatr;
        aiMatrix4x4::Rotation(scale_orientation[3], aiVector3D(scale_orientation[0], scale_orientation[1], scale_orientation[2]), tmatr); // SR
        matr *= tmatr;
        aiMatrix4x4::Scaling(scale, tmatr); // S
        matr *= tmatr;
        aiMatrix4x4::Rotation(-scale_orientation[3], aiVector3D(scale_orientation[0], scale_orientation[1], scale_orientation[2]), tmatr); // -SR
        matr *= tmatr;
        aiMatrix4x4::Translation(-center, tmatr); // -C
        matr *= tmatr;
        // and assign it
        ((CX3DNodeElementGroup *)mNodeElementCur)->Transformation = matr;
        // in grouping set of nodes check X3DMetadataObject is not needed, because it is done in <Scene> parser function.

        // for empty element exit from node in that place
        if (mReader->isEmptyElement()) {
            ParseHelper_Node_Exit();
        }
    } // if(!use.empty()) else
}

void X3DImporter::ParseNode_Grouping_TransformEnd() {
    ParseHelper_Node_Exit(); // go up in scene graph
}

void X3DImporter::ParseNode_Geometry2D_Arc2D() {
    std::string def, use;
    float endAngle = AI_MATH_HALF_PI_F;
    float radius = 1;
    float startAngle = 0;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("endAngle", endAngle, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_RET("radius", radius, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_RET("startAngle", startAngle, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Arc2D, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry2D(CX3DImporter_NodeElement::ENET_Arc2D, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        // create point list of geometry object and convert it to line set.
        std::list<aiVector3D> tlist;

        GeometryHelper_Make_Arc2D(startAngle, endAngle, radius, 10, tlist); ///TODO: IME - AI_CONFIG for NumSeg
        GeometryHelper_Extend_PointToLine(tlist, ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices);
        ((CX3DImporter_NodeElement_Geometry2D *)ne)->NumIndices = 2;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Arc2D");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <ArcClose2D
// DEF=""              ID
// USE=""              IDREF
// closureType="PIE"   SFString [initializeOnly], {"PIE", "CHORD"}
// endAngle="1.570796" SFFloat  [initializeOnly]
// radius="1"          SFFloat  [initializeOnly]
// solid="false"       SFBool   [initializeOnly]
// startAngle="0"      SFFloat  [initializeOnly]
// />
// The ArcClose node specifies a portion of a circle whose center is at (0,0) and whose angles are measured starting at the positive x-axis and sweeping
// towards the positive y-axis. The end points of the arc specified are connected as defined by the closureType field. The radius field specifies the radius
// of the circle of which the arc is a portion. The arc extends from the startAngle counterclockwise to the endAngle. The value of radius shall be greater
// than zero. The values of startAngle and endAngle shall be in the range [-2pi, 2pi] radians (or the equivalent if a different default angle base unit has
// been specified). If startAngle and endAngle have the same value, a circle is specified and closureType is ignored. If the absolute difference between
// startAngle and endAngle is greater than or equal to 2pi, a complete circle is produced with no chord or radial line(s) drawn from the center.
// A closureType of "PIE" connects the end point to the start point by defining two straight line segments first from the end point to the center and then
// the center to the start point. A closureType of "CHORD" connects the end point to the start point by defining a straight line segment from the end point
// to the start point. Textures are applied individually to each face of the ArcClose2D. On the front (+Z) and back (-Z) faces of the ArcClose2D, when
// viewed from the +Z-axis, the texture is mapped onto each face with the same orientation as if the image were displayed normally in 2D.
void X3DImporter::ParseNode_Geometry2D_ArcClose2D() {
    std::string def, use;
    std::string closureType("PIE");
    float endAngle = AI_MATH_HALF_PI_F;
    float radius = 1;
    bool solid = false;
    float startAngle = 0;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("closureType", closureType, mReader->getAttributeValue);
    MACRO_ATTRREAD_CHECK_RET("endAngle", endAngle, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_RET("radius", radius, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("startAngle", startAngle, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_ArcClose2D, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry2D(CX3DImporter_NodeElement::ENET_ArcClose2D, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        ((CX3DImporter_NodeElement_Geometry2D *)ne)->Solid = solid;
        // create point list of geometry object.
        GeometryHelper_Make_Arc2D(startAngle, endAngle, radius, 10, ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices); ///TODO: IME - AI_CONFIG for NumSeg
        // add chord or two radiuses only if not a circle was defined
        if (!((std::fabs(endAngle - startAngle) >= AI_MATH_TWO_PI_F) || (endAngle == startAngle))) {
            std::list<aiVector3D> &vlist = ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices; // just short alias.

            if ((closureType == "PIE") || (closureType == "\"PIE\""))
                vlist.push_back(aiVector3D(0, 0, 0)); // center point - first radial line
            else if ((closureType != "CHORD") && (closureType != "\"CHORD\""))
                Throw_IncorrectAttrValue("closureType");

            vlist.push_back(*vlist.begin()); // arc first point - chord from first to last point of arc(if CHORD) or second radial line(if PIE).
        }

        ((CX3DImporter_NodeElement_Geometry2D *)ne)->NumIndices = ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices.size();
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "ArcClose2D");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Circle2D
// DEF=""     ID
// USE=""     IDREF
// radius="1" SFFloat  [initializeOnly]
// />
void X3DImporter::ParseNode_Geometry2D_Circle2D() {
    std::string def, use;
    float radius = 1;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("radius", radius, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Circle2D, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry2D(CX3DImporter_NodeElement::ENET_Circle2D, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        // create point list of geometry object and convert it to line set.
        std::list<aiVector3D> tlist;

        GeometryHelper_Make_Arc2D(0, 0, radius, 10, tlist); ///TODO: IME - AI_CONFIG for NumSeg
        GeometryHelper_Extend_PointToLine(tlist, ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices);
        ((CX3DImporter_NodeElement_Geometry2D *)ne)->NumIndices = 2;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Circle2D");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Disk2D
// DEF=""          ID
// USE=""          IDREF
// innerRadius="0" SFFloat  [initializeOnly]
// outerRadius="1" SFFloat  [initializeOnly]
// solid="false"   SFBool   [initializeOnly]
// />
// The Disk2D node specifies a circular disk which is centred at (0, 0) in the local coordinate system. The outerRadius field specifies the radius of the
// outer dimension of the Disk2D. The innerRadius field specifies the inner dimension of the Disk2D. The value of outerRadius shall be greater than zero.
// The value of innerRadius shall be greater than or equal to zero and less than or equal to outerRadius. If innerRadius is zero, the Disk2D is completely
// filled. Otherwise, the area within the innerRadius forms a hole in the Disk2D. If innerRadius is equal to outerRadius, a solid circular line shall
// be drawn using the current line properties. Textures are applied individually to each face of the Disk2D. On the front (+Z) and back (-Z) faces of
// the Disk2D, when viewed from the +Z-axis, the texture is mapped onto each face with the same orientation as if the image were displayed normally in 2D.
void X3DImporter::ParseNode_Geometry2D_Disk2D() {
    std::string def, use;
    float innerRadius = 0;
    float outerRadius = 1;
    bool solid = false;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("innerRadius", innerRadius, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_RET("outerRadius", outerRadius, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Disk2D, ne);
    } else {
        std::list<aiVector3D> tlist_o, tlist_i;

        if (innerRadius > outerRadius) Throw_IncorrectAttrValue("innerRadius");

        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry2D(CX3DImporter_NodeElement::ENET_Disk2D, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        // create point list of geometry object.
        ///TODO: IME - AI_CONFIG for NumSeg
        GeometryHelper_Make_Arc2D(0, 0, outerRadius, 10, tlist_o); // outer circle
        if (innerRadius == 0.0f) { // make filled disk
            // in tlist_o we already have points of circle. just copy it and sign as polygon.
            ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices = tlist_o;
            ((CX3DImporter_NodeElement_Geometry2D *)ne)->NumIndices = tlist_o.size();
        } else if (innerRadius == outerRadius) { // make circle
            // in tlist_o we already have points of circle. convert it to line set.
            GeometryHelper_Extend_PointToLine(tlist_o, ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices);
            ((CX3DImporter_NodeElement_Geometry2D *)ne)->NumIndices = 2;
        } else { // make disk
            std::list<aiVector3D> &vlist = ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices; // just short alias.

            GeometryHelper_Make_Arc2D(0, 0, innerRadius, 10, tlist_i); // inner circle
            //
            // create quad list from two point lists
            //
            if (tlist_i.size() < 2) throw DeadlyImportError("Disk2D. Not enough points for creating quad list."); // tlist_i and tlist_o has equal size.

            // add all quads except last
            for (std::list<aiVector3D>::iterator it_i = tlist_i.begin(), it_o = tlist_o.begin(); it_i != tlist_i.end();) {
                // do not forget - CCW direction
                vlist.push_back(*it_i++); // 1st point
                vlist.push_back(*it_o++); // 2nd point
                vlist.push_back(*it_o); // 3rd point
                vlist.push_back(*it_i); // 4th point
            }

            // add last quad
            vlist.push_back(*tlist_i.end()); // 1st point
            vlist.push_back(*tlist_o.end()); // 2nd point
            vlist.push_back(*tlist_o.begin()); // 3rd point
            vlist.push_back(*tlist_o.begin()); // 4th point

            ((CX3DImporter_NodeElement_Geometry2D *)ne)->NumIndices = 4;
        }

        ((CX3DImporter_NodeElement_Geometry2D *)ne)->Solid = solid;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Disk2D");
        else
            mNodeElementCur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Polyline2D
// DEF=""          ID
// USE=""          IDREF
// lineSegments="" MFVec2F [intializeOnly]
// />
void X3DImporter::ParseNode_Geometry2D_Polyline2D() {
    std::string def, use;
    std::list<aiVector2D> lineSegments;
    X3DNodeElementBase *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_REF("lineSegments", lineSegments, XML_ReadNode_GetAttrVal_AsListVec2f);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Polyline2D, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry2D(CX3DImporter_NodeElement::ENET_Polyline2D, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        //
        // convert read point list of geometry object to line set.
        //
        std::list<aiVector3D> tlist;

        // convert vec2 to vec3
        for (std::list<aiVector2D>::iterator it2 = lineSegments.begin(); it2 != lineSegments.end(); ++it2)
            tlist.push_back(aiVector3D(it2->x, it2->y, 0));

        // convert point set to line set
        GeometryHelper_Extend_PointToLine(tlist, ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices);
        ((CX3DImporter_NodeElement_Geometry2D *)ne)->NumIndices = 2;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Polyline2D");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Polypoint2D
// DEF=""   ID
// USE=""   IDREF
// point="" MFVec2F [inputOutput]
// />
void X3DImporter::ParseNode_Geometry2D_Polypoint2D() {
    std::string def, use;
    std::list<aiVector2D> point;
    CX3DImporter_NodeElement *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_REF("point", point, XML_ReadNode_GetAttrVal_AsListVec2f);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Polypoint2D, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry2D(CX3DImporter_NodeElement::ENET_Polypoint2D, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        // convert vec2 to vec3
        for (std::list<aiVector2D>::iterator it2 = point.begin(); it2 != point.end(); ++it2) {
            ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices.push_back(aiVector3D(it2->x, it2->y, 0));
        }

        ((CX3DImporter_NodeElement_Geometry2D *)ne)->NumIndices = 1;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Polypoint2D");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Rectangle2D
// DEF=""        ID
// USE=""        IDREF
// size="2 2"    SFVec2f [initializeOnly]
// solid="false" SFBool  [initializeOnly]
// />
void X3DImporter::ParseNode_Geometry2D_Rectangle2D() {
    std::string def, use;
    aiVector2D size(2, 2);
    bool solid = false;
    CX3DImporter_NodeElement *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_REF("size", size, XML_ReadNode_GetAttrVal_AsVec2f);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Rectangle2D, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry2D(CX3DImporter_NodeElement::ENET_Rectangle2D, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        float x1 = -size.x / 2.0f;
        float x2 = size.x / 2.0f;
        float y1 = -size.y / 2.0f;
        float y2 = size.y / 2.0f;
        std::list<aiVector3D> &vlist = ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices; // just short alias.

        vlist.push_back(aiVector3D(x2, y1, 0)); // 1st point
        vlist.push_back(aiVector3D(x2, y2, 0)); // 2nd point
        vlist.push_back(aiVector3D(x1, y2, 0)); // 3rd point
        vlist.push_back(aiVector3D(x1, y1, 0)); // 4th point
        ((CX3DImporter_NodeElement_Geometry2D *)ne)->Solid = solid;
        ((CX3DImporter_NodeElement_Geometry2D *)ne)->NumIndices = 4;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Rectangle2D");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <TriangleSet2D
// DEF=""        ID
// USE=""        IDREF
// solid="false" SFBool  [initializeOnly]
// vertices=""   MFVec2F [inputOutput]
// />
void X3DImporter::ParseNode_Geometry2D_TriangleSet2D() {
    std::string def, use;
    bool solid = false;
    std::list<aiVector2D> vertices;
    CX3DImporter_NodeElement *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_REF("vertices", vertices, XML_ReadNode_GetAttrVal_AsListVec2f);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_TriangleSet2D, ne);
    } else {
        if (vertices.size() % 3) throw DeadlyImportError("TriangleSet2D. Not enough points for defining triangle.");

        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry2D(CX3DImporter_NodeElement::ENET_TriangleSet2D, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        // convert vec2 to vec3
        for (std::list<aiVector2D>::iterator it2 = vertices.begin(); it2 != vertices.end(); ++it2) {
            ((CX3DImporter_NodeElement_Geometry2D *)ne)->Vertices.push_back(aiVector3D(it2->x, it2->y, 0));
        }

        ((CX3DImporter_NodeElement_Geometry2D *)ne)->Solid = solid;
        ((CX3DImporter_NodeElement_Geometry2D *)ne)->NumIndices = 3;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "TriangleSet2D");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Box
// DEF=""       ID
// USE=""       IDREF
// size="2 2 2" SFVec3f [initializeOnly]
// solid="true" SFBool  [initializeOnly]
// />
// The Box node specifies a rectangular parallelepiped box centred at (0, 0, 0) in the local coordinate system and aligned with the local coordinate axes.
// By default, the box measures 2 units in each dimension, from -1 to +1. The size field specifies the extents of the box along the X-, Y-, and Z-axes
// respectively and each component value shall be greater than zero.
void X3DImporter::ParseNode_Geometry3D_Box() {
    std::string def, use;
    bool solid = true;
    aiVector3D size(2, 2, 2);
    CX3DImporter_NodeElement *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_REF("size", size, XML_ReadNode_GetAttrVal_AsVec3f);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Box, ne);
    } else {
        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry3D(CX3DImporter_NodeElement::ENET_Box, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        GeometryHelper_MakeQL_RectParallelepiped(size, ((CX3DImporter_NodeElement_Geometry3D *)ne)->Vertices); // get quad list
        ((CX3DImporter_NodeElement_Geometry3D *)ne)->Solid = solid;
        ((CX3DImporter_NodeElement_Geometry3D *)ne)->NumIndices = 4;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Box");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Cone
// DEF=""           ID
// USE=""           IDREF
// bottom="true"    SFBool [initializeOnly]
// bottomRadius="1" SFloat [initializeOnly]
// height="2"       SFloat [initializeOnly]
// side="true"      SFBool [initializeOnly]
// solid="true"     SFBool [initializeOnly]
// />
void X3DImporter::ParseNode_Geometry3D_Cone() {
    std::string use, def;
    bool bottom = true;
    float bottomRadius = 1;
    float height = 2;
    bool side = true;
    bool solid = true;
    CX3DImporter_NodeElement *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("side", side, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("bottom", bottom, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("height", height, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_RET("bottomRadius", bottomRadius, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Cone, ne);
    } else {
        const unsigned int tess = 30; ///TODO: IME tessellation factor through ai_property

        std::vector<aiVector3D> tvec; // temp array for vertices.

        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry3D(CX3DImporter_NodeElement::ENET_Cone, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        // make cone or parts according to flags.
        if (side) {
            StandardShapes::MakeCone(height, 0, bottomRadius, tess, tvec, !bottom);
        } else if (bottom) {
            StandardShapes::MakeCircle(bottomRadius, tess, tvec);
            height = -(height / 2);
            for (std::vector<aiVector3D>::iterator it = tvec.begin(); it != tvec.end(); ++it)
                it->y = height; // y - because circle made in oXZ.
        }

        // copy data from temp array
        for (std::vector<aiVector3D>::iterator it = tvec.begin(); it != tvec.end(); ++it)
            ((CX3DImporter_NodeElement_Geometry3D *)ne)->Vertices.push_back(*it);

        ((CX3DImporter_NodeElement_Geometry3D *)ne)->Solid = solid;
        ((CX3DImporter_NodeElement_Geometry3D *)ne)->NumIndices = 3;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Cone");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Cylinder
// DEF=""        ID
// USE=""        IDREF
// bottom="true" SFBool [initializeOnly]
// height="2"    SFloat [initializeOnly]
// radius="1"    SFloat [initializeOnly]
// side="true"   SFBool [initializeOnly]
// solid="true"  SFBool [initializeOnly]
// top="true"    SFBool [initializeOnly]
// />
void X3DImporter::ParseNode_Geometry3D_Cylinder() {
    std::string use, def;
    bool bottom = true;
    float height = 2;
    float radius = 1;
    bool side = true;
    bool solid = true;
    bool top = true;
    CX3DImporter_NodeElement *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("radius", radius, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("bottom", bottom, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("top", top, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("side", side, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("height", height, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Cylinder, ne);
    } else {
        const unsigned int tess = 30; ///TODO: IME tessellation factor through ai_property

        std::vector<aiVector3D> tside; // temp array for vertices of side.
        std::vector<aiVector3D> tcir; // temp array for vertices of circle.

        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry3D(CX3DImporter_NodeElement::ENET_Cylinder, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        // make cilynder or parts according to flags.
        if (side) StandardShapes::MakeCone(height, radius, radius, tess, tside, true);

        height /= 2; // height defined for whole cylinder, when creating top and bottom circle we are using just half of height.
        if (top || bottom) StandardShapes::MakeCircle(radius, tess, tcir);
        // copy data from temp arrays
        std::list<aiVector3D> &vlist = ((CX3DImporter_NodeElement_Geometry3D *)ne)->Vertices; // just short alias.

        for (std::vector<aiVector3D>::iterator it = tside.begin(); it != tside.end(); ++it)
            vlist.push_back(*it);

        if (top) {
            for (std::vector<aiVector3D>::iterator it = tcir.begin(); it != tcir.end(); ++it) {
                (*it).y = height; // y - because circle made in oXZ.
                vlist.push_back(*it);
            }
        } // if(top)

        if (bottom) {
            for (std::vector<aiVector3D>::iterator it = tcir.begin(); it != tcir.end(); ++it) {
                (*it).y = -height; // y - because circle made in oXZ.
                vlist.push_back(*it);
            }
        } // if(top)

        ((CX3DImporter_NodeElement_Geometry3D *)ne)->Solid = solid;
        ((CX3DImporter_NodeElement_Geometry3D *)ne)->NumIndices = 3;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Cylinder");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <ElevationGrid
// DEF=""                 ID
// USE=""                 IDREF
// ccw="true"             SFBool  [initializeOnly]
// colorPerVertex="true"  SFBool  [initializeOnly]
// creaseAngle="0"        SFloat  [initializeOnly]
// height=""              MFloat  [initializeOnly]
// normalPerVertex="true" SFBool  [initializeOnly]
// solid="true"           SFBool  [initializeOnly]
// xDimension="0"         SFInt32 [initializeOnly]
// xSpacing="1.0"         SFloat  [initializeOnly]
// zDimension="0"         SFInt32 [initializeOnly]
// zSpacing="1.0"         SFloat  [initializeOnly]
// >
//   <!-- ColorNormalTexCoordContentModel -->
// ColorNormalTexCoordContentModel can contain Color (or ColorRGBA), Normal and TextureCoordinate, in any order. No more than one instance of any single
// node type is allowed. A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </ElevationGrid>
// The ElevationGrid node specifies a uniform rectangular grid of varying height in the Y=0 plane of the local coordinate system. The geometry is described
// by a scalar array of height values that specify the height of a surface above each point of the grid. The xDimension and zDimension fields indicate
// the number of elements of the grid height array in the X and Z directions. Both xDimension and zDimension shall be greater than or equal to zero.
// If either the xDimension or the zDimension is less than two, the ElevationGrid contains no quadrilaterals.
void X3DImporter::ParseNode_Geometry3D_ElevationGrid() {
    std::string use, def;
    bool ccw = true;
    bool colorPerVertex = true;
    float creaseAngle = 0;
    std::vector<float> height;
    bool normalPerVertex = true;
    bool solid = true;
    int32_t xDimension = 0;
    float xSpacing = 1;
    int32_t zDimension = 0;
    float zSpacing = 1;
    CX3DImporter_NodeElement *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("ccw", ccw, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("colorPerVertex", colorPerVertex, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("normalPerVertex", normalPerVertex, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("creaseAngle", creaseAngle, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_REF("height", height, XML_ReadNode_GetAttrVal_AsArrF);
    MACRO_ATTRREAD_CHECK_RET("xDimension", xDimension, XML_ReadNode_GetAttrVal_AsI32);
    MACRO_ATTRREAD_CHECK_RET("xSpacing", xSpacing, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_RET("zDimension", zDimension, XML_ReadNode_GetAttrVal_AsI32);
    MACRO_ATTRREAD_CHECK_RET("zSpacing", zSpacing, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_ElevationGrid, ne);
    } else {
        if ((xSpacing == 0.0f) || (zSpacing == 0.0f)) throw DeadlyImportError("Spacing in <ElevationGrid> must be grater than zero.");
        if ((xDimension <= 0) || (zDimension <= 0)) throw DeadlyImportError("Dimension in <ElevationGrid> must be grater than zero.");
        if ((size_t)(xDimension * zDimension) != height.size()) Throw_IncorrectAttrValue("Heights count must be equal to \"xDimension * zDimension\"");

        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_ElevationGrid(CX3DImporter_NodeElement::ENET_ElevationGrid, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        CX3DImporter_NodeElement_ElevationGrid &grid_alias = *((CX3DImporter_NodeElement_ElevationGrid *)ne); // create alias for conveience

        { // create grid vertices list
            std::vector<float>::const_iterator he_it = height.begin();

            for (int32_t zi = 0; zi < zDimension; zi++) // rows
            {
                for (int32_t xi = 0; xi < xDimension; xi++) // columns
                {
                    aiVector3D tvec(xSpacing * xi, *he_it, zSpacing * zi);

                    grid_alias.Vertices.push_back(tvec);
                    ++he_it;
                }
            }
        } // END: create grid vertices list
        //
        // create faces list. In "coordIdx" format
        //
        // check if we have quads
        if ((xDimension < 2) || (zDimension < 2)) // only one element in dimension is set, create line set.
        {
            ((CX3DImporter_NodeElement_ElevationGrid *)ne)->NumIndices = 2; // will be holded as line set.
            for (size_t i = 0, i_e = (grid_alias.Vertices.size() - 1); i < i_e; i++) {
                grid_alias.CoordIdx.push_back(static_cast<int32_t>(i));
                grid_alias.CoordIdx.push_back(static_cast<int32_t>(i + 1));
                grid_alias.CoordIdx.push_back(-1);
            }
        } else // two or more elements in every dimension is set. create quad set.
        {
            ((CX3DImporter_NodeElement_ElevationGrid *)ne)->NumIndices = 4;
            for (int32_t fzi = 0, fzi_e = (zDimension - 1); fzi < fzi_e; fzi++) // rows
            {
                for (int32_t fxi = 0, fxi_e = (xDimension - 1); fxi < fxi_e; fxi++) // columns
                {
                    // points direction in face.
                    if (ccw) {
                        // CCW:
                        //	3 2
                        //	0 1
                        grid_alias.CoordIdx.push_back((fzi + 1) * xDimension + fxi);
                        grid_alias.CoordIdx.push_back((fzi + 1) * xDimension + (fxi + 1));
                        grid_alias.CoordIdx.push_back(fzi * xDimension + (fxi + 1));
                        grid_alias.CoordIdx.push_back(fzi * xDimension + fxi);
                    } else {
                        // CW:
                        //	0 1
                        //	3 2
                        grid_alias.CoordIdx.push_back(fzi * xDimension + fxi);
                        grid_alias.CoordIdx.push_back(fzi * xDimension + (fxi + 1));
                        grid_alias.CoordIdx.push_back((fzi + 1) * xDimension + (fxi + 1));
                        grid_alias.CoordIdx.push_back((fzi + 1) * xDimension + fxi);
                    } // if(ccw) else

                    grid_alias.CoordIdx.push_back(-1);
                } // for(int32_t fxi = 0, fxi_e = (xDimension - 1); fxi < fxi_e; fxi++)
            } // for(int32_t fzi = 0, fzi_e = (zDimension - 1); fzi < fzi_e; fzi++)
        } // if((xDimension < 2) || (zDimension < 2)) else

        grid_alias.ColorPerVertex = colorPerVertex;
        grid_alias.NormalPerVertex = normalPerVertex;
        grid_alias.CreaseAngle = creaseAngle;
        grid_alias.Solid = solid;
        // check for child nodes
        if (!mReader->isEmptyElement()) {
            ParseHelper_Node_Enter(ne);
            MACRO_NODECHECK_LOOPBEGIN("ElevationGrid");
            // check for X3DComposedGeometryNodes
            if (XML_CheckNode_NameEqual("Color")) {
                ParseNode_Rendering_Color();
                continue;
            }
            if (XML_CheckNode_NameEqual("ColorRGBA")) {
                ParseNode_Rendering_ColorRGBA();
                continue;
            }
            if (XML_CheckNode_NameEqual("Normal")) {
                ParseNode_Rendering_Normal();
                continue;
            }
            if (XML_CheckNode_NameEqual("TextureCoordinate")) {
                ParseNode_Texturing_TextureCoordinate();
                continue;
            }
            // check for X3DMetadataObject
            if (!ParseHelper_CheckRead_X3DMetadataObject()) XML_CheckNode_SkipUnsupported("ElevationGrid");

            MACRO_NODECHECK_LOOPEND("ElevationGrid");
            ParseHelper_Node_Exit();
        } // if(!mReader->isEmptyElement())
        else {
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element
        } // if(!mReader->isEmptyElement()) else

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

template <typename TVector>
static void GeometryHelper_Extrusion_CurveIsClosed(std::vector<TVector> &pCurve, const bool pDropTail, const bool pRemoveLastPoint, bool &pCurveIsClosed) {
    size_t cur_sz = pCurve.size();

    pCurveIsClosed = false;
    // for curve with less than four points checking is have no sense,
    if (cur_sz < 4) return;

    for (size_t s = 3, s_e = cur_sz; s < s_e; s++) {
        // search for first point of duplicated part.
        if (pCurve[0] == pCurve[s]) {
            bool found = true;

            // check if tail(indexed by b2) is duplicate of head(indexed by b1).
            for (size_t b1 = 1, b2 = (s + 1); b2 < cur_sz; b1++, b2++) {
                if (pCurve[b1] != pCurve[b2]) { // points not match: clear flag and break loop.
                    found = false;

                    break;
                }
            } // for(size_t b1 = 1, b2 = (s + 1); b2 < cur_sz; b1++, b2++)

            // if duplicate tail is found then drop or not it depending on flags.
            if (found) {
                pCurveIsClosed = true;
                if (pDropTail) {
                    if (!pRemoveLastPoint) s++; // prepare value for iterator's arithmetics.

                    pCurve.erase(pCurve.begin() + s, pCurve.end()); // remove tail
                }

                break;
            } // if(found)
        } // if(pCurve[0] == pCurve[s])
    } // for(size_t s = 3, s_e = (cur_sz - 1); s < s_e; s++)
}

static aiVector3D GeometryHelper_Extrusion_GetNextY(const size_t pSpine_PointIdx, const std::vector<aiVector3D> &pSpine, const bool pSpine_Closed) {
    const size_t spine_idx_last = pSpine.size() - 1;
    aiVector3D tvec;

    if ((pSpine_PointIdx == 0) || (pSpine_PointIdx == spine_idx_last)) // at first special cases
    {
        if (pSpine_Closed) { // If the spine curve is closed: The SCP for the first and last points is the same and is found using (spine[1] - spine[n - 2]) to compute the Y-axis.
            // As we even for closed spine curve last and first point in pSpine are not the same: duplicates(spine[n - 1] which are equivalent to spine[0])
            // in tail are removed.
            // So, last point in pSpine is a spine[n - 2]
            tvec = pSpine[1] - pSpine[spine_idx_last];
        } else if (pSpine_PointIdx == 0) { // The Y-axis used for the first point is the vector from spine[0] to spine[1]
            tvec = pSpine[1] - pSpine[0];
        } else { // The Y-axis used for the last point it is the vector from spine[n-2] to spine[n-1]. In our case(see above about dropping tail) spine[n - 1] is
            // the spine[0].
            tvec = pSpine[spine_idx_last] - pSpine[spine_idx_last - 1];
        }
    } // if((pSpine_PointIdx == 0) || (pSpine_PointIdx == spine_idx_last))
    else { // For all points other than the first or last: The Y-axis for spine[i] is found by normalizing the vector defined by (spine[i+1] - spine[i-1]).
        tvec = pSpine[pSpine_PointIdx + 1] - pSpine[pSpine_PointIdx - 1];
    } // if((pSpine_PointIdx == 0) || (pSpine_PointIdx == spine_idx_last)) else

    return tvec.Normalize();
}

static aiVector3D GeometryHelper_Extrusion_GetNextZ(const size_t pSpine_PointIdx, const std::vector<aiVector3D> &pSpine, const bool pSpine_Closed,
        const aiVector3D pVecZ_Prev) {
    const aiVector3D zero_vec(0);
    const size_t spine_idx_last = pSpine.size() - 1;

    aiVector3D tvec;

    // at first special cases
    if (pSpine.size() < 3) // spine have not enough points for vector calculations.
    {
        tvec.Set(0, 0, 1);
    } else if (pSpine_PointIdx == 0) // special case: first point
    {
        if (pSpine_Closed) // for calculating use previous point in curve s[n - 2]. In list it's a last point, because point s[n - 1] was removed as duplicate.
        {
            tvec = (pSpine[1] - pSpine[0]) ^ (pSpine[spine_idx_last] - pSpine[0]);
        } else // for not closed curve first and next point(s[0] and s[1]) has the same vector Z.
        {
            bool found = false;

            // As said: "If the Z-axis of the first point is undefined (because the spine is not closed and the first two spine segments are collinear)
            // then the Z-axis for the first spine point with a defined Z-axis is used."
            // Walk through spine and find Z.
            for (size_t next_point = 2; (next_point <= spine_idx_last) && !found; next_point++) {
                // (pSpine[2] - pSpine[1]) ^ (pSpine[0] - pSpine[1])
                tvec = (pSpine[next_point] - pSpine[next_point - 1]) ^ (pSpine[next_point - 2] - pSpine[next_point - 1]);
                found = !tvec.Equal(zero_vec);
            }

            // if entire spine are collinear then use OZ axis.
            if (!found) tvec.Set(0, 0, 1);
        } // if(pSpine_Closed) else
    } // else if(pSpine_PointIdx == 0)
    else if (pSpine_PointIdx == spine_idx_last) // special case: last point
    {
        if (pSpine_Closed) { // do not forget that real last point s[n - 1] is removed as duplicated. And in this case we are calculating vector Z for point s[n - 2].
            tvec = (pSpine[0] - pSpine[pSpine_PointIdx]) ^ (pSpine[pSpine_PointIdx - 1] - pSpine[pSpine_PointIdx]);
            // if taken spine vectors are collinear then use previous vector Z.
            if (tvec.Equal(zero_vec)) tvec = pVecZ_Prev;
        } else { // vector Z for last point of not closed curve is previous vector Z.
            tvec = pVecZ_Prev;
        }
    } else // regular point
    {
        tvec = (pSpine[pSpine_PointIdx + 1] - pSpine[pSpine_PointIdx]) ^ (pSpine[pSpine_PointIdx - 1] - pSpine[pSpine_PointIdx]);
        // if taken spine vectors are collinear then use previous vector Z.
        if (tvec.Equal(zero_vec)) tvec = pVecZ_Prev;
    }

    // After determining the Z-axis, its dot product with the Z-axis of the previous spine point is computed. If this value is negative, the Z-axis
    // is flipped (multiplied by -1).
    if ((tvec * pVecZ_Prev) < 0) tvec = -tvec;

    return tvec.Normalize();
}

// <Extrusion
// DEF=""                                 ID
// USE=""                                 IDREF
// beginCap="true"                        SFBool     [initializeOnly]
// ccw="true"                             SFBool     [initializeOnly]
// convex="true"                          SFBool     [initializeOnly]
// creaseAngle="0.0"                      SFloat     [initializeOnly]
// crossSection="1 1 1 -1 -1 -1 -1 1 1 1" MFVec2f    [initializeOnly]
// endCap="true"                          SFBool     [initializeOnly]
// orientation="0 0 1 0"                  MFRotation [initializeOnly]
// scale="1 1"                            MFVec2f    [initializeOnly]
// solid="true"                           SFBool     [initializeOnly]
// spine="0 0 0 0 1 0"                    MFVec3f    [initializeOnly]
// />
void X3DImporter::ParseNode_Geometry3D_Extrusion() {
    std::string use, def;
    bool beginCap = true;
    bool ccw = true;
    bool convex = true;
    float creaseAngle = 0;
    std::vector<aiVector2D> crossSection;
    bool endCap = true;
    std::vector<float> orientation;
    std::vector<aiVector2D> scale;
    bool solid = true;
    std::vector<aiVector3D> spine;
    CX3DImporter_NodeElement *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("beginCap", beginCap, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("ccw", ccw, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("convex", convex, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("creaseAngle", creaseAngle, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_REF("crossSection", crossSection, XML_ReadNode_GetAttrVal_AsArrVec2f);
    MACRO_ATTRREAD_CHECK_RET("endCap", endCap, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_REF("orientation", orientation, XML_ReadNode_GetAttrVal_AsArrF);
    MACRO_ATTRREAD_CHECK_REF("scale", scale, XML_ReadNode_GetAttrVal_AsArrVec2f);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_REF("spine", spine, XML_ReadNode_GetAttrVal_AsArrVec3f);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Extrusion, ne);
    } else {
        //
        // check if default values must be assigned
        //
        if (spine.size() == 0) {
            spine.resize(2);
            spine[0].Set(0, 0, 0), spine[1].Set(0, 1, 0);
        } else if (spine.size() == 1) {
            throw DeadlyImportError("ParseNode_Geometry3D_Extrusion. Spine must have at least two points.");
        }

        if (crossSection.size() == 0) {
            crossSection.resize(5);
            crossSection[0].Set(1, 1), crossSection[1].Set(1, -1), crossSection[2].Set(-1, -1), crossSection[3].Set(-1, 1), crossSection[4].Set(1, 1);
        }

        { // orientation
            size_t ori_size = orientation.size() / 4;

            if (ori_size < spine.size()) {
                float add_ori[4]; // values that will be added

                if (ori_size == 1) // if "orientation" has one element(means one MFRotation with four components) then use it value for all spine points.
                {
                    add_ori[0] = orientation[0], add_ori[1] = orientation[1], add_ori[2] = orientation[2], add_ori[3] = orientation[3];
                } else // else - use default values
                {
                    add_ori[0] = 0, add_ori[1] = 0, add_ori[2] = 1, add_ori[3] = 0;
                }

                orientation.reserve(spine.size() * 4);
                for (size_t i = 0, i_e = (spine.size() - ori_size); i < i_e; i++)
                    orientation.push_back(add_ori[0]), orientation.push_back(add_ori[1]), orientation.push_back(add_ori[2]), orientation.push_back(add_ori[3]);
            }

            if (orientation.size() % 4) throw DeadlyImportError("Attribute \"orientation\" in <Extrusion> must has multiple four quantity of numbers.");
        } // END: orientation

        { // scale
            if (scale.size() < spine.size()) {
                aiVector2D add_sc;

                if (scale.size() == 1) // if "scale" has one element then use it value for all spine points.
                    add_sc = scale[0];
                else // else - use default values
                    add_sc.Set(1, 1);

                scale.reserve(spine.size());
                for (size_t i = 0, i_e = (spine.size() - scale.size()); i < i_e; i++)
                    scale.push_back(add_sc);
            }
        } // END: scale
        //
        // create and if needed - define new geometry object.
        //
        ne = new CX3DImporter_NodeElement_IndexedSet(CX3DImporter_NodeElement::ENET_Extrusion, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        CX3DImporter_NodeElement_IndexedSet &ext_alias = *((CX3DImporter_NodeElement_IndexedSet *)ne); // create alias for conveience
        // assign part of input data
        ext_alias.CCW = ccw;
        ext_alias.Convex = convex;
        ext_alias.CreaseAngle = creaseAngle;
        ext_alias.Solid = solid;

        //
        // How we done it at all?
        // 1. At first we will calculate array of basises for every point in spine(look SCP in ISO-dic). Also "orientation" vector
        // are applied vor every basis.
        // 2. After that we can create array of point sets: which are scaled, transferred to basis of relative basis and at final translated to real position
        // using relative spine point.
        // 3. Next step is creating CoordIdx array(do not forget "-1" delimiter). While creating CoordIdx also created faces for begin and end caps, if
        // needed. While createing CootdIdx is taking in account CCW flag.
        // 4. The last step: create Vertices list.
        //
        bool spine_closed; // flag: true if spine curve is closed.
        bool cross_closed; // flag: true if cross curve is closed.
        std::vector<aiMatrix3x3> basis_arr; // array of basises. ROW_a - X, ROW_b - Y, ROW_c - Z.
        std::vector<std::vector<aiVector3D>> pointset_arr; // array of point sets: cross curves.

        // detect closed curves
        GeometryHelper_Extrusion_CurveIsClosed(crossSection, true, true, cross_closed); // true - drop tail, true - remove duplicate end.
        GeometryHelper_Extrusion_CurveIsClosed(spine, true, true, spine_closed); // true - drop tail, true - remove duplicate end.
        // If both cap are requested and spine curve is closed then we can make only one cap. Because second cap will be the same surface.
        if (spine_closed) {
            beginCap |= endCap;
            endCap = false;
        }

        { // 1. Calculate array of basises.
            aiMatrix4x4 rotmat;
            aiVector3D vecX(0), vecY(0), vecZ(0);

            basis_arr.resize(spine.size());
            for (size_t i = 0, i_e = spine.size(); i < i_e; i++) {
                aiVector3D tvec;

                // get axises of basis.
                vecY = GeometryHelper_Extrusion_GetNextY(i, spine, spine_closed);
                vecZ = GeometryHelper_Extrusion_GetNextZ(i, spine, spine_closed, vecZ);
                vecX = (vecY ^ vecZ).Normalize();
                // get rotation matrix and apply "orientation" to basis
                aiMatrix4x4::Rotation(orientation[i * 4 + 3], aiVector3D(orientation[i * 4], orientation[i * 4 + 1], orientation[i * 4 + 2]), rotmat);
                tvec = vecX, tvec *= rotmat, basis_arr[i].a1 = tvec.x, basis_arr[i].a2 = tvec.y, basis_arr[i].a3 = tvec.z;
                tvec = vecY, tvec *= rotmat, basis_arr[i].b1 = tvec.x, basis_arr[i].b2 = tvec.y, basis_arr[i].b3 = tvec.z;
                tvec = vecZ, tvec *= rotmat, basis_arr[i].c1 = tvec.x, basis_arr[i].c2 = tvec.y, basis_arr[i].c3 = tvec.z;
            } // for(size_t i = 0, i_e = spine.size(); i < i_e; i++)
        } // END: 1. Calculate array of basises

        { // 2. Create array of point sets.
            aiMatrix4x4 scmat;
            std::vector<aiVector3D> tcross(crossSection.size());

            pointset_arr.resize(spine.size());
            for (size_t spi = 0, spi_e = spine.size(); spi < spi_e; spi++) {
                aiVector3D tc23vec;

                tc23vec.Set(scale[spi].x, 0, scale[spi].y);
                aiMatrix4x4::Scaling(tc23vec, scmat);
                for (size_t cri = 0, cri_e = crossSection.size(); cri < cri_e; cri++) {
                    aiVector3D tvecX, tvecY, tvecZ;

                    tc23vec.Set(crossSection[cri].x, 0, crossSection[cri].y);
                    // apply scaling to point
                    tcross[cri] = scmat * tc23vec;
                    //
                    // transfer point to new basis
                    // calculate coordinate in new basis
                    tvecX.Set(basis_arr[spi].a1, basis_arr[spi].a2, basis_arr[spi].a3), tvecX *= tcross[cri].x;
                    tvecY.Set(basis_arr[spi].b1, basis_arr[spi].b2, basis_arr[spi].b3), tvecY *= tcross[cri].y;
                    tvecZ.Set(basis_arr[spi].c1, basis_arr[spi].c2, basis_arr[spi].c3), tvecZ *= tcross[cri].z;
                    // apply new coordinates and translate it to spine point.
                    tcross[cri] = tvecX + tvecY + tvecZ + spine[spi];
                } // for(size_t cri = 0, cri_e = crossSection.size(); cri < cri_e; i++)

                pointset_arr[spi] = tcross; // store transferred point set
            } // for(size_t spi = 0, spi_e = spine.size(); spi < spi_e; i++)
        } // END: 2. Create array of point sets.

        { // 3. Create CoordIdx.
            // add caps if needed
            if (beginCap) {
                // add cap as polygon. vertices of cap are places at begin, so just add numbers from zero.
                for (size_t i = 0, i_e = crossSection.size(); i < i_e; i++)
                    ext_alias.CoordIndex.push_back(static_cast<int32_t>(i));

                // add delimiter
                ext_alias.CoordIndex.push_back(-1);
            } // if(beginCap)

            if (endCap) {
                // add cap as polygon. vertices of cap are places at end, as for beginCap use just sequence of numbers but with offset.
                size_t beg = (pointset_arr.size() - 1) * crossSection.size();

                for (size_t i = beg, i_e = (beg + crossSection.size()); i < i_e; i++)
                    ext_alias.CoordIndex.push_back(static_cast<int32_t>(i));

                // add delimiter
                ext_alias.CoordIndex.push_back(-1);
            } // if(beginCap)

            // add quads
            for (size_t spi = 0, spi_e = (spine.size() - 1); spi <= spi_e; spi++) {
                const size_t cr_sz = crossSection.size();
                const size_t cr_last = crossSection.size() - 1;

                size_t right_col; // hold index basis for points of quad placed in right column;

                if (spi != spi_e)
                    right_col = spi + 1;
                else if (spine_closed) // if spine curve is closed then one more quad is needed: between first and last points of curve.
                    right_col = 0;
                else
                    break; // if spine curve is not closed then break the loop, because spi is out of range for that type of spine.

                for (size_t cri = 0; cri < cr_sz; cri++) {
                    if (cri != cr_last) {
                        MACRO_FACE_ADD_QUAD(ccw, ext_alias.CoordIndex,
                                static_cast<int32_t>(spi * cr_sz + cri),
                                static_cast<int32_t>(right_col * cr_sz + cri),
                                static_cast<int32_t>(right_col * cr_sz + cri + 1),
                                static_cast<int32_t>(spi * cr_sz + cri + 1));
                        // add delimiter
                        ext_alias.CoordIndex.push_back(-1);
                    } else if (cross_closed) // if cross curve is closed then one more quad is needed: between first and last points of curve.
                    {
                        MACRO_FACE_ADD_QUAD(ccw, ext_alias.CoordIndex,
                                static_cast<int32_t>(spi * cr_sz + cri),
                                static_cast<int32_t>(right_col * cr_sz + cri),
                                static_cast<int32_t>(right_col * cr_sz + 0),
                                static_cast<int32_t>(spi * cr_sz + 0));
                        // add delimiter
                        ext_alias.CoordIndex.push_back(-1);
                    }
                } // for(size_t cri = 0; cri < cr_sz; cri++)
            } // for(size_t spi = 0, spi_e = (spine.size() - 2); spi < spi_e; spi++)
        } // END: 3. Create CoordIdx.

        { // 4. Create vertices list.
            // just copy all vertices
            for (size_t spi = 0, spi_e = spine.size(); spi < spi_e; spi++) {
                for (size_t cri = 0, cri_e = crossSection.size(); cri < cri_e; cri++) {
                    ext_alias.Vertices.push_back(pointset_arr[spi][cri]);
                }
            }
        } // END: 4. Create vertices list.
        //PrintVectorSet("Ext. CoordIdx", ext_alias.CoordIndex);
        //PrintVectorSet("Ext. Vertices", ext_alias.Vertices);
        // check for child nodes
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Extrusion");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <IndexedFaceSet
// DEF=""                         ID
// USE=""                         IDREF
// ccw="true"             SFBool  [initializeOnly]
// colorIndex=""          MFInt32 [initializeOnly]
// colorPerVertex="true"  SFBool  [initializeOnly]
// convex="true"          SFBool  [initializeOnly]
// coordIndex=""          MFInt32 [initializeOnly]
// creaseAngle="0"        SFFloat [initializeOnly]
// normalIndex=""         MFInt32 [initializeOnly]
// normalPerVertex="true" SFBool  [initializeOnly]
// solid="true"           SFBool  [initializeOnly]
// texCoordIndex=""       MFInt32 [initializeOnly]
// >
//    <!-- ComposedGeometryContentModel -->
// ComposedGeometryContentModel is the child-node content model corresponding to X3DComposedGeometryNodes. It can contain Color (or ColorRGBA), Coordinate,
// Normal and TextureCoordinate, in any order. No more than one instance of these nodes is allowed. Multiple VertexAttribute (FloatVertexAttribute,
// Matrix3VertexAttribute, Matrix4VertexAttribute) nodes can also be contained.
// A ProtoInstance node (with the proper node type) can be substituted for any node in this content model.
// </IndexedFaceSet>
void X3DImporter::ParseNode_Geometry3D_IndexedFaceSet() {
    std::string use, def;
    bool ccw = true;
    std::vector<int32_t> colorIndex;
    bool colorPerVertex = true;
    bool convex = true;
    std::vector<int32_t> coordIndex;
    float creaseAngle = 0;
    std::vector<int32_t> normalIndex;
    bool normalPerVertex = true;
    bool solid = true;
    std::vector<int32_t> texCoordIndex;
    CX3DImporter_NodeElement *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("ccw", ccw, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_REF("colorIndex", colorIndex, XML_ReadNode_GetAttrVal_AsArrI32);
    MACRO_ATTRREAD_CHECK_RET("colorPerVertex", colorPerVertex, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("convex", convex, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_REF("coordIndex", coordIndex, XML_ReadNode_GetAttrVal_AsArrI32);
    MACRO_ATTRREAD_CHECK_RET("creaseAngle", creaseAngle, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_REF("normalIndex", normalIndex, XML_ReadNode_GetAttrVal_AsArrI32);
    MACRO_ATTRREAD_CHECK_RET("normalPerVertex", normalPerVertex, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_CHECK_REF("texCoordIndex", texCoordIndex, XML_ReadNode_GetAttrVal_AsArrI32);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_IndexedFaceSet, ne);
    } else {
        // check data
        if (coordIndex.size() == 0) throw DeadlyImportError("IndexedFaceSet must contain not empty \"coordIndex\" attribute.");

        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_IndexedSet(CX3DImporter_NodeElement::ENET_IndexedFaceSet, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        CX3DImporter_NodeElement_IndexedSet &ne_alias = *((CX3DImporter_NodeElement_IndexedSet *)ne);

        ne_alias.CCW = ccw;
        ne_alias.ColorIndex = colorIndex;
        ne_alias.ColorPerVertex = colorPerVertex;
        ne_alias.Convex = convex;
        ne_alias.CoordIndex = coordIndex;
        ne_alias.CreaseAngle = creaseAngle;
        ne_alias.NormalIndex = normalIndex;
        ne_alias.NormalPerVertex = normalPerVertex;
        ne_alias.Solid = solid;
        ne_alias.TexCoordIndex = texCoordIndex;
        // check for child nodes
        if (!mReader->isEmptyElement()) {
            ParseHelper_Node_Enter(ne);
            MACRO_NODECHECK_LOOPBEGIN("IndexedFaceSet");
            // check for X3DComposedGeometryNodes
            if (XML_CheckNode_NameEqual("Color")) {
                ParseNode_Rendering_Color();
                continue;
            }
            if (XML_CheckNode_NameEqual("ColorRGBA")) {
                ParseNode_Rendering_ColorRGBA();
                continue;
            }
            if (XML_CheckNode_NameEqual("Coordinate")) {
                ParseNode_Rendering_Coordinate();
                continue;
            }
            if (XML_CheckNode_NameEqual("Normal")) {
                ParseNode_Rendering_Normal();
                continue;
            }
            if (XML_CheckNode_NameEqual("TextureCoordinate")) {
                ParseNode_Texturing_TextureCoordinate();
                continue;
            }
            // check for X3DMetadataObject
            if (!ParseHelper_CheckRead_X3DMetadataObject()) XML_CheckNode_SkipUnsupported("IndexedFaceSet");

            MACRO_NODECHECK_LOOPEND("IndexedFaceSet");
            ParseHelper_Node_Exit();
        } // if(!mReader->isEmptyElement())
        else {
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element
        }

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}

// <Sphere
// DEF=""       ID
// USE=""       IDREF
// radius="1"   SFloat [initializeOnly]
// solid="true" SFBool [initializeOnly]
// />
void X3DImporter::ParseNode_Geometry3D_Sphere() {
    std::string use, def;
    ai_real radius = 1;
    bool solid = true;
    CX3DImporter_NodeElement *ne(nullptr);

    MACRO_ATTRREAD_LOOPBEG;
    MACRO_ATTRREAD_CHECKUSEDEF_RET(def, use);
    MACRO_ATTRREAD_CHECK_RET("radius", radius, XML_ReadNode_GetAttrVal_AsFloat);
    MACRO_ATTRREAD_CHECK_RET("solid", solid, XML_ReadNode_GetAttrVal_AsBool);
    MACRO_ATTRREAD_LOOPEND;

    // if "USE" defined then find already defined element.
    if (!use.empty()) {
        MACRO_USE_CHECKANDAPPLY(def, use, ENET_Sphere, ne);
    } else {
        const unsigned int tess = 3; ///TODO: IME tessellation factor through ai_property

        std::vector<aiVector3D> tlist;

        // create and if needed - define new geometry object.
        ne = new CX3DImporter_NodeElement_Geometry3D(CX3DImporter_NodeElement::ENET_Sphere, NodeElement_Cur);
        if (!def.empty()) ne->ID = def;

        StandardShapes::MakeSphere(tess, tlist);
        // copy data from temp array and apply scale
        for (std::vector<aiVector3D>::iterator it = tlist.begin(); it != tlist.end(); ++it) {
            ((CX3DImporter_NodeElement_Geometry3D *)ne)->Vertices.push_back(*it * radius);
        }

        ((CX3DImporter_NodeElement_Geometry3D *)ne)->Solid = solid;
        ((CX3DImporter_NodeElement_Geometry3D *)ne)->NumIndices = 3;
        // check for X3DMetadataObject childs.
        if (!mReader->isEmptyElement())
            ParseNode_Metadata(ne, "Sphere");
        else
            NodeElement_Cur->Child.push_back(ne); // add made object as child to current element

        NodeElement_List.push_back(ne); // add element to node element list because its a new object in graph
    } // if(!use.empty()) else
}



void X3DImporter::readMetadataObject(XmlNode &node) {
    const std::string &name = node.name();
    if (name == "MetadataBoolean") {
        readMetadataBoolean(node, mNodeElementCur);
    } else if (name == "MetadataDouble") {
        readMetadataDouble(node, mNodeElementCur);
    } else if (name == "MetadataFloat") {
        readMetadataFloat(node, mNodeElementCur);
    } else if (name == "MetadataInteger") {
        readMetadataInteger(node, mNodeElementCur);
    } else if (name == "MetadataSet") {
        readMetadataSet(node, mNodeElementCur);
    } else if (name == "MetadataString") {
        readMetadataString(node, mNodeElementCur);
    }
}


aiMatrix4x4 PostprocessHelper_Matrix_GlobalToCurrent() {
    X3DNodeElementBase *cur_node = nullptr;
    std::list<aiMatrix4x4> matr;
    aiMatrix4x4 out_matr;

    // starting walk from current element to root
    cur_node = cur_node;
    if (cur_node != nullptr) {
        do {
            // if cur_node is group then store group transformation matrix in list.
            if (cur_node->Type == X3DNodeElementBase::ENET_Group) matr.push_back(((X3DNodeElementBase *)cur_node)->Transformation);

            cur_node = cur_node->Parent;
        } while (cur_node != nullptr);
    }

    // multiplicate all matrices in reverse order
    for (std::list<aiMatrix4x4>::reverse_iterator rit = matr.rbegin(); rit != matr.rend(); ++rit)
        out_matr = out_matr * (*rit);

    return out_matr;
}

void X3DImporter::PostprocessHelper_CollectMetadata(const CX3DImporter_NodeElement &pNodeElement, std::list<X3DNodeElementBase *> &pList) const {
    // walk through childs and find for metadata.
    for (std::list<X3DNodeElementBase *>::const_iterator el_it = pNodeElement.Child.begin(); el_it != pNodeElement.Child.end(); ++el_it) {
        if (((*el_it)->Type == X3DElemType::ENET_MetaBoolean) || ((*el_it)->Type == X3DElemType::ENET_MetaDouble) ||
                ((*el_it)->Type == X3DElemType::ENET_MetaFloat) || ((*el_it)->Type == X3DElemType::ENET_MetaInteger) ||
                ((*el_it)->Type == X3DElemType::ENET_MetaString)) {
            pList.push_back(*el_it);
        } else if ((*el_it)->Type == X3DElemType::ENET_MetaSet) {
            PostprocessHelper_CollectMetadata(**el_it, pList);
        }
    } // for(std::list<CX3DImporter_NodeElement*>::const_iterator el_it = pNodeElement.Child.begin(); el_it != pNodeElement.Child.end(); el_it++)
}

bool X3DImporter::PostprocessHelper_ElementIsMetadata(const CX3DImporter_NodeElement::EType pType) const {
    if ((pType == X3DNodeElementBase::ENET_MetaBoolean) || (pType == X3DElemType::ENET_MetaDouble) ||
            (pType == X3DElemType::ENET_MetaFloat) || (pType == X3DElemType::ENET_MetaInteger) ||
            (pType == X3DElemType::ENET_MetaString) || (pType == X3DElemType::ENET_MetaSet)) {
        return true;
    }
    return false;
}

bool X3DImporter::PostprocessHelper_ElementIsMesh(const CX3DImporter_NodeElement::EType pType) const {
    if ((pType == X3DElemType::ENET_Arc2D) || (pType == X3DElemType::ENET_ArcClose2D) ||
            (pType == X3DElemType::ENET_Box) || (pType == X3DElemType::ENET_Circle2D) ||
            (pType == X3DElemType::ENET_Cone) || (pType == X3DElemType::ENET_Cylinder) ||
            (pType == X3DElemType::ENET_Disk2D) || (pType == X3DElemType::ENET_ElevationGrid) ||
            (pType == X3DElemType::ENET_Extrusion) || (pType == X3DElemType::ENET_IndexedFaceSet) ||
            (pType == X3DElemType::ENET_IndexedLineSet) || (pType == X3DElemType::ENET_IndexedTriangleFanSet) ||
            (pType == X3DElemType::ENET_IndexedTriangleSet) || (pType == X3DElemType::ENET_IndexedTriangleStripSet) ||
            (pType == X3DElemType::ENET_PointSet) || (pType == X3DElemType::ENET_LineSet) ||
            (pType == X3DElemType::ENET_Polyline2D) || (pType == X3DElemType::ENET_Polypoint2D) ||
            (pType == X3DElemType::ENET_Rectangle2D) || (pType == X3DElemType::ENET_Sphere) ||
            (pType == X3DElemType::ENET_TriangleFanSet) || (pType == X3DElemType::ENET_TriangleSet) ||
            (pType == X3DElemType::ENET_TriangleSet2D) || (pType == X3DElemType::ENET_TriangleStripSet)) {
        return true;
    } else {
        return false;
    }
}

void X3DImporter::Postprocess_BuildLight(const CX3DImporter_NodeElement &pNodeElement, std::list<aiLight *> &pSceneLightList) const {
    const CX3DImporter_NodeElement_Light &ne = *((CX3DImporter_NodeElement_Light *)&pNodeElement);
    aiMatrix4x4 transform_matr = PostprocessHelper_Matrix_GlobalToCurrent();
    aiLight *new_light = new aiLight;

    new_light->mName = ne.ID;
    new_light->mColorAmbient = ne.Color * ne.AmbientIntensity;
    new_light->mColorDiffuse = ne.Color * ne.Intensity;
    new_light->mColorSpecular = ne.Color * ne.Intensity;
    switch (pNodeElement.Type) {
    case CX3DImporter_NodeElement::ENET_DirectionalLight:
        new_light->mType = aiLightSource_DIRECTIONAL;
        new_light->mDirection = ne.Direction, new_light->mDirection *= transform_matr;

        break;
    case CX3DImporter_NodeElement::ENET_PointLight:
        new_light->mType = aiLightSource_POINT;
        new_light->mPosition = ne.Location, new_light->mPosition *= transform_matr;
        new_light->mAttenuationConstant = ne.Attenuation.x;
        new_light->mAttenuationLinear = ne.Attenuation.y;
        new_light->mAttenuationQuadratic = ne.Attenuation.z;

        break;
    case CX3DImporter_NodeElement::ENET_SpotLight:
        new_light->mType = aiLightSource_SPOT;
        new_light->mPosition = ne.Location, new_light->mPosition *= transform_matr;
        new_light->mDirection = ne.Direction, new_light->mDirection *= transform_matr;
        new_light->mAttenuationConstant = ne.Attenuation.x;
        new_light->mAttenuationLinear = ne.Attenuation.y;
        new_light->mAttenuationQuadratic = ne.Attenuation.z;
        new_light->mAngleInnerCone = ne.BeamWidth;
        new_light->mAngleOuterCone = ne.CutOffAngle;

        break;
    default:
        throw DeadlyImportError("Postprocess_BuildLight. Unknown type of light: " + to_string(pNodeElement.Type) + ".");
    }

    pSceneLightList.push_back(new_light);
}

void X3DImporter::Postprocess_BuildMaterial(const CX3DImporter_NodeElement &pNodeElement, aiMaterial **pMaterial) const {
    // check argument
    if (pMaterial == nullptr) throw DeadlyImportError("Postprocess_BuildMaterial. pMaterial is nullptr.");
    if (*pMaterial != nullptr) throw DeadlyImportError("Postprocess_BuildMaterial. *pMaterial must be nullptr.");

    *pMaterial = new aiMaterial;
    aiMaterial &taimat = **pMaterial; // creating alias for convenience.

    // at this point pNodeElement point to <Appearance> node. Walk through childs and add all stored data.
    for (std::list<CX3DImporter_NodeElement *>::const_iterator el_it = pNodeElement.Child.begin(); el_it != pNodeElement.Child.end(); ++el_it) {
        if ((*el_it)->Type == CX3DImporter_NodeElement::ENET_Material) {
            aiColor3D tcol3;
            float tvalf;
            CX3DImporter_NodeElement_Material &tnemat = *((CX3DImporter_NodeElement_Material *)*el_it);

            tcol3.r = tnemat.AmbientIntensity, tcol3.g = tnemat.AmbientIntensity, tcol3.b = tnemat.AmbientIntensity;
            taimat.AddProperty(&tcol3, 1, AI_MATKEY_COLOR_AMBIENT);
            taimat.AddProperty(&tnemat.DiffuseColor, 1, AI_MATKEY_COLOR_DIFFUSE);
            taimat.AddProperty(&tnemat.EmissiveColor, 1, AI_MATKEY_COLOR_EMISSIVE);
            taimat.AddProperty(&tnemat.SpecularColor, 1, AI_MATKEY_COLOR_SPECULAR);
            tvalf = 1;
            taimat.AddProperty(&tvalf, 1, AI_MATKEY_SHININESS_STRENGTH);
            taimat.AddProperty(&tnemat.Shininess, 1, AI_MATKEY_SHININESS);
            tvalf = 1.0f - tnemat.Transparency;
            taimat.AddProperty(&tvalf, 1, AI_MATKEY_OPACITY);
        } // if((*el_it)->Type == CX3DImporter_NodeElement::ENET_Material)
        else if ((*el_it)->Type == CX3DImporter_NodeElement::ENET_ImageTexture) {
            CX3DImporter_NodeElement_ImageTexture &tnetex = *((CX3DImporter_NodeElement_ImageTexture *)*el_it);
            aiString url_str(tnetex.URL.c_str());
            int mode = aiTextureOp_Multiply;

            taimat.AddProperty(&url_str, AI_MATKEY_TEXTURE_DIFFUSE(0));
            taimat.AddProperty(&tnetex.RepeatS, 1, AI_MATKEY_MAPPINGMODE_U_DIFFUSE(0));
            taimat.AddProperty(&tnetex.RepeatT, 1, AI_MATKEY_MAPPINGMODE_V_DIFFUSE(0));
            taimat.AddProperty(&mode, 1, AI_MATKEY_TEXOP_DIFFUSE(0));
        } // else if((*el_it)->Type == CX3DImporter_NodeElement::ENET_ImageTexture)
        else if ((*el_it)->Type == CX3DImporter_NodeElement::ENET_TextureTransform) {
            aiUVTransform trans;
            CX3DImporter_NodeElement_TextureTransform &tnetextr = *((CX3DImporter_NodeElement_TextureTransform *)*el_it);

            trans.mTranslation = tnetextr.Translation - tnetextr.Center;
            trans.mScaling = tnetextr.Scale;
            trans.mRotation = tnetextr.Rotation;
            taimat.AddProperty(&trans, 1, AI_MATKEY_UVTRANSFORM_DIFFUSE(0));
        } // else if((*el_it)->Type == CX3DImporter_NodeElement::ENET_TextureTransform)
    } // for(std::list<CX3DImporter_NodeElement*>::const_iterator el_it = pNodeElement.Child.begin(); el_it != pNodeElement.Child.end(); el_it++)
}

void X3DImporter::Postprocess_BuildMesh(const CX3DImporter_NodeElement &pNodeElement, aiMesh **pMesh) const {
    // check argument
    if (pMesh == nullptr) throw DeadlyImportError("Postprocess_BuildMesh. pMesh is nullptr.");
    if (*pMesh != nullptr) throw DeadlyImportError("Postprocess_BuildMesh. *pMesh must be nullptr.");

    /************************************************************************************************************************************/
    /************************************************************ Geometry2D ************************************************************/
    /************************************************************************************************************************************/
    if ((pNodeElement.Type == CX3DImporter_NodeElement::ENET_Arc2D) || (pNodeElement.Type == CX3DImporter_NodeElement::ENET_ArcClose2D) ||
            (pNodeElement.Type == CX3DImporter_NodeElement::ENET_Circle2D) || (pNodeElement.Type == CX3DImporter_NodeElement::ENET_Disk2D) ||
            (pNodeElement.Type == CX3DImporter_NodeElement::ENET_Polyline2D) || (pNodeElement.Type == CX3DImporter_NodeElement::ENET_Polypoint2D) ||
            (pNodeElement.Type == CX3DImporter_NodeElement::ENET_Rectangle2D) || (pNodeElement.Type == CX3DImporter_NodeElement::ENET_TriangleSet2D)) {
        CX3DImporter_NodeElement_Geometry2D &tnemesh = *((CX3DImporter_NodeElement_Geometry2D *)&pNodeElement); // create alias for convenience
        std::vector<aiVector3D> tarr;

        tarr.reserve(tnemesh.Vertices.size());
        for (std::list<aiVector3D>::iterator it = tnemesh.Vertices.begin(); it != tnemesh.Vertices.end(); ++it)
            tarr.push_back(*it);
        *pMesh = StandardShapes::MakeMesh(tarr, static_cast<unsigned int>(tnemesh.NumIndices)); // create mesh from vertices using Assimp help.

        return; // mesh is build, nothing to do anymore.
    }
    /************************************************************************************************************************************/
    /************************************************************ Geometry3D ************************************************************/
    /************************************************************************************************************************************/
    //
    // Predefined figures
    //
    if ((pNodeElement.Type == CX3DImporter_NodeElement::ENET_Box) || (pNodeElement.Type == CX3DImporter_NodeElement::ENET_Cone) ||
            (pNodeElement.Type == CX3DImporter_NodeElement::ENET_Cylinder) || (pNodeElement.Type == CX3DImporter_NodeElement::ENET_Sphere)) {
        CX3DImporter_NodeElement_Geometry3D &tnemesh = *((CX3DImporter_NodeElement_Geometry3D *)&pNodeElement); // create alias for convenience
        std::vector<aiVector3D> tarr;

        tarr.reserve(tnemesh.Vertices.size());
        for (std::list<aiVector3D>::iterator it = tnemesh.Vertices.begin(); it != tnemesh.Vertices.end(); ++it)
            tarr.push_back(*it);

        *pMesh = StandardShapes::MakeMesh(tarr, static_cast<unsigned int>(tnemesh.NumIndices)); // create mesh from vertices using Assimp help.

        return; // mesh is build, nothing to do anymore.
    }
    //
    // Parametric figures
    //
    if (pNodeElement.Type == CX3DImporter_NodeElement::ENET_ElevationGrid) {
        CX3DImporter_NodeElement_ElevationGrid &tnemesh = *((CX3DImporter_NodeElement_ElevationGrid *)&pNodeElement); // create alias for convenience

        // at first create mesh from existing vertices.
        *pMesh = GeometryHelper_MakeMesh(tnemesh.CoordIdx, tnemesh.Vertices);
        // copy additional information from children
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Color)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_Color *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_ColorRGBA)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_ColorRGBA *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Normal)
                MeshGeometry_AddNormal(**pMesh, ((CX3DImporter_NodeElement_Normal *)*ch_it)->Value, tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_TextureCoordinate)
                MeshGeometry_AddTexCoord(**pMesh, ((CX3DImporter_NodeElement_TextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of ElevationGrid: " + to_string((*ch_it)->Type) + ".");
        } // for(std::list<CX3DImporter_NodeElement*>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == CX3DImporter_NodeElement::ENET_ElevationGrid)
    //
    // Indexed primitives sets
    //
    if (pNodeElement.Type == CX3DImporter_NodeElement::ENET_IndexedFaceSet) {
        CX3DImporter_NodeElement_IndexedSet &tnemesh = *((CX3DImporter_NodeElement_IndexedSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
                *pMesh = GeometryHelper_MakeMesh(tnemesh.CoordIndex, ((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Color)
                MeshGeometry_AddColor(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((CX3DImporter_NodeElement_Color *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_ColorRGBA)
                MeshGeometry_AddColor(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((CX3DImporter_NodeElement_ColorRGBA *)*ch_it)->Value,
                        tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Normal)
                MeshGeometry_AddNormal(**pMesh, tnemesh.CoordIndex, tnemesh.NormalIndex, ((CX3DImporter_NodeElement_Normal *)*ch_it)->Value,
                        tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_TextureCoordinate)
                MeshGeometry_AddTexCoord(**pMesh, tnemesh.CoordIndex, tnemesh.TexCoordIndex, ((CX3DImporter_NodeElement_TextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of IndexedFaceSet: " + to_string((*ch_it)->Type) + ".");
        } // for(std::list<CX3DImporter_NodeElement*>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == CX3DImporter_NodeElement::ENET_IndexedFaceSet)

    if (pNodeElement.Type == CX3DImporter_NodeElement::ENET_IndexedLineSet) {
        CX3DImporter_NodeElement_IndexedSet &tnemesh = *((CX3DImporter_NodeElement_IndexedSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
                *pMesh = GeometryHelper_MakeMesh(tnemesh.CoordIndex, ((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Color)
                MeshGeometry_AddColor(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((CX3DImporter_NodeElement_Color *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_ColorRGBA)
                MeshGeometry_AddColor(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((CX3DImporter_NodeElement_ColorRGBA *)*ch_it)->Value,
                        tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of IndexedLineSet: " + to_string((*ch_it)->Type) + ".");
        } // for(std::list<CX3DImporter_NodeElement*>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == CX3DImporter_NodeElement::ENET_IndexedLineSet)

    if ((pNodeElement.Type == CX3DImporter_NodeElement::ENET_IndexedTriangleSet) ||
            (pNodeElement.Type == CX3DImporter_NodeElement::ENET_IndexedTriangleFanSet) ||
            (pNodeElement.Type == CX3DImporter_NodeElement::ENET_IndexedTriangleStripSet)) {
        CX3DImporter_NodeElement_IndexedSet &tnemesh = *((CX3DImporter_NodeElement_IndexedSet *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
                *pMesh = GeometryHelper_MakeMesh(tnemesh.CoordIndex, ((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Color)
                MeshGeometry_AddColor(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((CX3DImporter_NodeElement_Color *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_ColorRGBA)
                MeshGeometry_AddColor(**pMesh, tnemesh.CoordIndex, tnemesh.ColorIndex, ((CX3DImporter_NodeElement_ColorRGBA *)*ch_it)->Value,
                        tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Normal)
                MeshGeometry_AddNormal(**pMesh, tnemesh.CoordIndex, tnemesh.NormalIndex, ((CX3DImporter_NodeElement_Normal *)*ch_it)->Value,
                        tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_TextureCoordinate)
                MeshGeometry_AddTexCoord(**pMesh, tnemesh.CoordIndex, tnemesh.TexCoordIndex, ((CX3DImporter_NodeElement_TextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of IndexedTriangleSet or IndexedTriangleFanSet, or \
																	IndexedTriangleStripSet: " +
                                        to_string((*ch_it)->Type) + ".");
        } // for(std::list<CX3DImporter_NodeElement*>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if((pNodeElement.Type == CX3DImporter_NodeElement::ENET_IndexedTriangleFanSet) || (pNodeElement.Type == CX3DImporter_NodeElement::ENET_IndexedTriangleStripSet))

    if (pNodeElement.Type == CX3DImporter_NodeElement::ENET_Extrusion) {
        CX3DImporter_NodeElement_IndexedSet &tnemesh = *((CX3DImporter_NodeElement_IndexedSet *)&pNodeElement); // create alias for convenience

        *pMesh = GeometryHelper_MakeMesh(tnemesh.CoordIndex, tnemesh.Vertices);

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == CX3DImporter_NodeElement::ENET_Extrusion)

    //
    // Primitives sets
    //
    if (pNodeElement.Type == CX3DImporter_NodeElement::ENET_PointSet) {
        CX3DImporter_NodeElement_Set &tnemesh = *((CX3DImporter_NodeElement_Set *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
                std::vector<aiVector3D> vec_copy;

                vec_copy.reserve(((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value.size());
                for (std::list<aiVector3D>::const_iterator it = ((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value.begin();
                        it != ((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value.end(); ++it) {
                    vec_copy.push_back(*it);
                }

                *pMesh = StandardShapes::MakeMesh(vec_copy, 1);
            }
        }

        // copy additional information from children
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Color)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_Color *)*ch_it)->Value, true);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_ColorRGBA)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_ColorRGBA *)*ch_it)->Value, true);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of PointSet: " + to_string((*ch_it)->Type) + ".");
        } // for(std::list<CX3DImporter_NodeElement*>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == CX3DImporter_NodeElement::ENET_PointSet)

    if (pNodeElement.Type == CX3DImporter_NodeElement::ENET_LineSet) {
        CX3DImporter_NodeElement_Set &tnemesh = *((CX3DImporter_NodeElement_Set *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
                *pMesh = GeometryHelper_MakeMesh(tnemesh.CoordIndex, ((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Color)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_Color *)*ch_it)->Value, true);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_ColorRGBA)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_ColorRGBA *)*ch_it)->Value, true);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of LineSet: " + to_string((*ch_it)->Type) + ".");
        } // for(std::list<CX3DImporter_NodeElement*>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == CX3DImporter_NodeElement::ENET_LineSet)

    if (pNodeElement.Type == CX3DImporter_NodeElement::ENET_TriangleFanSet) {
        CX3DImporter_NodeElement_Set &tnemesh = *((CX3DImporter_NodeElement_Set *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
                *pMesh = GeometryHelper_MakeMesh(tnemesh.CoordIndex, ((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if (nullptr == *pMesh) {
                break;
            }
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Color)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_Color *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_ColorRGBA)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_ColorRGBA *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Normal)
                MeshGeometry_AddNormal(**pMesh, tnemesh.CoordIndex, tnemesh.NormalIndex, ((CX3DImporter_NodeElement_Normal *)*ch_it)->Value,
                        tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_TextureCoordinate)
                MeshGeometry_AddTexCoord(**pMesh, tnemesh.CoordIndex, tnemesh.TexCoordIndex, ((CX3DImporter_NodeElement_TextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of TrianlgeFanSet: " + to_string((*ch_it)->Type) + ".");
        } // for(std::list<CX3DImporter_NodeElement*>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == CX3DImporter_NodeElement::ENET_TriangleFanSet)

    if (pNodeElement.Type == CX3DImporter_NodeElement::ENET_TriangleSet) {
        CX3DImporter_NodeElement_Set &tnemesh = *((CX3DImporter_NodeElement_Set *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
                std::vector<aiVector3D> vec_copy;

                vec_copy.reserve(((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value.size());
                for (std::list<aiVector3D>::const_iterator it = ((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value.begin();
                        it != ((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value.end(); ++it) {
                    vec_copy.push_back(*it);
                }

                *pMesh = StandardShapes::MakeMesh(vec_copy, 3);
            }
        }

        // copy additional information from children
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Color)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_Color *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_ColorRGBA)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_ColorRGBA *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Normal)
                MeshGeometry_AddNormal(**pMesh, tnemesh.CoordIndex, tnemesh.NormalIndex, ((CX3DImporter_NodeElement_Normal *)*ch_it)->Value,
                        tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_TextureCoordinate)
                MeshGeometry_AddTexCoord(**pMesh, tnemesh.CoordIndex, tnemesh.TexCoordIndex, ((CX3DImporter_NodeElement_TextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of TrianlgeSet: " + to_string((*ch_it)->Type) + ".");
        } // for(std::list<CX3DImporter_NodeElement*>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == CX3DImporter_NodeElement::ENET_TriangleSet)

    if (pNodeElement.Type == CX3DImporter_NodeElement::ENET_TriangleStripSet) {
        CX3DImporter_NodeElement_Set &tnemesh = *((CX3DImporter_NodeElement_Set *)&pNodeElement); // create alias for convenience

        // at first search for <Coordinate> node and create mesh.
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
                *pMesh = GeometryHelper_MakeMesh(tnemesh.CoordIndex, ((CX3DImporter_NodeElement_Coordinate *)*ch_it)->Value);
            }
        }

        // copy additional information from children
        for (std::list<CX3DImporter_NodeElement *>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it) {
            ai_assert(*pMesh);
            if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Color)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_Color *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_ColorRGBA)
                MeshGeometry_AddColor(**pMesh, ((CX3DImporter_NodeElement_ColorRGBA *)*ch_it)->Value, tnemesh.ColorPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Coordinate) {
            } // skip because already read when mesh created.
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_Normal)
                MeshGeometry_AddNormal(**pMesh, tnemesh.CoordIndex, tnemesh.NormalIndex, ((CX3DImporter_NodeElement_Normal *)*ch_it)->Value,
                        tnemesh.NormalPerVertex);
            else if ((*ch_it)->Type == CX3DImporter_NodeElement::ENET_TextureCoordinate)
                MeshGeometry_AddTexCoord(**pMesh, tnemesh.CoordIndex, tnemesh.TexCoordIndex, ((CX3DImporter_NodeElement_TextureCoordinate *)*ch_it)->Value);
            else
                throw DeadlyImportError("Postprocess_BuildMesh. Unknown child of TriangleStripSet: " + to_string((*ch_it)->Type) + ".");
        } // for(std::list<CX3DImporter_NodeElement*>::iterator ch_it = tnemesh.Child.begin(); ch_it != tnemesh.Child.end(); ++ch_it)

        return; // mesh is build, nothing to do anymore.
    } // if(pNodeElement.Type == CX3DImporter_NodeElement::ENET_TriangleStripSet)

    throw DeadlyImportError("Postprocess_BuildMesh. Unknown mesh type: " + to_string(pNodeElement.Type) + ".");
}

void X3DImporter::Postprocess_BuildNode(const X3DNodeElementBase &pNodeElement, aiNode &pSceneNode, std::list<aiMesh *> &pSceneMeshList,
        std::list<aiMaterial *> &pSceneMaterialList, std::list<aiLight *> &pSceneLightList) const {
    X3DElementList::const_iterator chit_begin = pNodeElement.Children.begin();
    X3DElementList::const_iterator chit_end = pNodeElement.Children.end();
    std::list<aiNode *> SceneNode_Child;
    std::list<unsigned int> SceneNode_Mesh;

    // At first read all metadata
    Postprocess_CollectMetadata(pNodeElement, pSceneNode);
    // check if we have deal with grouping node. Which can contain transformation or switch
    if (pNodeElement.Type == X3DElemType::ENET_Group) {
        const CX3DNodeElementGroup &tne_group = *((CX3DNodeElementGroup*)&pNodeElement); // create alias for convenience

        pSceneNode.mTransformation = tne_group.Transformation;
        if (tne_group.UseChoice) {
            // If Choice is less than zero or greater than the number of nodes in the children field, nothing is chosen.
            if ((tne_group.Choice < 0) || ((size_t)tne_group.Choice >= pNodeElement.Children.size())) {
                chit_begin = pNodeElement.Children.end();
                chit_end = pNodeElement.Children.end();
            } else {
                for (size_t i = 0; i < (size_t)tne_group.Choice; i++)
                    ++chit_begin; // forward iterator to chosen node.

                chit_end = chit_begin;
                ++chit_end; // point end iterator to next element after chosen node.
            }
        } // if(tne_group.UseChoice)
    } // if(pNodeElement.Type == CX3DImporter_NodeElement::ENET_Group)

    // Reserve memory for fast access and check children.
    for (std::list<X3DNodeElementBase *>::const_iterator it = chit_begin; it != chit_end; ++it) { // in this loop we do not read metadata because it's already read at begin.
        if ((*it)->Type == X3DElemType::ENET_Group) {
            // if child is group then create new node and do recursive call.
            aiNode *new_node = new aiNode;

            new_node->mName = (*it)->ID;
            new_node->mParent = &pSceneNode;
            SceneNode_Child.push_back(new_node);
            Postprocess_BuildNode(**it, *new_node, pSceneMeshList, pSceneMaterialList, pSceneLightList);
        } else if ((*it)->Type == X3DElemType::ENET_Shape) {
            // shape can contain only one geometry and one appearance nodes.
            Postprocess_BuildShape(*((CX3DImporter_NodeElement_Shape *)*it), SceneNode_Mesh, pSceneMeshList, pSceneMaterialList);
        } else if (((*it)->Type == X3DElemType::ENET_DirectionalLight) || ((*it)->Type == X3DElemType::ENET_PointLight) ||
                   ((*it)->Type == X3DElemType::ENET_SpotLight)) {
            Postprocess_BuildLight(*((X3DElemType *)*it), pSceneLightList);
        } else if (!PostprocessHelper_ElementIsMetadata((*it)->Type)) // skip metadata
        {
            throw DeadlyImportError("Postprocess_BuildNode. Unknown type: " + to_string((*it)->Type) + ".");
        }
    } // for(std::list<CX3DImporter_NodeElement*>::const_iterator it = chit_begin; it != chit_end; it++)

    // copy data about children and meshes to aiNode.
    if (!SceneNode_Child.empty()) {
        std::list<aiNode *>::const_iterator it = SceneNode_Child.begin();

        pSceneNode.mNumChildren = static_cast<unsigned int>(SceneNode_Child.size());
        pSceneNode.mChildren = new aiNode *[pSceneNode.mNumChildren];
        for (size_t i = 0; i < pSceneNode.mNumChildren; i++)
            pSceneNode.mChildren[i] = *it++;
    }

    if (!SceneNode_Mesh.empty()) {
        std::list<unsigned int>::const_iterator it = SceneNode_Mesh.begin();

        pSceneNode.mNumMeshes = static_cast<unsigned int>(SceneNode_Mesh.size());
        pSceneNode.mMeshes = new unsigned int[pSceneNode.mNumMeshes];
        for (size_t i = 0; i < pSceneNode.mNumMeshes; i++)
            pSceneNode.mMeshes[i] = *it++;
    }

    // that's all. return to previous deals
}

void X3DImporter::Postprocess_BuildShape(const CX3DImporter_NodeElement_Shape &pShapeNodeElement, std::list<unsigned int> &pNodeMeshInd,
        std::list<aiMesh *> &pSceneMeshList, std::list<aiMaterial *> &pSceneMaterialList) const {
    aiMaterial *tmat = nullptr;
    aiMesh *tmesh = nullptr;
    X3DElemType mesh_type = X3DElemType::ENET_Invalid;
    unsigned int mat_ind = 0;

    for (X3DElementList::const_iterator it = pShapeNodeElement.Children.begin(); it != pShapeNodeElement.Children.end(); ++it) {
        if (PostprocessHelper_ElementIsMesh((*it)->Type)) {
            Postprocess_BuildMesh(**it, &tmesh);
            if (tmesh != nullptr) {
                // if mesh successfully built then add data about it to arrays
                pNodeMeshInd.push_back(static_cast<unsigned int>(pSceneMeshList.size()));
                pSceneMeshList.push_back(tmesh);
                // keep mesh type. Need above for texture coordinate generation.
                mesh_type = (*it)->Type;
            }
        } else if ((*it)->Type == X3DElemType::ENET_Appearance) {
            Postprocess_BuildMaterial(**it, &tmat);
            if (tmat != nullptr) {
                // if material successfully built then add data about it to array
                mat_ind = static_cast<unsigned int>(pSceneMaterialList.size());
                pSceneMaterialList.push_back(tmat);
            }
        }
    } // for(std::list<CX3DImporter_NodeElement*>::const_iterator it = pShapeNodeElement.Child.begin(); it != pShapeNodeElement.Child.end(); it++)

    // associate read material with read mesh.
    if ((tmesh != nullptr) && (tmat != nullptr)) {
        tmesh->mMaterialIndex = mat_ind;
        // Check texture mapping. If material has texture but mesh has no texture coordinate then try to ask Assimp to generate texture coordinates.
        if ((tmat->GetTextureCount(aiTextureType_DIFFUSE) != 0) && !tmesh->HasTextureCoords(0)) {
            int32_t tm;
            aiVector3D tvec3;

            switch (mesh_type) {
            case X3DElemType::ENET_Box:
                tm = aiTextureMapping_BOX;
                break;
            case X3DElemType::ENET_Cone:
            case X3DElemType::ENET_Cylinder:
                tm = aiTextureMapping_CYLINDER;
                break;
            case X3DElemType::ENET_Sphere:
                tm = aiTextureMapping_SPHERE;
                break;
            default:
                tm = aiTextureMapping_PLANE;
                break;
            } // switch(mesh_type)

            tmat->AddProperty(&tm, 1, AI_MATKEY_MAPPING_DIFFUSE(0));
        } // if((tmat->GetTextureCount(aiTextureType_DIFFUSE) != 0) && !tmesh->HasTextureCoords(0))
    } // if((tmesh != nullptr) && (tmat != nullptr))
}

void X3DImporter::Postprocess_CollectMetadata(const CX3DImporter_NodeElement &pNodeElement, aiNode &pSceneNode) const {
    X3DElementList meta_list;
    size_t meta_idx;

    PostprocessHelper_CollectMetadata(pNodeElement, meta_list); // find metadata in current node element.
    if (!meta_list.empty()) {
        if (pSceneNode.mMetaData != nullptr) {
            throw DeadlyImportError("Postprocess. MetaData member in node are not nullptr. Something went wrong.");
        }

        // copy collected metadata to output node.
        pSceneNode.mMetaData = aiMetadata::Alloc(static_cast<unsigned int>(meta_list.size()));
        meta_idx = 0;
        for (X3DElementList::const_iterator it = meta_list.begin(); it != meta_list.end(); ++it, ++meta_idx) {
            CX3DImporter_NodeElement_Meta *cur_meta = (CX3DImporter_NodeElement_Meta *)*it;

            // due to limitations we can add only first element of value list.
            // Add an element according to its type.
            if ((*it)->Type == CX3DImporter_NodeElement::ENET_MetaBoolean) {
                if (((CX3DImporter_NodeElement_MetaBoolean *)cur_meta)->Value.size() > 0)
                    pSceneNode.mMetaData->Set(static_cast<unsigned int>(meta_idx), cur_meta->Name, *(((CX3DImporter_NodeElement_MetaBoolean *)cur_meta)->Value.begin()));
            } else if ((*it)->Type == CX3DImporter_NodeElement::ENET_MetaDouble) {
                if (((CX3DImporter_NodeElement_MetaDouble *)cur_meta)->Value.size() > 0)
                    pSceneNode.mMetaData->Set(static_cast<unsigned int>(meta_idx), cur_meta->Name, (float)*(((CX3DImporter_NodeElement_MetaDouble *)cur_meta)->Value.begin()));
            } else if ((*it)->Type == CX3DImporter_NodeElement::ENET_MetaFloat) {
                if (((CX3DImporter_NodeElement_MetaFloat *)cur_meta)->Value.size() > 0)
                    pSceneNode.mMetaData->Set(static_cast<unsigned int>(meta_idx), cur_meta->Name, *(((CX3DImporter_NodeElement_MetaFloat *)cur_meta)->Value.begin()));
            } else if ((*it)->Type == CX3DImporter_NodeElement::ENET_MetaInteger) {
                if (((CX3DImporter_NodeElement_MetaInteger *)cur_meta)->Value.size() > 0)
                    pSceneNode.mMetaData->Set(static_cast<unsigned int>(meta_idx), cur_meta->Name, *(((CX3DImporter_NodeElement_MetaInteger *)cur_meta)->Value.begin()));
            } else if ((*it)->Type == CX3DImporter_NodeElement::ENET_MetaString) {
                if (((CX3DImporter_NodeElement_MetaString *)cur_meta)->Value.size() > 0) {
                    aiString tstr(((CX3DImporter_NodeElement_MetaString *)cur_meta)->Value.begin()->data());

                    pSceneNode.mMetaData->Set(static_cast<unsigned int>(meta_idx), cur_meta->Name, tstr);
                }
            } else {
                throw DeadlyImportError("Postprocess. Unknown metadata type.");
            } // if((*it)->Type == CX3DImporter_NodeElement::ENET_Meta*) else
        } // for(std::list<CX3DImporter_NodeElement*>::const_iterator it = meta_list.begin(); it != meta_list.end(); it++)
    } // if( !meta_list.empty() )
}


#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER

} // namespace Assimp
