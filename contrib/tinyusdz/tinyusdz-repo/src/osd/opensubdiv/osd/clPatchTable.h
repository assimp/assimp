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

#ifndef OPENSUBDIV3_OSD_CL_PATCH_TABLE_H
#define OPENSUBDIV3_OSD_CL_PATCH_TABLE_H

#include "../version.h"

#include "../osd/opencl.h"
#include "../osd/nonCopyable.h"
#include "../osd/types.h"

#include <vector>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far{
    class PatchTable;
};

namespace Osd {

/// \brief CL patch table
///
/// This class is a CL buffer representation of Far::PatchTable.
///
/// CLEvaluator consumes this table to evaluate on the patches.
///
///
class CLPatchTable : private NonCopyable<CLPatchTable> {
public:
    /// Creator. Returns NULL if error
    static CLPatchTable *Create(Far::PatchTable const *patchTable,
                                cl_context clContext);

    template <typename DEVICE_CONTEXT>
    static CLPatchTable * Create(Far::PatchTable const *patchTable,
                                 DEVICE_CONTEXT context) {
        return Create(patchTable, context->GetContext());
    }

    /// Destructor
    ~CLPatchTable();

    /// Returns the CL memory of the array of Osd::PatchArray buffer
    cl_mem GetPatchArrayBuffer() const { return _patchArrays; }

    /// Returns the CL memory of the patch control vertices
    cl_mem GetPatchIndexBuffer() const { return _indexBuffer; }

    /// Returns the CL memory of the array of Osd::PatchParam buffer
    cl_mem GetPatchParamBuffer() const { return _patchParamBuffer; }

    /// Returns the CL memory of the array of Osd::PatchArray buffer
    cl_mem GetVaryingPatchArrayBuffer() const { return _varyingPatchArrays; }

    /// Returns the CL memory of the varying control vertices
    cl_mem GetVaryingPatchIndexBuffer() const { return _varyingIndexBuffer; }

    /// Returns the number of face-varying channel buffers
    int GetNumFVarChannels() const { return (int)_fvarPatchArrays.size(); }

    /// Returns the CL memory of the array of Osd::PatchArray buffer
    cl_mem GetFVarPatchArrayBuffer(int fvarChannel = 0) const { return _fvarPatchArrays[fvarChannel]; }

    /// Returns the CL memory of the face-varying control vertices
    cl_mem GetFVarPatchIndexBuffer(int fvarChannel = 0) const { return _fvarIndexBuffers[fvarChannel]; }

    /// Returns the CL memory of the array of Osd::PatchParam buffer
    cl_mem GetFVarPatchParamBuffer(int fvarChannel = 0) const { return _fvarParamBuffers[fvarChannel]; }

protected:
    CLPatchTable();

    bool allocate(Far::PatchTable const *patchTable, cl_context clContext);

    cl_mem _patchArrays;
    cl_mem _indexBuffer;
    cl_mem _patchParamBuffer;

    cl_mem _varyingPatchArrays;
    cl_mem _varyingIndexBuffer;

    std::vector<cl_mem> _fvarPatchArrays;
    std::vector<cl_mem> _fvarIndexBuffers;
    std::vector<cl_mem> _fvarParamBuffers;

};

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv

#endif  // OPENSUBDIV3_OSD_CL_PATCH_TABLE_H
