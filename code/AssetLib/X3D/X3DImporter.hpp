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
#ifndef INCLUDED_AI_X3D_IMPORTER_H
#define INCLUDED_AI_X3D_IMPORTER_H

#include <assimp/BaseImporter.h>
#include <assimp/XmlParser.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/ProgressHandler.hpp>

#include <list>
#include <string>
#include <vector>

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
    std::list<X3DNodeElementBase *> Children;
    X3DElemType Type;

protected:
    X3DNodeElementBase(X3DElemType type, X3DNodeElementBase *pParent) :
            Type(type), Parent(pParent) {
        // empty
    }
};

/// This struct hold <Color> value.
struct CX3DImporter_NodeElement_Color : X3DNodeElementBase {
    std::list<aiColor3D> Value; ///< Stored value.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    CX3DImporter_NodeElement_Color(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Color, pParent) {}

}; // struct CX3DImporter_NodeElement_Color

/// This struct hold <ColorRGBA> value.
struct CX3DImporter_NodeElement_ColorRGBA : X3DNodeElementBase {
    std::list<aiColor4D> Value; ///< Stored value.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    CX3DImporter_NodeElement_ColorRGBA(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_ColorRGBA, pParent) {}

}; // struct CX3DImporter_NodeElement_ColorRGBA

/// This struct hold <Coordinate> value.
struct CX3DImporter_NodeElement_Coordinate : public X3DNodeElementBase {
    std::list<aiVector3D> Value; ///< Stored value.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    CX3DImporter_NodeElement_Coordinate(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Coordinate, pParent) {}

}; // struct CX3DImporter_NodeElement_Coordinate

/// This struct hold <Normal> value.
struct CX3DImporter_NodeElement_Normal : X3DNodeElementBase {
    std::list<aiVector3D> Value; ///< Stored value.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    CX3DImporter_NodeElement_Normal(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Normal, pParent) {}

}; // struct CX3DImporter_NodeElement_Normal

/// This struct hold <TextureCoordinate> value.
struct CX3DImporter_NodeElement_TextureCoordinate : X3DNodeElementBase {
    std::list<aiVector2D> Value; ///< Stored value.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    CX3DImporter_NodeElement_TextureCoordinate(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_TextureCoordinate, pParent) {}

}; // struct CX3DImporter_NodeElement_TextureCoordinate

/// Two-dimensional figure.
struct CX3DImporter_NodeElement_Geometry2D : X3DNodeElementBase {
    std::list<aiVector3D> Vertices; ///< Vertices list.
    size_t NumIndices; ///< Number of indices in one face.
    bool Solid; ///< Flag: if true then render must use back-face culling, else render must draw both sides of object.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pType - type of geometry object.
    CX3DImporter_NodeElement_Geometry2D(X3DElemType pType, X3DNodeElementBase *pParent) :
            X3DNodeElementBase(pType, pParent), Solid(true) {}

}; // class CX3DImporter_NodeElement_Geometry2D

/// Three-dimensional body.
struct CX3DImporter_NodeElement_Geometry3D : X3DNodeElementBase {
    std::list<aiVector3D> Vertices; ///< Vertices list.
    size_t NumIndices; ///< Number of indices in one face.
    bool Solid; ///< Flag: if true then render must use back-face culling, else render must draw both sides of object.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pType - type of geometry object.
    CX3DImporter_NodeElement_Geometry3D(X3DElemType pType, X3DNodeElementBase *pParent) :
            X3DNodeElementBase(pType, pParent), Vertices(), NumIndices(0), Solid(true) {
        // empty
    }
}; // class CX3DImporter_NodeElement_Geometry3D

/// Uniform rectangular grid of varying height.
struct CX3DImporter_NodeElement_ElevationGrid : CX3DImporter_NodeElement_Geometry3D {
    bool NormalPerVertex; ///< If true then normals are defined for every vertex, else for every face(line).
    bool ColorPerVertex; ///< If true then colors are defined for every vertex, else for every face(line).
    /// If the angle between the geometric normals of two adjacent faces is less than the crease angle, normals shall be calculated so that the faces are
    /// shaded smoothly across the edge; otherwise, normals shall be calculated so that a lighting discontinuity across the edge is produced.
    float CreaseAngle;
    std::vector<int32_t> CoordIdx; ///< Coordinates list by faces. In X3D format: "-1" - delimiter for faces.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pType - type of geometry object.
    CX3DImporter_NodeElement_ElevationGrid(X3DElemType pType, X3DNodeElementBase *pParent) :
            CX3DImporter_NodeElement_Geometry3D(pType, pParent) {}
}; // class CX3DImporter_NodeElement_IndexedSet

/// Shape with indexed vertices.
struct CX3DImporter_NodeElement_IndexedSet : public CX3DImporter_NodeElement_Geometry3D {
    /// The ccw field defines the ordering of the vertex coordinates of the geometry with respect to user-given or automatically generated normal vectors
    /// used in the lighting model equations. If ccw is TRUE, the normals shall follow the right hand rule; the orientation of each normal with respect to
    /// the vertices (taken in order) shall be such that the vertices appear to be oriented in a counterclockwise order when the vertices are viewed (in the
    /// local coordinate system of the Shape) from the opposite direction as the normal. If ccw is FALSE, the normals shall be oriented in the opposite
    /// direction. If normals are not generated but are supplied using a Normal node, and the orientation of the normals does not match the setting of the
    /// ccw field, results are undefined.
    bool CCW;
    std::vector<int32_t> ColorIndex; ///< Field to specify the polygonal faces by indexing into the <Color> or <ColorRGBA>.
    bool ColorPerVertex; ///< If true then colors are defined for every vertex, else for every face(line).
    /// The convex field indicates whether all polygons in the shape are convex (TRUE). A polygon is convex if it is planar, does not intersect itself,
    /// and all of the interior angles at its vertices are less than 180 degrees. Non planar and self intersecting polygons may produce undefined results
    /// even if the convex field is FALSE.
    bool Convex;
    std::vector<int32_t> CoordIndex; ///< Field to specify the polygonal faces by indexing into the <Coordinate>.
    /// If the angle between the geometric normals of two adjacent faces is less than the crease angle, normals shall be calculated so that the faces are
    /// shaded smoothly across the edge; otherwise, normals shall be calculated so that a lighting discontinuity across the edge is produced.
    float CreaseAngle;
    std::vector<int32_t> NormalIndex; ///< Field to specify the polygonal faces by indexing into the <Normal>.
    bool NormalPerVertex; ///< If true then normals are defined for every vertex, else for every face(line).
    std::vector<int32_t> TexCoordIndex; ///< Field to specify the polygonal faces by indexing into the <TextureCoordinate>.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pType - type of geometry object.
    CX3DImporter_NodeElement_IndexedSet(X3DElemType pType, X3DNodeElementBase *pParent) :
            CX3DImporter_NodeElement_Geometry3D(pType, pParent) {}
}; // class CX3DImporter_NodeElement_IndexedSet

/// Shape with set of vertices.
struct CX3DImporter_NodeElement_Set : CX3DImporter_NodeElement_Geometry3D {
    /// The ccw field defines the ordering of the vertex coordinates of the geometry with respect to user-given or automatically generated normal vectors
    /// used in the lighting model equations. If ccw is TRUE, the normals shall follow the right hand rule; the orientation of each normal with respect to
    /// the vertices (taken in order) shall be such that the vertices appear to be oriented in a counterclockwise order when the vertices are viewed (in the
    /// local coordinate system of the Shape) from the opposite direction as the normal. If ccw is FALSE, the normals shall be oriented in the opposite
    /// direction. If normals are not generated but are supplied using a Normal node, and the orientation of the normals does not match the setting of the
    /// ccw field, results are undefined.
    bool CCW;
    bool ColorPerVertex; ///< If true then colors are defined for every vertex, else for every face(line).
    bool NormalPerVertex; ///< If true then normals are defined for every vertex, else for every face(line).
    std::vector<int32_t> CoordIndex; ///< Field to specify the polygonal faces by indexing into the <Coordinate>.
    std::vector<int32_t> NormalIndex; ///< Field to specify the polygonal faces by indexing into the <Normal>.
    std::vector<int32_t> TexCoordIndex; ///< Field to specify the polygonal faces by indexing into the <TextureCoordinate>.
    std::vector<int32_t> VertexCount; ///< Field describes how many vertices are to be used in each polyline(polygon) from the <Coordinate> field.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pType - type of geometry object.
    CX3DImporter_NodeElement_Set(X3DElemType type, X3DNodeElementBase *pParent) :
            CX3DImporter_NodeElement_Geometry3D(type, pParent) {}

}; // class CX3DImporter_NodeElement_Set

/// This struct hold <Shape> value.
struct CX3DImporter_NodeElement_Shape : X3DNodeElementBase {

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    CX3DImporter_NodeElement_Shape(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Shape, pParent) {}
}; // struct CX3DImporter_NodeElement_Shape

/// This struct hold <Appearance> value.
struct CX3DImporter_NodeElement_Appearance : public X3DNodeElementBase {

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    CX3DImporter_NodeElement_Appearance(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Appearance, pParent) {}

}; // struct CX3DImporter_NodeElement_Appearance

struct CX3DImporter_NodeElement_Material : public X3DNodeElementBase {
    float AmbientIntensity; ///< Specifies how much ambient light from light sources this surface shall reflect.
    aiColor3D DiffuseColor; ///< Reflects all X3D light sources depending on the angle of the surface with respect to the light source.
    aiColor3D EmissiveColor; ///< Models "glowing" objects. This can be useful for displaying pre-lit models.
    float Shininess; ///< Lower shininess values produce soft glows, while higher values result in sharper, smaller highlights.
    aiColor3D SpecularColor; ///< The specularColor and shininess fields determine the specular highlights.
    float Transparency; ///< Specifies how "clear" an object is, with 1.0 being completely transparent, and 0.0 completely opaque.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pType - type of geometry object.
    CX3DImporter_NodeElement_Material(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Material, pParent),
            AmbientIntensity(0.0f),
            DiffuseColor(),
            EmissiveColor(),
            Shininess(0.0f),
            SpecularColor(),
            Transparency(1.0f) {
        // empty
    }
}; // class CX3DImporter_NodeElement_Material

/// This struct hold <ImageTexture> value.
struct CX3DImporter_NodeElement_ImageTexture : X3DNodeElementBase {
    /// RepeatS and RepeatT, that specify how the texture wraps in the S and T directions. If repeatS is TRUE (the default), the texture map is repeated
    /// outside the [0.0, 1.0] texture coordinate range in the S direction so that it fills the shape. If repeatS is FALSE, the texture coordinates are
    /// clamped in the S direction to lie within the [0.0, 1.0] range. The repeatT field is analogous to the repeatS field.
    bool RepeatS;
    bool RepeatT; ///< See \ref RepeatS.
    std::string URL; ///< URL of the texture.

                     /// Constructor
    /// \param [in] pParent - pointer to parent node.
    CX3DImporter_NodeElement_ImageTexture(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_ImageTexture, pParent) {}

}; // struct CX3DImporter_NodeElement_ImageTexture

/// This struct hold <TextureTransform> value.
struct CX3DImporter_NodeElement_TextureTransform : X3DNodeElementBase {
    aiVector2D Center; ///< Specifies a translation offset in texture coordinate space about which the rotation and scale fields are applied.
    float Rotation; ///< Specifies a rotation in angle base units of the texture coordinates about the center point after the scale has been applied.
    aiVector2D Scale; ///< Specifies a scaling factor in S and T of the texture coordinates about the center point.
    aiVector2D Translation; ///<  Specifies a translation of the texture coordinates.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    CX3DImporter_NodeElement_TextureTransform(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_TextureTransform, pParent) {}

}; // struct CX3DImporter_NodeElement_TextureTransform

struct CX3DNodeElementGroup : X3DNodeElementBase {
    aiMatrix4x4 Transformation; ///< Transformation matrix.

    /// As you know node elements can use already defined node elements when attribute "USE" is defined.
    /// Standard search when looking for an element in the whole scene graph, existing at this moment.
    /// If a node is marked as static, the children(or lower) can not search for elements in the nodes upper then static.
    bool Static;

    bool UseChoice; ///< Flag: if true then use number from \ref Choice to choose what the child will be kept.
    int32_t Choice; ///< Number of the child which will be kept.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pStatic - static node flag.
    CX3DNodeElementGroup(X3DNodeElementBase *pParent, const bool pStatic = false) :
            X3DNodeElementBase(X3DElemType::ENET_Group, pParent), Static(pStatic), UseChoice(false) {}
};

struct X3DNodeElementMeta : X3DNodeElementBase {
    std::string Name; ///< Name of metadata object.
    std::string Reference;

    virtual ~X3DNodeElementMeta() {
        // empty
    }

protected:
    X3DNodeElementMeta(X3DElemType type, X3DNodeElementBase *parent) :
            X3DNodeElementBase(type, parent) {
        // empty
    }
};

struct X3DNodeElementMetaBoolean : X3DNodeElementMeta {
    std::vector<bool> Value; ///< Stored value.

    explicit X3DNodeElementMetaBoolean(X3DNodeElementBase *pParent) :
            X3DNodeElementMeta(X3DElemType::ENET_MetaBoolean, pParent) {
        // empty
    }
};

struct X3DNodeElementMetaDouble : X3DNodeElementMeta {
    std::vector<double> Value; ///< Stored value.

    explicit X3DNodeElementMetaDouble(X3DNodeElementBase *pParent) :
            X3DNodeElementMeta(X3DElemType::ENET_MetaDouble, pParent) {
        // empty
    }
};

struct X3DNodeElementMetaFloat : public X3DNodeElementMeta {
    std::vector<float> Value; ///< Stored value.

    explicit X3DNodeElementMetaFloat(X3DNodeElementBase *pParent) :
            X3DNodeElementMeta(X3DElemType::ENET_MetaFloat, pParent) {
        // empty
    }
};

struct X3DNodeElementMetaInt : public X3DNodeElementMeta {
    std::vector<int32_t> Value; ///< Stored value.

    explicit X3DNodeElementMetaInt(X3DNodeElementBase *pParent) :
            X3DNodeElementMeta(X3DElemType::ENET_MetaInteger, pParent) {
        // empty
    }
};

struct X3DNodeElementMetaSet : public X3DNodeElementMeta {
    std::list<X3DNodeElementMeta> Value; ///< Stored value.

    explicit X3DNodeElementMetaSet(X3DNodeElementBase *pParent) :
            X3DNodeElementMeta(X3DElemType::ENET_MetaSet, pParent) {
        // empty
    }
};

struct X3DNodeElementMetaString : X3DNodeElementMeta {
    std::list<std::string> Value; ///< Stored value.

    explicit X3DNodeElementMetaString(X3DNodeElementBase *pParent) :
            X3DNodeElementMeta(X3DElemType::ENET_MetaString, pParent) {
        // empty
    }
};

/// \struct CX3DImporter_NodeElement_Light
/// This struct hold <TextureTransform> value.
struct X3DNodeNodeElementLight : X3DNodeElementBase {
    float AmbientIntensity; ///< Specifies the intensity of the ambient emission from the light.
    aiColor3D Color; ///< specifies the spectral colour properties of both the direct and ambient light emission as an RGB value.
    aiVector3D Direction; ///< Specifies the direction vector of the illumination emanating from the light source in the local coordinate system.
    /// \var Global
    /// Field that determines whether the light is global or scoped. Global lights illuminate all objects that fall within their volume of lighting influence.
    /// Scoped lights only illuminate objects that are in the same transformation hierarchy as the light.
    bool Global;
    float Intensity; ///< Specifies the brightness of the direct emission from the light.
    /// \var Attenuation
    /// PointLight node's illumination falls off with distance as specified by three attenuation coefficients. The attenuation factor
    /// is: "1 / max(attenuation[0] + attenuation[1] * r + attenuation[2] * r2, 1)", where r is the distance from the light to the surface being illuminated.
    aiVector3D Attenuation;
    aiVector3D Location; ///< Specifies a translation offset of the centre point of the light source from the light's local coordinate system origin.
    float Radius; ///< Specifies the radial extent of the solid angle and the maximum distance from location that may be illuminated by the light source.
    float BeamWidth; ///< Specifies an inner solid angle in which the light source emits light at uniform full intensity.
    float CutOffAngle; ///< The light source's emission intensity drops off from the inner solid angle (beamWidth) to the outer solid angle (cutOffAngle).

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pLightType - type of the light source.
    X3DNodeNodeElementLight(X3DElemType pLightType, X3DNodeElementBase *pParent) :
            X3DNodeElementBase(pLightType, pParent) {}

}; // struct CX3DImporter_NodeElement_Light

using X3DElementList = std::list<X3DNodeElementBase*>;

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
    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler);
    const aiImporterDesc *GetInfo() const;
    void Clear();
    void readMetadata(XmlNode &node);
    void readScene(XmlNode &node);
    void readViewpoint(XmlNode &node);
    void readMetadataObject(XmlNode &node);
    void ParseDirectionalLight(XmlNode &node);
    void ParseNode_Lighting_PointLight(XmlNode &node);
    void ParseNode_Lighting_SpotLight(XmlNode &node);
    void Postprocess_BuildNode(const X3DNodeElementBase &pNodeElement, aiNode &pSceneNode, std::list<aiMesh *> &pSceneMeshList,
            std::list<aiMaterial *> &pSceneMaterialList, std::list<aiLight *> &pSceneLightList) const;

private:
    static const aiImporterDesc Description;
    X3DNodeElementBase *mNodeElementCur;
    aiScene *mScene;
}; // class X3DImporter

} // namespace Assimp

#endif // INCLUDED_AI_X3D_IMPORTER_H
