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

#include "../far/patchMap.h"

#include <algorithm>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

//
//  Inline quadtree assembly methods used by the constructor:
//

// sets all the children to point to the patch of given index
inline void
PatchMap::QuadNode::SetChildren(int index) {

    for (int i=0; i<4; ++i) {
        children[i].isSet  = true;
        children[i].isLeaf = true;
        children[i].index  = index;
    }
}

// sets the child in "quadrant" to point to the node or patch of the given index
inline void
PatchMap::QuadNode::SetChild(int quadrant, int index, bool isLeaf) {

    assert(!children[quadrant].isSet);
    children[quadrant].isSet  = true;
    children[quadrant].isLeaf = isLeaf;
    children[quadrant].index  = index;
}

inline void
PatchMap::assignRootNode(QuadNode * node, int index) {

    //  Assign the given index to all children of the node (all leaves)
    node->SetChildren(index);
}

inline PatchMap::QuadNode *
PatchMap::assignLeafOrChildNode(QuadNode * node, bool isLeaf, int quadrant, int index) {

    //  Assign the node given if it is a leaf node, otherwise traverse
    //  the node -- creating/assigning a new child node if needed

    if (isLeaf) {
        node->SetChild(quadrant, index, true);
        return node;
    }
    if (node->children[quadrant].isSet) {
        return &_quadtree[node->children[quadrant].index];
    } else {
        int newChildNodeIndex = (int)_quadtree.size();
        _quadtree.push_back(QuadNode());
        node->SetChild(quadrant, newChildNodeIndex, false);
        return &_quadtree[newChildNodeIndex];
    }
}

//
//  Constructor and initialization methods for the handles and quadtree:
//
PatchMap::PatchMap(PatchTable const & patchTable) :
    _minPatchFace(-1), _maxPatchFace(-1), _maxDepth(0) {

    _patchesAreTriangular =
        patchTable.GetVaryingPatchDescriptor().GetNumControlVertices() == 3;

    if (patchTable.GetNumPatchesTotal() > 0) {
        initializeHandles(patchTable);
        initializeQuadtree(patchTable);
    }
}

void
PatchMap::initializeHandles(PatchTable const & patchTable) {

    //
    //  Populate the vector of patch Handles.  Keep track of the min and max
    //  face indices to allocate resources accordingly and limit queries:
    //
    _minPatchFace = (int) patchTable.GetPatchParamTable()[0].GetFaceId();
    _maxPatchFace = _minPatchFace;

    int numArrays  = (int) patchTable.GetNumPatchArrays();
    int numPatches = (int) patchTable.GetNumPatchesTotal();

    _handles.resize(numPatches);

    for (int pArray = 0, handleIndex = 0; pArray < numArrays; ++pArray) {

        ConstPatchParamArray params = patchTable.GetPatchParams(pArray);

        int patchSize = patchTable.GetPatchArrayDescriptor(pArray).GetNumControlVertices();

        for (Index j=0; j < patchTable.GetNumPatches(pArray); ++j, ++handleIndex) {

            Handle & h = _handles[handleIndex];

            h.arrayIndex = pArray;
            h.patchIndex = handleIndex;
            h.vertIndex  = j * patchSize;

            int patchFaceId = params[j].GetFaceId();
            _minPatchFace = std::min(_minPatchFace, patchFaceId);
            _maxPatchFace = std::max(_maxPatchFace, patchFaceId);
        }
    }
}

void
PatchMap::initializeQuadtree(PatchTable const & patchTable) {

    //
    //  Reserve quadtree nodes for the worst case and prune later.  Set the
    //  initial size to accomodate the root node of each patch face:
    //
    int nPatchFaces = (_maxPatchFace - _minPatchFace) + 1;

    int nHandles = (int)_handles.size();

    _quadtree.reserve(nPatchFaces + nHandles);
    _quadtree.resize(nPatchFaces);

    PatchParamTable const & params = patchTable.GetPatchParamTable();

    for (int handle = 0; handle < nHandles; ++handle) {

        PatchParam const & param = params[handle];

        int depth     = param.GetDepth();
        int rootDepth = param.NonQuadRoot();

        _maxDepth = std::max(_maxDepth, depth);

        QuadNode * node = &_quadtree[param.GetFaceId() - _minPatchFace];

        if (depth == rootDepth) {
            assignRootNode(node, handle);
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

                node = assignLeafOrChildNode(node, (j == depth), quadrant, handle);
            }
        } else {
            //  Use an interior UV point of triangles to identify quadrants:
            double u = 0.25;
            double v = 0.25;
            param.UnnormalizeTriangle(u, v);

            double median = 0.5;
            bool triRotated = false;

            for (int j = rootDepth + 1; j <= depth; ++j, median *= 0.5) {
                int quadrant = transformUVToTriQuadrant(median, u, v, triRotated);

                node = assignLeafOrChildNode(node, (j == depth), quadrant, handle);
            }
        }
    }

    //  Swap the Node vector with a copy to reduce worst case memory allocation:
    QuadTree tmpTree = _quadtree;
    _quadtree.swap(tmpTree);
}

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
