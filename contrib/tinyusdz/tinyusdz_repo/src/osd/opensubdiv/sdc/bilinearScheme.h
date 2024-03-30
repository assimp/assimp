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
#ifndef OPENSUBDIV3_SDC_BILINEAR_SCHEME_H
#define OPENSUBDIV3_SDC_BILINEAR_SCHEME_H

#include "../version.h"

#include "../sdc/scheme.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Sdc {

//
//  Specializations for Scheme<SCHEME_BILINEAR>:
//

//
//  Bilinear traits:
//
template <>
inline Split Scheme<SCHEME_BILINEAR>::GetTopologicalSplitType() { return SPLIT_TO_QUADS; }

template <>
inline int Scheme<SCHEME_BILINEAR>::GetRegularFaceSize() { return 4; }

template <>
inline int Scheme<SCHEME_BILINEAR>::GetRegularVertexValence() { return 4; }

template <>
inline int Scheme<SCHEME_BILINEAR>::GetLocalNeighborhoodSize() { return 0; }


//
//  Refinement masks:
//
template <>
template <typename EDGE, typename MASK>
void
Scheme<SCHEME_BILINEAR>::ComputeEdgeVertexMask(EDGE const& edge, MASK& mask,
                                                Crease::Rule, Crease::Rule) const {
    //  This should be inline, otherwise trivially replicate it:
    assignCreaseMaskForEdge(edge, mask);
}

template <>
template <typename VERTEX, typename MASK>
void
Scheme<SCHEME_BILINEAR>::ComputeVertexVertexMask(VERTEX const& vertex, MASK& mask,
                                                  Crease::Rule, Crease::Rule) const {
    //  This should be inline, otherwise trivially replicate it:
    assignCornerMaskForVertex(vertex, mask);
}


//
//  Limit masks for position -- the limit position of all vertices is the refined vertex.
//
template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_BILINEAR>::assignCornerLimitMask(VERTEX const& /* vertex */, MASK& posMask) const {

    posMask.SetNumVertexWeights(1);
    posMask.SetNumEdgeWeights(0);
    posMask.SetNumFaceWeights(0);
    posMask.SetFaceWeightsForFaceCenters(false);

    posMask.VertexWeight(0) = 1.0f;
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_BILINEAR>::assignCreaseLimitMask(VERTEX const& vertex, MASK& posMask,
                                               int const /* creaseEnds */[2]) const {

    assignCornerLimitMask(vertex, posMask);
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_BILINEAR>::assignSmoothLimitMask(VERTEX const& vertex, MASK& posMask) const {

    assignCornerLimitMask(vertex, posMask);
}

//
//  Limit masks for tangents -- these are ambiguous around all vertices.  Provide
//  the tangents based on the incident edges of the first face.
//
template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_BILINEAR>::assignCornerLimitTangentMasks(VERTEX const& /* vertex */,
        MASK& tan1Mask, MASK& tan2Mask) const {

    tan1Mask.SetNumVertexWeights(1);
    tan1Mask.SetNumEdgeWeights(2);
    tan1Mask.SetNumFaceWeights(0);
    tan1Mask.SetFaceWeightsForFaceCenters(false);

    tan2Mask.SetNumVertexWeights(1);
    tan2Mask.SetNumEdgeWeights(2);
    tan2Mask.SetNumFaceWeights(0);
    tan2Mask.SetFaceWeightsForFaceCenters(false);

    tan1Mask.VertexWeight(0) = -1.0f;
    tan1Mask.EdgeWeight(0) = 1.0f;
    tan1Mask.EdgeWeight(1) = 0.0f;

    tan2Mask.VertexWeight(0) = -1.0f;
    tan2Mask.EdgeWeight(0) = 0.0f;
    tan2Mask.EdgeWeight(1) = 1.0f;
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_BILINEAR>::assignCreaseLimitTangentMasks(VERTEX const& vertex,
        MASK& tan1Mask, MASK& tan2Mask, int const /* creaseEnds */[2]) const {

    assignCornerLimitTangentMasks(vertex, tan1Mask, tan2Mask);
}

template <>
template <typename VERTEX, typename MASK>
inline void
Scheme<SCHEME_BILINEAR>::assignSmoothLimitTangentMasks(VERTEX const& vertex,
        MASK& tan1Mask, MASK& tan2Mask) const {

    assignCornerLimitTangentMasks(vertex, tan1Mask, tan2Mask);
}

} // end namespace sdc

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_SDC_BILINEAR_SCHEME_H */
