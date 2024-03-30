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

#include "../bfr/tessellation.h"

#include <cstring>
#include <cstdio>
#include <cassert>
#include <limits>
#include <algorithm>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  Tessellation patterns are composed of concentric rings of points (or
//  "coords" for coordinates) and facets -- beginning with the boundary
//  and moving inward.  Each ring of can be further divided into subsets
//  corresponding elements associated with each edge.
//
//  While the nature of these rings is similar for the different types of
//  parameterizations, each is different enough to have warranted its own
//  implementation at a high level.  Lower level utilities for assembling
//  the rings from strips of coords and facets are common to all.
//
//  WIP - consider moving some of these implementation details to separate
//        internal header and source files
//
namespace {
    //
    //  Simple classes to provide array-like interfaces to the primitive
    //  data buffers passed by clients.  Both parametric coordinate pairs
    //  and the integer tuples (size 3 or 4) representing a single facet
    //  of a tessellation are represented here.
    //
    //  These array interfaces are "minimal" in the sense that they provide
    //  only what is needed here -- rather than trying to support a wider
    //  range of use where full generality is needed (e.g. the arithmetic
    //  operators are limited to those used here for advancing through and
    //  assigning elements of the array, so the full range required for
    //  arbitrary address arithmetic are not included).
    //
    //  Both arrays support a user-specified stride within which the tuple
    //  for the coord or facet is assigned.
    //

    //  Floating point pair and its array for points of a tessellation:
    template <typename REAL>
    class Coord2 {
    public:
        Coord2(REAL * data) : _uv(data) { }
        Coord2() : _uv(0) { }
        ~Coord2() { }

        void Set(REAL u, REAL v) { _uv[0] = u, _uv[1] = v; }

        REAL const & operator[](int index) const { return _uv[index]; }
        REAL       & operator[](int index)       { return _uv[index]; }

    private:
        REAL * _uv;
    };

    template <typename REAL>
    class Coord2Array {
    public:
        Coord2Array(REAL * data, int stride) : _data(data), _stride(stride) { }
        Coord2Array() : _data(0), _stride(2) { }
        ~Coord2Array() { }

        Coord2Array operator+(int offset) {
            return Coord2Array(_data + offset*_stride, _stride);
        }

        Coord2<REAL> operator[](int index) {
            return Coord2<REAL>(_data + index*_stride);
        }

    private:
        REAL * _data;
        int    _stride;
    };

    //  Integer 3- or 4-tuple and its array for facets of a tessellation:
    class Facet {
    public:
        Facet(int * indices, int n) : _T(indices), _size(n) { }
        Facet() : _T(0), _size(4) { }
        ~Facet() { }

        void Set(int a, int b, int c) {
            //  Assign size-1 to ensure last index of 4-tuple is set
            _T[_size - 1] = -1;
            _T[0] = a, _T[1] = b, _T[2] = c;
        }
        void Set(int a, int b, int c, int d) {
            _T[0] = a, _T[1] = b, _T[2] = c, _T[3] = d;
        }

        int const & operator[](int index) const { return _T[index]; }
        int       & operator[](int index)       { return _T[index]; }

    private:
        int * _T;
        int   _size;
    };

    class FacetArray {
    public:
        FacetArray(int * data, int size, int stride) :
                _data(data), _size(size), _stride(stride) { }
        ~FacetArray() { }

        FacetArray operator+(int offset) {
            return FacetArray(_data + offset*_stride, _size, _stride);
        }

        Facet operator[](int index) {
            return Facet(_data + index*_stride, _size);
        }

    private:
        FacetArray() : _data(0), _size(4), _stride(4) { }

        int * _data;
        int   _size;
        int   _stride;
    };

    //
    //  Functions for assembling simple, common sets of coordinate pairs:
    //
    template <typename REAL>
    inline int
    appendUIsoLine(Coord2Array<REAL> P, int n, REAL u, REAL v, REAL dv) {

        for (int i = 0; i < n; ++i, v += dv) {
            P[i].Set(u,v);
        }
        return n;
    }

    template <typename REAL>
    inline int
    appendVIsoLine(Coord2Array<REAL> P, int n, REAL u, REAL v, REAL du) {

        for (int i = 0; i < n; ++i, u += du) {
            P[i].Set(u,v);
        }
        return n;
    }

    template <typename REAL>
    inline int
    appendUVLine(Coord2Array<REAL> P, int n, REAL u, REAL v, REAL du, REAL dv) {

        for (int i = 0; i < n; ++i, u += du, v += dv) {
            P[i].Set(u,v);
        }
        return n;
    }

    //
    //  Functions for assembling simple, common sets of facets:
    //
    inline int
    appendTri(FacetArray facets, int t0, int t1, int t2) {

        facets[0].Set(t0, t1, t2);
        return 1;
    }

    inline int
    appendQuad(FacetArray facets, int q0, int q1, int q2, int q3,
                  int triangulationSign) {

        if (triangulationSign == 0) {
            // no triangulation
            facets[0].Set(q0, q1, q2, q3);
            return 1;
        } else if (triangulationSign > 0) {
            // triangulate along diagonal in direction of leading edge
            facets[0].Set(q0, q1, q2);
            facets[1].Set(q2, q3, q0);
            return 2;
        } else {
            // triangulate along diagonal opposing the leading edge
            facets[0].Set(q2, q3, q1);
            facets[1].Set(q0, q1, q3);
            return 2;
        }
    }

    inline int
    appendTriFan(FacetArray facets, int size, int startIndex) {

        for (int i = 1; i <= size; ++i) {
            facets[i-1].Set(startIndex + (i - 1),
                            startIndex + ((i < size) ? i : 0),
                            startIndex + size);
        }
        return size;
    }

    //
    //  Useful struct for storing bounding indices and other topology of
    //  a strip of facets so points can be connected in various ways:
    //
    //  A strip of facets is defined between an outer and inner ring of
    //  points -- denoted as follows, where the "i" and "o" prefixes are
    //  used to designate points on the inner and outer rings:
    //
    //    oPrev  ---  iFirst  ... iFirst+/-i ...  iLast    --- oLast+1
    //      |                                                    |
    //    oFirst --- oFirst+1 ...  oFirst+j ... oFirst+N-1 --- oLast
    //
    //  Since these points form part of a ring, they will wrap around to
    //  the beginning of the ring for the last edge and so the sequence
    //  is not always sequential.  Transitions to the "first" and "last"
    //  of both the outer and inner rings are potentially discontinuous,
    //  which is why they are provided as separate members.
    //
    //  This topological structure is similar but slightly different for
    //  quad-based versus triangular parameterizations.  For quad-based
    //  parameterizations the parametric range of the inner and outer
    //  sequences are the same, but for triangular, the extent of the
    //  inner ring is one edge less (and the triangular domain may be
    //  offset a half edge length so that uniformly spaced points on
    //  both will alternate).
    //
    struct FacetStrip {
    public:
        FacetStrip() { std::memset((void*) this, 0, sizeof(*this)); }

        int connectUniformQuads(    FacetArray facets) const;
        int connectUniformTris(     FacetArray facets) const;
        int connectNonUniformFacets(FacetArray facets) const;

    public:
        //  Members defining how the strip should be used:
        unsigned int quadTopology    : 1;
        unsigned int quadTriangulate : 1;
        unsigned int innerReversed   : 1;

        unsigned int excludeFirstFace : 1;
        unsigned int splitFirstFace   : 1;
        unsigned int splitLastFace    : 1;
        unsigned int includeLastFace  : 1;

        //  Members defining the dimensions of the strip -- the number
        //  of "inner edges" potentially excludes the two edges that
        //  connect the inner ring to the outer:
        int outerEdges;
        int innerEdges;

        //  Members containing indices for points noted above.  Since
        //  a strip may wrap around the concentric rings of points,
        //  pairs of points that may appear to have successive indices
        //  will not -- which is why these are assigned externally:
        int outerFirst, outerLast, outerPrev;
        int innerFirst, innerLast;
    };

    int
    FacetStrip::connectUniformQuads(FacetArray facets) const {

        assert(quadTopology);
        assert(innerEdges == (outerEdges - 2));
        //
        //  For connecting quads, the pattern is simplified as follows:
        //
        //      oPrev ---- iFirst  ...   iLast ---- oLast+1
        //        | 3      2 | 3         2 | 3       2 |
        //        | 0      1 | 0         1 | 0       1 |
        //      oFirst -- oFirst+1 ... oFirst+N-1 -- oLast
        //
        //  with the first and last quads not sharing any inner edges
        //  (between inner-first and inner-last) and potentially being
        //  split to include the triangle on the outer edge.
        //
        //  It is typical for the first quad to always be included and
        //  for the last to be excluded -- the last quad usually being
        //  included by the next strip in the ring (unless split).
        //
        int nFacets = 0;

        //  Split or assign the first quad (precedes inner edges):
        int out0 = outerFirst;
        int in0  = innerFirst;

        if (splitFirstFace) {
            nFacets += appendTri(facets + nFacets, out0, out0 + 1, in0);
        } else if (!excludeFirstFace) {
            nFacets += appendQuad(facets + nFacets,
                             out0, out0 + 1, in0, outerPrev, quadTriangulate);
        }

        //  Assign quads sharing the inner edges (last is a special case):
        int outI = outerFirst + 1;
        int inI  = innerFirst;

        if (innerEdges) {
            int triSign = quadTriangulate;
            int inDelta = innerReversed ? -1 : 1;

            for (int i = 1; i <= innerEdges; ++i, ++outI, inI += inDelta) {
                if (i > (innerEdges / 2)) triSign = - (int)quadTriangulate;

                int outJ = outI + 1;
                int inJ  = (i < innerEdges) ? (inI + inDelta) : innerLast;

                nFacets += appendQuad(facets + nFacets,
                                 outI, outJ, inJ, inI, triSign);
            }
        }

        //  Split or assign the last quad (follows inner edges):
        int outN = outerLast;
        int inN  = innerLast;

        if (splitLastFace) {
            nFacets += appendTri(facets + nFacets, outI, outN, inN);
        } else if (includeLastFace) {
            nFacets += appendQuad(facets + nFacets,
                             outI, outN, outN+1, inN, - (int)quadTriangulate);
        }
        return nFacets;
    }

    int
    FacetStrip::connectUniformTris(FacetArray facets) const {

        assert(!quadTopology);
        assert(!excludeFirstFace);
        assert(!includeLastFace);
        assert(!innerReversed);
        //
        //  Assign the set of tris for the "sawtooth" strip with N outer
        //  edges and N-3 inner edges of the inner ring:
        //
        //               1       3              2M-1
        //       oPrev --- iFirst -- i1  ...  ii --- iLast -- oLast+1
        //          / 2\1  0/  \    /  \       \1  0/ 2\    /  \.
        //         /0  1\2 /    \  /    \       \2 /0  1\  /    \.
        //    oFirst --- o1 ---- o2  ..  oi  ... oM --- oN-1 --- oLast
        //           0       2       4              2M
        //
        //  The first and last pair of tris may optionally be split by
        //  connecting the "first" or "last" points between the two rows
        //  (i.e. [oFirst, oFirst+1, iFirst]) which bisects the two
        //  triangles normally included.
        //
        //  Following the first pair (or single tri if split), a single
        //  leading triangle ([o1, o2, iFirst] above) is then assigned,
        //  followed by pairs of adjacent tris below each inner edge:
        //  the first of the pair based on the inner edge, the second on
        //  the outer edge.
        //
        int nFacets = 0;

        //  Split or assign the first pair of tris (precedes inner edges):
        int out0 = outerFirst;
        int in0  = innerFirst;

        if (splitFirstFace) {
            nFacets += appendTri(facets + nFacets, out0, out0+1, in0);
        } else {
            nFacets += appendTri(facets + nFacets, out0, out0+1,outerPrev);
            nFacets += appendTri(facets + nFacets, in0, outerPrev, out0+1);
        }

        //  Assign the next tri -- preceding the pairs for the inner edges:
        nFacets += appendTri(facets + nFacets, out0 + 1, out0 + 2, in0);

        //  Assign pair of tris below each inner edge (last is special):
        int outI = outerFirst + 2;
        int inI  = innerFirst;

        if (innerEdges) {
            for (int i = 1; i <= innerEdges; ++i, ++inI, ++outI) {
                int outJ = outI + 1;
                int inJ  = (i < innerEdges) ? (inI  + 1) : innerLast;

                nFacets += appendTri(facets + nFacets, inJ, inI, outI);
                nFacets += appendTri(facets + nFacets, outI, outJ, inJ);
            }
        }

        //  Split the last pair of tris (follows  inner edges):
        int outN = outerLast;
        int inN  = innerLast;

        if (splitLastFace) {
            nFacets += appendTri(facets + nFacets, outI, outN, inN);
        }
        return nFacets;
    }

    int
    FacetStrip::connectNonUniformFacets(FacetArray facets) const {

        //
        //  General case:
        //
        //   oPrev -- iFirst  .  ...  i0+/-i  ...   .   iLast --*
        //        |   /       .                     .        \  |
        //        | /         |                     |         \ |
        //   oFirst -------- o0  ...   o0+i   ...  oN-1 ------ oLast
        //
        //  The sequence of edges -- both inner and outer -- is parameterized
        //  over the integer range [0 .. M*N] where M and N are the resolution
        //  (number of edges) of the inner and outer rings respectively.
        //
        //  Note that the current implementation expects the faces at the
        //  ends to be "split", i.e. a diagonal edge created between the
        //  first/last points of the inner and outer rings at both ends.
        //  It is possible that this will later be relaxed (allowing an
        //  unsplit quad at the corner to be generated), as is currently
        //  the case with uniform strips.  In the meantime, the caller is
        //  expected to explicitly request split corners to make it clear
        //  where they need to adapt later.
        //
        assert(splitFirstFace && splitLastFace);

        int M = innerEdges + (quadTopology ? 2 : 3);
        int N = outerEdges;

        int dtOuter = M;
        int dtInner = N;

        int dtMin = std::min(dtInner, dtOuter);
        int dtMax = std::max(dtInner, dtOuter);

        //  Use larger slope when M ~= N to accomodate tri insertion:
        int dtSlopeMax = ((dtMax / 2) < dtMin) ? (dtMin - 1) : (dtMax / 2);

        int tOuterLast   = dtOuter *  N;
        int tOuterMiddle = tOuterLast / 2;

        int tInnerOffset = 0;
        int tInnerLast   = dtInner * (M - 1);

        //  If tris, adjust parametric range for the inner edges:
        if (!quadTopology) {
            tInnerOffset = dtInner / 2;
            tInnerLast  += tInnerOffset - dtInner;
        }

        int dInner = innerReversed ? -1 : 1;

        //
        //  Two points are successively identified on each of the inner and
        //  outer sequence of edges, from which facets will be generated:
        //
        //           inner0  inner1
        //              * ----- * . . .
        //             /
        //            /
        //           * ----------- * . . .
        //        outer0        outer1
        //
        //  Identify the parameterization and coordinate indices for the
        //  points starting the sequence:
        //
        int tOuter0 = 0;
        int cOuter0 = outerFirst;

        int tOuter1 = dtOuter;
        int cOuter1 = (N == 1) ? outerLast : (outerFirst + 1);

        int tInner0 = tInnerOffset + dtInner;
        int cInner0 = innerFirst;

        int tInner1 = tInner0 + (innerEdges ? dtInner : 0);
        int cInner1 = (innerEdges == 1) ? innerLast : (innerFirst + dInner);

        //
        //  Walk forward through the strip, identifying each successive quad
        //  and choosing the most "vertical" edge to use to triangulate it:
        //
        bool keepQuads = quadTopology && !quadTriangulate;

        int nFacetsExpected = 0;
        if (keepQuads) {
            nFacetsExpected = std::max(innerEdges, outerEdges);
            //  Include a symmetric center triangle if any side is odd:
            if ((nFacetsExpected & 1) == 0) {
                nFacetsExpected += (innerEdges & 1) || (outerEdges & 1);
            }
        } else {
            nFacetsExpected = innerEdges + outerEdges;
        }

        //  These help maintain symmetry where possible:
        int  nFacetsLeading = nFacetsExpected / 2;
        int  nFacetsMiddle  = nFacetsExpected & 1;

        int  middleFacet = nFacetsMiddle ? nFacetsLeading : -1;
        bool middleQuad  = keepQuads && (outerEdges & 1) && (innerEdges & 1);

        //
        //  Assign all expected facets sequentially -- advancing references
        //  to the inner and outer edges according to what is used for each:
        //
        for (int facetIndex = 0; facetIndex < nFacetsExpected; ++facetIndex) {
            bool generateTriOuter = false;
            bool generateTriInner = false;
            bool generateQuad     = false;

            //
            //  Detect simple cases first:  the symmetric center face or 
            //  triangles in the absence of an inner or outer edge:
            //
            if (facetIndex == middleFacet) {
                if (middleQuad) {
                    generateQuad = true;
                } else if (outerEdges & 1) {
                    generateTriOuter = true;
                } else {
                    generateTriInner = true;
                }
            } else if (tInner1 == tInner0) {
                generateTriOuter = true;
            } else if (tOuter1 == tOuter0) {
                generateTriInner = true;
            } else {
                //
                //  For the general case, assign a quad if specified and
                //  possible.  Otherwise continue with a triangle.  Both
                //  situations avoid poor aspect and preserve symmetry:
                //
                if (keepQuads) {
                    //  If face is after the midpoint, use the same kind of
                    //  face as its mirrored counterpart. Otherwise, reject a
                    //  quad trying to cross the midpoint.  Finally, test the
                    //  slope of the "vertical" edge of the potential quad:
                    if (facetIndex >= nFacetsLeading) {
                        int mirroredFacetIndex = nFacetsLeading - 1 -
                                (facetIndex - nFacetsLeading - nFacetsMiddle);

                        generateQuad = (facets[mirroredFacetIndex][3] >= 0);
                    } else if ((tInner1 > tOuterMiddle) ||
                               (tOuter1 > tOuterMiddle)) {
                        generateQuad = false;
                    } else {
                        int dtSlope1 = std::abs(tOuter1 - tInner1);

                        generateQuad = (dtSlope1 <= dtSlopeMax);
                    }
                }

                if (!generateQuad) {
                    //  Can't detect symmetric triangles as inner or outer as
                    //  easily as quads, but the test is relatively simple --
                    //  choose the diagonal spanning the shortest interval
                    //  (when equal, choose relative to midpoint for symmetry):
                    int dtDiagToOuter1 = tOuter1 - tInner0;
                    int dtDiagToInner1 = tInner1 - tOuter0;

                    bool useOuterEdge = (dtDiagToOuter1 == dtDiagToInner1)
                                      ? (tOuter1 > tOuterMiddle)
                                      : (dtDiagToOuter1 < dtDiagToInner1);
                    if (useOuterEdge) {
                        generateTriOuter = true;
                    } else {
                        generateTriInner = true;
                    }
                }
            }

            //  Assign the face as determined above:
            if (generateTriOuter) {
                facets[facetIndex].Set(cOuter0, cOuter1, cInner0);
            } else if (generateTriInner) {
                facets[facetIndex].Set(cInner1, cInner0, cOuter0);
            } else {
                facets[facetIndex].Set(cOuter0, cOuter1, cInner1, cInner0);
            }

            //  Advance to the next point of the next outer edge:
            bool advanceOuter = generateTriOuter || generateQuad;
            if (advanceOuter) {
                tOuter0 = tOuter1;
                cOuter0 = cOuter1;

                tOuter1 += dtOuter;
                cOuter1  = cOuter1 + 1;
                if (tOuter1 >= tOuterLast) {
                    tOuter1 = tOuterLast;
                    cOuter1 = outerLast;
                }
            }

            //  Advance to the next point of the next inner edge:
            bool advanceInner = generateTriInner || generateQuad;
            if (advanceInner) {
                tInner0 = tInner1;
                cInner0 = cInner1;

                tInner1 += dtInner;
                cInner1  = cInner1 + dInner;
                if (tInner1 >= tInnerLast) {
                    tInner1  = tInnerLast;
                    cInner1  = innerLast;
                }
            }
        }
        return nFacetsExpected;
    }
}


//
//  Utility functions to help assembly of tessellation patterns -- grouped
//  into local structs/namespaces for each of the supported parameterization
//  types:  quad, triangle (tri) or quadrangulated sub-faces (qsub):
//
//  Given the similar structure to these -- the construction of patterns
//  using concentric rings of Coords, rings of Facets between successive
//  concentric rings, etc. -- there are some opportunities for refactoring
//  some of these.  (But there are typically subtle differences between
//  each that complicate doing so.)
//
class quad {
public:
    //  Public methods for counting coords and facets:
    static int CountUniformFacets(int edgeRes, bool triangulate);
    static int CountSegmentedFacets(int const uvRes[], bool triangulate);
    static int CountNonUniformFacets(int const outerRes[], int const uvRes[],
                                     bool triangulate);

    static int CountInteriorCoords(int edgeRes);
    static int CountInteriorCoords(int const uvRes[]);

    //  Public methods for identifying and assigning coordinate pairs:
    template <typename REAL>
    static int GetEdgeCoords(int edge, int edgeRes,
                             Coord2Array<REAL> coords);
    template <typename REAL>
    static int GetBoundaryCoords(int const edgeRates[],
                                 Coord2Array<REAL> coords);
    template <typename REAL>
    static int GetInteriorCoords(int const uvRes[2],
                                 Coord2Array<REAL> coords);

    //  Public methods for identifying and assigning facets:
    static int GetUniformFacets(int edgeRes, bool triangulate,
                                FacetArray facets);
    static int GetSegmentedFacets(int const uvRes[], bool triangulate,
                                  FacetArray facets);
    static int GetNonUniformFacets(int const outerRes[], int const innerRes[],
                                   int nBoundaryEdges, bool triangulate,
                                   FacetArray facets);
private:
    //  Private methods used by those above:
    static int countUniformCoords(int edgeRes);
    static int countNonUniformEdgeFacets(int outerRes, int innerRes);

    template <typename REAL>
    static int getCenterCoord(Coord2Array<REAL> coords);
    template <typename REAL>
    static int getInteriorRingCoords(int  uRes,   int  vRes,
                                     REAL uStart, REAL vStart,
                                     REAL uDelta, REAL vDelta,
                                     Coord2Array<REAL> coords);

    static int getInteriorRingFacets(int uRes, int vRes,
                                     int indexOfFirstCoord, bool triangulate,
                                     FacetArray facets);
    static int getBoundaryRingFacets(int const outerRes[], int uRes, int vRes,
                                     int nBoundaryEdges, bool triangulate,
                                     FacetArray facets);
    static int getSingleStripFacets(int uRes, int vRes,
                                    int indexOfFirstCoord, bool triangulate,
                                    FacetArray facets);
};

class tri {
public:
    //  Public methods for counting coords and facets:
    static int CountUniformFacets(int edgeRes);
    static int CountNonUniformFacets(int const outerRes[], int innerRes);

    static int CountInteriorCoords(int edgeRes);

    //  Public methods for identifying and assigning coordinate pairs:
    template <typename REAL>
    static int GetEdgeCoords(int edge, int edgeRes,
                             Coord2Array<REAL> coords);
    template <typename REAL>
    static int GetBoundaryCoords(int const edgeRates[],
                                 Coord2Array<REAL> coords);
    template <typename REAL>
    static int GetInteriorCoords(int edgeRes,
                                 Coord2Array<REAL> coords);

    //  Public methods for identifying and assigning facets:
    static int GetUniformFacets(int edgeRes,
                                FacetArray facets);
    static int GetNonUniformFacets(int const outerRes[], int innerRes,
                                   int nBoundaryEdges,
                                   FacetArray facets);
private:
    //  Private methods used by those above:
    static int countUniformCoords(int edgeRes);

    template <typename REAL>
    static int getCenterCoord(Coord2Array<REAL> coords);
    template <typename REAL>
    static int getInteriorRingCoords(int  edgeRes,
                                     REAL uStart, REAL vStart, REAL tDelta,
                                     Coord2Array<REAL> coords);

    static int getInteriorRingFacets(int edgeRes, int indexOfFirstCoord,
                                     FacetArray facets);
    static int getBoundaryRingFacets(int const outerRes[], int innerRes,
                                     int nBoundaryEdges,
                                     FacetArray facets);
};

class qsub {
public:
    //  Public methods for counting coords and facets:
    static int CountUniformFacets(int N, int edgeRes, bool triangulate);
    static int CountNonUniformFacets(int N, int const outerRes[], int innerRes,
                                     bool triangulate);

    static int CountInteriorCoords(int N, int edgeRes);

    //  Public methods for identifying and assigning coordinate pairs (note
    //  these require a Parameterization while others only need face size N)
    template <typename REAL>
    static int GetEdgeCoords(Parameterization P, int edge, int edgeRes,
                             Coord2Array<REAL> coords);
    template <typename REAL>
    static int GetBoundaryCoords(Parameterization P, int const edgeRates[],
                                 Coord2Array<REAL> coords);
    template <typename REAL>
    static int GetInteriorCoords(Parameterization P, int edgeRes,
                                 Coord2Array<REAL> coords);

    //  Public methods for identifying and assigning facets:
    static int GetUniformFacets(int N, int edgeRes, bool triangulate,
                                FacetArray facets);
    static int GetNonUniformFacets(int N, int const outerRes[], int innerRes,
                                   int nBoundaryEdges, bool triangulate,
                                   FacetArray facets);
private:
    //  Private methods used by those above:
    static int countUniformCoords(int N, int edgeRes);

    template <typename REAL>
    static int getCenterCoord(Coord2Array<REAL> coords);
    template <typename REAL>
    static int getCenterRingCoords(Parameterization P, REAL tStart,
                                   Coord2Array<REAL> coords);
    template <typename REAL>
    static int getRingEdgeCoords(Parameterization P, int edge, int edgeRes,
                                 bool incFirst, bool incLast,
                                 REAL tStart, REAL tDelta,
                                 Coord2Array<REAL> coords);
    template <typename REAL>
    static int getInteriorRingCoords(Parameterization P, int edgeRes,
                                     REAL tStart, REAL tDelta,
                                     Coord2Array<REAL> coords);

    static int getCenterFacets(int N, int indexOfFirstCoord,
                               FacetArray facets);
    static int getInteriorRingFacets(int N, int edgeRes,
                                     int indexOfFirstCoord, bool triangulate,
                                     FacetArray facets);
    static int getBoundaryRingFacets(int N, int const outerRes[], int innerRes,
                                     int nBoundaryEdges, bool triangulate,
                                     FacetArray facets);
};

//
//  Implementations for quad functions:
//
inline int
quad::CountUniformFacets(int edgeRes, bool triangulate) {
    return (edgeRes * edgeRes) << (int) triangulate;
}

inline int
quad::CountSegmentedFacets(int const uvRes[], bool triangulate) {

    //  WIP - may extend later to handle different opposing outer rates
    assert((uvRes[0] == 1) || (uvRes[1] == 1));
    return (uvRes[0] * uvRes[1]) << (int) triangulate;
}

int
quad::countNonUniformEdgeFacets(int outerRes, int innerRes) {

    int nFacets = std::max(outerRes, (innerRes - 2));

    //  If the lesser is odd, a triangle will be added in the middle:
    if ((nFacets & 1) == 0) {
        nFacets += (outerRes & 1) || (innerRes & 1);
    }
    return nFacets;
}

int
quad::CountNonUniformFacets(int const outerRes[], int const innerRes[],
                            bool triangulate) {

    int uRes = innerRes[0];
    int vRes = innerRes[1];
    assert((uRes > 1) && (vRes > 1));

    //  Count interior facets based on edges of inner ring:
    int innerUEdges = uRes - 2;
    int innerVEdges = vRes - 2;

    int nInterior = innerUEdges * innerVEdges;

    //  If triangulating, things are much simpler:
    if (triangulate) {
        int nFacets = nInterior * 2;

        nFacets += innerUEdges + outerRes[0];
        nFacets += innerVEdges + outerRes[1];
        nFacets += innerUEdges + outerRes[2];
        nFacets += innerVEdges + outerRes[3];
        return nFacets;
    }

    //
    //  Accumulate boundary facets for each edge based on uniformity...
    //
    //  A uniform edge contributes a quad for each inner edge, plus one
    //  facet for the leading corner (quad if uniform, tri if not) and a
    //  tri for the trailing corner if it is not uniform.  A non-uniform
    //  edge contributes quads and tris based on the larger of the inner
    //  and outer resolutions.
    //
    bool uniformEdges[4];
    uniformEdges[0] = (outerRes[0] == uRes);
    uniformEdges[1] = (outerRes[1] == vRes);
    uniformEdges[2] = (outerRes[2] == uRes);
    uniformEdges[3] = (outerRes[3] == vRes);

    bool uniformCorners[4];
    uniformCorners[0] = (uniformEdges[0] && uniformEdges[3]);
    uniformCorners[1] = (uniformEdges[1] && uniformEdges[0]);
    uniformCorners[2] = (uniformEdges[2] && uniformEdges[1]);
    uniformCorners[3] = (uniformEdges[3] && uniformEdges[2]);

    int nBoundary = 0;
    nBoundary += uniformEdges[0] ? (innerUEdges + 1 + !uniformCorners[1]) :
                 countNonUniformEdgeFacets(outerRes[0], uRes);
    nBoundary += uniformEdges[1] ? (innerVEdges + 1 + !uniformCorners[2]) :
                 countNonUniformEdgeFacets(outerRes[1], vRes);
    nBoundary += uniformEdges[2] ? (innerUEdges + 1 + !uniformCorners[3]) :
                 countNonUniformEdgeFacets(outerRes[2], uRes);
    nBoundary += uniformEdges[3] ? (innerVEdges + 1 + !uniformCorners[0]) :
                 countNonUniformEdgeFacets(outerRes[3], vRes);
    return nInterior + nBoundary;
}

inline int
quad::countUniformCoords(int edgeRes) {
    return (edgeRes + 1) * (edgeRes + 1);
}

inline int
quad::CountInteriorCoords(int edgeRes) {
    return (edgeRes - 1) * (edgeRes - 1);
}

inline int
quad::CountInteriorCoords(int const uvRes[]) {
    return (uvRes[0] - 1) * (uvRes[1] - 1);
}

template <typename REAL>
inline int
quad::getCenterCoord(Coord2Array<REAL> coords) {

    coords[0].Set(0.5f, 0.5f);
    return 1;
}

template <typename REAL>
int
quad::GetEdgeCoords(int edge, int edgeRes, Coord2Array<REAL> coords) {

    REAL dt = 1.0f / (REAL)edgeRes;

    REAL t0 = dt;
    REAL t1 = 1.0f - dt;

    int n = edgeRes - 1;

    switch (edge) {
    case 0:  return appendVIsoLine<REAL>(coords, n, t0,   0.0f,  dt);
    case 1:  return appendUIsoLine<REAL>(coords, n, 1.0f, t0,    dt);
    case 2:  return appendVIsoLine<REAL>(coords, n, t1,   1.0f, -dt);
    case 3:  return appendUIsoLine<REAL>(coords, n, 0.0f, t1,   -dt);
    }
    return 0;
}

template <typename REAL>
int
quad::GetBoundaryCoords(int const edgeRates[], Coord2Array<REAL> coords) {

    int nCoords = 0;
    nCoords += appendVIsoLine<REAL>(coords + nCoords, edgeRates[0], 0.0f, 0.0f,
                                        1.0f / (REAL) edgeRates[0]);
    nCoords += appendUIsoLine<REAL>(coords + nCoords, edgeRates[1], 1.0f, 0.0f,
                                        1.0f / (REAL) edgeRates[1]);
    nCoords += appendVIsoLine<REAL>(coords + nCoords, edgeRates[2], 1.0f, 1.0f,
                                       -1.0f / (REAL) edgeRates[2]);
    nCoords += appendUIsoLine<REAL>(coords + nCoords, edgeRates[3], 0.0f, 1.0f,
                                       -1.0f / (REAL) edgeRates[3]);
    return nCoords;
}

template <typename REAL>
int
quad::getInteriorRingCoords(int uRes, int vRes,
                            REAL u0, REAL v0, REAL du, REAL dv,
                            Coord2Array<REAL> coords) {

    int nCoords = 0;
    if ((uRes > 0) && (vRes > 0)) {
        REAL u1 = 1.0f - u0;
        REAL v1 = 1.0f - v0;

        nCoords += appendVIsoLine(coords + nCoords, uRes, u0, v0, du);
        nCoords += appendUIsoLine(coords + nCoords, vRes, u1, v0, dv);
        nCoords += appendVIsoLine(coords + nCoords, uRes, u1, v1,-du);
        nCoords += appendUIsoLine(coords + nCoords, vRes, u0, v1,-dv);
    } else if (uRes > 0) {
        nCoords += appendVIsoLine(coords, uRes+1, u0, v0, du);
    } else if (vRes > 0) {
        nCoords += appendUIsoLine(coords, vRes+1, u0, v0, dv);
    } else {
        return getCenterCoord(coords);
    }
    return nCoords;
}

template <typename REAL>
int
quad::GetInteriorCoords(int const uvRes[2], Coord2Array<REAL> coords) {

    int nIntRings = std::min((uvRes[0] / 2), (uvRes[1] / 2));
    if (nIntRings == 0) return 0;

    REAL du = 1.0f / (REAL) uvRes[0];
    REAL dv = 1.0f / (REAL) uvRes[1];
    REAL u  = du;
    REAL v  = dv;

    int uRes = uvRes[0] - 2;
    int vRes = uvRes[1] - 2;

    //
    //  Note that with separate U and V res, one can go negative so beware
    //  of making any assumptions -- defer to the function for the ring:
    //
    int nCoords = 0;
    for (int i=0; i < nIntRings; ++i, uRes -= 2, vRes -= 2, u += du, v += dv) {
        nCoords += getInteriorRingCoords(uRes, vRes, u, v, du, dv,
                                         coords + nCoords);
    }
    return nCoords;
}

int
quad::getSingleStripFacets(int uRes, int vRes, int coord0, bool triangulate,
                           FacetArray facets) {

    assert((uRes == 1) || (vRes == 1));

    FacetStrip qStrip;
    qStrip.quadTopology    = true;
    qStrip.quadTriangulate = triangulate;
    qStrip.splitFirstFace  = false;
    qStrip.splitLastFace   = false;
    qStrip.innerReversed   = true;
    qStrip.includeLastFace = true;

    if (uRes > 1) {
        qStrip.outerEdges = uRes;
        qStrip.innerEdges = uRes - 2;

        //  Assign these successively around the strip:
        qStrip.outerFirst = coord0;
        qStrip.outerLast  = qStrip.outerFirst + uRes;
        qStrip.innerLast  = qStrip.outerLast  + 2;
        qStrip.innerFirst = qStrip.outerLast  + uRes;
        qStrip.outerPrev  = qStrip.innerFirst + 1;

        return qStrip.connectUniformQuads(facets);
    } else {
        qStrip.outerEdges = vRes;
        qStrip.innerEdges = vRes - 2;

        qStrip.outerPrev  = coord0;
        qStrip.outerFirst = coord0 + 1;
        qStrip.outerLast  = qStrip.outerFirst + vRes;
        qStrip.innerLast  = qStrip.outerLast  + 2;
        qStrip.innerFirst = qStrip.outerLast  + vRes;

        return qStrip.connectUniformQuads(facets);
    }
}

int
quad::getInteriorRingFacets(int uRes, int vRes, int coord0, bool triangulate,
                            FacetArray facets) {

    assert((uRes >= 0) && (vRes >= 0));

    //
    //  Deal with some simple and special cases first:
    //
    int totalInnerFacets = uRes * vRes;
    if (totalInnerFacets == 0) return 0;

    if (totalInnerFacets == 1) {
        return appendQuad(facets, coord0, coord0+1, coord0+2, coord0+3,
                                triangulate);
    }

    //  The single interior strip is enclosed by a single ring:
    if ((uRes == 1) || (vRes == 1)) {
        return getSingleStripFacets(uRes, vRes, coord0, triangulate, facets);
    }

    //
    //  The general case -- one or more quads for each edge that are
    //  connected to the next interior ring of vertices:
    //
    int nFacets = 0;

    int uResInner = uRes - 2;
    int vResInner = vRes - 2;

    int outerRingStart = coord0;
    int innerRingStart = coord0 + 2 * (uRes + vRes);

    FacetStrip qStrip;
    qStrip.quadTopology    = true;
    qStrip.quadTriangulate = triangulate;
    qStrip.splitFirstFace  = false;
    qStrip.splitLastFace   = false;

    qStrip.outerEdges    = uRes;
    qStrip.outerFirst    = outerRingStart;
    qStrip.outerPrev     = innerRingStart - 1;
    qStrip.outerLast     = outerRingStart + uRes;
    qStrip.innerEdges    = uResInner;
    qStrip.innerReversed = false;
    qStrip.innerFirst    = innerRingStart;
    qStrip.innerLast     = innerRingStart + uResInner;
    nFacets += qStrip.connectUniformQuads(facets + nFacets);

    qStrip.outerEdges    = vRes;
    qStrip.outerFirst   += uRes;
    qStrip.outerPrev     = qStrip.outerFirst - 1;
    qStrip.outerLast     = qStrip.outerFirst + vRes;
    qStrip.innerEdges    = vResInner;
    qStrip.innerReversed = false;
    qStrip.innerFirst    = qStrip.innerLast;
    qStrip.innerLast    += vResInner;
    nFacets += qStrip.connectUniformQuads(facets + nFacets);

    qStrip.outerEdges    = uRes;
    qStrip.outerFirst   += vRes;
    qStrip.outerPrev     = qStrip.outerFirst - 1;
    qStrip.outerLast     = qStrip.outerFirst + uRes;
    qStrip.innerEdges    = uResInner;
    qStrip.innerReversed = (vResInner == 0);
    qStrip.innerFirst    = qStrip.innerLast;
    qStrip.innerLast    += uResInner * (qStrip.innerReversed ? -1 : 1);
    nFacets += qStrip.connectUniformQuads(facets + nFacets);

    qStrip.outerEdges    = vRes;
    qStrip.outerFirst   += uRes;
    qStrip.outerPrev     = qStrip.outerFirst - 1;
    qStrip.outerLast     = outerRingStart;
    qStrip.innerEdges    = vResInner;
    qStrip.innerReversed = (uResInner == 0);
    qStrip.innerFirst    = qStrip.innerLast;
    qStrip.innerLast     = innerRingStart;
    nFacets += qStrip.connectUniformQuads(facets + nFacets);

    return nFacets;
}

int
quad::getBoundaryRingFacets(int const outerRes[], int uRes, int vRes,
                            int nBoundaryEdges, bool triangulate,
                            FacetArray facets) {

    //  Identify edges and corners that should preserve uniform behavior:
    bool uniformEdges[4];
    uniformEdges[0] = (outerRes[0] == uRes);
    uniformEdges[1] = (outerRes[1] == vRes);
    uniformEdges[2] = (outerRes[2] == uRes);
    uniformEdges[3] = (outerRes[3] == vRes);

    bool uniformCorners[4];
    uniformCorners[0] = (uniformEdges[0] && uniformEdges[3]);
    uniformCorners[1] = (uniformEdges[1] && uniformEdges[0]);
    uniformCorners[2] = (uniformEdges[2] && uniformEdges[1]);
    uniformCorners[3] = (uniformEdges[3] && uniformEdges[2]);

    //  Initialize inner edge counts and the FacetStrip for local use:
    assert((uRes > 1) && (vRes > 1));
    int innerResU = uRes - 2;
    int innerResV = vRes - 2;

    int nFacets = 0;

    int outerRingStart = 0;
    int innerRingStart = nBoundaryEdges;

    FacetStrip qStrip;
    qStrip.quadTopology    = true;
    qStrip.quadTriangulate = triangulate;

    //  Assign strip indices for the inner and outer rings:
    qStrip.outerEdges    = outerRes[0];
    qStrip.outerFirst    = outerRingStart;
    qStrip.outerPrev     = innerRingStart - 1;
    qStrip.outerLast     = outerRingStart + outerRes[0];
    qStrip.innerEdges    = innerResU;
    qStrip.innerReversed = false;
    qStrip.innerFirst    = innerRingStart;
    qStrip.innerLast     = innerRingStart + innerResU;
    if (uniformEdges[0]) {
        qStrip.splitFirstFace = !uniformCorners[0];
        qStrip.splitLastFace  = !uniformCorners[1];
        nFacets += qStrip.connectUniformQuads(facets + nFacets);
    } else {
        qStrip.splitFirstFace = true;
        qStrip.splitLastFace  = true;
        nFacets += qStrip.connectNonUniformFacets(facets + nFacets);
    }

    qStrip.outerEdges    = outerRes[1];
    qStrip.outerFirst    = qStrip.outerLast;
    qStrip.outerPrev     = qStrip.outerFirst - 1;
    qStrip.outerLast    += outerRes[1];
    qStrip.innerEdges    = innerResV;
    qStrip.innerReversed = false;
    qStrip.innerFirst    = qStrip.innerLast;
    qStrip.innerLast    += innerResV;
    if (uniformEdges[1]) {
        qStrip.splitFirstFace = !uniformCorners[1];
        qStrip.splitLastFace  = !uniformCorners[2];
        nFacets += qStrip.connectUniformQuads(facets + nFacets);
    } else {
        qStrip.splitFirstFace = true;
        qStrip.splitLastFace  = true;
        nFacets += qStrip.connectNonUniformFacets(facets + nFacets);
    }

    qStrip.outerEdges    = outerRes[2];
    qStrip.outerFirst    = qStrip.outerLast;
    qStrip.outerPrev     = qStrip.outerFirst - 1;
    qStrip.outerLast    += outerRes[2];
    qStrip.innerEdges    = innerResU;
    qStrip.innerReversed = (innerResV == 0);
    qStrip.innerFirst    = qStrip.innerLast;
    qStrip.innerLast    += innerResU * (qStrip.innerReversed ? -1 : 1);
    if (uniformEdges[2]) {
        qStrip.splitFirstFace = !uniformCorners[2];
        qStrip.splitLastFace  = !uniformCorners[3];
        nFacets += qStrip.connectUniformQuads(facets + nFacets);
    } else {
        qStrip.splitFirstFace = true;
        qStrip.splitLastFace  = true;
        nFacets += qStrip.connectNonUniformFacets(facets + nFacets);
    }

    qStrip.outerEdges    = outerRes[3];
    qStrip.outerFirst    = qStrip.outerLast;
    qStrip.outerPrev     = qStrip.outerFirst - 1;
    qStrip.outerLast     = 0;
    qStrip.innerEdges    = innerResV;
    qStrip.innerReversed = (innerResU == 0);
    qStrip.innerFirst    = qStrip.innerLast;
    qStrip.innerLast     = innerRingStart;
    if (uniformEdges[3]) {
        qStrip.splitFirstFace = !uniformCorners[3];
        qStrip.splitLastFace  = !uniformCorners[0];
        nFacets += qStrip.connectUniformQuads(facets + nFacets);
    } else {
        qStrip.splitFirstFace = true;
        qStrip.splitLastFace  = true;
        nFacets += qStrip.connectNonUniformFacets(facets + nFacets);
    }
    return nFacets;
}

int
quad::GetSegmentedFacets(int const innerRes[], bool triangulate,
                         FacetArray facets) {

    //  WIP - may extend later to handle different opposing outer rates
    //        resulting in a non-uniform strip between the opposing edges
    int uRes = innerRes[0];
    int vRes = innerRes[1];
    assert((uRes == 1) || (vRes == 1));

    return getSingleStripFacets(uRes, vRes, 0, triangulate, facets);
}

int
quad::GetNonUniformFacets(int const outerRes[], int const innerRes[],
                          int nBoundaryEdges, bool triangulate,
                          FacetArray facets) {

    int uRes = innerRes[0];
    int vRes = innerRes[1];
    assert((uRes > 1) && (vRes > 1));

    //  First, generate the ring of boundary facets separately:
    int nFacets = getBoundaryRingFacets(outerRes, uRes, vRes, nBoundaryEdges,
                                        triangulate, facets);

    //  Second, generate the remaining rings of interior facets:
    int nRings = (std::min(uRes,vRes) + 1) / 2;
    int coord0 = nBoundaryEdges;

    for (int ring = 1; ring < nRings; ++ring) {
        uRes = std::max(uRes - 2, 0);
        vRes = std::max(vRes - 2, 0);

        nFacets += getInteriorRingFacets(uRes, vRes, coord0, triangulate,
                                         facets + nFacets);
        coord0  += 2 * (uRes + vRes);
    }
    return nFacets;
}

int
quad::GetUniformFacets(int res, bool triangulate, FacetArray facets) {

    //  The trivial case should have been handled by the caller:
    assert(res > 1);

    int nRings = (res + 1) / 2;

    int nFacets = 0;
    int coord0 = 0;
    for (int ring = 0; ring < nRings; ++ring, res -= 2) {
        nFacets += getInteriorRingFacets(res, res, coord0, triangulate,
                                         facets + nFacets);
        coord0  += 4 * res;
    }
    return nFacets;
}


//
//  REMINDER TO SELF -- according to the OpenGL docs, the "inner" tess
//  rates are expected to reflect a tessellation of the entire face, i.e.
//  they are not the outer rates with 2 subtracted, but are the same as
//  the outer rates.  Their minimum is therefore 1 -- no inner vertices,
//  BUT any non-unit outer rate will trigger an interior point.
//
//  Note that triangles will need considerably different treatment in
//  some cases given the way we diverge from the OpenGL patterns, e.g.
//  the corner faces are not bisected in the uniform case but may need
//  to be when non-uniform.
//

//
//  Implementations for tri functions:
//
inline int
tri::CountUniformFacets(int edgeRes) {
    return edgeRes * edgeRes;
}

int
tri::CountNonUniformFacets(int const outerRes[], int innerRes) {

    assert(innerRes > 2);

    //  Count interior facets based on edges of inner ring:
    int nInnerEdges = innerRes - 3;

    int nInterior = nInnerEdges ? CountUniformFacets(nInnerEdges) : 0;

    //
    //  Note the number of boundary facets is not affected by the uniform
    //  behavior at corners when rates match -- in contrast to quads.  In
    //  both cases, two tris are generated from four points at the corner,
    //  just with a different edge bisecting that "quad".
    //
    int nBoundary = (nInnerEdges + outerRes[0]) +
                    (nInnerEdges + outerRes[1]) +
                    (nInnerEdges + outerRes[2]);

    return nInterior + nBoundary;
}

inline int
tri::countUniformCoords(int edgeRes) {
    return edgeRes * (edgeRes + 1) / 2;
}

inline int
tri::CountInteriorCoords(int edgeRes) {
    return countUniformCoords(edgeRes - 2);
}

template <typename REAL>
inline int
tri::getCenterCoord(Coord2Array<REAL> coords) {

    coords[0].Set(1.0f/3.0f, 1.0f/3.0f);
    return 1;
}

template <typename REAL>
int
tri::GetEdgeCoords(int edge, int edgeRes, Coord2Array<REAL> coords) {

    REAL dt = 1.0f / (REAL)edgeRes;

    REAL t0 = dt;
    REAL t1 = 1.0f - dt;

    int n = edgeRes - 1;

    switch (edge) {
    case 0:  return appendVIsoLine<REAL>(coords, n,  t0, 0.0f, dt);
    case 1:  return appendUVLine<REAL>(  coords, n,  t1,  t0, -dt, dt);
    case 2:  return appendUIsoLine<REAL>(coords, n, 0.0f, t1, -dt);
    }
    return 0;
}

template <typename REAL>
int
tri::GetBoundaryCoords(int const edgeRates[], Coord2Array<REAL> coords) {

    int nCoords = 0;
    nCoords += appendVIsoLine<REAL>(coords + nCoords, edgeRates[0], 0.0f, 0.0f,
                                        1.0f / (REAL) edgeRates[0]);
    nCoords += appendUVLine<REAL>(  coords + nCoords, edgeRates[1], 1.0f, 0.0f,
                                       -1.0f / (REAL) edgeRates[1],
                                        1.0f / (REAL) edgeRates[1]);
    nCoords += appendUIsoLine<REAL>(coords + nCoords, edgeRates[2], 0.0f, 1.0f,
                                       -1.0f / (REAL) edgeRates[2]);
    return nCoords;
}

template <typename REAL>
int
tri::getInteriorRingCoords(int edgeRes, REAL u0, REAL v0, REAL dt,
                           Coord2Array<REAL> coords) {
    assert(edgeRes);

    REAL u1 = 1.0f - u0*2.0f;
    REAL v1 = 1.0f - v0*2.0f;

    int nCoords = 0;
    nCoords += appendVIsoLine(coords + nCoords, edgeRes, u0, v0, dt);
    nCoords += appendUVLine(  coords + nCoords, edgeRes, u1, v0,-dt, dt);
    nCoords += appendUIsoLine(coords + nCoords, edgeRes, u0, v1,-dt);
    return nCoords;
}

template <typename REAL>
int
tri::GetInteriorCoords(int edgeRes, Coord2Array<REAL> coords) {

    int nIntRings = edgeRes / 3;
    if (nIntRings == 0) return 0;

    REAL dt = 1.0f / (REAL) edgeRes;
    REAL u  = dt;
    REAL v  = dt;

    int ringRes = edgeRes - 3;

    int nCoords = 0;
    for (int i = 0; i < nIntRings; ++i, ringRes -= 3, u += dt, v += dt) {
        if (ringRes == 0) {
            nCoords += getCenterCoord(coords + nCoords);
        } else {
            nCoords += getInteriorRingCoords(ringRes, u, v, dt,
                                             coords + nCoords);
        }
    }
    return nCoords;
}

int
tri::getInteriorRingFacets(int edgeRes, int coord0, FacetArray facets) {

    //
    //  Deal with trivial cases with no inner vertices:
    //
    if (edgeRes < 1) {
        return 0;
    } else if (edgeRes == 1) {
        return appendTri(facets, coord0, coord0+1, coord0+2);
    } else if (edgeRes == 2) {
        appendTri(facets + 0, coord0+0, coord0+1, coord0+5);
        appendTri(facets + 1, coord0+2, coord0+3, coord0+1);
        appendTri(facets + 2, coord0+4, coord0+5, coord0+3);
        appendTri(facets + 3, coord0+1, coord0+3, coord0+5);
        return 4;
    }

    //
    //  Generate facets for the 3 tri-strips for each edge:
    //
    int nFacets = 0;

    int outerEdges = edgeRes;
    int innerEdges = edgeRes - 3;

    int outerRingStart = coord0;
    int innerRingStart = coord0 + 3 * outerEdges;

    FacetStrip tStrip;
    tStrip.quadTopology  = false;
    tStrip.innerReversed = false;
    tStrip.innerEdges    = innerEdges;
    tStrip.outerEdges    = outerEdges;

    tStrip.outerFirst = outerRingStart;
    tStrip.outerLast  = outerRingStart + outerEdges;
    tStrip.outerPrev  = innerRingStart - 1;
    tStrip.innerFirst = innerRingStart;
    tStrip.innerLast  = innerRingStart + innerEdges;
    nFacets += tStrip.connectUniformTris(facets + nFacets);

    tStrip.outerFirst += outerEdges;
    tStrip.outerLast  += outerEdges;
    tStrip.outerPrev   = tStrip.outerFirst - 1;
    tStrip.innerFirst += innerEdges;
    tStrip.innerLast  += innerEdges;
    nFacets += tStrip.connectUniformTris(facets + nFacets);

    tStrip.outerFirst += outerEdges;
    tStrip.outerLast   = outerRingStart;
    tStrip.outerPrev   = tStrip.outerFirst - 1;
    tStrip.innerFirst += innerEdges;
    tStrip.innerLast   = innerRingStart;
    nFacets += tStrip.connectUniformTris(facets + nFacets);

    return nFacets;
}

int
tri::getBoundaryRingFacets(int const outerRes[], int innerRes,
                           int nBoundaryEdges, FacetArray facets) {

    //  Identify edges and corners that should preserve uniform behavior:
    bool uniformEdges[3];
    uniformEdges[0] = (outerRes[0] == innerRes);
    uniformEdges[1] = (outerRes[1] == innerRes);
    uniformEdges[2] = (outerRes[2] == innerRes);

    bool uniformCorners[3];
    uniformCorners[0] = (uniformEdges[0] && uniformEdges[2]);
    uniformCorners[1] = (uniformEdges[1] && uniformEdges[0]);
    uniformCorners[2] = (uniformEdges[2] && uniformEdges[1]);

    //  Initialize inner edge count and the FacetStrip for local use:
    assert(innerRes > 2);
    int innerEdges = innerRes - 3;

    int nFacets = 0;

    int outerRingStart = 0;
    int innerRingStart = nBoundaryEdges;

    FacetStrip tStrip;
    tStrip.quadTopology  = false;
    tStrip.innerReversed = false;
    tStrip.innerEdges    = innerEdges;

    //  Assign the three strips of Facets:
    tStrip.outerEdges   = outerRes[0];
    tStrip.outerFirst   = outerRingStart;
    tStrip.outerLast    = outerRingStart + outerRes[0];
    tStrip.outerPrev    = innerRingStart - 1;
    tStrip.innerFirst   = innerRingStart;
    tStrip.innerLast    = innerRingStart + innerEdges;
    if (uniformEdges[0]) {
        tStrip.splitFirstFace = !uniformCorners[0];
        tStrip.splitLastFace  = !uniformCorners[1];
        nFacets += tStrip.connectUniformTris(facets + nFacets);
    } else {
        tStrip.splitFirstFace = true;
        tStrip.splitLastFace  = true;
        nFacets += tStrip.connectNonUniformFacets(facets + nFacets);
    }

    tStrip.outerEdges   = outerRes[1];
    tStrip.outerFirst   = tStrip.outerLast;
    tStrip.outerLast   += outerRes[1];
    tStrip.outerPrev    = tStrip.outerFirst - 1;
    tStrip.innerFirst   = tStrip.innerLast;
    tStrip.innerLast   += innerEdges;
    if (uniformEdges[1]) {
        tStrip.splitFirstFace = !uniformCorners[1];
        tStrip.splitLastFace  = !uniformCorners[2];
        nFacets += tStrip.connectUniformTris(facets + nFacets);
    } else {
        tStrip.splitFirstFace = true;
        tStrip.splitLastFace  = true;
        nFacets += tStrip.connectNonUniformFacets(facets + nFacets);
    }

    tStrip.outerEdges   = outerRes[2];
    tStrip.outerFirst   = tStrip.outerLast;
    tStrip.outerLast    = 0;
    tStrip.outerPrev    = tStrip.outerFirst - 1;
    tStrip.innerFirst   = tStrip.innerLast;
    tStrip.innerLast    = innerRingStart;
    if (uniformEdges[2]) {
        tStrip.splitFirstFace = !uniformCorners[2];
        tStrip.splitLastFace  = !uniformCorners[0];
        nFacets += tStrip.connectUniformTris(facets + nFacets);
    } else {
        tStrip.splitFirstFace = true;
        tStrip.splitLastFace  = true;
        nFacets += tStrip.connectNonUniformFacets(facets + nFacets);
    }
    return nFacets;
}

int
tri::GetUniformFacets(int edgeRes, FacetArray facets) {

    //  The trivial case should have been handled by the caller:
    assert(edgeRes > 1);

    int nRings = 1 + (edgeRes / 3);

    int nFacets = 0;
    int coord0  = 0;
    for (int ring = 0; ring < nRings; ++ring, edgeRes -= 3) {
        nFacets += getInteriorRingFacets(edgeRes, coord0, facets + nFacets);
        coord0  += 3 * edgeRes;
    }
    return nFacets;
}

int
tri::GetNonUniformFacets(int const outerRes[], int innerRes,
                         int nBoundaryEdges, FacetArray facets) {

    assert(innerRes > 2);

    //  First, generate the ring of boundary facets separately:
    int nFacets = getBoundaryRingFacets(outerRes, innerRes,
                                        nBoundaryEdges, facets);

    //  Second, generate the remaining rings of interior facets:
    int nRings = 1 + (innerRes / 3);
    int coord0  = nBoundaryEdges;

    for (int ring = 1; ring < nRings; ++ring) {
        innerRes -= 3;

        nFacets += getInteriorRingFacets(innerRes,
                                         coord0, facets + nFacets);
        coord0  += 3 * innerRes;
    }
    return nFacets;
}


//
//  These utilities support quadrangulated polygons used for quad-based
//  subdivision schemes.
//

//
//  The formulae to enumerate points and facets for a uniform tessellation
//  reflect the differing topologies for the odd and even case:
//
inline int
qsub::CountUniformFacets(int N, int edgeRes, bool triangulate) {

    bool resIsOdd = (edgeRes & 1);

    int H = edgeRes / 2;

    int nQuads  = (H + resIsOdd) * H * N;
    int nCenter = resIsOdd ? ((N == 3) ? 1 : N) : 0;

    return (nQuads << (int)triangulate) + nCenter;
}

int
qsub::CountNonUniformFacets(int N, int const outerRes[], int innerRes,
                            bool triangulate) {

    assert(innerRes > 1);

    //  Count interior facets based on edges of inner ring:
    int nInnerEdges = innerRes - 2;

    int nInterior = 0;
    if (nInnerEdges) {
        nInterior = CountUniformFacets(N, nInnerEdges, triangulate);
    }

    //
    //  Accumulate boundary facets for uniform vs non-uniform edge.  Uniform
    //  has a quad for each inner edge, plus one facet for leading corner
    //  and a tri for the trailing corner if not uniform.  Non-uniform has
    //  a tri for each inner edge and each outer edge:
    //
    int nBoundary = 0;
    for (int i = 0; i < N; ++i) {
        if (triangulate) {
            nBoundary += nInnerEdges + outerRes[i];
        } else if (outerRes[i] == innerRes) {
            nBoundary += nInnerEdges + 1 + (innerRes != outerRes[(i+1) % N]);
        } else {
            int nEdge = std::max(nInnerEdges, outerRes[i]);
            if ((nEdge & 1) == 0) {
                nEdge += (nInnerEdges & 1) || (outerRes[i] & 1);
            }
            nBoundary += nEdge;
        }
    }
    return nInterior + nBoundary;
}

inline int
qsub::countUniformCoords(int N, int edgeRes) {

    int H = edgeRes / 2;
    return (edgeRes & 1) ? (H+1)* (H+1) * N + ((N == 3) ? 0 : 1)
                         :   H  * (H+1) * N + 1;
}

inline int
qsub::CountInteriorCoords(int N, int edgeRes) {

    assert(edgeRes > 1);
    return countUniformCoords(N, edgeRes - 2);
}

template <typename REAL>
inline int
qsub::getCenterCoord(Coord2Array<REAL> coords) {

    coords[0].Set(0.5f, 0.5f);
    return 1;
}

template <typename REAL>
int
qsub::getRingEdgeCoords(Parameterization P, int edge, int edgeRes,
                        bool incFirst, bool incLast,
                        REAL tOrigin, REAL dt,
                        Coord2Array<REAL> coords) {

    //
    //  Determine number of coords in each half, excluding the ends.  The
    //  second half will get the extra when odd so that the sequence starts
    //  exactly on the boundary of the second sub-face (avoiding floating
    //  point error when accumulating to the boundary of the first):
    //
    int n0 = (edgeRes - 1) / 2;
    int n1 = (edgeRes - 1) - n0;

    int nCoords = 0;
    if (incFirst || n0) {
        REAL uv0[2];
        P.GetVertexCoord(edge, uv0);

        //  u ranges from [tOrigin < 0.5] while v is constant
        if (incFirst) {
            coords[nCoords++].Set(uv0[0] + tOrigin, uv0[1] + tOrigin);
        }
        if (n0) {
            REAL u = uv0[0] + tOrigin + dt;
            REAL v = uv0[1] + tOrigin;
            nCoords += appendVIsoLine(coords + nCoords, n0, u, v, dt);
        }
    }
    if (n1 || incLast) {
        REAL uv1[2];
        P.GetVertexCoord((edge + 1) % P.GetFaceSize(), uv1);

        //  u is constant while v ranges from [0.5 > tOrigin] (even)
        if (n1) {
            REAL u = uv1[0] + tOrigin;
            REAL v = uv1[1] + ((edgeRes & 1) ? (0.5f - 0.5f * dt) : 0.5f);
            nCoords += appendUIsoLine(coords + nCoords, n1, u, v, -dt);
        }
        if (incLast) {
            coords[nCoords++].Set(uv1[0] + tOrigin, uv1[1] + tOrigin);
        }
    }
    return nCoords;
}

template <typename REAL>
int
qsub::GetEdgeCoords(Parameterization P, int edge, int edgeRes,
                    Coord2Array<REAL> coords) {

    return getRingEdgeCoords<REAL>(P, edge, edgeRes, false, false,
                                   0.0f, 1.0f / (REAL)edgeRes,
                                   coords);
}

template <typename REAL>
int
qsub::GetBoundaryCoords(Parameterization P, int const edgeRates[],
                        Coord2Array<REAL> coords) {


    int N = P.GetFaceSize();

    int nCoords = 0;
    for (int i = 0; i < N; ++i) {
        nCoords += getRingEdgeCoords<REAL>(P, i, edgeRates[i], true, false,
                                           0.0f, 1.0f / (REAL)edgeRates[i],
                                           coords + nCoords);
    }
    return nCoords;
}

template <typename REAL>
int
qsub::getInteriorRingCoords(Parameterization P, int edgeRes,
                            REAL tOrigin, REAL dt,
                            Coord2Array<REAL> coords) {
    assert(edgeRes > 1);

    int N = P.GetFaceSize();

    int nCoords = 0;
    for (int i = 0; i < N; ++i) {
        nCoords += getRingEdgeCoords(P, i, edgeRes, true, false,
                                     tOrigin, dt,
                                     coords + nCoords);
    }
    return nCoords;
}

template <typename REAL>
int
qsub::getCenterRingCoords(Parameterization P, REAL tOrigin,
                          Coord2Array<REAL> coords) {

    int N = P.GetFaceSize();

    //  Just need the single corner point for each edge here:
    for (int i = 0; i < N; ++i) {
        REAL uv[2];
        P.GetVertexCoord(i, uv);
        coords[i].Set(uv[0] + tOrigin, uv[1] + tOrigin);
    }
    return (N == 3) ? N : (N + getCenterCoord(coords + N));
}

template <typename REAL>
int
qsub::GetInteriorCoords(Parameterization P, int edgeRes,
                        Coord2Array<REAL> coords) {

    int nIntRings = edgeRes / 2;
    if (nIntRings == 0) return 0;

    REAL dt = 1.0f / (REAL) edgeRes;
    REAL t  = dt;

    int ringRes = edgeRes - 2;

    int nCoords = 0;
    for (int i = 0; i < nIntRings; ++i, ringRes -= 2, t += dt) {
        if (ringRes == 0) {
            nCoords += getCenterCoord(coords + nCoords);
        } else if (ringRes == 1) {
            nCoords += getCenterRingCoords(P, t, coords + nCoords);
        } else {
            nCoords += getInteriorRingCoords(P, ringRes, t, dt,
                                             coords + nCoords);
        }
    }
    return nCoords;
}

int
qsub::getCenterFacets(int N, int coord0, FacetArray facets) {

    return (N == 3) ? appendTri(facets, coord0, coord0+1, coord0+2)
                    : appendTriFan(facets, N, coord0);
}

int
qsub::getInteriorRingFacets(int N, int edgeRes, int coord0, bool triangulate,
                            FacetArray facets) {

    //
    //  Deal with trivial cases with no inner vertices:
    //
    if (edgeRes < 1) return 0;

    if (edgeRes == 1) {
        return getCenterFacets(N, coord0, facets);
    }

    //
    //  Generate facets for the N quad-strips for each edge:
    //
    int outerRes  = edgeRes;
    int outerRing = coord0;

    int innerRes  = outerRes - 2;
    int innerRing = outerRing + N * outerRes;

    int nFacets = 0;

    FacetStrip qStrip;
    qStrip.quadTopology    = true;
    qStrip.quadTriangulate = triangulate;
    qStrip.outerEdges      = outerRes;
    qStrip.innerEdges      = innerRes;
    qStrip.innerReversed   = false;
    qStrip.splitFirstFace  = false;
    qStrip.splitLastFace   = false;

    for (int edge = 0; edge < N; ++edge) {
        qStrip.outerFirst = outerRing + edge * outerRes;
        qStrip.innerFirst = innerRing + edge * innerRes;

        qStrip.outerPrev = (edge ? qStrip.outerFirst : innerRing) - 1;

        if (edge < N-1) {
            qStrip.outerLast = qStrip.outerFirst + outerRes;
            qStrip.innerLast = qStrip.innerFirst + innerRes;
        } else {
            qStrip.outerLast = outerRing;
            qStrip.innerLast = innerRing;
        }

        nFacets += qStrip.connectUniformQuads(facets + nFacets);
    }
    return nFacets;
}

int
qsub::getBoundaryRingFacets(int N, int const outerRes[], int innerRes,
                            int nBoundaryEdges, bool triangulate,
                            FacetArray facets) {

    int innerEdges = std::max(innerRes - 2, 0);

    int nFacets = 0;

    int outerRingStart = 0;
    int innerRingStart = nBoundaryEdges;

    //  Initialize properties of the strip that are fixed:
    FacetStrip qStrip;
    qStrip.quadTopology    = true;
    qStrip.quadTriangulate = triangulate;
    qStrip.innerReversed   = false;
    qStrip.innerEdges      = innerEdges;

    for (int edge = 0; edge < N; ++edge) {
        qStrip.outerEdges = outerRes[edge];

        //  Initialize the indices starting this strip:
        if (edge) {
            qStrip.outerFirst = qStrip.outerLast;
            qStrip.outerPrev  = qStrip.outerFirst - 1;
            qStrip.innerFirst = qStrip.innerLast;
        } else {
            qStrip.outerFirst = outerRingStart;
            qStrip.outerPrev  = innerRingStart - 1;
            qStrip.innerFirst = innerRingStart;
        }

        //  Initialize the indices ending this strip:
        if (edge < N-1) {
            qStrip.outerLast = qStrip.outerFirst + qStrip.outerEdges;
            qStrip.innerLast = qStrip.innerFirst + qStrip.innerEdges;
        } else {
            qStrip.outerLast = outerRingStart;
            qStrip.innerLast = innerRingStart;
        }

        //  Test rates at, before and after this edge for uniform behavior:
        if ((outerRes[edge] == innerRes) && (innerRes > 1)) {
            qStrip.splitFirstFace = (outerRes[(edge-1+N) % N] != innerRes);
            qStrip.splitLastFace  = (outerRes[(edge + 1) % N] != innerRes);

            nFacets += qStrip.connectUniformQuads(facets+nFacets);
        } else {
            qStrip.splitFirstFace = true;
            qStrip.splitLastFace  = true;

            nFacets += qStrip.connectNonUniformFacets(facets + nFacets);
        }
    }
    return nFacets;
}
    
int
qsub::GetUniformFacets(int N, int edgeRes, bool triangulate,
                       FacetArray facets) {

    //  The trivial (single facet) case should be handled externally:
    if (edgeRes == 1) {
        return getCenterFacets(N, 0, facets);
    }

    int nRings = (edgeRes + 1) / 2;

    int nFacets = 0;
    int coord0  = 0;
    for (int ring = 0; ring < nRings; ++ring, edgeRes -= 2) {
        nFacets += getInteriorRingFacets(N, edgeRes, coord0, triangulate,
                                         facets + nFacets);
        coord0  += N * edgeRes;
    }
    return nFacets;
}

int
qsub::GetNonUniformFacets(int N, int const outerRes[], int innerRes,
                          int nBoundaryEdges, bool triangulate,
                          FacetArray facets) {

    //  First, generate the ring of boundary facets separately:
    int nFacets = getBoundaryRingFacets(N, outerRes, innerRes, nBoundaryEdges,
                                        triangulate, facets);

    //  Second, generate the remaining rings of interior facets:
    int nRings = (innerRes + 1) / 2;
    int coord0  = nBoundaryEdges;

    for (int ring = 1; ring < nRings; ++ring) {
        innerRes = std::max(innerRes - 2, 0);

        nFacets += getInteriorRingFacets(N, innerRes, coord0, triangulate,
                                         facets + nFacets);
        coord0  += N * innerRes;
    }
    return nFacets;
}


//
//  Internal initialization methods:
//
void
Tessellation::initializeDefaults() {

    std::memset((void*) this, 0, sizeof(*this));

    //  Assign any non-zero defaults:
    _triangulate = true;

    _isValid = false;
}

bool
Tessellation::validateArguments(Parameterization const & p,
        int numRates, int const rates[], Options const & options) {

    //  Check the Parameterization:
    if (!p.IsValid()) return false;

    //  Check given tessellation rates:
    if (numRates < 1) return false;
    for (int i = 0; i < numRates; ++i) {
        if (rates[i] < 1) return false;
    }

    //  Check given buffer strides in Options:
    int coordStride = options.GetCoordStride();
    if (coordStride && (coordStride < 2)) return false;

    int facetStride = options.GetFacetStride();
    if (facetStride && (facetStride < options.GetFacetSize())) return false;

    return true;
}

void
Tessellation::initialize(Parameterization const & p,
        int numRates, int const rates[], Options const & options) {

    //  Validate arguments and initialize simple members:
    initializeDefaults();

    if (!validateArguments(p, numRates, rates, options)) return;

    _param = p;

    _facetSize   = (short) options.GetFacetSize();
    _facetStride = options.GetFacetStride() ?
                   options.GetFacetStride() : options.GetFacetSize();

    _coordStride = options.GetCoordStride() ? options.GetCoordStride() : 2;

    //  Initialize the full array of rates, returning sum of outer edge rates
    int sumOfOuterRates = initializeRates(numRates, rates);

    //  Initialize the inventory based on the Parameterization type:
    _triangulate = (_facetSize == 3) || !options.PreserveQuads();

    switch (_param.GetType()) {
    case Parameterization::QUAD:
        initializeInventoryForParamQuad(sumOfOuterRates);
        break;
    case Parameterization::TRI:
        initializeInventoryForParamTri(sumOfOuterRates);
        break;
    case Parameterization::QUAD_SUBFACES:
        initializeInventoryForParamQPoly(sumOfOuterRates);
        break;
    }
    _isValid = true;

    //  Debugging output:
    bool printNonUniform = false; // !_isUniform;
    if (printNonUniform) {
        int N = _param.GetFaceSize();
        printf("Tessellation::initialize(%d, numRates = %d):\n", N, numRates);
        printf("    is uniform          = %d\n", _isUniform);
        printf("        outer rates     =");
        for (int i = 0; i < N; ++i) printf(" %d", _outerRates[i]);
        printf("\n");
        printf("        inner rate(s)   = %d", _innerRates[0]);
        if (N == 4) printf(" %d\n", _innerRates[1]);
        printf("\n");
        printf("    num boundary points = %d\n", _numBoundaryPoints);
        printf("    num interior points = %d\n", _numInteriorPoints);
        printf("    num facets          = %d\n", _numFacets);
    }
}

int
Tessellation::initializeRates(int numGivenRates, int const givenRates[]) {

    _numGivenRates = numGivenRates;

    //  Allocate space for rates of N-sided faces if necessary:
    int N = _param.GetFaceSize();
    if (N > (int)(sizeof(_outerRatesLocal) / sizeof(int))) {
        _outerRates = new int[N];
    } else {
        _outerRates = &_outerRatesLocal[0];
    }
    bool isQuad = (N == 4);

    //  Keep track of the total tessellation rate for all edges to return:
    int const MaxRate = std::numeric_limits<short>::max();

    int totalEdgeRate = 0;
    if (numGivenRates < N) {
        //  Given one or two inner rates, infer outer (others < N ignored):
        if ((numGivenRates == 2) && isQuad) {
            //  Infer outer rates from two given inner rates of quad:
            _innerRates[0] = std::min(givenRates[0], MaxRate);
            _innerRates[1] = std::min(givenRates[1], MaxRate);

            _outerRates[0] = _outerRates[2] = _innerRates[0];
            _outerRates[1] = _outerRates[3] = _innerRates[1];

            _isUniform = (_innerRates[0] == _innerRates[1]);
            totalEdgeRate = 2 * (_innerRates[0] + _innerRates[1]);
        } else {
            //  Infer outer rates from single inner rate (uniform):
            _innerRates[0] = std::min(givenRates[0], MaxRate);
            _innerRates[1] = _innerRates[0];

            std::fill(_outerRates, _outerRates + N, _innerRates[0]);

            _isUniform = true;
            totalEdgeRate = _innerRates[0] * N;
        }
    } else {
        //  Assign the N outer rates:
        _isUniform = true;
        for (int i = 0; i < N; ++i) {
            _outerRates[i] = std::min(givenRates[i], MaxRate);
            _isUniform = _isUniform && (_outerRates[i] == _outerRates[0]);
            totalEdgeRate += _outerRates[i];
        }

        //  Assign any given inner rates or infer:
        if (numGivenRates > N) {
            //  Assign single inner rate, assign/infer second for quad:
            _innerRates[0] = std::min(givenRates[N], MaxRate);
            _innerRates[1] = ((numGivenRates == 6) && isQuad)
                           ? std::min(givenRates[5], MaxRate) : _innerRates[0];

            _isUniform = _isUniform && (_innerRates[0] == _outerRates[0]);
            _isUniform = _isUniform && (_innerRates[1] == _outerRates[0]);
        } else if (isQuad) {
            //  Infer two inner rates for quads (avg of opposite edges):
            _innerRates[0] = (_outerRates[0] + _outerRates[2]) / 2;
            _innerRates[1] = (_outerRates[1] + _outerRates[3]) / 2;
        } else {
            //  Infer single inner rate for non-quads (avg of edge rates)
            _innerRates[0] = totalEdgeRate / N;
            _innerRates[1] = _innerRates[0];
        }
    }
    return totalEdgeRate;
}

int
Tessellation::GetRates(int rates[]) const {

    int N = _param.GetFaceSize();

    int numOuterRates = std::min<int>(N, _numGivenRates);
    int numInnerRates = std::max<int>(0, _numGivenRates - N);

    for (int i = 0; i < numOuterRates; ++i) {
        rates[i] = _outerRates[i];
    }
    for (int i = 0; i < numInnerRates; ++i) {
        rates[N + i] = _innerRates[i > 0];
    }
    return _numGivenRates;
}

void
Tessellation::initializeInventoryForParamQuad(int sumOfEdgeRates) {

    int const * inner = &_innerRates[0];
    int const * outer = &_outerRates[0];

    if (_isUniform) {
        if (inner[0] > 1) {
            _numInteriorPoints = quad::CountInteriorCoords(inner[0]);
            _numFacets = quad::CountUniformFacets(inner[0], _triangulate);
        } else if (_triangulate) {
            _numInteriorPoints = 0;
            _numFacets = 2;
            _splitQuad = true;
        } else {
            _numInteriorPoints = 0;
            _numFacets = 1;
            _singleFace = true;
        }
    } else {
        //
        //  For quads another low-res case is recognized when there are
        //  no interior points, but the face has extra boundary points.
        //  Instead of introducing a center point, the face is considered
        //  to be "segmented" into other faces that cover it without the
        //  addition of any interior vertices.
        //
        //  This currently occurs for a pure 1 x M tessellation -- from
        //  which a quad strip is generated -- but could be extended to
        //  handle the 1 x M inner case with additional points on the
        //  opposing edges.
        //
        if ((inner[0] > 1) && (inner[1] > 1)) {
            _numInteriorPoints = quad::CountInteriorCoords(_innerRates);
            _numFacets = quad::CountNonUniformFacets(_outerRates, _innerRates,
                                                     _triangulate);
        } else if ((outer[0] == inner[0]) && (inner[0] == outer[2]) &&
                   (outer[1] == inner[1]) && (inner[1] == outer[3])) {
            _numInteriorPoints = 0;
            _numFacets = quad::CountSegmentedFacets(_innerRates, _triangulate);
            _segmentedFace = true;
        } else {
            _numInteriorPoints = 1;
            _numFacets = sumOfEdgeRates;
            _triangleFan = true;
        }
    }
    _numBoundaryPoints = sumOfEdgeRates;
}

void
Tessellation::initializeInventoryForParamTri(int sumOfEdgeRates) {

    int res = _innerRates[0];

    if (_isUniform) {
        if (res > 1) {
            _numInteriorPoints = tri::CountInteriorCoords(res);
            _numFacets = tri::CountUniformFacets(res);
        } else {
            _numInteriorPoints = 0;
            _numFacets = 1;
            _singleFace = true;
        }
    } else {
        if (res > 2) {
            _numInteriorPoints = tri::CountInteriorCoords(res);
            _numFacets = tri::CountNonUniformFacets(_outerRates, res);
        } else {
            _numInteriorPoints = 1;
            _numFacets = sumOfEdgeRates;
            _triangleFan = true;
        }
    }
    _numBoundaryPoints = sumOfEdgeRates;
}

void
Tessellation::initializeInventoryForParamQPoly(int sumOfEdgeRates) {

    int N   = _param.GetFaceSize();
    int res = _innerRates[0];

    if (_isUniform) {
        if (res > 1) {
            _numInteriorPoints = qsub::CountInteriorCoords(N, res);
            _numFacets = qsub::CountUniformFacets(N, res, _triangulate);
        } else if (N == 3) {
            _numInteriorPoints = 0;
            _numFacets = 1;
            _singleFace  = true;
        } else {
            _numInteriorPoints = 1;
            _numFacets = N;
            _triangleFan = true;
        }
    } else {
        if (res > 1) {
            _numInteriorPoints = qsub::CountInteriorCoords(N, res);
            _numFacets = qsub::CountNonUniformFacets(N, _outerRates, res,
                                                     _triangulate);
        } else {
            _numInteriorPoints = 1;
            _numFacets = sumOfEdgeRates;
            _triangleFan = true;
        }
    }
    _numBoundaryPoints = sumOfEdgeRates;
}

//
//  Tessellation constructors and destructor:
//
Tessellation::Tessellation(
        Parameterization const & p, int uniformRate,
        Options const & options) {

    initialize(p, 1, &uniformRate, options);
}

Tessellation::Tessellation(
        Parameterization const & p, int numRates, int const rates[],
        Options const & options) {

    initialize(p, numRates, rates, options);
}

Tessellation::~Tessellation() {

    if (_outerRates != &_outerRatesLocal[0]) {
        delete[] _outerRates;
    }
}


//
//  Main methods to retrieve samples and facets:
//
template <typename REAL>
int
Tessellation::GetEdgeCoords(int edge, REAL coordBuffer[]) const {

    //  Remember this method excludes coords at the end vertices
    int edgeRes = _outerRates[edge];

    Coord2Array<REAL> coords(coordBuffer, _coordStride);

    switch (_param.GetType()) {
    case Parameterization::QUAD:
        return quad::GetEdgeCoords(edge, edgeRes, coords);
    case Parameterization::TRI:
        return tri::GetEdgeCoords(edge, edgeRes, coords);
    case Parameterization::QUAD_SUBFACES:
        return qsub::GetEdgeCoords(_param, edge, edgeRes, coords);
    default:
        assert(0);
    }
    return -1;
}

template <typename REAL>
int
Tessellation::GetBoundaryCoords(REAL coordBuffer[]) const {

    Coord2Array<REAL> coords(coordBuffer, _coordStride);

    switch (_param.GetType()) {
    case Parameterization::QUAD:
        return quad::GetBoundaryCoords(_outerRates, coords);
    case Parameterization::TRI:
        return tri::GetBoundaryCoords(_outerRates, coords);
    case Parameterization::QUAD_SUBFACES:
        return qsub::GetBoundaryCoords(_param, _outerRates, coords);
    default:
        assert(0);
    }
    return -1;
}

template <typename REAL>
int
Tessellation::GetInteriorCoords(REAL coordBuffer[]) const {

    if (_numInteriorPoints == 0) return 0;

    if (_numInteriorPoints == 1) {
        _param.GetCenterCoord(coordBuffer);
        return 1;
    }

    Coord2Array<REAL> coords(coordBuffer, _coordStride);

    switch (_param.GetType()) {
    case Parameterization::QUAD:
        return quad::GetInteriorCoords(_innerRates, coords);
    case Parameterization::TRI:
        return tri::GetInteriorCoords(_innerRates[0], coords);
    case Parameterization::QUAD_SUBFACES:
        return qsub::GetInteriorCoords(_param, _innerRates[0], coords);
    default:
        assert(0);
    }
    return 0;
}

int
Tessellation::GetFacets(int facetIndices[]) const {

    FacetArray facets(facetIndices, _facetSize, _facetStride);

    int N = GetFaceSize();

    if (_singleFace) {
        if (N == 3) {
            return appendTri(facets, 0, 1, 2);
        } else {
            return appendQuad(facets, 0, 1, 2, 3, 0);
        }
    }
    if (_triangleFan) {
        return appendTriFan(facets, _numFacets, 0);
    }
    if (_splitQuad) {
        return appendQuad(facets, 0, 1, 2, 3, _triangulate);
    }

    int nFacets = 0;
    switch (_param.GetType()) {
    case Parameterization::QUAD:
        if (_isUniform) {
            nFacets = quad::GetUniformFacets(_innerRates[0], _triangulate,
                                facets);
        } else if (_segmentedFace) {
            nFacets = quad::GetSegmentedFacets(_innerRates, _triangulate,
                                facets);
        } else {
            nFacets = quad::GetNonUniformFacets(_outerRates, _innerRates,
                                _numBoundaryPoints, _triangulate, facets);
        }
        break;
    case Parameterization::TRI:
        if (_isUniform) {
            nFacets = tri::GetUniformFacets(_innerRates[0], facets);
        } else {
            nFacets = tri::GetNonUniformFacets(_outerRates, _innerRates[0],
                                _numBoundaryPoints, facets);
        }
        break;
    case Parameterization::QUAD_SUBFACES:
        if (_isUniform) {
            nFacets = qsub::GetUniformFacets(N, _innerRates[0], _triangulate,
                                facets);
        } else {
            nFacets = qsub::GetNonUniformFacets(N, _outerRates, _innerRates[0],
                                _numBoundaryPoints, _triangulate, facets);
        }
        break;
    default:
        assert(0);
    }
    assert(nFacets == _numFacets);
    return nFacets;
}

void
Tessellation::TransformFacetCoordIndices(int facetIndices[], int commonOffset) {

    if (_facetSize == 4) {
        for (int i = 0; i < _numFacets; ++i, facetIndices += _facetStride) {
            facetIndices[0] += commonOffset;
            facetIndices[1] += commonOffset;
            facetIndices[2] += commonOffset;
            if (facetIndices[3] >= 0) {
                facetIndices[3] += commonOffset;
            }
        }
    } else {
        for (int i = 0; i < _numFacets; ++i, facetIndices += _facetStride) {
            facetIndices[0] += commonOffset;
            facetIndices[1] += commonOffset;
            facetIndices[2] += commonOffset;
        }
    }
}

void
Tessellation::TransformFacetCoordIndices(int facetIndices[],
                                         int const boundaryIndices[],
                                         int interiorOffset) {

    for (int i = 0; i < _numFacets; ++i, facetIndices += _facetStride) {
        for (int j = 0; j < (int)_facetSize; ++j) {
            int & index = facetIndices[j];
            if (index >= 0) {
                index = (index < _numBoundaryPoints)
                      ? boundaryIndices[index]
                      : (index + interiorOffset);
            }
        }
    }
}

void
Tessellation::TransformFacetCoordIndices(int facetIndices[],
                                         int const boundaryIndices[],
                                         int const interiorIndices[]) {

    for (int i = 0; i < _numFacets; ++i, facetIndices += _facetStride) {
        for (int j = 0; j < (int)_facetSize; ++j) {
            int & index = facetIndices[j];
            if (index >= 0) {
                index = (index < _numBoundaryPoints)
                      ? boundaryIndices[index]
                      : interiorIndices[index - _numBoundaryPoints];
            }
        }
    }
}

//
//  Explicit instantiation for multiple precision coordinate pairs:
//
template int
Tessellation::GetBoundaryCoords<float>(float coordBuffer[]) const;
template int
Tessellation::GetBoundaryCoords<double>(double coordBuffer[]) const;

template int
Tessellation::GetInteriorCoords<float>(float coordBuffer[]) const;
template int
Tessellation::GetInteriorCoords<double>(double coordBuffer[]) const;

template int
Tessellation::GetEdgeCoords<float>(int edge, float coordBuffer[]) const;
template int
Tessellation::GetEdgeCoords<double>(int edge, double coordBuffer[]) const;

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
