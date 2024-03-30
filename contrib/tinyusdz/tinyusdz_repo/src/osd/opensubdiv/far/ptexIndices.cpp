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
#include "../far/ptexIndices.h"

#include "../far/error.h"
#include "../vtr/level.h"

#include <cassert>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {


PtexIndices::PtexIndices(TopologyRefiner const &refiner) {
    initializePtexIndices(refiner);
}

PtexIndices::~PtexIndices() {

}


void
PtexIndices::initializePtexIndices(TopologyRefiner const &refiner) {

    int regFaceSize = Sdc::SchemeTypeTraits::GetRegularFaceSize(
            refiner.GetSchemeType());

    Vtr::internal::Level const & coarseLevel = refiner.getLevel(0);

    int nfaces = coarseLevel.getNumFaces();
    _ptexIndices.resize(nfaces+1);
    int ptexID=0;
    for (int i = 0; i < nfaces; ++i) {
        _ptexIndices[i] = ptexID;
        Vtr::ConstIndexArray fverts = coarseLevel.getFaceVertices(i);
        ptexID += fverts.size()==regFaceSize ? 1 : fverts.size();
    }
    // last entry contains the number of ptex texture faces
    _ptexIndices[nfaces]=ptexID;
}

int
PtexIndices::GetNumFaces() const {
    return _ptexIndices.back();
}

int
PtexIndices::GetFaceId(Index f) const {
    assert(f<(int)_ptexIndices.size());
    return _ptexIndices[f];
}

namespace {
    // Returns the face adjacent to 'face' along edge 'edge'
    inline Index
    getAdjacentFace(Vtr::internal::Level const & level, Index edge, Index face) {
        Far::ConstIndexArray adjFaces = level.getEdgeFaces(edge);
        if (adjFaces.size()!=2) {
            return -1;
        }
        return (adjFaces[0]==face) ? adjFaces[1] : adjFaces[0];
    }
}

void
PtexIndices::GetAdjacency(
    TopologyRefiner const &refiner,
    int face, int quadrant,
    int adjFaces[4], int adjEdges[4]) const {

    int regFaceSize =
        Sdc::SchemeTypeTraits::GetRegularFaceSize(refiner.GetSchemeType());

    Vtr::internal::Level const & level = refiner.getLevel(0);

    ConstIndexArray fedges = level.getFaceEdges(face);

    if (fedges.size() == regFaceSize) {

        // Regular ptex quad face
        for (int i=0; i<regFaceSize; ++i) {
            int edge = fedges[i];
            Index adjface = getAdjacentFace(level, edge, face);
            if (adjface==-1) {
                adjFaces[i] = -1;  // boundary or non-manifold
                adjEdges[i] = 0;
            } else {

                ConstIndexArray aedges = level.getFaceEdges(adjface);
                if (aedges.size()==regFaceSize) {
                    adjFaces[i] = _ptexIndices[adjface];
                    adjEdges[i] = aedges.FindIndex(edge);
                    assert(adjEdges[i]!=-1);
                } else {
                    // neighbor is a sub-face
                    adjFaces[i] = _ptexIndices[adjface] +
                                  (aedges.FindIndex(edge)+1)%aedges.size();
                    adjEdges[i] = 3;
                }
                assert(adjFaces[i]!=-1);
            }
        }
        if (regFaceSize == 3) {
            adjFaces[3] = -1;
            adjEdges[3] = 0;
        }
    } else if (regFaceSize == 4) {

        //  Ptex sub-face 'quadrant' (non-quad)
        //
        // Ptex adjacency pattern for non-quads:
        //
        //             v2
        /*             o
        //            / \
        //           /   \
        //          /0   3\
        //         /       \
        //        o_ 1   2 _o
        //       /  -_   _-  \
        //      /  2  -o-  1  \
        //     /3      |      0\
        //    /       1|2       \
        //   /    0    |    3    \
        //  o----------o----------o
        // v0                     v1
        */
        assert(quadrant>=0 && quadrant<fedges.size());

        int nextQuadrant = (quadrant+1) % fedges.size(),
            prevQuadrant = (quadrant+fedges.size()-1) % fedges.size();

        {   // resolve neighbors within the sub-face (edges 1 & 2)
            adjFaces[1] = _ptexIndices[face] + nextQuadrant;
            adjEdges[1] = 2;

            adjFaces[2] = _ptexIndices[face] + prevQuadrant;
            adjEdges[2] = 1;
        }

        {   // resolve neighbor outside the sub-face (edge 0)
            int edge0 = fedges[quadrant];
            Index adjface0 = getAdjacentFace(level, edge0, face);
            if (adjface0==-1) {
                adjFaces[0] = -1;  // boundary or non-manifold
                adjEdges[0] = 0;
            } else {
                ConstIndexArray afedges = level.getFaceEdges(adjface0);
                if (afedges.size()==4) {
                   adjFaces[0] = _ptexIndices[adjface0];
                   adjEdges[0] = afedges.FindIndexIn4Tuple(edge0);
                } else {
                   int subedge = (afedges.FindIndex(edge0)+1)%afedges.size();
                   adjFaces[0] = _ptexIndices[adjface0] + subedge;
                   adjEdges[0] = 3;
                }
                assert(adjFaces[0]!=-1);
            }

            // resolve neighbor outside the sub-face (edge 3)
            int edge3 = fedges[prevQuadrant];
            Index adjface3 = getAdjacentFace(level, edge3, face);
            if (adjface3==-1) {
                adjFaces[3]=-1;  // boundary or non-manifold
                adjEdges[3]=0;
            } else {
                ConstIndexArray afedges = level.getFaceEdges(adjface3);
                if (afedges.size()==4) {
                   adjFaces[3] = _ptexIndices[adjface3];
                   adjEdges[3] = afedges.FindIndexIn4Tuple(edge3);
                } else {
                   int subedge = afedges.FindIndex(edge3);
                   adjFaces[3] = _ptexIndices[adjface3] + subedge;
                   adjEdges[3] = 0;
                }
                assert(adjFaces[3]!=-1);
            }
        }
    } else {
        Far::Error(FAR_RUNTIME_ERROR,
                "Failure in PtexIndices::GetAdjacency() -- "
                "irregular faces only supported for quad schemes.");
    }
}

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
