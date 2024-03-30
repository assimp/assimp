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

#ifndef OPENSUBDIV3_BFR_FACE_VERTEX_H
#define OPENSUBDIV3_BFR_FACE_VERTEX_H

#include "../version.h"

#include "../bfr/vertexTag.h"
#include "../bfr/vertexDescriptor.h"
#include "../bfr/faceVertexSubset.h"
#include "../bfr/surfaceData.h"
#include "../sdc/crease.h"

#include <cassert>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  The FaceVertex class is the primary internal class for gathering all
//  topological information around the corner of a face.  As such, it
//  wraps an instance of the public VertexDescriptor class populated by the
//  Factory subclasses.  It extends an instance of VertexDescriptor with
//  additional topological information and methods to make it more widely
//  available and useful to internal classes.
//
//  One fundamental extension of FaceVertex is that it includes the
//  location of the face in the ring of incident faces around the vertex.
//  VertexDescriptor alone simply specifies the neighborhood of the
//  vertex, but the FaceVertex provides context relative to the face for
//  which all of this information is being gathered.
//
//  Several instances of FaceVertex (one for each corner of a face)
//  are necessary to fully define the limit surface for a face, but in
//  many cases, only a subset of the FaceVertex's incident faces will
//  actually contribute to the surface. A companion class for defining
//  that subset is defined elsewhere.
//  
class FaceVertex {
public:
    typedef internal::SurfaceData::Index Index;

public:
    FaceVertex() { }
    ~FaceVertex() { }

    //  Methods supporting construction/initialization (subclass required
    //  to populate VertexDescriptor between Initialize and Finalize):
    void Initialize(int faceSize, int regFaceSize);
    void Finalize(int faceInVertex);

    VertexDescriptor & GetVertexDescriptor() { return _vDesc; }

    void ConnectUnOrderedFaces(Index const faceVertexIndices[]);

public:
    //  Methods to initialize and find subsets:
    typedef FaceVertexSubset Subset;

    int GetVertexSubset(Subset * subset) const;

    int FindFaceVaryingSubset(Subset       * fvarSubset,
                              Index  const   fvarIndices[],
                              Subset const & vtxSubset) const;

    //  Methods to control sharpness of the corner in a subset:
    void SharpenSubset(Subset * subset) const;
    void SharpenSubset(Subset * subset, float sharpness) const;
    void UnSharpenSubset(Subset * subset) const;

public:
    //
    //  Public methods to query simple properties:
    //
    VertexTag GetTag() const { return _tag; }

    int GetFace() const { return _faceInRing; }

    int GetNumFaces()        const { return _vDesc._numFaces; }
    int GetNumFaceVertices() const { return _numFaceVerts; }

    bool HasCommonFaceSize() const { return (_commonFaceSize > 0); }
    int  GetCommonFaceSize() const { return _commonFaceSize; }

public:
    //
    //  Public methods to inspect incident faces and connected neighbors:
    //
    int GetFaceSize(int face) const;

    //  Get neighbors of a specific face (return -1 if unconnected):
    int GetFaceNext(    int face) const;
    int GetFacePrevious(int face) const;

    //  Find faces relative to this face (require a known safe step size):
    int GetFaceAfter (int stepForwardFromCornerFace) const;
    int GetFaceBefore(int stepBackwardFromCornerFace) const;

    //  Get first and last faces of a subset:
    int GetFaceFirst(Subset const & subset) const;
    int GetFaceLast( Subset const & subset) const;

public:
    //
    //  Public methods to access indices assigned to incident faces:
    //
    int GetFaceIndexOffset(int face) const;

    Index GetFaceIndexAtCorner(Index const indices[]) const;

    Index GetFaceIndexAtCorner(int face, Index const indices[]) const;
    Index GetFaceIndexTrailing(int face, Index const indices[]) const;
    Index GetFaceIndexLeading( int face, Index const indices[]) const;

    bool FaceIndicesMatchAtCorner(  int f1, int f2, Index const indices[])const;
    bool FaceIndicesMatchAtEdgeEnd( int f1, int f2, Index const indices[])const;
    bool FaceIndicesMatchAcrossEdge(int f1, int f2, Index const indices[])const;

public:
    //
    //  Public methods for sharpness of the vertex or its incident edges:
    //
    float GetVertexSharpness() const;

    float GetFaceEdgeSharpness(int faceEdge) const;
    float GetFaceEdgeSharpness(int face, bool trailingEdge) const;

    bool IsFaceEdgeSharp(    int face, bool trailingEdge) const;
    bool IsFaceEdgeInfSharp( int face, bool trailingEdge) const;
    bool IsFaceEdgeSemiSharp(int face, bool trailingEdge) const;

    bool  HasImplicitVertexSharpness() const;
    float GetImplicitVertexSharpness() const;

private:
    //  Internal convenience methods:
    bool isOrdered()   const { return _tag.IsOrdered(); }
    bool isUnOrdered() const { return _tag.IsUnOrdered(); }
    bool isBoundary()  const { return _tag.IsBoundary(); }
    bool isInterior()  const { return _tag.IsInterior(); }
    bool isManifold()  const { return _tag.IsManifold(); }

    int getConnectedFaceNext(int face) const;
    int getConnectedFacePrev(int face) const;

private:
    //  Internal methods for assembling and managing subsets:
    int initCompleteSubset(Subset * subset) const;

    int findConnectedSubsetExtent(Subset * subset) const;

    int findFVarSubsetExtent(Subset const & vtxSubset,
                             Subset       * fvarSubset,
                             Index  const   fvarIndices[]) const;

    void adjustSubsetTags(Subset       * subset,
                          Subset const * superset = 0) const;

    bool subsetHasInfSharpEdges( Subset const & subset) const;
    bool subsetHasSemiSharpEdges(Subset const & subset) const;
    bool subsetHasIrregularFaces(Subset const & subset) const;

private:
    //  Internal methods to connect a set of unordered faces (given their
    //  associated face-vertex indices) and assess the resulting topology:
    struct Edge;

    int  createUnOrderedEdges(Edge        edges[],
                              short       faceEdgeIndices[],
                              Index const faceVertIndices[]) const;

    void markDuplicateEdges(Edge        edges[],
                            short const faceEdgeIndices[],
                            Index const faceVertIndices[]) const;

    void assignUnOrderedFaceNeighbors(Edge const  edges[],
                                      short const faceEdgeIndices[]);

    void finalizeUnOrderedTags(Edge const edges[], int numEdges);

    //  Ordered counterpart to the above method for finalizing tags
    void finalizeOrderedTags();

private:
    typedef Vtr::internal::StackBuffer<short,16,true> ShortBuffer;

    //  Private members:
    VertexDescriptor _vDesc;
    VertexTag        _tag;

    short _faceInRing;
    short _commonFaceSize;

    unsigned char _regFaceSize;
    unsigned char _isExpInfSharp  :  1;
    unsigned char _isExpSemiSharp :  1;
    unsigned char _isImpInfSharp  :  1;
    unsigned char _isImpSemiSharp :  1;

    int _numFaceVerts;

    ShortBuffer _faceEdgeNeighbors;
};


//
//  Inline methods for inspecting/traversing incident faces of the vertex:
//
inline int
FaceVertex::GetFaceSize(int face) const {
    return _commonFaceSize ? _commonFaceSize :
            (_vDesc._faceSizeOffsets[face+1] - _vDesc._faceSizeOffsets[face]);
}

inline int
FaceVertex::getConnectedFaceNext(int face) const {
    return _faceEdgeNeighbors[2*face + 1];
}
inline int
FaceVertex::getConnectedFacePrev(int face) const {
    return _faceEdgeNeighbors[2*face];
}

inline int
FaceVertex::GetFaceNext(int face) const {
    if (isUnOrdered()) {
        return getConnectedFaceNext(face);
    } else if (face < (_vDesc._numFaces - 1)) {
        return face + 1;
    } else {
        return isBoundary() ? -1 : 0;
    }
}
inline int
FaceVertex::GetFacePrevious(int face) const {
    if (isUnOrdered()) {
        return getConnectedFacePrev(face);
    } else if (face) {
        return face - 1;
    } else {
        return isBoundary() ? -1 : (_vDesc._numFaces - 1);
    }
}

inline int
FaceVertex::GetFaceAfter(int step) const {
    assert(step >= 0);
    if (isOrdered()) {
        return (_faceInRing + step) % _vDesc._numFaces;
    } else if (step == 1) {
        return getConnectedFaceNext(_faceInRing);
    } else if (step == 2) {
        return getConnectedFaceNext(getConnectedFaceNext(_faceInRing));
    } else {
        int face = _faceInRing;
        for ( ; step > 0; --step) {
            face = getConnectedFaceNext(face);
        }
        return face;
    }
}
inline int
FaceVertex::GetFaceBefore(int step) const {
    assert(step >= 0);
    if (isOrdered()) {
        return (_faceInRing - step + _vDesc._numFaces) % _vDesc._numFaces;
    } else if (step == 1) {
        return getConnectedFacePrev(_faceInRing);
    } else if (step == 2) {
        return getConnectedFacePrev(getConnectedFacePrev(_faceInRing));
    } else {
        int face = _faceInRing;
        for ( ; step > 0; --step) {
            face = getConnectedFacePrev(face);
        }
        return face;
    }
}

inline int
FaceVertex::GetFaceFirst(Subset const & subset) const {
    return GetFaceBefore(subset._numFacesBefore);
}
inline int
FaceVertex::GetFaceLast(Subset const & subset) const {
    return GetFaceAfter(subset._numFacesAfter);
}

//
//  Inline methods for accessing indices associated with incident faces:
//
inline int
FaceVertex::GetFaceIndexOffset(int face) const {
    return _commonFaceSize ? (face * _commonFaceSize) :
                             _vDesc._faceSizeOffsets[face];
}

inline FaceVertex::Index
FaceVertex::GetFaceIndexAtCorner(Index const indices[]) const {
    return indices[GetFaceIndexOffset(_faceInRing)];
}
inline FaceVertex::Index
FaceVertex::GetFaceIndexAtCorner(int face, Index const indices[]) const {
    return indices[GetFaceIndexOffset(face)];
}
inline FaceVertex::Index
FaceVertex::GetFaceIndexLeading(int face, Index const indices[]) const {
    return indices[GetFaceIndexOffset(face) + 1];
}
inline FaceVertex::Index
FaceVertex::GetFaceIndexTrailing(int face, Index const indices[]) const {
    // It is safe to use "face+1" here for the last face:
    return indices[GetFaceIndexOffset(face+1) - 1];
}

inline bool
FaceVertex::FaceIndicesMatchAtCorner(int facePrev, int faceNext,
                                         Index const indices[]) const {
    return GetFaceIndexAtCorner(facePrev, indices) ==
           GetFaceIndexAtCorner(faceNext, indices);
}
inline bool
FaceVertex::FaceIndicesMatchAtEdgeEnd(int facePrev, int faceNext,
                                         Index const indices[]) const {
    return GetFaceIndexTrailing(facePrev, indices) ==
           GetFaceIndexLeading(faceNext, indices);
}
inline bool
FaceVertex::FaceIndicesMatchAcrossEdge(int facePrev, int faceNext,
                                         Index const indices[]) const {
    return FaceIndicesMatchAtCorner (facePrev, faceNext, indices) &&
           FaceIndicesMatchAtEdgeEnd(facePrev, faceNext, indices);
}

//
//  Inline methods for accessing vertex and edge sharpness:
//
inline float
FaceVertex::GetVertexSharpness() const {
    return _vDesc._vertSharpness;
}

inline float
FaceVertex::GetFaceEdgeSharpness(int faceEdge) const {
    return _vDesc._faceEdgeSharpness[faceEdge];
}
inline float
FaceVertex::GetFaceEdgeSharpness(int face, bool trailing) const {
    return _vDesc._faceEdgeSharpness[face*2 + trailing];
}

inline bool
FaceVertex::IsFaceEdgeSharp(int face, bool trailing) const {
    return Sdc::Crease::IsSharp(_vDesc._faceEdgeSharpness[face*2+trailing]);
}
inline bool
FaceVertex::IsFaceEdgeInfSharp(int face, bool trailing) const {
    return Sdc::Crease::IsInfinite(_vDesc._faceEdgeSharpness[face*2+trailing]);
}
inline bool
FaceVertex::IsFaceEdgeSemiSharp(int face, bool trailing) const {
    return Sdc::Crease::IsSemiSharp(_vDesc._faceEdgeSharpness[face*2+trailing]);
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_FACE_VERTEX_H */
