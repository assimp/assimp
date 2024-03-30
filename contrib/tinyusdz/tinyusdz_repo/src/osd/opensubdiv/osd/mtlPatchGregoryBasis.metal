#line 0 "osd/mtlPatchGregoryBasis.metal"

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
// Patches.GregoryBasis.Hull
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
// Patches.GregoryBasis.Factors
//----------------------------------------------------------

void OsdComputePerPatchGregoryFactors(
        int3 patchParam,
        float tessLevel,
        float4x4 projectionMatrix,
        float4x4 modelViewMatrix,
        threadgroup PatchVertexType* patchVertices,
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
    bezcv[ 0] = patchVertices[ 0].position.xyz;
    bezcv[ 1] = patchVertices[ 1].position.xyz;
    bezcv[ 2] = patchVertices[ 7].position.xyz;
    bezcv[ 3] = patchVertices[ 5].position.xyz;
    bezcv[ 4] = patchVertices[ 2].position.xyz;
    bezcv[ 5] = patchVertices[ 3].position.xyz;
    bezcv[ 6] = patchVertices[ 8].position.xyz;
    bezcv[ 7] = patchVertices[ 6].position.xyz;
    bezcv[ 8] = patchVertices[16].position.xyz;
    bezcv[ 9] = patchVertices[18].position.xyz;
    bezcv[10] = patchVertices[13].position.xyz;
    bezcv[11] = patchVertices[12].position.xyz;
    bezcv[12] = patchVertices[15].position.xyz;
    bezcv[13] = patchVertices[17].position.xyz;
    bezcv[14] = patchVertices[11].position.xyz;
    bezcv[15] = patchVertices[10].position.xyz;

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
        patchVertices,
#if !USE_PTVS_FACTORS
        osdBuffer.patchTessBuffer[patchID],
#endif
        quadFactors
        );
}

//----------------------------------------------------------
// Patches.GregoryBasis.Vertex
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
    //Does nothing, all transforms are in the PTVS
}

//----------------------------------------------------------
// Patches.GregoryBasis.Domain
//----------------------------------------------------------

#define USE_128BIT_GREGORY_BASIS_INDICES_READ 1


#if USE_STAGE_IN
template<typename PerPatchVertexGregoryBasis>
#endif
OsdPatchVertex ds_gregory_basis_patches(
        const float tessLevel,
#if !USE_PTVS_FACTORS
        float4 tessOuterLo,
        float4 tessOuterHi,
#endif
#if USE_STAGE_IN
        PerPatchVertexGregoryBasis patch,
#else
        const device OsdInputVertexType* patch,
        const device unsigned* patchIndices,
#endif
        int3 patchParam,
        float2 domainCoord
        )
{
#if USE_STAGE_IN
    float3 cv[20];
    for(int i = 0; i < 20; i++)
        cv[i] = patch[i].position;
#else
#if USE_128BIT_GREGORY_BASIS_INDICES_READ
    float3 cv[20];
    for(int i = 0; i < 5; i++) {
        int4 indices = ((device int4*)patchIndices)[i];

        int n = i * 4;
        cv[n + 0] = (patch + indices[0])->position;
        cv[n + 1] = (patch + indices[1])->position;
        cv[n + 2] = (patch + indices[2])->position;
        cv[n + 3] = (patch + indices[3])->position;
    }
#else
    float3 cv[20];
    for (int i = 0; i < 20; ++i) {
        cv[i] = patch[patchIndices[i]].position;
    }
#endif
#endif

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
    return ds_gregory_basis_patches(
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
            osdBuffers.vertexBuffer,
            osdBuffers.indexBuffer + patchID * VERTEX_CONTROL_POINTS_PER_PATCH,
            osdBuffers.patchParamBuffer[patchID],
#endif
            domainCoord
            );
}
