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
// Patches.VertexGregoryTriangle
//----------------------------------------------------------

void vs_main_patches( in InputVertex input,
                      out HullVertex output )
{
    output.position = input.position;
    OSD_PATCH_CULL_COMPUTE_CLIPFLAGS(input.position);
}

//----------------------------------------------------------
// Patches.HullGregoryTriangle
//----------------------------------------------------------

[domain("tri")]
[partitioning(OSD_PARTITIONING)]
[outputtopology("triangle_cw")]
[outputcontrolpoints(18)]
[patchconstantfunc("HSConstFunc")]
OsdPerPatchVertexGregoryBasis hs_main_patches(
    in InputPatch<HullVertex, 18> patch,
    uint primitiveID : SV_PrimitiveID,
    in uint ID : SV_OutputControlPointID )
{
    OsdPerPatchVertexGregoryBasis output;

    float3 cv = patch[ID].position.xyz;

    int3 patchParam = OsdGetPatchParam(OsdGetPatchIndex(primitiveID));
    OsdComputePerPatchVertexGregoryBasis(patchParam, ID, cv, output);

    return output;
}

HS_CONSTANT_FUNC_TRIANGLE_OUT
HSConstFunc(
    InputPatch<HullVertex, 18> patch,
    uint primitiveID : SV_PrimitiveID)
{
    HS_CONSTANT_FUNC_TRIANGLE_OUT output;

    int3 patchParam = OsdGetPatchParam(OsdGetPatchIndex(primitiveID));

    float4 tessLevelOuter = float4(0,0,0,0);
    float2 tessLevelInner = float2(0,0);
    float4 tessOuterLo = float4(0,0,0,0);
    float4 tessOuterHi = float4(0,0,0,0);

    OSD_PATCH_CULL_TRIANGLE(18);

#if defined OSD_ENABLE_SCREENSPACE_TESSELLATION
    // Gather bezier control points to compute limit surface tess levels
    float3 cv[15];
    cv[ 0] = patch[ 0].position.xyz;
    cv[ 1] = patch[ 1].position.xyz;
    cv[ 2] = patch[15].position.xyz;
    cv[ 3] = patch[ 7].position.xyz;
    cv[ 4] = patch[ 5].position.xyz;
    cv[ 5] = patch[ 2].position.xyz;
    cv[ 6] = patch[ 3].position.xyz;
    cv[ 7] = patch[ 8].position.xyz;
    cv[ 8] = patch[ 6].position.xyz;
    cv[ 9] = patch[17].position.xyz;
    cv[10] = patch[13].position.xyz;
    cv[11] = patch[16].position.xyz;
    cv[12] = patch[11].position.xyz;
    cv[13] = patch[12].position.xyz;
    cv[14] = patch[10].position.xyz;

    OsdEvalPatchBezierTriangleTessLevels(
                cv, patchParam,
                tessLevelOuter, tessLevelInner,
                tessOuterLo, tessOuterHi);
#else
    OsdGetTessLevelsUniformTriangle(
                patchParam,
                tessLevelOuter, tessLevelInner,
                tessOuterLo, tessOuterHi);
#endif

    output.tessLevelOuter[0] = tessLevelOuter[0];
    output.tessLevelOuter[1] = tessLevelOuter[1];
    output.tessLevelOuter[2] = tessLevelOuter[2];

    output.tessLevelInner[0] = tessLevelInner[0];

    output.tessOuterLo = tessOuterLo;
    output.tessOuterHi = tessOuterHi;

    return output;
}

//----------------------------------------------------------
// Patches.DomainGregoryTriangle
//----------------------------------------------------------

[domain("tri")]
void ds_main_patches(
    in HS_CONSTANT_FUNC_TRIANGLE_OUT input,
    in OutputPatch<OsdPerPatchVertexBezier, 18> patch,
    in float3 domainCoord : SV_DomainLocation,
    out OutputVertex output )
{
    float3 P = float3(0,0,0), dPu = float3(0,0,0), dPv = float3(0,0,0);
    float3 N = float3(0,0,0), dNu = float3(0,0,0), dNv = float3(0,0,0);

    float3 cv[18];
    for (int i = 0; i < 18; ++i) {
        cv[i] = patch[i].P;
    }

    float2 UV = OsdGetTessParameterizationTriangle(domainCoord,
                                                   input.tessOuterLo,
                                                   input.tessOuterHi);

    int3 patchParam = patch[0].patchParam;
    OsdEvalPatchGregoryTriangle(patchParam, UV, cv, P, dPu, dPv, N, dNu, dNv);

    // all code below here is client code
    output.position = mul(OsdModelViewMatrix(), float4(P, 1.0f));
    output.normal = mul(OsdModelViewMatrix(), float4(N, 0.0f)).xyz;
    output.tangent = mul(OsdModelViewMatrix(), float4(dPu, 0.0f)).xyz;
    output.bitangent = mul(OsdModelViewMatrix(), float4(dPv, 0.0f)).xyz;
#ifdef OSD_COMPUTE_NORMAL_DERIVATIVES
    output.Nu = dNu;
    output.Nv = dNv;
#endif

    output.patchCoord = OsdInterpolatePatchCoordTriangle(UV, patchParam);

    OSD_DISPLACEMENT_CALLBACK;

    output.positionOut = mul(OsdProjectionMatrix(), output.position);
    output.edgeDistance = 0;
}
