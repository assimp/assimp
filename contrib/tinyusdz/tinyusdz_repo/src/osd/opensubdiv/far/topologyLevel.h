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
#ifndef OPENSUBDIV3_FAR_TOPOLOGY_LEVEL_H
#define OPENSUBDIV3_FAR_TOPOLOGY_LEVEL_H

#include "../version.h"

#include "../vtr/level.h"
#include "../vtr/refinement.h"
#include "../far/types.h"

#include <vector>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

///
/// \brief An interface for accessing data in a specific level of a refined topology hierarchy.
///
/// TopologyLevel provides an interface to data in a specific level of a topology hierarchy.
/// Instances of TopologyLevel are created and owned by a TopologyRefiner,
/// which will return const-references to them.  Such references are only valid during the
/// lifetime of the TopologyRefiner that created and returned them, and only for a given refinement,
/// i.e. if the TopologyRefiner is re-refined, any references to TopoologyLevels are invalidated.
///
class TopologyLevel {

public:
    //@{
    /// @name Methods to inspect the overall inventory of components:
    ///
    /// All three main component types are indexed locally within each level.  For
    /// some topological relationships -- notably face-vertices, which is often
    /// the only relationship of interest -- the total number of entries is also
    /// made available.
    ///

    /// \brief Return the number of vertices in this level
    int GetNumVertices() const     { return _level->getNumVertices(); }

    /// \brief Return the number of faces in this level
    int GetNumFaces() const        { return _level->getNumFaces(); }

    /// \brief Return the number of edges in this level
    int GetNumEdges() const        { return _level->getNumEdges(); }

    /// \brief Return the total number of face-vertices, i.e. the sum of all vertices for all faces
    int GetNumFaceVertices() const { return _level->getNumFaceVerticesTotal(); }
    //@}

    //@{
    /// @name Methods to inspect topological relationships for individual components:
    ///
    /// With three main component types (vertices, faces and edges), for each of the
    /// three components the TopologyLevel stores the incident/adjacent components of
    /// the other two types.  So there are six relationships available for immediate
    /// inspection.  All are accessed by methods that return an array of fixed size
    /// containing the indices of the incident components.
    ///
    /// For some of the relations, i.e. those for which the incident components are
    /// of higher order or 'contain' the component itself (e.g. a vertex has incident
    /// faces that contain it), an additional 'local index' is available that identifies
    /// the component within each of its neighbors.  For example, if vertex V is the k'th
    /// vertex in some face F, then when F occurs in the set of incident vertices of V,
    /// the local index corresponding to F will be k.  The ordering of local indices
    /// matches the ordering of the incident component to which it corresponds.
    //

    /// \brief Access the vertices incident a given face
    ConstIndexArray GetFaceVertices(Index f) const { return _level->getFaceVertices(f); }

    /// \brief Access the edges incident a given face
    ConstIndexArray GetFaceEdges(Index f) const    { return _level->getFaceEdges(f); }

    /// \brief Access the vertices incident a given edge
    ConstIndexArray GetEdgeVertices(Index e) const { return _level->getEdgeVertices(e); }

    /// \brief Access the faces incident a given edge
    ConstIndexArray GetEdgeFaces(Index e) const    { return _level->getEdgeFaces(e); }

    /// \brief Access the faces incident a given vertex
    ConstIndexArray GetVertexFaces(Index v) const  { return _level->getVertexFaces(v); }

    /// \brief Access the edges incident a given vertex
    ConstIndexArray GetVertexEdges(Index v) const  { return _level->getVertexEdges(v); }

    /// \brief Access the local indices of a vertex with respect to its incident faces
    ConstLocalIndexArray GetVertexFaceLocalIndices(Index v) const { return _level->getVertexFaceLocalIndices(v); }

    /// \brief Access the local indices of a vertex with respect to its incident edges
    ConstLocalIndexArray GetVertexEdgeLocalIndices(Index v) const { return _level->getVertexEdgeLocalIndices(v); }

    /// \brief Access the local indices of an edge with respect to its incident faces
    ConstLocalIndexArray GetEdgeFaceLocalIndices(Index e) const   { return _level->getEdgeFaceLocalIndices(e); }

    /// \brief Identify the edge matching the given vertex pair
    Index FindEdge(Index v0, Index v1) const { return _level->findEdge(v0, v1); }
    //@}

    //@{
    /// @name Methods to inspect other topological properties of individual components:
    ///

    /// \brief Return if the edge is non-manifold
    bool IsEdgeNonManifold(Index e) const   { return _level->isEdgeNonManifold(e); }

    /// \brief Return if the vertex is non-manifold
    bool IsVertexNonManifold(Index v) const { return _level->isVertexNonManifold(v); }

    /// \brief Return if the edge is a boundary (only one incident face)
    bool IsEdgeBoundary(Index e) const   { return _level->getEdgeTag(e)._boundary; }

    /// \brief Return if the vertex is on a boundary (at least one incident boundary edge)
    bool IsVertexBoundary(Index v) const { return _level->getVertexTag(v)._boundary; }

    /// \brief Return if the vertex is a corner (only one incident face)
    bool IsVertexCorner(Index v) const { return (_level->getNumVertexFaces(v) == 1); }

    /// \brief Return if the valence of the vertex is regular (must be manifold)
    ///
    /// Note that this test only determines if the valence of the vertex is regular
    /// with respect to the assigned subdivision scheme -- not if the neighborhood
    /// around the vertex is regular. The latter depends on a number of factors
    /// including the incident faces of the vertex (they must all be regular) and
    /// the presence of sharpness at the vertex itself or its incident edges.
    ///
    /// The regularity of the valence is a necessary but not a sufficient condition
    /// in determining the regularity of the neighborhood. For example, while the
    /// valence of an interior vertex may be regular, its neighborhood is not if the
    /// vertex was made infinitely sharp.  Conversely, a corner vertex is considered
    /// regular by its valence but its neighborhood is not if the vertex was not made
    /// infinitely sharp.
    ///
    /// Whether the valence of the vertex is regular is also a property that remains
    /// the same for the vertex in all subdivision levels. In contrast, the regularity
    /// of the region around the vertex may change as the presence of irregular faces
    /// or semi-sharp features is reduced by subdivision.
    ///
    bool IsVertexValenceRegular(Index v) const { return !_level->getVertexTag(v)._xordinary || IsVertexCorner(v); }
    //@}

    //@{
    /// @name Methods to inspect feature tags for individual components:
    ///
    /// While only a subset of components may have been tagged with features such
    /// as sharpness, all such features have a default value and so all components
    /// can be inspected.

    /// \brief Return the sharpness assigned a given edge
    float GetEdgeSharpness(Index e) const   { return _level->getEdgeSharpness(e); }

    /// \brief Return the sharpness assigned a given vertex
    float GetVertexSharpness(Index v) const { return _level->getVertexSharpness(v); }

    /// \brief Return if the edge is infinitely-sharp
    bool IsEdgeInfSharp(Index e) const { return _level->getEdgeTag(e)._infSharp; }

    /// \brief Return if the vertex is infinitely-sharp
    bool IsVertexInfSharp(Index v) const { return _level->getVertexTag(v)._infSharp; }

    /// \brief Return if the edge is semi-sharp
    bool IsEdgeSemiSharp(Index e) const { return _level->getEdgeTag(e)._semiSharp; }

    /// \brief Return if the vertex is semi-sharp
    bool IsVertexSemiSharp(Index v) const { return _level->getVertexTag(v)._semiSharp; }

    /// \brief Return if a given face has been tagged as a hole
    bool  IsFaceHole(Index f) const         { return _level->isFaceHole(f); }

    /// \brief Return the subdivision rule assigned a given vertex specific to this level
    Sdc::Crease::Rule GetVertexRule(Index v) const { return _level->getVertexRule(v); }
    //@}

    //@{
    /// @name Methods to inspect face-varying data:
    ///
    /// Face-varying data is organized into topologically independent channels,
    /// each with an integer identifier.  Access to face-varying data generally
    /// requires the specification of a channel, though with a single channel
    /// being a common situation the first/only channel will be assumed if
    /// unspecified.
    ///
    /// A face-varying channel is composed of a set of values that may be shared
    /// by faces meeting at a common vertex.  Just as there are sets of vertices
    /// that are associated with faces by index (ranging from 0 to
    /// num-vertices - 1), face-varying values are also referenced by index
    /// (ranging from 0 to num-values -1).
    ///
    /// The face-varying values associated with a face are accessed similarly to
    /// the way in which vertices associated with the face are accessed -- an
    /// array of fixed size containing the indices for each corner is provided
    /// for inspection, iteration, etc.
    ///
    /// When the face-varying topology around a vertex "matches", it has the
    /// same limit properties and so results in the same limit surface when
    /// collections of adjacent vertices match.  Like other references to
    /// "topology", this includes consideration of sharpness.  So it may be
    /// that face-varying values are assigned around a vertex on a boundary in
    /// a way that appears to match, but the face-varying interpolation option
    /// requires sharpening of that vertex in face-varying space -- the
    /// difference in the topology of the resulting limit surfaces leading to
    /// the query returning false for the match.  The edge case is simpler in
    /// that it only considers continuity across the edge, not the entire
    /// neighborhood around each end vertex.

    /// \brief Return the number of face-varying channels (should be same for all levels)
    int GetNumFVarChannels() const { return _level->getNumFVarChannels(); }

    /// \brief Return the total number of face-varying values in a particular channel
    /// (the upper bound of a face-varying value index)
    int GetNumFVarValues(int channel = 0) const { return _level->getNumFVarValues(channel); }

    /// \brief Access the face-varying values associated with a particular face
    ConstIndexArray GetFaceFVarValues(Index f, int channel = 0) const {
        return _level->getFaceFVarValues(f, channel);
    }

    /// \brief Return if face-varying topology around a vertex matches
    bool DoesVertexFVarTopologyMatch(Index v, int channel = 0) const {
        return _level->doesVertexFVarTopologyMatch(v, channel);
    }

    /// \brief Return if face-varying topology across the edge only matches
    bool DoesEdgeFVarTopologyMatch(Index e, int channel = 0) const {
        return _level->doesEdgeFVarTopologyMatch(e, channel);
    }

    /// \brief Return if face-varying topology around a face matches
    bool DoesFaceFVarTopologyMatch(Index f, int channel = 0) const {
        return _level->doesFaceFVarTopologyMatch(f, channel);
    }

    //@}

    //@{
    /// @name Methods to identify parent or child components in adjoining levels of refinement:

    /// \brief Access the child faces (in the next level) of a given face
    ConstIndexArray GetFaceChildFaces(Index f) const { return _refToChild->getFaceChildFaces(f); }

    /// \brief Access the child edges (in the next level) of a given face
    ConstIndexArray GetFaceChildEdges(Index f) const { return _refToChild->getFaceChildEdges(f); }

    /// \brief Access the child edges (in the next level) of a given edge
    ConstIndexArray GetEdgeChildEdges(Index e) const { return _refToChild->getEdgeChildEdges(e); }

    /// \brief Return the child vertex (in the next level) of a given face
    Index GetFaceChildVertex(  Index f) const { return _refToChild->getFaceChildVertex(f); }

    /// \brief Return the child vertex (in the next level) of a given edge
    Index GetEdgeChildVertex(  Index e) const { return _refToChild->getEdgeChildVertex(e); }

    /// \brief Return the child vertex (in the next level) of a given vertex
    Index GetVertexChildVertex(Index v) const { return _refToChild->getVertexChildVertex(v); }

    /// \brief Return the parent face (in the previous level) of a given face
    Index GetFaceParentFace(Index f) const { return _refToParent->getChildFaceParentFace(f); }
    //@}

    //@{
    /// @name Debugging aides:

    bool ValidateTopology() const { return _level->validateTopology(); }
    void PrintTopology(bool children = true) const { _level->print((children && _refToChild) ? _refToChild : 0); }
    //@}


private:
    friend class TopologyRefiner;

    Vtr::internal::Level const *      _level;
    Vtr::internal::Refinement const * _refToParent;
    Vtr::internal::Refinement const * _refToChild;

public:
    //  Not intended for public use, but required by std::vector, etc...
    TopologyLevel() { }
    ~TopologyLevel() { }
};

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_FAR_TOPOLOGY_LEVEL_H */
