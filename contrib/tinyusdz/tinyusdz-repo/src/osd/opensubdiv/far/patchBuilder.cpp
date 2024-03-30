//
//   Copyright 2018 DreamWorks Animation LLC.
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

#include "../far/patchBuilder.h"
#include "../far/catmarkPatchBuilder.h"
#include "../far/loopPatchBuilder.h"
#include "../far/bilinearPatchBuilder.h"
#include "../vtr/level.h"
#include "../vtr/fvarLevel.h"
#include "../vtr/refinement.h"
#include "../vtr/stackBuffer.h"

#include <cassert>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

using Vtr::Array;
using Vtr::ConstArray;
using Vtr::internal::Level;
using Vtr::internal::FVarLevel;
using Vtr::internal::Refinement;
using Vtr::internal::StackBuffer;

namespace Far {

//
//  Local helper functions for topology queries:
//
namespace {

    //
    //  Inline methods to encapsulate fast and specific modulus operations.
    //  The mod-4 case is trivial.  For mod-N, since we are always doing
    //  the test (i + 1) % N, or (i - 1 + N) % N, the subtraction suffices.
    //
    inline int fastMod4(int x) {
        return x & 0x3;
    }
    inline int fastMod3(int x) {
        static int const mod3Array[] = { 0, 1, 2, 0, 1, 2 };
        return mod3Array[x];
    }
    inline int fastModN(int x, int N) {
        return (x < N) ? x : (x - N);
    }

    //
    //  Local helper functions for identifying the subset of a ring around a
    //  corner that contributes to a patch -- parameterized by a mask that
    //  indicates what kind of edge is to delimit the span.
    //
    //  Note that the two main methods need both face-verts and face-edges
    //  for each corner, and that we don't really need the face-index once
    //  we have them -- consider passing the fVerts and fEdges as arguments
    //  as they will otherwise be retrieved repeatedly for each corner.
    //
    //  (As these mature it is likely they will be moved to Vtr, as a method
    //  to identify a VSpan would complement the existing method to gather
    //  the vertex/values associated with it.  The manifold vs non-manifold
    //  choice would then also be encapsulated -- provided both remain free
    //  of PatchTable-specific logic.)
    //
    inline Level::ETag
    getSingularEdgeMask(bool includeAllInfSharpEdges = false) {

        Level::ETag eTagMask;
        eTagMask.clear();
        eTagMask._boundary = true;
        eTagMask._nonManifold = true;
        eTagMask._infSharp = includeAllInfSharpEdges;
        return eTagMask;
    }

    inline bool
    isEdgeSingular(Level const & level, FVarLevel const * fvarLevel,
                   Index eIndex, Level::ETag eTagMask)
    {
        Level::ETag eTag = level.getEdgeTag(eIndex);
        if (fvarLevel) {
            eTag = fvarLevel->getEdgeTag(eIndex).combineWithLevelETag(eTag);
        }
        return (eTag.getBits() & eTagMask.getBits()) > 0;
    }

    void
    identifyManifoldCornerSpan(Level const & level, Index fIndex,
                               int fCorner, Level::ETag eTagMask,
                               Level::VSpan & vSpan, int fvc = -1)
    {
        FVarLevel const * fvarLevel = (fvc < 0) ? 0 : &level.getFVarLevel(fvc);

        ConstIndexArray fVerts = level.getFaceVertices(fIndex);
        ConstIndexArray fEdges = level.getFaceEdges(fIndex);

        ConstIndexArray vEdges = level.getVertexEdges(fVerts[fCorner]);
        int             nEdges = vEdges.size();

        int iLeadingStart  = vEdges.FindIndex(fEdges[fCorner]);
        int iTrailingStart = fastModN(iLeadingStart + 1, nEdges);

        vSpan.clear();
        vSpan._numFaces = 1;
        vSpan._cornerInSpan = 0;

        int iLeading  = iLeadingStart;
        while (! isEdgeSingular(level, fvarLevel, vEdges[iLeading], eTagMask)) {
            ++vSpan._numFaces;
            ++vSpan._cornerInSpan;
            iLeading = fastModN(iLeading + nEdges - 1, nEdges);
            if (iLeading == iTrailingStart) break;
        }

        int iTrailing = iTrailingStart;
        if (iTrailing != iLeading) {
            while (!isEdgeSingular(level, fvarLevel, vEdges[iTrailing], eTagMask)) {
                ++vSpan._numFaces;
                iTrailing = fastModN(iTrailing + 1, nEdges);
                if (iTrailing == iLeadingStart) break;
            }
        }
        vSpan._startFace = (LocalIndex) iLeading;
    }

    void
    identifyNonManifoldCornerSpan(Level const & level, Index fIndex,
                                  int fCorner, Level::ETag eTagMask,
                                  Level::VSpan & vSpan, int fvc = -1)
    {
        FVarLevel const * fvarLevel = (fvc < 0) ? 0 : &level.getFVarLevel(fvc);

        ConstIndexArray fEdges = level.getFaceEdges(fIndex);

        Index eLeadingStart  = fEdges[fCorner];
        Index eTrailingStart = fEdges[(fCorner + fEdges.size() - 1) % fEdges.size()];

        vSpan.clear();
        vSpan._numFaces = 1;
        vSpan._cornerInSpan = 0;

        Index startFace   = fIndex;
        int   startCorner = fCorner;

        //  Traverse clockwise to find the leading edge of the span -- keeping
        //  track of the starting face and corner to use later:
        Index fLeading = fIndex;
        Index eLeading = eLeadingStart;
        while (!isEdgeSingular(level, fvarLevel, eLeading, eTagMask)) {
            ++vSpan._numFaces;
            ++vSpan._cornerInSpan;

            //  Identify the face opposite the current leading edge, identify
            //  the edges of the next face, and then the next leading edge:
            //
            ConstIndexArray eFaces = level.getEdgeFaces(eLeading);
            assert(eFaces.size() == 2);
            fLeading = (eFaces[0] == fLeading) ? eFaces[1] : eFaces[0];
            fEdges = level.getFaceEdges(fLeading);

            startFace   = fLeading;
            startCorner = (fEdges.FindIndex(eLeading) + 1) % fEdges.size();

            eLeading = fEdges[startCorner];
            if (eLeading == eTrailingStart) {
                vSpan._periodic = !isEdgeSingular(level, fvarLevel, eLeading, eTagMask);
                break;
            }
        }

        //  Traverse counter-clockwise to find the trailing edge of the span (unless
        //  the above traversal reached where it started):
        Index fTrailing = fIndex;
        Index eTrailing = eTrailingStart;
        if (eTrailing != eLeading) {
            while (!isEdgeSingular(level, fvarLevel, eTrailing, eTagMask)) {
                ++vSpan._numFaces;

                //  Identify the face opposite the current trailing edge, identify
                //  the edges of the next face, and then the next trailing edge:
                //
                ConstIndexArray eFaces = level.getEdgeFaces(eTrailing);
                assert(eFaces.size() == 2);
                fTrailing = (eFaces[0] == fTrailing) ? eFaces[1] : eFaces[0];
                fEdges = level.getFaceEdges(fTrailing);

                eTrailing = fEdges[(fEdges.FindIndex(eTrailing) + fEdges.size() - 1) % fEdges.size()];
                if (eTrailing == eLeadingStart) {
                    vSpan._periodic = !isEdgeSingular(level, fvarLevel, eTrailing, eTagMask);
                    break;
                }
            }
        }

        //  Identify the span's starting point relative to the incident components
        //  of the vertex, using the start face and corner of the leading edge:
        //
        Index vIndex = level.getFaceVertices(fIndex)[fCorner];

        ConstIndexArray      vFaces   = level.getVertexFaces(vIndex);
        ConstLocalIndexArray vInFaces = level.getVertexFaceLocalIndices(vIndex);

        vSpan._startFace = (LocalIndex) vFaces.size();
        for (int i = 0; i < vFaces.size(); ++i) {
            if ((vFaces[i] == startFace) && (vInFaces[i] == startCorner)) {
                vSpan._startFace = (LocalIndex) i;
                break;
            }
        }
        assert(vSpan._startFace < vFaces.size());
    }

    //  Simple conveniences for the span search functions:
    inline int
    countManifoldCornerSpan(Level const & level, Index fIndex, int fCorner,
                            Level::ETag eTagMask, int fvc = -1)
    {
        Level::VSpan vSpan;
        identifyManifoldCornerSpan(level, fIndex, fCorner, eTagMask, vSpan, fvc);
        return vSpan._numFaces;
    }
    inline int
    countNonManifoldCornerSpan(Level const & level, Index fIndex, int fCorner,
                               Level::ETag eTagMask, int fvc = -1)
    {
        Level::VSpan vSpan;
        identifyNonManifoldCornerSpan(level, fIndex, fCorner, eTagMask, vSpan, fvc);
        return vSpan._numFaces;
    }


    //
    //  Gathering the one-ring of vertices from triangles surrounding a vertex:
    //      - the neighborhood of the vertex is assumed to be tri-regular (manifold)
    //
    //  Ordering of resulting vertices:
    //      The surrounding one-ring follows the ordering of the incident faces.  For each
    //  incident tri, the vertex opposite its leading edge is added.  If the vertex is on a
    //  boundary, a second vertex on the boundary edge will be contributed from the last face.
    //
    int
    gatherTriRegularRingAroundVertex(Level const& level,
        Index vIndex, int ringPoints[], int fvarChannel) {

        ConstIndexArray vEdges = level.getVertexEdges(vIndex);

        ConstIndexArray vFaces = level.getVertexFaces(vIndex);
        ConstLocalIndexArray vInFaces = level.getVertexFaceLocalIndices(vIndex);

        bool isBoundary = (vEdges.size() > vFaces.size());

        int ringIndex = 0;
        for (int i = 0; i < vFaces.size(); ++i) {
            //
            //  For each tri, we want the the vertex at the end of the leading edge:
            //
            ConstIndexArray fPoints = (fvarChannel < 0)
                                    ? level.getFaceVertices(vFaces[i])
                                    : level.getFaceFVarValues(vFaces[i], fvarChannel);

            int vInThisFace = vInFaces[i];

            ringPoints[ringIndex++] = fPoints[fastMod3(vInThisFace + 1)];

            if (isBoundary && (i == (vFaces.size() - 1))) {
                ringPoints[ringIndex++] = fPoints[fastMod3(vInThisFace + 2)];
            }
        }
        return ringIndex;
    }

    int
    gatherRegularPartialRingAroundVertex(Level const& level,
        Index vIndex, Level::VSpan const & span, int ringPoints[], int fvarChannel) {

        bool isManifold = !level.isVertexNonManifold(vIndex);

        ConstIndexArray      vFaces   = level.getVertexFaces(vIndex);
        ConstLocalIndexArray vInFaces = level.getVertexFaceLocalIndices(vIndex);

        int nFaces    = span._numFaces;
        int startFace = span._startFace;

        Index nextFace    = vFaces[startFace];
        int   vInNextFace = vInFaces[startFace];

        int ringIndex = 0;
        for (int i = 0; i < nFaces; ++i) {
            Index thisFace    = nextFace;
            int   vInThisFace = vInNextFace;

            ConstIndexArray fPoints = (fvarChannel < 0)
                                    ? level.getFaceVertices(thisFace)
                                    : level.getFaceFVarValues(thisFace, fvarChannel);

            bool isQuad = (fPoints.size() == 4);
            if (isQuad) {
                ringPoints[ringIndex++] = fPoints[fastMod4(vInThisFace + 1)];
                ringPoints[ringIndex++] = fPoints[fastMod4(vInThisFace + 2)];
            } else {
                ringPoints[ringIndex++] = fPoints[fastMod3(vInThisFace + 1)];
            }

            if (i == (nFaces - 1)) {
                if (!span._periodic) {
                    if (isQuad) {
                        ringPoints[ringIndex++] = fPoints[fastMod4(vInThisFace + 3)];
                    } else {
                        ringPoints[ringIndex++] = fPoints[fastMod3(vInThisFace + 2)];
                    }
                }
            } else if (isManifold) {
                int iNext = fastModN(startFace + i + 1, vFaces.size());

                nextFace    = vFaces[iNext];
                vInNextFace = vInFaces[iNext];
            } else {
                int nextInThisFace = fastModN(vInThisFace + fPoints.size() - 1, fPoints.size());

                Index nextEdge = level.getFaceEdges(thisFace)[nextInThisFace];
                ConstIndexArray eFaces = level.getEdgeFaces(nextEdge);

                nextFace    = (eFaces[0] == thisFace) ? eFaces[1] : eFaces[0];
                vInNextFace = level.getFaceEdges(nextFace).FindIndex(nextEdge);
            }
        }
        return ringIndex;
    }

    //
    //  Functions to encode/decode the 5-bit boundary mask for a triangular patch
    //  from the two 3-bit boundary vertex and bounday edge masks.  When referring
    //  to a "boundary vertex" in the encoded bits, we are referring to a vertex on
    //  a boundary while its incident edges of the triangle are not boundaries --
    //  topologically distinct from a vertex at the end of a boundary edge.
    //
    //  The 5-bit encoding is as follows:
    //
    //      - the upper 2 bits indicate how to interpret the lower 3 bits:
    //          0 - as boundary edges only (all boundary vertices are implicit)
    //          1 - as "boundary vertices" only (no boundary edges)
    //          2 - a single boundary edge with opposite "boundary vertex"
    //
    //      - the lower 3 bits are set according to boundary features present
    //
    //  There are a total of 18 possible boundary configurations:
    //
    //      - no boundaries at all (1 case)
    //      - one boundary edge (3 cases)
    //      - two boundary edges (3 cases)
    //      - three boundary edges (1 case)
    //      - one boundary vertex (3 cases)
    //      - two boundary vertices (3 cases)
    //      - three boundarey vertices (1 case)
    //      - one boundary edge with opposite boundary vertex (3 cases)
    //
    inline int unpackTriBoundaryMaskLower(int mask) { return mask & 0x7; }
    inline int unpackTriBoundaryMaskUpper(int mask) { return (mask >> 3) & 0x3; }

    inline int packTriBoundaryMask(int upper, int lower) { return (upper << 3) | lower; }

    int
    encodeTriBoundaryMask(int eBits, int vBits)
    {
        int upperBits = 0;
        int lowerBits = eBits;

        if (vBits) {
            if (eBits == 0) {
                upperBits = 1;
                lowerBits = vBits;
            } else if ((vBits == 7) && ((eBits == 1) || (eBits == 2) || (eBits == 4))) {
                upperBits = 2;
                lowerBits = eBits;
            }
        }
        return packTriBoundaryMask(upperBits, lowerBits);
    }
    void
    decodeTriBoundaryMask(int mask, int & eBits, int & vBits)
    {
        static int const eBitsToVBits[] = { 0, 3, 6, 7, 5, 7, 7, 7 };

        int lowerBits = unpackTriBoundaryMaskLower(mask);
        int upperBits = unpackTriBoundaryMaskUpper(mask);

        switch (upperBits) {
        case 0:
            eBits = lowerBits;
            vBits = eBitsToVBits[eBits];
            break;
        case 1:
            eBits = 0;
            vBits = lowerBits;
            break;
        case 2:
            eBits = lowerBits;
            vBits = 0x7;
            break;
        }
    }

    inline Index
    getNextFaceInVertFaces(Level const & level, int thisFaceInVFaces,
                           ConstIndexArray const & vFaces,
                           ConstLocalIndexArray const & vInFaces,
                           bool manifold, int & vInNextFace) {

        Index nextFace;
        if (manifold) {
            int nextFaceInVFaces = fastModN(thisFaceInVFaces + 1, vFaces.size());

            nextFace    = vFaces[nextFaceInVFaces];
            vInNextFace = vInFaces[nextFaceInVFaces];
        } else {
            Index thisFace    = vFaces[thisFaceInVFaces];
            int   vInThisFace = vInFaces[thisFaceInVFaces];

            ConstIndexArray fEdges = level.getFaceEdges(thisFace);

            Index nextEdge = fEdges[fastModN((vInThisFace + fEdges.size() - 1), fEdges.size())];

            ConstIndexArray eFaces = level.getEdgeFaces(nextEdge);
            assert(eFaces.size() == 2);

            nextFace = (eFaces[0] == thisFace) ? eFaces[1] : eFaces[0];

            int edgeInNextFace = level.getFaceEdges(nextFace).FindIndex(nextEdge);

            vInNextFace = edgeInNextFace;
        }
        return nextFace;
    }
    inline Index
    getPrevFaceInVertFaces(Level const & level, int thisFaceInVFaces,
                           ConstIndexArray const & vFaces,
                           ConstLocalIndexArray const & vInFaces,
                           bool manifold, int & vInPrevFace) {

        Index prevFace;
        if (manifold) {
            int prevFaceInVFaces = (thisFaceInVFaces ? thisFaceInVFaces : vFaces.size()) - 1;

            prevFace    = vFaces[prevFaceInVFaces];
            vInPrevFace = vInFaces[prevFaceInVFaces];
        } else {
            Index thisFace    = vFaces[thisFaceInVFaces];
            int   vInThisFace = vInFaces[thisFaceInVFaces];

            ConstIndexArray fEdges = level.getFaceEdges(thisFace);

            Index prevEdge = fEdges[vInThisFace];

            ConstIndexArray eFaces = level.getEdgeFaces(prevEdge);
            assert(eFaces.size() == 2);

            prevFace = (eFaces[0] == thisFace) ? eFaces[1] : eFaces[0];

            int edgeInPrevFace = level.getFaceEdges(prevFace).FindIndex(prevEdge);

            vInPrevFace = fastModN(edgeInPrevFace + 1, fEdges.size());
        }
        return prevFace;
    }

    inline ConstIndexArray
    getFacePoints(Level const& level, Index faceIndex, int fvarChannel)
    {
        return (fvarChannel < 0) ? level.getFaceVertices(faceIndex)
                                 : level.getFaceFVarValues(faceIndex, fvarChannel);
    }

} // namespace anon


//
//  Factory method and constructor:
//
PatchBuilder*
PatchBuilder::Create(TopologyRefiner const& refiner, Options const& options) {

    switch (refiner.GetSchemeType()) {
    case Sdc::SCHEME_BILINEAR:
        return new BilinearPatchBuilder(refiner, options);
    case Sdc::SCHEME_CATMARK:
        return new CatmarkPatchBuilder(refiner, options);
    case Sdc::SCHEME_LOOP:
        return new LoopPatchBuilder(refiner, options);
    }
    assert("Unrecognized Sdc::SchemeType for PatchBuilder construction" == 0);
    return 0;
}

PatchBuilder::PatchBuilder(
    TopologyRefiner const& refiner, Options const& options) :
        _refiner(refiner), _options(options) {

    //
    //  Initialize members with properties of the subdivision scheme and patch
    //  choices for quick retrieval:
    //
    _schemeType        = refiner.GetSchemeType();
    _schemeRegFaceSize = Sdc::SchemeTypeTraits::GetRegularFaceSize(_schemeType);
    _schemeIsLinear    = Sdc::SchemeTypeTraits::GetLocalNeighborhoodSize(_schemeType) == 0;

    //  Initialization of members involving patch types is deferred to the
    //  subclass for the scheme
}

PatchBuilder::~PatchBuilder() {
}


//
//  Topology inspections methods for a particular face in the hierarchy:
//
bool
PatchBuilder::IsFaceAPatch(int levelIndex, Index faceIndex) const {

    Level const & level = _refiner.getLevel(levelIndex);

    //  Faces tagged as holes are not patches (no limit surface)
    if (_refiner.HasHoles() && level.isFaceHole(faceIndex)) return false;

    //  Base faces are patches unless an irregular face or incident one:
    if (levelIndex == 0) {
        if (_schemeIsLinear) {
            return level.getFaceVertices(faceIndex).size() == _schemeRegFaceSize;
        } else {
            return !level.getFaceCompositeVTag(faceIndex)._incidIrregFace;
        }
    }
    
    //  Refined faces are patches unless "incomplete", i.e. they exist solely to
    //  support an adjacent patch (can only use the more commonly used combined
    //  VTag for all corners for quads -- need a Refinement tag for tris):
    if (_schemeRegFaceSize == 4) {
        return !level.getFaceCompositeVTag(faceIndex)._incomplete;
    } else {
        Refinement const & refinement = _refiner.getRefinement(levelIndex - 1);
        return !refinement.getChildFaceTag(faceIndex)._incomplete;
    }
}

bool
PatchBuilder::IsFaceALeaf(int levelIndex, Index faceIndex) const {

    //  All faces in the last level are leaves
    if (levelIndex < _refiner.GetMaxLevel()) {
        //  Faces selected for further refinement are not leaves
        if (_refiner.getRefinement(levelIndex).
                        getParentFaceSparseTag(faceIndex)._selected) {
            return false;
        }
    }
    return true;
}

bool
PatchBuilder::IsPatchRegular(int levelIndex, Index faceIndex, int fvc) const {

    if (_schemeIsLinear) {
        //  The previous face-is-a-patch test precludes an irregular patch
        return true;
    }

    Level const & level = _refiner.getLevel(levelIndex);

    //
    //  Retrieve individual VTags for the four corners and combine, as we may
    //  need the individual VTags for closer inspection.
    //
    //  Immediately return regular status based on xordinary bit if completely
    //  smooth at all corners, i.e. no inf-sharp corners or boundaries present
    //  (which also rules out the presence of non-manifold vertices)
    //
    Level::VTag vTags[4];
    level.getFaceVTags(faceIndex, vTags, fvc);

    Level::VTag fCompVTag = Level::VTag::BitwiseOr(vTags, _schemeRegFaceSize);

    if (!fCompVTag._infSharp && !fCompVTag._infSharpEdges) {
        return !fCompVTag._xordinary;
    }

    //
    //  Irregular features will exist at corners that are either non-manifold,
    //  extra-ordinary, or that are tagged with inf-sharp irregularities (may
    //  be regular even if extra-ordinary or vice versa -- depending on the
    //  specific inf-sharp edges present around the vertex).
    //
    //  Build a bit-mask for the irregular features -- if the composite tag
    //  has no irregular features, we can immediately return.
    //
    bool testInfSharpFeatures = !_options.approxInfSharpWithSmooth;

    Level::VTag irregFeatureTag(0);
    irregFeatureTag._nonManifold  = true;
    irregFeatureTag._xordinary    = true;
    irregFeatureTag._infIrregular = testInfSharpFeatures;

    int irregFeatureMask = irregFeatureTag.getBits(); 

    if ((fCompVTag.getBits() & irregFeatureMask) == 0) {
        return true;
    }

    //
    //  If the irregular feature is isolated, we can use the combined corner
    //  tags to determine regularity -- unless specified options require a
    //  closer inspection of the single irregular feature:
    //
    bool mayHaveIrregFaces  = _refiner._hasIrregFaces;
    int  needsExtraIsoLevel = fCompVTag._xordinary && mayHaveIrregFaces;

    bool featureIsIsolated = levelIndex > needsExtraIsoLevel;
    if (featureIsIsolated) {
        bool featureRequiresFurtherInspection = fCompVTag._nonManifold ||
                (_options.approxSmoothCornerWithSharp &&
                        fCompVTag._xordinary && fCompVTag._boundary) ||
                (testInfSharpFeatures &&
                        fCompVTag._infIrregular && fCompVTag._infSharpEdges);

        if (!featureRequiresFurtherInspection) {
            if (testInfSharpFeatures) {
                return !fCompVTag._infIrregular;
            } else {
                return !fCompVTag._xordinary;
            }
        }
    }
    
    //
    //  Inspect all or the single isolated corner.  Use the irregular feature
    //  mask to quickly skip regular corners and return on the first irregular
    //  feature encountered:
    //
    int nRegBoundaryFaces = (_schemeRegFaceSize == 4) ? 2 : 3;

    for (int i = 0; i < _schemeRegFaceSize; ++i) {
        Level::VTag vTag = vTags[i];

        if ((vTag.getBits() & irregFeatureMask) == 0) continue;

        if (vTag._nonManifold) {
            //  Identify the span containing the face and assess:
            int nSpanFaces = countNonManifoldCornerSpan(level, faceIndex, i,
                getSingularEdgeMask(testInfSharpFeatures), fvc);
            if (vTag._infSharp) {
                if (nSpanFaces != 1) return false;
            } else {
                if (nSpanFaces != (nRegBoundaryFaces)) return false;
            }
            continue;
        }

        if (vTag._xordinary) {
            //  A smooth xordinary vertex is always irregular
            if (!vTag._infSharpEdges) return false;

            //  A smooth corner vertex may be interpreted as regular:
            if (_options.approxSmoothCornerWithSharp &&
                    vTag._boundary && !vTag._infSharp) {
                Level::ETag eTags[4];
                level.getFaceETags(faceIndex, eTags, fvc);
                int iPrev = i ? (i - 1) : (_schemeRegFaceSize - 1);
                if (eTags[i]._boundary && eTags[iPrev]._boundary) {
                    continue;
                }
            }

            //  All others irregular, unless further inspecting inf-sharp
            if (!testInfSharpFeatures) return false;
        }

        if (vTag._infIrregular) {
            //  Inf-sharp vertex with no inf-sharp edges:
            if (!vTag._infSharpEdges) return false;

            //  Irregular boundary crease:
            if (vTag._infSharpCrease && vTag._boundary) return false;

            //  Identify the span containing the face and assess:
            int nSpanFaces = countManifoldCornerSpan(level, faceIndex, i,
                getSingularEdgeMask(true), fvc);
            if (vTag._infSharpCrease) {
                if (nSpanFaces != (nRegBoundaryFaces)) return false;
            } else {
                if (nSpanFaces != 1) return false;
            }
        }
    }
    return true;
}

int
PatchBuilder::GetRegularPatchBoundaryMask(int levelIndex, Index faceIndex,
    int fvarChannel) const {

    if (_schemeIsLinear) {
        //  Boundaries for patches not dependent on the 1-ring are ignored
        return 0;
    }

    Level const & level = _refiner.getLevel(levelIndex);

    //  Gather tags for the four corners and edges.  Regardless of the
    //  options for treating non-manifold or inf-sharp patches, for a
    //  regular patch we can infer all that we need from these tags:
    //
    Level::VTag vTags[4];
    Level::ETag eTags[4];

    level.getFaceVTags(faceIndex, vTags, fvarChannel);

    Level::VTag fTag = Level::VTag::BitwiseOr(vTags, _schemeRegFaceSize);
    if (!fTag._infSharpEdges) {
        return 0;
    }

    level.getFaceETags(faceIndex, eTags, fvarChannel);

    //
    //  Test the edge tags for boundary features.  For quads this is
    //  sufficient, so return the edge bits.
    //
    bool testInfSharpFeatures = !_options.approxInfSharpWithSmooth;

    Level::ETag eFeatureTag(0);
    eFeatureTag._boundary    = true;
    eFeatureTag._infSharp    = testInfSharpFeatures;
    eFeatureTag._nonManifold = true;

    int eFeatureMask = eFeatureTag.getBits();

    int eBits = (((eTags[0].getBits() & eFeatureMask) != 0) << 0) |
                (((eTags[1].getBits() & eFeatureMask) != 0) << 1) |
                (((eTags[2].getBits() & eFeatureMask) != 0) << 2);
    if (_schemeRegFaceSize == 4) {
        eBits |= (((eTags[3].getBits() & eFeatureMask) != 0) << 3);
        return eBits;
    }

    //
    //  For triangles, test the vertex tags for boundary features (we
    //  can have boundary vertices with no boundary edges) and return
    //  the encoded result of the two sets of 3 bits:
    //
    Level::VTag vFeatureTag(0);
    vFeatureTag._boundary      = true;
    vFeatureTag._infSharpEdges = testInfSharpFeatures;
    vFeatureTag._nonManifold   = true;

    int vFeatureMask = vFeatureTag.getBits();

    int vBits = (((vTags[0].getBits() & vFeatureMask) != 0) << 0) |
                (((vTags[1].getBits() & vFeatureMask) != 0) << 1) |
                (((vTags[2].getBits() & vFeatureMask) != 0) << 2);

    return (eBits || vBits) ? encodeTriBoundaryMask(eBits, vBits) : 0;
}

void
PatchBuilder::GetIrregularPatchCornerSpans(int levelIndex, Index faceIndex,
        Level::VSpan cornerSpans[4], int fvarChannel) const {

    Level const & level = _refiner.getLevel(levelIndex);

    //  Retrieve tags and identify other information for the corner vertices:
    Level::VTag vTags[4];
    level.getFaceVTags(faceIndex, vTags, fvarChannel);

    FVarLevel::ValueTag fvarTags[4];
    if (fvarChannel >= 0) {
        level.getFVarLevel(fvarChannel).getFaceValueTags(faceIndex, fvarTags);
    }

    //
    //  For each corner, identify the span of interest around the vertex,
    //  using the complete neighborhood when possible (which does not require
    //  a search):
    //
    bool testInfSharpFeatures = !_options.approxInfSharpWithSmooth;

    Level::ETag singularEdgeMask = getSingularEdgeMask(testInfSharpFeatures);

    for (int i = 0; i < _schemeRegFaceSize; ++i) {
        Level::VTag vTag = vTags[i];

        bool isNonManifold = vTag._nonManifold;

        bool isFVarMisMatch = (fvarChannel >= 0) && fvarTags[i]._mismatch;

        bool testInfSharpEdges = testInfSharpFeatures &&
                vTag._infSharpEdges && (vTag._rule != Sdc::Crease::RULE_DART);

        //
        //  Identify a discontinuity in the one-ring, otherwise use an
        //  unassigned (cleared) span to indicate use of the full ring:
        //
        if (testInfSharpEdges || isFVarMisMatch || isNonManifold) {
            if (isNonManifold) {
                identifyNonManifoldCornerSpan(level, faceIndex,
                        i, singularEdgeMask, cornerSpans[i], fvarChannel);
            } else {
                identifyManifoldCornerSpan(level, faceIndex,
                        i, singularEdgeMask, cornerSpans[i], fvarChannel);
            }
        } else {
            cornerSpans[i].clear();
        }

        //  Sharpen the span if a corner or subject to inf-sharp features:
        if (vTag._corner) {
            //  Corners tagged in FVar space need additional qualification:
            if (isFVarMisMatch) {
                cornerSpans[i]._sharp = (cornerSpans[i]._numFaces == 1) || isNonManifold;
            } else {
                cornerSpans[i]._sharp = true;
            }
        } else if (isNonManifold) {
            cornerSpans[i]._sharp = vTag._infSharp;
        } else if (testInfSharpFeatures) {
            cornerSpans[i]._sharp = testInfSharpEdges
                    ? !vTag._infSharpCrease : vTag._infSharp;
        }

        //  Legacy option -- reinterpret a smooth corner as sharp:
        bool smoothCorner = !cornerSpans[i]._sharp;
        if (smoothCorner && _options.approxSmoothCornerWithSharp && vTag._xordinary &&
                vTag._boundary && !vTag._infSharp && !vTag._nonManifold) {
            int nFaces = cornerSpans[i].isAssigned()
                ? cornerSpans[i]._numFaces
                : level.getVertexFaces(level.getFaceVertices(faceIndex)[i]).size();
            cornerSpans[i]._sharp = (nFaces == 1);
        }
    }
}

int
PatchBuilder::getRegularFacePoints(int levelIndex, Index faceIndex,
        Index patchPoints[], int fvarChannel) const {

    Level const & level = _refiner.getLevel(levelIndex);

    ConstIndexArray facePoints = (fvarChannel < 0)
                               ? level.getFaceVertices(faceIndex)
                               : level.getFaceFVarValues(faceIndex, fvarChannel);

    for (int i = 0; i < facePoints.size(); ++i) {
        patchPoints[i] = facePoints[i];
    }
    return facePoints.size();
}

int
PatchBuilder::getQuadRegularPatchPoints(int levelIndex, Index faceIndex,
        int regBoundaryMask, Index patchPoints[],
        int fvarChannel) const {

    if (regBoundaryMask < 0) {
        regBoundaryMask = GetRegularPatchBoundaryMask(levelIndex, faceIndex);
    }
    bool interiorPatch = (regBoundaryMask == 0);

    static int const patchPointsPerCorner[4][4] = { {  5,  4,  0,  1 },
                                                    {  6,  2,  3,  7 },
                                                    { 10, 11, 15, 14 },
                                                    {  9, 13, 12,  8 } };

    int eMask = regBoundaryMask;

    Level const & level = _refiner.getLevel(levelIndex);

    ConstIndexArray fVerts  = level.getFaceVertices(faceIndex);
    ConstIndexArray fPoints = getFacePoints(level, faceIndex, fvarChannel);

    Index boundaryPoint = INDEX_INVALID;
    if (!interiorPatch && _options.fillMissingBoundaryPoints) {
        boundaryPoint = fPoints[0];
    }

    for (int i = 0; i < 4; ++i) {
        Index v = fVerts[i];

        const int* cornerPointIndices = patchPointsPerCorner[i];

        ConstIndexArray      vFaces   = level.getVertexFaces(v);
        ConstLocalIndexArray vInFaces = level.getVertexFaceLocalIndices(v);

        //  Identify the patch face in the ring of incident faces.  (There's
        //  no need to deal with multiple occurrences of the face in the ring
        //  here -- as can happen with non-manifold vertices -- as such corners
        //  will be sharp, regular boundaries and not need the incident faces.)
        //
        Index f = faceIndex;
        int fInVFaces = vFaces.FindIndex(f);

        //  Identify the exterior points for this corner from the appropriate 
        //  incident face:
        //
        bool interiorCorner = interiorPatch || (((eMask & (1 << i)) |
                                                 (eMask & (1 << fastMod4(i+3)))) == 0);
        if (interiorCorner) {
            int fOppInVFaces = fastMod4(fInVFaces + 2);

            Index fOpp = vFaces[fOppInVFaces];
            int vInFOpp = vInFaces[fOppInVFaces];
            ConstIndexArray fOppPoints = getFacePoints(level, fOpp, fvarChannel);

            patchPoints[cornerPointIndices[1]] = fOppPoints[fastMod4(vInFOpp + 1)];
            patchPoints[cornerPointIndices[2]] = fOppPoints[fastMod4(vInFOpp + 2)];
            patchPoints[cornerPointIndices[3]] = fOppPoints[fastMod4(vInFOpp + 3)];
        } else if ((eMask & (1 << i)) && (eMask & (1 << fastMod4(i+3)))) {
            //  Two indicent boundary edges -- no incident faces
            patchPoints[cornerPointIndices[1]] = boundaryPoint;
            patchPoints[cornerPointIndices[2]] = boundaryPoint;
            patchPoints[cornerPointIndices[3]] = boundaryPoint;
        } else if (eMask & (1 << i)) {
            //  Leading/outgoing boundary edge -- need next face:
            int vInFNext;
            Index fNext = getNextFaceInVertFaces(level, fInVFaces, vFaces, vInFaces,
                                   !level.getVertexTag(v)._nonManifold, vInFNext);

            ConstIndexArray fNextPoints = getFacePoints(level, fNext, fvarChannel);

            patchPoints[cornerPointIndices[1]] = fNextPoints[fastMod4(vInFNext + 3)];
            patchPoints[cornerPointIndices[2]] = boundaryPoint;
            patchPoints[cornerPointIndices[3]] = boundaryPoint;
        } else {
            //  Trailing/incoming boundary edge -- need previous face:
            int vInFPrev;
            Index fPrev = getPrevFaceInVertFaces(level, fInVFaces, vFaces, vInFaces,
                                   !level.getVertexTag(v)._nonManifold, vInFPrev);

            ConstIndexArray fPrevPoints = getFacePoints(level, fPrev, fvarChannel);

            patchPoints[cornerPointIndices[1]] = boundaryPoint;
            patchPoints[cornerPointIndices[2]] = boundaryPoint;
            patchPoints[cornerPointIndices[3]] = fPrevPoints[fastMod4(vInFPrev + 1)];
        }
        patchPoints[cornerPointIndices[0]] = fPoints[i];
    }
    return 16;
}

int
PatchBuilder::getTriRegularPatchPoints(int levelIndex, Index faceIndex,
        int regBoundaryMask, Index patchPoints[],
        int fvarChannel) const {

    if (regBoundaryMask < 0) {
        regBoundaryMask = GetRegularPatchBoundaryMask(levelIndex, faceIndex);
    }
    bool interiorPatch = (regBoundaryMask == 0);

    static int const patchPointsPerCorner[3][4] = { { 4, 7, 3, 0 },
                                                    { 5, 1, 2, 6 },
                                                    { 8, 9, 11, 10 } };

    int vMask = 0;
    int eMask = 0;
    if (!interiorPatch) {
        decodeTriBoundaryMask(regBoundaryMask, eMask, vMask);
    }

    Level const & level = _refiner.getLevel(levelIndex);

    ConstIndexArray fVerts  = level.getFaceVertices(faceIndex);
    ConstIndexArray fPoints = getFacePoints(level, faceIndex, fvarChannel);

    Index boundaryPoint = INDEX_INVALID;
    if (!interiorPatch && _options.fillMissingBoundaryPoints) {
        boundaryPoint = fPoints[0];
    }

    for (int i = 0; i < 3; ++i) {
        Index v = fVerts[i];

        const int* cornerPointIndices = patchPointsPerCorner[i];

        ConstIndexArray      vFaces   = level.getVertexFaces(v);
        ConstLocalIndexArray vInFaces = level.getVertexFaceLocalIndices(v);

        //  Identify the patch face in the ring of incident faces.  (There's
        //  no need to deal with multiple occurrences of the face in the ring
        //  here -- as can happen with non-manifold vertices -- as such corners
        //  will be sharp, regular boundaries and not need the incident faces.)
        //
        Index f = faceIndex;
        int fInVFaces = vFaces.FindIndex(f);

        //  Identify the exterior points for this corner from the appropriate 
        //  incident faces:
        //
        bool interiorCorner = interiorPatch || ((vMask & (1 << i)) == 0);
        if (interiorCorner) {
            int f2InVFaces = fastModN(fInVFaces + 2, 6);
            int f3InVFaces = fastModN(fInVFaces + 3, 6);

            Index f2 = vFaces[f2InVFaces];
            Index f3 = vFaces[f3InVFaces];

            int vInf2 = vInFaces[f2InVFaces];
            int vInf3 = vInFaces[f3InVFaces];

            ConstIndexArray f2Points = getFacePoints(level, f2, fvarChannel);
            ConstIndexArray f3Points = getFacePoints(level, f3, fvarChannel);

            patchPoints[cornerPointIndices[1]] = f2Points[fastMod3(vInf2 + 1)];
            patchPoints[cornerPointIndices[2]] = f3Points[fastMod3(vInf3 + 1)];
            patchPoints[cornerPointIndices[3]] = f3Points[fastMod3(vInf3 + 2)];
        } else if ((eMask & (1 << i)) && (eMask & (1 << fastMod3(i+2)))) {
            //  Two indicent boundary edges -- no incident faces

            patchPoints[cornerPointIndices[1]] = boundaryPoint;
            patchPoints[cornerPointIndices[2]] = boundaryPoint;
            patchPoints[cornerPointIndices[3]] = boundaryPoint;
        } else if (eMask & (1 << i)) {
            //  Leading/outgoing boundary edge, i.e. f0 of {f0,f1,f2}, need f2:
            int f2InVFaces = fastModN(fInVFaces + 2, vFaces.size());

            Index f2 = vFaces[f2InVFaces];
            int vInf2 = vInFaces[f2InVFaces];

            if (level.getVertexTag(v)._nonManifold) {
                Index f1 = getNextFaceInVertFaces(level, fInVFaces,
                                            vFaces, vInFaces, false, vInf2);
                f2 = getNextFaceInVertFaces(level, vFaces.FindIndex(f1),
                                            vFaces, vInFaces, false, vInf2);
            }
            ConstIndexArray f2Points = getFacePoints(level, f2, fvarChannel);

            patchPoints[cornerPointIndices[1]] = f2Points[fastMod3(vInf2 + 1)];
            patchPoints[cornerPointIndices[2]] = f2Points[fastMod3(vInf2 + 2)];
            patchPoints[cornerPointIndices[3]] = boundaryPoint;
        } else if (eMask & (1 << fastMod3(i+2))) {
            //  Trailing/incoming boundary edge, i.e. f2 of {f0,f1,f2}, need f0:
            int f0InVFaces = fastModN(fInVFaces + vFaces.size() - 2, vFaces.size());

            Index f0 = vFaces[f0InVFaces];
            int vInf0 = vInFaces[f0InVFaces];

            if (level.getVertexTag(v)._nonManifold) {
                Index f1 = getPrevFaceInVertFaces(level, fInVFaces,
                                            vFaces, vInFaces, false, vInf0);
                f0 = getPrevFaceInVertFaces(level, vFaces.FindIndex(f1),
                                            vFaces, vInFaces, false, vInf0);
            }
            ConstIndexArray f0Points = getFacePoints(level, f0, fvarChannel);

            patchPoints[cornerPointIndices[1]] = boundaryPoint;
            patchPoints[cornerPointIndices[2]] = boundaryPoint;
            patchPoints[cornerPointIndices[3]] = f0Points[fastMod3(vInf0 + 1)];
        } else {
            //  Boundary vertex on edge, i.e. f1 of {f0,f1,f2}, need next face f2:
            int vInf2;
            Index f2 = getNextFaceInVertFaces(level, fInVFaces, vFaces, vInFaces,
                                       !level.getVertexTag(v)._nonManifold, vInf2);

            ConstIndexArray f2Points = getFacePoints(level, f2, fvarChannel);

            patchPoints[cornerPointIndices[1]] = f2Points[fastMod3(vInf2 + 2)];
            patchPoints[cornerPointIndices[2]] = boundaryPoint;
            patchPoints[cornerPointIndices[3]] = boundaryPoint;
        }
        patchPoints[cornerPointIndices[0]] = fPoints[i];
    }
    return 12;
}

int
PatchBuilder::GetRegularPatchPoints(int levelIndex, Index faceIndex,
        int regBoundaryMask, Index patchPoints[],
        int fvarChannel) const {

    if (_schemeIsLinear) {
        return getRegularFacePoints(
            levelIndex, faceIndex, patchPoints, fvarChannel);
    } else if (_schemeRegFaceSize == 4) {
        return getQuadRegularPatchPoints(
            levelIndex, faceIndex, regBoundaryMask, patchPoints, fvarChannel);
    } else {
        return getTriRegularPatchPoints(
            levelIndex, faceIndex, regBoundaryMask, patchPoints, fvarChannel);
    }
    return 0;
}

int
PatchBuilder::assembleIrregularSourcePatch(
        int levelIndex, Index faceIndex, Level::VSpan const cornerSpans[],
        SourcePatch & sourcePatch) const {

    //
    //  Initialize the four Patch corners and finalize the patch:
    //
    Level const & level = _refiner.getLevel(levelIndex);

    ConstIndexArray fVerts = level.getFaceVertices(faceIndex);

    for (int corner = 0; corner < fVerts.size(); ++corner) {
        //
        //  Retrieve corner properties from the VSpan when explicitly assigned.
        //  Otherwise, identify properties from the incident faces and tags and
        //  find the face for the patch within the set of incident faces:
        //
        Level::VTag vTag = level.getVertexTag(fVerts[corner]);

        SourcePatch::Corner & patchCorner = sourcePatch._corners[corner];

        if (cornerSpans[corner].isAssigned()) {
            patchCorner._numFaces  = cornerSpans[corner]._numFaces;
            patchCorner._patchFace = cornerSpans[corner]._cornerInSpan;
            patchCorner._boundary  = !cornerSpans[corner]._periodic;
        } else {
            ConstIndexArray vFaces = level.getVertexFaces(fVerts[corner]);

            patchCorner._numFaces  = (LocalIndex) vFaces.size();
            patchCorner._patchFace = (LocalIndex) vFaces.FindIndex(faceIndex);
            patchCorner._boundary  = vTag._boundary;
        }
        patchCorner._sharp = cornerSpans[corner]._sharp;
        patchCorner._dart  = (vTag._rule == Sdc::Crease::RULE_DART) && vTag._infSharpEdges;
    }
    sourcePatch.Finalize(fVerts.size());

    return sourcePatch.GetNumSourcePoints();
}


//
//  Gather patch points from around the face of a level given a previously
//  initialized SourcePatch.  This is historically specific to an irregular
//  patch and still relies on the cornerSpans (which may or may not have been
//  initialized when the SourcePatch was created) rather than inspecting the
//  corners of the SourcePatch.
//
//  We need temporary/local space for rings around each corner -- both for
//  the Vtr::Level and the corresponding rings of the patch.
//
//  Get the corresponding rings from the Vtr::Level and the patch descriptor:
//  the values of the latter will be indices for points[] whose values will
//  come from values of former, i.e. points[localRing[i]] = sourceRing[i].
//  Points that overlap will be assigned multiple times, but messy logic to
//  deal with overlap while determining the correspondence is avoided.
//
int
PatchBuilder::gatherIrregularSourcePoints(
        int levelIndex, Index faceIndex,
        Level::VSpan const cornerSpans[4], SourcePatch & sourcePatch,
        Index patchVerts[], int fvarChannel) const {

    //
    //  Allocate temporary space for rings around the corners in both the Level
    //  and the Patch, then retrieve corresponding rings and assign the source
    //  vertices to the given array of patch points
    //
    int numSourceVerts = sourcePatch.GetNumSourcePoints();

    StackBuffer<Index,64,true> sourceRingVertices(sourcePatch.GetMaxRingSize());
    StackBuffer<Index,64,true> patchRingPoints(sourcePatch.GetMaxRingSize());

    Level const & level = _refiner.getLevel(levelIndex);

    ConstIndexArray faceVerts = level.getFaceVertices(faceIndex);
    for (int corner = 0; corner < sourcePatch._numCorners; ++corner) {
        Index cornerVertex = faceVerts[corner];
        
        //  Gather the ring of source points from the Vtr level:
        int sourceRingSize = 0;
        if (cornerSpans[corner].isAssigned()) {
            sourceRingSize = gatherRegularPartialRingAroundVertex(level,
                cornerVertex, cornerSpans[corner], sourceRingVertices,
                fvarChannel);
        } else if (sourcePatch._numCorners == 4) {
            sourceRingSize = level.gatherQuadRegularRingAroundVertex(
                cornerVertex, sourceRingVertices,
                fvarChannel);
        } else {
            sourceRingSize = gatherTriRegularRingAroundVertex(level,
                cornerVertex, sourceRingVertices,
                fvarChannel);
        }

        //  Gather the ring of local points from the patch:
        int patchRingSize = sourcePatch.GetCornerRingPoints(
                corner, patchRingPoints);
        assert(patchRingSize == sourceRingSize);

        //  Identify source points for corresponding local patch points of ring:
        for (int i = 0; i < patchRingSize; ++i) {
            assert(patchRingPoints[i] < numSourceVerts);
            patchVerts[patchRingPoints[i]] = sourceRingVertices[i];
        }
    }
    return numSourceVerts;
}

int
PatchBuilder::GetIrregularPatchSourcePoints(
        int levelIndex, Index faceIndex, Level::VSpan const cornerSpans[],
        Index sourcePoints[], int fvarChannel) const {

    SourcePatch sourcePatch;
    assembleIrregularSourcePatch(
            levelIndex, faceIndex, cornerSpans, sourcePatch);

    return gatherIrregularSourcePoints(levelIndex, faceIndex,
        cornerSpans, sourcePatch, sourcePoints, fvarChannel);
}

//
//  Template conversion methods for the matrix type -- explicit instantiation
//  for float and double is required and follows the definition:
//
template <typename REAL>
int
PatchBuilder::GetIrregularPatchConversionMatrix(
        int levelIndex, Index faceIndex,
        Level::VSpan const cornerSpans[],
        SparseMatrix<REAL> & conversionMatrix) const {

    SourcePatch sourcePatch;
    assembleIrregularSourcePatch(
            levelIndex, faceIndex, cornerSpans, sourcePatch);

    return convertToPatchType(
        sourcePatch, GetIrregularPatchType(), conversionMatrix);
}
template int PatchBuilder::GetIrregularPatchConversionMatrix<float>(
        int levelIndex, Index faceIndex, Level::VSpan const cornerSpans[],
        SparseMatrix<float> & conversionMatrix) const;
template int PatchBuilder::GetIrregularPatchConversionMatrix<double>(
        int levelIndex, Index faceIndex, Level::VSpan const cornerSpans[],
        SparseMatrix<double> & conversionMatrix) const;


bool
PatchBuilder::IsRegularSingleCreasePatch(int levelIndex, Index faceIndex,
        SingleCreaseInfo & creaseInfo) const {

    if (_schemeRegFaceSize != 4) return false;

    Level const & level = _refiner.getLevel(levelIndex);

    return level.isSingleCreasePatch(faceIndex,
                &creaseInfo.creaseSharpness, &creaseInfo.creaseEdgeInFace);
}   

PatchParam
PatchBuilder::ComputePatchParam(int levelIndex, Index faceIndex,
        PtexIndices const& ptexIndices, bool isRegular,
        int boundaryMask, bool computeTransitionMask) const {

    // Move up the hierarchy accumulating u,v indices to the coarse level:
    int depth = levelIndex;
    int childIndexInParent = 0,
        u = 0,
        v = 0,
        ofs = 1;

    int regFaceSize = _schemeRegFaceSize;

    bool irregBase =
        _refiner.GetLevel(depth).GetFaceVertices(faceIndex).size() !=
        regFaceSize;

    // For triangle refinement, the parameterization is rotated at
    // the fourth triangle subface at each level. The u and v values
    // computed for rotated triangles will be negative while we are
    // walking through the refinement levels.
    bool rotatedTriangle = false;

    int childFaceIndex = faceIndex;
    for (int i = depth; i > 0; --i) {
        Refinement const& refinement  = _refiner.getRefinement(i-1);
        Level const&      parentLevel = _refiner.getLevel(i-1);

        Index parentFaceIndex =
            refinement.getChildFaceParentFace(childFaceIndex);

        irregBase =
            parentLevel.getFaceVertices(parentFaceIndex).size() !=
            regFaceSize;

        if (_schemeRegFaceSize == 3) {
            // For now, we don't consider irregular faces for
            // triangle refinement.

            childIndexInParent =
                refinement.getChildFaceInParentFace(childFaceIndex);

            if (rotatedTriangle) {
                switch ( childIndexInParent ) {
                    case 0 :                     break;
                    case 1 : { u-=ofs;         } break;
                    case 2 : {         v-=ofs; } break;
                    case 3 : { u+=ofs; v+=ofs; rotatedTriangle = false; } break;
                }
            } else {
                switch ( childIndexInParent ) {
                    case 0 :                     break;
                    case 1 : { u+=ofs;         } break;
                    case 2 : {         v+=ofs; } break;
                    case 3 : { u-=ofs; v-=ofs; rotatedTriangle = true; } break;
                }
            }
            ofs = (unsigned short)(ofs << 1);
        } else if (!irregBase) {
            childIndexInParent =
                refinement.getChildFaceInParentFace(childFaceIndex);

            switch ( childIndexInParent ) {
                case 0 :                     break;
                case 1 : { u+=ofs;         } break;
                case 2 : { u+=ofs; v+=ofs; } break;
                case 3 : {         v+=ofs; } break;
            }
            ofs = (unsigned short)(ofs << 1);
        } else {
            // If the root face is not a quad, we need to offset the ptex index
            // CCW to match the correct child face
            ConstIndexArray children =
                refinement.getFaceChildFaces(parentFaceIndex);

            for (int j=0; j<children.size(); ++j) {
                if (children[j] == childFaceIndex) {
                    childIndexInParent = j;
                    break;
                }
            }
        }
        childFaceIndex = parentFaceIndex;
    }
    if (rotatedTriangle) {
        // If the triangle is tagged as rotated at this point then the
        // computed u and v parameters will both be negative and we map
        // them onto positive values in the opposite diagonal of the
        // parameter space.
        u += ofs;
        v += ofs;
    }
    int baseFaceIndex = childFaceIndex;

    //  Need to store ptex index from base face and child of an irregular face:
    Index ptexIndex = ptexIndices.GetFaceId(baseFaceIndex);
    assert(ptexIndex != -1);
    if (irregBase) {
        ptexIndex += childIndexInParent;
    }

    //  Compute/identify the transition mask if requested, otherwise leave it 0:
    int transitionMask = 0;
    if (computeTransitionMask && (levelIndex < _refiner.GetMaxLevel())) {
        transitionMask = _refiner.getRefinement(levelIndex).
                            getParentFaceSparseTag(faceIndex)._transitional;
    }

    PatchParam param;
    param.Set(ptexIndex, (short)u, (short)v, (unsigned short) depth, irregBase,
              (unsigned short) boundaryMask, (unsigned short) transitionMask,
              (unsigned short) isRegular);
    return param;
}


//
//  SourcePatch
//
//  This class allows the full topological specification of the neighborhood
//  of vertices and edges around a face that collectively define a rectangular
//  piece of surface corresponding to that face.  All components are declared
//  in terms of local indices and explicitly avoid any references/indices to
//  an external representation.  It is assembled by specifying the topology
//  of each corner (number of faces/edges, boundary, etc.) and finalization
//  determines a set of source vertices in a canonical orientation relative
//  to the face and any patch which may be derived from it.
//
//  Any/all corners can be arbitrarily irregular.  Information for each corner
//  is similar to what is provided in a Vtr::Level::VSpan but does not require
//  any Vtr dependent orientation (e.g. the leading edge) and also requires
//  identification of the incident face that corresponds to the patch (i.e.
//  the "patch face").
//
//  The set of local source vertices begins with the corner vertices of the
//  face corresponding to the patch.  Since the 1-rings of the corner vertices
//  overlap, a subset of the 1-rings is identified as the "local ring points"
//  for a corner, which are the points most associated with the corner.  While
//  one of these local ring points may overlap with an adjacent corner, the
//  local ring points for each corner are indexed successively for each corner.
//
//  The cumulative set of source points forming the 1-ring around the patch
//  face is indexed successively in a counter-clockwise orientation beginning
//  with the first edge-vertex of the first corner, e.g. for a patch with
//  three regular corners and an irregular boundary:
//
//     13          12       11         10
//        x--------x--------x--------x
//        |        |        |        |
//        |        |        |        |
//        |        |        |        |
//        |        |        |        |
//     14 x--------x--------x--------x 9
//        |        |3      2|        |
//        |        |        |        |
//        |        |        |        |
//        |        |0      1|        |
//      4 x--------x--------x--------x 8
//        |        |        |
//        |        |        |
//        |        |        |
//        |        |        |
//        x--------x--------x
//      5          6         7
//
//  The set of source points consists of the corner points {0,1,2,3} followed
//  by the four sets of points {4,5,6}, {7,8}, {9,10,11} and {12,13,14} --
//  each being the exterior subset of the points of the one-ring for the
//  corresponding corner point.
//
//  The 1-ring for each corner is made available for both assembling the points
//  of the resulting Gregory patch and for defining correspondence between the
//  original source of the vertices.  All 1-rings are oriented counter-clockwise
//  and begin with a vertex at the end of an edge.  The 1-rings for boundaries
//  begin/end with vertices at the ends of the leading/trailing edges.  Interior
//  1-rings are ordered such that the patch-face vertices occur as specified.
//

//
//  SourcePatch method to initialize other internal members once required
//  members of all corners have been explicitly initialized.  This deals with
//  all of the awkward ways in which rings of vertices around each corner
//  overlap in order to define the canonical ordering of vertices (and avoiding
//  have the same vertex twice).
//
//  Note:  Considering passing Corner[4] to a constructor so that this is all
//  dealt with in the constructor.
//
void
SourcePatch::Finalize(int size) {

    //
    //  Determine the sizes of the rings and the total number of points
    //  involved.  In the process, identify which corners share ring points
    //  with their neighbors and accumulate maximal ring sizes and valence:
    //
    bool isQuad = (size == 4);

    _numCorners = size;

    _maxValence = 0;
    _maxRingSize = 0;
    _numSourcePoints = _numCorners;

    for (int cIndex = 0; cIndex < _numCorners; ++cIndex) {
        //
        //  Need valence-2 information for neighbors as it affects sizing:
        //
        int cPrev = fastModN(cIndex + 2 + isQuad, _numCorners);
        int cNext = fastModN(cIndex + 1,          _numCorners);

        bool prevIsVal2Interior = ((_corners[cPrev]._numFaces == 2) &&
                                   !_corners[cPrev]._boundary);
        bool thisIsVal2Interior = ((_corners[cIndex]._numFaces == 2) &&
                                   !_corners[cIndex]._boundary);
        bool nextIsVal2Interior = ((_corners[cNext]._numFaces == 2) &&
                                   !_corners[cNext]._boundary);

        Corner & corner = _corners[cIndex];

        corner._val2Interior = thisIsVal2Interior;
        corner._val2Adjacent = prevIsVal2Interior || nextIsVal2Interior;

        //
        //  General cases are >= 3-face interior and >= 2-face boundary:
        //
        if ((corner._numFaces + corner._boundary) > 2) {
            //
            //  Quads generally share with both prev and next, but triangles
            //  never share with prev because of the necessary asymmetry of
            //  the local ring points.
            //
            if (corner._boundary) {
                corner._sharesWithPrev = isQuad && (corner._patchFace != (corner._numFaces - 1));
                corner._sharesWithNext = (corner._patchFace != 0);
            } else if (corner._dart) {
                Corner & cP = _corners[cPrev];
                Corner & cN = _corners[cNext];

                bool cPrevOnDartEdge = cP._boundary && (cP._patchFace == 0);
                bool cNextOnDartEdge = cN._boundary && (cN._patchFace == cN._numFaces - 1);

                corner._sharesWithPrev = isQuad && !cPrevOnDartEdge;
                corner._sharesWithNext = !cNextOnDartEdge;
            } else {
                corner._sharesWithPrev = isQuad;
                corner._sharesWithNext = true;
            }

            _ringSizes[cIndex]      = corner._numFaces * (1 + isQuad) + corner._boundary;
            _localRingSizes[cIndex] = _ringSizes[cIndex] - (_numCorners - 1)
                                    - corner._sharesWithPrev - corner._sharesWithNext;

            if (corner._val2Adjacent) {
                _localRingSizes[cIndex] -= prevIsVal2Interior;
                _localRingSizes[cIndex] -= (nextIsVal2Interior && isQuad);
            }
        } else {
            corner._sharesWithPrev = false;
            corner._sharesWithNext = false;

            //  Single-face boundary/corner and valence-2 interior:
            if (corner._numFaces == 1) {
                _ringSizes[cIndex]      = _numCorners - 1;
                _localRingSizes[cIndex] = 0;
            } else {
                _ringSizes[cIndex]      = 2 * (1 + isQuad);
                _localRingSizes[cIndex] = isQuad;
            }
        }
        _localRingOffsets[cIndex] = _numSourcePoints;

        _maxValence  = std::max(_maxValence, corner._numFaces + corner._boundary);
        _maxRingSize = std::max(_maxRingSize, _ringSizes[cIndex]);

        _numSourcePoints += _localRingSizes[cIndex];
    }
}


int
SourcePatch::GetCornerRingPoints(int corner, int ringPoints[]) const {

    bool isQuad = (_numCorners == 4);

    int cNext = fastModN(corner + 1,          _numCorners);
    int cOpp  = fastModN(corner + 1 + isQuad, _numCorners);
    int cPrev = fastModN(corner + 2 + isQuad, _numCorners);

    //
    //  Assemble the ring in a canonical ordering beginning with the points of
    //  the 2 or 3 other corners of the face followed by the local ring -- with
    //  any shared or compensating points (for valence-2 interior) preceding
    //  and following the points local to the ring.
    //
    int ringSize = 0;

    //  The adjacent corner points:
    ringPoints[ringSize++] = cNext;
    if (isQuad) {
        ringPoints[ringSize++] = cOpp;
    }
    ringPoints[ringSize++] = cPrev;

    //  Shared points preceding the local ring points:
    if (_corners[cPrev]._val2Interior) {
        ringPoints[ringSize++] = isQuad ? cOpp : cNext;
    }
    if (_corners[corner]._sharesWithPrev) {
        ringPoints[ringSize++] = _localRingOffsets[cPrev] + _localRingSizes[cPrev] - 1;
    }

    //  The local ring points:
    for (int i = 0; i < _localRingSizes[corner]; ++i) {
        ringPoints[ringSize++] = _localRingOffsets[corner] + i;
    }

    //  Shared points following the local ring points:
    if (isQuad) {
        if (_corners[corner]._sharesWithNext) {
            ringPoints[ringSize++] = _localRingOffsets[cNext];
        }
        if (_corners[cNext]._val2Interior) {
            ringPoints[ringSize++] = cOpp;
        }
    } else {
        if (_corners[corner]._sharesWithNext) {
            if (_corners[cNext]._val2Interior) {
                ringPoints[ringSize++] = cPrev;
            } else if (_localRingSizes[cNext] == 0) {
                ringPoints[ringSize++] = _localRingOffsets[cPrev];
            } else {
                ringPoints[ringSize++] = _localRingOffsets[cNext];
            }
        }
    }
    assert(ringSize == _ringSizes[corner]);

    //  The assembled ordering matches the desired ordering if the patch-face
    //  is first, so rotate the assembled ring if that's not the case:
    //
    if (_corners[corner]._patchFace) {
        int rotationOffset = ringSize - (1 + isQuad) * _corners[corner]._patchFace;
        std::rotate(ringPoints, ringPoints + rotationOffset, ringPoints + ringSize);
    }
    return ringSize;
}

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
