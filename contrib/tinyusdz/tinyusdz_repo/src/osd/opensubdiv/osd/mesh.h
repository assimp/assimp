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

#ifndef OPENSUBDIV3_OSD_MESH_H
#define OPENSUBDIV3_OSD_MESH_H

#include "../version.h"

#include <bitset>
#include <cassert>
#include <cstring>
#include <vector>

#include "../far/topologyRefiner.h"
#include "../far/patchTableFactory.h"
#include "../far/stencilTable.h"
#include "../far/stencilTableFactory.h"

#include "../osd/bufferDescriptor.h"

struct ID3D11DeviceContext;

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Osd {

enum MeshBits {
    MeshAdaptive             = 0,
    MeshInterleaveVarying    = 1,
    MeshFVarData             = 2,
    MeshFVarAdaptive         = 3,
    MeshUseSmoothCornerPatch = 4,
    MeshUseSingleCreasePatch = 5,
    MeshUseInfSharpPatch     = 6,
    MeshEndCapBilinearBasis  = 7,  // exclusive
    MeshEndCapBSplineBasis   = 8,  // exclusive
    MeshEndCapGregoryBasis   = 9,  // exclusive
    MeshEndCapLegacyGregory  = 10, // exclusive
    NUM_MESH_BITS            = 11,
};
typedef std::bitset<NUM_MESH_BITS> MeshBitset;

// ---------------------------------------------------------------------------

template <class PATCH_TABLE>
class MeshInterface {
public:
    typedef PATCH_TABLE PatchTable;
    typedef typename PatchTable::VertexBufferBinding VertexBufferBinding;

public:
    MeshInterface() { }

    virtual ~MeshInterface() { }

    virtual int GetNumVertices() const = 0;

    virtual int GetMaxValence() const = 0;

    virtual void UpdateVertexBuffer(float const *vertexData,
                                    int startVertex, int numVerts) = 0;

    virtual void UpdateVaryingBuffer(float const *varyingData,
                                     int startVertex, int numVerts) = 0;

    virtual void Refine() = 0;

    virtual void Synchronize() = 0;

    virtual PatchTable * GetPatchTable() const = 0;

    virtual Far::PatchTable const *GetFarPatchTable() const = 0;

    virtual VertexBufferBinding BindVertexBuffer() = 0;

    virtual VertexBufferBinding BindVaryingBuffer() = 0;

protected:
    static inline void refineMesh(Far::TopologyRefiner & refiner,
                                  int level, bool adaptive,
                                  bool singleCreasePatch) {
        if (adaptive) {
            Far::TopologyRefiner::AdaptiveOptions options(level);
            options.useSingleCreasePatch = singleCreasePatch;
            refiner.RefineAdaptive(options);
        } else {
            //  This dependency on FVar channels should not be necessary
            bool fullTopologyInLastLevel = refiner.GetNumFVarChannels()>0;

            Far::TopologyRefiner::UniformOptions options(level);
            options.fullTopologyInLastLevel = fullTopologyInLastLevel;
            refiner.RefineUniform(options);
        }
    }
    static inline void refineMesh(Far::TopologyRefiner & refiner,
                                  int level, MeshBitset bits) {
        if (bits.test(MeshAdaptive)) {
            Far::TopologyRefiner::AdaptiveOptions options(level);
            options.useSingleCreasePatch = bits.test(MeshUseSingleCreasePatch);
            options.useInfSharpPatch = bits.test(MeshUseInfSharpPatch);
            options.considerFVarChannels = bits.test(MeshFVarAdaptive);
            refiner.RefineAdaptive(options);
        } else {
            //  This dependency on FVar channels should not be necessary
            bool fullTopologyInLastLevel = refiner.GetNumFVarChannels()>0;

            Far::TopologyRefiner::UniformOptions options(level);
            options.fullTopologyInLastLevel = fullTopologyInLastLevel;
            refiner.RefineUniform(options);
        }
    }
};

// ---------------------------------------------------------------------------

template <typename STENCIL_TABLE, typename SRC_STENCIL_TABLE,
          typename DEVICE_CONTEXT>
STENCIL_TABLE const *
convertToCompatibleStencilTable(
    SRC_STENCIL_TABLE const *table, DEVICE_CONTEXT *context) {
    if (! table) return NULL;
    return STENCIL_TABLE::Create(table, context);
}

template <>
inline Far::StencilTable const *
convertToCompatibleStencilTable<Far::StencilTable, Far::StencilTable, void>(
    Far::StencilTable const *table, void *  /*context*/) {
    // no need for conversion
    // XXX: We don't want to even copy.
    if (! table) return NULL;
    return new Far::StencilTable(*table);
}

template <>
inline Far::LimitStencilTable const *
convertToCompatibleStencilTable<Far::LimitStencilTable, Far::LimitStencilTable, void>(
    Far::LimitStencilTable const *table, void *  /*context*/) {
    // no need for conversion
    // XXX: We don't want to even copy.
    if (! table) return NULL;
    return new Far::LimitStencilTable(*table);
}

template <>
inline Far::StencilTable const *
convertToCompatibleStencilTable<Far::StencilTable, Far::StencilTable, ID3D11DeviceContext>(
    Far::StencilTable const *table, ID3D11DeviceContext *  /*context*/) {
    // no need for conversion
    // XXX: We don't want to even copy.
    if (! table) return NULL;
    return new Far::StencilTable(*table);
}

// ---------------------------------------------------------------------------

// Osd evaluator cache: for the GPU backends require compiled instance
//   (GLXFB, GLCompute, CL)
//
// note: this is just an example usage and client applications are supposed
//       to implement their own structure for Evaluator instance.
//
template <typename EVALUATOR>
class EvaluatorCacheT {
public:
    ~EvaluatorCacheT() {
        for(typename Evaluators::iterator it = _evaluators.begin();
            it != _evaluators.end(); ++it) {
            delete it->evaluator;
        }
    }

    // XXX: FIXME, linear search
    struct Entry {
        Entry(BufferDescriptor const &srcDescArg,
              BufferDescriptor const &dstDescArg,
              BufferDescriptor const &duDescArg,
              BufferDescriptor const &dvDescArg,
              EVALUATOR *evalArg) : srcDesc(srcDescArg), dstDesc(dstDescArg),
                              duDesc(duDescArg), dvDesc(dvDescArg),
                              duuDesc(BufferDescriptor()),
                              duvDesc(BufferDescriptor()),
                              dvvDesc(BufferDescriptor()),
                              evaluator(evalArg) {}
        Entry(BufferDescriptor const &srcDescArg,
              BufferDescriptor const &dstDescArg,
              BufferDescriptor const &duDescArg,
              BufferDescriptor const &dvDescArg,
              BufferDescriptor const &duuDescArg,
              BufferDescriptor const &duvDescArg,
              BufferDescriptor const &dvvDescArg,
              EVALUATOR *evalArg) : srcDesc(srcDescArg), dstDesc(dstDescArg),
                              duDesc(duDescArg), dvDesc(dvDescArg),
                              duuDesc(duuDescArg),
                              duvDesc(duvDescArg),
                              dvvDesc(dvvDescArg),
                              evaluator(evalArg) {}
        BufferDescriptor srcDesc, dstDesc;
        BufferDescriptor duDesc, dvDesc;
        BufferDescriptor duuDesc, duvDesc, dvvDesc;
        EVALUATOR *evaluator;
    };
    typedef std::vector<Entry> Evaluators;

    template <typename DEVICE_CONTEXT>
    EVALUATOR *GetEvaluator(BufferDescriptor const &srcDesc,
                            BufferDescriptor const &dstDesc,
                            DEVICE_CONTEXT *deviceContext) {
        return GetEvaluator(srcDesc, dstDesc,
                            BufferDescriptor(),
                            BufferDescriptor(),
                            BufferDescriptor(),
                            BufferDescriptor(),
                            BufferDescriptor(),
                            deviceContext);
    }

    template <typename DEVICE_CONTEXT>
    EVALUATOR *GetEvaluator(BufferDescriptor const &srcDesc,
                            BufferDescriptor const &dstDesc,
                            BufferDescriptor const &duDesc,
                            BufferDescriptor const &dvDesc,
                            DEVICE_CONTEXT *deviceContext) {
        return GetEvaluator(srcDesc, dstDesc,
                            duDesc, dvDesc,
                            BufferDescriptor(),
                            BufferDescriptor(),
                            BufferDescriptor(),
                            deviceContext);
    }

    template <typename DEVICE_CONTEXT>
    EVALUATOR *GetEvaluator(BufferDescriptor const &srcDesc,
                            BufferDescriptor const &dstDesc,
                            BufferDescriptor const &duDesc,
                            BufferDescriptor const &dvDesc,
                            BufferDescriptor const &duuDesc,
                            BufferDescriptor const &duvDesc,
                            BufferDescriptor const &dvvDesc,
                            DEVICE_CONTEXT *deviceContext) {

        for(typename Evaluators::iterator it = _evaluators.begin();
            it != _evaluators.end(); ++it) {
            if (isEqual(srcDesc, it->srcDesc) &&
                isEqual(dstDesc, it->dstDesc) &&
                isEqual(duDesc,  it->duDesc) &&
                isEqual(dvDesc,  it->dvDesc) &&
                isEqual(duuDesc, it->duuDesc) &&
                isEqual(duvDesc, it->duvDesc) &&
                isEqual(dvvDesc, it->dvvDesc)) {
                return it->evaluator;
            }
        }
        EVALUATOR *e = EVALUATOR::Create(srcDesc, dstDesc,
                                         duDesc, dvDesc,
                                         duuDesc, duvDesc, dvvDesc,
                                         deviceContext);
        _evaluators.push_back(Entry(srcDesc, dstDesc,
                                    duDesc, dvDesc,
                                    duuDesc, duvDesc, dvvDesc, e));
        return e;
    }

private:
    static bool isEqual(BufferDescriptor const &a,
                        BufferDescriptor const &b) {
        int offsetA = a.stride ? (a.offset % a.stride) : 0;
        int offsetB = b.stride ? (b.offset % b.stride) : 0;

        // Note: XFB kernel needs to be configured with the local offset
        // of the dstDesc to skip preceding primvars.
        return (offsetA == offsetB &&
                a.length == b.length &&
                a.stride == b.stride);
    }

    Evaluators _evaluators;
};

/// @cond INTERNAL

// template helpers to see if the evaluator is instantiatable or not.
template <typename EVALUATOR>
struct instantiatable
{
    typedef char yes[1];
    typedef char no[2];
    template <typename C> static yes &chk(typename C::Instantiatable *t=0);
    template <typename C> static no  &chk(...);
    static bool const value = sizeof(chk<EVALUATOR>(0)) == sizeof(yes);
};
template <bool C, typename T=void>
struct enable_if { typedef T type; };
template <typename T>
struct enable_if<false, T> { };

/// @endcond

// extract a kernel from cache if available
template <typename EVALUATOR, typename DEVICE_CONTEXT>
static EVALUATOR *GetEvaluator(
    EvaluatorCacheT<EVALUATOR> *cache,
    BufferDescriptor const &srcDesc,
    BufferDescriptor const &dstDesc,
    BufferDescriptor const &duDesc,
    BufferDescriptor const &dvDesc,
    BufferDescriptor const &duuDesc,
    BufferDescriptor const &duvDesc,
    BufferDescriptor const &dvvDesc,
    DEVICE_CONTEXT deviceContext,
    typename enable_if<instantiatable<EVALUATOR>::value, void>::type*t=0) {
    (void)t;
    if (cache == NULL) return NULL;
    return cache->GetEvaluator(srcDesc, dstDesc,
                               duDesc, dvDesc, duuDesc, duvDesc, dvvDesc,
                               deviceContext);
}

template <typename EVALUATOR, typename DEVICE_CONTEXT>
static EVALUATOR *GetEvaluator(
    EvaluatorCacheT<EVALUATOR> *cache,
    BufferDescriptor const &srcDesc,
    BufferDescriptor const &dstDesc,
    BufferDescriptor const &duDesc,
    BufferDescriptor const &dvDesc,
    DEVICE_CONTEXT deviceContext,
    typename enable_if<instantiatable<EVALUATOR>::value, void>::type*t=0) {
    (void)t;
    if (cache == NULL) return NULL;
    return cache->GetEvaluator(srcDesc, dstDesc, duDesc, dvDesc, deviceContext);
}

template <typename EVALUATOR, typename DEVICE_CONTEXT>
static EVALUATOR *GetEvaluator(
    EvaluatorCacheT<EVALUATOR> *cache,
    BufferDescriptor const &srcDesc,
    BufferDescriptor const &dstDesc,
    DEVICE_CONTEXT deviceContext,
    typename enable_if<instantiatable<EVALUATOR>::value, void>::type*t=0) {
    (void)t;
    if (cache == NULL) return NULL;
    return cache->GetEvaluator(srcDesc, dstDesc,
                               BufferDescriptor(),
                               BufferDescriptor(),
                               deviceContext);
}

// fallback
template <typename EVALUATOR, typename DEVICE_CONTEXT>
static EVALUATOR *GetEvaluator(
    EvaluatorCacheT<EVALUATOR> *,
    BufferDescriptor const &,
    BufferDescriptor const &,
    BufferDescriptor const &,
    BufferDescriptor const &,
    BufferDescriptor const &,
    BufferDescriptor const &,
    BufferDescriptor const &,
    DEVICE_CONTEXT,
    typename enable_if<!instantiatable<EVALUATOR>::value, void>::type*t=0) {
    (void)t;
    return NULL;
}

template <typename EVALUATOR, typename DEVICE_CONTEXT>
static EVALUATOR *GetEvaluator(
    EvaluatorCacheT<EVALUATOR> *,
    BufferDescriptor const &,
    BufferDescriptor const &,
    BufferDescriptor const &,
    BufferDescriptor const &,
    DEVICE_CONTEXT,
    typename enable_if<!instantiatable<EVALUATOR>::value, void>::type*t=0) {
    (void)t;
    return NULL;
}

template <typename EVALUATOR, typename DEVICE_CONTEXT>
static EVALUATOR *GetEvaluator(
    EvaluatorCacheT<EVALUATOR> *,
    BufferDescriptor const &,
    BufferDescriptor const &,
    DEVICE_CONTEXT,
    typename enable_if<!instantiatable<EVALUATOR>::value, void>::type*t=0) {
    (void)t;
    return NULL;
}

// ---------------------------------------------------------------------------

template <typename VERTEX_BUFFER,
          typename STENCIL_TABLE,
          typename EVALUATOR,
          typename PATCH_TABLE,
          typename DEVICE_CONTEXT = void>
class Mesh : public MeshInterface<PATCH_TABLE> {
public:
    typedef VERTEX_BUFFER VertexBuffer;
    typedef EVALUATOR Evaluator;
    typedef STENCIL_TABLE StencilTable;
    typedef PATCH_TABLE PatchTable;
    typedef DEVICE_CONTEXT DeviceContext;
    typedef EvaluatorCacheT<Evaluator> EvaluatorCache;
    typedef typename PatchTable::VertexBufferBinding VertexBufferBinding;

    Mesh(Far::TopologyRefiner * refiner,
         int numVertexElements,
         int numVaryingElements,
         int level,
         MeshBitset bits = MeshBitset(),
         EvaluatorCache * evaluatorCache = NULL,
         DeviceContext * deviceContext = NULL) :

            _refiner(refiner),
            _farPatchTable(NULL),
            _numVertices(0),
            _maxValence(0),
            _vertexBuffer(NULL),
            _varyingBuffer(NULL),
            _vertexStencilTable(NULL),
            _varyingStencilTable(NULL),
            _evaluatorCache(evaluatorCache),
            _patchTable(NULL),
            _deviceContext(deviceContext) {

        assert(_refiner);

        MeshInterface<PATCH_TABLE>::refineMesh(
            *_refiner, level, bits);

        int vertexBufferStride = numVertexElements +
            (bits.test(MeshInterleaveVarying) ? numVaryingElements : 0);
        int varyingBufferStride =
            (bits.test(MeshInterleaveVarying) ? 0 : numVaryingElements);

        initializeContext(numVertexElements,
                          numVaryingElements,
                          level, bits);

        initializeVertexBuffers(_numVertices,
                                vertexBufferStride,
                                varyingBufferStride);

        // configure vertex buffer descriptor
        _vertexDesc =
            BufferDescriptor(0, numVertexElements, vertexBufferStride);
        if (bits.test(MeshInterleaveVarying)) {
            _varyingDesc = BufferDescriptor(
                numVertexElements, numVaryingElements, vertexBufferStride);
        } else {
            _varyingDesc = BufferDescriptor(
                0, numVaryingElements, varyingBufferStride);
        }
    }

    virtual ~Mesh() {
        delete _refiner;
        delete _farPatchTable;
        delete _vertexBuffer;
        delete _varyingBuffer;
        delete _vertexStencilTable;
        delete _varyingStencilTable;
        delete _patchTable;
        // deviceContext and evaluatorCache are not owned by this class.
    }

    virtual void UpdateVertexBuffer(float const *vertexData,
                                    int startVertex, int numVerts) {
        _vertexBuffer->UpdateData(vertexData, startVertex, numVerts,
                                  _deviceContext);
    }

    virtual void UpdateVaryingBuffer(float const *varyingData,
                                     int startVertex, int numVerts) {
        _varyingBuffer->UpdateData(varyingData, startVertex, numVerts,
                                   _deviceContext);
    }

    virtual void Refine() {

        int numControlVertices = _refiner->GetLevel(0).GetNumVertices();

        BufferDescriptor srcDesc = _vertexDesc;
        BufferDescriptor dstDesc(srcDesc);
        dstDesc.offset += numControlVertices * dstDesc.stride;

        // note that the _evaluatorCache can be NULL and thus
        // the evaluatorInstance can be NULL
        //  (for uninstantiatable kernels CPU,TBB etc)
        Evaluator const *instance = GetEvaluator<Evaluator>(
            _evaluatorCache, srcDesc, dstDesc,
            _deviceContext);

        Evaluator::EvalStencils(_vertexBuffer, srcDesc,
                                _vertexBuffer, dstDesc,
                                _vertexStencilTable,
                                instance, _deviceContext);

        if (_varyingDesc.length > 0) {
            BufferDescriptor vSrcDesc = _varyingDesc;
            BufferDescriptor vDstDesc(vSrcDesc);
            vDstDesc.offset += numControlVertices * vDstDesc.stride;

            instance = GetEvaluator<Evaluator>(
                _evaluatorCache, vSrcDesc, vDstDesc,
                _deviceContext);

            if (_varyingBuffer) {
                // non-interleaved
                Evaluator::EvalStencils(_varyingBuffer, vSrcDesc,
                                        _varyingBuffer, vDstDesc,
                                        _varyingStencilTable,
                                        instance, _deviceContext);
            } else {
                // interleaved
                Evaluator::EvalStencils(_vertexBuffer, vSrcDesc,
                                        _vertexBuffer, vDstDesc,
                                        _varyingStencilTable,
                                        instance, _deviceContext);
            }
        }
    }

    virtual void Synchronize() {
        Evaluator::Synchronize(_deviceContext);
    }

    virtual PatchTable * GetPatchTable() const {
        return _patchTable;
    }

    virtual Far::PatchTable const *GetFarPatchTable() const {
        return _farPatchTable;
    }

    virtual int GetNumVertices() const { return _numVertices; }

    virtual int GetMaxValence() const { return _maxValence; }

    virtual VertexBufferBinding BindVertexBuffer() {
        return _vertexBuffer->BindVBO(_deviceContext);
    }

    virtual VertexBufferBinding BindVaryingBuffer() {
        return _varyingBuffer->BindVBO(_deviceContext);
    }

    virtual VertexBuffer * GetVertexBuffer() {
        return _vertexBuffer;
    }

    virtual VertexBuffer * GetVaryingBuffer() {
        return _varyingBuffer;
    }

    virtual Far::TopologyRefiner const * GetTopologyRefiner() const {
        return _refiner;
    }

private:
    void initializeContext(int numVertexElements,
                           int numVaryingElements,
                           int level, MeshBitset bits) {
        assert(_refiner);

        Far::StencilTableFactory::Options options;
        options.generateOffsets = true;
        options.generateIntermediateLevels =
            _refiner->IsUniform() ? false : true;

        Far::StencilTable const * vertexStencils = NULL;
        Far::StencilTable const * varyingStencils = NULL;

        if (numVertexElements>0) {

            vertexStencils = Far::StencilTableFactory::Create(*_refiner,
                                                              options);
        }

        if (numVaryingElements>0) {

            options.interpolationMode =
                Far::StencilTableFactory::INTERPOLATE_VARYING;

            varyingStencils = Far::StencilTableFactory::Create(*_refiner,
                                                               options);
        }

        Far::PatchTableFactory::Options poptions(level);
        poptions.generateFVarTables = bits.test(MeshFVarData);
        poptions.generateFVarLegacyLinearPatches = !bits.test(MeshFVarAdaptive);
        poptions.generateLegacySharpCornerPatches = !bits.test(MeshUseSmoothCornerPatch);
        poptions.useSingleCreasePatch = bits.test(MeshUseSingleCreasePatch);
        poptions.useInfSharpPatch = bits.test(MeshUseInfSharpPatch);

        // points on bilinear and gregory basis endcap boundaries can be
        // shared among adjacent patches to save some stencils.
        if (bits.test(MeshEndCapBilinearBasis)) {
            poptions.SetEndCapType(
                Far::PatchTableFactory::Options::ENDCAP_BILINEAR_BASIS);
            poptions.shareEndCapPatchPoints = true;
        } else if (bits.test(MeshEndCapBSplineBasis)) {
            poptions.SetEndCapType(
                Far::PatchTableFactory::Options::ENDCAP_BSPLINE_BASIS);
        } else if (bits.test(MeshEndCapGregoryBasis)) {
            poptions.SetEndCapType(
                Far::PatchTableFactory::Options::ENDCAP_GREGORY_BASIS);
            poptions.shareEndCapPatchPoints = true;
        } else if (bits.test(MeshEndCapLegacyGregory)) {
            poptions.SetEndCapType(
                Far::PatchTableFactory::Options::ENDCAP_LEGACY_GREGORY);
        }

        _farPatchTable = Far::PatchTableFactory::Create(*_refiner, poptions);

        // if there's endcap stencils, merge it into regular stencils.
        if (_farPatchTable->GetLocalPointStencilTable()) {
            // append stencils
            if (Far::StencilTable const *vertexStencilsWithLocalPoints =
                Far::StencilTableFactory::AppendLocalPointStencilTable(
                    *_refiner,
                    vertexStencils,
                    _farPatchTable->GetLocalPointStencilTable())) {
                delete vertexStencils;
                vertexStencils = vertexStencilsWithLocalPoints;
            }
            if (varyingStencils) {
                if (Far::StencilTable const *varyingStencilsWithLocalPoints =
                    Far::StencilTableFactory::AppendLocalPointStencilTable(
                        *_refiner,
                        varyingStencils,
                        _farPatchTable->GetLocalPointVaryingStencilTable())) {
                    delete varyingStencils;
                    varyingStencils = varyingStencilsWithLocalPoints;
                }
            }
        }

        _maxValence = _farPatchTable->GetMaxValence();
        _patchTable = PatchTable::Create(_farPatchTable, _deviceContext);

        // numvertices = coarse verts + refined verts + gregory basis verts
        _numVertices = vertexStencils->GetNumControlVertices()
            + vertexStencils->GetNumStencils();

        // convert to device stenciltable if necessary.
        _vertexStencilTable =
            convertToCompatibleStencilTable<StencilTable>(
            vertexStencils, _deviceContext);
        _varyingStencilTable =
            convertToCompatibleStencilTable<StencilTable>(
            varyingStencils, _deviceContext);

        // FIXME: we do extra copyings for Far::Stencils.
        delete vertexStencils;
        delete varyingStencils;
    }

    void initializeVertexBuffers(int numVertices,
                                 int numVertexElements,
                                 int numVaryingElements) {

        if (numVertexElements) {
            _vertexBuffer = VertexBuffer::Create(numVertexElements,
                                                 numVertices, _deviceContext);
        }

        if (numVaryingElements) {
            _varyingBuffer = VertexBuffer::Create(numVaryingElements,
                                                  numVertices, _deviceContext);
        }
    }

    Far::TopologyRefiner * _refiner;
    Far::PatchTable * _farPatchTable;

    int _numVertices;
    int _maxValence;

    VertexBuffer * _vertexBuffer;
    VertexBuffer * _varyingBuffer;

    BufferDescriptor _vertexDesc;
    BufferDescriptor _varyingDesc;

    StencilTable const * _vertexStencilTable;
    StencilTable const * _varyingStencilTable;
    EvaluatorCache * _evaluatorCache;

    PatchTable *_patchTable;
    DeviceContext *_deviceContext;
};

} // end namespace Osd

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv

#endif  // OPENSUBDIV3_OSD_MESH_H
