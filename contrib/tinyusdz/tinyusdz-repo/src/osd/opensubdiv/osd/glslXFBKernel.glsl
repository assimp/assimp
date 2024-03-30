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

//------------------------------------------------------------------------------

uniform samplerBuffer vertexBuffer;
uniform int srcOffset = 0;
out float outVertexBuffer[LENGTH];

//------------------------------------------------------------------------------

struct Vertex {
    float vertexData[LENGTH];
};

void clear(out Vertex v) {
    for (int i = 0; i < LENGTH; i++) {
        v.vertexData[i] = 0;
    }
}

void addWithWeight(inout Vertex v, Vertex src, float weight) {
    for(int j = 0; j < LENGTH; j++) {
        v.vertexData[j] += weight * src.vertexData[j];
    }
}

Vertex readVertex(int index) {
    Vertex v;
    int vertexIndex = srcOffset + index * SRC_STRIDE;
    for(int j = 0; j < LENGTH; j++) {
        v.vertexData[j] = texelFetch(vertexBuffer, vertexIndex+j).x;
    }
    return v;
}

void writeVertex(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outVertexBuffer[i] = v.vertexData[i];
    }
}

//------------------------------------------------------------------------------

#if defined(OPENSUBDIV_GLSL_XFB_USE_1ST_DERIVATIVES) && \
    defined(OPENSUBDIV_GLSL_XFB_INTERLEAVED_1ST_DERIVATIVE_BUFFERS)
out float outDeriv1Buffer[2*LENGTH];

void writeDu(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outDeriv1Buffer[i] = v.vertexData[i];
    }
}

void writeDv(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outDeriv1Buffer[i+LENGTH] = v.vertexData[i];
    }
}
#elif defined(OPENSUBDIV_GLSL_XFB_USE_1ST_DERIVATIVES)
out float outDuBuffer[LENGTH];
out float outDvBuffer[LENGTH];

void writeDu(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outDuBuffer[i] = v.vertexData[i];
    }
}

void writeDv(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outDvBuffer[i] = v.vertexData[i];
    }
}
#endif

#if defined(OPENSUBDIV_GLSL_XFB_USE_2ND_DERIVATIVES) && \
    defined(OPENSUBDIV_GLSL_XFB_INTERLEAVED_2ND_DERIVATIVE_BUFFERS)
out float outDeriv2Buffer[3*LENGTH];

void writeDuu(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outDeriv2Buffer[i] = v.vertexData[i];
    }
}

void writeDuv(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outDeriv2Buffer[i+LENGTH] = v.vertexData[i];
    }
}

void writeDvv(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outDeriv2Buffer[i+2*LENGTH] = v.vertexData[i];
    }
}
#elif defined(OPENSUBDIV_GLSL_XFB_USE_2ND_DERIVATIVES)
out float outDuuBuffer[LENGTH];
out float outDuvBuffer[LENGTH];
out float outDvvBuffer[LENGTH];

void writeDuu(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outDuuBuffer[i] = v.vertexData[i];
    }
}

void writeDuv(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outDuvBuffer[i] = v.vertexData[i];
    }
}

void writeDvv(Vertex v) {
    for(int i = 0; i < LENGTH; i++) {
        outDvvBuffer[i] = v.vertexData[i];
    }
}
#endif

//------------------------------------------------------------------------------

#if defined(OPENSUBDIV_GLSL_XFB_KERNEL_EVAL_STENCILS)

uniform usamplerBuffer sizes;
uniform isamplerBuffer offsets;
uniform isamplerBuffer indices;
uniform samplerBuffer  weights;

#if defined(OPENSUBDIV_GLSL_XFB_USE_1ST_DERIVATIVES)
uniform samplerBuffer  duWeights;
uniform samplerBuffer  dvWeights;
#endif

#if defined(OPENSUBDIV_GLSL_XFB_USE_2ND_DERIVATIVES)
uniform samplerBuffer  duuWeights;
uniform samplerBuffer  duvWeights;
uniform samplerBuffer  dvvWeights;
#endif

uniform int batchStart = 0;
uniform int batchEnd = 0;

void main() {
    int current = gl_VertexID + batchStart;

    if (current>=batchEnd) {
        return;
    }

    Vertex dst, du, dv, duu, duv, dvv;
    clear(dst);
    clear(du);
    clear(dv);
    clear(duu);
    clear(duv);
    clear(dvv);

    int offset = texelFetch(offsets, current).x;
    uint size = texelFetch(sizes, current).x;

    for (int stencil=0; stencil<size; ++stencil) {
        int index = texelFetch(indices, offset+stencil).x;
        float weight = texelFetch(weights, offset+stencil).x;
        addWithWeight(dst, readVertex( index ), weight);

#if defined(OPENSUBDIV_GLSL_XFB_USE_1ST_DERIVATIVES)
        float duWeight = texelFetch(duWeights, offset+stencil).x;
        float dvWeight = texelFetch(dvWeights, offset+stencil).x;
        addWithWeight(du,  readVertex(index), duWeight);
        addWithWeight(dv,  readVertex(index), dvWeight);
#endif
#if defined(OPENSUBDIV_GLSL_XFB_USE_2ND_DERIVATIVES)
        float duuWeight = texelFetch(duuWeights, offset+stencil).x;
        float duvWeight = texelFetch(duvWeights, offset+stencil).x;
        float dvvWeight = texelFetch(dvvWeights, offset+stencil).x;
        addWithWeight(duu,  readVertex(index), duuWeight);
        addWithWeight(duv,  readVertex(index), duvWeight);
        addWithWeight(dvv,  readVertex(index), dvvWeight);
#endif
    }
    writeVertex(dst);

#if defined(OPENSUBDIV_GLSL_XFB_USE_1ST_DERIVATIVES)
    writeDu(du);
    writeDv(dv);
#endif
#if defined(OPENSUBDIV_GLSL_XFB_USE_2ND_DERIVATIVES)
    writeDuu(duu);
    writeDuv(duv);
    writeDvv(dvv);
#endif
}

#endif

//------------------------------------------------------------------------------

#if defined(OPENSUBDIV_GLSL_XFB_KERNEL_EVAL_PATCHES)

layout (location = 0) in ivec3 patchHandles;
layout (location = 1) in vec2  patchCoords;

layout (std140) uniform PatchArrays {
    OsdPatchArray patchArrays[2];
};
uniform isamplerBuffer patchParamBuffer;
uniform isamplerBuffer patchIndexBuffer;

OsdPatchArray GetPatchArray(int arrayIndex) {
    return patchArrays[arrayIndex];
}

OsdPatchParam GetPatchParam(int patchIndex) {
    ivec3 patchParamBits = texelFetch(patchParamBuffer, patchIndex).xyz;
    return OsdPatchParamInit(patchParamBits.x, patchParamBits.y, patchParamBits.z);
}

void main() {
    int current = gl_VertexID;

    ivec3 handle = patchHandles;
    int arrayIndex = handle.x;
    int patchIndex = handle.y;

    vec2 coord = patchCoords;

    OsdPatchArray array = GetPatchArray(arrayIndex);
    OsdPatchParam param = GetPatchParam(patchIndex);

    int patchType = OsdPatchParamIsRegular(param) ? array.regDesc : array.desc;

    float wP[20], wDu[20], wDv[20], wDuu[20], wDuv[20], wDvv[20];
    int nPoints = OsdEvaluatePatchBasis(patchType, param,
        coord.x, coord.y, wP, wDu, wDv, wDuu, wDuv, wDvv);

    Vertex dst, du, dv, duu, duv, dvv;
    clear(dst);
    clear(du);
    clear(dv);
    clear(duu);
    clear(duv);
    clear(dvv);

    int indexBase = array.indexBase + array.stride *
                (patchIndex - array.primitiveIdBase);

    for (int cv = 0; cv < nPoints; ++cv) {
        int index = texelFetch(patchIndexBuffer, indexBase + cv).x;
        addWithWeight(dst, readVertex(index), wP[cv]);
        addWithWeight(du,  readVertex(index), wDu[cv]);
        addWithWeight(dv,  readVertex(index), wDv[cv]);
        addWithWeight(duu, readVertex(index), wDuu[cv]);
        addWithWeight(duv, readVertex(index), wDuv[cv]);
        addWithWeight(dvv, readVertex(index), wDvv[cv]);
    }

    writeVertex(dst);

#if defined(OPENSUBDIV_GLSL_XFB_USE_1ST_DERIVATIVES)
    writeDu(du);
    writeDv(dv);
#endif
#if defined(OPENSUBDIV_GLSL_XFB_USE_2ND_DERIVATIVES)
    writeDuu(duu);
    writeDuv(duv);
    writeDvv(dvv);
#endif
}

#endif

