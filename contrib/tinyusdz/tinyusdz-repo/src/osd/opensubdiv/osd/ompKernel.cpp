//
//   Copyright 2013 Pixar
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

#include "../osd/ompKernel.h"
#include "../osd/bufferDescriptor.h"

#include <cassert>
#include <cstdlib>
#include <omp.h>
#include <vector>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

template <class T> T *
elementAtIndex(T * src, int index, BufferDescriptor const &desc) {

    return src + index * desc.stride;
}

static inline void
clear(float *dst, BufferDescriptor const &desc) {

    assert(dst);
    memset(dst, 0, desc.length*sizeof(float));
}

static inline void
addWithWeight(float *dst, const float *src, int srcIndex, float weight,
              BufferDescriptor const &desc) {

    assert(src && dst);
    src = elementAtIndex(src, srcIndex, desc);
    for (int k = 0; k < desc.length; ++k) {
        dst[k] += src[k] * weight;
    }
}

static inline void
copy(float *dst, int dstIndex, const float *src,
     BufferDescriptor const &desc) {

    assert(src && dst);

    dst = elementAtIndex(dst, dstIndex, desc);
    memcpy(dst, src, desc.length*sizeof(float));
}


// XXXX manuelk this should be optimized further by using SIMD - considering
//              OMP is somewhat obsolete - this is probably not worth it.
void
OmpEvalStencils(float const * src, BufferDescriptor const &srcDesc,
                float * dst,       BufferDescriptor const &dstDesc,
                int const * sizes,
                int const * offsets,
                int const * indices,
                float const * weights,
                int start, int end) {
    start = (start > 0 ? start : 0);
    
    src += srcDesc.offset;
    dst += dstDesc.offset;

    int numThreads = omp_get_max_threads();
    int n = end - start;

    float * result = (float*)alloca(srcDesc.length * numThreads * sizeof(float));

#pragma omp parallel for
    for (int i = 0; i < n; ++i) {

        int index = i + start; // Stencil index

        // Get thread-local pointers
        int const           * threadIndices = indices + offsets[index];
        float const         * threadWeights = weights + offsets[index];

        int threadId = omp_get_thread_num();

        float * threadResult = result + threadId*srcDesc.length;

        clear(threadResult, dstDesc);

        for (int j=0; j<(int)sizes[index]; ++j) {
            addWithWeight(threadResult, src,
                threadIndices[j], threadWeights[j], srcDesc);
        }

        copy(dst, i, threadResult, dstDesc);
    }
}

void
OmpEvalStencils(float const * src, BufferDescriptor const &srcDesc,
                float * dst,       BufferDescriptor const &dstDesc,
                float * dstDu,     BufferDescriptor const &dstDuDesc,
                float * dstDv,     BufferDescriptor const &dstDvDesc,
                int const * sizes,
                int const * offsets,
                int const * indices,
                float const * weights,
                float const * duWeights,
                float const * dvWeights,
                int start, int end) {
    start = (start > 0 ? start : 0);

    src += srcDesc.offset;
    dst += dstDesc.offset;
    dstDu += dstDuDesc.offset;
    dstDv += dstDvDesc.offset;

    int numThreads = omp_get_max_threads();
    int n = end - start;

    float * result = (float*)alloca(srcDesc.length * numThreads * sizeof(float));
    float * resultDu = (float*)alloca(srcDesc.length * numThreads * sizeof(float));
    float * resultDv = (float*)alloca(srcDesc.length * numThreads * sizeof(float));

#pragma omp parallel for
    for (int i = 0; i < n; ++i) {

        int index = i + start; // Stencil index

        // Get thread-local pointers
        int const           * threadIndices = indices + offsets[index];
        float const         * threadWeights = weights + offsets[index];
        float const         * threadWeightsDu = duWeights + offsets[index];
        float const         * threadWeightsDv = dvWeights + offsets[index];

        int threadId = omp_get_thread_num();

        float * threadResult = result + threadId*srcDesc.length;
        float * threadResultDu = resultDu + threadId*srcDesc.length;
        float * threadResultDv = resultDv + threadId*srcDesc.length;

        clear(threadResult, dstDesc);
        clear(threadResultDu, dstDuDesc);
        clear(threadResultDv, dstDvDesc);

        for (int j=0; j<(int)sizes[index]; ++j) {
            addWithWeight(threadResult, src,
                threadIndices[j], threadWeights[j], srcDesc);
            addWithWeight(threadResultDu, src,
                threadIndices[j], threadWeightsDu[j], srcDesc);
            addWithWeight(threadResultDv, src,
                threadIndices[j], threadWeightsDv[j], srcDesc);
        }

        copy(dst, i, threadResult, dstDesc);
        copy(dstDu, i, threadResultDu, dstDuDesc);
        copy(dstDv, i, threadResultDv, dstDvDesc);
    }

}

void
OmpEvalStencils(float const * src, BufferDescriptor const &srcDesc,
                float * dst,       BufferDescriptor const &dstDesc,
                float * dstDu,     BufferDescriptor const &dstDuDesc,
                float * dstDv,     BufferDescriptor const &dstDvDesc,
                float * dstDuu,    BufferDescriptor const &dstDuuDesc,
                float * dstDuv,    BufferDescriptor const &dstDuvDesc,
                float * dstDvv,    BufferDescriptor const &dstDvvDesc,
                int const * sizes,
                int const * offsets,
                int const * indices,
                float const * weights,
                float const * duWeights,
                float const * dvWeights,
                float const * duuWeights,
                float const * duvWeights,
                float const * dvvWeights,
                int start, int end) {
    start = (start > 0 ? start : 0);

    src += srcDesc.offset;
    dst += dstDesc.offset;
    dstDu += dstDuDesc.offset;
    dstDv += dstDvDesc.offset;
    dstDuu += dstDuuDesc.offset;
    dstDuv += dstDuvDesc.offset;
    dstDvv += dstDvvDesc.offset;

    int numThreads = omp_get_max_threads();
    int n = end - start;

    float * result = (float*)alloca(srcDesc.length * numThreads * sizeof(float));
    float * resultDu = (float*)alloca(srcDesc.length * numThreads * sizeof(float));
    float * resultDv = (float*)alloca(srcDesc.length * numThreads * sizeof(float));
    float * resultDuu = (float*)alloca(srcDesc.length * numThreads * sizeof(float));
    float * resultDuv = (float*)alloca(srcDesc.length * numThreads * sizeof(float));
    float * resultDvv = (float*)alloca(srcDesc.length * numThreads * sizeof(float));

#pragma omp parallel for
    for (int i = 0; i < n; ++i) {

        int index = i + start; // Stencil index

        // Get thread-local pointers
        int const           * threadIndices = indices + offsets[index];
        float const         * threadWeights = weights + offsets[index];
        float const         * threadWeightsDu = duWeights + offsets[index];
        float const         * threadWeightsDv = dvWeights + offsets[index];
        float const         * threadWeightsDuu = duuWeights + offsets[index];
        float const         * threadWeightsDuv = duvWeights + offsets[index];
        float const         * threadWeightsDvv = dvvWeights + offsets[index];

        int threadId = omp_get_thread_num();

        float * threadResult = result + threadId*srcDesc.length;
        float * threadResultDu = resultDu + threadId*srcDesc.length;
        float * threadResultDv = resultDv + threadId*srcDesc.length;
        float * threadResultDuu = resultDuu + threadId*srcDesc.length;
        float * threadResultDuv = resultDuv + threadId*srcDesc.length;
        float * threadResultDvv = resultDvv + threadId*srcDesc.length;

        clear(threadResult, dstDesc);
        clear(threadResultDu, dstDuDesc);
        clear(threadResultDv, dstDvDesc);
        clear(threadResultDuu, dstDuuDesc);
        clear(threadResultDuv, dstDuvDesc);
        clear(threadResultDvv, dstDvvDesc);

        for (int j=0; j<(int)sizes[index]; ++j) {
            addWithWeight(threadResult, src,
                threadIndices[j], threadWeights[j], srcDesc);
            addWithWeight(threadResultDu, src,
                threadIndices[j], threadWeightsDu[j], srcDesc);
            addWithWeight(threadResultDv, src,
                threadIndices[j], threadWeightsDv[j], srcDesc);
            addWithWeight(threadResultDuu, src,
                threadIndices[j], threadWeightsDuu[j], srcDesc);
            addWithWeight(threadResultDuv, src,
                threadIndices[j], threadWeightsDuv[j], srcDesc);
            addWithWeight(threadResultDvv, src,
                threadIndices[j], threadWeightsDvv[j], srcDesc);
        }

        copy(dst, i, threadResult, dstDesc);
        copy(dstDu, i, threadResultDu, dstDuDesc);
        copy(dstDv, i, threadResultDv, dstDvDesc);
        copy(dstDuu, i, threadResultDuu, dstDuuDesc);
        copy(dstDuv, i, threadResultDuv, dstDuvDesc);
        copy(dstDvv, i, threadResultDvv, dstDvvDesc);
    }

}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv
