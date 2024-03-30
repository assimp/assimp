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
#include "../sdc/types.h"
#include "../sdc/crease.h"
#include "../vtr/array.h"
#include "../vtr/stackBuffer.h"
#include "../vtr/refinement.h"
#include "../vtr/fvarLevel.h"

#include "../vtr/fvarRefinement.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>


//
//  FVarRefinement:
//      Analogous to Refinement -- retains data to facilitate refinement and
//  population of refined face-varying data channels.
//
namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {
namespace internal {

//
//  Simple (for now) constructor and destructor:
//
FVarRefinement::FVarRefinement(Refinement const& refinement,
                               FVarLevel&        parentFVarLevel,
                               FVarLevel&        childFVarLevel) :
    _refinement(refinement),
    _parentLevel(refinement.parent()),
    _parentFVar(parentFVarLevel),
    _childLevel(refinement.child()),
    _childFVar(childFVarLevel) {
}

FVarRefinement::~FVarRefinement() {
}


//
// Methods supporting the refinement of face-varying data that has previously
// been applied to the Refinement member. So these methods already have access
// to fully refined child components.
//

void
FVarRefinement::applyRefinement() {

    //
    //  Transfer basic properties from the parent to child level:
    //
    _childFVar._options = _parentFVar._options;

    _childFVar._isLinear              = _parentFVar._isLinear;
    _childFVar._hasLinearBoundaries   = _parentFVar._hasLinearBoundaries;
    _childFVar._hasDependentSharpness = _parentFVar._hasDependentSharpness;

    //
    //  It's difficult to know immediately how many child values arise from the
    //  refinement -- particularly when sparse, so we get a close upper bound,
    //  resize for that number and trim when finished:
    //
    estimateAndAllocateChildValues();
    populateChildValues();
    trimAndFinalizeChildValues();

    propagateEdgeTags();
    propagateValueTags();
    if (_childFVar.hasSmoothBoundaries()) {
        propagateValueCreases();
        reclassifySemisharpValues();
    }

    //
    //  The refined face-values are technically redundant as they can be constructed
    //  from the face-vertex siblings -- do so here as a post-process
    //
    if (_childFVar.getNumValues() > _childLevel.getNumVertices()) {
        _childFVar.initializeFaceValuesFromVertexFaceSiblings();
    } else {
        _childFVar.initializeFaceValuesFromFaceVertices();
    }

    //printf("FVar refinement to level %d:\n", _childLevel.getDepth());
    //_childFVar.print();

    //printf("Validating refinement to level %d:\n", _childLevel.getDepth());
    //_childFVar.validate();
    //assert(_childFVar.validate());
}

//
//  Quickly estimate the memory required for face-varying vertex-values in the child
//  and allocate them.  For uniform refinement this estimate should exactly match the
//  desired result.  For sparse refinement the excess should generally be low as the
//  sparse boundary components generally occur where face-varying data is continuous.
//
void
FVarRefinement::estimateAndAllocateChildValues() {

    int maxVertexValueCount = _refinement.getNumChildVerticesFromFaces();

    Index cVert    = _refinement.getFirstChildVertexFromEdges();
    Index cVertEnd = cVert + _refinement.getNumChildVerticesFromEdges();
    for ( ; cVert < cVertEnd; ++cVert) {
        Index pEdge = _refinement.getChildVertexParentIndex(cVert);

        maxVertexValueCount += _parentFVar.edgeTopologyMatches(pEdge)
                             ? 1 : _parentLevel.getEdgeFaces(pEdge).size();
    }

    cVert    = _refinement.getFirstChildVertexFromVertices();
    cVertEnd = cVert + _refinement.getNumChildVerticesFromVertices();
    for ( ; cVert < cVertEnd; ++cVert) {
        assert(_refinement.isChildVertexComplete(cVert));
        Index pVert = _refinement.getChildVertexParentIndex(cVert);

        maxVertexValueCount += _parentFVar.getNumVertexValues(pVert);
    }

    //
    //  Now allocate/initialize for the maximum -- use resize() and trim the size later
    //  to avoid the constant growing with reserve() and incremental sizing.  We know
    //  the estimate should be close and memory wasted should be small, so initialize
    //  all to zero as well to avoid writing in all but affected areas:
    //
    //  Resize vectors that mirror the component counts:
    _childFVar.resizeComponents();

    //  Resize the vertex-value tags in the child level:
    _childFVar._vertValueTags.resize(maxVertexValueCount);

    //  Resize the vertex-value "parent source" mapping in the refinement:
    _childValueParentSource.resize(maxVertexValueCount, 0);
}

void
FVarRefinement::trimAndFinalizeChildValues() {

    _childFVar._vertValueTags.resize(_childFVar._valueCount);
    if (_childFVar.hasSmoothBoundaries()) {
        _childFVar._vertValueCreaseEnds.resize(_childFVar._valueCount);
    }

    _childValueParentSource.resize(_childFVar._valueCount);

    //  Allocate and initialize the vector of indices (redundant after level 0):
    _childFVar._vertValueIndices.resize(_childFVar._valueCount);
    for (int i = 0; i < _childFVar._valueCount; ++i) {
        _childFVar._vertValueIndices[i] = i;
    }
}

inline int
FVarRefinement::populateChildValuesForEdgeVertex(Index cVert, Index pEdge) {

    //
    //  Determine the number of sibling values for the child vertex of this discts
    //  edge and populate their related topological data (e.g. source face).
    //
    //  This turns out to be very simple.  For FVar refinement to handle all cases
    //  of non-manifold edges, when an edge is discts we generate a FVar value for
    //  each face incident the edge.  So in the uniform refinement case we will
    //  have as many child values as parent faces incident the edge.  But even when
    //  refinement is sparse, if this edge-vertex is not complete, we will still be
    //  guaranteed that a child face exists for each parent face since one of the
    //  edge's end vertices must be complete and therefore include all child faces.
    //
    ConstIndexArray  pEdgeFaces = _parentLevel.getEdgeFaces(pEdge);
    if (pEdgeFaces.size() == 1) {
        //  No sibling so the first face (0) guaranteed to be a source and all
        //  sibling indices per incident face will also be 0 -- all of which was
        //  done on initialization, so nothing further to do.
        return 1;
    }

    //
    //  Update the parent-source of all child values:
    //
    int   cValueCount  = pEdgeFaces.size();
    Index cValueOffset = _childFVar.getVertexValueOffset(cVert);

    for (int i = 0; i < cValueCount; ++i) {
        _childValueParentSource[cValueOffset + i] = (LocalIndex) i;
    }

    //
    //  Update the vertex-face siblings for the faces incident the child vertex:
    //
    ConstIndexArray         cVertFaces        = _childLevel.getVertexFaces(cVert);
    FVarLevel::SiblingArray cVertFaceSiblings = _childFVar.getVertexFaceSiblings(cVert);

    assert(cVertFaces.size() == cVertFaceSiblings.size());
    assert(cVertFaces.size() >= cValueCount);

    for (int i = 0; i < cVertFaceSiblings.size(); ++i) {
        Index pFaceI = _refinement.getChildFaceParentFace(cVertFaces[i]);
        if (pEdgeFaces.size() == 2) {
            //  Only two parent faces and all siblings previously initialized to 0:
            if (pFaceI == pEdgeFaces[1]) {
                cVertFaceSiblings[i] = (LocalIndex) 1;
            }
        } else {
            //  Non-manifold case with > 2 parent faces -- match child faces to parent:
            for (int j = 0; j < pEdgeFaces.size(); ++j) {
                if (pFaceI == pEdgeFaces[j]) {
                    cVertFaceSiblings[i] = (LocalIndex) j;
                }
            }
        }
    }
    return cValueCount;
}

inline int
FVarRefinement::populateChildValuesForVertexVertex(Index cVert, Index pVert) {

    //
    //  We should not be getting incomplete vertex-vertices from feature-adaptive
    //  refinement (as neighboring vertices will be face-vertices or edge-vertices).
    //  This will get messy when we do (i.e. sparse refinement of Bilinear or more
    //  flexible and specific sparse refinement of Catmark) but for now assume 1-to-1.
    //
    assert(_refinement.isChildVertexComplete(cVert));

    //  Number of child values is same as number of parent values since complete:
    int cValueCount = _parentFVar.getNumVertexValues(pVert);

    if (cValueCount > 1) {
        Index cValueIndex = _childFVar.getVertexValueOffset(cVert);

        // Update the parent source for all child values:
        for (int j = 1; j < cValueCount; ++j) {
            _childValueParentSource[cValueIndex + j] = (LocalIndex) j;
        }

        // Update the vertex-face siblings:
        FVarLevel::ConstSiblingArray pVertFaceSiblings = _parentFVar.getVertexFaceSiblings(pVert);
        FVarLevel::SiblingArray      cVertFaceSiblings = _childFVar.getVertexFaceSiblings(cVert);
        for (int j = 0; j < cVertFaceSiblings.size(); ++j) {
            cVertFaceSiblings[j] = pVertFaceSiblings[j];
        }
    }
    return cValueCount;
}

void
FVarRefinement::populateChildValues() {

    //
    //  Be sure to match the same vertex ordering as Refinement, i.e. face-vertices
    //  first vs vertex-vertices first, etc.  A few optimizations within the use of
    //  face-varying data take advantage of this assumption, and it just makes sense
    //  to be consistent (e.g. if there is a 1-to-1 correspondence between vertices
    //  and their FVar-values, their children will correspond).
    //
    _childFVar._valueCount = 0;

    if (_refinement.hasFaceVerticesFirst()) {
        populateChildValuesFromFaceVertices();
        populateChildValuesFromEdgeVertices();
        populateChildValuesFromVertexVertices();
    } else {
        populateChildValuesFromVertexVertices();
        populateChildValuesFromFaceVertices();
        populateChildValuesFromEdgeVertices();
    }
}

void
FVarRefinement::populateChildValuesFromFaceVertices() {

    Index cVert    = _refinement.getFirstChildVertexFromFaces();
    Index cVertEnd = cVert + _refinement.getNumChildVerticesFromFaces();
    for ( ; cVert < cVertEnd; ++cVert) {
        _childFVar._vertSiblingOffsets[cVert] = _childFVar._valueCount;
        _childFVar._vertSiblingCounts[cVert]  = 1;
        _childFVar._valueCount ++;
    }
}
void
FVarRefinement::populateChildValuesFromEdgeVertices() {

    Index cVert    = _refinement.getFirstChildVertexFromEdges();
    Index cVertEnd = cVert + _refinement.getNumChildVerticesFromEdges();
    for ( ; cVert < cVertEnd; ++cVert) {
        Index pEdge = _refinement.getChildVertexParentIndex(cVert);

        _childFVar._vertSiblingOffsets[cVert] = _childFVar._valueCount;
        if (_parentFVar.edgeTopologyMatches(pEdge)) {
            _childFVar._vertSiblingCounts[cVert] = 1;
            _childFVar._valueCount ++;
        } else {
            int cValueCount = populateChildValuesForEdgeVertex(cVert, pEdge);
            _childFVar._vertSiblingCounts[cVert] = (LocalIndex)cValueCount;
            _childFVar._valueCount += cValueCount;
        }
    }
}
void
FVarRefinement::populateChildValuesFromVertexVertices() {

    Index cVert    = _refinement.getFirstChildVertexFromVertices();
    Index cVertEnd = cVert + _refinement.getNumChildVerticesFromVertices();
    for ( ; cVert < cVertEnd; ++cVert) {
        Index pVert = _refinement.getChildVertexParentIndex(cVert);

        _childFVar._vertSiblingOffsets[cVert] = _childFVar._valueCount;
        if (_parentFVar.valueTopologyMatches(_parentFVar.getVertexValueOffset(pVert))) {
            _childFVar._vertSiblingCounts[cVert] = 1;
            _childFVar._valueCount ++;
        } else {
            int cValueCount = populateChildValuesForVertexVertex(cVert, pVert);
            _childFVar._vertSiblingCounts[cVert] = (LocalIndex)cValueCount;
            _childFVar._valueCount += cValueCount;
        }
    }
}

void
FVarRefinement::propagateEdgeTags() {

    //
    //  Edge tags correspond to child edges and originate from faces or edges:
    //      Face-edges:
    //          - tag can be initialized as cts (*)
    //              * what was this comment:  "discts based on parent face-edges at ends"
    //      Edge-edges:
    //          - tag propagated from parent edge
    //          - need to modify if parent edge was discts at one end
    //              - child edge for the matching end inherits tag
    //              - child edge at the other end is doubly discts
    //
    FVarLevel::ETag eTagMatch;
    eTagMatch.clear();
    eTagMatch._mismatch = false;

    for (int eIndex = 0; eIndex < _refinement.getNumChildEdgesFromFaces(); ++eIndex) {
        _childFVar._edgeTags[eIndex] = eTagMatch;
    }
    for (int eIndex = _refinement.getNumChildEdgesFromFaces(); eIndex < _childLevel.getNumEdges(); ++eIndex) {
        Index pEdge = _refinement.getChildEdgeParentIndex(eIndex);

        _childFVar._edgeTags[eIndex] = _parentFVar._edgeTags[pEdge];
    }
}

void
FVarRefinement::propagateValueTags() {

    //
    //  Value tags correspond to vertex-values and originate from all three sources:
    //      Face-values:
    //          - trivially initialized as matching
    //      Edge-values:
    //          - conditionally initialized based on parent edge continuity
    //          - should be trivial though (unlike edge-tags for the child edges)
    //      Vertex-values:
    //          - if complete, trivially propagated/inherited
    //          - if incomplete, need to map to child subset
    //

    //
    //  Values from face-vertices -- all match and are sequential:
    //
    FVarLevel::ValueTag valTagMatch;
    valTagMatch.clear();

    Index cVert      = _refinement.getFirstChildVertexFromFaces();
    Index cVertEnd   = cVert + _refinement.getNumChildVerticesFromFaces();
    Index cVertValue = _childFVar.getVertexValueOffset(cVert);
    for ( ; cVert < cVertEnd; ++cVert, ++cVertValue) {
        _childFVar._vertValueTags[cVertValue] = valTagMatch;
    }

    //
    //  Values from edge-vertices -- for edges that are split, tag as mismatched and tag
    //  as corner or crease depending on the presence of creases in the parent:
    //
    FVarLevel::ValueTag valTagMismatch = valTagMatch;
    valTagMismatch._mismatch = true;

    FVarLevel::ValueTag valTagCrease = valTagMismatch;
    valTagCrease._crease = true;

    FVarLevel::ValueTag& valTagSplitEdge = _parentFVar.hasSmoothBoundaries() ? valTagCrease : valTagMismatch;

    cVert    = _refinement.getFirstChildVertexFromEdges();
    cVertEnd = cVert + _refinement.getNumChildVerticesFromEdges();
    for ( ; cVert < cVertEnd; ++cVert) {
        Index pEdge = _refinement.getChildVertexParentIndex(cVert);

        FVarLevel::ValueTagArray cValueTags = _childFVar.getVertexValueTags(cVert);

        FVarLevel::ETag pEdgeTag = _parentFVar._edgeTags[pEdge];
        if (pEdgeTag._mismatch || pEdgeTag._linear) {
            std::fill(cValueTags.begin(), cValueTags.end(), valTagSplitEdge);
        } else {
            std::fill(cValueTags.begin(), cValueTags.end(), valTagMatch);
        }
    }

    //
    //  Values from vertex-vertices -- inherit tags from parent values when complete
    //  otherwise (not yet supported) need to identify the parent value for each child:
    //
    cVert    = _refinement.getFirstChildVertexFromVertices();
    cVertEnd = cVert + _refinement.getNumChildVerticesFromVertices();

    for ( ; cVert < cVertEnd; ++cVert) {
        Index pVert = _refinement.getChildVertexParentIndex(cVert);
        assert(_refinement.isChildVertexComplete(cVert));

        FVarLevel::ConstValueTagArray pValueTags = _parentFVar.getVertexValueTags(pVert);
        FVarLevel::ValueTagArray cValueTags = _childFVar.getVertexValueTags(cVert);

        memcpy(cValueTags.begin(), pValueTags.begin(),
            pValueTags.size()*sizeof(FVarLevel::ValueTag));
    }
}

void
FVarRefinement::propagateValueCreases() {

    assert(_childFVar.hasSmoothBoundaries());

    //  Skip child vertices from faces:

    //
    //  For each child vertex from an edge that has FVar values and is complete, initialize
    //  the crease-ends for those values tagged as smooth boundaries
    //
    //  Note that this does depend on the nature of the topological split, i.e. how many
    //  child faces are incident the new child vertex for each face that becomes a crease,
    //  so identify constants to be used in each iteration first:
    //
    int incChildFacesPerEdge = (_refinement.getRegularFaceSize() == 4) ? 2 : 3;

    Index cVert    = _refinement.getFirstChildVertexFromEdges();
    Index cVertEnd = cVert + _refinement.getNumChildVerticesFromEdges();
    for ( ; cVert < cVertEnd; ++cVert) {
        FVarLevel::ValueTagArray cValueTags = _childFVar.getVertexValueTags(cVert);

        if (!cValueTags[0].isMismatch()) continue;
        if (!_refinement.isChildVertexComplete(cVert)) continue;

        FVarLevel::CreaseEndPairArray cValueCreaseEnds = _childFVar.getVertexValueCreaseEnds(cVert);

        int creaseStartFace = 0;
        int creaseEndFace = creaseStartFace + incChildFacesPerEdge - 1;

        for (int i = 0; i < cValueTags.size(); ++i) {
            if (!cValueTags[i].isInfSharp()) {
                cValueCreaseEnds[i]._startFace = (LocalIndex) creaseStartFace;
                cValueCreaseEnds[i]._endFace   = (LocalIndex) creaseEndFace;
            }
            creaseStartFace += incChildFacesPerEdge;
            creaseEndFace   += incChildFacesPerEdge;
        }
    }

    //
    //  For each child vertex from a vertex that has FVar values and is complete, initialize
    //  the crease-ends for those values tagged as smooth or semi-sharp (to become smooth
    //  eventually):
    //
    cVert    = _refinement.getFirstChildVertexFromVertices();
    cVertEnd = cVert + _refinement.getNumChildVerticesFromVertices();
    for ( ; cVert < cVertEnd; ++cVert) {
        FVarLevel::ValueTagArray cValueTags = _childFVar.getVertexValueTags(cVert);

        if (!cValueTags[0].isMismatch()) continue;
        if (!_refinement.isChildVertexComplete(cVert)) continue;

        Index pVert = _refinement.getChildVertexParentIndex(cVert);

        FVarLevel::ConstCreaseEndPairArray pCreaseEnds = _parentFVar.getVertexValueCreaseEnds(pVert);
        FVarLevel::CreaseEndPairArray cCreaseEnds = _childFVar.getVertexValueCreaseEnds(cVert);

        for (int j = 0; j < cValueTags.size(); ++j) {
            if (!cValueTags[j].isInfSharp()) {
                cCreaseEnds[j] = pCreaseEnds[j];
            }
        }
    }
}

void
FVarRefinement::reclassifySemisharpValues() {

    //
    //  Reclassify the tags of semi-sharp vertex values to smooth creases according to
    //  changes in sharpness:
    //
    //  Vertex values introduced on edge-verts can never be semi-sharp as they will be
    //  introduced on discts edges, which are implicitly infinitely sharp, so we can
    //  skip them entirely.
    //
    //  So we just need to deal with those values descended from parent vertices that
    //  were semi-sharp.  The child values will have inherited the semi-sharp tag from
    //  their parent values -- we will be able to clear it in many simple cases but
    //  ultimately will need to inspect each value:
    //
    bool hasDependentSharpness = _parentFVar._hasDependentSharpness;

    internal::StackBuffer<Index,16> cVertEdgeBuffer(_childLevel.getMaxValence());

    Index cVert    = _refinement.getFirstChildVertexFromVertices();
    Index cVertEnd = cVert + _refinement.getNumChildVerticesFromVertices();

    for ( ; cVert < cVertEnd; ++cVert) {
        FVarLevel::ValueTagArray cValueTags = _childFVar.getVertexValueTags(cVert);

        if (!cValueTags[0].isMismatch()) continue;
        if (!_refinement.isChildVertexComplete(cVert)) continue;

        //  If the parent vertex wasn't semi-sharp, the child vertex and values can't be:
        Index       pVert     = _refinement.getChildVertexParentIndex(cVert);
        Level::VTag pVertTags = _parentLevel.getVertexTag(pVert);

        if (!pVertTags._semiSharp && !pVertTags._semiSharpEdges) continue;

        //  If the child vertex is still sharp, all values remain unaffected:
        Level::VTag cVertTags = _childLevel.getVertexTag(cVert);

        if (cVertTags._semiSharp || cVertTags._infSharp) continue;

        //  If the child is no longer semi-sharp, we can just clear those values marked
        //  (i.e. make them creases, others may remain corners) and continue:
        //
        if (!cVertTags._semiSharp && !cVertTags._semiSharpEdges) {
            for (int j = 0; j < cValueTags.size(); ++j) {
                if (cValueTags[j]._semiSharp) {
                    cValueTags[j]._semiSharp = false;
                    cValueTags[j]._depSharp = false;
                    cValueTags[j]._crease = true;
                }
            }
            continue;
        }

        //  There are some semi-sharp edges left -- for those values tagged as semi-sharp,
        //  see if they are still semi-sharp and clear those that are not:
        //
        FVarLevel::CreaseEndPairArray const cValueCreaseEnds = _childFVar.getVertexValueCreaseEnds(cVert);

        //  Beware accessing the child's vert-edges -- full topology may not be enabled:
        ConstIndexArray cVertEdges;
        if (_childLevel.getNumVertexEdgesTotal()) {
            cVertEdges = _childLevel.getVertexEdges(cVert);
        } else {
            ConstIndexArray      pVertEdges  = _parentLevel.getVertexEdges(pVert);
            ConstLocalIndexArray pVertInEdge = _parentLevel.getVertexEdgeLocalIndices(pVert);
            for (int i = 0; i < pVertEdges.size(); ++i) {
                cVertEdgeBuffer[i] = _refinement.getEdgeChildEdges(pVertEdges[i])[pVertInEdge[i]];
            }
            cVertEdges = IndexArray(cVertEdgeBuffer, pVertEdges.size());
        }

        for (int j = 0; j < cValueTags.size(); ++j) {
            if (cValueTags[j]._semiSharp && !cValueTags[j]._depSharp) {
                LocalIndex vStartFace = cValueCreaseEnds[j]._startFace;
                LocalIndex vEndFace   = cValueCreaseEnds[j]._endFace;

                bool isStillSemiSharp = false;
                if (vEndFace > vStartFace) {
                    for (int k = vStartFace + 1; !isStillSemiSharp && (k <= vEndFace); ++k) {
                        isStillSemiSharp = _childLevel.getEdgeTag(cVertEdges[k])._semiSharp;
                    }
                } else if (vStartFace > vEndFace) {
                    for (int k = vStartFace + 1; !isStillSemiSharp && (k < cVertEdges.size()); ++k) {
                        isStillSemiSharp = _childLevel.getEdgeTag(cVertEdges[k])._semiSharp;
                    }
                    for (int k = 0; !isStillSemiSharp && (k <= vEndFace); ++k) {
                        isStillSemiSharp = _childLevel.getEdgeTag(cVertEdges[k])._semiSharp;
                    }
                }
                if (!isStillSemiSharp) {
                    cValueTags[j]._semiSharp = false;
                    cValueTags[j]._depSharp = false;
                    cValueTags[j]._crease = true;
                }
            }
        }

        //
        //  Now account for "dependent sharpness" (only matters when we have two values) --
        //  if one value was dependent/sharpened based on the other, clear the dependency
        //  tag if it is no longer sharp:
        //
        if ((cValueTags.size() == 2) && hasDependentSharpness) {
            if (cValueTags[0]._depSharp && !cValueTags[1]._semiSharp) {
                cValueTags[0]._depSharp = false;
            } else if (cValueTags[1]._depSharp && !cValueTags[0]._semiSharp) {
                cValueTags[1]._depSharp = false;
            }
        }
    }
}

float
FVarRefinement::getFractionalWeight(Index pVert, LocalIndex pSibling,
                                    Index cVert, LocalIndex /* cSibling */) const {

    //
    //  Need to identify sharpness values for edges within the spans for both the
    //  parent and child...
    //
    //  Consider gathering the complete parent and child sharpness vectors outside
    //  this method and re-using them for each sibling, i.e. passing them to this
    //  method somehow.  We may also need them there for mask-related purposes...
    //
    internal::StackBuffer<Index,16> cVertEdgeBuffer;

    ConstIndexArray pVertEdges = _parentLevel.getVertexEdges(pVert);
    ConstIndexArray cVertEdges;

    //  Beware accessing the child's vert-edges -- full topology may not be enabled:
    if (_childLevel.getNumVertexEdgesTotal()) {
        cVertEdges = _childLevel.getVertexEdges(cVert);
    } else {
        cVertEdgeBuffer.SetSize(pVertEdges.size());

        ConstLocalIndexArray pVertInEdge = _parentLevel.getVertexEdgeLocalIndices(pVert);
        for (int i = 0; i < pVertEdges.size(); ++i) {
            cVertEdgeBuffer[i] = _refinement.getEdgeChildEdges(pVertEdges[i])[pVertInEdge[i]];
        }
        cVertEdges = IndexArray(cVertEdgeBuffer, pVertEdges.size());
    }
 
    internal::StackBuffer<float,32> sharpnessBuffer(2 * pVertEdges.size());
    float * pEdgeSharpness = sharpnessBuffer;
    float * cEdgeSharpness = sharpnessBuffer + pVertEdges.size();

    FVarLevel::CreaseEndPair pValueCreaseEnds = _parentFVar.getVertexValueCreaseEnds(pVert)[pSibling];

    LocalIndex pStartFace = pValueCreaseEnds._startFace;
    LocalIndex pEndFace   = pValueCreaseEnds._endFace;

    int interiorEdgeCount = 0;
    if (pEndFace > pStartFace) {
        for (int i = pStartFace + 1; i <= pEndFace; ++i, ++interiorEdgeCount) {
            pEdgeSharpness[interiorEdgeCount] = _parentLevel.getEdgeSharpness(pVertEdges[i]);
            cEdgeSharpness[interiorEdgeCount] = _childLevel.getEdgeSharpness(cVertEdges[i]);
        }
    } else if (pStartFace > pEndFace) {
        for (int i = pStartFace + 1; i < pVertEdges.size(); ++i, ++interiorEdgeCount) {
            pEdgeSharpness[interiorEdgeCount] = _parentLevel.getEdgeSharpness(pVertEdges[i]);
            cEdgeSharpness[interiorEdgeCount] = _childLevel.getEdgeSharpness(cVertEdges[i]);
        }
        for (int i = 0; i <= pEndFace; ++i, ++interiorEdgeCount) {
            pEdgeSharpness[interiorEdgeCount] = _parentLevel.getEdgeSharpness(pVertEdges[i]);
            cEdgeSharpness[interiorEdgeCount] = _childLevel.getEdgeSharpness(cVertEdges[i]);
        }
    }
    return Sdc::Crease(_refinement.getOptions()).ComputeFractionalWeightAtVertex(
            _parentLevel.getVertexSharpness(pVert), _childLevel.getVertexSharpness(cVert),
            interiorEdgeCount, pEdgeSharpness, cEdgeSharpness);
}

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
