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

#include "../osd/d3d11LegacyGregoryPatchTable.h"

#include <D3D11.h>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

D3D11LegacyGregoryPatchTable::D3D11LegacyGregoryPatchTable() :
    _vertexValenceBuffer(0), _quadOffsetsBuffer(0),
    _vertexSRV(0), _vertexValenceSRV(0), _quadOffsetsSRV(0) {
    _quadOffsetsBase[0] = _quadOffsetsBase[1] = 0;
}

D3D11LegacyGregoryPatchTable::~D3D11LegacyGregoryPatchTable() {
    if (_vertexValenceBuffer) _vertexValenceBuffer->Release();
    if (_quadOffsetsBuffer)   _quadOffsetsBuffer->Release();
    if (_vertexSRV)           _vertexSRV->Release();
    if (_vertexValenceSRV)    _vertexValenceSRV->Release();
    if (_quadOffsetsSRV)      _quadOffsetsSRV->Release();
}

D3D11LegacyGregoryPatchTable *
D3D11LegacyGregoryPatchTable::Create(Far::PatchTable const *farPatchTable,
                                     ID3D11DeviceContext *pd3d11DeviceContext) {
    ID3D11Device *pd3d11Device = NULL;
    pd3d11DeviceContext->GetDevice(&pd3d11Device);
    assert(pd3d11Device);

    D3D11LegacyGregoryPatchTable *result = new D3D11LegacyGregoryPatchTable();

    Far::PatchTable::VertexValenceTable const &
        valenceTable = farPatchTable->GetVertexValenceTable();
    Far::PatchTable::QuadOffsetsTable const &
        quadOffsetsTable = farPatchTable->GetQuadOffsetsTable();

    if (! valenceTable.empty()) {
        D3D11_BUFFER_DESC bd;
        bd.ByteWidth = UINT(valenceTable.size() * sizeof(unsigned int));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        bd.CPUAccessFlags = 0;
        bd.MiscFlags = 0;
        bd.StructureByteStride = sizeof(unsigned int);
        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = &valenceTable[0];
        HRESULT hr = pd3d11Device->CreateBuffer(&bd, &initData,
                                                &result->_vertexValenceBuffer);
        if (FAILED(hr)) {
            delete result;
            return NULL;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
        ZeroMemory(&srvd, sizeof(srvd));
        srvd.Format = DXGI_FORMAT_R32_SINT;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvd.Buffer.FirstElement = 0;
        srvd.Buffer.NumElements = UINT(valenceTable.size());
        hr = pd3d11Device->CreateShaderResourceView(
            result->_vertexValenceBuffer, &srvd,
            &result->_vertexValenceSRV);
        if (FAILED(hr)) {
            delete result;
            return NULL;
        }
    }

    if (! quadOffsetsTable.empty()) {
        D3D11_BUFFER_DESC bd;
        bd.ByteWidth = UINT(quadOffsetsTable.size() * sizeof(unsigned int));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        bd.CPUAccessFlags = 0;
        bd.MiscFlags = 0;
        bd.StructureByteStride = sizeof(unsigned int);
        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = &quadOffsetsTable[0];
        HRESULT hr = pd3d11Device->CreateBuffer(&bd, &initData,
                                                &result->_quadOffsetsBuffer);
        if (FAILED(hr)) {
            delete result;
            return NULL;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
        ZeroMemory(&srvd, sizeof(srvd));
        srvd.Format = DXGI_FORMAT_R32_SINT;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvd.Buffer.FirstElement = 0;
        srvd.Buffer.NumElements = UINT(quadOffsetsTable.size());
        hr = pd3d11Device->CreateShaderResourceView(
            result->_quadOffsetsBuffer, &srvd, &result->_quadOffsetsSRV);
        if (FAILED(hr)) {
            delete result;
            return NULL;
        }
    }

    result->_quadOffsetsBase[0] = 0;
    result->_quadOffsetsBase[1] = 0;

    // scan patchtable to find quadOffsetsBase.
    for (int i = 0; i < farPatchTable->GetNumPatchArrays(); ++i) {
        // GREGORY_BOUNDARY's quadoffsets come after GREGORY's.
        if (farPatchTable->GetPatchArrayDescriptor(i) ==
            Far::PatchDescriptor::GREGORY) {
            result->_quadOffsetsBase[1] = farPatchTable->GetNumPatches(i) * 4;
            break;
        }
    }

    return result;
}

void
D3D11LegacyGregoryPatchTable::UpdateVertexBuffer(
    ID3D11Buffer *vbo, int numVertices, int numVertexElements,
    ID3D11DeviceContext *pd3d11DeviceContext) {
    ID3D11Device *pd3d11Device = NULL;
    pd3d11DeviceContext->GetDevice(&pd3d11Device);
    assert(pd3d11Device);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    ZeroMemory(&srvd, sizeof(srvd));
    srvd.Format = DXGI_FORMAT_R32_FLOAT;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvd.Buffer.FirstElement = 0;
    srvd.Buffer.NumElements = numVertices * numVertexElements;
    HRESULT hr = pd3d11Device->CreateShaderResourceView(vbo, &srvd,
                                                        &_vertexSRV);
    if (FAILED(hr)) {
        return;
    }
}

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv
