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

#include "../osd/mtlLegacyGregoryPatchTable.h"
#include <Metal/Metal.h>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {
namespace Osd {

static id<MTLBuffer> createBuffer(const void* data, const size_t length,
                                  MTLContext* context)
{
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
}

MTLLegacyGregoryPatchTable::~MTLLegacyGregoryPatchTable()
{

}

MTLLegacyGregoryPatchTable* MTLLegacyGregoryPatchTable::Create(const Far::PatchTable *farPatchTable,
                                                               MTLContext* context)
{
    auto pt = new MTLLegacyGregoryPatchTable();
    auto& vertexValenceTable = farPatchTable->GetVertexValenceTable();
    auto& quadOffsetsTable = farPatchTable->GetQuadOffsetsTable();

    if(!vertexValenceTable.empty())
    {
        pt->_vertexValenceBuffer = createBuffer(vertexValenceTable.data(), vertexValenceTable.size() * sizeof(vertexValenceTable[0]), context);
    }

    if(!quadOffsetsTable.empty())
    {
        pt->_quadOffsetsBuffer = createBuffer(quadOffsetsTable.data(), quadOffsetsTable.size() * sizeof(quadOffsetsTable[0]), context);
    }

    pt->_quadOffsetsBase[0] = 0;
    pt->_quadOffsetsBase[1] = 0;

    for(auto i = 0; i < farPatchTable->GetNumPatchArrays(); i++)
    {
        if(farPatchTable->GetPatchArrayDescriptor(i).GetType() == Far::PatchDescriptor::GREGORY)
        {
            pt->_quadOffsetsBase[1] = farPatchTable->GetNumPatches(i) * 4;
            break;
        }
    }

    return pt;
}

void MTLLegacyGregoryPatchTable::UpdateVertexBuffer(id<MTLBuffer> vbo, int numVertices, int numVertexElements, MTLContext* context)
{
    _vertexBuffer = vbo;
}

} //end namespace Osd

} //end namespace OPENSUBDIV_VERSION
} //end namespace OpenSubdiv
