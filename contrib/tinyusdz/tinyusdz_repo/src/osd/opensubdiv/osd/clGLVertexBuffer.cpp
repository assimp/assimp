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

#include "glLoader.h"

#include "../osd/clGLVertexBuffer.h"

#include <cassert>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

CLGLVertexBuffer::CLGLVertexBuffer(int numElements,
                                         int numVertices,
                                         cl_context /* clContext */)
    : _numElements(numElements), _numVertices(numVertices),
      _vbo(0), _clQueue(0), _clMemory(0), _clMapped(false) {

    // Initialize internal OpenGL loader library if necessary
    OpenSubdiv::internal::GLLoader::libraryInitializeGL();
}

CLGLVertexBuffer::~CLGLVertexBuffer() {

    unmap();
    clReleaseMemObject(_clMemory);
    glDeleteBuffers(1, &_vbo);
}

CLGLVertexBuffer *
CLGLVertexBuffer::Create(int numElements, int numVertices, cl_context clContext)
{
    CLGLVertexBuffer *instance =
        new CLGLVertexBuffer(numElements, numVertices, clContext);
    if (instance->allocate(clContext)) return instance;
    delete instance;
    return NULL;
}

void
CLGLVertexBuffer::UpdateData(const float *src, int startVertex, int numVertices,
                             cl_command_queue queue) {

    size_t size = numVertices * _numElements * sizeof(float);
    size_t offset = startVertex * _numElements * sizeof(float);

    map(queue);
    clEnqueueWriteBuffer(queue, _clMemory, true, offset, size, src, 0, NULL, NULL);
}

int
CLGLVertexBuffer::GetNumElements() const {

    return _numElements;
}

int
CLGLVertexBuffer::GetNumVertices() const {

    return _numVertices;
}

cl_mem
CLGLVertexBuffer::BindCLBuffer(cl_command_queue queue) {

    map(queue);
    return _clMemory;
}

GLuint
CLGLVertexBuffer::BindVBO(void * /*deviceContext*/) {

    unmap();
    return _vbo;
}

bool
CLGLVertexBuffer::allocate(cl_context clContext) {

    assert(clContext);

    // create GL buffer first
    int size = _numElements * _numVertices * sizeof(float);

#if defined(GL_ARB_direct_state_access)
    if (OSD_OPENGL_HAS(ARB_direct_state_access)) {
        glCreateBuffers(1, &_vbo);
        glNamedBufferData(_vbo, size, 0, GL_DYNAMIC_DRAW);
    } else
#endif
    {
        GLint prev = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev);
        glGenBuffers(1, &_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, prev);
    }

    // register vbo as cl memory
    cl_int err;
    _clMemory = clCreateFromGLBuffer(clContext,
                                     CL_MEM_READ_WRITE, _vbo, &err);

    if (err != CL_SUCCESS) return false;
    return true;
}

void
CLGLVertexBuffer::map(cl_command_queue queue) {

    if (_clMapped) return;    // XXX: what if another queue is given?
    _clQueue = queue;
    clEnqueueAcquireGLObjects(queue, 1, &_clMemory, 0, 0, 0);
    _clMapped = true;
}

void
CLGLVertexBuffer::unmap() {

    if (! _clMapped) return;
    clEnqueueReleaseGLObjects(_clQueue, 1, &_clMemory, 0, 0, 0);
    _clMapped = false;
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv
