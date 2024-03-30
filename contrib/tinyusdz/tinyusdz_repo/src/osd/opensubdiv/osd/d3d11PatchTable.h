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

#ifndef OPENSUBDIV3_OSD_GL_PATCH_TABLE_H
#define OPENSUBDIV3_OSD_GL_PATCH_TABLE_H

#include "../version.h"

#include <vector>
#include "../far/patchDescriptor.h"
#include "../osd/nonCopyable.h"
#include "../osd/types.h"

struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far{
    class PatchTable;
};

namespace Osd {

class D3D11PatchTable : private NonCopyable<D3D11PatchTable> {
public:
    typedef ID3D11Buffer * VertexBufferBinding;

    D3D11PatchTable();
    ~D3D11PatchTable();

    template<typename DEVICE_CONTEXT>
    static D3D11PatchTable *Create(Far::PatchTable const *farPatchTable,
                                   DEVICE_CONTEXT context) {
        return Create(farPatchTable, context->GetDeviceContext());
    }

    static D3D11PatchTable *Create(Far::PatchTable const *farPatchTable,
                                   ID3D11DeviceContext *deviceContext);

    PatchArrayVector const &GetPatchArrays() const {
        return _patchArrays;
    }

    /// Returns the index buffer containing the patch control vertices
    ID3D11Buffer* GetPatchIndexBuffer() const {
        return _indexBuffer;
    }

    /// Returns the SRV containing the patch parameter
    ID3D11ShaderResourceView* GetPatchParamSRV() const {
        return _patchParamBufferSRV;
    }

protected:
    // allocate buffers from patchTable
    bool allocate(Far::PatchTable const *farPatchTable,
                  ID3D11DeviceContext *deviceContext);

    PatchArrayVector _patchArrays;

    ID3D11Buffer             *_indexBuffer;
    ID3D11Buffer             *_patchParamBuffer;
    ID3D11ShaderResourceView *_patchParamBufferSRV;
};


}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv

#endif  // OPENSUBDIV3_OSD_GL_PATCH_TABLE_H
