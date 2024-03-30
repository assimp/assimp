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

#ifndef OPENSUBDIV3_OSD_PATCH_BASIS_COMMON_EVAL_H
#define OPENSUBDIV3_OSD_PATCH_BASIS_COMMON_EVAL_H

OSD_FUNCTION_STORAGE_CLASS
// template <typename REAL>
int
OsdEvaluatePatchBasisNormalized(
    int patchType, OsdPatchParam param,
    OSD_REAL s, OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 20)) {

    int boundaryMask = OsdPatchParamGetBoundary(param);

    int nPoints = 0;
    if (patchType == OSD_PATCH_DESCRIPTOR_REGULAR) {
#if OSD_ARRAY_ARG_BOUND_OPTIONAL
        nPoints = Osd_EvalBasisBSpline(s, t, wP, wDs, wDt, wDss, wDst, wDtt);
        if (boundaryMask != 0) {
            Osd_boundBasisBSpline(
                boundaryMask, wP, wDs, wDt, wDss, wDst, wDtt);
        }
#else
        OSD_REAL wP16[16], wDs16[16], wDt16[16],
                 wDss16[16], wDst16[16], wDtt16[16];
        nPoints = Osd_EvalBasisBSpline(
                s, t, wP16, wDs16, wDt16, wDss16, wDst16, wDtt16);
        if (boundaryMask != 0) {
            Osd_boundBasisBSpline(
                boundaryMask, wP16, wDs16, wDt16, wDss16, wDst16, wDtt16);
        }
        for (int i=0; i<nPoints; ++i) {
            wP[i] = wP16[i];
            wDs[i] = wDs16[i]; wDt[i] = wDt16[i];
            wDss[i] = wDss16[i]; wDst[i] = wDst16[i]; wDtt[i] = wDtt16[i];
        }
#endif
    } else if (patchType == OSD_PATCH_DESCRIPTOR_LOOP) {
#if OSD_ARRAY_ARG_BOUND_OPTIONAL
        nPoints = Osd_EvalBasisBoxSplineTri(
                s, t, wP, wDs, wDt, wDss, wDst, wDtt);
        if (boundaryMask != 0) {
            Osd_boundBasisBoxSplineTri(
                boundaryMask, wP, wDs, wDt, wDss, wDst, wDtt);
        }
#else
        OSD_REAL wP12[12], wDs12[12], wDt12[12],
                 wDss12[12], wDst12[12], wDtt12[12];
        nPoints = Osd_EvalBasisBoxSplineTri(
                s, t, wP12, wDs12, wDt12, wDss12, wDst12, wDtt12);
        if (boundaryMask != 0) {
            Osd_boundBasisBoxSplineTri(
                boundaryMask, wP12, wDs12, wDt12, wDss12, wDst12, wDtt12);
        }
        for (int i=0; i<nPoints; ++i) {
            wP[i] = wP12[i];
            wDs[i] = wDs12[i]; wDt[i] = wDt12[i];
            wDss[i] = wDss12[i]; wDst[i] = wDst12[i]; wDtt[i] = wDtt12[i];
        }
#endif
    } else if (patchType == OSD_PATCH_DESCRIPTOR_GREGORY_BASIS) {
        nPoints = Osd_EvalBasisGregory(s, t, wP, wDs, wDt, wDss, wDst, wDtt);
    } else if (patchType == OSD_PATCH_DESCRIPTOR_GREGORY_TRIANGLE) {
#if OSD_ARRAY_ARG_BOUND_OPTIONAL
        nPoints = Osd_EvalBasisGregoryTri(s, t, wP, wDs, wDt, wDss, wDst, wDtt);
#else
        OSD_REAL wP18[18], wDs18[18], wDt18[18],
                 wDss18[18], wDst18[18], wDtt18[18];
        nPoints = Osd_EvalBasisGregoryTri(
                s, t, wP18, wDs18, wDt18, wDss18, wDst18, wDtt18);
        for (int i=0; i<nPoints; ++i) {
            wP[i] = wP18[i];
            wDs[i] = wDs18[i]; wDt[i] = wDt18[i];
            wDss[i] = wDss18[i]; wDst[i] = wDst18[i]; wDtt[i] = wDtt18[i];
        }
#endif
    } else if (patchType == OSD_PATCH_DESCRIPTOR_QUADS) {
#if OSD_ARRAY_ARG_BOUND_OPTIONAL
        nPoints = Osd_EvalBasisLinear(s, t, wP, wDs, wDt, wDss, wDst, wDtt);
#else
        OSD_REAL wP4[4], wDs4[4], wDt4[4],
                 wDss4[4], wDst4[4], wDtt4[4];
        nPoints = Osd_EvalBasisLinear(
                s, t, wP4, wDs4, wDt4, wDss4, wDst4, wDtt4);
        for (int i=0; i<nPoints; ++i) {
            wP[i] = wP4[i];
            wDs[i] = wDs4[i]; wDt[i] = wDt4[i];
            wDss[i] = wDss4[i]; wDst[i] = wDst4[i]; wDtt[i] = wDtt4[i];
        }
#endif
    } else if (patchType == OSD_PATCH_DESCRIPTOR_TRIANGLES) {
#if OSD_ARRAY_ARG_BOUND_OPTIONAL
        nPoints = Osd_EvalBasisLinearTri(s, t, wP, wDs, wDt, wDss, wDst, wDtt);
#else
        OSD_REAL wP3[3], wDs3[3], wDt3[3],
                 wDss3[3], wDst3[3], wDtt3[3];
        nPoints = Osd_EvalBasisLinearTri(
                s, t, wP3, wDs3, wDt3, wDss3, wDst3, wDtt3);
        for (int i=0; i<nPoints; ++i) {
            wP[i] = wP3[i];
            wDs[i] = wDs3[i]; wDt[i] = wDt3[i];
            wDss[i] = wDss3[i]; wDst[i] = wDst3[i]; wDtt[i] = wDtt3[i];
        }
#endif
    } else {
        // assert(0);
    }
    return nPoints;
}

OSD_FUNCTION_STORAGE_CLASS
// template <typename REAL>
int
OsdEvaluatePatchBasis(
    int patchType, OsdPatchParam param,
    OSD_REAL s, OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 20)) {

    OSD_REAL derivSign = 1.0f;

    if ((patchType == OSD_PATCH_DESCRIPTOR_LOOP) ||
        (patchType == OSD_PATCH_DESCRIPTOR_GREGORY_TRIANGLE) ||
        (patchType == OSD_PATCH_DESCRIPTOR_TRIANGLES)) {
        OSD_REAL uv[2] = OSD_ARRAY_2(OSD_REAL, s, t);
        OsdPatchParamNormalizeTriangle(param, uv);
        s = uv[0];
        t = uv[1];
        if (OsdPatchParamIsTriangleRotated(param)) {
            derivSign = -1.0f;
        }
    } else {
        OSD_REAL uv[2] = OSD_ARRAY_2(OSD_REAL, s, t);
        OsdPatchParamNormalize(param, uv);
        s = uv[0];
        t = uv[1];
    }

    int nPoints = OsdEvaluatePatchBasisNormalized(
        patchType, param, s, t, wP, wDs, wDt, wDss, wDst, wDtt);

    if (OSD_OPTIONAL(wDs && wDt)) {
        OSD_REAL d1Scale =
                derivSign * OSD_REAL_CAST(1 << OsdPatchParamGetDepth(param));

        for (int i = 0; i < nPoints; ++i) {
            wDs[i] *= d1Scale;
            wDt[i] *= d1Scale;
        }

        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            OSD_REAL d2Scale = derivSign * d1Scale * d1Scale;

            for (int i = 0; i < nPoints; ++i) {
                wDss[i] *= d2Scale;
                wDst[i] *= d2Scale;
                wDtt[i] *= d2Scale;
            }
        }
    }
    return nPoints;
}

#endif /* OPENSUBDIV3_OSD_PATCH_BASIS_COMMON_EVAL_H */
