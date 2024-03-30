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

#include "../bfr/faceTopology.h"
#include "../sdc/crease.h"

#include <cstring>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {


//
//  Constructor needs the same Sdc scheme/options as the SurfaceFactory
//  to support internal work -- may need to figure another way to assign
//  these if we later need a default constructor for some purpose...
//
FaceTopology::FaceTopology(Sdc::SchemeType schemeType,
                           Sdc::Options schemeOptions) :
    _schemeType(schemeType),
    _schemeOptions(schemeOptions),
    _regFaceSize(Sdc::SchemeTypeTraits::GetRegularFaceSize(schemeType)),
    _isInitialized(false) {

}

//
//  Main initialize/finalize used by base factory to delimit assignment:
//
void
FaceTopology::Initialize(int faceSize) {

    _faceSize = faceSize;
    _numFaceVertsTotal = 0;

    _combinedTag.Clear();

    _isInitialized = true;
    _isFinalized   = false;

    _corner.SetSize(faceSize);
}

void
FaceTopology::Finalize() {

    //
    //  Inspect all corner vertex topologies -- accumulating the presence
    //  of irregular features for the face and assigning other internal
    //  members used to assemble the limit surface:
    //
    //  WIP - potentially want to identify presence of degenerate faces
    //  below too, i.e. face size < 3.  A subclass may specify these in
    //  an ordered set and that would mess up some of the topological
    //  traversals.  In such case, we can initialize the vertex subset
    //  to excludes such faces -- treating their edges as non-manifold.
    //
    //  Probably need to add yet another bit per vertex here to know when
    //  to process an otherwise simple manifold ring, i.e. hasDegenFaces
    //
    assert(_isInitialized);

    for (int i = 0; i < _faceSize; ++i) {
        FaceVertex & cTop  = GetTopology(i);

        _combinedTag.Combine(cTop.GetTag());

        _numFaceVertsTotal += cTop.GetNumFaceVertices();
    }

    _isFinalized = true;
}

void
FaceTopology::ResolveUnOrderedCorners(Index const fvIndices[]) {

    //
    //  Inspect and deal with any corner that did not have its incident
    //  faces specified in counter-clockwise order (and so which may be
    //  non-manifold).  The face-vertex indices are required for the
    //  corner to identify the connectivity between them for later use:
    //
    //  Be sure to reset the combined tags as resolution of ordering
    //  will also detect non-manifold (or manifold) features, e.g.
    //  sharpening or the presence of boundaries edges.
    //
    _combinedTag.Clear();
 
    for (int i = 0; i < _faceSize; ++i) {
        FaceVertex & cTop = GetTopology(i);

        if (cTop.GetTag().IsUnOrdered()) {
            cTop.ConnectUnOrderedFaces(fvIndices);
        }

        _combinedTag.Combine(cTop.GetTag());

        fvIndices += cTop.GetNumFaceVertices();
    }
}

void
FaceTopology::print(Index const faceVertIndices[]) const {

    MultiVertexTag const & tag = _combinedTag;

    printf("FaceTopology:\n");
    printf("    face size      = %d\n", _faceSize);
    printf("    num-face-verts = %d\n", _numFaceVertsTotal);
    printf("  Tags:\n");
    printf("    inf-sharp verts  = %d\n", tag.HasInfSharpVertices());
    printf("    semi-sharp verts = %d\n", tag.HasSemiSharpVertices());
    printf("    inf-sharp edges  = %d\n", tag.HasInfSharpEdges());
    printf("    semi-sharp edges = %d\n", tag.HasSemiSharpEdges());
    printf("    inf-sharp darts  = %d\n", tag.HasInfSharpDarts());
    printf("    unsharp boundary = %d\n", tag.HasNonSharpBoundary());
    printf("    irregular faces  = %d\n", tag.HasIrregularFaceSizes());
    printf("    unordered verts  = %d\n", tag.HasUnOrderedVertices());

    if (faceVertIndices) {
        Index const * cornerFaceVertIndices = faceVertIndices;

        for (int i = 0; i < _faceSize; ++i) {
            printf("    corner %d:\n", i);

            FaceVertex const & cTop = GetTopology(i);
            printf("        topology:  num faces  = %d, boundary = %d\n",
                    cTop.GetNumFaces(), cTop.GetTag().IsBoundary());

            printf("        face-vert indices:\n");

            for (int j = 0, n = 0; j < cTop.GetNumFaces(); ++j) {
                printf("        face %d:  ", j);
                int S = cTop.GetFaceSize(j);
                for (int k = 0; k < S; ++k, ++n) {
                    printf("%3d", cornerFaceVertIndices[n]);
                }
                printf("\n");
            }
            cornerFaceVertIndices += cTop.GetNumFaceVertices();
        }
    }
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
