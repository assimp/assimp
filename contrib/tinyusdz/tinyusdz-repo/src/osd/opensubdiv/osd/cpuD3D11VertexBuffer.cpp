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

#include "../osd/cpuD3D11VertexBuffer.h"
#include "../far/error.h"

#include <D3D11.h>
#include <cassert>
#include <string.h>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

CpuD3D11VertexBuffer::CpuD3D11VertexBuffer(int numElements, int numVertices)
    : _numElements(numElements), _numVertices(numVertices),
      _d3d11Buffer(NULL), _cpuBuffer(NULL) {
}

CpuD3D11VertexBuffer::~CpuD3D11VertexBuffer() {

    delete[] _cpuBuffer;

    if (_d3d11Buffer) _d3d11Buffer->Release();
}

CpuD3D11VertexBuffer *
CpuD3D11VertexBuffer::Create(int numElements, int numVertices,
                             ID3D11DeviceContext *deviceContext) {

    CpuD3D11VertexBuffer *instance =
        new CpuD3D11VertexBuffer(numElements, numVertices);
    ID3D11Device *device;
    deviceContext->GetDevice(&device);
    if (instance->allocate(device)) return instance;
    delete instance;
    return NULL;
}

void
CpuD3D11VertexBuffer::UpdateData(const float *src, int startVertex,
                                 int numVertices,
                                 void * /*deviceContext*/) {

    memcpy(_cpuBuffer + startVertex * _numElements, src,
           _numElements * numVertices * sizeof(float));
}

int
CpuD3D11VertexBuffer::GetNumElements() const {

    return _numElements;
}

int
CpuD3D11VertexBuffer::GetNumVertices() const {

    return _numVertices;
}

float*
CpuD3D11VertexBuffer::BindCpuBuffer() {

    return _cpuBuffer;
}

ID3D11Buffer *
CpuD3D11VertexBuffer::BindD3D11Buffer(ID3D11DeviceContext *deviceContext) {

    assert(deviceContext);

    D3D11_MAPPED_SUBRESOURCE resource;
    HRESULT hr = deviceContext->Map(_d3d11Buffer, 0,
                                    D3D11_MAP_WRITE_DISCARD, 0, &resource);

    if (FAILED(hr)) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "Fail to map buffer\n");
        return NULL;
    }

    int size = _numElements * _numVertices * sizeof(float);

    memcpy(resource.pData, _cpuBuffer, size);

    deviceContext->Unmap(_d3d11Buffer, 0);

    return _d3d11Buffer;
}

bool
CpuD3D11VertexBuffer::allocate(ID3D11Device *device) {

    _cpuBuffer = new float[_numElements * _numVertices];

    // XXX: should move this constructor to factory for error handling
    D3D11_BUFFER_DESC hBufferDesc;
    hBufferDesc.ByteWidth = _numElements * _numVertices * sizeof(float);
    hBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    hBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_SHADER_RESOURCE;
    hBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hBufferDesc.MiscFlags = 0;
    hBufferDesc.StructureByteStride = sizeof(float);  // XXX ?

    HRESULT hr;
    hr = device->CreateBuffer(&hBufferDesc, NULL, &_d3d11Buffer);
    if (FAILED(hr)) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                 "Fail in CreateBuffer\n");
        return false;
    }
    return true;
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv

