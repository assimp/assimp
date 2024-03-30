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
#include "../sdc/crease.h"
#include "../vtr/types.h"
#include "../vtr/level.h"
#include "../vtr/triRefinement.h"

#include <cassert>
#include <cstdio>
#include <utility>


namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {
namespace internal {

//
//  Simple constructor, destructor and basic initializers:
//
TriRefinement::TriRefinement(Level const & parentArg, Level & childArg, Sdc::Options const & optionsArg) :
    Refinement(parentArg, childArg, optionsArg) {

    _splitType   = Sdc::SPLIT_TO_TRIS;
    _regFaceSize = 3;
}

TriRefinement::~TriRefinement() {
}


//
//  Methods to construct the parent-to-child mapping
//
void
TriRefinement::allocateParentChildIndices() {

    //
    //  Initialize the vectors of indices mapping parent components to those child components
    //  that will originate from each.
    //
    //
    //  Beware these child-counts when Loop subdivision supports N-sided faces in the cage
    //      - there will 2*(N-2) additional face-child-faces for each N-sided face
    //      - there will 2*(N-2)+1 additional face-child-edges for each N-sided face
    //      - there will 1 face-child-vertex for each N-sided face
    //  Can consider these reasonable estimates and grow as needed later -- but be clear
    //  about it if so.
    //
    int faceChildFaceCount = _parent->getNumFaces() * 4;
    int faceChildEdgeCount = (int) _parent->_faceEdgeIndices.size();
    int edgeChildEdgeCount = (int) _parent->_edgeVertIndices.size();

    int faceChildVertCount = 0;
    int edgeChildVertCount = _parent->getNumEdges();
    int vertChildVertCount = _parent->getNumVertices();

    //
    //  First initialize the count/offset vectors for the child-faces and child-edges of
    //  parent faces.  For now we can use the parent's face-vert counts for the child-edges
    //  of faces, but we must use a local vector for the child-faces.
    //
    //  This will be more necessary (and need adjustment) when N-sided faces are supported.
    //
    _localFaceChildFaceCountsAndOffsets.resize(_parent->getNumFaces() * 2, 4);
    for (int i = 0; i < _parent->getNumFaces(); ++i) {
        _localFaceChildFaceCountsAndOffsets[i*2 + 1] = 4 * i;
    }

    _faceChildFaceCountsAndOffsets = IndexArray(&_localFaceChildFaceCountsAndOffsets[0],
                                                (int)_localFaceChildFaceCountsAndOffsets.size());
    _faceChildEdgeCountsAndOffsets = _parent->shareFaceVertCountsAndOffsets();

    //
    //  Given we will be ignoring initial values with uniform refinement and assigning all
    //  directly, initializing here is a waste...
    //
    Index initValue = 0;

    _faceChildFaceIndices.resize(faceChildFaceCount, initValue);
    _faceChildEdgeIndices.resize(faceChildEdgeCount, initValue);
    _edgeChildEdgeIndices.resize(edgeChildEdgeCount, initValue);

    _faceChildVertIndex.resize(faceChildVertCount, initValue);
    _edgeChildVertIndex.resize(edgeChildVertCount, initValue);
    _vertChildVertIndex.resize(vertChildVertCount, initValue);
}


//
//  Methods to populate the face-vertex relation of the child Level:
//      - child faces only originate from parent faces
//
void
TriRefinement::populateFaceVertexRelation() {

    //  Both face-vertex and face-edge share the face-vertex counts/offsets within a
    //  Level, so be sure not to re-initialize it if already done:
    //
    if (_child->_faceVertCountsAndOffsets.size() == 0) {
        populateFaceVertexCountsAndOffsets();
    }
    _child->_faceVertIndices.resize(_child->getNumFaces() * 3);

    populateFaceVerticesFromParentFaces();
}

void
TriRefinement::populateFaceVertexCountsAndOffsets() {

    _child->_faceVertCountsAndOffsets.resize(_child->getNumFaces() * 2, 3);

    for (int i = 0; i < _child->getNumFaces(); ++i) {
        _child->_faceVertCountsAndOffsets[i*2 + 1] = i * 3;
    }
}

void
TriRefinement::populateFaceVerticesFromParentFaces() {

   for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        ConstIndexArray pFaceVerts = _parent->getFaceVertices(pFace),
                        pFaceEdges = _parent->getFaceEdges(pFace),
                        pFaceChildren = getFaceChildFaces(pFace);

        assert(pFaceVerts.size() == 3);
        assert(pFaceChildren.size() == 4);

        Index cVertsOfPEdges[3];
        cVertsOfPEdges[0] = _edgeChildVertIndex[pFaceEdges[0]];
        cVertsOfPEdges[1] = _edgeChildVertIndex[pFaceEdges[1]];
        cVertsOfPEdges[2] = _edgeChildVertIndex[pFaceEdges[2]];

        //
        //  For the child face at vertex I (where I is 0..2), the child vertex
        //  of vertex I becomes the I'th vertex of its child face.  This matches
        //  the pattern for quads of irregular faces for Catmark.
        //
        //  The orientation for the 4th "interior" face is unclear -- it begins
        //  with the child vertex of the 2nd edge of the triangle.  According
        //  to the notes with the Hbr implementation "the ordering of vertices
        //  here is done to preserve parametric space as best we can."
        //
        if (IndexIsValid(pFaceChildren[0])) {
            IndexArray cFaceVerts = _child->getFaceVertices(pFaceChildren[0]);

            cFaceVerts[0] = _vertChildVertIndex[pFaceVerts[0]];
            cFaceVerts[1] = cVertsOfPEdges[0];
            cFaceVerts[2] = cVertsOfPEdges[2];
        }
        if (IndexIsValid(pFaceChildren[1])) {
            IndexArray cFaceVerts = _child->getFaceVertices(pFaceChildren[1]);

            cFaceVerts[0] = cVertsOfPEdges[0];
            cFaceVerts[1] = _vertChildVertIndex[pFaceVerts[1]];
            cFaceVerts[2] = cVertsOfPEdges[1];
        }
        if (IndexIsValid(pFaceChildren[2])) {
            IndexArray cFaceVerts = _child->getFaceVertices(pFaceChildren[2]);

            cFaceVerts[0] = cVertsOfPEdges[2];
            cFaceVerts[1] = cVertsOfPEdges[1];
            cFaceVerts[2] = _vertChildVertIndex[pFaceVerts[2]];
        }
        if (IndexIsValid(pFaceChildren[3])) {
            IndexArray cFaceVerts = _child->getFaceVertices(pFaceChildren[3]);

            cFaceVerts[0] = cVertsOfPEdges[1];
            cFaceVerts[1] = cVertsOfPEdges[2];
            cFaceVerts[2] = cVertsOfPEdges[0];
        }
    }
}


//
//  Methods to populate the face-vertex relation of the child Level:
//      - child faces only originate from parent faces
//
void
TriRefinement::populateFaceEdgeRelation() {

    //  Both face-vertex and face-edge share the face-vertex counts/offsets, so be sure
    //  not to re-initialize it if already done:
    //
    if (_child->_faceVertCountsAndOffsets.size() == 0) {
        populateFaceVertexCountsAndOffsets();
    }
    _child->_faceEdgeIndices.resize(_child->getNumFaces() * 3);

    populateFaceEdgesFromParentFaces();
}

void
TriRefinement::populateFaceEdgesFromParentFaces() {

    for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        ConstIndexArray pFaceVerts = _parent->getFaceVertices(pFace),
                        pFaceEdges = _parent->getFaceEdges(pFace),
                        pFaceChildFaces = getFaceChildFaces(pFace),
                        pFaceChildEdges = getFaceChildEdges(pFace);

        assert(pFaceChildFaces.size() == 4);
        assert(pFaceChildEdges.size() == 3);

        Index pEdgeChildEdges[3][2];
        for (int i = 0; i < 3; ++i) {
            Index            pEdge  = pFaceEdges[i];
            ConstIndexArray cEdges = getEdgeChildEdges(pEdge);

            ConstIndexArray pEdgeVerts = _parent->getEdgeVertices(pEdge);

            //  Be careful to consider degenerate edge when orienting here:
            bool edgeReversedWrtFace = (pEdgeVerts[0] != pEdgeVerts[1]) &&
                                       (pFaceVerts[i] != pEdgeVerts[0]);

            pEdgeChildEdges[i][0] = cEdges[edgeReversedWrtFace];
            pEdgeChildEdges[i][1] = cEdges[!edgeReversedWrtFace];
        }

        if (IndexIsValid(pFaceChildFaces[0])) {
            IndexArray cFaceEdges = _child->getFaceEdges(pFaceChildFaces[0]);

            cFaceEdges[0] = pEdgeChildEdges[0][0];
            cFaceEdges[1] = pFaceChildEdges[0];
            cFaceEdges[2] = pEdgeChildEdges[2][1];
        }
        if (IndexIsValid(pFaceChildFaces[1])) {
            IndexArray cFaceEdges = _child->getFaceEdges(pFaceChildFaces[1]);

            cFaceEdges[0] = pEdgeChildEdges[0][1];
            cFaceEdges[1] = pEdgeChildEdges[1][0];
            cFaceEdges[2] = pFaceChildEdges[1];
        }
        if (IndexIsValid(pFaceChildFaces[2])) {
            IndexArray cFaceEdges = _child->getFaceEdges(pFaceChildFaces[2]);

            cFaceEdges[0] = pFaceChildEdges[2];
            cFaceEdges[1] = pEdgeChildEdges[1][1];
            cFaceEdges[2] = pEdgeChildEdges[2][0];
        }
        if (IndexIsValid(pFaceChildFaces[3])) {
            IndexArray cFaceEdges = _child->getFaceEdges(pFaceChildFaces[3]);

            cFaceEdges[0] = pFaceChildEdges[2];
            cFaceEdges[1] = pFaceChildEdges[0];
            cFaceEdges[2] = pFaceChildEdges[1];
        }
    }
}


//
//  Methods to populate the edge-vertex relation of the child Level:
//      - child edges originate from parent faces and edges
//
void
TriRefinement::populateEdgeVertexRelation() {

    _child->_edgeVertIndices.resize(_child->getNumEdges() * 2);

    populateEdgeVerticesFromParentFaces();
    populateEdgeVerticesFromParentEdges();
}

void
TriRefinement::populateEdgeVerticesFromParentFaces() {

    for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        ConstIndexArray pFaceEdges      = _parent->getFaceEdges(pFace),
                        pFaceChildEdges = getFaceChildEdges(pFace);

        assert(pFaceEdges.size() == 3);
        assert(pFaceChildEdges.size() == 3);

        Index pEdgeChildVerts[3];
        pEdgeChildVerts[0] = _edgeChildVertIndex[pFaceEdges[0]];
        pEdgeChildVerts[1] = _edgeChildVertIndex[pFaceEdges[1]];
        pEdgeChildVerts[2] = _edgeChildVertIndex[pFaceEdges[2]];

        if (IndexIsValid(pFaceChildEdges[0])) {
            IndexArray cEdgeVerts = _child->getEdgeVertices(pFaceChildEdges[0]);

            cEdgeVerts[0] = pEdgeChildVerts[0];
            cEdgeVerts[1] = pEdgeChildVerts[2];
        }
        if (IndexIsValid(pFaceChildEdges[1])) {
            IndexArray cEdgeVerts = _child->getEdgeVertices(pFaceChildEdges[1]);

            cEdgeVerts[0] = pEdgeChildVerts[1];
            cEdgeVerts[1] = pEdgeChildVerts[0];
        }
        if (IndexIsValid(pFaceChildEdges[2])) {
            IndexArray cEdgeVerts = _child->getEdgeVertices(pFaceChildEdges[2]);

            cEdgeVerts[0] = pEdgeChildVerts[2];
            cEdgeVerts[1] = pEdgeChildVerts[1];
        }
    }
}

void
TriRefinement::populateEdgeVerticesFromParentEdges() {

    for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
        ConstIndexArray pEdgeVerts      = _parent->getEdgeVertices(pEdge),
                        pEdgeChildEdges = getEdgeChildEdges(pEdge);

        if (IndexIsValid(pEdgeChildEdges[0])) {
            IndexArray cEdgeVerts = _child->getEdgeVertices(pEdgeChildEdges[0]);

            cEdgeVerts[0] = _edgeChildVertIndex[pEdge];
            cEdgeVerts[1] = _vertChildVertIndex[pEdgeVerts[0]];
        }
        if (IndexIsValid(pEdgeChildEdges[1])) {
            IndexArray cEdgeVerts = _child->getEdgeVertices(pEdgeChildEdges[1]);

            cEdgeVerts[0] = _edgeChildVertIndex[pEdge];
            cEdgeVerts[1] = _vertChildVertIndex[pEdgeVerts[1]];
        }
    }
}


//
//  Methods to populate the edge-face relation of the child Level:
//      - child edges originate from parent faces and edges
//      - sparse refinement poses challenges with allocation here
//          - we need to update the counts/offsets as we populate
//
void
TriRefinement::populateEdgeFaceRelation() {

    //
    //  This is essentially the same as the quad-split version except for the
    //  sizing estimates:
    //      - every child-edge within a face will have 2 incident faces
    //      - every child-edge from a edge may have N incident faces
    //          - use the parents edge-face count for this
    //
    int childEdgeFaceIndexSizeEstimate = (int)_faceChildEdgeIndices.size() * 2 +
                                         (int)_parent->_edgeFaceIndices.size() * 2;

    _child->_edgeFaceCountsAndOffsets.resize(_child->getNumEdges() * 2);
    _child->_edgeFaceIndices.resize(childEdgeFaceIndexSizeEstimate);
    _child->_edgeFaceLocalIndices.resize(childEdgeFaceIndexSizeEstimate);

    // Update _maxEdgeFaces from the parent level before calling the 
    // populateEdgeFacesFromParent methods below, as these may further
    // update _maxEdgeFaces.
    _child->_maxEdgeFaces = _parent->_maxEdgeFaces;

    populateEdgeFacesFromParentFaces();
    populateEdgeFacesFromParentEdges();

    //  Revise the over-allocated estimate based on what is used (as indicated in the
    //  count/offset for the last vertex) and trim the index vector accordingly:
    childEdgeFaceIndexSizeEstimate = _child->getNumEdgeFaces(_child->getNumEdges()-1) +
                                     _child->getOffsetOfEdgeFaces(_child->getNumEdges()-1);
    _child->_edgeFaceIndices.resize(childEdgeFaceIndexSizeEstimate);
    _child->_edgeFaceLocalIndices.resize(childEdgeFaceIndexSizeEstimate);
}

void
TriRefinement::populateEdgeFacesFromParentFaces() {

    for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        ConstIndexArray pFaceChildFaces = getFaceChildFaces(pFace),
                        pFaceChildEdges = getFaceChildEdges(pFace);

        assert(pFaceChildFaces.size() == 4);
        assert(pFaceChildEdges.size() == 3);

        //  Every child-edge of a face potentially shares the middle child face:
        Index cFaceMiddle       = pFaceChildFaces[3];
        bool  isFaceMiddleValid = IndexIsValid(cFaceMiddle);

        for (int j = 0; j < pFaceChildEdges.size(); ++j) {
            Index cEdge = pFaceChildEdges[j];
            if (IndexIsValid(cEdge)) {
                //  Reserve enough edge-faces, populate and trim as needed:
                _child->resizeEdgeFaces(cEdge, 2);

                IndexArray      cEdgeFaces  = _child->getEdgeFaces(cEdge);
                LocalIndexArray cEdgeInFace = _child->getEdgeFaceLocalIndices(cEdge);

                int cEdgeFaceCount = 0;
                if (IndexIsValid(pFaceChildFaces[j])) {
                    cEdgeFaces[cEdgeFaceCount] = pFaceChildFaces[j];
                    cEdgeInFace[cEdgeFaceCount] = (LocalIndex) ((j + 1) % 3);
                    cEdgeFaceCount++;
                }
                if (isFaceMiddleValid) {
                    cEdgeFaces[cEdgeFaceCount] = cFaceMiddle;
                    cEdgeInFace[cEdgeFaceCount] = (LocalIndex) ((j + 1) % 3);
                    cEdgeFaceCount++;
                }
                _child->trimEdgeFaces(cEdge, cEdgeFaceCount);
            }
        }
    }
}

void
TriRefinement::populateEdgeFacesFromParentEdges() {

    for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
        ConstIndexArray pEdgeChildEdges = getEdgeChildEdges(pEdge);
        if (!IndexIsValid(pEdgeChildEdges[0]) && !IndexIsValid(pEdgeChildEdges[1])) continue;

        ConstIndexArray      pEdgeFaces  = _parent->getEdgeFaces(pEdge);
        ConstLocalIndexArray pEdgeInFace = _parent->getEdgeFaceLocalIndices(pEdge);
        ConstIndexArray      pEdgeVerts = _parent->getEdgeVertices(pEdge);

        for (int j = 0; j < 2; ++j) {
            Index cEdge = pEdgeChildEdges[j];
            if (!IndexIsValid(cEdge)) continue;

            //
            //  Reserve enough edge-faces, populate and trim as needed:
            //
            _child->resizeEdgeFaces(cEdge, pEdgeFaces.size());

            IndexArray      cEdgeFaces  = _child->getEdgeFaces(cEdge);
            LocalIndexArray cEdgeInFace = _child->getEdgeFaceLocalIndices(cEdge);

            //
            //  Each parent face may contribute an incident child face:
            //
            //      For each incident face and local-index, we immediately know
            //  the two child faces that are associated with the two child edges.
            //  We just need to identify how to pair them based on edge direction.
            //
            //      Note also here, that we could identify the pairs of child faces
            //  once for the parent before dealing with each child edge (we do the
            //  "find edge in face search" twice here as a result).  We will
            //  generally have 2 or 1 incident face to the parent edge so we
            //  can put the child-pairs on the stack.
            //
            //      Here's a more promising alternative -- instead of iterating
            //  through the child edges to "pull" data from the parent, iterate
            //  through the parent edges' faces and apply valid child faces to
            //  the appropriate child edge.  We should be able to use end-verts
            //  of the parent edge to get the corresponding child face for each,
            //  but we can't avoid a vert-in-face search and a subsequent parity
            //  test of the end-vert.
            //
            int cEdgeFaceCount = 0;

            for (int i = 0; i < pEdgeFaces.size(); ++i) {
                Index pFace      = pEdgeFaces[i];
                int   edgeInFace = pEdgeInFace[i];

                ConstIndexArray pFaceVerts = _parent->getFaceVertices(pFace),
                                pFaceChildren = getFaceChildFaces(pFace);

                //  Inspect either this child of the face or the next -- be careful
                //  to consider degenerate edge when orienting here:
                int childOfEdge = (pEdgeVerts[0] == pEdgeVerts[1]) ? j :
                                  (pFaceVerts[edgeInFace] != pEdgeVerts[j]);

                int childInFace = edgeInFace + childOfEdge;
                if (childInFace == pFaceVerts.size()) childInFace = 0;

                if (IndexIsValid(pFaceChildren[childInFace])) {
                    cEdgeFaces[cEdgeFaceCount]  = pFaceChildren[childInFace];
                    cEdgeInFace[cEdgeFaceCount] = (LocalIndex) edgeInFace;
                    cEdgeFaceCount++;
                }
            }
            _child->trimEdgeFaces(cEdge, cEdgeFaceCount);
        }
    }
}


//
//  Methods to populate the vertex-face relation of the child Level:
//      - child vertices originate from parent faces, edges and vertices
//      - sparse refinement poses challenges with allocation here:
//          - we need to update the counts/offsets as we populate
//          - note this imposes ordering constraints and inhibits concurrency
//
void
TriRefinement::populateVertexFaceRelation() {

    //
    //  Unlike quad-splitting, we don't have to consider vertices originating from
    //  faces.  We also have to consider 3 faces for every incident face for vertices
    //  originating from edges.
    //
    int childVertFaceIndexSizeEstimate = (int)_parent->_edgeFaceIndices.size() * 3
                                       + (int)_parent->_vertFaceIndices.size();

    _child->_vertFaceCountsAndOffsets.resize(_child->getNumVertices() * 2);
    _child->_vertFaceIndices.resize(         childVertFaceIndexSizeEstimate);
    _child->_vertFaceLocalIndices.resize(    childVertFaceIndexSizeEstimate);

    //  Remember -- no vertices-from-faces to consider here (until N-gon support)
    if (getFirstChildVertexFromVertices() == 0) {
        populateVertexFacesFromParentVertices();
        populateVertexFacesFromParentEdges();
    } else {
        populateVertexFacesFromParentEdges();
        populateVertexFacesFromParentVertices();
    }

    //  Revise the over-allocated estimate based on what is used (as indicated in the
    //  count/offset for the last vertex) and trim the index vectors accordingly:
    childVertFaceIndexSizeEstimate = _child->getNumVertexFaces(_child->getNumVertices()-1) +
                                     _child->getOffsetOfVertexFaces(_child->getNumVertices()-1);
    _child->_vertFaceIndices.resize(     childVertFaceIndexSizeEstimate);
    _child->_vertFaceLocalIndices.resize(childVertFaceIndexSizeEstimate);
}

void
TriRefinement::populateVertexFacesFromParentEdges() {

    for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
        Index cVert = _edgeChildVertIndex[pEdge];
        if (!IndexIsValid(cVert)) continue;

        ConstIndexArray      pEdgeFaces  = _parent->getEdgeFaces(pEdge);
        ConstLocalIndexArray pEdgeInFace = _parent->getEdgeFaceLocalIndices(pEdge);

        //
        //  Reserve enough vert-faces, populate and trim to the actual size:
        //
        _child->resizeVertexFaces(cVert, 2 * pEdgeFaces.size());

        IndexArray      cVertFaces  = _child->getVertexFaces(cVert);
        LocalIndexArray cVertInFace = _child->getVertexFaceLocalIndices(cVert);

        int cVertFaceCount = 0;
        for (int i = 0; i < pEdgeFaces.size(); ++i) {
            Index pFace      = pEdgeFaces[i];
            int   edgeInFace = pEdgeInFace[i];

            //
            //  Identify the corresponding three child faces for this parent face and
            //  their orientation wrt the child vertex to which they are incident --
            //  since we have the desired ordering of child faces from the parent face,
            //  we don't care about the orientation of the parent edge.
            //
            LocalIndex leadingFace  = (LocalIndex) ((edgeInFace + 1) % 3);
            LocalIndex middleFace   = (LocalIndex) 3;
            LocalIndex trailingFace = (LocalIndex) edgeInFace;

            LocalIndex leadingLocalIndex  = (LocalIndex) edgeInFace;
            LocalIndex middleLocalIndex   = (LocalIndex) ((edgeInFace + 2) % 3);
            LocalIndex trailingLocalIndex = (LocalIndex) ((edgeInFace + 1) % 3);

            //
            //  Now simply assign those of the three child faces that are valid:
            //
            ConstIndexArray pFaceChildFaces = getFaceChildFaces(pFace);
            assert(pFaceChildFaces.size() == 4);

            Index cFace = pFaceChildFaces[leadingFace];
            if (IndexIsValid(cFace)) {
                cVertFaces[cVertFaceCount] = cFace;
                cVertInFace[cVertFaceCount] = leadingLocalIndex;
                cVertFaceCount++;
            }

            cFace = pFaceChildFaces[middleFace];
            if (IndexIsValid(cFace)) {
                cVertFaces[cVertFaceCount] = cFace;
                cVertInFace[cVertFaceCount] = middleLocalIndex;
                cVertFaceCount++;
            }

            cFace = pFaceChildFaces[trailingFace];
            if (IndexIsValid(cFace)) {
                cVertFaces[cVertFaceCount] = cFace;
                cVertInFace[cVertFaceCount] = trailingLocalIndex;
                cVertFaceCount++;
            }
        }
        _child->trimVertexFaces(cVert, cVertFaceCount);
    }
}

void
TriRefinement::populateVertexFacesFromParentVertices() {

    for (Index pVert = 0; pVert < _parent->getNumVertices(); ++pVert) {
        Index cVert = _vertChildVertIndex[pVert];
        if (!IndexIsValid(cVert)) continue;

        //
        //  Inspect the parent vert's faces:
        //
        ConstIndexArray      pVertFaces  = _parent->getVertexFaces(pVert);
        ConstLocalIndexArray pVertInFace = _parent->getVertexFaceLocalIndices(pVert);

        //
        //  Reserve enough vert-faces, populate and trim to the actual size:
        //
        _child->resizeVertexFaces(cVert, pVertFaces.size());

        IndexArray      cVertFaces  = _child->getVertexFaces(cVert);
        LocalIndexArray cVertInFace = _child->getVertexFaceLocalIndices(cVert);

        int cVertFaceCount = 0;
        for (int i = 0; i < pVertFaces.size(); ++i) {
            Index      pFace      = pVertFaces[i];
            LocalIndex pFaceChild = pVertInFace[i];

            Index cFace = getFaceChildFaces(pFace)[pFaceChild];
            if (IndexIsValid(cFace)) {
                cVertFaces[cVertFaceCount] = cFace;
                cVertInFace[cVertFaceCount] = pFaceChild;
                cVertFaceCount++;
            }
        }
        _child->trimVertexFaces(cVert, cVertFaceCount);
    }
}


//
//  Methods to populate the vertex-edge relation of the child Level:
//      - child vertices originate from parent faces, edges and vertices
//      - sparse refinement poses challenges with allocation here:
//          - we need to update the counts/offsets as we populate
//          - note this imposes ordering constraints and inhibits concurrency
//
void
TriRefinement::populateVertexEdgeRelation() {

    //
    //  Notes on allocating/initializing the vertex-edge counts/offsets vector:
    //
    //  Be aware of scheme-specific decisions here, e.g.:
    //      - no verts from parent faces for Loop
    //      - more interior edges and faces for verts from parent edges for Loop
    //      - no guaranteed "neighborhood" around Bilinear verts from verts
    //
    //  If uniform subdivision, vert-edge count will be:
    //      - 2 + 2*N faces incident parent edge for verts from parent edges
    //      - same as parent vert for verts from parent verts
    //  If sparse subdivision, vert-edge count will be:
    //      - non-trivial function of child faces in parent face
    //          - 1 child face will always result in 2 child edges
    //          * 2 child faces can mean 3 or 4 child edges
    //          - 3 child faces will always result in 4 child edges
    //      - 1 or 2 + N faces incident parent edge for verts from parent edges
    //          - where the 1 or 2 is number of child edges of parent edge
    //          - any end vertex will require all N child faces (catmark)
    //      - same as parent vert for verts from parent verts (catmark)
    //
    int childVertEdgeIndexSizeEstimate = (int)_parent->_edgeFaceIndices.size() * 2 + _parent->getNumEdges() * 2
                                       + (int)_parent->_vertEdgeIndices.size();

    _child->_vertEdgeCountsAndOffsets.resize(_child->getNumVertices() * 2);
    _child->_vertEdgeIndices.resize(         childVertEdgeIndexSizeEstimate);
    _child->_vertEdgeLocalIndices.resize(    childVertEdgeIndexSizeEstimate);

    if (getFirstChildVertexFromVertices() == 0) {
        populateVertexEdgesFromParentVertices();
        populateVertexEdgesFromParentEdges();
    } else {
        populateVertexEdgesFromParentEdges();
        populateVertexEdgesFromParentVertices();
    }

    //  Revise the over-allocated estimate based on what is used (as indicated in the
    //  count/offset for the last vertex) and trim the index vectors accordingly:
    childVertEdgeIndexSizeEstimate = _child->getNumVertexEdges(_child->getNumVertices()-1) +
                                     _child->getOffsetOfVertexEdges(_child->getNumVertices()-1);
    _child->_vertEdgeIndices.resize(     childVertEdgeIndexSizeEstimate);
    _child->_vertEdgeLocalIndices.resize(childVertEdgeIndexSizeEstimate);
}

void
TriRefinement::populateVertexEdgesFromParentEdges() {

    for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
        Index cVert = _edgeChildVertIndex[pEdge];
        if (!IndexIsValid(cVert)) continue;

        //
        //  First inspect the parent edge -- its parent faces then its child edges:
        //
        ConstIndexArray      pEdgeFaces  = _parent->getEdgeFaces(pEdge);
        ConstLocalIndexArray pEdgeInFace = _parent->getEdgeFaceLocalIndices(pEdge);

        ConstIndexArray pEdgeVerts      = _parent->getEdgeVertices(pEdge),
                        pEdgeChildEdges = getEdgeChildEdges(pEdge);

        //
        //  Reserve enough vert-edges, populate and trim to the actual size:
        //
        _child->resizeVertexEdges(cVert, pEdgeFaces.size() + 2);

        IndexArray      cVertEdges  = _child->getVertexEdges(cVert);
        LocalIndexArray cVertInEdge = _child->getVertexEdgeLocalIndices(cVert);

        //
        //  We need to order the incident edges around the vertex appropriately:
        //      - one child edge of the parent edge ("leading" in face 0)
        //      - two child edges interior to face 0
        //      - one other child edge of the parent edge ("trailing" in face 0)
        //      - child edges of all remaining faces
        //  Be careful to place the leading/trailing child edges of the parent edge
        //  correctly -- edges are not directed their orientation may vary.  The
        //  interior child edges are appropriately oriented wrt their parent face.
        //
        //  Also need to consider no faces at all, in which case we just want the
        //  child edges of the parent edge.
        //
        int cVertEdgeCount = 0;

        //  We only care about edge reversal in the first iteration -- in which
        //  the child edges of the parent edges are assigned.  Other iterations
        //  only assign the child edges from the incident parent face:
        bool pEdgeReversed = false;
        Index cEdgeOfEdge0 = INDEX_INVALID,
              cEdgeOfEdge1 = INDEX_INVALID;

        for (int i = 0; i < pEdgeFaces.size(); ++i) {
            Index pFace      = pEdgeFaces[i];
            int   edgeInFace = pEdgeInFace[i];

            ConstIndexArray pFaceChildEdges = getFaceChildEdges(pFace);

            //  Test the orientation of a non-degenerate edge in the first face:
            if (i == 0) {
                if (pEdgeVerts[0] != pEdgeVerts[1]) {
                    pEdgeReversed = (_parent->getFaceVertices(pFace)[edgeInFace] != pEdgeVerts[0]);
                }
                cEdgeOfEdge0 = pEdgeChildEdges[!pEdgeReversed];
                cEdgeOfEdge1 = pEdgeChildEdges[pEdgeReversed];
            }

            //
            //  Identify the two interior and incident child edges within the face --
            //  bracketed by the child edges of the parent edge when dealing with the
            //  first face:
            //
            Index cEdgeOfFace0 = pFaceChildEdges[(edgeInFace + 1) % 3];
            Index cEdgeOfFace1 = pFaceChildEdges[edgeInFace];

            if ((i == 0) && IndexIsValid(cEdgeOfEdge0)) {
                cVertEdges[cVertEdgeCount] = cEdgeOfEdge0;
                cVertInEdge[cVertEdgeCount] = 0;
                cVertEdgeCount++;
            }
            if (IndexIsValid(cEdgeOfFace0)) {
                cVertEdges[cVertEdgeCount] = cEdgeOfFace0;
                cVertInEdge[cVertEdgeCount] = 1;
                cVertEdgeCount++;
            }
            if (IndexIsValid(cEdgeOfFace1)) {
                cVertEdges[cVertEdgeCount] = cEdgeOfFace1;
                cVertInEdge[cVertEdgeCount] = 0;
                cVertEdgeCount++;
            }
            if ((i == 0) && IndexIsValid(cEdgeOfEdge1)) {
                cVertEdges[cVertEdgeCount] = cEdgeOfEdge1;
                cVertInEdge[cVertEdgeCount] = 0;
                cVertEdgeCount++;
            }
        }
        _child->trimVertexEdges(cVert, cVertEdgeCount);
    }
}
void
TriRefinement::populateVertexEdgesFromParentVertices() {

    for (Index pVert = 0; pVert < _parent->getNumVertices(); ++pVert) {
        Index cVert = _vertChildVertIndex[pVert];
        if (!IndexIsValid(cVert)) continue;

        //
        //  Inspect the parent vert's edges first:
        //
        ConstIndexArray      pVertEdges  = _parent->getVertexEdges(pVert);
        ConstLocalIndexArray pVertInEdge = _parent->getVertexEdgeLocalIndices(pVert);

        //
        //  Reserve enough vert-edges, populate and trim to the actual size:
        //
        _child->resizeVertexEdges(cVert, pVertEdges.size());

        IndexArray      cVertEdges  = _child->getVertexEdges(cVert);
        LocalIndexArray cVertInEdge = _child->getVertexEdgeLocalIndices(cVert);

        int cVertEdgeCount = 0;
        for (int i = 0; i < pVertEdges.size(); ++i) {
            Index cEdge = getEdgeChildEdges(pVertEdges[i])[pVertInEdge[i]];
            if (IndexIsValid(cEdge)) {
                cVertEdges[cVertEdgeCount] = cEdge;
                cVertInEdge[cVertEdgeCount] = 1;
                cVertEdgeCount++;
            }
        }
        _child->trimVertexEdges(cVert, cVertEdgeCount);
    }
}

//
//  Methods to populate child-component indices for sparse selection:
//
//  Need to find a better place for these anon helper methods now that they are required
//  both in the base class and the two subclasses for quad- and tri-splitting...
//
namespace {
    Index const IndexSparseMaskNeighboring = (1 << 0);
    Index const IndexSparseMaskSelected    = (1 << 1);

    inline void markSparseIndexNeighbor(Index& index) { index = IndexSparseMaskNeighboring; }
    inline void markSparseIndexSelected(Index& index) { index = IndexSparseMaskSelected; }
}

void
TriRefinement::markSparseFaceChildren() {

    assert(_parentFaceTag.size() > 0);

    //
    //  For each parent face:
    //      All boundary edges will be adequately marked as a result of the pass over the
    //  edges above and boundary vertices marked by selection.  So all that remains is to
    //  identify the child faces and interior child edges for a face requiring neighboring
    //  child faces.
    //      For each corner vertex selected, we need to mark the corresponding child face,
    //  the two interior child edges and shared child vertex in the middle.
    //
    for (Index pFace = 0; pFace < parent().getNumFaces(); ++pFace) {
        //
        //  Mark all descending child components of a selected face.  Otherwise inspect
        //  its incident vertices to see if anything neighboring has been selected --
        //  requiring partial refinement of this face.
        //
        //  Remember that a selected face cannot be transitional, and that only a
        //  transitional face will be partially refined.
        //
        IndexArray fChildFaces = getFaceChildFaces(pFace);
        IndexArray fChildEdges = getFaceChildEdges(pFace);

        assert(fChildFaces.size() == 4);
        assert(fChildEdges.size() == 3);

        ConstIndexArray fVerts = parent().getFaceVertices(pFace);

        SparseTag& pFaceTag = _parentFaceTag[pFace];

        if (pFaceTag._selected) {
            markSparseIndexSelected(fChildFaces[0]);
            markSparseIndexSelected(fChildFaces[1]);
            markSparseIndexSelected(fChildFaces[2]);
            markSparseIndexSelected(fChildFaces[3]);

            markSparseIndexSelected(fChildEdges[0]);
            markSparseIndexSelected(fChildEdges[1]);
            markSparseIndexSelected(fChildEdges[2]);

            pFaceTag._transitional = 0;
        } else {
            int marked = _parentVertexTag[fVerts[0]]._selected 
                       + _parentVertexTag[fVerts[1]]._selected 
                       + _parentVertexTag[fVerts[2]]._selected;

            if (marked) {
                //
                //  If marked, see if we have any transitional edges, in which case we
                //  need to include the middle face:
                //
                ConstIndexArray fEdges = parent().getFaceEdges(pFace);

                pFaceTag._transitional = (unsigned char)
                       ((_parentEdgeTag[fEdges[0]]._transitional << 0) |
                        (_parentEdgeTag[fEdges[1]]._transitional << 1) |
                        (_parentEdgeTag[fEdges[2]]._transitional << 2));

                //  Now mark the child faces and their associated edges:
                //
                if (pFaceTag._transitional) {
                    markSparseIndexNeighbor(fChildFaces[3]);

                    markSparseIndexNeighbor(fChildEdges[0]);
                    markSparseIndexNeighbor(fChildEdges[1]);
                    markSparseIndexNeighbor(fChildEdges[2]);
                }
                if (_parentVertexTag[fVerts[0]]._selected) {
                    markSparseIndexNeighbor(fChildFaces[0]);
                    markSparseIndexNeighbor(fChildEdges[0]);
                }
                if (_parentVertexTag[fVerts[1]]._selected) {
                    markSparseIndexNeighbor(fChildFaces[1]);
                    markSparseIndexNeighbor(fChildEdges[1]);
                }
                if (_parentVertexTag[fVerts[2]]._selected) {
                    markSparseIndexNeighbor(fChildFaces[2]);
                    markSparseIndexNeighbor(fChildEdges[2]);
                }
            }
        }
    }
}

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
