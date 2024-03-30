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

#ifndef OPENSUBDIV3_OSD_CL_EVALUATOR_H
#define OPENSUBDIV3_OSD_CL_EVALUATOR_H

#include "../version.h"

#include "../osd/opencl.h"
#include "../osd/types.h"
#include "../osd/bufferDescriptor.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {
    class PatchTable;
    class StencilTable;
    class LimitStencilTable;
}

namespace Osd {

/// \brief OpenCL stencil table
///
/// This class is an OpenCL buffer representation of Far::StencilTable.
///
/// CLCompute consumes this table to apply stencils
///
///
class CLStencilTable {
public:
    template <typename DEVICE_CONTEXT>
    static CLStencilTable *Create(Far::StencilTable const *stencilTable,
                                  DEVICE_CONTEXT context) {
        return new CLStencilTable(stencilTable, context->GetContext());
    }

    template <typename DEVICE_CONTEXT>
    static CLStencilTable *Create(
        Far::LimitStencilTable const *limitStencilTable,
        DEVICE_CONTEXT context) {
        return new CLStencilTable(limitStencilTable, context->GetContext());
    }

    CLStencilTable(Far::StencilTable const *stencilTable,
                   cl_context clContext);
    CLStencilTable(Far::LimitStencilTable const *limitStencilTable,
                   cl_context clContext);
    ~CLStencilTable();

    // interfaces needed for CLComputeKernel
    cl_mem GetSizesBuffer()      const { return _sizes; }
    cl_mem GetOffsetsBuffer()    const { return _offsets; }
    cl_mem GetIndicesBuffer()    const { return _indices; }
    cl_mem GetWeightsBuffer()    const { return _weights; }
    cl_mem GetDuWeightsBuffer()  const { return _duWeights; }
    cl_mem GetDvWeightsBuffer()  const { return _dvWeights; }
    cl_mem GetDuuWeightsBuffer() const { return _duuWeights; }
    cl_mem GetDuvWeightsBuffer() const { return _duvWeights; }
    cl_mem GetDvvWeightsBuffer() const { return _dvvWeights; }
    int GetNumStencils()         const { return _numStencils; }

private:
    cl_mem _sizes;
    cl_mem _offsets;
    cl_mem _indices;
    cl_mem _weights;
    cl_mem _duWeights;
    cl_mem _dvWeights;
    cl_mem _duuWeights;
    cl_mem _duvWeights;
    cl_mem _dvvWeights;
    int _numStencils;
};

// ---------------------------------------------------------------------------

class CLEvaluator {
public:
    typedef bool Instantiatable;

    /// Generic creator template.
    template <typename DEVICE_CONTEXT>
    static CLEvaluator *Create(BufferDescriptor const &srcDesc,
                               BufferDescriptor const &dstDesc,
                               BufferDescriptor const &duDesc,
                               BufferDescriptor const &dvDesc,
                               DEVICE_CONTEXT deviceContext) {
        return Create(srcDesc, dstDesc, duDesc, dvDesc,
                      deviceContext->GetContext(),
                      deviceContext->GetCommandQueue());
    }

    static CLEvaluator * Create(BufferDescriptor const &srcDesc,
                                BufferDescriptor const &dstDesc,
                                BufferDescriptor const &duDesc,
                                BufferDescriptor const &dvDesc,
                                cl_context clContext,
                                cl_command_queue clCommandQueue) {
        CLEvaluator *instance = new CLEvaluator(clContext, clCommandQueue);
        if (instance->Compile(srcDesc, dstDesc, duDesc, dvDesc))
            return instance;
        delete instance;
        return NULL;
    }

    /// Generic creator template.
    template <typename DEVICE_CONTEXT>
    static CLEvaluator *Create(BufferDescriptor const &srcDesc,
                               BufferDescriptor const &dstDesc,
                               BufferDescriptor const &duDesc,
                               BufferDescriptor const &dvDesc,
                               BufferDescriptor const &duuDesc,
                               BufferDescriptor const &duvDesc,
                               BufferDescriptor const &dvvDesc,
                               DEVICE_CONTEXT deviceContext) {
        return Create(srcDesc, dstDesc, duDesc, dvDesc,
                      duuDesc, duvDesc, dvvDesc,
                      deviceContext->GetContext(),
                      deviceContext->GetCommandQueue());
    }

    static CLEvaluator * Create(BufferDescriptor const &srcDesc,
                                BufferDescriptor const &dstDesc,
                                BufferDescriptor const &duDesc,
                                BufferDescriptor const &dvDesc,
                                BufferDescriptor const &duuDesc,
                                BufferDescriptor const &duvDesc,
                                BufferDescriptor const &dvvDesc,
                                cl_context clContext,
                                cl_command_queue clCommandQueue) {
        CLEvaluator *instance = new CLEvaluator(clContext, clCommandQueue);
        if (instance->Compile(srcDesc, dstDesc, duDesc, dvDesc,
                              duuDesc, duvDesc, dvvDesc))
            return instance;
        delete instance;
        return NULL;
    }

    /// Constructor.
    CLEvaluator(cl_context context, cl_command_queue queue);

    /// Destructor.
    ~CLEvaluator();

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
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for results to be written
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       SSBO interfaces.
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename STENCIL_TABLE, typename DEVICE_CONTEXT>
    static bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        STENCIL_TABLE const *stencilTable,
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

        if (instance) {
            return instance->EvalStencils(srcBuffer, srcDesc,
                                          dstBuffer, dstDesc,
                                          stencilTable,
                                          numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              BufferDescriptor(),
                              BufferDescriptor(),
                              deviceContext);
            if (instance) {
                bool r = instance->EvalStencils(srcBuffer, srcDesc,
                                                dstBuffer, dstDesc,
                                                stencilTable,
                                                numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for results to be written
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for du results to be written
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for dv results to be written
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       SSBO interfaces.
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename STENCIL_TABLE, typename DEVICE_CONTEXT>
    static bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        STENCIL_TABLE const *stencilTable,
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

        if (instance) {
            return instance->EvalStencils(srcBuffer, srcDesc,
                                          dstBuffer, dstDesc,
                                          duBuffer,  duDesc,
                                          dvBuffer,  dvDesc,
                                          stencilTable,
                                          numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc, duDesc, dvDesc,
                              deviceContext);
            if (instance) {
                bool r = instance->EvalStencils(srcBuffer, srcDesc,
                                                dstBuffer, dstDesc,
                                                duBuffer,  duDesc,
                                                dvBuffer,  dvDesc,
                                                stencilTable,
                                                numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for results to be written
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for du results to be written
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for dv results to be written
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for du results to be written
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for du results to be written
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for dv results to be written
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       SSBO interfaces.
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename STENCIL_TABLE, typename DEVICE_CONTEXT>
    static bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        DST_BUFFER *duuBuffer, BufferDescriptor const &duuDesc,
        DST_BUFFER *duvBuffer, BufferDescriptor const &duvDesc,
        DST_BUFFER *dvvBuffer, BufferDescriptor const &dvvDesc,
        STENCIL_TABLE const *stencilTable,
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

        if (instance) {
            return instance->EvalStencils(srcBuffer, srcDesc,
                                          dstBuffer, dstDesc,
                                          duBuffer,  duDesc,
                                          dvBuffer,  dvDesc,
                                          duuBuffer, duuDesc,
                                          duvBuffer, duvDesc,
                                          dvvBuffer, dvvDesc,
                                          stencilTable,
                                          numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            instance = Create(srcDesc, dstDesc,
                              duDesc, dvDesc,
                              duuDesc, duvDesc, dvvDesc,
                              deviceContext);
            if (instance) {
                bool r = instance->EvalStencils(srcBuffer, srcDesc,
                                                dstBuffer, dstDesc,
                                                duBuffer,  duDesc,
                                                dvBuffer,  dvDesc,
                                                duuBuffer, duuDesc,
                                                duvBuffer, duvDesc,
                                                dvvBuffer, dvvDesc,
                                                stencilTable,
                                                numStartEvents, startEvents, endEvent);
                delete instance;
                return r;
            }
            return false;
        }
    }

    /// \brief Generic stencil function.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for results to be written
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       SSBO interfaces.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        STENCIL_TABLE const *stencilTable,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {
        return EvalStencils(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                            dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                            stencilTable->GetSizesBuffer(),
                            stencilTable->GetOffsetsBuffer(),
                            stencilTable->GetIndicesBuffer(),
                            stencilTable->GetWeightsBuffer(),
                            0,
                            stencilTable->GetNumStencils(),
                            numStartEvents, startEvents, endEvent);
    }

    /// \brief Generic stencil function.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for results to be written
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for du results to be written
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for dv results to be written
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       SSBO interfaces.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER, typename STENCIL_TABLE>
    bool EvalStencils(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        STENCIL_TABLE const *stencilTable,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {
        return EvalStencils(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                            dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                            duBuffer->BindCLBuffer(_clCommandQueue), duDesc,
                            dvBuffer->BindCLBuffer(_clCommandQueue), dvDesc,
                            stencilTable->GetSizesBuffer(),
                            stencilTable->GetOffsetsBuffer(),
                            stencilTable->GetIndicesBuffer(),
                            stencilTable->GetWeightsBuffer(),
                            stencilTable->GetDuWeightsBuffer(),
                            stencilTable->GetDvWeightsBuffer(),
                            0,
                            stencilTable->GetNumStencils(),
                            numStartEvents, startEvents, endEvent);
    }

    /// \brief Generic stencil function.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for read
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for results to be written
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for du results to be written
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for dv results to be written
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for du results to be written
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for du results to be written
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindCLBuffer() method returning the
    ///                       cl_mem object for dv results to be written
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param stencilTable   stencil table to be applied. The table must have
    ///                       SSBO interfaces.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
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
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {
        return EvalStencils(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                            dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                            duBuffer->BindCLBuffer(_clCommandQueue), duDesc,
                            dvBuffer->BindCLBuffer(_clCommandQueue), dvDesc,
                            duuBuffer->BindCLBuffer(_clCommandQueue), duuDesc,
                            duvBuffer->BindCLBuffer(_clCommandQueue), duvDesc,
                            dvvBuffer->BindCLBuffer(_clCommandQueue), dvvDesc,
                            stencilTable->GetSizesBuffer(),
                            stencilTable->GetOffsetsBuffer(),
                            stencilTable->GetIndicesBuffer(),
                            stencilTable->GetWeightsBuffer(),
                            stencilTable->GetDuWeightsBuffer(),
                            stencilTable->GetDvWeightsBuffer(),
                            stencilTable->GetDuuWeightsBuffer(),
                            stencilTable->GetDuvWeightsBuffer(),
                            stencilTable->GetDvvWeightsBuffer(),
                            0,
                            stencilTable->GetNumStencils(),
                            numStartEvents, startEvents, endEvent);
    }

    /// Dispatch the CL compute kernel asynchronously.
    /// returns false if the kernel hasn't been compiled yet.
    bool EvalStencils(cl_mem src, BufferDescriptor const &srcDesc,
                      cl_mem dst, BufferDescriptor const &dstDesc,
                      cl_mem sizes,
                      cl_mem offsets,
                      cl_mem indices,
                      cl_mem weights,
                      int start,
                      int end,
                      unsigned int numStartEvents=0,
                      const cl_event* startEvents=NULL,
                      cl_event* endEvent=NULL) const;

    /// \brief Dispatch the CL compute kernel asynchronously.
    /// returns false if the kernel hasn't been compiled yet.
    ///
    /// @param src              CL buffer of input primvar source data
    ///
    /// @param srcDesc          vertex buffer descriptor for the srcBuffer
    ///
    /// @param dst              CL buffer of output primvar destination data
    ///
    /// @param dstDesc          vertex buffer descriptor for the dstBuffer
    ///
    /// @param du               CL buffer of output derivative wrt u
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dv               CL buffer of output derivative wrt v
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param sizes            CL buffer of the sizes in the stencil table
    ///
    /// @param offsets          CL buffer of the offsets in the stencil table
    ///
    /// @param indices          CL buffer of the indices in the stencil table
    ///
    /// @param weights          CL buffer of the weights in the stencil table
    ///
    /// @param duWeights        CL buffer of the du weights in the stencil table
    ///
    /// @param dvWeights        CL buffer of the dv weights in the stencil table
    ///
    /// @param start            start index of stencil table
    ///
    /// @param end              end index of stencil table
    ///
    /// @param numStartEvents   the number of events in the array pointed to by
    ///                         startEvents.
    ///
    /// @param startEvents      points to an array of cl_event which will determine
    ///                         when it is safe for the OpenCL device to begin work
    ///                         or NULL if it can begin immediately.
    ///
    /// @param endEvent         pointer to a cl_event which will receive a copy of
    ///                         the cl_event which indicates when all work for this
    ///                         call has completed.  This cl_event has an incremented
    ///                         reference count and should be released via
    ///                         clReleaseEvent().  NULL if not required.
    ///
    bool EvalStencils(cl_mem src, BufferDescriptor const &srcDesc,
                      cl_mem dst, BufferDescriptor const &dstDesc,
                      cl_mem du,  BufferDescriptor const &duDesc,
                      cl_mem dv,  BufferDescriptor const &dvDesc,
                      cl_mem sizes,
                      cl_mem offsets,
                      cl_mem indices,
                      cl_mem weights,
                      cl_mem duWeights,
                      cl_mem dvWeights,
                      int start,
                      int end,
                      unsigned int numStartEvents=0,
                      const cl_event* startEvents=NULL,
                      cl_event* endEvent=NULL) const;

    /// \brief Dispatch the CL compute kernel asynchronously.
    /// returns false if the kernel hasn't been compiled yet.
    ///
    /// @param src              CL buffer of input primvar source data
    ///
    /// @param srcDesc          vertex buffer descriptor for the srcBuffer
    ///
    /// @param dst              CL buffer of output primvar destination data
    ///
    /// @param dstDesc          vertex buffer descriptor for the dstBuffer
    ///
    /// @param du               CL buffer of output derivative wrt u
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dv               CL buffer of output derivative wrt v
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duu              CL buffer of output 2nd derivative wrt u
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duv              CL buffer of output 2nd derivative wrt u and v
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvv              CL buffer of output 2nd derivative wrt v
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param sizes            CL buffer of the sizes in the stencil table
    ///
    /// @param offsets          CL buffer of the offsets in the stencil table
    ///
    /// @param indices          CL buffer of the indices in the stencil table
    ///
    /// @param weights          CL buffer of the weights in the stencil table
    ///
    /// @param duWeights        CL buffer of the du weights in the stencil table
    ///
    /// @param dvWeights        CL buffer of the dv weights in the stencil table
    ///
    /// @param duuWeights       CL buffer of the duu weights in the stencil table
    ///
    /// @param duvWeights       CL buffer of the duv weights in the stencil table
    ///
    /// @param dvvWeights       CL buffer of the dvv weights in the stencil table
    ///
    /// @param start            start index of stencil table
    ///
    /// @param end              end index of stencil table
    ///
    /// @param numStartEvents   the number of events in the array pointed to by
    ///                         startEvents.
    ///
    /// @param startEvents      points to an array of cl_event which will determine
    ///                         when it is safe for the OpenCL device to begin work
    ///                         or NULL if it can begin immediately.
    ///
    /// @param endEvent         pointer to a cl_event which will receive a copy of
    ///                         the cl_event which indicates when all work for this
    ///                         call has completed.  This cl_event has an incremented
    ///                         reference count and should be released via
    ///                         clReleaseEvent().  NULL if not required.
    ///
    bool EvalStencils(cl_mem src, BufferDescriptor const &srcDesc,
                      cl_mem dst, BufferDescriptor const &dstDesc,
                      cl_mem du,  BufferDescriptor const &duDesc,
                      cl_mem dv,  BufferDescriptor const &dvDesc,
                      cl_mem duu, BufferDescriptor const &duuDesc,
                      cl_mem duv, BufferDescriptor const &duvDesc,
                      cl_mem dvv, BufferDescriptor const &dvvDesc,
                      cl_mem sizes,
                      cl_mem offsets,
                      cl_mem indices,
                      cl_mem weights,
                      cl_mem duWeights,
                      cl_mem dvWeights,
                      cl_mem duuWeights,
                      cl_mem duvWeights,
                      cl_mem dvvWeights,
                      int start,
                      int end,
                      unsigned int numStartEvents=0,
                      const cl_event* startEvents=NULL,
                      cl_event* endEvent=NULL) const;

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
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE,
              typename DEVICE_CONTEXT>
    static bool EvalPatches(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

        if (instance) {
            return instance->EvalPatches(srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            (void)deviceContext;  // unused
            instance = Create(srcDesc, dstDesc,
                              BufferDescriptor(),
                              BufferDescriptor(),
                              deviceContext);
            if (instance) {
                bool r = instance->EvalPatches(srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE,
              typename DEVICE_CONTEXT>
    static bool EvalPatches(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

        if (instance) {
            return instance->EvalPatches(srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         duBuffer, duDesc,
                                         dvBuffer, dvDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            (void)deviceContext;  // unused
            instance = Create(srcDesc, dstDesc, duDesc, dvDesc, deviceContext);
            if (instance) {
                bool r = instance->EvalPatches(srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               duBuffer, duDesc,
                                               dvBuffer, dvDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable,
                                               numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE,
              typename DEVICE_CONTEXT>
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
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

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
                                         numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            (void)deviceContext;  // unused
            instance = Create(srcDesc, dstDesc,
                              duDesc, dvDesc,
                              duuDesc, duvDesc, dvvDesc,
                              deviceContext);
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
                                               numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatches(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {

        return EvalPatches(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                           dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                           0, BufferDescriptor(),
                           0, BufferDescriptor(),
                           numPatchCoords,
                           patchCoords->BindCLBuffer(_clCommandQueue),
                           patchTable->GetPatchArrayBuffer(),
                           patchTable->GetPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           numStartEvents, startEvents, endEvent);
    }

    /// \brief Generic limit eval function with derivatives. This function has
    ///        a same signature as other device kernels have so that it can be
    ///        called in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of source data
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of destination data
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of destination data
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of destination data
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CLPatchTable or equivalent
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
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
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {

        return EvalPatches(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                           dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                           duBuffer->BindCLBuffer(_clCommandQueue),  duDesc,
                           dvBuffer->BindCLBuffer(_clCommandQueue),  dvDesc,
                           numPatchCoords,
                           patchCoords->BindCLBuffer(_clCommandQueue),
                           patchTable->GetPatchArrayBuffer(),
                           patchTable->GetPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           numStartEvents, startEvents, endEvent);
    }

    /// \brief Generic limit eval function with derivatives. This function has
    ///        a same signature as other device kernels have so that it can be
    ///        called in the same way.
    ///
    /// @param srcBuffer        Input primvar buffer.
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of source data
    ///
    /// @param srcDesc          vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer        Output primvar buffer
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of destination data
    ///
    /// @param dstDesc          vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer         Output buffer derivative wrt u
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of destination data
    ///
    /// @param duDesc           vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer         Output buffer derivative wrt v
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of destination data
    ///
    /// @param dvDesc           vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer        Output buffer 2nd derivative wrt u
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of destination data
    ///
    /// @param duuDesc          vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer        Output buffer 2nd derivative wrt u and v
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of destination data
    ///
    /// @param duvDesc          vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer        Output buffer 2nd derivative wrt v
    ///                         must have BindCLBuffer() method returning a CL
    ///                         buffer object of destination data
    ///
    /// @param dvvDesc          vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords   number of patchCoords.
    ///
    /// @param patchCoords      array of locations to be evaluated.
    ///
    /// @param patchTable       CLPatchTable or equivalent
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
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
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {

        return EvalPatches(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                           dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                           duBuffer->BindCLBuffer(_clCommandQueue),  duDesc,
                           dvBuffer->BindCLBuffer(_clCommandQueue),  dvDesc,
                           duuBuffer->BindCLBuffer(_clCommandQueue), duuDesc,
                           duvBuffer->BindCLBuffer(_clCommandQueue), duvDesc,
                           dvvBuffer->BindCLBuffer(_clCommandQueue), dvvDesc,
                           numPatchCoords,
                           patchCoords->BindCLBuffer(_clCommandQueue),
                           patchTable->GetPatchArrayBuffer(),
                           patchTable->GetPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           numStartEvents, startEvents, endEvent);
    }

    bool EvalPatches(cl_mem src, BufferDescriptor const &srcDesc,
                     cl_mem dst, BufferDescriptor const &dstDesc,
                     cl_mem du,  BufferDescriptor const &duDesc,
                     cl_mem dv,  BufferDescriptor const &dvDesc,
                     int numPatchCoords,
                     cl_mem patchCoordsBuffer,
                     cl_mem patchArrayBuffer,
                     cl_mem patchIndexBuffer,
                     cl_mem patchParamsBuffer,
                     unsigned int numStartEvents=0,
                     const cl_event* startEvents=NULL,
                     cl_event* endEvent=NULL) const;

    bool EvalPatches(cl_mem src, BufferDescriptor const &srcDesc,
                     cl_mem dst, BufferDescriptor const &dstDesc,
                     cl_mem du,  BufferDescriptor const &duDesc,
                     cl_mem dv,  BufferDescriptor const &dvDesc,
                     cl_mem duu, BufferDescriptor const &duuDesc,
                     cl_mem duv, BufferDescriptor const &duvDesc,
                     cl_mem dvv, BufferDescriptor const &dvvDesc,
                     int numPatchCoords,
                     cl_mem patchCoordsBuffer,
                     cl_mem patchArrayBuffer,
                     cl_mem patchIndexBuffer,
                     cl_mem patchParamsBuffer,
                     unsigned int numStartEvents=0,
                     const cl_event* startEvents=NULL,
                     cl_event* endEvent=NULL) const;

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE,
              typename DEVICE_CONTEXT>
    static bool EvalPatchesVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

        if (instance) {
            return instance->EvalPatchesVarying(
                                         srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            (void)deviceContext;  // unused
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
                                               numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatchesVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {

        return EvalPatches(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                           dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                           0, BufferDescriptor(),
                           0, BufferDescriptor(),
                           numPatchCoords,
                           patchCoords->BindCLBuffer(_clCommandQueue),
                           patchTable->GetVaryingPatchArrayBuffer(),
                           patchTable->GetVaryingPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           numStartEvents, startEvents, endEvent);
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE,
              typename DEVICE_CONTEXT>
    static bool EvalPatchesVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

        if (instance) {
            return instance->EvalPatchesVarying(
                                         srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         duBuffer, duDesc,
                                         dvBuffer, dvDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable,
                                         numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            (void)deviceContext;  // unused
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
                                               numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
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
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {

        return EvalPatches(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                           dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                           duBuffer->BindCLBuffer(_clCommandQueue), duDesc,
                           dvBuffer->BindCLBuffer(_clCommandQueue), dvDesc,
                           numPatchCoords,
                           patchCoords->BindCLBuffer(_clCommandQueue),
                           patchTable->GetVaryingPatchArrayBuffer(),
                           patchTable->GetVaryingPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           numStartEvents, startEvents, endEvent);
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE,
              typename DEVICE_CONTEXT>
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
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

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
                                         numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            (void)deviceContext;  // unused
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
                                               numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
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
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {

        return EvalPatches(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                           dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                           duBuffer->BindCLBuffer(_clCommandQueue), duDesc,
                           dvBuffer->BindCLBuffer(_clCommandQueue), dvDesc,
                           duuBuffer->BindCLBuffer(_clCommandQueue), duuDesc,
                           duvBuffer->BindCLBuffer(_clCommandQueue), duvDesc,
                           dvvBuffer->BindCLBuffer(_clCommandQueue), dvvDesc,
                           numPatchCoords,
                           patchCoords->BindCLBuffer(_clCommandQueue),
                           patchTable->GetVaryingPatchArrayBuffer(),
                           patchTable->GetVaryingPatchIndexBuffer(),
                           patchTable->GetPatchParamBuffer(),
                           numStartEvents, startEvents, endEvent);
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE,
              typename DEVICE_CONTEXT>
    static bool EvalPatchesFaceVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        int fvarChannel,
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

        if (instance) {
            return instance->EvalPatchesFaceVarying(
                                         srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable, fvarChannel,
                                         numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            (void)deviceContext;  // unused
            instance = Create(srcDesc, dstDesc,
                              BufferDescriptor(),
                              BufferDescriptor(),
                              deviceContext);
            if (instance) {
                bool r = instance->EvalPatchesFaceVarying(
                                               srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable, fvarChannel,
                                               numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE>
    bool EvalPatchesFaceVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        int fvarChannel = 0,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {

        return EvalPatches(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                           dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                           0, BufferDescriptor(),
                           0, BufferDescriptor(),
                           numPatchCoords,
                           patchCoords->BindCLBuffer(_clCommandQueue),
                           patchTable->GetFVarPatchArrayBuffer(fvarChannel),
                           patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           patchTable->GetFVarPatchParamBuffer(fvarChannel),
                           numStartEvents, startEvents, endEvent);
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE,
              typename DEVICE_CONTEXT>
    static bool EvalPatchesFaceVarying(
        SRC_BUFFER *srcBuffer, BufferDescriptor const &srcDesc,
        DST_BUFFER *dstBuffer, BufferDescriptor const &dstDesc,
        DST_BUFFER *duBuffer,  BufferDescriptor const &duDesc,
        DST_BUFFER *dvBuffer,  BufferDescriptor const &dvDesc,
        int numPatchCoords,
        PATCHCOORD_BUFFER *patchCoords,
        PATCH_TABLE *patchTable,
        int fvarChannel,
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

        if (instance) {
            return instance->EvalPatchesFaceVarying(
                                         srcBuffer, srcDesc,
                                         dstBuffer, dstDesc,
                                         duBuffer, duDesc,
                                         dvBuffer, dvDesc,
                                         numPatchCoords, patchCoords,
                                         patchTable, fvarChannel,
                                         numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            (void)deviceContext;  // unused
            instance = Create(srcDesc, dstDesc,
                              duDesc, dvDesc, deviceContext);
            if (instance) {
                bool r = instance->EvalPatchesFaceVarying(
                                               srcBuffer, srcDesc,
                                               dstBuffer, dstDesc,
                                               duBuffer, duDesc,
                                               dvBuffer, dvDesc,
                                               numPatchCoords, patchCoords,
                                               patchTable, fvarChannel,
                                               numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
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
        int fvarChannel = 0,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {

        return EvalPatches(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                           dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                           duBuffer->BindCLBuffer(_clCommandQueue), duDesc,
                           dvBuffer->BindCLBuffer(_clCommandQueue), dvDesc,
                           numPatchCoords,
                           patchCoords->BindCLBuffer(_clCommandQueue),
                           patchTable->GetFVarPatchArrayBuffer(fvarChannel),
                           patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           patchTable->GetFVarPatchParamBuffer(fvarChannel),
                           numStartEvents, startEvents, endEvent);
    }

    /// \brief Generic limit eval function. This function has a same
    ///        signature as other device kernels have so that it can be called
    ///        in the same way.
    ///
    /// @param srcBuffer      Input primvar buffer.
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param instance       cached compiled instance. Clients are supposed to
    ///                       pre-compile an instance of this class and provide
    ///                       to this function. If it's null the kernel still
    ///                       compute by instantiating on-demand kernel although
    ///                       it may cause a performance problem.
    ///
    /// @param deviceContext  client providing context class which supports
    ///                         cL_context GetContext()
    ///                         cl_command_queue GetCommandQueue()
    ///                       methods.
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
    ///
    template <typename SRC_BUFFER, typename DST_BUFFER,
              typename PATCHCOORD_BUFFER, typename PATCH_TABLE,
              typename DEVICE_CONTEXT>
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
        CLEvaluator const *instance,
        DEVICE_CONTEXT deviceContext,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) {

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
                                         patchTable, fvarChannel,
                                         numStartEvents, startEvents, endEvent);
        } else {
            // Create an instance on demand (slow)
            (void)deviceContext;  // unused
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
                                               patchTable, fvarChannel,
                                               numStartEvents, startEvents, endEvent);
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
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of source data
    ///
    /// @param srcDesc        vertex buffer descriptor for the input buffer
    ///
    /// @param dstBuffer      Output primvar buffer
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dstDesc        vertex buffer descriptor for the output buffer
    ///
    /// @param duBuffer       Output buffer derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duDesc         vertex buffer descriptor for the duBuffer
    ///
    /// @param dvBuffer       Output buffer derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvDesc         vertex buffer descriptor for the dvBuffer
    ///
    /// @param duuBuffer      Output buffer 2nd derivative wrt u
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duuDesc        vertex buffer descriptor for the duuBuffer
    ///
    /// @param duvBuffer      Output buffer 2nd derivative wrt u and v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param duvDesc        vertex buffer descriptor for the duvBuffer
    ///
    /// @param dvvBuffer      Output buffer 2nd derivative wrt v
    ///                       must have BindCLBuffer() method returning a CL
    ///                       buffer object of destination data
    ///
    /// @param dvvDesc        vertex buffer descriptor for the dvvBuffer
    ///
    /// @param numPatchCoords number of patchCoords.
    ///
    /// @param patchCoords    array of locations to be evaluated.
    ///                       must have BindCLBuffer() method returning an
    ///                       array of PatchCoord struct.
    ///
    /// @param patchTable     CLPatchTable or equivalent
    ///
    /// @param fvarChannel    face-varying channel
    ///
    /// @param numStartEvents the number of events in the array pointed to by
    ///                       startEvents.
    ///
    /// @param startEvents    points to an array of cl_event which will determine
    ///                       when it is safe for the OpenCL device to begin work
    ///                       or NULL if it can begin immediately.
    ///
    /// @param endEvent       pointer to a cl_event which will receive a copy of
    ///                       the cl_event which indicates when all work for this
    ///                       call has completed.  This cl_event has an incremented
    ///                       reference count and should be released via
    ///                       clReleaseEvent().  NULL if not required.
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
        int fvarChannel = 0,
        unsigned int numStartEvents=0,
        const cl_event* startEvents=NULL,
        cl_event* endEvent=NULL) const {

        return EvalPatches(srcBuffer->BindCLBuffer(_clCommandQueue), srcDesc,
                           dstBuffer->BindCLBuffer(_clCommandQueue), dstDesc,
                           duBuffer->BindCLBuffer(_clCommandQueue), duDesc,
                           dvBuffer->BindCLBuffer(_clCommandQueue), dvDesc,
                           duuBuffer->BindCLBuffer(_clCommandQueue), duuDesc,
                           duvBuffer->BindCLBuffer(_clCommandQueue), duvDesc,
                           dvvBuffer->BindCLBuffer(_clCommandQueue), dvvDesc,
                           numPatchCoords,
                           patchCoords->BindCLBuffer(_clCommandQueue),
                           patchTable->GetFVarPatchArrayBuffer(fvarChannel),
                           patchTable->GetFVarPatchIndexBuffer(fvarChannel),
                           patchTable->GetFVarPatchParamBuffer(fvarChannel),
                           numStartEvents, startEvents, endEvent);
    }

    /// ----------------------------------------------------------------------
    ///
    ///   Other methods
    ///
    /// ----------------------------------------------------------------------

    /// Configure OpenCL kernel.
    /// Returns false if it fails to compile the kernel.
    bool Compile(BufferDescriptor const &srcDesc,
                 BufferDescriptor const &dstDesc,
                 BufferDescriptor const &duDesc = BufferDescriptor(),
                 BufferDescriptor const &dvDesc = BufferDescriptor(),
                 BufferDescriptor const &duuDesc = BufferDescriptor(),
                 BufferDescriptor const &duvDesc = BufferDescriptor(),
                 BufferDescriptor const &dvvDesc = BufferDescriptor());

    /// Wait the OpenCL kernels finish.
    template <typename DEVICE_CONTEXT>
    static void Synchronize(DEVICE_CONTEXT deviceContext) {
        Synchronize(deviceContext->GetCommandQueue());
    }

    static void Synchronize(cl_command_queue queue);

private:
    cl_context _clContext;
    cl_command_queue _clCommandQueue;
    cl_program _program;
    cl_kernel _stencilKernel;
    cl_kernel _stencilDerivKernel;
    cl_kernel _patchKernel;
};

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv


#endif  // OPENSUBDIV3_OSD_CL_EVALUATOR_H
