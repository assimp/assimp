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
/// \file   X3DImporter_Node.hpp
/// \brief  Elements of scene graph.
/// \date   2015-2016
/// \author smal.root@gmail.com

#ifndef INCLUDED_AI_X3D_IMPORTER_NODE_H
#define INCLUDED_AI_X3D_IMPORTER_NODE_H

// Header files, Assimp.
#include <assimp/scene.h>
#include <assimp/types.h>

// Header files, stdlib.
#include <list>
#include <vector>
#include <string>

enum X3DElemType {
    ENET_Group,                  ///< Element has type "Group".
    ENET_MetaBoolean,            ///< Element has type "Metadata boolean".
    ENET_MetaDouble,             ///< Element has type "Metadata double".
    ENET_MetaFloat,              ///< Element has type "Metadata float".
    ENET_MetaInteger,            ///< Element has type "Metadata integer".
    ENET_MetaSet,                ///< Element has type "Metadata set".
    ENET_MetaString,             ///< Element has type "Metadata string".
    ENET_Arc2D,                  ///< Element has type "Arc2D".
    ENET_ArcClose2D,             ///< Element has type "ArcClose2D".
    ENET_Circle2D,               ///< Element has type "Circle2D".
    ENET_Disk2D,                 ///< Element has type "Disk2D".
    ENET_Polyline2D,             ///< Element has type "Polyline2D".
    ENET_Polypoint2D,            ///< Element has type "Polypoint2D".
    ENET_Rectangle2D,            ///< Element has type "Rectangle2D".
    ENET_TriangleSet2D,          ///< Element has type "TriangleSet2D".
    ENET_Box,                    ///< Element has type "Box".
    ENET_Cone,                   ///< Element has type "Cone".
    ENET_Cylinder,               ///< Element has type "Cylinder".
    ENET_Sphere,                 ///< Element has type "Sphere".
    ENET_ElevationGrid,          ///< Element has type "ElevationGrid".
    ENET_Extrusion,              ///< Element has type "Extrusion".
    ENET_Coordinate,             ///< Element has type "Coordinate".
    ENET_Normal,                 ///< Element has type "Normal".
    ENET_TextureCoordinate,      ///< Element has type "TextureCoordinate".
    ENET_IndexedFaceSet,         ///< Element has type "IndexedFaceSet".
    ENET_IndexedLineSet,         ///< Element has type "IndexedLineSet".
    ENET_IndexedTriangleSet,     ///< Element has type "IndexedTriangleSet".
    ENET_IndexedTriangleFanSet,  ///< Element has type "IndexedTriangleFanSet".
    ENET_IndexedTriangleStripSet,///< Element has type "IndexedTriangleStripSet".
    ENET_LineSet,                ///< Element has type "LineSet".
    ENET_PointSet,               ///< Element has type "PointSet".
    ENET_TriangleSet,            ///< Element has type "TriangleSet".
    ENET_TriangleFanSet,         ///< Element has type "TriangleFanSet".
    ENET_TriangleStripSet,       ///< Element has type "TriangleStripSet".
    ENET_Color,                  ///< Element has type "Color".
    ENET_ColorRGBA,              ///< Element has type "ColorRGBA".
    ENET_Shape,                  ///< Element has type "Shape".
    ENET_Appearance,             ///< Element has type "Appearance".
    ENET_Material,               ///< Element has type "Material".
    ENET_ImageTexture,           ///< Element has type "ImageTexture".
    ENET_TextureTransform,       ///< Element has type "TextureTransform".
    ENET_DirectionalLight,       ///< Element has type "DirectionalLight".
    ENET_PointLight,             ///< Element has type "PointLight".
    ENET_SpotLight,              ///< Element has type "SpotLight".

    ENET_Invalid                 ///< Element has invalid type and possible contain invalid data.
};

/// \class X3DNodeElementBase
/// Base class for elements of nodes.
class X3DNodeElementBase
{
	/***********************************************/
	/******************** Types ********************/
	/***********************************************/

public:

	/***********************************************/
	/****************** Constants ******************/
	/***********************************************/

public:

	const X3DElemType Type;

	/***********************************************/
	/****************** Variables ******************/
	/***********************************************/

public:

	std::string ID;///< ID of the element. Can be empty. In X3D synonym for "ID" attribute.
	X3DNodeElementBase* Parent;///< Parent element. If nullptr then this node is root.
	std::list<X3DNodeElementBase*> Children;///< Child elements.

	/***********************************************/
	/****************** Functions ******************/
	/***********************************************/

    /// @brief  The destructor, virtual.
    virtual ~X3DNodeElementBase() {
        // empty
    }

private:
	/// Disabled copy constructor.
	X3DNodeElementBase(const X3DNodeElementBase& pNodeElement);

	/// Disabled assign operator.
	X3DNodeElementBase& operator=(const X3DNodeElementBase& pNodeElement);

	/// Disabled default constructor.
	X3DNodeElementBase();

protected:
	/// In constructor inheritor must set element type.
	/// \param [in] pType - element type.
	/// \param [in] pParent - parent element.
	X3DNodeElementBase(const X3DElemType pType, X3DNodeElementBase* pParent)
		: Type(pType), Parent(pParent)
	{}
};// class IX3DImporter_NodeElement

/// \class X3DNodeElementGroup
/// Class that define grouping node. Define transformation matrix for children.
/// Also can select which child will be kept and others are removed.
class X3DNodeElementGroup : public X3DNodeElementBase
{
	/***********************************************/
	/****************** Variables ******************/
	/***********************************************/

public:

	aiMatrix4x4 Transformation;///< Transformation matrix.

	/// \var bool Static
	/// As you know node elements can use already defined node elements when attribute "USE" is defined.
	/// Standard search when looking for an element in the whole scene graph, existing at this moment.
	/// If a node is marked as static, the children(or lower) can not search for elements in the nodes upper then static.
	bool Static;

	bool UseChoice;///< Flag: if true then use number from \ref Choice to choose what the child will be kept.
	int32_t Choice;///< Number of the child which will be kept.

	/***********************************************/
	/****************** Functions ******************/
	/***********************************************/

private:

	/// \fn X3DNodeElementGroup(const X3DNodeElementGroup& pNode)
	/// Disabled copy constructor.
	X3DNodeElementGroup(const X3DNodeElementGroup& pNode);

	/// \fn X3DNodeElementGroup& operator=(const X3DNodeElementGroup& pNode)
	/// Disabled assign operator.
	X3DNodeElementGroup& operator=(const X3DNodeElementGroup& pNode);

	/// \fn X3DNodeElementGroup()
	/// Disabled default constructor.
	X3DNodeElementGroup();

public:

	/// \fn X3DNodeElementGroup(X3DNodeElementGroup* pParent, const bool pStatic = false)
	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	/// \param [in] pStatic - static node flag.
	X3DNodeElementGroup(X3DNodeElementBase* pParent, const bool pStatic = false)
		: X3DNodeElementBase(ENET_Group, pParent), Static(pStatic), UseChoice(false)
	{}

};// class X3DNodeElementGroup

/// \class X3DNodeElementMeta
/// This struct describe metavalue.
class X3DNodeElementMeta : public X3DNodeElementBase
{
	/***********************************************/
	/****************** Variables ******************/
	/***********************************************/

public:

	std::string Name;///< Name of metadata object.
	/// \var std::string Reference
	/// If provided, it identifies the metadata standard or other specification that defines the name field. If the reference field is not provided or is
	/// empty, the meaning of the name field is considered implicit to the characters in the string.
	std::string Reference;

	/***********************************************/
	/****************** Functions ******************/
	/***********************************************/

private:

	/// \fn X3DNodeElementMeta(const X3DNodeElementMeta& pNode)
	/// Disabled copy constructor.
	X3DNodeElementMeta(const X3DNodeElementMeta& pNode);

	/// \fn X3DNodeElementMeta& operator=(const X3DNodeElementMeta& pNode)
	/// Disabled assign operator.
	X3DNodeElementMeta& operator=(const X3DNodeElementMeta& pNode);

	/// \fn X3DNodeElementMeta()
	/// Disabled default constructor.
	X3DNodeElementMeta();

public:

	/// \fn X3DNodeElementMeta(const X3DElemType pType, X3DNodeElementBase* pParent)
	/// In constructor inheritor must set element type.
	/// \param [in] pType - element type.
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementMeta(const X3DElemType pType, X3DNodeElementBase* pParent)
		: X3DNodeElementBase(pType, pParent)
	{}

};// class X3DNodeElementMeta

/// \struct X3DNodeElementMetaBoolean
/// This struct describe metavalue of type boolean.
struct X3DNodeElementMetaBoolean : public X3DNodeElementMeta
{
	std::vector<bool> Value;///< Stored value.

	/// \fn X3DNodeElementMetaBoolean(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementMetaBoolean(X3DNodeElementBase* pParent)
		: X3DNodeElementMeta(ENET_MetaBoolean, pParent)
	{}

};// struct X3DNodeElementMetaBoolean

/// \struct X3DNodeElementMetaDouble
/// This struct describe metavalue of type double.
struct X3DNodeElementMetaDouble : public X3DNodeElementMeta
{
	std::vector<double> Value;///< Stored value.

	/// \fn X3DNodeElementMetaDouble(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementMetaDouble(X3DNodeElementBase* pParent)
		: X3DNodeElementMeta(ENET_MetaDouble, pParent)
	{}

};// struct X3DNodeElementMetaDouble

/// \struct X3DNodeElementMetaFloat
/// This struct describe metavalue of type float.
struct X3DNodeElementMetaFloat : public X3DNodeElementMeta
{
	std::vector<float> Value;///< Stored value.

	/// \fn X3DNodeElementMetaFloat(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementMetaFloat(X3DNodeElementBase* pParent)
		: X3DNodeElementMeta(ENET_MetaFloat, pParent)
	{}

};// struct X3DNodeElementMetaFloat

/// \struct X3DNodeElementMetaInt
/// This struct describe metavalue of type integer.
struct X3DNodeElementMetaInt : public X3DNodeElementMeta
{
	std::vector<int32_t> Value;///< Stored value.

	/// \fn X3DNodeElementMetaInt(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementMetaInt(X3DNodeElementBase* pParent)
		: X3DNodeElementMeta(ENET_MetaInteger, pParent)
	{}

};// struct X3DNodeElementMetaInt

/// \struct CX3DImporter_NodeElement_MetaSet
/// This struct describe container for metaobjects.
struct CX3DImporter_NodeElement_MetaSet : public X3DNodeElementMeta
{
	std::list<X3DNodeElementMeta> Value;///< Stored value.

	/// \fn CX3DImporter_NodeElement_MetaSet(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	CX3DImporter_NodeElement_MetaSet(X3DNodeElementBase* pParent)
		: X3DNodeElementMeta(ENET_MetaSet, pParent)
	{}

};// struct CX3DImporter_NodeElement_MetaSet

/// \struct X3DNodeElementMetaString
/// This struct describe metavalue of type string.
struct X3DNodeElementMetaString : public X3DNodeElementMeta
{
	std::list<std::string> Value;///< Stored value.

	/// \fn X3DNodeElementMetaString(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementMetaString(X3DNodeElementBase* pParent)
		: X3DNodeElementMeta(ENET_MetaString, pParent)
	{}

};// struct X3DNodeElementMetaString

/// \struct X3DNodeElementColor
/// This struct hold <Color> value.
struct X3DNodeElementColor : public X3DNodeElementBase
{
	std::list<aiColor3D> Value;///< Stored value.

	/// \fn X3DNodeElementColor(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementColor(X3DNodeElementBase* pParent)
		: X3DNodeElementBase(ENET_Color, pParent)
	{}

};// struct X3DNodeElementColor

/// \struct X3DNodeElementColorRGBA
/// This struct hold <ColorRGBA> value.
struct X3DNodeElementColorRGBA : public X3DNodeElementBase
{
	std::list<aiColor4D> Value;///< Stored value.

	/// \fn X3DNodeElementColorRGBA(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementColorRGBA(X3DNodeElementBase* pParent)
		: X3DNodeElementBase(ENET_ColorRGBA, pParent)
	{}

};// struct X3DNodeElementColorRGBA

/// \struct X3DNodeElementCoordinate
/// This struct hold <Coordinate> value.
struct X3DNodeElementCoordinate : public X3DNodeElementBase
{
	std::list<aiVector3D> Value;///< Stored value.

	/// \fn X3DNodeElementCoordinate(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementCoordinate(X3DNodeElementBase* pParent)
		: X3DNodeElementBase(ENET_Coordinate, pParent)
	{}

};// struct X3DNodeElementCoordinate

/// \struct X3DNodeElementNormal
/// This struct hold <Normal> value.
struct X3DNodeElementNormal : public X3DNodeElementBase
{
	std::list<aiVector3D> Value;///< Stored value.

	/// \fn X3DNodeElementNormal(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementNormal(X3DNodeElementBase* pParent)
		: X3DNodeElementBase(ENET_Normal, pParent)
	{}

};// struct X3DNodeElementNormal

/// \struct X3DNodeElementTextureCoordinate
/// This struct hold <TextureCoordinate> value.
struct X3DNodeElementTextureCoordinate : public X3DNodeElementBase
{
	std::list<aiVector2D> Value;///< Stored value.

	/// \fn X3DNodeElementTextureCoordinate(X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementTextureCoordinate(X3DNodeElementBase* pParent)
		: X3DNodeElementBase(ENET_TextureCoordinate, pParent)
	{}

};// struct X3DNodeElementTextureCoordinate

/// \class X3DNodeElementGeometry2D
/// Two-dimensional figure.
class X3DNodeElementGeometry2D : public X3DNodeElementBase
{
	/***********************************************/
	/****************** Variables ******************/
	/***********************************************/

public:

	std::list<aiVector3D> Vertices;///< Vertices list.
	size_t NumIndices;///< Number of indices in one face.
	bool Solid;///< Flag: if true then render must use back-face culling, else render must draw both sides of object.

	/***********************************************/
	/****************** Functions ******************/
	/***********************************************/

private:

	/// \fn X3DNodeElementGeometry2D(const X3DNodeElementGeometry2D& pNode)
	/// Disabled copy constructor.
	X3DNodeElementGeometry2D(const X3DNodeElementGeometry2D& pNode);

	/// \fn X3DNodeElementGeometry2D& operator=(const X3DNodeElementGeometry2D& pNode)
	/// Disabled assign operator.
	X3DNodeElementGeometry2D& operator=(const X3DNodeElementGeometry2D& pNode);

public:

	/// \fn X3DNodeElementGeometry2D(const X3DElemType pType, X3DNodeElementBase* pParent)
	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	/// \param [in] pType - type of geometry object.
	X3DNodeElementGeometry2D(const X3DElemType pType, X3DNodeElementBase* pParent)
		: X3DNodeElementBase(pType, pParent), Solid(true)
	{}

};// class X3DNodeElementGeometry2D

/// \class X3DNodeElementGeometry3D
/// Three-dimensional body.
class X3DNodeElementGeometry3D : public X3DNodeElementBase {
public:
	std::list<aiVector3D> Vertices;  ///< Vertices list.
	size_t                NumIndices;///< Number of indices in one face.
	bool                  Solid;     ///< Flag: if true then render must use back-face culling, else render must draw both sides of object.

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	/// \param [in] pType - type of geometry object.
	X3DNodeElementGeometry3D(const X3DElemType pType, X3DNodeElementBase* pParent)
	: X3DNodeElementBase(pType, pParent)
	, Vertices()
	, NumIndices( 0 )
	, Solid(true) {
        // empty		
	}

private:
	/// Disabled copy constructor.
	X3DNodeElementGeometry3D(const X3DNodeElementGeometry3D& pNode);

	/// Disabled assign operator.
	X3DNodeElementGeometry3D& operator=(const X3DNodeElementGeometry3D& pNode);
};// class X3DNodeElementGeometry3D

/// \class X3DNodeElementElevationGrid
/// Uniform rectangular grid of varying height.
class X3DNodeElementElevationGrid : public X3DNodeElementGeometry3D
{
	/***********************************************/
	/****************** Variables ******************/
	/***********************************************/

public:

	bool NormalPerVertex;///< If true then normals are defined for every vertex, else for every face(line).
	bool ColorPerVertex;///< If true then colors are defined for every vertex, else for every face(line).
	/// \var CreaseAngle
	/// If the angle between the geometric normals of two adjacent faces is less than the crease angle, normals shall be calculated so that the faces are
	/// shaded smoothly across the edge; otherwise, normals shall be calculated so that a lighting discontinuity across the edge is produced.
	float CreaseAngle;
	std::vector<int32_t> CoordIdx;///< Coordinates list by faces. In X3D format: "-1" - delimiter for faces.

	/***********************************************/
	/****************** Functions ******************/
	/***********************************************/

private:

	/// \fn X3DNodeElementElevationGrid(const X3DNodeElementElevationGrid& pNode)
	/// Disabled copy constructor.
	X3DNodeElementElevationGrid(const X3DNodeElementElevationGrid& pNode);

	/// \fn X3DNodeElementElevationGrid& operator=(const X3DNodeElementElevationGrid& pNode)
	/// Disabled assign operator.
	X3DNodeElementElevationGrid& operator=(const X3DNodeElementElevationGrid& pNode);

public:

	/// \fn X3DNodeElementElevationGrid(const X3DElemType pType, X3DNodeElementBase* pParent)
	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	/// \param [in] pType - type of geometry object.
	X3DNodeElementElevationGrid(const X3DElemType pType, X3DNodeElementBase* pParent)
		: X3DNodeElementGeometry3D(pType, pParent)
	{}

};// class X3DNodeElementIndexedSet

/// \class X3DNodeElementIndexedSet
/// Shape with indexed vertices.
class X3DNodeElementIndexedSet : public X3DNodeElementGeometry3D
{
	/***********************************************/
	/****************** Variables ******************/
	/***********************************************/

public:

	/// \var CCW
	/// The ccw field defines the ordering of the vertex coordinates of the geometry with respect to user-given or automatically generated normal vectors
	/// used in the lighting model equations. If ccw is TRUE, the normals shall follow the right hand rule; the orientation of each normal with respect to
	/// the vertices (taken in order) shall be such that the vertices appear to be oriented in a counterclockwise order when the vertices are viewed (in the
	/// local coordinate system of the Shape) from the opposite direction as the normal. If ccw is FALSE, the normals shall be oriented in the opposite
	/// direction. If normals are not generated but are supplied using a Normal node, and the orientation of the normals does not match the setting of the
	/// ccw field, results are undefined.
	bool CCW;
	std::vector<int32_t> ColorIndex;///< Field to specify the polygonal faces by indexing into the <Color> or <ColorRGBA>.
	bool ColorPerVertex;///< If true then colors are defined for every vertex, else for every face(line).
	/// \var Convex
	/// The convex field indicates whether all polygons in the shape are convex (TRUE). A polygon is convex if it is planar, does not intersect itself,
	/// and all of the interior angles at its vertices are less than 180 degrees. Non planar and self intersecting polygons may produce undefined results
	/// even if the convex field is FALSE.
	bool Convex;
	std::vector<int32_t> CoordIndex;///< Field to specify the polygonal faces by indexing into the <Coordinate>.
	/// \var CreaseAngle
	/// If the angle between the geometric normals of two adjacent faces is less than the crease angle, normals shall be calculated so that the faces are
	/// shaded smoothly across the edge; otherwise, normals shall be calculated so that a lighting discontinuity across the edge is produced.
	float CreaseAngle;
	std::vector<int32_t> NormalIndex;///< Field to specify the polygonal faces by indexing into the <Normal>.
	bool NormalPerVertex;///< If true then normals are defined for every vertex, else for every face(line).
	std::vector<int32_t> TexCoordIndex;///< Field to specify the polygonal faces by indexing into the <TextureCoordinate>.

	/***********************************************/
	/****************** Functions ******************/
	/***********************************************/

private:

	/// \fn X3DNodeElementIndexedSet(const X3DNodeElementIndexedSet& pNode)
	/// Disabled copy constructor.
	X3DNodeElementIndexedSet(const X3DNodeElementIndexedSet& pNode);

	/// \fn X3DNodeElementIndexedSet& operator=(const X3DNodeElementIndexedSet& pNode)
	/// Disabled assign operator.
	X3DNodeElementIndexedSet& operator=(const X3DNodeElementIndexedSet& pNode);

public:

	/// \fn X3DNodeElementIndexedSet(const X3DElemType pType, X3DNodeElementBase* pParent)
	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	/// \param [in] pType - type of geometry object.
	X3DNodeElementIndexedSet(const X3DElemType pType, X3DNodeElementBase* pParent)
		: X3DNodeElementGeometry3D(pType, pParent)
	{}

};// class X3DNodeElementIndexedSet

/// \class X3DNodeElementSet
/// Shape with set of vertices.
class X3DNodeElementSet : public X3DNodeElementGeometry3D
{
	/***********************************************/
	/****************** Variables ******************/
	/***********************************************/

public:

	/// \var CCW
	/// The ccw field defines the ordering of the vertex coordinates of the geometry with respect to user-given or automatically generated normal vectors
	/// used in the lighting model equations. If ccw is TRUE, the normals shall follow the right hand rule; the orientation of each normal with respect to
	/// the vertices (taken in order) shall be such that the vertices appear to be oriented in a counterclockwise order when the vertices are viewed (in the
	/// local coordinate system of the Shape) from the opposite direction as the normal. If ccw is FALSE, the normals shall be oriented in the opposite
	/// direction. If normals are not generated but are supplied using a Normal node, and the orientation of the normals does not match the setting of the
	/// ccw field, results are undefined.
	bool CCW;
	bool ColorPerVertex;///< If true then colors are defined for every vertex, else for every face(line).
	bool NormalPerVertex;///< If true then normals are defined for every vertex, else for every face(line).
	std::vector<int32_t> CoordIndex;///< Field to specify the polygonal faces by indexing into the <Coordinate>.
	std::vector<int32_t> NormalIndex;///< Field to specify the polygonal faces by indexing into the <Normal>.
	std::vector<int32_t> TexCoordIndex;///< Field to specify the polygonal faces by indexing into the <TextureCoordinate>.
	std::vector<int32_t> VertexCount;///< Field describes how many vertices are to be used in each polyline(polygon) from the <Coordinate> field.

	/***********************************************/
	/****************** Functions ******************/
	/***********************************************/

private:

	/// \fn X3DNodeElementSet(const X3DNodeElementSet& pNode)
	/// Disabled copy constructor.
	X3DNodeElementSet(const X3DNodeElementSet& pNode);

	/// \fn X3DNodeElementSet& operator=(const X3DNodeElementSet& pNode)
	/// Disabled assign operator.
	X3DNodeElementSet& operator=(const X3DNodeElementSet& pNode);

public:

	/// \fn X3DNodeElementSet(const X3DElemType pType, X3DNodeElementBase* pParent)
	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	/// \param [in] pType - type of geometry object.
	X3DNodeElementSet(const X3DElemType pType, X3DNodeElementBase* pParent)
		: X3DNodeElementGeometry3D(pType, pParent)
	{}

};// class X3DNodeElementSet

/// \struct X3DNodeElementShape
/// This struct hold <Shape> value.
struct X3DNodeElementShape : public X3DNodeElementBase
{
	/// \fn X3DNodeElementShape(X3DNodeElementShape* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementShape(X3DNodeElementBase* pParent)
		: X3DNodeElementBase(ENET_Shape, pParent)
	{}

};// struct X3DNodeElementShape

/// \struct CX3DImporter_NodeElement_Appearance
/// This struct hold <Appearance> value.
struct CX3DImporter_NodeElement_Appearance : public X3DNodeElementBase
{
	/// \fn CX3DImporter_NodeElement_Appearance(CX3DImporter_NodeElement_Appearance* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	CX3DImporter_NodeElement_Appearance(X3DNodeElementBase* pParent)
		: X3DNodeElementBase(ENET_Appearance, pParent)
	{}

};// struct CX3DImporter_NodeElement_Appearance

/// \class X3DNodeElementMaterial
/// Material.
class X3DNodeElementMaterial : public X3DNodeElementBase {
public:
	float     AmbientIntensity;///< Specifies how much ambient light from light sources this surface shall reflect.
	aiColor3D DiffuseColor;    ///< Reflects all X3D light sources depending on the angle of the surface with respect to the light source.
	aiColor3D EmissiveColor;   ///< Models "glowing" objects. This can be useful for displaying pre-lit models.
	float     Shininess;       ///< Lower shininess values produce soft glows, while higher values result in sharper, smaller highlights.
	aiColor3D SpecularColor;   ///< The specularColor and shininess fields determine the specular highlights.
	float     Transparency;    ///< Specifies how "clear" an object is, with 1.0 being completely transparent, and 0.0 completely opaque.

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	/// \param [in] pType - type of geometry object.
	X3DNodeElementMaterial(X3DNodeElementBase* pParent)
	: X3DNodeElementBase(ENET_Material, pParent)
	, AmbientIntensity( 0.0f )
	, DiffuseColor()
	, EmissiveColor()
	, Shininess( 0.0f )
	, SpecularColor()
	, Transparency( 1.0f ) {
		// empty
	}

private:
	/// Disabled copy constructor.
	X3DNodeElementMaterial(const X3DNodeElementMaterial& pNode);

	/// Disabled assign operator.
	X3DNodeElementMaterial& operator=(const X3DNodeElementMaterial& pNode);
};// class X3DNodeElementMaterial

/// \struct X3DNodeElementImageTexture
/// This struct hold <ImageTexture> value.
struct X3DNodeElementImageTexture : public X3DNodeElementBase
{
	/// \var RepeatS
	/// RepeatS and RepeatT, that specify how the texture wraps in the S and T directions. If repeatS is TRUE (the default), the texture map is repeated
	/// outside the [0.0, 1.0] texture coordinate range in the S direction so that it fills the shape. If repeatS is FALSE, the texture coordinates are
	/// clamped in the S direction to lie within the [0.0, 1.0] range. The repeatT field is analogous to the repeatS field.
	bool RepeatS;
	bool RepeatT;///< See \ref RepeatS.
	std::string URL;///< URL of the texture.
	/// \fn X3DNodeElementImageTexture(X3DNodeElementImageTexture* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementImageTexture(X3DNodeElementBase* pParent)
		: X3DNodeElementBase(ENET_ImageTexture, pParent)
	{}

};// struct X3DNodeElementImageTexture

/// \struct X3DNodeElementTextureTransform
/// This struct hold <TextureTransform> value.
struct X3DNodeElementTextureTransform : public X3DNodeElementBase
{
	aiVector2D Center;///< Specifies a translation offset in texture coordinate space about which the rotation and scale fields are applied.
	float Rotation;///< Specifies a rotation in angle base units of the texture coordinates about the center point after the scale has been applied.
	aiVector2D Scale;///< Specifies a scaling factor in S and T of the texture coordinates about the center point.
	aiVector2D Translation;///<  Specifies a translation of the texture coordinates.

	/// \fn X3DNodeElementTextureTransform(X3DNodeElementTextureTransform* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	X3DNodeElementTextureTransform(X3DNodeElementBase* pParent)
		: X3DNodeElementBase(ENET_TextureTransform, pParent)
	{}

};// struct X3DNodeElementTextureTransform

/// \struct X3DNodeElementLight
/// This struct hold <TextureTransform> value.
struct X3DNodeElementLight : public X3DNodeElementBase
{
	float AmbientIntensity;///< Specifies the intensity of the ambient emission from the light.
	aiColor3D Color;///< specifies the spectral colour properties of both the direct and ambient light emission as an RGB value.
	aiVector3D Direction;///< Specifies the direction vector of the illumination emanating from the light source in the local coordinate system.
	/// \var Global
	/// Field that determines whether the light is global or scoped. Global lights illuminate all objects that fall within their volume of lighting influence.
	/// Scoped lights only illuminate objects that are in the same transformation hierarchy as the light.
	bool Global;
	float Intensity;///< Specifies the brightness of the direct emission from the light.
	/// \var Attenuation
	/// PointLight node's illumination falls off with distance as specified by three attenuation coefficients. The attenuation factor
	/// is: "1 / max(attenuation[0] + attenuation[1] * r + attenuation[2] * r2, 1)", where r is the distance from the light to the surface being illuminated.
	aiVector3D Attenuation;
	aiVector3D Location;///< Specifies a translation offset of the centre point of the light source from the light's local coordinate system origin.
	float Radius;///< Specifies the radial extent of the solid angle and the maximum distance from location that may be illuminated by the light source.
	float BeamWidth;///< Specifies an inner solid angle in which the light source emits light at uniform full intensity.
	float CutOffAngle;///< The light source's emission intensity drops off from the inner solid angle (beamWidth) to the outer solid angle (cutOffAngle).

	/// \fn X3DNodeElementLight(X3DElemType pLightType, X3DNodeElementBase* pParent)
	/// Constructor
	/// \param [in] pParent - pointer to parent node.
	/// \param [in] pLightType - type of the light source.
	X3DNodeElementLight(X3DElemType pLightType, X3DNodeElementBase* pParent)
		: X3DNodeElementBase(pLightType, pParent)
	{}

};// struct X3DNodeElementLight

#endif // INCLUDED_AI_X3D_IMPORTER_NODE_H
