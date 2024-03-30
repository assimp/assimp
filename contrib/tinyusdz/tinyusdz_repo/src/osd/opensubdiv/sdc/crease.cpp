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
#include "../sdc/crease.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Sdc {

//
//  Declarations of creasing constants and non-inline methods:
//
float const Crease::SHARPNESS_SMOOTH   =  0.0f;
float const Crease::SHARPNESS_INFINITE = 10.0f;


//
//  Creasing queries dependent on sharpness values:
//
Crease::Rule
Crease::DetermineVertexVertexRule(float vertexSharpness, int sharpEdgeCount) const {

    if (IsSharp(vertexSharpness)) return Crease::RULE_CORNER;

    return (sharpEdgeCount > 2) ? Crease::RULE_CORNER : (Crease::Rule)(1 << sharpEdgeCount);
}

Crease::Rule
Crease::DetermineVertexVertexRule(float        vertexSharpness,
                                     int          incidentEdgeCount,
                                     float const* incidentEdgeSharpness) const {

    if (IsSharp(vertexSharpness)) return Crease::RULE_CORNER;

    int sharpEdgeCount = 0;
    for (int i = 0; i < incidentEdgeCount; ++i) {
        sharpEdgeCount += IsSharp(incidentEdgeSharpness[i]);
    }
    return (sharpEdgeCount > 2) ? Crease::RULE_CORNER : (Crease::Rule)(1 << sharpEdgeCount);
}

float
Crease::ComputeFractionalWeightAtVertex(float        parentVertexSharpness,
                                           float        childVertexSharpness,
                                           int          incidentEdgeCount,
                                           float const* parentSharpness,
                                           float const* childSharpness) const {

    int   transitionCount = 0;
    float transitionSum   = 0.0f;

    if (IsSharp(parentVertexSharpness) && IsSmooth(childVertexSharpness)) {
        transitionCount = 1;
        transitionSum   = parentVertexSharpness;
    }

    //
    //  We need the child-edge sharpness values for non-simple methods to ensure
    //  that the sharpness went from a non-zero value (potentially greater than
    //  1.0) to zero...
    //
    if (IsUniform() || (childSharpness == 0)) {
        for (int i = 0; i < incidentEdgeCount; ++i) {
            if (IsSharp(parentSharpness[i]) && (parentSharpness[i] <= 1.0f)) {
                transitionSum   += parentSharpness[i];
                transitionCount ++;
            }
        }
    } else {
        for (int i = 0; i < incidentEdgeCount; ++i) {
            if (IsSharp(parentSharpness[i]) && IsSmooth(childSharpness[i])) {
                transitionSum   += parentSharpness[i];
                transitionCount ++;
            }
        }
    }
    if (transitionCount == 0) return 0.0f;
    float fractionalWeight = transitionSum / (float)transitionCount;
    return (fractionalWeight > 1.0f) ? 1.0f : fractionalWeight;
}

//
//  Subdividing edge sharpness values (vertex sharpness is inline):
//
float
Crease::SubdivideEdgeSharpnessAtVertex(float         edgeSharpness,
                                       int           incEdgeCountAtVertex,
                                       float const * incEdgeSharpness) const {

    if (IsUniform() || (incEdgeCountAtVertex < 2)) {
        return decrementSharpness(edgeSharpness);
    }

    if (IsSmooth(edgeSharpness)) return Crease::SHARPNESS_SMOOTH;
    if (IsInfinite(edgeSharpness)) return Crease::SHARPNESS_INFINITE;

    float sharpSum   = 0.0f;
    int   sharpCount = 0;
    for (int i = 0; i < incEdgeCountAtVertex; ++i) {
        if (IsSemiSharp(incEdgeSharpness[i])) {
            sharpCount ++;
            sharpSum += incEdgeSharpness[i];
        }
    }
    if (sharpCount > 1) {
        //  Chaikin rule is 3/4 original sharpness + 1/4 average of the others

        float avgSharpnessAtVertex = (sharpSum - edgeSharpness) / (float)(sharpCount - 1);

        edgeSharpness = (0.75f * edgeSharpness) + (0.25f * avgSharpnessAtVertex);
    }
    edgeSharpness -= 1.0f;
    return IsSharp(edgeSharpness) ? edgeSharpness : Crease::SHARPNESS_SMOOTH;
}

void
Crease::SubdivideEdgeSharpnessesAroundVertex(int          edgeCount,
                                                float const* parentSharpness,
                                                float *      childSharpness) const {

    if (IsUniform() || (edgeCount < 2)) {
        for (int i = 0; i < edgeCount; ++i) {
            childSharpness[i] = decrementSharpness(parentSharpness[i]);
        }
        return;
    }

    //
    //  Chaikin creasing is most efficiently computed for all edges around a vertex at
    //  once as the subdivided value for each creased edge depends on the average of
    //  the other edges around the vertex.  So we can sum up the sharpness around the
    //  vertex once and use that for each edge, rather than iterating around the vertex
    //  for each incident edge.
    //
    if (_options.GetCreasingMethod() == Options::CREASE_CHAIKIN) {
        float sharpSum   = 0.0f;
        int   sharpCount = 0;
        for (int i = 0; i < edgeCount; ++i) {
            if (IsSemiSharp(parentSharpness[i])) {
                sharpCount ++;
                sharpSum += parentSharpness[i];
            }
        }

        //
        //  The smooth case is most common -- specialize for it first:
        //
        if (sharpCount == 0) {
            for (int i = 0; i < edgeCount; ++i) {
                childSharpness[i] = parentSharpness[i];
            }
        } else {
            for (int i = 0; i < edgeCount; ++i) {
                float const& pSharp = parentSharpness[i];
                float&       cSharp = childSharpness[i];

                if (IsSmooth(pSharp)) {
                    cSharp = Crease::SHARPNESS_SMOOTH;
                } else if (IsInfinite(pSharp)) {
                    cSharp = Crease::SHARPNESS_INFINITE;
                } else if (sharpCount == 1) {
                    //  Need special case here anyway to avoid divide by zero below...
                    cSharp = decrementSharpness(pSharp);
                } else {
                    float pOtherAverage = (sharpSum - pSharp) / (float)(sharpCount - 1);

                    //  Chaikin rule is 3/4 original sharpness + 1/4 average of the others
                    cSharp = ((0.75f * pSharp) + (0.25f * pOtherAverage)) - 1.0f;
                    if (IsSmooth(cSharp)) cSharp = Crease::SHARPNESS_SMOOTH;
                }
            }
        }
    }
}

} // end namespace sdc

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
