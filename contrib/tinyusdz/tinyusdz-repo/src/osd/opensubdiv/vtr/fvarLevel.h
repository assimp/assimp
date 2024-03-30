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
#ifndef OPENSUBDIV3_VTR_FVAR_LEVEL_H
#define OPENSUBDIV3_VTR_FVAR_LEVEL_H

#include "../version.h"

#include "../sdc/types.h"
#include "../sdc/crease.h"
#include "../sdc/options.h"
#include "../vtr/types.h"
#include "../vtr/level.h"

#include <vector>
#include <cassert>
#include <cstring>


namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {
namespace internal {

//
//  FVarLevel:
//      A "face-varying channel" includes the topology for a set of face-varying
//  data, relative to the topology of the Level with which it is associated.
//
//  Analogous to a set of vertices and face-vertices that define the topology for
//  the geometry, a channel requires a set of "values" and "face-values".  The
//  "values" are indices of entries in a set of face-varying data, just as vertices
//  are indices into a set of vertex data.  The face-values identify a value for
//  each vertex of the face, and so define topology for the values that may be
//  unique to each channel.
//
//  In addition to the value size and the vector of face-values (which matches the
//  size of the geometry's face-vertices), tags are associated with each component
//  to identify deviations of the face-varying topology from the vertex topology.
//  And since there may be a one-to-many mapping between vertices and face-varying
//  values, that mapping is also allocated.
//
//  It turns out that the mapping used is able to completely encode the set of
//  face-values and is more amenable to refinement.  Currently the face-values
//  take up almost half the memory of this representation, so if memory does
//  become a concern, we do not need to store them.  The only reason we do so now
//  is that the face-value interface for specifying base topology and inspecting
//  subsequent levels is very familiar to that of face-vertices for clients.  So
//  having them available for such access is convenient.
//
//  Regarding scope and access...
//      Unclear at this early state, but leaning towards nesting this class within
//  Level, given the intimate dependency between the two.
//      Everything is being declared public for now to facilitate access until it's
//  clearer how this functionality will be provided.
//
class FVarLevel {
public:
    //
    //  Component tags -- trying to minimize the types needed here:
    //
    //  Tag per Edge:
    //      - facilitates topological analysis around each vertex
    //      - required during refinement to spawn one or more edge-values
    //
    struct ETag {
        ETag() { }

        void clear() { std::memset(this, 0, sizeof(ETag)); }

        typedef unsigned char ETagSize;

        ETagSize _mismatch : 1;  // local FVar topology does not match
        ETagSize _disctsV0 : 1;  // discontinuous at vertex 0
        ETagSize _disctsV1 : 1;  // discontinuous at vertex 1
        ETagSize _linear   : 1;  // linear boundary constraints

        Level::ETag combineWithLevelETag(Level::ETag) const;
    };

    //
    //  Tag per Value:
    //      - informs both refinement and interpolation
    //          - every value spawns a child value in refinement
    //      - includes a subset of Level::VTag to be later combined with a VTag
    //
    struct ValueTag {
        ValueTag() { }

        void clear() { std::memset(this, 0, sizeof(ValueTag)); }

        bool isMismatch() const    { return _mismatch; }
        bool isCrease() const      { return _crease; }
        bool isCorner() const      { return !_crease; }
        bool isSemiSharp() const   { return _semiSharp; }
        bool isInfSharp() const    { return !_semiSharp && !_crease; }
        bool isDepSharp() const    { return _depSharp; }
        bool hasCreaseEnds() const { return _crease || _semiSharp; }

        bool hasInfSharpEdges() const   { return _infSharpEdges; }
        bool hasInfIrregularity() const { return _infIrregular; }

        typedef unsigned char ValueTagSize;

        //  If there is no mismatch, no other members should be inspected
        ValueTagSize _mismatch    : 1;  // local FVar topology does not match
        ValueTagSize _xordinary   : 1;  // local FVar topology is extra-ordinary
        ValueTagSize _nonManifold : 1;  // local FVar topology is non-manifold
        ValueTagSize _crease      : 1;  // value is a crease, otherwise a corner
        ValueTagSize _semiSharp   : 1;  // value is a corner decaying to crease
        ValueTagSize _depSharp    : 1;  // value is a corner by dependency on another

        ValueTagSize _infSharpEdges : 1;  // value is a corner by inf-sharp features
        ValueTagSize _infIrregular  : 1;  // value span includes inf-sharp irregularity

        Level::VTag combineWithLevelVTag(Level::VTag) const;

        //  Alternate constructor and accessor for dealing with integer bits directly:
        explicit ValueTag(ValueTagSize bits) {
            std::memcpy(this, &bits, sizeof(bits));
        }
        ValueTagSize getBits() const {
            ValueTagSize bits;
            std::memcpy(&bits, this, sizeof(bits));
            return bits;
        }
    };

    typedef Vtr::ConstArray<ValueTag> ConstValueTagArray;
    typedef Vtr::Array<ValueTag> ValueTagArray;

    //
    //  Simple struct containing the "end faces" of a crease, i.e. the faces which
    //  contain the FVar values to be used when interpolating the crease.  (Prefer
    //  the struct over std::pair for its member names)
    //
    struct CreaseEndPair {
        LocalIndex _startFace;
        LocalIndex _endFace;
    };

    typedef Vtr::ConstArray<CreaseEndPair> ConstCreaseEndPairArray;
    typedef Vtr::Array<CreaseEndPair> CreaseEndPairArray;

    typedef LocalIndex      Sibling;

    typedef ConstLocalIndexArray ConstSiblingArray;
    typedef LocalIndexArray SiblingArray;

public:
    FVarLevel(Level const& level);
    ~FVarLevel();

    //  Queries for the entire channel:
    Level const& getLevel() const { return _level; }

    int getNumValues() const          { return _valueCount; }
    int getNumFaceValuesTotal() const { return (int) _faceVertValues.size(); }

    bool isLinear() const            { return _isLinear; }
    bool hasLinearBoundaries() const { return _hasLinearBoundaries; }
    bool hasSmoothBoundaries() const { return ! _hasLinearBoundaries; }
    bool hasCreaseEnds() const       { return hasSmoothBoundaries(); }

    Sdc::Options getOptions() const { return _options; }

    //  Queries per face:
    ConstIndexArray  getFaceValues(Index fIndex) const;
    IndexArray       getFaceValues(Index fIndex);

    //  Queries per edge:
    ETag getEdgeTag(Index eIndex) const          { return _edgeTags[eIndex]; }
    bool edgeTopologyMatches(Index eIndex) const { return !getEdgeTag(eIndex)._mismatch; }

    //  Queries per vertex (and its potential sibling values):
    int   getNumVertexValues(Index v) const                  { return _vertSiblingCounts[v]; }
    Index getVertexValueOffset(Index v, Sibling i = 0) const { return _vertSiblingOffsets[v] + i; }

    Index getVertexValue(Index v, Sibling i = 0) const { return _vertValueIndices[getVertexValueOffset(v,i)]; }

    Index findVertexValueIndex(Index vertexIndex, Index valueIndex) const;

    //  Methods to access/modify array properties per vertex:
    ConstIndexArray  getVertexValues(Index vIndex) const;
    IndexArray       getVertexValues(Index vIndex);

    ConstValueTagArray  getVertexValueTags(Index vIndex) const;
    ValueTagArray       getVertexValueTags(Index vIndex);

    ConstCreaseEndPairArray  getVertexValueCreaseEnds(Index vIndex) const;
    CreaseEndPairArray       getVertexValueCreaseEnds(Index vIndex);

    ConstSiblingArray  getVertexFaceSiblings(Index vIndex) const;
    SiblingArray       getVertexFaceSiblings(Index vIndex);

    //  Queries per value:
    ValueTag getValueTag(Index valueIndex) const          { return _vertValueTags[valueIndex]; }
    bool     valueTopologyMatches(Index valueIndex) const { return !getValueTag(valueIndex)._mismatch; }

    CreaseEndPair getValueCreaseEndPair(Index valueIndex) const { return _vertValueCreaseEnds[valueIndex]; }

    //  Tag queries related to faces (use Level methods for those returning Level::VTag/ETag)
    void getFaceValueTags(Index faceIndex, ValueTag valueTags[]) const;

    ValueTag getFaceCompositeValueTag(Index faceIndex) const;

    //  Higher-level topological queries, i.e. values in a neighborhood:
    void getEdgeFaceValues(Index eIndex, int fIncToEdge, Index valuesPerVert[2]) const;
    void getVertexEdgeValues(Index vIndex, Index valuesPerEdge[]) const;
    void getVertexCreaseEndValues(Index vIndex, Sibling sibling, Index endValues[2]) const;

    //  Initialization and allocation helpers:
    void setOptions(Sdc::Options const& options);
    void resizeVertexValues(int numVertexValues);
    void resizeValues(int numValues);
    void resizeComponents();

    //  Topological analysis methods -- tagging and face-value population:
    void completeTopologyFromFaceValues(int regBoundaryValence);
    void initializeFaceValuesFromFaceVertices();
    void initializeFaceValuesFromVertexFaceSiblings();

    struct ValueSpan;
    void gatherValueSpans(Index vIndex, ValueSpan * vValueSpans) const;

    //  Debugging methods:
    bool validate() const;
    void print() const;
    void buildFaceVertexSiblingsFromVertexFaceSiblings(std::vector<Sibling>& fvSiblings) const;

private:
    //  Just as Refinements build Levels, FVarRefinements build FVarLevels...
    friend class FVarRefinement;

    Level const & _level;

    //  Linear interpolation options vary between channels:
    Sdc::Options _options;

    bool _isLinear;
    bool _hasLinearBoundaries;
    bool _hasDependentSharpness;
    int  _valueCount;

    //
    //  Vectors recording face-varying topology including tags that help propagate
    //  data through the refinement hierarchy.  Vectors are not sparse but most use
    //  8-bit values relative to the local topology.
    //
    //  The vector of face-values is actually redundant here, but is constructed as
    //  it is most convenient for clients.  It represents almost half the memory of
    //  the topology (4 32-bit integers per face) and not surprisingly, populating
    //  it takes a considerable amount of the refinement time (1/3).  We can reduce
    //  both if we are willing to compute these on demand for clients.
    //
    //  Per-face (matches face-verts of corresponding level):
    std::vector<Index> _faceVertValues;

    //  Per-edge:
    std::vector<ETag> _edgeTags;

    //  Per-vertex:
    std::vector<Sibling>  _vertSiblingCounts;
    std::vector<int>      _vertSiblingOffsets;
    std::vector<Sibling>  _vertFaceSiblings;

    //  Per-value:
    std::vector<Index>         _vertValueIndices;
    std::vector<ValueTag>      _vertValueTags;
    std::vector<CreaseEndPair> _vertValueCreaseEnds;
};

//
//  Access/modify the values associated with each face:
//
inline ConstIndexArray
FVarLevel::getFaceValues(Index fIndex) const {

    int vCount  = _level.getNumFaceVertices(fIndex);
    int vOffset = _level.getOffsetOfFaceVertices(fIndex);
    return ConstIndexArray(&_faceVertValues[vOffset], vCount);
}
inline IndexArray
FVarLevel::getFaceValues(Index fIndex) {

    int vCount  = _level.getNumFaceVertices(fIndex);
    int vOffset = _level.getOffsetOfFaceVertices(fIndex);
    return IndexArray(&_faceVertValues[vOffset], vCount);
}

inline FVarLevel::ConstSiblingArray
FVarLevel::getVertexFaceSiblings(Index vIndex) const {

    int vCount  = _level.getNumVertexFaces(vIndex);
    int vOffset = _level.getOffsetOfVertexFaces(vIndex);
    return ConstSiblingArray(&_vertFaceSiblings[vOffset], vCount);
}
inline FVarLevel::SiblingArray
FVarLevel::getVertexFaceSiblings(Index vIndex) {

    int vCount  = _level.getNumVertexFaces(vIndex);
    int vOffset = _level.getOffsetOfVertexFaces(vIndex);
    return SiblingArray(&_vertFaceSiblings[vOffset], vCount);
}

inline ConstIndexArray
FVarLevel::getVertexValues(Index vIndex) const
{
    int vCount  = getNumVertexValues(vIndex);
    int vOffset = getVertexValueOffset(vIndex);
    return ConstIndexArray(&_vertValueIndices[vOffset], vCount);
}
inline IndexArray
FVarLevel::getVertexValues(Index vIndex)
{
    int vCount  = getNumVertexValues(vIndex);
    int vOffset = getVertexValueOffset(vIndex);
    return IndexArray(&_vertValueIndices[vOffset], vCount);
}

inline FVarLevel::ConstValueTagArray
FVarLevel::getVertexValueTags(Index vIndex) const
{
    int vCount  = getNumVertexValues(vIndex);
    int vOffset = getVertexValueOffset(vIndex);
    return ConstValueTagArray(&_vertValueTags[vOffset], vCount);
}
inline FVarLevel::ValueTagArray
FVarLevel::getVertexValueTags(Index vIndex)
{
    int vCount  = getNumVertexValues(vIndex);
    int vOffset = getVertexValueOffset(vIndex);
    return ValueTagArray(&_vertValueTags[vOffset], vCount);
}

inline FVarLevel::ConstCreaseEndPairArray
FVarLevel::getVertexValueCreaseEnds(Index vIndex) const
{
    int vCount  = getNumVertexValues(vIndex);
    int vOffset = getVertexValueOffset(vIndex);
    return ConstCreaseEndPairArray(&_vertValueCreaseEnds[vOffset], vCount);
}
inline FVarLevel::CreaseEndPairArray
FVarLevel::getVertexValueCreaseEnds(Index vIndex)
{
    int vCount  = getNumVertexValues(vIndex);
    int vOffset = getVertexValueOffset(vIndex);
    return CreaseEndPairArray(&_vertValueCreaseEnds[vOffset], vCount);
}

inline Index
FVarLevel::findVertexValueIndex(Index vertexIndex, Index valueIndex) const {

    if (_level.getDepth() > 0) return valueIndex;

    Index vvIndex = getVertexValueOffset(vertexIndex);
    while (_vertValueIndices[vvIndex] != valueIndex) {
        ++ vvIndex;
    }
    return vvIndex;
}

//
//  Methods related to tagging:
//
inline Level::ETag
FVarLevel::ETag::combineWithLevelETag(Level::ETag levelTag) const
{
    if (this->_mismatch) {
        levelTag._boundary = true;
        levelTag._infSharp = true;
    }
    return levelTag;
}
inline Level::VTag
FVarLevel::ValueTag::combineWithLevelVTag(Level::VTag levelTag) const
{
    if (this->_mismatch) {
        //
        //  Semi-sharp FVar values are always tagged and treated as corners
        //  (at least three sharp edges (two boundary edges and one interior
        //  semi-sharp) and/or vertex is semi-sharp) until the sharpness has
        //  decayed, but they ultimately lie on the inf-sharp crease of the
        //  FVar boundary.  Consider this when tagging inf-sharp features.
        //
        if (this->isCorner()) {
            levelTag._rule = (Level::VTag::VTagSize) Sdc::Crease::RULE_CORNER;
        } else {
            levelTag._rule = (Level::VTag::VTagSize) Sdc::Crease::RULE_CREASE;
        }
        if (this->isCrease() || this->isSemiSharp()) {
            levelTag._infSharp = false;
            levelTag._infSharpCrease = true;
            levelTag._corner = false;
        } else {
            levelTag._infSharp = true;
            levelTag._infSharpCrease = false;
            levelTag._corner = !this->_infIrregular && !this->_infSharpEdges;
        }
        levelTag._infSharpEdges = true;
        levelTag._infIrregular = this->_infIrregular;

        levelTag._boundary = true;
        levelTag._xordinary = this->_xordinary;

        levelTag._nonManifold |= this->_nonManifold;
    }
    return levelTag;
}

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_VTR_FVAR_LEVEL_H */
