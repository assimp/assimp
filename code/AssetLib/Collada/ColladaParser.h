/*
 Open Asset Import Library (assimp)
 ----------------------------------------------------------------------

 Copyright (c) 2006-2024, assimp team

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

/** @file ColladaParser.h
 *  @brief Defines the parser helper class for the collada loader
 */

#pragma once
#ifndef AI_COLLADAPARSER_H_INC
#define AI_COLLADAPARSER_H_INC

#include "ColladaHelper.h"
#include <assimp/TinyFormatter.h>
#include <assimp/ai_assert.h>
#include <assimp/XmlParser.h>

#include <map>

namespace Assimp {

class ZipArchiveIOSystem;

// ------------------------------------------------------------------------------------------
/** Parser helper class for the Collada loader.
     *
     *  Does all the XML reading and builds internal data structures from it,
     *  but leaves the resolving of all the references to the loader.
     */
class ColladaParser {
    friend class ColladaLoader;

    /** Converts a path read from a collada file to the usual representation */
    static void UriDecodePath(aiString &ss);

protected:
    /** Map for generic metadata as aiString */
    typedef std::map<std::string, aiString> StringMetaData;

    /** Constructor from XML file */
    ColladaParser(IOSystem *pIOHandler, const std::string &pFile);

    /** Destructor */
    ~ColladaParser();

    /** Attempts to read the ZAE manifest and returns the DAE to open */
    static std::string ReadZaeManifest(ZipArchiveIOSystem &zip_archive);

    /** Reads the contents of the file */
    void ReadContents(XmlNode &node);

    /** Reads the structure of the file */
    void ReadStructure(XmlNode &node);

    /** Reads asset information such as coordinate system information and legal blah */
    void ReadAssetInfo(XmlNode &node);

    /** Reads contributor information such as author and legal blah */
    void ReadContributorInfo(XmlNode &node);

    /** Reads generic metadata into provided map and renames keys for Assimp */
    void ReadMetaDataItem(XmlNode &node, StringMetaData &metadata);

    /** Reads the animation library */
    void ReadAnimationLibrary(XmlNode &node);

    /** Reads the animation clip library */
    void ReadAnimationClipLibrary(XmlNode &node);

    /** Unwrap controllers dependency hierarchy */
    void PostProcessControllers();

    /** Re-build animations from animation clip library, if present, otherwise combine single-channel animations */
    void PostProcessRootAnimations();

    /** Reads an animation into the given parent structure */
    void ReadAnimation(XmlNode &node, Collada::Animation *pParent);

    /** Reads an animation sampler into the given anim channel */
    void ReadAnimationSampler(XmlNode &node, Collada::AnimationChannel &pChannel);

    /** Reads the skeleton controller library */
    void ReadControllerLibrary(XmlNode &node);

    /** Reads a controller into the given mesh structure */
    void ReadController(XmlNode &node, Collada::Controller &pController);

    /** Reads the joint definitions for the given controller */
    void ReadControllerJoints(XmlNode &node, Collada::Controller &pController);

    /** Reads the joint weights for the given controller */
    void ReadControllerWeights(XmlNode &node, Collada::Controller &pController);

    /** Reads the image library contents */
    void ReadImageLibrary(XmlNode &node);

    /** Reads an image entry into the given image */
    void ReadImage(XmlNode &node, Collada::Image &pImage);

    /** Reads the material library */
    void ReadMaterialLibrary(XmlNode &node);

    /** Reads a material entry into the given material */
    void ReadMaterial(XmlNode &node, Collada::Material &pMaterial);

    /** Reads the camera library */
    void ReadCameraLibrary(XmlNode &node);

    /** Reads a camera entry into the given camera */
    void ReadCamera(XmlNode &node, Collada::Camera &pCamera);

    /** Reads the light library */
    void ReadLightLibrary(XmlNode &node);

    /** Reads a light entry into the given light */
    void ReadLight(XmlNode &node, Collada::Light &pLight);

    /** Reads the effect library */
    void ReadEffectLibrary(XmlNode &node);

    /** Reads an effect entry into the given effect*/
    void ReadEffect(XmlNode &node, Collada::Effect &pEffect);

    /** Reads an COMMON effect profile */
    void ReadEffectProfileCommon(XmlNode &node, Collada::Effect &pEffect);

    /** Read sampler properties */
    void ReadSamplerProperties(XmlNode &node, Collada::Sampler &pSampler);

    /** Reads an effect entry containing a color or a texture defining that color */
    void ReadEffectColor(XmlNode &node, aiColor4D &pColor, Collada::Sampler &pSampler);

    /** Reads an effect entry containing a float */
    void ReadEffectFloat(XmlNode &node, ai_real &pFloat);

    /** Reads an effect parameter specification of any kind */
    void ReadEffectParam(XmlNode &node, Collada::EffectParam &pParam);

    /** Reads the geometry library contents */
    void ReadGeometryLibrary(XmlNode &node);

    /** Reads a geometry from the geometry library. */
    void ReadGeometry(XmlNode &node, Collada::Mesh &pMesh);

    /** Reads a mesh from the geometry library */
    void ReadMesh(XmlNode &node, Collada::Mesh &pMesh);

    /** Reads a source element - a combination of raw data and an accessor defining
         * things that should not be redefinable. Yes, that's another rant.
         */
    void ReadSource(XmlNode &node);

    /** Reads a data array holding a number of elements, and stores it in the global library.
         * Currently supported are array of floats and arrays of strings.
         */
    void ReadDataArray(XmlNode &node);

    /** Reads an accessor and stores it in the global library under the given ID -
         * accessors use the ID of the parent <source> element
         */
    void ReadAccessor(XmlNode &node, const std::string &pID);

    /** Reads input declarations of per-vertex mesh data into the given mesh */
    void ReadVertexData(XmlNode &node, Collada::Mesh &pMesh);

    /** Reads input declarations of per-index mesh data into the given mesh */
    void ReadIndexData(XmlNode &node, Collada::Mesh &pMesh);

    /** Reads a single input channel element and stores it in the given array, if valid */
    void ReadInputChannel(XmlNode &node, std::vector<Collada::InputChannel> &poChannels);

    /** Reads a <p> primitive index list and assembles the mesh data into the given mesh */
    size_t ReadPrimitives(XmlNode &node, Collada::Mesh &pMesh, std::vector<Collada::InputChannel> &pPerIndexChannels,
            size_t pNumPrimitives, const std::vector<size_t> &pVCount, Collada::PrimitiveType pPrimType);

    /** Copies the data for a single primitive into the mesh, based on the InputChannels */
    void CopyVertex(size_t currentVertex, size_t numOffsets, size_t numPoints, size_t perVertexOffset,
            Collada::Mesh &pMesh, std::vector<Collada::InputChannel> &pPerIndexChannels,
            size_t currentPrimitive, const std::vector<size_t> &indices);

    /** Reads one triangle of a tristrip into the mesh */
    void ReadPrimTriStrips(size_t numOffsets, size_t perVertexOffset, Collada::Mesh &pMesh,
            std::vector<Collada::InputChannel> &pPerIndexChannels, size_t currentPrimitive, const std::vector<size_t> &indices);

    /** Extracts a single object from an input channel and stores it in the appropriate mesh data array */
    void ExtractDataObjectFromChannel(const Collada::InputChannel &pInput, size_t pLocalIndex, Collada::Mesh &pMesh);

    /** Reads the library of node hierarchies and scene parts */
    void ReadSceneLibrary(XmlNode &node);

    /** Reads a scene node's contents including children and stores it in the given node */
    void ReadSceneNode(XmlNode &node, Collada::Node *pNode);

    /** Reads a node transformation entry of the given type and adds it to the given node's transformation list. */
    void ReadNodeTransformation(XmlNode &node, Collada::Node *pNode, Collada::TransformType pType);

    /** Reads a mesh reference in a node and adds it to the node's mesh list */
    void ReadNodeGeometry(XmlNode &node, Collada::Node *pNode);

    /** Reads the collada scene */
    void ReadScene(XmlNode &node);

    // Processes bind_vertex_input and bind elements
    void ReadMaterialVertexInputBinding(XmlNode &node, Collada::SemanticMappingTable &tbl);

    /** Reads embedded textures from a ZAE archive*/
    void ReadEmbeddedTextures(ZipArchiveIOSystem &zip_archive);

protected:
    /** Calculates the resulting transformation from all the given transform steps */
    aiMatrix4x4 CalculateResultTransform(const std::vector<Collada::Transform> &pTransforms) const;

    /** Determines the input data type for the given semantic string */
    Collada::InputType GetTypeForSemantic(const std::string &pSemantic);

    /** Finds the item in the given library by its reference, throws if not found */
    template <typename Type>
    const Type &ResolveLibraryReference(const std::map<std::string, Type> &pLibrary, const std::string &pURL) const;

protected:
    // Filename, for a verbose error message
    std::string mFileName;

    // XML reader, member for everyday use
    XmlParser mXmlParser;

    /** All data arrays found in the file by ID. Might be referred to by actually
         everyone. Collada, you are a steaming pile of indirection. */
    using DataLibrary = std::map<std::string, Collada::Data> ;
    DataLibrary mDataLibrary;

    /** Same for accessors which define how the data in a data array is accessed. */
    using AccessorLibrary = std::map<std::string, Collada::Accessor> ;
    AccessorLibrary mAccessorLibrary;

    /** Mesh library: mesh by ID */
    using MeshLibrary = std::map<std::string, Collada::Mesh *>;
    MeshLibrary mMeshLibrary;

    /** node library: root node of the hierarchy part by ID */
    using NodeLibrary = std::map<std::string, Collada::Node *>;
    NodeLibrary mNodeLibrary;

    /** Image library: stores texture properties by ID */
    using ImageLibrary = std::map<std::string, Collada::Image> ;
    ImageLibrary mImageLibrary;

    /** Effect library: surface attributes by ID */
    using EffectLibrary = std::map<std::string, Collada::Effect> ;
    EffectLibrary mEffectLibrary;

    /** Material library: surface material by ID */
    using MaterialLibrary = std::map<std::string, Collada::Material> ;
    MaterialLibrary mMaterialLibrary;

    /** Light library: surface light by ID */
    using LightLibrary = std::map<std::string, Collada::Light> ;
    LightLibrary mLightLibrary;

    /** Camera library: surface material by ID */
    using CameraLibrary = std::map<std::string, Collada::Camera> ;
    CameraLibrary mCameraLibrary;

    /** Controller library: joint controllers by ID */
    using ControllerLibrary = std::map<std::string, Collada::Controller> ;
    ControllerLibrary mControllerLibrary;

    /** Animation library: animation references by ID */
    using AnimationLibrary = std::map<std::string, Collada::Animation *> ;
    AnimationLibrary mAnimationLibrary;

    /** Animation clip library: clip animation references by ID */
    using AnimationClipLibrary = std::vector<std::pair<std::string, std::vector<std::string>>> ;
    AnimationClipLibrary mAnimationClipLibrary;

    /** Pointer to the root node. Don't delete, it just points to one of
         the nodes in the node library. */
    Collada::Node *mRootNode;

    /** Root animation container */
    Collada::Animation mAnims;

    /** Size unit: how large compared to a meter */
    ai_real mUnitSize;

    /** Which is the up vector */
    enum { UP_X,
        UP_Y,
        UP_Z } mUpDirection;

    /** Asset metadata (global for scene) */
    StringMetaData mAssetMetaData;

    /** Collada file format version */
    Collada::FormatVersion mFormat;
};

// ------------------------------------------------------------------------------------------------
// Finds the item in the given library by its reference, throws if not found
template <typename Type>
const Type &ColladaParser::ResolveLibraryReference(const std::map<std::string, Type> &pLibrary, const std::string &pURL) const {
    typename std::map<std::string, Type>::const_iterator it = pLibrary.find(pURL);
    if (it == pLibrary.end()) {
        throw DeadlyImportError("Unable to resolve library reference \"", pURL, "\".");
    }
    return it->second;
}

} // end of namespace Assimp

#endif // AI_COLLADAPARSER_H_INC
