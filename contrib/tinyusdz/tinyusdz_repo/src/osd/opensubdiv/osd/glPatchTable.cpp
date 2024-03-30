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

#include "glLoader.h"

#include "../osd/glPatchTable.h"

#include "../far/patchTable.h"
#include "../osd/cpuPatchTable.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

GLPatchTable::GLPatchTable() :
    _patchIndexBuffer(0), _patchParamBuffer(0),
    _patchIndexTexture(0), _patchParamTexture(0) {

    // Initialize internal OpenGL loader library if necessary
    OpenSubdiv::internal::GLLoader::libraryInitializeGL();
}

GLPatchTable::~GLPatchTable() {
    if (_patchIndexBuffer) glDeleteBuffers(1, &_patchIndexBuffer);
    if (_patchParamBuffer) glDeleteBuffers(1, &_patchParamBuffer);
    if (_patchIndexTexture) glDeleteTextures(1, &_patchIndexTexture);
    if (_patchParamTexture) glDeleteTextures(1, &_patchParamTexture);
    if (_varyingIndexBuffer) glDeleteBuffers(1, &_varyingIndexBuffer);
    if (_varyingIndexTexture) glDeleteTextures(1, &_varyingIndexTexture);
    for (int fvc=0; fvc<(int)_fvarIndexBuffers.size(); ++fvc) {
        if (_fvarIndexBuffers[fvc]) glDeleteBuffers(1, &_fvarIndexBuffers[fvc]);
    }
    for (int fvc=0; fvc<(int)_fvarIndexTextures.size(); ++fvc) {
        if (_fvarIndexTextures[fvc]) glDeleteTextures(1, &_fvarIndexTextures[fvc]);
    }
}

GLPatchTable *
GLPatchTable::Create(Far::PatchTable const *farPatchTable,
                     void * /*deviceContext*/) {
    GLPatchTable *instance = new GLPatchTable();
    if (instance->allocate(farPatchTable)) return instance;
    delete instance;
    return 0;
}

bool
GLPatchTable::allocate(Far::PatchTable const *farPatchTable) {
    glGenBuffers(1, &_patchIndexBuffer);
    glGenBuffers(1, &_patchParamBuffer);

    CpuPatchTable patchTable(farPatchTable);

    size_t numPatchArrays = patchTable.GetNumPatchArrays();
    GLsizei indexSize = (GLsizei)patchTable.GetPatchIndexSize();
    GLsizei patchParamSize = (GLsizei)patchTable.GetPatchParamSize();

    // copy patch array
    _patchArrays.assign(patchTable.GetPatchArrayBuffer(),
                        patchTable.GetPatchArrayBuffer() + numPatchArrays);

    // copy index buffer
    glBindBuffer(GL_ARRAY_BUFFER, _patchIndexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 indexSize * sizeof(GLint),
                 patchTable.GetPatchIndexBuffer(),
                 GL_STATIC_DRAW);

    // copy patchparam buffer
    glBindBuffer(GL_ARRAY_BUFFER, _patchParamBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 patchParamSize * sizeof(PatchParam),
                 patchTable.GetPatchParamBuffer(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // make both buffer as texture buffers too.
    glGenTextures(1, &_patchIndexTexture);
    glGenTextures(1, &_patchParamTexture);

    glBindTexture(GL_TEXTURE_BUFFER, _patchIndexTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _patchIndexBuffer);

    glBindTexture(GL_TEXTURE_BUFFER, _patchParamTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32I, _patchParamBuffer);

    // varying
    _varyingPatchArrays.assign(
        patchTable.GetVaryingPatchArrayBuffer(),
        patchTable.GetVaryingPatchArrayBuffer() + numPatchArrays);

    glGenBuffers(1, &_varyingIndexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _varyingIndexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 patchTable.GetVaryingPatchIndexSize() * sizeof(GLint),
                 patchTable.GetVaryingPatchIndexBuffer(),
                 GL_STATIC_DRAW);

    glGenTextures(1, &_varyingIndexTexture);
    glBindTexture(GL_TEXTURE_BUFFER, _varyingIndexTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _varyingIndexBuffer);

    // face-varying
    int numFVarChannels = patchTable.GetNumFVarChannels();
    _fvarPatchArrays.resize(numFVarChannels);
    _fvarIndexBuffers.resize(numFVarChannels);
    _fvarIndexTextures.resize(numFVarChannels);
    _fvarParamBuffers.resize(numFVarChannels);
    _fvarParamTextures.resize(numFVarChannels);
    for (int fvc=0; fvc<numFVarChannels; ++fvc) {
        _fvarPatchArrays[fvc].assign(
            patchTable.GetFVarPatchArrayBuffer(fvc),
            patchTable.GetFVarPatchArrayBuffer(fvc) + numPatchArrays);

        glGenBuffers(1, &_fvarIndexBuffers[fvc]);
        glBindBuffer(GL_ARRAY_BUFFER, _fvarIndexBuffers[fvc]);
        glBufferData(GL_ARRAY_BUFFER,
                     patchTable.GetFVarPatchIndexSize(fvc) * sizeof(GLint),
                     patchTable.GetFVarPatchIndexBuffer(fvc),
                 GL_STATIC_DRAW);

        glGenTextures(1, &_fvarIndexTextures[fvc]);
        glBindTexture(GL_TEXTURE_BUFFER, _fvarIndexTextures[fvc]);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _fvarIndexBuffers[fvc]);

        glGenBuffers(1, &_fvarParamBuffers[fvc]);
        glBindBuffer(GL_ARRAY_BUFFER, _fvarParamBuffers[fvc]);
        glBufferData(GL_ARRAY_BUFFER,
                     patchTable.GetFVarPatchParamSize(fvc) * sizeof(PatchParam),
                     patchTable.GetFVarPatchParamBuffer(fvc),
                 GL_STATIC_DRAW);

        glGenTextures(1, &_fvarParamTextures[fvc]);
        glBindTexture(GL_TEXTURE_BUFFER, _fvarParamTextures[fvc]);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32I, _fvarParamBuffers[fvc]);
        glBindTexture(GL_TEXTURE_BUFFER, 0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    return true;
}


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv

