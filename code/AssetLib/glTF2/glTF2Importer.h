/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team


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
#ifndef AI_GLTF2IMPORTER_H_INC
#define AI_GLTF2IMPORTER_H_INC

#include <assimp/BaseImporter.h>

struct aiNode;

namespace glTF2 {
    class Asset;
}

namespace Assimp {

/**
 * Load the glTF2 format.
 * https://github.com/KhronosGroup/glTF/tree/master/specification
 */
class glTF2Importer : public BaseImporter {
public:
    glTF2Importer();
    ~glTF2Importer() override;
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool checkSig) const override;

protected:
    const aiImporterDesc *GetInfo() const override;
    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) override;
    virtual void SetupProperties(const Importer *pImp) override;

private:
    void ImportEmbeddedTextures(glTF2::Asset &a);
    void ImportMaterials(glTF2::Asset &a);
    void ImportMeshes(glTF2::Asset &a);
    void ImportCameras(glTF2::Asset &a);
    void ImportLights(glTF2::Asset &a);
    void ImportNodes(glTF2::Asset &a);
    void ImportAnimations(glTF2::Asset &a);
    void ImportCommonMetadata(glTF2::Asset &a);

private:
    std::vector<unsigned int> meshOffsets;
    std::vector<int> mEmbeddedTexIdxs;
    aiScene *mScene;

    /// An instance of rapidjson::IRemoteSchemaDocumentProvider
    void *mSchemaDocumentProvider = nullptr;
};

} // namespace Assimp

#endif // AI_GLTF2IMPORTER_H_INC
