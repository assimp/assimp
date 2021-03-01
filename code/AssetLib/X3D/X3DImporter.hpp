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
/// \file   X3DImporter.hpp
/// \brief  X3D-format files importer for Assimp.
/// \date   2015-2016
/// \author smal.root@gmail.com
// Thanks to acorn89 for support.

#ifndef INCLUDED_AI_X3D_IMPORTER_H
#define INCLUDED_AI_X3D_IMPORTER_H

// Header files, Assimp.
#include <assimp/BaseImporter.h>
#include <assimp/XmlParser.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/ProgressHandler.hpp>

#include <list>

namespace Assimp {

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
enum class X3DElemType {
    ENET_Group, ///< Element has type "Group".
    ENET_MetaBoolean, ///< Element has type "Metadata boolean".
    ENET_MetaDouble, ///< Element has type "Metadata double".
    ENET_MetaFloat, ///< Element has type "Metadata float".
    ENET_MetaInteger, ///< Element has type "Metadata integer".
    ENET_MetaSet, ///< Element has type "Metadata set".
    ENET_MetaString, ///< Element has type "Metadata string".
    ENET_Arc2D, ///< Element has type "Arc2D".
    ENET_ArcClose2D, ///< Element has type "ArcClose2D".
    ENET_Circle2D, ///< Element has type "Circle2D".
    ENET_Disk2D, ///< Element has type "Disk2D".
    ENET_Polyline2D, ///< Element has type "Polyline2D".
    ENET_Polypoint2D, ///< Element has type "Polypoint2D".
    ENET_Rectangle2D, ///< Element has type "Rectangle2D".
    ENET_TriangleSet2D, ///< Element has type "TriangleSet2D".
    ENET_Box, ///< Element has type "Box".
    ENET_Cone, ///< Element has type "Cone".
    ENET_Cylinder, ///< Element has type "Cylinder".
    ENET_Sphere, ///< Element has type "Sphere".
    ENET_ElevationGrid, ///< Element has type "ElevationGrid".
    ENET_Extrusion, ///< Element has type "Extrusion".
    ENET_Coordinate, ///< Element has type "Coordinate".
    ENET_Normal, ///< Element has type "Normal".
    ENET_TextureCoordinate, ///< Element has type "TextureCoordinate".
    ENET_IndexedFaceSet, ///< Element has type "IndexedFaceSet".
    ENET_IndexedLineSet, ///< Element has type "IndexedLineSet".
    ENET_IndexedTriangleSet, ///< Element has type "IndexedTriangleSet".
    ENET_IndexedTriangleFanSet, ///< Element has type "IndexedTriangleFanSet".
    ENET_IndexedTriangleStripSet, ///< Element has type "IndexedTriangleStripSet".
    ENET_LineSet, ///< Element has type "LineSet".
    ENET_PointSet, ///< Element has type "PointSet".
    ENET_TriangleSet, ///< Element has type "TriangleSet".
    ENET_TriangleFanSet, ///< Element has type "TriangleFanSet".
    ENET_TriangleStripSet, ///< Element has type "TriangleStripSet".
    ENET_Color, ///< Element has type "Color".
    ENET_ColorRGBA, ///< Element has type "ColorRGBA".
    ENET_Shape, ///< Element has type "Shape".
    ENET_Appearance, ///< Element has type "Appearance".
    ENET_Material, ///< Element has type "Material".
    ENET_ImageTexture, ///< Element has type "ImageTexture".
    ENET_TextureTransform, ///< Element has type "TextureTransform".
    ENET_DirectionalLight, ///< Element has type "DirectionalLight".
    ENET_PointLight, ///< Element has type "PointLight".
    ENET_SpotLight, ///< Element has type "SpotLight".

    ENET_Invalid ///< Element has invalid type and possible contain invalid data.
};

struct X3DNodeElementBase {
    X3DNodeElementBase *Parent;
    std::string ID;
    std::list<X3DNodeElementBase *> Child;
    X3DElemType Type;
};

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
    void ParseFile(const std::string &pFile, IOSystem *pIOHandler);
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool pCheckSig) const;
    void GetExtensionList(std::set<std::string> &pExtensionList);
    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler);
    const aiImporterDesc *GetInfo() const;
    void Clear();

private:
    static const aiImporterDesc Description;
    X3DNodeElementBase *mNodeElementCur; ///< Current element.
}; // class X3DImporter

} // namespace Assimp

#endif // INCLUDED_AI_X3D_IMPORTER_H
