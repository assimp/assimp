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

#ifndef OPENSUBDIV3_BFR_SURFACE_H
#define OPENSUBDIV3_BFR_SURFACE_H

#include "../version.h"

#include "../bfr/surfaceData.h"
#include "../bfr/parameterization.h"
#include "../vtr/array.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

///
/// @brief Encapsulates the limit surface for a face of a mesh
///
/// The Surface class encapsulates the limit surface for a face of a mesh
/// for any data interpolation type (vertex, varying and face-varying) and
/// provides the public interface for its evaluation. Surface is a class
/// template parameterized to support evaluation in single or double
/// precision.
///
/// @tparam REAL  Floating point precision (float or double only)
///
/// Instances of Surface are created or initialized by a subclass of the
/// SurfaceFactory. Since existing instances can be re-initialized, they
/// should be tested for validity after such re-initialization.
///
/// All Surfaces are assigned a Parameterization based on the subdivision
/// scheme and the size of the face, which can then be used for evaluation
/// and tessellation of the surface.
///
template <typename REAL>
class Surface {
public:
    /// @brief Simple struct defining the size and stride of points in
    ///        arrays.
    struct PointDescriptor {
        PointDescriptor() : size(0), stride(0) { }
        PointDescriptor(int n) : size(n), stride(n) { }
        PointDescriptor(int n, int m) : size(n), stride(m) { }

        int size, stride;
    };

    /// @brief Integer type representing a mesh index
    typedef int Index;

public:
    //@{
    /// @name Construction and initialization
    ///
    /// Instances of Surface may be explicitly constructed, but are
    /// initialized by SurfaceFactory and so only default construction
    /// is provided. An instance will be invalid (and so unusable) if
    /// default constructed, or if the factory that initialized it
    /// determined that the face associated with it has no limit surface.
    ///

    /// @brief Return true if successfully initialized
    bool IsValid() const { return _data.isValid(); }

    /// @brief Clear a previously initialized Surface
    void Clear() { _data.reinitialize(); }

    /// @brief Default construction produces an invalid instance
    Surface();

    Surface(Surface const & src) = default;
    Surface& operator=(Surface const & src) = default;
    ~Surface() = default;
    //@}

    //@{
    /// @name Simple queries
    ///
    /// Simple queries of valid Surface.
    ///

    /// @brief Return the Parameterization
    Parameterization GetParameterization() const { return _data.getParam(); }

    /// @brief Return the size of the face
    int GetFaceSize() const { return GetParameterization().GetFaceSize(); }

    /// @brief Return if the Surface is a single regular patch
    bool IsRegular() const { return _data.isRegular(); }

    /// @brief Return if the Surface is linear
    bool IsLinear() const { return _data.isLinear(); }
    //@}

    //@{
    /// @name Methods to manage control points
    ///
    /// Control points are the subset of points in the mesh that influence
    /// a Surface. They can be identified as part of the mesh data by their
    /// indices, or gathered into an array for other purposes.
    ///
    /// It is not necessary to deal directly with control points for
    /// evaluation, but they are useful with limit stencils and other
    /// purposes, e.g. computing a bounding box of the control hull of
    /// the Surface.
    ///
    /// Note that methods that access control points from the array of
    /// mesh data require that the array be contiguous. If a large data
    /// set is fragmented into blocks or pages, these methods cannot be
    /// used and control points will need to be gathered explicitly.
    ///       

    /// @brief Return the number of control points affecting the Surface
    int GetNumControlPoints() const { return _data.getNumCVs(); }

    /// @brief Identify indices of control points in the mesh
    int GetControlPointIndices(Index meshPointIndices[]) const;

    /// @brief Gather control points in a local array
    ///
    /// @tparam REAL_MESH        Floating point precision of mesh points
    ///
    /// @param  meshPoints       Input array of mesh point data
    /// @param  meshPointDesc    The size and stride of mesh point data
    /// @param  controlPoints    Output array of control point data
    /// @param  controlPointDesc The size and stride of control point data
    ///
    template <typename REAL_MESH>
    void GatherControlPoints(REAL_MESH       const   meshPoints[],
                             PointDescriptor const & meshPointDesc,
                             REAL                    controlPoints[],
                             PointDescriptor const & controlPointDesc) const;

    /// @brief Compute bounds of control points from a local array
    void BoundControlPoints(REAL            const   controlPoints[],
                            PointDescriptor const & controlPointDesc,
                            REAL                    minExtent[],
                            REAL                    maxExtent[]) const;

    /// @brief Compute bounds of control points from the mesh data
    void BoundControlPointsFromMesh(REAL            const   meshPoints[],
                                    PointDescriptor const & meshPointDesc,
                                    REAL                    minExtent[],
                                    REAL                    maxExtent[]) const;
    //@}

    //@{
    /// @name Methods to manage patch points
    ///
    /// Patch points are derived from the control points and are used to
    /// evaluate the Surface. The patch points always include the control
    /// points as a subset.
    ///       

    /// @brief Return the number of patch points representing the Surface
    int GetNumPatchPoints() const;

    ///
    /// @brief Prepare patch points in a local array for evaluation
    ///
    /// The patch points consist of the control points plus any additional
    /// points derived from them that may be required to represent the
    /// limit surface as one or more parametric patches.
    ///
    /// @param  meshPoints     Input array of mesh point data
    /// @param  meshPointDesc  The size and stride of mesh point data
    /// @param  patchPoints    Output array of patch point data
    /// @param  patchPointDesc The size and stride of patch point data
    ///
    /// Note that this method requires the mesh data be in a contiguous
    /// array. If a large data set is fragmented into blocks or pages, this
    /// method cannot be used. The control points will need to be gathered
    /// explicitly as the subset of patch points, after which the method to
    /// compute the remaining patch points can be used.
    ///
    void PreparePatchPoints(REAL            const   meshPoints[],
                            PointDescriptor const & meshPointDesc,
                            REAL                    patchPoints[],
                            PointDescriptor const & patchPointDesc) const;

    /// @brief Compute all patch points following the control points
    ///
    /// For cases where the control points have already been gathered into
    /// an array allocated for the patch points, the remaining patch points
    /// will be computed.
    ///
    /// @param  patchPoints    Array of patch point data to be modified
    /// @param  patchPointDesc The size and stride of patch point data
    ///
    void ComputePatchPoints(REAL                    patchPoints[],
                            PointDescriptor const & patchPointDesc) const;
    //@}

    //@{
    /// @name Evaluation of positions and derivatives
    ///
    /// Evaluation methods use the patch points to compute position, 1st and
    /// 2nd derivatives of the Surface at a given (u,v) coordinate within
    /// the domain of the Surface's Parameterization. All parameters of the
    /// different overloads are required.
    ///       

    /// @brief Evaluation of position
    void Evaluate(REAL const uv[2],
                  REAL const patchPoints[], PointDescriptor const & pointDesc,
                  REAL P[]) const;

    /// @brief Overload of evaluation for 1st derivatives
    void Evaluate(REAL const uv[2],
                  REAL const patchPoints[], PointDescriptor const & pointDesc,
                  REAL P[], REAL Du[], REAL Dv[]) const;

    /// @brief Overload of evaluation for 2nd derivatives
    void Evaluate(REAL const uv[2],
                  REAL const patchPoints[], PointDescriptor const & pointDesc,
                  REAL P[], REAL Du[],  REAL Dv[],
                  REAL Duu[], REAL Duv[], REAL Dvv[]) const;
    //@}

    //@{
    /// @name Evaluation and application of limit stencils
    ///
    /// Limit stencils are sets of coefficients that express an evaluation
    /// as a linear combination of the control points. As with the direct
    /// evaluation methods, they are overloaded to optionally provide
    /// evaluation for 1st and 2nd derivatives.
    ///
    /// In addition to methods to provide limit stencils, methods are also
    /// provided to apply them to the control points. Since application of
    /// stencils is identical for each (i.e. the same for position and any
    /// derivative) no overloads are provided for derivatives.
    ///       

    /// @brief Evaluation of the limit stencil for position
    int EvaluateStencil(REAL const uv[2], REAL sP[]) const;

    /// @brief Overload of limit stencil evaluation for 1st derivatives
    int EvaluateStencil(REAL const uv[2], REAL sP[],
                        REAL sDu[], REAL sDv[]) const;

    /// @brief Overload of limit stencil evaluation for 2nd derivatives
    int EvaluateStencil(REAL const uv[2], REAL sP[],
                        REAL sDu[],  REAL sDv[],
                        REAL sDuu[], REAL sDuv[], REAL sDvv[]) const;

    /// @brief Apply a single stencil to control points from a local array
    void ApplyStencil(REAL const stencil[],
                      REAL const controlPoints[], PointDescriptor const &,
                      REAL result[]) const;

    /// @brief Apply a single stencil to control points from the mesh data
    void ApplyStencilFromMesh(REAL const stencil[],
                              REAL const meshPoints[], PointDescriptor const &,
                              REAL result[]) const;
    //@}

private:
    //  Internal methods for evaluating derivatives, basis weights and
    //  stencils for regular, irregular and irregular linear patches:
    typedef Vtr::ConstArray<int> IndexArray;

    void evaluateDerivs(REAL const uv[2], REAL const patchPoints[],
                        PointDescriptor const &, REAL * derivs[]) const;
    void evalRegularDerivs(REAL const uv[2], REAL const patchPoints[],
                           PointDescriptor const &, REAL * derivs[]) const;
    void evalIrregularDerivs(REAL const uv[2], REAL const patchPoints[],
                             PointDescriptor const &, REAL * derivs[]) const;
    void evalMultiLinearDerivs(REAL const uv[2], REAL const patchPoints[],
                               PointDescriptor const &, REAL * derivs[]) const;

    void       evalRegularBasis(REAL const uv[2], REAL * wDeriv[]) const;
    IndexArray evalIrregularBasis(REAL const uv[2], REAL * wDeriv[]) const;
    int        evalMultiLinearBasis(REAL const uv[2], REAL * wDeriv[]) const;

    int evaluateStencils(REAL const uv[2], REAL * sDeriv[]) const;
    int evalRegularStencils(REAL const uv[2], REAL * sDeriv[]) const;
    int evalIrregularStencils(REAL const uv[2], REAL * sDeriv[]) const;
    int evalMultiLinearStencils(REAL const uv[2], REAL * sDeriv[]) const;

    //  Internal methods to compute patch points:
    void computeLinearPatchPoints(REAL p[], PointDescriptor const &) const;
    void computeIrregularPatchPoints(REAL p[], PointDescriptor const &) const;

    //  Internal methods specific to regular or irregular patches:
    unsigned char getRegPatchType() const { return _data.getRegPatchType(); }
    unsigned char getRegPatchMask() const { return _data.getRegPatchMask(); }

    internal::IrregularPatchType const & getIrregPatch() const;

private:
    //  Access to the set of member variables - provided to the Factory:
    friend class SurfaceFactory;

    internal::SurfaceData       & getSurfaceData()       { return _data; }
    internal::SurfaceData const & getSurfaceData() const { return _data; }

private:
    //  All member variables encapsulated in a single class:
    internal::SurfaceData _data;
};


//
//  Simple inline methods composed of other methods:
//
template <typename REAL>
inline void
Surface<REAL>::ComputePatchPoints(REAL points[],
                                  PointDescriptor const & pointDesc) const {

    if (!IsRegular()) {
        if (IsLinear()) {
            computeLinearPatchPoints(points, pointDesc);
        } else {
            computeIrregularPatchPoints(points, pointDesc);
        }
    }
}

template <typename REAL>
inline void
Surface<REAL>::PreparePatchPoints(
        REAL const meshPoints[], PointDescriptor const & meshPointDesc,
        REAL patchPoints[],  PointDescriptor const & patchPointDesc) const {

    GatherControlPoints(meshPoints, meshPointDesc, patchPoints, patchPointDesc);
    ComputePatchPoints(patchPoints, patchPointDesc);
}

//
//  Inline invocations of more general methods for derivative overloads:
//
template <typename REAL>
inline void
Surface<REAL>::evaluateDerivs(REAL const uv[2],
                              REAL const patchPoints[],
                              PointDescriptor const & pointDesc,
                              REAL * derivatives[]) const {
    if (IsRegular()) {
        evalRegularDerivs(uv, patchPoints, pointDesc, derivatives);
    } else if (IsLinear()) {
        evalMultiLinearDerivs(uv, patchPoints, pointDesc, derivatives);
    } else {
        evalIrregularDerivs(uv, patchPoints, pointDesc, derivatives);
    }
}
template <typename REAL>
inline void
Surface<REAL>::Evaluate(REAL const uv[2],
                        REAL const patchPoints[],
                        PointDescriptor const & pointDesc,
                        REAL P[]) const {

    REAL * derivatives[6] = { P, 0, 0, 0, 0, 0 };
    evaluateDerivs(uv, patchPoints, pointDesc, derivatives);
}
template <typename REAL>
inline void
Surface<REAL>::Evaluate(REAL const uv[2],
                        REAL const patchPoints[],
                        PointDescriptor const & pointDesc,
                        REAL P[], REAL Du[], REAL Dv[]) const {

    REAL * derivatives[6] = { P, Du, Dv, 0, 0, 0 };
    evaluateDerivs(uv, patchPoints, pointDesc, derivatives);
}
template <typename REAL>
inline void
Surface<REAL>::Evaluate(REAL const uv[2],
                        REAL const patchPoints[],
                        PointDescriptor const & pointDesc,
                        REAL P[],   REAL Du[],  REAL Dv[],
                        REAL Duu[], REAL Duv[], REAL Dvv[]) const {

    REAL * derivatives[6] = { P, Du, Dv, Duu, Duv, Dvv };
    evaluateDerivs(uv, patchPoints, pointDesc, derivatives);
}

template <typename REAL>
inline int
Surface<REAL>::evaluateStencils(REAL const uv[2], REAL * sDeriv[]) const {

    if (IsRegular()) {
        return evalRegularStencils(uv, sDeriv);
    } else if (IsLinear()) {
        return evalMultiLinearStencils(uv, sDeriv);
    } else {
        return evalIrregularStencils(uv, sDeriv);
    }
}
template <typename REAL>
inline int
Surface<REAL>::EvaluateStencil(REAL const uv[2], REAL sP[]) const {

    REAL * derivativeStencils[6] = { sP, 0, 0, 0, 0, 0 };
    return evaluateStencils(uv, derivativeStencils);
}
template <typename REAL>
inline int
Surface<REAL>::EvaluateStencil(REAL const uv[2],
                          REAL sP[], REAL sDu[], REAL sDv[]) const {

    REAL * derivativeStencils[6] = { sP, sDu, sDv, 0, 0, 0 };
    return evaluateStencils(uv, derivativeStencils);
}
template <typename REAL>
inline int
Surface<REAL>::EvaluateStencil(REAL const uv[2],
                          REAL sP[],   REAL sDu[],  REAL sDv[],
                          REAL sDuu[], REAL sDuv[], REAL sDvv[]) const {

    REAL * derivativeStencils[6] = { sP, sDu, sDv, sDuu, sDuv, sDvv };
    return evaluateStencils(uv, derivativeStencils);
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_SURFACE */
