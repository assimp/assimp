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

#ifndef OPENSUBDIV3_OSD_TYPES_H
#define OPENSUBDIV3_OSD_TYPES_H

#include "../version.h"
#include "../far/patchTable.h"

#include <algorithm>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

/// \brief Coordinates set on a patch table
///
///  XXX: this class may be moved into Far
///
struct PatchCoord {
    // 5-ints struct.

    /// \brief Constructor
    ///
    /// @param handleArg    patch handle
    ///
    /// @param sArg         parametric location on the patch
    ///
    /// @param tArg         parametric location on the patch
    ///
    PatchCoord(Far::PatchTable::PatchHandle handleArg, float sArg, float tArg) :
        handle(handleArg), s(sArg), t(tArg) { }

    PatchCoord() : s(0), t(0) {
        handle.arrayIndex = 0;
        handle.patchIndex = 0;
        handle.vertIndex = 0;
    }

    Far::PatchTable::PatchHandle handle; ///< patch handle
    float s, t;              ///< parametric location on patch
};

struct PatchArray {
    // 4-ints struct.
    PatchArray(Far::PatchDescriptor desc_in,
               int numPatches_in, int indexBase_in, int primitiveIdBase_in) :
        regDesc(desc_in), desc(desc_in),
        numPatches(numPatches_in), indexBase(indexBase_in),
        stride(desc_in.GetNumControlVertices()),
        primitiveIdBase(primitiveIdBase_in) {}

    PatchArray(Far::PatchDescriptor regDesc_in, Far::PatchDescriptor irregDesc_in,
               int numPatches_in, int indexBase_in, int primitiveIdBase_in) :
        regDesc(regDesc_in), desc(irregDesc_in),
        numPatches(numPatches_in), indexBase(indexBase_in),
        stride(std::max(regDesc_in.GetNumControlVertices(),
                        irregDesc_in.GetNumControlVertices())),
        primitiveIdBase(primitiveIdBase_in) {}

    Far::PatchDescriptor const &GetDescriptor() const {
        return desc;
    }
    Far::PatchDescriptor const &GetDescriptorRegular() const {
        return regDesc;
    }
    Far::PatchDescriptor const &GetDescriptorIrregular() const {
        return desc;
    }

    int GetPatchType() const {
        return desc.GetType();
    }
    int GetPatchTypeRegular() const {
        return regDesc.GetType();
    }
    int GetPatchTypeIrregular() const {
        return desc.GetType();
    }

    int GetNumPatches() const {
        return numPatches;
    }
    int GetIndexBase() const {
        return indexBase;
    }
    int GetStride() const {
        return stride;
    }
    int GetPrimitiveIdBase() const {
        return primitiveIdBase;
    }

    // Separate regular and irregular patch descriptors for cases where the
    // array is mixed -- both will be equal if only a single type specified
    Far::PatchDescriptor regDesc;
    Far::PatchDescriptor desc;

    int numPatches;
    int indexBase;        // an offset within the index buffer
    int stride;           // stride in buffer between patches
    int primitiveIdBase;  // an offset within the patch param buffer
};

struct PatchParam : public Far::PatchParam {
    // int3 struct.
    float sharpness;
};

typedef std::vector<PatchArray> PatchArrayVector;
typedef std::vector<PatchParam> PatchParamVector;

}  // end namespace Osd

}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv


#endif  // OPENSUBDIV3_OSD_TYPES_H
