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

#include "../osd/mtlComputeEvaluator.h"
#include "../osd/mtlPatchShaderSource.h"

#include <vector>
#include <Metal/Metal.h>
#include <simd/simd.h>
#include <sstream>
#include <string>

#include "../far/stencilTable.h"
#include "../far/error.h"

#define PARAMETER_BUFFER_INDEX 0
#define SIZES_BUFFER_INDEX 1
#define OFFSETS_BUFFER_INDEX 2
#define INDICES_BUFFER_INDEX 3
#define WEIGHTS_BUFFER_INDEX 4
#define DST_VERTEX_BUFFER_INDEX 5
#define SRC_VERTEX_BUFFER_INDEX 6
#define DU_WEIGHTS_BUFFER_INDEX 7
#define DV_WEIGHTS_BUFFER_INDEX 8
#define DU_DERIVATIVE_BUFFER_INDEX 9
#define DV_DERIVATIVE_BUFFER_INDEX 10
#define DUU_WEIGHTS_BUFFER_INDEX 11
#define DUV_WEIGHTS_BUFFER_INDEX 12
#define DVV_WEIGHTS_BUFFER_INDEX 13
#define DUU_DERIVATIVE_BUFFER_INDEX 14
#define DUV_DERIVATIVE_BUFFER_INDEX 15
#define DVV_DERIVATIVE_BUFFER_INDEX 16
#define PATCH_ARRAYS_BUFFER_INDEX 17
#define PATCH_COORDS_BUFFER_INDEX 18
#define PATCH_INDICES_BUFFER_INDEX 19
#define PATCH_PARAMS_BUFFER_INDEX 20

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {
namespace Osd {

namespace mtl
{
  struct PatchCoord
  {
      int arrayIndex;
      int patchIndex;
      int vertIndex;
      float s;
      float t;
  };

  struct PatchParam
  {
      uint field0;
      uint field1;
      float sharpness;
  };

  struct KernelUniformArgs
  {
      int batchStart;
      int batchEnd;

      int srcOffset;
      int dstOffset;

      simd::int3 duDesc;
      simd::int3 dvDesc;

      simd::int3 duuDesc;
      simd::int3 duvDesc;
      simd::int3 dvvDesc;
  };
} //end namespace mtl

static const char *KernelSource =
#include "mtlComputeKernel.gen.h"
;

template <typename T>
static id<MTLBuffer> createBuffer(const std::vector<T> &vec,
                                      MTLContext* context)
{
    if (vec.empty()) {
        return nil;
    }

    const auto length = sizeof(T) * vec.size();
#if TARGET_OS_IOS || TARGET_OS_TV
    return [context->device newBufferWithBytes:vec.data() length:length options:MTLResourceOptionCPUCacheModeDefault];
#elif TARGET_OS_OSX
  @autoreleasepool {
    auto cmdBuf = [context->commandQueue commandBuffer];
    auto blitEncoder = [cmdBuf blitCommandEncoder];

    auto stageBuffer = [context->device newBufferWithBytes:vec.data() length:length options:MTLResourceOptionCPUCacheModeDefault];

    auto finalBuffer = [context->device newBufferWithLength:length options:MTLResourceStorageModePrivate];

    [blitEncoder copyFromBuffer:stageBuffer sourceOffset:0 toBuffer:finalBuffer destinationOffset:0 size:length];
    [blitEncoder endEncoding];
    [cmdBuf commit];
    [cmdBuf waitUntilCompleted];

#if !__has_feature(objc_arc)
      [stageBuffer release];
#endif

    return finalBuffer;
  }
#endif
}

using namespace OpenSubdiv::OPENSUBDIV_VERSION;
using namespace Osd;

MTLStencilTable::MTLStencilTable(Far::StencilTable const *stencilTable,
                                 MTLContext* context)
{
    assert(context != nil);
    assert(context->device != nil && context->commandQueue != nil);

    _numStencils = stencilTable->GetNumStencils();
    if (_numStencils > 0)
    {
      auto sizes = stencilTable->GetSizes();

      _sizesBuffer = createBuffer(stencilTable->GetSizes(), context);
      _offsetsBuffer = createBuffer(stencilTable->GetOffsets(), context);
      _indicesBuffer = createBuffer(stencilTable->GetControlIndices(), context);
      _weightsBuffer = createBuffer(stencilTable->GetWeights(), context);

      _sizesBuffer.label = @"StencilTable Sizes";
      _offsetsBuffer.label = @"StencilTable Offsets";
      _indicesBuffer.label = @"StencilTable Indices";
      _weightsBuffer.label = @"StencilTable Weights";
    }

    _duWeightsBuffer = nil;
    _dvWeightsBuffer = nil;
    _duuWeightsBuffer = nil;
    _duvWeightsBuffer = nil;
    _dvvWeightsBuffer = nil;
}

MTLStencilTable::MTLStencilTable(Far::LimitStencilTable const *stencilTable,
                                 MTLContext* context)
{
    assert(context != nil);
    assert(context->device != nil && context->commandQueue != nil);

    _numStencils = stencilTable->GetNumStencils();
    if (_numStencils > 0)
    {
      auto sizes = stencilTable->GetSizes();

      _sizesBuffer = createBuffer(stencilTable->GetSizes(), context);
      _offsetsBuffer = createBuffer(stencilTable->GetOffsets(), context);
      _indicesBuffer = createBuffer(stencilTable->GetControlIndices(), context);
      _weightsBuffer = createBuffer(stencilTable->GetWeights(), context);
      _duWeightsBuffer = createBuffer(stencilTable->GetDuWeights(), context);
      _dvWeightsBuffer = createBuffer(stencilTable->GetDvWeights(), context);
      _duuWeightsBuffer = createBuffer(stencilTable->GetDuuWeights(), context);
      _duvWeightsBuffer = createBuffer(stencilTable->GetDuvWeights(), context);
      _dvvWeightsBuffer = createBuffer(stencilTable->GetDvvWeights(), context);

      _sizesBuffer.label = @"StencilTable Sizes";
      _offsetsBuffer.label = @"StencilTable Offsets";
      _indicesBuffer.label = @"StencilTable Indices";
      _weightsBuffer.label = @"StencilTable Weights";
      if (_duWeightsBuffer) {
          _duWeightsBuffer.label = @"StencilTable duWeights";
      }
      if (_dvWeightsBuffer) {
          _dvWeightsBuffer.label = @"StencilTable dvWeights";
      }
      if (_duuWeightsBuffer) {
          _duuWeightsBuffer.label = @"StencilTable duuWeights";
      }
      if (_duvWeightsBuffer) {
          _duvWeightsBuffer.label = @"StencilTable duvWeights";
      }
      if (_dvvWeightsBuffer) {
          _dvvWeightsBuffer.label = @"StencilTable dvvWeights";
      }
    }
}

MTLStencilTable::~MTLStencilTable() {}

MTLComputeEvaluator *MTLComputeEvaluator::Create(
    BufferDescriptor const &srcDesc, BufferDescriptor const &dstDesc,
    BufferDescriptor const &duDesc, BufferDescriptor const &dvDesc,
    MTLContext* context)
{
    assert(context != nil);
    assert(context->device != nil && context->commandQueue != nil);

    auto instance = new MTLComputeEvaluator();
    if (instance->Compile(srcDesc, dstDesc, duDesc, dvDesc,
                          BufferDescriptor(),
                          BufferDescriptor(),
                          BufferDescriptor(),
                          context))
      return instance;

    delete instance;

    return nullptr;
}

MTLComputeEvaluator *MTLComputeEvaluator::Create(
    BufferDescriptor const &srcDesc, BufferDescriptor const &dstDesc,
    BufferDescriptor const &duDesc, BufferDescriptor const &dvDesc,
    BufferDescriptor const &duuDesc, BufferDescriptor const &duvDesc, BufferDescriptor const &dvvDesc,
    MTLContext* context)
{
    assert(context != nil);
    assert(context->device != nil && context->commandQueue != nil);

    auto instance = new MTLComputeEvaluator();
    if (instance->Compile(srcDesc, dstDesc, duDesc, dvDesc,
                          duuDesc, duvDesc, dvvDesc, context)) {
        return instance;
    }

    delete instance;

    return nullptr;
}

bool MTLComputeEvaluator::Compile(BufferDescriptor const &srcDesc,
                                  BufferDescriptor const &dstDesc,
                                  BufferDescriptor const &duDesc,
                                  BufferDescriptor const &dvDesc,
                                  BufferDescriptor const &duuDesc,
                                  BufferDescriptor const &duvDesc,
                                  BufferDescriptor const &dvvDesc,
                                  MTLContext* context)
{
    assert(context != nil);
    assert(context->device != nil && context->commandQueue != nil);

    using namespace Osd;
    using namespace Far;

    MTLCompileOptions *compileOptions = [[MTLCompileOptions alloc] init];
    compileOptions.preprocessorMacros = nil;

    bool deriv1 = duDesc.length > 0 || dvDesc.length > 0;
    bool deriv2 = duuDesc.length > 0 || duvDesc.length > 0 || dvvDesc.length > 0;

#define DEFINE(x,y) @(#x) : @(y)
    auto preprocessor = @{
    DEFINE(LENGTH, srcDesc.length),
    DEFINE(SRC_STRIDE, srcDesc.stride),
    DEFINE(DST_STRIDE, dstDesc.stride),
    DEFINE(WORK_GROUP_SIZE, _workGroupSize),
    DEFINE(OPENSUBDIV_MTL_COMPUTE_USE_1ST_DERIVATIVES, deriv1),
    DEFINE(OPENSUBDIV_MTL_COMPUTE_USE_2ND_DERIVATIVES, deriv2),
    DEFINE(PARAMETER_BUFFER_INDEX,PARAMETER_BUFFER_INDEX),
    DEFINE(SIZES_BUFFER_INDEX,SIZES_BUFFER_INDEX),
    DEFINE(OFFSETS_BUFFER_INDEX,OFFSETS_BUFFER_INDEX),
    DEFINE(INDICES_BUFFER_INDEX,INDICES_BUFFER_INDEX),
    DEFINE(WEIGHTS_BUFFER_INDEX,WEIGHTS_BUFFER_INDEX),
    DEFINE(SRC_VERTEX_BUFFER_INDEX,SRC_VERTEX_BUFFER_INDEX),
    DEFINE(DST_VERTEX_BUFFER_INDEX,DST_VERTEX_BUFFER_INDEX),
    DEFINE(DU_WEIGHTS_BUFFER_INDEX,DU_WEIGHTS_BUFFER_INDEX),
    DEFINE(DV_WEIGHTS_BUFFER_INDEX,DV_WEIGHTS_BUFFER_INDEX),
    DEFINE(DU_DERIVATIVE_BUFFER_INDEX,DU_DERIVATIVE_BUFFER_INDEX),
    DEFINE(DV_DERIVATIVE_BUFFER_INDEX,DV_DERIVATIVE_BUFFER_INDEX),
    DEFINE(DUU_WEIGHTS_BUFFER_INDEX,DUU_WEIGHTS_BUFFER_INDEX),
    DEFINE(DUV_WEIGHTS_BUFFER_INDEX,DUV_WEIGHTS_BUFFER_INDEX),
    DEFINE(DVV_WEIGHTS_BUFFER_INDEX,DVV_WEIGHTS_BUFFER_INDEX),
    DEFINE(DUU_DERIVATIVE_BUFFER_INDEX,DUU_DERIVATIVE_BUFFER_INDEX),
    DEFINE(DUV_DERIVATIVE_BUFFER_INDEX,DUV_DERIVATIVE_BUFFER_INDEX),
    DEFINE(DVV_DERIVATIVE_BUFFER_INDEX,DVV_DERIVATIVE_BUFFER_INDEX),
    DEFINE(PATCH_ARRAYS_BUFFER_INDEX,PATCH_ARRAYS_BUFFER_INDEX),
    DEFINE(PATCH_COORDS_BUFFER_INDEX,PATCH_COORDS_BUFFER_INDEX),
    DEFINE(PATCH_INDICES_BUFFER_INDEX,PATCH_INDICES_BUFFER_INDEX),
    DEFINE(PATCH_PARAMS_BUFFER_INDEX,PATCH_PARAMS_BUFFER_INDEX),
    };
#undef DEFINE

    compileOptions.preprocessorMacros = preprocessor;

    std::stringstream sourceString;
    sourceString << MTLPatchShaderSource::GetPatchBasisShaderSource();
    sourceString << KernelSource;

    NSError *err = nil;

    _computeLibrary =
    [context->device newLibraryWithSource:@(sourceString.str().c_str())
                                  options:compileOptions
                                    error:&err];

#if !__has_feature(objc_arc)
    [compileOptions release];
#endif

    if (!_computeLibrary)
    {
        Far::Error(Far::FAR_RUNTIME_ERROR, "Error compiling MTL Shader: %s\n",
                   err ? err.localizedDescription.UTF8String : "");
        return false;
    }

    auto evalStencilsFunction = [_computeLibrary newFunctionWithName:@"eval_stencils"];
    _evalStencils =
      [context->device newComputePipelineStateWithFunction:evalStencilsFunction
                                                     error:&err];

#if !__has_feature(objc_arc)
    [evalStencilsFunction release];
#endif

    if (!_evalStencils)
    {
        Far::Error(Far::FAR_RUNTIME_ERROR, "Error compiling MTL Pipeline eval_stencils: %s\n",
                   err ? err.localizedDescription.UTF8String : "");
        return false;
    }

    auto evalPatchesFunction = [_computeLibrary newFunctionWithName:@"eval_patches"];
    _evalPatches =
      [context->device newComputePipelineStateWithFunction:evalPatchesFunction
                                                     error:&err];

#if !__has_feature(objc_arc)
    [evalPatchesFunction release];
#endif

    if (!_evalPatches)
    {
        Far::Error(Far::FAR_RUNTIME_ERROR, "Error compiling MTL Pipeline eval_patches:  %s\n",
                   err ? err.localizedDescription.UTF8String : "");
        return false;
    }

    _parameterBuffer =
      [context->device newBufferWithLength:sizeof(mtl::KernelUniformArgs)
                                   options:MTLResourceOptionCPUCacheModeDefault];

    return true;
}

MTLComputeEvaluator::MTLComputeEvaluator() : _workGroupSize(32) {}

MTLComputeEvaluator::~MTLComputeEvaluator()
{
#if !__has_feature(objc_arc)
    [_computeLibrary release];
    [_evalStencils release];
    [_evalPatches release];
    [_parameterBuffer release];
#endif
}

void MTLComputeEvaluator::Synchronize(MTLContext*) { }

bool MTLComputeEvaluator::EvalStencils(
    id<MTLBuffer> srcBuffer, BufferDescriptor const &srcDesc,
    id<MTLBuffer> dstBuffer, BufferDescriptor const &dstDesc,
    id<MTLBuffer> duBuffer, BufferDescriptor const &duDesc,
    id<MTLBuffer> dvBuffer, BufferDescriptor const &dvDesc,
    id<MTLBuffer> sizesBuffer,
    id<MTLBuffer> offsetsBuffer,
    id<MTLBuffer> indicesBuffer,
    id<MTLBuffer> weightsBuffer,
    id<MTLBuffer> duWeightsBuffer,
    id<MTLBuffer> dvWeightsBuffer,
    int start, int end,
    MTLContext* context) const
{
    return EvalStencils(
        srcBuffer, srcDesc,
        dstBuffer, dstDesc,
        duBuffer, duDesc,
        dvBuffer, dvDesc,
        nil, BufferDescriptor(),
        nil, BufferDescriptor(),
        nil, BufferDescriptor(),
        sizesBuffer,
        offsetsBuffer,
        indicesBuffer,
        weightsBuffer,
        duWeightsBuffer,
        dvWeightsBuffer,
        nil, nil, nil,
        start, end,
        context);
}

bool MTLComputeEvaluator::EvalStencils(
    id<MTLBuffer> srcBuffer, BufferDescriptor const &srcDesc,
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
    int start, int end,
    MTLContext* context) const
{
    if(_evalStencils == nil)
      return false;

    auto count = end - start;
    if (count <= 0)
        return true;

    assert(context != nullptr);

    auto device = context->device;
    auto commandQueue = context->commandQueue;

    assert(device != nil && commandQueue != nil);

    mtl::KernelUniformArgs args;
    args.batchStart = start;
    args.batchEnd = end;
    args.srcOffset = srcDesc.offset;
    args.dstOffset = dstDesc.offset;
    args.duDesc = (simd::int3){duDesc.offset, duDesc.length, duDesc.stride};
    args.dvDesc = (simd::int3){dvDesc.offset, dvDesc.length, dvDesc.stride};
    args.duuDesc = (simd::int3){duuDesc.offset, duuDesc.length, duuDesc.stride};
    args.duvDesc = (simd::int3){duvDesc.offset, duvDesc.length, duvDesc.stride};
    args.dvvDesc = (simd::int3){dvvDesc.offset, dvvDesc.length, dvvDesc.stride};

    memcpy(_parameterBuffer.contents, &args, sizeof(args));

    auto commandBuffer = [commandQueue commandBuffer];

    auto computeEncoder = [commandBuffer computeCommandEncoder];

    [computeEncoder setBuffer:_parameterBuffer offset:0 atIndex:PARAMETER_BUFFER_INDEX];
    [computeEncoder setBuffer:sizesBuffer offset:0 atIndex:SIZES_BUFFER_INDEX];
    [computeEncoder setBuffer:weightsBuffer offset:0 atIndex:WEIGHTS_BUFFER_INDEX];
    [computeEncoder setBuffer:offsetsBuffer offset:0 atIndex:OFFSETS_BUFFER_INDEX];
    [computeEncoder setBuffer:indicesBuffer offset:0 atIndex:INDICES_BUFFER_INDEX];
    [computeEncoder setBuffer:srcBuffer offset:0 atIndex:SRC_VERTEX_BUFFER_INDEX];
    [computeEncoder setBuffer:dstBuffer offset:0 atIndex:DST_VERTEX_BUFFER_INDEX];
    if(duWeightsBuffer && dvWeightsBuffer)
    {
        [computeEncoder setBuffer:duWeightsBuffer offset:0 atIndex:DU_WEIGHTS_BUFFER_INDEX];
        [computeEncoder setBuffer:dvWeightsBuffer offset:0 atIndex:DV_WEIGHTS_BUFFER_INDEX];
    }
    [computeEncoder setBuffer:duBuffer offset:0 atIndex:DU_DERIVATIVE_BUFFER_INDEX];
    [computeEncoder setBuffer:dvBuffer offset:0 atIndex:DV_DERIVATIVE_BUFFER_INDEX];
    if(duuWeightsBuffer && duvWeightsBuffer && dvvWeightsBuffer)
    {
        [computeEncoder setBuffer:duuWeightsBuffer offset:0 atIndex:DUU_WEIGHTS_BUFFER_INDEX];
        [computeEncoder setBuffer:duvWeightsBuffer offset:0 atIndex:DUV_WEIGHTS_BUFFER_INDEX];
        [computeEncoder setBuffer:dvvWeightsBuffer offset:0 atIndex:DVV_WEIGHTS_BUFFER_INDEX];
    }
    if(duuBuffer && duvBuffer && dvvBuffer)
    {
        [computeEncoder setBuffer:duuBuffer offset:0 atIndex:DUU_DERIVATIVE_BUFFER_INDEX];
        [computeEncoder setBuffer:duvBuffer offset:0 atIndex:DUV_DERIVATIVE_BUFFER_INDEX];
        [computeEncoder setBuffer:dvvBuffer offset:0 atIndex:DVV_DERIVATIVE_BUFFER_INDEX];
    }
    [computeEncoder setComputePipelineState:_evalStencils];

    auto threadgroups = MTLSizeMake((count + _workGroupSize - 1) / _workGroupSize, 1, 1);
    auto threadsPerGroup = MTLSizeMake(_workGroupSize, 1, 1);
    [computeEncoder dispatchThreadgroups:threadgroups
                   threadsPerThreadgroup:threadsPerGroup];

    [computeEncoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    return true;
}

bool
MTLComputeEvaluator::EvalPatches(
    id<MTLBuffer> srcBuffer, const BufferDescriptor &srcDesc,
    id<MTLBuffer> dstBuffer, const BufferDescriptor &dstDesc,
    id<MTLBuffer> duBuffer, const BufferDescriptor &duDesc,
    id<MTLBuffer> dvBuffer, const BufferDescriptor &dvDesc,
    int numPatchCoords,
    id<MTLBuffer> patchCoordsBuffer,
    const PatchArrayVector &patchArrays,
    id<MTLBuffer> patchIndexBuffer,
    id<MTLBuffer> patchParamsBuffer,
    MTLContext* context) const
{
    return EvalPatches(
        srcBuffer, srcDesc,
        dstBuffer, dstDesc,
        duBuffer, duDesc,
        dvBuffer, dvDesc,
        nil, BufferDescriptor(),
        nil, BufferDescriptor(),
        nil, BufferDescriptor(),
        numPatchCoords,
        patchCoordsBuffer,
        patchArrays,
        patchIndexBuffer,
        patchParamsBuffer,
        context);
}

bool
MTLComputeEvaluator::EvalPatches(
    id<MTLBuffer> srcBuffer, const BufferDescriptor &srcDesc,
    id<MTLBuffer> dstBuffer, const BufferDescriptor &dstDesc,
    id<MTLBuffer> duBuffer,  const BufferDescriptor &duDesc,
    id<MTLBuffer> dvBuffer,  const BufferDescriptor &dvDesc,
    id<MTLBuffer> duuBuffer, const BufferDescriptor &duuDesc,
    id<MTLBuffer> duvBuffer, const BufferDescriptor &duvDesc,
    id<MTLBuffer> dvvBuffer, const BufferDescriptor &dvvDesc,
    int numPatchCoords,
    id<MTLBuffer> patchCoordsBuffer,
    const PatchArrayVector &patchArrays,
    id<MTLBuffer> patchIndexBuffer,
    id<MTLBuffer> patchParamsBuffer,
    MTLContext* context) const
{
    if(_evalPatches == nil)
        return false;

    assert(context != nullptr);

    auto device = context->device;
    auto commandQueue = context->commandQueue;

    assert(device != nil && commandQueue != nil);

    auto commandBuffer = [commandQueue commandBuffer];
    auto computeCommandEncoder = [commandBuffer computeCommandEncoder];

    mtl::KernelUniformArgs args;
    args.batchStart = 0;
    args.batchEnd = numPatchCoords;
    args.srcOffset = srcDesc.offset;
    args.dstOffset = dstDesc.offset;
    args.duDesc = (simd::int3){duDesc.offset, duDesc.length, duDesc.stride};
    args.dvDesc = (simd::int3){dvDesc.offset, dvDesc.length, dvDesc.stride};
    args.duuDesc = (simd::int3){duuDesc.offset, duuDesc.length, duuDesc.stride};
    args.duvDesc = (simd::int3){duvDesc.offset, duvDesc.length, duvDesc.stride};
    args.dvvDesc = (simd::int3){dvvDesc.offset, dvvDesc.length, dvvDesc.stride};

    [computeCommandEncoder setBytes:&args length:sizeof(mtl::KernelUniformArgs) atIndex:PARAMETER_BUFFER_INDEX];
    [computeCommandEncoder setBuffer:srcBuffer offset:0 atIndex:SRC_VERTEX_BUFFER_INDEX];
    [computeCommandEncoder setBuffer:dstBuffer offset:0 atIndex:DST_VERTEX_BUFFER_INDEX];
    if (duBuffer && dvBuffer) {
        [computeCommandEncoder setBuffer:duBuffer offset:0 atIndex:DU_DERIVATIVE_BUFFER_INDEX];
        [computeCommandEncoder setBuffer:dvBuffer offset:0 atIndex:DV_DERIVATIVE_BUFFER_INDEX];
    }
    if (duuBuffer && duvBuffer && dvvBuffer) {
        [computeCommandEncoder setBuffer:duuBuffer offset:0 atIndex:DUU_DERIVATIVE_BUFFER_INDEX];
        [computeCommandEncoder setBuffer:duvBuffer offset:0 atIndex:DUV_DERIVATIVE_BUFFER_INDEX];
        [computeCommandEncoder setBuffer:dvvBuffer offset:0 atIndex:DVV_DERIVATIVE_BUFFER_INDEX];
    }
    [computeCommandEncoder setBuffer:patchCoordsBuffer offset:0 atIndex:PATCH_COORDS_BUFFER_INDEX];
    [computeCommandEncoder setBuffer:patchIndexBuffer offset:0 atIndex:PATCH_INDICES_BUFFER_INDEX];
    [computeCommandEncoder setBuffer:patchParamsBuffer offset:0 atIndex:PATCH_PARAMS_BUFFER_INDEX];
    assert(!patchArrays.empty());
    [computeCommandEncoder setBytes:&patchArrays[0] length:sizeof(patchArrays[0]) * patchArrays.size() atIndex:PATCH_ARRAYS_BUFFER_INDEX];
    [computeCommandEncoder setComputePipelineState:_evalPatches];

    auto threadgroups =
    MTLSizeMake((numPatchCoords + _workGroupSize - 1) / _workGroupSize, 1, 1);
    auto threadsPerGroup = MTLSizeMake(_workGroupSize, 1, 1);
    [computeCommandEncoder dispatchThreadgroups:threadgroups
                   threadsPerThreadgroup:threadsPerGroup];

    [computeCommandEncoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    return true;
}

} //end namespace Osd

} //end namespace OPENSUBDIV_VERSION
} //end namespace OpenSubdiv
