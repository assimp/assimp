//
//   Copyright 2015 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

#ifndef OPENSUBDIV3_OSD_GL_PATCH_TABLE_H
#define OPENSUBDIV3_OSD_GL_PATCH_TABLE_H

#include "../version.h"

#include "../osd/nonCopyable.h"
#include "../osd/opengl.h"
#include "../osd/types.h"

#include <vector>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far{
    class PatchTable;
};

namespace Osd {

class GLPatchTable : private NonCopyable<GLPatchTable> {
public:
    typedef GLuint VertexBufferBinding;

    ~GLPatchTable();

    static GLPatchTable *Create(Far::PatchTable const *farPatchTable,
                                void *deviceContext = NULL);

    /// Returns the patch arrays for vertex index buffer data
    PatchArrayVector const &GetPatchArrays() const {
        return _patchArrays;
    }

    /// Returns the GL index buffer containing the patch control vertices
    GLuint GetPatchIndexBuffer() const {
        return _patchIndexBuffer;
    }

    /// Returns the GL index buffer containing the patch parameter
    GLuint GetPatchParamBuffer() const {
        return _patchParamBuffer;
    }

    /// Returns the GL texture buffer containing the patch control vertices
    GLuint GetPatchIndexTextureBuffer() const {
        return _patchIndexTexture;
    }

    /// Returns the GL texture buffer containing the patch parameter
    GLuint GetPatchParamTextureBuffer() const {
        return _patchParamTexture;
    }

    /// Returns the patch arrays for varying index buffer data
    PatchArrayVector const &GetVaryingPatchArrays() const {
        return _varyingPatchArrays;
    }

    /// Returns the GL index buffer containing the varying control vertices
    GLuint GetVaryingPatchIndexBuffer() const {
        return _varyingIndexBuffer;
    }

    /// Returns the GL texture buffer containing the varying control vertices
    GLuint GetVaryingPatchIndexTextureBuffer() const {
        return _varyingIndexTexture;
    }

    /// Returns the number of face-varying channel buffers
    int GetNumFVarChannels() const { return (int)_fvarPatchArrays.size(); }

    /// Returns the patch arrays for face-varying index buffer data
    PatchArrayVector const &GetFVarPatchArrays(int fvarChannel = 0) const {
        return _fvarPatchArrays[fvarChannel];
    }

    /// Returns the GL index buffer containing face-varying control vertices
    GLuint GetFVarPatchIndexBuffer(int fvarChannel = 0) const {
        return _fvarIndexBuffers[fvarChannel];
    }

    /// Returns the GL texture buffer containing face-varying control vertices
    GLuint GetFVarPatchIndexTextureBuffer(int fvarChannel = 0) const {
        return _fvarIndexTextures[fvarChannel];
    }

    /// Returns the GL index buffer containing face-varying patch params
    GLuint GetFVarPatchParamBuffer(int fvarChannel = 0) const {
        return _fvarParamBuffers[fvarChannel];
    }

    /// Returns the GL texture buffer containing face-varying patch params
    GLuint GetFVarPatchParamTextureBuffer(int fvarChannel = 0) const {
        return _fvarParamTextures[fvarChannel];
    }

protected:
    GLPatchTable();

    // allocate buffers from patchTable
    bool allocate(Far::PatchTable const *farPatchTable);

    PatchArrayVector _patchArrays;

    GLuint _patchIndexBuffer;
    GLuint _patchParamBuffer;

    GLuint _patchIndexTexture;
    GLuint _patchParamTexture;

    PatchArrayVector _varyingPatchArrays;
    GLuint _varyingIndexBuffer;
    GLuint _varyingIndexTexture;

    std::vector<PatchArrayVector> _fvarPatchArrays;
    std::vector<GLuint> _fvarIndexBuffers;
    std::vector<GLuint> _fvarIndexTextures;

    std::vector<GLuint> _fvarParamBuffers;
    std::vector<GLuint> _fvarParamTextures;
};


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv

#endif  // OPENSUBDIV3_OSD_GL_PATCH_TABLE_H
