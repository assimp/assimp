//
//   Copyright 2021 Pixar
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

#include "../bfr/surface.h"
#include "../bfr/surfaceData.h"
#include "../bfr/pointOperations.h"
#include "../bfr/patchTree.h"
#include "../far/patchParam.h"
#include "../far/patchDescriptor.h"
#include "../far/patchBasis.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  Constructor for the Surface -- defers to the constructor for its full
//  set of member variables, but marks the precision as double when needed
//  (as a specialization here):
//
template <typename REAL>
Surface<REAL>::Surface() : _data() {

    //  Surface<> should not be adding members outside its SurfaceData:
    assert(sizeof(*this) == sizeof(internal::SurfaceData));
}

template <>
Surface<double>::Surface() : _data() {

    _data.setDouble(true);
}


//
//  Simple internal utilities:
//
template <typename REAL>
inline internal::IrregularPatchType const &
Surface<REAL>::getIrregPatch() const {

    return _data.getIrregPatch();
}

template <typename REAL>
int
Surface<REAL>::GetNumPatchPoints() const {

    if (IsRegular()) {
        return GetNumControlPoints();
    } else if (IsLinear()) {
        return 2 * GetNumControlPoints() + 1;
    } else {
        return getIrregPatch().GetNumPointsTotal();
    }
}

template <typename REAL>
int
Surface<REAL>::GetControlPointIndices(Index cvs[]) const {

    std::memcpy(cvs, _data.getCVIndices(), _data.getNumCVs() * sizeof(Index));
    return _data.getNumCVs();
}


//
//  Methods for gathering and computing control and patch points:
//
template <typename REAL>
template <typename REAL_MESH>
void
Surface<REAL>::GatherControlPoints(
        REAL_MESH const meshPoints[], PointDescriptor const & meshDesc,
        REAL controlPoints[], PointDescriptor const & controlDesc) const {

    //
    //  Assemble parameters of the point copy operation and apply:
    //
    typedef points::CopyConsecutive<REAL,REAL_MESH> PointCopier;

    typename PointCopier::Parameters copyParams;
    copyParams.pointData    = meshPoints;
    copyParams.pointSize    = meshDesc.size;
    copyParams.pointStride  = meshDesc.stride;

    copyParams.srcCount   = GetNumControlPoints();
    copyParams.srcIndices = _data.getCVIndices();

    copyParams.resultData   = controlPoints;
    copyParams.resultStride = controlDesc.stride;

    PointCopier::Apply(copyParams);
}

template <typename REAL>
void
Surface<REAL>::computeLinearPatchPoints(REAL pointData[],
        PointDescriptor const & pointDesc) const {

    //
    //  The initial control points of the N-sided face will be followed
    //  by the midpoint of the face and the midpoint of the N edges.
    //
    //  Assemble parameters of the face splitting operation and apply:
    //
    int N = GetNumControlPoints();

    typedef points::SplitFace<REAL> PointSplitter;

    typename PointSplitter::Parameters splitParams;
    splitParams.pointData   = pointData;
    splitParams.pointSize   = pointDesc.size;
    splitParams.pointStride = pointDesc.stride;

    splitParams.srcCount = N;

    splitParams.resultData = pointData + pointDesc.stride * N;

    PointSplitter::Apply(splitParams);
}

template <typename REAL>
void
Surface<REAL>::computeIrregularPatchPoints(REAL pointData[],
        PointDescriptor const & pointDesc) const {

    //
    //  An "irregular patch" may be represented by a regular patch in
    //  rare cases, so be sure there are patch points to compute:
    //
    internal::IrregularPatchType const & irregPatch = getIrregPatch();

    int numControlPoints = GetNumControlPoints();
    int numPatchPoints   = irregPatch.GetNumPointsTotal();

    if (numPatchPoints == numControlPoints) return;

    //
    //  Assemble parameters of the point combination operation and apply:
    //
    typedef points::CombineConsecutive<REAL> PointCombiner;

    typename PointCombiner::Parameters combParams;
    combParams.pointData   = pointData;
    combParams.pointSize   = pointDesc.size;
    combParams.pointStride = pointDesc.stride;

    combParams.srcCount = numControlPoints;

    combParams.resultCount = numPatchPoints - numControlPoints;
    combParams.resultData  = pointData + pointDesc.stride * numControlPoints;
    combParams.weightData  = irregPatch.GetStencilMatrix<REAL>();

    PointCombiner::Apply(combParams);
}

//
//  Methods for computing the extent of the control points:
//
template <typename REAL>
void
Surface<REAL>::BoundControlPoints(
        REAL const controlPoints[], PointDescriptor const & pointDesc,
        REAL boundMin[], REAL boundMax[]) const {

    int numPoints = GetNumControlPoints();
    int pointSize = pointDesc.size;

    REAL const * p = controlPoints;
    std::memcpy(boundMin, p, pointSize * sizeof(REAL));
    std::memcpy(boundMax, p, pointSize * sizeof(REAL));

    for (int i = 1; i < numPoints; ++i) {
        p += pointDesc.stride;
        for (int j = 0; j < pointSize; ++j) {
            boundMin[j] = std::min(boundMin[j], p[j]);
            boundMax[j] = std::max(boundMax[j], p[j]);
        }
    }
}

template <typename REAL>
void
Surface<REAL>::BoundControlPointsFromMesh(
        REAL const meshPoints[], PointDescriptor const & pointDesc,
        REAL boundMin[], REAL boundMax[]) const {

    int numPoints = GetNumControlPoints();
    int pointSize = pointDesc.size;

    int const * meshIndices = _data.getCVIndices();

    REAL const * p = meshPoints + pointDesc.stride * meshIndices[0];
    std::memcpy(boundMin, p, pointSize * sizeof(REAL));
    std::memcpy(boundMax, p, pointSize * sizeof(REAL));

    for (int i = 1; i < numPoints; ++i) {
        p = meshPoints + pointDesc.stride * meshIndices[i];
        for (int j = 0; j < pointSize; ++j) {
            boundMin[j] = std::min(boundMin[j], p[j]);
            boundMax[j] = std::max(boundMax[j], p[j]);
        }
    }
}


//
//  Internal helper for evaluation:
//
namespace {
    template <typename REAL>
    inline int
    assignWeightsPerDeriv(REAL * const deriv[6], int wSize, REAL wBuffer[],
                          REAL * wDeriv[6]) {

        std::memset(wDeriv, 0, 6 * sizeof(REAL*));

        wDeriv[0] = wBuffer;
        if (deriv[1] && deriv[2]) {
            wDeriv[1] = wDeriv[0] + wSize;
            wDeriv[2] = wDeriv[1] + wSize;
            if (deriv[3] && deriv[4] && deriv[5]) {
                wDeriv[3] = wDeriv[2] + wSize;
                wDeriv[4] = wDeriv[3] + wSize;
                wDeriv[5] = wDeriv[4] + wSize;
                return 6;
            }
            return 3;
        }
        return 1;
    }
}


//
//  Evaluation methods accessing the local data for a simple regular patch:
//
template <typename REAL>
void
Surface<REAL>::evalRegularBasis(REAL const uv[2], REAL * wDeriv[]) const {

    Far::PatchParam patchParam;
    patchParam.Set(0, 0, 0, 0, 0, getRegPatchMask(), 0, true);

    Far::internal::EvaluatePatchBasisNormalized(
        getRegPatchType(), patchParam, uv[0], uv[1],
        wDeriv[0], wDeriv[1], wDeriv[2], wDeriv[3], wDeriv[4], wDeriv[5]);
}

template <typename REAL>
int
Surface<REAL>::evalRegularStencils(REAL const uv[2], REAL * sDeriv[]) const {

    //
    //  The control points of a regular patch are always the full set
    //  of points required by a patch, i.e. phantom points will have an
    //  entry of some kind (a duplicate).  For example, for an isolated
    //  quad, its regular patch still has 16 control points.  So we can
    //  return the basis weights as stencil weights for all cases.
    //
    Far::PatchParam patchParam;
    patchParam.Set(0, 0, 0, 0, 0, getRegPatchMask(), 0, true);

    Far::internal::EvaluatePatchBasisNormalized(
        getRegPatchType(), patchParam, uv[0], uv[1],
        sDeriv[0], sDeriv[1], sDeriv[2], sDeriv[3], sDeriv[4], sDeriv[5]);

    return GetNumControlPoints();
}

template <typename REAL>
void
Surface<REAL>::evalRegularDerivs(REAL const uv[2],
        REAL const patchPoints[], PointDescriptor const & pointDesc,
        REAL * deriv[]) const {

    //
    //  Regular basis evaluation simply returns weights for use with
    //  the entire set of patch control points.
    //
    //  Assign weights for requested derivatives and evaluate:
    //
    REAL   wBuffer[6 * 20];
    REAL * wDeriv[6];

    int numDerivs = assignWeightsPerDeriv(deriv, 20, wBuffer, wDeriv);

    evalRegularBasis(uv, wDeriv);

    //
    //  Assemble parameters of the point combination operation and apply:
    //
    points::CommonCombinationParameters<REAL> combineParams;
    combineParams.pointData   = patchPoints;
    combineParams.pointSize   = pointDesc.size;
    combineParams.pointStride = pointDesc.stride;

    combineParams.srcCount   = GetNumControlPoints();
    combineParams.srcIndices = 0;

    combineParams.resultCount = numDerivs;
    combineParams.resultArray = deriv;
    combineParams.weightArray = wDeriv;

    if (numDerivs == 1) {
        points::Combine1<REAL>::Apply(combineParams);
    } else if (numDerivs == 3) {
        points::Combine3<REAL>::Apply(combineParams);
    } else {
        points::CombineMultiple<REAL>::Apply(combineParams);
    }
}

//
//  Evaluation methods accessing the PatchTree for irregular patches:
//
template <typename REAL>
typename Surface<REAL>::IndexArray
Surface<REAL>::evalIrregularBasis(REAL const UV[2], REAL * wDeriv[]) const {

    Parameterization param = GetParameterization();
    REAL uv[2] = { UV[0], UV[1] };
    int subFace = param.HasSubFaces() ?
                  param.ConvertCoordToNormalizedSubFace(uv, uv) : 0;

    internal::IrregularPatchType const & irregPatch = getIrregPatch();
    int subPatchIndex = irregPatch.FindSubPatch(uv[0], uv[1], subFace);
    assert(subPatchIndex >= 0);

    irregPatch.EvalSubPatchBasis(subPatchIndex, uv[0], uv[1],
            wDeriv[0], wDeriv[1], wDeriv[2], wDeriv[3], wDeriv[4], wDeriv[5]);

    return irregPatch.GetSubPatchPoints(subPatchIndex);
}

template <typename REAL>
int
Surface<REAL>::evalIrregularStencils(REAL const UV[2], REAL * sDeriv[]) const {

    Parameterization param = GetParameterization();
    REAL uv[2] = { UV[0], UV[1] };
    int subFace = param.HasSubFaces() ?
                  param.ConvertCoordToNormalizedSubFace(uv, uv) : 0;

    internal::IrregularPatchType const & irregPatch = getIrregPatch();
    int subPatchIndex = irregPatch.FindSubPatch(uv[0], uv[1], subFace);
    assert(subPatchIndex >= 0);

    return irregPatch.EvalSubPatchStencils(
            subPatchIndex, uv[0], uv[1],
            sDeriv[0], sDeriv[1], sDeriv[2], sDeriv[3], sDeriv[4], sDeriv[5]);
}

template <typename REAL>
void
Surface<REAL>::evalIrregularDerivs(REAL const uv[2],
        REAL const patchPoints[], PointDescriptor const & pointDesc,
        REAL * deriv[]) const {

    //
    //  Non-linear irregular basis evaluation returns both the weights
    //  and the corresponding points of a sub-patch defined by a subset
    //  of the given patch points.
    //
    //  Assign weights for requested derivatives and evaluate:
    //
    REAL   wBuffer[6 * 20];
    REAL * wDeriv[6];

    int numDerivs = assignWeightsPerDeriv(deriv, 20, wBuffer, wDeriv);

    IndexArray indices = evalIrregularBasis(uv, wDeriv);

    //
    //  Assemble parameters of the point combination operation and apply:
    //
    points::CommonCombinationParameters<REAL> combineParams;
    combineParams.pointData   = patchPoints;
    combineParams.pointSize   = pointDesc.size;
    combineParams.pointStride = pointDesc.stride;

    combineParams.srcCount   = indices.size();
    combineParams.srcIndices = &indices[0];

    combineParams.resultCount = numDerivs;
    combineParams.resultArray = deriv;
    combineParams.weightArray = wDeriv;

    if (numDerivs == 1) {
        points::Combine1<REAL>::Apply(combineParams);
    } else if (numDerivs == 3) {
        points::Combine3<REAL>::Apply(combineParams);
    } else {
        points::CombineMultiple<REAL>::Apply(combineParams);
    }
}

//
//  Supporting methods for the N-sided quadrangulated linear patch:
//
namespace {
    //
    //  For stencils, there are four unique weights derived from the
    //  four bilinear weights of the sub-face.  Given these weights as
    //  input for a sub-face with origin at base point P, the resulting
    //  weights are associated with the N base points as follows:
    //
    //      w[0] = the point at the origin (P)
    //      w[1] = the point following P
    //      w[2] = the N-3 points not adjacent to P (contributing to center)
    //      w[3] = the point preceding P
    //
    template <typename REAL>
    inline void
    transformLinearQuadWeightsToStencil(REAL w[4], int N) {

        REAL wOrigin = w[0];
        REAL wNext   = w[1] * 0.5f;
        REAL wCenter = w[2] / (REAL)N;
        REAL wPrev   = w[3] * 0.5f;

        w[0] = wCenter + wNext + wPrev + wOrigin;
        w[1] = wCenter + wNext;
        w[2] = wCenter;
        w[3] = wCenter + wPrev;
    }

    template <typename REAL>
    inline void
    scaleWeights4(REAL w[4], REAL derivScale) {
        if (w) {
            w[0] *= derivScale;
            w[1] *= derivScale;
            w[2] *= derivScale;
            w[3] *= derivScale;
        }
    }
}

template <typename REAL>
int
Surface<REAL>::evalMultiLinearBasis(REAL const UV[2], REAL *wDeriv[]) const {

    Parameterization param = GetParameterization();
    assert(param.GetType() == Parameterization::QUAD_SUBFACES);

    REAL uv[2];
    int subFace = param.ConvertCoordToNormalizedSubFace(UV, uv);

    //  WIP - Prefer to eval Linear basis directly, i.e.:
    //
    //      Far::internal::EvalBasisLinear(u, v, wP, wDu, wDv);
    //
    //  but this internal Far function is sometimes optimized out, causing
    //  link errors.  Need to fix in Far with explicit instantiation...
    Far::internal::EvaluatePatchBasisNormalized(Far::PatchDescriptor::QUADS,
            Far::PatchParam(), uv[0], uv[1],
            wDeriv[0], wDeriv[1], wDeriv[2], wDeriv[3], wDeriv[4], wDeriv[5]);

    //  Scale weights for derivatives (only mixed partial of 2nd is non-zero):
    scaleWeights4<REAL>(wDeriv[1], 2.0f);
    scaleWeights4<REAL>(wDeriv[2], 2.0f);

    scaleWeights4<REAL>(wDeriv[4], 4.0f);

    return subFace;
}

template <typename REAL>
int
Surface<REAL>::evalMultiLinearStencils(REAL const uv[2], REAL *sDeriv[]) const {

    //
    //  Linear evaluation of irregular N-sided faces evaluates one of N
    //  locally subdivided quad faces and identifies that sub-face -- also
    //  the origin vertex of the quad. The basis weights are subsequently
    //  transformed into the four unique values that are then assigned to
    //  the N vertices of the face.
    //
    //  Assign weights for requested stencils and evaluate:
    //
    REAL   wBuffer[6 * 4];
    REAL * wDeriv[6];

    int numDerivs = assignWeightsPerDeriv(sDeriv, 4, wBuffer, wDeriv);

    int iOrigin = evalMultiLinearBasis(uv, wDeriv);

    //
    //  Transform the four linear weights to four unique stencil weights:
    //
    int numControlPoints = GetNumControlPoints();

    transformLinearQuadWeightsToStencil(wDeriv[0], numControlPoints);
    if (numDerivs > 1) {
        transformLinearQuadWeightsToStencil(wDeriv[1], numControlPoints);
        transformLinearQuadWeightsToStencil(wDeriv[2], numControlPoints);
        if (numDerivs > 3) {
            transformLinearQuadWeightsToStencil(wDeriv[4], numControlPoints);
        }
    }

    //
    //  Assign the N stencil weights from the four unique values:
    //
    int iNext = (iOrigin + 1) % numControlPoints;
    int iPrev = (iOrigin + numControlPoints - 1) % numControlPoints;

    for (int i = 0; i < numControlPoints; ++i) {
        int wIndex = 2;
        if (i == iOrigin) {
            wIndex = 0;
        } else if (i == iNext) {
            wIndex = 1;
        } else if (i == iPrev) {
            wIndex = 3;
        }

        sDeriv[0][i] = wDeriv[0][wIndex];
        if (numDerivs > 1) {
            sDeriv[1][i] = wDeriv[1][wIndex];
            sDeriv[2][i] = wDeriv[2][wIndex];
            if (numDerivs > 3) {
                sDeriv[3][i] = 0.0f;
                sDeriv[4][i] = wDeriv[4][wIndex];
                sDeriv[5][i] = 0.0f;
            }
        }
    }
    return numControlPoints;
}

template <typename REAL>
void
Surface<REAL>::evalMultiLinearDerivs(REAL const uv[],
        REAL const patchPoints[], PointDescriptor const & pointDesc,
        REAL * deriv[]) const {

    //
    //  Linear evaluation of irregular N-sided faces evaluates one of N
    //  locally subdivided quad faces and identifies that sub-face.
    //
    //  Assign weights for requested derivatives and evaluate:
    //
    REAL   wBuffer[6 * 4];
    REAL * wDeriv[6];

    int numDerivs = assignWeightsPerDeriv(deriv, 4, wBuffer, wDeriv);

    int subQuad = evalMultiLinearBasis(uv, wDeriv);

    //
    //  Identify the patch points for the sub-face and interpolate:
    //
    int N = GetNumControlPoints();

    int quadIndices[4];
    quadIndices[0] = subQuad;
    quadIndices[1] = N + 1 + subQuad;
    quadIndices[2] = N;
    quadIndices[3] = N + 1 + (subQuad + N - 1) % N;

    //
    //  Assemble parameters of the point combination operation and apply:
    //
    points::CommonCombinationParameters<REAL> combineParams;
    combineParams.pointData   = patchPoints;
    combineParams.pointSize   = pointDesc.size;
    combineParams.pointStride = pointDesc.stride;

    combineParams.srcCount   = 4;
    combineParams.srcIndices = quadIndices;

    combineParams.resultCount = numDerivs;
    combineParams.resultArray = deriv;
    combineParams.weightArray = wDeriv;

    if (numDerivs == 1) {
        points::Combine1<REAL>::Apply(combineParams);
    } else if (numDerivs == 3) {
        points::Combine3<REAL>::Apply(combineParams);
    } else {
        points::CombineMultiple<REAL>::Apply(combineParams);
    }
}


//
//  Public methods to apply stencils:
//
template <typename REAL>
void
Surface<REAL>::ApplyStencilFromMesh(REAL const stencil[],
        REAL const meshPoints[], PointDescriptor const & pointDesc,
        REAL result[]) const {

    //
    //  Assemble parameters of the point combination operation and apply:
    //
    typedef points::Combine1<REAL> PointCombiner;

    typename PointCombiner::Parameters combParams;
    combParams.pointData   = meshPoints;
    combParams.pointSize   = pointDesc.size;
    combParams.pointStride = pointDesc.stride;

    combParams.srcCount   = GetNumControlPoints();
    combParams.srcIndices = _data.getCVIndices();

    combParams.resultCount = 1;
    combParams.resultArray = &result;
    combParams.weightArray = &stencil;

    PointCombiner::Apply(combParams);
}

template <typename REAL>
void
Surface<REAL>::ApplyStencil(REAL const stencil[],
        REAL const controlPoints[], PointDescriptor const & pointDesc,
        REAL result[]) const {

    //
    //  Assemble parameters of the point combination operation and apply:
    //
    typedef points::Combine1<REAL> PointCombiner;

    typename PointCombiner::Parameters combParams;
    combParams.pointData   = controlPoints;
    combParams.pointSize   = pointDesc.size;
    combParams.pointStride = pointDesc.stride;

    combParams.srcCount   = GetNumControlPoints();
    combParams.srcIndices = 0;

    combParams.resultCount = 1;
    combParams.resultArray = &result;
    combParams.weightArray = &stencil;

    PointCombiner::Apply(combParams);
}


//
//  Explicitly instantiate Surface<> implementations for float and double:
//
template class Surface<float>;
template class Surface<double>;

//
//  Explicitly instantiate template methods for converting precision:
//
template void Surface<float>::GatherControlPoints(
                      float const [], PointDescriptor const &,
                      float       [], PointDescriptor const &) const;
template void Surface<float>::GatherControlPoints(
                      double const [], PointDescriptor const &,
                      float        [], PointDescriptor const &) const;

template void Surface<double>::GatherControlPoints(
                      double const [], PointDescriptor const &,
                      double       [], PointDescriptor const &) const;
template void Surface<double>::GatherControlPoints(
                      float const [], PointDescriptor const &,
                      double      [], PointDescriptor const &) const;

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
