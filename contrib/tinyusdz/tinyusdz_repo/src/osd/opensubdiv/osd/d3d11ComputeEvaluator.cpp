//
//   Copyright 2015 Pixar
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

#include "../osd/d3d11ComputeEvaluator.h"

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

#define INITGUID        // for IID_ID3D11ShaderReflection
#include <D3D11.h>
#include <D3D11shader.h>
#include <D3Dcompiler.h>

#include "../far/error.h"
#include "../far/stencilTable.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

static const char *shaderSource =
#include "../osd/hlslComputeKernel.gen.h"
;

// ----------------------------------------------------------------------------

// must match constant buffer declaration in hlslComputeKernel.hlsl
__declspec(align(16))

struct KernelUniformArgs {

    int start;     // batch
    int end;

    int srcOffset;
    int dstOffset;
};

// ----------------------------------------------------------------------------

template <typename T>
static ID3D11Buffer *createBuffer(std::vector<T> const &src,
                                  ID3D11Device *device) {

    size_t size = src.size()*sizeof(T);

    ID3D11Buffer *buffer = NULL;
    D3D11_BUFFER_DESC bd;
    bd.ByteWidth = (unsigned int)size;
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = &src.at(0);

    HRESULT hr = device->CreateBuffer(&bd, &initData, &buffer);
    if (FAILED(hr)) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "Error creating compute table buffer\n");
        return NULL;
    }
    return buffer;
}

static ID3D11ShaderResourceView *createSRV(ID3D11Buffer *buffer,
                                           DXGI_FORMAT format,
                                           ID3D11Device *device,
                                           size_t size) {
    ID3D11ShaderResourceView *srv = NULL;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    ZeroMemory(&srvd, sizeof(srvd));
    srvd.Format = format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvd.Buffer.FirstElement = 0;
    srvd.Buffer.NumElements = (unsigned int)size;

    HRESULT hr = device->CreateShaderResourceView(buffer, &srvd, &srv);
    if (FAILED(hr)) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "Error creating compute table shader resource view\n");
        return NULL;
    }
    return srv;
}

D3D11StencilTable::D3D11StencilTable(Far::StencilTable const *stencilTable,
                                     ID3D11DeviceContext *deviceContext)
 {
    ID3D11Device *device = NULL;
    deviceContext->GetDevice(&device);
    assert(device);

    _numStencils = stencilTable->GetNumStencils();
    if (_numStencils > 0) {
        std::vector<int> const &sizes = stencilTable->GetSizes();

        _sizesBuffer   = createBuffer(sizes, device);
        _offsetsBuffer = createBuffer(stencilTable->GetOffsets(), device);
        _indicesBuffer = createBuffer(stencilTable->GetControlIndices(), device);
        _weightsBuffer = createBuffer(stencilTable->GetWeights(), device);

        _sizes   = createSRV(_sizesBuffer,   DXGI_FORMAT_R32_SINT, device,
                             stencilTable->GetSizes().size());
        _offsets = createSRV(_offsetsBuffer, DXGI_FORMAT_R32_SINT, device,
                             stencilTable->GetOffsets().size());
        _indices = createSRV(_indicesBuffer, DXGI_FORMAT_R32_SINT, device,
                             stencilTable->GetControlIndices().size());
        _weights= createSRV(_weightsBuffer, DXGI_FORMAT_R32_FLOAT, device,
                            stencilTable->GetWeights().size());
    } else {
        _sizes = _offsets = _indices = _weights = NULL;
        _sizesBuffer = _offsetsBuffer = _indicesBuffer = _weightsBuffer = NULL;
    }
}

D3D11StencilTable::~D3D11StencilTable() {
    SAFE_RELEASE(_sizes);
    SAFE_RELEASE(_sizesBuffer);
    SAFE_RELEASE(_offsets);
    SAFE_RELEASE(_offsetsBuffer);
    SAFE_RELEASE(_indices);
    SAFE_RELEASE(_indicesBuffer);
    SAFE_RELEASE(_weights);
    SAFE_RELEASE(_weightsBuffer);
}

// ---------------------------------------------------------------------------


D3D11ComputeEvaluator::D3D11ComputeEvaluator() :
    _computeShader(NULL),
    _classLinkage(NULL),
    _singleBufferKernel(NULL),
    _separateBufferKernel(NULL),
    _uniformArgs(NULL),
    _workGroupSize(64) {

}

D3D11ComputeEvaluator *
D3D11ComputeEvaluator::Create(BufferDescriptor const &srcDesc,
                              BufferDescriptor const &dstDesc,
                              BufferDescriptor const &duDesc,
                              BufferDescriptor const &dvDesc,
                              ID3D11DeviceContext *deviceContext) {
    return Create(srcDesc, dstDesc, duDesc, dvDesc,
                  BufferDescriptor(),
                  BufferDescriptor(),
                  BufferDescriptor(),
                  deviceContext);
}

D3D11ComputeEvaluator *
D3D11ComputeEvaluator::Create(BufferDescriptor const &srcDesc,
                              BufferDescriptor const &dstDesc,
                              BufferDescriptor const &duDesc,
                              BufferDescriptor const &dvDesc,
                              BufferDescriptor const &duuDesc,
                              BufferDescriptor const &duvDesc,
                              BufferDescriptor const &dvvDesc,
                              ID3D11DeviceContext *deviceContext) {
    (void)deviceContext;  // not used

    // TODO: implements derivatives
    (void)duDesc;
    (void)dvDesc;

    D3D11ComputeEvaluator *instance = new D3D11ComputeEvaluator();
    if (instance->Compile(srcDesc, dstDesc, deviceContext)) return instance;
    delete instance;
    return NULL;
}

D3D11ComputeEvaluator::~D3D11ComputeEvaluator() {
    SAFE_RELEASE(_computeShader);
    SAFE_RELEASE(_classLinkage);
    SAFE_RELEASE(_singleBufferKernel);
    SAFE_RELEASE(_separateBufferKernel);
    SAFE_RELEASE(_uniformArgs);
}

bool
D3D11ComputeEvaluator::Compile(BufferDescriptor const &srcDesc,
                               BufferDescriptor const &dstDesc,
                               ID3D11DeviceContext *deviceContext) {

    if (srcDesc.length > dstDesc.length) {
        Far::Error(Far::FAR_RUNTIME_ERROR,
                   "srcDesc length must be less than or equal to "
                   "dstDesc length.\n");
        return false;
    }

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(D3D10_SHADER_RESOURCES_MAY_ALIAS)
     dwShaderFlags |= D3D10_SHADER_RESOURCES_MAY_ALIAS;
#endif

#ifdef _DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    std::ostringstream ss;
    ss << srcDesc.length;  std::string lengthValue(ss.str()); ss.str("");
    ss << srcDesc.stride;  std::string srcStrideValue(ss.str()); ss.str("");
    ss << dstDesc.stride;  std::string dstStrideValue(ss.str()); ss.str("");
    ss << _workGroupSize;  std::string workgroupSizeValue(ss.str()); ss.str("");

    D3D_SHADER_MACRO defines[] =
        { { "LENGTH", lengthValue.c_str() },
          { "SRC_STRIDE", srcStrideValue.c_str() },
          { "DST_STRIDE", dstStrideValue.c_str() },
          { "WORK_GROUP_SIZE", workgroupSizeValue.c_str() },
          { 0, 0 } };

    ID3DBlob * computeShaderBuffer = NULL;
    ID3DBlob * errorBuffer = NULL;

    HRESULT hr = D3DCompile(shaderSource, strlen(shaderSource),
                            NULL, &defines[0], NULL,
                            "cs_main", "cs_5_0",
                            dwShaderFlags, 0,
                            &computeShaderBuffer, &errorBuffer);
    if (FAILED(hr)) {
        if (errorBuffer != NULL) {
            Far::Error(Far::FAR_RUNTIME_ERROR,
                       "Error compiling HLSL shader: %s\n",
                       (CHAR*)errorBuffer->GetBufferPointer());
            errorBuffer->Release();
            return false;
        }
    }

    ID3D11Device *device = NULL;
    deviceContext->GetDevice(&device);
    assert(device);

    device->CreateClassLinkage(&_classLinkage);
    assert(_classLinkage);

    device->CreateComputeShader(computeShaderBuffer->GetBufferPointer(),
                                computeShaderBuffer->GetBufferSize(),
                                _classLinkage,
                                &_computeShader);
    assert(_computeShader);

    ID3D11ShaderReflection *reflector;
    D3DReflect(computeShaderBuffer->GetBufferPointer(),
               computeShaderBuffer->GetBufferSize(),
               IID_ID3D11ShaderReflection, (void**) &reflector);
    assert(reflector);

    assert(reflector->GetNumInterfaceSlots() == 1);
    reflector->Release();

    computeShaderBuffer->Release();

    _classLinkage->GetClassInstance("singleBufferCompute", 0, &_singleBufferKernel);
    assert(_singleBufferKernel);
    _classLinkage->GetClassInstance("separateBufferCompute", 0, &_separateBufferKernel);
    assert(_separateBufferKernel);

    D3D11_BUFFER_DESC cbDesc;
    ZeroMemory(&cbDesc, sizeof(cbDesc));
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.ByteWidth = sizeof(KernelUniformArgs);
    device->CreateBuffer(&cbDesc, NULL, &_uniformArgs);

    return true;
}

/* static */
void
D3D11ComputeEvaluator::Synchronize(ID3D11DeviceContext *deviceContext) {
    // XXX: this is currently just for the performance measuring purpose.

    // XXXFIXME!
    ID3D11Query *query = NULL;

    ID3D11Device *device = NULL;
    deviceContext->GetDevice(&device);
    assert(device);

    D3D11_QUERY_DESC desc;
    desc.Query = D3D11_QUERY_EVENT;
    desc.MiscFlags = 0;
    device->CreateQuery(&desc, &query);

    deviceContext->Flush();
    deviceContext->End(query);
    while (S_OK != deviceContext->GetData(query, NULL, 0, 0));

    SAFE_RELEASE(query);
}

bool
D3D11ComputeEvaluator::EvalStencils(ID3D11UnorderedAccessView *srcUAV,
                                    BufferDescriptor const &srcDesc,
                                    ID3D11UnorderedAccessView *dstUAV,
                                    BufferDescriptor const &dstDesc,
                                    ID3D11ShaderResourceView *sizesSRV,
                                    ID3D11ShaderResourceView *offsetsSRV,
                                    ID3D11ShaderResourceView *indicesSRV,
                                    ID3D11ShaderResourceView *weightsSRV,
                                    int start,
                                    int end,
                                    ID3D11DeviceContext *deviceContext) const {
    assert(deviceContext);

    int count = end - start;
    if (count <= 0) return true;

    KernelUniformArgs args;
    args.start = start;
    args.end = end;
    args.srcOffset = srcDesc.offset;
    args.dstOffset = dstDesc.offset;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    deviceContext->Map(_uniformArgs, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    CopyMemory(mappedResource.pData, &args, sizeof(KernelUniformArgs));

    deviceContext->Unmap(_uniformArgs, 0);
    deviceContext->CSSetConstantBuffers(0, 1, &_uniformArgs); // b0

    // Unbind the vertexBuffer from the input assembler
    ID3D11Buffer *NULLBuffer = 0;
    UINT voffset = 0, vstride = 0;
    deviceContext->IASetVertexBuffers(0, 1, &NULLBuffer, &voffset, &vstride);
    ID3D11ShaderResourceView *NULLSRV = 0;
    deviceContext->VSSetShaderResources(0, 1, &NULLSRV);

    // bind UAV
    ID3D11UnorderedAccessView *UAViews[] = { srcUAV, dstUAV };
    ID3D11ShaderResourceView *SRViews[] = {
        sizesSRV, offsetsSRV, indicesSRV, weightsSRV };

    // bind source vertex and stencil table
    deviceContext->CSSetShaderResources(1, 4, SRViews); // t1-t4

    if (srcUAV == dstUAV) {
        deviceContext->CSSetUnorderedAccessViews(0, 1, UAViews, 0); // u0
        // Dispatch src == dst buffer
        deviceContext->CSSetShader(_computeShader, &_singleBufferKernel, 1);
        deviceContext->Dispatch((count + _workGroupSize - 1) / _workGroupSize, 1, 1);
    } else {
        deviceContext->CSSetUnorderedAccessViews(0, 2, UAViews, 0); // u0, u1
        // Dispatch src != dst buffer
        deviceContext->CSSetShader(_computeShader, &_separateBufferKernel, 1);
        deviceContext->Dispatch((count + _workGroupSize - 1) / _workGroupSize, 1, 1);
    }

    // unbind stencil table and vertexbuffers
    SRViews[0] = SRViews[1] = SRViews[2] = SRViews[3] = NULL;
    deviceContext->CSSetShaderResources(1, 4, SRViews);

    UAViews[0] = UAViews[1] = NULL;
    deviceContext->CSSetUnorderedAccessViews(0, 2, UAViews, 0);

    return true;
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv
