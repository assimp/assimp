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

/// \file AMFImporter.hpp
/// \brief AMF-format files importer for Assimp.
/// \date 2016
/// \author smal.root@gmail.com
// Thanks to acorn89 for support.

#pragma once
#ifndef INCLUDED_AI_AMF_IMPORTER_H
#define INCLUDED_AI_AMF_IMPORTER_H

#include "AMFImporter_Node.hpp"

// Header files, Assimp.
#include "assimp/types.h"
#include <assimp/BaseImporter.h>
#include <assimp/XmlParser.h>
#include <assimp/importerdesc.h>
#include <assimp/DefaultLogger.hpp>

// Header files, stdlib.
#include <set>

namespace Assimp {

/// \class AMFImporter
/// Class that holding scene graph which include: geometry, metadata, materials etc.
///
/// Implementing features.
///
/// Limitations.
///
/// 1. When for texture mapping used set of source textures (r, g, b, a) not only one then attribute "tiled" for all set will be true if it true in any of
///    source textures.
///    Example. Triangle use for texture mapping three textures. Two of them has "tiled" set to false and one - set to true. In scene all three textures
///    will be tiled.
///
/// Unsupported features:
/// 1. Node <composite>, formulas in <composite> and <color>. For implementing this feature can be used expression parser "muParser" like in project
///    "amf_tools".
/// 2. Attribute "profile" in node <color>.
/// 3. Curved geometry: <edge>, <normal> and children nodes of them.
/// 4. Attributes: "unit" and "version" in <amf> read but do nothing.
/// 5. <metadata> stored only for root node <amf>.
/// 6. Color averaging of vertices for which <triangle>'s set different colors.
///
/// Supported nodes:
///    General:
///        <amf>; <constellation>; <instance> and children <deltax>, <deltay>, <deltaz>, <rx>, <ry>, <rz>; <metadata>;
///
///    Geometry:
///        <object>; <mesh>; <vertices>; <vertex>; <coordinates> and children <x>, <y>, <z>; <volume>; <triangle> and children <v1>, <v2>, <v3>;
///
///    Material:
///        <color> and children <r>, <g>, <b>, <a>; <texture>; <material>;
///        two variants of texture coordinates:
///            new - <texmap> and children <utex1>, <utex2>, <utex3>, <vtex1>, <vtex2>, <vtex3>
///            old - <map> and children <u1>, <u2>, <u3>, <v1>, <v2>, <v3>
///
class AMFImporter : public BaseImporter {
    using AMFMetaDataArray = std::vector<AMFMetadata *>;
    using MeshArray = std::vector<aiMesh *>;
    using NodeArray = std::vector<aiNode *>;

public:
    struct SPP_Material;

    /// Data type for post-processing step. More suitable container for part of material's composition.
    struct SPP_Composite {
        SPP_Material *Material; ///< Pointer to material - part of composition.
        std::string Formula; ///< Formula for calculating ratio of \ref Material.
    };

    /// Data type for post-processing step. More suitable container for texture.
    struct SPP_Texture {
        std::string ID;
        size_t Width, Height, Depth;
        bool Tiled;
        char FormatHint[9]; // 8 for string + 1 for terminator.
        uint8_t *Data;
    };

    /// Data type for post-processing step. Contain face data.
    struct SComplexFace {
        aiFace Face; ///< Face vertices.
        const AMFColor *Color; ///< Face color. Equal to nullptr if color is not set for the face.
        const AMFTexMap *TexMap; ///< Face texture mapping data. Equal to nullptr if texture mapping is not set for the face.
    };

    /// Data type for post-processing step. More suitable container for material.
    struct SPP_Material {
        std::string ID; ///< Material ID.
        std::list<AMFMetadata *> Metadata; ///< Metadata of material.
        AMFColor *Color; ///< Color of material.
        std::list<SPP_Composite> Composition; ///< List of child materials if current material is composition of few another.

        /// Return color calculated for specified coordinate.
        /// \param [in] pX - "x" coordinate.
        /// \param [in] pY - "y" coordinate.
        /// \param [in] pZ - "z" coordinate.
        /// \return calculated color.
        aiColor4D GetColor(const float pX, const float pY, const float pZ) const;
    };

    /// Default constructor.
    AMFImporter() AI_NO_EXCEPT;

    /// Default destructor.
    ~AMFImporter() override;

    /// Parse AMF file and fill scene graph. The function has no return value. Result can be found by analyzing the generated graph.
    /// Also exception can be thrown if trouble will found.
    /// \param [in] pFile - name of file to be parsed.
    /// \param [in] pIOHandler - pointer to IO helper object.
    void ParseFile(const std::string &pFile, IOSystem *pIOHandler);
    void ParseHelper_Node_Enter(AMFNodeElementBase *child);
    void ParseHelper_Node_Exit();
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool pCheckSig) const override;
    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) override;
    const aiImporterDesc *GetInfo() const override;
    bool Find_NodeElement(const std::string &pID, const AMFNodeElementBase::EType pType, AMFNodeElementBase **pNodeElement) const;
    bool Find_ConvertedNode(const std::string &pID, NodeArray &nodeArray, aiNode **pNode) const;
    bool Find_ConvertedMaterial(const std::string &pID, const SPP_Material **pConvertedMaterial) const;
    AI_WONT_RETURN void Throw_CloseNotFound(const std::string &nodeName) AI_WONT_RETURN_SUFFIX;
    AI_WONT_RETURN void Throw_IncorrectAttr(const std::string &nodeName, const std::string &pAttrName) AI_WONT_RETURN_SUFFIX;
    AI_WONT_RETURN void Throw_IncorrectAttrValue(const std::string &nodeName, const std::string &pAttrName) AI_WONT_RETURN_SUFFIX;
    AI_WONT_RETURN void Throw_MoreThanOnceDefined(const std::string &nodeName, const std::string &pNodeType, const std::string &pDescription) AI_WONT_RETURN_SUFFIX;
    AI_WONT_RETURN void Throw_ID_NotFound(const std::string &pID) const AI_WONT_RETURN_SUFFIX;
    void XML_CheckNode_MustHaveChildren(pugi::xml_node &node);
    bool XML_SearchNode(const std::string &nodeName);
    AMFImporter(const AMFImporter &pScene) = delete;
    AMFImporter &operator=(const AMFImporter &pScene) = delete;

private:
    /// Clear all temporary data.
    void Clear();

    /// Get data stored in <vertices> and place it to arrays.
    /// \param [in] pNodeElement - reference to node element which kept <object> data.
    /// \param [in] pVertexCoordinateArray - reference to vertices coordinates kept in <vertices>.
    /// \param [in] pVertexColorArray - reference to vertices colors for all <vertex's. If color for vertex is not set then corresponding member of array
    /// contain nullptr.
    void PostprocessHelper_CreateMeshDataArray(const AMFMesh &pNodeElement, std::vector<aiVector3D> &pVertexCoordinateArray,
            std::vector<AMFColor *> &pVertexColorArray) const;

    /// Return converted texture ID which related to specified source textures ID's. If converted texture does not exist then it will be created and ID on new
    /// converted texture will be returned. Conversion: set of textures from \ref CAMFImporter_NodeElement_Texture to one \ref SPP_Texture and place it
    /// to converted textures list.
    /// Any of source ID's can be absent(empty string) or even one ID only specified. But at least one ID must be specified.
    /// \param [in] pID_R - ID of source "red" texture.
    /// \param [in] pID_G - ID of source "green" texture.
    /// \param [in] pID_B - ID of source "blue" texture.
    /// \param [in] pID_A - ID of source "alpha" texture.
    /// \return index of the texture in array of the converted textures.
    size_t PostprocessHelper_GetTextureID_Or_Create(const std::string &pID_R, const std::string &pID_G, const std::string &pID_B, const std::string &pID_A);

    /// Separate input list by texture IDs. This step is needed because aiMesh can contain mesh which is use only one texture (or set: diffuse, bump etc).
    /// \param [in] pInputList - input list with faces. Some of them can contain color or texture mapping, or both of them, or nothing. Will be cleared after
    /// processing.
    /// \param [out] pOutputList_Separated - output list of the faces lists. Separated faces list by used texture IDs. Will be cleared before processing.
    void PostprocessHelper_SplitFacesByTextureID(std::list<SComplexFace> &pInputList, std::list<std::list<SComplexFace>> &pOutputList_Separated);

    /// Check if child elements of node element is metadata and add it to scene node.
    /// \param [in] pMetadataList - reference to list with collected metadata.
    /// \param [out] pSceneNode - scene node in which metadata will be added.
    void Postprocess_AddMetadata(const AMFMetaDataArray &pMetadataList, aiNode &pSceneNode) const;

    /// To create aiMesh and aiNode for it from <object>.
    /// \param [in] pNodeElement - reference to node element which kept <object> data.
    /// \param [out] meshList    - reference to a list with all aiMesh of the scene.
    /// \param [out] pSceneNode  - pointer to place where new aiNode will be created.
    void Postprocess_BuildNodeAndObject(const AMFObject &pNodeElement, MeshArray &meshList, aiNode **pSceneNode);

    /// Create mesh for every <volume> in <mesh>.
    /// \param [in] pNodeElement - reference to node element which kept <mesh> data.
    /// \param [in] pVertexCoordinateArray - reference to vertices coordinates for all <volume>'s.
    /// \param [in] pVertexColorArray - reference to vertices colors for all <volume>'s. If color for vertex is not set then corresponding member of array
    /// contain nullptr.
    /// \param [in] pObjectColor - pointer to colors for <object>. If color is not set then argument contain nullptr.
    /// \param [in] pMaterialList - reference to a list with defined materials.
    /// \param [out] pMeshList - reference to a list with all aiMesh of the scene.
    /// \param [out] pSceneNode - reference to aiNode which will own new aiMesh's.
    void Postprocess_BuildMeshSet(const AMFMesh &pNodeElement, const std::vector<aiVector3D> &pVertexCoordinateArray,
            const std::vector<AMFColor *> &pVertexColorArray, const AMFColor *pObjectColor,
            MeshArray &pMeshList, aiNode &pSceneNode);

    /// Convert material from \ref CAMFImporter_NodeElement_Material to \ref SPP_Material.
    /// \param [in] pMaterial - source CAMFImporter_NodeElement_Material.
    void Postprocess_BuildMaterial(const AMFMaterial &pMaterial);

    /// Create and add to aiNode's list new part of scene graph defined by <constellation>.
    /// \param [in] pConstellation - reference to <constellation> node.
    /// \param [out] nodeArray     - reference to aiNode's list.
    void Postprocess_BuildConstellation(AMFConstellation &pConstellation, NodeArray &nodeArray) const;

    /// Build Assimp scene graph in aiScene from collected data.
    /// \param [out] pScene - pointer to aiScene where tree will be built.
    void Postprocess_BuildScene(aiScene *pScene);

    /// Decode Base64-encoded data.
    /// \param [in] pInputBase64 - reference to input Base64-encoded string.
    /// \param [out] pOutputData - reference to output array for decoded data.
    void ParseHelper_Decode_Base64(const std::string &pInputBase64, std::vector<uint8_t> &pOutputData) const;

    /// Parse <AMF> node of the file.
    void ParseNode_Root();

    /// Parse <constellation> node of the file.
    void ParseNode_Constellation(XmlNode &node);

    /// Parse <instance> node of the file.
    void ParseNode_Instance(XmlNode &node);

    /// Parse <material> node of the file.
    void ParseNode_Material(XmlNode &node);

    /// Parse <metadata> node.
    void ParseNode_Metadata(XmlNode &node);

    /// Parse <object> node of the file.
    void ParseNode_Object(XmlNode &node);

    /// Parse <texture> node of the file.
    void ParseNode_Texture(XmlNode &node);

    /// Parse <coordinates> node of the file.
    void ParseNode_Coordinates(XmlNode &node);

    /// Parse <edge> node of the file.
    void ParseNode_Edge(XmlNode &node);

    /// Parse <mesh> node of the file.
    void ParseNode_Mesh(XmlNode &node);

    /// Parse <triangle> node of the file.
    void ParseNode_Triangle(XmlNode &node);

    /// Parse <vertex> node of the file.
    void ParseNode_Vertex(XmlNode &node);

    /// Parse <vertices> node of the file.
    void ParseNode_Vertices(XmlNode &node);

    /// Parse <volume> node of the file.
    void ParseNode_Volume(XmlNode &node);

    /// Parse <color> node of the file.
    void ParseNode_Color(XmlNode &node);

    /// Parse <texmap> of <map> node of the file.
    /// \param [in] pUseOldName - if true then use old name of node(and children) - <map>, instead of new name - <texmap>.
    void ParseNode_TexMap(XmlNode &node, const bool pUseOldName = false);



private:
    AMFNodeElementBase *mNodeElement_Cur; ///< Current element.
    std::list<AMFNodeElementBase *> mNodeElement_List; ///< All elements of scene graph.
    XmlParser *mXmlParser;
    std::string mUnit;
    std::string mVersion;
    std::list<SPP_Material> mMaterial_Converted; ///< List of converted materials for postprocessing step.
    std::list<SPP_Texture> mTexture_Converted; ///< List of converted textures for postprocessing step.
};

} // namespace Assimp

#endif // INCLUDED_AI_AMF_IMPORTER_H
