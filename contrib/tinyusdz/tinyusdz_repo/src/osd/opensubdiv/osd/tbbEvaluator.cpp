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

#include "../osd/tbbEvaluator.h"
#include "../osd/tbbKernel.h"

#include <tbb/task_scheduler_init.h>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

/* static */
bool
TbbEvaluator::EvalStencils(
    const float *src, BufferDescriptor const &srcDesc,
    float *dst,       BufferDescriptor const &dstDesc,
    const int * sizes,
    const int * offsets,
    const int * indices,
    const float * weights,
    int start, int end) {

    if (end <= start) return true;

    TbbEvalStencils(src, srcDesc, dst, dstDesc,
                    sizes, offsets, indices, weights, start, end);

    return true;
}

/* static */
bool
TbbEvaluator::EvalStencils(
    const float *src, BufferDescriptor const &srcDesc,
    float *dst,       BufferDescriptor const &dstDesc,
    float *du,        BufferDescriptor const &duDesc,
    float *dv,        BufferDescriptor const &dvDesc,
    const int * sizes,
    const int * offsets,
    const int * indices,
    const float * weights,
    const float * duWeights,
    const float * dvWeights,
    int start, int end) {

    if (end <= start) return true;
    if (srcDesc.length != dstDesc.length) return false;
    if (srcDesc.length != duDesc.length) return false;
    if (srcDesc.length != dvDesc.length) return false;

    TbbEvalStencils(src, srcDesc,
                    dst, dstDesc,
                    du,  duDesc,
                    dv,  dvDesc,
                    NULL, BufferDescriptor(),
                    NULL, BufferDescriptor(),
                    NULL, BufferDescriptor(),
                    sizes, offsets, indices,
                    weights, duWeights, dvWeights, NULL, NULL, NULL,
                    start, end);

    return true;
}

/* static */
bool
TbbEvaluator::EvalStencils(
    const float *src, BufferDescriptor const &srcDesc,
    float *dst,       BufferDescriptor const &dstDesc,
    float *du,        BufferDescriptor const &duDesc,
    float *dv,        BufferDescriptor const &dvDesc,
    float *duu,       BufferDescriptor const &duuDesc,
    float *duv,       BufferDescriptor const &duvDesc,
    float *dvv,       BufferDescriptor const &dvvDesc,
    const int * sizes,
    const int * offsets,
    const int * indices,
    const float * weights,
    const float * duWeights,
    const float * dvWeights,
    const float * duuWeights,
    const float * duvWeights,
    const float * dvvWeights,
    int start, int end) {

    if (end <= start) return true;
    if (srcDesc.length != dstDesc.length) return false;
    if (srcDesc.length != duDesc.length) return false;
    if (srcDesc.length != dvDesc.length) return false;
    if (srcDesc.length != duuDesc.length) return false;
    if (srcDesc.length != duvDesc.length) return false;
    if (srcDesc.length != dvvDesc.length) return false;

    TbbEvalStencils(src, srcDesc,
                    dst, dstDesc,
                    du,  duDesc,
                    dv,  dvDesc,
                    duu, duuDesc,
                    duv, duvDesc,
                    dvv, dvvDesc,
                    sizes, offsets, indices,
                    weights, duWeights, dvWeights,
                    duuWeights, duvWeights, dvvWeights,
                    start, end);

    return true;
}

/* static */
bool
TbbEvaluator::EvalPatches(
    const float *src, BufferDescriptor const &srcDesc,
    float *dst,       BufferDescriptor const &dstDesc,
    int numPatchCoords,
    const PatchCoord *patchCoords,
    const PatchArray *patchArrayBuffer,
    const int *patchIndexBuffer,
    const PatchParam *patchParamBuffer) {

    if (srcDesc.length != dstDesc.length) return false;

    TbbEvalPatches(src, srcDesc, dst, dstDesc,
                   NULL, BufferDescriptor(),
                   NULL, BufferDescriptor(),
                   NULL, BufferDescriptor(),
                   NULL, BufferDescriptor(),
                   NULL, BufferDescriptor(),
                   numPatchCoords, patchCoords,
                   patchArrayBuffer, patchIndexBuffer, patchParamBuffer);

    return true;
}

/* static */
bool
TbbEvaluator::EvalPatches(
    const float *src, BufferDescriptor const &srcDesc,
    float *dst,       BufferDescriptor const &dstDesc,
    float *du,        BufferDescriptor const &duDesc,
    float *dv,        BufferDescriptor const &dvDesc,
    int numPatchCoords,
    const PatchCoord *patchCoords,
    const PatchArray *patchArrayBuffer,
    const int *patchIndexBuffer,
    const PatchParam *patchParamBuffer) {

    if (srcDesc.length != dstDesc.length) return false;

    TbbEvalPatches(src, srcDesc, dst, dstDesc,
                   du,  duDesc,  dv,  dvDesc,
                   NULL, BufferDescriptor(),
                   NULL, BufferDescriptor(),
                   NULL, BufferDescriptor(),
                   numPatchCoords, patchCoords,
                   patchArrayBuffer, patchIndexBuffer, patchParamBuffer);

    return true;
}

/* static */
bool
TbbEvaluator::EvalPatches(
    const float *src, BufferDescriptor const &srcDesc,
    float *dst,       BufferDescriptor const &dstDesc,
    float *du,        BufferDescriptor const &duDesc,
    float *dv,        BufferDescriptor const &dvDesc,
    float *duu,       BufferDescriptor const &duuDesc,
    float *duv,       BufferDescriptor const &duvDesc,
    float *dvv,       BufferDescriptor const &dvvDesc,
    int numPatchCoords,
    const PatchCoord *patchCoords,
    const PatchArray *patchArrayBuffer,
    const int *patchIndexBuffer,
    const PatchParam *patchParamBuffer) {

    if (srcDesc.length != dstDesc.length) return false;

    TbbEvalPatches(src, srcDesc, dst, dstDesc,
                   du,  duDesc,  dv,  dvDesc,
                   duu, duuDesc, duv, duvDesc, dvv, dvvDesc,
                   numPatchCoords, patchCoords,
                   patchArrayBuffer, patchIndexBuffer, patchParamBuffer);

    return true;
}

/* static */
void
TbbEvaluator::Synchronize(void *) {
}

/* static */
void
TbbEvaluator::SetNumThreads(int numThreads) {
    if (numThreads == -1) {
        tbb::task_scheduler_init init;
    } else {
        tbb::task_scheduler_init init(numThreads);
    }
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv
