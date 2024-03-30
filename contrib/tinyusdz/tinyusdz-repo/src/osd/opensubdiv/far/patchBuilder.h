//
//   Copyright 2017 DreamWorks Animation LLC.
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

#ifndef OPENSUBDIV3_FAR_PATCH_BUILDER_H
#define OPENSUBDIV3_FAR_PATCH_BUILDER_H

#include "../version.h"

#include "../sdc/types.h"
#include "../far/types.h"
#include "../far/topologyRefiner.h"
#include "../far/patchDescriptor.h"
#include "../far/patchParam.h"
#include "../far/ptexIndices.h"
#include "../far/sparseMatrix.h"


namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

//
//  SourcePatch
//
//  This is a local utility class that captures the full local topology of an
//  arbitrarily irregular patch, i.e. a patch which may have one or all corners
//  irregular.  Given the topology at each corner the entire collection of
//  points involved is identified and oriented consistently.
//
//  Note (barfowl):
//      This was originally a class internal to PatchBuilder, but there is some
//  redundancy between it and the Level::VSpan used more publicly to identify
//  irregular corner topology.   Replacing VSpan with SourcePatch is now under
//  consideration, and doing so will impact its public/private interface (which
//  was left public to give PatchBuilder access).
//      A simpler constructor to initialize an instance given a set of Corners
//  would also be preferable if made more public (i.e. public for use within
//  the library, not exported to clients) -- eliminating the need for the
//  explicit initialization and required call to the Finalize() method that
//  the PatchBuilder currently performs internally.
//
class SourcePatch {
public:
    struct Corner {
        Corner() { std::memset((void*) this, 0, sizeof(Corner)); }

        LocalIndex _numFaces;   // valence of corner vertex
        LocalIndex _patchFace;  // location of patch within incident faces

        unsigned short _boundary : 1;
        unsigned short _sharp    : 1;
        unsigned short _dart     : 1;

        //  For internal bookkeeping -- consider hiding or moving elsewhere
        unsigned short _sharesWithPrev : 1;
        unsigned short _sharesWithNext : 1;
        unsigned short _val2Interior   : 1;
        unsigned short _val2Adjacent   : 1;
    };

public:
    SourcePatch() { std::memset((void*) this, 0, sizeof(SourcePatch)); }
    ~SourcePatch() { }

    //  To be called after all Corners have been initialized (hope to
    //  replace this with alternative constructor at some point)
    void Finalize(int size3or4);

    int GetNumSourcePoints() const { return _numSourcePoints; }
    int GetMaxValence() const { return _maxValence; }
    int GetMaxRingSize() const { return _maxRingSize; }

    int GetCornerRingSize(int corner) const { return _ringSizes[corner]; }
    int GetCornerRingPoints(int corner, int points[]) const;

//  public/private access needs to be reviewed when/if used more publicly
//private:
public:
    //  The SourcePatch is fully defined by its Corner members
    Corner _corners[4];
    int    _numCorners;

    //  Additional members (derived from Corners) to help assemble corner rings:
    int _numSourcePoints;
    int _maxValence;
    int _maxRingSize;

    int _ringSizes[4];
    int _localRingSizes[4];
    int _localRingOffsets[4];
};


//
//  PatchBuilder
//
//  This is the main class to assist the identification of limit surface
//  patches from faces in a TopologyRefiner for assembly into other, larger
//  datatypes.
//
//  The PatchBuilder takes a const reference to a refiner and supports
//  arbitrarily refined hierarchies, i.e. it is not restricted to uniform or
//  adaptive refinement strategies and does not include any logic relating
//  to the origin of the hierarchy.  It can associate a patch with any face
//  in the hierarchy (subject to a few minimum requirements) -- leaving the
//  decision as to which faces/patches are appropriate to its client.
//
//  PatchBuilder is an abstract base class with a subclass derived to support
//  each subdivision scheme -- as such, construction relies on a factory
//  method to create an instance of the appropriate subclass.  Only two pure
//  virtual methods are required (other than the required destructor):
//
//      - determine the patch type for a subdivision scheme given a more
//        general basis specification (e.g. Bezier, Gregory, Linear, etc)
//
//      - convert the vertices in the subdivision hierarchy into points of a
//        specified patch type, using computations specific to that scheme
//
//  The base class handles the more general topological analysis that
//  determines the nature of a patch associated with each face -- providing
//  both queries to the client, along with more involved methods to extract
//  or convert data associated with the patches.  There is no concrete "Patch"
//  class to which all clients would be required to conform.  The queries and
//  data returned are provided for clients to assemble into patches or other
//  aggregates as they see fit.
//
//  This is intended as an internal/private class for use within the library
//  for now -- possibly to be exported for use by clients when/if its
//  interface is appropriate and stable.
//
class PatchBuilder {
public:
    //
    //  A PatchBuilder is constructed given a patch "basis" rather than a
    //  "type" to use with the subdivision scheme involved.  The relevant
    //  explicit patch types will be determined from the basis and scheme:
    //
    enum BasisType {
        BASIS_UNSPECIFIED,
        BASIS_REGULAR,
        BASIS_GREGORY,
        BASIS_LINEAR,
        BASIS_BEZIER   // to be supported in future
    };

    //
    //  Required Options specify a patch basis to use for both regular and
    //  irregular patches -- sparing the client the need to repeatedly
    //  specify these for each face considered.  Other options are included
    //  to support legacy approximations:
    //
    struct Options {
        Options() : regBasisType(BASIS_UNSPECIFIED),
                    irregBasisType(BASIS_UNSPECIFIED),
                    fillMissingBoundaryPoints(false),
                    approxInfSharpWithSmooth(false),
                    approxSmoothCornerWithSharp(false) { }

        BasisType regBasisType;
        BasisType irregBasisType;
        bool      fillMissingBoundaryPoints;
        bool      approxInfSharpWithSmooth;
        bool      approxSmoothCornerWithSharp;
    };

public:
    //
    //  Public construction (via factory method) and destruction:
    //
    static PatchBuilder* Create(TopologyRefiner const& refiner,
                                Options const& options);
    virtual ~PatchBuilder();

    //
    //  High-level queries related to the subdivision scheme of the refiner, the
    //  patch types associated with it and those chosen to represent its faces:
    //
    int GetRegularFaceSize() const { return _schemeRegFaceSize; }

    BasisType GetRegularBasisType() const { return _options.regBasisType; }
    BasisType GetIrregularBasisType() const { return _options.irregBasisType; }

    PatchDescriptor::Type GetRegularPatchType() const { return _regPatchType; }
    PatchDescriptor::Type GetIrregularPatchType() const { return _irregPatchType; }

    PatchDescriptor::Type GetNativePatchType() const { return _nativePatchType; }
    PatchDescriptor::Type GetLinearPatchType() const { return _linearPatchType; }

    //
    //  Face-level queries to determine presence of patches:
    //
    bool IsFaceAPatch(int level, Index face) const;
    bool IsFaceALeaf(int level, Index face) const;

    //
    //  Patch-level topological queries:
    //
    bool IsPatchRegular(int level, Index face, int fvc = -1) const;

    int GetRegularPatchBoundaryMask(int level, Index face, int fvc = -1) const;

    void GetIrregularPatchCornerSpans(int level, Index face,
            Vtr::internal::Level::VSpan cornerSpans[4], int fvc = -1) const;

    bool DoesFaceVaryingPatchMatch(int level, Index face, int fvc) const {
        return _refiner.getLevel(level).doesFaceFVarTopologyMatch(face, fvc);
    }

    //
    //  Patch-level control point retrieval and methods for converting source
    //  points to a set of local points in a different basis
    //
    int GetRegularPatchPoints(int level, Index face,
            int regBoundaryMask, // compute internally when < 0
            Index patchPoints[],
            int fvc = -1) const;

    template <typename REAL>
    int GetIrregularPatchConversionMatrix(int level, Index face,
            Vtr::internal::Level::VSpan const cornerSpans[],
            SparseMatrix<REAL> &              matrix) const;

    int GetIrregularPatchSourcePoints(int level, Index face,
            Vtr::internal::Level::VSpan const cornerSpans[],
            Index                             sourcePoints[],
            int                               fvc = -1) const;
    //
    //  Queries related to "single-crease" patches -- currently a subset of
    //  regular interior patches:
    //
    struct SingleCreaseInfo {
        int   creaseEdgeInFace;
        float creaseSharpness;
    };
    bool IsRegularSingleCreasePatch(int level, Index face,
            SingleCreaseInfo & info) const;

    //
    //  Computing the PatchParam -- note the regrettable dependency on
    //  PtexIndices but PatchParam is essentially tied to it indefinitely.
    //  Better to pass it in than have the PatchBuilder build its own
    //  PtexIndices.
    //
    //  Consider creating a PatchParamFactory which can manage the PtexIndices
    //  along with this method.  It will then be able to generate additional
    //  data to accelerate these computations.
    //
    PatchParam ComputePatchParam(int level, Index face,
            PtexIndices const& ptexIndices, bool isRegular = true,
            int boundaryMask = 0, bool computeTransitionMask = false) const;

protected:
    PatchBuilder(TopologyRefiner const& refiner, Options const& options);

    //  Internal methods supporting topology queries:
    int getRegularFacePoints(int level, Index face,
            Index patchPoints[], int fvc) const;

    int getQuadRegularPatchPoints(int level, Index face,
            int regBoundaryMask, Index patchPoints[], int fvc) const;

    int getTriRegularPatchPoints(int level, Index face,
            int regBoundaryMask, Index patchPoints[], int fvc) const;

    //  Internal methods using the SourcePatch:
    int assembleIrregularSourcePatch(int level, Index face,
            Vtr::internal::Level::VSpan const cornerSpans[],
            SourcePatch & sourcePatch) const;

    int gatherIrregularSourcePoints(int level, Index face,
            Vtr::internal::Level::VSpan const cornerSpans[],
            SourcePatch &  sourcePatch,
            Index patchPoints[], int fvc) const;

protected:
    //
    //  Virtual methods to be provided by subclass for each scheme:
    //
    virtual PatchDescriptor::Type patchTypeFromBasis(BasisType basis) const = 0;

    //  Note overloading of the conversion for SparseMatrix<REAL>:
    virtual int convertToPatchType(SourcePatch const &   sourcePatch,
                                   PatchDescriptor::Type patchType,
                                   SparseMatrix<float> & matrix) const = 0;
    virtual int convertToPatchType(SourcePatch const &    sourcePatch,
                                   PatchDescriptor::Type  patchType,
                                   SparseMatrix<double> & matrix) const = 0;

protected:
    TopologyRefiner const& _refiner;
    Options const          _options;

    Sdc::SchemeType _schemeType;
    int             _schemeRegFaceSize;
    bool            _schemeIsLinear;

    PatchDescriptor::Type _regPatchType;
    PatchDescriptor::Type _irregPatchType;
    PatchDescriptor::Type _nativePatchType;
    PatchDescriptor::Type _linearPatchType;
};

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_FAR_PATCH_BUILDER_H */
