/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team


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

#include "X3DImporter_Node.hpp"

// Header files, Assimp.
#include <assimp/DefaultLogger.hpp>
#include <assimp/importerdesc.h>
#include <assimp/ProgressHandler.hpp>
#include <assimp/types.h>
#include <assimp/BaseImporter.h>
#include "legacyXML/irrXMLWrapper.h"
#include "FIReader.hpp"
//#include <regex>

namespace Assimp {

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
class X3DImporter : public BaseImporter
{
public:
    std::list<X3DNodeElementBase*> NodeElement_List;///< All elements of scene graph.

public:
    /***********************************************/
    /****************** Functions ******************/
    /***********************************************/

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
    void ParseFile( const std::string& pFile, IOSystem* pIOHandler );

    /***********************************************/
    /********* Functions: BaseImporter set *********/
    /***********************************************/

    bool CanRead( const std::string& pFile, IOSystem* pIOHandler, bool pCheckSig ) const;
    void GetExtensionList( std::set<std::string>& pExtensionList );
    void InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler );
    const aiImporterDesc* GetInfo()const;


private:
	/// Disabled copy constructor.
	X3DImporter(const X3DImporter& pScene);

	/// Disabled assign operator.
	X3DImporter& operator=(const X3DImporter& pScene);

	/// Clear all temporary data.
	void Clear();

	/***********************************************/
	/************* Functions: find set *************/
	/***********************************************/

	/// Find requested node element. Search will be made in all existing nodes.
	/// \param [in] pID - ID of requested element.
	/// \param [in] pType - type of requested element.
	/// \param [out] pElement - pointer to pointer to item found.
	/// \return true - if the element is found, else - false.
	bool FindNodeElement_FromRoot(const std::string& pID, const X3DElemType pType, X3DNodeElementBase** pElement);

	/// Find requested node element. Search will be made from pointed node down to childs.
	/// \param [in] pStartNode - pointer to start node.
	/// \param [in] pID - ID of requested element.
	/// \param [in] pType - type of requested element.
	/// \param [out] pElement - pointer to pointer to item found.
	/// \return true - if the element is found, else - false.
	bool FindNodeElement_FromNode(X3DNodeElementBase* pStartNode, const std::string& pID, const X3DElemType pType,
									X3DNodeElementBase** pElement);

	/// Find requested node element. For "Node"'s accounting flag "Static".
	/// \param [in] pName - name of requested element.
	/// \param [in] pType - type of requested element.
	/// \param [out] pElement - pointer to pointer to item found.
	/// \return true - if the element is found, else - false.
	bool FindNodeElement(const std::string& pName, const X3DElemType pType, X3DNodeElementBase** pElement);

	/***********************************************/
	/********* Functions: postprocess set **********/
	/***********************************************/

	/// \return transformation matrix from global coordinate system to local.
	aiMatrix4x4 PostprocessHelper_Matrix_GlobalToCurrent() const;

	/// Check if child elements of node element is metadata and add it to temporary list.
	/// \param [in] pNodeElement - node element where metadata is searching.
	/// \param [out] pList - temporary list for collected metadata.
	void PostprocessHelper_CollectMetadata(const X3DNodeElementBase& pNodeElement, std::list<X3DNodeElementBase*>& pList) const;

	/// Check if type of node element is metadata. E.g. <MetadataSet>, <MetadataString>.
	/// \param [in] pType - checked type.
	/// \return true - if the type corresponds to the metadata.
	bool PostprocessHelper_ElementIsMetadata(const X3DElemType pType) const;

	/// Check if type of node element is geometry object and can be used to build mesh. E.g. <Box>, <Arc2D>.
	/// \param [in] pType - checked type.
	/// \return true - if the type corresponds to the mesh.
	bool PostprocessHelper_ElementIsMesh(const X3DElemType pType) const;

	/// Read X3DNodeElementLight, create aiLight and add it to list of the lights.
	/// \param [in] pNodeElement - reference to lisght element(<DirectionalLight>, <PointLight>, <SpotLight>).
	/// \param [out] pSceneLightList - reference to list of the lights.
	void Postprocess_BuildLight(const X3DNodeElementBase& pNodeElement, std::list<aiLight*>& pSceneLightList) const;

	/// Create filled structure with type \ref aiMaterial from \ref X3DNodeElementBase. This function itseld extract
	/// all needed data from scene graph.
	/// \param [in] pNodeElement - reference to material element(<Appearance>).
	/// \param [out] pMaterial - pointer to pointer to created material. *pMaterial must be nullptr.
	void Postprocess_BuildMaterial(const X3DNodeElementBase& pNodeElement, aiMaterial** pMaterial) const;

	/// Create filled structure with type \ref aiMaterial from \ref X3DNodeElementBase. This function itseld extract
	/// all needed data from scene graph.
	/// \param [in] pNodeElement - reference to geometry object.
	/// \param [out] pMesh - pointer to pointer to created mesh. *pMesh must be nullptr.
	void Postprocess_BuildMesh(const X3DNodeElementBase& pNodeElement, aiMesh** pMesh) const;

	/// Create aiNode from X3DNodeElementBase. Also function check children and make recursive call.
	/// \param [out] pNode - pointer to pointer to created node. *pNode must be nullptr.
	/// \param [in] pNodeElement - X3DNodeElementBase which read.
	/// \param [out] pSceneNode - aiNode for filling.
	/// \param [out] pSceneMeshList - list with aiMesh which belong to scene.
	/// \param [out] pSceneMaterialList - list with aiMaterial which belong to scene.
	/// \param [out] pSceneLightList - list with aiLight which belong to scene.
	void Postprocess_BuildNode(const X3DNodeElementBase& pNodeElement, aiNode& pSceneNode, std::list<aiMesh*>& pSceneMeshList,
								std::list<aiMaterial*>& pSceneMaterialList, std::list<aiLight*>& pSceneLightList) const;

	/// To create mesh and material kept in <Schape>.
	/// \param pShapeNodeElement - reference to node element which kept <Shape> data.
	/// \param pNodeMeshInd - reference to list with mesh indices. When pShapeNodeElement will read new mesh index will be added to this list.
	/// \param pSceneMeshList - reference to list with meshes. When pShapeNodeElement will read new mesh will be added to this list.
	/// \param pSceneMaterialList - reference to list with materials. When pShapeNodeElement will read new material will be added to this list.
	void Postprocess_BuildShape(const CX3DImporter_NodeElement_Shape& pShapeNodeElement, std::list<unsigned int>& pNodeMeshInd,
								std::list<aiMesh*>& pSceneMeshList, std::list<aiMaterial*>& pSceneMaterialList) const;

	/// Check if child elements of node element is metadata and add it to scene node.
	/// \param [in] pNodeElement - node element where metadata is searching.
	/// \param [out] pSceneNode - scene node in which metadata will be added.
	void Postprocess_CollectMetadata(const X3DNodeElementBase& pNodeElement, aiNode& pSceneNode) const;

	/***********************************************/
	/************* Functions: throw set ************/
	/***********************************************/

	/// Call that function when argument is out of range and exception must be raised.
	/// \throw DeadlyImportError.
	/// \param [in] pArgument - argument name.
	void Throw_ArgOutOfRange(const std::string& pArgument);

	/// Call that function when close tag of node not found and exception must be raised.
	/// E.g.:
	/// <Scene>
	///     <Shape>
	/// </Scene> <!--- shape not closed --->
	/// \throw DeadlyImportError.
	/// \param [in] pNode - node name in which exception happened.
	void Throw_CloseNotFound(const std::string& pNode);

	/// Call that function when string value can not be converted to floating point value and exception must be raised.
	/// \param [in] pAttrValue - attribute value.
	/// \throw DeadlyImportError.
	void Throw_ConvertFail_Str2ArrF(const std::string& pAttrValue);

	/// Call that function when in node defined attributes "DEF" and "USE" and exception must be raised.
	/// E.g.: <Box DEF="BigBox" USE="MegaBox">
	/// \throw DeadlyImportError.
	void Throw_DEF_And_USE();

	/// Call that function when attribute name is incorrect and exception must be raised.
	/// \param [in] pAttrName - attribute name.
	/// \throw DeadlyImportError.
	void Throw_IncorrectAttr(const std::string& pAttrName);

	/// Call that function when attribute value is incorrect and exception must be raised.
	/// \param [in] pAttrName - attribute name.
	/// \throw DeadlyImportError.
	void Throw_IncorrectAttrValue(const std::string& pAttrName);

	/// Call that function when some type of nodes are defined twice or more when must be used only once and exception must be raised.
	/// E.g.:
	/// <Shape>
	///     <Box/>    <!--- first geometry node --->
	///     <Sphere/> <!--- second geometry node. raise exception --->
	/// </Shape>
	/// \throw DeadlyImportError.
	/// \param [in] pNodeType - type of node which defined one more time.
	/// \param [in] pDescription - message about error. E.g. what the node defined while exception raised.
	void Throw_MoreThanOnceDefined(const std::string& pNodeType, const std::string& pDescription);

	/// Call that function when count of opening and closing tags which create group(e.g. <Group>) are not equal and exception must be raised.
	/// E.g.:
	/// <Scene>
	///     <Transform>  <!--- first grouping node begin --->
	///         <Group>  <!--- second grouping node begin --->
	///     </Transform> <!--- first grouping node end --->
	/// </Scene> <!--- one grouping node still not closed --->
	/// \throw DeadlyImportError.
	/// \param [in] pNode - node name in which exception happened.
	void Throw_TagCountIncorrect(const std::string& pNode);

	/// Call that function when defined in "USE" element are not found in graph and exception must be raised.
	/// \param [in] pAttrValue - "USE" attribute value.
	/// \throw DeadlyImportError.
	void Throw_USE_NotFound(const std::string& pAttrValue);

	/***********************************************/
	/************** Functions: LOG set *************/
	/***********************************************/

	/// Short variant for calling \ref DefaultLogger::get()->info()
	void LogInfo(const std::string& pMessage) { DefaultLogger::get()->info(pMessage); }

	/***********************************************/
	/************** Functions: XML set *************/
	/***********************************************/

	/// Check if current node is empty: <node />. If not then exception will throwed.
	void XML_CheckNode_MustBeEmpty();

	/// Check if current node name is equal to pNodeName.
	/// \param [in] pNodeName - name for checking.
	/// return true if current node name is equal to pNodeName, else - false.
	bool XML_CheckNode_NameEqual(const std::string& pNodeName) { return mReader->getNodeName() == pNodeName; }

	/// Skip unsupported node and report about that. Depend on node name can be skipped begin tag of node all whole node.
	/// \param [in] pParentNodeName - parent node name. Used for reporting.
	void XML_CheckNode_SkipUnsupported(const std::string& pParentNodeName);

	/// Search for specified node in file. XML file read pointer(mReader) will point to found node or file end after search is end.
	/// \param [in] pNodeName - requested node name.
	/// return true - if node is found, else - false.
	bool XML_SearchNode(const std::string& pNodeName);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \return read data.
	bool XML_ReadNode_GetAttrVal_AsBool(const int pAttrIdx);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \return read data.
	float XML_ReadNode_GetAttrVal_AsFloat(const int pAttrIdx);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \return read data.
	int32_t XML_ReadNode_GetAttrVal_AsI32(const int pAttrIdx);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsCol3f(const int pAttrIdx, aiColor3D& pValue);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsVec2f(const int pAttrIdx, aiVector2D& pValue);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsVec3f(const int pAttrIdx, aiVector3D& pValue);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsArrB(const int pAttrIdx, std::vector<bool>& pValue);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsArrI32(const int pAttrIdx, std::vector<int32_t>& pValue);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsArrF(const int pAttrIdx, std::vector<float>& pValue);

    /// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsArrD(const int pAttrIdx, std::vector<double>& pValue);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsListCol3f(const int pAttrIdx, std::list<aiColor3D>& pValue);

	/// \overload void XML_ReadNode_GetAttrVal_AsListCol3f(const int pAttrIdx, std::vector<aiColor3D>& pValue)
	void XML_ReadNode_GetAttrVal_AsArrCol3f(const int pAttrIdx, std::vector<aiColor3D>& pValue);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsListCol4f(const int pAttrIdx, std::list<aiColor4D>& pValue);

	/// \overload void XML_ReadNode_GetAttrVal_AsListCol4f(const int pAttrIdx, std::list<aiColor4D>& pValue)
	void XML_ReadNode_GetAttrVal_AsArrCol4f(const int pAttrIdx, std::vector<aiColor4D>& pValue);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsListVec2f(const int pAttrIdx, std::list<aiVector2D>& pValue);

	/// \overload void XML_ReadNode_GetAttrVal_AsListVec2f(const int pAttrIdx, std::list<aiVector2D>& pValue)
	void XML_ReadNode_GetAttrVal_AsArrVec2f(const int pAttrIdx, std::vector<aiVector2D>& pValue);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsListVec3f(const int pAttrIdx, std::list<aiVector3D>& pValue);

	/// \overload void XML_ReadNode_GetAttrVal_AsListVec3f(const int pAttrIdx, std::list<aiVector3D>& pValue)
	void XML_ReadNode_GetAttrVal_AsArrVec3f(const int pAttrIdx, std::vector<aiVector3D>& pValue);

	/// Read attribute value.
	/// \param [in] pAttrIdx - attribute index (\ref mReader->getAttribute* set).
	/// \param [out] pValue - read data.
	void XML_ReadNode_GetAttrVal_AsListS(const int pAttrIdx, std::list<std::string>& pValue);

	/***********************************************/
	/******* Functions: geometry helper set  *******/
	/***********************************************/

	/// Create CoordIdx of line set from CoordIdx of polyline set.
	/// \param [in] pPolylineCoordIdx - vertices indices divided by delimiter "-1". Must contain faces with two or more indices.
	/// \param [out] pLineCoordIdx - made CoordIdx of line set.
	void GeometryHelper_Extend_PolylineIdxToLineIdx(const std::list<int32_t>& pPolylineCoordIdx, std::list<int32_t>& pLineCoordIdx);

    /// Add texture coordinates to mesh. Function work similar to \ref add_color;
	void MeshGeometry_AddTexCoord(aiMesh& pMesh, const std::vector<int32_t>& pCoordIdx, const std::vector<int32_t>& pTexCoordIdx,
								const std::list<aiVector2D>& pTexCoords) const;

    /// Add texture coordinates to mesh. Function work similar to \ref add_color;
	void MeshGeometry_AddTexCoord(aiMesh& pMesh, const std::list<aiVector2D>& pTexCoords) const;

	/***********************************************/
	/******** Functions: parse set private *********/
	/***********************************************/

	/// Create node element with type "Node" in scene graph. That operation is needed when you enter to X3D group node
	/// like <Group>, <Transform> etc. When exiting from X3D group node(e.g. </Group>) \ref ParseHelper_Node_Exit must
	/// be called.
	/// \param [in] pStatic - flag: if true then static node is created(e.g. <StaticGroup>).
	void ParseHelper_Group_Begin(const bool pStatic = false);

	/// Make pNode as current and enter deeper for parsing child nodes. At end \ref ParseHelper_Node_Exit must be called.
	/// \param [in] pNode - new current node.
	void ParseHelper_Node_Enter(X3DNodeElementBase* pNode);

	/// This function must be called when exiting from X3D group node(e.g. </Group>). \ref ParseHelper_Group_Begin.
	void ParseHelper_Node_Exit();

	/// Attribute values of floating point types can take form ".x"(without leading zero). irrXMLReader can not read this form of values and it
	/// must be converted to right form - "0.xxx".
	/// \param [in] pInStr - pointer to input string which can contain incorrect form of values.
	/// \param [out[ pOutString - output string with right form of values.
	void ParseHelper_FixTruncatedFloatString(const char* pInStr, std::string& pOutString);

	/// Check if current node has nodes of type X3DMetadataObject. Why we must do it? Because X3DMetadataObject can be in any non-empty X3DNode.
	/// Meaning that X3DMetadataObject can be in any non-empty node in <Scene>.
	/// \return true - if metadata node are found and parsed, false - metadata not found.
	bool ParseHelper_CheckRead_X3DMetadataObject();

	/// Check if current node has nodes of type X3DGeometricPropertyNode. X3DGeometricPropertyNode
	/// X3DGeometricPropertyNode inheritors:
	/// <FogCoordinate>, <HAnimDisplacer>, <Color>, <ColorRGBA>, <Coordinate>, <CoordinateDouble>, <GeoCoordinate>, <Normal>,
	/// <MultiTextureCoordinate>, <TextureCoordinate>, <TextureCoordinate3D>, <TextureCoordinate4D>, <TextureCoordinateGenerator>,
	/// <FloatVertexAttribute>, <Matrix3VertexAttribute>, <Matrix4VertexAttribute>.
	/// \return true - if nodes are found and parsed, false - nodes not found.
	bool ParseHelper_CheckRead_X3DGeometricPropertyNode();

	/// Parse <X3D> node of the file.
	void ParseNode_Root();

	/// Parse <head> node of the file.
	void ParseNode_Head();

	/// Parse <Scene> node of the file.
	void ParseNode_Scene();

	/// Parse child nodes of <Metadata*> node.
	/// \param [in] pNodeName - parsed node name. Must be set because that function is general and name needed for checking the end
	/// and error reporing.
	/// \param [in] pParentElement - parent metadata element.
	void ParseNode_Metadata(X3DNodeElementBase* pParentElement, const std::string& pNodeName);

	/// Parse <MetadataBoolean> node of the file.
	void ParseNode_MetadataBoolean();

	/// Parse <MetadataDouble> node of the file.
	void ParseNode_MetadataDouble();

	/// Parse <MetadataFloat> node of the file.
	void ParseNode_MetadataFloat();

	/// Parse <MetadataInteger> node of the file.
	void ParseNode_MetadataInteger();

	/// Parse <MetadataSet> node of the file.
	void ParseNode_MetadataSet();

	/// \fn void ParseNode_MetadataString()
	/// Parse <MetadataString> node of the file.
	void ParseNode_MetadataString();

	/// Parse <Arc2D> node of the file.
	void readArc2D();

	/// Parse <ArcClose2D> node of the file.
	void ParseNode_Geometry2D_ArcClose2D();

	/// Parse <Circle2D> node of the file.
	void ParseNode_Geometry2D_Circle2D();

	/// Parse <Disk2D> node of the file.
	void ParseNode_Geometry2D_Disk2D();

	/// Parse <Polyline2D> node of the file.
	void ParseNode_Geometry2D_Polyline2D();

	/// Parse <Polypoint2D> node of the file.
	void ParseNode_Geometry2D_Polypoint2D();

	/// Parse <Rectangle2D> node of the file.
	void ParseNode_Geometry2D_Rectangle2D();

	/// Parse <TriangleSet2D> node of the file.
	void ParseNode_Geometry2D_TriangleSet2D();

	/// Parse <Box> node of the file.
	void ParseNode_Geometry3D_Box();

	/// Parse <Cone> node of the file.
	void ParseNode_Geometry3D_Cone();

	/// Parse <Cylinder> node of the file.
	void ParseNode_Geometry3D_Cylinder();

	/// Parse <ElevationGrid> node of the file.
	void ParseNode_Geometry3D_ElevationGrid();

	/// Parse <Extrusion> node of the file.
	void ParseNode_Geometry3D_Extrusion();

	/// Parse <IndexedFaceSet> node of the file.
	void ParseNode_Geometry3D_IndexedFaceSet();

	/// Parse <Sphere> node of the file.
	void ParseNode_Geometry3D_Sphere();

	/// Parse <Group> node of the file. And create new node in scene graph.
	void startReadGroup();

	/// Doing actions at an exit from <Group>. Walk up in scene graph.
	void endReadGroup();

	/// Parse <StaticGroup> node of the file. And create new node in scene graph.
	void startReadStaticGroup();

	/// Doing actions at an exit from <StaticGroup>. Walk up in scene graph.
	void endReadStaticGroup();

	/// Parse <Switch> node of the file. And create new node in scene graph.
	void startReadSwitch();

	/// Doing actions at an exit from <Switch>. Walk up in scene graph.
	void endReadSwitch();

	/// Parse <Transform> node of the file. And create new node in scene graph.
	void startReadTransform();

	/// Doing actions at an exit from <Transform>. Walk up in scene graph.
	void endReadTransform();

	/// Parse <Color> node of the file.
	void readColor();

	/// Parse <ColorRGBA> node of the file.
	void readColorRGBA();

	/// Parse <Coordinate> node of the file.
	void ParseNode_Rendering_Coordinate();

	/// Parse <Normal> node of the file.
	void ParseNode_Rendering_Normal();

	/// Parse <IndexedLineSet> node of the file.
	void readIndexedLineSet();

	/// Parse <IndexedTriangleFanSet> node of the file.
	void readIndexedTriangleFanSet();

	/// Parse <IndexedTriangleSet> node of the file.
	void ParseNode_Rendering_IndexedTriangleSet();

	/// Parse <IndexedTriangleStripSet> node of the file.
	void ParseNode_Rendering_IndexedTriangleStripSet();

	/// Parse <LineSet> node of the file.
	void ParseNode_Rendering_LineSet();

	/// Parse <PointSet> node of the file.
	void ParseNode_Rendering_PointSet();

	/// Parse <TriangleFanSet> node of the file.
	void ParseNode_Rendering_TriangleFanSet();

	/// Parse <TriangleSet> node of the file.
	void ParseNode_Rendering_TriangleSet();

	/// Parse <TriangleStripSet> node of the file.
	void ParseNode_Rendering_TriangleStripSet();

	/// Parse <ImageTexture> node of the file.
	void ParseNode_Texturing_ImageTexture();

	/// Parse <TextureCoordinate> node of the file.
	void ParseNode_Texturing_TextureCoordinate();

	/// Parse <TextureTransform> node of the file.
	void ParseNode_Texturing_TextureTransform();

	/// Parse <Shape> node of the file.
	void ParseNode_Shape_Shape();

	/// Parse <Appearance> node of the file.
	void ParseNode_Shape_Appearance();

	/// Parse <Material> node of the file.
	void ParseNode_Shape_Material();

	/// Parse <Inline> node of the file.
	void ParseNode_Networking_Inline();

	/// Parse <DirectionalLight> node of the file.
	void ParseNode_Lighting_DirectionalLight();

	/// Parse <PointLight> node of the file.
	void ParseNode_Lighting_PointLight();

	/// Parse <SpotLight> node of the file.
	void ParseNode_Lighting_SpotLight();

private:
    /***********************************************/
    /******************** Types ********************/
    /***********************************************/

    /***********************************************/
    /****************** Constants ******************/
    /***********************************************/
    static const aiImporterDesc Description;
    //static const std::regex pattern_nws;
    //static const std::regex pattern_true;


    /***********************************************/
    /****************** Variables ******************/
    /***********************************************/
    X3DNodeElementBase* mNodeElementCur;///< Current element.
    std::unique_ptr<FIReader> mReader;///< Pointer to XML-reader object
    IOSystem *mpIOHandler;
};// class X3DImporter

}// namespace Assimp

#endif // INCLUDED_AI_X3D_IMPORTER_H
