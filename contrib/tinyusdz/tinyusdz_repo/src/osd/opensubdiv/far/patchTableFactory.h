//
//   Copyright 2013 Pixar
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

#ifndef OPENSUBDIV3_FAR_PATCH_TABLE_FACTORY_H
#define OPENSUBDIV3_FAR_PATCH_TABLE_FACTORY_H

#include "../version.h"

#include "../far/topologyRefiner.h"
#include "../far/patchTable.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

/// \brief Factory for constructing a PatchTable from a TopologyRefiner
///
class PatchTableFactory {
public:

    /// \brief Public options for the PatchTable factory
    ///
    struct Options {

        /// \brief Choice for approximating irregular patches (end-caps)
        ///
        /// This enum specifies how irregular patches (end-caps) are approximated.
        /// A basis is chosen, rather than a specific patch type, and has a
        /// corresponding patch type for each subdivision scheme, i.e. a quad and
        /// triangular patch type exists for each basis.  These choices provide a
        /// trade-off between surface quality and performance.
        ///
        enum EndCapType {
            ENDCAP_NONE = 0,        ///< unspecified
            ENDCAP_BILINEAR_BASIS,  ///< use linear patches (simple quads or tris)
            ENDCAP_BSPLINE_BASIS,   ///< use BSpline-like patches (same patch type as regular)
            ENDCAP_GREGORY_BASIS,   ///< use Gregory patches (highest quality, recommended default)
            ENDCAP_LEGACY_GREGORY   ///< legacy option for 2.x style Gregory patches (Catmark only)
        };

        Options(unsigned int maxIsolation=10) :
             generateAllLevels(false),
             includeBaseLevelIndices(true),
             includeFVarBaseLevelIndices(false),
             triangulateQuads(false),
             useSingleCreasePatch(false),
             useInfSharpPatch(false),
             maxIsolationLevel(maxIsolation & 0xf),
             endCapType(ENDCAP_GREGORY_BASIS),
             shareEndCapPatchPoints(true),
             generateVaryingTables(true),
             generateVaryingLocalPoints(true),
             generateFVarTables(false),
             patchPrecisionDouble(false),
             fvarPatchPrecisionDouble(false),
             generateFVarLegacyLinearPatches(true),
             generateLegacySharpCornerPatches(true),
             numFVarChannels(-1),
             fvarChannelIndices(0)
        { }

        /// \brief Get endcap basis type
        EndCapType GetEndCapType() const { return (EndCapType)endCapType; }

        /// \brief Set endcap basis type
        void SetEndCapType(EndCapType e) { endCapType = e & 0x7; }

        /// \brief Set maximum isolation level
        void SetMaxIsolationLevel(unsigned int level) { maxIsolationLevel = level & 0xf; }

        /// \brief Set precision of vertex patches
        template <typename REAL> void SetPatchPrecision();

        /// \brief Set precision of face-varying patches
        template <typename REAL> void SetFVarPatchPrecision();

        /// \brief Determine adaptive refinement options to match assigned patch options
        TopologyRefiner::AdaptiveOptions GetRefineAdaptiveOptions() const {
            TopologyRefiner::AdaptiveOptions adaptiveOptions(maxIsolationLevel);

            adaptiveOptions.useInfSharpPatch     = useInfSharpPatch;
            adaptiveOptions.useSingleCreasePatch = useSingleCreasePatch;
            adaptiveOptions.considerFVarChannels = generateFVarTables &&
                                                  !generateFVarLegacyLinearPatches;
            return adaptiveOptions;
        }

        unsigned int generateAllLevels           : 1, ///< Generate levels from 'firstLevel' to 'maxLevel' (Uniform mode only)
                     includeBaseLevelIndices     : 1, ///< Include base level in patch point indices (Uniform mode only)
                     includeFVarBaseLevelIndices : 1, ///< Include base level in face-varying patch point indices (Uniform mode only)
                     triangulateQuads            : 1, ///< Triangulate 'QUADS' primitives (Uniform mode only)

                     useSingleCreasePatch : 1, ///< Use single crease patch
                     useInfSharpPatch     : 1, ///< Use infinitely-sharp patch
                     maxIsolationLevel    : 4, ///< Cap adaptive feature isolation to the given level (max. 10)

                     // end-capping
                     endCapType              : 3, ///< EndCapType
                     shareEndCapPatchPoints  : 1, ///< Share endcap patch points among adjacent endcap patches.
                                                  ///< currently only work with GregoryBasis.

                     // varying
                     generateVaryingTables      : 1, ///< Generate varying patch tables
                     generateVaryingLocalPoints : 1, ///< Generate local points with varying patches

                     // face-varying
                     generateFVarTables  : 1, ///< Generate face-varying patch tables

                     // precision
                     patchPrecisionDouble     : 1, ///< Generate double-precision stencils for vertex patches
                     fvarPatchPrecisionDouble : 1, ///< Generate double-precision stencils for face-varying patches

                     // legacy behaviors (default to true)
                     generateFVarLegacyLinearPatches  : 1, ///< Generate all linear face-varying patches (legacy)
                     generateLegacySharpCornerPatches : 1; ///< Generate sharp regular patches at smooth corners (legacy)

        int          numFVarChannels;          ///< Number of channel indices and interpolation modes passed
        int const *  fvarChannelIndices;       ///< List containing the indices of the channels selected for the factory
    };

    /// \brief Instantiates a PatchTable from a client-provided TopologyRefiner.
    ///
    ///  A PatchTable can be constructed from a TopologyRefiner that has been
    ///  either adaptively or uniformly refined.  In both cases, the resulting
    ///  patches reference vertices in the various refined levels by index,
    ///  and those indices accumulate with the levels in different ways.
    ///
    ///  For adaptively refined patches, patches are defined at different levels,
    ///  including the base level, so the indices of patch vertices include
    ///  vertices from all levels.  A sparse set of patches can be created by
    ///  restricting the patches generated to those descending from a given set
    ///  of faces at the base level.  This sparse set of base faces is expected
    ///  to be a subset of the faces that were adaptively refined in the given
    ///  TopologyRefiner, otherwise results are undefined.
    ///
    ///  For uniformly refined patches, all patches are completely defined within
    ///  the last level.  There is often no use for intermediate levels and they
    ///  can usually be ignored.  Indices of patch vertices might therefore be
    ///  expected to be defined solely within the last level.  While this is true
    ///  for face-varying patches, for historical reasons it is not the case for
    ///  vertex and varying patches.  Indices for vertex and varying patches include
    ///  the base level in addition to the last level while indices for face-varying
    ///  patches include only the last level.
    ///
    /// @param refiner        TopologyRefiner from which to generate patches
    ///
    /// @param options        Options controlling the creation of the table
    ///
    /// @param selectedFaces  Only create patches for the given set of base faces.
    ///
    /// @return               A new instance of PatchTable
    ///
    static PatchTable * Create(TopologyRefiner const & refiner,
                               Options options = Options(),
                               ConstIndexArray selectedFaces = ConstIndexArray());

public:
    //  PatchFaceTag
    //
    //  This simple struct was previously used within the factory to take inventory of
    //  various kinds of patches to fully allocate buffers prior to populating them.  It
    //  was not intended to be exposed as part of the public interface.
    //
    //  It is no longer used internally and is being kept here to respect preservation
    //  of the public interface, but it will be deprecated at the earliest opportunity.
    //
    /// \brief Obsolete internal struct not intended for public use -- due to
    /// be deprecated.
    //
    struct PatchFaceTag {
    public:
        unsigned int   _hasPatch        : 1;
        unsigned int   _isRegular       : 1;
        unsigned int   _transitionMask  : 4;
        unsigned int   _boundaryMask    : 4;
        unsigned int   _boundaryIndex   : 2;
        unsigned int   _boundaryCount   : 3;
        unsigned int   _hasBoundaryEdge : 3;
        unsigned int   _isSingleCrease  : 1;

        void clear();
        void assignBoundaryPropertiesFromEdgeMask(int boundaryEdgeMask);
        void assignBoundaryPropertiesFromVertexMask(int boundaryVertexMask);
        void assignTransitionPropertiesFromEdgeMask(int boundaryVertexMask);
    };
    typedef std::vector<PatchFaceTag> PatchTagVector;
};


template <> inline void PatchTableFactory::Options::SetPatchPrecision<float>() {
    patchPrecisionDouble = false;
}
template <> inline void PatchTableFactory::Options::SetFVarPatchPrecision<float>() {
    fvarPatchPrecisionDouble = false;
}

template <> inline void PatchTableFactory::Options::SetPatchPrecision<double>() {
    patchPrecisionDouble = true;
}
template <> inline void PatchTableFactory::Options::SetFVarPatchPrecision<double>() {
    fvarPatchPrecisionDouble = true;
}


} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv


#endif /* OPENSUBDIV3_FAR_PATCH_TABLE_FACTORY_H */
