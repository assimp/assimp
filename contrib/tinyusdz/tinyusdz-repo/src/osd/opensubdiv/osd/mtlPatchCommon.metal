#line 0 "osd/mtlPatchCommon.metal"

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

//----------------------------------------------------------
// Patches.Common
//----------------------------------------------------------

#include <metal_stdlib>

#define offsetof_(X, Y) &(((device X*)nullptr)->Y)

#define OSD_IS_ADAPTIVE (OSD_PATCH_REGULAR || OSD_PATCH_BOX_SPLINE_TRIANGLE || OSD_PATCH_GREGORY_BASIS || OSD_PATCH_GREGORY_TRIANGLE || OSD_PATCH_GREGORY || OSD_PATCH_GREGORY_BOUNDARY)

#ifndef OSD_MAX_TESS_LEVEL
#define OSD_MAX_TESS_LEVEL 64
#endif

#ifndef OSD_NUM_ELEMENTS
#define OSD_NUM_ELEMENTS 3
#endif

using namespace metal;

using OsdPatchParamBufferType = packed_int3;

struct OsdPerVertexGregory {
    float3 P;
    short3 clipFlag;
    int valence;
    float3 e0;
    float3 e1;
#if OSD_PATCH_GREGORY_BOUNDARY
    int zerothNeighbor;
    float3 org;
#endif
    float3 r[OSD_MAX_VALENCE];
};

struct OsdPerPatchVertexGregory {
    packed_float3 P;
    packed_float3 Ep;
    packed_float3 Em;
    packed_float3 Fp;
    packed_float3 Fm;
};

//----------------------------------------------------------
// HLSL->Metal Compatibility
//----------------------------------------------------------

float4 mul(float4x4 a, float4 b)
{
    return a * b;
}

float3 mul(float4x4 a, float3 b)
{
    float3x3 m(a[0].xyz, a[1].xyz, a[2].xyz);
    return m * b;

}

//----------------------------------------------------------
// Patches.Common
//----------------------------------------------------------

struct HullVertex {
    float4 position;
#if OSD_ENABLE_PATCH_CULL
    short3 clipFlag;
#endif

    float3 GetPosition() threadgroup
    {
        return position.xyz;
    }

    void SetPosition(float3 v) threadgroup
    {
        position.xyz = v;
    }
};

// XXXdyu all downstream data can be handled by client code
struct OsdPatchVertex {
    float3 position;
    float3 normal;
    float3 tangent;
    float3 bitangent;
    float4 patchCoord; //u, v, faceLevel, faceId
    float2 tessCoord; // tesscoord.st
#if OSD_COMPUTE_NORMAL_DERIVATIVES
    float3 Nu;
    float3 Nv;
#endif
#if OSD_PATCH_ENABLE_SINGLE_CREASE
    float2 vSegments;
#endif
};

struct OsdPerPatchTessFactors {
    float4 tessOuterLo;
    float4 tessOuterHi;
};

struct OsdPerPatchVertexBezier {
    packed_float3 P;
#if OSD_PATCH_ENABLE_SINGLE_CREASE
    packed_float3 P1;
    packed_float3 P2;
#if !USE_PTVS_SHARPNESS
    float2 vSegments;
#endif
#endif
};

struct OsdPerPatchVertexGregoryBasis {
    packed_float3 P;
};

#if OSD_PATCH_REGULAR || OSD_PATCH_BOX_SPLINE_TRIANGLE
using PatchVertexType = HullVertex;
using PerPatchVertexType = OsdPerPatchVertexBezier;
#elif OSD_PATCH_GREGORY || OSD_PATCH_GREGORY_BOUNDARY
using PatchVertexType = OsdPerVertexGregory;
using PerPatchVertexType = OsdPerPatchVertexGregory;
#elif OSD_PATCH_GREGORY_BASIS || OSD_PATCH_GREGORY_TRIANGLE
using PatchVertexType = HullVertex;
using PerPatchVertexType = OsdPerPatchVertexGregoryBasis;
#else
using PatchVertexType = OsdInputVertexType;
using PerPatchVertexType = OsdInputVertexType;
#endif

//Shared buffers used by OSD that are common to all kernels
struct OsdPatchParamBufferSet
{
    const device OsdInputVertexType* vertexBuffer [[buffer(VERTEX_BUFFER_INDEX)]];
    const device unsigned* indexBuffer [[buffer(CONTROL_INDICES_BUFFER_INDEX)]];

    const device OsdPatchParamBufferType* patchParamBuffer [[buffer(OSD_PATCHPARAM_BUFFER_INDEX)]];

    device PerPatchVertexType* perPatchVertexBuffer [[buffer(OSD_PERPATCHVERTEX_BUFFER_INDEX)]];

#if !USE_PTVS_FACTORS
    device OsdPerPatchTessFactors* patchTessBuffer [[buffer(OSD_PERPATCHTESSFACTORS_BUFFER_INDEX)]];
#endif

#if OSD_PATCH_GREGORY || OSD_PATCH_GREGORY_BOUNDARY
    const device int* quadOffsetBuffer [[buffer(OSD_QUADOFFSET_BUFFER_INDEX)]];
    const device int* valenceBuffer [[buffer(OSD_VALENCE_BUFFER_INDEX)]];
#endif

    const constant unsigned& kernelExecutionLimit [[buffer(OSD_KERNELLIMIT_BUFFER_INDEX)]];
};

//Shared buffers used by OSD that are common to all PTVS implementations
struct OsdVertexBufferSet
{
    const device OsdInputVertexType* vertexBuffer [[buffer(VERTEX_BUFFER_INDEX)]];
    const device unsigned* indexBuffer [[buffer(CONTROL_INDICES_BUFFER_INDEX)]];

    const device OsdPatchParamBufferType* patchParamBuffer [[buffer(OSD_PATCHPARAM_BUFFER_INDEX)]];

    device PerPatchVertexType* perPatchVertexBuffer [[buffer(OSD_PERPATCHVERTEX_BUFFER_INDEX)]];

#if !USE_PTVS_FACTORS
    device OsdPerPatchTessFactors* patchTessBuffer [[buffer(OSD_PERPATCHTESSFACTORS_BUFFER_INDEX)]];
#endif
};

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

int3 OsdGetPatchParam(int patchIndex, const device OsdPatchParamBufferType* osdPatchParamBuffer)
{
#if OSD_PATCH_ENABLE_SINGLE_CREASE
    return int3(osdPatchParamBuffer[patchIndex]);
#else
    auto p = osdPatchParamBuffer[patchIndex];
    return int3(p[0], p[1], 0);
#endif
}

int OsdGetPatchIndex(int primitiveId)
{
    return primitiveId;
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
    return as_type<float>(patchParam.z);
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

bool OsdCullPerPatchVertex(
        threadgroup PatchVertexType* patch,
        float4x4 ModelViewMatrix
        )
{
#if OSD_ENABLE_BACKPATCH_CULL && OSD_PATCH_REGULAR
    auto v0 = float3(ModelViewMatrix * patch[5].position);
    auto v3 = float3(ModelViewMatrix * patch[6].position);
    auto v12 = float3(ModelViewMatrix * patch[9].position);

    auto n = normalize(cross(v3 - v0, v12 - v0));
    v0 = normalize(v0 + v3 + v12);

    if(dot(v0, n) > 0.6f)
    {
        return false;
    }
#endif
#if OSD_ENABLE_PATCH_CULL
    short3 clipFlag = short3(0,0,0);
    for(int i = 0; i < CONTROL_POINTS_PER_PATCH; ++i) {
        clipFlag |= patch[i].clipFlag;
    }
    if (any(clipFlag != short3(3,3,3))) {
        return false;
    }
#endif
    return true;
}

// ----------------------------------------------------------------------------

void
OsdUnivar4x4(float u, thread float* B)
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
}

void
OsdUnivar4x4(float u, thread float* B, thread float* D)
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
OsdUnivar4x4(float u, thread float* B, thread float* D, thread float* C)
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
OsdEvalBezier(device OsdPerPatchVertexBezier* cp, int3 patchParam, float2 uv)
{
    float3 BUCP[4] = {float3(0,0,0),float3(0,0,0),float3(0,0,0),float3(0,0,0)};

    float B[4], D[4];
    float s = OsdGetPatchSingleCreaseSegmentParameter(patchParam, uv);

    OsdUnivar4x4(uv.x, B, D);
#if OSD_PATCH_ENABLE_SINGLE_CREASE
#if USE_PTVS_SHARPNESS
    float sharpness = OsdGetPatchSharpness(patchParam);
    float Sf = floor(sharpness);
    float Sc = ceil(sharpness);
    float s0 = 1 - exp2(-Sf);
    float s1 = 1 - exp2(-Sc);

    float2 vSegments(s0, s1);
#else
    float2 vSegments = cp[0].vSegments;
#endif // USE_PTVS_SHARPNESS

    //By doing the offset calculation ahead of time it can be kept out of the actual indexing lookup.

    if(s <= vSegments.x)
        cp = (device OsdPerPatchVertexBezier*)(((device float*)cp) + 0);
    else if( s <= vSegments.y)
        cp = (device OsdPerPatchVertexBezier*)(((device float*)cp) + 3);
    else
        cp = (device OsdPerPatchVertexBezier*)(((device float*)cp) + 6);

    BUCP[0] += cp[0].P * B[0];
    BUCP[0] += cp[1].P * B[1];
    BUCP[0] += cp[2].P * B[2];
    BUCP[0] += cp[3].P * B[3];

    BUCP[1] += cp[4].P * B[0];
    BUCP[1] += cp[5].P * B[1];
    BUCP[1] += cp[6].P * B[2];
    BUCP[1] += cp[7].P * B[3];

    BUCP[2] += cp[8].P * B[0];
    BUCP[2] += cp[9].P * B[1];
    BUCP[2] += cp[10].P * B[2];
    BUCP[2] += cp[11].P * B[3];

    BUCP[3] += cp[12].P * B[0];
    BUCP[3] += cp[13].P * B[1];
    BUCP[3] += cp[14].P * B[2];
    BUCP[3] += cp[15].P * B[3];

#else // single crease
    for (int i=0; i<4; ++i) {
        for (int j=0; j<4; ++j) {
            float3 A = cp[4*i + j].P;
            BUCP[i] += A * B[j];
        }
    }
#endif  // single crease

    OsdUnivar4x4(uv.y, B);
    float3 P = B[0] * BUCP[0];
    for (int k=1; k<4; ++k) {
        P += B[k] * BUCP[k];
    }

    return P;
}

// ----------------------------------------------------------------------------
// Boundary Interpolation
// ----------------------------------------------------------------------------

template<typename VertexType>
void
OsdComputeBSplineBoundaryPoints(threadgroup VertexType* cpt, int3 patchParam)
{
    //APPL TODO - multithread this
    int boundaryMask = OsdGetPatchBoundaryMask(patchParam);

    //  Don't extrapolate corner points until all boundary points in place
    if ((boundaryMask & 1) != 0) {
        cpt[1].SetPosition(2*cpt[5].GetPosition() - cpt[9].GetPosition());
        cpt[2].SetPosition(2*cpt[6].GetPosition() - cpt[10].GetPosition());
    }
    if ((boundaryMask & 2) != 0) {
        cpt[7].SetPosition(2*cpt[6].GetPosition() - cpt[5].GetPosition());
        cpt[11].SetPosition(2*cpt[10].GetPosition() - cpt[9].GetPosition());
    }
    if ((boundaryMask & 4) != 0) {
        cpt[13].SetPosition(2*cpt[9].GetPosition() - cpt[5].GetPosition());
        cpt[14].SetPosition(2*cpt[10].GetPosition() - cpt[6].GetPosition());
    }
    if ((boundaryMask & 8) != 0) {
        cpt[4].SetPosition(2*cpt[5].GetPosition() - cpt[6].GetPosition());
        cpt[8].SetPosition(2*cpt[9].GetPosition() - cpt[10].GetPosition());
    }

    //  Now safe to extrapolate corner points:
    if ((boundaryMask & 1) != 0) {
        cpt[0].SetPosition(2*cpt[4].GetPosition() - cpt[8].GetPosition());
        cpt[3].SetPosition(2*cpt[7].GetPosition() - cpt[11].GetPosition());
    }
    if ((boundaryMask & 2) != 0) {
        cpt[3].SetPosition(2*cpt[2].GetPosition() - cpt[1].GetPosition());
        cpt[15].SetPosition(2*cpt[14].GetPosition() - cpt[13].GetPosition());
    }
    if ((boundaryMask & 4) != 0) {
        cpt[12].SetPosition(2*cpt[8].GetPosition() - cpt[4].GetPosition());
        cpt[15].SetPosition(2*cpt[11].GetPosition() - cpt[7].GetPosition());
    }
    if ((boundaryMask & 8) != 0) {
        cpt[0].SetPosition(2*cpt[1].GetPosition() - cpt[2].GetPosition());
        cpt[12].SetPosition(2*cpt[13].GetPosition() - cpt[14].GetPosition());
    }
}

template<typename VertexType>
void
OsdComputeBoxSplineTriangleBoundaryPoints(thread VertexType* cpt, int3 patchParam)
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
            cpt[0].SetPosition(cpt[4].GetPosition() + (cpt[4].GetPosition() - cpt[8].GetPosition()));
        } else {
            cpt[0].SetPosition(cpt[4].GetPosition() + (cpt[3].GetPosition() - cpt[7].GetPosition()));
        }
        cpt[1].SetPosition(cpt[4].GetPosition() + cpt[5].GetPosition() - cpt[8].GetPosition());
        if (edge1IsBoundary) {
            cpt[2].SetPosition(cpt[5].GetPosition() + (cpt[5].GetPosition() - cpt[8].GetPosition()));
        } else {
            cpt[2].SetPosition(cpt[5].GetPosition() + (cpt[6].GetPosition() - cpt[9].GetPosition()));
        }
    }
    if (edge1IsBoundary) {
        if (edge0IsBoundary) {
            cpt[6].SetPosition(cpt[5].GetPosition() + (cpt[5].GetPosition() - cpt[4].GetPosition()));
        } else {
            cpt[6].SetPosition(cpt[5].GetPosition() + (cpt[2].GetPosition() - cpt[1].GetPosition()));
        }
        cpt[9].SetPosition(cpt[5].GetPosition() + cpt[8].GetPosition() - cpt[4].GetPosition());
        if (edge2IsBoundary) {
            cpt[11].SetPosition(cpt[8].GetPosition() + (cpt[8].GetPosition() - cpt[4].GetPosition()));
        } else {
            cpt[11].SetPosition(cpt[8].GetPosition() + (cpt[10].GetPosition() - cpt[7].GetPosition()));
        }
    }
    if (edge2IsBoundary) {
        if (edge1IsBoundary) {
            cpt[10].SetPosition(cpt[8].GetPosition() + (cpt[8].GetPosition() - cpt[5].GetPosition()));
        } else {
            cpt[10].SetPosition(cpt[8].GetPosition() + (cpt[11].GetPosition() - cpt[9].GetPosition()));
        }
        cpt[7].SetPosition(cpt[8].GetPosition() + cpt[4].GetPosition() - cpt[5].GetPosition());
        if (edge0IsBoundary) {
            cpt[3].SetPosition(cpt[4].GetPosition() + (cpt[4].GetPosition() - cpt[5].GetPosition()));
        } else {
            cpt[3].SetPosition(cpt[4].GetPosition() + (cpt[0].GetPosition() - cpt[1].GetPosition()));
        }
    }

    if ((vBits & 1) != 0) {
        cpt[3].SetPosition(cpt[4].GetPosition() + cpt[7].GetPosition() - cpt[8].GetPosition());
        cpt[0].SetPosition(cpt[4].GetPosition() + cpt[1].GetPosition() - cpt[5].GetPosition());
    }
    if ((vBits & 2) != 0) {
        cpt[2].SetPosition(cpt[5].GetPosition() + cpt[1].GetPosition() - cpt[4].GetPosition());
        cpt[6].SetPosition(cpt[5].GetPosition() + cpt[9].GetPosition() - cpt[8].GetPosition());
    }
    if ((vBits & 4) != 0) {
        cpt[11].SetPosition(cpt[8].GetPosition() + cpt[9].GetPosition() - cpt[5].GetPosition());
        cpt[10].SetPosition(cpt[8].GetPosition() + cpt[7].GetPosition() - cpt[4].GetPosition());
    }
}

// ----------------------------------------------------------------------------
// BSpline
// ----------------------------------------------------------------------------

// compute single-crease patch matrix
float4x4
OsdComputeMs(float sharpness)
{
    float s = exp2(sharpness);
    float s2 = s*s;
    float s3 = s2*s;

    float4x4 m(
        float4(0, s + 1 + 3*s2 - s3, 7*s - 2 - 6*s2 + 2*s3, (1-s)*(s-1)*(s-1)),
        float4(0,       (1+s)*(1+s),        6*s - 2 - 2*s2,       (s-1)*(s-1)),
        float4(0,               1+s,               6*s - 2,               1-s),
        float4(0,                 1,               6*s - 2,                 1));

    m[0] /= (s*6.0);
    m[1] /= (s*6.0);
    m[2] /= (s*6.0);
    m[3] /= (s*6.0);

    m[0][0] = 1.0/6.0;

    return m;
}

float4x4
OsdComputeMs2(float sharpness, float factor)
{
    float s = exp2(sharpness);
    float s2 = s*s;
    float s3 = s2*s;
    float sx6 = s*6.0;
    float sx6m2 = sx6 - 2;
    float sfrac1 = 1-s;
    float ssub1 = s-1;
    float ssub1_2 = ssub1 * ssub1;
    float div6 = 1.0/6.0;

    float4x4 m(
               float4(0, s + 1 + 3*s2 - s3, 7*s - 2 - 6*s2 + 2*s3,    sfrac1 * ssub1_2),
               float4(0,      1 + 2*s + s2,         sx6m2 - 2*s2,             ssub1_2),
               float4(0,               1+s,                sx6m2,              sfrac1),
               float4(0,                 1,                sx6m2,                 1));

    m *= factor * (1/sx6);

    m[0][0] = div6 * factor;

    return m;
}

// flip matrix orientation
void OsdFlipMatrix(threadgroup float * src, threadgroup float * dst)
{
    for (int i = 0; i < 16; i++) dst[i] = src[15-i];
}

float4x4 OsdFlipMatrix(float4x4 m)
{
    return float4x4(float4(m[3][3], m[3][2], m[3][1], m[3][0]),
                    float4(m[2][3], m[2][2], m[2][1], m[2][0]),
                    float4(m[1][3], m[1][2], m[1][1], m[1][0]),
                    float4(m[0][3], m[0][2], m[0][1], m[0][0]));
}

// Regular BSpline to Bezier
constant float4x4 Q(
                    float4(1.f/6.f, 4.f/6.f, 1.f/6.f, 0.f),
                    float4(0.f,     4.f/6.f, 2.f/6.f, 0.f),
                    float4(0.f,     2.f/6.f, 4.f/6.f, 0.f),
                    float4(0.f,     1.f/6.f, 4.f/6.f, 1.f/6.f)
                    );

// Infinitely Sharp (boundary)
constant float4x4 Mi(
                     float4(1.f/6.f, 4.f/6.f, 1.f/6.f, 0.f),
                     float4(0.f,     4.f/6.f, 2.f/6.f, 0.f),
                     float4(0.f,     2.f/6.f, 4.f/6.f, 0.f),
                     float4(0.f,     0.f,     1.f,     0.f)
                     );

// convert BSpline cv to Bezier cv
template<typename VertexType> //VertexType should be some type that implements float3 VertexType::GetPosition()
void
OsdComputePerPatchVertexBSpline(
        int3 patchParam, unsigned ID,
        threadgroup VertexType* cv,
        device OsdPerPatchVertexBezier& result)
{
    int i = ID%4;
    int j = ID/4;

#if OSD_PATCH_ENABLE_SINGLE_CREASE

    float3 P  = float3(0,0,0); // 0 to 1-2^(-Sf)
    float3 P1 = float3(0,0,0); // 1-2^(-Sf) to 1-2^(-Sc)
    float3 P2 = float3(0,0,0); // 1-2^(-Sc) to 1
    float sharpness = OsdGetPatchSharpness(patchParam);

    int boundaryMask = OsdGetPatchBoundaryMask(patchParam);

    if (sharpness > 0 && (boundaryMask & 15))
    {
        float Sf = floor(sharpness);
        float Sc = ceil(sharpness);
        float Sr = fract(sharpness);

        float4x4 Mj = OsdComputeMs2(Sf, 1-Sr);
        float4x4 Ms = Mj;
        Mj += (Sr * Mi);
        Ms += OsdComputeMs2(Sc, Sr);

#if USE_PTVS_SHARPNESS
#else
        float s0 = 1 - exp2(-Sf);
        float s1 = 1 - exp2(-Sc);
        result.vSegments = float2(s0, s1);
#endif

        bool isBoundary[2];
        isBoundary[0] = (((boundaryMask & 8) != 0) || ((boundaryMask & 2) != 0)) ? true : false;
        isBoundary[1] = (((boundaryMask & 4) != 0) || ((boundaryMask & 1) != 0)) ? true : false;
        bool needsFlip[2];
        needsFlip[0] = (boundaryMask & 8) ? true : false;
        needsFlip[1] = (boundaryMask & 1) ? true : false;
        float3 Hi[4], Hj[4], Hs[4];

        if (isBoundary[0])
        {
            int t[4] = {0,1,2,3};
            int ti = i, step = 1, start = 0;
            if (needsFlip[0]) {
                t[0] = 3; t[1] = 2; t[2] = 1; t[3] = 0;
                ti = 3-i;
                start = 3; step = -1;
            }
            for (int l=0; l<4; ++l) {
                Hi[l] = Hj[l] = Hs[l] = float3(0,0,0);
                for (int k=0, tk = start; k<4; ++k, tk+=step) {
                    float3 p = cv[l*4 + k].GetPosition();
                    Hi[l] += Mi[ti][tk] * p;
                    Hj[l] += Mj[ti][tk] * p;
                    Hs[l] += Ms[ti][tk] * p;
                }
            }
        }
        else
        {
            for (int l=0; l<4; ++l) {
                Hi[l] = Hj[l] = Hs[l] = float3(0,0,0);
                for (int k=0; k<4; ++k) {
                    float3 p = cv[l*4 + k].GetPosition();
                    float3 val = Q[i][k] * p;
                    Hi[l] += val;
                    Hj[l] += val;
                    Hs[l] += val;
                }
            }
        }
        {
            int t[4] = {0,1,2,3};
            int tj = j, step = 1, start = 0;
            if (needsFlip[1]) {
                t[0] = 3; t[1] = 2; t[2] = 1; t[3] = 0;
                tj = 3-j;
                start = 3; step = -1;
            }
            for (int k=0, tk = start; k<4; ++k, tk+=step) {
                if (isBoundary[1])
                {
                    P  += Mi[tj][tk]*Hi[k];
                    P1 += Mj[tj][tk]*Hj[k];
                    P2 += Ms[tj][tk]*Hs[k];
                }
                else
                {
                    P  += Q[j][k]*Hi[k];
                    P1 += Q[j][k]*Hj[k];
                    P2 += Q[j][k]*Hs[k];
                }
            }
        }

    result.P  = P;
    result.P1 = P1;
    result.P2 = P2;
    } else {
#if USE_PTVS_SHARPNESS
#else
        result.vSegments = float2(0, 0);
#endif

        OsdComputeBSplineBoundaryPoints(cv, patchParam);

    float3 Hi[4];
    for (int l=0; l<4; ++l) {
        Hi[l] = float3(0,0,0);
        for (int k=0; k<4; ++k) {
            Hi[l] += Q[i][k] * cv[l*4 + k].GetPosition();
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
            H[l] += Q[i][k] * (cv + l*4 + k)->GetPosition();
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

template<typename PerPatchVertexBezier>
void
OsdEvalPatchBezier(int3 patchParam, float2 UV,
                   PerPatchVertexBezier cv,
                   thread float3& P, thread float3& dPu, thread float3& dPv,
                   thread float3& N, thread float3& dNu, thread float3& dNv,
                   thread float2& vSegments)
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
#if OSD_PATCH_ENABLE_SINGLE_CREASE
#if USE_PTVS_SHARPNESS
    float sharpness = OsdGetPatchSharpness(patchParam);
    float Sf = floor(sharpness);
    float Sc = ceil(sharpness);
    float s0 = 1 - exp2(-Sf);
    float s1 = 1 - exp2(-Sc);
    vSegments = float2(s0, s1);
#else // USE_PTVS_SHARPNESS
    vSegments = cv[0].vSegments;
#endif // USE_PTVS_SHARPNESS
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
#else
    LROW[0] = u0 * cv[ 0].P + u1 * cv[ 1].P + u2 * cv[ 2].P;
    LROW[1] = u0 * cv[ 4].P + u1 * cv[ 5].P + u2 * cv[ 6].P;
    LROW[2] = u0 * cv[ 8].P + u1 * cv[ 9].P + u2 * cv[10].P;
    LROW[3] = u0 * cv[12].P + u1 * cv[13].P + u2 * cv[14].P;

    RROW[0] = u0 * cv[ 1].P + u1 * cv[ 2].P + u2 * cv[ 3].P;
    RROW[1] = u0 * cv[ 5].P + u1 * cv[ 6].P + u2 * cv[ 7].P;
    RROW[2] = u0 * cv[ 9].P + u1 * cv[10].P + u2 * cv[11].P;
    RROW[3] = u0 * cv[13].P + u1 * cv[14].P + u2 * cv[15].P;
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
#if OSD_PATCH_ENABLE_SINGLE_CREASE
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

void
OsdComputePerPatchVertexGregoryBasis(int3 patchParam, int ID, float3 cv,
                                     device OsdPerPatchVertexGregoryBasis& result)
{
    result.P = cv;
}

void
OsdEvalPatchGregory(int3 patchParam, float2 UV, thread float3* cv,
                    thread float3& P, thread float3& dPu, thread float3& dPv,
                    thread float3& N, thread float3& dNu, thread float3& dNv)
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
    float2 vSegments;

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

    OsdEvalPatchBezier(patchParam, UV, bezcv, P, dPu, dPv, N, dNu, dNv, vSegments);
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
template<typename VertexType>
void
OsdComputePerPatchVertexBoxSplineTriangle(
        int3 patchParam, int ID,
        threadgroup VertexType* cv,
        device OsdPerPatchVertexBezier& result)
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

    //result.patchParam = patchParam;
    result.P = float3(0);

    int cvCoeffBase = 12 * ID;

    for (int i = 0; i < 12; ++i) {
        result.P += boxToBezierMatrix[cvCoeffBase + i] * cv[i].GetPosition();
    }
    result.P *= boxToBezierMatrixScale;
}

template<typename PerPatchVertexBezier>
void
OsdEvalPatchBezierTriangle(int3 patchParam, float2 UV,
                       PerPatchVertexBezier cv,
                       thread float3& P, thread float3& dPu, thread float3& dPv,
                       thread float3& N, thread float3& dNu, thread float3& dNv)
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
    dNu = float3(0);
    dNv = float3(0);

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
                        thread float3& P, thread float3& dPu, thread float3& dPv,
                        thread float3& N, thread float3& dNu, thread float3& dNv)
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
