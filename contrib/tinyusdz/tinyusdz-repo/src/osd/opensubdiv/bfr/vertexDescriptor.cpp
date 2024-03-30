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

#include "../bfr/vertexDescriptor.h"
#include "../bfr/limits.h"
#include "../sdc/crease.h"

#include <cstring>
#include <cstdio>
#include <algorithm>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  Main initialize/finalize methods used by clients to delimit the
//  assignment (most work is now handled by the containing class):
//
bool
VertexDescriptor::Initialize(int numFaces) {

    //  Mark invalid if too many or too few incident faces specified:
    _isValid  = (numFaces > 0) && (numFaces <= Limits::MaxValence());
    _numFaces = _isValid ? (short) numFaces : 0;

    //  Initialize all other members regardless of the above:
    _vertSharpness = 0.0f;

    _isManifold = false;
    _isBoundary = false;

    _hasFaceSizes     = false;
    _hasEdgeSharpness = false;

    _isInitialized = _isValid;
    _isFinalized   = false;

    return _isInitialized;
}

bool
VertexDescriptor::Finalize() {

    //  Fail if already invalid:
    if (!_isValid) return false;

    //  Test for valid face size assignments while converting the sizes
    //  to offsets. Also detect if the faces are all the same size -- in
    //  which case, ignore the explicit assignments:
    if (_hasFaceSizes) {
        int  size0 = _faceSizeOffsets[0];
        bool sameSizes = true;

        int sum = 0;
        for (int i = 0; i < _numFaces; ++i) {
            int faceSize = _faceSizeOffsets[i];
            if ((faceSize < 3) || (faceSize > Limits::MaxFaceSize())) {
                _isValid = false;
                return false;
            }
            sameSizes &= (faceSize == size0);

            _faceSizeOffsets[i] = sum;
            sum += faceSize;
        }
        _faceSizeOffsets[_numFaces] = sum;

        //  No need to make use of explicit face sizes and offsets:
        if (sameSizes) {
            _hasFaceSizes = false;
        }
    }
    _isFinalized = true;
    return true;
}

//
//  Internal methods for resizing local buffers:
//
void
VertexDescriptor::initFaceSizes() {

    _faceSizeOffsets.SetSize(_numFaces + 1);
    std::fill(&_faceSizeOffsets[0], &_faceSizeOffsets[_numFaces + 1], 0);
    _hasFaceSizes = true;
}

void
VertexDescriptor::initEdgeSharpness() {

    _faceEdgeSharpness.SetSize(_numFaces * 2);
    std::fill(&_faceEdgeSharpness[0], &_faceEdgeSharpness[_numFaces * 2], 0.0f);
    _hasEdgeSharpness = true;
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
