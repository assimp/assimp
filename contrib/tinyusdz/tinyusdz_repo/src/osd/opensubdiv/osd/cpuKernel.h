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

#ifndef OPENSUBDIV3_OSD_CPU_KERNEL_H
#define OPENSUBDIV3_OSD_CPU_KERNEL_H

#include "../version.h"
#include <cstring>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

struct BufferDescriptor;

void
CpuEvalStencils(float const * src, BufferDescriptor const &srcDesc,
                float * dst,       BufferDescriptor const &dstDesc,
                int const * sizes,
                int const * offsets,
                int const * indices,
                float const * weights,
                int start, int end);

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
                int start, int end);

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
                int start, int end);

//
// SIMD ICC optimization of the stencil kernel
//

#if defined ( __INTEL_COMPILER ) || defined ( __ICC )
    #define __ALIGN_DATA __declspec(align(32))
#else
    #define __ALIGN_DATA
#endif

// Note : this function is re-used in the TBB Compute kernel
template <int numElems> void
ComputeStencilKernel(float const * vertexSrc,
                     float * vertexDst,
                     int const * sizes,
                     int const * indices,
                     float const * weights,
                     int start,
                     int end) {

    __ALIGN_DATA float result[numElems],
                       result1[numElems];

    float const * src;
    float * dst, weight;

    for (int i=start; i<end; ++i) {

        // Clear
#if defined ( __INTEL_COMPILER ) || defined ( __ICC )
    #pragma simd
    #pragma vector aligned
#endif
        for (int k = 0; k<numElems; ++k)
            result[k] = 0.0f;

        for (int j=0; j<sizes[i]; ++j, ++indices, ++weights) {

            src = vertexSrc + (*indices)*numElems;
            weight = *weights;

            // AddWithWeight
#if defined ( __INTEL_COMPILER ) || defined ( __ICC )
    #pragma simd
    #pragma vector aligned
#endif
            for (int k=0; k<numElems; ++k) {
                result[k] += src[k] * weight;
            }
        }

#if defined ( __INTEL_COMPILER ) || defined ( __ICC )
    #pragma simd
    #pragma vector aligned
#endif
        for (int k=0; k<numElems; ++k) {
            result1[k] = result[k];
        }

        dst = vertexDst + i*numElems;
        memcpy(dst, result1, numElems*sizeof(float));
    }
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv

#endif  // OPENSUBDIV3_OSD_CPU_KERNEL_H
