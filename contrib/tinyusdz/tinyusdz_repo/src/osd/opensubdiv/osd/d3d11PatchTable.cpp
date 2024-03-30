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

#include "../osd/d3d11PatchTable.h"

#include <D3D11.h>
#include "../far/patchTable.h"
#include "../osd/cpuPatchTable.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

D3D11PatchTable::D3D11PatchTable() :
    _indexBuffer(0), _patchParamBuffer(0), _patchParamBufferSRV(0) {
}

D3D11PatchTable::~D3D11PatchTable() {
    if (_indexBuffer) _indexBuffer->Release();
    if (_patchParamBuffer) _patchParamBuffer->Release();
    if (_patchParamBufferSRV) _patchParamBufferSRV->Release();
}

D3D11PatchTable *
D3D11PatchTable::Create(Far::PatchTable const *farPatchTable,
                        ID3D11DeviceContext *pd3d11DeviceContext) {
    D3D11PatchTable *instance = new D3D11PatchTable();
    if (instance->allocate(farPatchTable, pd3d11DeviceContext))
        return instance;
    delete instance;
    return 0;
}

bool
D3D11PatchTable::allocate(Far::PatchTable const *farPatchTable,
                          ID3D11DeviceContext *pd3d11DeviceContext) {
    ID3D11Device *pd3d11Device = NULL;
    pd3d11DeviceContext->GetDevice(&pd3d11Device);
    assert(pd3d11Device);

    CpuPatchTable patchTable(farPatchTable);

    size_t numPatchArrays = patchTable.GetNumPatchArrays();
    size_t indexSize = patchTable.GetPatchIndexSize();
    size_t patchParamSize = patchTable.GetPatchParamSize();

    // copy patch array
    _patchArrays.assign(patchTable.GetPatchArrayBuffer(),
                        patchTable.GetPatchArrayBuffer() + numPatchArrays);

    // index buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.ByteWidth = (int)indexSize * sizeof(int);
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;
    bd.StructureByteStride = sizeof(int);
    HRESULT hr = pd3d11Device->CreateBuffer(&bd, NULL, &_indexBuffer);
    if (FAILED(hr)) {
        return false;
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = pd3d11DeviceContext->Map(_indexBuffer, 0,
                                  D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        return false;
    }
    unsigned int * indexBuffer = (unsigned int *) mappedResource.pData;
    memcpy(indexBuffer,
           patchTable.GetPatchIndexBuffer(),
           indexSize * sizeof(unsigned int));

    pd3d11DeviceContext->Unmap(_indexBuffer, 0);

    // patchparam buffer
    ZeroMemory(&bd, sizeof(bd));
    bd.ByteWidth = (int)patchParamSize * sizeof(PatchParam);
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;
    bd.StructureByteStride = sizeof(PatchParam);
    hr = pd3d11Device->CreateBuffer(&bd, NULL, &_patchParamBuffer);
    if (FAILED(hr)) {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    ZeroMemory(&srvd, sizeof(srvd));
    srvd.Format = DXGI_FORMAT_R32G32B32_UINT;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvd.Buffer.FirstElement = 0;
    srvd.Buffer.NumElements = (int)patchParamSize;
    hr = pd3d11Device->CreateShaderResourceView(
        _patchParamBuffer, &srvd, &_patchParamBufferSRV);
    if (FAILED(hr)) {
        return false;
    }
    hr = pd3d11DeviceContext->Map(_patchParamBuffer, 0,
                                  D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        return false;
    }
    unsigned int *dst = (unsigned int *) mappedResource.pData;
    memcpy(dst,
           patchTable.GetPatchParamBuffer(),
           patchParamSize * sizeof(PatchParam));

    pd3d11DeviceContext->Unmap(_patchParamBuffer, 0);

    return true;
}


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv

