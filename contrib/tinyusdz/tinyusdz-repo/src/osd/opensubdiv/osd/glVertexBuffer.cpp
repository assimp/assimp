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

#include "../osd/glVertexBuffer.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

GLVertexBuffer::GLVertexBuffer(int numElements, int numVertices)
    : _numElements(numElements),
      _numVertices(numVertices),
      _vbo(0) {

    // Initialize internal OpenGL loader library if necessary
    OpenSubdiv::internal::GLLoader::libraryInitializeGL();
}

GLVertexBuffer::~GLVertexBuffer() {

    glDeleteBuffers(1, &_vbo);
}

GLVertexBuffer *
GLVertexBuffer::Create(int numElements, int numVertices, void *) {

    GLVertexBuffer *instance =
        new GLVertexBuffer(numElements, numVertices);
    if (instance->allocate()) return instance;
    delete instance;
    return 0;
}

void
GLVertexBuffer::UpdateData(const float *src, int startVertex, int numVertices,
                           void * /*deviceContext*/) {

    int size = numVertices * _numElements * (int) sizeof(float);
#if defined(GL_ARB_direct_state_access)
    if (OSD_OPENGL_HAS(ARB_direct_state_access)) {
        glNamedBufferSubData(_vbo,
                             startVertex * _numElements * sizeof(float),
size, src);
    } else
#endif
    {
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferSubData(GL_ARRAY_BUFFER,
                        startVertex * _numElements * sizeof(float), size, src);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

int
GLVertexBuffer::GetNumElements() const {

    return _numElements;
}

int
GLVertexBuffer::GetNumVertices() const {

    return _numVertices;
}

GLuint
GLVertexBuffer::BindVBO(void * /*deviceContext*/) {

    return _vbo;
}

bool
GLVertexBuffer::allocate() {

    int size = _numElements * _numVertices * (int) sizeof(float);


#if defined(GL_ARB_direct_state_access)
    if (OSD_OPENGL_HAS(ARB_direct_state_access)) {
        glCreateBuffers(1, &_vbo);
        glNamedBufferDataEXT(_vbo, size, 0, GL_DYNAMIC_DRAW);
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

    return true;
}

}  // end namespace Osd

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv

