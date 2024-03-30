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

#ifndef OPENSUBDIV3_OSD_MTL_VERTEX_BUFFER_H
#define OPENSUBDIV3_OSD_MTL_VERTEX_BUFFER_H

#include "../version.h"
#include "../osd/mtlCommon.h"

@protocol MTLDevice;
@protocol MTLBuffer;

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

class CPUMTLVertexBuffer {
public:
    static CPUMTLVertexBuffer* Create(int numElements, int numVertices, MTLContext* context);

    void UpdateData(const float* src, int startVertex, int numVertices, MTLContext* context);

    int GetNumElements() const
    {
        return _numElements;
    }

    int GetNumVertices() const
    {
        return _numVertices;
    }

    float* BindCpuBuffer();
    id<MTLBuffer> BindMTLBuffer(MTLContext* context);

    id<MTLBuffer> BindVBO(MTLContext* context)
    {
        return BindMTLBuffer(context);
    }

protected:

    CPUMTLVertexBuffer(int numElements, int numVertices);

    bool allocate(MTLContext* context);

private:
    int _numElements;
    int _numVertices;
    id<MTLBuffer> _buffer;
    bool _dirty;
};

} //end namespace Osd

} //end namespace OPENSUBDIV_VERSION
    using namespace OPENSUBDIV_VERSION;

} //end namespace OpenSubdiv

#endif // OPENSUBDIV3_OSD_MTL_VERTEX_BUFFER_H
