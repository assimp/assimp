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

#ifndef OPENSUBDIV3_OSD_MTL_PATCH_TABLE_H
#define OPENSUBDIV3_OSD_MTL_PATCH_TABLE_H

#include "../version.h"
#include "../far/patchDescriptor.h"
#include "../osd/nonCopyable.h"
#include "../osd/types.h"
#include "../osd/mtlCommon.h"

@protocol MTLDevice;
@protocol MTLBuffer;

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {
    class PatchTable;
};

namespace Osd {

class MTLPatchTable : private NonCopyable<MTLPatchTable> {
public:
    typedef id<MTLBuffer> VertexBufferBinding;

    MTLPatchTable();
    ~MTLPatchTable();

    template<typename DEVICE_CONTEXT>
    static MTLPatchTable *Create(Far::PatchTable const *farPatchTable, DEVICE_CONTEXT context)
    {
        return Create(farPatchTable, context);
    }

    static MTLPatchTable *Create(Far::PatchTable const *farPatchTable, MTLContext* context);

    PatchArrayVector const &GetPatchArrays() const { return _patchArrays; }
    id<MTLBuffer> GetPatchIndexBuffer() const { return _indexBuffer; }
    id<MTLBuffer> GetPatchParamBuffer() const { return _patchParamBuffer; }

    PatchArrayVector const &GetVaryingPatchArrays() const { return _varyingPatchArrays; }
    id<MTLBuffer> GetVaryingPatchIndexBuffer() const { return _varyingPatchIndexBuffer; }

    int GetNumFVarChannels() const { return (int)_fvarPatchArrays.size(); }
    PatchArrayVector const &GetFVarPatchArrays(int fvarChannel = 0) const { return _fvarPatchArrays[fvarChannel]; }
    id<MTLBuffer> GetFVarPatchIndexBuffer(int fvarChannel = 0) const { return _fvarIndexBuffers[fvarChannel]; }
    id<MTLBuffer> GetFVarPatchParamBuffer(int fvarChannel = 0) const { return _fvarParamBuffers[fvarChannel]; }

protected:
    bool allocate(Far::PatchTable const *farPatchTable, MTLContext* context);

    PatchArrayVector _patchArrays;

    id<MTLBuffer> _indexBuffer;
    id<MTLBuffer> _patchParamBuffer;

    PatchArrayVector _varyingPatchArrays;

    id<MTLBuffer> _varyingPatchIndexBuffer;

    std::vector<PatchArrayVector> _fvarPatchArrays;
    std::vector<id<MTLBuffer>> _fvarIndexBuffers;
    std::vector<id<MTLBuffer>> _fvarParamBuffers;
};

} // end namespace Osd

} //end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} //end namespace OpenSubdiv

#endif //end OPENSUBDIV3_OSD_MTL_PATCH_TABLE_H
