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

#include "../osd/cpuVertexBuffer.h"

#include <string.h>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

CpuVertexBuffer::CpuVertexBuffer(int numElements, int numVertices)
    : _numElements(numElements),
      _numVertices(numVertices),
      _cpuBuffer(NULL) {

    _cpuBuffer = new float[numElements * numVertices];
}

CpuVertexBuffer::~CpuVertexBuffer() {

    delete[] _cpuBuffer;
}

CpuVertexBuffer *
CpuVertexBuffer::Create(int numElements, int numVertices,
                        void * /*deviceContext*/) {

    return new CpuVertexBuffer(numElements, numVertices);
}

void
CpuVertexBuffer::UpdateData(const float *src, int startVertex, int numVertices,
                            void * /*deviceContext*/) {

    memcpy(_cpuBuffer + startVertex * _numElements,
           src, GetNumElements() * numVertices * sizeof(float));
}

int
CpuVertexBuffer::GetNumElements() const {

    return _numElements;
}

int
CpuVertexBuffer::GetNumVertices() const {

    return _numVertices;
}

float*
CpuVertexBuffer::BindCpuBuffer() {

    return _cpuBuffer;
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv

