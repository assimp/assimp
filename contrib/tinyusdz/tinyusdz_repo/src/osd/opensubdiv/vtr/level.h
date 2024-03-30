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
#ifndef OPENSUBDIV3_VTR_LEVEL_H
#define OPENSUBDIV3_VTR_LEVEL_H

#include "../version.h"

#include "../sdc/types.h"
#include "../sdc/crease.h"
#include "../sdc/options.h"
#include "../vtr/types.h"

#include <algorithm>
#include <vector>
#include <cassert>
#include <cstring>


namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {
namespace internal {

class Refinement;
class TriRefinement;
class QuadRefinement;
class FVarRefinement;
class FVarLevel;

//
//  Level:
//      A refinement level includes a vectorized representation of the topology
//  for a particular subdivision level.  The topology is "complete" in that any
//  level can be used as the base level of another subdivision hierarchy and can
//  be considered a complete mesh independent of its ancestors.  It currently
//  does contain a "depth" member -- as some inferences can then be made about
//  the topology (i.e. all quads or all tris if not level 0).
//
//  This class is intended for private use within the library.  There are still
//  opportunities to specialize levels -- e.g. those supporting N-sided faces vs
//  those that are purely quads or tris -- so we prefer to insulate it from public
//  access.
//
//  The representation of topology here is to store six topological relationships
//  in tables of integers.  Each is stored in its own array(s) so the result is
//  a SOA representation of the topology.  The six relations are:
//
//      - face-verts:  vertices incident/comprising a face
//      - face-edges:  edges incident a face
//      - edge-verts:  vertices incident/comprising an edge
//      - edge-faces:  faces incident an edge
//      - vert-faces:  faces incident a vertex
//      - vert-edges:  edges incident a vertex
//
//  There is some redundancy here but the intent is not that this be a minimal
//  representation, the intent is that it be amenable to refinement.  Classes in
//  the Far layer essentially store 5 of these 6 in a permuted form -- we add
//  the face-edges here to simplify refinement.
//

class Level {

public:
    //
    //  Simple nested types to hold the tags for each component type -- some of
    //  which are user-specified features (e.g. whether a face is a hole or not)
    //  while others indicate the topological nature of the component, how it
    //  is affected by creasing in its neighborhood, etc.
    //
    //  Most of these properties are passed down to child components during
    //  refinement, but some -- notably the designation of a component as semi-
    //  sharp -- require re-determination as sharpness values are reduced at each
    //  level.
    //
    struct VTag {
        VTag() { }

        //  When cleared, the VTag ALMOST represents a smooth, regular, interior
        //  vertex -- the Type enum requires a bit be explicitly set for Smooth,
        //  so that must be done explicitly if desired on initialization.
        void clear() { std::memset((void*) this, 0, sizeof(VTag)); }

        typedef unsigned short VTagSize;

        VTagSize _nonManifold     : 1;  // fixed
        VTagSize _xordinary       : 1;  // fixed
        VTagSize _boundary        : 1;  // fixed
        VTagSize _corner          : 1;  // fixed
        VTagSize _infSharp        : 1;  // fixed
        VTagSize _semiSharp       : 1;  // variable
        VTagSize _semiSharpEdges  : 1;  // variable
        VTagSize _rule            : 4;  // variable when _semiSharp

        //  These next to tags are complementary -- the "incomplete" tag is only
        //  relevant for refined levels while the "incident an irregular face" tag
        //  is only relevant for the base level.  They could be combined as both
        //  indicate "no full regular ring" around a vertex
        VTagSize _incomplete      : 1;  // variable only set in refined levels
        VTagSize _incidIrregFace  : 1;  // variable only set in base level

        //  Tags indicating incident infinitely-sharp (permanent) features
        VTagSize _infSharpEdges   : 1;  // fixed
        VTagSize _infSharpCrease  : 1;  // fixed
        VTagSize _infIrregular    : 1;  // fixed

        //  Alternate constructor and accessor for dealing with integer bits directly:
        explicit VTag(VTagSize bits) {
            std::memcpy(this, &bits, sizeof(bits));
        }
        VTagSize getBits() const {
            VTagSize bits;
            std::memcpy(&bits, this, sizeof(bits));
            return bits;
        }

        static VTag BitwiseOr(VTag const vTags[], int size = 4);
    };
    struct ETag {
        ETag() { }

        //  When cleared, the ETag represents a smooth, manifold, interior edge
        void clear() { std::memset((void*) this, 0, sizeof(ETag)); }

        typedef unsigned char ETagSize;

        ETagSize _nonManifold  : 1;  // fixed
        ETagSize _boundary     : 1;  // fixed
        ETagSize _infSharp     : 1;  // fixed
        ETagSize _semiSharp    : 1;  // variable

        //  Alternate constructor and accessor for dealing with integer bits directly:
        explicit ETag(ETagSize bits) {
            std::memcpy(this, &bits, sizeof(bits));
        }
        ETagSize getBits() const {
            ETagSize bits;
            std::memcpy(&bits, this, sizeof(bits));
            return bits;
        }

        static ETag BitwiseOr(ETag const eTags[], int size = 4);
    };
    struct FTag {
        FTag() { }

        void clear() { std::memset((void*) this, 0, sizeof(FTag)); }

        typedef unsigned char FTagSize;

        FTagSize _hole  : 1;  // fixed

        //  On deck -- coming soon...
        //FTagSize _hasEdits : 1;  // variable
    };

    //  Additional simple struct to identify a "span" around a vertex, i.e. a
    //  subset of the faces around a vertex delimited by some property (e.g. a
    //  face-varying discontinuity, an inf-sharp edge, etc.)
    //
    //  The span requires an "origin" and a "size" to fully define its extent.
    //  Use of the size is required over a leading/trailing pair as the valence
    //  around a non-manifold vertex cannot be trivially determined from two
    //  extremeties.  Similarly a start face is chosen over an edge as starting
    //  with a manifold edge is ambiguous.  Additional tags also support
    //  non-manifold cases, e.g. periodic spans at the apex of a double cone.
    //
    //  Currently setting the size to 0 or leaving the span "unassigned" is an
    //  indication to use the full neighborhood rather than a subset -- prefer
    //  use of the const method here to direct inspection of the member.
    //
    struct VSpan {
        VSpan() { std::memset((void*) this, 0, sizeof(VSpan)); }

        void clear()            { std::memset((void*) this, 0, sizeof(VSpan)); }
        bool isAssigned() const { return _numFaces > 0; }

        LocalIndex _numFaces;
        LocalIndex _startFace;
        LocalIndex _cornerInSpan;

        unsigned short _periodic : 1;
        unsigned short _sharp    : 1;
    };

public:
    Level();
    ~Level();

    //  Simple accessors:
    int getDepth() const { return _depth; }

    int getNumVertices() const { return _vertCount; }
    int getNumFaces() const    { return _faceCount; }
    int getNumEdges() const    { return _edgeCount; }

    //  More global sizes may prove useful...
    int getNumFaceVerticesTotal() const { return (int) _faceVertIndices.size(); }
    int getNumFaceEdgesTotal() const    { return (int) _faceEdgeIndices.size(); }
    int getNumEdgeVerticesTotal() const { return (int) _edgeVertIndices.size(); }
    int getNumEdgeFacesTotal() const    { return (int) _edgeFaceIndices.size(); }
    int getNumVertexFacesTotal() const  { return (int) _vertFaceIndices.size(); }
    int getNumVertexEdgesTotal() const  { return (int) _vertEdgeIndices.size(); }

    int getMaxValence() const { return _maxValence; }
    int getMaxEdgeFaces() const { return _maxEdgeFaces; }

    //  Methods to access the relation tables/indices -- note that for some relations
    //  (i.e. those where a component is "contained by" a neighbor, or more generally
    //  when the neighbor is a simplex of higher dimension) we store an additional
    //  "local index", e.g. for the case of vert-faces if one of the faces F[i] is
    //  incident a vertex V, then L[i] is the "local index" in F[i] of vertex V.
    //  Once have only quads (or tris), this local index need only occupy two bits
    //  and could conceivably be packed into the same integer as the face index, but
    //  for now, given the need to support faces of potentially high valence we'll
    //  use an 8- or 16-bit integer.
    //
    //  Methods to access the six topological relations:
    ConstIndexArray getFaceVertices(Index faceIndex) const;
    ConstIndexArray getFaceEdges(Index faceIndex) const;
    ConstIndexArray getEdgeVertices(Index edgeIndex) const;
    ConstIndexArray getEdgeFaces(Index edgeIndex) const;
    ConstIndexArray getVertexFaces(Index vertIndex) const;
    ConstIndexArray getVertexEdges(Index vertIndex) const;

    ConstLocalIndexArray getEdgeFaceLocalIndices(Index edgeIndex) const;
    ConstLocalIndexArray getVertexFaceLocalIndices(Index vertIndex) const;
    ConstLocalIndexArray getVertexEdgeLocalIndices(Index vertIndex) const;

    //  Replace these with access to sharpness buffers/arrays rather than elements:
    float getEdgeSharpness(Index edgeIndex) const;
    float getVertexSharpness(Index vertIndex) const;
    Sdc::Crease::Rule getVertexRule(Index vertIndex) const;

    Index findEdge(Index v0Index, Index v1Index) const;

    // Holes
    void setFaceHole(Index faceIndex, bool b);
    bool isFaceHole(Index faceIndex) const;

    // Face-varying
    Sdc::Options getFVarOptions(int channel) const; 
    int getNumFVarChannels() const { return (int) _fvarChannels.size(); }
    int getNumFVarValues(int channel) const;
    ConstIndexArray getFaceFVarValues(Index faceIndex, int channel) const;

    FVarLevel & getFVarLevel(int channel) { return *_fvarChannels[channel]; }
    FVarLevel const & getFVarLevel(int channel) const { return *_fvarChannels[channel]; }

    //  Manifold/non-manifold tags:
    void setEdgeNonManifold(Index edgeIndex, bool b);
    bool isEdgeNonManifold(Index edgeIndex) const;

    void setVertexNonManifold(Index vertIndex, bool b);
    bool isVertexNonManifold(Index vertIndex) const;

    //  General access to all component tags:
    VTag const & getVertexTag(Index vertIndex) const { return _vertTags[vertIndex]; }
    ETag const & getEdgeTag(Index edgeIndex) const { return _edgeTags[edgeIndex]; }
    FTag const & getFaceTag(Index faceIndex) const { return _faceTags[faceIndex]; }

    VTag & getVertexTag(Index vertIndex) { return _vertTags[vertIndex]; }
    ETag & getEdgeTag(Index edgeIndex) { return _edgeTags[edgeIndex]; }
    FTag & getFaceTag(Index faceIndex) { return _faceTags[faceIndex]; }

public:

    //  Debugging aides:
    enum TopologyError {
        TOPOLOGY_MISSING_EDGE_FACES=0,
        TOPOLOGY_MISSING_EDGE_VERTS,
        TOPOLOGY_MISSING_FACE_EDGES,
        TOPOLOGY_MISSING_FACE_VERTS,
        TOPOLOGY_MISSING_VERT_FACES,
        TOPOLOGY_MISSING_VERT_EDGES,

        TOPOLOGY_FAILED_CORRELATION_EDGE_FACE,
        TOPOLOGY_FAILED_CORRELATION_FACE_VERT,
        TOPOLOGY_FAILED_CORRELATION_FACE_EDGE,

        TOPOLOGY_FAILED_ORIENTATION_INCIDENT_EDGE,
        TOPOLOGY_FAILED_ORIENTATION_INCIDENT_FACE,
        TOPOLOGY_FAILED_ORIENTATION_INCIDENT_FACES_EDGES,

        TOPOLOGY_DEGENERATE_EDGE,
        TOPOLOGY_NON_MANIFOLD_EDGE,

        TOPOLOGY_INVALID_CREASE_EDGE,
        TOPOLOGY_INVALID_CREASE_VERT
    };

    static char const * getTopologyErrorString(TopologyError errCode);

    typedef void (* ValidationCallback)(TopologyError errCode, char const * msg, void const * clientData);

    bool validateTopology(ValidationCallback callback=0, void const * clientData=0) const;

    void print(const Refinement* parentRefinement = 0) const;

public:
    //  High-level topology queries -- these may be moved elsewhere:

    bool isSingleCreasePatch(Index face, float* sharpnessOut=NULL, int* rotationOut=NULL) const;

    //
    //  When inspecting topology, the component tags -- particularly VTag and ETag -- are most
    //  often inspected in groups for the face to which they belong.  They are designed to be
    //  bitwise OR'd (the result then referred to as a "composite" tag) to make quick decisions
    //  about the face as a whole to avoid tedious topological inspection.
    //
    //  The same logic can be applied to topology in a FVar channel when tags specific to that
    //  channel are used.  Note that the VTags apply to the FVar values assigned to the corners
    //  of the face and not the vertex as a whole.  The "composite" face-varying VTag for a
    //  vertex is the union of VTags of all distinct FVar values for that vertex.
    //
    bool doesVertexFVarTopologyMatch(Index vIndex, int fvarChannel) const;
    bool doesFaceFVarTopologyMatch(  Index fIndex, int fvarChannel) const;
    bool doesEdgeFVarTopologyMatch(  Index eIndex, int fvarChannel) const;

    void getFaceVTags(Index fIndex, VTag vTags[], int fvarChannel = -1) const;
    void getFaceETags(Index fIndex, ETag eTags[], int fvarChannel = -1) const;

    VTag getFaceCompositeVTag(Index fIndex, int fvarChannel = -1) const;
    VTag getFaceCompositeVTag(ConstIndexArray & fVerts) const;

    VTag getVertexCompositeFVarVTag(Index vIndex, int fvarChannel) const;

    //
    //  When gathering "patch points" we may want the indices of the vertices or the corresponding
    //  FVar values for a particular channel.  Both are represented and equally accessible within
    //  the faces, so we allow all to be returned through these methods.  Setting the optional FVar
    //  channel to -1 will retrieve indices of vertices instead of FVar values:
    //
    int gatherQuadLinearPatchPoints(Index fIndex, Index patchPoints[], int rotation = 0,
                                                                       int fvarChannel = -1) const;

    int gatherQuadRegularInteriorPatchPoints(Index fIndex, Index patchPoints[], int rotation = 0,
                                                                                int fvarChannel = -1) const;
    int gatherQuadRegularBoundaryPatchPoints(Index fIndex, Index patchPoints[], int boundaryEdgeInFace,
                                                                                int fvarChannel = -1) const;
    int gatherQuadRegularCornerPatchPoints(  Index fIndex, Index patchPoints[], int cornerVertInFace,
                                                                                int fvarChannel = -1) const;

    int gatherQuadRegularRingAroundVertex(Index vIndex, Index ringPoints[],
                                          int fvarChannel = -1) const;
    int gatherQuadRegularPartialRingAroundVertex(Index vIndex, VSpan const & span, Index ringPoints[],
                                                 int fvarChannel = -1) const;

    //  WIP -- for future use, need to extend for face-varying...
    int gatherTriRegularInteriorPatchPoints(      Index fIndex, Index patchVerts[], int rotation = 0) const;
    int gatherTriRegularBoundaryVertexPatchPoints(Index fIndex, Index patchVerts[], int boundaryVertInFace) const;
    int gatherTriRegularBoundaryEdgePatchPoints(  Index fIndex, Index patchVerts[], int boundaryEdgeInFace) const;
    int gatherTriRegularCornerVertexPatchPoints(  Index fIndex, Index patchVerts[], int cornerVertInFace) const;
    int gatherTriRegularCornerEdgePatchPoints(    Index fIndex, Index patchVerts[], int cornerEdgeInFace) const;

public:
    //  Sizing methods used to construct a level to populate:
    void resizeFaces(       int numFaces);
    void resizeFaceVertices(int numFaceVertsTotal);
    void resizeFaceEdges(   int numFaceEdgesTotal);

    void resizeEdges(    int numEdges);
    void resizeEdgeVertices();  // always 2*edgeCount
    void resizeEdgeFaces(int numEdgeFacesTotal);

    void resizeVertices(   int numVertices);
    void resizeVertexFaces(int numVertexFacesTotal);
    void resizeVertexEdges(int numVertexEdgesTotal);

    void setMaxValence(int maxValence);

    //  Modifiers to populate the relations for each component:
    IndexArray getFaceVertices(Index faceIndex);
    IndexArray getFaceEdges(Index faceIndex);
    IndexArray getEdgeVertices(Index edgeIndex);
    IndexArray getEdgeFaces(Index edgeIndex);
    IndexArray getVertexFaces(Index vertIndex);
    IndexArray getVertexEdges(Index vertIndex);

    LocalIndexArray getEdgeFaceLocalIndices(Index edgeIndex);
    LocalIndexArray getVertexFaceLocalIndices(Index vertIndex);
    LocalIndexArray getVertexEdgeLocalIndices(Index vertIndex);

    //  Replace these with access to sharpness buffers/arrays rather than elements:
    float& getEdgeSharpness(Index edgeIndex);
    float& getVertexSharpness(Index vertIndex);

    //  Create, destroy and populate face-varying channels:
    int  createFVarChannel(int fvarValueCount, Sdc::Options const& options);
    void destroyFVarChannel(int channel);

    IndexArray getFaceFVarValues(Index faceIndex, int channel);

    void completeFVarChannelTopology(int channel, int regBoundaryValence);

    //  Counts and offsets for all relation types:
    //      - these may be unwarranted if we let Refinement access members directly...
    int getNumFaceVertices(     Index faceIndex) const { return _faceVertCountsAndOffsets[2*faceIndex]; }
    int getOffsetOfFaceVertices(Index faceIndex) const { return _faceVertCountsAndOffsets[2*faceIndex + 1]; }

    int getNumFaceEdges(     Index faceIndex) const { return getNumFaceVertices(faceIndex); }
    int getOffsetOfFaceEdges(Index faceIndex) const { return getOffsetOfFaceVertices(faceIndex); }

    int getNumEdgeVertices(     Index )          const { return 2; }
    int getOffsetOfEdgeVertices(Index edgeIndex) const { return 2 * edgeIndex; }

    int getNumEdgeFaces(     Index edgeIndex) const { return _edgeFaceCountsAndOffsets[2*edgeIndex]; }
    int getOffsetOfEdgeFaces(Index edgeIndex) const { return _edgeFaceCountsAndOffsets[2*edgeIndex + 1]; }

    int getNumVertexFaces(     Index vertIndex) const { return _vertFaceCountsAndOffsets[2*vertIndex]; }
    int getOffsetOfVertexFaces(Index vertIndex) const { return _vertFaceCountsAndOffsets[2*vertIndex + 1]; }

    int getNumVertexEdges(     Index vertIndex) const { return _vertEdgeCountsAndOffsets[2*vertIndex]; }
    int getOffsetOfVertexEdges(Index vertIndex) const { return _vertEdgeCountsAndOffsets[2*vertIndex + 1]; }

    ConstIndexArray getFaceVertices() const;

    //
    //  Note that for some relations, the size of the relations for a child component
    //  can vary radically from its parent due to the sparsity of the refinement.  So
    //  in these cases a few additional utilities are provided to help define the set
    //  of incident components.  Assuming adequate memory has been allocated, the
    //  "resize" methods here initialize the set of incident components by setting
    //  both the size and the appropriate offset, while "trim" is use to quickly lower
    //  the size from an upper bound and nothing else.
    //
    void resizeFaceVertices(Index FaceIndex, int count);

    void resizeEdgeFaces(Index edgeIndex, int count);
    void trimEdgeFaces(  Index edgeIndex, int count);

    void resizeVertexFaces(Index vertIndex, int count);
    void trimVertexFaces(  Index vertIndex, int count);

    void resizeVertexEdges(Index vertIndex, int count);
    void trimVertexEdges(  Index vertIndex, int count);

public:
    //
    //  Initial plans were to have a few specific classes properly construct the
    //  topology from scratch, e.g. the Refinement class and a Factory class for
    //  the base level, by populating all topological relations.  The need to have
    //  a class construct full topology given only a simple face-vertex list, made
    //  it necessary to write code to define and orient all relations -- and most
    //  of that seemed best placed here.
    //
    bool completeTopologyFromFaceVertices();
    Index findEdge(Index v0, Index v1, ConstIndexArray v0Edges) const;

    //  Methods supporting the above:
    void orientIncidentComponents();
    bool orderVertexFacesAndEdges(Index vIndex, Index* vFaces, Index* vEdges) const;
    bool orderVertexFacesAndEdges(Index vIndex);
    void populateLocalIndices();

    IndexArray shareFaceVertCountsAndOffsets() const;

private:
    //  Refinement classes (including all subclasses) build a Level:
    friend class Refinement;
    friend class TriRefinement;
    friend class QuadRefinement;

    //
    //  A Level is independent of subdivision scheme or options.  While it may have been
    //  affected by them in its construction, they are not associated with it -- a Level
    //  is pure topology and any subdivision parameters are external.
    //

    //  Simple members for inventory, etc.
    int _faceCount;
    int _edgeCount;
    int _vertCount;

    //  The "depth" member is clearly useful in both the topological splitting and the
    //  stencil queries, but arguably it ties the Level to a hierarchy which counters
    //  the idea of it being independent.
    int _depth;

    //  Maxima to help clients manage sizing of data buffers.  Given "max valence",
    //  the "max edge faces" is strictly redundant as it will always be less, but 
    //  since it will typically be so much less (i.e. 2) it is kept for now.
    int _maxEdgeFaces;
    int _maxValence;

    //
    //  Topology vectors:
    //      Note that of all of these, only data for the face-edge relation is not
    //      stored in the osd::FarTables in any form.  The FarTable vectors combine
    //      the edge-vert and edge-face relations.  The eventual goal is that this
    //      data be part of the osd::Far classes and be a superset of the FarTable
    //      vectors, i.e. no data duplication or conversion.  The fact that FarTable
    //      already stores 5 of the 6 possible relations should make the topology
    //      storage as a whole a non-issue.
    //
    //      The vert-face-child and vert-edge-child indices are also arguably not
    //      a topology relation but more one for parent/child relations.  But it is
    //      a topological relationship, and if named differently would not likely
    //      raise this.  It has been named with "child" in the name as it does play
    //      a more significant role during subdivision in mapping between parent
    //      and child components, and so has been named to reflect that more clearly.
    //

    //  Per-face:
    std::vector<Index> _faceVertCountsAndOffsets;  // 2 per face, redundant after level 0
    std::vector<Index> _faceVertIndices;           // 3 or 4 per face, variable at level 0
    std::vector<Index> _faceEdgeIndices;           // matches face-vert indices
    std::vector<FTag>  _faceTags;                  // 1 per face:  includes "hole" tag

    //  Per-edge:
    std::vector<Index>      _edgeVertIndices;           // 2 per edge
    std::vector<Index>      _edgeFaceCountsAndOffsets;  // 2 per edge
    std::vector<Index>      _edgeFaceIndices;           // varies with faces per edge
    std::vector<LocalIndex> _edgeFaceLocalIndices;      // varies with faces per edge

    std::vector<float>      _edgeSharpness;             // 1 per edge
    std::vector<ETag>       _edgeTags;                  // 1 per edge:  manifold, boundary, etc.

    //  Per-vertex:
    std::vector<Index>      _vertFaceCountsAndOffsets;  // 2 per vertex
    std::vector<Index>      _vertFaceIndices;           // varies with valence
    std::vector<LocalIndex> _vertFaceLocalIndices;      // varies with valence, 8-bit for now

    std::vector<Index>      _vertEdgeCountsAndOffsets;  // 2 per vertex
    std::vector<Index>      _vertEdgeIndices;           // varies with valence
    std::vector<LocalIndex> _vertEdgeLocalIndices;      // varies with valence, 8-bit for now

    std::vector<float>      _vertSharpness;             // 1 per vertex
    std::vector<VTag>       _vertTags;                  // 1 per vertex:  manifold, Sdc::Rule, etc.

    //  Face-varying channels:
    std::vector<FVarLevel*> _fvarChannels;
};

//
//  Access/modify the vertices incident a given face:
//
inline ConstIndexArray
Level::getFaceVertices(Index faceIndex) const {
    return ConstIndexArray(&_faceVertIndices[_faceVertCountsAndOffsets[faceIndex*2+1]],
                          _faceVertCountsAndOffsets[faceIndex*2]);
}
inline IndexArray
Level::getFaceVertices(Index faceIndex) {
    return IndexArray(&_faceVertIndices[_faceVertCountsAndOffsets[faceIndex*2+1]],
                          _faceVertCountsAndOffsets[faceIndex*2]);
}

inline void
Level::resizeFaceVertices(Index faceIndex, int count) {

    int* countOffsetPair = &_faceVertCountsAndOffsets[faceIndex*2];

    countOffsetPair[0] = count;
    countOffsetPair[1] = (faceIndex == 0) ? 0 : (countOffsetPair[-2] + countOffsetPair[-1]);

    _maxValence = std::max(_maxValence, count);
}

inline ConstIndexArray
Level::getFaceVertices() const {
    return ConstIndexArray(&_faceVertIndices[0], (int)_faceVertIndices.size());
}

//
//  Access/modify the edges incident a given face:
//
inline ConstIndexArray
Level::getFaceEdges(Index faceIndex) const {
    return ConstIndexArray(&_faceEdgeIndices[_faceVertCountsAndOffsets[faceIndex*2+1]],
                          _faceVertCountsAndOffsets[faceIndex*2]);
}
inline IndexArray
Level::getFaceEdges(Index faceIndex) {
    return IndexArray(&_faceEdgeIndices[_faceVertCountsAndOffsets[faceIndex*2+1]],
                          _faceVertCountsAndOffsets[faceIndex*2]);
}

//
//  Access/modify the faces incident a given vertex:
//
inline ConstIndexArray
Level::getVertexFaces(Index vertIndex) const {
    return ConstIndexArray( (&_vertFaceIndices[0]) + _vertFaceCountsAndOffsets[vertIndex*2+1],
                          _vertFaceCountsAndOffsets[vertIndex*2]);
}
inline IndexArray
Level::getVertexFaces(Index vertIndex) {
    return IndexArray( (&_vertFaceIndices[0]) + _vertFaceCountsAndOffsets[vertIndex*2+1],
                          _vertFaceCountsAndOffsets[vertIndex*2]);
}

inline ConstLocalIndexArray
Level::getVertexFaceLocalIndices(Index vertIndex) const {
    return ConstLocalIndexArray( (&_vertFaceLocalIndices[0]) + _vertFaceCountsAndOffsets[vertIndex*2+1],
                               _vertFaceCountsAndOffsets[vertIndex*2]);
}
inline LocalIndexArray
Level::getVertexFaceLocalIndices(Index vertIndex) {
    return LocalIndexArray( (&_vertFaceLocalIndices[0]) + _vertFaceCountsAndOffsets[vertIndex*2+1],
                               _vertFaceCountsAndOffsets[vertIndex*2]);
}

inline void
Level::resizeVertexFaces(Index vertIndex, int count) {
    int* countOffsetPair = &_vertFaceCountsAndOffsets[vertIndex*2];

    countOffsetPair[0] = count;
    countOffsetPair[1] = (vertIndex == 0) ? 0 : (countOffsetPair[-2] + countOffsetPair[-1]);
}
inline void
Level::trimVertexFaces(Index vertIndex, int count) {
    _vertFaceCountsAndOffsets[vertIndex*2] = count;
}

//
//  Access/modify the edges incident a given vertex:
//
inline ConstIndexArray
Level::getVertexEdges(Index vertIndex) const {
    return ConstIndexArray( (&_vertEdgeIndices[0]) +_vertEdgeCountsAndOffsets[vertIndex*2+1],
                          _vertEdgeCountsAndOffsets[vertIndex*2]);
}
inline IndexArray
Level::getVertexEdges(Index vertIndex) {
    return IndexArray( (&_vertEdgeIndices[0]) +_vertEdgeCountsAndOffsets[vertIndex*2+1],
                          _vertEdgeCountsAndOffsets[vertIndex*2]);
}

inline ConstLocalIndexArray
Level::getVertexEdgeLocalIndices(Index vertIndex) const {
    return ConstLocalIndexArray( (&_vertEdgeLocalIndices[0]) + _vertEdgeCountsAndOffsets[vertIndex*2+1],
                               _vertEdgeCountsAndOffsets[vertIndex*2]);
}
inline LocalIndexArray
Level::getVertexEdgeLocalIndices(Index vertIndex) {
    return LocalIndexArray( (&_vertEdgeLocalIndices[0]) + _vertEdgeCountsAndOffsets[vertIndex*2+1],
                               _vertEdgeCountsAndOffsets[vertIndex*2]);
}

inline void
Level::resizeVertexEdges(Index vertIndex, int count) {
    int* countOffsetPair = &_vertEdgeCountsAndOffsets[vertIndex*2];

    countOffsetPair[0] = count;
    countOffsetPair[1] = (vertIndex == 0) ? 0 : (countOffsetPair[-2] + countOffsetPair[-1]);

    _maxValence = std::max(_maxValence, count);
}
inline void
Level::trimVertexEdges(Index vertIndex, int count) {
    _vertEdgeCountsAndOffsets[vertIndex*2] = count;
}

inline void
Level::setMaxValence(int valence) {
    _maxValence = valence;
}

//
//  Access/modify the vertices incident a given edge:
//
inline ConstIndexArray
Level::getEdgeVertices(Index edgeIndex) const {
    return ConstIndexArray(&_edgeVertIndices[edgeIndex*2], 2);
}
inline IndexArray
Level::getEdgeVertices(Index edgeIndex) {
    return IndexArray(&_edgeVertIndices[edgeIndex*2], 2);
}

//
//  Access/modify the faces incident a given edge:
//
inline ConstIndexArray
Level::getEdgeFaces(Index edgeIndex) const {
    return ConstIndexArray(&_edgeFaceIndices[0] + 
                           _edgeFaceCountsAndOffsets[edgeIndex*2+1],
                           _edgeFaceCountsAndOffsets[edgeIndex*2]);
}
inline IndexArray
Level::getEdgeFaces(Index edgeIndex) {
    return IndexArray(&_edgeFaceIndices[0] +
                      _edgeFaceCountsAndOffsets[edgeIndex*2+1],
                      _edgeFaceCountsAndOffsets[edgeIndex*2]);
}

inline ConstLocalIndexArray
Level::getEdgeFaceLocalIndices(Index edgeIndex) const {
    return ConstLocalIndexArray(&_edgeFaceLocalIndices[0] +
                                _edgeFaceCountsAndOffsets[edgeIndex*2+1],
                                _edgeFaceCountsAndOffsets[edgeIndex*2]);
}
inline LocalIndexArray
Level::getEdgeFaceLocalIndices(Index edgeIndex) {
    return LocalIndexArray(&_edgeFaceLocalIndices[0] +
                           _edgeFaceCountsAndOffsets[edgeIndex*2+1],
                           _edgeFaceCountsAndOffsets[edgeIndex*2]);
}

inline void
Level::resizeEdgeFaces(Index edgeIndex, int count) {
    int* countOffsetPair = &_edgeFaceCountsAndOffsets[edgeIndex*2];

    countOffsetPair[0] = count;
    countOffsetPair[1] = (edgeIndex == 0) ? 0 : (countOffsetPair[-2] + countOffsetPair[-1]);

    _maxEdgeFaces = std::max(_maxEdgeFaces, count);
}
inline void
Level::trimEdgeFaces(Index edgeIndex, int count) {
    _edgeFaceCountsAndOffsets[edgeIndex*2] = count;
}

//
//  Access/modify sharpness values:
//
inline float
Level::getEdgeSharpness(Index edgeIndex) const {
    return _edgeSharpness[edgeIndex];
}
inline float&
Level::getEdgeSharpness(Index edgeIndex) {
    return _edgeSharpness[edgeIndex];
}

inline float
Level::getVertexSharpness(Index vertIndex) const {
    return _vertSharpness[vertIndex];
}
inline float&
Level::getVertexSharpness(Index vertIndex) {
    return _vertSharpness[vertIndex];
}

inline Sdc::Crease::Rule
Level::getVertexRule(Index vertIndex) const {
    return (Sdc::Crease::Rule) _vertTags[vertIndex]._rule;
}

//
//  Access/modify hole tag:
//
inline void
Level::setFaceHole(Index faceIndex, bool b) {
    _faceTags[faceIndex]._hole = b;
}
inline bool
Level::isFaceHole(Index faceIndex) const {
    return _faceTags[faceIndex]._hole;
}

//
//  Access/modify non-manifold tags:
//
inline void
Level::setEdgeNonManifold(Index edgeIndex, bool b) {
    _edgeTags[edgeIndex]._nonManifold = b;
}
inline bool
Level::isEdgeNonManifold(Index edgeIndex) const {
    return _edgeTags[edgeIndex]._nonManifold;
}

inline void
Level::setVertexNonManifold(Index vertIndex, bool b) {
    _vertTags[vertIndex]._nonManifold = b;
}
inline bool
Level::isVertexNonManifold(Index vertIndex) const {
    return _vertTags[vertIndex]._nonManifold;
}

//
//  Sizing methods to allocate space:
//
inline void
Level::resizeFaces(int faceCount) {
    _faceCount = faceCount;
    _faceVertCountsAndOffsets.resize(2 * faceCount);

    _faceTags.resize(faceCount);
    std::memset((void*) &_faceTags[0], 0, _faceCount * sizeof(FTag));
}
inline void
Level::resizeFaceVertices(int totalFaceVertCount) {
    _faceVertIndices.resize(totalFaceVertCount);
}
inline void
Level::resizeFaceEdges(int totalFaceEdgeCount) {
    _faceEdgeIndices.resize(totalFaceEdgeCount);
}

inline void
Level::resizeEdges(int edgeCount) {

    _edgeCount = edgeCount;
    _edgeFaceCountsAndOffsets.resize(2 * edgeCount);

    _edgeSharpness.resize(edgeCount);
    _edgeTags.resize(edgeCount);

    if (edgeCount>0) {
        std::memset((void*) &_edgeTags[0], 0, _edgeCount * sizeof(ETag));
    }
}
inline void
Level::resizeEdgeVertices() {

    _edgeVertIndices.resize(2 * _edgeCount);
}
inline void
Level::resizeEdgeFaces(int totalEdgeFaceCount) {

    _edgeFaceIndices.resize(totalEdgeFaceCount);
    _edgeFaceLocalIndices.resize(totalEdgeFaceCount);
}

inline void
Level::resizeVertices(int vertCount) {

    _vertCount = vertCount;
    _vertFaceCountsAndOffsets.resize(2 * vertCount);
    _vertEdgeCountsAndOffsets.resize(2 * vertCount);

    _vertSharpness.resize(vertCount);
    _vertTags.resize(vertCount);
    std::memset((void*) &_vertTags[0], 0, _vertCount * sizeof(VTag));
}
inline void
Level::resizeVertexFaces(int totalVertFaceCount) {

    _vertFaceIndices.resize(totalVertFaceCount);
    _vertFaceLocalIndices.resize(totalVertFaceCount);
}
inline void
Level::resizeVertexEdges(int totalVertEdgeCount) {

    _vertEdgeIndices.resize(totalVertEdgeCount);
    _vertEdgeLocalIndices.resize(totalVertEdgeCount);
}

inline IndexArray
Level::shareFaceVertCountsAndOffsets() const {
    // XXXX manuelk we have to force const casting here (classes don't 'share'
    // members usually...)
    return IndexArray(const_cast<Index *>(&_faceVertCountsAndOffsets[0]),
        (int)_faceVertCountsAndOffsets.size());
}

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_VTR_LEVEL_H */
