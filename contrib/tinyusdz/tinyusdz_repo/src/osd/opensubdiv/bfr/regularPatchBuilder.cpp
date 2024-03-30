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

#include "../bfr/regularPatchBuilder.h"

#include <cstring>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  Internal helper functions:
//
namespace {
    inline int
    encodeTriBoundaryMask(int eBits, int vBits) {

        int upperBits = 0;
        int lowerBits = eBits;

        if (vBits) {
            if (eBits == 0) {
                upperBits = 1;
                lowerBits = vBits;
            } else if ((vBits == 7) &&
                       ((eBits == 1) || (eBits == 2) || (eBits == 4))) {
                upperBits = 2;
                lowerBits = eBits;
            }
        }
        return (upperBits << 3) | lowerBits;
    }
}

//
//  Static methods:
//
int
RegularPatchBuilder::GetBoundaryMask(int faceSize, Index const cvs[]) {

    bool isQuad = (faceSize == 4);
    if (isQuad) {
        int eMask = ((cvs[ 1] < 0) << 0) |
                    ((cvs[ 7] < 0) << 1) |
                    ((cvs[14] < 0) << 2) |
                    ((cvs[ 8] < 0) << 3);
        return eMask;
    } else {
        int eMask = ((cvs[ 1] < 0) << 0) |
                    ((cvs[ 9] < 0) << 1) |
                    ((cvs[ 7] < 0) << 2) ;
        int vMask = (((cvs[ 0] < 0) | (cvs[ 3] < 0)) << 0) |
                    (((cvs[ 2] < 0) | (cvs[ 6] < 0)) << 1) |
                    (((cvs[10] < 0) | (cvs[11] < 0)) << 2) ;
        return encodeTriBoundaryMask(eMask, vMask);
    }
}

//
//  Constructor (empty destructor is inline):
//
RegularPatchBuilder::RegularPatchBuilder(FaceSurface const & surface) :
        _surface(surface) {

    _isQuad = (_surface.GetTopology()._faceSize == 4);
    if (_isQuad) {
        assert(_surface.GetTopology()._regFaceSize == 4);

        _patchType = Far::PatchDescriptor::REGULAR;
        _patchSize = 16;
    } else {
        assert(_surface.GetTopology()._faceSize == 3);
        assert(_surface.GetTopology()._regFaceSize == 3);

        _patchType = Far::PatchDescriptor::LOOP;
        _patchSize = 12;
    }

    _isBoundary = _surface.GetTag().HasBoundaryVertices();
    if (!_isBoundary) {
        _boundaryMask = 0;
    } else if (_isQuad) {
        //  Boundary mask for quad trivial -- bit for each boundary edge:
        FaceVertexSubset const * C = _surface.GetSubsets();
        int eMask = ((C[0].IsBoundary() & (C[0]._numFacesBefore == 0)) << 0) |
                    ((C[1].IsBoundary() & (C[1]._numFacesBefore == 0)) << 1) |
                    ((C[2].IsBoundary() & (C[2]._numFacesBefore == 0)) << 2) |
                    ((C[3].IsBoundary() & (C[3]._numFacesBefore == 0)) << 3);
        _boundaryMask = eMask;
    } else {
        //  Boundary mask for tris not so trivial -- boundary verts can exist
        //  on tris without boundary edges, so bits for both are combined:
        FaceVertexSubset const * C = _surface.GetSubsets();
        int eMask = ((C[0].IsBoundary() & (C[0]._numFacesBefore == 0)) << 0) |
                    ((C[1].IsBoundary() & (C[1]._numFacesBefore == 0)) << 1) |
                    ((C[2].IsBoundary() & (C[2]._numFacesBefore == 0)) << 2);
        int vMask =  (C[0].IsBoundary() << 0) |
                     (C[1].IsBoundary() << 1) |
                     (C[2].IsBoundary() << 2);
        _boundaryMask = encodeTriBoundaryMask(eMask, vMask);
    }
}

//
//  Methods for gathering control vertices:
//
void
RegularPatchBuilder::gatherInteriorPatchPoints4(Index P[]) const {

    Index const * fvIndices  = &_surface.GetIndices()[0];
    Index const * fvOpposite = 0;

    //
    //  For each of the 4 corners, identify the opposite face in the ring
    //  and assign its 4 indices to the corresponding quadrant of the patch:
    //
    FaceVertex const & cTop0 = _surface.GetCornerTopology(0);
    fvOpposite = fvIndices + cTop0.GetFaceIndexOffset(cTop0.GetFaceAfter(2));
    P[ 5] = fvOpposite[0];
    P[ 4] = fvOpposite[1];
    P[ 0] = fvOpposite[2];
    P[ 1] = fvOpposite[3];
    fvIndices += cTop0.GetNumFaceVertices();

    FaceVertex const & cTop1 = _surface.GetCornerTopology(1);
    fvOpposite = fvIndices + cTop1.GetFaceIndexOffset(cTop1.GetFaceAfter(2));
    P[ 6] = fvOpposite[0];
    P[ 2] = fvOpposite[1];
    P[ 3] = fvOpposite[2];
    P[ 7] = fvOpposite[3];
    fvIndices += cTop1.GetNumFaceVertices();

    FaceVertex const & cTop2 = _surface.GetCornerTopology(2);
    fvOpposite = fvIndices + cTop2.GetFaceIndexOffset(cTop2.GetFaceAfter(2));
    P[10] = fvOpposite[0];
    P[11] = fvOpposite[1];
    P[15] = fvOpposite[2];
    P[14] = fvOpposite[3];
    fvIndices += cTop2.GetNumFaceVertices();

    FaceVertex const & cTop3 = _surface.GetCornerTopology(3);
    fvOpposite = fvIndices + cTop3.GetFaceIndexOffset(cTop3.GetFaceAfter(2));
    P[ 9] = fvOpposite[0];
    P[13] = fvOpposite[1];
    P[12] = fvOpposite[2];
    P[ 8] = fvOpposite[3];
}

void
RegularPatchBuilder::gatherBoundaryPatchPoints4(Index P[]) const {

    Index const * fvIndices = &_surface.GetIndices()[0];

    //
    //  For each of the 4 corners -- whether boundary or interior -- one
    //  incident face contains all indices that will contribute to the points
    //  of the corresponding patch.  Identify it first and then retrieve and
    //  assign the indices accordingly:
    //
    for (int i = 0; i < 4; ++i) {
        FaceVertex       const & cTop = _surface.GetCornerTopology(i);
        FaceVertexSubset const & cSub = _surface.GetCornerSubset(i);

        int faceCorner = cTop.GetFace();

        int faceOther = faceCorner;
        if (!cSub.IsBoundary()) {
            faceOther = cTop.GetFaceAfter(2);
        } else if (cSub._numFacesAfter) {
            faceOther = cTop.GetFaceNext(faceCorner);
        } else if (cSub._numFacesBefore) {
            faceOther = cTop.GetFacePrevious(faceCorner);
        }

        Index const * fvOther = fvIndices + cTop.GetFaceIndexOffset(faceOther);

        Index fvPhantom = fvOther[0];

        switch (i) {
        case 0:
            P[5] = fvOther[0];
            if (!cSub.IsBoundary()) {
                P[4] = fvOther[1];
                P[0] = fvOther[2];
                P[1] = fvOther[3];
            } else {
                P[4] = cSub._numFacesAfter  ? fvOther[3] : fvPhantom;
                P[0] = fvPhantom;
                P[1] = cSub._numFacesBefore ? fvOther[1] : fvPhantom;
            }
            break;
        case 1:
            P[6] = fvOther[0];
            if (!cSub.IsBoundary()) {
                P[2] = fvOther[1];
                P[3] = fvOther[2];
                P[7] = fvOther[3];
            } else {
                P[2] = cSub._numFacesAfter  ? fvOther[3] : fvPhantom;
                P[3] = fvPhantom;
                P[7] = cSub._numFacesBefore ? fvOther[1] : fvPhantom;
            }
            break;
        case 2:
            P[10] = fvOther[0];
            if (!cSub.IsBoundary()) {
                P[11] = fvOther[1];
                P[15] = fvOther[2];
                P[14] = fvOther[3];
            } else {
                P[11] = cSub._numFacesAfter  ? fvOther[3] : fvPhantom;
                P[15] = fvPhantom;
                P[14] = cSub._numFacesBefore ? fvOther[1] : fvPhantom;
            }
            break;
        case 3:
            P[ 9] = fvOther[0];
            if (!cSub.IsBoundary()) {
                P[13] = fvOther[1];
                P[12] = fvOther[2];
                P[ 8] = fvOther[3];
            } else {
                P[13] = cSub._numFacesAfter  ? fvOther[3] : fvPhantom;
                P[12] = fvPhantom;
                P[ 8] = cSub._numFacesBefore ? fvOther[1] : fvPhantom;
            }
            break;
        }
        fvIndices += cTop.GetNumFaceVertices();
    }
}

void
RegularPatchBuilder::gatherInteriorPatchPoints3(Index P[]) const {

    Index const * fvIndices = &_surface.GetIndices()[0];

    //
    //  For each of the 3 corners, the indices for the four contributing
    //  points come from the 2nd and 3rd faces following the corner face:
    //
    Index const * fvNext2 = 0;
    Index const * fvNext3 = 0;

    FaceVertex const & cTop0 = _surface.GetCornerTopology(0);
    fvNext2 = fvIndices + cTop0.GetFaceIndexOffset(cTop0.GetFaceAfter(2));
    fvNext3 = fvIndices + cTop0.GetFaceIndexOffset(cTop0.GetFaceAfter(3));
    P[ 4] = fvNext2[0];
    P[ 7] = fvNext2[1];
    P[ 3] = fvNext2[2];
    P[ 0] = fvNext3[2];
    fvIndices += cTop0.GetNumFaceVertices();

    FaceVertex const & cTop1 = _surface.GetCornerTopology(1);
    fvNext2 = fvIndices + cTop1.GetFaceIndexOffset(cTop1.GetFaceAfter(2));
    fvNext3 = fvIndices + cTop1.GetFaceIndexOffset(cTop1.GetFaceAfter(3));
    P[ 5] = fvNext2[0];
    P[ 1] = fvNext2[1];
    P[ 2] = fvNext2[2];
    P[ 6] = fvNext3[2];
    fvIndices += cTop1.GetNumFaceVertices();

    FaceVertex const & cTop2 = _surface.GetCornerTopology(2);
    fvNext2 = fvIndices + cTop2.GetFaceIndexOffset(cTop2.GetFaceAfter(2));
    fvNext3 = fvIndices + cTop2.GetFaceIndexOffset(cTop2.GetFaceAfter(3));
    P[ 8] = fvNext2[0];
    P[ 9] = fvNext2[1];
    P[11] = fvNext2[2];
    P[10] = fvNext3[2];
}

void
RegularPatchBuilder::gatherBoundaryPatchPoints3(Index P[]) const {

    Index const * fvIndices = &_surface.GetIndices()[0];

    //
    //  For each of the 3 corners, one incident face contains all indices
    //  that will contribute to the points of the corresponding patch, but
    //  interior vertices require two:
    //
    for (int i = 0; i < 3; ++i) {
        FaceVertex       const & cTop = _surface.GetCornerTopology(i);
        FaceVertexSubset const & cSub = _surface.GetCornerSubset(i);

        int faceCorner = cTop.GetFace();

        int faceOther = -1;
        if (!cSub.IsBoundary()) {
            faceOther = cTop.GetFaceAfter(2);
        } else if (cSub._numFacesTotal == 1) {
            faceOther = faceCorner;
        } else if (cSub._numFacesBefore == 0) {
            faceOther = cTop.GetFaceAfter(2);
        } else if (cSub._numFacesAfter == 0) {
            faceOther = cTop.GetFaceBefore(2);
        } else {
            faceOther = cTop.GetFaceNext(faceCorner);
        }
        assert(faceOther >= 0);

        Index const * fvOther = fvIndices + cTop.GetFaceIndexOffset(faceOther);

        Index fvPhantom = fvOther[0];

        switch (i) {
        case 0:
            P[4] = fvOther[0];
            if (!cSub.IsBoundary()) {
                P[7] = fvOther[1];
                P[3] = fvOther[2];
                fvOther = fvIndices +
                          cTop.GetFaceIndexOffset(cTop.GetFaceNext(faceOther));
                P[0] = fvOther[2];
            } else {
                P[7] = (cSub._numFacesAfter) ? fvOther[3 - cSub._numFacesAfter]
                                             : fvPhantom;
                P[3] = (cSub._numFacesAfter  == 2) ? fvOther[2] : fvPhantom;
                P[0] = (cSub._numFacesBefore == 2) ? fvOther[1] : fvPhantom;
            }
            break;
        case 1:
            P[5] = fvOther[0];
            if (!cSub.IsBoundary()) {
                P[1] = fvOther[1];
                P[2] = fvOther[2];
                fvOther = fvIndices +
                          cTop.GetFaceIndexOffset(cTop.GetFaceNext(faceOther));
                P[6] = fvOther[2];
            } else {
                P[1] = (cSub._numFacesAfter) ? fvOther[3 - cSub._numFacesAfter]
                                             : fvPhantom;
                P[2] = (cSub._numFacesAfter  == 2) ? fvOther[2] : fvPhantom;
                P[6] = (cSub._numFacesBefore == 2) ? fvOther[1] : fvPhantom;
            }
            break;
        case 2:
            P[8] = fvOther[0];
            if (!cSub.IsBoundary()) {
                P[ 9] = fvOther[1];
                P[11] = fvOther[2];
                fvOther = fvIndices +
                          cTop.GetFaceIndexOffset(cTop.GetFaceNext(faceOther));
                P[10] = fvOther[2];
            } else {
                P[ 9] = (cSub._numFacesAfter) ? fvOther[3 - cSub._numFacesAfter]
                                              : fvPhantom;
                P[11] = (cSub._numFacesAfter  == 2) ? fvOther[2] : fvPhantom;
                P[10] = (cSub._numFacesBefore == 2) ? fvOther[1] : fvPhantom;
            }
            break;
        }
        fvIndices += cTop.GetNumFaceVertices();
    }
}

int
RegularPatchBuilder::GatherControlVertexIndices(Index cvIndices[]) const {

    if (_isQuad) {
        if (_isBoundary) {
            gatherBoundaryPatchPoints4(cvIndices);
        } else {
            gatherInteriorPatchPoints4(cvIndices);
        }
    } else {
        if (_isBoundary) {
            gatherBoundaryPatchPoints3(cvIndices);
        } else {
            gatherInteriorPatchPoints3(cvIndices);
        }
    }
    return _patchSize;
}


//
//  Methods for debugging...
//
void
RegularPatchBuilder::print(Index const P[]) const {

    printf("RegularPatchBuilder:\n");

    if (_patchType == Far::PatchDescriptor::REGULAR) {
        printf("    patch type  = REGULAR (B-Spline, quad)\n");
    } else if (_patchType == Far::PatchDescriptor::LOOP) {
        printf("    patch type  = LOOP (Box-Spline, tri)\n");
    } else {
        assert("Unknown _patchType for RegularPatchBuilder" == 0);
    }

    printf("    patch size  = %d\n", _patchSize);
    printf("    is quad     = %d\n", _isQuad);
    printf("    is boundary = %d\n", _isBoundary);

    if (P) {
        const char * label  = "    patch points:";
        const char * indent = "                 ";
        if (_isQuad) {
            printf("%s %4d %4d %4d %4d\n", label,  P[12], P[13], P[14], P[15]);
            printf("%s %4d %4d %4d %4d\n", indent, P[ 8], P[ 9], P[10], P[11]);
            printf("%s %4d %4d %4d %4d\n", indent, P[ 4], P[ 5], P[ 6], P[ 7]);
            printf("%s %4d %4d %4d %4d\n", indent, P[ 0], P[ 1], P[ 2], P[ 3]);
        } else {
            printf("%s       %4d  %4d\n",     label,  P[10], P[11]);
            printf("%s    %4d  %4d  %4d\n",   indent, P[7], P[8], P[9]);
            printf("%s %4d  %4d  %4d  %4d\n", indent, P[3], P[4], P[5], P[6]);
            printf("%s    %4d  %4d  %4d\n",   indent, P[0], P[1], P[2]);
        }
    }
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
