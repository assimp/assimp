#line 0 "osd/mtlPatchBoxSplineTriangle.metal"

//
//   Copyright 2019 Pixar
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
// Patches.BoxSplineTriangle.Hull
//----------------------------------------------------------

void OsdComputePerVertex(
        float4 position,
        threadgroup HullVertex& hullVertex,
        int vertexId,
        float4x4 modelViewProjectionMatrix,
        OsdPatchParamBufferSet osdBuffers
        )
{
    hullVertex.position = position;
#if OSD_ENABLE_PATCH_CULL
    float4 clipPos = mul(modelViewProjectionMatrix, position);
    short3 clip0 = short3(clipPos.x < clipPos.w,
                          clipPos.y < clipPos.w,
                          clipPos.z < clipPos.w);
    short3 clip1 = short3(clipPos.x > -clipPos.w,
                          clipPos.y > -clipPos.w,
                          clipPos.z > -clipPos.w);
    hullVertex.clipFlag = short3(clip0) + 2*short3(clip1);
#endif
}

//----------------------------------------------------------
// Patches.BoxSplineTriangle.Factors
//----------------------------------------------------------

void OsdComputePerPatchBezierTriangleFactors(
        int3 patchParam,
        float tessLevel,
        float4x4 projectionMatrix,
        float4x4 modelViewMatrix,
        device OsdPerPatchVertexBezier* patch,
#if !USE_PTVS_FACTORS
        device OsdPerPatchTessFactors& patchFactors,
#endif
        device MTLTriangleTessellationFactorsHalf& triFactors
        )
{
    float4 tessLevelOuter = float4(0);
    float2 tessLevelInner = float2(0);
    float4 tessOuterLo = float4(0);
    float4 tessOuterHi = float4(0);

#if OSD_ENABLE_SCREENSPACE_TESSELLATION
    // Gather bezier control points to compute limit surface tess levels
    float3 bezcv[15];
    for (int i=0; i<15; ++i) {
        bezcv[i] = patch[i].P;
    }

    OsdEvalPatchBezierTriangleTessLevels(
        tessLevel,
        projectionMatrix,
        modelViewMatrix,
        bezcv,
        patchParam,
        tessLevelOuter,
        tessLevelInner,
        tessOuterLo,
        tessOuterHi
        );
#else
    OsdGetTessLevelsUniformTriangle(
        tessLevel,
        patchParam,
        tessLevelOuter,
        tessLevelInner,
        tessOuterLo,
        tessOuterHi
        );
#endif

    triFactors.edgeTessellationFactor[0] = tessLevelOuter[0];
    triFactors.edgeTessellationFactor[1] = tessLevelOuter[1];
    triFactors.edgeTessellationFactor[2] = tessLevelOuter[2];
    triFactors.insideTessellationFactor  = tessLevelInner[0];
#if !USE_PTVS_FACTORS
    patchFactors.tessOuterLo = tessOuterLo;
    patchFactors.tessOuterHi = tessOuterHi;
#endif
}

void OsdComputePerPatchFactors(
        int3 patchParam,
        float tessLevel,
        unsigned patchID,
        float4x4 projectionMatrix,
        float4x4 modelViewMatrix,
        OsdPatchParamBufferSet osdBuffer,
        threadgroup PatchVertexType* patchVertices,
        device MTLTriangleTessellationFactorsHalf& triFactors
        )
{
    OsdComputePerPatchBezierTriangleFactors(
            patchParam,
            tessLevel,
            projectionMatrix,
            modelViewMatrix,
            osdBuffer.perPatchVertexBuffer + patchID * VERTEX_CONTROL_POINTS_PER_PATCH,
#if !USE_PTVS_FACTORS
            osdBuffer.patchTessBuffer[patchID],
#endif
            triFactors
            );
}

//----------------------------------------------------------
// Patches.BoxSplineTriangle.Vertex
//----------------------------------------------------------

void OsdComputePerPatchVertex(
        int3 patchParam,
        unsigned ID,
        unsigned PrimitiveID,
        unsigned ControlID,
        threadgroup PatchVertexType* patchVertices,
        OsdPatchParamBufferSet osdBuffers
        )
{
    OsdComputePerPatchVertexBoxSplineTriangle(
        patchParam, ID, patchVertices,
        osdBuffers.perPatchVertexBuffer[ControlID]);
}

//----------------------------------------------------------
// Patches.BoxSplineTriangle.Domain
//----------------------------------------------------------

template<typename PerPatchVertexBezier>
OsdPatchVertex ds_regular_patches(
        float tessLevel,
#if !USE_PTVS_FACTORS
        float4 tessOuterLo,
        float4 tessOuterHi,
#endif
        PerPatchVertexBezier cv,
        int3 patchParam,
        float3 domainCoord
        )
{
#if USE_PTVS_FACTORS
    float4 tessOuterLo(0), tessOuterHi(0);
    OsdGetTessLevelsUniformTriangle(
        tessLevel, patchParam, tessOuterLo, tessOuterHi);
#endif

    float2 UV = OsdGetTessParameterizationTriangle(domainCoord,
                                                   tessOuterLo,
                                                   tessOuterHi);

    OsdPatchVertex output;

    float3 P = float3(0), dPu = float3(0), dPv = float3(0);
    float3 N = float3(0), dNu = float3(0), dNv = float3(0);

    OsdEvalPatchBezierTriangle(patchParam, UV, cv, P, dPu, dPv, N, dNu, dNv);

    output.normal = N;
    output.tangent = dPu;
    output.bitangent = dPv;
#if OSD_COMPUTE_NORMAL_DERIVATIVES
    output.Nu = dNu;
    output.Nv = dNv;
#endif

    output.tessCoord = UV;
    output.patchCoord = OsdInterpolatePatchCoordTriangle(UV, patchParam);
    output.position = P;
    return output;
}

#if USE_STAGE_IN
template<typename PerPatchVertexBezier>
#endif
OsdPatchVertex OsdComputePatch(
        float tessLevel,
        float3 domainCoord,
        unsigned patchID,
#if USE_STAGE_IN
        PerPatchVertexBezier osdPatch
#else
        OsdVertexBufferSet osdBuffers
#endif
        )
{
    return ds_regular_patches(
            tessLevel,
#if !USE_PTVS_FACTORS
#if USE_STAGE_IN
            osdPatch.tessOuterLo,
            osdPatch.tessOuterHi,
#else
            osdBuffers.patchTessBuffer[patchID].tessOuterLo,
            osdBuffers.patchTessBuffer[patchID].tessOuterHi,
#endif
#endif
#if USE_STAGE_IN
            osdPatch.cv,
            osdPatch.patchParam,
#else
            osdBuffers.perPatchVertexBuffer + patchID * VERTEX_CONTROL_POINTS_PER_PATCH,
            osdBuffers.patchParamBuffer[patchID],
#endif
            domainCoord
            );
}

