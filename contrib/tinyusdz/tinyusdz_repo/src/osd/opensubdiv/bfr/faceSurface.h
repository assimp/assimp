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

#ifndef OPENSUBDIV3_BFR_FACE_SURFACE_H
#define OPENSUBDIV3_BFR_FACE_SURFACE_H

#include "../version.h"

#include "../bfr/faceTopology.h"
#include "../bfr/faceVertex.h"
#include "../vtr/stackBuffer.h"
#include "../vtr/types.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  The FaceSurface class combines references to several other classes and
//  data to provide a complete description of the limit surface of a face.
//
//  It is a simple aggregate of four sets of data:
//
//      - an instance of FaceTopology with all topological information
//      - a set of FaceVertexSubsets for topological extent of each corner
//      - a set of indices associated with all vertices of FaceTopology
//      - a subset of the Sdc::Options that actually affects the surface
//
//  with a few additional members summarizing features of these.  The full
//  set of topology and corresponding indices are provided on construction
//  and the rest are initialized as member variables.
//
//  FaceSurfaces are constructed/initialized in two ways:
//
//      - for the vertex topology of a face, initialization requires:
//          - an instance of FaceTopology
//          - vertex indices associated with the FaceTopology (though in
//            some cases the vertex indices are not necessary)
//
//      - for the face-varying topology of a face:
//          - an instance of FaceSurface capturing the vertex topology
//          - face-varying indices associated with the vertex topology
//
//  Once initialized, other than a few simple queries, it serves solely
//  as a container to be passed to other classes to assemble into regular
//  or irregular surfaces.
//
class FaceSurface {
public:
    typedef FaceTopology::Index Index;

public:
    //  Constructors for vertex and face-varying surfaces:
    FaceSurface();
    FaceSurface(FaceTopology const & vtxTopology, Index const vtxIndices[]);
    FaceSurface(FaceSurface const  & vtxSurface,  Index const fvarIndices[]);
    ~FaceSurface() { }

    bool IsInitialized() const;
    void Initialize(FaceTopology const & vtxTopology, Index const vtxInds[]);
    void Initialize(FaceSurface const  & vtxSurface,  Index const fvarInds[]);

    //   Main public methods to distinguish surface and topology:
    bool IsRegular() const { return _isRegular; }

    bool FVarTopologyMatchesVertex() const { return _matchesVertex; }

    //  Debugging:
    void print(bool printVerts = false) const;

public:
    //  Public access to the main members:
    FaceTopology     const & GetTopology() const { return *_topology; }
    FaceVertexSubset const * GetSubsets()  const { return _corners; }
    Index            const * GetIndices()  const { return _indices; }
    MultiVertexTag           GetTag()      const { return _combinedTag; }

public:
    //  Additional public access to data used by builder classes:
    int GetFaceSize() const;
    int GetRegFaceSize() const;

    Sdc::SchemeType GetSdcScheme() const;
    Sdc::Options    GetSdcOptionsInEffect() const;
    Sdc::Options    GetSdcOptionsAsAssigned() const;

    FaceVertex       const & GetCornerTopology(int corner) const;
    FaceVertexSubset const & GetCornerSubset(int corner) const;

    int GetNumIndices() const;

private:
    //  Internal methods:
    void preInitialize(FaceTopology const & topology, Index const indices[]);
    void postInitialize();

    bool isRegular() const;
    void reviseSdcOptionsInEffect();

    //  Methods to apply specified interpolation options to the corners:
    void sharpenBySdcVtxBoundaryInterpolation(
                   FaceVertexSubset       * vtxSubsetPtr,
                   FaceVertex       const & cornerTopology) const;

    void sharpenBySdcFVarLinearInterpolation(
                   FaceVertexSubset       * fvarSubsetPtr,
                   Index const              fvarIndices[],
                   FaceVertexSubset const & vtxSubset,
                   FaceVertex       const & cornerTopology) const;

private:
    typedef Vtr::internal::StackBuffer<FaceVertexSubset,8,true> CornerArray;

    FaceTopology const * _topology;
    Index        const * _indices;
    CornerArray          _corners;

    //  Members reflecting the effective subset of topology and options:
    MultiVertexTag _combinedTag;
    Sdc::Options   _optionsInEffect;

    unsigned int _isFaceVarying : 1;
    unsigned int _matchesVertex : 1;
    unsigned int _isRegular     : 1;
};

//
//  Inline constructors:
//
inline
FaceSurface::FaceSurface() : _topology(0), _indices(0) {
}
inline
FaceSurface::FaceSurface(FaceTopology const & vtxTop, Index const vIndices[]) {
    Initialize(vtxTop, vIndices);
}
inline
FaceSurface::FaceSurface(FaceSurface const & vtxSurf, Index const fvIndices[]) {
    Initialize(vtxSurf, fvIndices);
}

//
//  Inline accessors:
//
inline bool
FaceSurface::IsInitialized() const {
    return _topology != 0;
}

inline int
FaceSurface::GetFaceSize() const {
    return _topology->GetFaceSize();
}
inline int
FaceSurface::GetRegFaceSize() const {
    return _topology->GetRegFaceSize();
}

inline Sdc::SchemeType
FaceSurface::GetSdcScheme() const {
    return _topology->_schemeType;
}
inline Sdc::Options
FaceSurface::GetSdcOptionsInEffect() const {
    return _optionsInEffect;
}
inline Sdc::Options
FaceSurface::GetSdcOptionsAsAssigned() const {
    return _topology->_schemeOptions;
}

inline FaceVertex const &
FaceSurface::GetCornerTopology(int corner) const {
    return _topology->GetTopology(corner);
}

inline FaceVertexSubset const & 
FaceSurface::GetCornerSubset(int corner) const {
    return _corners[corner];
}

inline int
FaceSurface::GetNumIndices() const {
    return _topology->GetNumFaceVertices();
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_FACE_SURFACE_H */
