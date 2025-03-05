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
#ifndef INCLUDED_AI_X3D_IMPORTER_H
#define INCLUDED_AI_X3D_IMPORTER_H

#include "X3DImporter_Node.hpp"

#include <assimp/BaseImporter.h>
#include <assimp/XmlParser.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/ProgressHandler.hpp>

#include <list>
#include <string>

namespace Assimp {
AI_WONT_RETURN inline void Throw_ArgOutOfRange(const std::string &argument) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_CloseNotFound(const std::string &node) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_ConvertFail_Str2ArrF(const std::string &nodeName, const std::string &pAttrValue) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_ConvertFail_Str2ArrD(const std::string &nodeName, const std::string &pAttrValue) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_ConvertFail_Str2ArrB(const std::string &nodeName, const std::string &pAttrValue) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_ConvertFail_Str2ArrI(const std::string &nodeName, const std::string &pAttrValue) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_DEF_And_USE(const std::string &nodeName) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_IncorrectAttr(const std::string &nodeName, const std::string &pAttrName) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_IncorrectAttrValue(const std::string &nodeName, const std::string &pAttrName) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_MoreThanOnceDefined(const std::string &nodeName, const std::string &pNodeType, const std::string &pDescription) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_TagCountIncorrect(const std::string &pNode) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN inline void Throw_USE_NotFound(const std::string &nodeName, const std::string &pAttrValue) AI_WONT_RETURN_SUFFIX;

inline void Throw_ArgOutOfRange(const std::string &argument) {
    throw DeadlyImportError("Argument value is out of range for: \"" + argument + "\".");
}

inline void Throw_CloseNotFound(const std::string &node) {
    throw DeadlyImportError("Close tag for node <" + node + "> not found. Seems file is corrupt.");
}

inline void Throw_ConvertFail_Str2ArrF(const std::string &nodeName, const std::string &pAttrValue) {
    throw DeadlyImportError("In <" + nodeName + "> failed to convert attribute value \"" + pAttrValue +
                            "\" from string to array of floats.");
}

inline void Throw_ConvertFail_Str2ArrD(const std::string &nodeName, const std::string &pAttrValue) {
    throw DeadlyImportError("In <" + nodeName + "> failed to convert attribute value \"" + pAttrValue +
                            "\" from string to array of doubles.");
}

inline void Throw_ConvertFail_Str2ArrB(const std::string &nodeName, const std::string &pAttrValue) {
    throw DeadlyImportError("In <" + nodeName + "> failed to convert attribute value \"" + pAttrValue +
                            "\" from string to array of booleans.");
}

inline void Throw_ConvertFail_Str2ArrI(const std::string &nodeName, const std::string &pAttrValue) {
    throw DeadlyImportError("In <" + nodeName + "> failed to convert attribute value \"" + pAttrValue +
                            "\" from string to array of integers.");
}

inline void Throw_DEF_And_USE(const std::string &nodeName) {
    throw DeadlyImportError("\"DEF\" and \"USE\" can not be defined both in <" + nodeName + ">.");
}

inline void Throw_IncorrectAttr(const std::string &nodeName, const std::string &pAttrName) {
    throw DeadlyImportError("Node <" + nodeName + "> has incorrect attribute \"" + pAttrName + "\".");
}

inline void Throw_IncorrectAttrValue(const std::string &nodeName, const std::string &pAttrName) {
    throw DeadlyImportError("Attribute \"" + pAttrName + "\" in node <" + nodeName + "> has incorrect value.");
}

inline void Throw_MoreThanOnceDefined(const std::string &nodeName, const std::string &pNodeType, const std::string &pDescription) {
    throw DeadlyImportError("\"" + pNodeType + "\" node can be used only once in " + nodeName + ". Description: " + pDescription);
}

inline void Throw_TagCountIncorrect(const std::string &pNode) {
    throw DeadlyImportError("Count of open and close tags for node <" + pNode + "> are not equivalent. Seems file is corrupt.");
}

inline void Throw_USE_NotFound(const std::string &nodeName, const std::string &pAttrValue) {
    throw DeadlyImportError("Not found node with name \"" + pAttrValue + "\" in <" + nodeName + ">.");
}

inline void LogInfo(const std::string &message) {
    DefaultLogger::get()->info(message);
}

/// \class X3DImporter
/// Class that holding scene graph which include: groups, geometry, metadata etc.
///
/// Limitations.
///
/// Pay attention that X3D is format for interactive graphic and simulations for web browsers.
/// So not all features can be imported using Assimp.
///
/// Unsupported nodes:
/// 	CAD geometry component:
///			"CADAssembly", "CADFace", "CADLayer", "CADPart", "IndexedQuadSet", "QuadSet"
///		Core component:
///			"ROUTE", "ExternProtoDeclare", "ProtoDeclare", "ProtoInstance", "ProtoInterface", "WorldInfo"
///		Distributed interactive simulation (DIS) component:
///			"DISEntityManager", "DISEntityTypeMapping", "EspduTransform", "ReceiverPdu", "SignalPdu", "TransmitterPdu"
///		Cube map environmental texturing component:
///			"ComposedCubeMapTexture", "GeneratedCubeMapTexture", "ImageCubeMapTexture"
///		Environmental effects component:
///			"Background", "Fog", "FogCoordinate", "LocalFog", "TextureBackground"
///		Environmental sensor component:
///			"ProximitySensor", "TransformSensor", "VisibilitySensor"
///		Followers component:
///			"ColorChaser", "ColorDamper", "CoordinateChaser", "CoordinateDamper", "OrientationChaser", "OrientationDamper", "PositionChaser",
///			"PositionChaser2D", "PositionDamper", "PositionDamper2D", "ScalarChaser", "ScalarDamper", "TexCoordChaser2D", "TexCoordDamper2D"
///		Geospatial component:
///			"GeoCoordinate", "GeoElevationGrid", "GeoLocation", "GeoLOD", "GeoMetadata", "GeoOrigin", "GeoPositionInterpolator", "GeoProximitySensor",
///			"GeoTouchSensor", "GeoTransform", "GeoViewpoint"
///		Humanoid Animation (H-Anim) component:
///			"HAnimDisplacer", "HAnimHumanoid", "HAnimJoint", "HAnimSegment", "HAnimSite"
///		Interpolation component:
///			"ColorInterpolator", "CoordinateInterpolator", "CoordinateInterpolator2D", "EaseInEaseOut", "NormalInterpolator", "OrientationInterpolator",
///			"PositionInterpolator", "PositionInterpolator2D", "ScalarInterpolator", "SplinePositionInterpolator", "SplinePositionInterpolator2D",
///			"SplineScalarInterpolator", "SquadOrientationInterpolator",
///		Key device sensor component:
///			"KeySensor", "StringSensor"
///		Layering component:
///			"Layer", "LayerSet", "Viewport"
///		Layout component:
///			"Layout", "LayoutGroup", "LayoutLayer", "ScreenFontStyle", "ScreenGroup"
///		Navigation component:
///			"Billboard", "Collision", "LOD", "NavigationInfo", "OrthoViewpoint", "Viewpoint", "ViewpointGroup"
///		Networking component:
///			"EXPORT", "IMPORT", "Anchor", "LoadSensor"
///		NURBS component:
///			"Contour2D", "ContourPolyline2D", "CoordinateDouble", "NurbsCurve", "NurbsCurve2D", "NurbsOrientationInterpolator", "NurbsPatchSurface",
///			"NurbsPositionInterpolator", "NurbsSet", "NurbsSurfaceInterpolator", "NurbsSweptSurface", "NurbsSwungSurface", "NurbsTextureCoordinate",
///			"NurbsTrimmedSurface"
///		Particle systems component:
///			"BoundedPhysicsModel", "ConeEmitter", "ExplosionEmitter", "ForcePhysicsModel", "ParticleSystem", "PointEmitter", "PolylineEmitter",
///			"SurfaceEmitter", "VolumeEmitter", "WindPhysicsModel"
///		Picking component:
///			"LinePickSensor", "PickableGroup", "PointPickSensor", "PrimitivePickSensor", "VolumePickSensor"
///		Pointing device sensor component:
///			"CylinderSensor", "PlaneSensor", "SphereSensor", "TouchSensor"
///		Rendering component:
///			"ClipPlane"
///		Rigid body physics:
///			"BallJoint", "CollidableOffset", "CollidableShape", "CollisionCollection", "CollisionSensor", "CollisionSpace", "Contact", "DoubleAxisHingeJoint",
///			"MotorJoint", "RigidBody", "RigidBodyCollection", "SingleAxisHingeJoint", "SliderJoint", "UniversalJoint"
///		Scripting component:
///			"Script"
///		Programmable shaders component:
///			"ComposedShader", "FloatVertexAttribute", "Matrix3VertexAttribute", "Matrix4VertexAttribute", "PackagedShader", "ProgramShader", "ShaderPart",
///			"ShaderProgram",
///		Shape component:
///			"FillProperties", "LineProperties", "TwoSidedMaterial"
///		Sound component:
///			"AudioClip", "Sound"
///		Text component:
///			"FontStyle", "Text"
///		Texturing3D Component:
///			"ComposedTexture3D", "ImageTexture3D", "PixelTexture3D", "TextureCoordinate3D", "TextureCoordinate4D", "TextureTransformMatrix3D",
///			"TextureTransform3D"
///		Texturing component:
///			"MovieTexture", "MultiTexture", "MultiTextureCoordinate", "MultiTextureTransform", "PixelTexture", "TextureCoordinateGenerator",
///			"TextureProperties",
///		Time component:
///			"TimeSensor"
///		Event Utilities component:
///			"BooleanFilter", "BooleanSequencer", "BooleanToggle", "BooleanTrigger", "IntegerSequencer", "IntegerTrigger", "TimeTrigger",
///		Volume rendering component:
///			"BlendedVolumeStyle", "BoundaryEnhancementVolumeStyle", "CartoonVolumeStyle", "ComposedVolumeStyle", "EdgeEnhancementVolumeStyle",
///			"IsoSurfaceVolumeData", "OpacityMapVolumeStyle", "ProjectionVolumeStyle", "SegmentedVolumeData", "ShadedVolumeStyle",
///			"SilhouetteEnhancementVolumeStyle", "ToneMappedVolumeStyle", "VolumeData"
///
/// Supported nodes:
///		Core component:
///			"MetadataBoolean", "MetadataDouble", "MetadataFloat", "MetadataInteger", "MetadataSet", "MetadataString"
///		Geometry2D component:
///			"Arc2D", "ArcClose2D", "Circle2D", "Disk2D", "Polyline2D", "Polypoint2D", "Rectangle2D", "TriangleSet2D"
///		Geometry3D component:
///			"Box", "Cone", "Cylinder", "ElevationGrid", "Extrusion", "IndexedFaceSet", "Sphere"
///		Grouping component:
///			"Group", "StaticGroup", "Switch", "Transform"
///		Lighting component:
///			"DirectionalLight", "PointLight", "SpotLight"
///		Networking component:
///			"Inline"
///		Rendering component:
///			"Color", "ColorRGBA", "Coordinate", "IndexedLineSet", "IndexedTriangleFanSet", "IndexedTriangleSet", "IndexedTriangleStripSet", "LineSet",
///			"PointSet", "TriangleFanSet", "TriangleSet", "TriangleStripSet", "Normal"
///		Shape component:
///			"Shape", "Appearance", "Material"
///		Texturing component:
///			"ImageTexture", "TextureCoordinate", "TextureTransform"
///
/// Limitations of attribute "USE".
/// If "USE" is set then node must be empty, like that:
///		<Node USE='name'/>
/// not the
///		<Node USE='name'><!-- something --> </Node>
///
/// Ignored attributes: "creaseAngle", "convex", "solid".
///
/// Texture coordinates generating: only for Sphere, Cone, Cylinder. In all other case used PLANE mapping.
///		It's better that Assimp main code has powerful texture coordinates generator. Then is not needed to
///		duplicate this code in every importer.
///
/// Lighting limitations.
///		If light source placed in some group with "DEF" set. And after that some node is use it group with "USE" attribute then
///		you will get error about duplicate light sources. That's happening because Assimp require names for lights but do not like
///		duplicates of it )).
///
///	Color for faces.
/// That's happening when attribute "colorPerVertex" is set to "false". But Assimp do not hold how many colors has mesh and require
/// equal length for mVertices and mColors. You will see the colors but vertices will use call which last used in "colorIdx".
///
///	That's all for now. Enjoy
///

using X3DElementList = std::list<X3DNodeElementBase *>;

class X3DImporter : public BaseImporter {
public:
    std::list<X3DNodeElementBase *> NodeElement_List; ///< All elements of scene graph.

public:
    /// Default constructor.
    X3DImporter();

    /// Default destructor.
    ~X3DImporter();

    /***********************************************/
    /******** Functions: parse set, public *********/
    /***********************************************/

    /// Parse X3D file and fill scene graph. The function has no return value. Result can be found by analyzing the generated graph.
    /// Also exception can be thrown if trouble will found.
    /// \param [in] pFile - name of file to be parsed.
    /// \param [in] pIOHandler - pointer to IO helper object.
    void ParseFile(const std::string &file, IOSystem *pIOHandler);
    void ParseFile(std::istream &myIstream);
    void ParseFile(XmlParser &theParser);
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool pCheckSig) const;
    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler);
    const aiImporterDesc *GetInfo() const;
    void Clear();

private:
    X3DNodeElementBase *MACRO_USE_CHECKANDAPPLY(XmlNode &node, const std::string &pDEF, const std::string &pUSE, X3DElemType pType, X3DNodeElementBase *pNE);
    bool isNodeEmpty(XmlNode &node);
    void checkNodeMustBeEmpty(XmlNode &node);
    void skipUnsupportedNode(const std::string &pParentNodeName, XmlNode &node);
    void readHead(XmlNode &node);
    void readChildNodes(XmlNode &node, const std::string &pParentNodeName);
    void readScene(XmlNode &node);

    bool FindNodeElement_FromRoot(const std::string &pID, const X3DElemType pType, X3DNodeElementBase **pElement);
    bool FindNodeElement_FromNode(X3DNodeElementBase *pStartNode, const std::string &pID,
            const X3DElemType pType, X3DNodeElementBase **pElement);
    bool FindNodeElement(const std::string &pID, const X3DElemType pType, X3DNodeElementBase **pElement);
    void ParseHelper_Group_Begin(const bool pStatic = false);
    void ParseHelper_Node_Enter(X3DNodeElementBase *pNode);
    void ParseHelper_Node_Exit();

    // 2D geometry
    void readArc2D(XmlNode &node);
    void readArcClose2D(XmlNode &node);
    void readCircle2D(XmlNode &node);
    void readDisk2D(XmlNode &node);
    void readPolyline2D(XmlNode &node);
    void readPolypoint2D(XmlNode &node);
    void readRectangle2D(XmlNode &node);
    void readTriangleSet2D(XmlNode &node);

    // 3D geometry
    void readBox(XmlNode &node);
    void readCone(XmlNode &node);
    void readCylinder(XmlNode &node);
    void readElevationGrid(XmlNode &node);
    void readExtrusion(XmlNode &node);
    void readIndexedFaceSet(XmlNode &node);
    void readSphere(XmlNode &node);

    // group
    void startReadGroup(XmlNode &node);
    void endReadGroup();
    void startReadStaticGroup(XmlNode &node);
    void endReadStaticGroup();
    void startReadSwitch(XmlNode &node);
    void endReadSwitch();
    void startReadTransform(XmlNode &node);
    void endReadTransform();

    // light
    void readDirectionalLight(XmlNode &node);
    void readPointLight(XmlNode &node);
    void readSpotLight(XmlNode &node);

    // metadata
    bool checkForMetadataNode(XmlNode &node);
    void childrenReadMetadata(XmlNode &node, X3DNodeElementBase *pParentElement, const std::string &pNodeName);
    void readMetadataBoolean(XmlNode &node);
    void readMetadataDouble(XmlNode &node);
    void readMetadataFloat(XmlNode &node);
    void readMetadataInteger(XmlNode &node);
    void readMetadataSet(XmlNode &node);
    void readMetadataString(XmlNode &node);

    // networking
    void readInline(XmlNode &node);

    // postprocessing
    aiMatrix4x4 PostprocessHelper_Matrix_GlobalToCurrent() const;
    void PostprocessHelper_CollectMetadata(const X3DNodeElementBase &pNodeElement, std::list<X3DNodeElementBase *> &pList) const;
    bool PostprocessHelper_ElementIsMetadata(const X3DElemType pType) const;
    bool PostprocessHelper_ElementIsMesh(const X3DElemType pType) const;
    void Postprocess_BuildLight(const X3DNodeElementBase &pNodeElement, std::list<aiLight *> &pSceneLightList) const;
    void Postprocess_BuildMaterial(const X3DNodeElementBase &pNodeElement, aiMaterial **pMaterial) const;
    void Postprocess_BuildMesh(const X3DNodeElementBase &pNodeElement, aiMesh **pMesh) const;
    void Postprocess_BuildNode(const X3DNodeElementBase &pNodeElement, aiNode &pSceneNode, std::list<aiMesh *> &pSceneMeshList,
            std::list<aiMaterial *> &pSceneMaterialList, std::list<aiLight *> &pSceneLightList) const;
    void Postprocess_BuildShape(const X3DNodeElementShape &pShapeNodeElement, std::list<unsigned int> &pNodeMeshInd,
            std::list<aiMesh *> &pSceneMeshList, std::list<aiMaterial *> &pSceneMaterialList) const;
    void Postprocess_CollectMetadata(const X3DNodeElementBase &pNodeElement, aiNode &pSceneNode) const;

    // rendering
    void readColor(XmlNode &node);
    void readColorRGBA(XmlNode &node);
    void readCoordinate(XmlNode &node);
    void readIndexedLineSet(XmlNode &node);
    void readIndexedTriangleFanSet(XmlNode &node);
    void readIndexedTriangleSet(XmlNode &node);
    void readIndexedTriangleStripSet(XmlNode &node);
    void readLineSet(XmlNode &node);
    void readPointSet(XmlNode &node);
    void readTriangleFanSet(XmlNode &node);
    void readTriangleSet(XmlNode &node);
    void readTriangleStripSet(XmlNode &node);
    void readNormal(XmlNode &node);

    // shape
    void readShape(XmlNode &node);
    void readAppearance(XmlNode &node);
    void readMaterial(XmlNode &node);

    // texturing
    void readImageTexture(XmlNode &node);
    void readTextureCoordinate(XmlNode &node);
    void readTextureTransform(XmlNode &node);

    static const aiImporterDesc Description;
    X3DNodeElementBase *mNodeElementCur;
    aiScene *mScene;
    IOSystem *mpIOHandler;
}; // class X3DImporter

} // namespace Assimp

#endif // INCLUDED_AI_X3D_IMPORTER_H
