//
//   Copyright 2015 DreamWorks Animation LLC.
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
#ifndef OPENSUBDIV3_FAR_PRIMVAR_REFINER_H
#define OPENSUBDIV3_FAR_PRIMVAR_REFINER_H

#include "../version.h"

#include "../sdc/types.h"
#include "../sdc/options.h"
#include "../sdc/bilinearScheme.h"
#include "../sdc/catmarkScheme.h"
#include "../sdc/loopScheme.h"
#include "../vtr/level.h"
#include "../vtr/fvarLevel.h"
#include "../vtr/refinement.h"
#include "../vtr/fvarRefinement.h"
#include "../vtr/stackBuffer.h"
#include "../vtr/componentInterfaces.h"
#include "../far/types.h"
#include "../far/error.h"
#include "../far/topologyLevel.h"
#include "../far/topologyRefiner.h"

#include <cassert>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

///
///  \brief Applies refinement operations to generic primvar data.
///
template <typename REAL>
class PrimvarRefinerReal {

public:
    PrimvarRefinerReal(TopologyRefiner const & refiner) : _refiner(refiner) { }
    ~PrimvarRefinerReal() { }

    TopologyRefiner const & GetTopologyRefiner() const { return _refiner; }

    //@{
    ///  @name Primvar data interpolation
    ///
    /// \anchor templating
    ///
    /// \note Interpolation methods template both the source and destination
    ///       data buffer classes. Client-code is expected to provide interfaces
    ///       that implement the functions specific to its primitive variable
    ///       data layout. Template APIs must implement the following:
    ///       <br><br> \code{.cpp}
    ///
    ///       class MySource {
    ///           MySource & operator[](int index);
    ///       };
    ///
    ///       class MyDestination {
    ///           void Clear();
    ///           void AddWithWeight(MySource const & value, float weight);
    ///           void AddWithWeight(MyDestination const & value, float weight);
    ///       };
    ///
    ///       \endcode
    ///       <br>
    ///       It is possible to implement a single interface only and use it as
    ///       both source and destination.
    ///       <br><br>
    ///       Primitive variable buffers are expected to be arrays of instances,
    ///       passed either as direct pointers or with a container
    ///       (ex. std::vector<MyVertex>).
    ///       Some interpolation methods however allow passing the buffers by
    ///       reference: this allows to work transparently with arrays and
    ///       containers (or other schemes that overload the '[]' operator)
    ///       <br><br>
    ///       See the <a href=http://graphics.pixar.com/opensubdiv/docs/tutorials.html>
    ///       Far tutorials</a> for code examples.
    ///

    /// \brief Apply vertex interpolation weights to a primvar buffer for a single
    ///        level of refinement.
    ///
    /// The destination buffer must allocate an array of data for all the
    /// refined vertices, i.e. at least refiner.GetLevel(level).GetNumVertices()
    ///
    /// @param level  The refinement level
    ///
    /// @param src    Source primvar buffer (\ref templating control vertex data)
    ///
    /// @param dst    Destination primvar buffer (\ref templating refined vertex data)
    ///
    template <class T, class U> void Interpolate(int level, T const & src, U & dst) const;

    /// \brief Apply only varying interpolation weights to a primvar buffer
    ///        for a single level of refinement.
    ///
    /// This method can useful if the varying primvar data does not need to be
    /// re-computed over time.
    ///
    /// The destination buffer must allocate an array of data for all the
    /// refined vertices, i.e. at least refiner.GetLevel(level).GetNumVertices()
    ///
    /// @param level  The refinement level
    ///
    /// @param src    Source primvar buffer (\ref templating control vertex data)
    ///
    /// @param dst    Destination primvar buffer (\ref templating refined vertex data)
    ///
    template <class T, class U> void InterpolateVarying(int level, T const & src, U & dst) const;

    /// \brief Refine uniform (per-face) primvar data between levels.
    ///
    /// Data is simply copied from a parent face to its child faces and does not involve
    /// any weighting.  Setting the source primvar data for the base level to be the index
    /// of each face allows the propagation of the base face to primvar data for child
    /// faces in all levels.
    ///
    /// The destination buffer must allocate an array of data for all the refined faces,
    /// i.e. at least refiner.GetLevel(level).GetNumFaces()
    ///
    /// @param level  The refinement level
    ///
    /// @param src    Source primvar buffer
    ///
    /// @param dst    Destination primvar buffer
    ///
    template <class T, class U> void InterpolateFaceUniform(int level, T const & src, U & dst) const;

    /// \brief Apply face-varying interpolation weights to a primvar buffer
    ///        associated with a particular face-varying channel.
    ///
    /// Unlike vertex and varying primvar buffers, there is not a 1-to-1 correspondence
    /// between vertices and face-varying values -- typically there are more face-varying
    /// values than vertices.  Each face-varying channel is also independent in how its
    /// values relate to the vertices.
    ///
    /// The destination buffer must allocate an array of data for all the refined values,
    /// i.e. at least refiner.GetLevel(level).GetNumFVarValues(channel).
    ///
    template <class T, class U> void InterpolateFaceVarying(int level, T const & src, U & dst, int channel = 0) const;


    /// \brief Apply limit weights to a primvar buffer
    ///
    /// The source buffer must refer to an array of previously interpolated
    /// vertex data for the last refinement level.  The destination buffer
    /// must allocate an array for all vertices at the last refinement level,
    /// i.e. at least refiner.GetLevel(refiner.GetMaxLevel()).GetNumVertices()
    ///
    /// @param src     Source primvar buffer (refined data) for last level
    ///
    /// @param dstPos  Destination primvar buffer (data at the limit)
    ///
    template <class T, class U> void Limit(T const & src, U & dstPos) const;

    template <class T, class U, class U1, class U2>
    void Limit(T const & src, U & dstPos, U1 & dstTan1, U2 & dstTan2) const;

    template <class T, class U> void LimitFaceVarying(T const & src, U & dst, int channel = 0) const;

    //@}

private:
    typedef REAL Weight;

    //  Non-copyable:
    PrimvarRefinerReal(PrimvarRefinerReal const & src) : _refiner(src._refiner) { }
    PrimvarRefinerReal & operator=(PrimvarRefinerReal const &) { return *this; }

    template <Sdc::SchemeType SCHEME, class T, class U> void interpFromFaces(int, T const &, U &) const;
    template <Sdc::SchemeType SCHEME, class T, class U> void interpFromEdges(int, T const &, U &) const;
    template <Sdc::SchemeType SCHEME, class T, class U> void interpFromVerts(int, T const &, U &) const;

    template <Sdc::SchemeType SCHEME, class T, class U> void interpFVarFromFaces(int, T const &, U &, int) const;
    template <Sdc::SchemeType SCHEME, class T, class U> void interpFVarFromEdges(int, T const &, U &, int) const;
    template <Sdc::SchemeType SCHEME, class T, class U> void interpFVarFromVerts(int, T const &, U &, int) const;

    template <Sdc::SchemeType SCHEME, class T, class U, class U1, class U2>
    void limit(T const & src, U & pos, U1 * tan1, U2 * tan2) const;

    template <Sdc::SchemeType SCHEME, class T, class U>
    void limitFVar(T const & src, U & dst, int channel) const;

private:
    TopologyRefiner const &  _refiner;

private:
    //
    //  Local class to fulfill interface for <typename MASK> in the Scheme mask queries:
    //
    class Mask {
    public:
        typedef REAL Weight;  //  Also part of the expected interface

    public:
        Mask(Weight* v, Weight* e, Weight* f) : 
            _vertWeights(v), _edgeWeights(e), _faceWeights(f),
            _vertCount(0), _edgeCount(0), _faceCount(0), 
            _faceWeightsForFaceCenters(false)
        { }

        ~Mask() { }

    public:  //  Generic interface expected of <typename MASK>:
        int GetNumVertexWeights() const { return _vertCount; }
        int GetNumEdgeWeights()   const { return _edgeCount; }
        int GetNumFaceWeights()   const { return _faceCount; }

        void SetNumVertexWeights(int count) { _vertCount = count; }
        void SetNumEdgeWeights(  int count) { _edgeCount = count; }
        void SetNumFaceWeights(  int count) { _faceCount = count; }

        Weight const& VertexWeight(int index) const { return _vertWeights[index]; }
        Weight const& EdgeWeight(  int index) const { return _edgeWeights[index]; }
        Weight const& FaceWeight(  int index) const { return _faceWeights[index]; }

        Weight& VertexWeight(int index) { return _vertWeights[index]; }
        Weight& EdgeWeight(  int index) { return _edgeWeights[index]; }
        Weight& FaceWeight(  int index) { return _faceWeights[index]; }

        bool AreFaceWeightsForFaceCenters() const  { return _faceWeightsForFaceCenters; }
        void SetFaceWeightsForFaceCenters(bool on) { _faceWeightsForFaceCenters = on; }

    private:
        Weight* _vertWeights;
        Weight* _edgeWeights;
        Weight* _faceWeights;

        int _vertCount;
        int _edgeCount;
        int _faceCount;

        bool _faceWeightsForFaceCenters;
    };
};


//
//  Public entry points to the methods.  Queries of the scheme type and its
//  use as a template parameter in subsequent implementation will be factored
//  out of a later release:
//
template <typename REAL>
template <class T, class U>
inline void
PrimvarRefinerReal<REAL>::Interpolate(int level, T const & src, U & dst) const {

    assert(level>0 && level<=(int)_refiner._refinements.size());

    switch (_refiner._subdivType) {
    case Sdc::SCHEME_CATMARK:
        interpFromFaces<Sdc::SCHEME_CATMARK>(level, src, dst);
        interpFromEdges<Sdc::SCHEME_CATMARK>(level, src, dst);
        interpFromVerts<Sdc::SCHEME_CATMARK>(level, src, dst);
        break;
    case Sdc::SCHEME_LOOP:
        interpFromFaces<Sdc::SCHEME_LOOP>(level, src, dst);
        interpFromEdges<Sdc::SCHEME_LOOP>(level, src, dst);
        interpFromVerts<Sdc::SCHEME_LOOP>(level, src, dst);
        break;
    case Sdc::SCHEME_BILINEAR:
        interpFromFaces<Sdc::SCHEME_BILINEAR>(level, src, dst);
        interpFromEdges<Sdc::SCHEME_BILINEAR>(level, src, dst);
        interpFromVerts<Sdc::SCHEME_BILINEAR>(level, src, dst);
        break;
    }
}

template <typename REAL>
template <class T, class U>
inline void
PrimvarRefinerReal<REAL>::InterpolateFaceVarying(int level, T const & src, U & dst, int channel) const {

    assert(level>0 && level<=(int)_refiner._refinements.size());

    switch (_refiner._subdivType) {
    case Sdc::SCHEME_CATMARK:
        interpFVarFromFaces<Sdc::SCHEME_CATMARK>(level, src, dst, channel);
        interpFVarFromEdges<Sdc::SCHEME_CATMARK>(level, src, dst, channel);
        interpFVarFromVerts<Sdc::SCHEME_CATMARK>(level, src, dst, channel);
        break;
    case Sdc::SCHEME_LOOP:
        interpFVarFromFaces<Sdc::SCHEME_LOOP>(level, src, dst, channel);
        interpFVarFromEdges<Sdc::SCHEME_LOOP>(level, src, dst, channel);
        interpFVarFromVerts<Sdc::SCHEME_LOOP>(level, src, dst, channel);
        break;
    case Sdc::SCHEME_BILINEAR:
        interpFVarFromFaces<Sdc::SCHEME_BILINEAR>(level, src, dst, channel);
        interpFVarFromEdges<Sdc::SCHEME_BILINEAR>(level, src, dst, channel);
        interpFVarFromVerts<Sdc::SCHEME_BILINEAR>(level, src, dst, channel);
        break;
    }
}

template <typename REAL>
template <class T, class U>
inline void
PrimvarRefinerReal<REAL>::Limit(T const & src, U & dst) const {

    if (_refiner.getLevel(_refiner.GetMaxLevel()).getNumVertexEdgesTotal() == 0) {
        Error(FAR_RUNTIME_ERROR,
            "Failure in PrimvarRefiner::Limit() -- "
            "last level of refinement does not include full topology.");
        return;
    }

    switch (_refiner._subdivType) {
    case Sdc::SCHEME_CATMARK:
        limit<Sdc::SCHEME_CATMARK>(src, dst, (U*)0, (U*)0);
        break;
    case Sdc::SCHEME_LOOP:
        limit<Sdc::SCHEME_LOOP>(src, dst, (U*)0, (U*)0);
        break;
    case Sdc::SCHEME_BILINEAR:
        limit<Sdc::SCHEME_BILINEAR>(src, dst, (U*)0, (U*)0);
        break;
    }
}

template <typename REAL>
template <class T, class U, class U1, class U2>
inline void
PrimvarRefinerReal<REAL>::Limit(T const & src, U & dstPos, U1 & dstTan1, U2 & dstTan2) const {

    if (_refiner.getLevel(_refiner.GetMaxLevel()).getNumVertexEdgesTotal() == 0) {
        Error(FAR_RUNTIME_ERROR,
            "Failure in PrimvarRefiner::Limit() -- "
            "last level of refinement does not include full topology.");
        return;
    }

    switch (_refiner._subdivType) {
    case Sdc::SCHEME_CATMARK:
        limit<Sdc::SCHEME_CATMARK>(src, dstPos, &dstTan1, &dstTan2);
        break;
    case Sdc::SCHEME_LOOP:
        limit<Sdc::SCHEME_LOOP>(src, dstPos, &dstTan1, &dstTan2);
        break;
    case Sdc::SCHEME_BILINEAR:
        limit<Sdc::SCHEME_BILINEAR>(src, dstPos, &dstTan1, &dstTan2);
        break;
    }
}

template <typename REAL>
template <class T, class U>
inline void
PrimvarRefinerReal<REAL>::LimitFaceVarying(T const & src, U & dst, int channel) const {

    if (_refiner.getLevel(_refiner.GetMaxLevel()).getNumVertexEdgesTotal() == 0) {
        Error(FAR_RUNTIME_ERROR,
            "Failure in PrimvarRefiner::LimitFaceVarying() -- "
            "last level of refinement does not include full topology.");
        return;
    }

    switch (_refiner._subdivType) {
    case Sdc::SCHEME_CATMARK:
        limitFVar<Sdc::SCHEME_CATMARK>(src, dst, channel);
        break;
    case Sdc::SCHEME_LOOP:
        limitFVar<Sdc::SCHEME_LOOP>(src, dst, channel);
        break;
    case Sdc::SCHEME_BILINEAR:
        limitFVar<Sdc::SCHEME_BILINEAR>(src, dst, channel);
        break;
    }
}

template <typename REAL>
template <class T, class U>
inline void
PrimvarRefinerReal<REAL>::InterpolateFaceUniform(int level, T const & src, U & dst) const {

    assert(level>0 && level<=(int)_refiner._refinements.size());

    Vtr::internal::Refinement const & refinement = _refiner.getRefinement(level-1);
    Vtr::internal::Level const & child = refinement.child();

    for (int cFace = 0; cFace < child.getNumFaces(); ++cFace) {

        Vtr::Index pFace = refinement.getChildFaceParentFace(cFace);

        dst[cFace] = src[pFace];
    }
}

template <typename REAL>
template <class T, class U>
inline void
PrimvarRefinerReal<REAL>::InterpolateVarying(int level, T const & src, U & dst) const {

    assert(level>0 && level<=(int)_refiner._refinements.size());

    Vtr::internal::Refinement const & refinement = _refiner.getRefinement(level-1);
    Vtr::internal::Level const &      parent     = refinement.parent();

    //
    //  Group values to interpolate based on origin -- note that there may
    //  be none originating from faces:
    //
    if (refinement.getNumChildVerticesFromFaces() > 0) {

        for (int face = 0; face < parent.getNumFaces(); ++face) {

            Vtr::Index cVert = refinement.getFaceChildVertex(face);
            if (Vtr::IndexIsValid(cVert)) {

                //  Apply the weights to the parent face's vertices:
                ConstIndexArray fVerts = parent.getFaceVertices(face);

                Weight fVaryingWeight = 1.0f / (Weight) fVerts.size();

                dst[cVert].Clear();
                for (int i = 0; i < fVerts.size(); ++i) {
                    dst[cVert].AddWithWeight(src[fVerts[i]], fVaryingWeight);
                }
            }
        }
    }
    for (int edge = 0; edge < parent.getNumEdges(); ++edge) {

        Vtr::Index cVert = refinement.getEdgeChildVertex(edge);
        if (Vtr::IndexIsValid(cVert)) {

            //  Apply the weights to the parent edges's vertices
            ConstIndexArray eVerts = parent.getEdgeVertices(edge);

            dst[cVert].Clear();
            dst[cVert].AddWithWeight(src[eVerts[0]], 0.5f);
            dst[cVert].AddWithWeight(src[eVerts[1]], 0.5f);
        }
    }
    for (int vert = 0; vert < parent.getNumVertices(); ++vert) {

        Vtr::Index cVert = refinement.getVertexChildVertex(vert);
        if (Vtr::IndexIsValid(cVert)) {

            //  Essentially copy the parent vertex:
            dst[cVert].Clear();
            dst[cVert].AddWithWeight(src[vert], 1.0f);
        }
    }
}


//
//  Internal implementation methods -- grouping vertices to be interpolated
//  based on the type of parent component from which they originated:
//
template <typename REAL>
template <Sdc::SchemeType SCHEME, class T, class U>
inline void
PrimvarRefinerReal<REAL>::interpFromFaces(int level, T const & src, U & dst) const {

    Vtr::internal::Refinement const & refinement = _refiner.getRefinement(level-1);
    Vtr::internal::Level const &      parent     = refinement.parent();

    if (refinement.getNumChildVerticesFromFaces() == 0) return;

    Sdc::Scheme<SCHEME> scheme(_refiner._subdivOptions);

    Vtr::internal::StackBuffer<Weight,16> fVertWeights(parent.getMaxValence());

    for (int face = 0; face < parent.getNumFaces(); ++face) {

        Vtr::Index cVert = refinement.getFaceChildVertex(face);
        if (!Vtr::IndexIsValid(cVert))
            continue;

        //  Declare and compute mask weights for this vertex relative to its parent face:
        ConstIndexArray fVerts = parent.getFaceVertices(face);

        Mask fMask(fVertWeights, 0, 0);
        Vtr::internal::FaceInterface fHood(fVerts.size());

        scheme.ComputeFaceVertexMask(fHood, fMask);

        //  Apply the weights to the parent face's vertices:
        dst[cVert].Clear();

        for (int i = 0; i < fVerts.size(); ++i) {

            dst[cVert].AddWithWeight(src[fVerts[i]], fVertWeights[i]);
        }
    }
}

template <typename REAL>
template <Sdc::SchemeType SCHEME, class T, class U>
inline void
PrimvarRefinerReal<REAL>::interpFromEdges(int level, T const & src, U & dst) const {

    Vtr::internal::Refinement const & refinement = _refiner.getRefinement(level-1);
    Vtr::internal::Level const &      parent     = refinement.parent();
    Vtr::internal::Level const &      child      = refinement.child();

    Sdc::Scheme<SCHEME> scheme(_refiner._subdivOptions);

    Vtr::internal::EdgeInterface eHood(parent);

    Weight                               eVertWeights[2];
    Vtr::internal::StackBuffer<Weight,8> eFaceWeights(parent.getMaxEdgeFaces());

    for (int edge = 0; edge < parent.getNumEdges(); ++edge) {

        Vtr::Index cVert = refinement.getEdgeChildVertex(edge);
        if (!Vtr::IndexIsValid(cVert))
            continue;

        //  Declare and compute mask weights for this vertex relative to its parent edge:
        ConstIndexArray eVerts = parent.getEdgeVertices(edge),
                        eFaces = parent.getEdgeFaces(edge);

        Mask eMask(eVertWeights, 0, eFaceWeights);

        eHood.SetIndex(edge);

        Sdc::Crease::Rule pRule = (parent.getEdgeSharpness(edge) > 0.0f) ? Sdc::Crease::RULE_CREASE : Sdc::Crease::RULE_SMOOTH;
        Sdc::Crease::Rule cRule = child.getVertexRule(cVert);

        scheme.ComputeEdgeVertexMask(eHood, eMask, pRule, cRule);

        //  Apply the weights to the parent edges's vertices and (if applicable) to
        //  the child vertices of its incident faces:
        dst[cVert].Clear();
        dst[cVert].AddWithWeight(src[eVerts[0]], eVertWeights[0]);
        dst[cVert].AddWithWeight(src[eVerts[1]], eVertWeights[1]);

        if (eMask.GetNumFaceWeights() > 0) {

            for (int i = 0; i < eFaces.size(); ++i) {

                if (eMask.AreFaceWeightsForFaceCenters()) {
                    assert(refinement.getNumChildVerticesFromFaces() > 0);
                    Vtr::Index cVertOfFace = refinement.getFaceChildVertex(eFaces[i]);

                    assert(Vtr::IndexIsValid(cVertOfFace));
                    dst[cVert].AddWithWeight(dst[cVertOfFace], eFaceWeights[i]);
                } else {
                    Vtr::Index            pFace      = eFaces[i];
                    ConstIndexArray pFaceEdges = parent.getFaceEdges(pFace),
                                    pFaceVerts = parent.getFaceVertices(pFace);

                    int eInFace = 0;
                    for ( ; pFaceEdges[eInFace] != edge; ++eInFace ) ;

                    int vInFace = eInFace + 2;
                    if (vInFace >= pFaceVerts.size()) vInFace -= pFaceVerts.size();

                    Vtr::Index pVertNext = pFaceVerts[vInFace];
                    dst[cVert].AddWithWeight(src[pVertNext], eFaceWeights[i]);
                }
            }
        }
    }
}

template <typename REAL>
template <Sdc::SchemeType SCHEME, class T, class U>
inline void
PrimvarRefinerReal<REAL>::interpFromVerts(int level, T const & src, U & dst) const {

    Vtr::internal::Refinement const & refinement = _refiner.getRefinement(level-1);
    Vtr::internal::Level const &      parent     = refinement.parent();
    Vtr::internal::Level const &      child      = refinement.child();

    Sdc::Scheme<SCHEME> scheme(_refiner._subdivOptions);

    Vtr::internal::VertexInterface vHood(parent, child);

    Vtr::internal::StackBuffer<Weight,32> weightBuffer(2*parent.getMaxValence());

    for (int vert = 0; vert < parent.getNumVertices(); ++vert) {

        Vtr::Index cVert = refinement.getVertexChildVertex(vert);
        if (!Vtr::IndexIsValid(cVert))
            continue;

        //  Declare and compute mask weights for this vertex relative to its parent edge:
        ConstIndexArray vEdges = parent.getVertexEdges(vert),
                        vFaces = parent.getVertexFaces(vert);

        Weight   vVertWeight,
               * vEdgeWeights = weightBuffer,
               * vFaceWeights = vEdgeWeights + vEdges.size();

        Mask vMask(&vVertWeight, vEdgeWeights, vFaceWeights);

        vHood.SetIndex(vert, cVert);

        Sdc::Crease::Rule pRule = parent.getVertexRule(vert);
        Sdc::Crease::Rule cRule = child.getVertexRule(cVert);

        scheme.ComputeVertexVertexMask(vHood, vMask, pRule, cRule);

        //  Apply the weights to the parent vertex, the vertices opposite its incident
        //  edges, and the child vertices of its incident faces:
        //
        //  In order to improve numerical precision, it's better to apply smaller weights
        //  first, so begin with the face-weights followed by the edge-weights and the
        //  vertex weight last.
        dst[cVert].Clear();

        if (vMask.GetNumFaceWeights() > 0) {
            assert(vMask.AreFaceWeightsForFaceCenters());

            for (int i = 0; i < vFaces.size(); ++i) {

                Vtr::Index cVertOfFace = refinement.getFaceChildVertex(vFaces[i]);
                assert(Vtr::IndexIsValid(cVertOfFace));
                dst[cVert].AddWithWeight(dst[cVertOfFace], vFaceWeights[i]);
            }
        }
        if (vMask.GetNumEdgeWeights() > 0) {

            for (int i = 0; i < vEdges.size(); ++i) {

                ConstIndexArray eVerts = parent.getEdgeVertices(vEdges[i]);
                Vtr::Index pVertOppositeEdge = (eVerts[0] == vert) ? eVerts[1] : eVerts[0];

                dst[cVert].AddWithWeight(src[pVertOppositeEdge], vEdgeWeights[i]);
            }
        }
        dst[cVert].AddWithWeight(src[vert], vVertWeight);
    }
}


//
// Internal face-varying implementation details:
//
template <typename REAL>
template <Sdc::SchemeType SCHEME, class T, class U>
inline void
PrimvarRefinerReal<REAL>::interpFVarFromFaces(int level, T const & src, U & dst, int channel) const {

    Vtr::internal::Refinement const & refinement = _refiner.getRefinement(level-1);

    if (refinement.getNumChildVerticesFromFaces() == 0) return;

    Sdc::Scheme<SCHEME> scheme(_refiner._subdivOptions);

    Vtr::internal::Level const & parentLevel = refinement.parent();
    Vtr::internal::Level const & childLevel  = refinement.child();

    Vtr::internal::FVarLevel const & parentFVar = parentLevel.getFVarLevel(channel);
    Vtr::internal::FVarLevel const & childFVar  = childLevel.getFVarLevel(channel);

    Vtr::internal::StackBuffer<Weight,16> fValueWeights(parentLevel.getMaxValence());

    for (int face = 0; face < parentLevel.getNumFaces(); ++face) {

        Vtr::Index cVert = refinement.getFaceChildVertex(face);
        if (!Vtr::IndexIsValid(cVert))
            continue;

        Vtr::Index cVertValue = childFVar.getVertexValueOffset(cVert);

        //  The only difference for face-varying here is that we get the values associated
        //  with each face-vertex directly from the FVarLevel, rather than using the parent
        //  face-vertices directly.  If any face-vertex has any sibling values, then we may
        //  get the wrong one using the face-vertex index directly.

        //  Declare and compute mask weights for this vertex relative to its parent face:
        ConstIndexArray fValues = parentFVar.getFaceValues(face);

        Mask fMask(fValueWeights, 0, 0);
        Vtr::internal::FaceInterface fHood(fValues.size());

        scheme.ComputeFaceVertexMask(fHood, fMask);

        //  Apply the weights to the parent face's vertices:
        dst[cVertValue].Clear();

        for (int i = 0; i < fValues.size(); ++i) {
            dst[cVertValue].AddWithWeight(src[fValues[i]], fValueWeights[i]);
        }
    }
}

template <typename REAL>
template <Sdc::SchemeType SCHEME, class T, class U>
inline void
PrimvarRefinerReal<REAL>::interpFVarFromEdges(int level, T const & src, U & dst, int channel) const {

    Vtr::internal::Refinement const & refinement = _refiner.getRefinement(level-1);

    Sdc::Scheme<SCHEME> scheme(_refiner._subdivOptions);

    Vtr::internal::Level const & parentLevel = refinement.parent();
    Vtr::internal::Level const & childLevel  = refinement.child();

    Vtr::internal::FVarRefinement const & refineFVar = refinement.getFVarRefinement(channel);
    Vtr::internal::FVarLevel const &      parentFVar = parentLevel.getFVarLevel(channel);
    Vtr::internal::FVarLevel const &      childFVar  = childLevel.getFVarLevel(channel);

    //
    //  Allocate and initialize (if linearly interpolated) interpolation weights for
    //  the edge mask:
    //
    Weight                               eVertWeights[2];
    Vtr::internal::StackBuffer<Weight,8> eFaceWeights(parentLevel.getMaxEdgeFaces());

    Mask eMask(eVertWeights, 0, eFaceWeights);

    bool isLinearFVar = parentFVar.isLinear() || (_refiner._subdivType == Sdc::SCHEME_BILINEAR);
    if (isLinearFVar) {
        eMask.SetNumVertexWeights(2);
        eMask.SetNumEdgeWeights(0);
        eMask.SetNumFaceWeights(0);

        eVertWeights[0] = 0.5f;
        eVertWeights[1] = 0.5f;
    }

    Vtr::internal::EdgeInterface eHood(parentLevel);

    for (int edge = 0; edge < parentLevel.getNumEdges(); ++edge) {

        Vtr::Index cVert = refinement.getEdgeChildVertex(edge);
        if (!Vtr::IndexIsValid(cVert))
            continue;

        ConstIndexArray cVertValues = childFVar.getVertexValues(cVert);

        bool fvarEdgeVertMatchesVertex = childFVar.valueTopologyMatches(cVertValues[0]);
        if (fvarEdgeVertMatchesVertex) {
            //
            //  If smoothly interpolated, compute new weights for the edge mask:
            //
            if (!isLinearFVar) {
                eHood.SetIndex(edge);

                Sdc::Crease::Rule pRule = (parentLevel.getEdgeSharpness(edge) > 0.0f)
                                        ? Sdc::Crease::RULE_CREASE : Sdc::Crease::RULE_SMOOTH;
                Sdc::Crease::Rule cRule = childLevel.getVertexRule(cVert);

                scheme.ComputeEdgeVertexMask(eHood, eMask, pRule, cRule);
            }

            //  Apply the weights to the parent edge's vertices and (if applicable) to
            //  the child vertices of its incident faces:
            //
            //  Even though the face-varying topology matches the vertex topology, we need
            //  to be careful here when getting values corresponding to the two end-vertices.
            //  While the edge may be continuous, the vertices at their ends may have
            //  discontinuities elsewhere in their neighborhood (i.e. on the "other side"
            //  of the end-vertex) and so have sibling values associated with them.  In most
            //  cases the topology for an end-vertex will match and we can use it directly,
            //  but we must still check and retrieve as needed.
            //
            //  Indices for values corresponding to face-vertices are guaranteed to match,
            //  so we can use the child-vertex indices directly.
            //
            //  And by "directly", we always use getVertexValue(vertexIndex) to reference
            //  values in the "src" to account for the possible indirection that may exist at
            //  level 0 -- where there may be fewer values than vertices and an additional
            //  indirection is necessary.  We can use a vertex index directly for "dst" when
            //  it matches.
            //
            Vtr::Index eVertValues[2];

            parentFVar.getEdgeFaceValues(edge, 0, eVertValues);

            Index cVertValue = cVertValues[0];

            dst[cVertValue].Clear();
            dst[cVertValue].AddWithWeight(src[eVertValues[0]], eVertWeights[0]);
            dst[cVertValue].AddWithWeight(src[eVertValues[1]], eVertWeights[1]);

            if (eMask.GetNumFaceWeights() > 0) {

                ConstIndexArray  eFaces = parentLevel.getEdgeFaces(edge);

                for (int i = 0; i < eFaces.size(); ++i) {
                    if (eMask.AreFaceWeightsForFaceCenters()) {

                        Vtr::Index cVertOfFace = refinement.getFaceChildVertex(eFaces[i]);
                        assert(Vtr::IndexIsValid(cVertOfFace));

                        Vtr::Index cValueOfFace = childFVar.getVertexValueOffset(cVertOfFace);
                        dst[cVertValue].AddWithWeight(dst[cValueOfFace], eFaceWeights[i]);
                    } else {
                        Vtr::Index            pFace      = eFaces[i];
                        ConstIndexArray pFaceEdges = parentLevel.getFaceEdges(pFace),
                                        pFaceVerts = parentLevel.getFaceVertices(pFace);

                        int eInFace = 0;
                        for ( ; pFaceEdges[eInFace] != edge; ++eInFace ) ;

                        //  Edge "i" spans vertices [i,i+1] so we want i+2...
                        int vInFace = eInFace + 2;
                        if (vInFace >= pFaceVerts.size()) vInFace -= pFaceVerts.size();

                        Vtr::Index pValueNext = parentFVar.getFaceValues(pFace)[vInFace];
                        dst[cVertValue].AddWithWeight(src[pValueNext], eFaceWeights[i]);
                    }
                }
            }
        } else {
            //
            //  Mismatched edge-verts should just be linearly interpolated between the pairs of
            //  values for each sibling of the child edge-vertex -- the question is:  which face
            //  holds that pair of values for a given sibling?
            //
            //  In the manifold case, the sibling and edge-face indices will correspond.  We
            //  will eventually need to update this to account for > 3 incident faces.
            //
            for (int i = 0; i < cVertValues.size(); ++i) {
                Vtr::Index eVertValues[2];
                int      eFaceIndex = refineFVar.getChildValueParentSource(cVert, i);
                assert(eFaceIndex == i);

                parentFVar.getEdgeFaceValues(edge, eFaceIndex, eVertValues);

                Index cVertValue = cVertValues[i];

                dst[cVertValue].Clear();
                dst[cVertValue].AddWithWeight(src[eVertValues[0]], 0.5);
                dst[cVertValue].AddWithWeight(src[eVertValues[1]], 0.5);
            }
        }
    }
}

template <typename REAL>
template <Sdc::SchemeType SCHEME, class T, class U>
inline void
PrimvarRefinerReal<REAL>::interpFVarFromVerts(int level, T const & src, U & dst, int channel) const {

    Vtr::internal::Refinement const & refinement = _refiner.getRefinement(level-1);

    Sdc::Scheme<SCHEME> scheme(_refiner._subdivOptions);

    Vtr::internal::Level const & parentLevel = refinement.parent();
    Vtr::internal::Level const & childLevel  = refinement.child();

    Vtr::internal::FVarRefinement const & refineFVar = refinement.getFVarRefinement(channel);
    Vtr::internal::FVarLevel const &      parentFVar = parentLevel.getFVarLevel(channel);
    Vtr::internal::FVarLevel const &      childFVar  = childLevel.getFVarLevel(channel);

    bool isLinearFVar = parentFVar.isLinear() || (_refiner._subdivType == Sdc::SCHEME_BILINEAR);

    Vtr::internal::StackBuffer<Weight,32> weightBuffer(2*parentLevel.getMaxValence());

    Vtr::internal::StackBuffer<Vtr::Index,16> vEdgeValues(parentLevel.getMaxValence());

    Vtr::internal::VertexInterface vHood(parentLevel, childLevel);

    for (int vert = 0; vert < parentLevel.getNumVertices(); ++vert) {

        Vtr::Index cVert = refinement.getVertexChildVertex(vert);
        if (!Vtr::IndexIsValid(cVert))
            continue;

        ConstIndexArray pVertValues = parentFVar.getVertexValues(vert),
                        cVertValues = childFVar.getVertexValues(cVert);

        bool fvarVertVertMatchesVertex = childFVar.valueTopologyMatches(cVertValues[0]);
        if (isLinearFVar && fvarVertVertMatchesVertex) {
            dst[cVertValues[0]].Clear();
            dst[cVertValues[0]].AddWithWeight(src[pVertValues[0]], 1.0f);
            continue;
        }

        if (fvarVertVertMatchesVertex) {
            //
            //  Declare and compute mask weights for this vertex relative to its parent edge:
            //
            //  (We really need to encapsulate this somewhere else for use here and in the
            //  general case)
            //
            ConstIndexArray vEdges = parentLevel.getVertexEdges(vert);

            Weight   vVertWeight;
            Weight * vEdgeWeights = weightBuffer;
            Weight * vFaceWeights = vEdgeWeights + vEdges.size();

            Mask vMask(&vVertWeight, vEdgeWeights, vFaceWeights);

            vHood.SetIndex(vert, cVert);

            Sdc::Crease::Rule pRule = parentLevel.getVertexRule(vert);
            Sdc::Crease::Rule cRule = childLevel.getVertexRule(cVert);

            scheme.ComputeVertexVertexMask(vHood, vMask, pRule, cRule);

            //  Apply the weights to the parent vertex, the vertices opposite its incident
            //  edges, and the child vertices of its incident faces:
            //
            //  Even though the face-varying topology matches the vertex topology, we need
            //  to be careful here when getting values corresponding to vertices at the
            //  ends of edges.  While the edge may be continuous, the end vertex may have
            //  discontinuities elsewhere in their neighborhood (i.e. on the "other side"
            //  of the end-vertex) and so have sibling values associated with them.  In most
            //  cases the topology for an end-vertex will match and we can use it directly,
            //  but we must still check and retrieve as needed.
            //
            //  Indices for values corresponding to face-vertices are guaranteed to match,
            //  so we can use the child-vertex indices directly.
            //
            //  And by "directly", we always use getVertexValue(vertexIndex) to reference
            //  values in the "src" to account for the possible indirection that may exist at
            //  level 0 -- where there may be fewer values than vertices and an additional
            //  indirection is necessary.  We can use a vertex index directly for "dst" when
            //  it matches.
            //
            //  As with applying the mask to vertex data, in order to improve numerical
            //  precision, it's better to apply smaller weights first, so begin with the
            //  face-weights followed by the edge-weights and the vertex weight last.
            //
            Vtr::Index pVertValue = pVertValues[0];
            Vtr::Index cVertValue = cVertValues[0];

            dst[cVertValue].Clear();
            if (vMask.GetNumFaceWeights() > 0) {
                assert(vMask.AreFaceWeightsForFaceCenters());

                ConstIndexArray vFaces = parentLevel.getVertexFaces(vert);

                for (int i = 0; i < vFaces.size(); ++i) {

                    Vtr::Index cVertOfFace  = refinement.getFaceChildVertex(vFaces[i]);
                    assert(Vtr::IndexIsValid(cVertOfFace));

                    Vtr::Index cValueOfFace = childFVar.getVertexValueOffset(cVertOfFace);
                    dst[cVertValue].AddWithWeight(dst[cValueOfFace], vFaceWeights[i]);
                }
            }
            if (vMask.GetNumEdgeWeights() > 0) {

                parentFVar.getVertexEdgeValues(vert, vEdgeValues);

                for (int i = 0; i < vEdges.size(); ++i) {
                    dst[cVertValue].AddWithWeight(src[vEdgeValues[i]], vEdgeWeights[i]);
                }
            }
            dst[cVertValue].AddWithWeight(src[pVertValue], vVertWeight);
        } else {
            //
            //  Each FVar value associated with a vertex will be either a corner or a crease,
            //  or potentially in transition from corner to crease:
            //      - if the CHILD is a corner, there can be no transition so we have a corner
            //      - otherwise if the PARENT is a crease, both will be creases (no transition)
            //      - otherwise the parent must be a corner and the child a crease (transition)
            //
            Vtr::internal::FVarLevel::ConstValueTagArray pValueTags = parentFVar.getVertexValueTags(vert);
            Vtr::internal::FVarLevel::ConstValueTagArray cValueTags = childFVar.getVertexValueTags(cVert);

            for (int cSiblingIndex = 0; cSiblingIndex < cVertValues.size(); ++cSiblingIndex) {
                int pSiblingIndex = refineFVar.getChildValueParentSource(cVert, cSiblingIndex);
                assert(pSiblingIndex == cSiblingIndex);

                typedef Vtr::internal::FVarLevel::Sibling SiblingIntType;

                SiblingIntType cSibling = (SiblingIntType) cSiblingIndex;
                SiblingIntType pSibling = (SiblingIntType) pSiblingIndex;

                Vtr::Index pVertValue = pVertValues[pSibling];
                Vtr::Index cVertValue = cVertValues[cSibling];

                dst[cVertValue].Clear();
                if (isLinearFVar || cValueTags[cSibling].isCorner()) {
                    dst[cVertValue].AddWithWeight(src[pVertValue], 1.0f);
                } else {
                    //
                    //  We have either a crease or a transition from corner to crease -- in
                    //  either case, we need the end values for the full/fractional crease:
                    //
                    Index pEndValues[2];
                    parentFVar.getVertexCreaseEndValues(vert, pSibling, pEndValues);

                    Weight vWeight = 0.75f;
                    Weight eWeight = 0.125f;

                    //
                    //  If semi-sharp we need to apply fractional weighting -- if made sharp because
                    //  of the other sibling (dependent-sharp) use the fractional weight from that
                    //  other sibling (should only occur when there are 2):
                    //
                    if (pValueTags[pSibling].isSemiSharp()) {
                        Weight wCorner = pValueTags[pSibling].isDepSharp()
                                      ? refineFVar.getFractionalWeight(vert, !pSibling, cVert, !cSibling)
                                      : refineFVar.getFractionalWeight(vert, pSibling, cVert, cSibling);
                        Weight wCrease = 1.0f - wCorner;

                        vWeight = wCrease * 0.75f + wCorner;
                        eWeight = wCrease * 0.125f;
                    }
                    dst[cVertValue].AddWithWeight(src[pEndValues[0]], eWeight);
                    dst[cVertValue].AddWithWeight(src[pEndValues[1]], eWeight);
                    dst[cVertValue].AddWithWeight(src[pVertValue], vWeight);
                }
            }
        }
    }
}

template <typename REAL>
template <Sdc::SchemeType SCHEME, class T, class U, class U1, class U2>
inline void
PrimvarRefinerReal<REAL>::limit(T const & src, U & dstPos, U1 * dstTan1Ptr, U2 * dstTan2Ptr) const {

    Sdc::Scheme<SCHEME> scheme(_refiner._subdivOptions);

    Vtr::internal::Level const & level = _refiner.getLevel(_refiner.GetMaxLevel());

    int  maxWeightsPerMask = 1 + 2 * level.getMaxValence();
    bool hasTangents = (dstTan1Ptr && dstTan2Ptr);
    int  numMasks = 1 + (hasTangents ? 2 : 0);

    Vtr::internal::StackBuffer<Index,33> indexBuffer(maxWeightsPerMask);
    Vtr::internal::StackBuffer<Weight,99> weightBuffer(numMasks * maxWeightsPerMask);

    Weight * vPosWeights = weightBuffer,
           * ePosWeights = vPosWeights + 1,
           * fPosWeights = ePosWeights + level.getMaxValence();
    Weight * vTan1Weights = vPosWeights + maxWeightsPerMask,
           * eTan1Weights = ePosWeights + maxWeightsPerMask,
           * fTan1Weights = fPosWeights + maxWeightsPerMask;
    Weight * vTan2Weights = vTan1Weights + maxWeightsPerMask,
           * eTan2Weights = eTan1Weights + maxWeightsPerMask,
           * fTan2Weights = fTan1Weights + maxWeightsPerMask;

    Mask posMask( vPosWeights,  ePosWeights,  fPosWeights);
    Mask tan1Mask(vTan1Weights, eTan1Weights, fTan1Weights);
    Mask tan2Mask(vTan2Weights, eTan2Weights, fTan2Weights);

    //  This is a bit obscure -- assigning both parent and child as last level -- but
    //  this mask type was intended for another purpose.  Consider one for the limit:
    Vtr::internal::VertexInterface vHood(level, level);

    for (int vert = 0; vert < level.getNumVertices(); ++vert) {
        ConstIndexArray vEdges = level.getVertexEdges(vert);

        //  Incomplete vertices (present in sparse refinement) do not have their full
        //  topological neighborhood to determine a proper limit -- just leave the
        //  vertex at the refined location and continue to the next:
        if (level.getVertexTag(vert)._incomplete || (vEdges.size() == 0)) {
            dstPos[vert].Clear();
            dstPos[vert].AddWithWeight(src[vert], 1.0);
            if (hasTangents) {
                (*dstTan1Ptr)[vert].Clear();
                (*dstTan2Ptr)[vert].Clear();
            }
            continue;
        }

        //
        //  Limit masks require the subdivision Rule for the vertex in order to deal
        //  with infinitely sharp features correctly -- including boundaries and corners.
        //  The vertex neighborhood is minimally defined with vertex and edge counts.
        //
        Sdc::Crease::Rule vRule = level.getVertexRule(vert);

        //  This is a bit obscure -- child vertex index will be ignored here
        vHood.SetIndex(vert, vert);

        if (hasTangents) {
            scheme.ComputeVertexLimitMask(vHood, posMask, tan1Mask, tan2Mask, vRule);
        } else {
            scheme.ComputeVertexLimitMask(vHood, posMask, vRule);
        }

        //
        //  Gather the neighboring vertices of this vertex -- the vertices opposite its
        //  incident edges, and the opposite vertices of its incident faces:
        //
        Index * eIndices = indexBuffer;
        Index * fIndices = indexBuffer + vEdges.size();

        for (int i = 0; i < vEdges.size(); ++i) {
            ConstIndexArray eVerts = level.getEdgeVertices(vEdges[i]);

            eIndices[i] = (eVerts[0] == vert) ? eVerts[1] : eVerts[0];
        }
        if (posMask.GetNumFaceWeights() || (hasTangents && tan1Mask.GetNumFaceWeights())) {
            ConstIndexArray      vFaces = level.getVertexFaces(vert);
            ConstLocalIndexArray vInFace = level.getVertexFaceLocalIndices(vert);

            for (int i = 0; i < vFaces.size(); ++i) {
                ConstIndexArray fVerts = level.getFaceVertices(vFaces[i]);

                LocalIndex vOppInFace = (vInFace[i] + 2);
                if (vOppInFace >= fVerts.size()) vOppInFace -= (LocalIndex)fVerts.size();

                fIndices[i] = level.getFaceVertices(vFaces[i])[vOppInFace];
            }
        }

        //
        //  Combine the weights and indices for position and tangents.  As with applying
        //  refinement masks to vertex data, in order to improve numerical precision, it's
        //  better to apply smaller weights first, so begin with the face-weights followed
        //  by the edge-weights and the vertex weight last.
        //
        dstPos[vert].Clear();
        for (int i = 0; i < posMask.GetNumFaceWeights(); ++i) {
            dstPos[vert].AddWithWeight(src[fIndices[i]], fPosWeights[i]);
        }
        for (int i = 0; i < posMask.GetNumEdgeWeights(); ++i) {
            dstPos[vert].AddWithWeight(src[eIndices[i]], ePosWeights[i]);
        }
        dstPos[vert].AddWithWeight(src[vert], vPosWeights[0]);

        //
        //  Apply the tangent masks -- both will have the same number of weights and 
        //  indices (one tangent may be "padded" to accommodate the other), but these
        //  may differ from those of the position:
        //
        if (hasTangents) {
            assert(tan1Mask.GetNumFaceWeights() == tan2Mask.GetNumFaceWeights());
            assert(tan1Mask.GetNumEdgeWeights() == tan2Mask.GetNumEdgeWeights());

            U1 & dstTan1 = *dstTan1Ptr;
            U2 & dstTan2 = *dstTan2Ptr;

            dstTan1[vert].Clear();
            dstTan2[vert].Clear();
            for (int i = 0; i < tan1Mask.GetNumFaceWeights(); ++i) {
                dstTan1[vert].AddWithWeight(src[fIndices[i]], fTan1Weights[i]);
                dstTan2[vert].AddWithWeight(src[fIndices[i]], fTan2Weights[i]);
            }
            for (int i = 0; i < tan1Mask.GetNumEdgeWeights(); ++i) {
                dstTan1[vert].AddWithWeight(src[eIndices[i]], eTan1Weights[i]);
                dstTan2[vert].AddWithWeight(src[eIndices[i]], eTan2Weights[i]);
            }
            dstTan1[vert].AddWithWeight(src[vert], vTan1Weights[0]);
            dstTan2[vert].AddWithWeight(src[vert], vTan2Weights[0]);
        }
    }
}

template <typename REAL>
template <Sdc::SchemeType SCHEME, class T, class U>
inline void
PrimvarRefinerReal<REAL>::limitFVar(T const & src, U & dst, int channel) const {

    Sdc::Scheme<SCHEME> scheme(_refiner._subdivOptions);

    Vtr::internal::Level const &      level       = _refiner.getLevel(_refiner.GetMaxLevel());
    Vtr::internal::FVarLevel const &  fvarChannel = level.getFVarLevel(channel);

    int maxWeightsPerMask = 1 + 2 * level.getMaxValence();

    Vtr::internal::StackBuffer<Weight,33> weightBuffer(maxWeightsPerMask);
    Vtr::internal::StackBuffer<Index,16> vEdgeBuffer(level.getMaxValence());

    //  This is a bit obscure -- assign both parent and child as last level
    Vtr::internal::VertexInterface vHood(level, level);

    for (int vert = 0; vert < level.getNumVertices(); ++vert) {

        ConstIndexArray vEdges  = level.getVertexEdges(vert);
        ConstIndexArray vValues = fvarChannel.getVertexValues(vert);

        //  Incomplete vertices (present in sparse refinement) do not have their full
        //  topological neighborhood to determine a proper limit -- just leave the
        //  values (perhaps more than one per vertex) at the refined location.
        //
        //  The same can be done if the face-varying channel is purely linear.
        //
        bool isIncomplete = (level.getVertexTag(vert)._incomplete || (vEdges.size() == 0));
        if (isIncomplete || fvarChannel.isLinear()) {
            for (int i = 0; i < vValues.size(); ++i) {
                Vtr::Index vValue = vValues[i];

                dst[vValue].Clear();
                dst[vValue].AddWithWeight(src[vValue], 1.0f);
            }
            continue;
        }

        bool fvarVertMatchesVertex = fvarChannel.valueTopologyMatches(vValues[0]);
        if (fvarVertMatchesVertex) {

            //  Assign the mask weights to the common buffer and compute the mask:
            //
            Weight * vWeights = weightBuffer,
                   * eWeights = vWeights + 1,
                   * fWeights = eWeights + vEdges.size();

            Mask vMask(vWeights, eWeights, fWeights);

            vHood.SetIndex(vert, vert);

            scheme.ComputeVertexLimitMask(vHood, vMask, level.getVertexRule(vert));

            //
            //  Apply mask to corresponding FVar values for neighboring vertices:
            //
            Vtr::Index vValue = vValues[0];

            dst[vValue].Clear();
            if (vMask.GetNumFaceWeights() > 0) {
                assert(!vMask.AreFaceWeightsForFaceCenters());

                ConstIndexArray      vFaces = level.getVertexFaces(vert);
                ConstLocalIndexArray vInFace = level.getVertexFaceLocalIndices(vert);

                for (int i = 0; i < vFaces.size(); ++i) {
                    ConstIndexArray faceValues = fvarChannel.getFaceValues(vFaces[i]);
                    LocalIndex vOppInFace = vInFace[i] + 2;
                    if (vOppInFace >= faceValues.size()) vOppInFace -= faceValues.size();

                    Index vValueOppositeFace = faceValues[vOppInFace];

                    dst[vValue].AddWithWeight(src[vValueOppositeFace], fWeights[i]);
                }
            }
            if (vMask.GetNumEdgeWeights() > 0) {
                Index * vEdgeValues = vEdgeBuffer;
                fvarChannel.getVertexEdgeValues(vert, vEdgeValues);

                for (int i = 0; i < vEdges.size(); ++i) {
                    dst[vValue].AddWithWeight(src[vEdgeValues[i]], eWeights[i]);
                }
            }
            dst[vValue].AddWithWeight(src[vValue], vWeights[0]);
        } else {
            //
            //  Sibling FVar values associated with a vertex will be either a corner or a crease:
            //
            for (int i = 0; i < vValues.size(); ++i) {
                Vtr::Index vValue = vValues[i];

                dst[vValue].Clear();
                if (fvarChannel.getValueTag(vValue).isCorner()) {
                    dst[vValue].AddWithWeight(src[vValue], 1.0f);
                } else {
                    Index vEndValues[2];
                    fvarChannel.getVertexCreaseEndValues(vert, i, vEndValues);

                    dst[vValue].AddWithWeight(src[vEndValues[0]], 1.0f/6.0f);
                    dst[vValue].AddWithWeight(src[vEndValues[1]], 1.0f/6.0f);
                    dst[vValue].AddWithWeight(src[vValue], 2.0f/3.0f);
                }
            }
        }
    }
}

class PrimvarRefiner : public PrimvarRefinerReal<float> {
public:
    PrimvarRefiner(TopologyRefiner const & refiner)
        : PrimvarRefinerReal<float>(refiner) { }
};

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_FAR_PRIMVAR_REFINER_H */
