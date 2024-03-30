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

#include "../osd/clEvaluator.h"

#include <sstream>
#include <string>
#include <vector>
#include <cstdio>

#include "../osd/opencl.h"
#include "../far/error.h"
#include "../far/stencilTable.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

static const char *clSource =
#include "clKernel.gen.h"
;
static const char *patchBasisTypesSource =
#include "patchBasisCommonTypes.gen.h"
;
static const char *patchBasisSource =
#include "patchBasisCommon.gen.h"
;
static const char *patchBasisEvalSource =
#include "patchBasisCommonEval.gen.h"
;

// ----------------------------------------------------------------------------

template <class T> cl_mem
createCLBuffer(std::vector<T> const & src, cl_context clContext) {
    if (src.empty()) {
        return NULL;
    }

    cl_int errNum = 0;
    cl_mem devicePtr = clCreateBuffer(clContext,
                                      CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
                                      src.size()*sizeof(T),
                                      (void*)(&src.at(0)),
                                      &errNum);

    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "clCreateBuffer: %d", errNum);
    }

    return devicePtr;
}

// ----------------------------------------------------------------------------

CLStencilTable::CLStencilTable(Far::StencilTable const *stencilTable,
                               cl_context clContext) {
    _numStencils = stencilTable->GetNumStencils();

    if (_numStencils > 0) {
        _sizes   = createCLBuffer(stencilTable->GetSizes(), clContext);
        _offsets = createCLBuffer(stencilTable->GetOffsets(), clContext);
        _indices = createCLBuffer(stencilTable->GetControlIndices(),
                                  clContext);
        _weights = createCLBuffer(stencilTable->GetWeights(), clContext);
        _duWeights = _dvWeights = NULL;
        _duuWeights = _duvWeights = _dvvWeights = NULL;
    } else {
        _sizes = _offsets = _indices = _weights = NULL;
        _duWeights = _dvWeights = NULL;
        _duuWeights = _duvWeights = _dvvWeights = NULL;
    }
}

CLStencilTable::CLStencilTable(Far::LimitStencilTable const *limitStencilTable,
                               cl_context clContext) {
    _numStencils = limitStencilTable->GetNumStencils();

    if (_numStencils > 0) {
        _sizes   = createCLBuffer(limitStencilTable->GetSizes(), clContext);
        _offsets = createCLBuffer(limitStencilTable->GetOffsets(), clContext);
        _indices = createCLBuffer(limitStencilTable->GetControlIndices(),
                                  clContext);
        _weights = createCLBuffer(limitStencilTable->GetWeights(), clContext);
        _duWeights = createCLBuffer(
            limitStencilTable->GetDuWeights(), clContext);
        _dvWeights = createCLBuffer(
            limitStencilTable->GetDvWeights(), clContext);
        _duuWeights = createCLBuffer(
            limitStencilTable->GetDuuWeights(), clContext);
        _duvWeights = createCLBuffer(
            limitStencilTable->GetDuvWeights(), clContext);
        _dvvWeights = createCLBuffer(
            limitStencilTable->GetDvvWeights(), clContext);
    } else {
        _sizes = _offsets = _indices = _weights = NULL;
        _duWeights = _dvWeights = NULL;
        _duuWeights = _duvWeights = _dvvWeights = NULL;
    }
}

CLStencilTable::~CLStencilTable() {
    if (_sizes)   clReleaseMemObject(_sizes);
    if (_offsets) clReleaseMemObject(_offsets);
    if (_indices) clReleaseMemObject(_indices);
    if (_weights) clReleaseMemObject(_weights);
    if (_duWeights) clReleaseMemObject(_duWeights);
    if (_dvWeights) clReleaseMemObject(_dvWeights);
    if (_duuWeights) clReleaseMemObject(_duuWeights);
    if (_duvWeights) clReleaseMemObject(_duvWeights);
    if (_dvvWeights) clReleaseMemObject(_dvvWeights);
}

// ---------------------------------------------------------------------------

CLEvaluator::CLEvaluator(cl_context context, cl_command_queue queue)
    : _clContext(context), _clCommandQueue(queue),
      _program(NULL), _stencilKernel(NULL), _stencilDerivKernel(NULL),
      _patchKernel(NULL) {
}

CLEvaluator::~CLEvaluator() {
    if (_stencilKernel) clReleaseKernel(_stencilKernel);
    if (_stencilDerivKernel) clReleaseKernel(_stencilDerivKernel);
    if (_patchKernel) clReleaseKernel(_patchKernel);
    if (_program) clReleaseProgram(_program);
}

bool
CLEvaluator::Compile(BufferDescriptor const &srcDesc,
                     BufferDescriptor const &dstDesc,
                     BufferDescriptor const & /*duDesc*/,
                     BufferDescriptor const & /*dvDesc*/,
                     BufferDescriptor const & /*duuDesc*/,
                     BufferDescriptor const & /*duvDesc*/,
                     BufferDescriptor const & /*dvvDesc*/) {
    if (srcDesc.length > dstDesc.length) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "srcDesc length must be less than or equal to "
                   "dstDesc length.\n");
        return false;
    }

    cl_int errNum;

    std::ostringstream defines;
    defines << "#define LENGTH "     << srcDesc.length << "\n"
            << "#define SRC_STRIDE " << srcDesc.stride << "\n"
            << "#define DST_STRIDE " << dstDesc.stride << "\n"
            << "#define OSD_PATCH_BASIS_OPENCL\n";
    std::string defineStr = defines.str();

    const char *sources[] = { defineStr.c_str(),
                              patchBasisTypesSource,
                              patchBasisSource,
                              patchBasisEvalSource,
                              clSource };
    _program = clCreateProgramWithSource(_clContext, 5, sources, 0, &errNum);
    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "clCreateProgramWithSource (%d)", errNum);
    }

    errNum = clBuildProgram(_program, 0, NULL, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "clBuildProgram (%d) \n", errNum);

        cl_int numDevices = 0;
        clGetContextInfo(
            _clContext, CL_CONTEXT_NUM_DEVICES,
            sizeof(cl_uint), &numDevices, NULL);

        cl_device_id *devices = new cl_device_id[numDevices];
        clGetContextInfo(_clContext, CL_CONTEXT_DEVICES,
                         sizeof(cl_device_id)*numDevices, devices, NULL);

        for (int i = 0; i < numDevices; ++i) {
            char cBuildLog[10240];
            clGetProgramBuildInfo(
                _program, devices[i],
                CL_PROGRAM_BUILD_LOG, sizeof(cBuildLog), cBuildLog, NULL);
            Far::Error(Far::FAR_RUNTIME_ERROR, cBuildLog);
        }
        delete[] devices;

        return false;
    }

    _stencilKernel = clCreateKernel(_program, "computeStencils", &errNum);
    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "buildKernel (%d)\n", errNum);
        return false;
    }

    _stencilDerivKernel = clCreateKernel(_program,
                                         "computeStencilsDerivatives", &errNum);
    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "buildKernel (%d)\n", errNum);
        return false;
    }

    _patchKernel = clCreateKernel(_program, "computePatches", &errNum);

    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "buildKernel (%d)\n", errNum);
        return false;
    }
    return true;
}

bool
CLEvaluator::EvalStencils(cl_mem src, BufferDescriptor const &srcDesc,
                          cl_mem dst, BufferDescriptor const &dstDesc,
                          cl_mem sizes,
                          cl_mem offsets,
                          cl_mem indices,
                          cl_mem weights,
                          int start, int end,
                          unsigned int numStartEvents,
                          const cl_event* startEvents,
                          cl_event* endEvent) const {
    if (end <= start) return true;

    size_t globalWorkSize = (size_t)(end - start);

    clSetKernelArg(_stencilKernel, 0, sizeof(cl_mem), &src);
    clSetKernelArg(_stencilKernel, 1, sizeof(int), &srcDesc.offset);
    clSetKernelArg(_stencilKernel, 2, sizeof(cl_mem), &dst);
    clSetKernelArg(_stencilKernel, 3, sizeof(int), &dstDesc.offset);
    clSetKernelArg(_stencilKernel, 4, sizeof(cl_mem), &sizes);
    clSetKernelArg(_stencilKernel, 5, sizeof(cl_mem), &offsets);
    clSetKernelArg(_stencilKernel, 6, sizeof(cl_mem), &indices);
    clSetKernelArg(_stencilKernel, 7, sizeof(cl_mem), &weights);
    clSetKernelArg(_stencilKernel, 8, sizeof(int), &start);
    clSetKernelArg(_stencilKernel, 9, sizeof(int), &end);

    cl_int errNum = clEnqueueNDRangeKernel(
        _clCommandQueue, _stencilKernel, 1, NULL,
        &globalWorkSize, NULL, numStartEvents, startEvents, endEvent);

    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "ApplyStencilKernel (%d) ", errNum);
        return false;
    }

    if (endEvent == NULL)
    {
    clFinish(_clCommandQueue);
    }
    return true;
}

bool
CLEvaluator::EvalStencils(cl_mem src, BufferDescriptor const &srcDesc,
                          cl_mem dst, BufferDescriptor const &dstDesc,
                          cl_mem du, BufferDescriptor const &duDesc,
                          cl_mem dv, BufferDescriptor const &dvDesc,
                          cl_mem sizes,
                          cl_mem offsets,
                          cl_mem indices,
                          cl_mem weights,
                          cl_mem duWeights,
                          cl_mem dvWeights,
                          int start, int end,
                          unsigned int numStartEvents,
                          const cl_event* startEvents,
                          cl_event* endEvent) const {
    if (end <= start) return true;

    size_t globalWorkSize = (size_t)(end - start);

    BufferDescriptor empty;
    clSetKernelArg(_stencilDerivKernel,  0, sizeof(cl_mem), &src);
    clSetKernelArg(_stencilDerivKernel,  1, sizeof(int), &srcDesc.offset);
    clSetKernelArg(_stencilDerivKernel,  2, sizeof(cl_mem), &dst);
    clSetKernelArg(_stencilDerivKernel,  3, sizeof(int), &dstDesc.offset);
    clSetKernelArg(_stencilDerivKernel,  4, sizeof(cl_mem), &du);
    clSetKernelArg(_stencilDerivKernel,  5, sizeof(int), &duDesc.offset);
    clSetKernelArg(_stencilDerivKernel,  6, sizeof(int), &duDesc.stride);
    clSetKernelArg(_stencilDerivKernel,  7, sizeof(cl_mem), &dv);
    clSetKernelArg(_stencilDerivKernel,  8, sizeof(int), &dvDesc.offset);
    clSetKernelArg(_stencilDerivKernel,  9, sizeof(int), &dvDesc.stride);
    clSetKernelArg(_stencilDerivKernel, 10, sizeof(cl_mem), NULL);
    clSetKernelArg(_stencilDerivKernel, 11, sizeof(int), &empty.offset);
    clSetKernelArg(_stencilDerivKernel, 12, sizeof(int), &empty.stride);
    clSetKernelArg(_stencilDerivKernel, 13, sizeof(cl_mem), NULL);
    clSetKernelArg(_stencilDerivKernel, 14, sizeof(int), &empty.offset);
    clSetKernelArg(_stencilDerivKernel, 15, sizeof(int), &empty.stride);
    clSetKernelArg(_stencilDerivKernel, 16, sizeof(cl_mem), NULL);
    clSetKernelArg(_stencilDerivKernel, 17, sizeof(int), &empty.offset);
    clSetKernelArg(_stencilDerivKernel, 18, sizeof(int), &empty.stride);
    clSetKernelArg(_stencilDerivKernel, 19, sizeof(cl_mem), &sizes);
    clSetKernelArg(_stencilDerivKernel, 20, sizeof(cl_mem), &offsets);
    clSetKernelArg(_stencilDerivKernel, 21, sizeof(cl_mem), &indices);
    clSetKernelArg(_stencilDerivKernel, 22, sizeof(cl_mem), &weights);
    clSetKernelArg(_stencilDerivKernel, 23, sizeof(cl_mem), &duWeights);
    clSetKernelArg(_stencilDerivKernel, 24, sizeof(cl_mem), &dvWeights);
    clSetKernelArg(_stencilDerivKernel, 25, sizeof(cl_mem), NULL);
    clSetKernelArg(_stencilDerivKernel, 26, sizeof(cl_mem), NULL);
    clSetKernelArg(_stencilDerivKernel, 27, sizeof(cl_mem), NULL);
    clSetKernelArg(_stencilDerivKernel, 28, sizeof(int), &start);
    clSetKernelArg(_stencilDerivKernel, 29, sizeof(int), &end);

    cl_int errNum = clEnqueueNDRangeKernel(
        _clCommandQueue, _stencilDerivKernel, 1, NULL,
        &globalWorkSize, NULL, numStartEvents, startEvents, endEvent);

    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "ApplyStencilKernel (%d) ", errNum);
        return false;
    }

    if (endEvent == NULL) {
        clFinish(_clCommandQueue);
    }
    return true;
}

bool
CLEvaluator::EvalStencils(cl_mem src, BufferDescriptor const &srcDesc,
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
                          int start, int end,
                          unsigned int numStartEvents,
                          const cl_event* startEvents,
                          cl_event* endEvent) const {
    if (end <= start) return true;

    size_t globalWorkSize = (size_t)(end - start);

    clSetKernelArg(_stencilDerivKernel,  0, sizeof(cl_mem), &src);
    clSetKernelArg(_stencilDerivKernel,  1, sizeof(int), &srcDesc.offset);
    clSetKernelArg(_stencilDerivKernel,  2, sizeof(cl_mem), &dst);
    clSetKernelArg(_stencilDerivKernel,  3, sizeof(int), &dstDesc.offset);
    clSetKernelArg(_stencilDerivKernel,  4, sizeof(cl_mem), &du);
    clSetKernelArg(_stencilDerivKernel,  5, sizeof(int), &duDesc.offset);
    clSetKernelArg(_stencilDerivKernel,  6, sizeof(int), &duDesc.stride);
    clSetKernelArg(_stencilDerivKernel,  7, sizeof(cl_mem), &dv);
    clSetKernelArg(_stencilDerivKernel,  8, sizeof(int), &dvDesc.offset);
    clSetKernelArg(_stencilDerivKernel,  9, sizeof(int), &dvDesc.stride);
    clSetKernelArg(_stencilDerivKernel, 10, sizeof(cl_mem), &duu);
    clSetKernelArg(_stencilDerivKernel, 11, sizeof(int), &duuDesc.offset);
    clSetKernelArg(_stencilDerivKernel, 12, sizeof(int), &duuDesc.stride);
    clSetKernelArg(_stencilDerivKernel, 13, sizeof(cl_mem), &duv);
    clSetKernelArg(_stencilDerivKernel, 14, sizeof(int), &duvDesc.offset);
    clSetKernelArg(_stencilDerivKernel, 15, sizeof(int), &duvDesc.stride);
    clSetKernelArg(_stencilDerivKernel, 16, sizeof(cl_mem), &dvv);
    clSetKernelArg(_stencilDerivKernel, 17, sizeof(int), &dvvDesc.offset);
    clSetKernelArg(_stencilDerivKernel, 18, sizeof(int), &dvvDesc.stride);
    clSetKernelArg(_stencilDerivKernel, 19, sizeof(cl_mem), &sizes);
    clSetKernelArg(_stencilDerivKernel, 20, sizeof(cl_mem), &offsets);
    clSetKernelArg(_stencilDerivKernel, 21, sizeof(cl_mem), &indices);
    clSetKernelArg(_stencilDerivKernel, 22, sizeof(cl_mem), &weights);
    clSetKernelArg(_stencilDerivKernel, 23, sizeof(cl_mem), &duWeights);
    clSetKernelArg(_stencilDerivKernel, 24, sizeof(cl_mem), &dvWeights);
    clSetKernelArg(_stencilDerivKernel, 25, sizeof(cl_mem), &duuWeights);
    clSetKernelArg(_stencilDerivKernel, 26, sizeof(cl_mem), &duvWeights);
    clSetKernelArg(_stencilDerivKernel, 27, sizeof(cl_mem), &dvvWeights);
    clSetKernelArg(_stencilDerivKernel, 28, sizeof(int), &start);
    clSetKernelArg(_stencilDerivKernel, 29, sizeof(int), &end);

    cl_int errNum = clEnqueueNDRangeKernel(
        _clCommandQueue, _stencilDerivKernel, 1, NULL,
        &globalWorkSize, NULL, numStartEvents, startEvents, endEvent);

    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "ApplyStencilKernel (%d) ", errNum);
        return false;
    }

    if (endEvent == NULL) {
        clFinish(_clCommandQueue);
    }
    return true;
}

bool
CLEvaluator::EvalPatches(cl_mem src, BufferDescriptor const &srcDesc,
                         cl_mem dst, BufferDescriptor const &dstDesc,
                         cl_mem du,  BufferDescriptor const &duDesc,
                         cl_mem dv,  BufferDescriptor const &dvDesc,
                         int numPatchCoords,
                         cl_mem patchCoordsBuffer,
                         cl_mem patchArrayBuffer,
                         cl_mem patchIndexBuffer,
                         cl_mem patchParamBuffer,
                         unsigned int numStartEvents,
                         const cl_event* startEvents,
                         cl_event* endEvent) const {

    size_t globalWorkSize = (size_t)(numPatchCoords);

    BufferDescriptor empty;
    clSetKernelArg(_patchKernel,  0, sizeof(cl_mem), &src);
    clSetKernelArg(_patchKernel,  1, sizeof(int),    &srcDesc.offset);
    clSetKernelArg(_patchKernel,  2, sizeof(cl_mem), &dst);
    clSetKernelArg(_patchKernel,  3, sizeof(int),    &dstDesc.offset);
    clSetKernelArg(_patchKernel,  4, sizeof(cl_mem), &du);
    clSetKernelArg(_patchKernel,  5, sizeof(int),    &duDesc.offset);
    clSetKernelArg(_patchKernel,  6, sizeof(int),    &duDesc.stride);
    clSetKernelArg(_patchKernel,  7, sizeof(cl_mem), &dv);
    clSetKernelArg(_patchKernel,  8, sizeof(int),    &dvDesc.offset);
    clSetKernelArg(_patchKernel,  9, sizeof(int),    &dvDesc.stride);
    clSetKernelArg(_patchKernel, 10, sizeof(cl_mem), NULL);
    clSetKernelArg(_patchKernel, 11, sizeof(int),    &empty.offset);
    clSetKernelArg(_patchKernel, 12, sizeof(int),    &empty.stride);
    clSetKernelArg(_patchKernel, 13, sizeof(cl_mem), NULL);
    clSetKernelArg(_patchKernel, 14, sizeof(int),    &empty.offset);
    clSetKernelArg(_patchKernel, 15, sizeof(int),    &empty.stride);
    clSetKernelArg(_patchKernel, 16, sizeof(cl_mem), NULL);
    clSetKernelArg(_patchKernel, 17, sizeof(int),    &empty.offset);
    clSetKernelArg(_patchKernel, 18, sizeof(int),    &empty.stride);
    clSetKernelArg(_patchKernel, 19, sizeof(cl_mem), &patchCoordsBuffer);
    clSetKernelArg(_patchKernel, 20, sizeof(cl_mem), &patchArrayBuffer);
    clSetKernelArg(_patchKernel, 21, sizeof(cl_mem), &patchIndexBuffer);
    clSetKernelArg(_patchKernel, 22, sizeof(cl_mem), &patchParamBuffer);

    cl_int errNum = clEnqueueNDRangeKernel(
        _clCommandQueue, _patchKernel, 1, NULL,
        &globalWorkSize, NULL, numStartEvents, startEvents, endEvent);

    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "ApplyPatchKernel (%d) ", errNum);
        return false;
    }

    if (endEvent == NULL) {
        clFinish(_clCommandQueue);
    }
    return true;
}

bool
CLEvaluator::EvalPatches(cl_mem src, BufferDescriptor const &srcDesc,
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
                         cl_mem patchParamBuffer,
                         unsigned int numStartEvents,
                         const cl_event* startEvents,
                         cl_event* endEvent) const {

    size_t globalWorkSize = (size_t)(numPatchCoords);

    clSetKernelArg(_patchKernel,  0, sizeof(cl_mem), &src);
    clSetKernelArg(_patchKernel,  1, sizeof(int),    &srcDesc.offset);
    clSetKernelArg(_patchKernel,  2, sizeof(cl_mem), &dst);
    clSetKernelArg(_patchKernel,  3, sizeof(int),    &dstDesc.offset);
    clSetKernelArg(_patchKernel,  4, sizeof(cl_mem), &du);
    clSetKernelArg(_patchKernel,  5, sizeof(int),    &duDesc.offset);
    clSetKernelArg(_patchKernel,  6, sizeof(int),    &duDesc.stride);
    clSetKernelArg(_patchKernel,  7, sizeof(cl_mem), &dv);
    clSetKernelArg(_patchKernel,  8, sizeof(int),    &dvDesc.offset);
    clSetKernelArg(_patchKernel,  9, sizeof(int),    &dvDesc.stride);
    clSetKernelArg(_patchKernel, 10, sizeof(cl_mem), &duu);
    clSetKernelArg(_patchKernel, 11, sizeof(int),    &duuDesc.offset);
    clSetKernelArg(_patchKernel, 12, sizeof(int),    &duuDesc.stride);
    clSetKernelArg(_patchKernel, 13, sizeof(cl_mem), &duv);
    clSetKernelArg(_patchKernel, 14, sizeof(int),    &duvDesc.offset);
    clSetKernelArg(_patchKernel, 15, sizeof(int),    &duvDesc.stride);
    clSetKernelArg(_patchKernel, 16, sizeof(cl_mem), &dvv);
    clSetKernelArg(_patchKernel, 17, sizeof(int),    &dvvDesc.offset);
    clSetKernelArg(_patchKernel, 18, sizeof(int),    &dvvDesc.stride);
    clSetKernelArg(_patchKernel, 19, sizeof(cl_mem), &patchCoordsBuffer);
    clSetKernelArg(_patchKernel, 20, sizeof(cl_mem), &patchArrayBuffer);
    clSetKernelArg(_patchKernel, 21, sizeof(cl_mem), &patchIndexBuffer);
    clSetKernelArg(_patchKernel, 22, sizeof(cl_mem), &patchParamBuffer);

    cl_int errNum = clEnqueueNDRangeKernel(
        _clCommandQueue, _patchKernel, 1, NULL,
        &globalWorkSize, NULL, numStartEvents, startEvents, endEvent);

    if (errNum != CL_SUCCESS) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "ApplyPatchKernel (%d) ", errNum);
        return false;
    }

    if (endEvent == NULL) {
        clFinish(_clCommandQueue);
    }
    return true;
}


/* static */
void
CLEvaluator::Synchronize(cl_command_queue clCommandQueue) {
    clFinish(clCommandQueue);
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv
