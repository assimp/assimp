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

#include "../osd/cpuEvaluator.h"
#include "../osd/cpuKernel.h"
#include "../osd/patchBasisCommonTypes.h"
#include "../osd/patchBasisCommon.h"
#include "../osd/patchBasisCommonEval.h"

#include <cstdlib>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

/* static */
bool
CpuEvaluator::EvalStencils(const float *src, BufferDescriptor const &srcDesc,
                           float *dst,       BufferDescriptor const &dstDesc,
                           const int * sizes,
                           const int * offsets,
                           const int * indices,
                           const float * weights,
                           int start, int end) {

    if (end <= start) return true;
    if (srcDesc.length != dstDesc.length) return false;

    // XXX: we can probably expand cpuKernel.cpp to here.
    CpuEvalStencils(src, srcDesc, dst, dstDesc,
                    sizes, offsets, indices, weights, start, end);

    return true;
}

/* static */
bool
CpuEvaluator::EvalStencils(const float *src, BufferDescriptor const &srcDesc,
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

    CpuEvalStencils(src, srcDesc,
                    dst, dstDesc,
                    du,  duDesc,
                    dv,  dvDesc,
                    sizes, offsets, indices,
                    weights, duWeights, dvWeights,
                    start, end);

    return true;
}

/* static */
bool
CpuEvaluator::EvalStencils(const float *src, BufferDescriptor const &srcDesc,
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

    CpuEvalStencils(src, srcDesc,
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

template <typename T>
struct BufferAdapter {
    BufferAdapter(T *p, int length, int stride) :
        _p(p), _length(length), _stride(stride) { }
    void Clear() {
        for (int i = 0; i < _length; ++i) _p[i] = 0;
    }
    void AddWithWeight(T const *src, float w) {
        if (_p) {
            for (int i = 0; i < _length; ++i) {
                _p[i] += src[i] * w;
            }
        }
    }
    const T *operator[] (int index) const {
        return _p + _stride * index;
    }
    BufferAdapter<T> & operator ++() {
        if (_p) {
            _p += _stride;
        }
        return *this;
    }

    T *_p;
    int _length;
    int _stride;
};

/* static */
bool
CpuEvaluator::EvalPatches(const float *src, BufferDescriptor const &srcDesc,
                          float *dst,       BufferDescriptor const &dstDesc,
                          int numPatchCoords,
                          const PatchCoord *patchCoords,
                          const PatchArray *patchArrays,
                          const int *patchIndexBuffer,
                          const PatchParam *patchParamBuffer) {
    if (src) {
        src += srcDesc.offset;
    } else {
        return false;
    }
    if (dst) {
        dst += dstDesc.offset;
        if (srcDesc.length != dstDesc.length) return false;
    } else {
        return false;
    }

    BufferAdapter<const float> srcT(src, srcDesc.length, srcDesc.stride);
    BufferAdapter<float>       dstT(dst, dstDesc.length, dstDesc.stride);

    float wP[20];

    for (int i = 0; i < numPatchCoords; ++i) {
        PatchCoord const &coord = patchCoords[i];
        PatchArray const &array = patchArrays[coord.handle.arrayIndex];

        Osd::PatchParam const & paramStruct =
            patchParamBuffer[coord.handle.patchIndex];
        OsdPatchParam param = OsdPatchParamInit(
            paramStruct.field0, paramStruct.field1, paramStruct.sharpness);

        int patchType = OsdPatchParamIsRegular(param)
            ? array.GetPatchTypeRegular()
            : array.GetPatchTypeIrregular();

        int nPoints = OsdEvaluatePatchBasis(patchType, param,
                coord.s, coord.t, wP, 0, 0, 0, 0, 0);

        int indexBase = array.GetIndexBase() + array.GetStride() *
                (coord.handle.patchIndex - array.GetPrimitiveIdBase());

        const int *cvs = &patchIndexBuffer[indexBase];

        dstT.Clear();
        for (int j = 0; j < nPoints; ++j) {
            dstT.AddWithWeight(srcT[cvs[j]], wP[j]);
        }
        ++dstT;
    }
    return true;
}

/* static */
bool
CpuEvaluator::EvalPatches(const float *src, BufferDescriptor const &srcDesc,
                          float *dst,       BufferDescriptor const &dstDesc,
                          float *du,        BufferDescriptor const &duDesc,
                          float *dv,        BufferDescriptor const &dvDesc,
                          int numPatchCoords,
                          const PatchCoord *patchCoords,
                          const PatchArray *patchArrays,
                          const int *patchIndexBuffer,
                          const PatchParam *patchParamBuffer) {
    if (src) {
        src += srcDesc.offset;
    } else {
        return false;
    }
    if (dst) {
        if (srcDesc.length != dstDesc.length) return false;
        dst += dstDesc.offset;
    }
    if (du) {
        du  += duDesc.offset;
        if (srcDesc.length != duDesc.length) return false;
    }
    if (dv) {
        dv  += dvDesc.offset;
        if (srcDesc.length != dvDesc.length) return false;
    }

    BufferAdapter<const float> srcT(src, srcDesc.length, srcDesc.stride);
    BufferAdapter<float>       dstT(dst, dstDesc.length, dstDesc.stride);
    BufferAdapter<float>        duT(du,  duDesc.length,  duDesc.stride);
    BufferAdapter<float>        dvT(dv,  dvDesc.length,  dvDesc.stride);

    float wP[20], wDs[20], wDt[20];

    for (int i = 0; i < numPatchCoords; ++i) {
        PatchCoord const &coord = patchCoords[i];
        PatchArray const &array = patchArrays[coord.handle.arrayIndex];

        Osd::PatchParam const & paramStruct =
            patchParamBuffer[coord.handle.patchIndex];
        OsdPatchParam param = OsdPatchParamInit(
            paramStruct.field0, paramStruct.field1, paramStruct.sharpness);

        int patchType = OsdPatchParamIsRegular(param)
            ? array.GetPatchTypeRegular()
            : array.GetPatchTypeIrregular();

        int nPoints = OsdEvaluatePatchBasis(patchType, param,
                coord.s, coord.t, wP, wDs, wDt, 0, 0, 0);

        int indexBase = array.GetIndexBase() + array.GetStride() *
                (coord.handle.patchIndex - array.GetPrimitiveIdBase());

        const int *cvs = &patchIndexBuffer[indexBase];

        dstT.Clear();
        duT.Clear();
        dvT.Clear();
        for (int j = 0; j < nPoints; ++j) {
            dstT.AddWithWeight(srcT[cvs[j]], wP[j]);
            duT.AddWithWeight (srcT[cvs[j]], wDs[j]);
            dvT.AddWithWeight (srcT[cvs[j]], wDt[j]);
        }
        ++dstT;
        ++duT;
        ++dvT;
    }
    return true;
}

/* static */
bool
CpuEvaluator::EvalPatches(const float *src, BufferDescriptor const &srcDesc,
                          float *dst,       BufferDescriptor const &dstDesc,
                          float *du,        BufferDescriptor const &duDesc,
                          float *dv,        BufferDescriptor const &dvDesc,
                          float *duu,       BufferDescriptor const &duuDesc,
                          float *duv,       BufferDescriptor const &duvDesc,
                          float *dvv,       BufferDescriptor const &dvvDesc,
                          int numPatchCoords,
                          const PatchCoord *patchCoords,
                          const PatchArray *patchArrays,
                          const int *patchIndexBuffer,
                          const PatchParam *patchParamBuffer) {
    if (src) {
        src += srcDesc.offset;
    } else {
        return false;
    }
    if (dst) {
        if (srcDesc.length != dstDesc.length) return false;
        dst += dstDesc.offset;
    }
    if (du) {
        du  += duDesc.offset;
        if (srcDesc.length != duDesc.length) return false;
    }
    if (dv) {
        dv  += dvDesc.offset;
        if (srcDesc.length != dvDesc.length) return false;
    }
    if (duu) {
        duu += duuDesc.offset;
        if (srcDesc.length != duuDesc.length) return false;
    }
    if (duv) {
        duv += duvDesc.offset;
        if (srcDesc.length != duvDesc.length) return false;
    }
    if (dvv) {
        dvv += dvvDesc.offset;
        if (srcDesc.length != dvvDesc.length) return false;
    }

    BufferAdapter<const float> srcT(src, srcDesc.length, srcDesc.stride);
    BufferAdapter<float>       dstT(dst, dstDesc.length, dstDesc.stride);
    BufferAdapter<float>       duT(du,   duDesc.length,  duDesc.stride);
    BufferAdapter<float>       dvT(dv,   dvDesc.length,  dvDesc.stride);
    BufferAdapter<float>       duuT(duu, duuDesc.length, duuDesc.stride);
    BufferAdapter<float>       duvT(duv, duvDesc.length, duvDesc.stride);
    BufferAdapter<float>       dvvT(dvv, dvvDesc.length, dvvDesc.stride);

    float wP[20], wDu[20], wDv[20], wDuu[20], wDuv[20], wDvv[20];

    for (int i = 0; i < numPatchCoords; ++i) {
        PatchCoord const &coord = patchCoords[i];
        PatchArray const &array = patchArrays[coord.handle.arrayIndex];

        Osd::PatchParam const & paramStruct =
            patchParamBuffer[coord.handle.patchIndex];
        OsdPatchParam param = OsdPatchParamInit(
            paramStruct.field0, paramStruct.field1, paramStruct.sharpness);

        int patchType = OsdPatchParamIsRegular(param)
            ? array.GetPatchTypeRegular()
            : array.GetPatchTypeIrregular();

        int nPoints = OsdEvaluatePatchBasis(patchType, param,
                coord.s, coord.t, wP, wDu, wDv, wDuu, wDuv, wDvv);

        int indexBase = array.GetIndexBase() + array.GetStride() *
                (coord.handle.patchIndex - array.GetPrimitiveIdBase());

        const int *cvs = &patchIndexBuffer[indexBase];

        dstT.Clear();
        duT.Clear();
        dvT.Clear();
        duuT.Clear();
        duvT.Clear();
        dvvT.Clear();
        for (int j = 0; j < nPoints; ++j) {
            dstT.AddWithWeight(srcT[cvs[j]], wP[j]);
            duT.AddWithWeight (srcT[cvs[j]], wDu[j]);
            dvT.AddWithWeight (srcT[cvs[j]], wDv[j]);
            duuT.AddWithWeight (srcT[cvs[j]], wDuu[j]);
            duvT.AddWithWeight (srcT[cvs[j]], wDuv[j]);
            dvvT.AddWithWeight (srcT[cvs[j]], wDvv[j]);
        }
        ++dstT;
        ++duT;
        ++dvT;
        ++duuT;
        ++duvT;
        ++dvvT;
    }
    return true;
}


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv
