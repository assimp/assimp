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

/** @file PbrtExporter.h
* Declares the exporter class to write a scene to a pbrt file
*/
#ifndef AI_PBRTEXPORTER_H_INC
#define AI_PBRTEXPORTER_H_INC

#ifndef ASSIMP_BUILD_NO_PBRT_EXPORTER

#include <assimp/types.h>
#include <assimp/StreamWriter.h>
#include <assimp/Exceptional.h>

#include <map>
#include <set>
#include <string>
#include <sstream>

struct aiScene;
struct aiNode;
struct aiMaterial;
struct aiMesh;

namespace Assimp {

class IOSystem;
class IOStream;
class ExportProperties;

// ---------------------------------------------------------------------
/** Helper class to export a given scene to a Pbrt file. */
// ---------------------------------------------------------------------
class PbrtExporter
{
public:
    /// Constructor for a specific scene to export
    PbrtExporter(const aiScene *pScene, IOSystem *pIOSystem,
            const std::string &path, const std::string &file);

    /// Destructor
    virtual ~PbrtExporter();

private:
    // the scene to export
    const aiScene* mScene;

    /// Stringstream to write all output into
    std::stringstream mOutput;

    /// The IOSystem for output
    IOSystem* mIOSystem;

    /// Path of the directory where the scene will be exported
    const std::string mPath;

    /// Name of the file (without extension) where the scene will be exported
    const std::string mFile;

private:
    //  A private set to keep track of which textures have been declared
    std::set<std::string> mTextureSet;

    // Transform to apply to the root node and all root objects such as cameras, lights, etc.
    aiMatrix4x4 mRootTransform;

    aiMatrix4x4 GetNodeTransform(const aiString& name) const;
    static std::string TransformAsString(const aiMatrix4x4& m);

    static std::string RemoveSuffix(std::string filename);
    std::string CleanTextureFilename(const aiString &f, bool rewriteExtension = true) const;

    void WriteMetaData();

    void WriteWorldDefinition();

    void WriteCameras();
    void WriteCamera(int i);

    void WriteLights();

    void WriteTextures();
    static bool TextureHasAlphaMask(const std::string &filename);

    void WriteMaterials();
    void WriteMaterial(int i);

    void WriteMesh(aiMesh* mesh);

    void WriteInstanceDefinition(int i);
    void WriteGeometricObjects(aiNode* node, aiMatrix4x4 parentTransform,
                               std::map<int, int> &meshUses);
};

} // namespace Assimp

#endif // ASSIMP_BUILD_NO_PBRT_EXPORTER

#endif // AI_PBRTEXPORTER_H_INC
