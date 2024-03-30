#line 0 "osd/mtlPatchCommonTess.metal"

//
//   Copyright 2015-2019 Pixar
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

// ----------------------------------------------------------------------------
// Tessellation
// ----------------------------------------------------------------------------

// For now, fractional spacing is supported only with screen space tessellation
#ifndef OSD_ENABLE_SCREENSPACE_TESSELLATION
#undef OSD_FRACTIONAL_EVEN_SPACING
#undef OSD_FRACTIONAL_ODD_SPACING
#endif

//
// Organization of B-spline and Bezier control points.
//
// Each patch is defined by 16 control points (labeled 0-15).
//
// The patch will be evaluated across the domain from (0,0) at
// the lower-left to (1,1) at the upper-right. When computing
// adaptive tessellation metrics, we consider refined vertex-vertex
// and edge-vertex points along the transition edges of the patch
// (labeled vv* and ev* respectively).
//
// The two segments of each transition edge are labeled Lo and Hi,
// with the Lo segment occurring before the Hi segment along the
// transition edge's domain parameterization. These Lo and Hi segment
// tessellation levels determine how domain evaluation coordinates
// are remapped along transition edges. The Hi segment value will
// be zero for a non-transition edge.
//
// (0,1)                                         (1,1)
//
//   vv3                  ev23                   vv2
//        |       Lo3       |       Hi3       |
//      --O-----------O-----+-----O-----------O--
//        | 12        | 13     14 |        15 |
//        |           |           |           |
//        |           |           |           |
//    Hi0 |           |           |           | Hi2
//        |           |           |           |
//        O-----------O-----------O-----------O
//        | 8         | 9      10 |        11 |
//        |           |           |           |
// ev03 --+           |           |           +-- ev12
//        |           |           |           |
//        | 4         | 5       6 |         7 |
//        O-----------O-----------O-----------O
//        |           |           |           |
//    Lo0 |           |           |           | Lo2
//        |           |           |           |
//        |           |           |           |
//        | 0         | 1       2 |         3 |
//      --O-----------O-----+-----O-----------O--
//        |       Lo1       |       Hi1       |
//   vv0                  ev01                   vv1
//
// (0,0)                                         (1,0)
//

float OsdComputePostProjectionSphereExtent(
        const float4x4 OsdProjectionMatrix, float3 center, float diameter)
{
    float4 p = OsdProjectionMatrix * float4(center, 1.0);
    return abs(diameter * OsdProjectionMatrix[1][1] / p.w);
}

float OsdComputeTessLevel(
        const float OsdTessLevel,
        const float4x4 OsdProjectionMatrix, const float4x4 OsdModelViewMatrix,
        float3 p0, float3 p1)
{
    // Adaptive factor can be any computation that depends only on arg values.
    // Project the diameter of the edge's bounding sphere instead of using the
    // length of the projected edge itself to avoid problems near silhouettes.
    p0 = (OsdModelViewMatrix * float4(p0, 1.0)).xyz;
    p1 = (OsdModelViewMatrix * float4(p1, 1.0)).xyz;
    float3 center = (p0 + p1) / 2.0;
    float diameter = distance(p0, p1);
    float projLength = OsdComputePostProjectionSphereExtent(OsdProjectionMatrix, center, diameter);
    float tessLevel = max(1.0, OsdTessLevel * projLength);

    // We restrict adaptive tessellation levels to half of the device
    // supported maximum because transition edges are split into two
    // halves and the sum of the two corresponding levels must not exceed
    // the device maximum. We impose this limit even for non-transition
    // edges because a non-transition edge must be able to match up with
    // one half of the transition edge of an adjacent transition patch.
    return min(tessLevel, (float)(OSD_MAX_TESS_LEVEL / 2));
}

void
OsdGetTessLevelsUniform(const float OsdTessLevel, int3 patchParam,
                        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    // Uniform factors are simple powers of two for each level.
    // The maximum here can be increased if we know the maximum
    // refinement level of the mesh:
    //     min(OSD_MAX_TESS_LEVEL, pow(2, MaximumRefinementLevel-1)
    int refinementLevel = OsdGetPatchRefinementLevel(patchParam);
    float tessLevel = min(OsdTessLevel, (float)OSD_MAX_TESS_LEVEL) /
                        pow(2, refinementLevel - 1.0f);

    // tessLevels of transition edge should be clamped to 2.
    int transitionMask = OsdGetPatchTransitionMask(patchParam);
    float4 tessLevelMin = float4(1) + float4(((transitionMask & 8) >> 3),
                                             ((transitionMask & 1) >> 0),
                                             ((transitionMask & 2) >> 1),
                                             ((transitionMask & 4) >> 2));

    tessOuterLo = max(float4(tessLevel), tessLevelMin);
    tessOuterHi = float4(0);
}

void
OsdGetTessLevelsUniformTriangle(const float OsdTessLevel, int3 patchParam,
                        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    // Uniform factors are simple powers of two for each level.
    // The maximum here can be increased if we know the maximum
    // refinement level of the mesh:
    //     min(OSD_MAX_TESS_LEVEL, pow(2, MaximumRefinementLevel-1)
    int refinementLevel = OsdGetPatchRefinementLevel(patchParam);
    float tessLevel = min(OsdTessLevel, (float)OSD_MAX_TESS_LEVEL) /
                        pow(2, refinementLevel - 1.0f);

    // tessLevels of transition edge should be clamped to 2.
    int transitionMask = OsdGetPatchTransitionMask(patchParam);
    float4 tessLevelMin = float4(1) + float4(((transitionMask & 4) >> 2),
                                             ((transitionMask & 1) >> 0),
                                             ((transitionMask & 2) >> 1),
                                             0);

    tessOuterLo = max(float4(tessLevel), tessLevelMin);
    tessOuterHi = float4(0);
}

void
OsdGetTessLevelsRefinedPoints(
        const float OsdTessLevel,
        const float4x4 OsdProjectionMatrix, const float4x4 OsdModelViewMatrix,
        float3 cp[16], int3 patchParam,
        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    // Each edge of a transition patch is adjacent to one or two patches
    // at the next refined level of subdivision. We compute the corresponding
    // vertex-vertex and edge-vertex refined points along the edges of the
    // patch using Catmull-Clark subdivision stencil weights.
    // For simplicity, we let the optimizer discard unused computation.

    float3 vv0 = (cp[0] + cp[2] + cp[8] + cp[10]) * 0.015625 +
                 (cp[1] + cp[4] + cp[6] + cp[9]) * 0.09375 + cp[5] * 0.5625;
    float3 ev01 = (cp[1] + cp[2] + cp[9] + cp[10]) * 0.0625 +
                  (cp[5] + cp[6]) * 0.375;

    float3 vv1 = (cp[1] + cp[3] + cp[9] + cp[11]) * 0.015625 +
                 (cp[2] + cp[5] + cp[7] + cp[10]) * 0.09375 + cp[6] * 0.5625;
    float3 ev12 = (cp[5] + cp[7] + cp[9] + cp[11]) * 0.0625 +
                  (cp[6] + cp[10]) * 0.375;

    float3 vv2 = (cp[5] + cp[7] + cp[13] + cp[15]) * 0.015625 +
                 (cp[6] + cp[9] + cp[11] + cp[14]) * 0.09375 + cp[10] * 0.5625;
    float3 ev23 = (cp[5] + cp[6] + cp[13] + cp[14]) * 0.0625 +
                  (cp[9] + cp[10]) * 0.375;

    float3 vv3 = (cp[4] + cp[6] + cp[12] + cp[14]) * 0.015625 +
                 (cp[5] + cp[8] + cp[10] + cp[13]) * 0.09375 + cp[9] * 0.5625;
    float3 ev03 = (cp[4] + cp[6] + cp[8] + cp[10]) * 0.0625 +
                  (cp[5] + cp[9]) * 0.375;

    tessOuterLo = float4(0);
    tessOuterHi = float4(0);

    int transitionMask = OsdGetPatchTransitionMask(patchParam);

    if ((transitionMask & 8) != 0) {
        tessOuterLo[0] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, vv0, ev03);
        tessOuterHi[0] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, vv3, ev03);
    } else {
        tessOuterLo[0] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, cp[5], cp[9]);
    }
    if ((transitionMask & 1) != 0) {
        tessOuterLo[1] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, vv0, ev01);
        tessOuterHi[1] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, vv1, ev01);
    } else {
        tessOuterLo[1] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, cp[5], cp[6]);
    }
    if ((transitionMask & 2) != 0) {
        tessOuterLo[2] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, vv1, ev12);
        tessOuterHi[2] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, vv2, ev12);
    } else {
        tessOuterLo[2] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, cp[6], cp[10]);
    }
    if ((transitionMask & 4) != 0) {
        tessOuterLo[3] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, vv3, ev23);
        tessOuterHi[3] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, vv2, ev23);
    } else {
        tessOuterLo[3] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, cp[9], cp[10]);
    }
}

//
//  Patch boundary corners are ordered counter-clockwise from the first
//  corner while patch boundary edges and their midpoints are similarly
//  ordered counter-clockwise beginning at the edge preceding corner[0].
//
void
Osd_GetTessLevelsFromPatchBoundaries4(
        const float OsdTessLevel,
        const float4x4 OsdProjectionMatrix,
        const float4x4 OsdModelViewMatrix,
        float3 corners[3], float3 midpoints[3],
        int3 patchParam,
        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    tessOuterLo = float4(0);
    tessOuterHi = float4(0);

    int transitionMask = OsdGetPatchTransitionMask(patchParam);

    if ((transitionMask & 8) != 0) {
        tessOuterLo[0] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[0], midpoints[0]);
        tessOuterHi[0] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[3], midpoints[0]);
    } else {
        tessOuterLo[0] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[0], corners[3]);
    }
    if ((transitionMask & 1) != 0) {
        tessOuterLo[1] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[0], midpoints[1]);
        tessOuterHi[1] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[1], midpoints[1]);
    } else {
        tessOuterLo[1] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[0], corners[1]);
    }
    if ((transitionMask & 2) != 0) {
        tessOuterLo[2] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[1], midpoints[2]);
        tessOuterHi[2] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[2], midpoints[2]);
    } else {
        tessOuterLo[2] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[1], corners[2]);
    }
    if ((transitionMask & 4) != 0) {
        tessOuterLo[3] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[3], midpoints[3]);
        tessOuterHi[3] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[2], midpoints[3]);
    } else {
        tessOuterLo[3] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[3], corners[2]);
    }
}

void
Osd_GetTessLevelsFromPatchBoundaries3(
        const float OsdTessLevel,
        const float4x4 OsdProjectionMatrix,
        const float4x4 OsdModelViewMatrix,
        float3 corners[3], float3 midpoints[3],
        int3 patchParam,
        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    tessOuterLo = float4(0);
    tessOuterHi = float4(0);

    int transitionMask = OsdGetPatchTransitionMask(patchParam);

    if ((transitionMask & 4) != 0) {
        tessOuterLo[0] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[0], midpoints[0]);
        tessOuterHi[0] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[2], midpoints[0]);
    } else {
        tessOuterLo[0] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[0], corners[2]);
    }
    if ((transitionMask & 1) != 0) {
        tessOuterLo[1] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[0], midpoints[1]);
        tessOuterHi[1] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[1], midpoints[1]);
    } else {
        tessOuterLo[1] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[0], corners[1]);
    }
    if ((transitionMask & 2) != 0) {
        tessOuterLo[2] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[2], midpoints[2]);
        tessOuterHi[2] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[1], midpoints[2]);
    } else {
        tessOuterLo[2] = OsdComputeTessLevel(
            OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
            corners[1], corners[2]);
    }
}

float3
Osd_EvalBezierCurveMidPoint(float3 p0, float3 p1, float3 p2, float3 p3)
{
    //  Coefficients for the midpoint are { 1/8, 3/8, 3/8, 1/8 }:
    return 0.125 * (p0 + p3) + 0.375 * (p1 + p2);
}

float3
Osd_EvalQuarticBezierCurveMidPoint(float3 p0, float3 p1, float3 p2, float3 p3, float3 p4)
{
    //  Coefficients for the midpoint are { 1/16, 1/4, 3/8, 1/4, 1/16 }:
    return 0.0625 * (p0 + p4) + 0.25 * (p1 + p3) + 0.375 * p2;
}

void
OsdEvalPatchBezierTessLevels(
        const float OsdTessLevel,
        const float4x4 OsdProjectionMatrix,
        const float4x4 OsdModelViewMatrix,
        device OsdPerPatchVertexBezier* cpBezier,
        int3 patchParam,
        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    // Each edge of a transition patch is adjacent to one or two patches
    // at the next refined level of subdivision. When the patch control
    // points have been converted to the Bezier basis, the control points
    // at the four corners are on the limit surface (since a Bezier patch
    // interpolates its corner control points). We can compute an adaptive
    // tessellation level for transition edges on the limit surface by
    // evaluating a limit position at the mid point of each transition edge.

    tessOuterLo = float4(0);
    tessOuterHi = float4(0);

    float3 corners[4];
    float3 midpoints[4];

    int transitionMask = OsdGetPatchTransitionMask(patchParam);

#if OSD_PATCH_ENABLE_SINGLE_CREASE
    corners[0] = OsdEvalBezier(cpBezier, patchParam, float2(0.0, 0.0));
    corners[1] = OsdEvalBezier(cpBezier, patchParam, float2(1.0, 0.0));
    corners[2] = OsdEvalBezier(cpBezier, patchParam, float2(1.0, 1.0));
    corners[3] = OsdEvalBezier(cpBezier, patchParam, float2(0.0, 1.0));

    midpoints[0] = ((transitionMask & 8) == 0) ? float3(0) :
        OsdEvalBezier(cpBezier, patchParam, float2(0.0, 0.5));
    midpoints[1] = ((transitionMask & 1) == 0) ? float3(0) :
        OsdEvalBezier(cpBezier, patchParam, float2(0.5, 0.0));
    midpoints[2] = ((transitionMask & 2) == 0) ? float3(0) :
        OsdEvalBezier(cpBezier, patchParam, float2(1.0, 0.5));
    midpoints[3] = ((transitionMask & 4) == 0) ? float3(0) :
        OsdEvalBezier(cpBezier, patchParam, float2(0.5, 1.0));

#else // OSD_PATCH_ENABLE_SINGLE_CREASE
    corners[0] = cpBezier[ 0].P;
    corners[1] = cpBezier[ 3].P;
    corners[2] = cpBezier[15].P;
    corners[3] = cpBezier[12].P;

    midpoints[0] = ((transitionMask & 8) == 0) ? float3(0) :
        Osd_EvalBezierCurveMidPoint(
            cpBezier[0].P, cpBezier[4].P, cpBezier[8].P, cpBezier[12].P);
    midpoints[1] = ((transitionMask & 1) == 0) ? float3(0) :
        Osd_EvalBezierCurveMidPoint(
            cpBezier[0].P, cpBezier[1].P, cpBezier[2].P, cpBezier[3].P);
    midpoints[2] = ((transitionMask & 2) == 0) ? float3(0) :
        Osd_EvalBezierCurveMidPoint(
            cpBezier[3].P, cpBezier[7].P, cpBezier[11].P, cpBezier[15].P);
    midpoints[3] = ((transitionMask & 4) == 0) ? float3(0) :
        Osd_EvalBezierCurveMidPoint(
            cpBezier[12].P, cpBezier[13].P, cpBezier[14].P, cpBezier[15].P);
#endif

    Osd_GetTessLevelsFromPatchBoundaries4(
        OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
        corners, midpoints, patchParam, tessOuterLo, tessOuterHi);
}

void
OsdEvalPatchBezierTessLevels(
        const float OsdTessLevel,
        const float4x4 OsdProjectionMatrix,
        const float4x4 OsdModelViewMatrix,
        float3 cv[16],
        int3 patchParam,
        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    // Each edge of a transition patch is adjacent to one or two patches
    // at the next refined level of subdivision. When the patch control
    // points have been converted to the Bezier basis, the control points
    // at the four corners are on the limit surface (since a Bezier patch
    // interpolates its corner control points). We can compute an adaptive
    // tessellation level for transition edges on the limit surface by
    // evaluating a limit position at the mid point of each transition edge.

    tessOuterLo = float4(0);
    tessOuterHi = float4(0);

    float3 corners[4];
    float3 midpoints[4];

    int transitionMask = OsdGetPatchTransitionMask(patchParam);

    corners[0] = cv[ 0];
    corners[1] = cv[ 3];
    corners[2] = cv[15];
    corners[3] = cv[12];

    midpoints[0] = ((transitionMask & 8) == 0) ? float3(0) :
        Osd_EvalBezierCurveMidPoint(cv[0], cv[4], cv[8], cv[12]);
    midpoints[1] = ((transitionMask & 1) == 0) ? float3(0) :
        Osd_EvalBezierCurveMidPoint(cv[0], cv[1], cv[2], cv[3]);
    midpoints[2] = ((transitionMask & 2) == 0) ? float3(0) :
        Osd_EvalBezierCurveMidPoint(cv[3], cv[7], cv[11], cv[15]);
    midpoints[3] = ((transitionMask & 4) == 0) ? float3(0) :
        Osd_EvalBezierCurveMidPoint(cv[12], cv[13], cv[14], cv[15]);

    Osd_GetTessLevelsFromPatchBoundaries4(
        OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
        corners, midpoints, patchParam, tessOuterLo, tessOuterHi);
}

void
OsdEvalPatchBezierTriangleTessLevels(
        const float OsdTessLevel,
        const float4x4 OsdProjectionMatrix,
        const float4x4 OsdModelViewMatrix,
        float3 cv[15],
        int3 patchParam,
        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    // Each edge of a transition patch is adjacent to one or two patches
    // at the next refined level of subdivision. When the patch control
    // points have been converted to the Bezier basis, the control points
    // at the corners are on the limit surface (since a Bezier patch
    // interpolates its corner control points). We can compute an adaptive
    // tessellation level for transition edges on the limit surface by
    // evaluating a limit position at the mid point of each transition edge.

    tessOuterLo = float4(0);
    tessOuterHi = float4(0);

    int transitionMask = OsdGetPatchTransitionMask(patchParam);

    float3 corners[3];
    corners[0] = cv[0];
    corners[1] = cv[4];
    corners[2] = cv[14];

    float3 midpoints[3];
    midpoints[0] = ((transitionMask & 4) == 0) ? float3(0) :
        Osd_EvalQuarticBezierCurveMidPoint(cv[0], cv[5], cv[9], cv[12], cv[14]);
    midpoints[1] = ((transitionMask & 1) == 0) ? float3(0) :
        Osd_EvalQuarticBezierCurveMidPoint(cv[0], cv[1], cv[2], cv[3], cv[4]);
    midpoints[2] = ((transitionMask & 2) == 0) ? float3(0) :
        Osd_EvalQuarticBezierCurveMidPoint(cv[4], cv[8], cv[11], cv[13], cv[14]);

    Osd_GetTessLevelsFromPatchBoundaries3(
        OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
        corners, midpoints, patchParam, tessOuterLo, tessOuterHi);
}

// Round up to the nearest even integer
float OsdRoundUpEven(float x) {
    return 2*ceil(x/2);
}

// Round up to the nearest odd integer
float OsdRoundUpOdd(float x) {
    return 2*ceil((x+1)/2)-1;
}

// Compute outer and inner tessellation levels taking into account the
// current tessellation spacing mode.
void
OsdComputeTessLevels(thread float4& tessOuterLo, thread float4& tessOuterHi,
                     thread float4& tessLevelOuter, thread float2& tessLevelInner)
{
    // Outer levels are the sum of the Lo and Hi segments where the Hi
    // segments will have lengths of zero for non-transition edges.

#if defined OSD_FRACTIONAL_EVEN_SPACING
    // Combine fractional outer transition edge levels before rounding.
    float4 combinedOuter = tessOuterLo + tessOuterHi;

    // Round the segments of transition edges separately. We will recover the
    // fractional parameterization of transition edges after tessellation.

    tessLevelOuter = combinedOuter;
    if (tessOuterHi[0] > 0) {
        tessLevelOuter[0] =
            OsdRoundUpEven(tessOuterLo[0]) + OsdRoundUpEven(tessOuterHi[0]);
    }
    if (tessOuterHi[1] > 0) {
        tessLevelOuter[1] =
            OsdRoundUpEven(tessOuterLo[1]) + OsdRoundUpEven(tessOuterHi[1]);
    }
    if (tessOuterHi[2] > 0) {
        tessLevelOuter[2] =
            OsdRoundUpEven(tessOuterLo[2]) + OsdRoundUpEven(tessOuterHi[2]);
    }
    if (tessOuterHi[3] > 0) {
        tessLevelOuter[3] =
            OsdRoundUpEven(tessOuterLo[3]) + OsdRoundUpEven(tessOuterHi[3]);
    }
#elif defined OSD_FRACTIONAL_ODD_SPACING
    // Combine fractional outer transition edge levels before rounding.
    float4 combinedOuter = tessOuterLo + tessOuterHi;

    // Round the segments of transition edges separately. We will recover the
    // fractional parameterization of transition edges after tessellation.
    //
    // The sum of the two outer odd segment lengths will be an even number
    // which the tessellator will increase by +1 so that there will be a
    // total odd number of segments. We clamp the combinedOuter tess levels
    // (used to compute the inner tess levels) so that the outer transition
    // edges will be sampled without degenerate triangles.

    tessLevelOuter = combinedOuter;
    if (tessOuterHi[0] > 0) {
        tessLevelOuter[0] =
            OsdRoundUpOdd(tessOuterLo[0]) + OsdRoundUpOdd(tessOuterHi[0]);
        combinedOuter = max(float4(3), combinedOuter);
    }
    if (tessOuterHi[1] > 0) {
        tessLevelOuter[1] =
            OsdRoundUpOdd(tessOuterLo[1]) + OsdRoundUpOdd(tessOuterHi[1]);
        combinedOuter = max(float4(3), combinedOuter);
    }
    if (tessOuterHi[2] > 0) {
        tessLevelOuter[2] =
            OsdRoundUpOdd(tessOuterLo[2]) + OsdRoundUpOdd(tessOuterHi[2]);
        combinedOuter = max(float4(3), combinedOuter);
    }
    if (tessOuterHi[3] > 0) {
        tessLevelOuter[3] =
            OsdRoundUpOdd(tessOuterLo[3]) + OsdRoundUpOdd(tessOuterHi[3]);
        combinedOuter = max(float4(3), combinedOuter);
    }
#else
    // Round equally spaced transition edge levels before combining.
    tessOuterLo = round(tessOuterLo);
    tessOuterHi = round(tessOuterHi);

    float4 combinedOuter = tessOuterLo + tessOuterHi;
    tessLevelOuter = combinedOuter;
#endif

    // Inner levels are the averages the corresponding outer levels.
    tessLevelInner[0] = (combinedOuter[1] + combinedOuter[3]) * 0.5;
    tessLevelInner[1] = (combinedOuter[0] + combinedOuter[2]) * 0.5;
}

void
OsdComputeTessLevelsTriangle(
        thread float4& tessOuterLo, thread float4& tessOuterHi,
        thread float4& tessLevelOuter, thread float2& tessLevelInner)
{
    OsdComputeTessLevels(tessOuterLo, tessOuterHi,
                         tessLevelOuter, tessLevelInner);

    // Inner level is the max of the three outer levels.
    tessLevelInner[0] = max(max(tessLevelOuter[0],
                                tessLevelOuter[1]),
                                tessLevelOuter[2]);
}

void
OsdGetTessLevelsUniform(
        const float OsdTessLevel, int3 patchParam,
        thread float4& tessLevelOuter, thread float2& tessLevelInner,
        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    OsdGetTessLevelsUniform(OsdTessLevel, patchParam, tessOuterLo, tessOuterHi);
    OsdComputeTessLevels(tessOuterLo, tessOuterHi, tessLevelOuter, tessLevelInner);
}

void
OsdGetTessLevelsUniformTriangle(
        const float OsdTessLevel, int3 patchParam,
        thread float4& tessLevelOuter, thread float2& tessLevelInner,
        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    OsdGetTessLevelsUniformTriangle(OsdTessLevel, patchParam, tessOuterLo, tessOuterHi);
    OsdComputeTessLevelsTriangle(tessOuterLo, tessOuterHi, tessLevelOuter, tessLevelInner);
}

void
OsdEvalPatchBezierTessLevels(
         const float OsdTessLevels,
         const float4x4 OsdProjectionMatrix,
         const float4x4 OsdModelViewMatrix,
         device OsdPerPatchVertexBezier* cpBezier,
         int3 patchParam,
         thread float4& tessLevelOuter, thread float2& tessLevelInner,
         thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    OsdEvalPatchBezierTessLevels(
                OsdTessLevels,
                OsdProjectionMatrix, OsdModelViewMatrix,
                cpBezier, patchParam,
                tessOuterLo, tessOuterHi);

    OsdComputeTessLevels(tessOuterLo, tessOuterHi,
                         tessLevelOuter, tessLevelInner);
}

void
OsdEvalPatchBezierTessLevels(
         const float OsdTessLevels,
         const float4x4 OsdProjectionMatrix,
         const float4x4 OsdModelViewMatrix,
         float3 cv[16],
         int3 patchParam,
         thread float4& tessLevelOuter, thread float2& tessLevelInner,
         thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    OsdEvalPatchBezierTessLevels(
                OsdTessLevels,
                OsdProjectionMatrix, OsdModelViewMatrix,
                cv, patchParam,
                tessOuterLo, tessOuterHi);

    OsdComputeTessLevels(tessOuterLo, tessOuterHi,
                         tessLevelOuter, tessLevelInner);
}

void
OsdEvalPatchBezierTriangleTessLevels(
         const float OsdTessLevels,
         const float4x4 OsdProjectionMatrix,
         const float4x4 OsdModelViewMatrix,
         float3 cv[15],
         int3 patchParam,
         thread float4& tessLevelOuter, thread float2& tessLevelInner,
         thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    OsdEvalPatchBezierTriangleTessLevels(
                OsdTessLevels,
                OsdProjectionMatrix, OsdModelViewMatrix,
                cv, patchParam,
                tessOuterLo, tessOuterHi);

    OsdComputeTessLevelsTriangle(tessOuterLo, tessOuterHi,
                                 tessLevelOuter, tessLevelInner);
}

void
OsdGetTessLevelsAdaptiveRefinedPoints(
    const float OsdTessLevel,
    const float4x4 OsdProjectionMatrix,
    const float4x4 OsdModelViewMatrix,
    float3 cpRefined[16], int3 patchParam,
    thread float4& tessLevelOuter, thread float2& tessLevelInner,
    thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    OsdGetTessLevelsRefinedPoints(OsdTessLevel,
                                  OsdProjectionMatrix, OsdModelViewMatrix,
                                  cpRefined, patchParam,
                                  tessOuterLo, tessOuterHi);

    OsdComputeTessLevels(tessOuterLo, tessOuterHi,
                         tessLevelOuter, tessLevelInner);
}

//  Deprecated -- prefer use of newer Bezier patch equivalent:
void
OsdGetTessLevelsLimitPoints(
    const float OsdTessLevel,
    const float4x4 OsdProjectionMatrix,
    const float4x4 OsdModelViewMatrix,
    device OsdPerPatchVertexBezier* cpBezier,
    int3 patchParam,
    thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    OsdEvalPatchBezierTessLevels(
        OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix,
        cpBezier, patchParam, tessOuterLo, tessOuterHi);
}

//  Deprecated -- prefer use of newer Bezier patch equivalent:
void
OsdGetTessLevelsAdaptiveLimitPoints(
        const float OsdTessLevel,
        const float4x4 OsdProjectionMatrix,
        const float4x4 OsdModelViewMatrix,
        device OsdPerPatchVertexBezier* cpBezier,
        int3 patchParam,
        thread float4& tessLevelOuter, thread float2& tessLevelInner,
        thread float4& tessOuterLo, thread float4& tessOuterHi)
{
    OsdGetTessLevelsLimitPoints(OsdTessLevel,
                                OsdProjectionMatrix, OsdModelViewMatrix,
                                cpBezier, patchParam,
                                tessOuterLo, tessOuterHi);

    OsdComputeTessLevels(tessOuterLo, tessOuterHi,
                         tessLevelOuter, tessLevelInner);
}

//  Deprecated -- prefer use of newer Bezier patch equivalent:
void
OsdGetTessLevels(
        const float OsdTessLevel,
        const float4x4 OsdProjectionMatrix, const float4x4 OsdModelViewMatrix,
        float3 cp0, float3 cp1, float3 cp2, float3 cp3, int3 patchParam,
        thread float4& tessLevelOuter, thread float2& tessLevelInner)
{
    float4 tessOuterLo = float4(0);
    float4 tessOuterHi = float4(0);

#if OSD_ENABLE_SCREENSPACE_TESSELLATION
    tessOuterLo[0] = OsdComputeTessLevel(
        OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, cp0, cp1);
    tessOuterLo[1] = OsdComputeTessLevel(
        OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, cp0, cp3);
    tessOuterLo[2] = OsdComputeTessLevel(
        OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, cp2, cp3);
    tessOuterLo[3] = OsdComputeTessLevel(
        OsdTessLevel, OsdProjectionMatrix, OsdModelViewMatrix, cp1, cp2);
#else
    OsdGetTessLevelsUniform(OsdTessLevel, patchParam, tessOuterLo, tessOuterHi);
#endif

    OsdComputeTessLevels(tessOuterLo, tessOuterHi,
                         tessLevelOuter, tessLevelInner);
}

#if defined OSD_FRACTIONAL_EVEN_SPACING || defined OSD_FRACTIONAL_ODD_SPACING
float
OsdGetTessFractionalSplit(float t, float level, float levelUp)
{
    // Fractional tessellation of an edge will produce n segments where n
    // is the tessellation level of the edge (level) rounded up to the
    // nearest even or odd integer (levelUp). There will be n-2 segments of
    // equal length (dx1) and two additional segments of equal length (dx0)
    // that are typically shorter than the other segments. The two additional
    // segments should be placed symmetrically on opposite sides of the
    // edge (offset).

#if defined OSD_FRACTIONAL_EVEN_SPACING
    if (level <= 2) return t;

    float base = pow(2.0,floor(log2(levelUp)));
    float offset = 1.0/(int(2*base-levelUp)/2 & int(base/2-1));

#elif defined OSD_FRACTIONAL_ODD_SPACING
    if (level <= 1) return t;

    float base = pow(2.0,floor(log2(levelUp)));
    float offset = 1.0/(((int(2*base-levelUp)/2+1) & int(base/2-1))+1);
#endif

    float dx0 = (1.0 - (levelUp-level)/2) / levelUp;
    float dx1 = (1.0 - 2.0*dx0) / (levelUp - 2.0*ceil(dx0));

    if (t < 0.5) {
        float x = levelUp/2 - round(t*levelUp);
        return 0.5 - (x*dx1 + int(x*offset > 1) * (dx0 - dx1));
    } else if (t > 0.5) {
        float x = round(t*levelUp) - levelUp/2;
        return 0.5 + (x*dx1 + int(x*offset > 1) * (dx0 - dx1));
    } else {
        return t;
    }
}
#endif

float
OsdGetTessTransitionSplit(float t, float lo, float hi)
{
#if defined OSD_FRACTIONAL_EVEN_SPACING
    float loRoundUp = OsdRoundUpEven(lo);
    float hiRoundUp = OsdRoundUpEven(hi);

    // Convert the parametric t into a segment index along the combined edge.
    float ti = round(t * (loRoundUp + hiRoundUp));

    if (ti <= loRoundUp) {
        float t0 = ti / loRoundUp;
        return OsdGetTessFractionalSplit(t0, lo, loRoundUp) * 0.5;
    } else {
        float t1 = (ti - loRoundUp) / hiRoundUp;
        return OsdGetTessFractionalSplit(t1, hi, hiRoundUp) * 0.5 + 0.5;
    }
#elif defined OSD_FRACTIONAL_ODD_SPACING
    float loRoundUp = OsdRoundUpOdd(lo);
    float hiRoundUp = OsdRoundUpOdd(hi);

    // Convert the parametric t into a segment index along the combined edge.
    // The +1 below is to account for the extra segment produced by the
    // tessellator since the sum of two odd tess levels will be rounded
    // up by one to the next odd integer tess level.
    float ti = round(t * (loRoundUp + hiRoundUp + 1));

    if (ti <= loRoundUp) {
        float t0 = ti / loRoundUp;
        return OsdGetTessFractionalSplit(t0, lo, loRoundUp) * 0.5;
    } else if (ti > (loRoundUp+1)) {
        float t1 = (ti - (loRoundUp+1)) / hiRoundUp;
        return OsdGetTessFractionalSplit(t1, hi, hiRoundUp) * 0.5 + 0.5;
    } else {
        return 0.5;
    }
#else
    // Convert the parametric t into a segment index along the combined edge.
    float ti = round(t * (lo + hi));

    if (ti <= lo) {
        return (ti / lo) * 0.5;
    } else {
        return ((ti - lo) / hi) * 0.5 + 0.5;
    }
#endif
}

float2
OsdGetTessParameterization(float2 p, float4 tessOuterLo, float4 tessOuterHi)
{
    float2 UV = p;
    if (p.x == 0 && tessOuterHi[0] > 0) {
        UV.y = OsdGetTessTransitionSplit(UV.y, tessOuterLo[0], tessOuterHi[0]);
    } else
    if (p.y == 0 && tessOuterHi[1] > 0) {
        UV.x = OsdGetTessTransitionSplit(UV.x, tessOuterLo[1], tessOuterHi[1]);
    } else
    if (p.x == 1 && tessOuterHi[2] > 0) {
        UV.y = OsdGetTessTransitionSplit(UV.y, tessOuterLo[2], tessOuterHi[2]);
    } else
    if (p.y == 1 && tessOuterHi[3] > 0) {
        UV.x = OsdGetTessTransitionSplit(UV.x, tessOuterLo[3], tessOuterHi[3]);
    }
    return UV;
}

float2
OsdGetTessParameterizationTriangle(float3 p, float4 tessOuterLo, float4 tessOuterHi)
{
    float2 UV = p.xy;
    if (p.x == 0 && tessOuterHi[0] > 0) {
        UV.y = OsdGetTessTransitionSplit(UV.y, tessOuterLo[0], tessOuterHi[0]);
    } else
    if (p.y == 0 && tessOuterHi[1] > 0) {
        UV.x = OsdGetTessTransitionSplit(UV.x, tessOuterLo[1], tessOuterHi[1]);
    } else
    if (p.z == 0 && tessOuterHi[2] > 0) {
        UV.x = OsdGetTessTransitionSplit(UV.x, tessOuterLo[2], tessOuterHi[2]);
        UV.y = 1.0 - UV.x;
    }
    return UV;
}
