/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team
Copyright (c) 2024-2026, Ahsan Mehmood (OpenSKP)

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

/** @file SkpImporter.h
 *  @brief Declares the importer class to read a scene from a native
 *  SketchUp (.skp) file, using the OpenSKP library
 *  (https://github.com/iamahsanmehmood/openskp, MIT licensed).
 */
#ifndef AI_SKPIMPORTER_H_INC
#define AI_SKPIMPORTER_H_INC

#ifndef ASSIMP_BUILD_NO_SKP_IMPORTER

#include <assimp/BaseImporter.h>

#include <string>
#include <unordered_map>
#include <vector>

struct aiNode;
struct aiMesh;
struct aiMaterial;

namespace openskp {
struct InstancedScene;
struct InstancedNode;
} // namespace openskp

namespace Assimp {

/// Imports SketchUp's native .skp format (both the legacy, pre-2021 MFC
/// CArchive container and the modern 2021+ VFF/ZIP container) by delegating
/// all parsing to OpenSKP, a from-scratch, MIT-licensed, dependency-light
/// C++17 .skp reader that requires no Trimble SDK. This importer's own job
/// is purely the OpenSKP InstancedScene -> aiScene translation: OpenSKP
/// already resolves the file into flattened materials, deduplicated mesh
/// resources (definition geometry, resolved once per distinct
/// material/representation combination the file actually uses) and a scene
/// graph of placed nodes that reference those resources by index - a shape
/// that maps onto aiScene/aiNode/aiMesh close to 1:1, including SketchUp's
/// own component-instancing (multiple aiNodes referencing the same aiMesh
/// indices, exactly like assimp's own mesh-sharing convention).
class SkpImporter : public BaseImporter {
public:
    SkpImporter() = default;
    ~SkpImporter() override = default;

    /// \brief  Returns whether the class can handle the format of the given file.
    /// \remark See BaseImporter::CanRead() for details.
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool checkSig) const override;

protected:
    const aiImporterDesc *GetInfo() const override;

    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) override;

private:
    /// Populates pScene's materials and (if any placed material references a
    /// texture) embedded textures from scene.gltf_materials/scene.textures.
    /// Returns, for each material index, the assimp texture index to
    /// reference (embedded as "*N"), or -1 if that material has none.
    std::vector<int> importMaterialsAndTextures(const openskp::InstancedScene &scene, aiScene *pScene);

    /// Populates pScene's mesh array from every InstancedMeshResource's
    /// LocalPrimitives (one aiMesh per LocalPrimitive - already
    /// single-material, matching assimp's own per-mesh material
    /// convention). Returns, per mesh_resource_id, the list of aiMesh
    /// indices a node referencing that resource should list in mMeshes.
    std::unordered_map<std::string, std::vector<unsigned int>> importMeshes(
            const openskp::InstancedScene &scene, aiScene *pScene, const std::vector<int> &materialTextureIndex);

    /// Recursively converts one OpenSKP InstancedNode (and its subtree) into
    /// a newly allocated aiNode, wiring mMeshes from meshIndicesByResource.
    aiNode *importNode(const openskp::InstancedNode &node,
            const std::unordered_map<std::string, std::vector<unsigned int>> &meshIndicesByResource,
            aiNode *parent);
};

} // namespace Assimp

#endif // ASSIMP_BUILD_NO_SKP_IMPORTER

#endif // AI_SKPIMPORTER_H_INC
