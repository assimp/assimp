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

#ifndef OPENSUBDIV3_OSD_MTL_COMPUTE_EVALUATOR_H
#define OPENSUBDIV3_OSD_MTL_COMPUTE_EVALUATOR_H

#include "../version.h"

#include "../osd/types.h"
#include "../osd/bufferDescriptor.h"
#include "../osd/mtlCommon.h"

@protocol MTLDevice;
@protocol MTLBuffer;
@protocol MTLLibrary;
@protocol MTLComputePipelineState;

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {
    class PatchTable;
    class StencilTable;
    class LimitStencilTable;
}

namespace Osd {

class MTLStencilTable
{
public:
    template<typename STENCIL_TABLE, typename DEVICE_CONTEXT>
    static MTLStencilTable* Create(STENCIL_TABLE* stencilTable,
                                   DEVICE_CONTEXT context)
    {
        return new MTLStencilTable(stencilTable, context);
    }


    MTLStencilTable(Far::StencilTable const* stencilTable, MTLContext* context);
    MTLStencilTable(Far::LimitStencilTable const* stencilTable, MTLContext* context);
    ~MTLStencilTable();

    id<MTLBuffer> GetSizesBuffer() const { return _sizesBuffer; }
    id<MTLBuffer> GetOffsetsBuffer() const { return _offsetsBuffer; }
    id<MTLBuffer> GetIndicesBuffer() const  { return _indicesBuffer; }
    id<MTLBuffer> GetWeightsBuffer() const { return _weightsBuffer; }
    id<MTLBuffer> GetDuWeightsBuffer() const { return _duWeightsBuffer; }
    id<MTLBuffer> GetDvWeightsBuffer() const { return _dvWeightsBuffer; }
    id<MTLBuffer> GetDuuWeightsBuffer() const { return _duuWeightsBuffer; }
    id<MTLBuffer> GetDuvWeightsBuffer() const { return _duvWeightsBuffer; }
    id<MTLBuffer> GetDvvWeightsBuffer() const { return _dvvWeightsBuffer; }

    int GetNumStencils() const  { return _numStencils; }

private:
    id<MTLBuffer> _sizesBuffer;
    id<MTLBuffer> _offsetsBuffer;
    id<MTLBuffer> _indicesBuffer;
    id<MTLBuffer> _weightsBuffer;
    id<MTLBuffer> _duWeightsBuffer;
    id<MTLBuffer> _dvWeightsBuffer;
    id<MTLBuffer> _duuWeightsBuffer;
    id<MTLBuffer> _duvWeightsBuffer;
    id<MTLBuffer> _dvvWeightsBuffer;

    int _numStencils;
};

class MTLComputeEvaluator
{
public:
    typedef bool Instantiatable;

    static MTLComputeEvaluator * Create(BufferDescriptor const &srcDesc,
                                        BufferDescriptor const &dstDesc,
                                        BufferDescriptor const &duDesc,
                                        BufferDescriptor const &dvDesc,
                                        MTLContext* context);

    static MTLComputeEvaluator * Create(BufferDescriptor const &srcDesc,
                                        BufferDescriptor const &dstDesc,
                                        BufferDescriptor const &duDesc,
                                        BufferDescriptor const &dvDesc,
                                        BufferDescriptor const &duuDesc,
                                        BufferDescriptor const &duvDesc,
                                        BufferDescriptor const &dvvDesc,
                                        MTLContext* context);

    MTLComputeEvaluator();
    ~MTLComputeEvaluator();

    /// ----------------------------------------------------------------------
    ///
    ///   Stencil evaluations with StencilTable
    ///
    /// ----------------------------------------------------------------------

    /// \brief Generic static stencil function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        transparently from OsdMesh template interface.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       MTLBuffer interfaces.
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param context        used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    static bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        STENCIL_TABLE const *stencilTable,
        MTLComputeEvaluator const *instance,
        MTLContext* context)
    {
        if (instance) {
            return instance->EvalStencils(srcBuffer, srcDesc,
                                          dstBuffer, dstDesc,
                                          stencilTable,
                                          context);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              BufferDescriptor(),
                              BufferDescriptor(),
                              context);
            if (instance) {
                bool r = instance->EvalStencils(srcBuffer, srcDesc,
                                                dstBuffer, dstDesc,
                                                stencilTable,
                                                context);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic static stencil function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        transparently from OsdMesh template interface.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the dstBuffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       MTLBuffer interfaces.
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param context        used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    static bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        STENCIL_TABLE const *stencilTable,
        MTLComputeEvaluator const *instance,
        MTLContext* context) {

        if (instance) {
            return instance->EvalStencils(srcBuffer, srcDesc,
                                          dstBuffer, dstDesc,
                                          duBuffer,  duDesc,
                                          dvBuffer,  dvDesc,
                                          stencilTable,
                                          context);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc, duDesc, dvDesc, context);
            if (instance) {
                bool r = instance->EvalStencils(srcBuffer, srcDesc,
                                                dstBuffer, dstDesc,
                                                duBuffer,  duDesc,
                                                dvBuffer,  dvDesc,
                                                stencilTable,
                                                context);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic static stencil function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        transparently from OsdMesh template interface.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the dstBuffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       MTLBuffer interfaces.
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param context        used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
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
        MTLComputeEvaluator const *instance,
        MTLContext* context) {

        if (instance) {
            return instance->EvalStencils(srcBuffer, srcDesc,
                                          dstBuffer, dstDesc,
                                          duBuffer,  duDesc,
                                          dvBuffer,  dvDesc,
                                          duuBuffer, duuDesc,
                                          duvBuffer, duvDesc,
                                          dvvBuffer, dvvDesc,
                                          stencilTable,
                                          context);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc, duDesc, dvDesc,
                              duuDesc, duvDesc, dvvDesc, context);
            if (instance) {
                bool r = instance->EvalStencils(srcBuffer, srcDesc,
                                                dstBuffer, dstDesc,
                                                duBuffer,  duDesc,
                                                dvBuffer,  dvDesc,
                                                duuBuffer, duuDesc,
                                                duvBuffer, duvDesc,
                                                dvvBuffer, dvvDesc,
                                                stencilTable,
                                                context);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic stencil function.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       MTLBuffer interfaces.
    ///
    /// @param context        used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    bool EvalStencils(
                      SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
                      DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
                      STENCIL_TABLE const *stencilTable,
                      MTLContext* context) const
    {
        return EvalStencils(srcBuffer->BindMTLBuffer(context), srcDesc,
                            dstBuffer->BindMTLBuffer(context), dstDesc,
                            0, BufferDescriptor(),
                            0, BufferDescriptor(),
                            stencilTable->GetSizesBuffer(),
                            stencilTable->GetOffsetsBuffer(),
                            stencilTable->GetIndicesBuffer(),
                            stencilTable->GetWeightsBuffer(),
                            0,
                            0,
                            /* start = */ 0,
                            /* end   = */ stencilTable->GetNumStencils(),
                            context);
    }

    /// \brief Generic stencil function.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the dstBuffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       MTLBuffer interfaces.
    ///
    /// @param context        used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        STENCIL_TABLE const *stencilTable,
        MTLContext* context) const
    {
        return EvalStencils(srcBuffer->BindMTLBuffer(context), srcDesc,
                            dstBuffer->BindMTLBuffer(context), dstDesc,
                            duBuffer->BindMTLBuffer(context),  duDesc,
                            dvBuffer->BindMTLBuffer(context),  dvDesc,
                            stencilTable->GetSizesBuffer(),
                            stencilTable->GetOffsetsBuffer(),
                            stencilTable->GetIndicesBuffer(),
                            stencilTable->GetWeightsBuffer(),
                            stencilTable->GetDuWeightsBuffer(),
                            stencilTable->GetDvWeightsBuffer(),
                            /* start = */ 0,
                            /* end   = */ stencilTable->GetNumStencils(),
                            context);
    }

    /// \brief Generic stencil function.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the dstBuffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duuDesc         vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       MTLBuffer interfaces.
    ///
    /// @param context        used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        DST_BUFFER *duuBuffer, BufferDescriptor const &duuDesc,
        DST_BUFFER *duvBuffer, BufferDescriptor const &duvDesc,
        DST_BUFFER *dvvBuffer, BufferDescriptor const &dvvDesc,
        STENCIL_TABLE const *stencilTable,
        MTLContext* context) const
    {
        return EvalStencils(srcBuffer->BindMTLBuffer(context), srcDesc,
                            dstBuffer->BindMTLBuffer(context), dstDesc,
                            duBuffer->BindMTLBuffer(context),  duDesc,
                            dvBuffer->BindMTLBuffer(context),  dvDesc,
                            duuBuffer->BindMTLBuffer(context), duuDesc,
                            duvBuffer->BindMTLBuffer(context), duvDesc,
                            dvvBuffer->BindMTLBuffer(context), dvvDesc,
                            stencilTable->GetSizesBuffer(),
                            stencilTable->GetOffsetsBuffer(),
                            stencilTable->GetIndicesBuffer(),
                            stencilTable->GetWeightsBuffer(),
                            stencilTable->GetDuWeightsBuffer(),
                            stencilTable->GetDvWeightsBuffer(),
                            stencilTable->GetDuuWeightsBuffer(),
                            stencilTable->GetDuvWeightsBuffer(),
                            stencilTable->GetDvvWeightsBuffer(),
                            /* start = */ 0,
                            /* end   = */ stencilTable->GetNumStencils(),
                            context);
    }

    /// \brief Dispatch the MTL compute kernel on GPU asynchronously
    /// returns false if the kernel hasn't been compiled yet.
    ///
    /// @param srcBuffer        MTLBuffer of input primvar source data
    ///
    /// @param srcDesc          vertex buffer descriptor for the srcBuffer
    ///
    /// @param dstBuffer        MTLBuffer of output primvar destination data
    ///
    /// @param dstDesc          vertex buffer descriptor for the dstBuffer
    ///
    /// @param duBuffer         MTLBuffer of output derivative wrt u
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         MTLBuffer of output derivative wrt v
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param sizesBuffer      MTLBuffer of the sizes in the stencil table
    ///
    /// @param offsetsBuffer    MTLBuffer of the offsets in the stencil table
    ///
    /// @param indicesBuffer    MTLBuffer of the indices in the stencil table
    ///
    /// @param weightsBuffer    MTLBuffer of the weights in the stencil table
    ///
    /// @param duWeightsBuffer  MTLBuffer of the du weights in the stencil table
    ///
    /// @param dvWeightsBuffer  MTLBuffer of the dv weights in the stencil table
    ///
    /// @param start            start index of stencil table
    ///
    /// @param end              end index of stencil table
    ///
    /// @param context          used to obtain the MTLDevice object and command queue
    ///                         to obtain command buffers from.
    ///
    bool EvalStencils(id<MTLBuffer> srcBuffer, BufferDescriptor const &srcDesc,
                      id<MTLBuffer> dstBuffer, BufferDescriptor const &dstDesc,
                      id<MTLBuffer> duBuffer,  BufferDescriptor const &duDesc,
                      id<MTLBuffer> dvBuffer,  BufferDescriptor const &dvDesc,
                      id<MTLBuffer> sizesBuffer,
                      id<MTLBuffer> offsetsBuffer,
                      id<MTLBuffer> indicesBuffer,
                      id<MTLBuffer> weightsBuffer,
                      id<MTLBuffer> duWeightsBuffer,
                      id<MTLBuffer> dvWeightsBuffer,
                      int start,
                      int end,
                      MTLContext* context) const;

    /// \brief Dispatch the MTL compute kernel on GPU asynchronously
    /// returns false if the kernel hasn't been compiled yet.
    ///
    /// @param srcBuffer        MTLBuffer of input primvar source data
    ///
    /// @param srcDesc          vertex buffer descriptor for the srcBuffer
    ///
    /// @param dstBuffer        MTLBuffer of output primvar destination data
    ///
    /// @param dstDesc          vertex buffer descriptor for the dstBuffer
    ///
    /// @param duBuffer         MTLBuffer of output derivative wrt u
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         MTLBuffer of output derivative wrt v
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer        MTLBuffer of output 2nd derivative wrt u
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer        MTLBuffer of output 2nd derivative wrt u and v
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer        MTLBuffer of output 2nd derivative wrt v
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param sizesBuffer      MTLBuffer of the sizes in the stencil table
    ///
    /// @param offsetsBuffer    MTLBuffer of the offsets in the stencil table
    ///
    /// @param indicesBuffer    MTLBuffer of the indices in the stencil table
    ///
    /// @param weightsBuffer    MTLBuffer of the weights in the stencil table
    ///
    /// @param duWeightsBuffer  MTLBuffer of the du weights in the stencil table
    ///
    /// @param dvWeightsBuffer  MTLBuffer of the dv weights in the stencil table
    ///
    /// @param duuWeightsBuffer MTLBuffer of the duu weights in the stencil table
    ///
    /// @param duvWeightsBuffer MTLBuffer of the duv weights in the stencil table
    ///
    /// @param dvvWeightsBuffer MTLBuffer of the dvv weights in the stencil table
    ///
    /// @param start            start index of stencil table
    ///
    /// @param end              end index of stencil table
    ///
    /// @param context          used to obtain the MTLDevice object and command queue
    ///                         to obtain command buffers from.
    ///
    bool EvalStencils(id<MTLBuffer> srcBuffer, BufferDescriptor const &srcDesc,
                      id<MTLBuffer> dstBuffer, BufferDescriptor const &dstDesc,
                      id<MTLBuffer> duBuffer,  BufferDescriptor const &duDesc,
                      id<MTLBuffer> dvBuffer,  BufferDescriptor const &dvDesc,
                      id<MTLBuffer> duuBuffer, BufferDescriptor const &duuDesc,
                      id<MTLBuffer> duvBuffer, BufferDescriptor const &duvDesc,
                      id<MTLBuffer> dvvBuffer, BufferDescriptor const &dvvDesc,
                      id<MTLBuffer> sizesBuffer,
                      id<MTLBuffer> offsetsBuffer,
                      id<MTLBuffer> indicesBuffer,
                      id<MTLBuffer> weightsBuffer,
                      id<MTLBuffer> duWeightsBuffer,
                      id<MTLBuffer> dvWeightsBuffer,
                      id<MTLBuffer> duuWeightsBuffer,
                      id<MTLBuffer> duvWeightsBuffer,
                      id<MTLBuffer> dvvWeightsBuffer,
                      int start,
                      int end,
                      MTLContext* context) const;

    /// ----------------------------------------------------------------------
    ///
    ///   Limit evaluations with PatchTable
    ///
    /// ----------------------------------------------------------------------

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param context        used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatches(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        MTLComputeEvaluator const *instance,
        MTLContext* context) {

        if (instance) {
            return instance->EvalPatches(srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         context);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              BufferDescriptor(),
                              BufferDescriptor(),
                              context);
            if (instance) {
                bool r = instance->EvalPatches(srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               context);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param context        used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
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
        MTLComputeEvaluator* instance,
        MTLContext* context) {

        if (instance) {
            return instance->EvalPatches(srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         duBuffer, duDesc,
                                         dvBuffer, dvDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         context);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              duDesc, dvDesc, context);
            if (instance) {
                bool r = instance->EvalPatches(srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               duBuffer, duDesc,
                                               dvBuffer, dvDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               context);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param context        used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
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
        MTLComputeEvaluator* instance,
        MTLContext* context) {

        if (instance) {
            return instance->EvalPatches(srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         duBuffer, duDesc,
                                         dvBuffer, dvDesc,
                                         duuBuffer, duuDesc,
                                         duvBuffer, duvDesc,
                                         dvvBuffer, dvvDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         context);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              duDesc, dvDesc,
                              duuDesc, duvDesc, dvvDesc, context);
            if (instance) {
                bool r = instance->EvalPatches(srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               duBuffer, duDesc,
                                               dvBuffer, dvDesc,
                                               duuBuffer, duuDesc,
                                               duvBuffer, duvDesc,
                                               dvvBuffer, dvvDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               context);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBOBuffer() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param context        used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatches(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        MTLContext* context) const {

        return EvalPatches(srcBuffer->BindMTLBuffer(context), srcDesc,
                           dstBuffer->BindMTLBuffer(context), dstDesc,
                           0, BufferDescriptor(),
                           0, BufferDescriptor(),
                           numPatchCoords,
                           patchCoords->BindMTLBuffer(context),
                           patchTable->GetPatchArrays(),
                           patchTable->GetPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           context);
    }

    /// \brief Generic limit eval function with derivatives. This function has
    ///        a same signature as other device kernels have so that it can be
    ///        called in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindVBO() method returning an
    ///                         MTLBuffer object of source data
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindVBO() method returning an
    ///                         MTLBuffer object of destination data
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindVBO() method returning an
    ///                         MTLBuffer object of destination data
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindVBO() method returning an
    ///                         MTLBuffer object of destination data
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       MTLPatchTable or equivalent
    ///
    /// @param context          used to obtain the MTLDevice object and command queue
    ///                         to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatches(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        MTLContext* context) const {

        return EvalPatches(srcBuffer->BindMTLBuffer(context), srcDesc,
                           dstBuffer->BindMTLBuffer(context), dstDesc,
                           duBuffer->BindMTLBuffer(context),  duDesc,
                           dvBuffer->BindMTLBuffer(context),  dvDesc,
                           numPatchCoords,
                           patchCoords->BindMTLBuffer(context),
                           patchTable->GetPatchArrays(),
                           patchTable->GetPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           context);
    }

    /// \brief Generic limit eval function with derivatives. This function has
    ///        a same signature as other device kernels have so that it can be
    ///        called in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindVBO() method returning a
    ///                         MTLBuffer object of source data
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindVBO() method returning a
    ///                         MTLBuffer object of destination data
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindVBO() method returning a
    ///                         MTLBuffer object of destination data
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindVBO() method returning a
    ///                         MTLBuffer object of destination data
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindVBO() method returning a
    ///                         MTLBuffer object of destination data
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer        Output buffer 2nd derivative wrt u and v
    ///                         must have BindVBO() method returning a
    ///                         MTLBuffer object of destination data
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer        Output buffer 2nd derivative wrt v
    ///                         must have BindVBO() method returning a
    ///                         MTLBuffer object of destination data
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       MTLPatchTable or equivalent
    ///
    /// @param context          used to obtain the MTLDevice object and command queue
    ///                         to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatches(
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
        MTLContext* context) const {

        return EvalPatches(srcBuffer->BindMTLBuffer(context), srcDesc,
                           dstBuffer->BindMTLBuffer(context), dstDesc,
                           duBuffer->BindMTLBuffer(context),  duDesc,
                           dvBuffer->BindMTLBuffer(context),  dvDesc,
                           duuBuffer->BindMTLBuffer(context), duuDesc,
                           duvBuffer->BindMTLBuffer(context), duvDesc,
                           dvvBuffer->BindMTLBuffer(context), dvvDesc,
                           numPatchCoords,
                           patchCoords->BindMTLBuffer(context),
                           patchTable->GetPatchArrays(),
                           patchTable->GetPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           context);
    }

    bool EvalPatches(id<MTLBuffer> srcBuffer, BufferDescriptor const &srcDesc,
                     id<MTLBuffer> dstBuffer, BufferDescriptor const &dstDesc,
                     id<MTLBuffer> duBuffer,  BufferDescriptor const &duDesc,
                     id<MTLBuffer> dvBuffer,  BufferDescriptor const &dvDesc,
                     int numPatchCoords,
                     id<MTLBuffer> patchCoordsBuffer,
                     const PatchArrayVector &patchArrays,
                     id<MTLBuffer> patchIndexBuffer,
                     id<MTLBuffer> patchParamsBuffer,
                     MTLContext* context) const;

    bool EvalPatches(id<MTLBuffer> srcBuffer, BufferDescriptor const &srcDesc,
                     id<MTLBuffer> dstBuffer, BufferDescriptor const &dstDesc,
                     id<MTLBuffer> duBuffer,  BufferDescriptor const &duDesc,
                     id<MTLBuffer> dvBuffer,  BufferDescriptor const &dvDesc,
                     id<MTLBuffer> duuBuffer, BufferDescriptor const &duuDesc,
                     id<MTLBuffer> duvBuffer, BufferDescriptor const &duvDesc,
                     id<MTLBuffer> dvvBuffer, BufferDescriptor const &dvvDesc,
                     int numPatchCoords,
                     id<MTLBuffer> patchCoordsBuffer,
                     const PatchArrayVector &patchArrays,
                     id<MTLBuffer> patchIndexBuffer,
                     id<MTLBuffer> patchParamsBuffer,
                     MTLContext* context) const;

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    static bool EvalPatchesVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        MTLComputeEvaluator const *instance,
        MTLContext* deviceContext) {

        if (instance) {
            return instance->EvalPatchesVarying(
                                         srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         deviceContext);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              BufferDescriptor(),
                              BufferDescriptor(),
                              deviceContext);
            if (instance) {
                bool r = instance->EvalPatchesVarying(
                                               srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               deviceContext);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatchesVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        MTLContext* deviceContext) const {

        return EvalPatches(srcBuffer->BindMTLBuffer(deviceContext), srcDesc,
                           dstBuffer->BindMTLBuffer(deviceContext), dstDesc,
                           0, BufferDescriptor(),
                           0, BufferDescriptor(),
                           numPatchCoords,
                           patchCoords->BindMTLBuffer(deviceContext),
                           patchTable->GetVaryingPatchArrays(),
                           patchTable->GetVaryingPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           deviceContext
                           );
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
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
        MTLComputeEvaluator const *instance,
        MTLContext* deviceContext) {

        if (instance) {
            return instance->EvalPatchesVarying(
                                         srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         duBuffer, duDesc,
                                         dvBuffer, dvDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         deviceContext);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              duDesc, dvDesc,
                              deviceContext);
            if (instance) {
                bool r = instance->EvalPatchesVarying(
                                               srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               duBuffer, duDesc,
                                               dvBuffer, dvDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               deviceContext);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatchesVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        MTLContext* deviceContext) const {

        return EvalPatches(srcBuffer->BindMTLBuffer(deviceContext), srcDesc,
                           dstBuffer->BindMTLBuffer(deviceContext), dstDesc,
                           duBuffer->BindMTLBuffer(deviceContext),  duDesc,
                           dvBuffer->BindMTLBuffer(deviceContext),  dvDesc,
                           numPatchCoords,
                           patchCoords->BindMTLBuffer(deviceContext),
                           patchTable->GetVaryingPatchArrays(),
                           patchTable->GetVaryingPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           deviceContext
                           );
    }


    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvvDesc       vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
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
        MTLComputeEvaluator const *instance,
        MTLContext* deviceContext) {

        if (instance) {
            return instance->EvalPatchesVarying(
                                         srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         duBuffer, duDesc,
                                         dvBuffer, dvDesc,
                                         duuBuffer, duuDesc,
                                         duvBuffer, duvDesc,
                                         dvvBuffer, dvvDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         deviceContext);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              duDesc, dvDesc,
                              duuDesc, duvDesc, dvvDesc,
                              deviceContext);
            if (instance) {
                bool r = instance->EvalPatchesVarying(
                                               srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               duBuffer, duDesc,
                                               dvBuffer, dvDesc,
                                               duuBuffer, duuDesc,
                                               duvBuffer, duvDesc,
                                               dvvBuffer, dvvDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               deviceContext);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatchesVarying(
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
        MTLContext* deviceContext) const {

        return EvalPatches(srcBuffer->BindMTLBuffer(deviceContext), srcDesc,
                           dstBuffer->BindMTLBuffer(deviceContext), dstDesc,
                           duBuffer->BindMTLBuffer(deviceContext),  duDesc,
                           dvBuffer->BindMTLBuffer(deviceContext),  dvDesc,
                           duuBuffer->BindMTLBuffer(deviceContext), duuDesc,
                           duvBuffer->BindMTLBuffer(deviceContext), duvDesc,
                           dvvBuffer->BindMTLBuffer(deviceContext), dvvDesc,
                           numPatchCoords,
                           patchCoords->BindMTLBuffer(deviceContext),
                           patchTable->GetVaryingPatchArrays(),
                           patchTable->GetVaryingPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           deviceContext
                           );
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
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
        MTLComputeEvaluator const *instance,
        MTLContext* deviceContext) {

        if (instance) {
            return instance->EvalPatchesFaceVarying(
                                         srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         fvarChannel,
                                         deviceContext);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              BufferDescriptor(),
                              BufferDescriptor(),
                              deviceContext);
            if (instance) {
                bool r = instance->EvalPatchesFaceVarying(
                                               srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               fvarChannel,
                                               deviceContext);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatchesFaceVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        int fvarChannel,
        MTLContext* deviceContext) const {

        return EvalPatches(srcBuffer->BindMTLBuffer(deviceContext), srcDesc,
                           dstBuffer->BindMTLBuffer(deviceContext), dstDesc,
                           0, BufferDescriptor(),
                           0, BufferDescriptor(),
                           numPatchCoords,
                           patchCoords->BindMTLBuffer(deviceContext),
                           patchTable->GetFVarPatchArrays(fvarChannel),
                           patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           patchTable->GetFVarPatchParamBuffer(fvarChannel),
                           deviceContext
                           );
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
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
        MTLComputeEvaluator const *instance,
        MTLContext* deviceContext) {

        if (instance) {
            return instance->EvalPatchesFaceVarying(
                                         srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         duBuffer, duDesc,
                                         dvBuffer, dvDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         fvarChannel,
                                         deviceContext);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              duDesc, dvDesc,
                              deviceContext);
            if (instance) {
                bool r = instance->EvalPatchesFaceVarying(
                                               srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               duBuffer, duDesc,
                                               dvBuffer, dvDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               fvarChannel,
                                               deviceContext);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatchesFaceVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        int fvarChannel,
        MTLContext* deviceContext) const {

        return EvalPatches(srcBuffer->BindMTLBuffer(deviceContext), srcDesc,
                           dstBuffer->BindMTLBuffer(deviceContext), dstDesc,
                           duBuffer->BindMTLBuffer(deviceContext),  duDesc,
                           dvBuffer->BindMTLBuffer(deviceContext),  dvDesc,
                           numPatchCoords,
                           patchCoords->BindMTLBuffer(deviceContext),
                           patchTable->GetFVarPatchArrays(fvarChannel),
                           patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           patchTable->GetFVarPatchParamBuffer(fvarChannel),
                           deviceContext
                           );
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindVBO() method returning an
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvvDesc       vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
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
        MTLComputeEvaluator const *instance,
        MTLContext* deviceContext) {

        if (instance) {
            return instance->EvalPatchesFaceVarying(
                                         srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         duBuffer, duDesc,
                                         dvBuffer, dvDesc,
                                         duuBuffer, duuDesc,
                                         duvBuffer, duvDesc,
                                         dvvBuffer, dvvDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         fvarChannel,
                                         deviceContext);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              duDesc, dvDesc,
                              duuDesc, duvDesc, dvvDesc,
                              deviceContext);
            if (instance) {
                bool r = instance->EvalPatchesFaceVarying(
                                               srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               duBuffer, duDesc,
                                               dvBuffer, dvDesc,
                                               duuBuffer, duuDesc,
                                               duvBuffer, duvDesc,
                                               dvvBuffer, dvvDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               fvarChannel,
                                               deviceContext);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindVBO() method returning a
    ///                       MTLBuffer object of destination data
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindVBO() method returning an
    ///                       array of PatchCoord struct in VBO.
    ///
    /// @param patchTable     MTLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param deviceContext  used to obtain the MTLDevice object and command queue
    ///                       to obtain command buffers from.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatchesFaceVarying(
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
        MTLContext* deviceContext) const {

        return EvalPatches(srcBuffer->BindMTLBuffer(deviceContext), srcDesc,
                           dstBuffer->BindMTLBuffer(deviceContext), dstDesc,
                           duBuffer->BindMTLBuffer(deviceContext),  duDesc,
                           dvBuffer->BindMTLBuffer(deviceContext),  dvDesc,
                           duuBuffer->BindMTLBuffer(deviceContext), duuDesc,
                           duvBuffer->BindMTLBuffer(deviceContext), duvDesc,
                           dvvBuffer->BindMTLBuffer(deviceContext), dvvDesc,
                           numPatchCoords,
                           patchCoords->BindMTLBuffer(deviceContext),
                           patchTable->GetFVarPatchArrays(fvarChannel),
                           patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           patchTable->GetFVarPatchParamBuffer(fvarChannel),
                           fvarChannel,
                           deviceContext
                           );
    }

    /// Configure compute pipline state. Returns false if it fails to create the pipeline state.
    bool Compile(BufferDescriptor const &srcDesc,
                 BufferDescriptor const &dstDesc,
                 BufferDescriptor const &duDesc,
                 BufferDescriptor const &dvDesc,
                 BufferDescriptor const &duuDesc,
                 BufferDescriptor const &duvDesc,
                 BufferDescriptor const &dvvDesc,
                 MTLContext* context);

    /// Wait for the dispatched kernel to finish.
    static void Synchronize(MTLContext* context);

    private:

    id<MTLLibrary> _computeLibrary;
    id<MTLComputePipelineState> _evalStencils;
    id<MTLComputePipelineState> _evalPatches;
    id<MTLBuffer> _parameterBuffer;

    int _workGroupSize;
};

} //end namespace Osd

} //end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} //end namespace OpenSubdiv

#endif // OPENSUBDIV3_OSD_MTL_COMPUTE_EVALUATOR_H
