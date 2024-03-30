#line 0 "osd/mtlComputeKernel.metal"

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

#include <metal_stdlib>

#ifndef OPENSUBDIV_MTL_COMPUTE_USE_1ST_DERIVATIVES
#define OPENSUBDIV_MTL_COMPUTE_USE_1ST_DERIVATIVES 0
#endif

#ifndef OPENSUBDIV_MTL_COMPUTE_USE_2ND_DERIVATIVES
#define OPENSUBDIV_MTL_COMPUTE_USE_2ND_DERIVATIVES 0
#endif

using namespace metal;

struct KernelUniformArgs
{
    int batchStart;
    int batchEnd;

    int srcOffset;
    int dstOffset;

    int3 duDesc;
    int3 dvDesc;

    int3 duuDesc;
    int3 duvDesc;
    int3 dvvDesc;
};

struct Vertex {
    float vertexData[LENGTH];
};

void clear(thread Vertex& v) {
    for (int i = 0; i < LENGTH; ++i) {
        v.vertexData[i] = 0;
    }
}

Vertex readVertex(int index, device const float* vertexBuffer, KernelUniformArgs args) {
    Vertex v;
    int vertexIndex = args.srcOffset + index * SRC_STRIDE;
    for (int i = 0; i < LENGTH; ++i) {
        v.vertexData[i] = vertexBuffer[vertexIndex + i];
    }
    return v;
}

void writeVertex(int index, Vertex v, device float* vertexBuffer, KernelUniformArgs args) {
    int vertexIndex = args.dstOffset + index * DST_STRIDE;
    for (int i = 0; i < LENGTH; ++i) {
        vertexBuffer[vertexIndex + i] = v.vertexData[i];
    }
}

void writeVertexSeparate(int index, Vertex v, device float* dstVertexBuffer, KernelUniformArgs args) {
    int vertexIndex = args.dstOffset + index * DST_STRIDE;
    for (int i = 0; i < LENGTH; ++i) {
        dstVertexBuffer[vertexIndex + i] = v.vertexData[i];
    }
}

void addWithWeight(thread Vertex& v, const Vertex src, float weight) {
    for (int i = 0; i < LENGTH; ++i) {
        v.vertexData[i] += weight * src.vertexData[i];
    }
}

#if OPENSUBDIV_MTL_COMPUTE_USE_1ST_DERIVATIVES
void writeDu(int index, Vertex du, device float* duDerivativeBuffer, KernelUniformArgs args)
{
    int duIndex = args.duDesc.x + index * args.duDesc.z;
    for(int i = 0; i < LENGTH; i++) {
        duDerivativeBuffer[duIndex + i] = du.vertexData[i];
    }
}

void writeDv(int index, Vertex dv, device float* dvDerivativeBuffer, KernelUniformArgs args)
{
    int dvIndex = args.dvDesc.x + index * args.dvDesc.z;
    for(int i = 0; i < LENGTH; i++) {
        dvDerivativeBuffer[dvIndex + i] = dv.vertexData[i];
    }
}
#endif

#if OPENSUBDIV_MTL_COMPUTE_USE_2ND_DERIVATIVES
void writeDuu(int index, Vertex duu, device float* duuDerivativeBuffer, KernelUniformArgs args)
{
    int duuIndex = args.duuDesc.x + index * args.duuDesc.z;
    for(int i = 0; i < LENGTH; i++) {
        duuDerivativeBuffer[duuIndex + i] = duu.vertexData[i];
    }
}

void writeDuv(int index, Vertex duv, device float* duvDerivativeBuffer, KernelUniformArgs args)
{
    int duvIndex = args.duvDesc.x + index * args.duvDesc.z;
    for(int i = 0; i < LENGTH; i++) {
        duvDerivativeBuffer[duvIndex + i] = duv.vertexData[i];
    }
}

void writeDvv(int index, Vertex dvv, device float* dvvDerivativeBuffer, KernelUniformArgs args)
{
    int dvvIndex = args.dvvDesc.x + index * args.dvvDesc.z;
    for(int i = 0; i < LENGTH; i++) {
        dvvDerivativeBuffer[dvvIndex + i] = dvv.vertexData[i];
    }
}
#endif

// ---------------------------------------------------------------------------

kernel void eval_stencils(
    uint thread_position_in_grid [[thread_position_in_grid]],
    const device int* sizes [[buffer(SIZES_BUFFER_INDEX)]],
    const device int* offsets [[buffer(OFFSETS_BUFFER_INDEX)]],
    const device int* indices [[buffer(INDICES_BUFFER_INDEX)]],
    const device float* weights [[buffer(WEIGHTS_BUFFER_INDEX)]],
    const device float* srcVertices [[buffer(SRC_VERTEX_BUFFER_INDEX)]],
    device float* dstVertexBuffer [[buffer(DST_VERTEX_BUFFER_INDEX)]],
#if OPENSUBDIV_MTL_COMPUTE_USE_1ST_DERIVATIVES
    const device float* duWeights [[buffer(DU_WEIGHTS_BUFFER_INDEX)]],
    const device float* dvWeights [[buffer(DV_WEIGHTS_BUFFER_INDEX)]],
    device float* duDerivativeBuffer [[buffer(DU_DERIVATIVE_BUFFER_INDEX)]],
    device float* dvDerivativeBuffer [[buffer(DV_DERIVATIVE_BUFFER_INDEX)]],
#endif
#if OPENSUBDIV_MTL_COMPUTE_USE_2ND_DERIVATIVES
    const device float* duuWeights [[buffer(DUU_WEIGHTS_BUFFER_INDEX)]],
    const device float* duvWeights [[buffer(DUV_WEIGHTS_BUFFER_INDEX)]],
    const device float* dvvWeights [[buffer(DVV_WEIGHTS_BUFFER_INDEX)]],
    device float* duuDerivativeBuffer [[buffer(DUU_DERIVATIVE_BUFFER_INDEX)]],
    device float* duvDerivativeBuffer [[buffer(DUV_DERIVATIVE_BUFFER_INDEX)]],
    device float* dvvDerivativeBuffer [[buffer(DVV_DERIVATIVE_BUFFER_INDEX)]],
#endif
    const constant KernelUniformArgs& args [[buffer(PARAMETER_BUFFER_INDEX)]]
)
{
    auto current  = thread_position_in_grid + args.batchStart;
    if(current >= (unsigned int)args.batchEnd)
        return;

    Vertex dst;
    clear(dst);


    auto offset = offsets[current];
    auto size = sizes[current];

    for(auto stencil = 0; stencil < size; stencil++)
    {
        auto vindex = offset + stencil;
        addWithWeight(dst, readVertex(indices[vindex], srcVertices, args), weights[vindex]);
    }

    writeVertex(current, dst, dstVertexBuffer, args);

#if OPENSUBDIV_MTL_COMPUTE_USE_1ST_DERIVATIVES
    Vertex du, dv;
    clear(du);
    clear(dv);


    for(auto i = 0; i < size; i++)
    {
        auto src = readVertex(indices[offset + i], srcVertices, args);
        addWithWeight(du, src, duWeights[offset + i]);
        addWithWeight(dv, src, dvWeights[offset + i]);
    }

    writeDu(current, du, duDerivativeBuffer, args);
    writeDv(current, dv, dvDerivativeBuffer, args);
#endif

#if OPENSUBDIV_MTL_COMPUTE_USE_2ND_DERIVATIVES
    Vertex duu, duv, dvv;
    clear(duu);
    clear(duv);
    clear(dvv);


    for(auto i = 0; i < size; i++)
    {
        auto src = readVertex(indices[offset + i], srcVertices, args);
        addWithWeight(duu, src, duuWeights[offset + i]);
        addWithWeight(duv, src, duvWeights[offset + i]);
        addWithWeight(dvv, src, dvvWeights[offset + i]);
    }

    writeDuu(current, duu, duuDerivativeBuffer, args);
    writeDuv(current, duv, duvDerivativeBuffer, args);
    writeDvv(current, dvv, dvvDerivativeBuffer, args);
#endif
}


// ---------------------------------------------------------------------------

// PERFORMANCE: stride could be constant, but not as significant as length

// ---------------------------------------------------------------------------

kernel void eval_patches(
    uint thread_position_in_grid [[thread_position_in_grid]],
    const constant int* patchArrays [[buffer(PATCH_ARRAYS_BUFFER_INDEX)]],
    const device int* patchCoords [[buffer(PATCH_COORDS_BUFFER_INDEX)]],
    const device int* patchIndices [[buffer(PATCH_INDICES_BUFFER_INDEX)]],
    const device uint* patchParams [[buffer(PATCH_PARAMS_BUFFER_INDEX)]],
    const device float* srcVertexBuffer [[buffer(SRC_VERTEX_BUFFER_INDEX)]],
    device float* dstVertexBuffer [[buffer(DST_VERTEX_BUFFER_INDEX)]],
#if OPENSUBDIV_MTL_COMPUTE_USE_1ST_DERIVATIVES
    device float* duDerivativeBuffer [[buffer(DU_DERIVATIVE_BUFFER_INDEX)]],
    device float* dvDerivativeBuffer [[buffer(DV_DERIVATIVE_BUFFER_INDEX)]],
#endif
#if OPENSUBDIV_MTL_COMPUTE_USE_2ND_DERIVATIVES
    device float* duuDerivativeBuffer [[buffer(DUU_DERIVATIVE_BUFFER_INDEX)]],
    device float* duvDerivativeBuffer [[buffer(DUV_DERIVATIVE_BUFFER_INDEX)]],
    device float* dvvDerivativeBuffer [[buffer(DVV_DERIVATIVE_BUFFER_INDEX)]],
#endif
    const constant KernelUniformArgs& args [[buffer(PARAMETER_BUFFER_INDEX)]]
)
{
    auto current = thread_position_in_grid;

    // unpack struct (5 ints unaligned)
    OsdPatchCoord patchCoord = OsdPatchCoordInit(patchCoords[current*5+0],
                                                 patchCoords[current*5+1],
                                                 patchCoords[current*5+2],
                                  as_type<float>(patchCoords[current*5+3]),
                                  as_type<float>(patchCoords[current*5+4]));

    OsdPatchArray patchArray = OsdPatchArrayInit(patchArrays[patchCoord.arrayIndex*6+0],
                                                 patchArrays[patchCoord.arrayIndex*6+1],
                                                 patchArrays[patchCoord.arrayIndex*6+2],
                                                 patchArrays[patchCoord.arrayIndex*6+3],
                                                 patchArrays[patchCoord.arrayIndex*6+4],
                                                 patchArrays[patchCoord.arrayIndex*6+5]);

    OsdPatchParam patchParam = OsdPatchParamInit(patchParams[patchCoord.patchIndex*3+0],
                                                 patchParams[patchCoord.patchIndex*3+1],
                                  as_type<float>(patchParams[patchCoord.patchIndex*3+2]));

    int patchType = OsdPatchParamIsRegular(patchParam)
        ? patchArray.regDesc : patchArray.desc;

    float wP[20], wDu[20], wDv[20], wDuu[20], wDuv[20], wDvv[20];
    int nPoints = OsdEvaluatePatchBasis(patchType, patchParam,
        patchCoord.s, patchCoord.t, wP, wDu, wDv, wDuu, wDuv, wDvv);

    Vertex dst, du, dv, duu, duv, dvv;
    clear(dst);
    clear(du);
    clear(dv);
    clear(duu);
    clear(duv);
    clear(dvv);

    auto indexBase = patchArray.indexBase + patchArray.stride *
                (patchCoord.patchIndex - patchArray.primitiveIdBase);
    for(auto cv = 0; cv < nPoints; cv++)
    {
        auto index = patchIndices[indexBase + cv];
        auto src = readVertex(index, srcVertexBuffer, args);
        addWithWeight(dst, src, wP[cv]);
        addWithWeight(du,  src, wDu[cv]);
        addWithWeight(dv,  src, wDv[cv]);
        addWithWeight(duu, src, wDuu[cv]);
        addWithWeight(duv, src, wDuv[cv]);
        addWithWeight(dvv, src, wDvv[cv]);
    }

    writeVertex(current, dst, dstVertexBuffer, args);

#if OPENSUBDIV_MTL_COMPUTE_USE_1ST_DERIVATIVES
    if(args.duDesc.y > 0)
        writeDu(current, du, duDerivativeBuffer, args);

    if(args.dvDesc.y > 0)
        writeDv(current, dv, dvDerivativeBuffer, args);
#endif

#if OPENSUBDIV_MTL_COMPUTE_USE_2ND_DERIVATIVES
    if(args.duuDesc.y > 0)
        writeDuu(current, duu, duuDerivativeBuffer, args);

    if(args.duvDesc.y > 0)
        writeDuv(current, duv, duvDerivativeBuffer, args);

    if(args.dvvDesc.y > 0)
        writeDvv(current, dvv, dvvDerivativeBuffer, args);
#endif
}
