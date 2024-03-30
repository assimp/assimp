//
//   Copyright 2017 DreamWorks Animation LLC.
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

#ifndef OPENSUBDIV3_FAR_BILINEAR_PATCH_BUILDER_H
#define OPENSUBDIV3_FAR_BILINEAR_PATCH_BUILDER_H

#include "../version.h"

#include "../far/patchBuilder.h"


namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

//
//  BilinearPatchBuilder
//
//  Declaration of PatchBuilder subclass supporting Sdc::SCHEME_BILINEAR.
//  Required virtual methods are included, along with any customizations
//  local to their implementation.
//
class BilinearPatchBuilder : public PatchBuilder {
public:
    BilinearPatchBuilder(TopologyRefiner const& refiner, Options const& options);
    virtual ~BilinearPatchBuilder();

protected:
    virtual PatchDescriptor::Type patchTypeFromBasis(BasisType basis) const;

    virtual int convertToPatchType(SourcePatch const &   sourcePatch,
                                   PatchDescriptor::Type patchType,
                                   SparseMatrix<float> & matrix) const;
    virtual int convertToPatchType(SourcePatch const &    sourcePatch,
                                   PatchDescriptor::Type  patchType,
                                   SparseMatrix<double> & matrix) const;
private:
    template <typename REAL>
    int convertSourcePatch(SourcePatch const &   sourcePatch,
                           PatchDescriptor::Type patchType,
                           SparseMatrix<REAL> &  matrix) const;
};

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_FAR_BILINEAR_PATCH_BUILDER_H */
