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
#include "../vtr/level.h"

#include "../vtr/fvarLevel.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>


//
//  FVarLevel:
//      Simple container of face-varying topology, associated with a particular
//  level.  It is typically constructed and initialized similarly to levels -- the
//  base level in a Factory and subsequent levels by refinement.
//
namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {
namespace internal {


//
//  Information about the "span" for a face-varying value -- the set of faces
//  that share face-varying continuous edges around their common vertex.
//
//  This is intended for transient internal use only when analyzing the base
//  level topology.  Information gathered for a single span is translated into
//  topology tags for the value (ValueTag) which classify the value and persist
//  in the FVarLevel for later refinement and analysis.  The ValueSpan exists
//  solely to derive the ValueTag and is not intended (or capable) of capturing
//  the full topological extent of many spans.
//
struct FVarLevel::ValueSpan {
    LocalIndex _size;
    LocalIndex _start;
    LocalIndex _disctsEdgeCount;
    LocalIndex _semiSharpEdgeCount;
    LocalIndex _infSharpEdgeCount;
};


//
//  Simple (for now) constructor and destructor:
//
FVarLevel::FVarLevel(Level const& level) :
    _level(level),
    _isLinear(false),
    _hasLinearBoundaries(false),
    _hasDependentSharpness(false),
    _valueCount(0) {
}

FVarLevel::~FVarLevel() {
}

//
//  Initialization and sizing methods to allocate space:
//
void
FVarLevel::setOptions(Sdc::Options const& options) {
    _options = options;
}

void
FVarLevel::resizeComponents() {

    //  Per-face members:
    _faceVertValues.resize(_level.getNumFaceVerticesTotal());

    //  Per-edge members:
    ETag edgeTagMatch;
    edgeTagMatch.clear();
    _edgeTags.resize(_level.getNumEdges(), edgeTagMatch);

    //  Per-vertex members:
    _vertSiblingCounts.resize(_level.getNumVertices());
    _vertSiblingOffsets.resize(_level.getNumVertices());

    _vertFaceSiblings.resize(_level.getNumVertexFacesTotal(), 0);
}

void
FVarLevel::resizeVertexValues(int vertexValueCount) {

    _vertValueIndices.resize(vertexValueCount);

    ValueTag valueTagMatch;
    valueTagMatch.clear();
    _vertValueTags.resize(vertexValueCount, valueTagMatch);

    if (hasCreaseEnds()) {
        _vertValueCreaseEnds.resize(vertexValueCount);
    }
}

void
FVarLevel::resizeValues(int valueCount) {
    _valueCount = valueCount;
}


//
//  Initialize the component tags once all face-values have been assigned...
//
//  Constructing the mapping between vertices and their face-varying values involves:
//
//      - iteration through all vertices to mark edge discontinuities and classify
//      - allocation of vectors mapping vertices to their multiple (sibling) values
//      - iteration through all vertices and their distinct values to tag topologically
//
//  Once values have been identified for each vertex and tagged, refinement propagates
//  the tags to child values using more simplified logic (child values inherit the
//  topology of their parent) and no further analysis is required.
//
void
FVarLevel::completeTopologyFromFaceValues(int regularBoundaryValence) {

    //
    //  Assign some members and local variables based on the interpolation options (the
    //  members support queries that are expected later):
    //
    //  Given the growing number of options and behaviors to support, this is likely going
    //  to get another pass.  It may be worth identifying the behavior for each "feature",
    //  i.e. determine smooth or sharp for corners, creases and darts, but the fact that
    //  the rule for one value may be dependent on that of another complicates this.
    //
    using Sdc::Options;

    Options::VtxBoundaryInterpolation geomOptions = _options.GetVtxBoundaryInterpolation();
    Options::FVarLinearInterpolation  fvarOptions = _options.GetFVarLinearInterpolation();

    _isLinear = (fvarOptions == Options::FVAR_LINEAR_ALL);

    _hasLinearBoundaries = (fvarOptions == Options::FVAR_LINEAR_ALL) ||
                           (fvarOptions == Options::FVAR_LINEAR_BOUNDARIES);

    _hasDependentSharpness = (fvarOptions == Options::FVAR_LINEAR_CORNERS_PLUS1) ||
                             (fvarOptions == Options::FVAR_LINEAR_CORNERS_PLUS2);

    bool geomCornersAreSmooth = (geomOptions != Options::VTX_BOUNDARY_EDGE_AND_CORNER);
    bool fvarCornersAreSharp  = (fvarOptions != Options::FVAR_LINEAR_NONE);

    bool makeSmoothCornersSharp = geomCornersAreSmooth && fvarCornersAreSharp;

    bool sharpenBothIfOneCorner  = (fvarOptions == Options::FVAR_LINEAR_CORNERS_PLUS2);

    bool sharpenDarts = sharpenBothIfOneCorner || _hasLinearBoundaries;


    //
    //  It's awkward and potentially inefficient to try and accomplish everything in one
    //  pass over the vertices...
    //
    //  Make a first pass through the vertices to identify discts edges and to determine
    //  the number of values-per-vertex for subsequent allocation.  The presence of a
    //  discts edge warrants marking vertices at BOTH ends as having mismatched topology
    //  wrt the vertices (part of why full topological analysis is deferred).
    //
    //  So this first pass will allocate/initialize the overall structure of the topology.
    //  Given N vertices and M (as yet unknown) sibling values, the first pass achieves
    //  the following:
    //
    //      - assigns a local vector indicating which of the N vertices "match"
    //          - requires a single value but must also have no discts incident edges
    //      - determines the number of values associated with each of the N vertices
    //      - assigns an offset to the first value for each of the N vertices
    //      - initializes the vert-face "siblings" for all N vertices
    //  and
    //      - tags any incident edges as discts
    //
    //  The second pass initializes remaining members based on the total number of siblings
    //  M after allocating appropriate vectors dependent on M.
    //
    std::vector<LocalIndex> vertexMismatch(_level.getNumVertices(), 0);

    _vertFaceSiblings.resize(_level.getNumVertexFacesTotal(), 0);

    int const maxValence = _level.getMaxValence();

    internal::StackBuffer<Index,16>     indexBuffer(maxValence);
    internal::StackBuffer<int,16>       valueBuffer(maxValence);
    internal::StackBuffer<Sibling,16>   siblingBuffer(maxValence);
    internal::StackBuffer<ValueSpan,16> spanBuffer(maxValence);

    int *     uniqueValues   = valueBuffer;
    Sibling * vValueSiblings = siblingBuffer;

    int totalValueCount = 0;
    for (int vIndex = 0; vIndex < _level.getNumVertices(); ++vIndex) {
        //
        //  Retrieve the FVar values from each incident face and store locally for
        //  use -- we will identify the index of its corresponding "sibling" as we
        //  inspect them more closely later:
        //
        ConstIndexArray       vFaces  = _level.getVertexFaces(vIndex);
        ConstLocalIndexArray  vInFace = _level.getVertexFaceLocalIndices(vIndex);

        Index * vValues = indexBuffer;

        for (int i = 0; i < vFaces.size(); ++i) {
            vValues[i] = _faceVertValues[_level.getOffsetOfFaceVertices(vFaces[i]) + vInFace[i]];
        }

        //
        //  Inspect the incident edges of the vertex and tag those whose FVar values are
        //  discts between the two (or more) faces sharing that edge.  When manifold, we
        //  know an edge is discts when two successive fvar-values differ -- so we will
        //  make use of the local buffer of values.  Unfortunately we can't infer anything
        //  about the edges for a non-manifold vertex, so that case will be more complex.
        //
        ConstIndexArray       vEdges  = _level.getVertexEdges(vIndex);
        ConstLocalIndexArray  vInEdge = _level.getVertexEdgeLocalIndices(vIndex);

        bool vIsManifold = !_level.getVertexTag(vIndex)._nonManifold;
        bool vIsBoundary = _level.getVertexTag(vIndex)._boundary;

        if (vIsManifold) {
            //
            //  We want to use face indices here as we are accessing the fvar-values per
            //  face.  The indexing range here maps to the interior edges for boundary
            //  and interior verts:
            //
            for (int i = vIsBoundary; i < vFaces.size(); ++i) {
                int vFaceNext = i;
                int vFacePrev = i ? (i - 1) : (vFaces.size() - 1);

                if (vValues[vFaceNext] != vValues[vFacePrev]) {
                    Index eIndex = vEdges[i];

                    //  Tag both end vertices as not matching topology:
                    ConstIndexArray eVerts = _level.getEdgeVertices(eIndex);
                    vertexMismatch[eVerts[0]] = true;
                    vertexMismatch[eVerts[1]] = true;

                    //  Tag the corresponding edge as discts:
                    ETag& eTag = _edgeTags[eIndex];

                    eTag._disctsV0 = (eVerts[0] == vIndex);
                    eTag._disctsV1 = (eVerts[1] == vIndex);
                    eTag._mismatch = true;
                    eTag._linear = (ETag::ETagSize) _hasLinearBoundaries;
                }
            }
        } else if (vFaces.size() > 0) {
            //
            //  Unfortunately for non-manifold cases we can't make as much use of the
            //  retrieved face-values as there is no correlation between the incident
            //  edge and face lists.  So inspect each edge for continuity between its
            //  faces in general -- which is awkward (and what we were hoping to avoid
            //  by doing the overall vertex traversal to begin with):
            //
            for (int i = 0; i < vEdges.size(); ++i) {
                Index eIndex = vEdges[i];

                ConstIndexArray eFaces  = _level.getEdgeFaces(eIndex);
                if (eFaces.size() < 2) continue;

                ConstLocalIndexArray eInFace = _level.getEdgeFaceLocalIndices(eIndex);
                ConstIndexArray      eVerts  = _level.getEdgeVertices(eIndex);

                int   vertInEdge = vInEdge[i];
                bool  markEdgeDiscts = false;
                Index valueIndexInFace0 = 0;
                for (int j = 0; !markEdgeDiscts && (j < eFaces.size()); ++j) {
                    Index           fIndex  = eFaces[j];
                    ConstIndexArray fVerts  = _level.getFaceVertices(fIndex);
                    ConstIndexArray fValues = getFaceValues(fIndex);

                    int edgeInFace   = eInFace[j];
                    int edgeReversed = (eVerts[0] != fVerts[edgeInFace]);
                    int vertInFace   = edgeInFace + (vertInEdge != edgeReversed);
                    if (vertInFace == fVerts.size()) vertInFace = 0;

                    if (j == 0) {
                        valueIndexInFace0 = fValues[vertInFace];
                    } else {
                        markEdgeDiscts = (fValues[vertInFace] != valueIndexInFace0);
                    }
                }
                if (markEdgeDiscts) {
                    //  Tag both end vertices as not matching topology:
                    vertexMismatch[eVerts[0]] = true;
                    vertexMismatch[eVerts[1]] = true;

                    //  Tag the corresponding edge as discts:
                    ETag& eTag = _edgeTags[eIndex];

                    eTag._disctsV0 = (eVerts[0] == vIndex);
                    eTag._disctsV1 = (eVerts[1] == vIndex);
                    eTag._mismatch = true;
                    eTag._linear = (ETag::ETagSize) _hasLinearBoundaries;
                }
            }
        }

        //
        //  While we've tagged the vertex as having mismatched FVar topology in the presence of
        //  any discts edges, we also need to account for different treatment of vertices along
        //  geometric boundaries if the FVar interpolation rules affect them.  So inspect all
        //  boundary vertices that have not already been tagged.
        //
        if (vIsBoundary && !vertexMismatch[vIndex]) {
            if (_hasLinearBoundaries && (vFaces.size() > 0)) {
                vertexMismatch[vIndex] = true;

                if (vIsManifold) {
                    _edgeTags[vEdges[0]]._linear = true;
                    _edgeTags[vEdges[vEdges.size()-1]]._linear = true;
                } else {
                    for (int i = 0; i < vEdges.size(); ++i) {
                        if (_level.getEdgeTag(vEdges[i])._boundary) {
                            _edgeTags[vEdges[i]]._linear = true;
                        }
                    }
                }
            } else if (vFaces.size() == 1) {
                if (makeSmoothCornersSharp) {
                    vertexMismatch[vIndex] = true;
                }
            }
        }

        //
        //  Inspect the set of fvar-values around the vertex to identify the number of
        //  unique values.  While doing so, associate a "sibling index" (over the range
        //  of unique values) with each value around the vertex (this latter need makes
        //  it harder to make simple use of std::sort() and uniq() on the set of values)
        //
        int uniqueValueCount = 1;

        uniqueValues[0] = vValues[0];
        vValueSiblings[0] = 0;

        for (int i = 1; i < vFaces.size(); ++i) {
            if (vValues[i] == vValues[i-1]) {
                vValueSiblings[i] = vValueSiblings[i-1];
            } else {
                //  Add the "new" value if not already present -- unless found, the
                //  sibling index will be for the next/new unique value:
                vValueSiblings[i] = (Sibling) uniqueValueCount;

                if (uniqueValueCount == 1) {
                    uniqueValues[uniqueValueCount++] = vValues[i];
                } else if ((uniqueValueCount == 2) && (uniqueValues[0] != vValues[i])) {
                    uniqueValues[uniqueValueCount++] = vValues[i];
                } else {
                    int* uniqueBegin = uniqueValues;
                    int* uniqueEnd   = uniqueValues + uniqueValueCount;
                    int* uniqueFound = std::find(uniqueBegin, uniqueEnd, vValues[i]);
                    if (uniqueFound == uniqueEnd) {
                        uniqueValues[uniqueValueCount++] = vValues[i];
                    } else {
                        vValueSiblings[i] = (Sibling) (uniqueFound - uniqueBegin);
                    }
                }
            }
        }

        //
        //  Some non-manifold cases can have multiple fvar-values but without any discts
        //  edges that would previously have identified mismatch (e.g. two faces meeting
        //  at a common vertex), so deal with that case now that we've counted values:
        //
        if (!vIsManifold && !vertexMismatch[vIndex]) {
            vertexMismatch[vIndex] = (uniqueValueCount > 1);
        }

        //
        //  Update the value count and offset for this vertex and cumulative totals:
        //
        _vertSiblingCounts[vIndex]  = (LocalIndex) uniqueValueCount;
        _vertSiblingOffsets[vIndex] = totalValueCount;

        totalValueCount += uniqueValueCount;

        //  Update the vert-face siblings from the local array above:
        if (uniqueValueCount > 1) {
            SiblingArray vFaceSiblings = getVertexFaceSiblings(vIndex);
            for (int i = 0; i < vFaces.size(); ++i) {
                vFaceSiblings[i] = vValueSiblings[i];
            }
        }
    }

    //
    //  Now that we know the total number of additional sibling values (M values in addition
    //  to the N vertex values) allocate space to accommodate all N + M vertex values.
    //
    //  Then make the second pass through the vertices to identify the values associated with
    //  each and to inspect and tag local face-varying topology for those that don't match:
    //
    resizeVertexValues(totalValueCount);

    for (int vIndex = 0; vIndex < _level.getNumVertices(); ++vIndex) {
        ConstIndexArray       vFaces  = _level.getVertexFaces(vIndex);
        ConstLocalIndexArray  vInFace = _level.getVertexFaceLocalIndices(vIndex);

        //
        //  First step is to assign the values associated with the faces by retrieving them
        //  from the faces.  If the face-varying topology around this vertex matches the vertex
        //  topology, there is little more to do as other members were bulk-initialized to
        //  match, so we can continue immediately:
        //
        IndexArray vValues = getVertexValues(vIndex);

        if (vFaces.size() > 0) {
            vValues[0] = _faceVertValues[_level.getOffsetOfFaceVertices(vFaces[0]) + vInFace[0]];
        } else {
            vValues[0] = 0;
        }
        if (!vertexMismatch[vIndex]) {
            continue;
        }
        if (vValues.size() > 1) {
            ConstSiblingArray vFaceSiblings = getVertexFaceSiblings(vIndex);

            for (int i = 1, nextSibling = 1; i < vFaces.size(); ++i) {
                if (vFaceSiblings[i] == nextSibling) {
                    vValues[nextSibling++] = _faceVertValues[_level.getOffsetOfFaceVertices(vFaces[i]) + vInFace[i]];
                }
            }
        }

        //  XXXX (barfowl) -- this pre-emptive sharpening of values will need to be
        //  revisited soon.  This intentionally avoids the overhead of identifying the
        //  local topology of the values along its boundaries -- necessary for smooth
        //  boundary values but not for sharp as far as refining and limiting the
        //  values is concerned.  But ultimately we need more information than just
        //  the sharp tag when it comes to identifying and gathering FVar patches.
        //
        //  Currently values for non-manifold vertices are sharpened, and that may
        //  also need to be revisited.
        //
        //  Until then...
        //
        //  If all values for this vertex are to be designated as sharp, the value tags
        //  have already been initialized for this by default, so we can continue.  On
        //  further inspection there may be other cases where all are determined to be
        //  sharp, but use what information we can now to avoid that inspection:
        //
        //  Regarding sharpness of the vertex itself, its vertex tags reflect the inf-
        //  or semi-sharp nature of the vertex and edges around it, so be careful not
        //  to assume too much from say, the presence of an incident inf-sharp edge.
        //  We can make clear decisions based on the sharpness of the vertex itself.
        //
        ValueTagArray vValueTags = getVertexValueTags(vIndex);

        Level::VTag const vTag = _level.getVertexTag(vIndex);

        bool allCornersAreSharp = _hasLinearBoundaries || vTag._infSharp || vTag._nonManifold ||
                                  (_hasDependentSharpness && (vValues.size() > 2)) ||
                                  (sharpenDarts && (vValues.size() == 1) && !vTag._boundary);

        //
        //  Values may be a mix of sharp corners and smooth boundaries -- start by
        //  gathering information about the "span" of faces for each value.
        //
        //  Note that the term "span" presumes sequential and continuous, but the
        //  result for a span may include multiple disconnected regions sharing the
        //  common value -- think of a familiar non-manifold "bowtie" vertex in FVar
        //  space.  Such spans are locally non-manifold but are marked as "disjoint"
        //  to avoid overloading "non-manifold" here.
        //
        ValueSpan * vValueSpans = spanBuffer;
        memset(vValueSpans, 0, vValues.size() * sizeof(ValueSpan));

        gatherValueSpans(vIndex, vValueSpans);

        //
        //  Spans are identified as sharp or smooth based on their own local topology,
        //  but the sharpness of one span may be dependent on the sharpness of another
        //  if certain linear-interpolation options were specified.  Mark both as
        //  infinitely sharp where possible (rather than semi-sharp) to avoid
        //  re-assessing this dependency as sharpness is reduced during refinement.
        //
        bool hasDependentValuesToSharpen = false;
        if (!allCornersAreSharp) {
            if (_hasDependentSharpness && (vValues.size() == 2)) {
                //  Detect interior inf-sharp or discts edges:
                allCornersAreSharp = vValueSpans[0]._infSharpEdgeCount || vValueSpans[1]._infSharpEdgeCount ||
                                     vValueSpans[0]._disctsEdgeCount   || vValueSpans[1]._disctsEdgeCount;

                //  Detect a sharp corner, making both sharp:
                if (sharpenBothIfOneCorner) {
                    allCornersAreSharp |= (vValueSpans[0]._size == 1) || (vValueSpans[1]._size == 1);
                }

                //  If only one semi-sharp, need to mark the other as dependent on it:
                hasDependentValuesToSharpen = (vValueSpans[0]._semiSharpEdgeCount > 0) !=
                                              (vValueSpans[1]._semiSharpEdgeCount > 0);
            }
        }

        //
        //  Inspect each vertex value to determine if it is a smooth boundary (crease) and tag
        //  it accordingly.  If not semi-sharp, be sure to consider those values sharpened by
        //  the topology of other values.
        //
        for (int i = 0; i < vValues.size(); ++i) {
            ValueTag & valueTag = vValueTags[i];

            valueTag.clear();
            valueTag._mismatch = true;

            ValueSpan const & vSpan = vValueSpans[i];
            if (vSpan._disctsEdgeCount) {
                valueTag._nonManifold = true;
                continue;
            }
            assert(vSpan._size != 0);

            bool isInfSharp = allCornersAreSharp || vSpan._infSharpEdgeCount ||
                              ((vSpan._size == 1) && fvarCornersAreSharp);

            if (vSpan._size == 1) {
                valueTag._xordinary = !isInfSharp;
            } else {
                valueTag._xordinary = (vSpan._size != regularBoundaryValence);
            }

            valueTag._infSharpEdges = (vSpan._infSharpEdgeCount > 0);
            valueTag._infIrregular = vSpan._infSharpEdgeCount ? ((vSpan._size - vSpan._infSharpEdgeCount) > 1)
                                   : (isInfSharp ? (vSpan._size > 1) : valueTag._xordinary);

            if (!isInfSharp) {
                //
                //  Remember that a semi-sharp value (or one dependent on one) needs to be
                //  treated as a corner (at least three sharp edges or one sharp vertex)
                //  until the sharpness has decayed, so don't tag them as creases here.
                //  But do initialize and maintain the ends of the crease until needed.
                //
                if (vSpan._semiSharpEdgeCount || vTag._semiSharp) {
                    valueTag._semiSharp = true;
                } else if (hasDependentValuesToSharpen) {
                    valueTag._semiSharp = true;
                    valueTag._depSharp = true;
                } else {
                    valueTag._crease = true;
                }

                if (hasCreaseEnds()) {
                    CreaseEndPair & valueCrease = getVertexValueCreaseEnds(vIndex)[i];

                    valueCrease._startFace = vSpan._start;
                    if ((i == 0) && (vSpan._start != 0)) {
                        valueCrease._endFace = (LocalIndex) (vSpan._start + vSpan._size - 1 - vFaces.size());
                    } else {
                        valueCrease._endFace = (LocalIndex) (vSpan._start + vSpan._size - 1);
                    }
                }
            }
        }
    }
    //printf("completed fvar topology...\n");
    //print();
    //printf("validating...\n");
    //assert(validate());
}

//
//  Values tagged as creases have their two "end values" identified relative to the incident
//  faces of the vertex for compact storage and quick retrieval.  This method identifies the
//  values for the two ends of such a crease value:
//
void
FVarLevel::getVertexCreaseEndValues(Index vIndex, Sibling vSibling, Index endValues[2]) const {

    ConstCreaseEndPairArray vValueCreaseEnds = getVertexValueCreaseEnds(vIndex);

    ConstIndexArray      vFaces  = _level.getVertexFaces(vIndex);
    ConstLocalIndexArray vInFace = _level.getVertexFaceLocalIndices(vIndex);

    LocalIndex vertFace0 = vValueCreaseEnds[vSibling]._startFace;
    LocalIndex vertFace1 = vValueCreaseEnds[vSibling]._endFace;

    ConstIndexArray face0Values = getFaceValues(vFaces[vertFace0]);
    ConstIndexArray face1Values = getFaceValues(vFaces[vertFace1]);

    int endInFace0 = vInFace[vertFace0];
    int endInFace1 = vInFace[vertFace1];

    endInFace0 = (endInFace0 == (face0Values.size() - 1)) ? 0 : (endInFace0 + 1);
    endInFace1 = (endInFace1 ? endInFace1 : face1Values.size()) - 1;

    endValues[0] = face0Values[endInFace0];
    endValues[1] = face1Values[endInFace1];
}

//
//  Debugging aids...
//
bool
FVarLevel::validate() const {

    //
    //  Verify that member sizes match sizes for the associated level:
    //
    if ((int)_vertSiblingCounts.size() != _level.getNumVertices()) {
        printf("Error:  vertex count mismatch\n");
        return false;
    }
    if ((int)_edgeTags.size() != _level.getNumEdges()) {
        printf("Error:  edge count mismatch\n");
        return false;
    }
    if ((int)_faceVertValues.size() != _level.getNumFaceVerticesTotal()) {
        printf("Error:  face-value/face-vert count mismatch\n");
        return false;
    }
    if (_level.getDepth() > 0) {
        if (_valueCount != (int)_vertValueIndices.size()) {
            printf("Error:  value/vertex-value count mismatch\n");
            return false;
        }
    }

    //
    //  Verify that face-verts and (locally computed) face-vert siblings yield the
    //  expected face-vert values:
    //
    std::vector<Sibling> fvSiblingVector;
    buildFaceVertexSiblingsFromVertexFaceSiblings(fvSiblingVector);

    for (int fIndex = 0; fIndex < _level.getNumFaces(); ++fIndex) {
        ConstIndexArray     fVerts = _level.getFaceVertices(fIndex);
        ConstIndexArray    fValues = getFaceValues(fIndex);
        Sibling const  * fSiblings = &fvSiblingVector[_level.getOffsetOfFaceVertices(fIndex)];

        for (int fvIndex = 0; fvIndex < fVerts.size(); ++fvIndex) {
            Index vIndex = fVerts[fvIndex];

            Index   fvValue   = fValues[fvIndex];
            Sibling fvSibling = fSiblings[fvIndex];
            if (fvSibling >= getNumVertexValues(vIndex)) {
                printf("Error:  invalid sibling %d for face-vert %d.%d = %d\n", fvSibling, fIndex, fvIndex, vIndex);
                return false;
            }

            Index testValue = getVertexValue(vIndex, fvSibling);
            if (testValue != fvValue) {
                printf("Error:  unexpected value %d for sibling %d of face-vert %d.%d = %d (expecting %d)\n",
                        testValue, fvSibling, fIndex, fvIndex, vIndex, fvValue);
                return false;
            }
        }
    }

    //
    //  Verify that the vert-face siblings yield the expected value:
    //
    for (int vIndex = 0; vIndex < _level.getNumVertices(); ++vIndex) {
        ConstIndexArray      vFaces    = _level.getVertexFaces(vIndex);
        ConstLocalIndexArray vInFace   = _level.getVertexFaceLocalIndices(vIndex);
        ConstSiblingArray    vSiblings = getVertexFaceSiblings(vIndex);

        for (int j = 0; j < vFaces.size(); ++j) {
            Sibling vSibling = vSiblings[j];
            if (vSibling >= getNumVertexValues(vIndex)) {
                printf("Error:  invalid sibling %d at vert-face %d.%d\n", vSibling, vIndex, j);
                return false;
            }

            Index fIndex  = vFaces[j];
            int   fvIndex = vInFace[j];
            Index fvValue = getFaceValues(fIndex)[fvIndex];

            Index vValue = getVertexValue(vIndex, vSibling);
            if (vValue != fvValue) {
                printf("Error:  value mismatch between face-vert %d.%d and vert-face %d.%d (%d != %d)\n",
                        fIndex, fvIndex, vIndex, j, fvValue, vValue);
                return false;
            }
        }
    }
    return true;
}

void
FVarLevel::print() const {

    std::vector<Sibling> fvSiblingVector;
    buildFaceVertexSiblingsFromVertexFaceSiblings(fvSiblingVector);

    printf("Face-varying data channel:\n");
    printf("  Inventory:\n");
    printf("    vertex count       = %d\n", _level.getNumVertices());
    printf("    source value count = %d\n", _valueCount);
    printf("    vertex value count = %d\n", (int)_vertValueIndices.size());

    printf("  Face values:\n");
    for (int i = 0; i < _level.getNumFaces(); ++i) {
        ConstIndexArray  fVerts    = _level.getFaceVertices(i);
        ConstIndexArray  fValues   = getFaceValues(i);
        Sibling const  * fSiblings = &fvSiblingVector[_level.getOffsetOfFaceVertices(i)];

        printf("    face%4d:  ", i);

        printf("verts =");
        for (int j = 0; j < fVerts.size(); ++j) {
            printf("%4d", fVerts[j]);
        }
        printf(",  values =");
        for (int j = 0; j < fValues.size(); ++j) {
            printf("%4d", fValues[j]);
        }
        printf(",  siblings =");
        for (int j = 0; j < fVerts.size(); ++j) {
            printf("%4d", (int)fSiblings[j]);
        }
        printf("\n");
    }

    printf("  Vertex values:\n");
    for (int i = 0; i < _level.getNumVertices(); ++i) {
        int vCount  = getNumVertexValues(i);
        int vOffset = getVertexValueOffset(i);

        printf("    vert%4d:  vcount = %1d, voffset =%4d, ", i, vCount, vOffset);

        ConstIndexArray vValues = getVertexValues(i);

        printf("values =");
        for (int j = 0; j < vValues.size(); ++j) {
            printf("%4d", vValues[j]);
        }
        if (vCount > 1) {
            ConstValueTagArray vValueTags = getVertexValueTags(i);

            printf(", crease =");
            for (int j = 0; j < vValueTags.size(); ++j) {
                printf("%4d", vValueTags[j]._crease);
            }
            printf(", semi-sharp =");
            for (int j = 0; j < vValueTags.size(); ++j) {
                printf("%2d", vValueTags[j]._semiSharp);
            }
        }
        printf("\n");
    }

    printf("  Edge discontinuities:\n");
    for (int i = 0; i < _level.getNumEdges(); ++i) {
        ETag const eTag = getEdgeTag(i);
        if (eTag._mismatch) {
            ConstIndexArray eVerts = _level.getEdgeVertices(i);
            printf("    edge%4d:  verts = [%4d%4d], discts = [%d,%d]\n", i, eVerts[0], eVerts[1],
                    eTag._disctsV0, eTag._disctsV1);
        }
    }
}



void
FVarLevel::initializeFaceValuesFromFaceVertices() {

    ConstIndexArray srcFaceVerts = _level.getFaceVertices();
    Index *         dstFaceValues = &_faceVertValues[0];

    std::memcpy(dstFaceValues, &srcFaceVerts[0], srcFaceVerts.size() * sizeof(Index));
}


void
FVarLevel::initializeFaceValuesFromVertexFaceSiblings() {
    //
    //  Iterate through all face-values first and initialize them with the first value
    //  associated with each face-vertex.  Then make a second sparse pass through the
    //  vertex-faces to offset those with multiple values.  This turns out to be much
    //  more efficient than a single iteration through the vertex-faces since the first
    //  pass is much more memory coherent.
    //
    ConstIndexArray fvIndices = _level.getFaceVertices();
    for (int i = 0; i < fvIndices.size(); ++i) {
        _faceVertValues[i] = getVertexValueOffset(fvIndices[i]);
    }

    //
    //  Now use the vert-face-siblings to populate the face-vert-values:
    //
    for (int vIndex = 0; vIndex < _level.getNumVertices(); ++vIndex) {
        if (getNumVertexValues(vIndex) > 1) {
            ConstIndexArray      vFaces    = _level.getVertexFaces(vIndex);
            ConstLocalIndexArray vInFace   = _level.getVertexFaceLocalIndices(vIndex);
            ConstSiblingArray    vSiblings = getVertexFaceSiblings(vIndex);

            for (int j = 0; j < vFaces.size(); ++j) {
                if (vSiblings[j]) {
                    int fvOffset = _level.getOffsetOfFaceVertices(vFaces[j]);

                    _faceVertValues[fvOffset + vInFace[j]] += vSiblings[j];
                }
            }
        }
    }
}

void
FVarLevel::buildFaceVertexSiblingsFromVertexFaceSiblings(std::vector<Sibling>& fvSiblings) const {

    fvSiblings.resize(_level.getNumFaceVerticesTotal());
    std::memset(&fvSiblings[0], 0, _level.getNumFaceVerticesTotal() * sizeof(Sibling));

    for (int vIndex = 0; vIndex < _level.getNumVertices(); ++vIndex) {
        //  We can skip cases of one sibling as we initialized to 0...
        if (getNumVertexValues(vIndex) > 1) {
            ConstIndexArray      vFaces    = _level.getVertexFaces(vIndex);
            ConstLocalIndexArray vInFace   = _level.getVertexFaceLocalIndices(vIndex);
            ConstSiblingArray    vSiblings = getVertexFaceSiblings(vIndex);

            for (int j = 0; j < vFaces.size(); ++j) {
                if (vSiblings[j] > 0) {
                    fvSiblings[_level.getOffsetOfFaceVertices(vFaces[j]) + vInFace[j]] = vSiblings[j];
                }
            }
        }
    }
}


//
//  Higher-level topological queries, i.e. values in a neighborhood:
//    - given an edge, return values corresponding to its vertices within a given face
//    - given a vertex, return values corresponding to verts at the ends of its edges
//
void
FVarLevel::getEdgeFaceValues(Index eIndex, int fIncToEdge, Index valuesPerVert[2]) const {

    ConstIndexArray eVerts = _level.getEdgeVertices(eIndex);

    if ((getNumVertexValues(eVerts[0]) + getNumVertexValues(eVerts[1])) > 2) {
        Index eFace   = _level.getEdgeFaces(eIndex)[fIncToEdge];
        int   eInFace = _level.getEdgeFaceLocalIndices(eIndex)[fIncToEdge];

        ConstIndexArray fValues = getFaceValues(eFace);

        valuesPerVert[0] = fValues[eInFace];
        valuesPerVert[1] = fValues[((eInFace + 1) < fValues.size()) ? (eInFace + 1) : 0];

        //  Given the way these two end-values are used (both weights the same) we really
        //  don't need to ensure the value pair matches the vertex pair...
        if (eVerts[0] != _level.getFaceVertices(eFace)[eInFace]) {
            std::swap(valuesPerVert[0], valuesPerVert[1]);
        }
    } else {
        //  Remember the extra level of indirection at level 0 -- avoid it here:
        if (_level.getDepth() > 0) {
            valuesPerVert[0] = getVertexValueOffset(eVerts[0]);
            valuesPerVert[1] = getVertexValueOffset(eVerts[1]);
        } else {
            valuesPerVert[0] = getVertexValue(eVerts[0]);
            valuesPerVert[1] = getVertexValue(eVerts[1]);
        }
    }
}

void
FVarLevel::getVertexEdgeValues(Index vIndex, Index valuesPerEdge[]) const {

    ConstIndexArray      vEdges  = _level.getVertexEdges(vIndex);
    ConstLocalIndexArray vInEdge = _level.getVertexEdgeLocalIndices(vIndex);

    ConstIndexArray      vFaces  = _level.getVertexFaces(vIndex);
    ConstLocalIndexArray vInFace = _level.getVertexFaceLocalIndices(vIndex);

    bool vIsBoundary = _level.getVertexTag(vIndex)._boundary;
    bool vIsManifold = ! _level.getVertexTag(vIndex)._nonManifold;

    bool isBaseLevel = (_level.getDepth() == 0);

    for (int i = 0; i < vEdges.size(); ++i) {
        Index           eIndex = vEdges[i];
        ConstIndexArray eVerts = _level.getEdgeVertices(eIndex);

        //  Remember this method is for presumed continuous edges around the vertex:
        assert(edgeTopologyMatches(eIndex));

        Index vOther = eVerts[!vInEdge[i]];
        if (getNumVertexValues(vOther) == 1) {
            valuesPerEdge[i] = isBaseLevel ? getVertexValue(vOther) : getVertexValueOffset(vOther);
        } else if (vIsManifold) {
            if (vIsBoundary && (i == (vEdges.size() - 1))) {
                ConstIndexArray fValues = getFaceValues(vFaces[i-1]);

                int prevInFace = vInFace[i-1] ? (vInFace[i-1] - 1) : (fValues.size() - 1);
                valuesPerEdge[i] = fValues[prevInFace];
            } else {
                ConstIndexArray fValues = getFaceValues(vFaces[i]);

                int nextInFace = (vInFace[i] == (fValues.size() - 1)) ? 0 : (vInFace[i] + 1);
                valuesPerEdge[i] = fValues[nextInFace];
            }
        } else {
            Index eFace0   = _level.getEdgeFaces(eIndex)[0];
            int   eInFace0 = _level.getEdgeFaceLocalIndices(eIndex)[0];

            ConstIndexArray fVerts  = _level.getFaceVertices(eFace0);
            ConstIndexArray fValues = getFaceValues(eFace0);
            if (vOther == fVerts[eInFace0]) {
                valuesPerEdge[i] = fValues[eInFace0];
            } else {
                int valueInFace = (eInFace0 == (fValues.size() - 1)) ? 0 : (eInFace0 + 1);
                valuesPerEdge[i] = fValues[valueInFace];
            }
        }
    }
}

//
//  Gather information about the "span" of faces for each value:
//
//  The "size" (number of faces in which each value occurs), is most immediately useful
//  in determining whether a value is a corner or smooth boundary, while other properties
//  such as the first face and whether or not the span is interrupted by discts, semi-
//  sharp or infinite edges, are useful to fully qualify smooth boundaries by the caller.
//
void
FVarLevel::gatherValueSpans(Index vIndex, ValueSpan * vValueSpans) const {

    ConstIndexArray vEdges = _level.getVertexEdges(vIndex);
    ConstIndexArray vFaces = _level.getVertexFaces(vIndex);

    ConstSiblingArray vFaceSiblings = getVertexFaceSiblings(vIndex);

    bool vHasSingleValue = (getNumVertexValues(vIndex) == 1);
    bool vIsBoundary = vEdges.size() > vFaces.size();
    bool vIsNonManifold = _level.getVertexTag(vIndex)._nonManifold;

    if (vIsNonManifold) {
        //  This needs more work as spans around a non-manifold vertex may themselves be
        //  manifold.  Just mark all spans with a discts edge for now to trigger them 
        //  non-manifold

        ConstIndexArray vValues = getVertexValues(vIndex);
        for (int i = 0; i < vValues.size(); ++i) {
            vValueSpans[i]._size = 0;
            vValueSpans[i]._disctsEdgeCount = 1;
        }
    } else if (vHasSingleValue && !vIsBoundary) {
        //  Mark an interior dart disjoint if more than one discts edge:
        vValueSpans[0]._size  = 0;
        vValueSpans[0]._start = 0;
        for (int i = 0; i < vEdges.size(); ++i) {
            if (_edgeTags[vEdges[i]]._mismatch) {
                if (vValueSpans[0]._size > 0) {
                    vValueSpans[0]._disctsEdgeCount = 1;
                    break;
                } else {
                    vValueSpans[0]._size  = (LocalIndex) vFaces.size();
                    vValueSpans[0]._start = (LocalIndex) i;
                }
            } else if (_level.getEdgeTag(vEdges[i])._infSharp) {
                ++ vValueSpans[0]._infSharpEdgeCount;
            } else if (_level.getEdgeTag(vEdges[i])._semiSharp) {
                ++ vValueSpans[0]._semiSharpEdgeCount;
            }
        }
        vValueSpans[0]._size = (LocalIndex) vFaces.size();
    } else {
        //  Walk around the vertex and accumulate span info for each value -- be
        //  careful about the span for the first value "wrapping" around:
        vValueSpans[0]._size  = 1;
        vValueSpans[0]._start = 0;
        if (!vIsBoundary && (vFaceSiblings[vFaces.size() - 1] == 0)) {
            if (_edgeTags[vEdges[0]]._mismatch) {
                ++ vValueSpans[0]._disctsEdgeCount;
            } else if (_level.getEdgeTag(vEdges[0])._infSharp) {
                ++ vValueSpans[0]._infSharpEdgeCount;
            } else if (_level.getEdgeTag(vEdges[0])._semiSharp) {
                ++ vValueSpans[0]._semiSharpEdgeCount;
            }
        }
        for (int i = 1; i < vFaces.size(); ++i) {
            if (vFaceSiblings[i] == vFaceSiblings[i-1]) {
                if (_edgeTags[vEdges[i]]._mismatch) {
                    ++ vValueSpans[vFaceSiblings[i]]._disctsEdgeCount;
                } else if (_level.getEdgeTag(vEdges[i])._infSharp) {
                    ++ vValueSpans[vFaceSiblings[i]]._infSharpEdgeCount;
                } else if (_level.getEdgeTag(vEdges[i])._semiSharp) {
                    ++ vValueSpans[vFaceSiblings[i]]._semiSharpEdgeCount;
                }
            } else {
                //  If we have already set the span for this value, mark disjoint
                if (vValueSpans[vFaceSiblings[i]]._size > 0) {
                    ++ vValueSpans[vFaceSiblings[i]]._disctsEdgeCount;
                }
                vValueSpans[vFaceSiblings[i]]._start = (LocalIndex) i;
            }
            ++ vValueSpans[vFaceSiblings[i]]._size;
        }
        //  If the span for value 0 has wrapped around, decrement the disjoint added
        //  at the interior edge where it started the closing part of the span:
        if ((vFaceSiblings[vFaces.size() - 1] == 0) && !vIsBoundary) {
            -- vValueSpans[0]._disctsEdgeCount;
        }
    }
}

//
//  Methods to retrieve and combine value and vertex tags:
//
void
FVarLevel::getFaceValueTags(Index faceIndex, ValueTag valueTags[]) const {

    ConstIndexArray faceValues = getFaceValues(faceIndex);
    ConstIndexArray faceVerts  = _level.getFaceVertices(faceIndex);

    for (int i = 0; i < faceValues.size(); ++i) {
        Index srcValueIndex = findVertexValueIndex(faceVerts[i], faceValues[i]);
        assert(_vertValueIndices[srcValueIndex] == faceValues[i]);

        valueTags[i] = _vertValueTags[srcValueIndex];
    }
}

FVarLevel::ValueTag
FVarLevel::getFaceCompositeValueTag(Index faceIndex) const {

    ConstIndexArray faceValues = getFaceValues(faceIndex);
    ConstIndexArray faceVerts  = _level.getFaceVertices(faceIndex);

    typedef ValueTag::ValueTagSize ValueTagSize;

    ValueTagSize compInt = 0;
    for (int i = 0; i < faceValues.size(); ++i) {
        Index srcValueIndex = findVertexValueIndex(faceVerts[i], faceValues[i]);
        assert(_vertValueIndices[srcValueIndex] == faceValues[i]);

        ValueTag const &   srcTag = _vertValueTags[srcValueIndex];
        ValueTagSize const srcInt = srcTag.getBits();

        compInt |= srcInt;
    }
    return ValueTag(compInt);
}

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
