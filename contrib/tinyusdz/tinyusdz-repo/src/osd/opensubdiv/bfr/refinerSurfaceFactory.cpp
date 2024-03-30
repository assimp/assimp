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

#include "../bfr/refinerSurfaceFactory.h"
#include "../bfr/vertexDescriptor.h"
#include "../far/topologyRefiner.h"

#include <map>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

using Far::ConstIndexArray;
using Far::ConstLocalIndexArray;


//
//  Main constructor and destructor:
//
RefinerSurfaceFactoryBase::RefinerSurfaceFactoryBase(
    Far::TopologyRefiner const & mesh, Options const & factoryOptions) :
        SurfaceFactory(mesh.GetSchemeType(),
                       mesh.GetSchemeOptions(),
                       factoryOptions),
        _mesh(mesh),
        _numFaces(mesh.GetLevel(0).GetNumFaces()),
        _numFVarChannels(mesh.GetNumFVarChannels()) {

    //  Management of internal cache deferred to subclasses
}


//
//  Inline support method to provide a valid face-varying channel from
//  a given face-varying ID/handle used in the factory interface:
//
inline int
RefinerSurfaceFactoryBase::getFaceVaryingChannel(FVarID fvarID) const {

    return ((0 <= fvarID) && (fvarID < _numFVarChannels)) ? (int)fvarID : -1;
}


//
//  Virtual methods from the SurfaceFactoryMeshAdapter interface supporting
//  Surface construction and initialization:
//
//  Simple/trivial face queries:
//
bool
RefinerSurfaceFactoryBase::isFaceHole(Index face) const {

    return _mesh.HasHoles() && _mesh.getLevel(0).isFaceHole(face);
}

int
RefinerSurfaceFactoryBase::getFaceSize(Index baseFace) const {

    return _mesh.GetLevel(0).GetFaceVertices(baseFace).size();
}

//
//  Specifying vertex or face-varying indices for a face:
//
int
RefinerSurfaceFactoryBase::getFaceVertexIndices(Index baseFace,
        Index indices[]) const {

    ConstIndexArray fVerts = _mesh.GetLevel(0).GetFaceVertices(baseFace);

    std::memcpy(indices, &fVerts[0], fVerts.size() * sizeof(Index));
    return fVerts.size();
}

int
RefinerSurfaceFactoryBase::getFaceFVarValueIndices(Index baseFace,
        FVarID fvarID, Index indices[]) const {

    int fvarChannel = getFaceVaryingChannel(fvarID);
    if (fvarChannel < 0) return 0;

    ConstIndexArray fvarValues =
            _mesh.GetLevel(0).GetFaceFVarValues(baseFace, fvarChannel);

    std::memcpy(indices, &fvarValues[0], fvarValues.size() * sizeof(Index));
    return fvarValues.size();
}

//
//  Specifying the topology around a face-vertex:
//
int
RefinerSurfaceFactoryBase::populateFaceVertexDescriptor(
        Index baseFace, int cornerVertex,
        VertexDescriptor * vertexDescriptor) const {

    VertexDescriptor & vd = *vertexDescriptor;

    //
    //  Identify the vertex index for the specified corner of the face
    //  and topology information related to it:
    //
    Vtr::internal::Level const & baseLevel = _mesh.getLevel(0);

    Far::Index vIndex = baseLevel.getFaceVertices(baseFace)[cornerVertex];

    ConstIndexArray vFaces = baseLevel.getVertexFaces(vIndex);
    int             nFaces = vFaces.size();

    Vtr::internal::Level::VTag vTag = baseLevel.getVertexTag(vIndex);
    bool isManifold = !vTag._nonManifold;

    //
    //  Initialize, assign and finalize the vertex topology:
    //
    //  Note there is no need to check valence or face sizes with any
    //  max here as TopologyRefiner construction excludes extreme cases.
    //
    vd.Initialize(nFaces);
    {
        //  Assign ordering and boundary status:
        vd.SetManifold(isManifold);
        vd.SetBoundary(vTag._boundary);

        //  Assign face sizes if not all regular:
        if (vTag._incidIrregFace) {
            for (int i = 0; i < nFaces; ++i) {
                vd.SetIncidentFaceSize(i,
                            baseLevel.getFaceVertices(vFaces[i]).size());
            }
        }

        //  Assign vertex sharpness when present:
        if (vTag._semiSharp || vTag._infSharp) {
            vd.SetVertexSharpness(
                    baseLevel.getVertexSharpness(vIndex));
        }

        //  Assign edge sharpness when present:
        if (vTag._semiSharpEdges || vTag._infSharpEdges) {
            if (isManifold) {
                //  Can use manifold/ordered edge indices here:
                ConstIndexArray vEdges = baseLevel.getVertexEdges(vIndex);

                for (int i = 0; i < vEdges.size(); ++i) {
                    vd.SetManifoldEdgeSharpness(i,
                            baseLevel.getEdgeSharpness(vEdges[i]));
                }
            } else {
                //  Must use face-edges and identify next/prev edges in face:
                ConstLocalIndexArray vInFace =
                    baseLevel.getVertexFaceLocalIndices(vIndex);

                for (int i = 0; i < nFaces; ++i) {
                    ConstIndexArray fEdges = baseLevel.getFaceEdges(vFaces[i]);

                    int eLeading  = vInFace[i];
                    int eTrailing = (eLeading ? eLeading : fEdges.size()) - 1;

                    vd.SetIncidentFaceEdgeSharpness(i,
                            baseLevel.getEdgeSharpness(fEdges[eLeading]),
                            baseLevel.getEdgeSharpness(fEdges[eTrailing]));
                }
            }
        }
    }
    vd.Finalize();

    //
    //  Return the index of the base face around the vertex:
    //
    //  Remember that for some non-manifold cases the face may occur
    //  multiple times around this vertex, so make sure to identify the
    //  instance that matches the specified corner of the face.
    //
    if (isManifold) {
        return vFaces.FindIndex(baseFace);
    } else {
        ConstLocalIndexArray vInFace =
                baseLevel.getVertexFaceLocalIndices(vIndex);
        for (int i = 0; i < vFaces.size(); ++i) {
            if ((vFaces[i] == baseFace) && (vInFace[i] == cornerVertex)) {
                return i;
            }
        }
        assert("Cannot identify face-vertex around non-manifold vertex." == 0);
        return -1;
    }
}


//
//  Specifying vertex and face-varying indices around a face-vertex --
//  both virtual methods trivially use a common internal method to get
//  the indices for a particular vertex Index:
//
int
RefinerSurfaceFactoryBase::getFaceVertexPointIndices(
        Index baseFace, int cornerVertex,
        Index indices[], int vtxOrFVarChannel) const {

    Vtr::internal::Level const & baseLevel = _mesh.getLevel(0);

    Far::Index vIndex = baseLevel.getFaceVertices(baseFace)[cornerVertex];

    ConstIndexArray      vFaces  = baseLevel.getVertexFaces(vIndex);
    ConstLocalIndexArray vInFace = baseLevel.getVertexFaceLocalIndices(vIndex);

    int nIndices = 0;
    for (int i = 0; i < vFaces.size(); ++i) {
        ConstIndexArray srcIndices = (vtxOrFVarChannel < 0) ?
                baseLevel.getFaceVertices(vFaces[i]) :
                baseLevel.getFaceFVarValues(vFaces[i], vtxOrFVarChannel);

        int srcStart = vInFace[i];
        int srcCount = srcIndices.size();
        for (int j = srcStart; j < srcCount; ++j) {
            indices[nIndices++] = srcIndices[j];
        }
        for (int j = 0; j < srcStart; ++j) {
            indices[nIndices++] = srcIndices[j];
        }
    }
    return nIndices;
}

int
RefinerSurfaceFactoryBase::getFaceVertexIncidentFaceVertexIndices(
        Index baseFace, int cornerVertex,
        Index indices[]) const {

    return getFaceVertexPointIndices(baseFace, cornerVertex, indices, -1);
}

int
RefinerSurfaceFactoryBase::getFaceVertexIncidentFaceFVarValueIndices(
        Index baseFace, int corner,
        FVarID fvarID, Index indices[]) const {

    int fvarChannel = getFaceVaryingChannel(fvarID);
    if (fvarChannel < 0) return 0;

    return getFaceVertexPointIndices(baseFace, corner, indices, fvarChannel);
}

//
//  Optional SurfaceFactoryMeshAdapter methods for determining if a face has
//  purely regular topology, and retrieving its control point indices if so:
//
bool
RefinerSurfaceFactoryBase::getFaceNeighborhoodVertexIndicesIfRegular(
        Index baseFace, Index vtxIndices[]) const {

    //
    //  Get the composite tag for the corners of the face and reject some
    //  of the obvious irregular features first:
    //
    Vtr::internal::Level const & baseLevel = _mesh.getLevel(0);

    Vtr::internal::Level::VTag fTag = baseLevel.getFaceCompositeVTag(baseFace);

    if (fTag._xordinary || fTag._nonManifold
                        || fTag._incidIrregFace
                        || fTag._semiSharp || fTag._semiSharpEdges
                        || fTag._infIrregular) {
        return false;
    }

    //
    //  At this point, we have not rejected inf-sharp features, as they
    //  are intertwined with boundaries -- which may be regular.  Recall
    //  that both edges and vertices may have been explicitly sharpened
    //  by application of the boundary interpolation options, so we must
    //  exclude faces with any inf-sharp features added elsewhere.
    //
    //  Recall also that in the case of "boundary none", a face that does
    //  not have a limit surface will have been tagged as a hole.  So all
    //  faces here -- and all regular faces in general -- have a limit.
    //
    //  To determine regular patches with inf-sharp features, we can first
    //  trivially reject an interior face if it has any inf-sharp features.
    //  Otherwise, we have to inspect the vertices of boundary patches:
    //
    assert(!baseLevel.isFaceHole(baseFace));

    if (!fTag._boundary) {
        if (fTag._infSharp || fTag._infSharpEdges) {
            return false;
        }
    } else {
        ConstIndexArray fVerts = baseLevel.getFaceVertices(baseFace);
        for (int i = 0; i < fVerts.size(); ++i) {
            Far::Index vIndex = fVerts[i];
            Vtr::internal::Level::VTag vTag = baseLevel.getVertexTag(vIndex);

            if (!vTag._boundary) {
                if (vTag._rule != Sdc::Crease::RULE_SMOOTH) return false;
            } else if (baseLevel.getVertexFaces(vIndex).size() == 1) {
                if (vTag._rule != Sdc::Crease::RULE_CORNER) return false;
            } else {
                if (vTag._rule != Sdc::Crease::RULE_CREASE) return false;
            }
        }
    }

    //  Only regular cases make it this far -- assign indices if requested:
    if (vtxIndices) {
        getFacePatchPointIndices(baseFace, vtxIndices, -1);
    }
    return true;
}

bool
RefinerSurfaceFactoryBase::getFaceNeighborhoodFVarValueIndicesIfRegular(
        Index baseFace, FVarID fvarID, Index fvarIndices[]) const {

    int fvarChannel = getFaceVaryingChannel(fvarID);
    if (fvarChannel < 0) return false;

    //
    //  This method will only be invoked when the vertex topology is
    //  regular, so no need to confirm that here.
    //
    //  It is also recommended that this method only be used when the
    //  face-varying topology exactly matches the vertex topology, i.e.
    //  don't try to return a regular boundary patch that is a subset
    //  of a regular interior patch, as face-varying interpolation
    //  rules may affect that boundary patch (making it irregular).
    //
    Vtr::internal::Level const & baseLevel = _mesh.getLevel(0);

    bool isRegular = baseLevel.doesFaceFVarTopologyMatch(baseFace, fvarChannel);

    if (isRegular && fvarIndices) {
        getFacePatchPointIndices(baseFace, fvarIndices, fvarChannel);
    }
    return isRegular;
}


//
//  Supporting functions to extract regular patch points.
//
//  The two main functions here load the patch points from faces into
//  arrays for patches -- knowing that the mesh topology is regular.
//  Various methods to do all or part if this in different forms exist
//  in other places in the Far code, but they typically cater to more
//  general purposes (e.g. including rotations). These are written to
//  be as fast as possible for the purpose here.
//
namespace {
    //  Local less-verbose typedefs for Far indices and arrays:
    typedef Vtr::internal::Level Level;
    typedef Far::Index           Index;
    typedef ConstIndexArray      IArray;
    typedef ConstLocalIndexArray LIArray;

    //  Avoid repeated integer modulo N operations:
    inline int _mod3(int x) { return (x < 3) ? x : (x - 3); }
    inline int _mod4(int x) { return (x & 3); }
    inline int _mod6(int x) { return (x < 6) ? x : (x - 6); }

    //
    //  Retrieval of the 16-point patch for quad schemes:
    //
    template <typename POINT>
    int
    gatherPatchPoints4(Level const & level, Index face, IArray const & fVerts,
                       POINT P[], int fvar) {

        static int const pointsPerCorner[4][4] = { {  5,  4,  0,  1 },
                                                   {  6,  2,  3,  7 },
                                                   { 10, 11, 15, 14 },
                                                   {  9, 13, 12,  8 } };

        for (int i = 0; i < 4; ++i) {
            int const * corner = pointsPerCorner[i];

            int     vIndex  = fVerts[i];
            IArray  vFaces  = level.getVertexFaces(vIndex);
            LIArray vInFace = level.getVertexFaceLocalIndices(vIndex);

            if (vFaces.size() == 4) {
                int iOpposite = _mod4(vFaces.FindIndexIn4Tuple(face) + 2);

                Index  fj = vFaces[iOpposite];
                int    j  = vInFace[iOpposite];
                IArray FV = (fvar < 0) ? level.getFaceVertices(fj) :
                                         level.getFaceFVarValues(fj, fvar);

                P[corner[0]] = FV[j];
                P[corner[1]] = FV[_mod4(j + 1)];
                P[corner[2]] = FV[_mod4(j + 2)];
                P[corner[3]] = FV[_mod4(j + 3)];
            } else if (vFaces.size() == 1) {
                Index FVcorner = (fvar < 0) ? vIndex :
                    level.getFaceFVarValues(vFaces[0], fvar)[vInFace[0]];

                P[corner[0]] = FVcorner;
                P[corner[1]] = -1;
                P[corner[2]] = -1;
                P[corner[3]] = -1;
            } else if (vFaces[0] == face) {
                Index  f1 = vFaces[1];
                int    j1 = vInFace[1];
                IArray FV = (fvar < 0) ? level.getFaceVertices(f1) :
                                         level.getFaceFVarValues(f1, fvar);

                P[corner[0]] = FV[j1];
                P[corner[1]] = FV[_mod4(j1 + 3)];
                P[corner[2]] = -1;
                P[corner[3]] = -1;
            } else {
                Index  f0 = vFaces[0];
                int    j0 = vInFace[0];
                IArray FV = (fvar < 0) ? level.getFaceVertices(f0) :
                                         level.getFaceFVarValues(f0, fvar);

                P[corner[0]] = FV[j0];
                P[corner[1]] = -1;
                P[corner[2]] = -1;
                P[corner[3]] = FV[_mod4(j0 + 1)];
            }
        }
        return 16;
    }

    //
    //  Retrieval of the 12-point patch for triangular schemes:
    //
    template <typename POINT>
    int
    gatherPatchPoints3(Level const & level, Index face, IArray const & fVerts,
                       POINT P[], int fvar) {

        static int const pointsPerCorner[3][4] = { {  4,  3,  0,  1 },
                                                   {  5,  2,  6,  9 },
                                                   {  8, 11, 10,  7 } };

        for (int i = 0; i < 3; ++i) {
            int const * corner = pointsPerCorner[i];

            int     vIndex  = fVerts[i];
            IArray  vFaces  = level.getVertexFaces(vIndex);
            LIArray vInFace = level.getVertexFaceLocalIndices(vIndex);

            if (vFaces.size() == 6) {
                int iOpposite = _mod6(vFaces.FindIndex(face) + 3);

                Index  f0  = vFaces[iOpposite];
                int    j0  = vInFace[iOpposite];
                IArray FV0 = (fvar < 0) ? level.getFaceVertices(f0) :
                                          level.getFaceFVarValues(f0, fvar);
                Index  f1  = vFaces[_mod6(iOpposite + 1)];
                int    j1  = vInFace[_mod6(iOpposite + 1)];
                IArray FV1 = (fvar < 0) ? level.getFaceVertices(f1) :
                                          level.getFaceFVarValues(f1, fvar);

                P[corner[0]] = FV0[j0];
                P[corner[1]] = FV0[_mod3(j0 + 1)];
                P[corner[2]] = FV0[_mod3(j0 + 2)];
                P[corner[3]] = FV1[_mod3(j1 + 2)];
            } else if (vFaces.size() == 1) {
                Index FVcorner = (fvar < 0) ? vIndex :
                    level.getFaceFVarValues(vFaces[0], fvar)[vInFace[0]];

                P[corner[0]] = FVcorner;
                P[corner[1]] = -1;
                P[corner[2]] = -1;
                P[corner[3]] = -1;
            } else if (vFaces[0] == face) {
                Index  f2 = vFaces[2];
                int    j2 = vInFace[2];
                IArray FV = (fvar < 0) ? level.getFaceVertices(f2) :
                                         level.getFaceFVarValues(f2, fvar);

                P[corner[0]] = FV[j2];
                P[corner[1]] = FV[_mod3(j2 + 2)];
                P[corner[2]] = -1;
                P[corner[3]] = -1;
            } else if (vFaces[1] == face) {
                Index  f0 = vFaces[0];
                int    j0 = vInFace[0];
                IArray FV = (fvar < 0) ? level.getFaceVertices(f0) :
                                         level.getFaceFVarValues(f0, fvar);

                P[corner[0]] = FV[j0];
                P[corner[1]] = -1;
                P[corner[2]] = -1;
                P[corner[3]] = FV[_mod3(j0 + 1)];
            } else {  //  (vFaces[2] == face)
                Index  f0 = vFaces[0];
                int    j0 = vInFace[0];
                IArray FV = (fvar < 0) ? level.getFaceVertices(f0) :
                                         level.getFaceFVarValues(f0, fvar);

                P[corner[0]] = FV[j0];
                P[corner[1]] = -1;
                P[corner[2]] = FV[_mod3(j0 + 1)];
                P[corner[3]] = FV[_mod3(j0 + 2)];
            }
        }
        return 12;
    }
}

//
//  Private method to dispatch the above patch point retrieval functions:
//
int
RefinerSurfaceFactoryBase::getFacePatchPointIndices(Index baseFace,
        Index indices[], int vtxOrFVarChannel) const {

    Vtr::internal::Level const & baseLevel = _mesh.getLevel(0);

    ConstIndexArray baseFaceVerts = baseLevel.getFaceVertices(baseFace);

    if (baseFaceVerts.size() == 4) {
        return gatherPatchPoints4(baseLevel, baseFace, baseFaceVerts,
                                  indices, vtxOrFVarChannel);
    } else {
        return gatherPatchPoints3(baseLevel, baseFace, baseFaceVerts,
                                  indices, vtxOrFVarChannel);
    }
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
