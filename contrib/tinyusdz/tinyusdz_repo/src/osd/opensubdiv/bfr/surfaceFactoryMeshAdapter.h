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

#ifndef OPENSUBDIV3_BFR_SURFACE_FACTORY_ADAPTER_H
#define OPENSUBDIV3_BFR_SURFACE_FACTORY_ADAPTER_H

#include "../version.h"

#include <cstdint>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

class VertexDescriptor;

///
/// @brief Abstract interface adapting SurfaceFactory to a connected mesh
///        representation
///
//  SurfaceFactoryMeshAdapter is an abstract class that defines the interface
//  through which subclasses of SurfaceFactory adapt to a connected mesh
//  representation. The interface defines methods that describe the mesh
//  topology and control indices in the neighborhood of a mesh -- from
//  which the SurfaceFactory to identifies an appropriate limit surface.
//
//  SurfaceFactoryMeshAdapter methods require a subclass to provide a complete
//  description of the topology around a base face, as well as indices
//  associated with it (both vertex and face-varying).  The intent here is
//  to keep the number of methods required to a minimum, and also to minimize
//  the number of invocations required by the factory.
//
//  With the need to support both linear and non-linear cases (for which
//  linear is trivial by comparison) and the limit surface for both vertex
//  and face-varying topologies, the result is a small set of methods
//  covering this matrix of functionality.
//
//  Since face-varying data may differ in topology from the vertex data --
//  with each set of face-varying data potentially having its own unique
//  topology -- sets of face-varying data are uniquely distinguished by an
//  associated integer (a face-varying ID).
//
class SurfaceFactoryMeshAdapter {
public:
    /// @brief Integer type representing a mesh index
    typedef int Index;

    /// @brief Type used to identify and specify face-varying primvars
    ///
    /// A face-varying ID is used to specify face-varying primvars for
    /// evaluation so that they can be identified by the subclass for
    /// the mesh.  It can be assigned as either a positive integer ID
    /// or pointer, with the subclass determining its interpretation.
    ///
    /// Often only one face-varying primvar is of interest, so a default
    /// can be assigned to the factory to avoid repeated specification.
    ///
    typedef std::intptr_t FVarID;

protected:
    /// @cond PROTECTED
    SurfaceFactoryMeshAdapter() { }
    virtual ~SurfaceFactoryMeshAdapter() { }
    /// @endcond

protected:
    //@{
    /// @name Methods to query simple face properties
    ///
    /// Methods to return simple properties associated with a face.
    ///

    /// @brief Returns if a face is a hole
    virtual bool isFaceHole(Index faceIndex) const = 0;

    /// @brief Returns the size of a face (number of vertices)
    virtual int getFaceSize(Index faceIndex) const = 0;
    //@}

    //@{
    /// @name Methods to gather indices for the face's vertices
    ///
    /// These methods gather indices associated with the vertices of a
    /// face, e.g. the indices of the vertices themselves, or the indices
    /// of face-varying values associated with the vertices. These are
    /// used to quickly deal with linear limit surfaces without any
    /// inspection of the neighboring topology.
    ///

    /// @brief Gather the indices of the face's vertices
    virtual int getFaceVertexIndices(Index faceIndex,
                    Index vertexIndices[]) const = 0;

    /// @brief Gather the face-varying indices of the face's vertices
    virtual int getFaceFVarValueIndices(Index faceIndex,
                    FVarID fvarID, Index fvarValueIndices[]) const = 0;
    //@}

protected:
    //@{
    /// @name Methods to identify the neighborhood of a face-vertex
    ///
    /// These methods identify the topology and associated indices for
    /// the complete set of incident faces surrounding a corner (or
    /// face-vertex) of a face.
    ///
    /// Methods here use "FaceVertex" in the name to emphasize that they
    /// require information for a particular corner vertex of the face.
    ///
    /// The topology around the face-vertex is described by populating a
    /// given instance of a simple VertexDescriptor class -- which fully
    /// describes the face-vertex, it incident faces and any sharpness
    /// assigned at or around the face-vertex.  (See the comments with
    /// the VertexDescriptor definition for more details.)
    ///
    /// Additional methods are then required to identify indices for the
    /// incident faces around a face-vertex. One method gathers the
    /// indices for control vertices of the mesh assigned to the incident
    /// faces (their VertexIndices), while the other gathers indices for
    /// a particular set of face-varying values assigned to them (their
    /// FVarValueIndices).
    ///
    /// Both methods expect the incident faces to be ordered consistent
    /// with the specification in VertexDescriptor, and all indices for
    /// all incident faces are required.
    ///
    /// The order of indices assigned to each face for these methods must
    /// also be specified relative to the face-vertex, rather than the
    /// way the face is defined.  For example, if a quad Q is defined by
    /// the four vertices {A, B, C, D}, when gathering the indices for Q
    /// as part of face-vertex C, the indices should be specified starting
    /// with C, i.e. as {C, D, A, B}.  Ordering indices this way makes it
    /// much easier for the factory to identify when face-varying topology
    /// differs from the vertex topology, and both the face-varying and
    /// vertex indices are ordered this way for consistency.
    ///
 
    /// @brief Describe the topology of incident faces around a face-vertex
    virtual int populateFaceVertexDescriptor(
                    Index faceIndex, int faceVertex,
                    VertexDescriptor * vertexDescriptor) const = 0;
 
    /// @brief Gather vertex indices of incident faces around a face-vertex
    virtual int getFaceVertexIncidentFaceVertexIndices(
                    Index faceIndex, int faceVertex,
                    Index vertexIndices[]) const = 0;

    /// @brief Gather face-varying indices of incident faces around a
    ///        face-vertex
    virtual int getFaceVertexIncidentFaceFVarValueIndices(
                    Index faceIndex, int faceVertex,
                    FVarID fvarID, Index fvarValueIndices[]) const = 0;
    //@}

protected:
    //@{
    /// @name Optional methods for purely regular topology
    ///
    /// Optional methods for advanced use to accelerate the case of
    /// purely regular topology around a face.
    ///
    /// For cases when a mesh can quickly determine if the neighborhood
    /// around a faces is purely regular, these methods can be used to
    /// quickly identify the control point indices for the corresponding
    /// regular patch defining its limit surface. In doing so, the more
    /// tedious topological assembly requiring information about each
    /// face-vertex can be avoided.
    ///
    /// The indices returned must be ordered according to the regular
    /// patch type corresponding to the subdivision scheme of the mesh.
    /// Boundary vertices are allowed and indicated by an Index of -1.
    ///
    /// The face-varying version will only be invoked if the vertex
    /// version is purely regular, in which case, the face-varying
    /// topology is expected to be similar.
    ///
    /// Note that these methods allow the caller (the SurfaceFactory) to
    /// pass nullptr (0) for the index arrays -- in which case only the
    /// return value should be provided.
    ///
    virtual bool getFaceNeighborhoodVertexIndicesIfRegular(
            Index faceIndex, Index vertexIndices[]) const;

    virtual bool getFaceNeighborhoodFVarValueIndicesIfRegular(
            Index faceIndex, FVarID fvarID, Index fvarValueIndices[]) const;
    //@}

private:
    //  No private members
};

//
//  Inline defaults for optional methods:
//
inline bool
SurfaceFactoryMeshAdapter::getFaceNeighborhoodVertexIndicesIfRegular(
        Index, Index[]) const {
    return false;
}

inline bool
SurfaceFactoryMeshAdapter::getFaceNeighborhoodFVarValueIndicesIfRegular(
        Index, FVarID, Index[]) const {
    return false;
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_SURFACE_FACTORY_ADAPTER_H */
