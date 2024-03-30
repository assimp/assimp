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

#ifndef OPENSUBDIV3_BFR_FACE_TOPOLOGY_H
#define OPENSUBDIV3_BFR_FACE_TOPOLOGY_H

#include "../version.h"

#include "../bfr/faceVertex.h"
#include "../vtr/stackBuffer.h"
#include "../sdc/types.h"
#include "../sdc/options.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  The FaceTopology class describes the full topological neighborhood
//  around a base face of a mesh, which includes everything topologically
//  necessary to define the limit surface for that face.
//
//  It is used solely by the base SurfaceFactory class -- first partially
//  populated by subclasses before being inspected and augmented to help
//  assemble the limit surface for the face. Its members include some of
//  the members of the SurfaceFactory class (e.g. the subdivision scheme
//  and options) to make them more available for its purposes.
//
//  The primary component of FaceTopology is an array of instances of
//  the FaceVertex class (one for each vertex of the face), which is a
//  lightweight wrapper around the public VertexDescriptor class that is
//  populated by subclasses of SurfaceFactory.
//
//  FaceTopology is one of three key components in defining the limit
//  surface around a face.  The others are a set of FaceVertexSubsets (one
//  for each FaceVertex) that specify the subset of the neighborhood of
//  the corners of the face that actually do contribute to its surface,
//  and the indices associated with vertices that FaceTopology describes
//  (which become the control points of the limit surface).
//
class FaceTopology {
public:
    typedef FaceVertex::Index Index;

public:
    FaceTopology(Sdc::SchemeType schemeType,
                 Sdc::Options    schemeOptions);
    ~FaceTopology() { }

    void Initialize(int faceSize);
    void Finalize();

public:
    Sdc::SchemeType GetSchemeType()    const { return _schemeType; }
    Sdc::Options    GetSchemeOptions() const { return _schemeOptions; }

    int GetFaceSize()    const { return _faceSize; }
    int GetRegFaceSize() const { return _regFaceSize; }

    FaceVertex       & GetTopology(int i)       { return _corner[i]; }
    FaceVertex const & GetTopology(int i) const { return _corner[i]; }

    MultiVertexTag const GetTag() const { return _combinedTag; }

    int GetNumFaceVertices() const { return _numFaceVertsTotal; }
    int GetNumFaceVertices(int i) const{return _corner[i].GetNumFaceVertices();}

    //  Methods to test for and resolve unordered corners of the face:
    bool HasUnOrderedCorners() const { return GetTag().HasUnOrderedVertices(); }
    void ResolveUnOrderedCorners(Index const faceVertexIndices[]);

    //  Debugging...
    void print(Index const faceVertIndices[]) const;

public:
    Sdc::SchemeType _schemeType;
    Sdc::Options    _schemeOptions;

    int _faceSize;
    int _regFaceSize;
    int _numFaceVertsTotal;

    MultiVertexTag _combinedTag;

    unsigned short _isInitialized : 1;
    unsigned short _isFinalized   : 1;

    Vtr::internal::StackBuffer<FaceVertex,4> _corner;
};

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_FACE_TOPOLOGY_H */
