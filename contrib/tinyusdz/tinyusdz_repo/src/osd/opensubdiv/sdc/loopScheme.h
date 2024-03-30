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
#ifndef OPENSUBDIV3_SDC_LOOP_SCHEME_H
#define OPENSUBDIV3_SDC_LOOP_SCHEME_H

#include "../version.h"

#include "../sdc/scheme.h"

#include <cassert>
#include <cmath>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {
namespace Sdc {

constexpr auto kPI = 3.14159265358979323846;
//constexpr auto kPI_2 = kPI/2.0;

//
//  Specializations for Sdc::Scheme<SCHEME_LOOP>:
//
//

//
//  Loop traits:
//
template <>
inline Split Scheme<SCHEME_LOOP>::GetTopologicalSplitType() { return SPLIT_TO_TRIS; }

template <>
inline int Scheme<SCHEME_LOOP>::GetRegularFaceSize() { return 3; }

template <>
inline int Scheme<SCHEME_LOOP>::GetRegularVertexValence() { return 6; }

template <>
inline int Scheme<SCHEME_LOOP>::GetLocalNeighborhoodSize() { return 1; }


//
//  Protected methods to assign the two types of masks for an edge-vertex --
//  Crease and Smooth.
//
//  The Crease case does not really need to be specialized, though it may be
//  preferable to define all explicitly here.
//
template <>
template <typename EDGE, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignCreaseMaskForEdge(EDGE const&, MASK& mask) const
{
    mask.SetNumVertexWeights(2);
    mask.SetNumEdgeWeights(0);
    mask.SetNumFaceWeights(0);
    mask.SetFaceWeightsForFaceCenters(false);

    mask.VertexWeight(0) = 0.5f;
    mask.VertexWeight(1) = 0.5f;
}

template <>
template <typename EDGE, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignSmoothMaskForEdge(EDGE const& edge, MASK& mask) const
{
    int faceCount = edge.GetNumFaces();

    mask.SetNumVertexWeights(2);
    mask.SetNumEdgeWeights(0);
    mask.SetNumFaceWeights(faceCount);
    mask.SetFaceWeightsForFaceCenters(false);

    //
    //  This is where we run into the issue of "face weights" -- we want to weight the
    //  face-centers for Catmark, but face-centers are not generated for Loop.  So do
    //  we make assumptions on how the mask is used, assign some property to the mask
    //  to indicate how they were assigned, or take input from the mask itself?
    //
    //  Regardless, we have two choices:
    //      - face-weights are for the vertices opposite the edge (as in Hbr):
    //          vertex weights = 0.375f;
    //          face weights   = 0.125f;
    //
    //      - face-weights are for the face centers:
    //          vertex weights = 0.125f;
    //          face weights   = 0.375f;
    //
    //  Coincidentally the coefficients are the same but reversed.
    //
    typedef typename MASK::Weight Weight;

    Weight vWeight = mask.AreFaceWeightsForFaceCenters() ? 0.125f : 0.375f;
    Weight fWeight = mask.AreFaceWeightsForFaceCenters() ? 0.375f : 0.125f;

    mask.VertexWeight(0) = vWeight;
    mask.VertexWeight(1) = vWeight;

    if (faceCount == 2) {
        mask.FaceWeight(0) = fWeight;
        mask.FaceWeight(1) = fWeight;
    } else {
        //  The non-manifold case is not clearly defined -- we adjust the above
        //  face-weight to preserve the ratio of edge-center and face-centers:
        fWeight *= 2.0f / (Weight) faceCount;
        for (int i = 0; i < faceCount; ++i) {
            mask.FaceWeight(i) = fWeight;
        }
    }
}


//
//  Protected methods to assign the three types of masks for a vertex-vertex --
//  Corner, Crease and Smooth (Dart is the same as Smooth).
//
//  Corner and Crease do not really need to be specialized, though it may be
//  preferable to define all explicitly here.
//
template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignCornerMaskForVertex(VERTEX const&, MASK& mask) const
{
    mask.SetNumVertexWeights(1);
    mask.SetNumEdgeWeights(0);
    mask.SetNumFaceWeights(0);
    mask.SetFaceWeightsForFaceCenters(false);

    mask.VertexWeight(0) = 1.0f;
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignCreaseMaskForVertex(VERTEX const& vertex, MASK& mask,
                                               int const creaseEnds[2]) const {
    typedef typename MASK::Weight Weight;

    int valence = vertex.GetNumEdges();

    mask.SetNumVertexWeights(1);
    mask.SetNumEdgeWeights(valence);
    mask.SetNumFaceWeights(0);
    mask.SetFaceWeightsForFaceCenters(false);

    Weight vWeight = 0.75f;
    Weight eWeight = 0.125f;

    mask.VertexWeight(0) = vWeight;
    for (int i = 0; i < valence; ++i) {
        mask.EdgeWeight(i) = 0.0f;
    }
    mask.EdgeWeight(creaseEnds[0]) = eWeight;
    mask.EdgeWeight(creaseEnds[1]) = eWeight;
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignSmoothMaskForVertex(VERTEX const& vertex, MASK& mask) const
{
    typedef typename MASK::Weight Weight;

    int valence = vertex.GetNumFaces();

    mask.SetNumVertexWeights(1);
    mask.SetNumEdgeWeights(valence);
    mask.SetNumFaceWeights(0);
    mask.SetFaceWeightsForFaceCenters(false);

    //  Specialize for the regular case:  1/16 per edge-vert, 5/8 for the vert itself:
    Weight eWeight = (Weight) 0.0625f;
    Weight vWeight = (Weight) 0.625f;

    if (valence != 6) {
        //  From HbrLoopSubdivision<T>::Subdivide(mesh, vertex):
        //     - could use some lookup tables here for common irregular valence (5, 7, 8)
        //       or all of these cosine calls will be adding up...

        double dValence   = (double) valence;
        double invValence = 1.0f / dValence;
        double cosTheta   = std::cos(kPI * 2.0f * invValence);

        double beta = 0.25f * cosTheta + 0.375f;

        eWeight = (Weight) ((0.625f - (beta * beta)) * invValence);
        vWeight = (Weight) (1.0f - (eWeight * dValence));
    }

    mask.VertexWeight(0) = vWeight;
    for (int i = 0; i < valence; ++i) {
        mask.EdgeWeight(i) = eWeight;
    }
}


//
//  Limit masks for position:
//
template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignCornerLimitMask(VERTEX const& /* vertex */, MASK& posMask) const {

    posMask.SetNumVertexWeights(1);
    posMask.SetNumEdgeWeights(0);
    posMask.SetNumFaceWeights(0);
    posMask.SetFaceWeightsForFaceCenters(false);

    posMask.VertexWeight(0) = 1.0f;
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignCreaseLimitMask(VERTEX const& vertex, MASK& posMask,
                                           int const creaseEnds[2]) const {

    typedef typename MASK::Weight Weight;

    int valence = vertex.GetNumEdges();

    posMask.SetNumVertexWeights(1);
    posMask.SetNumEdgeWeights(valence);
    posMask.SetNumFaceWeights(0);
    posMask.SetFaceWeightsForFaceCenters(false);

    //
    //  The refinement mask for a crease vertex is (1/8, 3/4, 1/8) and for a crease
    //  edge is (1/2, 1/2) -- producing a uniform B-spline curve along the crease
    //  (boundary) whether the vertex or its crease is regular or not.  The limit
    //  mask is therefore (1/6, 2/3, 1/6) for ALL cases.
    //
    //  An alternative limit mask (1/5, 3/5, 1/5) is often published for use either
    //  for irregular crease vertices or for all crease/boundary vertices, but this
    //  is based on an alternate refinement mask for the edge -- (3/8, 5/8) versus
    //  the usual (1/2, 1/2) -- and will not produce the B-spline curve desired.
    //
    Weight vWeight = (Weight) (4.0 / 6.0);
    Weight eWeight = (Weight) (1.0 / 6.0);

    posMask.VertexWeight(0) = vWeight;
    for (int i = 0; i < valence; ++i) {
        posMask.EdgeWeight(i) = 0.0f;
    }
    posMask.EdgeWeight(creaseEnds[0]) = eWeight;
    posMask.EdgeWeight(creaseEnds[1]) = eWeight;
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignSmoothLimitMask(VERTEX const& vertex, MASK& posMask) const {

    typedef typename MASK::Weight Weight;

    int valence = vertex.GetNumFaces();

    posMask.SetNumVertexWeights(1);
    posMask.SetNumEdgeWeights(valence);
    posMask.SetNumFaceWeights(0);
    posMask.SetFaceWeightsForFaceCenters(false);

    //  Specialize for the regular case:  1/12 per edge-vert, 1/2 for the vert itself:
    if (valence == 6) {
        Weight eWeight = (Weight) (1.0 / 12.0);
        Weight vWeight = 0.5f;

        posMask.VertexWeight(0) = vWeight;

        posMask.EdgeWeight(0) = eWeight;
        posMask.EdgeWeight(1) = eWeight;
        posMask.EdgeWeight(2) = eWeight;
        posMask.EdgeWeight(3) = eWeight;
        posMask.EdgeWeight(4) = eWeight;
        posMask.EdgeWeight(5) = eWeight;

    } else {
        double dValence   = (double) valence;
        double invValence = 1.0f / dValence;
        double cosTheta   = std::cos(kPI * 2.0f * invValence);

        double beta  = 0.25f * cosTheta + 0.375f;
        double gamma = (0.625f - (beta * beta)) * invValence;

        Weight eWeight = (Weight) (1.0f / (dValence + 3.0f / (8.0f * gamma)));
        Weight vWeight = (Weight) (1.0f - (eWeight * dValence));

        posMask.VertexWeight(0) = vWeight;
        for (int i = 0; i < valence; ++i) {
            posMask.EdgeWeight(i) = eWeight;
        }
    }
}

/*
//  Limit masks for tangents:
//
//  A note on tangent magnitudes:
//
//  Several formulae exist for limit tangents at a vertex to accommodate the
//  different topological configurations around the vertex.  While these produce
//  the desired direction, there is inconsistency in the resulting magnitudes.
//  Ideally a regular mesh of uniformly shaped triangles with similar edge lengths
//  should produce tangents of similar magnitudes throughout -- including corners
//  and boundaries.  So some of the common formulae for these are adjusted with
//  scale factors.
//
//  For uses where magnitude does not matter, this scaling should be irrelevant.
//  But just as with patches, where the magnitudes of partial derivatives are
//  consistent between similar patches, the magnitudes of limit tangents should
//  also be similar.
//
//  The reference tangents, in terms of magnitudes, are those produced by the
//  limit tangent mask for smooth interior vertices, for which well established
//  sin/cos formulae apply -- these remain unscaled.  Formulae for the other
//  crease/boundary, corner tangents and irregular cases are scaled to be more
//  consistent with these.
//
//  The crease/boundary tangents for the regular case can be viewed as derived
//  from the smooth interior masks with two "phantom" points extrapolated across
//  the regular boundary:
//
//            v3           v2          
//             X - - - - - X
//           /   \       /   \
//         /       \   /       \
//   v4  X - - - - - X - - - - - X  v1
//         .       . 0 .       .
//           .   .       .   .
//             .   .   .   .
//           (v5)         (v6)
//
//  where v5 = v0 + (v4 - v3) and v6 = v0 + v1 - v2.
//
//  When the standard limit tangent mask is applied, the cosines of increments
//  of pi/3 give us coefficients that are multiples of 1/2, leading to the first
//  tangent T1 = 3/2 * (v1 - v4), rather than the widely used T1 = v1 - v4.  So
//  this scale factor of 3/2 is applied to ensure tangents along the boundaries
//  are of similar magnitude as tangents in the immediate interior (which may be
//  parallel).
//
//  Tangents at corners are essentially a form of boundary tangent, and so its
//  simple difference formula is scaled to be consistent with adjoining boundary
//  tangents -- not just with the 3/2 factor from above, but with an additional
//  2.0 to compensate for the fact that the difference of only side of the vertex
//  is considered here.  The resulting scale factor of 3.0 for the regular corner
//  is what similarly arises by extrapolating an interior region around the
//  vertex and using the interior mask for the first tangent.
//
//  The cross-tangent formula for the regular crease/boundary is similarly found
//  from the above construction of the boundary, but the commonly used weights of
//  +/- 1 and 2 result from omitting the common factor of sqrt(3)/2 (arising from
//  the sines of increments of pi/3).  With that scale factor close to one, it has
//  less impact than the irregular cases, which are analogous to corner tangents
//  in that differences on only one side of the vertex are considered.  While a
//  scaling of 3.0 is similarly understandable for the valence 2 and 3 cases, it is
//  less obvious in the irregular formula for valence > 4, but similarly effective.
//
//  The end result of these adjustments should be a set of limit tangents that are
//  of similar magnitude over a regular mesh including boundaries and corners.
*/
template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignCornerLimitTangentMasks(VERTEX const& vertex,
        MASK& tan1Mask, MASK& tan2Mask) const {

    int valence = vertex.GetNumEdges();

    tan1Mask.SetNumVertexWeights(1);
    tan1Mask.SetNumEdgeWeights(valence);
    tan1Mask.SetNumFaceWeights(0);
    tan1Mask.SetFaceWeightsForFaceCenters(false);

    tan2Mask.SetNumVertexWeights(1);
    tan2Mask.SetNumEdgeWeights(valence);
    tan2Mask.SetNumFaceWeights(0);
    tan2Mask.SetFaceWeightsForFaceCenters(false);

    //  See note above regarding scale factor of 3.0:
    tan1Mask.VertexWeight(0) = -3.0f;
    tan1Mask.EdgeWeight(0)   =  3.0f;
    tan1Mask.EdgeWeight(1)   =  0.0f;

    tan2Mask.VertexWeight(0) = -3.0f;
    tan2Mask.EdgeWeight(0)   =  0.0f;
    tan2Mask.EdgeWeight(1)   =  3.0f;

    //  Should be at least 2 edges -- be sure to clear weights for any more:
    for (int i = 2; i < valence; ++i) {
        tan1Mask.EdgeWeight(i) = 0.0f;
        tan2Mask.EdgeWeight(i) = 0.0f;
    }
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignCreaseLimitTangentMasks(VERTEX const& vertex,
        MASK& tan1Mask, MASK& tan2Mask, int const creaseEnds[2]) const {

    typedef typename MASK::Weight Weight;

    //
    //  First, the tangent along the crease:
    //      The first crease edge is considered the "leading" edge of the span
    //  of surface for which we are evaluating tangents and the second edge the
    //  "trailing edge".  By convention, the tangent along the crease is oriented
    //  in the direction of the leading edge.
    //
    int valence = vertex.GetNumEdges();

    tan1Mask.SetNumVertexWeights(1);
    tan1Mask.SetNumEdgeWeights(valence);
    tan1Mask.SetNumFaceWeights(0);
    tan1Mask.SetFaceWeightsForFaceCenters(false);

    tan1Mask.VertexWeight(0) = 0.0f;
    for (int i = 0; i < valence; ++i) {
        tan1Mask.EdgeWeight(i) = 0.0f;
    }

    //  See the note above regarding scale factor of 1.5:
    tan1Mask.EdgeWeight(creaseEnds[0]) =  1.5f;
    tan1Mask.EdgeWeight(creaseEnds[1]) = -1.5f;

    //
    //  Second, the tangent across the interior faces:
    //      Note this is ambiguous for an interior vertex.  We currently return
    //  the tangent for the surface in the counter-clockwise span between the
    //  leading and trailing edges that form the crease.  Given the expected
    //  computation of a surface normal as Tan1 X Tan2, this tangent should be
    //  oriented "inward" from the crease/boundary -- across the surface rather
    //  than outward and away from it.
    //
    //  There is inconsistency in the orientation of this tangent in commonly
    //  published results:  the general formula provided for arbitrary valence
    //  has the tangent pointing across the crease and "outward" from the surface,
    //  while the special cases for regular valence and lower have the tangent
    //  pointing across the surface and "inward" from the crease.  So if we are
    //  to consistently orient the first tangent along the crease, regardless of
    //  the interior topology, we have to correct this.  With the first tangent
    //  following the direction of the leading crease edge, we want the second
    //  tangent pointing inward/across the surface -- so we flip the result of
    //  the general formula.
    //
    tan2Mask.SetNumVertexWeights(1);
    tan2Mask.SetNumEdgeWeights(valence);
    tan2Mask.SetNumFaceWeights(0);
    tan2Mask.SetFaceWeightsForFaceCenters(false);

    for (int i = 0; i < creaseEnds[0]; ++i) {
        tan2Mask.EdgeWeight(i) = 0.0f;
    }
    int interiorEdgeCount = creaseEnds[1] - creaseEnds[0] - 1;
    if (interiorEdgeCount == 2) {
        //  See note above regarding scale factor of (sin(60 degs) == sqrt(3)/2:

        static Weight const Root3    = (Weight) 1.73205080756887729352;
        static Weight const Root3by2 = (Weight) (Root3 * 0.5);

        tan2Mask.VertexWeight(0) = -Root3;

        tan2Mask.EdgeWeight(creaseEnds[0]) = -Root3by2;
        tan2Mask.EdgeWeight(creaseEnds[1]) = -Root3by2;

        tan2Mask.EdgeWeight(creaseEnds[0] + 1) = Root3;
        tan2Mask.EdgeWeight(creaseEnds[0] + 2) = Root3;
    } else if (interiorEdgeCount > 2) {
        //  See notes above regarding scale factor of -3.0 (-1 for orientation,
        //  2.0 for considering the region as a half-disk, and 1.5 in keeping
        //  with the crease tangent):

        double theta = kPI / (interiorEdgeCount + 1);

        tan2Mask.VertexWeight(0) = 0.0f;

        Weight cWeight = (Weight) (-3.0f * std::sin(theta));
        tan2Mask.EdgeWeight(creaseEnds[0]) = cWeight;
        tan2Mask.EdgeWeight(creaseEnds[1]) = cWeight;

        double eCoeff  = -3.0f * 2.0f * (std::cos(theta) - 1.0f);
        for (int i = 1; i <= interiorEdgeCount; ++i) {
            tan2Mask.EdgeWeight(creaseEnds[0] + i) = (Weight) (eCoeff * std::sin(i * theta));
        }
    } else if (interiorEdgeCount == 1) {
        //  See notes above regarding scale factor of 3.0:

        tan2Mask.VertexWeight(0) = -3.0f;

        tan2Mask.EdgeWeight(creaseEnds[0]) = 0.0f;
        tan2Mask.EdgeWeight(creaseEnds[1]) = 0.0f;

        tan2Mask.EdgeWeight(creaseEnds[0] + 1) = 3.0f;
    } else {
        //  See notes above regarding scale factor of 3.0:

        tan2Mask.VertexWeight(0) = -6.0f;

        tan2Mask.EdgeWeight(creaseEnds[0]) = 3.0f;
        tan2Mask.EdgeWeight(creaseEnds[1]) = 3.0f;
    }
    for (int i = creaseEnds[1] + 1; i < valence; ++i) {
        tan2Mask.EdgeWeight(i) = 0.0f;
    }
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_LOOP>::assignSmoothLimitTangentMasks(VERTEX const& vertex,
        MASK& tan1Mask, MASK& tan2Mask) const {

    typedef typename MASK::Weight Weight;

    int valence = vertex.GetNumFaces();

    tan1Mask.SetNumVertexWeights(1);
    tan1Mask.SetNumEdgeWeights(valence);
    tan1Mask.SetNumFaceWeights(0);
    tan1Mask.SetFaceWeightsForFaceCenters(false);

    tan2Mask.SetNumVertexWeights(1);
    tan2Mask.SetNumEdgeWeights(valence);
    tan2Mask.SetNumFaceWeights(0);
    tan2Mask.SetFaceWeightsForFaceCenters(false);

    tan1Mask.VertexWeight(0) = 0.0f;
    tan2Mask.VertexWeight(0) = 0.0f;

    if (valence == 6) {
        static Weight const Root3by2 = (Weight)(0.5 * 1.73205080756887729352);

        tan1Mask.EdgeWeight(0) =  1.0f;
        tan1Mask.EdgeWeight(1) =  0.5f;
        tan1Mask.EdgeWeight(2) = -0.5f;
        tan1Mask.EdgeWeight(3) = -1.0f;
        tan1Mask.EdgeWeight(4) = -0.5f;
        tan1Mask.EdgeWeight(5) =  0.5f;

        tan2Mask.EdgeWeight(0) =  0.0f;
        tan2Mask.EdgeWeight(1) =  Root3by2;
        tan2Mask.EdgeWeight(2) =  Root3by2;
        tan2Mask.EdgeWeight(3) =  0.0f;
        tan2Mask.EdgeWeight(4) = -Root3by2;
        tan2Mask.EdgeWeight(5) = -Root3by2;
    } else {
        double alpha = 2.0f * kPI / valence;
        for (int i = 0; i < valence; ++i) {
            double alphaI = alpha * i;
            tan1Mask.EdgeWeight(i) = (Weight) std::cos(alphaI);
            tan2Mask.EdgeWeight(i) = (Weight) std::sin(alphaI);
        }
    }
}

} // end namespace Sdc
} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_SDC_LOOP_SCHEME_H */
