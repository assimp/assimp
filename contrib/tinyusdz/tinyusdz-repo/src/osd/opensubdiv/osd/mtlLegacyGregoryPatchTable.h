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

#ifndef OPENSUBDIV3_OSD_MTL_LEGACY_GREGORY_PATCH_TABLE_H
#define OPENSUBDIV3_OSD_MTL_LEGACY_GREGORY_PATCH_TABLE_H

#include "../version.h"
#include "../far/patchTable.h"
#include "../osd/nonCopyable.h"
#include "../osd/mtlCommon.h"

@protocol MTLDevice;
@protocol MTLBuffer;

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

class MTLLegacyGregoryPatchTable
: private NonCopyable<MTLLegacyGregoryPatchTable>
{
public:
    ~MTLLegacyGregoryPatchTable();

    template<typename DEVICE_CONTEXT>
    static MTLLegacyGregoryPatchTable* Create(Far::PatchTable const* farPatchTable, DEVICE_CONTEXT context) {
        return Create(farPatchTable, context);
    }

    static MTLLegacyGregoryPatchTable* Create(Far::PatchTable const* farPatchTable, MTLContext* context);

    void UpdateVertexBuffer(id<MTLBuffer> vbo, int numVertices, int numVertexElements, MTLContext* context);

    id<MTLBuffer> GetVertexBuffer() const
    {
        return _vertexBuffer;
    }

    id<MTLBuffer> GetVertexValenceBuffer() const
    {
        return _vertexValenceBuffer;
    }

    id<MTLBuffer> GetQuadOffsetsBuffer() const
    {
        return _quadOffsetsBuffer;
    }

    int GetQuadOffsetsBase(Far::PatchDescriptor::Type type)
    {
        if(type == Far::PatchDescriptor::GREGORY_BOUNDARY)
            return _quadOffsetsBase[1];
        return _quadOffsetsBase[0];
    }

private:
    id<MTLBuffer> _vertexBuffer;
    id<MTLBuffer> _vertexValenceBuffer;
    id<MTLBuffer> _quadOffsetsBuffer;
    int _quadOffsetsBase[2];
};

} //end namespace Osd

} //end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} //end namespace OpenSuddiv

#endif // OPENSUBDIV3_OSD_MTL_LEGACY_GREGORY_PATCH_TABLE_H
