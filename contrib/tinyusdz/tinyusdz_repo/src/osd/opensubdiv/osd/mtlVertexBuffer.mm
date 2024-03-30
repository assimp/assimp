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

#include "../osd/mtlVertexBuffer.h"
#include <Metal/Metal.h>
#include <TargetConditionals.h>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

CPUMTLVertexBuffer::CPUMTLVertexBuffer(int numElements, int numVertices)
:
_numElements(numElements), _numVertices(numVertices),
_buffer(nullptr), _dirty(true)
{

}

bool CPUMTLVertexBuffer::allocate(MTLContext* context)
{
#if TARGET_OS_IOS || TARGET_OS_TV
    _buffer = [context->device newBufferWithLength: _numElements * _numVertices * sizeof(float) options:MTLResourceOptionCPUCacheModeDefault];
#elif TARGET_OS_OSX
    _buffer = [context->device newBufferWithLength: _numElements * _numVertices * sizeof(float) options:MTLResourceStorageModeManaged];
#endif
    if(_buffer == nil)
        return false;

    _dirty = true;
    _buffer.label = @"OSD VertexBuffer";

    return true;
}

CPUMTLVertexBuffer* CPUMTLVertexBuffer::Create(int numElements, int numVertices, MTLContext* context)
{
    auto instance = new CPUMTLVertexBuffer(numElements, numVertices);
    if(!instance->allocate(context))
    {
        delete instance;
        return nullptr;
    }

    return instance;
}

void CPUMTLVertexBuffer::UpdateData(const float* src, int startVertex, int numVertices, MTLContext* context)
{
    _dirty = true;
    memcpy(((float*)_buffer.contents) + startVertex * _numElements, src, _numElements * numVertices * sizeof(float));
}

float* CPUMTLVertexBuffer::BindCpuBuffer()
{
    _dirty = true;
    return (float*)_buffer.contents;
}

id<MTLBuffer> CPUMTLVertexBuffer::BindMTLBuffer(MTLContext* context)
{
#if TARGET_OS_OSX
    if(_dirty)
        [_buffer didModifyRange:NSMakeRange(0, _buffer.length)];
    _dirty = false;
#endif
    return _buffer;
}



} //end namepsace Osd

} //end namespace OPENSUBDIV_VERSION
} //end namespace OpenSubdiv
