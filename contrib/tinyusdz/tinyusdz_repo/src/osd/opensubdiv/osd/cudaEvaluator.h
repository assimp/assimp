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

#ifndef OPENSUBDIV3_OSD_CUDA_EVALUATOR_H
#define OPENSUBDIV3_OSD_CUDA_EVALUATOR_H

#include "../version.h"

#include <vector>
#include "../osd/bufferDescriptor.h"
#include "../osd/types.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {
    class PatchTable;
    class StencilTable;
    class LimitStencilTable;
}

namespace Osd {

/// \brief CUDA stencil table
///
/// This class is a cuda buffer representation of Far::StencilTable.
///
/// CudaEvaluator consumes this table to apply stencils
///
///
class CudaStencilTable {
public:
    static CudaStencilTable *Create(Far::StencilTable const *stencilTable,
                                    void *deviceContext = NULL) {
        (void)deviceContext;  // unused
        return new CudaStencilTable(stencilTable);
    }
    static CudaStencilTable *Create(Far::LimitStencilTable const *limitStencilTable,
                                    void *deviceContext = NULL) {
        (void)deviceContext;  // unused
        return new CudaStencilTable(limitStencilTable);
    }

    explicit CudaStencilTable(Far::StencilTable const *stencilTable);
    explicit CudaStencilTable(Far::LimitStencilTable const *limitStencilTable);
    ~CudaStencilTable();

    // interfaces needed for CudaCompute
    void *GetSizesBuffer() const { return _sizes; }
    void *GetOffsetsBuffer() const { return _offsets; }
    void *GetIndicesBuffer() const { return _indices; }
    void *GetWeightsBuffer() const { return _weights; }
    void *GetDuWeightsBuffer() const { return _duWeights; }
    void *GetDvWeightsBuffer() const { return _dvWeights; }
    void *GetDuuWeightsBuffer() const { return _duuWeights; }
    void *GetDuvWeightsBuffer() const { return _duvWeights; }
    void *GetDvvWeightsBuffer() const { return _dvvWeights; }
    int GetNumStencils() const { return _numStencils; }

private:
    void * _sizes,
         * _offsets,
         * _indices,
         * _weights,
         * _duWeights,
         * _dvWeights,
         * _duuWeights,
         * _duvWeights,
         * _dvvWeights;
    int _numStencils;
};

class CudaEvaluator {
public:
    /// ----------------------------------------------------------------------
    ///
    ///   Stencil evaluations with StencilTable
    ///
    /// ----------------------------------------------------------------------

    /// \brief Generic static compute function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        transparently from OsdMesh template interface.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCudaBuffer() method returning a
    ///                       const float pointer for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCudaBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       Cuda memory interfaces.
    ///
    /// @param instance       not used in the CudaEvaluator
    ///
    /// @param deviceContext  not used in the CudaEvaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    static bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        STENCIL_TABLE const *stencilTable,
        const void *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;  // unused
        (void)deviceContext;  // unused
        return EvalStencils(srcBuffer->BindCudaBuffer(), srcDesc,
                            dstBuffer->BindCudaBuffer(), dstDesc,
                            (int const *)stencilTable->GetSizesBuffer(),
                            (int const *)stencilTable->GetOffsetsBuffer(),
                            (int const *)stencilTable->GetIndicesBuffer(),
                            (float const *)stencilTable->GetWeightsBuffer(),
                            /*start = */ 0,
                            /*end   = */ stencilTable->GetNumStencils());
    }

    /// \brief Static eval stencils function which takes raw cuda buffers for
    ///        input and output.
    ///
    /// @param src            Input primvar pointer. An offset of srcDesc
    ///                       will be applied internally (i.e. the pointer
    ///                       should not include the offset)
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dst            Output primvar pointer. An offset of dstDesc
    ///                       will be applied internally.
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param sizes          pointer to the sizes buffer of the stencil table
    ///
    /// @param offsets        pointer to the offsets buffer of the stencil table
    ///
    /// @param indices        pointer to the indices buffer of the stencil table
    ///
    /// @param weights        pointer to the weights buffer of the stencil table
    ///
    /// @param start          start index of stencil table
    ///
    /// @param end            end index of stencil table
    ///
    static bool EvalStencils(
        const float *src, BufferDescriptor const &srcDesc,
        float *dst,       BufferDescriptor const &dstDesc,
        const int * sizes,
        const int * offsets,
        const int * indices,
        const float * weights,
        int start, int end);

    /// \brief Generic static eval stencils function with derivatives.
    ///        This function has a same signature as other device kernels
    ///        have so that it can be called in the same way from OsdMesh
    ///        template interface.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCudaBuffer() method returning a
    ///                       const float pointer for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCudaBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCudaBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCudaBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param stencilTable   stencil table to be applied.
    ///
    /// @param instance       not used in the cuda kernel
    ///                       (declared as a typed pointer to prevent
    ///                        undesirable template resolution)
    ///
    /// @param deviceContext  not used in the cuda kernel
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    static bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        STENCIL_TABLE const *stencilTable,
        const CudaEvaluator *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalStencils(srcBuffer->BindCudaBuffer(), srcDesc,
                            dstBuffer->BindCudaBuffer(), dstDesc,
                            duBuffer->BindCudaBuffer(),  duDesc,
                            dvBuffer->BindCudaBuffer(),  dvDesc,
                            (int const *)stencilTable->GetSizesBuffer(),
                            (int const *)stencilTable->GetOffsetsBuffer(),
                            (int const *)stencilTable->GetIndicesBuffer(),
                            (float const *)stencilTable->GetWeightsBuffer(),
                            (float const *)stencilTable->GetDuWeightsBuffer(),
                            (float const *)stencilTable->GetDvWeightsBuffer(),
                            /*start = */ 0,
                            /*end   = */ stencilTable->GetNumStencils());
    }

    /// \brief Static eval stencils function with derivatives, which takes
    ///        raw cuda pointers for input and output.
    ///
    /// @param src            Input primvar pointer. An offset of srcDesc
    ///                       will be applied internally (i.e. the pointer
    ///                       should not include the offset)
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dst            Output primvar pointer. An offset of dstDesc
    ///                       will be applied internally.
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param du             Output pointer derivative wrt u. An offset of
    ///                       duDesc will be applied internally.
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dv             Output pointer derivative wrt v. An offset of
    ///                       dvDesc will be applied internally.
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param sizes          pointer to the sizes buffer of the stencil table
    ///
    /// @param offsets        pointer to the offsets buffer of the stencil table
    ///
    /// @param indices        pointer to the indices buffer of the stencil table
    ///
    /// @param weights        pointer to the weights buffer of the stencil table
    ///
    /// @param duWeights      pointer to the du-weights buffer of the stencil table
    ///
    /// @param dvWeights      pointer to the dv-weights buffer of the stencil table
    ///
    /// @param start          start index of stencil table
    ///
    /// @param end            end index of stencil table
    ///
    static bool EvalStencils(
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
        int start, int end);

    /// \brief Generic static eval stencils function with derivatives.
    ///        This function has a same signature as other device kernels
    ///        have so that it can be called in the same way from OsdMesh
    ///        template interface.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCudaBuffer() method returning a
    ///                       const float pointer for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCudaBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCudaBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCudaBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindCudaBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindCudaBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindCudaBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param stencilTable   stencil table to be applied.
    ///
    /// @param instance       not used in the cuda kernel
    ///                       (declared as a typed pointer to prevent
    ///                        undesirable template resolution)
    ///
    /// @param deviceContext  not used in the cuda kernel
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    static bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        DST_BUFFER *duuBuffer, BufferDescriptor const &duuDesc,
        DST_BUFFER *duvBuffer, BufferDescriptor const &duvDesc,
        DST_BUFFER *dvvBuffer, BufferDescriptor const &dvvDesc,
        STENCIL_TABLE const *stencilTable,
        const CudaEvaluator *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalStencils(srcBuffer->BindCudaBuffer(), srcDesc,
                            dstBuffer->BindCudaBuffer(), dstDesc,
                            duBuffer->BindCudaBuffer(),  duDesc,
                            dvBuffer->BindCudaBuffer(),  dvDesc,
                            duuBuffer->BindCudaBuffer(), duuDesc,
                            duvBuffer->BindCudaBuffer(), duvDesc,
                            dvvBuffer->BindCudaBuffer(), dvvDesc,
                            (int const *)stencilTable->GetSizesBuffer(),
                            (int const *)stencilTable->GetOffsetsBuffer(),
                            (int const *)stencilTable->GetIndicesBuffer(),
                            (float const *)stencilTable->GetWeightsBuffer(),
                            (float const *)stencilTable->GetDuWeightsBuffer(),
                            (float const *)stencilTable->GetDvWeightsBuffer(),
                            (float const *)stencilTable->GetDuuWeightsBuffer(),
                            (float const *)stencilTable->GetDuvWeightsBuffer(),
                            (float const *)stencilTable->GetDvvWeightsBuffer(),
                            /*start = */ 0,
                            /*end   = */ stencilTable->GetNumStencils());
    }

    /// \brief Static eval stencils function with derivatives, which takes
    ///        raw cuda pointers for input and output.
    ///
    /// @param src            Input primvar pointer. An offset of srcDesc
    ///                       will be applied internally (i.e. the pointer
    ///                       should not include the offset)
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dst            Output primvar pointer. An offset of dstDesc
    ///                       will be applied internally.
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param du             Output pointer derivative wrt u. An offset of
    ///                       duDesc will be applied internally.
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dv             Output pointer derivative wrt v. An offset of
    ///                       dvDesc will be applied internally.
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duu            Output pointer 2nd derivative wrt u. An offset of
    ///                       duuDesc will be applied internally.
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duv            Output pointer 2nd derivative wrt u and v. An offset of
    ///                       duvDesc will be applied internally.
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvv            Output pointer 2nd derivative wrt v. An offset of
    ///                       dvvDesc will be applied internally.
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param sizes          pointer to the sizes buffer of the stencil table
    ///
    /// @param offsets        pointer to the offsets buffer of the stencil table
    ///
    /// @param indices        pointer to the indices buffer of the stencil table
    ///
    /// @param weights        pointer to the weights buffer of the stencil table
    ///
    /// @param duWeights      pointer to the du-weights buffer of the stencil table
    ///
    /// @param dvWeights      pointer to the dv-weights buffer of the stencil table
    ///
    /// @param duuWeights     pointer to the duu-weights buffer of the stencil table
    ///
    /// @param duvWeights     pointer to the duv-weights buffer of the stencil table
    ///
    /// @param dvvWeights     pointer to the dvv-weights buffer of the stencil table
    ///
    /// @param start          start index of stencil table
    ///
    /// @param end            end index of stencil table
    ///
    static bool EvalStencils(
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
        int start, int end);

    /// ----------------------------------------------------------------------
    ///
    ///   Limit evaluations with PatchTable
    ///
    /// ----------------------------------------------------------------------

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCudaBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///                         must have BindCudaBuffer() method returning an
    ///                         array of PatchCoord struct in cuda memory.
    ///
    /// @param patchTable       CudaPatchTable or equivalent
    ///
    /// @param instance         not used in the cuda evaluator
    ///
    /// @param deviceContext    not used in the cuda evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatches(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        CudaEvaluator const *instance,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCudaBuffer(), srcDesc,
                           dstBuffer->BindCudaBuffer(), dstDesc,
                           numPatchCoords,
                           (const PatchCoord *)patchCoords->BindCudaBuffer(),
                           (const PatchArray *)patchTable->GetPatchArrayBuffer(),
                           (const int *)patchTable->GetPatchIndexBuffer(),
                           (const PatchParam *)patchTable->GetPatchParamBuffer());
    }

    /// \brief Generic limit eval function with derivatives. This function has
    ///        a same signature as other device kernels have so that it can be
    ///        called in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCudaBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CudaPatchTable or equivalent
    ///
    /// @param instance         not used in the cuda evaluator
    ///
    /// @param deviceContext    not used in the cuda evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatches(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        CudaEvaluator const *instance,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCudaBuffer(), srcDesc,
                           dstBuffer->BindCudaBuffer(), dstDesc,
                           duBuffer->BindCudaBuffer(),  duDesc,
                           dvBuffer->BindCudaBuffer(),  dvDesc,
                           numPatchCoords,
                           (const PatchCoord *)patchCoords->BindCudaBuffer(),
                           (const PatchArray *)patchTable->GetPatchArrayBuffer(),
                           (const int *)patchTable->GetPatchIndexBuffer(),
                           (const PatchParam *)patchTable->GetPatchParamBuffer());
    }

    /// \brief Generic limit eval function with derivatives. This function has
    ///        a same signature as other device kernels have so that it can be
    ///        called in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCudaBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer        Output buffer 2nd derivative wrt v
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CudaPatchTable or equivalent
    ///
    /// @param instance         not used in the cuda evaluator
    ///
    /// @param deviceContext    not used in the cuda evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatches(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        DST_BUFFER *duuBuffer, BufferDescriptor const &duuDesc,
        DST_BUFFER *duvBuffer, BufferDescriptor const &duvDesc,
        DST_BUFFER *dvvBuffer, BufferDescriptor const &dvvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        CudaEvaluator const *instance,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCudaBuffer(), srcDesc,
                           dstBuffer->BindCudaBuffer(), dstDesc,
                           duBuffer->BindCudaBuffer(),  duDesc,
                           dvBuffer->BindCudaBuffer(),  dvDesc,
                           duuBuffer->BindCudaBuffer(), duuDesc,
                           duvBuffer->BindCudaBuffer(), duvDesc,
                           dvvBuffer->BindCudaBuffer(), dvvDesc,
                           numPatchCoords,
                           (const PatchCoord *)patchCoords->BindCudaBuffer(),
                           (const PatchArray *)patchTable->GetPatchArrayBuffer(),
                           (const int *)patchTable->GetPatchIndexBuffer(),
                           (const PatchParam *)patchTable->GetPatchParamBuffer());
    }

    /// \brief Static limit eval function. It takes an array of PatchCoord
    ///        and evaluate limit values on given PatchTable.
    ///
    /// @param src              Input primvar pointer. An offset of srcDesc
    ///                         will be applied internally (i.e. the pointer
    ///                         should not include the offset)
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dst              Output primvar pointer. An offset of dstDesc
    ///                         will be applied internally.
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchArrays      an array of Osd::PatchArray struct
    ///                         indexed by PatchCoord::arrayIndex
    ///
    /// @param patchIndices     an array of patch indices
    ///                         indexed by PatchCoord::vertIndex
    ///
    /// @param patchParams      an array of Osd::PatchParam struct
    ///                         indexed by PatchCoord::patchIndex
    ///
    static bool EvalPatches(
        const float *src, BufferDescriptor const &srcDesc,
        float *dst,       BufferDescriptor const &dstDesc,
        int numPatchCoords,
        const PatchCoord *patchCoords,
        const PatchArray *patchArrays,
        const int *patchIndices,
        const PatchParam *patchParams);

    /// \brief Static limit eval function. It takes an array of PatchCoord
    ///        and evaluate limit values on given PatchTable.
    ///
    /// @param src              Input primvar pointer. An offset of srcDesc
    ///                         will be applied internally (i.e. the pointer
    ///                         should not include the offset)
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dst              Output primvar pointer. An offset of dstDesc
    ///                         will be applied internally.
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param du               Output pointer derivative wrt u. An offset of
    ///                         duDesc will be applied internally.
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dv               Output pointer derivative wrt v. An offset of
    ///                         dvDesc will be applied internally.
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchArrays      an array of Osd::PatchArray struct
    ///                         indexed by PatchCoord::arrayIndex
    ///
    /// @param patchIndices     an array of patch indices
    ///                         indexed by PatchCoord::vertIndex
    ///
    /// @param patchParams      an array of Osd::PatchParam struct
    ///                         indexed by PatchCoord::patchIndex
    ///
    static bool EvalPatches(
        const float *src, BufferDescriptor const &srcDesc,
        float *dst,       BufferDescriptor const &dstDesc,
        float *du,        BufferDescriptor const &duDesc,
        float *dv,        BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PatchCoord const *patchCoords,
        PatchArray const *patchArrays,
        const int *patchIndices,
        PatchParam const *patchParams);

    /// \brief Static limit eval function. It takes an array of PatchCoord
    ///        and evaluate limit values on given PatchTable.
    ///
    /// @param src              Input primvar pointer. An offset of srcDesc
    ///                         will be applied internally (i.e. the pointer
    ///                         should not include the offset)
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dst              Output primvar pointer. An offset of dstDesc
    ///                         will be applied internally.
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param du               Output pointer derivative wrt u. An offset of
    ///                         duDesc will be applied internally.
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dv               Output pointer derivative wrt v. An offset of
    ///                         dvDesc will be applied internally.
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duu              Output pointer 2nd derivative wrt u. An offset of
    ///                         duuDesc will be applied internally.
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duv              Output pointer 2nd derivative wrt u and v. An offset of
    ///                         duvDesc will be applied internally.
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvv              Output pointer 2nd derivative wrt v. An offset of
    ///                         dvvDesc will be applied internally.
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchArrays      an array of Osd::PatchArray struct
    ///                         indexed by PatchCoord::arrayIndex
    ///
    /// @param patchIndices     an array of patch indices
    ///                         indexed by PatchCoord::vertIndex
    ///
    /// @param patchParams      an array of Osd::PatchParam struct
    ///                         indexed by PatchCoord::patchIndex
    ///
    static bool EvalPatches(
        const float *src, BufferDescriptor const &srcDesc,
        float *dst,       BufferDescriptor const &dstDesc,
        float *du,        BufferDescriptor const &duDesc,
        float *dv,        BufferDescriptor const &dvDesc,
        float *duu,       BufferDescriptor const &duuDesc,
        float *duv,       BufferDescriptor const &duvDesc,
        float *dvv,       BufferDescriptor const &dvvDesc,
        int numPatchCoords,
        PatchCoord const *patchCoords,
        PatchArray const *patchArrays,
        const int *patchIndices,
        PatchParam const *patchParams);

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCudaBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///                         must have BindCudaBuffer() method returning an
    ///                         array of PatchCoord struct in cuda memory.
    ///
    /// @param patchTable       CudaPatchTable or equivalent
    ///
    /// @param instance         not used in the cuda evaluator
    ///
    /// @param deviceContext    not used in the cuda evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatchesVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        CudaEvaluator const *instance,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCudaBuffer(), srcDesc,
                           dstBuffer->BindCudaBuffer(), dstDesc,
                           numPatchCoords,
                           (const PatchCoord *)patchCoords->BindCudaBuffer(),
                           (const PatchArray *)patchTable->GetVaryingPatchArrayBuffer(),
                           (const int *)patchTable->GetVaryingPatchIndexBuffer(),
                           (const PatchParam *)patchTable->GetPatchParamBuffer());
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCudaBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///                         must have BindCudaBuffer() method returning an
    ///                         array of PatchCoord struct in cuda memory.
    ///
    /// @param patchTable       CudaPatchTable or equivalent
    ///
    /// @param instance         not used in the cuda evaluator
    ///
    /// @param deviceContext    not used in the cuda evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatchesVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        CudaEvaluator const *instance,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCudaBuffer(), srcDesc,
                           dstBuffer->BindCudaBuffer(), dstDesc,
                           duBuffer->BindCudaBuffer(), duDesc,
                           dvBuffer->BindCudaBuffer(), dvDesc,
                           numPatchCoords,
                           (const PatchCoord *)patchCoords->BindCudaBuffer(),
                           (const PatchArray *)patchTable->GetVaryingPatchArrayBuffer(),
                           (const int *)patchTable->GetVaryingPatchIndexBuffer(),
                           (const PatchParam *)patchTable->GetPatchParamBuffer());
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCudaBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer        Output buffer 2nd derivative wrt v
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///                         must have BindCudaBuffer() method returning an
    ///                         array of PatchCoord struct in cuda memory.
    ///
    /// @param patchTable       CudaPatchTable or equivalent
    ///
    /// @param instance         not used in the cuda evaluator
    ///
    /// @param deviceContext    not used in the cuda evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatchesVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        DST_BUFFER *duuBuffer, BufferDescriptor const &duuDesc,
        DST_BUFFER *duvBuffer, BufferDescriptor const &duvDesc,
        DST_BUFFER *dvvBuffer, BufferDescriptor const &dvvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        CudaEvaluator const *instance,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCudaBuffer(), srcDesc,
                           dstBuffer->BindCudaBuffer(), dstDesc,
                           duBuffer->BindCudaBuffer(), duDesc,
                           dvBuffer->BindCudaBuffer(), dvDesc,
                           duuBuffer->BindCudaBuffer(), duuDesc,
                           duvBuffer->BindCudaBuffer(), duvDesc,
                           dvvBuffer->BindCudaBuffer(), dvvDesc,
                           numPatchCoords,
                           (const PatchCoord *)patchCoords->BindCudaBuffer(),
                           (const PatchArray *)patchTable->GetVaryingPatchArrayBuffer(),
                           (const int *)patchTable->GetVaryingPatchIndexBuffer(),
                           (const PatchParam *)patchTable->GetPatchParamBuffer());
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCudaBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///                         must have BindCudaBuffer() method returning an
    ///                         array of PatchCoord struct in cuda memory.
    ///
    /// @param patchTable       CudaPatchTable or equivalent
    ///
    /// @param fvarChannel      face-varying channel
    ///
    /// @param instance         not used in the cuda evaluator
    ///
    /// @param deviceContext    not used in the cuda evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatchesFaceVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        int fvarChannel,
        CudaEvaluator const *instance,
        void * deviceContext = NULL) {

        (void)instance;   // unused
        (void)deviceContext;   // unused

        return EvalPatches(srcBuffer->BindCudaBuffer(), srcDesc,
                           dstBuffer->BindCudaBuffer(), dstDesc,
                           numPatchCoords,
                           (const PatchCoord *)patchCoords->BindCudaBuffer(),
                           (const PatchArray *)patchTable->GetFVarPatchArrayBuffer(fvarChannel),
                           (const int *)patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           (const PatchParam *)patchTable->GetFVarPatchParamBuffer(fvarChannel));
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCudaBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///                         must have BindCudaBuffer() method returning an
    ///                         array of PatchCoord struct in cuda memory.
    ///
    /// @param patchTable       CudaPatchTable or equivalent
    ///
    /// @param fvarChannel      face-varying channel
    ///
    /// @param instance         not used in the cuda evaluator
    ///
    /// @param deviceContext    not used in the cuda evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatchesFaceVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer, BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer, BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        int fvarChannel,
        CudaEvaluator const *instance,
        void * deviceContext = NULL) {

        (void)instance;   // unused
        (void)deviceContext;   // unused

        return EvalPatches(srcBuffer->BindCudaBuffer(), srcDesc,
                           dstBuffer->BindCudaBuffer(), dstDesc,
                           duBuffer->BindCudaBuffer(), duDesc,
                           dvBuffer->BindCudaBuffer(), dvDesc,
                           numPatchCoords,
                           (const PatchCoord *)patchCoords->BindCudaBuffer(),
                           (const PatchArray *)patchTable->GetFVarPatchArrayBuffer(fvarChannel),
                           (const int *)patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           (const PatchParam *)patchTable->GetFVarPatchParamBuffer(fvarChannel));
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCudaBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer        Output buffer 2nd derivative wrt v
    ///                         must have BindCudaBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///                         must have BindCudaBuffer() method returning an
    ///                         array of PatchCoord struct in cuda memory.
    ///
    /// @param patchTable       CudaPatchTable or equivalent
    ///
    /// @param fvarChannel      face-varying channel
    ///
    /// @param instance         not used in the cuda evaluator
    ///
    /// @param deviceContext    not used in the cuda evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatchesFaceVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        DST_BUFFER *duuBuffer, BufferDescriptor const &duuDesc,
        DST_BUFFER *duvBuffer, BufferDescriptor const &duvDesc,
        DST_BUFFER *dvvBuffer, BufferDescriptor const &dvvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        int fvarChannel,
        CudaEvaluator const *instance,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCudaBuffer(), srcDesc,
                           dstBuffer->BindCudaBuffer(), dstDesc,
                           duBuffer->BindCudaBuffer(), duDesc,
                           dvBuffer->BindCudaBuffer(), dvDesc,
                           duuBuffer->BindCudaBuffer(), duuDesc,
                           duvBuffer->BindCudaBuffer(), duvDesc,
                           dvvBuffer->BindCudaBuffer(), dvvDesc,
                           numPatchCoords,
                           (const PatchCoord *)patchCoords->BindCudaBuffer(),
                           (const PatchArray *)patchTable->GetFVarPatchArrayBuffer(fvarChannel),
                           (const int *)patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           (const PatchParam *)patchTable->GetFVarPatchParamBuffer(fvarChannel));
    }

    /// ----------------------------------------------------------------------
    ///
    ///   Other methods
    ///
    /// ----------------------------------------------------------------------
    static void Synchronize(void *deviceContext = NULL);
};


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv


#endif  // OPENSUBDIV3_OSD_CUDA_EVALUATOR_H
