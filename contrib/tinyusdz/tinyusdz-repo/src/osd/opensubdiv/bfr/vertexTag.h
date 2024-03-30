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

#ifndef OPENSUBDIV3_BFR_VERTEX_TAG_H
#define OPENSUBDIV3_BFR_VERTEX_TAG_H

#include "../version.h"

#include <cstring>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  VertexTag is a simple set of bits that identify exceptional properties
//  at the corner vertices of a face that warrant closer inspection (and
//  potential additional processing).  As with some bitfields in Far, this
//  supports bitwise-OR so that tags for the corners of a face can quickly
//  be combined to determine properties of the associated limit surface.
//
//  In order to accommodate the two separate uses more clearly -- that of
//  a set of bits applying to a single corner/vertex versus a set of bits
//  resulting from the combination (bitwise-OR) of several -- the bitfield
//  is defined as a base class and two separate classes are derived from
//  it to suit those purposes.
//
class FeatureBits {
public:
    FeatureBits() { }
    ~FeatureBits() { }

public:
    //  Integer/bit conversions and operations:
    typedef unsigned short IntType;

    IntType GetBits() const {
        IntType bits;
        std::memcpy(&bits, this, sizeof(*this));
        return bits;
    }
    void SetBits(IntType bits) {
        std::memcpy(this, &bits, sizeof(*this));
    }
    void Clear() {
        SetBits(0);
    }

protected:
    friend class FaceVertex;
    friend struct FaceVertexSubset;

    IntType _boundaryVerts      : 1;
    IntType _infSharpVerts      : 1;
    IntType _infSharpEdges      : 1;
    IntType _infSharpDarts      : 1;
    IntType _semiSharpVerts     : 1;
    IntType _semiSharpEdges     : 1;
    IntType _unCommonFaceSizes  : 1;
    IntType _irregularFaceSizes : 1;
    IntType _unOrderedFaces     : 1;
    IntType _nonManifoldVerts   : 1;
    IntType _boundaryNonSharp   : 1;
};

//
//  VertexTag wraps the FeatureBits for use with a single corner/vertex:
//
//  Note that a bit is not defined to detect extra-ordinary or regular
//  valence.  Since subsets of the topology are ultimately used in the
//  limit surface definition, and face-varying surfaces are potentially
//  subsets of subsets, we intentionally avoid having to re-compute
//  that bit for each subset.  Such a bit has little value for a single
//  corner in Bfr, so the collective presence is determined when the
//  surface definition is finalized in the regular/irregular test.
//
class VertexTag : public FeatureBits {
public:
    VertexTag() { }
    ~VertexTag() { }

    //  Queries for single corner/vertex (some reversing sense of the bit):
    bool IsBoundary()            const { return  _boundaryVerts; }
    bool IsInterior()            const { return !_boundaryVerts; }
    bool IsInfSharp()            const { return  _infSharpVerts; }
    bool HasInfSharpEdges()      const { return  _infSharpEdges; }
    bool IsInfSharpDart()        const { return  _infSharpDarts; }
    bool IsSemiSharp()           const { return  _semiSharpVerts; }
    bool HasSemiSharpEdges()     const { return  _semiSharpEdges; }
    bool HasUnCommonFaceSizes()  const { return  _unCommonFaceSizes; }
    bool HasIrregularFaceSizes() const { return  _irregularFaceSizes; }
    bool IsOrdered()             const { return !_unOrderedFaces; }
    bool IsUnOrdered()           const { return  _unOrderedFaces; }
    bool IsManifold()            const { return !_nonManifoldVerts; }
    bool IsNonManifold()         const { return  _nonManifoldVerts; }
    bool HasNonSharpBoundary()   const { return  _boundaryNonSharp; }
    bool HasSharpEdges()         const { return   HasInfSharpEdges() ||
                                                  HasSemiSharpEdges(); }
};

//
//  MultiVertexTag wraps the FeatureBits for use with bits combined from 
//  several corners/vertices. It includes the Combine() method to apply
//  the bitwise-OR with a given VertexTag, in addition to using different
//  names for the access methods to reflect their collective nature (e.g.
//  the use of "has" versus "is").
//
class MultiVertexTag : public FeatureBits {
public:
    MultiVertexTag() { }
    ~MultiVertexTag() { }

    //  Queries for multiple VertexTags combined into one:
    bool HasBoundaryVertices()     const { return _boundaryVerts; }
    bool HasInfSharpVertices()     const { return _infSharpVerts; }
    bool HasInfSharpEdges()        const { return _infSharpEdges; }
    bool HasInfSharpDarts()        const { return _infSharpDarts; }
    bool HasSemiSharpVertices()    const { return _semiSharpVerts; }
    bool HasSemiSharpEdges()       const { return _semiSharpEdges; }
    bool HasUnCommonFaceSizes()    const { return _unCommonFaceSizes; }
    bool HasIrregularFaceSizes()   const { return _irregularFaceSizes; }
    bool HasUnOrderedVertices()    const { return _unOrderedFaces; }
    bool HasNonManifoldVertices()  const { return _nonManifoldVerts; }
    bool HasNonSharpBoundary()     const { return _boundaryNonSharp; }
    bool HasSharpVertices()        const { return  HasInfSharpVertices() ||
                                                   HasSemiSharpVertices(); }
    bool HasSharpEdges()           const { return  HasInfSharpEdges() ||
                                                   HasSemiSharpEdges(); }

    void Combine(VertexTag const & tag) {
        SetBits(GetBits() | tag.GetBits());
    }
};

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_VERTEX_TAG_H */
