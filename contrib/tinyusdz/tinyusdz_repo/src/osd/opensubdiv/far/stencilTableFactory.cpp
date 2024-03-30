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

#ifdef _MSC_VER
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#endif

#include "../far/stencilTableFactory.h"
#include "../far/stencilBuilder.h"
#include "../far/patchTable.h"
#include "../far/patchTableFactory.h"
#include "../far/patchMap.h"
#include "../far/topologyRefiner.h"
#include "../far/primvarRefiner.h"

#include <cassert>
#include <algorithm>
#include <iostream>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

using internal::StencilBuilder;

namespace {
#ifdef __INTEL_COMPILER
#pragma warning (push)
#pragma warning disable 1572
#endif

    template <typename REAL>
    inline bool isWeightZero(REAL w) { return (w == (REAL) 0.0); }

#ifdef __INTEL_COMPILER
#pragma warning (pop)
#endif
}

//------------------------------------------------------------------------------

template <typename REAL>
void
StencilTableFactoryReal<REAL>::generateControlVertStencils(
    int numControlVerts,
    StencilReal<REAL> & dst) {

    // Control vertices contribute a single index with a weight of 1.0
    for (int i=0; i<numControlVerts; ++i) {
        *dst._size = 1;
        *dst._indices = i;
        *dst._weights = (REAL) 1.0;
        dst.Next();
    }
}

//
// StencilTable factory
//
template <typename REAL>
StencilTableReal<REAL> const *
StencilTableFactoryReal<REAL>::Create(TopologyRefiner const & refiner,
    Options options) {

    bool interpolateVertex = options.interpolationMode==INTERPOLATE_VERTEX;
    bool interpolateVarying = options.interpolationMode==INTERPOLATE_VARYING;
    bool interpolateFaceVarying = options.interpolationMode==INTERPOLATE_FACE_VARYING;

    int numControlVertices = !interpolateFaceVarying
        ? refiner.GetLevel(0).GetNumVertices()
        : refiner.GetLevel(0).GetNumFVarValues(options.fvarChannel);

    int maxlevel = std::min(int(options.maxLevel), refiner.GetMaxLevel());
    if (maxlevel==0 && (! options.generateControlVerts)) {
        StencilTableReal<REAL> * result = new StencilTableReal<REAL>;
        result->_numControlVertices = numControlVertices;
        return result;
    }

    StencilBuilder<REAL> builder(numControlVertices,
                                /*genControlVerts*/ true,
                                /*compactWeights*/  true);

    //
    // Interpolate stencils for each refinement level
    //
    PrimvarRefinerReal<REAL> primvarRefiner(refiner);

    typename StencilBuilder<REAL>::Index srcIndex(&builder, 0);
    typename StencilBuilder<REAL>::Index dstIndex(&builder, numControlVertices);

    for (int level=1; level<=maxlevel; ++level) {
        if (interpolateVertex) {
            primvarRefiner.Interpolate(level, srcIndex, dstIndex);
        } else if (interpolateVarying) {
            primvarRefiner.InterpolateVarying(level, srcIndex, dstIndex);
        } else {
            primvarRefiner.InterpolateFaceVarying(level, srcIndex, dstIndex, options.fvarChannel);
        }

        if (options.factorizeIntermediateLevels) {
            srcIndex = dstIndex;
        }

        int dstVertex = !interpolateFaceVarying
            ? refiner.GetLevel(level).GetNumVertices()
            : refiner.GetLevel(level).GetNumFVarValues(options.fvarChannel);
        dstIndex = dstIndex[dstVertex];

        if (! options.factorizeIntermediateLevels) {
            // All previous verts are considered as coarse verts, as a
            // result, we don't update the srcIndex and update the coarse
            // vertex count.
            builder.SetCoarseVertCount(dstIndex.GetOffset());
        }
    }

    size_t firstOffset = numControlVertices;
    if (! options.generateIntermediateLevels)
        firstOffset = srcIndex.GetOffset();
 
    // Copy stencils from the StencilBuilder into the StencilTable.
    // Always initialize numControlVertices (useful for torus case)
    StencilTableReal<REAL> * result = 
                        new StencilTableReal<REAL>(numControlVertices,
                                          builder.GetStencilOffsets(),
                                          builder.GetStencilSizes(),
                                          builder.GetStencilSources(),
                                          builder.GetStencilWeights(),
                                          options.generateControlVerts,
                                          firstOffset);
    return result;
}

//------------------------------------------------------------------------------

template <typename REAL>
StencilTableReal<REAL> const *
StencilTableFactoryReal<REAL>::Create(int numTables,
    StencilTableReal<REAL> const ** tables) {

    // XXXtakahito:
    // This function returns NULL for empty inputs or erroneous condition.
    // It's convenient for skipping varying stencils etc, however,
    // other Create() API returns an empty stencil instead of NULL.
    // They need to be consistent.

    if ( (numTables<=0) || (! tables)) {
        return NULL;
    }

    int ncvs = -1,
        nstencils = 0,
        nelems = 0;

    for (int i=0; i<numTables; ++i) {

        StencilTableReal<REAL> const * st = tables[i];
        // allow the tables could have a null entry.
        if (!st) continue;

        if (ncvs >= 0 && st->GetNumControlVertices() != ncvs) {
            return NULL;
        }
        ncvs = st->GetNumControlVertices();
        nstencils += st->GetNumStencils();
        nelems += (int)st->GetControlIndices().size();
    }

    if (ncvs == -1) {
        return NULL;
    }

    StencilTableReal<REAL> * result = new StencilTableReal<REAL>;
    result->resize(nstencils, nelems);

    int * sizes = &result->_sizes[0];
    Index * indices = &result->_indices[0];
    REAL * weights = &result->_weights[0];
    for (int i=0; i<numTables; ++i) {
        StencilTableReal<REAL> const * st = tables[i];
        if (!st) continue;

        int st_nstencils = st->GetNumStencils(),
            st_nelems = (int)st->_indices.size();
        memcpy(sizes, &st->_sizes[0], st_nstencils*sizeof(int));
        memcpy(indices, &st->_indices[0], st_nelems*sizeof(Index));
        memcpy(weights, &st->_weights[0], st_nelems*sizeof(REAL));

        sizes += st_nstencils;
        indices += st_nelems;
        weights += st_nelems;
    }

    result->_numControlVertices = ncvs;

    // have to re-generate offsets from scratch
    result->generateOffsets();

    return result;
}

//------------------------------------------------------------------------------

template <typename REAL>
StencilTableReal<REAL> const *
StencilTableFactoryReal<REAL>::AppendLocalPointStencilTable(
    TopologyRefiner const &refiner,
    StencilTableReal<REAL> const * baseStencilTable,
    StencilTableReal<REAL> const * localPointStencilTable,
    bool factorize) {

    return appendLocalPointStencilTable(
        refiner,
        baseStencilTable,
        localPointStencilTable,
        /*channel*/-1,
        factorize);
}

template <typename REAL>
StencilTableReal<REAL> const *
StencilTableFactoryReal<REAL>::AppendLocalPointStencilTableFaceVarying(
    TopologyRefiner const &refiner,
    StencilTableReal<REAL> const * baseStencilTable,
    StencilTableReal<REAL> const * localPointStencilTable,
    int channel,
    bool factorize) {

    return appendLocalPointStencilTable(
        refiner,
        baseStencilTable,
        localPointStencilTable,
        channel,
        factorize);
}

template <typename REAL>
StencilTableReal<REAL> const *
StencilTableFactoryReal<REAL>::appendLocalPointStencilTable(
    TopologyRefiner const &refiner,
    StencilTableReal<REAL> const * baseStencilTable,
    StencilTableReal<REAL> const * localPointStencilTable,
    int channel,
    bool factorize) {

    // require the local point stencils exist and be non-empty
    if ((localPointStencilTable == NULL) ||
        (localPointStencilTable->GetNumStencils() == 0)) {
        return NULL;
    }

    int nControlVerts = channel < 0
        ? refiner.GetLevel(0).GetNumVertices()
        : refiner.GetLevel(0).GetNumFVarValues(channel);

    //  if no base stencils or empty, return copy of local point stencils
    if ((baseStencilTable == NULL) ||
        (baseStencilTable->GetNumStencils() == 0)) {
        StencilTableReal<REAL> * result =
                new StencilTableReal<REAL>(*localPointStencilTable);
        result->_numControlVertices = nControlVerts;
        return result;
    }

    // baseStencilTable can be built with or without singular stencils
    // (single weight of 1.0f) as place-holders for coarse mesh vertices.

    int controlVertsIndexOffset = 0;
    int nBaseStencils = baseStencilTable->GetNumStencils();
    int nBaseStencilsElements = (int)baseStencilTable->_indices.size();
    {
        int nverts = channel < 0
            ? refiner.GetNumVerticesTotal()
            : refiner.GetNumFVarValuesTotal(channel);
        if (nBaseStencils == nverts) {

            // the table contains stencils for the control vertices
            //
            //  <-----------------  nverts ------------------>
            //
            //  +---------------+----------------------------+-----------------+
            //  | control verts | refined verts   : (max lv) |   local points  |
            //  +---------------+----------------------------+-----------------+
            //  |          base stencil table                |    LP stencils  |
            //  +--------------------------------------------+-----------------+
            //                         ^                           /
            //                          \_________________________/
            //
            //
            controlVertsIndexOffset = 0;

        } else if (nBaseStencils == (nverts - nControlVerts)) {

            // the table does not contain stencils for the control vertices
            //
            //  <-----------------  nverts ------------------>
            //                  <------ nBaseStencils ------->
            //  +---------------+----------------------------+-----------------+
            //  | control verts | refined verts   : (max lv) |   local points  |
            //  +---------------+----------------------------+-----------------+
            //                  |     base stencil table     |    LP stencils  |
            //                  +----------------------------+-----------------+
            //                                 ^                   /
            //                                  \_________________/
            //  <-------------->
            //                 controlVertsIndexOffset
            //
            controlVertsIndexOffset = nControlVerts;

        } else {
            // these are not the stencils you are looking for.
            assert(0);
            return NULL;
        }
    }

    // copy all local point stencils to proto stencils, and factorize if needed.
    int nLocalPointStencils = localPointStencilTable->GetNumStencils();
    int nLocalPointStencilsElements = 0;

    StencilBuilder<REAL> builder(nControlVerts,
                                /*genControlVerts*/ false,
                                /*compactWeights*/  factorize);
    typename StencilBuilder<REAL>::Index origin(&builder, 0);
    typename StencilBuilder<REAL>::Index dst = origin;
    typename StencilBuilder<REAL>::Index srcIdx = origin;

    for (int i = 0 ; i < nLocalPointStencils; ++i) {
        StencilReal<REAL> src = localPointStencilTable->GetStencil(i);
        dst = origin[i];
        for (int j = 0; j < src.GetSize(); ++j) {
            Index index = src.GetVertexIndices()[j];
            REAL weight = src.GetWeights()[j];
            if (isWeightZero<REAL>(weight)) continue;

            if (factorize) {
                dst.AddWithWeight(
                    // subtracting controlVertsIndex if the baseStencil doesn't
                    // include control vertices (see above diagram)
                    // since currently local point stencils are created with
                    // absolute indices including control (level=0) vertices.
                    baseStencilTable->GetStencil(index - controlVertsIndexOffset),
                    weight);
            } else {
                srcIdx = origin[index + controlVertsIndexOffset];
                dst.AddWithWeight(srcIdx, weight);
            }
        }
        nLocalPointStencilsElements += builder.GetNumVertsInStencil(i);
    }

    // create new stencil table
    StencilTableReal<REAL> * result = new StencilTableReal<REAL>;
    result->_numControlVertices = nControlVerts;
    result->resize(nBaseStencils + nLocalPointStencils,
                   nBaseStencilsElements + nLocalPointStencilsElements);

    int* sizes = &result->_sizes[0];
    Index * indices = &result->_indices[0];
    REAL * weights = &result->_weights[0];

    // put base stencils first
    memcpy(sizes, &baseStencilTable->_sizes[0],
           nBaseStencils*sizeof(int));
    memcpy(indices, &baseStencilTable->_indices[0],
           nBaseStencilsElements*sizeof(Index));
    memcpy(weights, &baseStencilTable->_weights[0],
           nBaseStencilsElements*sizeof(REAL));

    sizes += nBaseStencils;
    indices += nBaseStencilsElements;
    weights += nBaseStencilsElements;

    // endcap stencils second
    for (int i = 0 ; i < nLocalPointStencils; ++i) {
        int size = builder.GetNumVertsInStencil(i);
        int idx = builder.GetStencilOffsets()[i];
        for (int j = 0; j < size; ++j) {
            *indices++ = builder.GetStencilSources()[idx+j];
            *weights++ = builder.GetStencilWeights()[idx+j];
        }
        *sizes++ = size;
    }

    // have to re-generate offsets from scratch
    result->generateOffsets();

    return result;
}

//------------------------------------------------------------------------------
template <typename REAL>
LimitStencilTableReal<REAL> const *
LimitStencilTableFactoryReal<REAL>::Create(TopologyRefiner const & refiner,
    LocationArrayVec const & locationArrays,
    StencilTableReal<REAL> const * cvStencilsIn,
    PatchTable const * patchTableIn,
    Options options) {

    // Compute the total number of stencils to generate
    int numStencils=0, numLimitStencils=0;
    for (int i=0; i<(int)locationArrays.size(); ++i) {
        assert(locationArrays[i].numLocations>=0);
        numStencils += locationArrays[i].numLocations;
    }
    if (numStencils<=0) {
        return 0;
    }

    bool uniform  = refiner.IsUniform();
    int  maxlevel = refiner.GetMaxLevel();

    bool interpolateVertex      = (options.interpolationMode == INTERPOLATE_VERTEX);
    bool interpolateVarying     = (options.interpolationMode == INTERPOLATE_VARYING);
    bool interpolateFaceVarying = (options.interpolationMode == INTERPOLATE_FACE_VARYING);
    int  fvarChannel            = options.fvarChannel;

    //
    //  Quick sanity checks for given PatchTable and/or StencilTables:
    //
    int nRefinedStencils = 0;
    if (uniform) {
        //  Uniform stencils must include at least the last level points:
        nRefinedStencils = interpolateFaceVarying
                         ? refiner.GetLevel(maxlevel).GetNumFVarValues(fvarChannel)
                         : refiner.GetLevel(maxlevel).GetNumVertices();
    } else {
        //  Adaptive stencils must include at least all refined points:
        nRefinedStencils = interpolateFaceVarying
                         ? refiner.GetNumFVarValuesTotal(fvarChannel)
                         : refiner.GetNumVerticesTotal();
    }
    if (cvStencilsIn && (cvStencilsIn->GetNumStencils() < nRefinedStencils)) {
        //  Too few stencils in given StencilTable
        return 0;
    }
    if (patchTableIn && (patchTableIn->IsFeatureAdaptive() == uniform)) {
        //  Adaptive/uniform mismatch with given PatchTable and refiner
        return 0;
    }

    // If an appropriate StencilTable was given, use it, otherwise, create a new one
    StencilTableReal<REAL> const * cvstencils = cvStencilsIn;
    if (! cvstencils) {
        //
        // Generate stencils for the control vertices - this is necessary to
        // properly factorize patches with control vertices at level 0 (natural
        // regular patches, such as in a torus)
        // note: the control vertices of the mesh are added as single-index
        //       stencils of weight 1.0f
        //
        typename StencilTableFactoryReal<REAL>::Options stencilTableOptions;
        stencilTableOptions.generateIntermediateLevels = uniform ? false :true;
        stencilTableOptions.generateControlVerts = true;
        stencilTableOptions.generateOffsets = true;
        stencilTableOptions.interpolationMode = options.interpolationMode;
        stencilTableOptions.fvarChannel = options.fvarChannel;

        cvstencils = StencilTableFactoryReal<REAL>::Create(refiner, stencilTableOptions);
    }

    // If an appropriate PatchTable was given, use it, otherwise, create a new one
    PatchTable const * patchtable = patchTableIn;
    if (! patchtable) {
        //
        // Ideally we could create a sparse PatchTable here for the given
        // Locations, but that requires inverting the ptex/base-face relation.
        // so the caller must deal with that and provide such a PatchTable
        //
        PatchTableFactory::Options patchTableOptions;
        patchTableOptions.SetPatchPrecision<REAL>();
        patchTableOptions.includeBaseLevelIndices = true;
        patchTableOptions.generateVaryingTables = interpolateVarying;
        patchTableOptions.generateFVarTables = interpolateFaceVarying;
        if (interpolateFaceVarying) {
            patchTableOptions.includeFVarBaseLevelIndices = true;
            patchTableOptions.numFVarChannels = 1;
            patchTableOptions.fvarChannelIndices = &fvarChannel;
            patchTableOptions.generateFVarLegacyLinearPatches = uniform ||
                !refiner.GetAdaptiveOptions().considerFVarChannels;
        }
        patchTableOptions.SetEndCapType(
            Far::PatchTableFactory::Options::ENDCAP_GREGORY_BASIS);
        patchTableOptions.useInfSharpPatch = !uniform &&
            refiner.GetAdaptiveOptions().useInfSharpPatch;

        patchtable = PatchTableFactory::Create(refiner, patchTableOptions);
    }

    // Append local point stencils and further verfiy size of given StencilTable:
    StencilTableReal<REAL> const * localstencils = 0;
    if (interpolateVertex) {
        localstencils = patchtable->GetLocalPointStencilTable<REAL>();
    } else if (interpolateFaceVarying) {
        localstencils = patchtable->GetLocalPointFaceVaryingStencilTable<REAL>(fvarChannel);
    } else {
        localstencils = patchtable->GetLocalPointVaryingStencilTable<REAL>();
    }

    if (localstencils && (cvstencils->GetNumStencils() == nRefinedStencils)) {
        StencilTableReal<REAL> const *refinedstencils = cvstencils;
        if (interpolateFaceVarying) {
            cvstencils = StencilTableFactoryReal<REAL>::AppendLocalPointStencilTableFaceVarying(
                    refiner, refinedstencils, localstencils, fvarChannel);
        } else {
            cvstencils = StencilTableFactoryReal<REAL>::AppendLocalPointStencilTable(
                    refiner, refinedstencils, localstencils);
        }
        if (!cvStencilsIn) delete refinedstencils;
    }

    assert(patchtable && cvstencils);

    // Create a patch-map to locate sub-patches faster
    PatchMap patchmap( *patchtable );

    //
    // Generate limit stencils for locations
    //
    int nControlVertices = interpolateFaceVarying
                         ? refiner.GetLevel(0).GetNumFVarValues(fvarChannel)
                         : refiner.GetLevel(0).GetNumVertices();

    StencilBuilder<REAL> builder(nControlVertices,
                                /*genControlVerts*/ false,
                                /*compactWeights*/  true);
    typename StencilBuilder<REAL>::Index origin(&builder, 0);
    typename StencilBuilder<REAL>::Index dst = origin;

    //
    //  Generally use the patches corresponding to the interpolation mode, but Uniform
    //  PatchTables do not have varying patches -- use the equivalent linear vertex
    //  patches in this case:
    //
    bool useVertexPatches = interpolateVertex || (interpolateVarying && uniform);
    bool useFVarPatches   = interpolateFaceVarying;

    REAL  wP[20], wDs[20], wDt[20], wDss[20], wDst[20], wDtt[20];

    for (size_t i=0; i<locationArrays.size(); ++i) {
        LocationArray const & array = locationArrays[i];
        assert(array.ptexIdx>=0);

        for (int j=0; j<array.numLocations; ++j) { // for each face we're working on
            REAL  s = array.s[j],
                  t = array.t[j]; // for each target (s,t) point on that face

            PatchMap::Handle const * handle = 
                                        patchmap.FindPatch(array.ptexIdx, s, t);
            if (handle) {
                ConstIndexArray cvs;
                if (useVertexPatches) {
                    cvs = patchtable->GetPatchVertices(*handle);
                } else if (useFVarPatches) {
                    cvs = patchtable->GetPatchFVarValues(*handle, fvarChannel);
                } else {
                    cvs = patchtable->GetPatchVaryingVertices(*handle);
                }

                StencilTableReal<REAL> const & src = *cvstencils;
                dst = origin[numLimitStencils];

                if (options.generate2ndDerivatives) {
                    if (useVertexPatches) {
                        patchtable->EvaluateBasis<REAL>(
                                *handle, s, t, wP, wDs, wDt, wDss, wDst, wDtt);
                    } else if (useFVarPatches) {
                        patchtable->EvaluateBasisFaceVarying<REAL>(
                                *handle, s, t, wP, wDs, wDt, wDss, wDst, wDtt, fvarChannel);
                    } else {
                        patchtable->EvaluateBasisVarying<REAL>(
                                *handle, s, t, wP, wDs, wDt, wDss, wDst, wDtt);
                    }

                    dst.Clear();
                    for (int k = 0; k < cvs.size(); ++k) {
                        dst.AddWithWeight(src[cvs[k]], wP[k], wDs[k], wDt[k], wDss[k], wDst[k], wDtt[k]);
                    }
                } else if (options.generate1stDerivatives) {
                    if (useVertexPatches) {
                        patchtable->EvaluateBasis<REAL>(
                                *handle, s, t, wP, wDs, wDt);
                    } else if (useFVarPatches) {
                        patchtable->EvaluateBasisFaceVarying<REAL>(
                                *handle, s, t, wP, wDs, wDt, 0, 0, 0, fvarChannel);
                    } else {
                        patchtable->EvaluateBasisVarying<REAL>(
                                *handle, s, t, wP, wDs, wDt);
                    }

                    dst.Clear();
                    for (int k = 0; k < cvs.size(); ++k) {
                        dst.AddWithWeight(src[cvs[k]], wP[k], wDs[k], wDt[k]);
                    }
                } else {
                    if (useVertexPatches) {
                        patchtable->EvaluateBasis<REAL>(
                                *handle, s, t, wP);
                    } else if (useFVarPatches) {
                        patchtable->EvaluateBasisFaceVarying<REAL>(
                                *handle, s, t, wP, 0, 0, 0, 0, 0, fvarChannel);
                    } else {
                        patchtable->EvaluateBasisVarying<REAL>(
                                *handle, s, t, wP);
                    }

                    dst.Clear();
                    for (int k = 0; k < cvs.size(); ++k) {
                        dst.AddWithWeight(src[cvs[k]], wP[k]);
                    }
                }

                ++numLimitStencils;
            }
        }
    }

    if (! cvStencilsIn) {
        delete cvstencils;
    }

    if (! patchTableIn) {
        delete patchtable;
    }

    //
    // Copy the proto-stencils into the limit stencil table
    //
    LimitStencilTableReal<REAL> * result = new LimitStencilTableReal<REAL>(
                                          nControlVertices,
                                          builder.GetStencilOffsets(),
                                          builder.GetStencilSizes(),
                                          builder.GetStencilSources(),
                                          builder.GetStencilWeights(),
                                          builder.GetStencilDuWeights(),
                                          builder.GetStencilDvWeights(),
                                          builder.GetStencilDuuWeights(),
                                          builder.GetStencilDuvWeights(),
                                          builder.GetStencilDvvWeights(),
                                          /*ctrlVerts*/false,
                                          /*fristOffset*/0);
    return result;
}

//
//  Explicit instantiation for float and double:
//
template class StencilTableFactoryReal<float>;
template class StencilTableFactoryReal<double>;

template class LimitStencilTableFactoryReal<float>;
template class LimitStencilTableFactoryReal<double>;


} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
