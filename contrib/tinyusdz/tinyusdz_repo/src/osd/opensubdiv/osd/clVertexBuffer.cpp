//
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

#include "../osd/clVertexBuffer.h"

#include <cassert>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

CLVertexBuffer::CLVertexBuffer(int numElements, int numVertices,
                                     cl_context /* clContext */)
    : _numElements(numElements), _numVertices(numVertices), _clMemory(NULL) {

}

CLVertexBuffer::~CLVertexBuffer() {

    clReleaseMemObject(_clMemory);
}

CLVertexBuffer *
CLVertexBuffer::Create(int numElements, int numVertices,
                          cl_context clContext) {
    CLVertexBuffer *instance =
        new CLVertexBuffer(numElements, numVertices, clContext);
    if (instance->allocate(clContext)) return instance;
    delete instance;
    return NULL;
}

void
CLVertexBuffer::UpdateData(const float *src, int startVertex, int numVertices, cl_command_queue queue,
    cl_event* startEvents, unsigned int numStartEvents, cl_event* endEvent) {

    size_t size = _numElements * numVertices * sizeof(float);
    size_t offset = startVertex * _numElements * sizeof(float);

    cl_bool blocking = (endEvent == NULL) ? CL_TRUE : CL_FALSE;
    cl_int err = clEnqueueWriteBuffer(queue, _clMemory, blocking, offset, size, src, numStartEvents, startEvents, endEvent);
    assert(err == CL_SUCCESS);
}

int
CLVertexBuffer::GetNumElements() const {

    return _numElements;
}

int
CLVertexBuffer::GetNumVertices() const {

    return _numVertices;
}

cl_mem
CLVertexBuffer::BindCLBuffer(cl_command_queue /* queue */) {

    return _clMemory;
}

bool
CLVertexBuffer::allocate(cl_context clContext) {
    assert(clContext);
    int size = _numVertices * _numElements * sizeof(float);
    cl_int err;

    _clMemory = clCreateBuffer(clContext, CL_MEM_READ_WRITE, size, NULL, &err);

    if (err != CL_SUCCESS) return false;
    return true;
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv

