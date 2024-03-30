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

#include "../osd/cpuKernel.h"
#include "../osd/bufferDescriptor.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
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
copy(float *dst, int dstIndex, const float *src, BufferDescriptor const &desc) {

    assert(src && dst);

    dst = elementAtIndex(dst, dstIndex, desc);
    memcpy(dst, src, desc.length*sizeof(float));
}

void
CpuEvalStencils(float const * src, BufferDescriptor const &srcDesc,
                float * dst,       BufferDescriptor const &dstDesc,
                int const * sizes,
                int const * offsets,
                int const * indices,
                float const * weights,
                int start, int end) {

    assert(start>=0 && start<end);

    if (start>0) {
        sizes += start;
        indices += offsets[start];
        weights += offsets[start];
    }

    src += srcDesc.offset;
    dst += dstDesc.offset;

    if (srcDesc.length == 4 && dstDesc.length == 4 &&
        srcDesc.stride == 4 && dstDesc.stride == 4) {

        // SIMD fast path for aligned primvar data (4 floats)
        ComputeStencilKernel<4>(src, dst,
            sizes, indices, weights, start,  end);

    } else if (srcDesc.length == 8 && dstDesc.length == 8 &&
               srcDesc.stride == 8 && dstDesc.stride == 8) {

        // SIMD fast path for aligned primvar data (8 floats)
        ComputeStencilKernel<8>(src, dst,
            sizes, indices, weights, start,  end);
    } else {

        // Slow path for non-aligned data

        float * result = (float*)alloca(srcDesc.length * sizeof(float));

        int nstencils = end-start;
        for (int i=0; i<nstencils; ++i, ++sizes) {

            clear(result, srcDesc);

            for (int j=0; j<*sizes; ++j) {
                addWithWeight(result, src, *indices++, *weights++, srcDesc);
            }

            copy(dst, i, result, dstDesc);
        }
    }
}

void
CpuEvalStencils(float const * src, BufferDescriptor const &srcDesc,
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
    if (start > 0) {
        sizes += start;
        indices += offsets[start];
        weights += offsets[start];
        duWeights += offsets[start];
        dvWeights += offsets[start];
    }

    src += srcDesc.offset;
    dst += dstDesc.offset;
    dstDu += dstDuDesc.offset;
    dstDv += dstDvDesc.offset;

    int nOutLength = dstDesc.length + dstDuDesc.length + dstDvDesc.length;
    float * result   = (float*)alloca(nOutLength * sizeof(float));
    float * resultDu = result + dstDesc.length;
    float * resultDv = resultDu + dstDuDesc.length;

    int nStencils = end - start;
    for (int i = 0; i < nStencils; ++i, ++sizes) {

        // clear
        memset(result, 0, nOutLength * sizeof(float));

        for (int j=0; j<*sizes; ++j) {
            addWithWeight(result,   src, *indices, *weights++,   srcDesc);
            addWithWeight(resultDu, src, *indices, *duWeights++, srcDesc);
            addWithWeight(resultDv, src, *indices, *dvWeights++, srcDesc);
            ++indices;
        }
        copy(dst,   i, result, dstDesc);
        copy(dstDu, i, resultDu, dstDuDesc);
        copy(dstDv, i, resultDv, dstDvDesc);
    }
}

void
CpuEvalStencils(float const * src, BufferDescriptor const &srcDesc,
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
    if (start > 0) {
        sizes += start;
        indices += offsets[start];
        weights += offsets[start];
        duWeights += offsets[start];
        dvWeights += offsets[start];
        duuWeights += offsets[start];
        duvWeights += offsets[start];
        dvvWeights += offsets[start];
    }

    src += srcDesc.offset;
    dst += dstDesc.offset;
    dstDu += dstDuDesc.offset;
    dstDv += dstDvDesc.offset;
    dstDuu += dstDuuDesc.offset;
    dstDuv += dstDuvDesc.offset;
    dstDvv += dstDvvDesc.offset;

    int nOutLength = dstDesc.length + dstDuDesc.length + dstDvDesc.length
                   + dstDuuDesc.length + dstDuvDesc.length + dstDvvDesc.length;
    float * result   = (float*)alloca(nOutLength * sizeof(float));
    float * resultDu = result + dstDesc.length;
    float * resultDv = resultDu + dstDuDesc.length;
    float * resultDuu = resultDv + dstDvDesc.length;
    float * resultDuv = resultDuu + dstDuuDesc.length;
    float * resultDvv = resultDuv + dstDuvDesc.length;

    int nStencils = end - start;
    for (int i = 0; i < nStencils; ++i, ++sizes) {

        // clear
        memset(result, 0, nOutLength * sizeof(float));

        for (int j=0; j<*sizes; ++j) {
            addWithWeight(result,   src, *indices, *weights++,   srcDesc);
            addWithWeight(resultDu, src, *indices, *duWeights++, srcDesc);
            addWithWeight(resultDv, src, *indices, *dvWeights++, srcDesc);
            addWithWeight(resultDuu, src, *indices, *duuWeights++, srcDesc);
            addWithWeight(resultDuv, src, *indices, *duvWeights++, srcDesc);
            addWithWeight(resultDvv, src, *indices, *dvvWeights++, srcDesc);
            ++indices;
        }
        copy(dst,   i, result, dstDesc);
        copy(dstDu, i, resultDu, dstDuDesc);
        copy(dstDv, i, resultDv, dstDvDesc);
        copy(dstDuu, i, resultDuu, dstDuuDesc);
        copy(dstDuv, i, resultDuv, dstDuvDesc);
        copy(dstDvv, i, resultDvv, dstDvvDesc);
    }
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv
