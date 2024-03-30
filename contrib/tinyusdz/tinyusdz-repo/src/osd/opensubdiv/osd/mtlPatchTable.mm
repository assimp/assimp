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

#import "../osd/mtlPatchTable.h"
#import <Metal/Metal.h>
#import "../far/patchTable.h"
#import "../osd/cpuPatchTable.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

MTLPatchTable::MTLPatchTable()
:
_indexBuffer(nil),
_patchParamBuffer(nil),
_varyingPatchIndexBuffer(nil)
{

}


MTLPatchTable::~MTLPatchTable()
{

}

static id<MTLBuffer> createBuffer(const void* data, const size_t length,
                                  MTLContext* context)
{
    if(length == 0)
        return nil;
#if TARGET_OS_IOS || TARGET_OS_TV
    return [context->device newBufferWithBytes:data length:length options:MTLResourceOptionCPUCacheModeDefault];
#elif TARGET_OS_OSX
  @autoreleasepool {
    auto cmdBuf = [context->commandQueue commandBuffer];
    auto blitEncoder = [cmdBuf blitCommandEncoder];

    auto stageBuffer = [context->device newBufferWithBytes:data length:length options:MTLResourceOptionCPUCacheModeDefault];

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


MTLPatchTable* MTLPatchTable::Create(const Far::PatchTable *farPatchTable, MTLContext* context)
{
    auto patchTable = new MTLPatchTable();
    if(patchTable->allocate(farPatchTable, context))
        return patchTable;

    delete patchTable;
    assert(0 && "MTLPatchTable Creation Failed");
    return nullptr;
}

bool MTLPatchTable::allocate(Far::PatchTable const *farPatchTable, MTLContext* context)
{
    CpuPatchTable cpuTable(farPatchTable);

    auto numPatchArrays = cpuTable.GetNumPatchArrays();
    auto indexSize = cpuTable.GetPatchIndexSize();
    auto patchParamSize = cpuTable.GetPatchParamSize();

    _patchArrays.assign(cpuTable.GetPatchArrayBuffer(), cpuTable.GetPatchArrayBuffer() + numPatchArrays);

    _indexBuffer = createBuffer(cpuTable.GetPatchIndexBuffer(), indexSize * sizeof(unsigned), context);
    if(_indexBuffer == nil)
        return false;

    _indexBuffer.label = @"OSD PatchIndexBuffer";

    _patchParamBuffer = createBuffer(cpuTable.GetPatchParamBuffer(), patchParamSize * sizeof(PatchParam), context);
    if(_patchParamBuffer == nil)
        return false;

    _patchParamBuffer.label = @"OSD PatchParamBuffer";

    _varyingPatchArrays.assign(cpuTable.GetVaryingPatchArrayBuffer(), cpuTable.GetVaryingPatchArrayBuffer() + numPatchArrays);

    _varyingPatchIndexBuffer = createBuffer(cpuTable.GetVaryingPatchIndexBuffer(), sizeof(int) * cpuTable.GetVaryingPatchIndexSize(), context);
    if(_varyingPatchIndexBuffer == nil && cpuTable.GetVaryingPatchIndexSize() > 0)
        return false;

    auto numFVarChannels = cpuTable.GetNumFVarChannels();
    _fvarPatchArrays.resize(numFVarChannels);
    _fvarIndexBuffers.resize(numFVarChannels);
    _fvarParamBuffers.resize(numFVarChannels);
    for(auto fvc = 0; fvc < numFVarChannels; fvc++)
    {
        _fvarPatchArrays[fvc].assign(cpuTable.GetFVarPatchArrayBuffer(fvc), cpuTable.GetFVarPatchArrayBuffer(fvc) + numPatchArrays);
        _fvarIndexBuffers[fvc] = createBuffer(cpuTable.GetFVarPatchIndexBuffer(fvc), cpuTable.GetFVarPatchIndexSize(fvc) * sizeof(int), context);
        if(_fvarIndexBuffers[fvc] == nil)
            return false;

        _fvarParamBuffers[fvc] = createBuffer(cpuTable.GetFVarPatchParamBuffer(fvc), cpuTable.GetFVarPatchParamSize(fvc) * sizeof(PatchParam), context);
        if(_fvarParamBuffers[fvc] == nil)
            return false;
    }


    return true;
}

} //end namespace Osd
} //end namespace OPENSUBDIV_VERSION
} //end namespace OpenSubdiv
