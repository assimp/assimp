/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
*/

/// \file AMFImporter_Node.hpp
/// \brief Elements of scene graph.
/// \date 2016
/// \author smal.root@gmail.com

#pragma once
#ifndef INCLUDED_AI_AMF_IMPORTER_NODE_H
#define INCLUDED_AI_AMF_IMPORTER_NODE_H

// Header files, Assimp.
#include <assimp/scene.h>
#include <assimp/types.h>

#include <list>
#include <string>
#include <vector>

/// \class CAMFImporter_NodeElement
/// Base class for elements of nodes.
class AMFNodeElementBase {
public:
	/// Define what data type contain node element.
	enum EType {
		ENET_Color, ///< Color element: <color>.
		ENET_Constellation, ///< Grouping element: <constellation>.
		ENET_Coordinates, ///< Coordinates element: <coordinates>.
		ENET_Edge, ///< Edge element: <edge>.
		ENET_Instance, ///< Grouping element: <constellation>.
		ENET_Material, ///< Material element: <material>.
		ENET_Metadata, ///< Metadata element: <metadata>.
		ENET_Mesh, ///< Metadata element: <mesh>.
		ENET_Object, ///< Element which hold object: <object>.
		ENET_Root, ///< Root element: <amf>.
		ENET_Triangle, ///< Triangle element: <triangle>.
		ENET_TexMap, ///< Texture coordinates element: <texmap> or <map>.
		ENET_Texture, ///< Texture element: <texture>.
		ENET_Vertex, ///< Vertex element: <vertex>.
		ENET_Vertices, ///< Vertex element: <vertices>.
		ENET_Volume, ///< Volume element: <volume>.

		ENET_Invalid ///< Element has invalid type and possible contain invalid data.
	};

	const EType Type; ///< Type of element.
	std::string ID; ///< ID of element.
	AMFNodeElementBase *Parent; ///< Parent element. If nullptr then this node is root.
	std::list<AMFNodeElementBase *> Child; ///< Child elements.

public: /// Destructor, virtual..
	virtual ~AMFNodeElementBase() = default;

	/// Disabled copy constructor and co.
	AMFNodeElementBase(const AMFNodeElementBase &pNodeElement) = delete;
	AMFNodeElementBase(AMFNodeElementBase &&) = delete;
	AMFNodeElementBase &operator=(const AMFNodeElementBase &pNodeElement) = delete;
	AMFNodeElementBase() = delete;

protected:
	/// In constructor inheritor must set element type.
	/// \param [in] pType - element type.
	/// \param [in] pParent - parent element.
	AMFNodeElementBase(const EType pType, AMFNodeElementBase *pParent) :
			Type(pType), Parent(pParent) {
		// empty
	}
}; // class IAMFImporter_NodeElement

/// \struct CAMFImporter_NodeElement_Constellation
/// A collection of objects or constellations with specific relative locations.
struct AMFConstellation : public AMFNodeElementBase {
	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFConstellation(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Constellation, pParent) {}

}; // struct CAMFImporter_NodeElement_Constellation

/// \struct CAMFImporter_NodeElement_Instance
/// Part of constellation.
struct AMFInstance : public AMFNodeElementBase {

	std::string ObjectID; ///< ID of object for instantiation.
	/// \var Delta - The distance of translation in the x, y, or z direction, respectively, in the referenced object's coordinate system, to
	/// create an instance of the object in the current constellation.
	aiVector3D Delta;

	/// \var Rotation - The rotation, in degrees, to rotate the referenced object about its x, y, and z axes, respectively, to create an
	/// instance of the object in the current constellation. Rotations shall be executed in order of x first, then y, then z.
	aiVector3D Rotation;

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFInstance(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Instance, pParent) {}
};

/// \struct CAMFImporter_NodeElement_Metadata
/// Structure that define metadata node.
struct AMFMetadata : public AMFNodeElementBase {

	std::string Type; ///< Type of "Value".
	std::string Value; ///< Value.

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFMetadata(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Metadata, pParent) {}
};

/// \struct CAMFImporter_NodeElement_Root
/// Structure that define root node.
struct AMFRoot : public AMFNodeElementBase {

	std::string Unit; ///< The units to be used. May be "inch", "millimeter", "meter", "feet", or "micron".
	std::string Version; ///< Version of format.

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFRoot(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Root, pParent) {}
};

/// \struct CAMFImporter_NodeElement_Color
/// Structure that define object node.
struct AMFColor : public AMFNodeElementBase {
	bool Composed; ///< Type of color stored: if true then look for formula in \ref Color_Composed[4], else - in \ref Color.
	std::string Color_Composed[4]; ///< By components formulas of composed color. [0..3] - RGBA.
	aiColor4D Color; ///< Constant color.
	std::string Profile; ///< The ICC color space used to interpret the three color channels r, g and b..

	/// @brief  Constructor.
	/// @param [in] pParent - pointer to parent node.
	AMFColor(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Color, pParent), Composed(false), Color() {
		// empty
	}
};

/// \struct CAMFImporter_NodeElement_Material
/// Structure that define material node.
struct AMFMaterial : public AMFNodeElementBase {

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFMaterial(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Material, pParent) {}
};

/// \struct CAMFImporter_NodeElement_Object
/// Structure that define object node.
struct AMFObject : public AMFNodeElementBase {

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFObject(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Object, pParent) {}
};

/// \struct CAMFImporter_NodeElement_Mesh
/// Structure that define mesh node.
struct AMFMesh : public AMFNodeElementBase {
	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFMesh(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Mesh, pParent) {}
};

/// \struct CAMFImporter_NodeElement_Vertex
/// Structure that define vertex node.
struct AMFVertex : public AMFNodeElementBase {
	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFVertex(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Vertex, pParent) {}
};

/// \struct CAMFImporter_NodeElement_Edge
/// Structure that define edge node.
struct AMFEdge : public AMFNodeElementBase {
	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFEdge(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Edge, pParent) {}
};

/// \struct CAMFImporter_NodeElement_Vertices
/// Structure that define vertices node.
struct AMFVertices : public AMFNodeElementBase {
	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFVertices(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Vertices, pParent) {}
};

/// \struct CAMFImporter_NodeElement_Volume
/// Structure that define volume node.
struct AMFVolume : public AMFNodeElementBase {
	std::string MaterialID; ///< Which material to use.
	std::string Type; ///< What this volume describes can be "region" or "support". If none specified, "object" is assumed.

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFVolume(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Volume, pParent) {}
};

/// \struct CAMFImporter_NodeElement_Coordinates
/// Structure that define coordinates node.
struct AMFCoordinates : public AMFNodeElementBase {
	aiVector3D Coordinate; ///< Coordinate.

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFCoordinates(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Coordinates, pParent) {}
};

/// \struct CAMFImporter_NodeElement_TexMap
/// Structure that define texture coordinates node.
struct AMFTexMap : public AMFNodeElementBase {
	aiVector3D TextureCoordinate[3]; ///< Texture coordinates.
	std::string TextureID_R; ///< Texture ID for red color component.
	std::string TextureID_G; ///< Texture ID for green color component.
	std::string TextureID_B; ///< Texture ID for blue color component.
	std::string TextureID_A; ///< Texture ID for alpha color component.

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFTexMap(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_TexMap, pParent), TextureCoordinate{} {
		// empty
	}
};

/// \struct CAMFImporter_NodeElement_Triangle
/// Structure that define triangle node.
struct AMFTriangle : public AMFNodeElementBase {
	size_t V[3]; ///< Triangle vertices.

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFTriangle(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Triangle, pParent) {
		// empty
	}
};

/// Structure that define texture node.
struct AMFTexture : public AMFNodeElementBase {
	size_t Width, Height, Depth; ///< Size of the texture.
	std::vector<uint8_t> Data; ///< Data of the texture.
	bool Tiled;

	/// Constructor.
	/// \param [in] pParent - pointer to parent node.
	AMFTexture(AMFNodeElementBase *pParent) :
			AMFNodeElementBase(ENET_Texture, pParent), Width(0), Height(0), Depth(0), Data(), Tiled(false) {
		// empty
	}
};

#endif // INCLUDED_AI_AMF_IMPORTER_NODE_H
