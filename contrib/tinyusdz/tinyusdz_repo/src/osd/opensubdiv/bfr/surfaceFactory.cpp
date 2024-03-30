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

#include "../bfr/hash.h"
#include "../bfr/limits.h"
#include "../bfr/surface.h"
#include "../bfr/surfaceFactory.h"
#include "../bfr/surfaceFactoryCache.h"
#include "../bfr/faceTopology.h"
#include "../bfr/faceSurface.h"
#include "../bfr/regularPatchBuilder.h"
#include "../bfr/irregularPatchBuilder.h"
#include "../bfr/patchTree.h"

#include <map>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {
//
//  DEBUG - static variables to keep track of constructed Surfaces
//        - note that these global variables have extremely limited use:
//            - they are initialized once per process
//            - reported and reset on destruction of a SurfaceFactory
//
//#define _BFR_DEBUG_TOP_TYPE_STATS
#ifdef _BFR_DEBUG_TOP_TYPE_STATS
static int __numLinearPatches     = 0;
static int __numExpRegularPatches = 0;
static int __numRegularPatches    = 0;
static int __numIrregularPatches  = 0;
static int __numIrregularUncached = 0;
static int __numIrregularInCache  = 0;
#endif

//
//  Definition of the private/nested SurfaceSet class:
//
//  This class (essentially a struct) encapsulates a clients specification
//  of a set of multiple surfaces and their intended interpolation types
//  (vertex, varying, and face-varying).  The multiple public creation
//  methods to request common subsets of surfaces all populate an instance
//  of SurfaceSet for internal use.
//
class SurfaceFactory::SurfaceSet {
public:
    SurfaceSet() : numSurfs(0), numFVarSurfs(0),
                   vtxSurf(0), varSurf(0),
                   fvarSurfs(0), fvarSurfPtrs(0), fvarIDs(0) { }

public:
    //  Assignment to member variable is intended to be explicit:
    int numSurfs;
    int numFVarSurfs;

    typedef internal::SurfaceData SurfaceType;

    SurfaceType  * vtxSurf;
    SurfaceType  * varSurf;
    SurfaceType  * fvarSurfs;
    SurfaceType ** fvarSurfPtrs;

    FVarID const * fvarIDs;

    void InitializeSurfaces() const {
        if (vtxSurf) vtxSurf->reinitialize();
        if (varSurf) varSurf->reinitialize();
        for (int i = 0; i < numFVarSurfs; ++i) {
            GetFVarSurface(i)->reinitialize();
        }
    }

public:
    //  Access to member variables is preferred through these methods,
    //  which may require a little more logic than expected:
    int GetNumSurfaces() const { return numSurfs; }

    bool          HasVertexSurface() const { return (vtxSurf != 0); }
    SurfaceType * GetVertexSurface() const { return vtxSurf; }

    bool          HasVaryingSurface() const { return (varSurf != 0); }
    SurfaceType * GetVaryingSurface() const { return varSurf; }

    //  More than one FVar surface may be present, and each may have
    //  a unique ID:
    bool HasFVarSurfaces()    const { return numFVarSurfs > 0; }
    int  GetNumFVarSurfaces() const { return numFVarSurfs; }

    FVarID GetFVarSurfaceID(int i) const {
        return fvarIDs ? fvarIDs[i] : FVarID(i);
    }

    SurfaceType * GetFVarSurface(int i)   const {
        //  Note that FVar Surfaces may be specified either as an
        //  array of Surfaces or an array of Surface pointers:
        return fvarSurfs ? (fvarSurfs + i) : fvarSurfPtrs[i];
    }
};


//
//  Main constructor and supporting initialization methods:
//
SurfaceFactory::SurfaceFactory(Sdc::SchemeType      subdivScheme,
                               Sdc::Options const & subdivOptions,
                               Options      const & factoryOptions) :
        _topologyCache(0) {

    //  Order of operations not important here:
    setSubdivisionOptions(subdivScheme, subdivOptions);
    setFactoryOptions(factoryOptions);
}

void
SurfaceFactory::setSubdivisionOptions(Sdc::SchemeType      subdivScheme,
                                      Sdc::Options const & subdivOptions) {

    //  Assign the main member variables before others derived from them:
    _subdivScheme  = subdivScheme;
    _subdivOptions = subdivOptions;

    //  Initialize members dependent on subdivision topology:
    _regFaceSize = Sdc::SchemeTypeTraits::GetRegularFaceSize(_subdivScheme);

    _linearScheme =
        (Sdc::SchemeTypeTraits::GetLocalNeighborhoodSize(_subdivScheme) == 0);

    _linearFVarInterp = _linearScheme ||
                       (_subdivOptions.GetFVarLinearInterpolation() ==
                                 Sdc::Options::FVAR_LINEAR_ALL);

    //  Initialize members related to the "face has limit" test:
    _rejectSmoothBoundariesForLimit = !_linearScheme &&
                       (_subdivOptions.GetVtxBoundaryInterpolation() ==
                                 Sdc::Options::VTX_BOUNDARY_NONE);

    _rejectIrregularFacesForLimit = !_linearScheme && (_regFaceSize == 3);

    _testNeighborhoodForLimit = _rejectSmoothBoundariesForLimit ||
                                _rejectIrregularFacesForLimit;
}

void
SurfaceFactory::setFactoryOptions(Options const & factoryOptions) {

    //  Assign the main member variable before others derived from them:
    _factoryOptions = factoryOptions;

    if (_factoryOptions.IsCachingEnabled()) {
        if (_factoryOptions.GetExternalCache()) {
            _topologyCache = _factoryOptions.GetExternalCache();
        }
    }
}

void
SurfaceFactory::setInternalCache(SurfaceFactoryCache * cache) {

    //  Remember caching must be on and an external cache takes precedence
    if (_factoryOptions.IsCachingEnabled()) {
        if (_factoryOptions.GetExternalCache() == 0) {
            _topologyCache = cache;
        }
    }
}

SurfaceFactory::~SurfaceFactory() {

#ifdef _BFR_DEBUG_TOP_TYPE_STATS
//  DEBUG - report and reset inventory:
printf("SurfaceFactory destructor:\n");
printf("    __numLinearPatches     = %6d\n", __numLinearPatches);
printf("    __numExpRegularPatches = %6d\n", __numExpRegularPatches);
printf("    __numRegularPatches    = %6d\n", __numRegularPatches);
printf("    __numIrregularPatches  = %6d\n", __numIrregularPatches);
if (!_factoryOptions.DisableTopologyCache()) {
printf("\n");
printf("    __numIrregularUncached = %6d\n", __numIrregularUncached);
printf("    __numIrregularInCache  = %6d\n", __numIrregularInCache);
}
__numLinearPatches     = 0;
__numExpRegularPatches = 0;
__numRegularPatches    = 0;
__numIrregularPatches  = 0;
__numIrregularUncached = 0;
__numIrregularInCache  = 0;
#endif
}

//
//  Notes on presence/absence of a limit surface...
//
//  Unfortunately it is not trivial to detect when a face does not have
//  an associated limit surface.  There are a few cases when a face will
//  not have a limit surface -- divided into simple and complex cases:
//
//      - simple:
//          - the face is a hole
//          - the face is degenerate (< 3 edges)
//      - complex:
//          - boundary interpolation option "none" is assigned:
//              - in which case some, not all, boundary faces have no limit
//          - Loop subdivision is applied to non-triangles
//
//  The simple cases are, as the name suggests, simple.  But the complex
//  cases require a greater inspection of the topological neighborhood of
//  the face.
//
//  With boundary faces when "boundary none" is set (not very often) it is
//  not enough to test if a face is a boundary -- if a boundary face has all
//  of its incident boundary edges (i.e. all boundary edges incident to all
//  of its face-vertices) then the boundary face has a limit surface.  This
//  requires a complete topological description of each corner of the face.
//
//  Similarly, the case of Loop subdivision in the presence of non-triangles
//  required determining if any corner of the face has an incident face
//  that is not a triangle.
//
//  The method here inspects a corner at a time and tries to reject a face
//  without a limit surface as soon as possible. But most cases are going to
//  require inspection of all corners -- and that same inspection is likely
//  to be applied later when constructing the limit.
//
inline bool
SurfaceFactory::faceHasLimitSimple(Index faceIndex, int faceSize) const {

    return (faceSize >= 3) && (faceSize <= Limits::MaxFaceSize()) &&
           !isFaceHole(faceIndex);
}

bool
SurfaceFactory::faceHasLimitNeighborhood(FaceTopology const & topology) const {

    assert(_testNeighborhoodForLimit);

    MultiVertexTag tag = topology.GetTag();

    if ((_rejectSmoothBoundariesForLimit && tag.HasNonSharpBoundary()) ||
        (_rejectIrregularFacesForLimit   && tag.HasIrregularFaceSizes())) {
        return false;
    }
    return true;
}

bool
SurfaceFactory::faceHasLimitNeighborhood(Index faceIndex) const {

    assert(_testNeighborhoodForLimit);

    //
    //  The FaceTopology was not available, and rather than construct it
    //  in its entirety, determine a corner at a time and return if any
    //  corner warrants it:
    //
    typedef Vtr::internal::StackBuffer<Index,32,true> CornerIndexBuffer;

    CornerIndexBuffer cFaceVertIndices;

    FaceVertex         faceVtx;
    VertexDescriptor & vtxDesc = faceVtx.GetVertexDescriptor();

    int faceSize = getFaceSize(faceIndex);
    for (int i = 0; i < faceSize; ++i) {
        //  Have the subclass load VertexDescriptor and finalize:
        faceVtx.Initialize(faceSize, _regFaceSize);

        int faceInRing = populateFaceVertexDescriptor(faceIndex, i, &vtxDesc);
        if (faceInRing < 0) return false;

        faceVtx.Finalize(faceInRing);

        //  Inspect the tag to reject cases with no limit surface:
        VertexTag faceVtxTag = faceVtx.GetTag();

        if (_rejectSmoothBoundariesForLimit) {
            if (faceVtxTag.IsUnOrdered()) {
                //  Need to load face-vertices, connect faces and inspect...
                cFaceVertIndices.SetSize(faceVtx.GetNumFaceVertices());

                if (getFaceVertexIncidentFaceVertexIndices(
                        faceIndex, i, cFaceVertIndices) < 0) return false;

                faceVtx.ConnectUnOrderedFaces(cFaceVertIndices);
            }
            if (faceVtxTag.HasNonSharpBoundary()) return false;
        }
        if (_rejectIrregularFacesForLimit) {
            if (faceVtxTag.HasIrregularFaceSizes()) return false;
        }
    }
    return true;
}

bool
SurfaceFactory::FaceHasLimitSurface(Index faceIndex) const {

    if (!faceHasLimitSimple(faceIndex, getFaceSize(faceIndex))) {
        return false;
    }
    if (_testNeighborhoodForLimit) {
        if (!isFaceNeighborhoodRegular(faceIndex, 0, 0)) {
            return faceHasLimitNeighborhood(faceIndex);
        }
    }
    return true;
}

Parameterization
SurfaceFactory::GetFaceParameterization(Index faceIndex) const {

    return Parameterization(_subdivScheme, getFaceSize(faceIndex));
}


//
//  Namespace with internal utilities to compute keys for unique surface
//  topologies to help cache their limit surface representations:
//
namespace {
    //  Need to redefine since SurfaceFactoryCache::Key is protected:
    typedef std::uint64_t KeyIntType;

    //
    //  Note that the data used in determining a topology key is not
    //  purely topological.  While most data determines a unique limit
    //  surface, a few parameters determine the approximation to it
    //  (e.g. the various adaptive refinement levels) or dictate other
    //  properties of its representation (e.g. double precision).
    //
    //  It may be worth separating these -- writing a method to deal
    //  with the pure topology first, then combining it with details
    //  of the representation.
    //

#ifdef _OPENSUBDIV_BFR_INCLUDE_PACKED_TOPOLOGY_KEY_
    //
    //  This alternate function for computing the cache key was applied
    //  to common topologies with low-valence and simply packs integer
    //  bitfields with the topology of the face and all of its corners.
    //
    //  It is typically 3x faster than the hashing method, but is suited
    //  for general use. Given the relatively low cost of computing the
    //  cache key (compared to building an irregular patch) use of this
    //  faster function is not generally significant, but could become
    //  so in future when/if other costs are reduced.
    //
    //  When the cost of building new patches is very low due to a high
    //  percentage of cache hits, the cost of computing the cache keys
    //  (relative to constructing the entire Surface) can rise to over
    //  10% in extreme cases.
    //
    KeyIntType
    packTopologyKey(FaceSurface const & surface,
                    IrregularPatchBuilder::Options options) {

        //
        //  Keep the bitfield struct local in scope unless needed elsewhere:
        //
        struct KeyBits {
            //  Be sure to clear to avoid uninitialized bits:
            KeyBits() { std::memset(this, 0, sizeof(*this)); }

            //  Bits for general options:
            KeyIntType subdScheme       :  2;
            KeyIntType sharpLevel       :  4;
            KeyIntType smoothLevel      :  4;
            KeyIntType usesDouble       :  1;
            KeyIntType unused           :  1;  // future use
            //  Bits for the corners:
            KeyIntType v0Valence        :  6;
            KeyIntType v1Valence        :  6;
            KeyIntType v2Valence        :  6;
            KeyIntType v3Valence        :  6;
            KeyIntType v0IsSharp        :  1;
            KeyIntType v1IsSharp        :  1;
            KeyIntType v2IsSharp        :  1;
            KeyIntType v3IsSharp        :  1;
            KeyIntType v0IsBoundary     :  1;
            KeyIntType v1IsBoundary     :  1;
            KeyIntType v2IsBoundary     :  1;
            KeyIntType v3IsBoundary     :  1;
            KeyIntType v0FaceInBoundary :  5;
            KeyIntType v1FaceInBoundary :  5;
            KeyIntType v2FaceInBoundary :  5;
            KeyIntType v3FaceInBoundary :  5;

            //
            //  Static methods to determine if bitfields can be used:
            //
            static bool InteriorValenceFits(int n) { return n < (1 << 6); }
            static bool BoundaryValenceFits(int n) { return n < (1 << 5); }

            //  A place holder if future Sdc::Options inhibit use of bitfields:
            static bool OptionsInhibitUsage(Sdc::Options) { return false; }
        };
        assert(sizeof(KeyBits) == sizeof(KeyIntType));

        //
        //  Quickly test if the topology can be packed into bitfields, or
        //  if hashing must be used.  Bitfields cannot be used when the
        //  following features are present:
        //
        //      - any sharp edges of any kind (semi-sharp or inf-sharp)
        //      - any semi-sharp vertices (inf-sharp is 1-bit per corner)
        //      - any incident irregular faces
        //
        //  These can quickly be determined by inspecting the topology tags.
        //  Two other situations are:
        //
        //      - any vertex with valence too high (more than ~6 bits)
        //      - any Sdc::Option that cannot be encoded (in theory only)
        //
        //  The former must inspect the valence of each face-vertex, while
        //  the latter requires inspecting the Sdc::Options -- and this is
        //  called out more as a future possibility...
        //
        //  In theory, if certain Sdc::Options impact the limit surface,
        //  they might need to be encoded, or might not be able to be fully
        //  encoded in future.  This is not currently the case in practice:
        //  boundary interpolation options are essentially unused as the
        //  boundary conditions are explicitly applied; the creasing method
        //  can be ignored here because creases cannot be packed; and the
        //  Catmark triangle subdivision option can be ignored because the
        //  presence of any irregular faces cannot be packed.
        //

        //  Immediate rejection of bitfields:
        MultiVertexTag combinedTag = surface.GetTag();
        if (combinedTag.HasSharpEdges() ||
            combinedTag.HasSemiSharpVertices() ||
            combinedTag.HasIrregularFaceSizes()) {
            return false;
        }

        //  Conditional rejection of bitfields for high valence:
        FaceVertexSubset const * subsets = surface.GetSubsets();

        for (int i = 0; i < surface.GetFaceSize(); ++i) {
            int valence = subsets[i]._numFacesTotal;
            if (subsets[i].IsBoundary()) {
                if (!KeyBits::BoundaryValenceFits(valence)) return false;
            } else {
                if (!KeyBits::InteriorValenceFits(valence)) return false;
            }
        }

        //  Conditional rejection of bitfields for specific Sdc::Options:
        if (KeyBits::OptionsInhibitUsage(surface.GetSdcOptionsInEffect())) {
            return false;
        }

        //
        //  Pack the topology of each FaceVertexSubset into bitfields:
        //
        KeyBits keyBits;

        keyBits.subdScheme  = surface.GetSdcScheme();
        keyBits.sharpLevel  = options.sharpLevel;
        keyBits.smoothLevel = options.smoothLevel;
        keyBits.usesDouble  = options.doublePrecision;

        keyBits.v0Valence        = subsets[0]._numFacesTotal;
        keyBits.v0IsSharp        = subsets[0].IsSharp();
        keyBits.v0IsBoundary     = subsets[0].IsBoundary();
        keyBits.v0FaceInBoundary = subsets[0]._numFacesBefore;

        keyBits.v1Valence        = subsets[1]._numFacesTotal;
        keyBits.v1IsSharp        = subsets[1].IsSharp();
        keyBits.v1IsBoundary     = subsets[1].IsBoundary();
        keyBits.v1FaceInBoundary = subsets[1]._numFacesBefore;

        keyBits.v2Valence        = subsets[2]._numFacesTotal;
        keyBits.v2IsSharp        = subsets[2].IsSharp();
        keyBits.v2IsBoundary     = subsets[2].IsBoundary();
        keyBits.v2FaceInBoundary = subsets[2]._numFacesBefore;

        if (surface.GetFaceSize() == 4) {
            keyBits.v3Valence        = subsets[3]._numFacesTotal;
            keyBits.v3IsSharp        = subsets[3].IsSharp();
            keyBits.v3IsBoundary     = subsets[3].IsBoundary();
            keyBits.v3FaceInBoundary = subsets[3]._numFacesBefore;
        }

        KeyIntType keyValue = 0;
        std::memcpy(&keyValue, &keyBits, sizeof(KeyIntType));
        return keyValue;
    }
#endif

    //
    //  Function to assign the topology of any FaceSurface to the desired
    //  integer using a hashing function that considers all topological
    //  features (incident face sizes, crease and corner sharpness, etc.):
    //
    KeyIntType
    hashTopologyKey(FaceSurface const & surface,
                    IrregularPatchBuilder::Options options) {

        //
        //  Structs for "headers" for the entire surface and each corner,
        //  to be assigned and copied into a larger buffer to be hashed:
        //
        typedef unsigned char uchar;

        struct SurfaceHeader {
            //  Be sure to clear to avoid uninitialized bits:
            SurfaceHeader() { std::memset(this, 0, sizeof(*this)); }

            short faceSize;
            uchar subdScheme;
            uchar subdCreasing;
            uchar subdTriSmooth;
            uchar sharpLevel;
            uchar smoothLevel;
            uchar usesDouble;
        };
        struct CornerHeader {
            //  Be sure to clear to avoid uninitialized bits:
            CornerHeader() { std::memset(this, 0, sizeof(*this)); }

            short numFaces;
            short faceInBoundary;

            uchar isBoundary    : 1;
            uchar isInfSharp    : 1;
            uchar isSemiSharp   : 1;
            uchar hasFaceSizes  : 1;
            uchar hasSharpEdges : 1;
        };

        //
        //  Consider using a "delimiter" between corners in the buffer of
        //  data to be hashed, i.e. a value with a distinct bit pattern
        //  such as -1, to help prevent aliasing (may not be needed):
        //
        int  delimiter = -1;
        bool useCornerDelimiter = false;

        //
        //  Local buffers for accumulating and hashing:
        //
        Vtr::internal::StackBuffer<float,16,true> floatBuffer;
        Vtr::internal::StackBuffer<short,16,true> shortBuffer;

        Vtr::internal::StackBuffer<char,256,true> hashBuffer;

        //
        //  Determine size of the main buffer to hash ahead of time:
        //
        //  Note there is some redundancy in the use of the uncommon
        //  faces sizes and sharp edges around each corner due to the
        //  way the corners' incident faces overlap. For typical cases
        //  the extra data used is not large. Only in extreme cases is
        //  it likely to be an issue -- but then the added processing
        //  and construction costs associated with such cases (e.g.
        //  high valence vertices, heavy use of creasing) will make
        //  make the overhead here insignificant.
        //
        size_t hashBufferSize = sizeof(SurfaceHeader);

        int faceSize = surface.GetFaceSize();
        for (int i = 0; i < faceSize; ++i) {
            FaceVertexSubset const & cSub = surface.GetCornerSubset(i);

            int N = cSub.GetNumFaces();

            hashBufferSize += sizeof(CornerHeader);

            hashBufferSize += cSub._tag.IsSemiSharp() ?
                              sizeof(float) : 0;
            hashBufferSize += cSub._tag.HasUnCommonFaceSizes() ?
                             (sizeof(short) * N) : 0;
            hashBufferSize += cSub._tag.HasSharpEdges() ?
                             (sizeof(float) * (N - cSub.IsBoundary())) : 0;

            hashBufferSize += useCornerDelimiter ? sizeof(delimiter) : 0;
        }
        hashBuffer.SetSize((int)hashBufferSize);

        //
        //  Start populating the buffer with the surface header:
        //
        Sdc::Options subdOptions = surface.GetSdcOptionsInEffect();

        SurfaceHeader sHeader;
        sHeader.faceSize      = (short) faceSize;
        sHeader.subdScheme    = (uchar) surface.GetSdcScheme();
        sHeader.subdCreasing  = (uchar) subdOptions.GetCreasingMethod();
        sHeader.subdTriSmooth = (uchar) subdOptions.GetTriangleSubdivision();
        sHeader.sharpLevel    = (uchar) options.sharpLevel;
        sHeader.smoothLevel   = (uchar) options.smoothLevel;
        sHeader.usesDouble    = (uchar) options.doublePrecision;

        std::memcpy(hashBuffer, &sHeader, sizeof(sHeader));

        //
        //  Populate the buffer for each corner of the surface:
        //
        char * bufferPtr = hashBuffer + sizeof(sHeader);

        for (int corner = 0; corner < faceSize; ++corner) {
            FaceVertex       const & cTop = surface.GetCornerTopology(corner);
            FaceVertexSubset const & cSub = surface.GetCornerSubset(corner);

            //  Assign the corner header:
            CornerHeader cHeader;
            cHeader.numFaces       = (short) cSub.GetNumFaces();
            cHeader.faceInBoundary = cSub._numFacesBefore;
            cHeader.isBoundary     = cSub.IsBoundary();
            cHeader.isInfSharp     = cSub.IsSharp();
            cHeader.isSemiSharp    = cSub._tag.IsSemiSharp();
            cHeader.hasFaceSizes   = cSub._tag.HasUnCommonFaceSizes();
            cHeader.hasSharpEdges  = cSub._tag.HasSharpEdges();

            std::memcpy(bufferPtr, &cHeader, sizeof(cHeader));
            bufferPtr += sizeof(cHeader);

            if (cHeader.isSemiSharp) {
                float sharpness = (cSub._localSharpness > 0.0f)
                                ?  cSub._localSharpness
                                :  cTop.GetVertexSharpness();
                std::memcpy(bufferPtr, &sharpness, sizeof(sharpness));
                bufferPtr += sizeof(sharpness);
            }
            if (cHeader.hasFaceSizes) {
                int n = cSub.GetNumFaces();
                shortBuffer.SetSize(n);
                for (int i = 0, f = cTop.GetFaceFirst(cSub); i < n;
                                f = cTop.GetFaceNext(f), ++i) {
                    shortBuffer[i] = (short) cTop.GetFaceSize(f);
                }
                std::memcpy(bufferPtr, shortBuffer, n * sizeof(short));
                bufferPtr += n * sizeof(short);
            }
            if (cHeader.hasSharpEdges) {
                int n = cSub.GetNumFaces() - cSub.IsBoundary();
                floatBuffer.SetSize(n);
                for (int i = 0, f = cTop.GetFaceFirst(cSub); i < n;
                                f = cTop.GetFaceNext(f), ++i) {
                    floatBuffer[i] = cTop.GetFaceEdgeSharpness(f, 1);
                }
                std::memcpy(bufferPtr, floatBuffer, n * sizeof(float));
                bufferPtr += n * sizeof(float);
            }

            if (useCornerDelimiter) {
                std::memcpy(bufferPtr, &delimiter, sizeof(delimiter));
                bufferPtr += sizeof(delimiter);
            }
        }
        assert((bufferPtr - hashBuffer) == (int)hashBufferSize);

        return internal::Hash64(hashBuffer, hashBufferSize);
    }
}

//
//  Methods supporting construction of linear, regular and irregular patches:
//
void
SurfaceFactory::assignLinearSurface(SurfaceType * surfacePtr,
        Index faceIndex, FVarID const * fvarPtrOrVtx) const {

    SurfaceType & surface = *surfacePtr;

    //  Initialize instance members from the associated irregular patch:
    int faceSize  = getFaceSize(faceIndex);

    surface.setParam(Parameterization(_subdivScheme, faceSize));

    surface.setRegular(faceSize == _regFaceSize);
    surface.setLinear(true);

    surface.setRegPatchMask(0);
    if (_regFaceSize == 4) {
        surface.setRegPatchType(Far::PatchDescriptor::QUADS);
    } else {
        surface.setRegPatchType(Far::PatchDescriptor::TRIANGLES);
    }

    //
    //  Finally, gather patch control points from the appropriate indices:
    //
    Index * surfaceCVs = surface.resizeCVs(faceSize);

    int count = 0;
    if (fvarPtrOrVtx == 0) {
        count = getFaceVertexIndices(faceIndex, surfaceCVs);
    } else {
        count = getFaceFVarValueIndices(faceIndex, *fvarPtrOrVtx, surfaceCVs);
    }
    //  If subclass fails to get indices, Surface will remain invalid
    if (count < faceSize) return;

    surface.setValid(true);
#ifdef _BFR_DEBUG_TOP_TYPE_STATS
__numLinearPatches ++;
#endif
}

void
SurfaceFactory::assignRegularSurface(SurfaceType * surfacePtr,
        Index const patchPoints[]) const {

    SurfaceType & surface = *surfacePtr;

    //
    //  Assign the parameterization and discriminants first:
    //
    surface.setParam(Parameterization(_subdivScheme, _regFaceSize));

    surface.setRegular(true);
    surface.setLinear(false);

    //
    //  Assemble the regular patch:
    //
    surface.setRegPatchType(RegularPatchBuilder::GetPatchType(_regFaceSize));
    surface.setRegPatchMask(RegularPatchBuilder::GetBoundaryMask(_regFaceSize,
                                                                  patchPoints));

    //
    //  Copy the patch control points from the given indices:
    //
    int patchSize = RegularPatchBuilder::GetPatchSize(_regFaceSize);

    Index const * pSrc = patchPoints;
    Index       * pDst = surface.resizeCVs(patchSize);

    //  Remember to replace negative indices in boundary patches:
    if (surface.getRegPatchMask() == 0) {
        std::memcpy(pDst, pSrc, patchSize * sizeof(Index));
    } else {
        //  Consider delegating this task to the RegularPatchBuilder:
        Index pPhantom = pSrc[5];
        assert(pPhantom >= 0);
        for (int i = 0; i < patchSize; ++i) {
            pDst[i] = (pSrc[i] < 0) ? pPhantom : pSrc[i];
        }
    }

    surface.setValid(true);
#ifdef _BFR_DEBUG_TOP_TYPE_STATS
__numExpRegularPatches ++;
#endif
}

void
SurfaceFactory::assignRegularSurface(SurfaceType * surfacePtr,
        FaceSurface const & descriptor) const {

    SurfaceType & surface = *surfacePtr;

    //
    //  Assign the parameterization and discriminants first:
    //
    surface.setParam(Parameterization(_subdivScheme, _regFaceSize));

    surface.setRegular(true);
    surface.setLinear(false);

    //
    //  Assemble the regular patch:
    //
    RegularPatchBuilder builder(descriptor);

    surface.setRegPatchType(builder.GetPatchType());
    surface.setRegPatchMask(builder.GetPatchParamBoundaryMask());

    //
    //  Gather the patch control points from the given indices:
    //
    builder.GatherControlVertexIndices(
            surface.resizeCVs(builder.GetNumControlVertices()));

    surface.setValid(true);
#ifdef _BFR_DEBUG_TOP_TYPE_STATS
__numRegularPatches ++;
#endif
}

void
SurfaceFactory::assignIrregularSurface(SurfaceType * surfacePtr,
        FaceSurface const & descriptor) const {

    //
    //  A builder for the irregular patch is required regardless of
    //  whether a new instance is constructed:
    //
    IrregularPatchBuilder::Options buildOptions;

    buildOptions.sharpLevel      = _factoryOptions.GetApproxLevelSharp();
    buildOptions.smoothLevel     = _factoryOptions.GetApproxLevelSmooth();
    buildOptions.doublePrecision = surfacePtr->isDouble();

    IrregularPatchBuilder builder(descriptor, buildOptions);

    //
    //  Construct a new irregular patch or identify one from the cache:
    //
    internal::IrregularPatchSharedPtr patch(0);

    if (_topologyCache == 0) {
        patch = builder.Build();
    } else {
        //
        //  Compute the cache key for the topology of this face, search the
        //  cache for an existing patch and build/add one if not found:
        //
        //  Be sure to use the return result of Add() when adding as it may
        //  be the case that another thread added a patch with the same key
        //  while this one was being built. Using the instance assigned to
        //  the cache intentionally releases the one built here.
        //
        SurfaceFactoryCache::KeyType key =
                hashTopologyKey(descriptor, buildOptions);

        patch = _topologyCache->Find(key);
        if (patch == 0) {
            patch = _topologyCache->Add(key, builder.Build());
#ifdef _BFR_DEBUG_TOP_TYPE_STATS
__numIrregularInCache ++;
#endif
        }
    }

    //
    //  Assign the Surface parameterization, discriminants and patch:
    //
    SurfaceType & surface = *surfacePtr;

    surface.setParam(Parameterization(_subdivScheme, descriptor.GetFaceSize()));

    surface.setRegular(false);
    surface.setLinear(false);

    surface.setIrregPatchPtr(patch);

    //  Gather the patch control points from the given indices:
    builder.GatherControlVertexIndices(
            surface.resizeCVs(patch->GetNumControlPoints()));

    surface.setValid(true);
#ifdef _BFR_DEBUG_TOP_TYPE_STATS
__numIrregularPatches  ++;
__numIrregularUncached += surface.ownsIrregPatch();
#endif
}

void
SurfaceFactory::copyNonLinearSurface(
        SurfaceType       * surfaceDstPtr,
        SurfaceType const & surfaceSrc,
        FaceSurface const & descriptor) const {

    SurfaceType & surfaceDst = *surfaceDstPtr;

    //  Should be creating a linear patch directly rather than copying:
    assert(!surfaceSrc.isLinear());

    //
    //  Assign the topological fields of the patch first:
    //
    surfaceDst.setParam(surfaceSrc.getParam());

    surfaceDst.setLinear(surfaceSrc.isLinear());
    surfaceDst.setRegular(surfaceSrc.isRegular());

    surfaceDst.resizeCVs(surfaceSrc.getNumCVs());

    //
    //  Assign regular/irregular fields and gather control points:
    //
    if (surfaceDst.isRegular()) {
        surfaceDst.setRegPatchType(surfaceSrc.getRegPatchType());
        surfaceDst.setRegPatchMask(surfaceSrc.getRegPatchMask());

        RegularPatchBuilder builder(descriptor);
        assert(builder.GetNumControlVertices() == surfaceDst.getNumCVs());

        builder.GatherControlVertexIndices(surfaceDst.getCVIndices());
#ifdef _BFR_DEBUG_TOP_TYPE_STATS
__numRegularPatches ++;
#endif
    } else {
        surfaceDst.setIrregPatchPtr(surfaceSrc.getIrregPatchPtr());

        IrregularPatchBuilder builder(descriptor);
        assert(builder.GetNumControlVertices() == surfaceDst.getNumCVs());

        builder.GatherControlVertexIndices(surfaceDst.getCVIndices());
#ifdef _BFR_DEBUG_TOP_TYPE_STATS
__numIrregularPatches  ++;
#endif
    }

    surfaceDst.setValid(true);
}


//
//  Methods to deal with topology assembly and inspection:
//
//  Note the difference between the "init" and "gather" methods:  "init"
//  fully resolves and initializes the topology by gathering face indices
//  locally and dealing with unordered faces if present, while the "gather"
//  method simply gathers the corner information -- allowing indices to be
//  provided for further use if needed.
//
bool
SurfaceFactory::initFaceNeighborhoodTopology(Index faceIndex,
        FaceTopology * faceTopologyPtr) const {

    FaceTopology & topology = *faceTopologyPtr;

    if (!gatherFaceNeighborhoodTopology(faceIndex, &topology)) {
        return false;
    }
    if (!topology.HasUnOrderedCorners()) {
        return true;
    }

    //  Gather the indices to determine topology between unordered faces:
    typedef Vtr::internal::StackBuffer<Index,72,true> IndexBuffer;

    IndexBuffer indices(topology._numFaceVertsTotal);
    if (gatherFaceNeighborhoodIndices(faceIndex, topology, 0, indices) < 0) {
        return false;
    }
    topology.ResolveUnOrderedCorners(indices);
    return true;
}

bool
SurfaceFactory::gatherFaceNeighborhoodTopology(Index faceIndex,
        FaceTopology * faceTopologyPtr) const {

    FaceTopology & faceTopology = *faceTopologyPtr;

    int N = getFaceSize(faceIndex);

    faceTopology.Initialize(N);

    for (int i = 0; i < N; ++i) {
        FaceVertex       & faceVtx = faceTopology.GetTopology(i);
        VertexDescriptor & vtxDesc = faceVtx.GetVertexDescriptor();

        faceVtx.Initialize(N, _regFaceSize);

        //  Subclass returning negative here indicates unsupported features
        //  or some other kind of failure:
        int faceInRing = populateFaceVertexDescriptor(faceIndex, i, &vtxDesc);
        if (faceInRing < 0) return false;

        faceVtx.Finalize(faceInRing);
    }

    faceTopology.Finalize();

    return true;
}

int
SurfaceFactory::gatherFaceNeighborhoodIndices(Index faceIndex,
        FaceTopology const & faceTopology,
        FVarID       const * fvarPtrOrVtx,
        Index                controlIndices[]) const {

    int faceSize = faceTopology.GetFaceSize();

    Index * indices  = controlIndices;
    int     nIndices = 0;

    for (int i = 0; i < faceSize; ++i) {
        int numFaceVerts = (fvarPtrOrVtx == 0) ?
                getFaceVertexIncidentFaceVertexIndices(faceIndex, i,
                        indices) :
                getFaceVertexIncidentFaceFVarValueIndices(faceIndex, i,
                        *fvarPtrOrVtx, indices);

        if (numFaceVerts != faceTopology.GetNumFaceVertices(i)) {
            return -1;
        }

        indices  += numFaceVerts;
        nIndices += numFaceVerts;
    }
    return nIndices;
}

bool
SurfaceFactory::isFaceNeighborhoodRegular(Index          faceIndex,
                                          FVarID const * fvarPtrOrVtx,
                                          Index          indices[]) const {
    return (fvarPtrOrVtx == 0) ?
        getFaceNeighborhoodVertexIndicesIfRegular(faceIndex, indices) :
        getFaceNeighborhoodFVarValueIndicesIfRegular(faceIndex, *fvarPtrOrVtx,
                                                     indices);
}

//
//  Main internal methods to populate set of limit Surfaces:
//
bool
SurfaceFactory::populateAllSurfaces(Index faceIndex,
        SurfaceSet * surfaceSetPtr) const {

    SurfaceSet & surfaces = *surfaceSetPtr;

    //  Abort if no Surfaces are specified to populate:
    if (surfaces.GetNumSurfaces() == 0) {
        return false;
    }

    //
    //  Be sure to re-initialize all Surfaces up-front, rather than
    //  deferring it to the assignment of each.  A failure of any one
    //  surface may leave others unvisited -- leaving it unchanged
    //  from previous use.
    //
    surfaces.InitializeSurfaces();

    //  Quickly reject faces with no limit (typically holes) -- some cases
    //  require full topological inspection and will be rejected later:
    if (!faceHasLimitSimple(faceIndex, getFaceSize(faceIndex))) {
        return false;
    }

    //  Determine if we have any non-linear cases to deal with -- which
    //  require gathering and inspection of the full neighborhood around
    //  the given face:
    int numFVarSurfaces = surfaces.GetNumFVarSurfaces();

    bool hasNonLinearSurfaces =
                (surfaces.HasVertexSurface() && !_linearScheme) ||
                (numFVarSurfaces && !_linearFVarInterp);

    bool hasLinearSurfaces =
                 surfaces.HasVaryingSurface() ||
                (surfaces.HasVertexSurface() && _linearScheme) ||
                (numFVarSurfaces && _linearFVarInterp);

    if (hasNonLinearSurfaces || _testNeighborhoodForLimit) {
        if (!populateNonLinearSurfaces(faceIndex, &surfaces)) {
            return false;
        }
    }
    if (hasLinearSurfaces) {
        if (!populateLinearSurfaces(faceIndex, &surfaces)) {
            return false;
        }
    }
    return true;
}

bool
SurfaceFactory::populateLinearSurfaces(Index faceIndex,
        SurfaceSet * surfaceSetPtr) const {

    SurfaceSet & surfaces = *surfaceSetPtr;

    if (surfaces.HasVaryingSurface()) {
        assignLinearSurface(surfaces.GetVaryingSurface(), faceIndex, 0);
    }

    if (_linearScheme && surfaces.HasVertexSurface()) {
        assignLinearSurface(surfaces.GetVertexSurface(), faceIndex, 0);
    }

    if (_linearFVarInterp) {
        int numFVarSurfaces = surfaces.GetNumFVarSurfaces();
        for (int i = 0; i < numFVarSurfaces; ++i) {
            FVarID fvarID = surfaces.GetFVarSurfaceID(i);
            assignLinearSurface(surfaces.GetFVarSurface(i), faceIndex, &fvarID);
        }
    }
    return true;
}

bool
SurfaceFactory::populateNonLinearSurfaces(Index faceIndex,
        SurfaceSet * surfaceSetPtr) const {

    SurfaceSet & surfaces = *surfaceSetPtr;

    typedef Vtr::internal::StackBuffer<Index,72,true> IndexBuffer;

    bool vtxIsNonLinear  = surfaces.HasVertexSurface() && !_linearScheme;
    bool fvarIsNonLinear = surfaces.HasFVarSurfaces()  && !_linearFVarInterp;
    bool anyNonLinear    = vtxIsNonLinear || fvarIsNonLinear;

    //
    //  First need to determine the vertex topology of the face and take
    //  appropriate action based on inputs.  It may be the case that the
    //  topology is only used to determine if the non-linear face has a
    //  limit surface and no non-linear surfaces are generated here (and
    //  linear varying or face-varying surfaces are determined elsewhere).
    //
    //  So determine the topology and deal with any required tests for
    //  the presence of limit surface.
    //
    //  If the face is "explicitly regular", i.e. the subclass can provide
    //  an immediate regular patch representation, the more tedious work
    //  to assemble the more general topological representation is avoided.
    //
    //  Note that while the vertex surface may be explicitly regular, if
    //  the face-varying topology does not match, i.e. there is a UV seam
    //  present around the face, the more general topological representation
    //  will be necessary to deal with a potentially irregular face-varying
    //  surface.
    //
    FaceTopology faceTopology(_subdivScheme, _subdivOptions);
    IndexBuffer  vtxIndices(16);
    FaceSurface  vtxSurfDesc;

    bool vtxIsExplicitlyRegular =
                isFaceNeighborhoodRegular(faceIndex, 0, vtxIndices);
    if (vtxIsExplicitlyRegular) {
        if (_testNeighborhoodForLimit && !anyNonLinear) {
            return true;
        }
    } else {
        //
        //  Three steps are required to get full topological description:
        //      - gathering the full description of the neighborhood
        //      - gathering vertex indices for the neighborhood
        //      - using the indices to resolve any unordered topology
        //  Gathering indices for the vertex surface and/or to resolve
        //  unordered topology is conditional.
        //
        if (!gatherFaceNeighborhoodTopology(faceIndex, &faceTopology)) {
            return false;
        }
        if (vtxIsNonLinear || faceTopology.HasUnOrderedCorners()) {
            vtxIndices.SetSize(faceTopology._numFaceVertsTotal);
            if (gatherFaceNeighborhoodIndices(faceIndex, faceTopology, 0,
                        vtxIndices) < 0) {
                return false;
            }
            if (faceTopology.HasUnOrderedCorners()) {
                faceTopology.ResolveUnOrderedCorners(vtxIndices);
            }
        }
        if (_testNeighborhoodForLimit) {
            if (!faceHasLimitNeighborhood(faceTopology)) {
                return false;
            } else if (!anyNonLinear) {
                return true;
            }
        }

        //  Initialize the vertex surface descriptor for use creating both
        //  the vertex Surface and any non-linear FVar Surfaces:
        vtxSurfDesc.Initialize(faceTopology, vtxIndices);
    }

    //
    //  Construct the Surface for vertex topology first, as face-varying
    //  surfaces that match topology may make use of it:
    //
    bool vtxSurfIsValid = false;
    if (vtxIsNonLinear) {
        SurfaceType & vtxSurf = *surfaces.GetVertexSurface();

        if (vtxIsExplicitlyRegular) {
            assignRegularSurface(&vtxSurf, vtxIndices);
        } else if (vtxSurfDesc.IsRegular()) {
            assignRegularSurface(&vtxSurf, vtxSurfDesc);
        } else {
            assignIrregularSurface(&vtxSurf, vtxSurfDesc);
        }
        vtxSurfIsValid = vtxSurf.isValid();
    }

    //
    //  Construct the Surface for the given face-varying topologies --
    //  all of which are potentially distinct.
    //
    //  If the vertex topology is explicitly regular, the face-varying
    //  surface can only make use of it if it shares the same topology
    //  and the subclass provides corresponding control points.
    //
    //  In all other cases the full topological description and the full
    //  description of the vertex surface must be provided.  The set of
    //  face-varying indices must then be gathered and used to create a
    //  face-varying surface descriptor, which uses the indices to find
    //  the relevant face-varying subsets for each corner.
    //
    if (fvarIsNonLinear) {
        //  We can re-use the vertex index buffer for face-varying indices:
        IndexBuffer & fvIndices = vtxIndices;

        int numFVarSurfaces = surfaces.GetNumFVarSurfaces();
        for (int i = 0; i < numFVarSurfaces; ++i) {
            SurfaceType & fvarSurf = *surfaces.GetFVarSurface(i);
            FVarID        fvarID   =  surfaces.GetFVarSurfaceID(i);

            //  First check if trivially regular, quickly assign and continue:
            bool fvarIsExplicitlyRegular = vtxIsExplicitlyRegular &&
                    isFaceNeighborhoodRegular(faceIndex, &fvarID, fvIndices);

            if (fvarIsExplicitlyRegular) {
                assignRegularSurface(&fvarSurf, fvIndices);
                continue;
            }

            //  Make sure topology, indices and vertex surface are initialized
            //  (will not be if vertex surface was explicitly regular):
            if (!vtxSurfDesc.IsInitialized()) {
                if (!initFaceNeighborhoodTopology(faceIndex, &faceTopology)) {
                    return false;
                }
                vtxSurfDesc.Initialize(faceTopology, 0);
            }
            fvIndices.SetSize(faceTopology._numFaceVertsTotal);

            //  Gather FVar indices and initialize FVar surface descriptor:
            if (gatherFaceNeighborhoodIndices(faceIndex, faceTopology,
                    &fvarID, fvIndices) < 0) {
                return false;
            }

            FaceSurface fvarSurfDesc(vtxSurfDesc, fvIndices);

            //  Detect matching or other topology and dispatch accordingly:
            if (fvarSurfDesc.FVarTopologyMatchesVertex() && vtxSurfIsValid) {
                copyNonLinearSurface(&fvarSurf, *surfaces.GetVertexSurface(),
                                     fvarSurfDesc);
            } else if (fvarSurfDesc.IsRegular()) {
                assignRegularSurface(&fvarSurf, fvarSurfDesc);
            } else {
                assignIrregularSurface(&fvarSurf, fvarSurfDesc);
            }
        }
    }
    return true;
}

//
//  Main internal method to initialize instances of Surface:
//
bool
SurfaceFactory::initSurfaces(Index faceIndex,
        internal::SurfaceData * vtxSurface,
        internal::SurfaceData * varSurface,
        internal::SurfaceData * fvarSurfaces,
        int                     fvarCount,
        FVarID const            fvarIDs[]) const {

    //  Note the SurfaceData for the first FVar Surface is assumed below
    //  to be the head of an array of SurfaceData, which will not be true
    //  if additional members are added to Surface<REAL> in future:
    assert(sizeof(internal::SurfaceData) == sizeof(Surface<float>));

    SurfaceSet surfaces;

    surfaces.vtxSurf      = vtxSurface;
    surfaces.varSurf      = varSurface;
    surfaces.fvarSurfs    = fvarSurfaces;
    surfaces.fvarIDs      = fvarIDs;
    surfaces.numFVarSurfs = fvarCount;
    surfaces.numSurfs     = fvarCount + (vtxSurface != 0) + (varSurface != 0);

    return populateAllSurfaces(faceIndex, &surfaces);
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
