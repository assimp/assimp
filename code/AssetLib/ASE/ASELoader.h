/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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

/** @file  ASELoader.h
 *  @brief Definition of the .ASE importer class.
 */
#ifndef AI_ASELOADER_H_INCLUDED
#define AI_ASELOADER_H_INCLUDED

#include <assimp/BaseImporter.h>
#include <assimp/types.h>
#include "ASEParser.h"

struct aiNode;

namespace Assimp {

#ifndef ASSIMP_BUILD_NO_3DS_IMPORTER

// --------------------------------------------------------------------------------
/// @class  ASEImporter
/// @brief  Importer class for the 3DS ASE ASCII format.
// --------------------------------------------------------------------------------
class ASEImporter final : public BaseImporter {
public:
    /// @brief Constructor
    ASEImporter() = default;

    /// @brief Destructor
    ~ASEImporter() override = default;

    bool CanRead(const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const override;

protected:
    const aiImporterDesc* GetInfo () const override;
    void InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler) override;
    void SetupProperties(const Importer* pImp) override;

private:
     /// @brief Generate normal vectors basing on smoothing groups
     /// (in some cases the normal are already contained in the file)
     /// @param [in] mesh Mesh to work on
     /// @return false if the normals have been recomputed
    bool GenerateNormals(ASE::Mesh& mesh);

    /// @brief Create valid vertex/normal/UV/color/face lists.
    /// All elements are unique, faces have only one set of indices after this step occurs.
    /// @param [in] mesh Mesh to work on
    void BuildUniqueRepresentation(ASE::Mesh& mesh);

    /// @brief Creates one-material-per-mesh meshes ;-)
    /// @param [in] mesh  Mesh to work with
    /// @param [in] avOut Receives the list of all created meshes
    void ConvertMeshes(ASE::Mesh& mesh, std::vector<aiMesh*>& avOut);

    /// @brief Converts a material to an aiMaterial object
    /// @param [in] mat Input material
    void ConvertMaterial(ASE::Material& mat);

    /// @brief  Sets up the final material indices for each mesh
    void BuildMaterialIndices();

    /// @brief Builds the node graph.
    void BuildNodes(std::vector<ASE::BaseNode*>& nodes);

    /// @brief Builds all output cameras
    void BuildCameras();

    /// @brief Builds output lights.
    void BuildLights();

    /// @brief Builds output animations.
    /// @param [in] nodes Input nodes
    void BuildAnimations(const std::vector<ASE::BaseNode*>& nodes);


    /// @brief Adds sub nodes to a node
    /// @param [in] nodes Input nodes
    /// @param [in] pcParent parent node to be filled
    /// @param [in] name Name of the parent node
    void AddNodes(const std::vector<ASE::BaseNode*>& nodes,
        aiNode* pcParent, const std::string &name);

    /// @brief Adds sub nodes to a node
    /// @param [in] nodes Input nodes
    /// @param [in] pcParent parent node to be filled
    /// @param [in] name Name of the parent node
    /// @param [in] matrix Transformation matrix to apply.
    void AddNodes(const std::vector<ASE::BaseNode*>& nodes,
        aiNode* pcParent, const std::string &name,
        const aiMatrix4x4& matrix);

    /// @brief Adds meshes to a given node.
    /// @param [in] snode Input node
    /// @param [in] node Parent node to be filled
    void AddMeshes(const ASE::BaseNode* snode, aiNode* node);

    /// @brief Generates a default material and add it to the parser's list
    ///        Called if no material has been found in the file (rare for ASE, but not impossible)
    void GenerateDefaultMaterial();

protected:
    ASE::Parser* mParser{nullptr};  ///< Parser instance
    char* mBuffer{nullptr};         ///< Buffer to hold the loaded file
    aiScene* pcScene{nullptr};      ///< Scene to be filled

    /// Config options: Recompute the normals in every case - WA for 3DS Max broken ASE normal export
    bool mConfigRecomputeNormals{false};
    bool mNoSkeletonMesh{false};
};

#endif // ASSIMP_BUILD_NO_3DS_IMPORTER

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC
