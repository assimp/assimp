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

#include "../osd/cudaGLVertexBuffer.h"
#include "../far/error.h"

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#include <cassert>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

CudaGLVertexBuffer::CudaGLVertexBuffer(int numElements, int numVertices)
    : _numElements(numElements), _numVertices(numVertices),
      _vbo(0), _devicePtr(0), _cudaResource(0) {

    // Initialize internal OpenGL loader library if necessary
    OpenSubdiv::internal::GLLoader::libraryInitializeGL();
}

CudaGLVertexBuffer::~CudaGLVertexBuffer() {

    unmap();
    cudaGraphicsUnregisterResource(_cudaResource);
    glDeleteBuffers(1, &_vbo);
}

CudaGLVertexBuffer *
CudaGLVertexBuffer::Create(int numElements, int numVertices, void *) {
    CudaGLVertexBuffer *instance =
        new CudaGLVertexBuffer(numElements, numVertices);
    if (instance->allocate()) return instance;
    Far::Error(Far::FAR_RUNTIME_ERROR, "CudaGLVertexBuffer::Create failed.\n");
    delete instance;
    return NULL;
}

void
CudaGLVertexBuffer::UpdateData(const float *src,
                               int startVertex, int numVertices,
                               void * /*deviceContext*/) {

    map();
    cudaError_t err = cudaMemcpy((float*)_devicePtr + _numElements * startVertex,
                                 src,
                                 _numElements * numVertices * sizeof(float),
                                 cudaMemcpyHostToDevice);
    if (err != cudaSuccess)
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "CudaGLVertexBuffer::UpdateData failed. : %s\n",
                   cudaGetErrorString(err));
}

int
CudaGLVertexBuffer::GetNumElements() const {

    return _numElements;
}

int
CudaGLVertexBuffer::GetNumVertices() const {

    return _numVertices;
}

float *
CudaGLVertexBuffer::BindCudaBuffer() {

    map();
    return static_cast<float*>(_devicePtr);
}

GLuint
CudaGLVertexBuffer::BindVBO(void * /*deviceContext*/) {

    unmap();
    return _vbo;
}

bool
CudaGLVertexBuffer::allocate() {

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

    // register vbo as cuda resource
    cudaError_t err = cudaGraphicsGLRegisterBuffer(
        &_cudaResource, _vbo, cudaGraphicsMapFlagsWriteDiscard);

    if (err != cudaSuccess) return false;
    return true;
}

void
CudaGLVertexBuffer::map() {

    if (_devicePtr) return;
    size_t num_bytes;
    void *ptr;

    cudaError_t err = cudaGraphicsMapResources(1, &_cudaResource, 0);
    if (err != cudaSuccess)
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "CudaGLVertexBuffer::map failed.\n%s\n",
                   cudaGetErrorString(err));
    err = cudaGraphicsResourceGetMappedPointer(&ptr, &num_bytes, _cudaResource);
    if (err != cudaSuccess)
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "CudaGLVertexBuffer::map failed.\n%s\n",
                   cudaGetErrorString(err));
    _devicePtr = ptr;
}

void
CudaGLVertexBuffer::unmap() {

    if (_devicePtr == NULL) return;
    cudaError_t err = cudaGraphicsUnmapResources(1, &_cudaResource, 0);
    if (err != cudaSuccess)
       Far::Error(Far::FAR_RUNTIME_ERROR,
                  "CudaGLVertexBuffer::unmap failed.\n%s\n",
                  cudaGetErrorString(err));
    _devicePtr = NULL;
}


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv

