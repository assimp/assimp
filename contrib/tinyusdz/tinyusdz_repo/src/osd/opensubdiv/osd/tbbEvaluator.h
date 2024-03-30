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

#ifndef OPENSUBDIV3_OSD_TBB_EVALUATOR_H
#define OPENSUBDIV3_OSD_TBB_EVALUATOR_H

#include "../version.h"
#include "../osd/bufferDescriptor.h"
#include "../osd/types.h"

#include <cstddef>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

class TbbEvaluator {
public:
    /// ----------------------------------------------------------------------
    ///
    ///   Stencil evaluations with StencilTable
    ///
    /// ----------------------------------------------------------------------

    /// \brief Generic static eval stencils function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way from OsdMesh template interface.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCpuBuffer() method returning a
    ///                       const float pointer for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCpuBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param stencilTable   Far::StencilTable or equivalent
    ///
    /// @param instance       not used in the tbb kernel
    ///                       (declared as a typed pointer to prevent
    ///                        undesirable template resolution)
    ///
    /// @param deviceContext  not used in the tbb kernel
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    static bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        STENCIL_TABLE const *stencilTable,
        TbbEvaluator const *instance = NULL,
        void *deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        if (stencilTable->GetNumStencils() == 0)
            return false;

        return EvalStencils(srcBuffer->BindCpuBuffer(), srcDesc,
                            dstBuffer->BindCpuBuffer(), dstDesc,
                            &stencilTable->GetSizes()[0],
                            &stencilTable->GetOffsets()[0],
                            &stencilTable->GetControlIndices()[0],
                            &stencilTable->GetWeights()[0],
                            /*start = */ 0,
                            /*end   = */ stencilTable->GetNumStencils());
    }

    /// \brief Static eval stencils function which takes raw CPU pointers for
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
    ///                       must have BindCpuBuffer() method returning a
    ///                       const float pointer for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCpuBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCpuBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCpuBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param stencilTable   Far::StencilTable or equivalent
    ///
    /// @param instance       not used in the tbb kernel
    ///                       (declared as a typed pointer to prevent
    ///                        undesirable template resolution)
    ///
    /// @param deviceContext  not used in the tbb kernel
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    static bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        STENCIL_TABLE const *stencilTable,
        const TbbEvaluator *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalStencils(srcBuffer->BindCpuBuffer(), srcDesc,
                            dstBuffer->BindCpuBuffer(), dstDesc,
                            duBuffer->BindCpuBuffer(),  duDesc,
                            dvBuffer->BindCpuBuffer(),  dvDesc,
                            &stencilTable->GetSizes()[0],
                            &stencilTable->GetOffsets()[0],
                            &stencilTable->GetControlIndices()[0],
                            &stencilTable->GetWeights()[0],
                            &stencilTable->GetDuWeights()[0],
                            &stencilTable->GetDvWeights()[0],
                            /*start = */ 0,
                            /*end   = */ stencilTable->GetNumStencils());
    }

    /// \brief Static eval stencils function with derivatives, which takes
    ///        raw CPU pointers for input and output.
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
    ///                       must have BindCpuBuffer() method returning a
    ///                       const float pointer for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCpuBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCpuBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCpuBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindCpuBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindCpuBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindCpuBuffer() method returning a
    ///                       float pointer for write
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param stencilTable   Far::StencilTable or equivalent
    ///
    /// @param instance       not used in the tbb kernel
    ///                       (declared as a typed pointer to prevent
    ///                        undesirable template resolution)
    ///
    /// @param deviceContext  not used in the tbb kernel
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
        const TbbEvaluator *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalStencils(srcBuffer->BindCpuBuffer(), srcDesc,
                            dstBuffer->BindCpuBuffer(), dstDesc,
                            duBuffer->BindCpuBuffer(),  duDesc,
                            dvBuffer->BindCpuBuffer(),  dvDesc,
                            duuBuffer->BindCpuBuffer(), duuDesc,
                            duvBuffer->BindCpuBuffer(), duvDesc,
                            dvvBuffer->BindCpuBuffer(), dvvDesc,
                            &stencilTable->GetSizes()[0],
                            &stencilTable->GetOffsets()[0],
                            &stencilTable->GetControlIndices()[0],
                            &stencilTable->GetWeights()[0],
                            &stencilTable->GetDuWeights()[0],
                            &stencilTable->GetDvWeights()[0],
                            &stencilTable->GetDuuWeights()[0],
                            &stencilTable->GetDuvWeights()[0],
                            &stencilTable->GetDvvWeights()[0],
                            /*start = */ 0,
                            /*end   = */ stencilTable->GetNumStencils());
    }

    /// \brief Static eval stencils function with derivatives, which takes
    ///        raw CPU pointers for input and output.
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
    ///                         must have BindCpuBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CpuPatchTable or equivalent
    ///                         XXX: currently Far::PatchTable can't be used
    ///                              due to interface mismatch
    ///
    /// @param instance         not used in the cpu evaluator
    ///
    /// @param deviceContext    not used in the cpu evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatches(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        TbbEvaluator const *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCpuBuffer(), srcDesc,
                           dstBuffer->BindCpuBuffer(), dstDesc,
                           numPatchCoords,
                           (const PatchCoord*)patchCoords->BindCpuBuffer(),
                           patchTable->GetPatchArrayBuffer(),
                           patchTable->GetPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer());
    }

    /// \brief Generic limit eval function with derivatives. This function has
    ///        a same signature as other device kernels have so that it can be
    ///        called in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCpuBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CpuPatchTable or equivalent
    ///                         XXX: currently Far::PatchTable can't be used
    ///                              due to interface mismatch
    ///
    /// @param instance         not used in the cpu evaluator
    ///
    /// @param deviceContext    not used in the cpu evaluator
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
        TbbEvaluator const *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        // XXX: PatchCoords is somewhat abusing vertex primvar buffer interop.
        //      ideally all buffer classes should have templated by datatype
        //      so that downcast isn't needed there.
        //      (e.g. Osd::CpuBuffer<PatchCoord> )
        //
        return EvalPatches(srcBuffer->BindCpuBuffer(), srcDesc,
                           dstBuffer->BindCpuBuffer(), dstDesc,
                           duBuffer->BindCpuBuffer(),  duDesc,
                           dvBuffer->BindCpuBuffer(),  dvDesc,
                           numPatchCoords,
                           (const PatchCoord*)patchCoords->BindCpuBuffer(),
                           patchTable->GetPatchArrayBuffer(),
                           patchTable->GetPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer());
    }

    /// \brief Generic limit eval function with derivatives. This function has
    ///        a same signature as other device kernels have so that it can be
    ///        called in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCpuBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer        Output buffer 2nd derivative wrt u and v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer        Output buffer 2nd derivative wrt v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CpuPatchTable or equivalent
    ///                         XXX: currently Far::PatchTable can't be used
    ///                              due to interface mismatch
    ///
    /// @param instance         not used in the cpu evaluator
    ///
    /// @param deviceContext    not used in the cpu evaluator
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
        TbbEvaluator const *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        // XXX: PatchCoords is somewhat abusing vertex primvar buffer interop.
        //      ideally all buffer classes should have templated by datatype
        //      so that downcast isn't needed there.
        //      (e.g. Osd::CpuBuffer<PatchCoord> )
        //
        return EvalPatches(srcBuffer->BindCpuBuffer(), srcDesc,
                           dstBuffer->BindCpuBuffer(), dstDesc,
                           duBuffer->BindCpuBuffer(),  duDesc,
                           dvBuffer->BindCpuBuffer(),  dvDesc,
                           duuBuffer->BindCpuBuffer(), duuDesc,
                           duvBuffer->BindCpuBuffer(), duvDesc,
                           dvvBuffer->BindCpuBuffer(), dvvDesc,
                           numPatchCoords,
                           (const PatchCoord*)patchCoords->BindCpuBuffer(),
                           patchTable->GetPatchArrayBuffer(),
                           patchTable->GetPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer());
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
    /// @param patchIndexBuffer an array of patch indices
    ///                         indexed by PatchCoord::vertIndex
    ///
    /// @param patchParamBuffer an array of Osd::PatchParam struct
    ///                         indexed by PatchCoord::patchIndex
    ///
    static bool EvalPatches(
        const float *src, BufferDescriptor const &srcDesc,
        float *dst,       BufferDescriptor const &dstDesc,
        int numPatchCoords,
        const PatchCoord *patchCoords,
        const PatchArray *patchArrays,
        const int *patchIndexBuffer,
        const PatchParam *patchParamBuffer);

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
    /// @param patchIndexBuffer an array of patch indices
    ///                         indexed by PatchCoord::vertIndex
    ///
    /// @param patchParamBuffer an array of Osd::PatchParam struct
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
        const int *patchIndexBuffer,
        PatchParam const *patchParamBuffer);

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
    /// @param patchIndexBuffer an array of patch indices
    ///                         indexed by PatchCoord::vertIndex
    ///
    /// @param patchParamBuffer an array of Osd::PatchParam struct
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
        const int *patchIndexBuffer,
        PatchParam const *patchParamBuffer);

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCpuBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CpuPatchTable or equivalent
    ///                         XXX: currently Far::PatchTable can't be used
    ///                              due to interface mismatch
    ///
    /// @param instance         not used in the cpu evaluator
    ///
    /// @param deviceContext    not used in the cpu evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatchesVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        TbbEvaluator const *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCpuBuffer(), srcDesc,
                           dstBuffer->BindCpuBuffer(), dstDesc,
                           numPatchCoords,
                           (const PatchCoord*)patchCoords->BindCpuBuffer(),
                           patchTable->GetVaryingPatchArrayBuffer(),
                           patchTable->GetVaryingPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer());
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCpuBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CpuPatchTable or equivalent
    ///                         XXX: currently Far::PatchTable can't be used
    ///                              due to interface mismatch
    ///
    /// @param instance         not used in the cpu evaluator
    ///
    /// @param deviceContext    not used in the cpu evaluator
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
        TbbEvaluator const *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCpuBuffer(), srcDesc,
                           dstBuffer->BindCpuBuffer(), dstDesc,
                           duBuffer->BindCpuBuffer(),  duDesc,
                           dvBuffer->BindCpuBuffer(),  dvDesc,
                           numPatchCoords,
                           (const PatchCoord*)patchCoords->BindCpuBuffer(),
                           patchTable->GetVaryingPatchArrayBuffer(),
                           patchTable->GetVaryingPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer());
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCpuBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer        Output buffer 2nd derivative wrt u and v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer        Output buffer 2nd derivative wrt v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CpuPatchTable or equivalent
    ///                         XXX: currently Far::PatchTable can't be used
    ///                              due to interface mismatch
    ///
    /// @param instance         not used in the cpu evaluator
    ///
    /// @param deviceContext    not used in the cpu evaluator
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
        TbbEvaluator const *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCpuBuffer(), srcDesc,
                           dstBuffer->BindCpuBuffer(), dstDesc,
                           duBuffer->BindCpuBuffer(),  duDesc,
                           dvBuffer->BindCpuBuffer(),  dvDesc,
                           duuBuffer->BindCpuBuffer(), duuDesc,
                           duvBuffer->BindCpuBuffer(), duvDesc,
                           dvvBuffer->BindCpuBuffer(), dvvDesc,
                           numPatchCoords,
                           (const PatchCoord*)patchCoords->BindCpuBuffer(),
                           patchTable->GetVaryingPatchArrayBuffer(),
                           patchTable->GetVaryingPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer());
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCpuBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CpuPatchTable or equivalent
    ///                         XXX: currently Far::PatchTable can't be used
    ///                              due to interface mismatch
    ///
    /// @param fvarChannel      face-varying channel
    ///
    /// @param instance         not used in the cpu evaluator
    ///
    /// @param deviceContext    not used in the cpu evaluator
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
        TbbEvaluator const *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCpuBuffer(), srcDesc,
                           dstBuffer->BindCpuBuffer(), dstDesc,
                           numPatchCoords,
                           (const PatchCoord*)patchCoords->BindCpuBuffer(),
                           patchTable->GetFVarPatchArrayBuffer(fvarChannel),
                           patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           patchTable->GetFVarPatchParamBuffer(fvarChannel));
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCpuBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CpuPatchTable or equivalent
    ///                         XXX: currently Far::PatchTable can't be used
    ///                              due to interface mismatch
    ///
    /// @param fvarChannel      face-varying channel
    ///
    /// @param instance         not used in the cpu evaluator
    ///
    /// @param deviceContext    not used in the cpu evaluator
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatchesFaceVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        int fvarChannel,
        TbbEvaluator const *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCpuBuffer(), srcDesc,
                           dstBuffer->BindCpuBuffer(), dstDesc,
                           duBuffer->BindCpuBuffer(),  duDesc,
                           dvBuffer->BindCpuBuffer(),  dvDesc,
                           numPatchCoords,
                           (const PatchCoord*)patchCoords->BindCpuBuffer(),
                           patchTable->GetFVarPatchArrayBuffer(fvarChannel),
                           patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           patchTable->GetFVarPatchParamBuffer(fvarChannel));
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCpuBuffer() method returning a
    ///                         const float pointer for read
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer        Output buffer 2nd derivative wrt u and v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer        Output buffer 2nd derivative wrt v
    ///                         must have BindCpuBuffer() method returning a
    ///                         float pointer for write
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CpuPatchTable or equivalent
    ///                         XXX: currently Far::PatchTable can't be used
    ///                              due to interface mismatch
    ///
    /// @param fvarChannel      face-varying channel
    ///
    /// @param instance         not used in the cpu evaluator
    ///
    /// @param deviceContext    not used in the cpu evaluator
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
        TbbEvaluator const *instance = NULL,
        void * deviceContext = NULL) {

        (void)instance;       // unused
        (void)deviceContext;  // unused

        return EvalPatches(srcBuffer->BindCpuBuffer(), srcDesc,
                           dstBuffer->BindCpuBuffer(), dstDesc,
                           duBuffer->BindCpuBuffer(),  duDesc,
                           dvBuffer->BindCpuBuffer(),  dvDesc,
                           duuBuffer->BindCpuBuffer(), duuDesc,
                           duvBuffer->BindCpuBuffer(), duvDesc,
                           dvvBuffer->BindCpuBuffer(), dvvDesc,
                           numPatchCoords,
                           (const PatchCoord*)patchCoords->BindCpuBuffer(),
                           patchTable->GetFVarPatchArrayBuffer(fvarChannel),
                           patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           patchTable->GetFVarPatchParamBuffer(fvarChannel));
    }

    /// ----------------------------------------------------------------------
    ///
    ///   Other methods
    ///
    /// ----------------------------------------------------------------------

    /// \brief synchronize all asynchronous computation invoked on this device.
    static void Synchronize(void *deviceContext = NULL);

    /// \brief initialize tbb task schedular
    ///        (optional: client may use tbb::task_scheduler_init)
    ///
    /// @param numThreads      how many threads
    ///
    static void SetNumThreads(int numThreads);
};


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv


#endif  // OPENSUBDIV3_OSD_TBB_EVALUATOR_H
