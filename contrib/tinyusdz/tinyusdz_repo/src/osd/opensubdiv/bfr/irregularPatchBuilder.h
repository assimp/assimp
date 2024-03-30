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

#ifndef OPENSUBDIV3_IRREGULAR_PATCH_BUILDER_H
#define OPENSUBDIV3_IRREGULAR_PATCH_BUILDER_H

#include "../version.h"

#include "../bfr/faceSurface.h"
#include "../bfr/irregularPatchType.h"
#include "../vtr/stackBuffer.h"

#include <map>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  IrregularPatchBuilder takes a FaceSurface (that has been flagged as not
//  regular) and builds a representation for the limit surface it defines.
//
//  It is intended to hide the construction details and final representation
//  of the limit surface from its clients, i.e. the SurfaceFactory.  If the
//  preferred representation changes, or more than one is made available, it
//  should have minimal impact on its clients (ideally none).
//
//  WIP - the nature of the approximating options needs more work...
//      - we need some way of specifying the options of PatchTree in a
//        way that's more in line with the Factory's public interface
//
class IrregularPatchBuilder {
public:
    typedef FaceSurface::Index Index;

public:
    //  WIP - see note above
    struct Options {
        Options() : sharpLevel(6), smoothLevel(2), doublePrecision(false) { }

        int  sharpLevel;
        int  smoothLevel;
        bool doublePrecision;
    };

public:
    IrregularPatchBuilder(FaceSurface const & surfaceDescription,
                          Options     const & options = Options());
    ~IrregularPatchBuilder() { }

    //  Debugging:
    void print() const;

public:
    //  Methods to query the number and indices of control vertices:
    int GetNumControlVertices() const { return _numControlVerts; }

    int GatherControlVertexIndices(Index cvIndices[]) const;

public:
    //  Methods to build irregular patches:

    internal::IrregularPatchSharedPtr Build();

private:
    //  Private methods to assemble the control hull:

    //  A simple struct keeps track of the contribution of each corner to
    //  the collective control hull. These are first initialized and then
    //  used by methods to gather the various topological data that define
    //  the hull.
    //
    //  Note that vertices of the base face and the base face itself are
    //  not included as part of this inventory for each corner, e.g. the
    //  number of control vertices or control faces may be zero if the
    //  corner has no incident faces.
    //  
    struct CornerHull {
        void Clear() { std::memset(this, 0, sizeof(*this)); }

        int          numControlFaces;
        int          numControlVerts;
        int          nextControlVert;
        int          surfaceIndicesOffset;
        unsigned int singleSharedVert : 1;
        unsigned int singleSharedFace : 1;
    };

    void initializeControlHullInventory();

    //  Methods to access the control vertex indices:
    Index const * getSurfaceIndices() const;
    Index const * getBaseFaceIndices() const;
    Index const * getCornerIndices(int corner) const;
    Index const * getCornerFaceIndices(int corner, int face) const;

    //  Methods to gather topology defining the control hull:
    int gatherControlFaces(int faceSizes[], int faceVertices[]) const;

    int gatherControlVertexSharpness(int indices[], float sharpness[]) const;
    int gatherControlEdgeSharpness(  int indices[], float sharpness[]) const;

    //  Methods to identify face-verts for an individual control face:
    void getControlFaceVertices(int  faceVerts[], int numFaceVerts,
                                int  corner,      int nextPerimeterVert) const;
    void getControlFaceVertices(int  faceVerts[], int numFaceVerts,
                                int  corner,      int nextPerimeterVert,
                                bool lastFace) const;
    void getControlFaceVertices(int  faceVerts[], int numFaceVerts,
                                int  corner,      Index const srcVerts[]) const;

    //  Methods for dealing with the control vertex map:
    void initializeControlVertexMap();

    void addMeshControlVertex(Index faceVertIndex);
    void addMeshControlVertices(Index const faceVertIndices[], int faceSize);

    int   getLocalControlVertex(Index meshVertexIndex) const;
    Index getMeshControlVertex( int   localVertexIndex) const;

    //  Methods for dealing with potentially overlapping faces:
    bool controlFacesMayOverlap() const { return _controlFacesOverlap; }

    void removeDuplicateControlFaces(int faceSizes[], int faceVerts[],
                                     int * numFaces, int * numFaceVerts) const;
    void sharpenBoundaryControlEdges(int edgeIndices[], float edgeSharpness[],
                                     int * numSharpEdges) const;

private:
    //  Private members:
    FaceSurface const & _surface;
    Options             _options;

    //  Members defining the control hull of the surface -- some storing
    //  contributions to the control hull for each corner:
    typedef Vtr::internal::StackBuffer<CornerHull,8,true> CornerHullArray;

    int  _numControlVerts;
    int  _numControlFaces;
    int  _numControlFaceVerts;
    bool _controlFacesOverlap;
    bool _useControlVertMap;

    CornerHullArray _cornerHullInfo;

    std::map<Index,int> _controlVertMap;
    std::vector<Index>  _controlVerts;
};

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_IRREGULAR_PATCH_BUILDER_H */
