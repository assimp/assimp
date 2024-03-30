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

#include "../osd/cudaPatchTable.h"

#include <cuda_runtime.h>

#include "../far/patchTable.h"
#include "../osd/cpuPatchTable.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

CudaPatchTable::CudaPatchTable() :
    _patchArrays(NULL), _indexBuffer(NULL), _patchParamBuffer(NULL),
    _varyingPatchArrays(NULL), _varyingIndexBuffer(NULL) {
}

CudaPatchTable::~CudaPatchTable() {
    if (_patchArrays) cudaFree(_patchArrays);
    if (_indexBuffer) cudaFree(_indexBuffer);
    if (_patchParamBuffer) cudaFree(_patchParamBuffer);
    if (_varyingPatchArrays) cudaFree(_varyingPatchArrays);
    if (_varyingIndexBuffer) cudaFree(_varyingIndexBuffer);
    for (int fvc=0; fvc<(int)_fvarPatchArrays.size(); ++fvc) {
        if (_fvarPatchArrays[fvc]) cudaFree(_fvarPatchArrays[fvc]);
    }
    for (int fvc=0; fvc<(int)_fvarIndexBuffers.size(); ++fvc) {
        if (_fvarIndexBuffers[fvc]) cudaFree(_fvarIndexBuffers[fvc]);
    }
    for (int fvc=0; fvc<(int)_fvarParamBuffers.size(); ++fvc) {
        if (_fvarParamBuffers[fvc]) cudaFree(_fvarParamBuffers[fvc]);
    }
}

CudaPatchTable *
CudaPatchTable::Create(Far::PatchTable const *farPatchTable,
                       void * /*deviceContext*/) {
    CudaPatchTable *instance = new CudaPatchTable();
    if (instance->allocate(farPatchTable)) return instance;
    delete instance;
    return 0;
}

bool
CudaPatchTable::allocate(Far::PatchTable const *farPatchTable) {
    CpuPatchTable patchTable(farPatchTable);

    size_t numPatchArrays = patchTable.GetNumPatchArrays();
    size_t indexSize = patchTable.GetPatchIndexSize();
    size_t patchParamSize = patchTable.GetPatchParamSize();

    cudaError_t err;
    err = cudaMalloc(&_patchArrays, numPatchArrays * sizeof(Osd::PatchArray));
    if (err != cudaSuccess) return false;

    err = cudaMalloc(&_indexBuffer, indexSize * sizeof(int));
    if (err != cudaSuccess) return false;

    err = cudaMalloc(&_patchParamBuffer, patchParamSize * sizeof(Osd::PatchParam));
    if (err != cudaSuccess) return false;

    err = cudaMalloc(&_varyingPatchArrays, numPatchArrays * sizeof(Osd::PatchArray));
    if (err != cudaSuccess) return false;

    size_t varyingIndexSize = patchTable.GetVaryingPatchIndexSize();
    err = cudaMalloc(&_varyingIndexBuffer, varyingIndexSize * sizeof(int));
    if (err != cudaSuccess) return false;

    size_t numFVarChannels = patchTable.GetNumFVarChannels();
    _fvarPatchArrays.resize(numFVarChannels, 0);
    _fvarIndexBuffers.resize(numFVarChannels, 0);
    _fvarParamBuffers.resize(numFVarChannels, 0);
    for (int fvc=0; fvc<(int)numFVarChannels; ++fvc) {
        err = cudaMalloc(&_fvarPatchArrays[fvc], numPatchArrays * sizeof(Osd::PatchArray));
        if (err != cudaSuccess) return false;

        err = cudaMemcpy(_fvarPatchArrays[fvc],
                         patchTable.GetFVarPatchArrayBuffer(fvc),
                         numPatchArrays * sizeof(Osd::PatchArray),
                         cudaMemcpyHostToDevice);
        if (err != cudaSuccess) return false;

        size_t fvarIndexSize = patchTable.GetFVarPatchIndexSize(fvc);
        err = cudaMalloc(&_fvarIndexBuffers[fvc], fvarIndexSize * sizeof(int));
        if (err != cudaSuccess) return false;

        err = cudaMemcpy(_fvarIndexBuffers[fvc],
                         patchTable.GetFVarPatchIndexBuffer(fvc),
                         fvarIndexSize * sizeof(int),
                         cudaMemcpyHostToDevice);
        if (err != cudaSuccess) return false;

        size_t fvarParamSize = patchTable.GetFVarPatchParamSize(fvc);
        err = cudaMalloc(&_fvarParamBuffers[fvc], fvarParamSize * sizeof(Osd::PatchParam));
        if (err != cudaSuccess) return false;

        err = cudaMemcpy(_fvarParamBuffers[fvc],
                         patchTable.GetFVarPatchParamBuffer(fvc),
                         patchParamSize * sizeof(PatchParam),
                         cudaMemcpyHostToDevice);
        if (err != cudaSuccess) return false;
    }

    // copy patch array
    err = cudaMemcpy(_patchArrays,
                     patchTable.GetPatchArrayBuffer(),
                     numPatchArrays * sizeof(Osd::PatchArray),
                     cudaMemcpyHostToDevice);
    if (err != cudaSuccess) return false;

    // copy index buffer
    err = cudaMemcpy(_indexBuffer,
                     patchTable.GetPatchIndexBuffer(),
                     indexSize * sizeof(int),
                     cudaMemcpyHostToDevice);
    if (err != cudaSuccess) return false;

    // patch param buffer
    err = cudaMemcpy(_patchParamBuffer,
                     patchTable.GetPatchParamBuffer(),
                     patchParamSize * sizeof(Osd::PatchParam),
                     cudaMemcpyHostToDevice);
    if (err != cudaSuccess) return false;

    // copy varying patch arrays and index buffer
    err = cudaMemcpy(_varyingPatchArrays,
                     patchTable.GetVaryingPatchArrayBuffer(),
                     numPatchArrays * sizeof(Osd::PatchArray),
                     cudaMemcpyHostToDevice);
    if (err != cudaSuccess) return false;
    err = cudaMemcpy(_varyingIndexBuffer,
                     patchTable.GetVaryingPatchIndexBuffer(),
                     varyingIndexSize * sizeof(int),
                     cudaMemcpyHostToDevice);
    if (err != cudaSuccess) return false;

    return true;
}


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv

