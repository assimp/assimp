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
#ifdef OSD_PATCH_VERTEX_GREGORY_SHADER

layout (location=0) in vec4 position;
OSD_USER_VARYING_ATTRIBUTE_DECLARE

out block {
    OsdPerVertexGregory v;
    OSD_USER_VARYING_DECLARE
} outpt;

void main()
{
    OsdComputePerVertexGregory(gl_VertexID, position.xyz, outpt.v);
    OSD_PATCH_CULL_COMPUTE_CLIPFLAGS(position);
    OSD_USER_VARYING_PER_VERTEX();
}

#endif

//----------------------------------------------------------
// Patches.TessControlGregory
//----------------------------------------------------------
#ifdef OSD_PATCH_TESS_CONTROL_GREGORY_SHADER

patch out vec4 tessOuterLo, tessOuterHi;

in block {
    OsdPerVertexGregory v;
    OSD_USER_VARYING_DECLARE
} inpt[];

out block {
    OsdPerPatchVertexGregory v;
    OSD_USER_VARYING_DECLARE
} outpt[4];

layout(vertices = 4) out;

void main()
{
    OsdPerVertexGregory cv[4];
    for (int i=0; i<4; ++i) {
        cv[i] = inpt[i].v;
    }

    ivec3 patchParam = OsdGetPatchParam(OsdGetPatchIndex(gl_PrimitiveID));
    OsdComputePerPatchVertexGregory(patchParam, gl_InvocationID, gl_PrimitiveID, cv, outpt[gl_InvocationID].v);

    OSD_USER_VARYING_PER_CONTROL_POINT(gl_InvocationID, gl_InvocationID);

#if defined OSD_ENABLE_SCREENSPACE_TESSELLATION
    // Wait for all basis conversion to be finished
    barrier();
#endif
    if (gl_InvocationID == 0) {
        vec4 tessLevelOuter = vec4(0);
        vec2 tessLevelInner = vec2(0);

        OSD_PATCH_CULL(4);

#if defined OSD_ENABLE_SCREENSPACE_TESSELLATION
        // Gather bezier control points to compute limit surface tess levels
        OsdPerPatchVertexBezier bezcv[16];
        bezcv[ 0].P = outpt[0].v.P;
        bezcv[ 1].P = outpt[0].v.Ep;
        bezcv[ 2].P = outpt[1].v.Em;
        bezcv[ 3].P = outpt[1].v.P;
        bezcv[ 4].P = outpt[0].v.Em;
        bezcv[ 5].P = outpt[0].v.Fp;
        bezcv[ 6].P = outpt[1].v.Fm;
        bezcv[ 7].P = outpt[1].v.Ep;
        bezcv[ 8].P = outpt[3].v.Ep;
        bezcv[ 9].P = outpt[3].v.Fm;
        bezcv[10].P = outpt[2].v.Fp;
        bezcv[11].P = outpt[2].v.Em;
        bezcv[12].P = outpt[3].v.P;
        bezcv[13].P = outpt[3].v.Em;
        bezcv[14].P = outpt[2].v.Ep;
        bezcv[15].P = outpt[2].v.P;

        OsdEvalPatchBezierTessLevels(bezcv, patchParam,
                                     tessLevelOuter, tessLevelInner,
                                     tessOuterLo, tessOuterHi);
#else
        OsdGetTessLevelsUniform(patchParam, tessLevelOuter, tessLevelInner,
                         tessOuterLo, tessOuterHi);
#endif

        gl_TessLevelOuter[0] = tessLevelOuter[0];
        gl_TessLevelOuter[1] = tessLevelOuter[1];
        gl_TessLevelOuter[2] = tessLevelOuter[2];
        gl_TessLevelOuter[3] = tessLevelOuter[3];

        gl_TessLevelInner[0] = tessLevelInner[0];
        gl_TessLevelInner[1] = tessLevelInner[1];
    }
}

#endif

//----------------------------------------------------------
// Patches.TessEvalGregory
//----------------------------------------------------------
#ifdef OSD_PATCH_TESS_EVAL_GREGORY_SHADER

layout(quads) in;
layout(OSD_SPACING) in;

patch in vec4 tessOuterLo, tessOuterHi;

in block {
    OsdPerPatchVertexGregory v;
    OSD_USER_VARYING_DECLARE
} inpt[];

out block {
    OutputVertex v;
    OSD_USER_VARYING_DECLARE
} outpt;

void main()
{
    vec3 P = vec3(0), dPu = vec3(0), dPv = vec3(0);
    vec3 N = vec3(0), dNu = vec3(0), dNv = vec3(0);

    vec3 cv[20];
    cv[0] = inpt[0].v.P;
    cv[1] = inpt[0].v.Ep;
    cv[2] = inpt[0].v.Em;
    cv[3] = inpt[0].v.Fp;
    cv[4] = inpt[0].v.Fm;

    cv[5] = inpt[1].v.P;
    cv[6] = inpt[1].v.Ep;
    cv[7] = inpt[1].v.Em;
    cv[8] = inpt[1].v.Fp;
    cv[9] = inpt[1].v.Fm;

    cv[10] = inpt[2].v.P;
    cv[11] = inpt[2].v.Ep;
    cv[12] = inpt[2].v.Em;
    cv[13] = inpt[2].v.Fp;
    cv[14] = inpt[2].v.Fm;

    cv[15] = inpt[3].v.P;
    cv[16] = inpt[3].v.Ep;
    cv[17] = inpt[3].v.Em;
    cv[18] = inpt[3].v.Fp;
    cv[19] = inpt[3].v.Fm;

    vec2 UV = OsdGetTessParameterization(gl_TessCoord.xy,
                                         tessOuterLo,
                                         tessOuterHi);

    ivec3 patchParam = inpt[0].v.patchParam;
    OsdEvalPatchGregory(patchParam, UV, cv, P, dPu, dPv, N, dNu, dNv);

    // all code below here is client code
    outpt.v.position = OsdModelViewMatrix() * vec4(P, 1.0f);
    outpt.v.normal = (OsdModelViewMatrix() * vec4(N, 0.0f)).xyz;
    outpt.v.tangent = (OsdModelViewMatrix() * vec4(dPu, 0.0f)).xyz;
    outpt.v.bitangent = (OsdModelViewMatrix() * vec4(dPv, 0.0f)).xyz;
#ifdef OSD_COMPUTE_NORMAL_DERIVATIVES
    outpt.v.Nu = dNu;
    outpt.v.Nv = dNv;
#endif

    outpt.v.tessCoord = UV;
    outpt.v.patchCoord = OsdInterpolatePatchCoord(UV, patchParam);

    OSD_USER_VARYING_PER_EVAL_POINT(UV, 0, 1, 3, 2);

    OSD_DISPLACEMENT_CALLBACK;

    gl_Position = OsdProjectionMatrix() * outpt.v.position;
}

#endif
