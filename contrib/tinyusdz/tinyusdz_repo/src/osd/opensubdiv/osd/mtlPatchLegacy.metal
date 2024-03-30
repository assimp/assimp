#line 0 "osd/mtlPatchBoxSplineTriangle.metal"

//
//   Copyright 2015-2019 Pixar
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

// ----------------------------------------------------------------------------
// Legacy Gregory
// ----------------------------------------------------------------------------
#if defined(OSD_PATCH_GREGORY) || defined(OSD_PATCH_GREGORY_BOUNDARY)

// precomputed catmark coefficient table up to valence 29
constant float OsdCatmarkCoefficient[30] = {
    0, 0, 0, 0.812816, 0.500000, 0.363644, 0.287514,
    0.238688, 0.204544, 0.179229, 0.159657,
    0.144042, 0.131276, 0.120632, 0.111614,
    0.103872, 0.09715, 0.0912559, 0.0860444,
    0.0814022, 0.0772401, 0.0734867, 0.0700842,
    0.0669851, 0.0641504, 0.0615475, 0.0591488,
    0.0569311, 0.0548745, 0.0529621
};

float
OsdComputeCatmarkCoefficient(int valence)
{
#if OSD_MAX_VALENCE < 30
    return OsdCatmarkCoefficient[valence];
#else
    if (valence < 30) {
        return OsdCatmarkCoefficient[valence];
    } else {
        float t = 2.0f / float(valence);
        return 1.0f / (valence * (cospi(t) + 5.0f +
                                  sqrt((cospi(t) + 9) * (cospi(t) + 1)))/16.0f);
    }
#endif
}

float cosfn(int n, int j) {
    return cospi((2.0f * j)/float(n));
}

float sinfn(int n, int j) {
    return sinpi((2.0f * j)/float(n));
}

#ifndef OSD_MAX_VALENCE
#define OSD_MAX_VALENCE 4
#endif

template<typename OsdVertexBuffer>
float3 OsdReadVertex(int vertexIndex, OsdVertexBuffer osdVertexBuffer)
{
    int index = (vertexIndex /*+ OsdBaseVertex()*/);
    return osdVertexBuffer[index].position;
}

template<typename OsdValenceBuffer>
int OsdReadVertexValence(int vertexID, OsdValenceBuffer osdValenceBuffer)
{
    int index = int(vertexID * (2 * OSD_MAX_VALENCE + 1));
    return osdValenceBuffer[index];
}

template<typename OsdValenceBuffer>
int OsdReadVertexIndex(int vertexID, int valenceVertex, OsdValenceBuffer osdValenceBuffer)
{
    int index = int(vertexID * (2 * OSD_MAX_VALENCE + 1) + 1 + valenceVertex);
    return osdValenceBuffer[index];
}

template<typename OsdQuadOffsetBuffer>
int OsdReadQuadOffset(int primitiveID, int offsetVertex, OsdQuadOffsetBuffer osdQuadOffsetBuffer)
{
    int index = int(4*primitiveID + offsetVertex);
    return osdQuadOffsetBuffer[index];
}


void OsdComputePerVertexGregory(unsigned vID, float3 P, threadgroup OsdPerVertexGregory& v, OsdPatchParamBufferSet osdBuffers)
{
    v.clipFlag = short3(0,0,0);

    int ivalence = OsdReadVertexValence(vID, osdBuffers.valenceBuffer);
    v.valence = ivalence;
    int valence = abs(ivalence);

    float3 f[OSD_MAX_VALENCE];
    float3 pos = P;
    float3 opos = float3(0,0,0);

#ifdef OSD_PATCH_GREGORY_BOUNDARY
    v.org = pos;
    int boundaryEdgeNeighbors[2];
    int currNeighbor = 0;
    int ibefore = 0;
    int zerothNeighbor = 0;
#endif

    for (int i=0; i<valence; ++i) {
        int im = (i+valence-1)%valence;
        int ip = (i+1)%valence;

        int idx_neighbor = OsdReadVertexIndex(vID, 2*i, osdBuffers.valenceBuffer);

#ifdef OSD_PATCH_GREGORY_BOUNDARY
        bool isBoundaryNeighbor = false;
        int valenceNeighbor = OsdReadVertexValence(idx_neighbor, osdBuffers.valenceBuffer);

        if (valenceNeighbor < 0) {
            isBoundaryNeighbor = true;
            if (currNeighbor<2) {
                boundaryEdgeNeighbors[currNeighbor] = idx_neighbor;
            }
            currNeighbor++;
            if (currNeighbor == 1) {
                ibefore = i;
                zerothNeighbor = i;
            } else {
                if (i-ibefore == 1) {
                    int tmp = boundaryEdgeNeighbors[0];
                    boundaryEdgeNeighbors[0] = boundaryEdgeNeighbors[1];
                    boundaryEdgeNeighbors[1] = tmp;
                    zerothNeighbor = i;
                }
            }
        }
#endif

        float3 neighbor = OsdReadVertex(idx_neighbor, osdBuffers.vertexBuffer);

        int idx_diagonal = OsdReadVertexIndex(vID, 2*i + 1, osdBuffers.valenceBuffer);
        float3 diagonal = OsdReadVertex(idx_diagonal, osdBuffers.vertexBuffer);

        int idx_neighbor_p = OsdReadVertexIndex(vID, 2*ip, osdBuffers.valenceBuffer);
        float3 neighbor_p = OsdReadVertex(idx_neighbor_p, osdBuffers.vertexBuffer);

        int idx_neighbor_m = OsdReadVertexIndex(vID, 2*im, osdBuffers.valenceBuffer);
        float3 neighbor_m = OsdReadVertex(idx_neighbor_m, osdBuffers.vertexBuffer);

        int idx_diagonal_m = OsdReadVertexIndex(vID, 2*im + 1, osdBuffers.valenceBuffer);
        float3 diagonal_m = OsdReadVertex(idx_diagonal_m, osdBuffers.vertexBuffer);

        f[i] = (pos * float(valence) + (neighbor_p + neighbor)*2.0f + diagonal) / (float(valence)+5.0f);

        opos += f[i];
        v.r[i] = (neighbor_p-neighbor_m)/3.0f + (diagonal - diagonal_m)/6.0f;
    }

    opos /= valence;
    v.P = float4(opos, 1.0f).xyz;

    float3 e;
    v.e0 = float3(0,0,0);
    v.e1 = float3(0,0,0);

    for(int i=0; i<valence; ++i) {
        int im = (i + valence -1) % valence;
        e = 0.5f * (f[i] + f[im]);
        v.e0 += cosfn(valence, i)*e;
        v.e1 += sinfn(valence, i)*e;
    }
    float ef = OsdComputeCatmarkCoefficient(valence);
    v.e0 *= ef;
    v.e1 *= ef;

#ifdef OSD_PATCH_GREGORY_BOUNDARY
    v.zerothNeighbor = zerothNeighbor;
    if (currNeighbor == 1) {
        boundaryEdgeNeighbors[1] = boundaryEdgeNeighbors[0];
    }

    if (ivalence < 0) {
        if (valence > 2) {
            v.P = (OsdReadVertex(boundaryEdgeNeighbors[0], osdBuffers.vertexBuffer) +
                   OsdReadVertex(boundaryEdgeNeighbors[1], osdBuffers.vertexBuffer) +
                   4.0f * pos)/6.0f;
        } else {
            v.P = pos;
        }

        v.e0 = (OsdReadVertex(boundaryEdgeNeighbors[0], osdBuffers.vertexBuffer) -
                OsdReadVertex(boundaryEdgeNeighbors[1], osdBuffers.vertexBuffer))/6.0;

        float k = float(float(valence) - 1.0f);    //k is the number of faces
        float c = cospi(1.0/k);
        float s = sinpi(1.0/k);
        float gamma = -(4.0f*s)/(3.0f*k+c);
        float alpha_0k = -((1.0f+2.0f*c)*sqrt(1.0f+c))/((3.0f*k+c)*sqrt(1.0f-c));
        float beta_0 = s/(3.0f*k + c);

        int idx_diagonal = OsdReadVertexIndex(vID, 2*zerothNeighbor + 1, osdBuffers.valenceBuffer);
        float3 diagonal = OsdReadVertex(idx_diagonal, osdBuffers.vertexBuffer);

        v.e1 = gamma * pos +
            alpha_0k * OsdReadVertex(boundaryEdgeNeighbors[0], osdBuffers.vertexBuffer) +
            alpha_0k * OsdReadVertex(boundaryEdgeNeighbors[1], osdBuffers.vertexBuffer) +
            beta_0 * diagonal;

        for (int x=1; x<valence - 1; ++x) {
            int curri = ((x + zerothNeighbor)%valence);
            float alpha = (4.0f*sinpi((float(x))/k))/(3.0f*k+c);
            float beta = (sinpi((float(x))/k) + sinpi((float(x+1))/k))/(3.0f*k+c);

            int idx_neighbor = OsdReadVertexIndex(vID, 2*curri, osdBuffers.valenceBuffer);
            float3 neighbor = OsdReadVertex(idx_neighbor, osdBuffers.vertexBuffer);

            idx_diagonal = OsdReadVertexIndex(vID, 2*curri + 1, osdBuffers.valenceBuffer);
            diagonal = OsdReadVertex(idx_diagonal, osdBuffers.vertexBuffer);

            v.e1 += alpha * neighbor + beta * diagonal;
        }

        v.e1 /= 3.0f;
    }
#endif
}

void
OsdComputePerPatchVertexGregory(int3 patchParam, unsigned ID, unsigned primitiveID,
                                threadgroup OsdPerVertexGregory* v,
                                device OsdPerPatchVertexGregory& result,
                                OsdPatchParamBufferSet osdBuffers)
{
    result.P = v[ID].P;

    int i = ID;
    int ip = (i+1)%4;
    int im = (i+3)%4;
    int valence = abs(v[i].valence);
    int n = valence;

    int start = OsdReadQuadOffset(primitiveID, i, osdBuffers.quadOffsetBuffer) & 0xff;
    int prev = (OsdReadQuadOffset(primitiveID, i, osdBuffers.quadOffsetBuffer) >> 8) & 0xff;

    int start_m = OsdReadQuadOffset(primitiveID, im, osdBuffers.quadOffsetBuffer) & 0xff;
    int prev_p = (OsdReadQuadOffset(primitiveID, ip, osdBuffers.quadOffsetBuffer) >> 8) & 0xff;

    int np = abs(v[ip].valence);
    int nm = abs(v[im].valence);

    // Control Vertices based on :
    // "Approximating Subdivision Surfaces with Gregory Patches
    //  for Hardware Tessellation"
    // Loop, Schaefer, Ni, Castano (ACM ToG Siggraph Asia 2009)
    //
    //  P3         e3-      e2+         P2
    //     O--------O--------O--------O
    //     |        |        |        |
    //     |        |        |        |
    //     |        | f3-    | f2+    |
    //     |        O        O        |
    // e3+ O------O            O------O e2-
    //     |     f3+          f2-     |
    //     |                          |
    //     |                          |
    //     |      f0-         f1+     |
    // e0- O------O            O------O e1+
    //     |        O        O        |
    //     |        | f0+    | f1-    |
    //     |        |        |        |
    //     |        |        |        |
    //     O--------O--------O--------O
    //  P0         e0+      e1-         P1
    //

#ifdef OSD_PATCH_GREGORY_BOUNDARY
    float3 Em_ip;
    if (v[ip].valence < -2) {
        int j = (np + prev_p - v[ip].zerothNeighbor) % np;
        Em_ip = v[ip].P + cospi(j/float(np-1))*v[ip].e0 + sinpi(j/float(np-1))*v[ip].e1;
    } else {
        Em_ip = v[ip].P + v[ip].e0*cosfn(np, prev_p) + v[ip].e1*sinfn(np, prev_p);
    }

    float3 Ep_im;
    if (v[im].valence < -2) {
        int j = (nm + start_m - v[im].zerothNeighbor) % nm;
        Ep_im = v[im].P + cospi(j/float(nm-1))*v[im].e0 + sinpi(j/float(nm-1))*v[im].e1;
    } else {
        Ep_im = v[im].P + v[im].e0*cosfn(nm, start_m) + v[im].e1*sinfn(nm, start_m);
    }

    if (v[i].valence < 0) {
        n = (n-1)*2;
    }
    if (v[im].valence < 0) {
        nm = (nm-1)*2;
    }
    if (v[ip].valence < 0) {
        np = (np-1)*2;
    }

    if (v[i].valence > 2) {
        result.Ep = v[i].P + (v[i].e0*cosfn(n, start) + v[i].e1*sinfn(n, start));
        result.Em = v[i].P + (v[i].e0*cosfn(n, prev) +  v[i].e1*sinfn(n, prev));

        float s1=3-2*cosfn(n,1)-cosfn(np,1);
        float s2=2*cosfn(n,1);

        result.Fp = (cosfn(np,1)*v[i].P + s1*result.Ep + s2*Em_ip + v[i].r[start])/3.0f;
        s1 = 3.0f-2.0f*cospi(2.0f/float(n))-cospi(2.0f/float(nm));
        result.Fm = (cosfn(nm,1)*v[i].P + s1*result.Em + s2*Ep_im - v[i].r[prev])/3.0f;

    } else if (v[i].valence < -2) {
        int j = (valence + start - v[i].zerothNeighbor) % valence;

        result.Ep = v[i].P + cospi(j/float(valence-1))*v[i].e0 + sinpi(j/float(valence-1))*v[i].e1;
        j = (valence + prev - v[i].zerothNeighbor) % valence;
        result.Em = v[i].P + cospi(j/float(valence-1))*v[i].e0 + sinpi(j/float(valence-1))*v[i].e1;

        float3 Rp = ((-2.0f * v[i].org - 1.0f * v[im].org) + (2.0f * v[ip].org + 1.0f * v[(i+2)%4].org))/3.0f;
        float3 Rm = ((-2.0f * v[i].org - 1.0f * v[ip].org) + (2.0f * v[im].org + 1.0f * v[(i+2)%4].org))/3.0f;

        float s1 = 3-2*cosfn(n,1)-cosfn(np,1);
        float s2 = 2*cosfn(n,1);

        result.Fp = (cosfn(np,1)*v[i].P + s1*result.Ep + s2*Em_ip + v[i].r[start])/3.0f;
        s1 = 3.0f-2.0f*cospi(2.0f/float(n))-cospi(2.0f/float(nm));
        result.Fm = (cosfn(nm,1)*v[i].P + s1*result.Em + s2*Ep_im - v[i].r[prev])/3.0f;

        if (v[im].valence < 0) {
            s1 = 3-2*cosfn(n,1)-cosfn(np,1);
            result.Fp = result.Fm = (cosfn(np,1)*v[i].P + s1*result.Ep + s2*Em_ip + v[i].r[start])/3.0f;
        } else if (v[ip].valence < 0) {
            s1 = 3.0f-2.0f*cospi(2.0f/n)-cospi(2.0f/nm);
            result.Fm = result.Fp = (cosfn(nm,1)*v[i].P + s1*result.Em + s2*Ep_im - v[i].r[prev])/3.0f;
        }

    } else if (v[i].valence == -2) {
        result.Ep = (2.0f * v[i].org + v[ip].org)/3.0f;
        result.Em = (2.0f * v[i].org + v[im].org)/3.0f;
        result.Fp = result.Fm = (4.0f * v[i].org + v[(i+2)%n].org + 2.0f * v[ip].org + 2.0f * v[im].org)/9.0f;
    }

#else // not OSD_PATCH_GREGORY_BOUNDARY

    result.Ep = v[i].P + v[i].e0 * cosfn(n, start) + v[i].e1*sinfn(n, start);
    result.Em = v[i].P + v[i].e0 * cosfn(n, prev ) + v[i].e1*sinfn(n, prev );

    float3 Em_ip = v[ip].P + v[ip].e0*cosfn(np, prev_p) + v[ip].e1*sinfn(np, prev_p);
    float3 Ep_im = v[im].P + v[im].e0*cosfn(nm, start_m) + v[im].e1*sinfn(nm, start_m);

    float s1 = 3-2*cosfn(n,1)-cosfn(np,1);
    float s2 = 2*cosfn(n,1);

    result.Fp = (cosfn(np,1)*v[i].P + s1*result.Ep + s2*Em_ip + v[i].r[start])/3.0f;
    s1 = 3.0f-2.0f*cospi(2.0f/float(n))-cospi(2.0f/float(nm));
    result.Fm = (cosfn(nm,1)*v[i].P + s1*result.Em +s2*Ep_im - v[i].r[prev])/3.0f;
#endif
}

#endif  // OSD_PATCH_GREGORY || OSD_PATCH_GREGORY_BOUNDARY
