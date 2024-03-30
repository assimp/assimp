//
//   Copyright 2014 DreamWorks Animation LLC.
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
#ifndef OPENSUBDIV3_SDC_OPTIONS_H
#define OPENSUBDIV3_SDC_OPTIONS_H

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Sdc {

///
///  \brief All supported options applying to subdivision scheme.
///
///  The Options class contains all supported options that can be applied to a
///  subdivision scheme to affect the shape of the limit surface.  These differ from
///  approximations that may be applied at a higher level, i.e. options to limit the
///  level of feature adaptive subdivision, options to ignore fractional creasing,
///  or creasing entirely, etc. These options define the shape of a particular
///  limit surface, including the "shape" of primitive variable data associated with
///  it.
///
///  The intent is that these sets of options be defined at a high level and
///  propagated into the lowest-level computation in support of each subdivision
///  scheme.  Ideally it remains a set of bit-fields (essentially an int) and so
///  remains light weight and easily passed around by value.
///

class Options {
public:
    enum VtxBoundaryInterpolation {
        VTX_BOUNDARY_NONE = 0,        ///< no boundary interpolation, except where
                                      ///< boundary edges were explicitly sharpened
        VTX_BOUNDARY_EDGE_ONLY,       ///< all boundary edges sharpened and interpolated
        VTX_BOUNDARY_EDGE_AND_CORNER  ///< all boundary edges and corner vertices
                                      ///< sharpened and interpolated
    };
    enum FVarLinearInterpolation {
        FVAR_LINEAR_NONE = 0,         ///< smooth everywhere ("edge only")
        FVAR_LINEAR_CORNERS_ONLY,     ///< sharpen corners only
        FVAR_LINEAR_CORNERS_PLUS1,    ///< ("edge corner")
        FVAR_LINEAR_CORNERS_PLUS2,    ///< ("edge and corner + propagate corner")
        FVAR_LINEAR_BOUNDARIES,       ///< sharpen all boundaries ("always sharp")
        FVAR_LINEAR_ALL               ///< bilinear interpolation ("bilinear")
    };
    enum CreasingMethod {
        CREASE_UNIFORM = 0,           ///< Catmark rule
        CREASE_CHAIKIN                ///< Chaikin rule
    };
    enum TriangleSubdivision {
        TRI_SUB_CATMARK = 0,          ///< Catmark weights (Catmark scheme only)
        TRI_SUB_SMOOTH                ///< "smooth triangle" weights (Catmark scheme only)
    };

public:

    Options() : _vtxBoundInterp(VTX_BOUNDARY_NONE),
                _fvarLinInterp(FVAR_LINEAR_ALL),
                _creasingMethod(CREASE_UNIFORM),
                _triangleSub(TRI_SUB_CATMARK) { }

    //
    //  Trivial get/set methods:
    //

    /// \brief Get vertex boundary interpolation rule
    VtxBoundaryInterpolation GetVtxBoundaryInterpolation() const { return (VtxBoundaryInterpolation) _vtxBoundInterp; }

    /// \brief Set vertex boundary interpolation rule
    void SetVtxBoundaryInterpolation(VtxBoundaryInterpolation b) { _vtxBoundInterp = (EnumIntType) b; }

    /// \brief Get face-varying interpolation rule
    FVarLinearInterpolation GetFVarLinearInterpolation() const { return (FVarLinearInterpolation) _fvarLinInterp; }

    /// \brief Set face-varying interpolation rule
    void SetFVarLinearInterpolation(FVarLinearInterpolation b) { _fvarLinInterp = (EnumIntType) b; }

    /// \brief Get edge crease rule
    CreasingMethod GetCreasingMethod() const { return (CreasingMethod) _creasingMethod; }

    /// \brief Set edge crease rule
    void SetCreasingMethod(CreasingMethod c) { _creasingMethod = (EnumIntType) c; }

    /// \brief Get triangle subdivision weights rule (Catmark scheme only !)
    TriangleSubdivision GetTriangleSubdivision() const { return (TriangleSubdivision) _triangleSub; }

    /// \brief Set triangle subdivision weights rule (Catmark scheme only !)
    void SetTriangleSubdivision(TriangleSubdivision t) { _triangleSub = (EnumIntType) t; }

private:
    //  Use a small integer type to pack these rather than bitfields:
    typedef unsigned char EnumIntType;

    EnumIntType _vtxBoundInterp;
    EnumIntType _fvarLinInterp;
    EnumIntType _creasingMethod;
    EnumIntType _triangleSub;
};

} // end namespace sdc

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_SDC_OPTIONS_H */
