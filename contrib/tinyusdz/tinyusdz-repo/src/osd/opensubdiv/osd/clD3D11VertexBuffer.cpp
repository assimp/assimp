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

#include "../osd/clD3D11VertexBuffer.h"

#include <cassert>
#include <D3D11.h>
#if defined(OPENSUBDIV_HAS_CL_D3D11_H)
#include <CL/cl_d3d11.h>
#elif defined(OPENSUBDIV_HAS_CL_D3D11_EXT_H)
#include <CL/cl_d3d11_ext.h>
#else
#error "d3d11.h or d3d11_ext.h must be found in cmake"
#endif

#include "../far/error.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

#if defined(OPENSUBDIV_HAS_CL_D3D11_H)
typedef clCreateFromD3D11BufferKHR_fn clCreateFromD3D11Buffer_fn;
typedef clEnqueueAcquireD3D11ObjectsKHR_fn clEnqueueAcquireD3D11Objects_fn;
typedef clEnqueueReleaseD3D11ObjectsKHR_fn clEnqueueReleaseD3D11Objects_fn;
#else
typedef clCreateFromD3D11BufferNV_fn clCreateFromD3D11Buffer_fn;
typedef clEnqueueAcquireD3D11ObjectsNV_fn clEnqueueAcquireD3D11Objects_fn;
typedef clEnqueueReleaseD3D11ObjectsNV_fn clEnqueueReleaseD3D11Objects_fn;
#endif

static clCreateFromD3D11Buffer_fn clCreateFromD3D11Buffer = NULL;
static clEnqueueAcquireD3D11Objects_fn clEnqueueAcquireD3D11Objects = NULL;
static clEnqueueReleaseD3D11Objects_fn clEnqueueReleaseD3D11Objects = NULL;

// XXX: clGetExtensionFunctionAddress is marked as deprecated and
//      clGetExtensionFunctionAddressForPlatform should be used,
//      however it requires OpenCL 1.2.
//      until we bump the requirement to 1.2, mute the deprecated warning.
#if defined(_MSC_VER)
#pragma warning(disable: 4996)
#endif


static void resolveInteropFunctions() {

    if (! clCreateFromD3D11Buffer) {
        clCreateFromD3D11Buffer =
            (clCreateFromD3D11Buffer_fn)
            clGetExtensionFunctionAddress("clCreateFromD3D11BufferKHR");
    }
    if (! clCreateFromD3D11Buffer) {
        clCreateFromD3D11Buffer =
            (clCreateFromD3D11Buffer_fn)
            clGetExtensionFunctionAddress("clCreateFromD3D11BufferNV");
    }

    if (! clEnqueueAcquireD3D11Objects) {
        clEnqueueAcquireD3D11Objects = (clEnqueueAcquireD3D11Objects_fn)
            clGetExtensionFunctionAddress("clEnqueueAcquireD3D11ObjectsKHR");
    }
    if (! clEnqueueAcquireD3D11Objects) {
        clEnqueueAcquireD3D11Objects = (clEnqueueAcquireD3D11Objects_fn)
            clGetExtensionFunctionAddress("clEnqueueAcquireD3D11ObjectsNV");
    }

    if (! clEnqueueReleaseD3D11Objects) {
        clEnqueueReleaseD3D11Objects = (clEnqueueReleaseD3D11Objects_fn)
            clGetExtensionFunctionAddress("clEnqueueReleaseD3D11ObjectsKHR");
    }
    if (! clEnqueueReleaseD3D11Objects) {
        clEnqueueReleaseD3D11Objects = (clEnqueueReleaseD3D11Objects_fn)
            clGetExtensionFunctionAddress("clEnqueueReleaseD3D11ObjectsNV");
    }
}

CLD3D11VertexBuffer::CLD3D11VertexBuffer(int numElements, int numVertices)
    : _numElements(numElements), _numVertices(numVertices),
      _d3d11Buffer(NULL), _clMemory(NULL), _clQueue(NULL), _clMapped(false) {
}

CLD3D11VertexBuffer::~CLD3D11VertexBuffer() {

    unmap();
    clReleaseMemObject(_clMemory);
    _d3d11Buffer->Release();
}

CLD3D11VertexBuffer *
CLD3D11VertexBuffer::Create(int numElements, int numVertices,
                            cl_context clContext,
                            ID3D11DeviceContext *deviceContext) {
    CLD3D11VertexBuffer *instance =
        new CLD3D11VertexBuffer(numElements, numVertices);

    ID3D11Device *device;
    deviceContext->GetDevice(&device);

    if (instance->allocate(clContext, device)) return instance;
    delete instance;
    return NULL;
}

void
CLD3D11VertexBuffer::UpdateData(const float *src, int startVertex,
                                int numVertices, cl_command_queue queue) {

    size_t size = numVertices * _numElements * sizeof(float);
    size_t offset = startVertex * _numElements * sizeof(float);

    map(queue);
    clEnqueueWriteBuffer(queue, _clMemory, true, offset, size, src, 0, NULL, NULL);
}

int
CLD3D11VertexBuffer::GetNumElements() const {

    return _numElements;
}

int
CLD3D11VertexBuffer::GetNumVertices() const {

    return _numVertices;
}

cl_mem
CLD3D11VertexBuffer::BindCLBuffer(cl_command_queue queue) {

    map(queue);
    return _clMemory;
}

ID3D11Buffer *
CLD3D11VertexBuffer::BindD3D11Buffer(ID3D11DeviceContext *deviceContext) {

    unmap();
    return _d3d11Buffer;
}

bool
CLD3D11VertexBuffer::allocate(cl_context clContext, ID3D11Device *device) {

    D3D11_BUFFER_DESC hBufferDesc;
    hBufferDesc.ByteWidth           = _numElements * _numVertices * sizeof(float);
    hBufferDesc.Usage               = D3D11_USAGE_DYNAMIC;
    hBufferDesc.BindFlags           = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_SHADER_RESOURCE;
    hBufferDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    hBufferDesc.MiscFlags           = 0;
    hBufferDesc.StructureByteStride = sizeof(float);

    HRESULT hr;
    hr = device->CreateBuffer(&hBufferDesc, NULL, &_d3d11Buffer);
    if (FAILED(hr)) {
        Far::Error(Far::FAR_RUNTIME_ERROR, "Fail in CreateBuffer\n");
        return false;
    }

    if (! clCreateFromD3D11Buffer) {
        resolveInteropFunctions();
        if (! clCreateFromD3D11Buffer) {
            return false;
        }
    }

    // register d3d11buffer as cl memory
    cl_int err;

    _clMemory = clCreateFromD3D11Buffer(clContext, CL_MEM_READ_WRITE, _d3d11Buffer, &err);

    if (err != CL_SUCCESS) return false;
    return true;
}

void
CLD3D11VertexBuffer::map(cl_command_queue queue) {

    if (_clMapped) return;
    _clQueue = queue;

    if (! clEnqueueAcquireD3D11Objects) {
        resolveInteropFunctions();
        if (! clEnqueueAcquireD3D11Objects) {
            return;
        }
    }

    clEnqueueAcquireD3D11Objects(queue, 1, &_clMemory, 0, 0, 0);
    _clMapped = true;
}

void
CLD3D11VertexBuffer::unmap() {

    if (! _clMapped) return;
    if (! clEnqueueReleaseD3D11Objects) {
        resolveInteropFunctions();
        if (! clEnqueueReleaseD3D11Objects) {
            return;
        }
    }
    clEnqueueReleaseD3D11Objects(_clQueue, 1, &_clMemory, 0, 0, 0);
    _clMapped = false;
}


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv

