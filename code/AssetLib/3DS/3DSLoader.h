
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

/** @file  3DSLoader.h
 *  @brief 3DS File format loader
 */
#ifndef AI_3DSIMPORTER_H_INC
#define AI_3DSIMPORTER_H_INC

#include <assimp/BaseImporter.h>
#include <assimp/types.h>

#ifndef ASSIMP_BUILD_NO_3DS_IMPORTER

#include "3DSHelper.h"
#include <assimp/StreamReader.h>

struct aiNode;

namespace Assimp {

using namespace D3DS;

// ---------------------------------------------------------------------------------
/** Importer class for 3D Studio r3 and r4 3DS files
 */
class Discreet3DSImporter : public BaseImporter {
public:
    Discreet3DSImporter();
    ~Discreet3DSImporter();

public:
    // -------------------------------------------------------------------
    /** Returns whether the class can handle the format of the given file.
     * See BaseImporter::CanRead() for details.
     */
    bool CanRead(const std::string &p_File, IOSystem *p_IOHandler,
            bool p_checkSig) const;

    // -------------------------------------------------------------------
    /** Called prior to ReadFile().
     * The function is a request to the importer to update its configuration
     * basing on the Importer's configuration property list.
     */
    void SetupProperties(const Importer *p_Importer);

protected:
    // -------------------------------------------------------------------
    /** Return importer meta information.
     * See #BaseImporter::GetInfo for the details
     */
    const aiImporterDesc *GetInfo() const;

    // -------------------------------------------------------------------
    /** Imports the given file into the given scene structure.
     * See BaseImporter::InternReadFile() for details
     */
    void InternReadFile(const std::string &p_File, aiScene *p_Scene,
            IOSystem *p_IOHandler);

    // -------------------------------------------------------------------
    /** Converts a temporary material to the outer representation
     */
    void ConvertMaterial(D3DS::Material &p_cMat,
            aiMaterial &p_pcOut);

    // -------------------------------------------------------------------
    /** Read a chunk
     *
     *  @param p_cOut Receives the current chunk
     */
    void ReadChunk(Discreet3DS::Chunk &p_cOut, StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a percentage chunk. mCurrent will point to the next
    * chunk behind afterwards. If no percentage chunk is found
    * QNAN is returned.
    */
    ai_real ParsePercentageChunk(StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a color chunk. mCurrent will point to the next
    * chunk behind afterwards. If no color chunk is found
    * QNAN is returned in all members.
    */
    void ParseColorChunk(aiColor3D *p_cOut,
            StreamReaderLE *p_stream,
            bool p_acceptPercent = true);

    // -------------------------------------------------------------------
    /** Skip a chunk in the file
    */
    void SkipChunk(StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Generate the nodegraph
    */
    void GenerateNodeGraph(aiScene *p_cOut, D3DS::Node *p_rootNode);

    // -------------------------------------------------------------------
    /** Parse a main top-level chunk in the file
    */
    void ParseMainChunk(D3DS::Node *p_rootNode, D3DS::Node *p_currentNode,
            StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a top-level chunk in the file
    */
    void ParseChunk(const char *name, unsigned int num, StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a top-level editor chunk in the file
    */
    void ParseEditorChunk(D3DS::Node *p_rootNode, D3DS::Node *p_currentNode,
            StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a top-level object chunk in the file
    */
    void ParseObjectChunk(StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a material chunk in the file
    */
    void ParseMaterialChunk(StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a mesh chunk in the file
    */
    void ParseMeshChunk(StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a light chunk in the file
    */
    void ParseLightChunk(StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a camera chunk in the file
    */
    void ParseCameraChunk(StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a face list chunk in the file
    */
    void ParseFaceChunk(StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a keyframe chunk in the file
    */
    void ParseKeyframeChunk(D3DS::Node *p_rootNode, D3DS::Node *p_currentNode,
            StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a hierarchy chunk in the file
    */
    void ParseHierarchyChunk(uint16_t parent, D3DS::Node *p_rootNode, D3DS::Node *p_currentNode,
            StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Parse a texture chunk in the file
    */
    void ParseTextureChunk(D3DS::Texture *p_cOut, StreamReaderLE *p_stream);

    // -------------------------------------------------------------------
    /** Convert the meshes in the file
    */
    void ConvertMeshes(aiScene *p_cOut);

    // -------------------------------------------------------------------
    /** Replace the default material in the scene
    */
    void ReplaceDefaultMaterial();

    // -------------------------------------------------------------------
    /** Convert the whole scene
    */
    void ConvertScene(aiScene *p_cOut);

    // -------------------------------------------------------------------
    /** generate unique vertices for a mesh
    */
    void MakeUnique(D3DS::Mesh &sMesh);

    // -------------------------------------------------------------------
    /** Add a node to the node graph
    */
    void AddNodeToGraph(aiScene *pcSOut, aiNode *p_cOut, D3DS::Node *pcIn,
            aiMatrix4x4 &absTrafo);

    // -------------------------------------------------------------------
    /** Search for a node in the graph.
    * Called recursively
    */
    void InverseNodeSearch(D3DS::Node *pcNode, D3DS::Node *p_rootNode, D3DS::Node *pcCurrent);

    // -------------------------------------------------------------------
    /** Apply the master scaling factor to the mesh
    */
    void ApplyMasterScale(aiScene *p_Scene);

    // -------------------------------------------------------------------
    /** Clamp all indices in the file to a valid range
    */
    void CheckIndices(D3DS::Mesh &sMesh);

    // -------------------------------------------------------------------
    /** Skip the TCB info in a track key
    */
    void SkipTCBInfo(StreamReaderLE *p_stream);

protected:
    /** Stream to read from */
    // StreamReaderLE *stream;

    /** Last touched node index */
    short mLastNodeIndex;

    /** Scene under construction */
    D3DS::Scene *mScene;

    /** Ambient base color of the scene */
    aiColor3D mClrAmbient;

    /** Master scaling factor of the scene */
    ai_real mMasterScale;

    /** Path to the background image of the scene */
    std::string mBackgroundImage;
    bool bHasBG;

    /** true if PRJ file */
    bool bIsPrj;
};

} // end of namespace Assimp

#endif // !! ASSIMP_BUILD_NO_3DS_IMPORTER

#endif // AI_3DSIMPORTER_H_INC
