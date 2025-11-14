/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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

#include "AssetLib/VRML/VrmlConverter.hpp"
#include "X3DImporter.hpp"
#include "X3DImporter_Macro.hpp"

#include <assimp/DefaultIOSystem.h>

// Header files, stdlib.
#include <iterator>
#include <memory>

#if defined(ASSIMP_BUILD_NO_VRML_IMPORTER)
#define X3D_FORMATS_DESCR_STR "Extensible 3D(X3D, X3DB) Importer"
#define X3D_FORMATS_EXTENSIONS_STR "x3d x3db"
#else
#define X3D_FORMATS_DESCR_STR "VRML(WRL, X3DV) and Extensible 3D(X3D, X3DB) Importer"
#define X3D_FORMATS_EXTENSIONS_STR "wrl x3d x3db x3dv"
#endif // #if defined(ASSIMP_BUILD_NO_VRML_IMPORTER)

namespace Assimp {

/// Constant which holds the importer description
const aiImporterDesc X3DImporter::Description = {
    X3D_FORMATS_DESCR_STR,
    "smalcom",
    "",
    "See documentation in source code. Chapter: Limitations.",
    aiImporterFlags_SupportTextFlavour | aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_LimitedSupport | aiImporterFlags_Experimental,
    0,
    0,
    0,
    0,
    X3D_FORMATS_EXTENSIONS_STR
};

bool X3DImporter::isNodeEmpty(XmlNode &node) {
    return node.first_child().empty();
}

void X3DImporter::checkNodeMustBeEmpty(XmlNode &node) {
    if (!isNodeEmpty(node)) throw DeadlyImportError(std::string("Node <") + node.name() + "> must be empty.");
}

void X3DImporter::skipUnsupportedNode(const std::string &pParentNodeName, XmlNode &node) {
    static const size_t Uns_Skip_Len = 192;
    static constexpr char const * Uns_Skip[Uns_Skip_Len] = {
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

    if (nn.empty()) {
        const std::string nv = node.value();
        if (!nv.empty()) {
            LogInfo("Ignoring comment \"" + nv + "\" in " + pParentNodeName + ".");
            return;
        }
    }

    bool found = false;

    for (size_t i = 0; i < Uns_Skip_Len; i++) {
        if (nn == Uns_Skip[i]) {
            found = true;
        }
    }

    if (!found) throw DeadlyImportError("Unknown node \"" + nn + "\" in " + pParentNodeName + ".");

    LogInfo("Skipping node \"" + nn + "\" in " + pParentNodeName + ".");
}

X3DImporter::X3DImporter() :
        mNodeElementCur(nullptr),
        mScene(nullptr),
        mpIOHandler(nullptr) {
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
    if (!fileStream) {
        throw DeadlyImportError("Failed to open file " + file + ".");
    }

    XmlParser theParser;
    if (!theParser.parse(fileStream.get())) {
        return;
    }
    ParseFile(theParser);
}

void X3DImporter::ParseFile(std::istream &myIstream) {
    XmlParser theParser;
    if (!theParser.parse(myIstream)) {
        LogInfo("ParseFile(): ERROR: failed to convert VRML istream to xml");
        return;
    }
    ParseFile(theParser);
}

void X3DImporter::ParseFile(XmlParser &theParser) {
    XmlNode *node = theParser.findNode("X3D");
    if (nullptr == node) {
        return;
    }

    for (auto &currentNode : node->children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "head") {
            readHead(currentNode);
        } else if (currentName == "Scene") {
            readScene(currentNode);
        } else {
            skipUnsupportedNode("X3D", currentNode);
        }
    }
}

bool X3DImporter::CanRead(const std::string &pFile, IOSystem * /*pIOHandler*/, bool checkSig) const {
    if (checkSig) {
        if (GetExtension(pFile) == "x3d")
            return true;
    }

    return false;
}

void X3DImporter::InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) {
    mpIOHandler = pIOHandler;

    Clear();
    std::stringstream ss = ConvertVrmlFileToX3dXmlFile(pFile);
    const bool isReadFromMem{ ss.str().length() > 0 };
    if (!isReadFromMem) {
        std::shared_ptr<IOStream> stream(pIOHandler->Open(pFile, "rb"));
        if (!stream) {
            throw DeadlyImportError("Could not open file for reading");
        }
    }
    std::string::size_type slashPos = pFile.find_last_of("\\/");

    mScene = pScene;
    pScene->mRootNode = new aiNode(pFile);
    pScene->mRootNode->mParent = nullptr;
    pScene->mFlags |= AI_SCENE_FLAGS_ALLOW_SHARED;

    if (isReadFromMem) {
        ParseFile(ss);
    } else {
        pIOHandler->PushDirectory(slashPos == std::string::npos ? std::string() : pFile.substr(0, slashPos + 1));
        ParseFile(pFile, pIOHandler);
        pIOHandler->PopDirectory();
    }

    //search for root node element

    mNodeElementCur = NodeElement_List.front();
    if (mNodeElementCur == nullptr) {
        return;
    }
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

void X3DImporter::readHead(XmlNode &node) {
    std::vector<meta_entry> metaArray;
    for (auto currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "meta") {
            //checkNodeMustBeEmpty(node);
            meta_entry entry;
            if (XmlParser::getStdStrAttribute(currentNode, "name", entry.name)) {
                XmlParser::getStdStrAttribute(currentNode, "content", entry.value);
                metaArray.emplace_back(entry);
            }
        }
        // TODO: check if other node types in head should be supported
    }
    mScene->mMetaData = aiMetadata::Alloc(static_cast<unsigned int>(metaArray.size()));
    unsigned int i = 0;
    for (const auto& currentMeta : metaArray) {
        mScene->mMetaData->Set(i, currentMeta.name, aiString(currentMeta.value));
        ++i;
    }
}

void X3DImporter::readChildNodes(XmlNode &node, const std::string &pParentNodeName) {
    if (node.empty()) {
        return;
    }
    for (auto currentNode : node.children()) {
        const std::string &currentName = currentNode.name();
        if (currentName == "Shape")
            readShape(currentNode);
        else if (currentName == "Group") {
            startReadGroup(currentNode);
            readChildNodes(currentNode, "Group");
            endReadGroup();
        } else if (currentName == "StaticGroup") {
            startReadStaticGroup(currentNode);
            readChildNodes(currentNode, "StaticGroup");
            endReadStaticGroup();
        } else if (currentName == "Transform") {
            startReadTransform(currentNode);
            readChildNodes(currentNode, "Transform");
            endReadTransform();
        } else if (currentName == "Switch") {
            startReadSwitch(currentNode);
            readChildNodes(currentNode, "Switch");
            endReadSwitch();
        } else if (currentName == "DirectionalLight") {
            readDirectionalLight(currentNode);
        } else if (currentName == "PointLight") {
            readPointLight(currentNode);
        } else if (currentName == "SpotLight") {
            readSpotLight(currentNode);
        } else if (currentName == "Inline") {
            readInline(currentNode);
        } else if (!checkForMetadataNode(currentNode)) {
            skipUnsupportedNode(pParentNodeName, currentNode);
        }
    }
}

void X3DImporter::readScene(XmlNode &node) {
    ParseHelper_Group_Begin(true);
    readChildNodes(node, "Scene");
    ParseHelper_Node_Exit();
}

/*********************************************************************************************************************************************/
/************************************************************ Functions: find set ************************************************************/
/*********************************************************************************************************************************************/

bool X3DImporter::FindNodeElement_FromRoot(const std::string &pID, const X3DElemType pType, X3DNodeElementBase **pElement) {
    for (std::list<X3DNodeElementBase *>::iterator it = NodeElement_List.begin(); it != NodeElement_List.end(); ++it) {
        if (((*it)->Type == pType) && ((*it)->ID == pID)) {
            if (pElement != nullptr) *pElement = *it;

            return true;
        }
    } // for(std::list<CX3DImporter_NodeElement*>::iterator it = NodeElement_List.begin(); it != NodeElement_List.end(); it++)

    return false;
}

bool X3DImporter::FindNodeElement_FromNode(X3DNodeElementBase *pStartNode, const std::string &pID,
        const X3DElemType pType, X3DNodeElementBase **pElement) {
    bool found = false; // flag: true - if requested element is found.

    // Check if pStartNode - this is the element, we are looking for.
    if ((pStartNode->Type == pType) && (pStartNode->ID == pID)) {
        found = true;
        if (pElement != nullptr) {
            *pElement = pStartNode;
        }

        goto fne_fn_end;
    } // if((pStartNode->Type() == pType) && (pStartNode->ID() == pID))

    // Check childs of pStartNode.
    for (std::list<X3DNodeElementBase *>::iterator ch_it = pStartNode->Children.begin(); ch_it != pStartNode->Children.end(); ++ch_it) {
        found = FindNodeElement_FromNode(*ch_it, pID, pType, pElement);
        if (found) {
            break;
        }
    } // for(std::list<CX3DImporter_NodeElement*>::iterator ch_it = it->Children.begin(); ch_it != it->Children.end(); ch_it++)

fne_fn_end:

    return found;
}

bool X3DImporter::FindNodeElement(const std::string &pID, const X3DElemType pType, X3DNodeElementBase **pElement) {
    X3DNodeElementBase *tnd = mNodeElementCur; // temporary pointer to node.
    bool static_search = false; // flag: true if searching in static node.

    // At first check if we have deal with static node. Go up through parent nodes and check flag.
    while (tnd != nullptr) {
        if (tnd->Type == X3DElemType::ENET_Group) {
            if (((X3DNodeElementGroup *)tnd)->Static) {
                static_search = true; // Flag found, stop walking up. Node with static flag will holded in tnd variable.
                break;
            }
        }

        tnd = tnd->Parent; // go up in graph.
    } // while (tnd != nullptr)

    // at now call appropriate search function.
    if (static_search) {
        return FindNodeElement_FromNode(tnd, pID, pType, pElement);
    } else {
        return FindNodeElement_FromRoot(pID, pType, pElement);
    }
}

/*********************************************************************************************************************************************/
/************************************************************ Functions: parse set ***********************************************************/
/*********************************************************************************************************************************************/

void X3DImporter::ParseHelper_Group_Begin(const bool pStatic) {
    X3DNodeElementGroup *new_group = new X3DNodeElementGroup(mNodeElementCur, pStatic); // create new node with current node as parent.

    // if we are adding not the root element then add new element to current element child list.
    if (mNodeElementCur != nullptr) {
        mNodeElementCur->Children.push_back(new_group);
    }

    NodeElement_List.push_back(new_group); // it's a new element - add it to list.
    mNodeElementCur = new_group; // switch current element to new one.
}

void X3DImporter::ParseHelper_Node_Enter(X3DNodeElementBase *pNode) {
    ai_assert(nullptr != pNode);

    mNodeElementCur->Children.push_back(pNode); // add new element to current element child list.
    mNodeElementCur = pNode; // switch current element to new one.
}

void X3DImporter::ParseHelper_Node_Exit() {
    // check if we can walk up.
    if (mNodeElementCur != nullptr) {
        mNodeElementCur = mNodeElementCur->Parent;
    }
}

} // namespace Assimp

#endif // !ASSIMP_BUILD_NO_X3D_IMPORTER
