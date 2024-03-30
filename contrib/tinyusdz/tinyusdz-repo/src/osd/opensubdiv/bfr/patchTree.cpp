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

#include "../bfr/patchTree.h"
#include "../far/patchBasis.h"

#include <algorithm>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {
namespace Bfr {

using Far::PatchDescriptor;
using Far::PatchParam;


//
//  Avoid warnings comparing floating point values to zero:
//
namespace {
#ifdef __INTEL_COMPILER
#pragma warning (push)
#pragma warning disable 1572
#endif

    template <typename REAL>
    inline bool isWeightZero(REAL w) { return (w == (REAL)0.0); }

#ifdef __INTEL_COMPILER
#pragma warning (pop)
#endif
}


//
//  Simple inline methods for the PatchTree::TreeNode:
//
// sets all the children to point to the patch of given index
inline void
PatchTree::TreeNode::SetChildren(int index) {

    for (int i=0; i<4; ++i) {
        children[i].isSet  = true;
        children[i].isLeaf = true;
        children[i].SetIndex(index);
    }
}

// sets the child in "quadrant" to point to node or patch of the given index
inline void
PatchTree::TreeNode::SetChild(int quadrant, int index, bool isLeaf) {

    assert(!children[quadrant].isSet);
    children[quadrant].isSet  = true;
    children[quadrant].isLeaf = isLeaf;
    children[quadrant].SetIndex(index);
}


//
//  PatchTree constructor and destructor:
//
PatchTree::PatchTree() :
    _useDoublePrecision(false),
    _patchesIncludeNonLeaf(false),
    _patchesAreTriangular(false),
    _regPatchType(PatchDescriptor::NON_PATCH),
    _irregPatchType(PatchDescriptor::NON_PATCH),
    _regPatchSize(0),
    _irregPatchSize(0),
    _patchPointStride(0),
    _numSubFaces(0),
    _numControlPoints(0),
    _numRefinedPoints(0),
    _numSubPatchPoints(0),
    _numIrregPatches(0),
    _treeDepth(-1) {
}

PatchTree::~PatchTree() {
}


//
//  Class methods supporting access to patches:
//
PatchTree::PatchPointArray
PatchTree::GetSubPatchPoints(int patchIndex) const {

    return PatchPointArray(
            &_patchPoints[patchIndex * _patchPointStride],
            _patchParams[patchIndex].IsRegular() ? _regPatchSize
                                                 : _irregPatchSize);
}

template <typename REAL>
int
PatchTree::EvalSubPatchBasis(int patchIndex, REAL u, REAL v,
                             REAL wP[], REAL wDu[], REAL wDv[],
                             REAL wDuu[], REAL wDuv[], REAL wDvv[]) const {

    PatchParam const & param = _patchParams[patchIndex];

    return Far::internal::EvaluatePatchBasis(
            param.IsRegular() ? _regPatchType : _irregPatchType,
            param, u, v, wP, wDu, wDv, wDuu, wDuv, wDvv);
}

template <typename REAL>
int
PatchTree::EvalSubPatchStencils(int patchIndex, REAL u, REAL v,
                                REAL sP[], REAL sDu[], REAL sDv[],
                                REAL sDuu[], REAL sDuv[], REAL sDvv[]) const{

    //
    //  If evaluating a regular interior patch at the base level, evaluate
    //  basis directly into the output stencils:
    //
    PatchParam const & param = _patchParams[patchIndex];

    if ((param.GetDepth() == 0) && param.IsRegular() && !param.GetBoundary()) {
        assert(_regPatchSize == _numControlPoints);
        return Far::internal::EvaluatePatchBasis(
                _regPatchType, param, u, v, sP, sDu, sDv, sDuu, sDuv, sDuv);
    }

    //  Invoke according to precision of the internal stencil matrix:
    if (_useDoublePrecision) {
        return evalSubPatchStencils<double>(patchIndex, u, v,
                                            sP, sDu, sDv, sDuu, sDuv, sDvv);
    } else {
        return evalSubPatchStencils<float>(patchIndex, u, v,
                                           sP, sDu, sDv, sDuu, sDuv, sDvv);
    }
}

namespace {
    template <typename REAL_SRC, typename REAL_DST>
    void addToArray(REAL_DST * dst, int n, REAL_DST w, REAL_SRC const * src) {

        if (isWeightZero(w)) return;

        //  WIP - we can guarantee these vectors are aligned (in future),
        //        so anything here to make use of SSE/AVX will be worth it
        //      - prefer something portable to ensure auto-vectorization
        //      - we can also pad the matrix so "n" is a multiple of 4...
        //      - note will need to specialize when cast required

        for (int i = 0; i < n; ++i) {
            dst[i] += (REAL_DST) (w * src[i]);
        }
    }
}

template <typename REAL_MATRIX, typename REAL>
int
PatchTree::evalSubPatchStencils(int patchIndex, REAL u, REAL v,
                                REAL sP[], REAL sDu[], REAL sDv[],
                                REAL sDuu[], REAL sDuv[], REAL sDvv[]) const{

    PatchParam const & param = _patchParams[patchIndex];

    //
    //  Basis weights must be evaluated into local arrays and transformed
    //  into stencil weights (in terms of the base level control points
    //  rather than the patch's control points):
    //
    REAL wDuBuffer[20],  wDvBuffer[20];
    REAL wDuuBuffer[20], wDuvBuffer[20], wDvvBuffer[20];

    REAL   wP[20];
    REAL * wDu  = 0;
    REAL * wDv  = 0;
    REAL * wDuu = 0;
    REAL * wDuv = 0;
    REAL * wDvv = 0;

    bool d1 = sDu  && sDv;
    if (d1) {
        wDu = wDuBuffer;
        wDv = wDvBuffer;
    }

    bool d2 = d1 && sDuu && sDuv && sDvv;
    if (d2) {
        wDuu = wDuuBuffer;
        wDuv = wDuvBuffer;
        wDvv = wDvvBuffer;
    }

    Far::internal::EvaluatePatchBasis(
            param.IsRegular() ? _regPatchType : _irregPatchType,
            param, u, v, wP, wDu, wDv, wDuu, wDuv, wDvv);

    PatchPointArray patchPoints = GetSubPatchPoints(patchIndex);

    //
    //  Clear and accumulate the stencil weights for the contribution of
    //  each point of the patch:
    //
    std::memset(sP, 0, sizeof(REAL) * _numControlPoints);
    if (d1) {
        std::memset(sDu, 0, sizeof(REAL) * _numControlPoints);
        std::memset(sDv, 0, sizeof(REAL) * _numControlPoints);
    }
    if (d2) {
        std::memset(sDuu, 0, sizeof(REAL) * _numControlPoints);
        std::memset(sDuv, 0, sizeof(REAL) * _numControlPoints);
        std::memset(sDvv, 0, sizeof(REAL) * _numControlPoints);
    }

    for (int i = 0; i < patchPoints.size(); ++i) {
        int pIndex = patchPoints[i];
        if (pIndex < _numControlPoints) {
            sP[pIndex] += wP [i];
            if (d1) {
                sDu[pIndex] += wDu[i];
                sDv[pIndex] += wDv[i];
            }
            if (d2) {
                sDuu[pIndex] += wDuu[i];
                sDuv[pIndex] += wDuv[i];
                sDvv[pIndex] += wDvv[i];
            }
        } else {
            std::vector<REAL_MATRIX> const & pStencilMtx =
                getStencilMatrix<REAL_MATRIX>();
            assert(!pStencilMtx.empty());

            REAL_MATRIX const * pStencilRow =
                &pStencilMtx[(pIndex - _numControlPoints) * _numControlPoints];

            addToArray(sP, _numControlPoints, wP[i], pStencilRow);
            if (d1) {
                addToArray(sDu, _numControlPoints, wDu[i], pStencilRow);
                addToArray(sDv, _numControlPoints, wDv[i], pStencilRow);
            }
            if (d2) {
                addToArray(sDuu, _numControlPoints, wDuu[i], pStencilRow);
                addToArray(sDuv, _numControlPoints, wDuv[i], pStencilRow);
                addToArray(sDvv, _numControlPoints, wDvv[i], pStencilRow);
            }
        }
    }
    return _numControlPoints;
}

//
//  Local functions and class methods supporting tree searches and construction:
//
namespace {
    template <typename T>
    inline int
    transformUVToQuadQuadrant(T const & median, T & u, T & v) {

        int uHalf = (u >= median);
        if (uHalf) u -= median;

        int vHalf = (v >= median);
        if (vHalf) v -= median;

        return (vHalf << 1) | uHalf;
    }

    template <typename T>
    int inline
    transformUVToTriQuadrant(T const & median, T & u, T & v, bool & rotated) {

        if (!rotated) {
            if (u >= median) {
                u -= median;
                return 1;
            }
            if (v >= median) {
                v -= median;
                return 2;
            }
            if ((u + v) >= median) {
                rotated = true;
                return 3;
            }
            return 0;
        } else {
            if (u < median) {
                v -= median;
                return 1;
            }
            if (v < median) {
                u -= median;
                return 2;
            }
            u -= median;
            v -= median;
            if ((u + v) < median) {
                rotated = true;
                return 3;
            }
            return 0;
        }
    }
} // end namespace

inline PatchTree::TreeNode *
PatchTree::assignLeafOrChildNode(TreeNode * node,
        bool isLeaf, int quadrant, int patchIndex) {

    //  This is getting far enough away from PatchMap's original
    //  structure and implementation that it warrants a face lift...

    if (!node->children[quadrant].isSet) {
        if (isLeaf) {
            node->SetChild(quadrant, patchIndex, true);
            return node;
        } else {
            int newNodeIndex = (int)_treeNodes.size();
            _treeNodes.push_back(TreeNode());
            node->SetChild(quadrant, newNodeIndex, false);
            return &_treeNodes[newNodeIndex];
        }
    }

    if (isLeaf || node->children[quadrant].isLeaf) {
        //  Need to replace the leaf index with new node and index:
        int newNodeIndex = (int)_treeNodes.size();
        _treeNodes.push_back(TreeNode());
        TreeNode * newNode = &_treeNodes[newNodeIndex];

        //  Move existing patch index from child to new child node:
        newNode->patchIndex = node->children[quadrant].index;

        node->children[quadrant].SetIndex(newNodeIndex);
        node->children[quadrant].isLeaf = false;
        if (isLeaf) {
            newNode->SetChild(quadrant, patchIndex, true);
        }
        return newNode;
    } else {
        //  Simply return the existing interior node:
        return &_treeNodes[node->children[quadrant].index];
    }
}

void
PatchTree::buildQuadtree() {

    int numPatches = (int) _patchParams.size();

    _treeNodes.reserve(numPatches);
    _treeNodes.resize(_numSubFaces ? _numSubFaces : 1);
    _treeDepth = 0;

    for (int patchIndex = 0; patchIndex < numPatches; ++patchIndex) {

        PatchParam const & param = _patchParams[patchIndex];

        int depth     = param.GetDepth();
        int rootDepth = param.NonQuadRoot();
        int subFace   = param.GetFaceId();
        assert((subFace == 0) || (subFace < _numSubFaces));

        TreeNode * node = &_treeNodes[subFace];

        _treeDepth = std::max(depth, _treeDepth);

        if (depth == rootDepth) {
            node->patchIndex = patchIndex;
            continue;
        }
            
        if (!_patchesAreTriangular) {
            //  Use the UV bits of the PatchParam directly for quad patches:
            int u = param.GetU();
            int v = param.GetV();

            for (int j = rootDepth + 1; j <= depth; ++j) {
                int uBit = (u >> (depth - j)) & 1;
                int vBit = (v >> (depth - j)) & 1;

                int quadrant = (vBit << 1) | uBit;

                node = assignLeafOrChildNode(node, (j == depth), quadrant,
                                             patchIndex);
            }
        } else {
            //  Use an interior UV point of triangles to identify quadrants:
            double u = 0.25f;
            double v = 0.25f;
            param.UnnormalizeTriangle(u, v);

            double median = 0.5f;
            bool triRotated = false;

            for (int j = rootDepth + 1; j <= depth; ++j, median *= 0.5f) {
                int quadrant = transformUVToTriQuadrant(median, u, v,
                                                        triRotated);

                node = assignLeafOrChildNode(node, (j == depth), quadrant,
                                             patchIndex);
            }
        }
    }
}

int
PatchTree::searchQuadtree(double u, double v,
        int subFace, int searchDepth) const {

    //
    //  These details warrant closer inspection and possible tweaking
    //  since non-leaf patches were optionally added to the tree...
    //

    //
    //  Identify the root patch and make a quick exit when seeking it.  If
    //  there is no patch at level 0 but subpatches present (possible e.g.
    //  if adjacent to an irregular face) force the search to level 1:
    //
    TreeNode const * node = &_treeNodes[subFace];

    if (_treeDepth == 0) {
        assert(node->patchIndex >= 0);
        return node->patchIndex;
    }

    int maxDepth = ((searchDepth >= 0) && _patchesIncludeNonLeaf) ? searchDepth
                                                                  : _treeDepth;
    if (maxDepth == (_numSubFaces > 0)) {
        if (node->patchIndex >= 0) {
            return node->patchIndex;
        }
        maxDepth = 1;
    }

    //
    //  Search the tree for the sub-patch containing the given (u,v)
    //
    double median = 0.5f;
    bool triRotated = false;

    for (int depth = 1; depth <= maxDepth; ++depth, median *= 0.5f) {

        int quadrant = _patchesAreTriangular
                     ? transformUVToTriQuadrant(median, u, v, triRotated)
                     : transformUVToQuadQuadrant(median, u, v);

        //  Identify child patch if leaf, otherwise child node:
        if (node->children[quadrant].isLeaf) {
            return node->children[quadrant].index;
        } else if (node->children[quadrant].isSet) {
            node = &_treeNodes[node->children[quadrant].index];
        }
    }
    assert(node->patchIndex >= 0);
    return node->patchIndex;
}

//
//  Explicit instantiation for methods supporting float and double:
//
template int PatchTree::EvalSubPatchBasis<float>(int patchIndex,
                float u, float v,
                float wP[], float wDu[], float wDv[],
                float wDuu[], float wDuv[], float wDvv[]) const;
template int PatchTree::EvalSubPatchStencils<float>(int patchIndex,
                float u, float v,
                float sP[], float sDu[], float sDv[],
                float sDuu[], float sDuv[], float sDvv[]) const;

template int PatchTree::EvalSubPatchBasis<double>(int patchIndex,
                double u, double v,
                double wP[], double wDu[], double wDv[],
                double wDuu[], double wDuv[], double wDvv[]) const;
template int PatchTree::EvalSubPatchStencils<double>(int patchIndex,
                double u, double v,
                double sP[], double sDu[], double sDv[],
                double sDuu[], double sDuv[], double sDvv[]) const;

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
