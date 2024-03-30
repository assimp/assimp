//
//   Copyright 2014 DreamWorks Animation LLC.
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

#include "../far/patchDescriptor.h"

#include <cassert>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {



//
// Lists of valid patch Descriptors for each subdivision scheme
//
// Historically this has only included the non-linear patch types, though
// it is possible for linear patches to represent irregularities for both
// Catmark and Loop, and the Bilinear scheme is adaptively refined into
// linear quads (e.g. a pentagon becoming five quads).
//

ConstPatchDescriptorArray
PatchDescriptor::GetAdaptivePatchDescriptors(Sdc::SchemeType type) {

    static PatchDescriptor _loopDescriptors[] = {
        PatchDescriptor(LOOP),
        PatchDescriptor(GREGORY_TRIANGLE),
    };

    static PatchDescriptor _catmarkDescriptors[] = {
        PatchDescriptor(REGULAR),
        PatchDescriptor(GREGORY),
        PatchDescriptor(GREGORY_BOUNDARY),
        PatchDescriptor(GREGORY_BASIS),
    };

    switch (type) {
        case Sdc::SCHEME_BILINEAR :
            return ConstPatchDescriptorArray(0, 0);
        case Sdc::SCHEME_CATMARK :
            return ConstPatchDescriptorArray(_catmarkDescriptors,
                (int)(sizeof(_catmarkDescriptors)/sizeof(PatchDescriptor)));
        case Sdc::SCHEME_LOOP :
            return ConstPatchDescriptorArray(_loopDescriptors,
                (int)(sizeof(_loopDescriptors)/sizeof(PatchDescriptor)));
        default:
          assert(0);
    }
    return ConstPatchDescriptorArray(0, 0);;
}

void
PatchDescriptor::print() const {
    static char const * types[13] = {
        "NON_PATCH", "POINTS", "LINES", "QUADS", "TRIANGLES", "LOOP",
            "REGULAR", "GREGORY", "GREGORY_BOUNDARY", "GREGORY_BASIS",
            "GREGORY_TRIANGLE"};

    printf("    type %s\n",
        types[_type]);
}



} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
