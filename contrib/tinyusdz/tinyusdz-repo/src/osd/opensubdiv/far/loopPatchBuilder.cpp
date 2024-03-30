//
//   Copyright 2018 DreamWorks Animation LLC.
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

#ifdef _MSC_VER
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#endif

#include "../far/loopPatchBuilder.h"
#include "../vtr/stackBuffer.h"
#include "../sdc/loopScheme.h"

#include <cassert>
#include <cmath>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

using Vtr::Array;
using Vtr::ConstArray;
using Vtr::internal::StackBuffer;

namespace Far {

constexpr auto kPI = 3.14159265358979323846;

//
//  A simple struct with methods to compute Loop limit points (following
//  the pattern established for Catmull-Clark limit points)
//
//  Unlike the corresponding Catmull-Clark struct, Loop limit points are
//  computed using the limit masks provided by the Sdc Scheme for Loop.
//
template <typename REAL>
struct LoopLimits {
public:
    //
    //  Local classes to fulfill the interface requirements of template
    //  arguments to the Sdc::Scheme methods
    //
    //  Class fulfilling <typename VERTEX>:
    //
    class LimitVertex {
    public:
        LimitVertex() : _nFaces(0), _nEdges(0) { }
        LimitVertex(int faces, int edges) : _nFaces(faces), _nEdges(edges) { }

        void Assign(int faces, int edges) { _nFaces = faces; _nEdges = edges; }

        int GetNumEdges() const { return _nEdges; }
        int GetNumFaces() const { return _nFaces; }

        void GetSharpnessPerEdge(float sharpness[]) const {
            sharpness[0] = Sdc::Crease::SHARPNESS_INFINITE;
            for (int i = 1; i < _nEdges - 1; ++i) {
                sharpness[i] = Sdc::Crease::SHARPNESS_SMOOTH;
            }
            sharpness[_nEdges - 1] = Sdc::Crease::SHARPNESS_INFINITE;
        }

    private:
        int _nFaces;
        int _nEdges;
    };

    //
    //  Class fulfilling <typename MASK>:
    //
    class LimitMask {
    public:
        typedef REAL Weight;  //  Also part of the expected interface

    public:
        LimitMask(Weight* w) : _weights(w), _valence(0) { }
        ~LimitMask() { }

    public:  //  Generic interface expected of <typename MASK>:
        int GetNumVertexWeights() const { return 1; }
        int GetNumEdgeWeights()   const { return _valence; }
        int GetNumFaceWeights()   const { return 0; }

        void SetNumVertexWeights(int)     { }
        void SetNumEdgeWeights(int count) { _valence = count; }
        void SetNumFaceWeights(int)       { }

        Weight const& VertexWeight(int)     const { return _weights[0]; }
        Weight const& EdgeWeight(int index) const { return _weights[1 + index]; }
        Weight const& FaceWeight(int)       const { return _weights[0]; }

        Weight& VertexWeight(int)       { return _weights[0]; }
        Weight& EdgeWeight(  int index) { return _weights[1 + index]; }
        Weight& FaceWeight(  int)       { return _weights[0]; }

        bool AreFaceWeightsForFaceCenters() const  { return false; }
        void SetFaceWeightsForFaceCenters(bool)    { }

    private:
        Weight* _weights;
        int     _valence;
    };
public:
    typedef Sdc::Scheme<Sdc::SCHEME_LOOP>   SchemeLoop;

    static void ComputeInteriorPointWeights(int valence, int faceInRing,
                REAL* pWeights, REAL* epWeights, REAL* emWeights);

    static void ComputeBoundaryPointWeights(int valence, int faceInRing,
                REAL* pWeights, REAL* epWeights, REAL* emWeights);
};

template <typename REAL>
void
LoopLimits<REAL>::ComputeInteriorPointWeights(int valence, int faceInRing,
                REAL* pWeights, REAL* epWeights, REAL* emWeights) {

    int ringSize = valence + 1;

    LimitVertex vertex(valence, valence);

    bool computeTangentPoints = epWeights && emWeights;
    if (!computeTangentPoints) {
        //
        //  The interior position mask is symmetric -- no need to rotate
        //  or otherwise account for orientation:
        //
        LimitMask pMask(pWeights);

        SchemeLoop().ComputeVertexLimitMask(vertex, pMask,
                Sdc::Crease::RULE_SMOOTH);
    } else {
        //
        //  The interior tangent masks will be directed along the first
        //  two edges (the second a rotation of the first).  Adjust the
        //  tangent weights for a point along the tangent, then rotate
        //  according to the face within the ring:
        //
        int weightWidth = valence + 1;
        StackBuffer<REAL, 32, true> tWeights(2 * weightWidth);
        REAL * t1Weights = &tWeights[0];
        REAL * t2Weights = t1Weights + ringSize;

        LimitMask pMask(pWeights);
        LimitMask t1Mask(t1Weights);
        LimitMask t2Mask(t2Weights);

        SchemeLoop().ComputeVertexLimitMask(vertex, pMask, t1Mask, t2Mask,
                Sdc::Crease::RULE_SMOOTH);

        //
        //  Use the subdominant eigenvalue to scale the limit tangent t1:
        //
        //      e = (3 + cos(2*PI/valence)) / 8
        //
        //  Combine it with a normalizing factor of (2 / valence) to account
        //  for the scale inherent in the tangent weights, and (2 / 3) to
        //  match desired placement of the cubic point in the regular case.
        //
        //  The weights for t1 can simply be rotated around the ring to yield
        //  t2.  Combine the weights for the point in a single set for t2 and
        //  then copy it into the appropriate orientation for ep and em:
        //
        double theta = 2.0 * kPI / (double) valence;

        REAL tanScale = (REAL) ((3.0 + 2.0 * std::cos(theta)) / (6.0 * valence));

        for (int i = 0; i < ringSize; ++i) {
            t2Weights[i] = pWeights[i] + t1Weights[i] * tanScale;
        }

        int n1 = faceInRing;
        int n2 = valence - n1;

        epWeights[0] = t2Weights[0];
        std::memcpy(epWeights + 1,      t2Weights + 1 + n2, n1 * sizeof(REAL));
        std::memcpy(epWeights + 1 + n1, t2Weights + 1,      n2 * sizeof(REAL));

        n1 = (faceInRing + 1) % valence;
        n2 = valence - n1;

        emWeights[0] = t2Weights[0];
        std::memcpy(emWeights + 1,      t2Weights + 1 + n2, n1 * sizeof(REAL));
        std::memcpy(emWeights + 1 + n1, t2Weights + 1,      n2 * sizeof(REAL));
    }
}

template <typename REAL>
void
LoopLimits<REAL>::ComputeBoundaryPointWeights(int valence, int faceInRing,
                REAL* pWeights, REAL* epWeights, REAL* emWeights) {

    LimitVertex vertex(valence - 1, valence);

    bool computeTangentPoints = epWeights && emWeights;
    if (!computeTangentPoints) {
        //
        //  The boundary position mask will be assigned non-zero weights
        //  for the vertex and its first and last edges:
        //
        LimitMask pMask(pWeights);

        SchemeLoop().ComputeVertexLimitMask(vertex, pMask,
                Sdc::Crease::RULE_CREASE);
    } else {
        //
        //  The boundary tangent masks need more explicit handling than
        //  the interior.  One of the tangents will be along the boundary
        //  and the other towards the interior, but one or both edge
        //  points may be along interior edges.  A boundary edge point is
        //  easy to deal with once identified, but interior edge points
        //  need a numerical rotation of the interior tangent to orient it
        //  along the desired edge.
        //
        int weightWidth = valence + 1;
        StackBuffer<REAL, 32, true> tWeights(2 * weightWidth);
        REAL * t1Weights = &tWeights[0];
        REAL * t2Weights = t1Weights + weightWidth;

        REAL t1Leading  =  (REAL) (1.0 / 6.0);
        REAL t1Trailing = -(REAL) (1.0 / 6.0);

        REAL t2Scale = (REAL) (1.0 / 24.0);

        LimitMask pMask(pWeights);
        LimitMask t1Mask(t1Weights);
        LimitMask t2Mask(t2Weights);

        SchemeLoop().ComputeVertexLimitMask(vertex, pMask, t1Mask, t2Mask,
                Sdc::Crease::RULE_CREASE);

        bool epOnLeadingEdge  = (faceInRing == 0);
        bool emOnTrailingEdge = (faceInRing == (valence - 1));

        if (epOnLeadingEdge) {
            std::memset(&epWeights[0], 0, weightWidth * sizeof(REAL));

            epWeights[0] = (REAL) (2.0 / 3.0);
            epWeights[1] = (REAL) (1.0 / 3.0);
        } else {
            int  iEdgeNext     = faceInRing;
            REAL faceAngle     = (REAL) (kPI / (double)(valence - 1));
            REAL faceAngleNext = faceAngle * (REAL)iEdgeNext;
            REAL cosAngleNext  = std::cos(faceAngleNext);
            REAL sinAngleNext  = std::sin(faceAngleNext);

            for (int i = 0; i < weightWidth; ++i) {
                epWeights[i] = t2Scale * t2Weights[i] * sinAngleNext;
            }
            epWeights[0]       += pWeights[0];
            epWeights[1]       += pWeights[1]       + t1Leading  * cosAngleNext;
            epWeights[valence] += pWeights[valence] + t1Trailing * cosAngleNext;
        }

        if (emOnTrailingEdge) {
            std::memset(&emWeights[0], 0, weightWidth * sizeof(REAL));

            emWeights[0]       = (REAL) (2.0 / 3.0);
            emWeights[valence] = (REAL) (1.0 / 3.0);
        } else {
            int  iEdgePrev     = (faceInRing + 1) % valence;
            REAL faceAngle     = (REAL) (kPI / (double)(valence - 1));
            REAL faceAnglePrev = faceAngle * (REAL)iEdgePrev;
            REAL cosAnglePrev  = std::cos(faceAnglePrev);
            REAL sinAnglePrev  = std::sin(faceAnglePrev);

            for (int i = 0; i < weightWidth; ++i) {
                emWeights[i] = t2Scale * t2Weights[i] * sinAnglePrev;
            }
            emWeights[0]       += pWeights[0];
            emWeights[1]       += pWeights[1]       + t1Leading  * cosAnglePrev;
            emWeights[valence] += pWeights[valence] + t1Trailing * cosAnglePrev;
        }
    }
}


//
//  SparseMatrixRow
//
//  This was copied from the CatmarkPatchBuilder as a starting point, so
//  comments below relate to the state of CatmarkPatchBuilder...
//
// This is a utility class representing a row of a SparseMatrix -- which
// in turn corresponds to a point of a resulting patch.  Instances of this
// class are intended to encapsulate the contributions of a point and be
// passed to functions as such.
//
// (Consider moving this to PatchBuilder as a protected class or maybe a
// public class within SparseMatrix itself, e.g. SparseMatrix<REAL>::Row.)
//
namespace {
    template <typename REAL>
    class SparseMatrixRow {
    public:
        SparseMatrixRow(SparseMatrix<REAL> & matrix, int row) :
            _size(matrix.GetRowSize(row)),
            _indices(matrix.SetRowColumns(row).begin()),
            _weights(matrix.SetRowElements(row).begin()) { }

        int GetSize() const { return _size; }

        void SetWeight(int i, REAL weight) { _weights[i] = weight; }

        void Assign(int rowEntry, Index index, REAL weight) {
            _indices[rowEntry] = index;
            _weights[rowEntry] = weight;
        }

        void Copy(SparseMatrixRow<REAL> const & other) {
            assert(GetSize() == other.GetSize());
            std::memcpy(_indices, other._indices, _size * sizeof(Index));
            std::memcpy(_weights, other._weights, _size * sizeof(REAL));
        }

    public:
        int     _size;
        Index * _indices;
        REAL *  _weights;
    };
} // end namespace

//
//  Simple utility functions for dealing with SparseMatrix:
//
namespace {
#ifdef __INTEL_COMPILER
#pragma warning (push)
#pragma warning disable 1572
#endif
    template <typename REAL>
    inline bool _isZero(REAL w) { return (w == (REAL)0.0); }
#ifdef __INTEL_COMPILER
#pragma warning (pop)
#endif

    template <typename REAL>
    void
    _initializeFullMatrix(SparseMatrix<REAL> & M, int nRows, int nColumns) {

        M.Resize(nRows, nColumns, nRows * nColumns);

        //  Fill row 0 with index for every column:
        M.SetRowSize(0, nColumns);
        Array<int> row0Columns = M.SetRowColumns(0);
        for (int i = 0; i < nColumns; ++i) {
            row0Columns[i] = i;
        }

        //  Copy row 0's indices into all other rows:
        for (int row = 1; row < nRows; ++row) {
            M.SetRowSize(row, nColumns);
            Array<int> dstRowColumns = M.SetRowColumns(row);
            std::memcpy(&dstRowColumns[0], &row0Columns[0], nColumns * sizeof(int));
        }
    }

    template <typename REAL>
    void
    _resizeMatrix(SparseMatrix<REAL> & matrix,
                  int numRows, int numColumns, int numElements,
                  int const rowSizes[]) {

        matrix.Resize(numRows, numColumns, numElements);
        for (int i = 0; i < numRows; ++i) {
            matrix.SetRowSize(i, rowSizes[i]);
        }
        assert(matrix.GetNumElements() == numElements);
    }

    template <typename REAL>
    void
    _addSparsePointToFullRow(REAL * fullRow,
                             SparseMatrixRow<REAL> const & p,
                             REAL s, int * indexMask) {

        for (int i = 0; i < p.GetSize(); ++i) {
            int index = p._indices[i];

            fullRow[index] += s * p._weights[i];

            indexMask[index] = 1 + index;
        }
    }

    template <typename REAL>
    void
    _combineSparsePointsInFullRow(SparseMatrixRow<REAL> & p,
                                  REAL aCoeff, SparseMatrixRow<REAL> const & a,
                                  REAL bCoeff, SparseMatrixRow<REAL> const & b,
                                  int rowSize, REAL * rowBuffer, int * maskBuffer) {

        std::memset(maskBuffer, 0, rowSize * sizeof(int));
        std::memset(rowBuffer,  0, rowSize * sizeof(REAL));

        _addSparsePointToFullRow(rowBuffer, a, aCoeff, maskBuffer);
        _addSparsePointToFullRow(rowBuffer, b, bCoeff, maskBuffer);

        int nWeights = 0;
        for (int i = 0; i < rowSize; ++i) {
            if (maskBuffer[i]) {
                p.Assign(nWeights++, maskBuffer[i] - 1, rowBuffer[i]);
            }
        }
        assert(nWeights <= p.GetSize());
        for (int i = nWeights; i < p.GetSize(); ++i, ++nWeights) {
            p.Assign(i, 0, 0.0f);
        }
    }

    template <typename REAL>
    void
    _addSparseRowToFull(REAL * fullRow,
                        SparseMatrix<REAL> const & M, int sparseRow, REAL s) {

        ConstArray<int>  indices = M.GetRowColumns(sparseRow);
        ConstArray<REAL> weights = M.GetRowElements(sparseRow);

        for (int i = 0; i < indices.size(); ++i) {
            fullRow[indices[i]] += s * weights[i];
        }
    }

    template <typename REAL>
    void
    _combineSparseMatrixRowsInFull(SparseMatrix<REAL> & dstMatrix, int dstRowIndex,
           SparseMatrix<REAL> const & srcMatrix, int numSrcRows,
           int const srcRowIndices[], REAL const * srcRowWeights) {

        REAL * dstRow = &dstMatrix.SetRowElements(dstRowIndex)[0];

        std::memset(dstRow, 0, dstMatrix.GetNumColumns() * sizeof(REAL));

        for (int i = 0; i < numSrcRows; ++i) {
            if (!_isZero(srcRowWeights[i])) {
                _addSparseRowToFull(dstRow, srcMatrix, srcRowIndices[i], srcRowWeights[i]);
            }
        }
    }

    template <typename REAL>
    void
    _matrixPrintDensity(const char* prefix, SparseMatrix<REAL> const & M) {
    
        int fullSize = M.GetNumRows() * M.GetNumColumns();
        int sparseSize = M.GetNumElements();

        int nonZeroSize = 0;
        for (int i = 0; i < M.GetNumRows(); ++i) {
            ConstArray<REAL> elements = M.GetRowElements(i);
            for (int j = 0; j < elements.size(); ++j) {
                nonZeroSize += (elements[j] != 0);
            }
        }
        printf("%s(%dx%d = %d):  elements = %d, non-zero = %d, density = %.1f\n",
            prefix, M.GetNumRows(), M.GetNumColumns(), fullSize,
            sparseSize, nonZeroSize, (REAL)nonZeroSize * 100.0f / (REAL)fullSize);
    }

    //
    //  The valence-2 interior case poses problems for the way patch points
    //  are computed as combinations of source points and stored as a row in
    //  a SparseMatrix.  An interior vertex of valence-2 causes duplicate
    //  vertices to appear in the 1-rings of its neighboring vertices and we
    //  want the entries of a SparseMatrix row to be unique.
    //
    //  For the most part, this does not pose a problem while the matrix (set
    //  of patch points) is being constructed, so we leave those duplicate
    //  entries in place and deal with them as a post-process here.
    //
    //  The SourcePatch is also sensitive to the presence of such valence-2
    //  vertices for its own reasons (it needs to identifiy a unique set of
    //  source points from a set of corner rings), so a simple query of its
    //  corners indicates when this post-process is necessary.  (And since
    //  this case is a rare occurrence, efficiency is not a major concern.)
    //
    template <typename REAL>
    void _removeValence2Duplicates(SparseMatrix<REAL> & M) {

        //  This will later be determined by the PatchBuilder member:
        int regFaceSize = 4;

        SparseMatrix<REAL> T;
        T.Resize(M.GetNumRows(), M.GetNumColumns(), M.GetNumElements());

        int nRows = M.GetNumRows();
        for (int row = 0; row < nRows; ++row) {
            int srcRowSize = M.GetRowSize(row);

            int const *  srcIndices = M.GetRowColumns(row).begin();
            REAL const * srcWeights = M.GetRowElements(row).begin();

            //  Scan the entries to see if there are duplicates -- copy
            //  the row if not, otherwise, need to compress it:
            bool cornerUsed[4] = { false, false, false, false };

            int srcDupCount = 0;
            for (int i = 0; i < srcRowSize; ++i) {
                int srcIndex = srcIndices[i];
                if (srcIndex < regFaceSize) {
                    srcDupCount += (int) cornerUsed[srcIndex];
                    cornerUsed[srcIndex] = true;
                }
            }

            //  Size this row for the destination and copy or compress:
            T.SetRowSize(row, srcRowSize - srcDupCount);

            int*  dstIndices = T.SetRowColumns(row).begin();
            REAL* dstWeights = T.SetRowElements(row).begin();

            if (srcDupCount) {
                REAL * cornerDstPtr[4] = { 0, 0, 0, 0 };

                for (int i = 0; i < srcRowSize; ++i) {
                    int  srcIndex  = *srcIndices++;
                    REAL srcWeight = *srcWeights++;

                    if (srcIndex < regFaceSize) {
                        if (cornerDstPtr[srcIndex]) {
                            *cornerDstPtr[srcIndex] += srcWeight;
                            continue;
                        } else {
                            cornerDstPtr[srcIndex] = dstWeights;
                        }
                    }
                    *dstIndices++ = srcIndex;
                    *dstWeights++ = srcWeight;
                }
            } else {
                std::memcpy(&dstIndices[0], &srcIndices[0], srcRowSize * sizeof(Index));
                std::memcpy(&dstWeights[0], &srcWeights[0], srcRowSize * sizeof(REAL));
            }
        }
        M.Swap(T);
    }
} // end namespace for SparseMatrix utilities


//
//  GregoryTriConverter
//
//  The GregoryTriConverter class provides a change-of-basis matrix from source
//  vertices in a Loop mesh to the 18 control points of a quartic Gregory triangle.
//
//  The quartic triangle is first constructed as a cubic/quartic hybrid -- with
//  cubic boundary curves and cross-boundary continuity formulated in terms of
//  cubics.  The result is then raised to a full quartic once continuity across
//  all boundaries is achieved.  In most cases 2 of the 3 boundaries will be
//  cubic (though now represented as quartic) and only one boundary need be a
//  true quartic to meet a regular Box-spline patch.
//
//  Control points are labeled using the convention adopted for quads, with
//  Ep and Em referring to the "plus" and "minus" edge points and similarly
//  for the face points Fp and Fm.  The additional quartic "mid-edge" points
//  associated with each boundary are referred to as M.
//
template <typename REAL>
class GregoryTriConverter {
public:
    typedef REAL                    Weight;
    typedef SparseMatrix<Weight>    Matrix;
    typedef SparseMatrixRow<Weight> Point;
public:
    GregoryTriConverter() : _numSourcePoints(0) { }
    GregoryTriConverter(SourcePatch const & sourcePatch);
    GregoryTriConverter(SourcePatch const & sourcePatch, Matrix & sparseMatrix);

    void Initialize(SourcePatch const & sourcePatch);

    bool IsIsolatedInteriorPatch() const { return _isIsolatedInteriorPatch; }
    bool HasVal2InteriorCorner() const { return _hasVal2InteriorCorner; }
    int  GetIsolatedInteriorCorner() const { return _isolatedCorner; }
    int  GetIsolatedInteriorValence() const { return _isolatedValence; }

    void Convert(Matrix & sparseMatrix) const;

private:
    //
    //  Local nested class for GregoryTriConverter to cache information for
    //  the corners of the source patch.  It copies some information from the
    //  SourcePatch so that we don't have to keep it around, but it contains
    //  additional information relevant to the determination of the Gregory
    //  points -- most notably classifications of the face-points and the
    //  cosines of angles for the face corners that are used repeatedly.
    //
    struct CornerTopology {
        //  Basic flags copied from the SourcePatch
        unsigned int isBoundary  : 1;
        unsigned int isSharp     : 1;
        unsigned int isDart      : 1;
        unsigned int isRegular   : 1;
        unsigned int isVal2Int   : 1;
        unsigned int isCorner    : 1;

        //  Flags for edge- and face-points relating to adjacent corners:
        unsigned int epOnBoundary : 1;
        unsigned int emOnBoundary : 1;

        unsigned int fpIsRegular : 1;
        unsigned int fmIsRegular : 1;
        unsigned int fpIsCopied  : 1;
        unsigned int fmIsCopied  : 1;

        //  Other values stored for repeated use:
        int  valence;
        int  numFaces;
        int  faceInRing;

        REAL faceAngle;
        REAL cosFaceAngle;

        //  Its useful to have the ring for each corner immediately available:
        StackBuffer<int, 30, true> ringPoints;
    };

    //
    //  Methods to resize the matrix before populating it:
    //
    void resizeMatrixUnisolated(Matrix & matrix) const;

    void resizeMatrixIsolatedIrregular(Matrix & matrix, int irregCornerIndex,
                                                        int irregValence) const;

    //
    //  Methods to compute the various rows of points in the matrix:
    //
    void assignRegularEdgePoints(int cornerIndex, Matrix & matrix) const;
    void computeIrregularEdgePoints(int cornerIndex, Matrix & matrix,
                                    Weight *weightBuffer) const;

    void assignRegularFacePoints(int cornerIndex, Matrix & matrix) const;
    void computeIrregularFacePoints(int cornerIndex, Matrix & matrix,
                                    Weight *weightBuffer, int *indexBuffer) const;

    void computeIrregularInteriorEdgePoints(int cornerIndex,
                                            Point & P, Point & Ep, Point & Em,
                                            Weight *weightBuffer) const;
    void computeIrregularBoundaryEdgePoints(int cornerIndex,
                                            Point & P, Point & Ep, Point & Em,
                                            Weight *weightBuffer) const;

    int  getIrregularFacePointSize(int cornerNear, int cornerFar) const;
    void computeIrregularFacePoint(
                int cornerNear, int edgeInNearRing, int cornerFar,
                Point const & p, Point const & eNear, Point const & eFar,
                Point & fNear, REAL signForSideOfEdge /* -1.0 or 1.0 */,
                Weight *rowWeights, int *columnMask) const;

    void assignRegularMidEdgePoint(int edgeIndex, Matrix & matrix) const;
    void computeIrregularMidEdgePoint(int edgeIndex, Matrix & matrix,
                                      Weight *rowWeights, int *columnMask) const;

    void promoteCubicEdgePointsToQuartic(Matrix & matrix,
                                         Weight *rowWeights, int *columnMask) const;

private:
    int _numSourcePoints;
    int _maxValence;

    bool _isIsolatedInteriorPatch;
    bool _hasVal2InteriorCorner;
    int  _isolatedCorner;
    int  _isolatedValence;

    CornerTopology _corners[3];
};


//
//  GregoryTriConverter
//
//  Construction and initialization/computation of the change-of-basis
//  matrix to a Gregory patch.
//
template <typename REAL>
GregoryTriConverter<REAL>::GregoryTriConverter(
        SourcePatch const & sourcePatch) {

    Initialize(sourcePatch);
}

template <typename REAL>
GregoryTriConverter<REAL>::GregoryTriConverter(
        SourcePatch const & sourcePatch, Matrix & sparseMatrix) {

    Initialize(sourcePatch);
    Convert(sparseMatrix);
}

template <typename REAL>
void
GregoryTriConverter<REAL>::Initialize(SourcePatch const & sourcePatch) {

    //
    //  Allocate and gather the 1-rings for the corner vertices and other
    //  topological information for more immediate access:
    //
    int width = sourcePatch.GetNumSourcePoints();
    _numSourcePoints = width;
    _maxValence      = sourcePatch.GetMaxValence();

    int boundaryCount = 0;
    int irregularCount = 0;
    int irregularCorner = -1;
    int irregularValence = -1;
    int sharpCount = 0;
    int val2IntCount = 0;

    for (int cIndex = 0; cIndex < 3; ++cIndex) {
        SourcePatch::Corner srcCorner = sourcePatch._corners[cIndex];

        CornerTopology& corner = _corners[cIndex];

        corner.isBoundary = srcCorner._boundary;
        corner.isSharp    = srcCorner._sharp;
        corner.isDart     = srcCorner._dart;
        corner.isCorner   = (srcCorner._numFaces == 1);
        corner.numFaces   = srcCorner._numFaces;
        corner.faceInRing = srcCorner._patchFace;
        corner.isVal2Int  = srcCorner._val2Interior;
        corner.valence    = corner.numFaces + corner.isBoundary;

        corner.isRegular = ((corner.numFaces << corner.isBoundary) == 6)
                         && !corner.isSharp;
        if (corner.isRegular) {
            corner.faceAngle = (REAL)(kPI / 3.0);
            corner.cosFaceAngle = 0.5f;
        } else {
            corner.faceAngle =
                (corner.isBoundary ? REAL(kPI) : REAL(2.0 * kPI))
                    / REAL(corner.numFaces);
            corner.cosFaceAngle = std::cos(corner.faceAngle);
        }

        corner.ringPoints.SetSize(sourcePatch.GetCornerRingSize(cIndex));
        sourcePatch.GetCornerRingPoints(cIndex, corner.ringPoints);

        //  Accumulate topology information to categorize the patch as a whole:
        boundaryCount += corner.isBoundary;
        if (!corner.isRegular) {
            irregularCount ++;
            irregularCorner = cIndex;
            irregularValence = corner.valence;
        }
        sharpCount += corner.isSharp;
        val2IntCount += corner.isVal2Int;
    }

    //  Make a second pass to assign tags dependent on adjacent corners
    for (int cIndex = 0; cIndex < 3; ++cIndex) {
        CornerTopology& corner = _corners[cIndex];

        int cNext = (cIndex + 1) % 3;
        int cPrev = (cIndex + 2) % 3;

        corner.epOnBoundary = false;
        corner.emOnBoundary = false;

        //
        //  Identify if the face points are regular or shared/copied from
        //  one of the pair:
        //
        corner.fpIsRegular = corner.isRegular && _corners[cNext].isRegular;
        corner.fmIsRegular = corner.isRegular && _corners[cPrev].isRegular;

        corner.fpIsCopied = false;
        corner.fmIsCopied = false;

        if (corner.isBoundary) {
            corner.epOnBoundary = (corner.faceInRing == 0);
            corner.emOnBoundary = (corner.faceInRing == (corner.numFaces - 1));

            //  Both face points are same when one of the two corners' edges
            //  is discontinuous -- one is then copied from the other (unless
            //  regular)
            if (corner.numFaces > 1) {
                if (corner.epOnBoundary) {
                    corner.fpIsRegular = corner.fmIsRegular;
                    corner.fpIsCopied  = !corner.fpIsRegular;
                }
                if (corner.emOnBoundary) {
                    corner.fmIsRegular = corner.fpIsRegular;
                    corner.fmIsCopied  = !corner.fmIsRegular;
                }
            } else {
                //  The case of a corner patch is always regular
                corner.fpIsRegular = true;
                corner.fmIsRegular = true;
            }
        }
    }
    _isIsolatedInteriorPatch = (irregularCount == 1) && (boundaryCount == 0) &&
                               (irregularValence > 2) && (sharpCount == 0);
    if (_isIsolatedInteriorPatch) {
        _isolatedCorner  = irregularCorner;
        _isolatedValence = irregularValence;
    }
    _hasVal2InteriorCorner = (val2IntCount > 0);
}

template <typename REAL>
void
GregoryTriConverter<REAL>::Convert(Matrix & matrix) const {

    //
    //  Initialize the sparse matrix to accomodate the coefficients for each
    //  row/point -- identify common topological cases to treat more easily
    //  (and note that specializing the population of the matrix may also be
    //  worthwhile in such cases)
    //
    if (_isIsolatedInteriorPatch) {
        resizeMatrixIsolatedIrregular(matrix, _isolatedCorner, _isolatedValence);
    } else {
        resizeMatrixUnisolated(matrix);
    }

    //
    //  Compute the corner and edge points P, Ep and Em first.  Since face
    //  points Fp and Fm involve edge points for two adjacent corners, their
    //  computation must follow:
    //
    int maxRingSize      = 1 + _maxValence;
    int weightBufferSize = std::max(3 * maxRingSize, 2 * _numSourcePoints);

    StackBuffer<Weight, 128, true> weightBuffer(weightBufferSize);
    StackBuffer<int, 128, true>    indexBuffer(weightBufferSize);

    for (int cIndex = 0; cIndex < 3; ++cIndex) {
        if (_corners[cIndex].isRegular) {
            assignRegularEdgePoints(cIndex, matrix);
        } else {
            computeIrregularEdgePoints(cIndex, matrix, weightBuffer);
        }
    }

    for (int cIndex = 0; cIndex < 3; ++cIndex) {
        if (_corners[cIndex].fpIsRegular || _corners[cIndex].fmIsRegular) {
            assignRegularFacePoints(cIndex, matrix);
        }
        if (!_corners[cIndex].fpIsRegular || !_corners[cIndex].fmIsRegular) {
            computeIrregularFacePoints(cIndex, matrix, weightBuffer, indexBuffer);
        }
    }

    for (int eIndex = 0; eIndex < 3; ++eIndex) {
        CornerTopology const & c0 = _corners[eIndex];
        CornerTopology const & c1 = _corners[(eIndex + 1) % 3];

        bool isBoundaryEdge = c0.epOnBoundary && c1.emOnBoundary;
        bool isDartEdge     = c0.epOnBoundary != c1.emOnBoundary;
        if (isBoundaryEdge || (c0.isRegular && c1.isRegular && !isDartEdge)) {
            assignRegularMidEdgePoint(eIndex, matrix);
        } else {
            computeIrregularMidEdgePoint(eIndex, matrix, weightBuffer, indexBuffer);
        }
    }
    promoteCubicEdgePointsToQuartic(matrix, weightBuffer, indexBuffer);

    if (_hasVal2InteriorCorner) {
        _removeValence2Duplicates(matrix);
    }
}

template <typename REAL>
void
GregoryTriConverter<REAL>::resizeMatrixIsolatedIrregular(
        Matrix & matrix, int cornerIndex, int cornerValence) const {

    int irregRingSize = 1 + cornerValence;

    int irregCorner   =  cornerIndex;
    int irregPlus     = (cornerIndex + 1) % 3;
    int irregMinus    = (cornerIndex + 2) % 3;

    int   rowSizes[18];
    int * rowSizePtr = 0;

    rowSizePtr = rowSizes + irregCorner * 5;
    *rowSizePtr++ = irregRingSize;
    *rowSizePtr++ = irregRingSize;
    *rowSizePtr++ = irregRingSize;
    *rowSizePtr++ = 3 + irregRingSize;
    *rowSizePtr++ = 3 + irregRingSize;

    rowSizePtr = rowSizes + irregPlus * 5;
    *rowSizePtr++ = 7;
    *rowSizePtr++ = 7;
    *rowSizePtr++ = 7;
    *rowSizePtr++ = 5;
    *rowSizePtr++ = 3 + irregRingSize;

    rowSizePtr = rowSizes + irregMinus * 5;
    *rowSizePtr++ = 7;
    *rowSizePtr++ = 7;
    *rowSizePtr++ = 7;
    *rowSizePtr++ = 3 + irregRingSize;
    *rowSizePtr++ = 5;

    //  The 3 quartic mid-edge points are not grouped with corners:
    rowSizes[15 + irregCorner] = 3 + irregRingSize;
    rowSizes[15 + irregPlus]   = 4;
    rowSizes[15 + irregMinus]  = 3 + irregRingSize;

    int numElements = 9*irregRingSize + 74;

    _resizeMatrix(matrix, 18, _numSourcePoints, numElements, rowSizes);
}

template <typename REAL>
void
GregoryTriConverter<REAL>::resizeMatrixUnisolated(Matrix & matrix) const {

    int rowSizes[18];

    int numElements = 0;

    for (int cIndex = 0; cIndex < 3; ++cIndex) {
        int * rowSize = rowSizes + cIndex*5;

        CornerTopology const & corner = _corners[cIndex];

        //  First, the corner and pair of edge points:
        if (corner.isRegular) {
            if (! corner.isBoundary) {
                rowSize[0] = 7;
                rowSize[1] = 7;
                rowSize[2] = 7;
            } else {
                rowSize[0] = 3;
                rowSize[1] = corner.epOnBoundary ? 3 : 5;
                rowSize[2] = corner.emOnBoundary ? 3 : 5;
            }
        } else {
            if (corner.isSharp) {
                rowSize[0] = 1;
                rowSize[1] = 2;
                rowSize[2] = 2;
            } else if (! corner.isBoundary) {
                int ringSize = 1 + corner.valence;
                rowSize[0] = ringSize;
                rowSize[1] = ringSize;
                rowSize[2] = ringSize;
            } else if (corner.numFaces > 1) {
                int ringSize = 1 + corner.valence;
                rowSize[0] = 3;
                rowSize[1] = corner.epOnBoundary ? 3 : ringSize;
                rowSize[2] = corner.emOnBoundary ? 3 : ringSize;
            } else {
                rowSize[0] = 3;
                rowSize[1] = 3;
                rowSize[2] = 3;
            }
        }
        numElements += rowSize[0] + rowSize[1] + rowSize[2];

        //  Second, the pair of face points:
        rowSize[3] = 5 - corner.epOnBoundary - corner.emOnBoundary;
        rowSize[4] = 5 - corner.epOnBoundary - corner.emOnBoundary;
        if (!corner.fpIsRegular || !corner.fmIsRegular) {
            int cNext = (cIndex + 1) % 3;
            int cPrev = (cIndex + 2) % 3;
            if (!corner.fpIsRegular) {
                rowSize[3] = getIrregularFacePointSize(cIndex,
                                corner.fpIsCopied ? cPrev : cNext);
            }
            if (!corner.fmIsRegular) {
                rowSize[4] = getIrregularFacePointSize(cIndex,
                                corner.fmIsCopied ? cNext : cPrev);
            }
        }
        numElements += rowSize[3] + rowSize[4];

        //  Third, the quartic mid-edge boundary point (edge following corner):
        int cNext = (cIndex + 1) % 3;
        CornerTopology const & cornerNext = _corners[cNext];

        int & midEdgeSize = rowSizes[15 + cIndex];

        if (corner.epOnBoundary && cornerNext.emOnBoundary) {
            midEdgeSize = 2;
        } else if (corner.isRegular && cornerNext.isRegular &&
                  (corner.epOnBoundary == cornerNext.emOnBoundary)) {
            midEdgeSize = 4;
        } else {
            midEdgeSize = getIrregularFacePointSize(cIndex, cNext);
        }
        numElements += midEdgeSize;
    }
    _resizeMatrix(matrix, 18, _numSourcePoints, numElements, rowSizes);
}

template <typename REAL>
void
GregoryTriConverter<REAL>::assignRegularEdgePoints(int cIndex, Matrix & matrix) const {

    Point p (matrix, 5*cIndex + 0);
    Point ep(matrix, 5*cIndex + 1);
    Point em(matrix, 5*cIndex + 2);

    CornerTopology const & corner = _corners[cIndex];

    int const * cRing = corner.ringPoints;

    if (! corner.isBoundary) {
        REAL const pScale = (REAL) (1.0 / 12.0);

        p.Assign(0, cIndex, 0.5f);
        p.Assign(1, cRing[0], pScale);
        p.Assign(2, cRing[1], pScale);
        p.Assign(3, cRing[2], pScale);
        p.Assign(4, cRing[3], pScale);
        p.Assign(5, cRing[4], pScale);
        p.Assign(6, cRing[5], pScale);
        assert(p.GetSize() == 7);

        //  Identify the edges along Ep and Em and those opposite them:
        REAL const eWeights[6] = { 7.0f, 5.0f, 1.0f, -1.0f, 1.0f, 5.0f };
        REAL const eScale = (REAL) (1.0 / 36.0);

        int iEdgeEp =  corner.faceInRing;
        int iEdgeEm = (corner.faceInRing + 1) % 6;

        ep.Assign(0, cIndex, 0.5f);
        ep.Assign(1, cRing[iEdgeEp],           eScale * eWeights[0]);
        ep.Assign(2, cRing[(iEdgeEp + 1) % 6], eScale * eWeights[1]);
        ep.Assign(3, cRing[(iEdgeEp + 2) % 6], eScale * eWeights[2]);
        ep.Assign(4, cRing[(iEdgeEp + 3) % 6], eScale * eWeights[3]);
        ep.Assign(5, cRing[(iEdgeEp + 4) % 6], eScale * eWeights[4]);
        ep.Assign(6, cRing[(iEdgeEp + 5) % 6], eScale * eWeights[5]);
        assert(ep.GetSize() == 7);

        em.Assign(0, cIndex, 0.5f);
        em.Assign(1, cRing[iEdgeEm],           eScale * eWeights[0]);
        em.Assign(2, cRing[(iEdgeEm + 1) % 6], eScale * eWeights[1]);
        em.Assign(3, cRing[(iEdgeEm + 2) % 6], eScale * eWeights[2]);
        em.Assign(4, cRing[(iEdgeEm + 3) % 6], eScale * eWeights[3]);
        em.Assign(5, cRing[(iEdgeEm + 4) % 6], eScale * eWeights[4]);
        em.Assign(6, cRing[(iEdgeEm + 5) % 6], eScale * eWeights[5]);
        assert(em.GetSize() == 7);
    } else {
        REAL oneThird  = (REAL) (1.0 / 3.0);
        REAL twoThirds = (REAL) (2.0 / 3.0);
        REAL oneSixth  = (REAL) (1.0 / 6.0);

        p.Assign(0, cIndex,   twoThirds);
        p.Assign(1, cRing[0], oneSixth);
        p.Assign(2, cRing[3], oneSixth);
        assert(p.GetSize() == 3);

        //
        //  We have three triangles here, and the two edge points may be along two
        //  of four edges -- two of which are interior and require weights adjusted
        //  from above to account for phantom points (yielding {1/2, 1/6, 1/6, 1/6})
        //
        if (corner.epOnBoundary) {
            ep.Assign(0, cIndex,   twoThirds);
            ep.Assign(1, cRing[0], oneThird);
            ep.Assign(2, cRing[3], 0.0f);
            assert(ep.GetSize() == 3);
        } else {
            ep.Assign(0, cIndex, 0.5f);
            ep.Assign(1, cRing[1], oneSixth);
            ep.Assign(2, cRing[2], oneSixth);
            ep.Assign(3, cRing[corner.emOnBoundary ? 3 : 0], oneSixth);
            ep.Assign(4, cRing[corner.emOnBoundary ? 0 : 3], 0.0f);
            assert(ep.GetSize() == 5);
        }

        if (corner.emOnBoundary) {
            em.Assign(0, cIndex,   twoThirds);
            em.Assign(1, cRing[3], oneThird);
            em.Assign(2, cRing[0], 0.0f);
            assert(em.GetSize() == 3);
        } else {
            em.Assign(0, cIndex, 0.5f);
            em.Assign(1, cRing[1], oneSixth);
            em.Assign(2, cRing[2], oneSixth);
            em.Assign(3, cRing[corner.epOnBoundary ? 0 : 3], oneSixth);
            em.Assign(4, cRing[corner.epOnBoundary ? 3 : 0], 0.0f);
            assert(em.GetSize() == 5);
        }
    }
}

template <typename REAL>
void
GregoryTriConverter<REAL>::computeIrregularEdgePoints(int cIndex,
        Matrix & matrix, Weight *weightBuffer) const {

    Point p (matrix, 5*cIndex + 0);
    Point ep(matrix, 5*cIndex + 1);
    Point em(matrix, 5*cIndex + 2);

    //
    //  The corner and edge points P, Ep and Em  are completely determined
    //  by the 1-ring of vertices around (and including) the corner vertex.
    //  We combine full sets of coefficients for the vertex and its 1-ring.
    //
    CornerTopology const & corner = _corners[cIndex];

    if (corner.isSharp) {
        //
        //  The sharp case -- both interior and boundary...
        //
        p.Assign(0, cIndex, 1.0f);
        assert(p.GetSize() == 1);

        // Approximating these for now, pending future investigation...
        ep.Assign(0, cIndex,         (REAL) (2.0 / 3.0));
        ep.Assign(1, (cIndex+1) % 3, (REAL) (1.0 / 3.0));
        assert(ep.GetSize() == 2);

        em.Assign(0, cIndex,         (REAL) (2.0 / 3.0));
        em.Assign(1, (cIndex+2) % 3, (REAL) (1.0 / 3.0));
        assert(em.GetSize() == 2);
    } else if (! corner.isBoundary) {
        //
        //  The irregular interior case:
        //
        computeIrregularInteriorEdgePoints(cIndex, p, ep, em, weightBuffer);
    } else if (corner.numFaces > 1) {
        //
        //  The irregular boundary case:
        //
        computeIrregularBoundaryEdgePoints(cIndex, p, ep, em, weightBuffer);
    } else {
        //
        //  The irregular/smooth corner case:
        //
        p.Assign(0, cIndex,         (REAL) (4.0 / 6.0));
        p.Assign(1, (cIndex+1) % 3, (REAL) (1.0 / 6.0));
        p.Assign(2, (cIndex+2) % 3, (REAL) (1.0 / 6.0));
        assert(p.GetSize() == 3);

        ep.Assign(0, cIndex,         (REAL) (2.0 / 3.0));
        ep.Assign(1, (cIndex+1) % 3, (REAL) (1.0 / 3.0));
        ep.Assign(2, (cIndex+2) % 3, 0.0f);
        assert(ep.GetSize() == 3);

        em.Assign(0, cIndex,         (REAL) (2.0 / 3.0));
        em.Assign(1, (cIndex+2) % 3, (REAL) (1.0 / 3.0));
        em.Assign(2, (cIndex+1) % 3, 0.0f);
        assert(em.GetSize() == 3);
    }
}


template <typename REAL>
void
GregoryTriConverter<REAL>::computeIrregularInteriorEdgePoints(
        int cIndex,
        Point& p, Point& ep, Point& em,
        Weight *ringWeights) const {

    CornerTopology const & corner = _corners[cIndex];

    int valence = corner.valence;
    int weightWidth = 1 + valence;

    Weight* pWeights  = &ringWeights[0];
    Weight* epWeights = pWeights + weightWidth;
    Weight* emWeights = pWeights + weightWidth * 2;

    //
    //  The interior (smooth) case -- invoke the public static method that
    //  computes pre-allocated ring weights for P, Ep and Em:
    //
    LoopLimits<REAL>::ComputeInteriorPointWeights(valence, corner.faceInRing,
            pWeights, epWeights, emWeights);

    //
    //  Transer the weights for the ring into the stencil form of the required
    //  Point type.  The limit mask for position involves all ring weights, and
    //  since Ep and Em depend on it, there should be no need to filter weights
    //  with value 0:
    //
    p.Assign( 0, cIndex, pWeights[0]);
    ep.Assign(0, cIndex, epWeights[0]);
    em.Assign(0, cIndex, emWeights[0]);

    for (int i = 1; i < weightWidth; ++i) {
        int pRingPoint = corner.ringPoints[i-1];

        p.Assign( i, pRingPoint, pWeights[i]);
        ep.Assign(i, pRingPoint, epWeights[i]);
        em.Assign(i, pRingPoint, emWeights[i]);
    }
    assert(p.GetSize() == weightWidth);
    assert(ep.GetSize() == weightWidth);
    assert(em.GetSize() == weightWidth);
}


template <typename REAL>
void
GregoryTriConverter<REAL>::computeIrregularBoundaryEdgePoints(
        int cIndex,
        Point& p, Point& ep, Point& em,
        Weight *ringWeights) const {

    CornerTopology const & corner = _corners[cIndex];

    int valence = corner.valence;
    int weightWidth = 1 + corner.valence;

    Weight* pWeights  = &ringWeights[0];
    Weight* epWeights = pWeights + weightWidth;
    Weight* emWeights = pWeights + weightWidth * 2;

    //
    //  The boundary (smooth) case -- invoke the public static method that
    //  computes pre-allocated ring weights for P, Ep and Em:
    //
    LoopLimits<REAL>::ComputeBoundaryPointWeights(valence, corner.faceInRing,
                pWeights, epWeights, emWeights);

    //
    //  Transfer ring weights into points -- exploiting cases where they
    //  are known to be non-zero only along the two boundary edges:
    //
    int N = weightWidth - 1;

    int p0 = cIndex;
    int p1 = corner.ringPoints[0];
    int pN = corner.ringPoints[valence-1];

    p.Assign(0, p0, pWeights[0]);
    p.Assign(1, p1, pWeights[1]);
    p.Assign(2, pN, pWeights[N]);
    assert(p.GetSize() == 3);

    //  If Ep is on the boundary edge, it has only two non-zero weights along
    //  that edge:
    ep.Assign(0, p0, epWeights[0]);
    if (corner.epOnBoundary) {
        ep.Assign(1, p1, epWeights[1]);
        ep.Assign(2, pN, 0.0f);
        assert(ep.GetSize() == 3);
    } else {
        for (int i = 1; i < weightWidth; ++i) {
            ep.Assign(i, corner.ringPoints[i-1], epWeights[i]);
        }
        assert(ep.GetSize() == weightWidth);
    }

    //  If Em is on the boundary edge, it has only two non-zero weights along
    //  that edge:
    em.Assign(0, p0, emWeights[0]);
    if (corner.emOnBoundary) {
        em.Assign(1, pN, emWeights[N]);
        em.Assign(2, p1, 0.0f);
        assert(em.GetSize() == 3);
    } else {
        for (int i = 1; i < weightWidth; ++i) {
            em.Assign(i, corner.ringPoints[i-1], emWeights[i]);
        }
        assert(em.GetSize() == weightWidth);
    }
}


template <typename REAL>
int
GregoryTriConverter<REAL>::getIrregularFacePointSize(
        int cIndexNear, int cIndexFar) const {

    CornerTopology const & nearCorner = _corners[cIndexNear];
    CornerTopology const & farCorner  = _corners[cIndexFar];

    if (nearCorner.isSharp && farCorner.isSharp) return 2;

    int nearSize = nearCorner.ringPoints.GetSize() - 3;
    int farSize  = farCorner.ringPoints.GetSize() - 3;

    return 4 + (((nearSize > 0) && !nearCorner.isSharp) ? nearSize : 0)
             + (((farSize > 0)  && !farCorner.isSharp)  ? farSize  : 0);
}

template <typename REAL>
void
GregoryTriConverter<REAL>::computeIrregularFacePoint(
        int cIndexNear, int edgeInNearCornerRing, int cIndexFar,
        Point const & p, Point const & eNear, Point const & eFar, Point & fNear,
        REAL signForSideOfEdge, Weight *rowWeights, int *columnMask) const {

    CornerTopology const & cornerNear = _corners[cIndexNear];
    CornerTopology const & cornerFar  = _corners[cIndexFar];

    int valence = cornerNear.valence;

    Weight cosNear = cornerNear.cosFaceAngle;
    Weight cosFar  = cornerFar.cosFaceAngle;

    //
    //  From Loop, Schaefer et al, a face point F is computed as:
    //
    //    F = (1/d) * (c0 * P0 + (d - 2*c0 - c1) * E0 + 2*c1 * E1 + R)
    //
    //  where d = 3 for quads and d = 4 for triangles, and R is:
    //
    //    R = 1/3 (Mm + Mp) + 2/3 (Cm + Cp)
    //
    //  where Mm and Mp are the midpoints of the two adjacent edges
    //  and Cm and Cp are the midpoints of the two adjacent faces.
    //
    Weight pCoeff     =                          cosFar  / 4.0f;
    Weight eNearCoeff = (4.0f - 2.0f * cosNear - cosFar) / 4.0f;
    Weight eFarCoeff  =         2.0f * cosNear           / 4.0f;

    int fullRowSize = _numSourcePoints;
    std::memset(&columnMask[0], 0, fullRowSize * sizeof(int));
    std::memset(&rowWeights[0], 0, fullRowSize * sizeof(Weight));

    _addSparsePointToFullRow(rowWeights, p,     pCoeff,     columnMask);
    _addSparsePointToFullRow(rowWeights, eNear, eNearCoeff, columnMask);
    _addSparsePointToFullRow(rowWeights, eFar,  eFarCoeff,  columnMask);

    //  Remember that R is to be computed about an interior edge and is
    //  comprised of the two pairs of points opposite the interior edge
    //
    int iEdgeInterior = edgeInNearCornerRing;
    int iEdgePrev     = (iEdgeInterior + valence - 1) % valence;
    int iEdgeNext     = (iEdgeInterior + 1) % valence;

    Weight rScale = (REAL) (0.25 * (7.0 / 18.0));

    rowWeights[cornerNear.ringPoints[iEdgePrev]] += -signForSideOfEdge * rScale;
    rowWeights[cornerNear.ringPoints[iEdgeNext]] +=  signForSideOfEdge * rScale;

    int nWeights = 0;
    for (int i = 0; i < fullRowSize; ++i) {
        if (columnMask[i]) {
            fNear.Assign(nWeights++, columnMask[i] - 1, rowWeights[i]);
        }
    }

    //  Complete the expected row size when val-2 corners induce duplicates:
    if (_hasVal2InteriorCorner && (nWeights < fNear.GetSize())) {
        while (nWeights < fNear.GetSize()) {
            fNear.Assign(nWeights++, cIndexNear, 0.0f);
        }
    }
    assert(fNear.GetSize() == nWeights);
}

template <typename REAL>
void
GregoryTriConverter<REAL>::assignRegularFacePoints(int cIndex, Matrix & matrix) const {

    CornerTopology const & corner = _corners[cIndex];

    int cNext = (cIndex+1) % 3;
    int cPrev = (cIndex+2) % 3;

    int const * cRing = corner.ringPoints;

    //
    //  Regular face-points are computed the same for both face-points of a
    //  a corner (fp and fm), so iterate through both and make appropriate
    //  assignments when tagged as regular:
    //
    for (int fIsFm = 0; fIsFm < 2; ++fIsFm) {
        bool fIsRegular = fIsFm ? corner.fmIsRegular : corner.fpIsRegular;
        if (!fIsRegular) continue;

        Point f(matrix, 5*cIndex + 3 + fIsFm);

        if (corner.isCorner) {
            f.Assign(0, cIndex, 0.5f);
            f.Assign(1, cNext,  0.25f);
            f.Assign(2, cPrev,  0.25f);
            assert(f.GetSize() == 3);
        } else if (corner.epOnBoundary) {
            //  Face is the first/leading face of the boundary ring:
            f.Assign(0, cIndex,   (REAL) (11.0 / 24.0));
            f.Assign(1, cRing[0], (REAL) ( 7.0 / 24.0));
            f.Assign(2, cRing[1], (REAL) ( 5.0 / 24.0));
            f.Assign(3, cRing[2], (REAL) ( 1.0 / 24.0));
            assert(f.GetSize() == 4);
        } else if (corner.emOnBoundary) {
            //  Face is the last/trailing face of the boundary ring:
            f.Assign(0, cIndex,   (REAL) (11.0 / 24.0));
            f.Assign(1, cRing[3], (REAL) ( 7.0 / 24.0));
            f.Assign(2, cRing[2], (REAL) ( 5.0 / 24.0));
            f.Assign(3, cRing[1], (REAL) ( 1.0 / 24.0));
            assert(f.GetSize() == 4);
        } else {
            //  Face is interior or the middle face of the boundary:
            int eNext = corner.isBoundary ? 0 : ((corner.faceInRing + 5) % 6);
            int ePrev = corner.isBoundary ? 3 : ((corner.faceInRing + 2) % 6);

            f.Assign(0, cIndex,       (REAL) (10.0 / 24.0));
            f.Assign(1, cPrev,                 0.25f);
            f.Assign(2, cNext,                 0.25f);
            f.Assign(3, cRing[ePrev], (REAL) ( 1.0 / 24.0));
            f.Assign(4, cRing[eNext], (REAL) ( 1.0 / 24.0));
            assert(f.GetSize() == 5);
        }
    }
}

template <typename REAL>
void
GregoryTriConverter<REAL>::computeIrregularFacePoints(int cIndex,
        Matrix & matrix, Weight *rowWeights, int *columnMask) const {

    //  Identify neighboring corners:
    CornerTopology const & corner = _corners[cIndex];

    int cNext = (cIndex+1) % 3;
    int cPrev = (cIndex+2) % 3;

    Point epPrev(matrix, 5*cPrev  + 1);
    Point em    (matrix, 5*cIndex + 2);
    Point p     (matrix, 5*cIndex + 0);
    Point ep    (matrix, 5*cIndex + 1);
    Point emNext(matrix, 5*cNext  + 2);

    Point fp(matrix, 5*cIndex + 3);
    Point fm(matrix, 5*cIndex + 4);

    //
    //  Compute the face points Fp and Fm in terms of the corner (P) and edge
    //  points (Ep and Em) previously computed.  The caller provides a buffer
    //  of the appropriate size (twice the width of the matrix) to use for
    //  combining weights, along with an integer buffer used to identify
    //  non-zero weights and preserve the sparsity of the combinations (note
    //  they use index + 1 to detect index 0 when cleared with 0 entries).
    //
    if (!corner.fpIsRegular && !corner.fpIsCopied) {
        int iEdgeP = corner.faceInRing;
        computeIrregularFacePoint(cIndex, iEdgeP, cNext,
                p, ep, emNext, fp, 1.0, rowWeights, columnMask);
    }
    if (!corner.fmIsRegular && !corner.fmIsCopied) {
        int iEdgeM = (corner.faceInRing + 1) % corner.valence;
        computeIrregularFacePoint(cIndex, iEdgeM, cPrev,
                p, em, epPrev, fm, -1.0, rowWeights, columnMask);
    }

    //  Copy Fp or Fm now that any shared values were computed above:
    if (corner.fpIsCopied) {
        fp.Copy(fm);
    }
    if (corner.fmIsCopied) {
        fm.Copy(fp);
    }

    if (!corner.fpIsRegular) assert(matrix.GetRowSize(5*cIndex + 3) == fp.GetSize());
    if (!corner.fmIsRegular) assert(matrix.GetRowSize(5*cIndex + 4) == fm.GetSize());
}

template <typename REAL>
void
GregoryTriConverter<REAL>::assignRegularMidEdgePoint(int edgeIndex,
                            Matrix & matrix) const {

    Point M(matrix, 15 + edgeIndex);

    CornerTopology const & corner = _corners[edgeIndex];
    if (corner.epOnBoundary) {
        //  Trivial boundary edge case -- midway between two corners

        M.Assign(0,  edgeIndex, 0.5f);
        M.Assign(1, (edgeIndex + 1) % 3, 0.5f);
        assert(M.GetSize() == 2);
    } else {
        //  Regular case -- two corners and two vertices opposite the edge

        int oppositeInRing = corner.isBoundary ?
            (corner.faceInRing - 1) : ((corner.faceInRing + 5) % 6);
        int oppositeVertex = corner.ringPoints[oppositeInRing];

        M.Assign(0,  edgeIndex,          (REAL) (1.0 / 3.0));
        M.Assign(1, (edgeIndex + 1) % 3, (REAL) (1.0 / 3.0));
        M.Assign(2, (edgeIndex + 2) % 3, (REAL) (1.0 / 6.0));
        M.Assign(3,  oppositeVertex,     (REAL) (1.0 / 6.0));
        assert(M.GetSize() == 4);
    }
}

template <typename REAL>
void
GregoryTriConverter<REAL>::computeIrregularMidEdgePoint(int edgeIndex, Matrix & matrix,
                                Weight * rowWeights, int * columnMask) const {
    //
    //  General case -- interpolate midway between cubic edge points E0 and E1:
    //
    int cIndex0 =  edgeIndex;
    int cIndex1 = (edgeIndex + 1) % 3;

    Point E0p(matrix, 5 * (cIndex0) + 1);
    Point E1m(matrix, 5 * (cIndex1) + 2);

    Point M(matrix, 15 + edgeIndex);

    _combineSparsePointsInFullRow(M, (REAL)0.5f, E0p, (REAL)0.5f, E1m,
                                  _numSourcePoints, rowWeights, columnMask);
}

template <typename REAL>
void
GregoryTriConverter<REAL>::promoteCubicEdgePointsToQuartic(Matrix & matrix,
                                Weight * rowWeights, int * columnMask) const {
    //
    //  Re-assign all regular edge-point weights with quartic coefficients,
    //  so only perform general combinations for the irregular case.
    //
    REAL const onBoundaryWeights[3]  = { 16, 7, 1 };
    REAL const regBoundaryWeights[5] = { 13, 3, 3, 4, 1 };
    REAL const regInteriorWeights[7] = { 12, 4, 3, 1,  0, 1, 3 };

    REAL const oneOver24 = (REAL) (1.0 / 24.0);

    for (int cIndex = 0; cIndex < 3; ++cIndex) {
        CornerTopology const & corner = _corners[cIndex];

        //
        //  Ordering of weight values for symmetric ep and em is the same, so
        //  we can re-assign in a loop of 2 for {ep, em}
        //
        Point P(matrix, 5 * cIndex);

        for (int ePair = 0; ePair < 2; ++ePair) {
            Point E(matrix, 5 * cIndex + 1 + ePair);

            REAL const * weightsToReassign = 0;

            bool eOnBoundary = ePair ? corner.emOnBoundary : corner.epOnBoundary;
            if (eOnBoundary && !corner.isSharp) {
                assert(E.GetSize() == 3);
                weightsToReassign = onBoundaryWeights;
            } else if (corner.isRegular) {
                if (corner.isBoundary) {
                    assert(E.GetSize() == 5);
                    weightsToReassign = regBoundaryWeights;
                } else {
                    assert(E.GetSize() == 7);
                    weightsToReassign = regInteriorWeights;
                }
            }
            if (weightsToReassign) {
                for (int i = 0; i < E.GetSize(); ++i) {
                    E.SetWeight(i, weightsToReassign[i] * oneOver24);
                }
            } else {
                _combineSparsePointsInFullRow(E, (REAL)0.25f, P, (REAL)0.75f, E,
                                              _numSourcePoints, rowWeights, columnMask);
            }
        }
    }
}

namespace {
    template <typename REAL>
    void
    _printPoint(SparseMatrixRow<REAL> const & p, bool printIndices = true,
                                                 bool printWeights = true) {
        printf("  Point size = %d:\n", p._size);
        if (printIndices) {
            printf("    Indices:  ");
            for (int j = 0; j < p._size; ++j) {
                printf("%6d ", p._indices[j]);
            }
            printf("\n");
        }
        if (printWeights) {
            printf("    Weights:  ");
            for (int j = 0; j < p._size; ++j) {
                printf("%6.3f ", (REAL)p._weights[j]);
            }
            printf("\n");
        }
    }

    template <typename REAL>
    void
    _printMatrix(SparseMatrix<REAL> const & matrix, bool printIndices = true,
                                                    bool printWeights = true) {

        printf("Matrix %d x %d, %d elements:\n",
            matrix.GetNumRows(), matrix.GetNumColumns(), matrix.GetNumElements());

        for (int i = 0; i < matrix.GetNumRows(); ++i) {
            int rowSize = matrix.GetRowSize(i);
            printf("  Row %d (size = %d):\n", i, rowSize);

            if (printIndices) {
                ConstArray<int> indices = matrix.GetRowColumns(i);
                printf("    Indices:  ");
                for (int j = 0; j < rowSize; ++j) {
                    printf("%6d ", indices[j]);
                }
                printf("\n");
            }
            if (printWeights) {
                ConstArray<REAL> weights = matrix.GetRowElements(i);
                printf("    Weights:  ");
                for (int j = 0; j < rowSize; ++j) {
                    printf("%6.3f ", (REAL)weights[j]);
                }
                printf("\n");
            }
        }
    }

#ifdef FAR_DEBUG_LOOP_PATCH_BUILDER
    void
    _printSourcePatch(SourcePatch const & patch, bool printCornerInfo = true,
                                                 bool printRingPoints = true) {

        printf("SoucePatch - %d corners, %d points:\n",
            patch._numCorners, patch._numSourcePoints);

        if (printCornerInfo) {
            printf("  Corner info:\n");
            for (int i = 0; i < patch._numCorners; ++i) {
                printf("%6d:  boundary = %d, sharp = %d, numFaces = %d, in-ring = %d, ringSize = %d\n", i,
                    patch._corners[i]._boundary, patch._corners[i]._sharp, patch._corners[i]._numFaces,
                    patch._corners[i]._patchFace, patch._ringSizes[i]);
            }
        }
        if (printRingPoints) {
            StackBuffer<Index,64,true> ringPoints;
            printf("  Ring points:\n");
            for (int i = 0; i < patch._numCorners; ++i) {
                int ringSize = patch._ringSizes[i];

                ringPoints.SetSize(ringSize);
                patch.GetCornerRingPoints(i, ringPoints);

                printf("%6d:  ", i);
                for (int j = 0; j < ringSize; ++j) {
                    printf("%d ", ringPoints[j]);
                }
                printf("\n");
            }
        }
    }
#endif
}


//
//  Not using the same "Converter" pattern used above and in the Catmark scheme,
//  since the two remaining conversions are fairly trivial.
//
template <typename REAL>
void
convertToLinear(SourcePatch const & sourcePatch, SparseMatrix<REAL> & matrix) {

    typedef REAL Weight;

    StackBuffer<Index,64,true>  indexBuffer(1 + sourcePatch.GetMaxRingSize());
    StackBuffer<Weight,64,true> weightBuffer(1 + sourcePatch.GetMaxRingSize());

    int numElements = sourcePatch.GetCornerRingSize(0) +
                      sourcePatch.GetCornerRingSize(1) +
                      sourcePatch.GetCornerRingSize(2);

    matrix.Resize(3, sourcePatch.GetNumSourcePoints(), numElements);

    bool hasVal2InteriorCorner = false;

    for (int cIndex = 0; cIndex < 3; ++cIndex) {
        SourcePatch::Corner const & sourceCorner = sourcePatch._corners[cIndex];

        int ringSize = sourcePatch.GetCornerRingSize(cIndex);
        if (sourceCorner._sharp) {
            matrix.SetRowSize(cIndex, 1);
        } else if (sourceCorner._boundary) {
            matrix.SetRowSize(cIndex, 3);
        } else {
            matrix.SetRowSize(cIndex, 1 + ringSize);
        }

        Array<Index>  rowIndices = matrix.SetRowColumns(cIndex);
        Array<Weight> rowWeights = matrix.SetRowElements(cIndex);

        indexBuffer[0] = cIndex;
        sourcePatch.GetCornerRingPoints(cIndex, &indexBuffer[1]);

        if (sourceCorner._sharp) {
            rowIndices[0] = cIndex;
            rowWeights[0] = 1.0f;
        } else if (sourceCorner._boundary) {
            LoopLimits<REAL>::ComputeBoundaryPointWeights(
                    1 + sourceCorner._numFaces, sourceCorner._patchFace,
                    &weightBuffer[0], 0, 0);

            rowIndices[0] = indexBuffer[0];
            rowIndices[1] = indexBuffer[1];
            rowIndices[2] = indexBuffer[ringSize];

            rowWeights[0] = weightBuffer[0];
            rowWeights[1] = weightBuffer[1];
            rowWeights[2] = weightBuffer[ringSize];
        } else {
            LoopLimits<REAL>::ComputeInteriorPointWeights(
                    sourceCorner._numFaces, sourceCorner._patchFace,
                    &weightBuffer[0], 0, 0);

            std::memcpy(&rowIndices[0], indexBuffer, rowIndices.size() * sizeof(Index));
            std::memcpy(&rowWeights[0], weightBuffer, rowWeights.size() * sizeof(Weight));
        }
        hasVal2InteriorCorner |= sourceCorner._val2Interior;
    }
    if (hasVal2InteriorCorner) {
        _removeValence2Duplicates(matrix);
    }
}

template <typename REAL>
void
convertToGregory(SourcePatch const & sourcePatch, SparseMatrix<REAL> & matrix) {

    GregoryTriConverter<REAL> gregoryConverter(sourcePatch);
    gregoryConverter.Convert(matrix);
}

template <typename REAL>
void
convertToLoop(SourcePatch const & sourcePatch, SparseMatrix<REAL> & matrix) {

    //
    //  Unlike quads, there are not enough degrees of freedom in the regular patch
    //  to enforce interpolation of the limit point and tangent at the corner while
    //  preserving the two adjoining boundary curves.  Since we end up destroying
    //  neighboring conintuity in doing so, we use a fully constructed Gregory
    //  patch here for the isolated corner case as well as the general case.
    //
    //  Unfortunately, the regular patch here -- a quartic Box-spline triangle --
    //  is not as flexible as the BSpline patches for Catmark.  Unlike BSplines
    //  and Bezier patches, the Box-splines do not span the full space of possible
    //  shapes (only 12 control points in a space spanned by 15 polynomials for
    //  the quartic case).  So it is possible to construct shapes with a Gregory
    //  or Bezier triangle that cannot be represented by the Box-spline.
    //
    //  The solution fits a Box-spline patch to the constructed Gregory patch with
    //  a set of constraints.  With quartic boundary curves, 12 constraints on the
    //  boundary curve make this tightly constrained.  Such a set of constraints
    //  is rank deficient (11 instead of 12) so an additional constraint on the
    //  midpoint of the patch is included and a conversion matrix is constructed
    //  from the pseudo-inverse of the 13 constraints.
    //
    //  For the full 12x15 conversion matrix from 15-point quartic Bezier patch
    //  back to a Box spline patch, the matrix rows and columns are ordered
    //  according to control point orientations used elsewhere.  Correllation of
    //  points between the Bezier and Gregory points is as follows:
    //
    //      Q0  Q1  Q2  Q3  Q4  Q5  Q6   Q7  Q8  Q9  Q10   Q11  Q12  Q13  Q14
    //      G0  G1 G15  G7  G5  G2 G3,4 G8,9 G6 G17 G13,14 G16  G11  G12  G10
    //
    //  As with conversion from Gregory to BSpline for Catmark, one of the face
    //  points is chosen as a Bezier point in the conversion rather than combining
    //  the pair (which would avoid slight asymmetric artefacts of the choice).
    //  And given the solution now depends primarily on the boundary, its not
    //  necessary to construct a full Gregory patch with enforced continuity.
    //
    REAL const gregoryToLoopMatrix[12][15] = {
        {  8.214411f,  7.571190f, -7.690082f,  2.237840f, -1.118922f,-16.428828f,  0.666666f,  0.666666f,
                       2.237835f,  6.309870f,  0.666666f, -1.690100f, -0.428812f, -0.428805f,  0.214407f },
        { -0.304687f,  0.609374f,  6.752593f,  0.609374f, -0.304687f,  0.609378f, -3.333333f, -3.333333f,
                       0.609378f, -1.247389f, -3.333333f, -1.247389f,  3.276037f,  3.276037f, -1.638020f },
        { -1.118922f,  2.237840f, -7.690082f,  7.571190f,  8.214411f,  2.237835f,  0.666666f,  0.666666f,
                     -16.428828f, -1.690100f,  0.666666f,  6.309870f, -0.428805f, -0.428812f,  0.214407f },
        {  8.214411f,-16.428828f,  6.309870f, -0.428812f,  0.214407f,  7.571190f,  0.666666f,  0.666666f,
                      -0.428805f, -7.690082f,  0.666666f, -1.690100f,  2.237840f,  2.237835f, -1.118922f },
        { -0.813368f,  1.626735f, -0.773435f, -1.039929f,  0.519965f,  1.626735f,  0.666666f,  0.666666f,
                      -1.039930f, -0.773435f,  0.666666f,  1.226558f, -1.039929f, -1.039930f,  0.519965f },
        {  0.519965f, -1.039929f, -0.773435f,  1.626735f, -0.813368f, -1.039930f,  0.666666f,  0.666666f,
                       1.626735f,  1.226558f,  0.666666f, -0.773435f, -1.039930f, -1.039929f,  0.519965f },
        {  0.214407f, -0.428812f,  6.309870f,-16.428828f,  8.214411f, -0.428805f,  0.666666f,  0.666666f,
                       7.571190f, -1.690100f,  0.666666f, -7.690082f,  2.237835f,  2.237840f, -1.118922f },
        { -0.304687f,  0.609378f, -1.247389f,  3.276037f, -1.638020f,  0.609374f, -3.333333f, -3.333333f,
                       3.276037f,  6.752593f, -3.333333f, -1.247389f,  0.609374f,  0.609378f, -0.304687f },
        {  0.519965f, -1.039930f,  1.226558f, -1.039930f,  0.519965f, -1.039929f,  0.666666f,  0.666666f,
                      -1.039929f, -0.773435f,  0.666666f, -0.773435f,  1.626735f,  1.626735f, -0.813368f },
        { -1.638020f,  3.276037f, -1.247389f,  0.609378f, -0.304687f,  3.276037f, -3.333333f, -3.333333f,
                       0.609374f, -1.247389f, -3.333333f,  6.752593f,  0.609378f,  0.609374f, -0.304687f },
        { -1.118922f,  2.237835f, -1.690100f, -0.428805f,  0.214407f,  2.237840f,  0.666666f,  0.666666f,
                      -0.428812f, -7.690082f,  0.666666f,  6.309870f,  7.571190f,-16.428828f,  8.214411f },
        {  0.214407f, -0.428805f, -1.690100f,  2.237835f, -1.118922f, -0.428812f,  0.666666f,  0.666666f,
                       2.237840f,  6.309870f,  0.666666f, -7.690082f,-16.428828f,  7.571190f,  8.214411f }
    };
    int const gRowIndices[15] = { 0,1,15,7,5, 2,4,8,6, 17,14,16, 11,12, 10 };

    SparseMatrix<REAL> G;
    convertToGregory<REAL>(sourcePatch, G);

    _initializeFullMatrix(matrix, 12, G.GetNumColumns());

    for (int i = 0; i < 12; ++i) {
        REAL const * gRowWeights = gregoryToLoopMatrix[i];
        _combineSparseMatrixRowsInFull(matrix, i, G, 15, gRowIndices, gRowWeights);
    }
}


//
//  Internal utilities more relevant to the LoopPatchBuilder:
//
namespace {
    //
    //  The patch type associated with each basis for Loop -- quickly
    //  indexed from an array.  The patch type here is essentially the
    //  triangle form of each basis.
    //
    PatchDescriptor::Type patchTypeFromBasisArray[] = {
            PatchDescriptor::NON_PATCH,        // undefined
            PatchDescriptor::LOOP,             // regular
            PatchDescriptor::GREGORY_TRIANGLE, // Gregory
            PatchDescriptor::TRIANGLES,        // linear
            PatchDescriptor::NON_PATCH };      // Bezier -- for future use
};

LoopPatchBuilder::LoopPatchBuilder(
    TopologyRefiner const& refiner, Options const& options) :
        PatchBuilder(refiner, options) {

    _regPatchType   = patchTypeFromBasisArray[_options.regBasisType];
    _irregPatchType = (_options.irregBasisType == BASIS_UNSPECIFIED)
                    ? _regPatchType
                    : patchTypeFromBasisArray[_options.irregBasisType];

    _nativePatchType = PatchDescriptor::LOOP;
    _linearPatchType = PatchDescriptor::TRIANGLES;
}

LoopPatchBuilder::~LoopPatchBuilder() {
}

PatchDescriptor::Type
LoopPatchBuilder::patchTypeFromBasis(BasisType basis) const {

    return patchTypeFromBasisArray[(int)basis];
}

template <typename REAL>
int
LoopPatchBuilder::convertSourcePatch(SourcePatch const &   sourcePatch,
                                     PatchDescriptor::Type patchType,
                                     SparseMatrix<REAL> &  matrix) const {

    assert(_schemeType == Sdc::SCHEME_LOOP);

    if (patchType == PatchDescriptor::LOOP) {
        convertToLoop<REAL>(sourcePatch, matrix);
    } else if (patchType == PatchDescriptor::TRIANGLES) {
        convertToLinear<REAL>(sourcePatch, matrix);
    } else if (patchType == PatchDescriptor::GREGORY_TRIANGLE) {
        convertToGregory<REAL>(sourcePatch, matrix);
    } else {
        assert("Unknown or unsupported patch type" == 0);
    }
    return matrix.GetNumRows();
}

int
LoopPatchBuilder::convertToPatchType(SourcePatch const &       sourcePatch,
                                         PatchDescriptor::Type patchType,
                                         SparseMatrix<float> & matrix) const {
    return convertSourcePatch(sourcePatch, patchType, matrix);
}
int
LoopPatchBuilder::convertToPatchType(SourcePatch const &        sourcePatch,
                                         PatchDescriptor::Type  patchType,
                                         SparseMatrix<double> & matrix) const {
    return convertSourcePatch(sourcePatch, patchType, matrix);
}


} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
