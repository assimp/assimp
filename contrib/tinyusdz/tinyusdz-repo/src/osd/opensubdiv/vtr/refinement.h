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
#ifndef OPENSUBDIV3_VTR_REFINEMENT_H
#define OPENSUBDIV3_VTR_REFINEMENT_H

#include "../version.h"

#include "../sdc/types.h"
#include "../sdc/options.h"
#include "../vtr/types.h"
#include "../vtr/level.h"

#include <vector>

//
//  Declaration for the main refinement class (Refinement) and its pre-requisites:
//
namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {
namespace internal {

class FVarRefinement;

//
//  Refinement:
//      A refinement is a mapping between two levels -- relating the components in the original
//  (parent) level to the one refined (child).  The refinement may be complete (uniform) or sparse
//  (adaptive or otherwise selective), so not all components in the parent level will spawn
//  components in the child level.
//
//  Refinement is an abstract class and expects subclasses corresponding to the different types
//  of topological splits that the supported subdivision schemes collectively require, i.e. those
//  listed in Sdc::SplitType.  Note the virtual requirements expected of the subclasses in the list
//  of protected methods -- they differ mainly in the topology that is created in the child Level
//  and not the propagation of tags through refinement, subdivision of sharpness values or the
//  treatment of face-varying data.  The primary subclasses are QuadRefinement and TriRefinement.
//
//  At a high level, all that is necessary in terms of interface is to construct, initialize
//  (linking the two levels), optionally select components for sparse refinement (via use of the
//  SparseSelector) and call the refine() method.  This usage is expected of Far::TopologyRefiner.
//
//  Since we really want this class to be restricted from public access eventually, all methods
//  begin with lower case (as is the convention for protected methods) and the list of friends
//  will be maintained more strictly.
//
class Refinement {

public:
    Refinement(Level const & parent, Level & child, Sdc::Options const& schemeOptions);
    virtual ~Refinement();

    Level const& parent() const { return *_parent; }
    Level const& child() const  { return *_child; }
    Level&       child()        { return *_child; }

    Sdc::Split getSplitType() const { return _splitType; }
    int getRegularFaceSize() const { return _regFaceSize; }
    Sdc::Options getOptions() const { return _options; }

    //  Face-varying:
    int getNumFVarChannels() const { return (int) _fvarChannels.size(); }

    FVarRefinement const & getFVarRefinement(int c) const { return *_fvarChannels[c]; }

    //
    //  Options associated with the actual refinement operation, which may end up
    //  quite involved if we want to allow for the refinement of data that is not
    //  of interest to be suppressed.  For now we have:
    //
    //      "sparse": the alternative to uniform refinement, which requires that
    //          components be previously selected/marked to be included.
    //
    //      "minimal topology": this is one that may get broken down into a finer
    //          set of options.  It suppresses "full topology" in the child level
    //          and only generates what is minimally necessary for interpolation --
    //          which requires at least the face-vertices for faces, but also the
    //          vertex-faces for any face-varying channels present.  So it will
    //          generate one or two of the six possible topological relations.
    //
    //  These are strictly controlled right now, e.g. for sparse refinement, we
    //  currently enforce full topology at the finest level to allow for subsequent
    //  patch construction.
    //
    struct Options {
        Options() : _sparse(false),
                    _faceVertsFirst(false),
                    _minimalTopology(false)
                    { }

        unsigned int _sparse          : 1;
        unsigned int _faceVertsFirst  : 1;
        unsigned int _minimalTopology : 1;

        //  Still under consideration:
        //unsigned int _childToParentMap : 1;
    };

    void refine(Options options = Options());

    bool hasFaceVerticesFirst() const { return _faceVertsFirst; }

public:
    //
    //  Access to members -- some testing classes (involving vertex interpolation)
    //  currently make use of these:
    //
    int getNumChildFacesFromFaces() const       { return _childFaceFromFaceCount; }
    int getNumChildEdgesFromFaces() const       { return _childEdgeFromFaceCount; }
    int getNumChildEdgesFromEdges() const       { return _childEdgeFromEdgeCount; }
    int getNumChildVerticesFromFaces() const    { return _childVertFromFaceCount; }
    int getNumChildVerticesFromEdges() const    { return _childVertFromEdgeCount; }
    int getNumChildVerticesFromVertices() const { return _childVertFromVertCount; }

    Index getFirstChildFaceFromFaces() const      { return _firstChildFaceFromFace; }
    Index getFirstChildEdgeFromFaces() const      { return _firstChildEdgeFromFace; }
    Index getFirstChildEdgeFromEdges() const      { return _firstChildEdgeFromEdge; }
    Index getFirstChildVertexFromFaces() const    { return _firstChildVertFromFace; }
    Index getFirstChildVertexFromEdges() const    { return _firstChildVertFromEdge; }
    Index getFirstChildVertexFromVertices() const { return _firstChildVertFromVert; }

    Index getFaceChildVertex(Index f) const   { return _faceChildVertIndex[f]; }
    Index getEdgeChildVertex(Index e) const   { return _edgeChildVertIndex[e]; }
    Index getVertexChildVertex(Index v) const { return _vertChildVertIndex[v]; }

    ConstIndexArray  getFaceChildFaces(Index parentFace) const;
    ConstIndexArray  getFaceChildEdges(Index parentFace) const;
    ConstIndexArray  getEdgeChildEdges(Index parentEdge) const;

    //  Child-to-parent relationships
    bool isChildVertexComplete(Index v) const       { return ! _childVertexTag[v]._incomplete; }

    Index getChildFaceParentFace(Index f) const     { return _childFaceParentIndex[f]; }
    int   getChildFaceInParentFace(Index f) const   { return _childFaceTag[f]._indexInParent; }

    Index getChildEdgeParentIndex(Index e) const    { return _childEdgeParentIndex[e]; }

    Index getChildVertexParentIndex(Index v) const  { return _childVertexParentIndex[v]; }

//
//  Modifiers intended for internal/protected use:
//
public:

    IndexArray getFaceChildFaces(Index parentFace);
    IndexArray getFaceChildEdges(Index parentFace);
    IndexArray getEdgeChildEdges(Index parentEdge);

public:
    //
    //  Tags have now been added per-component in Level, but there is additional need to tag
    //  components within Refinement -- we can't tag the parent level components for any
    //  refinement (in order to keep it const) and tags associated with children that are
    //  specific to the child-to-parent mapping may not be warranted in the child level.
    //
    //  Parent tags are only required for sparse refinement.  The main property to tag is
    //  whether a component was selected, and so a single SparseTag is used for all three
    //  component types.  Tagging if a component is "transitional" is also useful.  This may
    //  only be necessary for edges but is currently packed into a mask per-edge for faces,
    //  which could be deferred, in which case "transitional" could be a single bit.
    //
    //  Child tags are part of the child-to-parent mapping, which consists of the parent
    //  component index for each child component, plus a tag for the child indicating more
    //  about its relationship to its parent, e.g. is it completely defined, what the parent
    //  component type is, what is the index of the child within its parent, etc.
    //
    struct SparseTag {
        SparseTag() : _selected(0), _transitional(0) { }

        unsigned char _selected     : 1;  // component specifically selected for refinement
        unsigned char _transitional : 4;  // adjacent to a refined component (4-bits for face)
    };

    struct ChildTag {
        ChildTag() { }

        unsigned char _incomplete    : 1;  // incomplete neighborhood to represent limit of parent
        unsigned char _parentType    : 2;  // type of parent component:  vertex, edge or face
        unsigned char _indexInParent : 2;  // index of child wrt parent:  0-3, or iterative if N > 4
    };

    //  Methods to access and modify tags:
    SparseTag const & getParentFaceSparseTag(  Index f) const { return _parentFaceTag[f]; }
    SparseTag const & getParentEdgeSparseTag(  Index e) const { return _parentEdgeTag[e]; }
    SparseTag const & getParentVertexSparseTag(Index v) const { return _parentVertexTag[v]; }

    SparseTag & getParentFaceSparseTag(  Index f) { return _parentFaceTag[f]; }
    SparseTag & getParentEdgeSparseTag(  Index e) { return _parentEdgeTag[e]; }
    SparseTag & getParentVertexSparseTag(Index v) { return _parentVertexTag[v]; }

    ChildTag const & getChildFaceTag(  Index f) const { return _childFaceTag[f]; }
    ChildTag const & getChildEdgeTag(  Index e) const { return _childEdgeTag[e]; }
    ChildTag const & getChildVertexTag(Index v) const { return _childVertexTag[v]; }

    ChildTag & getChildFaceTag(  Index f) { return _childFaceTag[f]; }
    ChildTag & getChildEdgeTag(  Index e) { return _childEdgeTag[e]; }
    ChildTag & getChildVertexTag(Index v) { return _childVertexTag[v]; }

//  Remaining methods should really be protected -- for use by subclasses...
public:
    //
    //  Methods involved in constructing the parent-to-child mapping -- when the
    //  refinement is sparse, additional methods are needed to identify the selection:
    //
    void populateParentToChildMapping();
    void populateParentChildIndices();
    void printParentToChildMapping() const;

    virtual void allocateParentChildIndices() = 0;

    //  Supporting method for sparse refinement:
    void initializeSparseSelectionTags();
    void markSparseChildComponentIndices();
    void markSparseVertexChildren();
    void markSparseEdgeChildren();

    virtual void markSparseFaceChildren() = 0;

    void initializeChildComponentCounts();

    //
    //  Methods involved in constructing the child-to-parent mapping:
    //
    void populateChildToParentMapping();

    void populateFaceParentVectors(ChildTag const initialChildTags[2][4]);
    void populateFaceParentFromParentFaces(ChildTag const initialChildTags[2][4]);

    void populateEdgeParentVectors(ChildTag const initialChildTags[2][4]);
    void populateEdgeParentFromParentFaces(ChildTag const initialChildTags[2][4]);
    void populateEdgeParentFromParentEdges(ChildTag const initialChildTags[2][4]);

    void populateVertexParentVectors(ChildTag const initialChildTags[2][4]);
    void populateVertexParentFromParentFaces(ChildTag const initialChildTags[2][4]);
    void populateVertexParentFromParentEdges(ChildTag const initialChildTags[2][4]);
    void populateVertexParentFromParentVertices(ChildTag const initialChildTags[2][4]);

    //
    //  Methods involved in propagating component tags from parent to child:
    //
    void propagateComponentTags();

    void populateFaceTagVectors();
    void populateFaceTagsFromParentFaces();

    void populateEdgeTagVectors();
    void populateEdgeTagsFromParentFaces();
    void populateEdgeTagsFromParentEdges();

    void populateVertexTagVectors();
    void populateVertexTagsFromParentFaces();
    void populateVertexTagsFromParentEdges();
    void populateVertexTagsFromParentVertices();

    //
    //  Methods (and types) involved in subdividing the topology -- though not
    //  fully exploited, any subset of the 6 relations can be generated:
    //
    struct Relations {
        unsigned int   _faceVertices : 1;
        unsigned int   _faceEdges    : 1;
        unsigned int   _edgeVertices : 1;
        unsigned int   _edgeFaces    : 1;
        unsigned int   _vertexFaces  : 1;
        unsigned int   _vertexEdges  : 1;

        void setAll(bool enable) {
            _faceVertices = enable;
            _faceEdges    = enable;
            _edgeVertices = enable;
            _edgeFaces    = enable;
            _vertexFaces  = enable;
            _vertexEdges  = enable;
        }
    };

    void subdivideTopology(Relations const& relationsToSubdivide);

    virtual void populateFaceVertexRelation() = 0;
    virtual void populateFaceEdgeRelation() = 0;
    virtual void populateEdgeVertexRelation() = 0;
    virtual void populateEdgeFaceRelation() = 0;
    virtual void populateVertexFaceRelation() = 0;
    virtual void populateVertexEdgeRelation() = 0;

    //
    //  Methods involved in subdividing and inspecting sharpness values:
    //
    void subdivideSharpnessValues();

    void subdivideVertexSharpness();
    void subdivideEdgeSharpness();
    void reclassifySemisharpVertices();

    //
    //  Methods involved in subdividing face-varying topology:
    //
    void subdivideFVarChannels();

protected:
    // A debug method of Level prints a Refinement (should really change this)
    friend void Level::print(const Refinement *) const;

    //
    //  Data members -- the logical grouping of some of these (and methods that make use
    //  of them) may lead to grouping them into a few utility classes or structs...
    //

    //  Defined on construction:
    Level const * _parent;
    Level *       _child;
    Sdc::Options  _options;

    //  Defined by the subclass:
    Sdc::Split _splitType;
    int        _regFaceSize;

    //  Determined by the refinement options:
    bool _uniform;
    bool _faceVertsFirst;

    //
    //  Inventory and ordering of the types of child components:
    //
    int _childFaceFromFaceCount;  // arguably redundant (all faces originate from faces)
    int _childEdgeFromFaceCount;
    int _childEdgeFromEdgeCount;
    int _childVertFromFaceCount;
    int _childVertFromEdgeCount;
    int _childVertFromVertCount;

    int _firstChildFaceFromFace;  // arguably redundant (all faces originate from faces)
    int _firstChildEdgeFromFace;
    int _firstChildEdgeFromEdge;
    int _firstChildVertFromFace;
    int _firstChildVertFromEdge;
    int _firstChildVertFromVert;

    //
    //  The parent-to-child mapping:
    //      These are vectors sized according to the number of parent components (and
    //  their topology) that contain references/indices to the child components that
    //  result from them by refinement.  When refinement is sparse, parent components
    //  that have not spawned all child components will have their missing children
    //  marked as invalid.
    //
    //  NOTE the "Array" members here.  Often vectors within the Level can be shared
    //  with the Refinement, and an Array instance is used to do so.  If not shared
    //  the subclass just initializes the Array members after allocating its own local
    //  vector members.
    //
    IndexArray _faceChildFaceCountsAndOffsets;
    IndexArray _faceChildEdgeCountsAndOffsets;

    IndexVector _faceChildFaceIndices;  // *cannot* always use face-vert counts/offsets
    IndexVector _faceChildEdgeIndices;  // can use face-vert counts/offsets
    IndexVector _faceChildVertIndex;

    IndexVector _edgeChildEdgeIndices;  // trivial/corresponding pair for each
    IndexVector _edgeChildVertIndex;

    IndexVector _vertChildVertIndex;

    //
    //  The child-to-parent mapping:
    //
    IndexVector _childFaceParentIndex;
    IndexVector _childEdgeParentIndex;
    IndexVector _childVertexParentIndex;

    std::vector<ChildTag> _childFaceTag;
    std::vector<ChildTag> _childEdgeTag;
    std::vector<ChildTag> _childVertexTag;

    //
    //  Tags for sparse selection of components:
    //
    std::vector<SparseTag> _parentFaceTag;
    std::vector<SparseTag> _parentEdgeTag;
    std::vector<SparseTag> _parentVertexTag;

    //
    //  Refinement data for face-varying channels present in the Levels being refined:
    //
    std::vector<FVarRefinement*> _fvarChannels;
};

inline ConstIndexArray
Refinement::getFaceChildFaces(Index parentFace) const {

    return ConstIndexArray(&_faceChildFaceIndices[_faceChildFaceCountsAndOffsets[2*parentFace+1]],
                                             _faceChildFaceCountsAndOffsets[2*parentFace]);
}

inline IndexArray
Refinement::getFaceChildFaces(Index parentFace) {

    return IndexArray(&_faceChildFaceIndices[_faceChildFaceCountsAndOffsets[2*parentFace+1]],
                                             _faceChildFaceCountsAndOffsets[2*parentFace]);
}

inline ConstIndexArray
Refinement::getFaceChildEdges(Index parentFace) const {

    return ConstIndexArray(&_faceChildEdgeIndices[_faceChildEdgeCountsAndOffsets[2*parentFace+1]],
                                             _faceChildEdgeCountsAndOffsets[2*parentFace]);
}
inline IndexArray
Refinement::getFaceChildEdges(Index parentFace) {

    return IndexArray(&_faceChildEdgeIndices[_faceChildEdgeCountsAndOffsets[2*parentFace+1]],
                                             _faceChildEdgeCountsAndOffsets[2*parentFace]);
}

inline ConstIndexArray
Refinement::getEdgeChildEdges(Index parentEdge) const {

    return ConstIndexArray(&_edgeChildEdgeIndices[parentEdge*2], 2);
}

inline IndexArray
Refinement::getEdgeChildEdges(Index parentEdge) {

    return IndexArray(&_edgeChildEdgeIndices[parentEdge*2], 2);
}

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_VTR_REFINEMENT_H */
