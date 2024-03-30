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

#ifndef OPENSUBDIV3_OSD_CL_D3D11_VERTEX_BUFFER_H
#define OPENSUBDIV3_OSD_CL_D3D11_VERTEX_BUFFER_H

#include "../version.h"
#include "../osd/opencl.h"

struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;

struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

///
/// \brief Concrete vertex buffer class for OpenCL subdivision and DirectX
/// drawing.
///
/// D3D11VertexBuffer implements CLVertexBufferInterface and
/// D3D11VertexBufferInterface.
///
/// An instance of this buffer class can be passed to D3D11ComputeEvaluator.
///
class CLD3D11VertexBuffer {
public:
    /// Creator. Returns NULL if error.
    static CLD3D11VertexBuffer * Create(int numElements, int numVertices,
                                        cl_context clContext,
                                        ID3D11DeviceContext *deviceContext);

    /// template version for custom context (OpenCL) used by OsdMesh
    template<typename DEVICE_CONTEXT>
    static CLD3D11VertexBuffer * Create(int numElements, int numVertices,
                                        DEVICE_CONTEXT context) {
        return Create(numElements, numVertices,
                      context->GetContext(),
                      context->GetDeviceContext());
    }


    /// Destructor.
    virtual ~CLD3D11VertexBuffer();

    /// This method is meant to be used in client code in order to provide coarse
    /// vertices data to Osd.
    void UpdateData(const float *src, int startVertex, int numVertices,
                    cl_command_queue clQueue);

    /// template version for custom context (OpenCL) used by OsdMesh
    template<typename DEVICE_CONTEXT>
    void UpdateData(const float *src, int startVertex, int numVertices,
                    DEVICE_CONTEXT context) {
        UpdateData(src, startVertex, numVertices, context->GetCommandQueue());
    }

    /// Returns how many elements defined in this vertex buffer.
    int GetNumElements() const;

    /// Returns how many vertices allocated in this vertex buffer.
    int GetNumVertices() const;

    /// Returns the CL memory object.
    cl_mem BindCLBuffer(cl_command_queue queue);

    /// Returns the D3D11 buffer object.
    ID3D11Buffer *BindD3D11Buffer(ID3D11DeviceContext *deviceContext);

    /// Returns the D3D11 buffer object (for Osd::Mesh interface)
    ID3D11Buffer *BindVBO(ID3D11DeviceContext *deviceContext) {
        return BindD3D11Buffer(deviceContext);
    }

    /// template version for custom context (OpenCL) used by OsdMesh
    template<typename DEVICE_CONTEXT>
    ID3D11Buffer *BindVBO(DEVICE_CONTEXT context) {
        return BindD3D11Buffer(context->GetDeviceContext());
    }

protected:
    /// Constructor.
    CLD3D11VertexBuffer(int numElements, int numVertices);

    /// Allocates this buffer and registers as a cl resource.
    /// Returns true if success.
    bool allocate(cl_context clContext, ID3D11Device *device);

    /// Acquire a resource from DirectX.
    void map(cl_command_queue queue);

    /// Releases a resource to DirectX.
    void unmap();

private:
    int _numElements;
    int _numVertices;
    ID3D11Buffer *_d3d11Buffer;
    cl_command_queue _clQueue;
    cl_mem _clMemory;

    bool _clMapped;

};

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv

#endif  // OPENSUBDIV3_OSD_CL_D3D11_VERTEX_BUFFER_H
