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

#ifndef OPENSUBDIV3_BFR_TESSELLATION_H
#define OPENSUBDIV3_BFR_TESSELLATION_H

#include "../version.h"

#include "../bfr/parameterization.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

///
/// @brief Encapsulates a specific tessellation pattern of a Parameterization
///
/// Tessellation is a simple class that encapsulates a specified tessellation
/// pattern for a given Parameterization. Tessellation parameters are given
/// on construction and are fixed for its lifetime.
///
/// Methods allow inspection of the pattern in terms of the 2D coordinates of
/// the points comprising the pattern and the faces that connect them. The
/// 2D coordinates are referred to both in the documentation and the interface
/// as "coords" while the faces connecting them are referred to as "facets"
/// (to distinguish from the faces of the mesh, to which a Tessellation is
/// applied).
///
class Tessellation {
public:
    ///
    /// @brief Options configure a Tessellation to specify the nature of
    ///        both its results and the structure of the coordinate and
    ///        facet index arrays that its methods will populate.
    ///
    /// The sizes and strides of the target arrays should be specified
    /// explicitly as they are not inferred by the presence of other
    /// options.
    ///
    /// Modifiers of Options return a reference to itself to facilitate
    /// inline usage.
    ///
    class Options {
    public:
        Options() : _preserveQuads(false), _facetSize4(false),
                    _coordStride(0), _facetStride(0) { }

        /// @brief Select preservation of quads for quad-based subdivision
        ///        (requires 4-sided facets, default is off)
        Options & PreserveQuads(bool on);
        /// @brief Return if preservation of quads is set
        bool      PreserveQuads() const { return _preserveQuads; }

        /// @brief Assign the number of indices per facet (must be 3 or 4,
        ///        default is 3)
        Options & SetFacetSize(int numIndices);
        //  @brief Return the number of indices per facet
        int       GetFacetSize() const { return 3 + (int)_facetSize4; }

        /// @brief Assign the stride between facets (default is facet size)
        Options & SetFacetStride(int stride);
        /// @brief Return the stride between facets
        int       GetFacetStride() const { return _facetStride; }

        /// @brief Assign the stride between (u,v) pairs (default is 2)
        Options & SetCoordStride(int stride);
        /// @brief Return the stride between (u,v) pairs
        int       GetCoordStride() const { return _coordStride; }

    private:
        unsigned int _preserveQuads : 1;
        unsigned int _facetSize4    : 1;

        short _coordStride;
        short _facetStride;
    };

public:
    //@{
    /// @name Construction and initialization
    ///
    /// Constructors require a Parameterization of a face, a set of one or
    /// more tessellation rates, and a standard set of options.
    ///
    /// As with other classes, constructors can produce invalid instances if
    /// given obviously invalid arguments, e.g. an invalid Parameterization,
    /// non-positive tessellation rate, etc.
    ///

    /// @brief Simple constructor providing a single uniform tessellation rate.
    ///
    /// @param  p           Parameterization of a face to be tessellated
    /// @param  uniformRate Integer tessellation rate (non-zero)
    /// @param  options     Options describing tessellation results
    ///
    Tessellation(Parameterization const & p, int uniformRate,
                 Options const & options = Options());

    ///
    /// @brief General constructor providing multiple tessellation rates for
    ///        a non-uniform tessellation.
    ///
    /// @param  p           Parameterization of a face to be tessellated
    /// @param  numRates    The number of tessellation rates provided, which
    ///                     usually includes one per edge of the face (more
    ///                     details below)
    /// @param  rates       The array of non-zero integer tessellation rates
    /// @param  options     Options describing tessellation results
    ///
    /// For a Parameterization of a face with N edges, the acceptable number
    /// of tessellation rates can vary. Aside from N "outer" tessellation
    /// rates (one for each edge), all faces can have at least one "inner"
    /// rate additionally specified while quads can have two inner rates.
    ///
    /// If inner rates are not specified in addition to the N outer rates,
    /// they will be inferred (so it is not necessary to initialize quads
    /// distinctly from other faces). Similarly -- though less useful -- the
    /// smaller set of inner rates can be specified, leaving all outer rates
    /// to be inferred.
    ///
    /// For a face with N edges, the full set of acceptable rates and their
    /// interpretations is as follows:
    ///
    ///      1  - single explicit inner rate (uniform)
    ///      2  - (quads only) two explicit inner rates, outer rates inferred
    ///      N  - explicit edge rates, inner rates inferred
    ///     N+1 - explicit edge rates, explicit inner rate
    ///     N+2 - (quads only) explicit edge rates, two explicit inner rates
    ///
    /// When associating rates with edges, note that rates[0] corresponds
    /// to the edge between vertices 0 and 1. This is consistent with use
    /// elsewhere in OpenSubdiv -- where edge i lies between vertices i and
    /// i+1 -- but differs from the conventions used with many hardware
    ///  tessellation interfaces.
    ///
    Tessellation(Parameterization const & p, int numRates, int const rates[],
                 Options const & options = Options());

    /// @brief Return true if correctly initialized
    bool IsValid() const { return _isValid; }

    /// @brief Default construction is unavailable
    Tessellation() = delete;

    Tessellation(Tessellation const &) = delete;
    Tessellation & operator=(Tessellation const &) = delete;
    ~Tessellation();
    //@}

    //@{
    /// @name Simple queries
    ///
    /// Simple queries of a valid Tessellation.
    ///

    /// @brief Return the Parameterization
    Parameterization GetParameterization() const { return _param; }

    /// @brief Return the size of the face
    int GetFaceSize() const { return _param.GetFaceSize(); }

    /// @brief Retrieve the rates assigned
    int GetRates(int rates[]) const;

    /// @brief Return if the pattern is uniform
    bool IsUniform() const { return _isUniform; }
    //@}

    //@{
    /// @name Methods to inspect and gather coordinates
    ///
    /// Methods to determine the number of sample points involved in the
    /// tessellation pattern and their content are available for the entire
    /// pattern, or for parts of the boundary or interior of the pattern.
    ///
    /// The methods that assign the coordinate arrays also return the number
    /// of coordinates assigned, so the methods that just return those sizes
    /// are not necessary if arrays for the resulting coords have already
    /// been sufficiently allocated.
    ///

    /// @brief Return the number of coordinates in the entire pattern
    int GetNumCoords() const { return _numInteriorPoints + _numBoundaryPoints; }

    /// @brief Return the number of elements between each coordinate
    int GetCoordStride() const { return _coordStride; }

    /// @brief Return the number of boundary coordinates
    int GetNumBoundaryCoords() const { return _numBoundaryPoints; }

    /// @brief Return the number of interior coordinates
    int GetNumInteriorCoords() const { return _numInteriorPoints; }

    /// @brief Return the number of coordinates within a given edge
    ///        (excluding those at its end vertices)
    int GetNumEdgeCoords(int edge) const { return _outerRates[edge] - 1; }

    /// @brief Retrieve the coordinates for the entire pattern
    template <typename REAL>
    int GetCoords(REAL coordTuples[]) const;

    /// @brief Retrieve the coordinates for the boundary
    template <typename REAL>
    int GetBoundaryCoords(REAL coordTuples[]) const;

    /// @brief Retrieve the coordinates for the boundary
    template <typename REAL>
    int GetInteriorCoords(REAL coordTuples[]) const;

    /// @brief Retrieve the coordinate for a given vertex of the face
    template <typename REAL>
    int GetVertexCoord(int vertex, REAL coordTuples[]) const;

    /// @brief Retrieve the coordinates for a given edge of the face
    ///        (excluding those at its end vertices)
    template <typename REAL>
    int GetEdgeCoords(int edge,  REAL coordTuples[]) const;
    //@}

    //@{
    /// @name Methods to inspect and gather facets
    ///
    /// Methods to inspect the number and values of facets. Facets are
    /// simply integer tuples of size 3 or 4 which contain the indices
    /// of the coordinates generated by the Tessellation.
    ///
    /// Unlike the coordinates -- which can be separated into those on
    /// the boundary or interior of the pattern -- the facets are not
    /// distinguished in any way. 
    ///

    /// @brief Return the number of facets in the entire pattern
    int GetNumFacets() const { return _numFacets; }

    /// @brief Return the number of indices assigned to each facet
    int GetFacetSize() const { return _facetSize; }

    /// @brief Return the number of elements between each facet
    int GetFacetStride() const { return _facetStride; }

    /// @brief Retrieve the facet indices for the entire pattern
    int GetFacets(int facetTuples[]) const;
    //@}

    //@{
    /// @name Methods to modify the coordinate indices of facets
    ///
    /// Methods to modify the coordinate indices of facets rely on the
    /// coordinate indices being identifiable as those of boundary or
    /// interior coordinates. The first N coordinates generated by
    /// Tessellation will be on the boundary, while the remaining M
    /// will be for the interior.
    ///
    /// The boundary and interior coordinate indices can be transformed
    /// collectively or separately by offsets or by explicit reassignment.
    /// Both the boundary and interior indices must be modified at the
    /// same time while the Tessellation can distinguish them, i.e. a
    /// boundary coord is identified by index < N and an interior coord
    /// by index >= N.
    ///

    /// @brief Apply a common offset to all facet coordinate indices
    void TransformFacetCoordIndices(int facetTuples[], int commonOffset);

    /// @brief Reassign indices of boundary coordinates while offseting
    ///        those of interior coordinates
    void TransformFacetCoordIndices(int facetTuples[],
                                    int const boundaryIndices[],
                                    int       interiorOffset);

    /// @brief Reassign all facet coordinate indices
    void TransformFacetCoordIndices(int facetTuples[],
                                    int const boundaryIndices[],
                                    int const interiorIndices[]);
    //@}

private:
    //  Private initialization methods:
    bool validateArguments(Parameterization const & p,
                    int nRates, int const rates[], Options const & options);

    void initialize(Parameterization const & p,
                    int nRates, int const rates[], Options const & options);

    void initializeDefaults();
    int  initializeRates(int nRates, int const rates[]);
    void initializeInventoryForParamTri(int sumOfOuterRates);
    void initializeInventoryForParamQuad(int sumOfOuterRates);
    void initializeInventoryForParamQPoly(int sumOfOuterRates);

private:
    //  Private members:
    Parameterization _param;

    unsigned short _isValid       :  1;
    unsigned short _isUniform     :  1;
    unsigned short _triangulate   :  1;
    unsigned short _singleFace    :  1;
    unsigned short _segmentedFace :  1;
    unsigned short _triangleFan   :  1;
    unsigned short _splitQuad     :  1;

    short _facetSize;
    int   _facetStride;
    int   _coordStride;

    int _numGivenRates;
    int _numBoundaryPoints;
    int _numInteriorPoints;
    int _numFacets;

    int  _innerRates[2];
    int* _outerRates;
    int  _outerRatesLocal[4];
};

//
//  Inline implementations:
//
inline Tessellation::Options &
Tessellation::Options::PreserveQuads(bool on) {
    _preserveQuads = on;
    return *this;
}
inline Tessellation::Options &
Tessellation::Options::SetFacetSize(int numIndices) {
    _facetSize4 = (numIndices == 4);
    return *this;
}
inline Tessellation::Options &
Tessellation::Options::SetFacetStride(int stride)  {
    _facetStride = (short) stride;
    return *this;
}
inline Tessellation::Options &
Tessellation::Options::SetCoordStride(int stride) {
    _coordStride = (short) stride;
    return *this;
}

template <typename REAL>
inline int
Tessellation::GetVertexCoord(int vertex, REAL coord[]) const {
    _param.GetVertexCoord(vertex, coord);
    return 1;
}

template <typename REAL>
inline int
Tessellation::GetCoords(REAL coordTuples[]) const {
    int nCoords = GetBoundaryCoords(coordTuples);
    nCoords += GetInteriorCoords(coordTuples + nCoords * _coordStride);
    return nCoords;
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_TESSELLATION */
