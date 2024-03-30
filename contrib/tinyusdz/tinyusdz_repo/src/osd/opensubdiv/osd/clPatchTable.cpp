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

#include "../osd/clPatchTable.h"

#include "../far/error.h"
#include "../far/patchTable.h"
#include "../osd/opencl.h"
#include "../osd/cpuPatchTable.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

CLPatchTable::CLPatchTable() :
    _patchArrays(NULL), _indexBuffer(NULL), _patchParamBuffer(NULL) {
}

CLPatchTable::~CLPatchTable() {
    if (_patchArrays) clReleaseMemObject(_patchArrays);
    if (_indexBuffer) clReleaseMemObject(_indexBuffer);
    if (_patchParamBuffer) clReleaseMemObject(_patchParamBuffer);
    if (_varyingPatchArrays) clReleaseMemObject(_varyingPatchArrays);
    if (_varyingIndexBuffer) clReleaseMemObject(_varyingIndexBuffer);
    for (int fvc=0; fvc<(int)_fvarPatchArrays.size(); ++fvc) {
        if (_fvarPatchArrays[fvc]) clReleaseMemObject(_fvarPatchArrays[fvc]);
    }
    for (int fvc=0; fvc<(int)_fvarIndexBuffers.size(); ++fvc) {
        if (_fvarIndexBuffers[fvc]) clReleaseMemObject(_fvarIndexBuffers[fvc]);
    }
    for (int fvc=0; fvc<(int)_fvarParamBuffers.size(); ++fvc) {
        if (_fvarParamBuffers[fvc]) clReleaseMemObject(_fvarParamBuffers[fvc]);
    }
}

CLPatchTable *
CLPatchTable::Create(Far::PatchTable const *farPatchTable,
                     cl_context clContext) {
    CLPatchTable *instance = new CLPatchTable();
    if (instance->allocate(farPatchTable, clContext)) return instance;
    delete instance;
    return 0;
}

bool
CLPatchTable::allocate(Far::PatchTable const *farPatchTable, cl_context clContext) {
    CpuPatchTable patchTable(farPatchTable);

    size_t numPatchArrays = patchTable.GetNumPatchArrays();
    size_t indexSize = patchTable.GetPatchIndexSize();
    size_t patchParamSize = patchTable.GetPatchParamSize();

    cl_int err = 0;
    _patchArrays = clCreateBuffer(clContext,
                                  CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
                                  numPatchArrays * sizeof(Osd::PatchArray),
                                  (void*)patchTable.GetPatchArrayBuffer(),
                                  &err);
    if (err != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "clCreateBuffer: %d", err);
        return false;
    }

    _indexBuffer = clCreateBuffer(clContext,
                                  CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
                                  indexSize * sizeof(int),
                                  (void*)patchTable.GetPatchIndexBuffer(),
                                  &err);
    if (err != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "clCreateBuffer: %d", err);
        return false;
    }

    _patchParamBuffer = clCreateBuffer(clContext,
                                       CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
                                       patchParamSize * sizeof(Osd::PatchParam),
                                       (void*)patchTable.GetPatchParamBuffer(),
                                       &err);
    if (err != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "clCreateBuffer: %d", err);
        return false;
    }

    _varyingPatchArrays = clCreateBuffer(clContext,
                                  CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
                                  numPatchArrays * sizeof(Osd::PatchArray),
                                  (void*)patchTable.GetVaryingPatchArrayBuffer(),
                                  &err);
    if (err != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "clCreateBuffer: %d", err);
        return false;
    }

    _varyingIndexBuffer = clCreateBuffer(clContext,
                                  CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
                                  patchTable.GetVaryingPatchIndexSize() * sizeof(int),
                                  (void*)patchTable.GetVaryingPatchIndexBuffer(),
                                  &err);
    if (err != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "clCreateBuffer: %d", err);
        return false;
    }

    size_t numFVarChannels = patchTable.GetNumFVarChannels();
    _fvarPatchArrays.resize(numFVarChannels, 0);
    _fvarIndexBuffers.resize(numFVarChannels, 0);
    _fvarParamBuffers.resize(numFVarChannels, 0);
    for (int fvc=0; fvc<(int)numFVarChannels; ++fvc) {
        _fvarPatchArrays[fvc] = clCreateBuffer(clContext,
                                  CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
                                  numPatchArrays * sizeof(Osd::PatchArray),
                                  (void*)patchTable.GetFVarPatchArrayBuffer(fvc),
                                  &err);
        if (err != CL_SUCCESS) {
            Far::Error(Far::FAR_RUNTIME_ERROR, "clCreateBuffer: %d", err);
            return false;
        }

        _fvarIndexBuffers[fvc] = clCreateBuffer(clContext,
                                  CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
                                  patchTable.GetFVarPatchIndexSize(fvc) * sizeof(int),
                                  (void*)patchTable.GetFVarPatchIndexBuffer(fvc),
                                  &err);
        if (err != CL_SUCCESS) {
            Far::Error(Far::FAR_RUNTIME_ERROR, "clCreateBuffer: %d", err);
            return false;
        }

        _fvarParamBuffers[fvc] = clCreateBuffer(clContext,
                                   CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
                                   patchTable.GetFVarPatchParamSize(fvc) * sizeof(Osd::PatchParam),
                                   (void*)patchTable.GetFVarPatchParamBuffer(fvc),
                                   &err);
        if (err != CL_SUCCESS) {
            Far::Error(Far::FAR_RUNTIME_ERROR, "clCreateBuffer: %d", err);
            return false;
        }
    }

    return true;
}


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv

