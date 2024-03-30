//
//   Copyright 2021 Pixar
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

#include "../bfr/surfaceData.h"
#include "../bfr/patchTree.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {
namespace internal {

//
//  Constructors and other methods to manage data members for copy and
//  destruction:
//
SurfaceData::SurfaceData() : _cvIndices(), _param(),
    _isValid(false),
    _isDouble(false),
    _isRegular(true),
    _isLinear(false),
    _regPatchType(0),
    _regPatchMask(0),
    _irregPatch() {
}

SurfaceData & 
SurfaceData::operator=(SurfaceData const & src) {

    //  No need to explicitly manage pre-existing resources in destination
    //  as they will be either re-used or released when re-assigned

    //  No copy/operator= supported by StackBuffer so resize and copy:
    _cvIndices.SetSize(src._cvIndices.GetSize());
    std::memcpy(&_cvIndices[0],
        &src._cvIndices[0], src._cvIndices.GetSize() * sizeof(Index));

    _param = src._param;

    _isValid      = src._isValid;
    _isDouble     = src._isDouble;
    _isRegular    = src._isRegular;
    _isLinear     = src._isLinear;
    _regPatchType = src._regPatchType;
    _regPatchMask = src._regPatchMask;
    _irregPatch   = src._irregPatch;

    return *this;
}

void
SurfaceData::invalidate() {

    //  Release any attached memory before marking as invalid:
    _irregPatch = 0;

    _isValid = false;
}

} // end namespace internal
} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
