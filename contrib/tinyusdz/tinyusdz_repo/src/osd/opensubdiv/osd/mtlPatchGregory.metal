#line 0 "osd/mtlPatchGregory.metal"

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
// Patches.Gregory.Hull
//----------------------------------------------------------

void OsdComputePerVertex(
        float4 position,
        threadgroup OsdPerVertexGregory& hullVertex,
        int vertexId,
        float4x4 modelViewProjectionMatrix,
        OsdPatchParamBufferSet osdBuffers
        )
{
    OsdComputePerVertexGregory(vertexId, position.xyz, hullVertex, osdBuffers);

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
// Patches.Gregory.Factors
//----------------------------------------------------------

void OsdComputePerPatchGregoryFactors(
        int3 patchParam,
        float tessLevel,
        float4x4 projectionMatrix,
        float4x4 modelViewMatrix,
        device OsdPerPatchVertexGregory* patchVertices,
#if !USE_PTVS_FACTORS
        device OsdPerPatchTessFactors& patchFactors,
#endif
        device MTLQuadTessellationFactorsHalf& quadFactors
        )
{
    float4 tessLevelOuter = float4(0);
    float2 tessLevelInner = float2(0);
    float4 tessOuterLo = float4(0);
    float4 tessOuterHi = float4(0);

#if OSD_ENABLE_SCREENSPACE_TESSELLATION
    // Gather bezier control points to compute limit surface tess levels
    float3 bezcv[16];
    bezcv[ 0] = patchVertices[0].P;
    bezcv[ 1] = patchVertices[0].Ep;
    bezcv[ 2] = patchVertices[1].Em;
    bezcv[ 3] = patchVertices[1].P;
    bezcv[ 4] = patchVertices[0].Em;
    bezcv[ 5] = patchVertices[0].Fp;
    bezcv[ 6] = patchVertices[1].Fm;
    bezcv[ 7] = patchVertices[1].Ep;
    bezcv[ 8] = patchVertices[3].Ep;
    bezcv[ 9] = patchVertices[3].Fm;
    bezcv[10] = patchVertices[2].Fp;
    bezcv[11] = patchVertices[2].Em;
    bezcv[12] = patchVertices[3].P;
    bezcv[13] = patchVertices[3].Em;
    bezcv[14] = patchVertices[2].Ep;
    bezcv[15] = patchVertices[2].P;

    OsdEvalPatchBezierTessLevels(
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
    OsdGetTessLevelsUniform(
        tessLevel,
        patchParam,
        tessLevelOuter,
        tessLevelInner,
        tessOuterLo,
        tessOuterHi
        );
#endif

    quadFactors.edgeTessellationFactor[0] = tessLevelOuter[0];
    quadFactors.edgeTessellationFactor[1] = tessLevelOuter[1];
    quadFactors.edgeTessellationFactor[2] = tessLevelOuter[2];
    quadFactors.edgeTessellationFactor[3] = tessLevelOuter[3];
    quadFactors.insideTessellationFactor[0] = tessLevelInner[0];
    quadFactors.insideTessellationFactor[1] = tessLevelInner[1];
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
        device MTLQuadTessellationFactorsHalf& quadFactors
        )
{
    OsdComputePerPatchGregoryFactors(
        patchParam,
        tessLevel,
        projectionMatrix,
        modelViewMatrix,
        osdBuffer.perPatchVertexBuffer + patchID * VERTEX_CONTROL_POINTS_PER_PATCH,
#if !USE_PTVS_FACTORS
        osdBuffer.patchTessBuffer[patchID],
#endif
        quadFactors
        );
}

//----------------------------------------------------------
// Patches.Gregory.Vertex
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
    OsdComputePerPatchVertexGregory(
            patchParam,
            ID,
            PrimitiveID,
            patchVertices,
            osdBuffers.perPatchVertexBuffer[ControlID],
            osdBuffers);
}

//----------------------------------------------------------
// Patches.Gregory.Domain
//----------------------------------------------------------

template<typename PerPatchVertexGregory>
OsdPatchVertex ds_gregory_patches(
        const float tessLevel,
#if !USE_PTVS_FACTORS
        float4 tessOuterLo,
        float4 tessOuterHi,
#endif
        PerPatchVertexGregory patch,
        int3 patchParam,
        float2 domainCoord
        )
{
    float3 cv[20];
    cv[0] = patch[0].P;
    cv[1] = patch[0].Ep;
    cv[2] = patch[0].Em;
    cv[3] = patch[0].Fp;
    cv[4] = patch[0].Fm;
    cv[5] = patch[1].P;
    cv[6] = patch[1].Ep;
    cv[7] = patch[1].Em;
    cv[8] = patch[1].Fp;
    cv[9] = patch[1].Fm;
    cv[10] = patch[2].P;
    cv[11] = patch[2].Ep;
    cv[12] = patch[2].Em;
    cv[13] = patch[2].Fp;
    cv[14] = patch[2].Fm;
    cv[15] = patch[3].P;
    cv[16] = patch[3].Ep;
    cv[17] = patch[3].Em;
    cv[18] = patch[3].Fp;
    cv[19] = patch[3].Fm;

#if USE_PTVS_FACTORS
    float4 tessOuterLo(0), tessOuterHi(0);
    OsdGetTessLevelsUniform(tessLevel, patchParam, tessOuterLo, tessOuterHi);
#endif

    float2 UV = OsdGetTessParameterization(domainCoord,
                                           tessOuterLo,
                                           tessOuterHi);

    OsdPatchVertex output;

    float3 P = float3(0), dPu = float3(0), dPv = float3(0);
    float3 N = float3(0), dNu = float3(0), dNv = float3(0);

    OsdEvalPatchGregory(patchParam, UV, cv, P, dPu, dPv, N, dNu, dNv);

    // all code below here is client code
    output.position = P;
    output.normal = N;
    output.tangent = dPu;
    output.bitangent = dPv;
#if OSD_COMPUTE_NORMAL_DERIVATIVES
    output.Nu = dNu;
    output.Nv = dNv;
#endif

    output.tessCoord = UV;
    output.patchCoord = OsdInterpolatePatchCoord(UV, patchParam);

    return output;
}

#if USE_STAGE_IN
template<typename PerPatchVertexGregoryBasis>
#endif
OsdPatchVertex OsdComputePatch(
        float tessLevel,
        float2 domainCoord,
        unsigned patchID,
#if USE_STAGE_IN
        PerPatchVertexGregoryBasis osdPatch
#else
        OsdVertexBufferSet osdBuffers
#endif
        )
{
    return ds_gregory_patches(
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
            domainCoord);
}
