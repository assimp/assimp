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
#ifndef OPENSUBDIV3_SDC_CATMARK_SCHEME_H
#define OPENSUBDIV3_SDC_CATMARK_SCHEME_H

#include "../version.h"

#include "../sdc/scheme.h"

#include <cassert>
#include <cmath>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Sdc {

//
//  Specializations for Scheme<SCHEME_CATMARK>:
//

//
//  Catmark traits:
//
template <>
inline Split Scheme<SCHEME_CATMARK>::GetTopologicalSplitType() { return SPLIT_TO_QUADS; }

template <>
inline int Scheme<SCHEME_CATMARK>::GetRegularFaceSize() { return 4; }

template <>
inline int Scheme<SCHEME_CATMARK>::GetRegularVertexValence() { return 4; }

template <>
inline int Scheme<SCHEME_CATMARK>::GetLocalNeighborhoodSize() { return 1; }


//
//  Masks for edge-vertices:  the hard Crease mask does not need to be specialized
//  (simply the midpoint), so all that is left is the Smooth case:
//
//  The Smooth mask is complicated by the need to support the "triangle subdivision"
//  option, which applies different weighting in the presence of triangles.  It is
//  up for debate as to whether this is useful or not -- we may be able to deprecate
//  this option.
//
template <>
template <typename EDGE, typename MASK>
inline void
Scheme<SCHEME_CATMARK>::assignSmoothMaskForEdge(EDGE const& edge, MASK& mask) const {

    typedef typename MASK::Weight Weight;

    int faceCount = edge.GetNumFaces();

    mask.SetNumVertexWeights(2);
    mask.SetNumEdgeWeights(0);
    mask.SetNumFaceWeights(faceCount);
    mask.SetFaceWeightsForFaceCenters(true);

    //
    //  Determine if we need to inspect incident faces and apply alternate weighting for
    //  triangles -- and if so, determine which of the two are triangles.
    //
    bool face0IsTri = false;
    bool face1IsTri = false;
    bool useTriangleOption = (_options.GetTriangleSubdivision() == Options::TRI_SUB_SMOOTH);
    if (useTriangleOption) {
        if (faceCount == 2) {
            //
            //  Ideally we want to avoid this inspection when we have already subdivided at
            //  least once -- need something in the Edge interface to help avoid this, e.g.
            //  an IsRegular() query, the subdivision level...
            //
            int vertsPerFace[2];
            edge.GetNumVerticesPerFace(vertsPerFace);

            face0IsTri = (vertsPerFace[0] == 3);
            face1IsTri = (vertsPerFace[1] == 3);
            useTriangleOption = face0IsTri || face1IsTri;
        } else {
            useTriangleOption = false;
        }
    }

    if (! useTriangleOption) {
        mask.VertexWeight(0) = 0.25f;
        mask.VertexWeight(1) = 0.25f;

        if (faceCount == 2) {
            mask.FaceWeight(0) = 0.25f;
            mask.FaceWeight(1) = 0.25f;
        } else {
            Weight fWeight = 0.5f / (Weight)faceCount;
            for (int i = 0; i < faceCount; ++i) {
                mask.FaceWeight(i) = fWeight;
            }
        }
    } else {
        //
        //  This mimics the implementation in Hbr in terms of order of operations.
        //
        const Weight CATMARK_SMOOTH_TRI_EDGE_WEIGHT = (Weight) 0.470;

        Weight f0Weight = face0IsTri ? CATMARK_SMOOTH_TRI_EDGE_WEIGHT : 0.25f;
        Weight f1Weight = face1IsTri ? CATMARK_SMOOTH_TRI_EDGE_WEIGHT : 0.25f;

        Weight fWeight = 0.5f * (f0Weight + f1Weight);
        Weight vWeight = 0.5f * (1.0f - 2.0f * fWeight);

        mask.VertexWeight(0) = vWeight;
        mask.VertexWeight(1) = vWeight;

        mask.FaceWeight(0) = fWeight;
        mask.FaceWeight(1) = fWeight;
    }
}


//
//  Masks for vertex-vertices:  the hard Corner mask does not need to be specialized
//  (simply the vertex itself), leaving the Crease and Smooth cases (Dart is smooth):
//
template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_CATMARK>::assignCreaseMaskForVertex(VERTEX const& vertex, MASK& mask,
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
Scheme<SCHEME_CATMARK>::assignSmoothMaskForVertex(VERTEX const& vertex, MASK& mask) const {

    typedef typename MASK::Weight Weight;

    //
    //  A Smooth vertex must be manifold and interior -- manifold boundary vertices will be
    //  Creases and non-manifold vertices of any kind will be Corners or Creases.  If smooth
    //  rules for non-manifold vertices are ever defined, this will need adjusting:
    //
    assert(vertex.GetNumFaces() == vertex.GetNumEdges());

    int valence = vertex.GetNumFaces();

    mask.SetNumVertexWeights(1);
    mask.SetNumEdgeWeights(valence);
    mask.SetNumFaceWeights(valence);
    mask.SetFaceWeightsForFaceCenters(true);

    Weight vWeight = (Weight)(valence - 2) / (Weight)valence;
    Weight fWeight = 1.0f / (Weight)(valence * valence);
    Weight eWeight = fWeight;

    mask.VertexWeight(0) = vWeight;
    for (int i = 0; i < valence; ++i) {
        mask.EdgeWeight(i) = eWeight;
        mask.FaceWeight(i) = fWeight;
    }
}

//
//  Limit masks for position:
//
template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_CATMARK>::assignCornerLimitMask(VERTEX const& /* vertex */, MASK& posMask) const {

    posMask.SetNumVertexWeights(1);
    posMask.SetNumEdgeWeights(0);
    posMask.SetNumFaceWeights(0);
    posMask.SetFaceWeightsForFaceCenters(false);

    posMask.VertexWeight(0) = 1.0f;
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_CATMARK>::assignCreaseLimitMask(VERTEX const& vertex, MASK& posMask,
                                              int const creaseEnds[2]) const {

    typedef typename MASK::Weight Weight;

    int valence = vertex.GetNumEdges();

    posMask.SetNumVertexWeights(1);
    posMask.SetNumEdgeWeights(valence);
    posMask.SetNumFaceWeights(0);
    posMask.SetFaceWeightsForFaceCenters(false);

    Weight vWeight = (Weight)(2.0 / 3.0);
    Weight eWeight = (Weight)(1.0 / 6.0);

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
Scheme<SCHEME_CATMARK>::assignSmoothLimitMask(VERTEX const& vertex, MASK& posMask) const {

    typedef typename MASK::Weight Weight;

    int valence = vertex.GetNumFaces();
    if (valence == 2) {
        assignCornerLimitMask(vertex, posMask);
        return;
    }

    posMask.SetNumVertexWeights(1);
    posMask.SetNumEdgeWeights(valence);
    posMask.SetNumFaceWeights(valence);
    posMask.SetFaceWeightsForFaceCenters(false);

    //  Specialize for the regular case:
    if (valence == 4) {
        Weight fWeight = (Weight)(1.0 / 36.0);
        Weight eWeight = (Weight)(1.0 /  9.0);
        Weight vWeight = (Weight)(4.0 /  9.0);

        posMask.VertexWeight(0) = vWeight;

        posMask.EdgeWeight(0) = eWeight;
        posMask.EdgeWeight(1) = eWeight;
        posMask.EdgeWeight(2) = eWeight;
        posMask.EdgeWeight(3) = eWeight;

        posMask.FaceWeight(0) = fWeight;
        posMask.FaceWeight(1) = fWeight;
        posMask.FaceWeight(2) = fWeight;
        posMask.FaceWeight(3) = fWeight;
    } else {
        Weight Valence = (Weight) valence;

        Weight fWeight = 1.0f / (Valence * (Valence + 5.0f));
        Weight eWeight = 4.0f * fWeight;
        Weight vWeight = 1.0f - Valence * (eWeight + fWeight);

        posMask.VertexWeight(0) = vWeight;
        for (int i = 0; i < valence; ++i) {
            posMask.EdgeWeight(i) = eWeight;
            posMask.FaceWeight(i) = fWeight;
        }
    }
}

//
//  Limit masks for tangents -- these are stubs for now, or have a temporary
//  implementation
//
template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_CATMARK>::assignCornerLimitTangentMasks(VERTEX const& vertex,
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

    //  Should be at least 2 edges -- be sure to clear weights for any more:
    tan1Mask.VertexWeight(0) = -1.0f;
    tan1Mask.EdgeWeight(0)   =  1.0f;
    tan1Mask.EdgeWeight(1)   =  0.0f;

    tan2Mask.VertexWeight(0) = -1.0f;
    tan2Mask.EdgeWeight(0)   =  0.0f;
    tan2Mask.EdgeWeight(1)   =  1.0f;

    for (int i = 2; i < valence; ++i) {
        tan1Mask.EdgeWeight(i) = 0.0f;
        tan2Mask.EdgeWeight(i) = 0.0f;
    }
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_CATMARK>::assignCreaseLimitTangentMasks(VERTEX const& vertex,
        MASK& tan1Mask, MASK& tan2Mask, int const creaseEnds[2]) const {

    constexpr auto kPI = 3.14159265358979323846;

    typedef typename MASK::Weight Weight;

    //
    //  First, the tangent along the crease:
    //      The first crease edge is considered the "leading" edge of the span
    //  of surface for which we are evaluating tangents and the second edge the
    //  "trailing edge".  By convention, the tangent along the crease is oriented
    //  in the direction of the leading edge.
    //
    int numEdges = vertex.GetNumEdges();
    int numFaces = vertex.GetNumFaces();

    tan1Mask.SetNumVertexWeights(1);
    tan1Mask.SetNumEdgeWeights(numEdges);
    tan1Mask.SetNumFaceWeights(numFaces);
    tan1Mask.SetFaceWeightsForFaceCenters(false);

    tan1Mask.VertexWeight(0) = 0.0f;
    for (int i = 0; i < numEdges; ++i) {
        tan1Mask.EdgeWeight(i) = 0.0f;
    }
    for (int i = 0; i < numFaces; ++i) {
        tan1Mask.FaceWeight(i) = 0.0f;
    }

    tan1Mask.EdgeWeight(creaseEnds[0]) =  0.5f;
    tan1Mask.EdgeWeight(creaseEnds[1]) = -0.5f;

    //
    //  Second, the tangent across the interior faces:
    //      Note this is ambiguous for an interior vertex.  We currently return
    //  the tangent for the surface in the counter-clockwise span between the
    //  leading and trailing edges that form the crease.  Given the expected
    //  computation of a surface normal as Tan1 X Tan2, this tangent should be
    //  oriented "inward" from the crease/boundary -- across the surface rather
    //  than outward and away from it.
    //
    tan2Mask.SetNumVertexWeights(1);
    tan2Mask.SetNumEdgeWeights(numEdges);
    tan2Mask.SetNumFaceWeights(numFaces);
    tan2Mask.SetFaceWeightsForFaceCenters(false);

    //  Prepend weights of 0 preceding the crease:
    for (int i = 0; i < creaseEnds[0]; ++i) {
        tan2Mask.EdgeWeight(i) = 0.0f;
        tan2Mask.FaceWeight(i) = 0.0f;
    }

    //  Assign weights to crease edge and interior points:
    int interiorEdgeCount = creaseEnds[1] - creaseEnds[0] - 1;
    if (interiorEdgeCount == 1) {
        //  The regular case -- uniform B-spline cross-tangent:

        tan2Mask.VertexWeight(0) = (Weight)(-4.0 / 6.0);

        tan2Mask.EdgeWeight(creaseEnds[0])     = (Weight)(-1.0 / 6.0);
        tan2Mask.EdgeWeight(creaseEnds[0] + 1) = (Weight)( 4.0 / 6.0);
        tan2Mask.EdgeWeight(creaseEnds[1])     = (Weight)(-1.0 / 6.0);

        tan2Mask.FaceWeight(creaseEnds[0])     = (Weight)(1.0 / 6.0);
        tan2Mask.FaceWeight(creaseEnds[0] + 1) = (Weight)(1.0 / 6.0);
    } else if (interiorEdgeCount > 1) {
        //  The irregular case -- formulae from Biermann et al:

        double k     = (double) (interiorEdgeCount + 1);
        double theta = kPI / k;

        double cosTheta = std::cos(theta);
        double sinTheta = std::sin(theta);

        //  Loop/Schaefer use a different divisor here (3*k + cos(theta)):
        double commonDenom = 1.0f / (k * (3.0f + cosTheta));
        double R = (cosTheta + 1.0f) / sinTheta;

        double vertexWeight = 4.0f * R * (cosTheta - 1.0f);
        double creaseWeight = -R * (1.0f + 2.0f * cosTheta);

        tan2Mask.VertexWeight(0) = (Weight) (vertexWeight * commonDenom);

        tan2Mask.EdgeWeight(creaseEnds[0]) = (Weight) (creaseWeight * commonDenom);
        tan2Mask.EdgeWeight(creaseEnds[1]) = (Weight) (creaseWeight * commonDenom);

        tan2Mask.FaceWeight(creaseEnds[0]) = (Weight) (sinTheta * commonDenom);

        double sinThetaI      = 0.0f;
        double sinThetaIplus1 = sinTheta;
        for (int i = 1; i < k; ++i) {
            sinThetaI      = sinThetaIplus1;
            sinThetaIplus1 = std::sin((i+1)*theta);

            tan2Mask.EdgeWeight(creaseEnds[0] + i) = (Weight) ((4.0f * sinThetaI) * commonDenom);
            tan2Mask.FaceWeight(creaseEnds[0] + i) = (Weight) ((sinThetaI + sinThetaIplus1) * commonDenom);
        }
    } else {
        //  Special case for a single face -- simple average of boundary edges:

        tan2Mask.VertexWeight(0) = -6.0f;

        tan2Mask.EdgeWeight(creaseEnds[0]) = 3.0f;
        tan2Mask.EdgeWeight(creaseEnds[1]) = 3.0f;

        tan2Mask.FaceWeight(creaseEnds[0]) = 0.0f;
    }

    //  Append weights of 0 following the crease:
    for (int i = creaseEnds[1]; i < numFaces; ++i) {
        tan2Mask.FaceWeight(i) = 0.0f;
    }
    for (int i = creaseEnds[1] + 1; i < numEdges; ++i) {
        tan2Mask.EdgeWeight(i) = 0.0f;
    }
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_CATMARK>::assignSmoothLimitTangentMasks(VERTEX const& vertex,
        MASK& tan1Mask, MASK& tan2Mask) const {

    constexpr auto kPI = 3.14159265358979323846;
    typedef typename MASK::Weight Weight;

    int valence = vertex.GetNumFaces();
    if (valence == 2) {
        assignCornerLimitTangentMasks(vertex, tan1Mask, tan2Mask);
        return;
    }

    //  Compute tan1 initially -- tan2 is simply a rotation:
    tan1Mask.SetNumVertexWeights(1);
    tan1Mask.SetNumEdgeWeights(valence);
    tan1Mask.SetNumFaceWeights(valence);
    tan1Mask.SetFaceWeightsForFaceCenters(false);

    tan1Mask.VertexWeight(0) = 0.0f;

    if (valence == 4) {
        tan1Mask.EdgeWeight(0) =  4.0f;
        tan1Mask.EdgeWeight(1) =  0.0f;
        tan1Mask.EdgeWeight(2) = -4.0f;
        tan1Mask.EdgeWeight(3) =  0.0f;

        tan1Mask.FaceWeight(0) =  1.0f;
        tan1Mask.FaceWeight(1) = -1.0f;
        tan1Mask.FaceWeight(2) = -1.0f;
        tan1Mask.FaceWeight(3) =  1.0f;
    } else {
        double theta = 2.0f * kPI / (double)valence;

        double cosTheta     = std::cos(theta);
        double cosHalfTheta = std::cos(theta * 0.5f);

        double lambda = (5.0 / 16.0) + (1.0 / 16.0) *
                (cosTheta + cosHalfTheta * std::sqrt(2.0f * (9.0f + cosTheta)));

        double edgeWeightScale = 4.0f;
        double faceWeightScale = 1.0f / (4.0f * lambda - 1.0f);

        for (int i = 0; i < valence; ++i) {
            double cosThetaI      = std::cos(  i  * theta);
            double cosThetaIplus1 = std::cos((i+1)* theta);

            tan1Mask.EdgeWeight(i) = (Weight) (edgeWeightScale * cosThetaI);
            tan1Mask.FaceWeight(i) = (Weight) (faceWeightScale * (cosThetaI + cosThetaIplus1));
        }
    }

    //  Now rotate/copy tan1 weights to tan2:
    tan2Mask.SetNumVertexWeights(1);
    tan2Mask.SetNumEdgeWeights(valence);
    tan2Mask.SetNumFaceWeights(valence);
    tan2Mask.SetFaceWeightsForFaceCenters(false);

    tan2Mask.VertexWeight(0) = 0.0f;
    if (valence == 4) {
        tan2Mask.EdgeWeight(0) =  0.0f;
        tan2Mask.EdgeWeight(1) =  4.0f;
        tan2Mask.EdgeWeight(2) =  0.0f;
        tan2Mask.EdgeWeight(3) = -4.0f;

        tan2Mask.FaceWeight(0) =  1.0f;
        tan2Mask.FaceWeight(1) =  1.0f;
        tan2Mask.FaceWeight(2) = -1.0f;
        tan2Mask.FaceWeight(3) = -1.0f;
    } else {
        tan2Mask.EdgeWeight(0) = tan1Mask.EdgeWeight(valence-1);
        tan2Mask.FaceWeight(0) = tan1Mask.FaceWeight(valence-1);
        for (int i = 1; i < valence; ++i) {
            tan2Mask.EdgeWeight(i) = tan1Mask.EdgeWeight(i-1);
            tan2Mask.FaceWeight(i) = tan1Mask.FaceWeight(i-1);
        }
    }
}

} // end namespace sdc

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_SDC_CATMARK_SCHEME_H */
