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

#ifndef OPENSUBDIV3_BFR_VERTEX_DESCRIPTOR_H
#define OPENSUBDIV3_BFR_VERTEX_DESCRIPTOR_H

#include "../version.h"

#include "../vtr/stackBuffer.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

///
/// @brief Simple class used by subclasses of SurfaceFactory to describe a
///        vertex
///
/// VertexDescriptor is a simple class used by SurfaceFactory and its
/// subclasses to provide a complete topological description around the
/// vertex of a face, i.e. its valence, the sizes of its incident faces,
/// sharpness values, etc.
///
/// Instances are created and partially initialized by SurfaceFactory
/// before being passed to its subclasses to be fully populated. So
/// public construction is not available (or useful).
///
//
//  WIP - need to migrate some of these comments into Doxygen
//      - others will be moved to the external documentation
//
//  It is used by subclasses of SurfaceFactory to provide a complete
//  topological description for each vertex of a face, i.e. invoked via
//  the virtual method:
//
//      int populateFaceVertexDescriptor(Index baseFace,
//                                       int cornerVertex,
//                                       VertexDescriptor & v) const;
//
//  Assignment of the full topology can be involved in the presence of
//  irregular faces, non-manifold topology or creasing around a vertex, but
//  many cases will be simple.  For example, to specify a regular boundary
//  vertex of a Catmark mesh without any optional sharpness:
//
//      int  numIncidentFaces = 2;
//      bool vertexOnBoundary = true;
//
//      vd.Initialize(numIncidentFaces);
//          vd.SetManifold(true);
//          vd.SetBoundary(vertexOnBoundary);
//          vd.ClearIncidentFaceSizes();
//      vd.Finalize();
//
//  For a more general example, to assign a vertex of some valence whose
//  incident faces are of different sizes (e.g. required when triangles
//  appear around a vertex in an otherwise quad-dominant Catmark mesh):
//
//      int  numIncidentFaces = meshVertex.GetNumIncidentFaces();
//      bool vertexOnBoundary = meshVertex.IsBoundar();
//
//      vd.Initialize(numIncidentFaces);
//          vd.SetManifold(true);
//          vd.SetBoundary(vertexOnBoundary);
//
//          for (int i = 0; i < numIncidentFaces; ++i) {
//              vd.SetIncidentFaceSize(i, meshVertex.GetIncidentFaceSize(i));
//          }
//      vd.Finalize();
//
//  These examples specify the incident faces as forming a manifold ring
//  (or half-ring) around the vertex, i.e. they can be specified as a
//  continuous, connected sequence in counter-clockwise order (and also
//  without degeneracies).  In the case of a boundary vertex, the first
//  face must be on the leading edge of the boundary while the last is on
//  the trailing edge.  For an interior vertex, which face is specified
//  first does not matter (since the set is periodic).
//
//  In both cases, the location of the base face in this sequence -- the
//  face whose corner vertex is being described here -- must be specified
//  in the return value to populateFaceVertexDescriptor() (e.g. when a
//  boundary vertex has 3 incident faces, a return value of 0, 1 or 2
//  will indicate which is the base face).
//
//  The corresponding methods to specify mesh control vertex indices (or
//  face-varying indices) complete the specification of the neighborhood:
//
//      int getFaceCornerVertexIndices(Index baseFace, int cornerVertex,
//                                     Index vertexIndices[]) const;
//
//      int getFaceCornerFVarValueIndices(Index baseFace, int cornerVertex,
//                                        Index fvarValueIndices[],
//                                        int   fvarChannel) const;
//
//  and are invoked by the Factory when needed.
//
//  For each incident face, the indices for all vertices of that face are
//  to be specified (not the one-ring or some other subset).  These indices
//  must also be specified in an orientation relative to the vertex, i.e.
//  for a vertex A and an incident face with face-vertices that may be
//  stored internally as {D, C, A, B}, they must be specified with A first
//  as {A, B, C, D}.  This may seem a bit cumbersome, but it has clear
//  advantages when dealing with face-varying indices and unordered faces.
//
//  More compact ways of specifying vertex indices for ordered, manifold
//  cases may be worth exploring in future, but face-varying indices and
//  non-manifold (unordered) vertices will always require such a full set,
//  so both methods will need to co-exist.
//  
class VertexDescriptor {
public:
    //  The full declaration must be enclosed by calls to these methods:
    //
    //  Note that vertex valences or face sizes in excess of those defined
    //  in Bfr::Limits (typically 16-bits) are not valid.  When specifying
    //  values in excess of these limits, initialization will fail and/or
    //  the descriptor will be marked invalid and finalization will fail.
    //

    //@{
    /// @name Methods to begin and end specification
    ///
    /// Partially constructed instances are populated using a set of
    /// methods between calls to Initialize() and Finalize(). Both return
    /// false to indicate failure due to invalid input, or the instance
    /// can be inspected after each to determine if valid.
    /// 

    /// @brief Initialize specification with the number of incident faces
    bool Initialize(int numIncidentFaces);

    /// @brief Terminate the sequence of specifications
    bool Finalize();

    /// @brief Return if instance is valid
    bool IsValid() const;
    //@}

    //
    //  WIP - need to migrate these comments into Doxygen
    //
    //  Three groups of methods describe the topology around a vertex:
    //      - simple properties (vertex is a boundary, manifold, etc.)
    //      - sizes of incident faces (constant or size for each face)
    //      - sharpness of the vertex and its incident edges (optional)
    //

    //  Manifold and boundary conditions:
    //
    //  The manifold property is a strict condition but preferred for
    //  efficiency and is usually available from common connected mesh
    //  representations.  When declaring the topology as "manifold",
    //  the Factory assumes the following:
    //
    //      - all incident faces are "ordered" (counter-clockwise)
    //      - all incident faces are consistently oriented
    //      - all incident edges are non-degenerate
    //
    //  If not certain that all of these conditions are met, it is best
    //  to not declare manifold -- leaving the Factory to make sense of
    //  the set of incident faces from the face-vertex indices that are
    //  provided elsewhere.
    //  

    //@{
    /// @name Methods to specify topology
    ///
    /// Methods to specify the overall topology, the sizes of incident
    /// faces and any assigned sharpness values.

    /// @brief Declare the vertex neighborhood as manifold (ordered)
    void SetManifold(bool isManifold);

    /// @brief Declare the vertex neighborhood as being on a boundary
    void SetBoundary(bool isOnBoundary);

    /// @brief Assign the size of an incident face
    void SetIncidentFaceSize(int faceIndex, int faceSize);

    /// @brief Remove any assigned sizes of incident faces
    void ClearIncidentFaceSizes();

    /// @brief Assign sharpness to the vertex
    void SetVertexSharpness(float sharpness);

    /// @brief Remove any sharpness assigned to the vertex
    void ClearVertexSharpness();

    /// @brief Assign sharpness to the edge of a manifold neighborhood
    ///
    /// For use with a vertex declared manifold only, assigns a given
    /// sharpness to the indicated edge in the ordered sequence of edges
    /// around the vertex. In the case of a boundary vertex, the number
    /// of incident edges in this ordered sequence will exceed the number
    /// of incident faces by one.
    ///
    /// @param  edgeIndex     Index of the edge in the ordered sequence
    /// @param  edgeSharpness Sharpness to be assigned to the edge
    ///
    void SetManifoldEdgeSharpness(int edgeIndex, float edgeSharpness);

    /// @brief Assign sharpness to the edges of an incident face
    ///
    /// In all cases, sharpness can be assigned to edges by associating
    /// those edges with their incident faces. This method assigns sharpness
    /// to the two edges incident edges of an incident face. An alternative
    /// is available for the case of a manifold vertex.
    ///
    /// @param  faceIndex         Index of the incident face
    /// @param  leadingEdgeSharp  Sharpness to assign to the leading edge
    ///                           of the incident face, i.e. the edge of the
    ///                           face following the vertex.
    /// @param  trailingEdgeSharp Sharpness to assign to the trailing edge
    ///                           of the incident face, i.e. the edge of the
    ///                           face preceding the vertex.
    ///
    void SetIncidentFaceEdgeSharpness(int faceIndex, float leadingEdgeSharp,
                                                     float trailingEdgeSharp);

    /// @brief Remove any sharpness assigned to the incident edges
    void ClearEdgeSharpness();
    //@}

    //@{
    /// @name Methods to inspect topology to confirm assignment
    ///
    /// While the public interface is primarily intended for assignment,
    /// methods are available to inspect intermediate results.
    ///

    /// @brief Return if vertex neighborhood is manifold
    bool IsManifold() const;

    /// @brief Return if vertex neighborhood is on a boundary
    bool IsBoundary() const;

    /// @brief Return if the sizes of incident faces are assigned
    bool HasIncidentFaceSizes() const;

    /// @brief Return the size of an incident face
    int GetIncidentFaceSize(int faceIndex) const;

    /// @brief Return if sharpness was assigned to the vertex
    bool HasVertexSharpness() const;

    /// @brief Return the sharpness of the vertex
    float GetVertexSharpness() const;

    /// @brief Return if sharpness was assigned to the incident edges
    bool HasEdgeSharpness() const;

    /// @brief Return the sharpness assigned to a manifold edge
    float GetManifoldEdgeSharpness(int edgeIndex) const;

    /// @brief Return the sharpness assigned to edges of an incident face
    void GetIncidentFaceEdgeSharpness(int faceIndex,
            float * leadingEdgeSharp, float * trailingEdgeSharp) const;
    //@}

protected:
    /// @cond PROTECTED
    friend class FaceVertex;

    VertexDescriptor() { }
    ~VertexDescriptor() { }

    typedef Vtr::internal::StackBuffer<int,8,true>    IntBuffer;
    typedef Vtr::internal::StackBuffer<float,16,true> FloatBuffer;

    void initFaceSizes();
    void initEdgeSharpness();
    /// @endcond

protected:
    /// @cond PROTECTED
    //  Member variables assigned through the above interface:
    unsigned short _isValid       : 1;
    unsigned short _isInitialized : 1;
    unsigned short _isFinalized   : 1;

    unsigned short _isManifold : 1;
    unsigned short _isBoundary : 1;

    unsigned short _hasFaceSizes     : 1;
    unsigned short _hasEdgeSharpness : 1;

    short _numFaces;
    float _vertSharpness;

    FloatBuffer _faceEdgeSharpness;
    IntBuffer   _faceSizeOffsets;
    /// @endcond
};

//
//  Public inline methods for simple assignment:
//  
inline bool
VertexDescriptor::IsValid() const {
    return _isValid;
}

inline void
VertexDescriptor::SetManifold(bool isManifold) {
    _isManifold = isManifold;
}
inline bool
VertexDescriptor::IsManifold() const {
    return _isManifold;
}

inline void
VertexDescriptor::SetBoundary(bool isBoundary) {
    _isBoundary = isBoundary;
}
inline bool
VertexDescriptor::IsBoundary() const {
    return _isBoundary;
}

//
//  Public inline methods involving sizes of incident faces:
//  
inline bool
VertexDescriptor::HasIncidentFaceSizes() const {
    return _hasFaceSizes;
}
inline void
VertexDescriptor::ClearIncidentFaceSizes() {
    _hasFaceSizes = false;
}

inline void
VertexDescriptor::SetIncidentFaceSize(int incFaceIndex, int faceSize) {

    if (!_hasFaceSizes) initFaceSizes();

    _faceSizeOffsets[incFaceIndex] = faceSize;
}
inline int
VertexDescriptor::GetIncidentFaceSize(int incFaceIndex) const {

    return _isFinalized ?
          (_faceSizeOffsets[incFaceIndex+1] - _faceSizeOffsets[incFaceIndex]) :
           _faceSizeOffsets[incFaceIndex];
}

//
//  Public inline methods involving vertex sharpness:
//  
inline bool
VertexDescriptor::HasVertexSharpness() const {
    return _vertSharpness > 0.0f;
}
inline void
VertexDescriptor::ClearVertexSharpness() {
    _vertSharpness = 0.0f;
}

inline void
VertexDescriptor::SetVertexSharpness(float vertSharpness) {
    _vertSharpness = vertSharpness;
}
inline float
VertexDescriptor::GetVertexSharpness() const {
    return _vertSharpness;
}

//
//  Public inline methods involving vertex sharpness:
//  
inline bool
VertexDescriptor::HasEdgeSharpness() const {
    return _hasEdgeSharpness;
}
inline void
VertexDescriptor::ClearEdgeSharpness() {
    _hasEdgeSharpness = false;
}

inline void
VertexDescriptor::SetManifoldEdgeSharpness(int edgeIndex, float sharpness) {

    if (!_hasEdgeSharpness) initEdgeSharpness();

    //  Assign the leading edge of the face after the edge (even index):
    if (edgeIndex < _numFaces) {
        _faceEdgeSharpness[2*edgeIndex] = sharpness;
    }

    //  Assign the trailing edge of the face before the edge (odd index):
    if (edgeIndex > 0) {
        _faceEdgeSharpness[2*edgeIndex-1] = sharpness;
    } else if (!IsBoundary()) {
        _faceEdgeSharpness[2*_numFaces-1] = sharpness;
    }
}
inline float
VertexDescriptor::GetManifoldEdgeSharpness(int edgeIndex) const {

    //  All edges are first of the pair (even index) except last of boundary
    return _faceEdgeSharpness[2*edgeIndex - (edgeIndex == _numFaces)];
}

inline void
VertexDescriptor::SetIncidentFaceEdgeSharpness(int faceIndex,
        float leadingEdgeSharpness, float trailingEdgeSharpness) {

    if (!_hasEdgeSharpness) initEdgeSharpness();

    _faceEdgeSharpness[2*faceIndex  ] = leadingEdgeSharpness;
    _faceEdgeSharpness[2*faceIndex+1] = trailingEdgeSharpness;
}
inline void
VertexDescriptor::GetIncidentFaceEdgeSharpness(int faceIndex,
        float * leadingEdgeSharpness, float * trailingEdgeSharpness) const {

    *leadingEdgeSharpness  = _faceEdgeSharpness[2*faceIndex];
    *trailingEdgeSharpness = _faceEdgeSharpness[2*faceIndex+1];
}

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_VERTEX_DESCRIPTOR_H */
