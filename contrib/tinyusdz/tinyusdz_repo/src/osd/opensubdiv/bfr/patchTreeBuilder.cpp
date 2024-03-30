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

#include "../bfr/patchTreeBuilder.h"
#include "../far/primvarRefiner.h"
#include "../far/topologyRefiner.h"
#include "../far/topologyDescriptor.h"
#include "../far/patchBuilder.h"
#include "../far/sparseMatrix.h"
#include "../far/ptexIndices.h"
#include "../vtr/stackBuffer.h"

#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {
namespace Bfr {

using Vtr::internal::Level;
using Vtr::internal::StackBuffer;

using Far::TopologyRefiner;
using Far::Index;
using Far::ConstIndexArray;
using Far::SparseMatrix;
using Far::PatchBuilder;
using Far::PatchDescriptor;
using Far::PatchParam;

//
//  Construction initializes some of the main components of the
//  build process (e.g. the Far::PatchBuilder) but defers most of
//  the work to other methods:
//
PatchTreeBuilder::PatchTreeBuilder(TopologyRefiner & faceRefiner,
                                   Options const & options) :
    _patchTree(new PatchTree),
    _faceRefiner(faceRefiner),
    _faceAtRoot(0),
    _patchBuilder(0) {

    //
    //  Adaptive refinement in Far requires smooth level <= sharp level,
    //  with the sharp level taking precedence.  And if attempting to
    //  generate patches at the base level, force at least one level of
    //  refinement when necessary:
    //
    int adaptiveLevelPrimary = options.maxPatchDepthSharp;

    int adaptiveLevelSecondary = options.maxPatchDepthSmooth;
    if (adaptiveLevelSecondary > adaptiveLevelPrimary) {
        adaptiveLevelSecondary = adaptiveLevelPrimary;
    }

    //  If primary is 0, so is secondary -- see if level 1 required:
    if (adaptiveLevelSecondary == 0) {
        if (rootFaceNeedsRefinement()) {
            adaptiveLevelPrimary   = std::max(1, adaptiveLevelPrimary);
            adaptiveLevelSecondary = 1;
        }
    }

    //
    //  Apply adaptive refinement to a local refiner for this face:
    //
    TopologyRefiner::AdaptiveOptions adaptiveOptions(adaptiveLevelPrimary);

    adaptiveOptions.SetSecondaryLevel(adaptiveLevelSecondary);

    adaptiveOptions.useInfSharpPatch     = true;
    adaptiveOptions.useSingleCreasePatch = false;
    adaptiveOptions.considerFVarChannels = false;

    ConstIndexArray baseFaceArray(&_faceAtRoot, 1);

    _faceRefiner.RefineAdaptive(adaptiveOptions, baseFaceArray);

    //
    //  Determine offsets per level (we could eventually include local
    //  points in the levels in which the patch occurs)
    //
    int numLevels = _faceRefiner.GetNumLevels();
    _levelOffsets.resize(1 + numLevels);
    _levelOffsets[0] = 0;
    for (int i = 0; i < numLevels; ++i) {
        _levelOffsets[1 + i] = _levelOffsets[i]
                             + _faceRefiner.GetLevel(i).GetNumVertices();
    }

    //
    //  Create a PatchBuilder for this refiner:
    //
    PatchBuilder::BasisType patchBuilderIrregularBasis;
    if (options.irregularBasis == Options::REGULAR) {
        patchBuilderIrregularBasis = PatchBuilder::BASIS_REGULAR;
    } else if (options.irregularBasis == Options::LINEAR) {
        patchBuilderIrregularBasis = PatchBuilder::BASIS_LINEAR;
    } else {
        patchBuilderIrregularBasis = PatchBuilder::BASIS_GREGORY;
    }

    PatchBuilder::Options patchOptions;
    patchOptions.regBasisType                = PatchBuilder::BASIS_REGULAR;
    patchOptions.irregBasisType              = patchBuilderIrregularBasis;
    patchOptions.approxInfSharpWithSmooth    = false;
    patchOptions.approxSmoothCornerWithSharp = false;
    patchOptions.fillMissingBoundaryPoints   = true;

    _patchBuilder = PatchBuilder::Create(faceRefiner, patchOptions);

    //
    //  Initialize general PatchTree members relating to patch topology:
    //
    Vtr::internal::Level const & baseLevel = _faceRefiner.getLevel(0);

    int thisFaceSize = baseLevel.getFaceVertices(_faceAtRoot).size();
    int regFaceSize  = _patchBuilder->GetRegularFaceSize();

    //  Configuration:
    _patchTree->_useDoublePrecision = options.useDoublePrecision;

    _patchTree->_patchesIncludeNonLeaf = options.includeInteriorPatches;
    _patchTree->_patchesAreTriangular  = (regFaceSize == 3);

    _patchTree->_regPatchType   = _patchBuilder->GetRegularPatchType();
    _patchTree->_irregPatchType = _patchBuilder->GetIrregularPatchType();

    _patchTree->_regPatchSize =
        PatchDescriptor(_patchTree->_regPatchType).GetNumControlVertices();
    _patchTree->_irregPatchSize =
        PatchDescriptor(_patchTree->_irregPatchType).GetNumControlVertices();
    _patchTree->_patchPointStride =
        std::max(_patchTree->_regPatchSize, _patchTree->_irregPatchSize);

    //  Topology:
    _patchTree->_numSubFaces = (thisFaceSize == regFaceSize) ? 0 : thisFaceSize;

    _patchTree->_numControlPoints  = _faceRefiner.GetLevel(0).GetNumVertices();
    _patchTree->_numRefinedPoints  = _faceRefiner.GetNumVerticesTotal()
                                   - _patchTree->_numControlPoints;
    _patchTree->_numSubPatchPoints = _patchTree->_numRefinedPoints;
}

PatchTreeBuilder::~PatchTreeBuilder() {
    delete _patchBuilder;
}

const PatchTree *
PatchTreeBuilder::Build() {

    identifyPatches();
    initializePatches();
    if (_patchTree->_useDoublePrecision) {
        initializeStencilMatrix<double>();
    } else {
        initializeStencilMatrix<float>();
    }
    initializeQuadTree();

    return _patchTree;
}

bool
PatchTreeBuilder::rootFaceNeedsRefinement() const {

    //
    //  The Far::PatchBuilder cannot construct a single patch from a face
    //  in the base level under the following circumstances:
    //
    //      - the face is or is adjacent to an irregular face (non-quad)
    //      - the face contains an inf-sharp dart vertex
    //      - the face contains an interior val-2 vertex
    //      - the face contains an interior val-3 vertex adj to a tri
    //
    //  All but the first are subject to additional conditions (e.g.
    //  whether the irregular feature is isolated or not) but until those
    //  conditions are clear, such features will trigger refinement.
    //
    int           baseFace  = _faceAtRoot;
    Level const & baseLevel = _faceRefiner.getLevel(0);

    Level::VTag const & fTags  = baseLevel.getFaceCompositeVTag(baseFace);
    ConstIndexArray     fVerts = baseLevel.getFaceVertices(baseFace);

    //
    //  Vertices incident non-quads in any way are easily detected:
    //
    if (fTags._incidIrregFace) return true;

    //
    //  A dart and inf-sharp irregularity may indicate an inf-sharp dart,
    //  so inspect the face-vertices:
    //
    if ((fTags._rule & Sdc::Crease::RULE_DART) && fTags._infIrregular) {
        for (int i = 0; i < fVerts.size(); ++i) {
            Level::VTag const & vTag = baseLevel.getVertexTag(fVerts[i]);
            if ((vTag._rule & Sdc::Crease::RULE_DART) && vTag._infSharpEdges) {
                //  WIP - inf-sharp dart is fine in some cases (TBD):
                //          - possibly when the edge-end isolated
                //      - refine for any occurrence until fully determined
                return true;
            }
        }
    }

    //
    //  Interior extra-ordinary vertices of low valence require inspection
    //  of the face-vertices to test valence and other conditions:
    //
    if (fTags._xordinary) {
        for (int i = 0, fSize = fVerts.size(); i < fSize; ++i) {
            Level::VTag const & vTag = baseLevel.getVertexTag(fVerts[i]);
            if (vTag._xordinary && !vTag._boundary && !vTag._infSharpEdges) {
                int vValence = baseLevel.getVertexFaces(fVerts[i]).size();
                if ((vValence == 2) || ((vValence == 3) && (fSize == 3))) {
                    //  WIP - low valence verts are fine in some cases (TBD)
                    //          - val-2 a problem only when two adjacent
                    //      - refine for any occurrence until fully determined
                    return true;
                }
            }
        }
    }
    return false;
}

bool
PatchTreeBuilder::testFaceAncestors() const {

    //  Conditions of overlapping faces that require testing base face:
    return (_patchBuilder->GetRegularFaceSize() == 3) &&
           (_faceRefiner.getLevel(0).getNumEdges() == 3) &&
           (_faceRefiner.getLevel(0).getNumFaces() > 1);
}

bool
PatchTreeBuilder::faceAncestorIsRoot(int level, int face) const {

    // Move up the hierarchy to the base level:
    for (int i = level; i > 0; --i) {
        face = _faceRefiner.getRefinement(i-1).getChildFaceParentFace(face);
    }
    return (face == _faceAtRoot);
}

void
PatchTreeBuilder::identifyPatches() {

    //
    //  Take inventory of the patches.  Only one face exists at the base
    //  level -- the root face.  Check all other levels breadth first:
    //
    bool incNonLeaf = _patchTree->_patchesIncludeNonLeaf;

    _patchFaces.clear();

    int numIrregPatches = 0;

    if (_patchBuilder->IsFaceAPatch(0, _faceAtRoot)) {
        if (incNonLeaf || _patchBuilder->IsFaceALeaf(0, _faceAtRoot)) {
            bool isRegular = _patchBuilder->IsPatchRegular(0, _faceAtRoot);
            _patchFaces.push_back(PatchFace(0, _faceAtRoot, isRegular));
            numIrregPatches += !isRegular;
        }
    }

    //  Under rare circumstances, the normally quick test for a patch is
    //  flawed and includes faces descended from neighboring faces:
    bool testBaseFace = testFaceAncestors();

    int numLevels = _faceRefiner.GetNumLevels();
    for (int level = 1; level < numLevels; ++level) {
        int numFaces = _faceRefiner.getLevel(level).getNumFaces();

        for (int face = 0; face < numFaces; ++face) {
            if (testBaseFace && !faceAncestorIsRoot(level, face)) continue;

            if (_patchBuilder->IsFaceAPatch(level, face)) {
                if (incNonLeaf || _patchBuilder->IsFaceALeaf(level, face)) {
                    bool isRegular = _patchBuilder->IsPatchRegular(level, face);
                    _patchFaces.push_back(PatchFace(level, face, isRegular));
                    numIrregPatches += !isRegular;
                }
            }
        }
    }

    //
    //  Allocate and populate the arrays of patch data for the identified
    //  patches:
    //
    int numPatches = (int) _patchFaces.size();
    assert(numPatches);

    _patchTree->_patchPoints.resize(numPatches * _patchTree->_patchPointStride);
    _patchTree->_patchParams.resize(numPatches);

    _patchTree->_numIrregPatches = numIrregPatches;

    _patchTree->_numSubPatchPoints += numIrregPatches *
                                      _patchTree->_irregPatchSize;
}

void
PatchTreeBuilder::initializePatches() {

    //  Keep track of the growing index of local points in irregular patches:
    int irregPointIndexBase = _patchTree->_numControlPoints +
                              _patchTree->_numRefinedPoints;

    Far::PtexIndices ptexIndices(_faceRefiner);

    for (size_t i = 0; i < _patchFaces.size(); ++i) {
        PatchFace const & pf = _patchFaces[i];

        PatchParam & patchParam = _patchTree->_patchParams[i];

        Index * patchPoints =
                &_patchTree->_patchPoints[i * _patchTree->_patchPointStride];

        if (pf.isRegular) {
            //  Determine boundary mask before computing/assigning PatchParam:
            int boundaryMask =
                _patchBuilder->GetRegularPatchBoundaryMask(pf.level, pf.face);

            patchParam = _patchBuilder->ComputePatchParam(pf.level, pf.face,
                    ptexIndices, true, boundaryMask, true);

            //  Gather the points of the patch -- since they are assigned
            //  directly into the PatchTree's buffer by the PatchBuilder
            //  here, they must be offset as a post-process:
            _patchBuilder->GetRegularPatchPoints(pf.level, pf.face,
                boundaryMask, patchPoints);

            for (int j = 0; j < _patchTree->_regPatchSize; ++j) {
                patchPoints[j] += _levelOffsets[pf.level];
            }
        } else {
            //  Compute/assign the PatchParam for an irregular patch:
            patchParam = _patchBuilder->ComputePatchParam(pf.level, pf.face,
                    ptexIndices, false /*irreg*/, 0 /*mask*/, false);

            //  Assign indices of new/local points for this irregular patch:
            for (int j = 0; j < _patchTree->_irregPatchSize; ++j) {
                patchPoints[j] = irregPointIndexBase ++;
            }
        }
    }
}

//
//  Some local interpolatable types for combining stencil vectors -- the
//  rows of the stencil matrix:
//
namespace {
    //
    //  When accessing a "row" for a control point, the only non-zero
    //  entry is that at the index, with a value of 1, so just store
    //  that index so the StencilRows can combine it:
    //
    struct ControlRow {
        ControlRow(int index) : _index(index) { }
        ControlRow() { }

        ControlRow operator[] (int index) const {
            return ControlRow(index);
        }

        //  Members:
        int _index;
    };

    //
    //  A "row" for each stencil is just our typical vector of variable
    //  size that needs to support [].
    //
    //  For the first level, there are no source rows for the control
    //  points so combine with the proxy ControlRow defined above.  All
    //  other levels will accumulate StencilRows as weighted combinations
    //  of other StencilRows.
    //
    //  WIP - consider combining StencilRows to exploit SSE/AVX vectorization
    //      - we can (in future) easily guarantee both are 4-word aligned
    //      - we can also pad the rows to a multiple of 4
    //      - prefer writing the combination in a portable way that makes
    //        use of auto-vectorization
    //
    template <typename REAL>
    struct StencilRow {
        StencilRow() : _data(0), _size(0) { }
        StencilRow(REAL * data, int size) :
                    _data(data), _size(size) { }
        StencilRow(REAL const * data, int size) :
                    _data(const_cast<REAL*>(data)), _size(size) { }

        void Clear() {
            for (int i = 0; i < _size; ++i) {
                _data[i] = 0.0f;
            }
        }

        void AddWithWeight(ControlRow const & src, REAL weight) {
            assert(src._index >= 0);
            _data[src._index] += weight;
        }

        void AddWithWeight(StencilRow const & src, REAL weight) {
            assert(src._size == _size);
            //  Weights passed here by PrimvarRefiner should be non-zero
            //  WIP - see note on potential/future auto-vectorization above
            for (int i = 0; i < _size; ++i) {
                _data[i] += weight * src._data[i];
            }
        }

        StencilRow operator[](int index) const {
            return StencilRow(_data + index * _size, _size);
        }

        //  Members:
        REAL * _data;
        int    _size;
    };
}

template <typename REAL>
void
PatchTreeBuilder::initializeStencilMatrix() {

    if (_patchTree->_numSubPatchPoints == 0) return;

    //
    //  Allocate and initialize a full matrix of true stencils (i.e.
    //  factored in terms of the control points):
    //
    int numPointStencils = _patchTree->_numRefinedPoints + 
                          (_patchTree->_numIrregPatches *
                           _patchTree->_irregPatchSize);
    int numControlPoints = _patchTree->_numControlPoints;

    std::vector<REAL> & stencilMatrix = _patchTree->getStencilMatrix<REAL>();

    stencilMatrix.resize(numPointStencils*numControlPoints);

    //
    //  For refined points, initialize successive rows of the stencil matrix
    //  a level at a time using the PrimvarRefiner to accumulate contributing
    //  rows:
    //
    int numLevels = _faceRefiner.GetNumLevels();
    if (numLevels > 1) {
        Far::PrimvarRefinerReal<REAL> primvarRefiner(_faceRefiner);

        StencilRow<REAL> dstRow(&stencilMatrix[0], numControlPoints);
        primvarRefiner.Interpolate(1, ControlRow(-1), dstRow);

        for (int level = 2; level < numLevels; ++level) {
            StencilRow<REAL> srcRow = dstRow;
            dstRow = srcRow[_faceRefiner.getLevel(level-1).getNumVertices()];
            primvarRefiner.Interpolate(level, srcRow, dstRow);
        }
    }

    //
    //  For irregular patch points, append rows for each irregular patch:
    //
    if (_patchTree->_numIrregPatches) {
        SparseMatrix<REAL> irregConvMatrix;
        std::vector<Index> irregSourcePoints;

        int stencilIndexBase = _patchTree->_numRefinedPoints;

        for (size_t i = 0; i < _patchFaces.size(); ++i) {
            if (!_patchFaces[i].isRegular) {
                getIrregularPatchConversion(_patchFaces[i],
                        irregConvMatrix, irregSourcePoints);

                appendConversionStencilsToMatrix(stencilIndexBase,
                        irregConvMatrix,irregSourcePoints);

                stencilIndexBase += _patchTree->_irregPatchSize;
            }
        }
    }
}

template <typename REAL>
void
PatchTreeBuilder::appendConversionStencilsToMatrix(
        int                        stencilBaseIndex,
        SparseMatrix<REAL> const & conversionMatrix,
        std::vector<Index> const & sourcePoints) {

    //
    //  Each row of the sparse conversion matrix corresponds to a row
    //  of the stencil matrix -- which will be computed from the weights
    //  and indices of stencils indicated by the SparseMatrix row:
    //
    int numControlPoints = _patchTree->_numControlPoints;
    int numPatchPoints   = conversionMatrix.GetNumRows();

    std::vector<REAL> & stencilMatrix = _patchTree->getStencilMatrix<REAL>();

    StencilRow<REAL> srcStencils(&stencilMatrix[0], numControlPoints);
    StencilRow<REAL> dstStencils = srcStencils[stencilBaseIndex];

    for (int i = 0; i < numPatchPoints; ++i) {
        StencilRow<REAL> dstStencil = dstStencils[i];
        dstStencil.Clear();

        int  const * rowIndices = &conversionMatrix.GetRowColumns(i)[0];
        REAL const * rowWeights = &conversionMatrix.GetRowElements(i)[0];
        int          rowSize    =  conversionMatrix.GetRowSize(i);

        for (int j = 0; j < rowSize; ++j) {
            REAL srcWeight = rowWeights[j];
            int  srcIndex  = sourcePoints[rowIndices[j]];

            //  Simply increment single weight if this is a control point
            if (srcIndex < numControlPoints) {
                dstStencil._data[srcIndex] += srcWeight;
            } else {
                int srcStencilIndex = srcIndex - numControlPoints;

                StencilRow<REAL> srcStencil = srcStencils[srcStencilIndex];

                dstStencil.AddWithWeight(srcStencil, srcWeight);
            }
        }
    }
}

void
PatchTreeBuilder::initializeQuadTree() {

    _patchTree->buildQuadtree();
}

template <typename REAL>
void
PatchTreeBuilder::getIrregularPatchConversion(PatchFace const & pf,
    SparseMatrix<REAL> & conversionMatrix,
    std::vector<Index> & sourcePoints) {

    //
    //  The topology of an irregular patch is determined by its four corners:
    //
    Level::VSpan cornerSpans[4];
    _patchBuilder->GetIrregularPatchCornerSpans(pf.level, pf.face, cornerSpans);

    //
    //  Compute the conversion matrix from refined/source points to the
    //  set of points local to this patch:
    //
    _patchBuilder->GetIrregularPatchConversionMatrix(pf.level, pf.face,
            cornerSpans, conversionMatrix);

    //
    //  Identify the refined/source points for the patch and append stencils
    //  for the local patch points in terms of the source points:
    //
    int numSourcePoints = conversionMatrix.GetNumColumns();

    sourcePoints.resize(numSourcePoints);

    _patchBuilder->GetIrregularPatchSourcePoints(pf.level, pf.face,
                                                 cornerSpans, &sourcePoints[0]);

    int sourceIndexOffset = _levelOffsets[pf.level];
    for (int i = 0; i < numSourcePoints; ++i) {
        sourcePoints[i] += sourceIndexOffset;
    }
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
