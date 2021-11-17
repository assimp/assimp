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
#include <assimp/types.h>

#include <list>
#include <vector>

enum X3DElemType {
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

    virtual ~X3DNodeElementBase() {
        // empty
    }

protected:
    X3DNodeElementBase(X3DElemType type, X3DNodeElementBase *pParent) :
            Parent(pParent), Type(type) {
        // empty
    }
};

/// This struct hold <Color> value.
struct X3DNodeElementColor : X3DNodeElementBase {
    std::list<aiColor3D> Value; ///< Stored value.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    X3DNodeElementColor(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Color, pParent) {}

}; // struct X3DNodeElementColor

/// This struct hold <ColorRGBA> value.
struct X3DNodeElementColorRGBA : X3DNodeElementBase {
    std::list<aiColor4D> Value; ///< Stored value.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    X3DNodeElementColorRGBA(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_ColorRGBA, pParent) {}

}; // struct X3DNodeElementColorRGBA

/// This struct hold <Coordinate> value.
struct X3DNodeElementCoordinate : public X3DNodeElementBase {
    std::list<aiVector3D> Value; ///< Stored value.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    X3DNodeElementCoordinate(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Coordinate, pParent) {}

}; // struct X3DNodeElementCoordinate

/// This struct hold <Normal> value.
struct X3DNodeElementNormal : X3DNodeElementBase {
    std::list<aiVector3D> Value; ///< Stored value.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    X3DNodeElementNormal(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Normal, pParent) {}

}; // struct X3DNodeElementNormal

/// This struct hold <TextureCoordinate> value.
struct X3DNodeElementTextureCoordinate : X3DNodeElementBase {
    std::list<aiVector2D> Value; ///< Stored value.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    X3DNodeElementTextureCoordinate(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_TextureCoordinate, pParent) {}

}; // struct X3DNodeElementTextureCoordinate

/// Two-dimensional figure.
struct X3DNodeElementGeometry2D : X3DNodeElementBase {
    std::list<aiVector3D> Vertices; ///< Vertices list.
    size_t NumIndices; ///< Number of indices in one face.
    bool Solid; ///< Flag: if true then render must use back-face culling, else render must draw both sides of object.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pType - type of geometry object.
    X3DNodeElementGeometry2D(X3DElemType pType, X3DNodeElementBase *pParent) :
            X3DNodeElementBase(pType, pParent), Solid(true) {}

}; // class X3DNodeElementGeometry2D

/// Three-dimensional body.
struct X3DNodeElementGeometry3D : X3DNodeElementBase {
    std::list<aiVector3D> Vertices; ///< Vertices list.
    size_t NumIndices; ///< Number of indices in one face.
    bool Solid; ///< Flag: if true then render must use back-face culling, else render must draw both sides of object.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pType - type of geometry object.
    X3DNodeElementGeometry3D(X3DElemType pType, X3DNodeElementBase *pParent) :
            X3DNodeElementBase(pType, pParent), Vertices(), NumIndices(0), Solid(true) {
        // empty
    }
}; // class X3DNodeElementGeometry3D

/// Uniform rectangular grid of varying height.
struct X3DNodeElementElevationGrid : X3DNodeElementGeometry3D {
    bool NormalPerVertex; ///< If true then normals are defined for every vertex, else for every face(line).
    bool ColorPerVertex; ///< If true then colors are defined for every vertex, else for every face(line).
    /// If the angle between the geometric normals of two adjacent faces is less than the crease angle, normals shall be calculated so that the faces are
    /// shaded smoothly across the edge; otherwise, normals shall be calculated so that a lighting discontinuity across the edge is produced.
    float CreaseAngle;
    std::vector<int32_t> CoordIdx; ///< Coordinates list by faces. In X3D format: "-1" - delimiter for faces.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pType - type of geometry object.
    X3DNodeElementElevationGrid(X3DElemType pType, X3DNodeElementBase *pParent) :
            X3DNodeElementGeometry3D(pType, pParent) {}
}; // class X3DNodeElementIndexedSet

/// Shape with indexed vertices.
struct X3DNodeElementIndexedSet : public X3DNodeElementGeometry3D {
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
    X3DNodeElementIndexedSet(X3DElemType pType, X3DNodeElementBase *pParent) :
            X3DNodeElementGeometry3D(pType, pParent) {}
}; // class X3DNodeElementIndexedSet

/// Shape with set of vertices.
struct X3DNodeElementSet : X3DNodeElementGeometry3D {
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
    X3DNodeElementSet(X3DElemType type, X3DNodeElementBase *pParent) :
            X3DNodeElementGeometry3D(type, pParent) {}

}; // class X3DNodeElementSet

/// This struct hold <Shape> value.
struct X3DNodeElementShape : X3DNodeElementBase {

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    X3DNodeElementShape(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Shape, pParent) {}
}; // struct X3DNodeElementShape

/// This struct hold <Appearance> value.
struct X3DNodeElementAppearance : public X3DNodeElementBase {

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    X3DNodeElementAppearance(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Appearance, pParent) {}

}; // struct X3DNodeElementAppearance

struct X3DNodeElementMaterial : public X3DNodeElementBase {
    float AmbientIntensity; ///< Specifies how much ambient light from light sources this surface shall reflect.
    aiColor3D DiffuseColor; ///< Reflects all X3D light sources depending on the angle of the surface with respect to the light source.
    aiColor3D EmissiveColor; ///< Models "glowing" objects. This can be useful for displaying pre-lit models.
    float Shininess; ///< Lower shininess values produce soft glows, while higher values result in sharper, smaller highlights.
    aiColor3D SpecularColor; ///< The specularColor and shininess fields determine the specular highlights.
    float Transparency; ///< Specifies how "clear" an object is, with 1.0 being completely transparent, and 0.0 completely opaque.

    /// Constructor.
    /// \param [in] pParent - pointer to parent node.
    /// \param [in] pType - type of geometry object.
    X3DNodeElementMaterial(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_Material, pParent),
            AmbientIntensity(0.0f),
            DiffuseColor(),
            EmissiveColor(),
            Shininess(0.0f),
            SpecularColor(),
            Transparency(1.0f) {
        // empty
    }
}; // class X3DNodeElementMaterial

/// This struct hold <ImageTexture> value.
struct X3DNodeElementImageTexture : X3DNodeElementBase {
    /// RepeatS and RepeatT, that specify how the texture wraps in the S and T directions. If repeatS is TRUE (the default), the texture map is repeated
    /// outside the [0.0, 1.0] texture coordinate range in the S direction so that it fills the shape. If repeatS is FALSE, the texture coordinates are
    /// clamped in the S direction to lie within the [0.0, 1.0] range. The repeatT field is analogous to the repeatS field.
    bool RepeatS;
    bool RepeatT; ///< See \ref RepeatS.
    std::string URL; ///< URL of the texture.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    X3DNodeElementImageTexture(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_ImageTexture, pParent) {}

}; // struct X3DNodeElementImageTexture

/// This struct hold <TextureTransform> value.
struct X3DNodeElementTextureTransform : X3DNodeElementBase {
    aiVector2D Center; ///< Specifies a translation offset in texture coordinate space about which the rotation and scale fields are applied.
    float Rotation; ///< Specifies a rotation in angle base units of the texture coordinates about the center point after the scale has been applied.
    aiVector2D Scale; ///< Specifies a scaling factor in S and T of the texture coordinates about the center point.
    aiVector2D Translation; ///<  Specifies a translation of the texture coordinates.

    /// Constructor
    /// \param [in] pParent - pointer to parent node.
    X3DNodeElementTextureTransform(X3DNodeElementBase *pParent) :
            X3DNodeElementBase(X3DElemType::ENET_TextureTransform, pParent) {}

}; // struct X3DNodeElementTextureTransform

struct X3DNodeElementGroup : X3DNodeElementBase {
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
    X3DNodeElementGroup(X3DNodeElementBase *pParent, const bool pStatic = false) :
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
    std::vector<std::string> Value; ///< Stored value.

    explicit X3DNodeElementMetaString(X3DNodeElementBase *pParent) :
            X3DNodeElementMeta(X3DElemType::ENET_MetaString, pParent) {
        // empty
    }
};

/// \struct X3DNodeElementLight
/// This struct hold <TextureTransform> value.
struct X3DNodeElementLight : X3DNodeElementBase {
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
    X3DNodeElementLight(X3DElemType pLightType, X3DNodeElementBase *pParent) :
            X3DNodeElementBase(pLightType, pParent) {}

}; // struct X3DNodeElementLight

#endif // INCLUDED_AI_X3D_IMPORTER_NODE_H
