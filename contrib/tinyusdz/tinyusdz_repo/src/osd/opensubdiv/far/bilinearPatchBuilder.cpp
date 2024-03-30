//
//   Copyright 2018 DreamWorks Animation LLC.
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

#include "../far/bilinearPatchBuilder.h"

#include <cassert>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

using Vtr::internal::Level;
using Vtr::internal::FVarLevel;
using Vtr::internal::Refinement;

namespace Far {

namespace {

    //
    //  The patch type associated with each basis for Bilinear -- quickly indexed
    //  from an array.  The patch type here is essentially the quad form of each
    //  basis.
    //
    PatchDescriptor::Type patchTypeFromBasisArray[] = {
            PatchDescriptor::NON_PATCH,      // undefined
            PatchDescriptor::QUADS,          // regular
            PatchDescriptor::GREGORY_BASIS,  // Gregory
            PatchDescriptor::QUADS,          // linear
            PatchDescriptor::NON_PATCH };    // Bezier -- for future use
};

BilinearPatchBuilder::BilinearPatchBuilder(
    TopologyRefiner const& refiner, Options const& options) :
        PatchBuilder(refiner, options) {

    _regPatchType   = patchTypeFromBasisArray[_options.regBasisType];
    _irregPatchType = (_options.irregBasisType == BASIS_UNSPECIFIED)
                    ? _regPatchType
                    : patchTypeFromBasisArray[_options.irregBasisType];

    _nativePatchType = PatchDescriptor::QUADS;
    _linearPatchType = PatchDescriptor::QUADS;
}

BilinearPatchBuilder::~BilinearPatchBuilder() {
}

PatchDescriptor::Type
BilinearPatchBuilder::patchTypeFromBasis(BasisType basis) const {

    return patchTypeFromBasisArray[(int)basis];
}

template <typename REAL>
int
BilinearPatchBuilder::convertSourcePatch(SourcePatch const &   sourcePatch,
                                         PatchDescriptor::Type patchType,
                                         SparseMatrix<REAL> &  matrix) const {

    assert("Conversion from Bilinear patches to other bases not yet supported" == 0);

    //  For suppressing warnings until implemented...
    if (sourcePatch.GetNumSourcePoints() == 0) return -1;
    if (patchType == PatchDescriptor::NON_PATCH) return -1;
    if (matrix.GetNumRows() <= 0) return -1;
    return -1;
}

int
BilinearPatchBuilder::convertToPatchType(SourcePatch const &   sourcePatch,
                                         PatchDescriptor::Type patchType,
                                         SparseMatrix<float> & matrix) const {
    return convertSourcePatch(sourcePatch, patchType, matrix);
}
int
BilinearPatchBuilder::convertToPatchType(SourcePatch const &    sourcePatch,
                                         PatchDescriptor::Type  patchType,
                                         SparseMatrix<double> & matrix) const {
    return convertSourcePatch(sourcePatch, patchType, matrix);
}

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
