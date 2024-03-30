//
//   Copyright 2013 Pixar
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

//----------------------------------------------------------
// Patches.Common
//----------------------------------------------------------

struct InputVertex {
    float4 position : POSITION;
    float3 normal : NORMAL;
};

struct HullVertex {
    float4 position : POSITION;
#ifdef OSD_ENABLE_PATCH_CULL
    int3 clipFlag : CLIPFLAG;
#endif
};

// XXXdyu all downstream data can be handled by client code
struct OutputVertex {
    float4 positionOut : SV_Position;
    float4 position : POSITION1;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : TANGENT1;
    float4 patchCoord : PATCHCOORD; // u, v, faceLevel, faceId
    noperspective float4 edgeDistance : EDGEDISTANCE;
#if defined(OSD_COMPUTE_NORMAL_DERIVATIVES)
    float3 Nu : TANGENT2;
    float3 Nv : TANGENT3;
#endif
#if defined OSD_PATCH_ENABLE_SINGLE_CREASE
    float2 vSegments : VSEGMENTS;
#endif
};

// osd shaders need following functions defined
float4x4 OsdModelViewMatrix();
float4x4 OsdProjectionMatrix();
float4x4 OsdModelViewProjectionMatrix();
float OsdTessLevel();
int OsdGregoryQuadOffsetBase();
int OsdPrimitiveIdBase();
int OsdBaseVertex();

#ifndef OSD_DISPLACEMENT_CALLBACK
#define OSD_DISPLACEMENT_CALLBACK
#endif

// ----------------------------------------------------------------------------
// Patch Parameters
// ----------------------------------------------------------------------------

//
// Each patch has a corresponding patchParam. This is a set of three values
// specifying additional information about the patch:
//
//    faceId    -- topological face identifier (e.g. Ptex FaceId)
//    bitfield  -- refinement-level, non-quad, boundary, transition, uv-offset
//    sharpness -- crease sharpness for single-crease patches
//
// These are stored in OsdPatchParamBuffer indexed by the value returned
// from OsdGetPatchIndex() which is a function of the current PrimitiveID
// along with an optional client provided offset.
//

#if defined OSD_PATCH_ENABLE_SINGLE_CREASE
    Buffer<uint3> OsdPatchParamBuffer : register( t0 );
#else
    Buffer<uint2> OsdPatchParamBuffer : register( t0 );
#endif

int OsdGetPatchIndex(int primitiveId)
{
    return (primitiveId + OsdPrimitiveIdBase());
}

int3 OsdGetPatchParam(int patchIndex)
{
#if defined OSD_PATCH_ENABLE_SINGLE_CREASE
    return OsdPatchParamBuffer[patchIndex].xyz;
#else
    uint2 p = OsdPatchParamBuffer[patchIndex].xy;
    return int3(p.x, p.y, 0);
#endif
}

int OsdGetPatchFaceId(int3 patchParam)
{
    return (patchParam.x & 0xfffffff);
}

int OsdGetPatchFaceLevel(int3 patchParam)
{
    return (1 << ((patchParam.y & 0xf) - ((patchParam.y >> 4) & 1)));
}

int OsdGetPatchRefinementLevel(int3 patchParam)
{
    return (patchParam.y & 0xf);
}

int OsdGetPatchBoundaryMask(int3 patchParam)
{
    return ((patchParam.y >> 7) & 0x1f);
}

int OsdGetPatchTransitionMask(int3 patchParam)
{
    return ((patchParam.x >> 28) & 0xf);
}

int2 OsdGetPatchFaceUV(int3 patchParam)
{
    int u = (patchParam.y >> 22) & 0x3ff;
    int v = (patchParam.y >> 12) & 0x3ff;
    return int2(u,v);
}

bool OsdGetPatchIsRegular(int3 patchParam)
{
    return ((patchParam.y >> 5) & 0x1) != 0;
}

bool OsdGetPatchIsTriangleRotated(int3 patchParam)
{
    int2 uv = OsdGetPatchFaceUV(patchParam);
    return (uv.x + uv.y) >= OsdGetPatchFaceLevel(patchParam);
}

float OsdGetPatchSharpness(int3 patchParam)
{
    return asfloat(patchParam.z);
}

float OsdGetPatchSingleCreaseSegmentParameter(int3 patchParam, float2 uv)
{
    int boundaryMask = OsdGetPatchBoundaryMask(patchParam);
    float s = 0;
    if ((boundaryMask & 1) != 0) {
        s = 1 - uv.y;
    } else if ((boundaryMask & 2) != 0) {
        s = uv.x;
    } else if ((boundaryMask & 4) != 0) {
        s = uv.y;
    } else if ((boundaryMask & 8) != 0) {
        s = 1 - uv.x;
    }
    return s;
}

int4 OsdGetPatchCoord(int3 patchParam)
{
    int faceId = OsdGetPatchFaceId(patchParam);
    int faceLevel = OsdGetPatchFaceLevel(patchParam);
    int2 faceUV = OsdGetPatchFaceUV(patchParam);
    return int4(faceUV.x, faceUV.y, faceLevel, faceId);
}

float4 OsdInterpolatePatchCoord(float2 localUV, int3 patchParam)
{
    int4 perPrimPatchCoord = OsdGetPatchCoord(patchParam);
    int faceId = perPrimPatchCoord.w;
    int faceLevel = perPrimPatchCoord.z;
    float2 faceUV = float2(perPrimPatchCoord.x, perPrimPatchCoord.y);
    float2 uv = localUV/faceLevel + faceUV/faceLevel;
    // add 0.5 to integer values for more robust interpolation
    return float4(uv.x, uv.y, faceLevel+0.5, faceId+0.5);
}

float4 OsdInterpolatePatchCoordTriangle(float2 localUV, int3 patchParam)
{
    float4 result = OsdInterpolatePatchCoord(localUV, patchParam);
    if (OsdGetPatchIsTriangleRotated(patchParam)) {
        result.xy = float2(1.0f, 1.0f) - result.xy;
    }
    return result;
}

// ----------------------------------------------------------------------------
// patch culling
// ----------------------------------------------------------------------------

#ifdef OSD_ENABLE_PATCH_CULL

#define OSD_PATCH_CULL_COMPUTE_CLIPFLAGS(P)                     \
    float4 clipPos = mul(OsdModelViewProjectionMatrix(), P);    \
    int3 clip0 = int3(clipPos.x < clipPos.w,                    \
                      clipPos.y < clipPos.w,                    \
                      clipPos.z < clipPos.w);                   \
    int3 clip1 = int3(clipPos.x > -clipPos.w,                   \
                      clipPos.y > -clipPos.w,                   \
                      clipPos.z > -clipPos.w);                  \
    output.clipFlag = int3(clip0) + 2*int3(clip1);              \

#define OSD_PATCH_CULL(N)                          \
    int3 clipFlag = int3(0,0,0);                   \
    for(int i = 0; i < N; ++i) {                   \
        clipFlag |= patch[i].clipFlag;             \
    }                                              \
    if (any(clipFlag != int3(3,3,3))) {            \
        output.tessLevelInner[0] = 0;              \
        output.tessLevelInner[1] = 0;              \
        output.tessLevelOuter[0] = 0;              \
        output.tessLevelOuter[1] = 0;              \
        output.tessLevelOuter[2] = 0;              \
        output.tessLevelOuter[3] = 0;              \
        output.tessOuterLo = float4(0,0,0,0);      \
        output.tessOuterHi = float4(0,0,0,0);      \
        return output;                             \
    }

#define OSD_PATCH_CULL_TRIANGLE(N)                          \
    int3 clipFlag = int3(0,0,0);                   \
    for(int i = 0; i < N; ++i) {                   \
        clipFlag |= patch[i].clipFlag;             \
    }                                              \
    if (any(clipFlag != int3(3,3,3))) {            \
        output.tessLevelInner[0] = 0;              \
        output.tessLevelOuter[0] = 0;              \
        output.tessLevelOuter[1] = 0;              \
        output.tessLevelOuter[2] = 0;              \
        output.tessOuterLo = float4(0,0,0,0);      \
        output.tessOuterHi = float4(0,0,0,0);      \
        return output;                             \
    }

#else
#define OSD_PATCH_CULL_COMPUTE_CLIPFLAGS(P)
#define OSD_PATCH_CULL(N)
#define OSD_PATCH_CULL_TRIANGLE(N)
#endif

// ----------------------------------------------------------------------------

void
OsdUnivar4x4(in float u, out float B[4], out float D[4])
{
    float t = u;
    float s = 1.0f - u;

    float A0 = s * s;
    float A1 = 2 * s * t;
    float A2 = t * t;

    B[0] = s * A0;
    B[1] = t * A0 + s * A1;
    B[2] = t * A1 + s * A2;
    B[3] = t * A2;

    D[0] =    - A0;
    D[1] = A0 - A1;
    D[2] = A1 - A2;
    D[3] = A2;
}

void
OsdUnivar4x4(in float u, out float B[4], out float D[4], out float C[4])
{
    float t = u;
    float s = 1.0f - u;

    float A0 = s * s;
    float A1 = 2 * s * t;
    float A2 = t * t;

    B[0] = s * A0;
    B[1] = t * A0 + s * A1;
    B[2] = t * A1 + s * A2;
    B[3] = t * A2;

    D[0] =    - A0;
    D[1] = A0 - A1;
    D[2] = A1 - A2;
    D[3] = A2;

    A0 =   - s;
    A1 = s - t;
    A2 = t;

    C[0] =    - A0;
    C[1] = A0 - A1;
    C[2] = A1 - A2;
    C[3] = A2;
}

// ----------------------------------------------------------------------------

struct OsdPerPatchVertexBezier {
    int3 patchParam : PATCHPARAM;
    float3 P : POSITION;
#if defined OSD_PATCH_ENABLE_SINGLE_CREASE
    float3 P1 : POSITION1;
    float3 P2 : POSITION2;
    float2 vSegments : VSEGMENTS;
#endif
};

float3
OsdEvalBezier(float3 cp[16], float2 uv)
{
    float3 BUCP[4] = {float3(0,0,0),float3(0,0,0),float3(0,0,0),float3(0,0,0)};

    float B[4], D[4];

    OsdUnivar4x4(uv.x, B, D);
    for (int i=0; i<4; ++i) {
        for (int j=0; j<4; ++j) {
            float3 A = cp[4*i + j];
            BUCP[i] += A * B[j];
        }
    }

    float3 P = float3(0,0,0);

    OsdUnivar4x4(uv.y, B, D);
    for (int k=0; k<4; ++k) {
        P += B[k] * BUCP[k];
    }

    return P;
}

// When OSD_PATCH_ENABLE_SINGLE_CREASE is defined,
// this function evaluates single-crease patch, which is segmented into
// 3 parts in the v-direction.
//
//  v=0             vSegment.x        vSegment.y              v=1
//   +------------------+-------------------+------------------+
//   |       cp 0       |     cp 1          |      cp 2        |
//   | (infinite sharp) | (floor sharpness) | (ceil sharpness) |
//   +------------------+-------------------+------------------+
//
float3
OsdEvalBezier(OsdPerPatchVertexBezier cp[16], int3 patchParam, float2 uv)
{
    float3 BUCP[4] = {float3(0,0,0),float3(0,0,0),float3(0,0,0),float3(0,0,0)};

    float B[4], D[4];
    float s = OsdGetPatchSingleCreaseSegmentParameter(patchParam, uv);

    OsdUnivar4x4(uv.x, B, D);
#if defined OSD_PATCH_ENABLE_SINGLE_CREASE
    float2 vSegments = cp[0].vSegments;
    if (s <= vSegments.x) {
        for (int i=0; i<4; ++i) {
            for (int j=0; j<4; ++j) {
                float3 A = cp[4*i + j].P;
                BUCP[i] += A * B[j];
            }
        }
    } else if (s <= vSegments.y) {
        for (int i=0; i<4; ++i) {
            for (int j=0; j<4; ++j) {
                float3 A = cp[4*i + j].P1;
                BUCP[i] += A * B[j];
            }
        }
    } else {
        for (int i=0; i<4; ++i) {
            for (int j=0; j<4; ++j) {
                float3 A = cp[4*i + j].P2;
                BUCP[i] += A * B[j];
            }
        }
    }
#else
    for (int i=0; i<4; ++i) {
        for (int j=0; j<4; ++j) {
            float3 A = cp[4*i + j].P;
            BUCP[i] += A * B[j];
        }
    }
#endif

    float3 P = float3(0,0,0);

    OsdUnivar4x4(uv.y, B, D);
    for (int k=0; k<4; ++k) {
        P += B[k] * BUCP[k];
    }

    return P;
}

// ----------------------------------------------------------------------------
// Boundary Interpolation
// ----------------------------------------------------------------------------

void
OsdComputeBSplineBoundaryPoints(inout float3 cpt[16], int3 patchParam)
{
    int boundaryMask = OsdGetPatchBoundaryMask(patchParam);

    //  Don't extrapolate corner points until all boundary points in place
    if ((boundaryMask & 1) != 0) {
        cpt[1] = 2*cpt[5] - cpt[9];
        cpt[2] = 2*cpt[6] - cpt[10];
    }
    if ((boundaryMask & 2) != 0) {
        cpt[7] = 2*cpt[6] - cpt[5];
        cpt[11] = 2*cpt[10] - cpt[9];
    }
    if ((boundaryMask & 4) != 0) {
        cpt[13] = 2*cpt[9] - cpt[5];
        cpt[14] = 2*cpt[10] - cpt[6];
    }
    if ((boundaryMask & 8) != 0) {
        cpt[4] = 2*cpt[5] - cpt[6];
        cpt[8] = 2*cpt[9] - cpt[10];
    }

    //  Now safe to extrapolate corner points:
    if ((boundaryMask & 1) != 0) {
        cpt[0] = 2*cpt[4] - cpt[8];
        cpt[3] = 2*cpt[7] - cpt[11];
    }
    if ((boundaryMask & 2) != 0) {
        cpt[3] = 2*cpt[2] - cpt[1];
        cpt[15] = 2*cpt[14] - cpt[13];
    }
    if ((boundaryMask & 4) != 0) {
        cpt[12] = 2*cpt[8] - cpt[4];
        cpt[15] = 2*cpt[11] - cpt[7];
    }
    if ((boundaryMask & 8) != 0) {
        cpt[0] = 2*cpt[1] - cpt[2];
        cpt[12] = 2*cpt[13] - cpt[14];
    }
}

void
OsdComputeBoxSplineTriangleBoundaryPoints(inout float3 cpt[12], int3 patchParam)
{
    int boundaryMask = OsdGetPatchBoundaryMask(patchParam);
    if (boundaryMask == 0) return;

    int upperBits = (boundaryMask >> 3) & 0x3;
    int lowerBits = boundaryMask & 7;

    int eBits = lowerBits;
    int vBits = 0;

    if (upperBits == 1) {
        vBits = eBits;
        eBits = 0;
    } else if (upperBits == 2) {
        //  Opposite vertex bit is edge bit rotated one to the right:
        vBits = ((eBits & 1) << 2) | (eBits >> 1);
    }

    bool edge0IsBoundary = (eBits & 1) != 0;
    bool edge1IsBoundary = (eBits & 2) != 0;
    bool edge2IsBoundary = (eBits & 4) != 0;

    if (edge0IsBoundary) {
        if (edge2IsBoundary) {
            cpt[0] = cpt[4] + (cpt[4] - cpt[8]);
        } else {
            cpt[0] = cpt[4] + (cpt[3] - cpt[7]);
        }
        cpt[1] = cpt[4] + cpt[5] - cpt[8];
        if (edge1IsBoundary) {
            cpt[2] = cpt[5] + (cpt[5] - cpt[8]);
        } else {
            cpt[2] = cpt[5] + (cpt[6] - cpt[9]);
        }
    }
    if (edge1IsBoundary) {
        if (edge0IsBoundary) {
            cpt[6] = cpt[5] + (cpt[5] - cpt[4]);
        } else {
            cpt[6] = cpt[5] + (cpt[2] - cpt[1]);
        }
        cpt[9] = cpt[5] + cpt[8] - cpt[4];
        if (edge2IsBoundary) {
            cpt[11] = cpt[8] + (cpt[8] - cpt[4]);
        } else {
            cpt[11] = cpt[8] + (cpt[10] - cpt[7]);
        }
    }
    if (edge2IsBoundary) {
        if (edge1IsBoundary) {
            cpt[10] = cpt[8] + (cpt[8] - cpt[5]);
        } else {
            cpt[10] = cpt[8] + (cpt[11] - cpt[9]);
        }
        cpt[7] = cpt[8] + cpt[4] - cpt[5];
        if (edge0IsBoundary) {
            cpt[3] = cpt[4] + (cpt[4] - cpt[5]);
        } else {
            cpt[3] = cpt[4] + (cpt[0] - cpt[1]);
        }
    }

    if ((vBits & 1) != 0) {
        cpt[3] = cpt[4] + cpt[7] - cpt[8];
        cpt[0] = cpt[4] + cpt[1] - cpt[5];
    }
    if ((vBits & 2) != 0) {
        cpt[2] = cpt[5] + cpt[1] - cpt[4];
        cpt[6] = cpt[5] + cpt[9] - cpt[8];
    }
    if ((vBits & 4) != 0) {
        cpt[11] = cpt[8] + cpt[9] - cpt[5];
        cpt[10] = cpt[8] + cpt[7] - cpt[4];
    }
}

// ----------------------------------------------------------------------------
// BSpline
// ----------------------------------------------------------------------------

// compute single-crease patch matrix
float4x4
OsdComputeMs(float sharpness)
{
    float s = pow(2.0f, sharpness);
    float s2 = s*s;
    float s3 = s2*s;

    float4x4 m ={
        0, s + 1 + 3*s2 - s3, 7*s - 2 - 6*s2 + 2*s3, (1-s)*(s-1)*(s-1),
        0,       (1+s)*(1+s),        6*s - 2 - 2*s2,       (s-1)*(s-1),
        0,               1+s,               6*s - 2,               1-s,
        0,                 1,               6*s - 2,                 1 };

    m /= (s*6.0);
    m[0][0] = 1.0/6.0;

    return m;
}

// flip matrix orientation
float4x4
OsdFlipMatrix(float4x4 m)
{
    return float4x4(m[3][3], m[3][2], m[3][1], m[3][0],
                    m[2][3], m[2][2], m[2][1], m[2][0],
                    m[1][3], m[1][2], m[1][1], m[1][0],
                    m[0][3], m[0][2], m[0][1], m[0][0]);
}

// Regular BSpline to Bezier
static float4x4 Q = {
    1.f/6.f, 4.f/6.f, 1.f/6.f, 0.f,
    0.f,     4.f/6.f, 2.f/6.f, 0.f,
    0.f,     2.f/6.f, 4.f/6.f, 0.f,
    0.f,     1.f/6.f, 4.f/6.f, 1.f/6.f
};

// Infinitely Sharp (boundary)
static float4x4 Mi = {
    1.f/6.f, 4.f/6.f, 1.f/6.f, 0.f,
    0.f,     4.f/6.f, 2.f/6.f, 0.f,
    0.f,     2.f/6.f, 4.f/6.f, 0.f,
    0.f,     0.f,     1.f,     0.f
};

// convert BSpline cv to Bezier cv
void
OsdComputePerPatchVertexBSpline(int3 patchParam, int ID, float3 cv[16],
                                out OsdPerPatchVertexBezier result)
{
    result.patchParam = patchParam;

    int i = ID%4;
    int j = ID/4;

#if defined OSD_PATCH_ENABLE_SINGLE_CREASE

    float3 P  = float3(0,0,0); // 0 to 1-2^(-Sf)
    float3 P1 = float3(0,0,0); // 1-2^(-Sf) to 1-2^(-Sc)
    float3 P2 = float3(0,0,0); // 1-2^(-Sc) to 1

    float sharpness = OsdGetPatchSharpness(patchParam);
    if (sharpness > 0) {
        float Sf = floor(sharpness);
        float Sc = ceil(sharpness);
        float Sr = frac(sharpness);
        float4x4 Mf = OsdComputeMs(Sf);
        float4x4 Mc = OsdComputeMs(Sc);
        float4x4 Mj = (1-Sr) * Mf + Sr * Mi;
        float4x4 Ms = (1-Sr) * Mf + Sr * Mc;
        float s0 = 1 - pow(2, -floor(sharpness));
        float s1 = 1 - pow(2, -ceil(sharpness));
        result.vSegments = float2(s0, s1);

        float4x4 MUi = Q, MUj = Q, MUs = Q;
        float4x4 MVi = Q, MVj = Q, MVs = Q;

        int boundaryMask = OsdGetPatchBoundaryMask(patchParam);
        if ((boundaryMask & 1) != 0) {
            MVi = OsdFlipMatrix(Mi);
            MVj = OsdFlipMatrix(Mj);
            MVs = OsdFlipMatrix(Ms);
        }
        if ((boundaryMask & 2) != 0) {
            MUi = Mi;
            MUj = Mj;
            MUs = Ms;
        }
        if ((boundaryMask & 4) != 0) {
            MVi = Mi;
            MVj = Mj;
            MVs = Ms;
        }
        if ((boundaryMask & 8) != 0) {
            MUi = OsdFlipMatrix(Mi);
            MUj = OsdFlipMatrix(Mj);
            MUs = OsdFlipMatrix(Ms);
        }

        float3 Hi[4], Hj[4], Hs[4];
        for (int l=0; l<4; ++l) {
            Hi[l] = Hj[l] = Hs[l] = float3(0,0,0);
            for (int k=0; k<4; ++k) {
                Hi[l] += MUi[i][k] * cv[l*4 + k];
                Hj[l] += MUj[i][k] * cv[l*4 + k];
                Hs[l] += MUs[i][k] * cv[l*4 + k];
            }
        }
        for (int k=0; k<4; ++k) {
            P  += MVi[j][k]*Hi[k];
            P1 += MVj[j][k]*Hj[k];
            P2 += MVs[j][k]*Hs[k];
        }

        result.P  = P;
        result.P1 = P1;
        result.P2 = P2;
    } else {
        result.vSegments = float2(0, 0);

        OsdComputeBSplineBoundaryPoints(cv, patchParam);

        float3 Hi[4];
        for (int l=0; l<4; ++l) {
            Hi[l] = float3(0,0,0);
            for (int k=0; k<4; ++k) {
                Hi[l] += Q[i][k] * cv[l*4 + k];
            }
        }
        for (int k=0; k<4; ++k) {
            P += Q[j][k]*Hi[k];
        }

        result.P  = P;
        result.P1 = P;
        result.P2 = P;
    }
#else
    OsdComputeBSplineBoundaryPoints(cv, patchParam);

    float3 H[4];
    for (int l=0; l<4; ++l) {
        H[l] = float3(0,0,0);
        for(int k=0; k<4; ++k) {
            H[l] += Q[i][k] * cv[l*4 + k];
        }
    }
    {
        result.P = float3(0,0,0);
        for (int k=0; k<4; ++k){
            result.P += Q[j][k]*H[k];
        }
    }
#endif
}

void
OsdEvalPatchBezier(int3 patchParam, float2 UV,
                   OsdPerPatchVertexBezier cv[16],
                   out float3 P, out float3 dPu, out float3 dPv,
                   out float3 N, out float3 dNu, out float3 dNv)
{
    //
    //  Use the recursive nature of the basis functions to compute a 2x2 set
    //  of intermediate points (via repeated linear interpolation).  These
    //  points define a bilinear surface tangent to the desired surface at P
    //  and so containing dPu and dPv.  The cost of computing P, dPu and dPv
    //  this way is comparable to that of typical tensor product evaluation
    //  (if not faster).
    //
    //  If N = dPu X dPv degenerates, it often results from an edge of the
    //  2x2 bilinear hull collapsing or two adjacent edges colinear. In both
    //  cases, the expected non-planar quad degenerates into a triangle, and
    //  the tangent plane of that triangle provides the desired normal N.
    //

    //  Reduce 4x4 points to 2x4 -- two levels of linear interpolation in U
    //  and so 3 original rows contributing to each of the 2 resulting rows:
    float u    = UV.x;
    float uinv = 1.0f - u;

    float u0 = uinv * uinv;
    float u1 = u * uinv * 2.0f;
    float u2 = u * u;

    float3 LROW[4], RROW[4];
#ifndef OSD_PATCH_ENABLE_SINGLE_CREASE
    LROW[0] = u0 * cv[ 0].P + u1 * cv[ 1].P + u2 * cv[ 2].P;
    LROW[1] = u0 * cv[ 4].P + u1 * cv[ 5].P + u2 * cv[ 6].P;
    LROW[2] = u0 * cv[ 8].P + u1 * cv[ 9].P + u2 * cv[10].P;
    LROW[3] = u0 * cv[12].P + u1 * cv[13].P + u2 * cv[14].P;

    RROW[0] = u0 * cv[ 1].P + u1 * cv[ 2].P + u2 * cv[ 3].P;
    RROW[1] = u0 * cv[ 5].P + u1 * cv[ 6].P + u2 * cv[ 7].P;
    RROW[2] = u0 * cv[ 9].P + u1 * cv[10].P + u2 * cv[11].P;
    RROW[3] = u0 * cv[13].P + u1 * cv[14].P + u2 * cv[15].P;
#else
    float2 vSegments = cv[0].vSegments;
    float s = OsdGetPatchSingleCreaseSegmentParameter(patchParam, UV);

    for (int i = 0; i < 4; ++i) {
        int j = i*4;
        if (s <= vSegments.x) {
            LROW[i] = u0 * cv[ j ].P + u1 * cv[j+1].P + u2 * cv[j+2].P;
            RROW[i] = u0 * cv[j+1].P + u1 * cv[j+2].P + u2 * cv[j+3].P;
        } else if (s <= vSegments.y) {
            LROW[i] = u0 * cv[ j ].P1 + u1 * cv[j+1].P1 + u2 * cv[j+2].P1;
            RROW[i] = u0 * cv[j+1].P1 + u1 * cv[j+2].P1 + u2 * cv[j+3].P1;
        } else {
            LROW[i] = u0 * cv[ j ].P2 + u1 * cv[j+1].P2 + u2 * cv[j+2].P2;
            RROW[i] = u0 * cv[j+1].P2 + u1 * cv[j+2].P2 + u2 * cv[j+3].P2;
        }
    }
#endif

    //  Reduce 2x4 points to 2x2 -- two levels of linear interpolation in V
    //  and so 3 original pairs contributing to each of the 2 resulting:
    float v    = UV.y;
    float vinv = 1.0f - v;

    float v0 = vinv * vinv;
    float v1 = v * vinv * 2.0f;
    float v2 = v * v;

    float3 LPAIR[2], RPAIR[2];
    LPAIR[0] = v0 * LROW[0] + v1 * LROW[1] + v2 * LROW[2];
    RPAIR[0] = v0 * RROW[0] + v1 * RROW[1] + v2 * RROW[2];

    LPAIR[1] = v0 * LROW[1] + v1 * LROW[2] + v2 * LROW[3];
    RPAIR[1] = v0 * RROW[1] + v1 * RROW[2] + v2 * RROW[3];

    //  Interpolate points on the edges of the 2x2 bilinear hull from which
    //  both position and partials are trivially determined:
    float3 DU0 = vinv * LPAIR[0] + v * LPAIR[1];
    float3 DU1 = vinv * RPAIR[0] + v * RPAIR[1];
    float3 DV0 = uinv * LPAIR[0] + u * RPAIR[0];
    float3 DV1 = uinv * LPAIR[1] + u * RPAIR[1];

    int level = OsdGetPatchFaceLevel(patchParam);
    dPu = (DU1 - DU0) * 3 * level;
    dPv = (DV1 - DV0) * 3 * level;

    P = u * DU1 + uinv * DU0;

    //  Compute the normal and test for degeneracy:
    //
    //  We need a geometric measure of the size of the patch for a suitable
    //  tolerance.  Magnitudes of the partials are generally proportional to
    //  that size -- the sum of the partials is readily available, cheap to
    //  compute, and has proved effective in most cases (though not perfect).
    //  The size of the bounding box of the patch, or some approximation to
    //  it, would be better but more costly to compute.
    //
    float proportionalNormalTolerance = 0.00001f;

    float nEpsilon = (length(dPu) + length(dPv)) * proportionalNormalTolerance;

    N = cross(dPu, dPv);

    float nLength = length(N);
    if (nLength > nEpsilon) {
        N = N / nLength;
    } else {
        float3 diagCross = cross(RPAIR[1] - LPAIR[0], LPAIR[1] - RPAIR[0]);
        float diagCrossLength = length(diagCross);
        if (diagCrossLength > nEpsilon) {
            N = diagCross / diagCrossLength;
        }
    }

#ifndef OSD_COMPUTE_NORMAL_DERIVATIVES
    dNu = float3(0,0,0);
    dNv = float3(0,0,0);
#else
    //
    //  Compute 2nd order partials of P(u,v) in order to compute 1st order partials
    //  for the un-normalized n(u,v) = dPu X dPv, then project into the tangent
    //  plane of normalized N.  With resulting dNu and dNv we can make another
    //  attempt to resolve a still-degenerate normal.
    //
    //  We don't use the Weingarten equations here as they require N != 0 and also
    //  are a little less numerically stable/accurate in single precision.
    //
    float B0u[4], B1u[4], B2u[4];
    float B0v[4], B1v[4], B2v[4];

    OsdUnivar4x4(UV.x, B0u, B1u, B2u);
    OsdUnivar4x4(UV.y, B0v, B1v, B2v);

    float3 dUU = float3(0,0,0);
    float3 dVV = float3(0,0,0);
    float3 dUV = float3(0,0,0);

    for (int i=0; i<4; ++i) {
        for (int j=0; j<4; ++j) {
#ifdef OSD_PATCH_ENABLE_SINGLE_CREASE
            int k = 4*i + j;
            float3 CV = (s <= vSegments.x) ? cv[k].P
                   :   ((s <= vSegments.y) ? cv[k].P1
                                           : cv[k].P2);
#else
            float3 CV = cv[4*i + j].P;
#endif
            dUU += (B0v[i] * B2u[j]) * CV;
            dVV += (B2v[i] * B0u[j]) * CV;
            dUV += (B1v[i] * B1u[j]) * CV;
        }
    }

    dUU *= 6 * level;
    dVV *= 6 * level;
    dUV *= 9 * level;

    dNu = cross(dUU, dPv) + cross(dPu, dUV);
    dNv = cross(dUV, dPv) + cross(dPu, dVV);

    float nLengthInv = 1.0;
    if (nLength > nEpsilon) {
        nLengthInv = 1.0 / nLength;
    } else {
        //  N may have been resolved above if degenerate, but if N was resolved
        //  we don't have an accurate length for its un-normalized value, and that
        //  length is needed to project the un-normalized dNu and dNv into the
        //  tangent plane of N.
        //
        //  So compute N more accurately with available second derivatives, i.e.
        //  with a 1st order Taylor approximation to un-normalized N(u,v).

        float DU = (UV.x == 1.0f) ? -1.0f : 1.0f;
        float DV = (UV.y == 1.0f) ? -1.0f : 1.0f;

        N = DU * dNu + DV * dNv;

        nLength = length(N);
        if (nLength > nEpsilon) {
            nLengthInv = 1.0f / nLength;
            N = N * nLengthInv;
        }
    }

    //  Project derivatives of non-unit normals into tangent plane of N:
    dNu = (dNu - dot(dNu,N) * N) * nLengthInv;
    dNv = (dNv - dot(dNv,N) * N) * nLengthInv;
#endif
}

// ----------------------------------------------------------------------------
// Gregory Basis
// ----------------------------------------------------------------------------

struct OsdPerPatchVertexGregoryBasis {
    int3 patchParam : PATCHPARAM;
    float3 P : POSITION0;
};

void
OsdComputePerPatchVertexGregoryBasis(int3 patchParam, int ID, float3 cv,
                                     out OsdPerPatchVertexGregoryBasis result)
{
    result.patchParam = patchParam;
    result.P = cv;
}

void
OsdEvalPatchGregory(int3 patchParam, float2 UV, float3 cv[20],
                    out float3 P, out float3 dPu, out float3 dPv,
                    out float3 N, out float3 dNu, out float3 dNv)
{
    float u = UV.x, v = UV.y;
    float U = 1-u, V = 1-v;

    //(0,1)                              (1,1)
    //   P3         e3-      e2+         P2
    //      15------17-------11-------10
    //      |        |        |        |
    //      |        |        |        |
    //      |        | f3-    | f2+    |
    //      |       19       13        |
    //  e3+ 16-----18          14-----12 e2-
    //      |     f3+          f2-     |
    //      |                          |
    //      |                          |
    //      |     f0-         f1+      |
    //  e0- 2------4            8------6 e1+
    //      |        3 f0+    9        |
    //      |        |        | f1-    |
    //      |        |        |        |
    //      |        |        |        |
    //      0--------1--------7--------5
    //    P0        e0+      e1-         P1
    //(0,0)                               (1,0)

    float d11 = u+v;
    float d12 = U+v;
    float d21 = u+V;
    float d22 = U+V;

    OsdPerPatchVertexBezier bezcv[16];

    bezcv[ 5].P = (d11 == 0.0) ? cv[3]  : (u*cv[3] + v*cv[4])/d11;
    bezcv[ 6].P = (d12 == 0.0) ? cv[8]  : (U*cv[9] + v*cv[8])/d12;
    bezcv[ 9].P = (d21 == 0.0) ? cv[18] : (u*cv[19] + V*cv[18])/d21;
    bezcv[10].P = (d22 == 0.0) ? cv[13] : (U*cv[13] + V*cv[14])/d22;

    bezcv[ 0].P = cv[0];
    bezcv[ 1].P = cv[1];
    bezcv[ 2].P = cv[7];
    bezcv[ 3].P = cv[5];
    bezcv[ 4].P = cv[2];
    bezcv[ 7].P = cv[6];
    bezcv[ 8].P = cv[16];
    bezcv[11].P = cv[12];
    bezcv[12].P = cv[15];
    bezcv[13].P = cv[17];
    bezcv[14].P = cv[11];
    bezcv[15].P = cv[10];

    OsdEvalPatchBezier(patchParam, UV, bezcv, P, dPu, dPv, N, dNu, dNv);
}

//
//  Convert the 12 points of a regular patch resulting from Loop subdivision
//  into a more accessible Bezier patch for both tessellation assessment and
//  evaluation.
//
//  Regular patch for Loop subdivision -- quartic triangular Box spline:
//
//                           10 --- 11
//                           . .   . .
//                          .   . .   .
//                         7 --- 8 --- 9
//                        . .   . .   . .
//                       .   . .   . .   .
//                      3 --- 4 --- 5 --- 6
//                       .   . .   . .   .
//                        . .   . .   . .
//                         0 --- 1 --- 2
//
//  The equivalant quartic Bezier triangle (15 points):
//
//                              14
//                              . .
//                             .   .
//                           12 --- 13
//                           . .   . .
//                          .   . .   .
//                         9 -- 10 --- 11
//                        . .   . .   . .
//                       .   . .   . .   .
//                      5 --- 6 --- 7 --- 8
//                     . .   . .   . .   . .
//                    .   . .   . .   . .   .
//                   0 --- 1 --- 2 --- 3 --- 4
//
//  A hybrid cubic/quartic Bezier patch with cubic boundaries is a close
//  approximation and would only use 12 control points, but we need a full
//  quartic patch to maintain accuracy along boundary curves -- especially
//  between subdivision levels.
//
void
OsdComputePerPatchVertexBoxSplineTriangle(int3 patchParam, int ID, float3 cv[12],
                                          out OsdPerPatchVertexBezier result)
{
    //
    //  Conversion matrix from 12-point Box spline to 15-point quartic Bezier
    //  patch and its common scale factor:
    //
    const float boxToBezierMatrix[12*15] = {
    // L0   L1   L2     L3   L4   L5   L6     L7   L8   L9     L10  L11
        2,   2,   0,     2,  12,   2,   0,     2,   2,   0,     0,   0,  // B0
        1,   3,   0,     0,  12,   4,   0,     1,   3,   0,     0,   0,  // B1
        0,   4,   0,     0,   8,   8,   0,     0,   4,   0,     0,   0,  // B2
        0,   3,   1,     0,   4,  12,   0,     0,   3,   1,     0,   0,  // B3
        0,   2,   2,     0,   2,  12,   2,     0,   2,   2,     0,   0,  // B4
        0,   1,   0,     1,  12,   3,   0,     3,   4,   0,     0,   0,  // B5
        0,   1,   0,     0,  10,   6,   0,     1,   6,   0,     0,   0,  // B6
        0,   1,   0,     0,   6,  10,   0,     0,   6,   1,     0,   0,  // B7
        0,   1,   0,     0,   3,  12,   1,     0,   4,   3,     0,   0,  // B8
        0,   0,   0,     0,   8,   4,   0,     4,   8,   0,     0,   0,  // B9
        0,   0,   0,     0,   6,   6,   0,     1,  10,   1,     0,   0,  // B10
        0,   0,   0,     0,   4,   8,   0,     0,   8,   4,     0,   0,  // B11
        0,   0,   0,     0,   4,   3,   0,     3,  12,   1,     1,   0,  // B12
        0,   0,   0,     0,   3,   4,   0,     1,  12,   3,     0,   1,  // B13
        0,   0,   0,     0,   2,   2,   0,     2,  12,   2,     2,   2   // B14
    };
    const float boxToBezierMatrixScale = 1.0 / 24.0;

    OsdComputeBoxSplineTriangleBoundaryPoints(cv, patchParam);

    result.patchParam = patchParam;
    result.P = float3(0,0,0);

    int cvCoeffBase = 12 * ID;

    for (int i = 0; i < 12; ++i) {
        result.P += boxToBezierMatrix[cvCoeffBase + i] * cv[i];
    }
    result.P *= boxToBezierMatrixScale;
}

void
OsdEvalPatchBezierTriangle(int3 patchParam, float2 UV,
                           OsdPerPatchVertexBezier cv[15],
                           out float3 P, out float3 dPu, out float3 dPv,
                           out float3 N, out float3 dNu, out float3 dNv)
{
    float u = UV.x;
    float v = UV.y;
    float w = 1.0 - u - v;

    float uu = u * u;
    float vv = v * v;
    float ww = w * w;

#ifdef OSD_COMPUTE_NORMAL_DERIVATIVES
    //
    //  When computing normal derivatives, we need 2nd derivatives, so compute
    //  an intermediate quadratic Bezier triangle from which 2nd derivatives
    //  can be easily computed, and which in turn yields the triangle that gives
    //  the position and 1st derivatives.
    //
    //  Quadratic barycentric basis functions (in addition to those above):
    float uv = u * v * 2.0;
    float vw = v * w * 2.0;
    float wu = w * u * 2.0;

    float3 Q0 = ww * cv[ 0].P + wu * cv[ 1].P + uu * cv[ 2].P +
                uv * cv[ 6].P + vv * cv[ 9].P + vw * cv[ 5].P;
    float3 Q1 = ww * cv[ 1].P + wu * cv[ 2].P + uu * cv[ 3].P +
                uv * cv[ 7].P + vv * cv[10].P + vw * cv[ 6].P;
    float3 Q2 = ww * cv[ 2].P + wu * cv[ 3].P + uu * cv[ 4].P +
                uv * cv[ 8].P + vv * cv[11].P + vw * cv[ 7].P;
    float3 Q3 = ww * cv[ 5].P + wu * cv[ 6].P + uu * cv[ 7].P +
                uv * cv[10].P + vv * cv[12].P + vw * cv[ 9].P;
    float3 Q4 = ww * cv[ 6].P + wu * cv[ 7].P + uu * cv[ 8].P +
                uv * cv[11].P + vv * cv[13].P + vw * cv[10].P;
    float3 Q5 = ww * cv[ 9].P + wu * cv[10].P + uu * cv[11].P +
                uv * cv[13].P + vv * cv[14].P + vw * cv[12].P;

    float3 V0 = w * Q0 + u * Q1 + v * Q3;
    float3 V1 = w * Q1 + u * Q2 + v * Q4;
    float3 V2 = w * Q3 + u * Q4 + v * Q5;
#else
    //
    //  When 2nd derivatives are not required, factor the recursive evaluation
    //  of a point to directly provide the three points of the triangle at the
    //  last stage -- which then trivially provides both position and 1st
    //  derivatives.  Each point of the triangle results from evaluating the
    //  corresponding cubic Bezier sub-triangle for each corner of the quartic:
    //
    //  Cubic barycentric basis functions:
    float uuu = uu * u;
    float uuv = uu * v * 3.0;
    float uvv = u * vv * 3.0;
    float vvv = vv * v;
    float vvw = vv * w * 3.0;
    float vww = v * ww * 3.0;
    float www = ww * w;
    float wwu = ww * u * 3.0;
    float wuu = w * uu * 3.0;
    float uvw = u * v * w * 6.0;

    float3 V0 = www * cv[ 0].P + wwu * cv[ 1].P + wuu * cv[ 2].P
              + uuu * cv[ 3].P + uuv * cv[ 7].P + uvv * cv[10].P
              + vvv * cv[12].P + vvw * cv[ 9].P + vww * cv[ 5].P + uvw * cv[ 6].P;

    float3 V1 = www * cv[ 1].P + wwu * cv[ 2].P + wuu * cv[ 3].P
              + uuu * cv[ 4].P + uuv * cv[ 8].P + uvv * cv[11].P
              + vvv * cv[13].P + vvw * cv[10].P + vww * cv[ 6].P + uvw * cv[ 7].P;

    float3 V2 = www * cv[ 5].P + wwu * cv[ 6].P + wuu * cv[ 7].P
              + uuu * cv[ 8].P + uuv * cv[11].P + uvv * cv[13].P
              + vvv * cv[14].P + vvw * cv[12].P + vww * cv[ 9].P + uvw * cv[10].P;
#endif

    //
    //  Compute P, du and dv all from the triangle formed from the three Vi:
    //
    P = w * V0 + u * V1 + v * V2;

    int dSign = OsdGetPatchIsTriangleRotated(patchParam) ? -1 : 1;
    int level = OsdGetPatchFaceLevel(patchParam);

    float d1Scale = dSign * level * 4;

    dPu = (V1 - V0) * d1Scale;
    dPv = (V2 - V0) * d1Scale;

    //  Compute N and test for degeneracy:
    //
    //  We need a geometric measure of the size of the patch for a suitable
    //  tolerance.  Magnitudes of the partials are generally proportional to
    //  that size -- the sum of the partials is readily available, cheap to
    //  compute, and has proved effective in most cases (though not perfect).
    //  The size of the bounding box of the patch, or some approximation to
    //  it, would be better but more costly to compute.
    //
    float proportionalNormalTolerance = 0.00001f;

    float nEpsilon = (length(dPu) + length(dPv)) * proportionalNormalTolerance;

    N = cross(dPu, dPv);
    float nLength = length(N);


#ifdef OSD_COMPUTE_NORMAL_DERIVATIVES
    //
    //  Compute normal derivatives using 2nd order partials, then use the
    //  normal derivatives to resolve a degenerate normal:
    //
    float d2Scale = dSign * level * level * 12;

    float3 dUU = (Q0 - 2 * Q1 + Q2)  * d2Scale;
    float3 dVV = (Q0 - 2 * Q3 + Q5)  * d2Scale;
    float3 dUV = (Q0 - Q1 + Q4 - Q3) * d2Scale;

    dNu = cross(dUU, dPv) + cross(dPu, dUV);
    dNv = cross(dUV, dPv) + cross(dPu, dVV);

    if (nLength < nEpsilon) {
        //  Use 1st order Taylor approximation of N(u,v) within patch interior:
        if (w > 0.0) {
            N =  dNu + dNv;
        } else if (u >= 1.0) {
            N = -dNu + dNv;
        } else if (v >= 1.0) {
            N =  dNu - dNv;
        } else {
            N = -dNu - dNv;
        }

        nLength = length(N);
        if (nLength < nEpsilon) {
            nLength = 1.0;
        }
    }
    N = N / nLength;

    //  Project derivs of non-unit normal function onto tangent plane of N:
    dNu = (dNu - dot(dNu,N) * N) / nLength;
    dNv = (dNv - dot(dNv,N) * N) / nLength;
#else
    dNu = float3(0,0,0);
    dNv = float3(0,0,0);

    //
    //  Resolve a degenerate normal using the interior triangle of the
    //  intermediate quadratic patch that results from recursive evaluation.
    //  This addresses common cases of degenerate or colinear boundaries
    //  without resorting to use of explicit 2nd derivatives:
    //
    if (nLength < nEpsilon) {
        float uv  = u * v * 2.0;
        float vw  = v * w * 2.0;
        float wu  = w * u * 2.0;

        float3 Q1 = ww * cv[ 1].P + wu * cv[ 2].P + uu * cv[ 3].P +
                    uv * cv[ 7].P + vv * cv[10].P + vw * cv[ 6].P;
        float3 Q3 = ww * cv[ 5].P + wu * cv[ 6].P + uu * cv[ 7].P +
                    uv * cv[10].P + vv * cv[12].P + vw * cv[ 9].P;
        float3 Q4 = ww * cv[ 6].P + wu * cv[ 7].P + uu * cv[ 8].P +
                    uv * cv[11].P + vv * cv[13].P + vw * cv[10].P;

        N = cross((Q4 - Q1), (Q3 - Q1));
        nLength = length(N);
        if (nLength < nEpsilon) {
            nLength = 1.0;
        }
    }
    N = N / nLength;
#endif
}

void
OsdEvalPatchGregoryTriangle(int3 patchParam, float2 UV, float3 cv[18],
                            out float3 P, out float3 dPu, out float3 dPv,
                            out float3 N, out float3 dNu, out float3 dNv)
{
    float u = UV.x;
    float v = UV.y;
    float w = 1.0 - u - v;

    float duv = u + v;
    float dvw = v + w;
    float dwu = w + u;

    OsdPerPatchVertexBezier bezcv[15];

    bezcv[ 6].P = (duv == 0.0) ? cv[3]  : ((u*cv[ 3] + v*cv[ 4]) / duv);
    bezcv[ 7].P = (dvw == 0.0) ? cv[8]  : ((v*cv[ 8] + w*cv[ 9]) / dvw);
    bezcv[10].P = (dwu == 0.0) ? cv[13] : ((w*cv[13] + u*cv[14]) / dwu);

    bezcv[ 0].P = cv[ 0];
    bezcv[ 1].P = cv[ 1];
    bezcv[ 2].P = cv[15];
    bezcv[ 3].P = cv[ 7];
    bezcv[ 4].P = cv[ 5];
    bezcv[ 5].P = cv[ 2];
    bezcv[ 8].P = cv[ 6];
    bezcv[ 9].P = cv[17];
    bezcv[11].P = cv[16];
    bezcv[12].P = cv[11];
    bezcv[13].P = cv[12];
    bezcv[14].P = cv[10];

    OsdEvalPatchBezierTriangle(patchParam, UV, bezcv, P, dPu, dPv, N, dNu, dNv);
}

