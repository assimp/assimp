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

#ifndef OPENSUBDIV3_BFR_PARAMETERIZATION_H
#define OPENSUBDIV3_BFR_PARAMETERIZATION_H

#include "../version.h"

#include "../sdc/types.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

///
/// @brief Simple class defining the 2D parameterization of a face
///
/// Parameterization is a simple class that provides information about the
/// parameterization of a face in a local (u,v) coordinate system. It is
/// defined by the size of a face (i.e. its number of vertices) and the
/// subdivision scheme that determines its limit surface.
///
/// As an example of how the subdivision scheme is essential in determining
/// the Parameterization, consider the case of a triangle.  A triangle is
/// regular for the Loop scheme and so has a very simple parameterization
/// as a triangular patch. But for the Catmull-Clark scheme, a triangle is
/// an irregular face that must first be subdivided -- making its limit
/// surface a piecewise collection of quadrilateral patches.
///
class Parameterization {
public:
    ///
    /// @brief Enumerated type for the different kinds of Parameterizations.
    ///
    /// The three kinds of parameterizations defined are:  quadrilateral,
    /// triangle and quadrangulated sub-faces.  This is not intended for
    /// common use, but is publicly available for situations when it is
    /// necessary to distinguish:
    ///
    enum Type { QUAD,          ///<  Quadrilateral
                TRI,           ///<  Triangle
                QUAD_SUBFACES  ///<  Partitioned into quadrilateral sub-faces
    };

public:
    //@{
    /// @name Construction and initialization
    ///
    /// Valid construction of a Parameterization is only achieved with
    /// the non-default constructor. A Parameterization will be invalid
    /// (and so unusable) if default constructed, or constructed using
    /// arguments that describe a face that cannot be parameterized.
    ///

    /// @brief Primary constructor with subdivision scheme and face size
    Parameterization(Sdc::SchemeType scheme, int faceSize);

    /// @brief Returns true if correctly initialized
    bool IsValid() const { return (_faceSize > 0); }

    /// @brief Default construction produces an invalid instance
    Parameterization() : _type(0), _uDim(0), _faceSize(0) { }

    Parameterization(Parameterization const &) = default;
    Parameterization & operator=(Parameterization const &) = default;
    ~Parameterization() = default;
    //@}

    //@{
    /// @name Simple queries
    ///
    /// Simple queries of a valid Parameterization.
    ///

    /// @brief Returns the type of parameterization assigned
    Type GetType() const { return (Type) _type; }

    /// @brief Returns the size (number of vertices) of the corresponding face
    int  GetFaceSize() const { return _faceSize; }
    //@}

public:
    //@{
    /// @name Methods to inspect parametric features
    ///
    /// Methods are available to inspect common topological features of a
    /// Parameterization, i.e. the parametric coordinates corresponding
    /// to the vertices, edges or center of the face it represents.
    ///
    /// Methods for vertices and edges require an index of the desired
    /// vertex or edge. The edge parameter "t" locally parameterizes the
    /// edge over [0,1] in a counter-clockwise orientation.
    ///

    /// @brief Returns the (u,v) coordinate of a given vertex
    template <typename REAL>
    void GetVertexCoord(int vertexIndex, REAL uvCoord[2]) const;

    /// @brief Returns the (u,v) coordinate at any point on a given edge
    template <typename REAL>
    void GetEdgeCoord(int edgeIndex, REAL t, REAL uvCoord[2]) const;

    /// @brief Returns the (u,v) coordinate for the center of the face
    template <typename REAL>
    void GetCenterCoord(REAL uvCoord[2]) const;
    //@}

public:
    //@{
    /// @name Methods to deal with discontinuous parameterizations
    ///
    /// Parameterizations that have been partitioned into sub-faces are
    /// discontinuous and warrant care in order to process them effectively --
    /// often requiring explicit conversions.
    ///
    /// These conversion methods to and from the local coordinates of a
    /// sub-face are only for use with instances of Parameterization that
    /// have such sub-faces. Results for input coordinates that are
    /// significantly outside the domain of the input parameterization are
    /// undefined.
    ///
    /// Note that sub-face coordinates that are normalized correspond to
    /// coordinates for Ptex faces.
    ///

    /// @brief Returns if Parameterization has been partitioned into sub-faces
    bool HasSubFaces() const;

    /// @brief Returns the integer sub-face containing the given (u,v)
    template <typename REAL>
    int GetSubFace(REAL const uvCoord[2]) const;

    /// @brief Convert (u,v) to a sub-face (return value) and its local (u,v)
    ///        coordinate
    template <typename REAL>
    int ConvertCoordToSubFace(
                REAL const uvCoord[2], REAL subFaceCoord[2]) const;

    /// @brief Convert a sub-face and its local (u,v) coordinate to (u,v)
    template <typename REAL>
    void ConvertSubFaceToCoord(int subFace,
                REAL const subFaceCoord[2], REAL uvCoord[2]) const;

    /// @brief Convert (u,v) to a sub-face (return value) and its normalized
    ///        (u,v) coordinate
    template <typename REAL>
    int ConvertCoordToNormalizedSubFace(
                REAL const uvCoord[2], REAL subFaceCoord[2]) const;

    /// @brief Convert a sub-face and its normalized (u,v) coordinate to (u,v)
    template <typename REAL>
    void ConvertNormalizedSubFaceToCoord(int subFace,
                REAL const subFaceCoord[2], REAL uvCoord[2]) const;
    //@}

private:
    template <typename REAL>
    int convertCoordToSubFace(bool normalized,
                REAL const uvCoord[2], REAL subFaceCoord[2]) const;
    template <typename REAL>
    void convertSubFaceToCoord(bool normalized, int subFace,
                REAL const subFaceCoord[2], REAL uvCoord[2]) const;

private:
    unsigned char  _type;
    unsigned char  _uDim;
    unsigned short _faceSize;
};

//
//  Inline sub-face coordinate conversion methods:
//
inline bool
Parameterization::HasSubFaces() const {
    return (_type == QUAD_SUBFACES);
}

template <typename REAL>
inline int
Parameterization::GetSubFace(REAL const uvCoord[2]) const {

    if (!HasSubFaces()) return 0;

    int uTile = (int) uvCoord[0];
    int vTile = (int) uvCoord[1];
    return (vTile + ((uvCoord[1] - (REAL) vTile) > 0.75f)) * _uDim +
           (uTile + ((uvCoord[0] - (REAL) uTile) > 0.75f));
}

//  Conversions to unnormalized sub-face coordinates:
template <typename REAL>
inline int
Parameterization::ConvertCoordToSubFace(
        REAL const uvCoord[2], REAL subCoord[2]) const {
    return convertCoordToSubFace<REAL>(false, uvCoord, subCoord);
}
template <typename REAL>
inline void
Parameterization::ConvertSubFaceToCoord(
        int subFace, REAL const subCoord[2], REAL uvCoord[2]) const {
    convertSubFaceToCoord<REAL>(false, subFace, subCoord, uvCoord);
}

//  Conversions to normalized sub-face coordinates:
template <typename REAL>
inline int
Parameterization::ConvertCoordToNormalizedSubFace(
        REAL const uvCoord[2], REAL subCoord[2]) const {
    return convertCoordToSubFace<REAL>(true, uvCoord, subCoord);
}
template <typename REAL>
inline void
Parameterization::ConvertNormalizedSubFaceToCoord(
        int subFace, REAL const subCoord[2], REAL uvCoord[2]) const {
    convertSubFaceToCoord<REAL>(true, subFace, subCoord, uvCoord);
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_PARAMETERIZATION */
