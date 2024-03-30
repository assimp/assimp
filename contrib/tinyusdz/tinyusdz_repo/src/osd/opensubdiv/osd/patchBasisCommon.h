//
//   Copyright 2016-2018 Pixar
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

#ifndef OPENSUBDIV3_OSD_PATCH_BASIS_COMMON_H
#define OPENSUBDIV3_OSD_PATCH_BASIS_COMMON_H

OSD_FUNCTION_STORAGE_CLASS
// template <typename REAL>
int
Osd_EvalBasisLinear(OSD_REAL s, OSD_REAL t,
        OSD_OUT_ARRAY(OSD_REAL, wP, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDs, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDt, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDss, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDst, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDtt, 4)) {

    OSD_REAL sC = 1.0f - s;
    OSD_REAL tC = 1.0f - t;

    if (OSD_OPTIONAL(wP)) {
        wP[0] = sC * tC;
        wP[1] =  s * tC;
        wP[2] =  s * t;
        wP[3] = sC * t;
    }

    if (OSD_OPTIONAL(wDs && wDt)) {
        wDs[0] = -tC;
        wDs[1] =  tC;
        wDs[2] =   t;
        wDs[3] =  -t;

        wDt[0] = -sC;
        wDt[1] =  -s;
        wDt[2] =   s;
        wDt[3] =  sC;

        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            for(int i=0;i<4;i++) {
                wDss[i] = 0.0f;
                wDtt[i] = 0.0f;
            }

            wDst[0] =  1.0f;
            wDst[1] = -1.0f;
            wDst[2] =  1.0f;
            wDst[3] = -1.0f;
        }
    }
    return 4;
}

// namespace {
    //
    //  Cubic BSpline curve basis evaluation:
    //
    OSD_FUNCTION_STORAGE_CLASS
    // template <typename REAL>
    void
    Osd_evalBSplineCurve(OSD_REAL t,
        OSD_OUT_ARRAY(OSD_REAL, wP, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDP, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDP2, 4)) {

        const OSD_REAL one6th = OSD_REAL_CAST(1.0f / 6.0f);

        OSD_REAL t2 = t * t;
        OSD_REAL t3 = t * t2;

        wP[0] = one6th * (1.0f - 3.0f*(t -      t2) -      t3);
        wP[1] = one6th * (4.0f           - 6.0f*t2  + 3.0f*t3);
        wP[2] = one6th * (1.0f + 3.0f*(t +      t2  -      t3));
        wP[3] = one6th * (                                 t3);

        if (OSD_OPTIONAL(wDP)) {
            wDP[0] = -0.5f*t2 +      t - 0.5f;
            wDP[1] =  1.5f*t2 - 2.0f*t;
            wDP[2] = -1.5f*t2 +      t + 0.5f;
            wDP[3] =  0.5f*t2;
        }
        if (OSD_OPTIONAL(wDP2)) {
            wDP2[0] = -       t + 1.0f;
            wDP2[1] =  3.0f * t - 2.0f;
            wDP2[2] = -3.0f * t + 1.0f;
            wDP2[3] =         t;
        }
    }

    //
    //  Weight adjustments to account for phantom end points:
    //
    OSD_FUNCTION_STORAGE_CLASS
    // template <typename REAL>
    void
    Osd_adjustBSplineBoundaryWeights(
            int boundary,
            OSD_INOUT_ARRAY(OSD_REAL, w, 16)) {

        if ((boundary & 1) != 0) {
            for (int i = 0; i < 4; ++i) {
                w[i + 8] -= w[i + 0];
                w[i + 4] += w[i + 0] * 2.0f;
                w[i + 0]  = 0.0f;
            }
        }
        if ((boundary & 2) != 0) {
            for (int i = 0; i < 16; i += 4) {
                w[i + 1] -= w[i + 3];
                w[i + 2] += w[i + 3] * 2.0f;
                w[i + 3]  = 0.0f;
            }
        }
        if ((boundary & 4) != 0) {
            for (int i = 0; i < 4; ++i) {
                w[i +  4] -= w[i + 12];
                w[i +  8] += w[i + 12] * 2.0f;
                w[i + 12]  = 0.0f;
            }
        }
        if ((boundary & 8) != 0) {
            for (int i = 0; i < 16; i += 4) {
                w[i + 2] -= w[i + 0];
                w[i + 1] += w[i + 0] * 2.0f;
                w[i + 0]  = 0.0f;
            }
        }
    }

    OSD_FUNCTION_STORAGE_CLASS
    // template <typename REAL>
    void
    Osd_boundBasisBSpline(
            int boundary,
            OSD_INOUT_ARRAY(OSD_REAL, wP, 16),
            OSD_INOUT_ARRAY(OSD_REAL, wDs, 16),
            OSD_INOUT_ARRAY(OSD_REAL, wDt, 16),
            OSD_INOUT_ARRAY(OSD_REAL, wDss, 16),
            OSD_INOUT_ARRAY(OSD_REAL, wDst, 16),
            OSD_INOUT_ARRAY(OSD_REAL, wDtt, 16)) {

        if (OSD_OPTIONAL(wP)) {
            Osd_adjustBSplineBoundaryWeights(boundary, wP);
        }
        if (OSD_OPTIONAL(wDs && wDt)) {
            Osd_adjustBSplineBoundaryWeights(boundary, wDs);
            Osd_adjustBSplineBoundaryWeights(boundary, wDt);

            if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
                Osd_adjustBSplineBoundaryWeights(boundary, wDss);
                Osd_adjustBSplineBoundaryWeights(boundary, wDst);
                Osd_adjustBSplineBoundaryWeights(boundary, wDtt);
            }
        }
    }

// } // end namespace

OSD_FUNCTION_STORAGE_CLASS
int
Osd_EvalBasisBSpline(OSD_REAL s, OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 16)) {

    OSD_REAL sWeights[4], tWeights[4], dsWeights[4], dtWeights[4], dssWeights[4], dttWeights[4];

    Osd_evalBSplineCurve(s, sWeights, OSD_OPTIONAL_INIT(wDs, dsWeights), OSD_OPTIONAL_INIT(wDss, dssWeights));
    Osd_evalBSplineCurve(t, tWeights, OSD_OPTIONAL_INIT(wDt, dtWeights), OSD_OPTIONAL_INIT(wDtt, dttWeights));

    if (OSD_OPTIONAL(wP)) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                wP[4*i+j] = sWeights[j] * tWeights[i];
            }
        }
    }

    if (OSD_OPTIONAL(wDs && wDt)) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                wDs[4*i+j] = dsWeights[j] * tWeights[i];
                wDt[4*i+j] = sWeights[j] * dtWeights[i];
            }
        }

        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    wDss[4*i+j] = dssWeights[j] * tWeights[i];
                    wDst[4*i+j] = dsWeights[j] * dtWeights[i];
                    wDtt[4*i+j] = sWeights[j] * dttWeights[i];
                }
            }
        }
    }
    return 16;
}

// namespace {
    //
    //  Cubic Bezier curve basis evaluation:
    //
    OSD_FUNCTION_STORAGE_CLASS
    // template <typename REAL>
    void
    Osd_evalBezierCurve(
        OSD_REAL t,
        OSD_OUT_ARRAY(OSD_REAL, wP, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDP, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDP2, 4)) {

        // The four uniform cubic Bezier basis functions (in terms of t and its
        // complement tC) evaluated at t:
        OSD_REAL t2 = t*t;
        OSD_REAL tC = 1.0f - t;
        OSD_REAL tC2 = tC * tC;

        wP[0] = tC2 * tC;
        wP[1] = tC2 * t * 3.0f;
        wP[2] = t2 * tC * 3.0f;
        wP[3] = t2 * t;

        // Derivatives of the above four basis functions at t:
        if (OSD_OPTIONAL(wDP)) {
           wDP[0] = -3.0f * tC2;
           wDP[1] =  9.0f * t2 - 12.0f * t + 3.0f;
           wDP[2] = -9.0f * t2 +  6.0f * t;
           wDP[3] =  3.0f * t2;
        }

        // Second derivatives of the basis functions at t:
        if (OSD_OPTIONAL(wDP2)) {
            wDP2[0] =   6.0f * tC;
            wDP2[1] =  18.0f * t - 12.0f;
            wDP2[2] = -18.0f * t +  6.0f;
            wDP2[3] =   6.0f * t;
        }
    }
// } // end namespace

OSD_FUNCTION_STORAGE_CLASS
int
Osd_EvalBasisBezier(OSD_REAL s, OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 16)) {

    OSD_REAL sWeights[4], tWeights[4], dsWeights[4], dtWeights[4], dssWeights[4], dttWeights[4];

    Osd_evalBezierCurve(s, OSD_OPTIONAL_INIT(wP, sWeights), OSD_OPTIONAL_INIT(wDs, dsWeights), OSD_OPTIONAL_INIT(wDss, dssWeights));
    Osd_evalBezierCurve(t, OSD_OPTIONAL_INIT(wP, tWeights), OSD_OPTIONAL_INIT(wDt, dtWeights), OSD_OPTIONAL_INIT(wDtt, dttWeights));

    if (OSD_OPTIONAL(wP)) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                wP[4*i+j] = sWeights[j] * tWeights[i];
            }
        }
    }

    if (OSD_OPTIONAL(wDs && wDt)) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                wDs[4*i+j] = dsWeights[j] * tWeights[i];
                wDt[4*i+j] = sWeights[j] * dtWeights[i];
            }
        }

        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    wDss[4*i+j] = dssWeights[j] * tWeights[i];
                    wDst[4*i+j] = dsWeights[j] * dtWeights[i];
                    wDtt[4*i+j] = sWeights[j] * dttWeights[i];
                }
            }
        }
    }
    return 16;
}

OSD_FUNCTION_STORAGE_CLASS
int
Osd_EvalBasisGregory(OSD_REAL s, OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 20)) {

    //  Indices of boundary and interior points and their corresponding Bezier points
    //  (this can be reduced with more direct indexing and unrolling of loops):
    //
    OSD_DATA_STORAGE_CLASS const int boundaryGregory[12] = OSD_ARRAY_12(int, 0, 1, 7, 5, 2, 6, 16, 12, 15, 17, 11, 10 );
    OSD_DATA_STORAGE_CLASS const int boundaryBezSCol[12] = OSD_ARRAY_12(int, 0, 1, 2, 3, 0, 3,  0,  3,  0,  1,  2,  3 );
    OSD_DATA_STORAGE_CLASS const int boundaryBezTRow[12] = OSD_ARRAY_12(int, 0, 0, 0, 0, 1, 1,  2,  2,  3,  3,  3,  3 );

    OSD_DATA_STORAGE_CLASS const int interiorGregory[8] = OSD_ARRAY_8(int, 3, 4,  8, 9,  13, 14,  18, 19 );
    OSD_DATA_STORAGE_CLASS const int interiorBezSCol[8] = OSD_ARRAY_8(int, 1, 1,  2, 2,   2,  2,   1,  1 );
    OSD_DATA_STORAGE_CLASS const int interiorBezTRow[8] = OSD_ARRAY_8(int, 1, 1,  1, 1,   2,  2,   2,  2 );

    //
    //  Bezier basis functions are denoted with B while the rational multipliers for the
    //  interior points will be denoted G -- so we have B(s), B(t) and G(s,t):
    //
    //  Directional Bezier basis functions B at s and t:
    OSD_REAL Bs[4], Bds[4], Bdss[4];
    OSD_REAL Bt[4], Bdt[4], Bdtt[4];

    Osd_evalBezierCurve(s, Bs, OSD_OPTIONAL_INIT(wDs, Bds), OSD_OPTIONAL_INIT(wDss, Bdss));
    Osd_evalBezierCurve(t, Bt, OSD_OPTIONAL_INIT(wDt, Bdt), OSD_OPTIONAL_INIT(wDtt, Bdtt));

    //  Rational multipliers G at s and t:
    OSD_REAL sC = 1.0f - s;
    OSD_REAL tC = 1.0f - t;

    //  Use <= here to avoid compiler warnings -- the sums should always be non-negative:
    OSD_REAL df0 = s  + t;   df0 = (df0 <= 0.0f) ? 1.0f : (1.0f / df0);
    OSD_REAL df1 = sC + t;   df1 = (df1 <= 0.0f) ? 1.0f : (1.0f / df1);
    OSD_REAL df2 = sC + tC;  df2 = (df2 <= 0.0f) ? 1.0f : (1.0f / df2);
    OSD_REAL df3 = s  + tC;  df3 = (df3 <= 0.0f) ? 1.0f : (1.0f / df3);

    //  Make sure the G[i] for pairs of interior points sum to 1 in all cases:
    OSD_REAL G[8] = OSD_ARRAY_8(OSD_REAL,  s*df0, (1.0f -  s*df0),
                                           t*df1, (1.0f -  t*df1),
                                          sC*df2, (1.0f - sC*df2),
                                          tC*df3, (1.0f - tC*df3) );

    //  Combined weights for boundary and interior points:
    for (int i = 0; i < 12; ++i) {
        wP[boundaryGregory[i]] = Bs[boundaryBezSCol[i]] * Bt[boundaryBezTRow[i]];
    }
    for (int j = 0; j < 8; ++j) {
        wP[interiorGregory[j]] = Bs[interiorBezSCol[j]] * Bt[interiorBezTRow[j]] * G[j];
    }

    //
    //  For derivatives, the basis functions for the interior points are rational and ideally
    //  require appropriate differentiation, i.e. product rule for the combination of B and G
    //  and the quotient rule for the rational G itself.  As initially proposed by Loop et al
    //  though, the approximation using the 16 Bezier points arising from the G(s,t) has
    //  proved adequate (and is what the GPU shaders use) so we continue to use that here.
    //
    //  An implementation of the true derivatives is provided and conditionally compiled for
    //  those that require it, e.g.:
    //
    //    dclyde's note: skipping half of the product rule like this does seem to change the
    //    result a lot in my tests.  This is not a runtime bottleneck for cloth sims anyway
    //    so I'm just using the accurate version.
    //
    if (OSD_OPTIONAL(wDs && wDt)) {
        bool find_second_partials = OSD_OPTIONAL(wDs && wDst && wDtt);

        //  Combined weights for boundary points -- simple tensor products:
        for (int i = 0; i < 12; ++i) {
            int iDst = boundaryGregory[i];
            int tRow = boundaryBezTRow[i];
            int sCol = boundaryBezSCol[i];

            wDs[iDst] = Bds[sCol] * Bt[tRow];
            wDt[iDst] = Bdt[tRow] * Bs[sCol];

            if (find_second_partials) {
                wDss[iDst] = Bdss[sCol] * Bt[tRow];
                wDst[iDst] = Bds[sCol] * Bdt[tRow];
                wDtt[iDst] = Bs[sCol] * Bdtt[tRow];
            }
        }

#ifndef OPENSUBDIV_GREGORY_EVAL_TRUE_DERIVATIVES
        //  Approximation to the true Gregory derivatives by differentiating the Bezier patch
        //  unique to the given (s,t), i.e. having F = (g^+ * f^+) + (g^- * f^-) as its four
        //  interior points:
        //
        //  Combined weights for interior points -- tensor products with G+ or G-:
        for (int j = 0; j < 8; ++j) {
            int iDst = interiorGregory[j];
            int tRow = interiorBezTRow[j];
            int sCol = interiorBezSCol[j];

            wDs[iDst] = Bds[sCol] * Bt[tRow] * G[j];
            wDt[iDst] = Bdt[tRow] * Bs[sCol] * G[j];

            if (find_second_partials) {
                wDss[iDst] = Bdss[sCol] * Bt[tRow] * G[j];
                wDst[iDst] = Bds[sCol] * Bdt[tRow] * G[j];
                wDtt[iDst] = Bs[sCol] * Bdtt[tRow] * G[j];
            }
        }
#else
        //  True Gregory derivatives using appropriate differentiation of composite functions:
        //
        //  Note that for G(s,t) = N(s,t) / D(s,t), all N' and D' are trivial constants (which
        //  simplifies things for higher order derivatives).  And while each pair of functions
        //  G (i.e. the G+ and G- corresponding to points f+ and f-) must sum to 1 to ensure
        //  Bezier equivalence (when f+ = f-), the pairs of G' must similarly sum to 0.  So we
        //  can potentially compute only one of the pair and negate the result for the other
        //  (and with 4 or 8 computations involving these constants, this is all very SIMD
        //  friendly...) but for now we treat all 8 independently for simplicity.
        //
        //float N[8] = OSD_ARRAY_8(float,    s,     t,      t,     sC,      sC,     tC,      tC,     s );
        OSD_REAL D[8] = OSD_ARRAY_8(OSD_REAL,  df0,   df0,    df1,    df1,     df2,    df2,     df3,   df3 );

        OSD_DATA_STORAGE_CLASS const OSD_REAL Nds[8] = OSD_ARRAY_8(OSD_REAL, 1.0f, 0.0f,  0.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f );
        OSD_DATA_STORAGE_CLASS const OSD_REAL Ndt[8] = OSD_ARRAY_8(OSD_REAL, 0.0f, 1.0f,  1.0f,  0.0f,  0.0f, -1.0f, -1.0f,  0.0f );

        OSD_DATA_STORAGE_CLASS const OSD_REAL Dds[8] = OSD_ARRAY_8(OSD_REAL, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f );
        OSD_DATA_STORAGE_CLASS const OSD_REAL Ddt[8] = OSD_ARRAY_8(OSD_REAL, 1.0f, 1.0f,  1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f );
        //  Combined weights for interior points -- (scaled) combinations of B, B', G and G':
        for (int k = 0; k < 8; ++k) {
            int iDst = interiorGregory[k];
            int tRow = interiorBezTRow[k];
            int sCol = interiorBezSCol[k];

            //  Quotient rule for G' (re-expressed in terms of G to simplify (and D = 1/D)):
            OSD_REAL Gds = (Nds[k] - Dds[k] * G[k]) * D[k];
            OSD_REAL Gdt = (Ndt[k] - Ddt[k] * G[k]) * D[k];

            //  Product rule combining B and B' with G and G':
            wDs[iDst] = (Bds[sCol] * G[k] + Bs[sCol] * Gds) * Bt[tRow];
            wDt[iDst] = (Bdt[tRow] * G[k] + Bt[tRow] * Gdt) * Bs[sCol];

            if (find_second_partials) {
                OSD_REAL Dsqr_inv = D[k]*D[k];

                OSD_REAL Gdss = 2.0f * Dds[k] * Dsqr_inv * (G[k] * Dds[k] - Nds[k]);
                OSD_REAL Gdst = Dsqr_inv * (2.0f * G[k] * Dds[k] * Ddt[k] - Nds[k] * Ddt[k] - Ndt[k] * Dds[k]);
                OSD_REAL Gdtt = 2.0f * Ddt[k] * Dsqr_inv * (G[k] * Ddt[k] - Ndt[k]);

                wDss[iDst] = (Bdss[sCol] * G[k] + 2.0f * Bds[sCol] * Gds + Bs[sCol] * Gdss) * Bt[tRow];
                wDst[iDst] =  Bt[tRow] * (Bs[sCol] * Gdst + Bds[sCol] * Gdt) +
                             Bdt[tRow] * (Bds[sCol] * G[k] + Bs[sCol] * Gds);
                wDtt[iDst] = (Bdtt[tRow] * G[k] + 2.0f * Bdt[tRow] * Gdt + Bt[tRow] * Gdtt) * Bs[sCol];
            }
        }
#endif
    }
    return 20;
}


OSD_FUNCTION_STORAGE_CLASS
// template <typename REAL>
int
Osd_EvalBasisLinearTri(OSD_REAL s, OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 3),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 3),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 3),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 3),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 3),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 3)) {

    if (OSD_OPTIONAL(wP)) {
        wP[0] = 1.0f - s - t;
        wP[1] = s;
        wP[2] = t;
    }
    if (OSD_OPTIONAL(wDs && wDt)) {
        wDs[0] = -1.0f;
        wDs[1] =  1.0f;
        wDs[2] =  0.0f;

        wDt[0] = -1.0f;
        wDt[1] =  0.0f;
        wDt[2] =  1.0f;

        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            wDss[0] = wDss[1] = wDss[2] = 0.0f;
            wDst[0] = wDst[1] = wDst[2] = 0.0f;
            wDtt[0] = wDtt[1] = wDtt[2] = 0.0f;
        }
    }
    return 3;
}


// namespace {
    OSD_FUNCTION_STORAGE_CLASS
    // template <typename REAL>
    void
    Osd_evalBivariateMonomialsQuartic(
        OSD_REAL s, OSD_REAL t,
        OSD_OUT_ARRAY(OSD_REAL, M, 15)) {

        M[0] = 1.0;

        M[1] = s;
        M[2] = t;

        M[3] = s * s;
        M[4] = s * t;
        M[5] = t * t;

        M[6] = M[3] * s;
        M[7] = M[4] * s;
        M[8] = M[4] * t;
        M[9] = M[5] * t;

        M[10] = M[6] * s;
        M[11] = M[7] * s;
        M[12] = M[3] * M[5];
        M[13] = M[8] * t;
        M[14] = M[9] * t;
    }

    OSD_FUNCTION_STORAGE_CLASS
    // template <typename REAL>
    void
    Osd_evalBoxSplineTriDerivWeights(
        OSD_INOUT_ARRAY(OSD_REAL, /*stMonomials*/M, 15),
        int ds, int dt,
        OSD_OUT_ARRAY(OSD_REAL, w, 12)) {

        // const OSD_REAL M[15] = stMonomials;

        OSD_REAL S = 1.0f;

        int totalOrder = ds + dt;
        if (totalOrder == 0) {
            S *= OSD_REAL_CAST(1.0 / 12.0);

            w[0]  = S * (1 - 2*M[1] - 4*M[2]          + 6*M[4] + 6*M[5] + 2*M[6]          - 6*M[8] - 4*M[9] -   M[10] - 2*M[11] + 2*M[13] +   M[14]);
            w[1]  = S * (1 + 2*M[1] - 2*M[2]          - 6*M[4]          - 4*M[6]          + 6*M[8] + 2*M[9] + 2*M[10] + 4*M[11] - 2*M[13] -   M[14]);
            w[2]  = S * (                                                 2*M[6]                            -   M[10] - 2*M[11]                    );
            w[3]  = S * (1 - 4*M[1] - 2*M[2] + 6*M[3] + 6*M[4]          - 4*M[6] - 6*M[7]          + 2*M[9] +   M[10] + 2*M[11] - 2*M[13] -   M[14]);
            w[4]  = S * (6                   -12*M[3] -12*M[4] -12*M[5] + 8*M[6] +12*M[7] +12*M[8] + 8*M[9] -   M[10] - 2*M[11] - 2*M[13] -   M[14]);
            w[5]  = S * (1 + 4*M[1] + 2*M[2] + 6*M[3] + 6*M[4]          - 4*M[6] - 6*M[7] -12*M[8] - 4*M[9] -   M[10] - 2*M[11] + 4*M[13] + 2*M[14]);
            w[6]  = S * (                                                                                       M[10] + 2*M[11]                    );
            w[7]  = S * (1 - 2*M[1] + 2*M[2]          - 6*M[4]          + 2*M[6] + 6*M[7]          - 4*M[9] -   M[10] - 2*M[11] + 4*M[13] + 2*M[14]);
            w[8]  = S * (1 + 2*M[1] + 4*M[2]          + 6*M[4] + 6*M[5] - 4*M[6] -12*M[7] - 6*M[8] - 4*M[9] + 2*M[10] + 4*M[11] - 2*M[13] -   M[14]);
            w[9]  = S * (                                                 2*M[6] + 6*M[7] + 6*M[8] + 2*M[9] -   M[10] - 2*M[11] - 2*M[13] -   M[14]);
            w[10] = S * (                                                                            2*M[9]                     - 2*M[13] -   M[14]);
            w[11] = S * (                                                                                                         2*M[13] +   M[14]);
        } else if (totalOrder == 1) {
            S *= OSD_REAL_CAST(1.0 / 6.0);

            if (ds != 0) {
                w[0]  = S * (-1          + 3*M[2] + 3*M[3]          - 3*M[5] - 2*M[6] - 3*M[7] +   M[9]);
                w[1]  = S * ( 1          - 3*M[2] - 6*M[3]          + 3*M[5] + 4*M[6] + 6*M[7] -   M[9]);
                w[2]  = S * (                       3*M[3]                   - 2*M[6] - 3*M[7]         );
                w[3]  = S * (-2 + 6*M[1] + 3*M[2] - 6*M[3] - 6*M[4]          + 2*M[6] + 3*M[7] -   M[9]);
                w[4]  = S * (   -12*M[1] - 6*M[2] +12*M[3] +12*M[4] + 6*M[5] - 2*M[6] - 3*M[7] -   M[9]);
                w[5]  = S * ( 2 + 6*M[1] + 3*M[2] - 6*M[3] - 6*M[4] - 6*M[5] - 2*M[6] - 3*M[7] + 2*M[9]);
                w[6]  = S * (                                                  2*M[6] + 3*M[7]         );
                w[7]  = S * (-1          - 3*M[2] + 3*M[3] + 6*M[4]          - 2*M[6] - 3*M[7] + 2*M[9]);
                w[8]  = S * ( 1          + 3*M[2] - 6*M[3] -12*M[4] - 3*M[5] + 4*M[6] + 6*M[7] -   M[9]);
                w[9]  = S * (                       3*M[3] + 6*M[4] + 3*M[5] - 2*M[6] - 3*M[7] -   M[9]);
                w[10] = S * (                                                                  -   M[9]);
                w[11] = S * (                                                                      M[9]);
            } else {
                w[0]  = S * (-2 + 3*M[1] + 6*M[2]          - 6*M[4] - 6*M[5]  -   M[6] + 3*M[8] + 2*M[9]);
                w[1]  = S * (-1 - 3*M[1]                   + 6*M[4] + 3*M[5]  + 2*M[6] - 3*M[8] - 2*M[9]);
                w[2]  = S * (                                                 -   M[6]                  );
                w[3]  = S * (-1 + 3*M[1]          - 3*M[3]          + 3*M[5]  +   M[6] - 3*M[8] - 2*M[9]);
                w[4]  = S * (   - 6*M[1] -12*M[2] + 6*M[3] +12*M[4] +12*M[5]  -   M[6] - 3*M[8] - 2*M[9]);
                w[5]  = S * ( 1 + 3*M[1]          - 3*M[3] -12*M[4] - 6*M[5]  -   M[6] + 6*M[8] + 4*M[9]);
                w[6]  = S * (                                                 +   M[6]                  );
                w[7]  = S * ( 1 - 3*M[1]          + 3*M[3]          - 6*M[5]  -   M[6] + 6*M[8] + 4*M[9]);
                w[8]  = S * ( 2 + 3*M[1] + 6*M[2] - 6*M[3] - 6*M[4] - 6*M[5]  + 2*M[6] - 3*M[8] - 2*M[9]);
                w[9]  = S * (                     + 3*M[3] + 6*M[4] + 3*M[5]  -   M[6] - 3*M[8] - 2*M[9]);
                w[10] = S * (                                         3*M[5]           - 3*M[8] - 2*M[9]);
                w[11] = S * (                                                            3*M[8] + 2*M[9]);
            }
        } else if (totalOrder == 2) {
            if (ds == 2) {
                w[0]  = S * (   +   M[1]          -   M[3] -   M[4]);
                w[1]  = S * (   - 2*M[1]          + 2*M[3] + 2*M[4]);
                w[2]  = S * (       M[1]          -   M[3] -   M[4]);
                w[3]  = S * ( 1 - 2*M[1] -   M[2] +   M[3] +   M[4]);
                w[4]  = S * (-2 + 4*M[1] + 2*M[2] -   M[3] -   M[4]);
                w[5]  = S * ( 1 - 2*M[1] -   M[2] -   M[3] -   M[4]);
                w[6]  = S * (                         M[3] +   M[4]);
                w[7]  = S * (   +   M[1] +   M[2] -   M[3] -   M[4]);
                w[8]  = S * (   - 2*M[1] - 2*M[2] + 2*M[3] + 2*M[4]);
                w[9]  = S * (       M[1] +   M[2] -   M[3] -   M[4]);
                w[10] =     0;
                w[11] =     0;
            } else if (dt == 2) {
                w[0]  = S * ( 1 -   M[1] - 2*M[2] +   M[4] +   M[5]);
                w[1]  = S * (   +   M[1] +   M[2] -   M[4] -   M[5]);
                w[2]  =     0;
                w[3]  = S * (            +   M[2] -   M[4] -   M[5]);
                w[4]  = S * (-2 + 2*M[1] + 4*M[2] -   M[4] -   M[5]);
                w[5]  = S * (   - 2*M[1] - 2*M[2] + 2*M[4] + 2*M[5]);
                w[6]  =     0;
                w[7]  = S * (            - 2*M[2] + 2*M[4] + 2*M[5]);
                w[8]  = S * ( 1 -   M[1] - 2*M[2] -   M[4] -   M[5]);
                w[9]  = S * (   +   M[1] +   M[2] -   M[4] -   M[5]);
                w[10] = S * (                M[2] -   M[4] -   M[5]);
                w[11] = S * (                         M[4] +   M[5]);
            } else {
                S *= OSD_REAL_CAST(1.0 / 2.0);

                w[0]  = S * ( 1          - 2*M[2] -   M[3] +   M[5]);
                w[1]  = S * (-1          + 2*M[2] + 2*M[3] -   M[5]);
                w[2]  = S * (                     -   M[3]         );
                w[3]  = S * ( 1 - 2*M[1]          +   M[3] -   M[5]);
                w[4]  = S * (-2 + 4*M[1] + 4*M[2] -   M[3] -   M[5]);
                w[5]  = S * ( 1 - 2*M[1] - 4*M[2] -   M[3] + 2*M[5]);
                w[6]  = S * (                     +   M[3]         );
                w[7]  = S * (-1 + 2*M[1]          -   M[3] + 2*M[5]);
                w[8]  = S * ( 1 - 4*M[1] - 2*M[2] + 2*M[3] -   M[5]);
                w[9]  = S * (   + 2*M[1] + 2*M[2] -   M[3] -   M[5]);
                w[10] = S * (                              -   M[5]);
                w[11] = S * (                                  M[5]);
            }
        } else {
            // assert(totalOrder <= 2);
        }
    }

    OSD_FUNCTION_STORAGE_CLASS
    // template <typename REAL>
    void
    Osd_adjustBoxSplineTriBoundaryWeights(
        int boundaryMask,
        OSD_INOUT_ARRAY(OSD_REAL, weights, 12)) {

        if (boundaryMask == 0) return;

        //
        //  Determine boundary edges and vertices from the lower 3 and upper
        //  2 bits of the 5-bit mask:
        //
        int upperBits = (boundaryMask >> 3) & 0x3;
        int lowerBits = boundaryMask & 7;

        int eBits = lowerBits;
        int vBits = 0;

        if (upperBits == 1) {
            //  Boundary vertices only:
            vBits = eBits;
            eBits = 0;
        } else if (upperBits == 2) {
            //  Opposite vertex bit is edge bit rotated one to the right:
            vBits = ((eBits & 1) << 2) | (eBits >> 1);
        }

        bool edge0IsBoundary = (eBits & 1) != 0;
        bool edge1IsBoundary = (eBits & 2) != 0;
        bool edge2IsBoundary = (eBits & 4) != 0;

        //
        //  Adjust weights for the 4 boundary points and 3 interior points
        //  to account for the 3 phantom points adjacent to each
        //  boundary edge:
        //
        if (edge0IsBoundary) {
            OSD_REAL w0 = weights[0];
            if (edge2IsBoundary) {
                //  P0 = B1 + (B1 - I1)
                weights[4] += w0;
                weights[4] += w0;
                weights[8] -= w0;
            } else {
                //  P0 = B1 + (B0 - I0)
                weights[4] += w0;
                weights[3] += w0;
                weights[7] -= w0;
            }

            //  P1 = B1 + (B2 - I1)
            OSD_REAL w1 = weights[1];
            weights[4] += w1;
            weights[5] += w1;
            weights[8] -= w1;

            OSD_REAL w2 = weights[2];
            if (edge1IsBoundary) {
                //  P2 = B2 + (B2 - I1)
                weights[5] += w2;
                weights[5] += w2;
                weights[8] -= w2;
            } else {
                //  P2 = B2 + (B3 - I2)
                weights[5] += w2;
                weights[6] += w2;
                weights[9] -= w2;
            }
            //  Clear weights for the phantom points:
            weights[0] = weights[1] = weights[2] = 0.0f;
        }
        if (edge1IsBoundary) {
            OSD_REAL w0 = weights[6];
            if (edge0IsBoundary) {
                //  P0 = B1 + (B1 - I1)
                weights[5] += w0;
                weights[5] += w0;
                weights[4] -= w0;
            } else {
                //  P0 = B1 + (B0 - I0)
                weights[5] += w0;
                weights[2] += w0;
                weights[1] -= w0;
            }

            //  P1 = B1 + (B2 - I1)
            OSD_REAL w1 = weights[9];
            weights[5] += w1;
            weights[8] += w1;
            weights[4] -= w1;

            OSD_REAL w2 = weights[11];
            if (edge2IsBoundary) {
                //  P2 = B2 + (B2 - I1)
                weights[8] += w2;
                weights[8] += w2;
                weights[4] -= w2;
            } else {
                //  P2 = B2 + (B3 - I2)
                weights[8]  += w2;
                weights[10] += w2;
                weights[7]  -= w2;
            }
            //  Clear weights for the phantom points:
            weights[6] = weights[9] = weights[11] = 0.0f;
        }
        if (edge2IsBoundary) {
            OSD_REAL w0 = weights[10];
            if (edge1IsBoundary) {
                //  P0 = B1 + (B1 - I1)
                weights[8] += w0;
                weights[8] += w0;
                weights[5] -= w0;
            } else {
                //  P0 = B1 + (B0 - I0)
                weights[8]  += w0;
                weights[11] += w0;
                weights[9]  -= w0;
            }

            //  P1 = B1 + (B2 - I1)
            OSD_REAL w1 = weights[7];
            weights[8] += w1;
            weights[4] += w1;
            weights[5] -= w1;

            OSD_REAL w2 = weights[3];
            if (edge0IsBoundary) {
                //  P2 = B2 + (B2 - I1)
                weights[4] += w2;
                weights[4] += w2;
                weights[5] -= w2;
            } else {
                //  P2 = B2 + (B3 - I2)
                weights[4] += w2;
                weights[0] += w2;
                weights[1] -= w2;
            }
            //  Clear weights for the phantom points:
            weights[10] = weights[7] = weights[3] = 0.0f;
        }

        //
        //  Adjust weights for the 3 boundary points and the 2 interior
        //  points to account for the 2 phantom points adjacent to
        //  each boundary vertex:
        //
        if ((vBits & 1) != 0) {
            //  P0 = B1 + (B0 - I0)
            OSD_REAL w0 = weights[3];
            weights[4] += w0;
            weights[7] += w0;
            weights[8] -= w0;

            //  P1 = B1 + (B2 - I1)
            OSD_REAL w1 = weights[0];
            weights[4] += w1;
            weights[1] += w1;
            weights[5] -= w1;

            //  Clear weights for the phantom points:
            weights[3] = weights[0] = 0.0f;
        }
        if ((vBits & 2) != 0) {
            //  P0 = B1 + (B0 - I0)
            OSD_REAL w0 = weights[2];
            weights[5] += w0;
            weights[1] += w0;
            weights[4] -= w0;

            //  P1 = B1 + (B2 - I1)
            OSD_REAL w1 = weights[6];
            weights[5] += w1;
            weights[9] += w1;
            weights[8] -= w1;

            //  Clear weights for the phantom points:
            weights[2] = weights[6] = 0.0f;
        }
        if ((vBits & 4) != 0) {
            //  P0 = B1 + (B0 - I0)
            OSD_REAL w0 = weights[11];
            weights[8] += w0;
            weights[9] += w0;
            weights[5] -= w0;

            //  P1 = B1 + (B2 - I1)
            OSD_REAL w1 = weights[10];
            weights[8] += w1;
            weights[7] += w1;
            weights[4] -= w1;

            //  Clear weights for the phantom points:
            weights[11] = weights[10] = 0.0f;
        }
    }

    OSD_FUNCTION_STORAGE_CLASS
    // template <typename REAL>
    void
    Osd_boundBasisBoxSplineTri(
            int boundary,
            OSD_INOUT_ARRAY(OSD_REAL, wP, 12),
            OSD_INOUT_ARRAY(OSD_REAL, wDs, 12),
            OSD_INOUT_ARRAY(OSD_REAL, wDt, 12),
            OSD_INOUT_ARRAY(OSD_REAL, wDss, 12),
            OSD_INOUT_ARRAY(OSD_REAL, wDst, 12),
            OSD_INOUT_ARRAY(OSD_REAL, wDtt, 12)) {

        if (OSD_OPTIONAL(wP)) {
            Osd_adjustBoxSplineTriBoundaryWeights(boundary, wP);
        }
        if (OSD_OPTIONAL(wDs && wDt)) {
            Osd_adjustBoxSplineTriBoundaryWeights(boundary, wDs);
            Osd_adjustBoxSplineTriBoundaryWeights(boundary, wDt);

            if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
                Osd_adjustBoxSplineTriBoundaryWeights(boundary, wDss);
                Osd_adjustBoxSplineTriBoundaryWeights(boundary, wDst);
                Osd_adjustBoxSplineTriBoundaryWeights(boundary, wDtt);
            }
        }
    }
// }  // namespace

OSD_FUNCTION_STORAGE_CLASS
// template <typename REAL>
int
Osd_EvalBasisBoxSplineTri(OSD_REAL s, OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 12),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 12),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 12),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 12),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 12),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 12)) {

    OSD_REAL stMonomials[15];
    Osd_evalBivariateMonomialsQuartic(s, t, stMonomials);

    if (OSD_OPTIONAL(wP)) {
        Osd_evalBoxSplineTriDerivWeights(stMonomials, 0, 0, wP);
    }
    if (OSD_OPTIONAL(wDs && wDt)) {
        Osd_evalBoxSplineTriDerivWeights(stMonomials, 1, 0, wDs);
        Osd_evalBoxSplineTriDerivWeights(stMonomials, 0, 1, wDt);

        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            Osd_evalBoxSplineTriDerivWeights(stMonomials, 2, 0, wDss);
            Osd_evalBoxSplineTriDerivWeights(stMonomials, 1, 1, wDst);
            Osd_evalBoxSplineTriDerivWeights(stMonomials, 0, 2, wDtt);
        }
    }
    return 12;
}


// namespace {
    OSD_FUNCTION_STORAGE_CLASS
    // template <typename REAL>
    void
    Osd_evalBezierTriDerivWeights(
        OSD_REAL s, OSD_REAL t, int ds, int dt,
        OSD_OUT_ARRAY(OSD_REAL, wB, 15)) {

        OSD_REAL u  = s;
        OSD_REAL v  = t;
        OSD_REAL w  = 1 - u - v;

        OSD_REAL uu = u * u;
        OSD_REAL vv = v * v;
        OSD_REAL ww = w * w;

        OSD_REAL uv = u * v;
        OSD_REAL vw = v * w;
        OSD_REAL uw = u * w;

        int totalOrder = ds + dt;
        if (totalOrder == 0) {
            wB[0]  =      ww * ww;
            wB[1]  =  4 * uw * ww;
            wB[2]  =  6 * uw * uw;
            wB[3]  =  4 * uw * uu;
            wB[4]  =      uu * uu;
            wB[5]  =  4 * vw * ww;
            wB[6]  = 12 * ww * uv;
            wB[7]  = 12 * uu * vw;
            wB[8]  =  4 * uv * uu;
            wB[9]  =  6 * vw * vw;
            wB[10] = 12 * vv * uw;
            wB[11] =  6 * uv * uv;
            wB[12] =  4 * vw * vv;
            wB[13] =  4 * uv * vv;
            wB[14] =      vv * vv;
        } else if (totalOrder == 1) {
            if (ds == 1) {
                wB[0]  =  -4 * ww * w;
                wB[1]  =   4 * ww * (w - 3 * u);
                wB[2]  =  12 * uw * (w - u);
                wB[3]  =   4 * uu * (3 * w - u);
                wB[4]  =   4 * uu * u;
                wB[5]  = -12 * vw * w;
                wB[6]  =  12 * vw * (w - 2 * u);
                wB[7]  =  12 * uv * (2 * w - u);
                wB[8]  =  12 * uv * u;
                wB[9]  = -12 * vv * w;
                wB[10] =  12 * vv * (w - u);
                wB[11] =  12 * vv * u;
                wB[12] =  -4 * vv * v;
                wB[13] =   4 * vv * v;
                wB[14] =   0;
            } else {
                wB[0]  =  -4 * ww * w;
                wB[1]  = -12 * ww * u;
                wB[2]  = -12 * uu * w;
                wB[3]  =  -4 * uu * u;
                wB[4]  =   0;
                wB[5]  =   4 * ww * (w - 3 * v);
                wB[6]  =  12 * uw * (w - 2 * v);
                wB[7]  =  12 * uu * (w - v);
                wB[8]  =   4 * uu * u;
                wB[9]  =  12 * vw * (w - v);
                wB[10] =  12 * uv * (2 * w - v);
                wB[11] =  12 * uv * u;;
                wB[12] =   4 * vv * (3 * w - v);
                wB[13] =  12 * vv * u;
                wB[14] =   4 * vv * v;
            }
        } else if (totalOrder == 2) {
            if (ds == 2) {
                wB[0]  =  12 * ww;
                wB[1]  =  24 * (uw - ww);
                wB[2]  =  12 * (uu - 4 * uw + ww);
                wB[3]  =  24 * (uw - uu);
                wB[4]  =  12 * uu;
                wB[5]  =  24 * vw;
                wB[6]  =  24 * (uv - 2 * vw);
                wB[7]  =  24 * (vw - 2 * uv);
                wB[8]  =  24 * uv;
                wB[9]  =  12 * vv;
                wB[10] = -24 * vv;
                wB[11] =  12 * vv;
                wB[12] =   0;
                wB[13] =   0;
                wB[14] =   0;
            } else if (dt == 2) {
                wB[0]  =  12 * ww;
                wB[1]  =  24 * uw;
                wB[2]  =  12 * uu;
                wB[3]  =   0;
                wB[4]  =   0;
                wB[5]  =  24 * (vw - ww);
                wB[6]  =  24 * (uv - 2 * uw);
                wB[7]  = -24 * uu;
                wB[8]  =   0;
                wB[9]  =  12 * (vv - 4 * vw + ww);
                wB[10] =  24 * (uw - 2 * uv);
                wB[11] =  12 * uu;
                wB[12] =  24 * (vw - vv);
                wB[13] =  24 * uv;
                wB[14] =  12 * vv;
            } else {
                wB[0]  =  12 * ww;
                wB[3]  = -12 * uu;
                wB[13] =  12 * vv;
                wB[11] =  24 * uv;
                wB[1]  =  24 * uw - wB[0];
                wB[2]  = -24 * uw - wB[3];
                wB[5]  =  24 * vw - wB[0];
                wB[6]  = -24 * vw + wB[11] - wB[1];
                wB[8]  = - wB[3];
                wB[7]  = -(wB[11] + wB[2]);
                wB[9]  =   wB[13] - wB[5] - wB[0];
                wB[10] = -(wB[9] + wB[11]);
                wB[12] = - wB[13];
                wB[4]  =   0;
                wB[14] =   0;
            }
        } else {
            // assert(totalOrder <= 2);
        }
    }
// } // end namespace

OSD_FUNCTION_STORAGE_CLASS
// template <typename REAL>
int
Osd_EvalBasisBezierTri(OSD_REAL s, OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 15),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 15),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 15),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 15),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 15),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 15)) {

    if (OSD_OPTIONAL(wP)) {
        Osd_evalBezierTriDerivWeights(s, t, 0, 0, wP);
    }
    if (OSD_OPTIONAL(wDs && wDt)) {
        Osd_evalBezierTriDerivWeights(s, t, 1, 0, wDs);
        Osd_evalBezierTriDerivWeights(s, t, 0, 1, wDt);

        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            Osd_evalBezierTriDerivWeights(s, t, 2, 0, wDss);
            Osd_evalBezierTriDerivWeights(s, t, 1, 1, wDst);
            Osd_evalBezierTriDerivWeights(s, t, 0, 2, wDtt);
        }
    }
    return 15;
}


// namespace {
    //
    //  Expanding a set of 15 Bezier basis functions for the 6 (3 pairs) of
    //  rational weights for the 18 Gregory basis functions:
    //
    OSD_FUNCTION_STORAGE_CLASS
    // template <typename REAL>
    void
    Osd_convertBezierWeightsToGregory(
        OSD_INOUT_ARRAY(OSD_REAL, wB, 15),
        OSD_INOUT_ARRAY(OSD_REAL, rG,  6),
        OSD_OUT_ARRAY(OSD_REAL, wG, 18)) {

        wG[0]  = wB[0];
        wG[1]  = wB[1];
        wG[2]  = wB[5];
        wG[3]  = wB[6] * rG[0];
        wG[4]  = wB[6] * rG[1];

        wG[5]  = wB[4];
        wG[6]  = wB[8];
        wG[7]  = wB[3];
        wG[8]  = wB[7] * rG[2];
        wG[9]  = wB[7] * rG[3];

        wG[10] = wB[14];
        wG[11] = wB[12];
        wG[12] = wB[13];
        wG[13] = wB[10] * rG[4];
        wG[14] = wB[10] * rG[5];

        wG[15] = wB[2];
        wG[16] = wB[11];
        wG[17] = wB[9];
    }
// } // end namespace

OSD_FUNCTION_STORAGE_CLASS
// template <typename REAL>
int
Osd_EvalBasisGregoryTri(OSD_REAL s, OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 18),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 18),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 18),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 18),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 18),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 18)) {

    //
    //  Bezier basis functions are denoted with B while the rational multipliers for the
    //  interior points will be denoted G -- so we have B(s,t) and G(s,t) (though we
    //  switch to barycentric (u,v,w) briefly to compute G)
    //
    OSD_REAL BP[15], BDs[15], BDt[15], BDss[15], BDst[15], BDtt[15];

    OSD_REAL G[6] = OSD_ARRAY_6(OSD_REAL, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f );
    OSD_REAL u = s;
    OSD_REAL v = t;
    OSD_REAL w = 1 - u - v;

    if ((u + v) > 0) {
        G[0]  = u / (u + v);
        G[1]  = v / (u + v);
    }
    if ((v + w) > 0) {
        G[2] = v / (v + w);
        G[3] = w / (v + w);
    }
    if ((w + u) > 0) {
        G[4] = w / (w + u);
        G[5] = u / (w + u);
    }

    //
    //  Compute Bezier basis functions and convert, adjusting interior points:
    //
    if (OSD_OPTIONAL(wP)) {
        Osd_evalBezierTriDerivWeights(s, t, 0, 0, BP);
        Osd_convertBezierWeightsToGregory(BP, G, wP);
    }
    if (OSD_OPTIONAL(wDs && wDt)) {
        //  TBD -- ifdef OPENSUBDIV_GREGORY_EVAL_TRUE_DERIVATIVES

        Osd_evalBezierTriDerivWeights(s, t, 1, 0, BDs);
        Osd_evalBezierTriDerivWeights(s, t, 0, 1, BDt);

        Osd_convertBezierWeightsToGregory(BDs, G, wDs);
        Osd_convertBezierWeightsToGregory(BDt, G, wDt);

        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            Osd_evalBezierTriDerivWeights(s, t, 2, 0, BDss);
            Osd_evalBezierTriDerivWeights(s, t, 1, 1, BDst);
            Osd_evalBezierTriDerivWeights(s, t, 0, 2, BDtt);

            Osd_convertBezierWeightsToGregory(BDss, G, wDss);
            Osd_convertBezierWeightsToGregory(BDst, G, wDst);
            Osd_convertBezierWeightsToGregory(BDtt, G, wDtt);
        }
    }
    return 18;
}

// The following functions are low-level internal methods which
// were exposed in earlier releases, but were never intended to
// be part of the supported public API.

//  Deprecated -- prefer use of OsdEvaluatePatchBasis() and
//  OsdEvaluatePatchBasisNormalized() methods.
OSD_FUNCTION_STORAGE_CLASS
void
OsdGetBezierWeights(
    OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 4),
    OSD_OUT_ARRAY(OSD_REAL, wDP, 4),
    OSD_OUT_ARRAY(OSD_REAL, wDP2, 4)) {

    Osd_evalBezierCurve(t, wP, wDP, wDP2);
}

//  Deprecated -- prefer use of OsdEvaluatePatchBasis() and
//  OsdEvaluatePatchBasisNormalized() methods.
OSD_FUNCTION_STORAGE_CLASS
void
OsdGetBSplineWeights(
    OSD_REAL t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 4),
    OSD_OUT_ARRAY(OSD_REAL, wDP, 4),
    OSD_OUT_ARRAY(OSD_REAL, wDP2, 4)) {

    Osd_evalBSplineCurve(t, wP, wDP, wDP2);
}

//  Deprecated -- prefer use of OsdEvaluatePatchBasis() and
//  OsdEvaluatePatchBasisNormalized() methods.
OSD_FUNCTION_STORAGE_CLASS
void
OsdGetBoxSplineWeights(
    float s, float t,
    OSD_OUT_ARRAY(OSD_REAL, wP, 12)) {

    OSD_REAL stMonomials[15];
    Osd_evalBivariateMonomialsQuartic(s, t, stMonomials);

    if (OSD_OPTIONAL(wP)) {
        Osd_evalBoxSplineTriDerivWeights(stMonomials, 0, 0, wP);
    }
}

//  Deprecated -- prefer use of OsdEvaluatePatchBasis() and
//  OsdEvaluatePatchBasisNormalized() methods.
OSD_FUNCTION_STORAGE_CLASS
void
OsdAdjustBoundaryWeights(
        int boundary,
        OSD_INOUT_ARRAY(OSD_REAL, sWeights, 4),
        OSD_INOUT_ARRAY(OSD_REAL, tWeights, 4)) {

    if ((boundary & 1) != 0) {
        tWeights[2] -= tWeights[0];
        tWeights[1] += tWeights[0] * 2.0f;
        tWeights[0]  = 0.0f;
    }
    if ((boundary & 2) != 0) {
        sWeights[1] -= sWeights[3];
        sWeights[2] += sWeights[3] * 2.0f;
        sWeights[3]  = 0.0f;
    }
    if ((boundary & 4) != 0) {
        tWeights[1] -= tWeights[3];
        tWeights[2] += tWeights[3] * 2.0f;
        tWeights[3]  = 0.0f;
    }
    if ((boundary & 8) != 0) {
        sWeights[2] -= sWeights[0];
        sWeights[1] += sWeights[0] * 2.0f;
        sWeights[0]  = 0.0f;
    }
}

//  Deprecated -- prefer use of OsdEvaluatePatchBasis() and
//  OsdEvaluatePatchBasisNormalized() methods.
OSD_FUNCTION_STORAGE_CLASS
void
OsdComputeTensorProductPatchWeights(
    float dScale, int boundary,
    OSD_IN_ARRAY(float, sWeights, 4),
    OSD_IN_ARRAY(float, tWeights, 4),
    OSD_IN_ARRAY(float, dsWeights, 4),
    OSD_IN_ARRAY(float, dtWeights, 4),
    OSD_IN_ARRAY(float, dssWeights, 4),
    OSD_IN_ARRAY(float, dttWeights, 4),
    OSD_OUT_ARRAY(float, wP, 16),
    OSD_OUT_ARRAY(float, wDs, 16),
    OSD_OUT_ARRAY(float, wDt, 16),
    OSD_OUT_ARRAY(float, wDss, 16),
    OSD_OUT_ARRAY(float, wDst, 16),
    OSD_OUT_ARRAY(float, wDtt, 16)) {

    if (OSD_OPTIONAL(wP)) {
        // Compute the tensor product weight of the (s,t) basis function
        // corresponding to each control vertex:

        OsdAdjustBoundaryWeights(boundary, sWeights, tWeights);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                wP[4*i+j] = sWeights[j] * tWeights[i];
            }
        }
    }

    if (OSD_OPTIONAL(wDs && wDt)) {
        // Compute the tensor product weight of the differentiated (s,t) basis
        // function corresponding to each control vertex (scaled accordingly):

        OsdAdjustBoundaryWeights(boundary, dsWeights, dtWeights);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                wDs[4*i+j] = dsWeights[j] * tWeights[i] * dScale;
                wDt[4*i+j] = sWeights[j] * dtWeights[i] * dScale;
            }
        }

        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            // Compute the tensor product weight of appropriate differentiated
            // (s,t) basis functions for each control vertex (scaled accordingly):
            float d2Scale = dScale * dScale;

            OsdAdjustBoundaryWeights(boundary, dssWeights, dttWeights);

            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    wDss[4*i+j] = dssWeights[j] * tWeights[i] * d2Scale;
                    wDst[4*i+j] = dsWeights[j] * dtWeights[i] * d2Scale;
                    wDtt[4*i+j] = sWeights[j] * dttWeights[i] * d2Scale;
                }
            }
        }
    }
}

//  Deprecated -- prefer use of OsdEvaluatePatchBasis() and
//  OsdEvaluatePatchBasisNormalized() methods.
OSD_FUNCTION_STORAGE_CLASS
void
OsdGetBilinearPatchWeights(
        OSD_REAL s, OSD_REAL t, OSD_REAL d1Scale,
        OSD_OUT_ARRAY(OSD_REAL, wP, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDs, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDt, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDss, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDst, 4),
        OSD_OUT_ARRAY(OSD_REAL, wDtt, 4)) {

    int nPoints = Osd_EvalBasisLinear(s, t, wP, wDs, wDt, wDss, wDst, wDtt);
    if (OSD_OPTIONAL(wDs && wDt)) {
        for (int i = 0; i < nPoints; ++i) {
            wDs[i] *= d1Scale;
            wDt[i] *= d1Scale;
        }
        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            OSD_REAL d2Scale = d1Scale * d1Scale;

            for (int i = 0; i < nPoints; ++i) {
                wDss[i] *= d2Scale;
                wDst[i] *= d2Scale;
                wDtt[i] *= d2Scale;
            }
        }
    }
}

//  Deprecated -- prefer use of OsdEvaluatePatchBasis() and
//  OsdEvaluatePatchBasisNormalized() methods.
OSD_FUNCTION_STORAGE_CLASS
void
OsdGetBSplinePatchWeights(
    OSD_REAL s, OSD_REAL t, OSD_REAL d1Scale, int boundary,
    OSD_OUT_ARRAY(OSD_REAL, wP, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 16)) {

    int nPoints = Osd_EvalBasisBSpline(s, t, wP, wDs, wDt, wDss, wDst, wDtt);
    Osd_boundBasisBSpline(boundary, wP, wDs, wDt, wDss, wDst, wDtt);
    if (OSD_OPTIONAL(wDs && wDt)) {
        for (int i = 0; i < nPoints; ++i) {
            wDs[i] *= d1Scale;
            wDt[i] *= d1Scale;
        }
        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            OSD_REAL d2Scale = d1Scale * d1Scale;

            for (int i = 0; i < nPoints; ++i) {
                wDss[i] *= d2Scale;
                wDst[i] *= d2Scale;
                wDtt[i] *= d2Scale;
            }
        }
    }
}

//  Deprecated -- prefer use of OsdEvaluatePatchBasis() and
//  OsdEvaluatePatchBasisNormalized() methods.
OSD_FUNCTION_STORAGE_CLASS
void
OsdGetBezierPatchWeights(
    OSD_REAL s, OSD_REAL t, OSD_REAL d1Scale,
    OSD_OUT_ARRAY(OSD_REAL, wP, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 16),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 16)) {
    int nPoints = Osd_EvalBasisBezier(s, t, wP, wDs, wDt, wDss, wDst, wDtt);
    if (OSD_OPTIONAL(wDs && wDt)) {
        for (int i = 0; i < nPoints; ++i) {
            wDs[i] *= d1Scale;
            wDt[i] *= d1Scale;
        }
        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            OSD_REAL d2Scale = d1Scale * d1Scale;

            for (int i = 0; i < nPoints; ++i) {
                wDss[i] *= d2Scale;
                wDst[i] *= d2Scale;
                wDtt[i] *= d2Scale;
            }
        }
    }
}

//  Deprecated -- prefer use of OsdEvaluatePatchBasis() and
//  OsdEvaluatePatchBasisNormalized() methods.
OSD_FUNCTION_STORAGE_CLASS
void
OsdGetGregoryPatchWeights(
    OSD_REAL s, OSD_REAL t, OSD_REAL d1Scale,
    OSD_OUT_ARRAY(OSD_REAL, wP, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDs, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDt, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDss, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDst, 20),
    OSD_OUT_ARRAY(OSD_REAL, wDtt, 20)) {
    int nPoints = Osd_EvalBasisGregory(s, t, wP, wDs, wDt, wDss, wDst, wDtt);
    if (OSD_OPTIONAL(wDs && wDt)) {
        for (int i = 0; i < nPoints; ++i) {
            wDs[i] *= d1Scale;
            wDt[i] *= d1Scale;
        }
        if (OSD_OPTIONAL(wDss && wDst && wDtt)) {
            OSD_REAL d2Scale = d1Scale * d1Scale;

            for (int i = 0; i < nPoints; ++i) {
                wDss[i] *= d2Scale;
                wDst[i] *= d2Scale;
                wDtt[i] *= d2Scale;
            }
        }
    }
}

#endif /* OPENSUBDIV3_OSD_PATCH_BASIS_H */
