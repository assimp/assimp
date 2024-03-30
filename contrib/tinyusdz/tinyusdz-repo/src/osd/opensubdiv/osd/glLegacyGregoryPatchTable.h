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

#ifndef OPENSUBDIV3_OSD_GL_LEGACY_GREGORY_PATCH_TABLE_H
#define OPENSUBDIV3_OSD_GL_LEGACY_GREGORY_PATCH_TABLE_H

#include "../version.h"

#include "../far/patchTable.h"
#include "../osd/nonCopyable.h"
#include "../osd/opengl.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

class GLLegacyGregoryPatchTable
    : private NonCopyable<GLLegacyGregoryPatchTable> {
public:
    ~GLLegacyGregoryPatchTable();

    static GLLegacyGregoryPatchTable *Create(Far::PatchTable const *patchTable);

    void UpdateVertexBuffer(GLuint vbo);

    GLuint GetVertexTextureBuffer() const {
        return _vertexTextureBuffer;
    }

    GLuint GetVertexValenceTextureBuffer() const {
        return _vertexValenceTextureBuffer;
    }

    GLuint GetQuadOffsetsTextureBuffer() const {
        return _quadOffsetsTextureBuffer;
    }

    GLuint GetQuadOffsetsBase(Far::PatchDescriptor::Type type) {
        if (type == Far::PatchDescriptor::GREGORY_BOUNDARY) {
            return _quadOffsetsBase[1];
        }
        return _quadOffsetsBase[0];
    }

protected:
    GLLegacyGregoryPatchTable();

private:
    GLuint _vertexTextureBuffer;
    GLuint _vertexValenceTextureBuffer;
    GLuint _quadOffsetsTextureBuffer;
    GLuint _quadOffsetsBase[2];       // gregory, boundaryGregory
};



}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv

#endif  // OPENSUBDIV3_OSD_GL_LEGACY_GREGORY_PATCH_TABLE_H
