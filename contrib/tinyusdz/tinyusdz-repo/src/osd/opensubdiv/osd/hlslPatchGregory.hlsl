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
// Patches.VertexGregory
//----------------------------------------------------------

void vs_main_patches( in InputVertex input,
                      uint vID : SV_VertexID,
                      out OsdPerVertexGregory output )
{
    OsdComputePerVertexGregory(vID, input.position, output);
    OSD_PATCH_CULL_COMPUTE_CLIPFLAGS(input.position);
}

//----------------------------------------------------------
// Patches.HullGregory
//----------------------------------------------------------

[domain("quad")]
[partitioning(OSD_PARTITIONING)]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("HSConstFunc")]
OsdPerPatchVertexGregory hs_main_patches(
    in InputPatch<OsdPerVertexGregory, 4> patch,
    uint primitiveID : SV_PrimitiveID,
    in uint ID : SV_OutputControlPointID )
{
    OsdPerPatchVertexGregory output;

    OsdPerVertexGregory cv[4];
    for (int i=0; i<4; ++i) {
        cv[i] = patch[i];
    }

    int3 patchParam = OsdGetPatchParam(OsdGetPatchIndex(primitiveID));
    OsdComputePerPatchVertexGregory(patchParam, ID, primitiveID, cv, output);

    return output;
}

HS_CONSTANT_FUNC_OUT HSConstFunc(
    InputPatch<OsdPerVertexGregory, 4> patch,
    OutputPatch<OsdPerPatchVertexGregory, 4> gregoryPatch,
    uint primitiveID : SV_PrimitiveID)
{
    HS_CONSTANT_FUNC_OUT output;

    int3 patchParam = OsdGetPatchParam(OsdGetPatchIndex(primitiveID));

    float4 tessLevelOuter = float4(0,0,0,0);
    float2 tessLevelInner = float2(0,0);
    float4 tessOuterLo = float4(0,0,0,0);
    float4 tessOuterHi = float4(0,0,0,0);

    OSD_PATCH_CULL(4);

#if defined OSD_ENABLE_SCREENSPACE_TESSELLATION
    // Gather bezier control points to compute limit surface tess levels
    OsdPerPatchVertexBezier bezcv[16];
    bezcv[ 0].P = gregoryPatch[0].P;
    bezcv[ 1].P = gregoryPatch[0].Ep;
    bezcv[ 2].P = gregoryPatch[1].Em;
    bezcv[ 3].P = gregoryPatch[1].P;
    bezcv[ 4].P = gregoryPatch[0].Em;
    bezcv[ 5].P = gregoryPatch[0].Fp;
    bezcv[ 6].P = gregoryPatch[1].Fm;
    bezcv[ 7].P = gregoryPatch[1].Ep;
    bezcv[ 8].P = gregoryPatch[3].Ep;
    bezcv[ 9].P = gregoryPatch[3].Fm;
    bezcv[10].P = gregoryPatch[2].Fp;
    bezcv[11].P = gregoryPatch[2].Em;
    bezcv[12].P = gregoryPatch[3].P;
    bezcv[13].P = gregoryPatch[3].Em;
    bezcv[14].P = gregoryPatch[2].Ep;
    bezcv[15].P = gregoryPatch[2].P;

    OsdEvalPatchBezierTessLevels(bezcv, patchParam,
                                 tessLevelOuter, tessLevelInner,
                                 tessOuterLo, tessOuterHi);
#else
    OsdGetTessLevelsUniform(patchParam, tessLevelOuter, tessLevelInner,
                     tessOuterLo, tessOuterHi);

#endif

    output.tessLevelOuter[0] = tessLevelOuter[0];
    output.tessLevelOuter[1] = tessLevelOuter[1];
    output.tessLevelOuter[2] = tessLevelOuter[2];
    output.tessLevelOuter[3] = tessLevelOuter[3];

    output.tessLevelInner[0] = tessLevelInner[0];
    output.tessLevelInner[1] = tessLevelInner[1];

    output.tessOuterLo = tessOuterLo;
    output.tessOuterHi = tessOuterHi;


    return output;
}

//----------------------------------------------------------
// Patches.DomainGregory
//----------------------------------------------------------

[domain("quad")]
void ds_main_patches(
    in HS_CONSTANT_FUNC_OUT input,
    in OutputPatch<OsdPerPatchVertexGregory, 4> patch,
    in float2 domainCoord : SV_DomainLocation,
    out OutputVertex output )
{
    float3 P = float3(0,0,0), dPu = float3(0,0,0), dPv = float3(0,0,0);
    float3 N = float3(0,0,0), dNu = float3(0,0,0), dNv = float3(0,0,0);

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

    float2 UV = OsdGetTessParameterization(domainCoord,
                                           input.tessOuterLo,
                                           input.tessOuterHi);

    int3 patchParam = patch[0].patchParam;
    OsdEvalPatchGregory(patchParam, UV, cv, P, dPu, dPv, N, dNu, dNv);

    // all code below here is client code
    output.position = mul(OsdModelViewMatrix(), float4(P, 1.0f));
    output.normal = mul(OsdModelViewMatrix(), float4(N, 0.0f)).xyz;
    output.tangent = mul(OsdModelViewMatrix(), float4(dPu, 0.0f)).xyz;
    output.bitangent = mul(OsdModelViewMatrix(), float4(dPv, 0.0f)).xyz;
#ifdef OSD_COMPUTE_NORMAL_DERIVATIVES
    output.Nu = dNu;
    output.Nv = dNv;
#endif

    output.patchCoord = OsdInterpolatePatchCoord(UV, patchParam);

    OSD_DISPLACEMENT_CALLBACK;

    output.positionOut = mul(OsdProjectionMatrix(), output.position);
    output.edgeDistance = 0;
}
