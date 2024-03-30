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

#include "../bfr/faceVertex.h"
#include "../sdc/crease.h"

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <map>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  Main initialize and finalize methods used to bracket the assignment
//  by clients to the VertexDescriptor member:
//
void
FaceVertex::Initialize(int faceSize, int regFaceSize) {

    _commonFaceSize = (short) faceSize;
    _regFaceSize    = (unsigned char) regFaceSize;
    _numFaceVerts   = 0;

    _isExpInfSharp  = false;
    _isExpSemiSharp = false;
    _isImpInfSharp  = false;
    _isImpSemiSharp = false;

    _vDesc._isValid       = false;
    _vDesc._isInitialized = false;
}

void
FaceVertex::Finalize(int faceInVertex) {

    assert(_vDesc._isFinalized);

    _faceInRing = (short) faceInVertex;

    //
    //  Initialize members from the VertexDescriptor:
    //
    if (!_vDesc.HasIncidentFaceSizes()) {
        //  Common face size was previously initialized to the face size
        _numFaceVerts = _vDesc._numFaces * _commonFaceSize;
    } else {
        _commonFaceSize = 0;
        //  Recall face sizes are available as differences between offsets:
        _numFaceVerts = _vDesc._faceSizeOffsets[_vDesc._numFaces];
    }

    //  Vertex sharpness:
    _isExpInfSharp  = Sdc::Crease::IsInfinite(_vDesc._vertSharpness);
    _isExpSemiSharp = Sdc::Crease::IsSemiSharp(_vDesc._vertSharpness);

    //
    //  Initialize tags from VertexDescriptor and other members
    //
    //  Note that not all tags can be assigned at this point if the vertex
    //  is defined by a set of unordered faces. In such cases, the tags
    //  will be assigned later when the connectivity between incident faces
    //  is determined. Those that can be assigned regardless of ordering
    //  are set here -- splitting the assignment of those remaining between
    //  ordered and unordered cases.
    //
    _tag.Clear();

    _tag._unCommonFaceSizes  = _vDesc.HasIncidentFaceSizes();
    _tag._irregularFaceSizes = (_commonFaceSize != _regFaceSize);

    _tag._infSharpVerts  = _isExpInfSharp;
    _tag._semiSharpVerts = _isExpSemiSharp;

    _tag._unOrderedFaces = !_vDesc.IsManifold();

    if (_vDesc.IsManifold()) {
        finalizeOrderedTags();
    }
}

void
FaceVertex::finalizeOrderedTags() {

    //
    //  A vertex with a set of ordered faces is required to be manifold:
    //
    _tag._unOrderedFaces   = false;
    _tag._nonManifoldVerts = false;

    _tag._boundaryVerts    = _vDesc.IsBoundary();
    _tag._boundaryNonSharp = _vDesc.IsBoundary();

    //
    //  Assign tags (and other members) affected by edge sharpness:
    //
    if (_vDesc.HasEdgeSharpness()) {
        float const * sharpness = &_vDesc._faceEdgeSharpness[0];

        //  Detect unsharpened boundary edges:
        bool isBoundary = _tag._boundaryVerts;
        if (isBoundary) {
            int last = 2 * _vDesc._numFaces - 1;
            _tag._boundaryNonSharp =
                    !Sdc::Crease::IsInfinite(sharpness[0]) ||
                    !Sdc::Crease::IsInfinite(sharpness[last]);
        }

        //  Detect interior inf-sharp and semi-sharp edges:
        int numInfSharpEdges  = 0;
        int numSemiSharpEdges = 0;

        for (int i = isBoundary; i < _vDesc._numFaces; ++i ) {
            if (Sdc::Crease::IsInfinite(sharpness[2*i])) {
                ++ numInfSharpEdges;
            } else if (Sdc::Crease::IsSharp(sharpness[2*i])) {
                ++ numSemiSharpEdges;
            }
        }

        //  Mark the presence of interior sharp edges:
        _tag._infSharpEdges  = (numInfSharpEdges > 0);
        _tag._semiSharpEdges = (numSemiSharpEdges > 0);
        _tag._infSharpDarts  = (numInfSharpEdges == 1) && !isBoundary;

        //  Detect edges effectively making the vertex sharp -- note that
        //  a vertex can be both explicitly and implicitly sharp (e.g. low
        //  semi-sharp vertex value with a higher semi-sharp edge):
        int numInfSharpTotal = numInfSharpEdges + isBoundary * 2;
        if (numInfSharpTotal > 2) {
            _isImpInfSharp = true;
        } else if ((numInfSharpTotal + numSemiSharpEdges) > 2) {
            _isImpSemiSharp = true;
        }

        //  Mark the vertex inf-sharp if implicitly inf-sharp:
        if (!_isExpInfSharp && _isImpInfSharp) {
            _tag._infSharpVerts  = true;
            _tag._semiSharpVerts = false;
        }
    }
}

bool
FaceVertex::HasImplicitVertexSharpness() const {

    return _isImpInfSharp || _isImpSemiSharp;
}

float
FaceVertex::GetImplicitVertexSharpness() const {

    if (_isImpInfSharp) {
        return Sdc::Crease::SHARPNESS_INFINITE;
    }
    assert(_isImpSemiSharp);

    //
    //  Since this will be applied at an inf-sharp crease, there will be
    //  two inf-sharp edges in addition to the semi-sharp, so we only
    //  need find the max of the semi-sharp edges and whatever explicit
    //  vertex sharpness may have been assigned.  Iterate through all
    //  faces and inspect the sharpness of each leading interior edge:
    //
    float sharpness = GetVertexSharpness();

    for (int i = 0; i < GetNumFaces(); ++i) {
        if (GetFacePrevious(i) >= 0) {
            sharpness = std::max(sharpness, GetFaceEdgeSharpness(2*i));
        }
    }
    return sharpness;
}

//
//  Methods to initialize and/or find subsets of the corner's topology:
//
int
FaceVertex::initCompleteSubset(Subset * subsetPtr) const {

    Subset & subset = *subsetPtr;

    //
    //  Initialize with tags and assign the extent:
    //
    int numFaces = GetNumFaces();

    subset.Initialize(GetTag());

    subset._numFacesTotal = (short) numFaces;
    if (isInterior()) {
        subset._numFacesBefore = 0;
        subset._numFacesAfter  = (short)(numFaces - 1);
    } else if (isOrdered()) {
        subset._numFacesBefore = _faceInRing;
        subset._numFacesAfter  = (short)(numFaces - 1 - subset._numFacesBefore);
    } else {
        //  Unordered faces -- boundary needs to identify its orientation:
        subset._numFacesAfter = 0;
        for (int f = GetFaceNext(_faceInRing); f >= 0; f = GetFaceNext(f)) {
            ++ subset._numFacesAfter;
        }
        subset._numFacesBefore = (short)(numFaces - 1 - subset._numFacesAfter);
    }
    return subset._numFacesTotal;
}

int
FaceVertex::findConnectedSubsetExtent(Subset * subsetPtr) const {

    Subset & subset = *subsetPtr;

    //
    //  Initialize with tags and mark manifold:
    //
    subset.Initialize(GetTag());

    subset._tag._nonManifoldVerts = false;

    //  Add faces to the dflt single face extent by seeking forward/backward:
    int fStart = _faceInRing;

    for (int f = GetFaceNext(fStart); f >= 0; f = GetFaceNext(f)) {
        if (f == fStart) {
            //  Periodic -- tag as such and return:
            subset.SetBoundary(false);
            return subset._numFacesTotal;
        }
        subset._numFacesAfter ++;
        subset._numFacesTotal ++;
    }
    for (int f = GetFacePrevious(fStart); f >= 0; f = GetFacePrevious(f)) {
        subset._numFacesBefore ++;
        subset._numFacesTotal ++;
    }
    subset.SetBoundary(true);
    return subset._numFacesTotal;
}

int
FaceVertex::GetVertexSubset(Subset * subsetPtr) const {

    //
    //  The subset from a manifold vertex is trivially complete (ordered
    //  or not), but for non-manifold cases we need to search and update
    //  the tags according to the content of the subset:
    //
    if (isManifold()) {
        initCompleteSubset(subsetPtr);
    } else {
        findConnectedSubsetExtent(subsetPtr);

        adjustSubsetTags(subsetPtr);

        //  And if on a non-manifold crease, test for implicit sharpness:
        if (!subsetPtr->IsSharp() && HasImplicitVertexSharpness()) {
            SharpenSubset(subsetPtr, GetImplicitVertexSharpness());
        }
    }
    return subsetPtr->_numFacesTotal;
}

int
FaceVertex::findFVarSubsetExtent(Subset const & vtxSub,
                                 Subset       * fvarSubsetPtr,
                                 Index  const   fvarIndices[]) const {

    Subset & fvarSub = *fvarSubsetPtr;

    //
    //  Initialize with tags and declare as a boundary to start:
    //
    fvarSub.Initialize(vtxSub._tag);

    fvarSub.SetBoundary(true);

    if (vtxSub._numFacesTotal == 1) return 1;

    //
    //  Inspect/gather faces "after" (counter-clockwise order from) the
    //  corner face.  If we arrive back at the corner face, a periodic
    //  set is complete, but check the continuity of the seam and apply
    //  before returning:
    //
    int cornerFace = _faceInRing;

    int numFacesAfterToVisit = vtxSub._numFacesAfter;
    if (numFacesAfterToVisit) {
        int thisFace = cornerFace;
        int nextFace = GetFaceNext(thisFace);
        for (int i = 0; i < numFacesAfterToVisit; ++i) {
            if (!FaceIndicesMatchAcrossEdge(thisFace, nextFace, fvarIndices)) {
                break;
            }
            ++ fvarSub._numFacesAfter;
            ++ fvarSub._numFacesTotal;

            thisFace = nextFace;
            nextFace = GetFaceNext(thisFace);
        }

        if (nextFace == cornerFace) {
            assert(vtxSub._numFacesBefore == 0);
            if (FaceIndicesMatchAtEdgeEnd(thisFace, cornerFace, fvarIndices)) {
                fvarSub.SetBoundary(false);
            }
            return fvarSub._numFacesTotal;
        }
    }

    //
    //  Inspect/gather faces "before" (clockwise order from) the corner
    //  face.  Include any faces "after" in the case of a periodic vertex
    //  that was interrupted by a discontinuity above:
    //
    int numFacesBeforeToVisit = vtxSub._numFacesBefore;
    if (!vtxSub.IsBoundary()) {
        numFacesBeforeToVisit += vtxSub._numFacesAfter - fvarSub._numFacesAfter;
    }
    if (numFacesBeforeToVisit) {
        int thisFace = cornerFace;
        int prevFace = GetFacePrevious(thisFace);
        for (int i = 0; i < numFacesBeforeToVisit; ++i) {
            if (!FaceIndicesMatchAcrossEdge(prevFace, thisFace, fvarIndices)) {
                break;
            }
            ++ fvarSub._numFacesBefore;
            ++ fvarSub._numFacesTotal;

            thisFace = prevFace;
            prevFace = GetFacePrevious(thisFace);
        }
    }
    return fvarSub._numFacesTotal;
}

int
FaceVertex::FindFaceVaryingSubset(Subset       * fvarSubsetPtr,
                                  Index  const   fvarIndices[],
                                  Subset const & vtxSub) const {

    Subset & fvarSub = *fvarSubsetPtr;

    //
    //  Find the face-varying extent and update the tags if its topology
    //  is a true subset of the vertex.  Also reset the sharpness in this
    //  case as the rules for the FVar interpolation options (applied
    //  later) take precedence over those of the vertex:
    //
    findFVarSubsetExtent(vtxSub, &fvarSub, fvarIndices);

    bool fvarTopologyMatchesVertex = fvarSub.ExtentMatchesSuperset(vtxSub);
    if (!fvarTopologyMatchesVertex) {
        if (fvarSub.IsSharp()) {
            UnSharpenSubset(&fvarSub);
        }
        adjustSubsetTags(&fvarSub, &vtxSub);
    }

    //  Sharpen if the vertex is non-manifold:
    if (!fvarSub.IsSharp() && !isManifold()) {
        SharpenSubset(&fvarSub);
    }

    //  Sharpen if the face-varying value is non-manifold, i.e. if there
    //  are any occurrences of the corner FVar index outside the subset:
    if (!fvarSub.IsSharp() && (fvarSub.GetNumFaces() < vtxSub.GetNumFaces())) {
        Index fvarMatch = GetFaceIndexAtCorner(fvarIndices);

        int numMatches = 0;
        for (int i = 0; i < GetNumFaces(); ++i) {
            numMatches += (GetFaceIndexAtCorner(i, fvarIndices) == fvarMatch);
            if (numMatches > fvarSub.GetNumFaces()) {
                SharpenSubset(&fvarSub);
                break;
            }
        }
    }
    return fvarSub.GetNumFaces();
}

//
//  Method to revise the tags for a subset of the corner, which may no
//  longer include properties that trigger exceptional behavior:
//
void
FaceVertex::SharpenSubset(Subset * subset) const {

    //  Mark the subset sharp and ensure any related tags are also
    //  updated accordingly:
    subset->_tag._infSharpVerts  = true;
    subset->_tag._semiSharpVerts = false;
}
void
FaceVertex::UnSharpenSubset(Subset * subset) const {

    //  Restore subset sharpness based on actual sharpness assignment:
    subset->_tag._infSharpVerts  = _isExpInfSharp;
    subset->_tag._semiSharpVerts = _isExpSemiSharp;
}
void
FaceVertex::SharpenSubset(Subset * subset, float sharpness) const {

    //  Mark the subset according to sharpness value
    if (sharpness > subset->_localSharpness) {
        subset->_localSharpness = sharpness;

        subset->_tag._infSharpVerts  = Sdc::Crease::IsInfinite(sharpness);
        subset->_tag._semiSharpVerts = Sdc::Crease::IsSemiSharp(sharpness);
    }
}

bool
FaceVertex::subsetHasIrregularFaces(Subset const & subset) const {

    assert(_tag.HasIrregularFaceSizes());

    if (!_tag._unCommonFaceSizes) return true;

    int f = GetFaceFirst(subset);
    for (int i = 0; i < subset.GetNumFaces(); ++i, f = GetFaceNext(f)) {
        if (GetFaceSize(f) != _regFaceSize) return true;
    }
    return false;
}

bool
FaceVertex::subsetHasInfSharpEdges(Subset const & subset) const {

    assert(_tag.HasInfSharpEdges());

    int n = subset.GetNumFaces();
    if (n > 1) {
        int f = GetFaceFirst(subset);
        //  Reduce number of faces to visit when inspecting trailing edges:
        for (int i = subset.IsBoundary(); i < n; ++i, f = GetFaceNext(f)) {
            if (IsFaceEdgeInfSharp(f, 1)) return true;
        }
    }
    return false;
}

bool
FaceVertex::subsetHasSemiSharpEdges(Subset const & subset) const {

    assert(_tag.HasSemiSharpEdges());

    int n = subset.GetNumFaces();
    if (n > 1) {
        int f = GetFaceFirst(subset);
        //  Reduce number of faces to visit when inspecting trailing edges:
        for (int i = subset.IsBoundary(); i < n; ++i, f = GetFaceNext(f)) {
            if (IsFaceEdgeSemiSharp(f, 1)) return true;
        }
    }
    return false;
}

void
FaceVertex::adjustSubsetTags(Subset       * subset,
                             Subset const * superset) const {

    VertexTag & subsetTag = subset->_tag;

    //  Adjust any tags related to boundary or sharpness status:
    if (subsetTag.IsBoundary()) {
        subsetTag._infSharpDarts = false;
    }
    if (subsetTag.IsInfSharp()) {
        subsetTag._semiSharpVerts = false;
    }

    //  Adjust for the presence of irregular faces or sharp edges if the
    //  subset is actually a proper subset of this entire corner or the
    //  optionally provided superset:
    int  numSuperFaces = superset ? superset->GetNumFaces() : GetNumFaces();
    bool superBoundary = superset ? superset->IsBoundary()  : isBoundary();

    if ((subset->GetNumFaces() < numSuperFaces) ||
        (subset->IsBoundary() != superBoundary)) {

        if (subsetTag._irregularFaceSizes) {
            subsetTag._irregularFaceSizes = subsetHasIrregularFaces(*subset);
        }
        if (subsetTag._infSharpEdges) {
            subsetTag._infSharpEdges = subsetHasInfSharpEdges(*subset);
            if (subsetTag._infSharpEdges && subset->IsBoundary()) {
                SharpenSubset(subset);
            }
        }
        if (subsetTag._semiSharpEdges) {
            subsetTag._semiSharpEdges = subsetHasSemiSharpEdges(*subset);
        }
    }
}


//
//  Main and supporting internal datatypes and methods to connect unordered
//  faces and allow for topological traversals of the incident faces:
//
//  The fundamental element of this process is the following definition of
//  an Edge. It is lightweight and only stores a state (boundary, interior,
//  or non-manifold) along with the one or two faces for a manifold edge.
//  It is initialized as a boundary when first created and is then modified
//  by adding additional incident faces.
//
struct FaceVertex::Edge {
    //  Empty constructor intentional since we over-allocate what we need:
    Edge() { }

    void clear() { std::memset(this, 0, sizeof(*this)); }
    void Initialize(Index vtx) { clear(), endVertex = vtx; }

    //  Transition of state as incident faces are added:
    void SetBoundary()    { boundary = 1; }
    void SetInterior()    { boundary = 0, interior = 1; }
    void SetNonManifold() { boundary = 0, interior = 0, nonManifold = 1; }

    //  Special cases forcing non-manifold
    void SetDegenerate()  { SetNonManifold(), degenerate = 1; }
    void SetDuplicate()   { SetNonManifold(), duplicate = 1; }

    void SetSharpness(float sharpness) {
        if (sharpness > 0.0f) {
            if (Sdc::Crease::IsInfinite(sharpness)) {
                infSharp = true;
            } else {
                semiSharp = true;
            }
        }
    }

    void SetFace(int newFace, bool newTrailing) {
        trailing = newTrailing;
        *(trailing ? &prevFace : &nextFace) = (short) newFace;
    }

    void AddFace(int newFace, bool newTrailing) {

        //  Update the state of the Edge based on the added incident face:
        if (boundary) {
            if (newTrailing == trailing) {
                //  Edge is reversed
                SetNonManifold();
            } else if (newFace == (trailing ? prevFace : nextFace)) {
                //  Edge is repeated in the face
                SetNonManifold();
            } else {
                //  Edge is manifold thus far -- promote to interior
                SetInterior();
                SetFace(newFace, newTrailing);
            }
        } else if (interior) {
            //  More than two incident faces -- make non-manifold
            SetNonManifold();
        }
    }

    Index endVertex;

    unsigned short boundary    : 1;
    unsigned short interior    : 1;
    unsigned short nonManifold : 1;
    unsigned short trailing    : 1;
    unsigned short degenerate  : 1;
    unsigned short duplicate   : 1;
    unsigned short infSharp    : 1;
    unsigned short semiSharp   : 1;

    short prevFace, nextFace;
};

void
FaceVertex::ConnectUnOrderedFaces(Index const fvIndices[]) {

    //
    //  There are two transient sets of data needed here:  a set of Edges
    //  that connect adjoining faces, and a set of indices (one for each
    //  of the 2*N face-edges) to identify the Edge for each face-edge.
    //
    //  IMPORTANT -- since these later edge indices are of the same type
    //  and size as the internal face-edge neighbors, we'll use that array
    //  to avoid a separate declaration (and possible allocation) and will
    //  update it in place later.
    //
    int numFaceEdges = GetNumFaces() * 2;

    _faceEdgeNeighbors.SetSize(numFaceEdges);

    //  Allocate and populate the edges and indices referring to them.
    //  Initialization fails to detect some "duplicate" edges in a face,
    //  so post-process to catch these before continuing:
    Vtr::internal::StackBuffer<Edge,32,true> edges(numFaceEdges);

    short * feEdges = &_faceEdgeNeighbors[0];

    int numEdges = createUnOrderedEdges(edges, feEdges, fvIndices);

    markDuplicateEdges(edges, feEdges, fvIndices);

    //  Use the connecting edges to assign neighboring faces (overwriting
    //  our edge indices) and finish initializing the tags retaining the
    //  properties of the corner:
    assignUnOrderedFaceNeighbors(edges, feEdges);

    finalizeUnOrderedTags(edges, numEdges);
}

//
//  Identify a set of shared edges between unordered faces so that we can
//  establish connections between them.
//
//  The "face-edge edges" are really just half-edges that refer (by index)
//  to potentially shared Edges.  As Edges are created, these half-edges
//  are made to refer to them, after which the state of the edge may change
//  due to the presence or orientation of additional incident faces.
//
int
FaceVertex::createUnOrderedEdges(Edge        edges[],
                                 short       feEdges[],
                                 Index const fvIndices[]) const {

    //  Optional map to help construction for high valence:
    typedef std::map<Index,int> EdgeMap;

    EdgeMap edgeMap;

    bool useMap = (GetNumFaces() > 16);

    //
    //  Iterate through the face-edge pairs to find connecting edges:
    //
    Index vCorner = GetFaceIndexAtCorner(0, fvIndices);

    int numFaceEdges = 2 * GetNumFaces();
    int numEdges = 0;

    //  Don't rely on the tag yet to determine presence of sharpness:
    bool hasSharpness = _vDesc.HasEdgeSharpness();

    for (int feIndex = 0; feIndex < numFaceEdges; ++feIndex) {
        Index vIndex = (feIndex & 1) ?
                       GetFaceIndexTrailing((feIndex >> 1), fvIndices) :
                       GetFaceIndexLeading( (feIndex >> 1), fvIndices);

        int eIndex = -1;
        if (vIndex != vCorner) {
            if (useMap) {
                EdgeMap::iterator eFound = edgeMap.find(vIndex);
                if (eFound != edgeMap.end()) {
                    eIndex = eFound->second;
                } else {
                    //  Make sure to create the new edge below at this index
                    edgeMap[vIndex] = numEdges;
                }
            } else {
                for (int j = 0; j < numEdges; ++j) {
                    if (edges[j].endVertex == vIndex) {
                        eIndex = j;
                        break;
                    }
                }
            }

            //  Update an existing edge or create a new one
            if (eIndex >= 0) {
                edges[eIndex].AddFace(feIndex >> 1, feIndex & 1);
            } else {
                //  Index of the new (pre-allocated) edge:
                eIndex = numEdges ++;

                //  Initialize a new edge as boundary (manifold)
                Edge & E = edges[eIndex];
                E.Initialize(vIndex);
                E.SetBoundary();
                E.SetFace(feIndex >> 1, feIndex & 1);
                if (hasSharpness) {
                    E.SetSharpness(GetFaceEdgeSharpness(feIndex));
                }
            }
        } else {
            //  If degenerate, create unique edge (non-manifold)
            eIndex = numEdges++;

            edges[eIndex].Initialize(vIndex);
            edges[eIndex].SetDegenerate();
        }
        assert(eIndex >= 0);
        feEdges[feIndex] = (short) eIndex;
    }
    return numEdges;
}

void
FaceVertex::markDuplicateEdges(Edge        edges[],
                               short const feEdges[],
                               Index const fvIndices[]) const {

    //
    //  The edge assignment thus far does not correctly detect the presence
    //  of all edges repeated or duplicated in the same face, e.g. for quad
    //  with vertices {A, B, A, C} the edge AB occurs both as AB and BA.
    //  When the face is oriented relative to corner B, we have {B, A, C, A}
    //  and edge BA will be detected as non-manifold -- but not from corner
    //  A or C.
    //
    //  So look for repeated instances of the corner vertex in the face and
    //  inspect its neighbors to see if they match the leading or trailing
    //  edges.
    //
    //  This is a trivial test for a quad:  if the opposite vertex matches
    //  the corner vertex, both the leading and trailing edges will be
    //  duplicated and so can immediately be marked non-manifold.  So deal
    //  with the common case of all neighboring quads separately.
    //
    if (_commonFaceSize == 3) return;

    Index vCorner = fvIndices[0];
    int numFaces = GetNumFaces();

    if (_commonFaceSize == 4) {
        Index const * fvOpposite = fvIndices + 2;
        for (int face = 0; face < numFaces; ++face, fvOpposite += 4) {
            if (*fvOpposite == vCorner) {
                edges[feEdges[2*face  ]].SetDuplicate();
                edges[feEdges[2*face+1]].SetDuplicate();
            }
        }
    } else {
        Index const * fv = fvIndices;

        for (int face = 0; face < numFaces; ++face) {
            int faceSize = GetFaceSize(face);

            if (faceSize == 4) {
                if (fv[2] == vCorner) {
                    edges[feEdges[2*face  ]].SetDuplicate();
                    edges[feEdges[2*face+1]].SetDuplicate();
                }
            } else {
                for (int j = 2; j < (faceSize - 2); ++j) {
                    if (fv[j] == vCorner) {
                        if (fv[j-1] == fv[1])
                            edges[feEdges[2*face  ]].SetDuplicate();
                        if (fv[j+1] == fv[faceSize-1])
                            edges[feEdges[2*face+1]].SetDuplicate();
                    }
                }
            }
            fv += faceSize;
        }
    }
}

void
FaceVertex::assignUnOrderedFaceNeighbors(Edge const  edges[],
                                         short const feEdges[]) {

    int numFaceEdges = 2 * GetNumFaces();

    for (int i = 0; i < numFaceEdges; ++i) {
        assert(feEdges[i] >= 0);

        Edge const & E = edges[feEdges[i]];
        bool edgeIsSingular = E.nonManifold || E.boundary;
        if (edgeIsSingular) {
            _faceEdgeNeighbors[i] = -1;
        } else {
            _faceEdgeNeighbors[i] = (i & 1) ? E.nextFace : E.prevFace;
        }
    }
}

void
FaceVertex::finalizeUnOrderedTags(Edge const edges[], int numEdges) {

    //
    //  Summarize properties of the corner given the number and nature of
    //  the edges around its vertex and initialize remaining members or
    //  tags that depend on them.
    //
    //  First, take inventory of relevant properties from the edges:
    //
    int numNonManifoldEdges = 0;
    int numInfSharpEdges    = 0;
    int numSemiSharpEdges   = 0;
    int numSingularEdges    = 0;

    bool hasBoundaryEdges         = false;
    bool hasBoundaryEdgesNotSharp = false;
    bool hasDegenerateEdges       = false;
    bool hasDuplicateEdges        = false;

    for (int i = 0; i < numEdges; ++i) {
        Edge const & E = edges[i];

        if (E.interior) {
            numInfSharpEdges  += E.infSharp;
            numSemiSharpEdges += E.semiSharp;
        } else if (E.boundary) {
            hasBoundaryEdges = true;
            hasBoundaryEdgesNotSharp |= !E.infSharp;
        } else {
            ++ numNonManifoldEdges;
            hasDegenerateEdges |= E.degenerate;
            hasDuplicateEdges  |= E.duplicate;
        }

        //  Singular edges include all that are effectively inf-sharp:
        numSingularEdges += E.nonManifold || E.boundary || E.infSharp;
    }

    //
    //  Next determine whether manifold or not.  Some obvious tests quickly
    //  indicate if the corner is non-manifold, but ultimately it will be
    //  necessary to traverse the faces to confirm that they form a single
    //  connected set (e.g. two cones sharing their apex vertex may appear
    //  manifold to this point but as two connected sets are non-manifold).
    //
    bool isNonManifold       = false;
    bool isNonManifoldCrease = false;

    if (numNonManifoldEdges) {
        isNonManifold = true;

        if (!hasDegenerateEdges && !hasDuplicateEdges && !hasBoundaryEdges) {
            //  Special crease case that avoids sharpening: two interior
            //  non-manifold edges radiating more than two sets of faces:
            isNonManifoldCrease = (numNonManifoldEdges == 2) &&
                                  (GetNumFaces() > numEdges);
        }
    } else {
        //  Mismatch between number of incident faces and edges:
        isNonManifold = ((numEdges - GetNumFaces()) != (int)hasBoundaryEdges);

        if (!isNonManifold) {
            //  If all faces are not connected, the set is non-manifold:
            Subset subset;
            int numFacesInSubset = findConnectedSubsetExtent(&subset);
            if (numFacesInSubset < GetNumFaces()) {
                isNonManifold = true;
            }
        }
    }

    //
    //  Assign tags and other members related to the inventory of edges
    //  (boundary status is relevant if non-manifold as it can affect
    //  the presence of the limit surface):
    //
    _tag._nonManifoldVerts = isNonManifold;

    _tag._boundaryVerts    = hasBoundaryEdges;
    _tag._boundaryNonSharp = hasBoundaryEdgesNotSharp;

    _tag._infSharpEdges  = (numInfSharpEdges > 0);
    _tag._semiSharpEdges = (numSemiSharpEdges > 0);
    _tag._infSharpDarts  = (numInfSharpEdges == 1) && !hasBoundaryEdges;

    //  Conditions effectively making the vertex sharp, include the usual
    //  excess of inf-sharp edges plus some non-manifold cases:
    if ((numSingularEdges > 2) || (isNonManifold && !isNonManifoldCrease)) {
        _isImpInfSharp = true;
    } else if ((numSingularEdges + numSemiSharpEdges) > 2) {
        _isImpSemiSharp = true;
    }

    //  Mark the vertex inf-sharp if implicitly inf-sharp:
    if (!_isExpInfSharp && _isImpInfSharp) {
        _tag._infSharpVerts = true;
        _tag._semiSharpVerts = false;
    }
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
