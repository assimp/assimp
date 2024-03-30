//
//   Copyright 2018 Pixar
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

#ifndef OPENSUBDIV3_OSD_PATCH_BASIS_COMMON_TYPES_H
#define OPENSUBDIV3_OSD_PATCH_BASIS_COMMON_TYPES_H

#if defined(OSD_PATCH_BASIS_GLSL)

    #define OSD_FUNCTION_STORAGE_CLASS
    #define OSD_DATA_STORAGE_CLASS
    #define OSD_REAL float
    #define OSD_REAL_CAST float
    #define OSD_OPTIONAL(a) true
    #define OSD_OPTIONAL_INIT(a,b) b
    #define OSD_ARRAY_ARG_BOUND_OPTIONAL 0
    #define OSD_IN_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_OUT_ARRAY(elementType, identifier, arraySize) \
            out elementType identifier[arraySize]
    #define OSD_INOUT_ARRAY(elementType, identifier, arraySize) \
            inout elementType identifier[arraySize]
    #define OSD_ARRAY_2(elementType,a0,a1) \
            elementType[](a0,a1)
    #define OSD_ARRAY_3(elementType,a0,a1,a2) \
            elementType[](a0,a1,a2)
    #define OSD_ARRAY_4(elementType,a0,a1,a2,a3) \
            elementType[](a0,a1,a2,a3)
    #define OSD_ARRAY_6(elementType,a0,a1,a2,a3,a4,a5) \
            elementType[](a0,a1,a2,a3,a4,a5)
    #define OSD_ARRAY_8(elementType,a0,a1,a2,a3,a4,a5,a6,a7) \
            elementType[](a0,a1,a2,a3,a4,a5,a6,a7)
    #define OSD_ARRAY_9(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8) \
            elementType[](a0,a1,a2,a3,a4,a5,a6,a7,a8)
    #define OSD_ARRAY_12(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
            elementType[](a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)

#elif defined(OSD_PATCH_BASIS_HLSL)

    #define OSD_FUNCTION_STORAGE_CLASS
    #define OSD_DATA_STORAGE_CLASS
    #define OSD_REAL float
    #define OSD_REAL_CAST float
    #define OSD_OPTIONAL(a) true
    #define OSD_OPTIONAL_INIT(a,b) b
    #define OSD_ARRAY_ARG_BOUND_OPTIONAL 0
    #define OSD_IN_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_OUT_ARRAY(elementType, identifier, arraySize) \
            out elementType identifier[arraySize]
    #define OSD_INOUT_ARRAY(elementType, identifier, arraySize) \
            inout elementType identifier[arraySize]
    #define OSD_ARRAY_2(elementType,a0,a1) \
            {a0,a1}
    #define OSD_ARRAY_3(elementType,a0,a1,a2) \
            {a0,a1,a2}
    #define OSD_ARRAY_4(elementType,a0,a1,a2,a3) \
            {a0,a1,a2,a3}
    #define OSD_ARRAY_6(elementType,a0,a1,a2,a3,a4,a5) \
            {a0,a1,a2,a3,a4,a5}
    #define OSD_ARRAY_8(elementType,a0,a1,a2,a3,a4,a5,a6,a7) \
            {a0,a1,a2,a3,a4,a5,a6,a7}
    #define OSD_ARRAY_9(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8) \
            {a0,a1,a2,a3,a4,a5,a6,a7,a8}
    #define OSD_ARRAY_12(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
            {a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11}

#elif defined(OSD_PATCH_BASIS_CUDA)

    #define OSD_FUNCTION_STORAGE_CLASS __device__
    #define OSD_DATA_STORAGE_CLASS
    #define OSD_REAL float
    #define OSD_REAL_CAST float
    #define OSD_OPTIONAL(a) true
    #define OSD_OPTIONAL_INIT(a,b) b
    #define OSD_ARRAY_ARG_BOUND_OPTIONAL 0
    #define OSD_IN_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_OUT_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_INOUT_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_ARRAY_2(elementType,a0,a1) \
            {a0,a1}
    #define OSD_ARRAY_3(elementType,a0,a1,a2) \
            {a0,a1,a2}
    #define OSD_ARRAY_4(elementType,a0,a1,a2,a3) \
            {a0,a1,a2,a3}
    #define OSD_ARRAY_6(elementType,a0,a1,a2,a3,a4,a5) \
            {a0,a1,a2,a3,a4,a5}
    #define OSD_ARRAY_8(elementType,a0,a1,a2,a3,a4,a5,a6,a7) \
            {a0,a1,a2,a3,a4,a5,a6,a7}
    #define OSD_ARRAY_9(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8) \
            {a0,a1,a2,a3,a4,a5,a6,a7,a8}
    #define OSD_ARRAY_12(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
            {a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11}

#elif defined(OSD_PATCH_BASIS_OPENCL)

    #define OSD_FUNCTION_STORAGE_CLASS static
    #define OSD_DATA_STORAGE_CLASS
    #define OSD_REAL float
    #define OSD_REAL_CAST convert_float
    #define OSD_OPTIONAL(a) true
    #define OSD_OPTIONAL_INIT(a,b) b
    #define OSD_ARRAY_ARG_BOUND_OPTIONAL 0
    #define OSD_IN_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_OUT_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_INOUT_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_ARRAY_2(elementType,a0,a1) \
            {a0,a1}
    #define OSD_ARRAY_3(elementType,a0,a1,a2) \
            {a0,a1,a2}
    #define OSD_ARRAY_4(elementType,a0,a1,a2,a3) \
            {a0,a1,a2,a3}
    #define OSD_ARRAY_6(elementType,a0,a1,a2,a3,a4,a5) \
            {a0,a1,a2,a3,a4,a5}
    #define OSD_ARRAY_8(elementType,a0,a1,a2,a3,a4,a5,a6,a7) \
            {a0,a1,a2,a3,a4,a5,a6,a7}
    #define OSD_ARRAY_9(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8) \
            {a0,a1,a2,a3,a4,a5,a6,a7,a8}
    #define OSD_ARRAY_12(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
            {a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11}

#elif defined(OSD_PATCH_BASIS_METAL)

    #define OSD_FUNCTION_STORAGE_CLASS
    #define OSD_DATA_STORAGE_CLASS
    #define OSD_REAL float
    #define OSD_REAL_CAST float
    #define OSD_OPTIONAL(a) true
    #define OSD_OPTIONAL_INIT(a,b) b
    #define OSD_ARRAY_ARG_BOUND_OPTIONAL 0
    #define OSD_IN_ARRAY(elementType, identifier, arraySize) \
            thread elementType* identifier
    #define OSD_OUT_ARRAY(elementType, identifier, arraySize) \
            thread elementType* identifier
    #define OSD_INOUT_ARRAY(elementType, identifier, arraySize) \
            thread elementType* identifier
    #define OSD_ARRAY_2(elementType,a0,a1) \
            {a0,a1}
    #define OSD_ARRAY_3(elementType,a0,a1,a2) \
            {a0,a1,a2}
    #define OSD_ARRAY_4(elementType,a0,a1,a2,a3) \
            {a0,a1,a2,a3}
    #define OSD_ARRAY_6(elementType,a0,a1,a2,a3,a4,a5) \
            {a0,a1,a2,a3,a4,a5}
    #define OSD_ARRAY_8(elementType,a0,a1,a2,a3,a4,a5,a6,a7) \
            {a0,a1,a2,a3,a4,a5,a6,a7}
    #define OSD_ARRAY_9(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8) \
            {a0,a1,a2,a3,a4,a5,a6,a7,a8}
    #define OSD_ARRAY_12(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
            {a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11}

#else

    #define OSD_FUNCTION_STORAGE_CLASS static inline
    #define OSD_DATA_STORAGE_CLASS static
    #define OSD_REAL float
    #define OSD_REAL_CAST float
    #define OSD_OPTIONAL(a) (a)
    #define OSD_OPTIONAL_INIT(a,b) (a ? b : 0)
    #define OSD_ARRAY_ARG_BOUND_OPTIONAL 1
    #define OSD_IN_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_OUT_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_INOUT_ARRAY(elementType, identifier, arraySize) \
            elementType identifier[arraySize]
    #define OSD_ARRAY_2(elementType,a0,a1) \
            {a0,a1}
    #define OSD_ARRAY_3(elementType,a0,a1,a2) \
            {a0,a1,a2}
    #define OSD_ARRAY_4(elementType,a0,a1,a2,a3) \
            {a0,a1,a2,a3}
    #define OSD_ARRAY_6(elementType,a0,a1,a2,a3,a4,a5) \
            {a0,a1,a2,a3,a4,a5}
    #define OSD_ARRAY_8(elementType,a0,a1,a2,a3,a4,a5,a6,a7) \
            {a0,a1,a2,a3,a4,a5,a6,a7}
    #define OSD_ARRAY_9(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8) \
            {a0,a1,a2,a3,a4,a5,a6,a7,a8}
    #define OSD_ARRAY_12(elementType,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
            {a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11}

#endif

#if defined(OSD_PATCH_BASIS_OPENCL)
// OpenCL binding uses typedef to provide the required "struct" type specifier.
typedef struct OsdPatchParam OsdPatchParam;
typedef struct OsdPatchArray OsdPatchArray;
typedef struct OsdPatchCoord OsdPatchCoord;
#endif

// Osd reflection of Far::PatchDescriptor
#define OSD_PATCH_DESCRIPTOR_QUADS            3
#define OSD_PATCH_DESCRIPTOR_TRIANGLES        4
#define OSD_PATCH_DESCRIPTOR_LOOP             5
#define OSD_PATCH_DESCRIPTOR_REGULAR          6
#define OSD_PATCH_DESCRIPTOR_GREGORY_BASIS    9
#define OSD_PATCH_DESCRIPTOR_GREGORY_TRIANGLE 10

// Osd reflection of Osd::PatchCoord
struct OsdPatchCoord {
   int arrayIndex;
   int patchIndex;
   int vertIndex;
   float s;
   float t;
};

OSD_FUNCTION_STORAGE_CLASS
OsdPatchCoord
OsdPatchCoordInit(
    int arrayIndex, int patchIndex, int vertIndex, float s, float t)
{
    OsdPatchCoord coord;
    coord.arrayIndex = arrayIndex;
    coord.patchIndex = patchIndex;
    coord.vertIndex = vertIndex;
    coord.s = s;
    coord.t = t;
    return coord;
}

// Osd reflection of Osd::PatchArray
struct OsdPatchArray {
    int regDesc;
    int desc;
    int numPatches;
    int indexBase;
    int stride;
    int primitiveIdBase;
};

OSD_FUNCTION_STORAGE_CLASS
OsdPatchArray
OsdPatchArrayInit(
    int regDesc, int desc,
    int numPatches, int indexBase, int stride, int primitiveIdBase)
{
    OsdPatchArray array;
    array.regDesc = regDesc;
    array.desc = desc;
    array.numPatches = numPatches;
    array.indexBase = indexBase;
    array.stride = stride;
    array.primitiveIdBase = primitiveIdBase;
    return array;
}

// Osd reflection of Osd::PatchParam
struct OsdPatchParam {
    int field0;
    int field1;
    float sharpness;
};

OSD_FUNCTION_STORAGE_CLASS
OsdPatchParam
OsdPatchParamInit(int field0, int field1, float sharpness)
{
    OsdPatchParam param;
    param.field0 = field0;
    param.field1 = field1;
    param.sharpness = sharpness;
    return param;
}

OSD_FUNCTION_STORAGE_CLASS
int
OsdPatchParamGetFaceId(OsdPatchParam param)
{
    return (param.field0 & 0xfffffff);
}

OSD_FUNCTION_STORAGE_CLASS
int
OsdPatchParamGetU(OsdPatchParam param)
{
    return ((param.field1 >> 22) & 0x3ff);
}

OSD_FUNCTION_STORAGE_CLASS
int
OsdPatchParamGetV(OsdPatchParam param)
{
    return ((param.field1 >> 12) & 0x3ff);
}

OSD_FUNCTION_STORAGE_CLASS
int
OsdPatchParamGetTransition(OsdPatchParam param)
{
    return ((param.field0 >> 28) & 0xf);
}

OSD_FUNCTION_STORAGE_CLASS
int
OsdPatchParamGetBoundary(OsdPatchParam param)
{
    return ((param.field1 >> 7) & 0x1f);
}

OSD_FUNCTION_STORAGE_CLASS
int
OsdPatchParamGetNonQuadRoot(OsdPatchParam param)
{
    return ((param.field1 >> 4) & 0x1);
}

OSD_FUNCTION_STORAGE_CLASS
int
OsdPatchParamGetDepth(OsdPatchParam param)
{
    return (param.field1 & 0xf);
}

OSD_FUNCTION_STORAGE_CLASS
OSD_REAL
OsdPatchParamGetParamFraction(OsdPatchParam param)
{
    return 1.0f / OSD_REAL_CAST(1 <<
        (OsdPatchParamGetDepth(param) - OsdPatchParamGetNonQuadRoot(param)));
}

OSD_FUNCTION_STORAGE_CLASS
bool
OsdPatchParamIsRegular(OsdPatchParam param)
{
    return (((param.field1 >> 5) & 0x1) != 0);
}

OSD_FUNCTION_STORAGE_CLASS
bool
OsdPatchParamIsTriangleRotated(OsdPatchParam param)
{
    return ((OsdPatchParamGetU(param) + OsdPatchParamGetV(param)) >=
            (1 << OsdPatchParamGetDepth(param)));
}

OSD_FUNCTION_STORAGE_CLASS
void
OsdPatchParamNormalize(
        OsdPatchParam param,
        OSD_INOUT_ARRAY(OSD_REAL, uv, 2))
{
    OSD_REAL fracInv = 1.0f / OsdPatchParamGetParamFraction(param);

    uv[0] = uv[0] * fracInv - OSD_REAL_CAST(OsdPatchParamGetU(param));
    uv[1] = uv[1] * fracInv - OSD_REAL_CAST(OsdPatchParamGetV(param));
}

OSD_FUNCTION_STORAGE_CLASS
void
OsdPatchParamUnnormalize(
        OsdPatchParam param,
        OSD_INOUT_ARRAY(OSD_REAL, uv, 2))
{
    OSD_REAL frac = OsdPatchParamGetParamFraction(param);

    uv[0] = (uv[0] + OSD_REAL_CAST(OsdPatchParamGetU(param))) * frac;
    uv[1] = (uv[1] + OSD_REAL_CAST(OsdPatchParamGetV(param))) * frac;
}

OSD_FUNCTION_STORAGE_CLASS
void
OsdPatchParamNormalizeTriangle(
        OsdPatchParam param,
        OSD_INOUT_ARRAY(OSD_REAL, uv, 2))
{
    if (OsdPatchParamIsTriangleRotated(param)) {
        OSD_REAL fracInv = 1.0f / OsdPatchParamGetParamFraction(param);

        int depthFactor = 1 << OsdPatchParamGetDepth(param);
        uv[0] = OSD_REAL_CAST(depthFactor - OsdPatchParamGetU(param)) - (uv[0] * fracInv);
        uv[1] = OSD_REAL_CAST(depthFactor - OsdPatchParamGetV(param)) - (uv[1] * fracInv);
    } else {
        OsdPatchParamNormalize(param, uv);
    }
}

OSD_FUNCTION_STORAGE_CLASS
void
OsdPatchParamUnnormalizeTriangle(
        OsdPatchParam param,
        OSD_INOUT_ARRAY(OSD_REAL, uv, 2))
{
    if (OsdPatchParamIsTriangleRotated(param)) {
        OSD_REAL frac = OsdPatchParamGetParamFraction(param);

        int depthFactor = 1 << OsdPatchParamGetDepth(param);
        uv[0] = (OSD_REAL_CAST(depthFactor - OsdPatchParamGetU(param)) - uv[0]) * frac;
        uv[1] = (OSD_REAL_CAST(depthFactor - OsdPatchParamGetV(param)) - uv[1]) * frac;
    } else {
        OsdPatchParamUnnormalize(param, uv);
    }
}

#endif /* OPENSUBDIV3_OSD_PATCH_BASIS_COMMON_TYPES_H */
