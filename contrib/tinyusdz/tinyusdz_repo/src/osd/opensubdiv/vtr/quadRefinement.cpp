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
#include "../vtr/quadRefinement.h"

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
QuadRefinement::QuadRefinement(Level const & parentArg, Level & childArg, Sdc::Options const & optionsArg) :
    Refinement(parentArg, childArg, optionsArg) {

    _splitType   = Sdc::SPLIT_TO_QUADS;
    _regFaceSize = 4;
}

QuadRefinement::~QuadRefinement() {
}


//
//  Methods to construct the parent-to-child mapping
//
void
QuadRefinement::allocateParentChildIndices() {

    //
    //  Initialize the vectors of indices mapping parent components to those child components
    //  that will originate from each.
    //
    int faceChildFaceCount = (int) _parent->_faceVertIndices.size();
    int faceChildEdgeCount = (int) _parent->_faceEdgeIndices.size();
    int edgeChildEdgeCount = (int) _parent->_edgeVertIndices.size();

    int faceChildVertCount = _parent->getNumFaces();
    int edgeChildVertCount = _parent->getNumEdges();
    int vertChildVertCount = _parent->getNumVertices();

    //
    //  First reference the parent Level's face-vertex counts/offsets -- they can be used
    //  here for both the face-child-faces and face-child-edges as they both have one per
    //  face-vertex.
    //
    //  Given we will be ignoring initial values with uniform refinement and assigning all
    //  directly, initializing here is a waste...
    //
    Index initValue = 0;

    _faceChildFaceCountsAndOffsets = _parent->shareFaceVertCountsAndOffsets();
    _faceChildEdgeCountsAndOffsets = _parent->shareFaceVertCountsAndOffsets();

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
QuadRefinement::populateFaceVertexRelation() {

    //  Both face-vertex and face-edge share the face-vertex counts/offsets within a
    //  Level, so be sure not to re-initialize it if already done:
    //
    if (_child->_faceVertCountsAndOffsets.size() == 0) {
        populateFaceVertexCountsAndOffsets();
    }
    _child->_faceVertIndices.resize(_child->getNumFaces() * 4);

    populateFaceVerticesFromParentFaces();
}

void
QuadRefinement::populateFaceVertexCountsAndOffsets() {

    _child->_faceVertCountsAndOffsets.resize(_child->getNumFaces() * 2);

    for (int i = 0; i < _child->getNumFaces(); ++i) {
        _child->_faceVertCountsAndOffsets[i*2 + 0] = 4;
        _child->_faceVertCountsAndOffsets[i*2 + 1] = i << 2;
    }
}

void
QuadRefinement::populateFaceVerticesFromParentFaces() {

    //
    //  This is pretty straightforward, but is a good example for the case of
    //  iterating through the parent faces rather than the child faces, as the
    //  same topology information for the parent faces is required for each of
    //  the child faces.
    //
    //  For each of the child faces of a parent face, identify the child vertices
    //  for its face-verts from the child vertices of the parent face, its edges
    //  and its vertices.
    //
    for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        ConstIndexArray pFaceVerts = _parent->getFaceVertices(pFace),
                        pFaceEdges = _parent->getFaceEdges(pFace),
                        pFaceChildren = getFaceChildFaces(pFace);

        int pFaceSize = pFaceVerts.size();
        for (int j = 0; j < pFaceSize; ++j) {
            Index cFace = pFaceChildren[j];
            if (IndexIsValid(cFace)) {
                int jPrev = j ? (j - 1) : (pFaceSize - 1);

                Index cVertOfFace  = _faceChildVertIndex[pFace];
                Index cVertOfEPrev = _edgeChildVertIndex[pFaceEdges[jPrev]];
                Index cVertOfVert  = _vertChildVertIndex[pFaceVerts[j]];
                Index cVertOfENext = _edgeChildVertIndex[pFaceEdges[j]];

                IndexArray cFaceVerts = _child->getFaceVertices(cFace);

                //  Note orientation wrt parent face -- quad vs non-quad...
                if (pFaceSize == 4) {
                    int jOpp  = jPrev ? (jPrev - 1) : 3;
                    int jNext = jOpp  ? (jOpp  - 1) : 3;

                    cFaceVerts[j]     = cVertOfVert;
                    cFaceVerts[jNext] = cVertOfENext;
                    cFaceVerts[jOpp]  = cVertOfFace;
                    cFaceVerts[jPrev] = cVertOfEPrev;
                } else {
                    cFaceVerts[0] = cVertOfVert;
                    cFaceVerts[1] = cVertOfENext;
                    cFaceVerts[2] = cVertOfFace;
                    cFaceVerts[3] = cVertOfEPrev;
                }
            }
        }
    }
}


//
//  Methods to populate the face-vertex relation of the child Level:
//      - child faces only originate from parent faces
//
void
QuadRefinement::populateFaceEdgeRelation() {

    //  Both face-vertex and face-edge share the face-vertex counts/offsets, so be sure
    //  not to re-initialize it if already done:
    //
    if (_child->_faceVertCountsAndOffsets.size() == 0) {
        populateFaceVertexCountsAndOffsets();
    }
    _child->_faceEdgeIndices.resize(_child->getNumFaces() * 4);

    populateFaceEdgesFromParentFaces();
}

void
QuadRefinement::populateFaceEdgesFromParentFaces() {

    //
    //  This is fairly straightforward, but since we are dealing with edges here, we
    //  occasionally have to deal with the limitation of them being undirected.  Since
    //  child faces from the same parent face share much in common, we iterate through
    //  the parent faces.
    //
    //  Each child face of the parent is based on a corner vertex from which we denote
    //  a "previous" and "next" edge, which are child edges of the parent face's edges.
    //  The two remaining edges per child faces are perpendicular to these prev/next
    //  edges and share the child vertex of the parent face.
    //
    for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        ConstIndexArray pFaceVerts = _parent->getFaceVertices(pFace),
                        pFaceEdges = _parent->getFaceEdges(pFace),
                        pFaceChildFaces = getFaceChildFaces(pFace),
                        pFaceChildEdges = getFaceChildEdges(pFace);

        int pFaceSize = pFaceVerts.size();

        for (int j = 0; j < pFaceSize; ++j) {
            Index cFace = pFaceChildFaces[j];
            if (IndexIsValid(cFace)) {
                //
                //  Identify the vertex pairs for the prev/next parent edges -- from
                //  which we will determine the prev/next child edges:
                //
                int jPrev = j ? (j - 1) : (pFaceSize - 1);

                Index           pPrevEdge      = pFaceEdges[jPrev];
                ConstIndexArray pPrevEdgeVerts = _parent->getEdgeVertices(pPrevEdge);

                Index           pNextEdge      = pFaceEdges[j];
                ConstIndexArray pNextEdgeVerts = _parent->getEdgeVertices(pNextEdge);

                //
                //  Now identify the two prev/next child edges (beware of degenerate
                //  edges here) and the two remaining perpendicular child edges:
                //
                Index pCornerVert = pFaceVerts[j];

                int cornerInPrevEdge = (pPrevEdgeVerts[0] != pPrevEdgeVerts[1])
                                     ? (pPrevEdgeVerts[0] != pCornerVert) : 1;

                int cornerInNextEdge = (pNextEdgeVerts[0] != pNextEdgeVerts[1])
                                     ? (pNextEdgeVerts[0] != pCornerVert) : 0;

                Index cEdgeOfEdgePrev = getEdgeChildEdges(pPrevEdge)[cornerInPrevEdge];
                Index cEdgeOfEdgeNext = getEdgeChildEdges(pNextEdge)[cornerInNextEdge];

                Index cEdgePerpEdgePrev = pFaceChildEdges[jPrev];
                Index cEdgePerpEdgeNext = pFaceChildEdges[j];

                //
                //  Assign the identified child edges to the child face's face-edges:
                //
                IndexArray cFaceEdges = _child->getFaceEdges(cFace);

                //  Note orientation wrt parent face -- quad vs non-quad...
                if (pFaceSize == 4) {
                    int jOpp  = jPrev ? (jPrev - 1) : 3;
                    int jNext = jOpp  ? (jOpp  - 1) : 3;

                    cFaceEdges[j]     = cEdgeOfEdgeNext;
                    cFaceEdges[jNext] = cEdgePerpEdgeNext;
                    cFaceEdges[jOpp]  = cEdgePerpEdgePrev;
                    cFaceEdges[jPrev] = cEdgeOfEdgePrev;
                } else {
                    cFaceEdges[0] = cEdgeOfEdgeNext;
                    cFaceEdges[1] = cEdgePerpEdgeNext;
                    cFaceEdges[2] = cEdgePerpEdgePrev;
                    cFaceEdges[3] = cEdgeOfEdgePrev;
                }
            }
        }
    }
}

//
//  Methods to populate the edge-vertex relation of the child Level:
//      - child edges originate from parent faces and edges
//
void
QuadRefinement::populateEdgeVertexRelation() {

    _child->_edgeVertIndices.resize(_child->getNumEdges() * 2);

    populateEdgeVerticesFromParentFaces();
    populateEdgeVerticesFromParentEdges();
}

void
QuadRefinement::populateEdgeVerticesFromParentFaces() {

    //
    //  This is straightforward.  All child edges of parent faces are assigned
    //  their first vertex from the child vertex of the face -- so it is common
    //  to all.  The second vertex is the child vertex of the parent edge to
    //  which the new child edge is perpendicular.
    //
    for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        ConstIndexArray pFaceEdges      = _parent->getFaceEdges(pFace),
                        pFaceChildEdges = getFaceChildEdges(pFace);

        for (int j = 0; j < pFaceEdges.size(); ++j) {
            Index cEdge = pFaceChildEdges[j];
            if (IndexIsValid(cEdge)) {
                IndexArray cEdgeVerts = _child->getEdgeVertices(cEdge);

                cEdgeVerts[0] = _faceChildVertIndex[pFace];
                cEdgeVerts[1] = _edgeChildVertIndex[pFaceEdges[j]];
            }
        }
    }
}

void
QuadRefinement::populateEdgeVerticesFromParentEdges() {

    //
    //  This is straightforward.  All child edges of parent edges are assigned
    //  their first vertex from the child vertex of the edge -- so it is common
    //  to both.  The second vertex is the child vertex of the vertex at the
    //  end of the parent edge.
    //
    for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
        ConstIndexArray pEdgeVerts = _parent->getEdgeVertices(pEdge),
                        pEdgeChildren = getEdgeChildEdges(pEdge);

        //  May want to unroll this trivial loop of 2...
        for (int j = 0; j < 2; ++j) {
            Index cEdge = pEdgeChildren[j];
            if (IndexIsValid(cEdge)) {
                IndexArray cEdgeVerts = _child->getEdgeVertices(cEdge);

                cEdgeVerts[0] = _edgeChildVertIndex[pEdge];
                cEdgeVerts[1] = _vertChildVertIndex[pEdgeVerts[j]];
            }
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
QuadRefinement::populateEdgeFaceRelation() {

    //
    //  Notes on allocating/initializing the edge-face counts/offsets vector:
    //
    //  Be aware of scheme-specific decisions here, e.g.:
    //      - inspection of sparse child faces for edges from faces
    //      - no guaranteed "neighborhood" around Bilinear verts from verts
    //
    //  If uniform subdivision, face count of a child edge will be:
    //      - 2 for new interior edges from parent faces
    //          == 2 * number of parent face verts for both quad- and tri-split
    //      - same as parent edge for edges from parent edges
    //  If sparse subdivision, face count of a child edge will be:
    //      - 1 or 2 for new interior edge depending on child faces in parent face
    //          - requires inspection if not all child faces present
    //      ? same as parent edge for edges from parent edges
    //          - given end vertex must have its full set of child faces
    //          - not for Bilinear -- only if neighborhood is non-zero
    //      - could at least make a quick traversal of components and use the above
    //        two points to get much closer estimate than what is used for uniform
    //
    int childEdgeFaceIndexSizeEstimate = (int)_parent->_faceVertIndices.size() * 2 +
                                         (int)_parent->_edgeFaceIndices.size() * 2;

    _child->_edgeFaceCountsAndOffsets.resize(_child->getNumEdges() * 2);
    _child->_edgeFaceIndices.resize(     childEdgeFaceIndexSizeEstimate);
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
    _child->_edgeFaceIndices.resize(     childEdgeFaceIndexSizeEstimate);
    _child->_edgeFaceLocalIndices.resize(childEdgeFaceIndexSizeEstimate);
}

void
QuadRefinement::populateEdgeFacesFromParentFaces() {

    //
    //  This is straightforward topologically, but when refinement is sparse the
    //  contents of the counts/offsets vector is not certain and is populated
    //  incrementally.  So there will be some resizing/trimming here.
    //
    //  Topologically, the child edges from within a parent face will typically
    //  have two incident child faces (only one or none if sparse).  These child
    //  edges and faces are interleaved within the parent and easily identified.
    //  Note that the edge-face "local indices" are also needed here and that
    //  orientation of child faces within their parent depends on it being a quad
    //  or not.
    //
    for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        ConstIndexArray pFaceChildFaces = getFaceChildFaces(pFace),
                        pFaceChildEdges = getFaceChildEdges(pFace);

        int pFaceSize = pFaceChildFaces.size();

        for (int j = 0; j < pFaceSize; ++j) {
            Index cEdge = pFaceChildEdges[j];
            if (IndexIsValid(cEdge)) {
                //
                //  Reserve enough edge-faces, populate and trim as needed:
                //
                _child->resizeEdgeFaces(cEdge, 2);

                IndexArray      cEdgeFaces  = _child->getEdgeFaces(cEdge);
                LocalIndexArray cEdgeInFace = _child->getEdgeFaceLocalIndices(cEdge);

                //  One or two child faces may be assigned:
                int jNext = ((j + 1) < pFaceSize) ? (j + 1) : 0;

                int cEdgeFaceCount = 0;
                if (IndexIsValid(pFaceChildFaces[j])) {
                    //  Note orientation wrt incident parent faces -- quad vs non-quad...
                    cEdgeFaces[cEdgeFaceCount]  = pFaceChildFaces[j];
                    cEdgeInFace[cEdgeFaceCount] = (LocalIndex)((pFaceSize == 4) ? jNext : 1);
                    cEdgeFaceCount++;
                }
                if (IndexIsValid(pFaceChildFaces[jNext])) {
                    //  Note orientation wrt incident parent faces -- quad vs non-quad...
                    cEdgeFaces[cEdgeFaceCount]  = pFaceChildFaces[jNext];
                    cEdgeInFace[cEdgeFaceCount] = (LocalIndex)((pFaceSize == 4) ? ((jNext + 2) & 3) : 2);
                    cEdgeFaceCount++;
                }
                _child->trimEdgeFaces(cEdge, cEdgeFaceCount);
            }
        }
    }
}

void
QuadRefinement::populateEdgeFacesFromParentEdges() {

    //
    //  Note -- the edge-face counts/offsets vector is not known
    //  ahead of time and is populated incrementally, so we cannot
    //  thread this yet...
    //
    for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
        ConstIndexArray pEdgeChildEdges = getEdgeChildEdges(pEdge);
        if (!IndexIsValid(pEdgeChildEdges[0]) && !IndexIsValid(pEdgeChildEdges[1])) continue;

        ConstIndexArray      pEdgeFaces = _parent->getEdgeFaces(pEdge);
        ConstLocalIndexArray pEdgeInFace = _parent->getEdgeFaceLocalIndices(pEdge);
        ConstIndexArray      pEdgeVerts = _parent->getEdgeVertices(pEdge);

        for (int j = 0; j < 2; ++j) {
            Index cEdge = pEdgeChildEdges[j];
            if (!IndexIsValid(cEdge)) continue;

            //  Reserve enough edge-faces, populate and trim as needed:
            _child->resizeEdgeFaces(cEdge, pEdgeFaces.size());

            IndexArray      cEdgeFaces  = _child->getEdgeFaces(cEdge);
            LocalIndexArray cEdgeInFace = _child->getEdgeFaceLocalIndices(cEdge);

            //
            //  Each parent face may contribute an incident child face:
            //
            int cEdgeFaceCount = 0;

            for (int i = 0; i < pEdgeFaces.size(); ++i) {
                Index pFace      = pEdgeFaces[i];
                int   edgeInFace = pEdgeInFace[i];

                ConstIndexArray pFaceVerts = _parent->getFaceVertices(pFace),
                                pFaceChildren = getFaceChildFaces(pFace);

                //
                //  We need to first identify the potentially incident child-face and see
                //  if it exists before we can assign it.  Beware a degenerate edge here
                //  when inspecting the undirected edge.
                //
                int childOfEdge = (pEdgeVerts[0] == pEdgeVerts[1]) ? j : (pFaceVerts[edgeInFace] != pEdgeVerts[j]);

                int childInFace = edgeInFace + childOfEdge;
                if (childInFace == pFaceChildren.size()) childInFace = 0;

                if (IndexIsValid(pFaceChildren[childInFace])) {
                    //  Note orientation wrt incident parent faces -- quad vs non-quad...
                    cEdgeFaces[cEdgeFaceCount] = pFaceChildren[childInFace];
                    cEdgeInFace[cEdgeFaceCount] = (LocalIndex)
                            ((pFaceVerts.size() == 4) ? edgeInFace : (childOfEdge ? 3 : 0));
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
QuadRefinement::populateVertexFaceRelation() {

    //
    //  Notes on allocating/initializing the vertex-face counts/offsets vector:
    //
    //  Be aware of scheme-specific decisions here, e.g.:
    //      - no verts from parent faces for Loop (unless N-gons supported)
    //      - more interior edges and faces for verts from parent edges for Loop
    //      - no guaranteed "neighborhood" around Bilinear verts from verts
    //
    //  If uniform subdivision, vert-face count will be (catmark or loop):
    //      - 4 or 0 for verts from parent faces (for catmark)
    //      - 2x or 3x number in parent edge for verts from parent edges
    //      - same as parent vert for verts from parent verts
    //  If sparse subdivision, vert-face count will be:
    //      - the number of child faces in parent face
    //      - 1 or 2x number in parent edge for verts from parent edges
    //          - where the 1 or 2 is number of child edges of parent edge
    //      - same as parent vert for verts from parent verts (catmark)
    //
    int childVertFaceIndexSizeEstimate = (int)_parent->_faceVertIndices.size()
                                       + (int)_parent->_edgeFaceIndices.size() * 2
                                       + (int)_parent->_vertFaceIndices.size();

    _child->_vertFaceCountsAndOffsets.resize(_child->getNumVertices() * 2);
    _child->_vertFaceIndices.resize(         childVertFaceIndexSizeEstimate);
    _child->_vertFaceLocalIndices.resize(    childVertFaceIndexSizeEstimate);

    if (getFirstChildVertexFromVertices() == 0) {
        populateVertexFacesFromParentVertices();
        populateVertexFacesFromParentFaces();
        populateVertexFacesFromParentEdges();
    } else {
        populateVertexFacesFromParentFaces();
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
QuadRefinement::populateVertexFacesFromParentFaces() {

    for (int pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        int cVert = _faceChildVertIndex[pFace];
        if (!IndexIsValid(cVert)) continue;

        ConstIndexArray pFaceChildren = getFaceChildFaces(pFace);
        int pFaceSize = pFaceChildren.size();

        //
        //  Reserve enough vert-faces, populate and trim to the actual size:
        //
        _child->resizeVertexFaces(cVert, pFaceSize);

        IndexArray      cVertFaces  = _child->getVertexFaces(cVert);
        LocalIndexArray cVertInFace = _child->getVertexFaceLocalIndices(cVert);

        //
        //  Inspect each of the child faces of this parent face and add those that
        //  exist as incident the child vertex of this face:
        //
        int cVertFaceCount = 0;
        for (int j = 0; j < pFaceSize; ++j) {
            if (IndexIsValid(pFaceChildren[j])) {
                //  Note orientation wrt parent face -- quad vs non-quad...
                cVertFaces[cVertFaceCount]  = pFaceChildren[j];
                cVertInFace[cVertFaceCount] = (LocalIndex)((pFaceSize == 4) ? ((j+2) & 3) : 2);
                cVertFaceCount++;
            }
        }
        _child->trimVertexFaces(cVert, cVertFaceCount);
    }
}

void
QuadRefinement::populateVertexFacesFromParentEdges() {

    for (int pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
        int cVert = _edgeChildVertIndex[pEdge];
        if (!IndexIsValid(cVert)) continue;

        ConstIndexArray      pEdgeFaces  = _parent->getEdgeFaces(pEdge);
        ConstLocalIndexArray pEdgeInFace = _parent->getEdgeFaceLocalIndices(pEdge);

        //
        //  Reserve enough vert-faces, populate and trim to the actual size:
        //
        _child->resizeVertexFaces(cVert, 2 * pEdgeFaces.size());

        IndexArray      cVertFaces  = _child->getVertexFaces(cVert);
        LocalIndexArray cVertInFace = _child->getVertexFaceLocalIndices(cVert);

        //
        //  For each face incident the parent edge, identify its corresponding two child faces
        //  and assign those of the two that exist.  The second face is considered and added
        //  first to preserve CC-wise ordering of faces wrt the vertex.
        //
        int cVertFaceCount = 0;
        for (int i = 0; i < pEdgeFaces.size(); ++i) {
            Index pFace      = pEdgeFaces[i];
            int   edgeInFace = pEdgeInFace[i];

            ConstIndexArray pFaceChildren = getFaceChildFaces(pFace);
            int pFaceSize = pFaceChildren.size();

            int faceChild0 = edgeInFace;
            int faceChild1 = edgeInFace + 1;
            if (faceChild1 == pFaceChildren.size()) faceChild1 = 0;

            if (IndexIsValid(pFaceChildren[faceChild1])) {
                //  Note orientation wrt incident parent faces -- quad vs non-quad...
                cVertFaces[cVertFaceCount] = pFaceChildren[faceChild1];
                cVertInFace[cVertFaceCount] = (LocalIndex)((pFaceSize == 4) ? faceChild0 : 3);
                cVertFaceCount++;
            }
            if (IndexIsValid(pFaceChildren[faceChild0])) {
                //  Note orientation wrt incident parent faces -- quad vs non-quad...
                cVertFaces[cVertFaceCount] = pFaceChildren[faceChild0];
                cVertInFace[cVertFaceCount] = (LocalIndex)((pFaceSize == 4) ? faceChild1 : 1);
                cVertFaceCount++;
            }
        }
        _child->trimVertexFaces(cVert, cVertFaceCount);
    }
}

void
QuadRefinement::populateVertexFacesFromParentVertices() {

    for (int pVert = 0; pVert < _parent->getNumVertices(); ++pVert) {
        int cVert = _vertChildVertIndex[pVert];
        if (!IndexIsValid(cVert)) continue;

        ConstIndexArray      pVertFaces  = _parent->getVertexFaces(pVert);
        ConstLocalIndexArray pVertInFace = _parent->getVertexFaceLocalIndices(pVert);

        //
        //  Reserve enough vert-faces, populate and trim to the actual size:
        //
        _child->resizeVertexFaces(cVert, pVertFaces.size());

        IndexArray      cVertFaces  = _child->getVertexFaces(cVert);
        LocalIndexArray cVertInFace = _child->getVertexFaceLocalIndices(cVert);

        //
        //  Inspect each of the faces incident the parent vertex and add those that
        //  spawned a child face corresponding to (and so incident) this child vertex:
        //
        int cVertFaceCount = 0;
        for (int i = 0; i < pVertFaces.size(); ++i) {
            Index      pFace      = pVertFaces[i];
            LocalIndex vertInFace = pVertInFace[i];

            ConstIndexArray pFaceChildren = getFaceChildFaces(pFace);

            if (IndexIsValid(pFaceChildren[vertInFace])) {
                int pFaceSize = pFaceChildren.size();

                //  Note orientation wrt incident parent faces -- quad vs non-quad...
                cVertFaces[cVertFaceCount] = pFaceChildren[vertInFace];
                cVertInFace[cVertFaceCount] = (LocalIndex)((pFaceSize == 4) ? vertInFace : 0);
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
QuadRefinement::populateVertexEdgeRelation() {

    //
    //  Notes on allocating/initializing the vertex-edge counts/offsets vector:
    //
    //  Be aware of scheme-specific decisions here, e.g.:
    //      - no verts from parent faces for Loop
    //      - more interior edges and faces for verts from parent edges for Loop
    //      - no guaranteed "neighborhood" around Bilinear verts from verts
    //
    //  If uniform subdivision, vert-edge count will be:
    //      - 4 or 0 for verts from parent faces (for catmark)
    //      - 2 + N or 2 + 2*N faces incident parent edge for verts from parent edges
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
    int childVertEdgeIndexSizeEstimate = (int)_parent->_faceVertIndices.size()
                                       + (int)_parent->_edgeFaceIndices.size() + _parent->getNumEdges() * 2
                                       + (int)_parent->_vertEdgeIndices.size();

    _child->_vertEdgeCountsAndOffsets.resize(_child->getNumVertices() * 2);
    _child->_vertEdgeIndices.resize(         childVertEdgeIndexSizeEstimate);
    _child->_vertEdgeLocalIndices.resize(    childVertEdgeIndexSizeEstimate);

    if (getFirstChildVertexFromVertices() == 0) {
        populateVertexEdgesFromParentVertices();
        populateVertexEdgesFromParentFaces();
        populateVertexEdgesFromParentEdges();
    } else {
        populateVertexEdgesFromParentFaces();
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
QuadRefinement::populateVertexEdgesFromParentFaces() {

    for (int pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        int cVert = _faceChildVertIndex[pFace];
        if (!IndexIsValid(cVert)) continue;

        ConstIndexArray pFaceVerts = _parent->getFaceVertices(pFace),
                        pFaceChildEdges = getFaceChildEdges(pFace);

        //
        //  Reserve enough vert-edges, populate and trim to the actual size:
        //
        _child->resizeVertexEdges(cVert, pFaceVerts.size());

        IndexArray      cVertEdges  = _child->getVertexEdges(cVert);
        LocalIndexArray cVertInEdge = _child->getVertexEdgeLocalIndices(cVert);

        //
        //  Need to ensure correct ordering here when complete -- we want the "leading"
        //  edge of each child face first.  The child vert is in the center of a new
        //  face so new "boundaries" will only occur when the vertex is incomplete.
        //
        int cVertEdgeCount = 0;
        for (int j = 0; j < pFaceVerts.size(); ++j) {
            int jLeadingEdge = j ? (j - 1) : (pFaceVerts.size() - 1);
            if (IndexIsValid(pFaceChildEdges[jLeadingEdge])) {
                cVertEdges[cVertEdgeCount] = pFaceChildEdges[jLeadingEdge];
                cVertInEdge[cVertEdgeCount] = 0;
                cVertEdgeCount++;
            }
        }
        _child->trimVertexEdges(cVert, cVertEdgeCount);
    }
}
void
QuadRefinement::populateVertexEdgesFromParentEdges() {

    //
    //  This relation turns out to be awkward to populate given the mixed parentage
    //  of the incident edges of the child vertex of an edge -- two child edges
    //  originate from the parent edge while one or more will originate from the
    //  faces incident the parent edge.  The need to interleave these for proper
    //  CC-wise orientation is what really complicates this.
    //
    //  Unlike other relations, we generate the results and then re-order them as
    //  needed.  In this case we assign the first two incident edges as the child
    //  edges of the parent edge, followed then by those originating from a parent
    //  face.  We then swap the second and third (and possibly the first two) so
    //  that we have the desired origin sequence beginning [edge, face, edge, ...]
    //
    for (int pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
        int cVert = _edgeChildVertIndex[pEdge];
        if (!IndexIsValid(cVert)) continue;

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
        //  Identify and assign the first two child edges of the parent edge -- until
        //  we look more closely at the orientation of the parent edge in the first
        //  face we don't know what order these two should be in, so just assign them
        //  for now and swap them later if necessary:
        //
        int cVertEdgeCount = 0;

        if (IndexIsValid(pEdgeChildEdges[0])) {
            cVertEdges[cVertEdgeCount] = pEdgeChildEdges[0];
            cVertInEdge[cVertEdgeCount] = 0;
            cVertEdgeCount++;
        }
        if (IndexIsValid(pEdgeChildEdges[1])) {
            cVertEdges[cVertEdgeCount] = pEdgeChildEdges[1];
            cVertInEdge[cVertEdgeCount] = 0;
            cVertEdgeCount++;
        }

        //
        //  Append the interior edge of each incident parent face -- swapping the
        //  first face-edge with the second edge-edge just added to get the desired
        //  sequence of child edges originating from (edge, face0, edge, ...)
        //
        for (int i = 0; i < pEdgeFaces.size(); ++i) {
            Index pFace      = pEdgeFaces[i];
            int   edgeInFace = pEdgeInFace[i];

            Index cEdgeOfFace = getFaceChildEdges(pFace)[edgeInFace];

            if (IndexIsValid(cEdgeOfFace)) {
                cVertEdges[cVertEdgeCount] = cEdgeOfFace;
                cVertInEdge[cVertEdgeCount] = 1;
                cVertEdgeCount++;

                //  Check if swapping this first face-edge with the last edge-edge
                //  is necessary:
                if ((i == 0) && (cVertEdgeCount == 3)) {
                    //  Remember to order the first of the two child edges according
                    //  to the parent edge's orientation in this first face:
                    if ((pEdgeVerts[0] != pEdgeVerts[1]) &&
                            (_parent->getFaceVertices(pFace)[edgeInFace] == pEdgeVerts[0])) {
                        std::swap(cVertEdges[0],  cVertEdges[1]);
                        std::swap(cVertInEdge[0], cVertInEdge[1]);
                    }
                    std::swap(cVertEdges[1],  cVertEdges[2]);
                    std::swap(cVertInEdge[1], cVertInEdge[2]);
                }
            }
        }
        _child->trimVertexEdges(cVert, cVertEdgeCount);
    }
}
void
QuadRefinement::populateVertexEdgesFromParentVertices() {

    for (int pVert = 0; pVert < _parent->getNumVertices(); ++pVert) {
        int cVert = _vertChildVertIndex[pVert];
        if (!IndexIsValid(cVert)) continue;

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
            Index      pEdgeIndex  = pVertEdges[i];
            LocalIndex pEdgeVert = pVertInEdge[i];

            Index pEdgeChildIndex = getEdgeChildEdges(pEdgeIndex)[pEdgeVert];
            if (IndexIsValid(pEdgeChildIndex)) {
                cVertEdges[cVertEdgeCount] = pEdgeChildIndex;
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
QuadRefinement::markSparseFaceChildren() {

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
    assert(_splitType == Sdc::SPLIT_TO_QUADS);

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

        ConstIndexArray fVerts = parent().getFaceVertices(pFace);

        SparseTag& pFaceTag = _parentFaceTag[pFace];

        if (pFaceTag._selected) {
            for (int i = 0; i < fVerts.size(); ++i) {
                markSparseIndexSelected(fChildFaces[i]);
                markSparseIndexSelected(fChildEdges[i]);
            }
            markSparseIndexSelected(_faceChildVertIndex[pFace]);

            pFaceTag._transitional = 0;
        } else {
            int marked = false;

            for (int i = 0; i < fVerts.size(); ++i) {
                if (_parentVertexTag[fVerts[i]]._selected) {
                    int iPrev = i ? (i - 1) : (fVerts.size() - 1);

                    markSparseIndexNeighbor(fChildFaces[i]);

                    markSparseIndexNeighbor(fChildEdges[i]);
                    markSparseIndexNeighbor(fChildEdges[iPrev]);

                    marked = true;
                }
            }
            if (marked) {
                markSparseIndexNeighbor(_faceChildVertIndex[pFace]);

                //
                //  Assign selection and transitional tags to faces when required:
                //
                //  Only non-selected faces may be "transitional", and we need to inspect
                //  all tags on its boundary edges to be sure.  Since we're inspecting each
                //  now (and may need to later) retain the transitional state of each in a
                //  4-bit mask that reflects the full transitional topology for later.
                //
                ConstIndexArray fEdges = parent().getFaceEdges(pFace);
                if (fEdges.size() == 4) {
                    pFaceTag._transitional = (unsigned char)
                           ((_parentEdgeTag[fEdges[0]]._transitional << 0) |
                            (_parentEdgeTag[fEdges[1]]._transitional << 1) |
                            (_parentEdgeTag[fEdges[2]]._transitional << 2) |
                            (_parentEdgeTag[fEdges[3]]._transitional << 3));
                } else if (fEdges.size() == 3) {
                    pFaceTag._transitional = (unsigned char)
                           ((_parentEdgeTag[fEdges[0]]._transitional << 0) |
                            (_parentEdgeTag[fEdges[1]]._transitional << 1) |
                            (_parentEdgeTag[fEdges[2]]._transitional << 2));
                } else {
                    pFaceTag._transitional = 0;
                    for (int i = 0; i < fEdges.size(); ++i) {
                        pFaceTag._transitional |= _parentEdgeTag[fEdges[i]]._transitional;
                    }
                }
            }
        }
    }
}

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
