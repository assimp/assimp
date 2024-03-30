//
//   Copyright 2014 DreamWorks Animation LLC.
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
#ifndef OPENSUBDIV3_FAR_TOPOLOGY_REFINER_FACTORY_H
#define OPENSUBDIV3_FAR_TOPOLOGY_REFINER_FACTORY_H

#include "../version.h"

#include "../far/topologyRefiner.h"
#include "../far/error.h"

#include <cassert>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

///\brief Private base class of Factories for constructing TopologyRefiners
///
/// TopologyRefinerFactoryBase is the base class for subclasses that are intended to
/// construct TopologyRefiners directly from meshes in their native representations.
/// The subclasses are parameterized by the mesh type \<class MESH\> and are expected
/// to inherit the details related to assembly and validation provided here that are
/// independent of the subclass' mesh type.
//
class TopologyRefinerFactoryBase {
protected:

    //
    //  Protected methods invoked by the subclass template to verify and process each
    //  stage of construction implemented by the subclass:
    //
    typedef Vtr::internal::Level::ValidationCallback TopologyCallback;

    static bool prepareComponentTopologySizing(TopologyRefiner& refiner);
    static bool prepareComponentTopologyAssignment(TopologyRefiner& refiner, bool fullValidation,
                                                   TopologyCallback callback, void const * callbackData);
    static bool prepareComponentTagsAndSharpness(TopologyRefiner& refiner);
    static bool prepareFaceVaryingChannels(TopologyRefiner& refiner);
};


///\brief Factory for constructing TopologyRefiners from specific mesh classes.
///
/// TopologyRefinerFactory<MESH> is the factory class template to convert an instance of
/// TopologyRefiner from an arbitrary mesh class.  While a class template, the implementation
/// is not (cannot) be complete, so specialization of a few methods is required (it is a
/// stateless factory, so no instance and only static methods).
///
/// This template provides both the interface and high level assembly for the construction
/// of the TopologyRefiner instance.  The high level construction executes a specific set
/// of operations to convert the client's MESH into TopologyRefiner.  This set of operations
/// combines methods independent of MESH from the base class with those specialized here for
/// class MESH.
///
template <class MESH>
class TopologyRefinerFactory : public TopologyRefinerFactoryBase {

public:

    /// \brief Options related to the construction of each TopologyRefiner.
    ///
    struct Options {

        Options(Sdc::SchemeType sdcType = Sdc::SCHEME_CATMARK, Sdc::Options sdcOptions = Sdc::Options()) :
            schemeType(sdcType),
            schemeOptions(sdcOptions),
            validateFullTopology(false) { }

        Sdc::SchemeType schemeType;             ///< The subdivision scheme type identifier
        Sdc::Options    schemeOptions;          ///< The full set of options for the scheme,
                                                ///< e.g. boundary interpolation rules...
        unsigned int validateFullTopology : 1;  ///< Apply more extensive validation of
                                                ///< the constructed topology -- intended
                                                ///< for debugging.
    };

    /// \brief Instantiates a TopologyRefiner from client-provided topological
    ///        representation.
    ///
    ///  If only the face-vertices topological relationships are specified
    ///  with this factory, edge relationships have to be inferred, which
    ///  requires additional processing. If the client topological rep can
    ///  provide this information, it is highly recommended to do so.
    ///
    /// @param mesh       Client's topological representation (or a converter)
    //
    /// @param options    Options controlling the creation of the TopologyRefiner
    ///
    /// @return           A new instance of TopologyRefiner or 0 for failure
    ///
    static TopologyRefiner* Create(MESH const& mesh, Options options = Options());

    /// \brief Instantiates a TopologyRefiner from the base level of an
    ///        existing instance.
    ///
    ///  This allows lightweight copies of the same topology to be refined
    ///  differently for each new instance.  As with other classes that refer
    ///  to an existing TopologyRefiner, it must generally exist for the entire
    ///  lifetime of the new instance.  In this case, the base level of the
    ///  original instance must be preserved.
    ///
    /// @param baseLevel  An existing TopologyRefiner to share base level.
    ///
    /// @return           A new instance of TopologyRefiner or 0 for failure
    ///
    static TopologyRefiner* Create(TopologyRefiner const & baseLevel);

protected:
    typedef Vtr::internal::Level::TopologyError TopologyError;

    //@{
    ///  @name  Methods to be provided to complete assembly of the TopologyRefiner
    ///
    ///
    ///  These methods are to be specialized to implement all details specific to
    ///  class MESH required to convert MESH data to TopologyRefiner.  Note that
    ///  some of these *must* be specialized in order to complete construction while
    ///  some are optional.
    ///
    ///  There are two minimal construction requirements (to specify the size and
    ///  content of all topology relations) and three optional (to specify feature
    ///  tags, face-varying data, and runtime validation and error reporting).
    ///
    ///  See comments in the generic stubs, the factory for Far::TopologyDescriptor
    ///  or the tutorials for more details on writing these.
    ///

    /// \brief  Specify the number of vertices, faces, face-vertices, etc.
    static bool resizeComponentTopology(TopologyRefiner& newRefiner, MESH const& mesh);

    /// \brief  Specify the relationships between vertices, faces, etc. ie the
    /// face-vertices, vertex-faces, edge-vertices, etc.
    static bool assignComponentTopology(TopologyRefiner& newRefiner, MESH const& mesh);

    /// \brief  (Optional) Specify edge or vertex sharpness or face holes
    static bool assignComponentTags(TopologyRefiner& newRefiner, MESH const& mesh);

    /// \brief  (Optional) Specify face-varying data per face
    static bool assignFaceVaryingTopology(TopologyRefiner& newRefiner, MESH const& mesh);

    /// \brief  (Optional) Control run-time topology validation and error reporting
    static void reportInvalidTopology(TopologyError errCode, char const * msg, MESH const& mesh);

    //@}

protected:
    //@{
    ///  @name  Base level assembly methods to be used within resizeComponentTopology()
    ///
    /// \brief These methods specify sizes of various quantities, e.g. the number of
    ///  vertices, faces, face-vertices, etc.  The number of the primary components
    ///  (vertices, faces and edges) should be specified prior to anything else that
    ///  references them (e.g. we need to know the number of faces before specifying
    ///  the vertices for that face.
    ///
    ///  If a full boundary representation with all neighborhood information is not
    ///  available, e.g. faces and vertices are available but not edges, only the
    ///  face-vertices should be specified.  The remaining topological relationships
    ///  will be constructed later in the assembly (though at greater cost than if
    ///  specified directly).
    ///
    ///  The sizes for topological relationships between individual components should be
    ///  specified in order, i.e. the number of face-vertices for each successive face.
    ///

    /// \brief Specify the number of vertices to be accommodated
    static void setNumBaseVertices(TopologyRefiner & newRefiner, int count);

    /// \brief Specify the number of faces to be accommodated
    static void setNumBaseFaces(TopologyRefiner & newRefiner, int count);

    /// \brief Specify the number of edges to be accommodated
    static void setNumBaseEdges(TopologyRefiner & newRefiner, int count);

    /// \brief Specify the number of vertices incident each face
    static void setNumBaseFaceVertices(TopologyRefiner & newRefiner, Index f, int count);

    /// \brief Specify the number of faces incident each edge
    static void setNumBaseEdgeFaces(TopologyRefiner & newRefiner, Index e, int count);

    /// \brief Specify the number of faces incident each vertex
    static void setNumBaseVertexFaces(TopologyRefiner & newRefiner, Index v, int count);

    /// \brief Specify the number of edges incident each vertex
    static void setNumBaseVertexEdges(TopologyRefiner & newRefiner, Index v, int count);

    static int getNumBaseVertices(TopologyRefiner const & newRefiner);
    static int getNumBaseFaces(TopologyRefiner const & newRefiner);
    static int getNumBaseEdges(TopologyRefiner const & newRefiner);
    //@}

    //@{
    ///  @name  Base level assembly methods to be used within assignComponentTopology()
    ///
    /// \brief These methods populate relationships between components -- in much the
    /// same manner as they are inspected once the TopologyRefiner is completed.
    ///
    /// An array of fixed size is returned from these methods and its entries are to be
    /// populated with the appropriate indices for its neighbors.  At minimum, the
    /// vertices for each face must be specified.  As noted previously, the remaining
    /// relationships will be constructed as needed.
    ///
    /// The ordering of entries in these arrays is important -- they are expected to
    /// be ordered counter-clockwise for a right-hand orientation.
    ///
    /// Non-manifold components must be explicitly tagged as such and they do not
    /// require the ordering expected of manifold components.  Special consideration
    /// must also be given to certain non-manifold situations, e.g. the same edge
    /// cannot appear twice in a face, and a degenerate edge (same vertex at both
    /// ends) can only have one incident face.  Such considerations are typically
    /// achievable by creating multiple instances of an edge.  So while there will
    /// always be a one-to-one correspondence between vertices and faces, the same
    /// is not guaranteed of edges in certain non-manifold circumstances.
    ///

    /// \brief Assign the vertices incident each face
    static IndexArray getBaseFaceVertices(TopologyRefiner & newRefiner, Index f);

    /// \brief Assign the edges incident each face
    static IndexArray getBaseFaceEdges(TopologyRefiner & newRefiner,    Index f);

    /// \brief Assign the vertices incident each edge
    static IndexArray getBaseEdgeVertices(TopologyRefiner & newRefiner, Index e);

    /// \brief Assign the faces incident each edge
    static IndexArray getBaseEdgeFaces(TopologyRefiner & newRefiner,    Index e);

    /// \brief Assign the faces incident each vertex
    static IndexArray getBaseVertexFaces(TopologyRefiner & newRefiner,  Index v);

    /// \brief Assign the edges incident each vertex
    static IndexArray getBaseVertexEdges(TopologyRefiner & newRefiner,  Index v);

    /// \brief Assign the local indices of a vertex within each of its incident faces
    static LocalIndexArray getBaseVertexFaceLocalIndices(TopologyRefiner & newRefiner, Index v);
    /// \brief Assign the local indices of a vertex within each of its incident edges
    static LocalIndexArray getBaseVertexEdgeLocalIndices(TopologyRefiner & newRefiner, Index v);
    /// \brief Assign the local indices of an edge within each of its incident faces
    static LocalIndexArray getBaseEdgeFaceLocalIndices(TopologyRefiner & newRefiner, Index e);

    /// \brief Determine all local indices by inspection (only for pure manifold meshes)
    static void populateBaseLocalIndices(TopologyRefiner & newRefiner);

    /// \brief Tag an edge as non-manifold
    static void setBaseEdgeNonManifold(TopologyRefiner & newRefiner, Index e, bool b);

    /// \brief Tag a vertex as non-manifold
    static void setBaseVertexNonManifold(TopologyRefiner & newRefiner, Index v, bool b);
    //@}

    //@{
    ///  @name  Base level assembly methods to be used within assignComponentTags()
    ///
    /// These methods are used to assign edge or vertex sharpness, for tagging faces
    /// as holes, etc.  Unlike topological assignment, only those components that
    /// possess a feature of interest need be explicitly assigned.
    ///
    /// Since topological construction is largely complete by this point, a method is
    /// available to identify an edge for sharpness assignment given a pair of vertices.
    ///

    /// \brief Identify an edge to be assigned a sharpness value given a vertex pair
    static Index findBaseEdge(TopologyRefiner const & newRefiner, Index v0, Index v1);

    /// \brief Assign a sharpness value to a given edge
    static void setBaseEdgeSharpness(TopologyRefiner & newRefiner, Index e, float sharpness);

    /// \brief Assign a sharpness value to a given vertex
    static void setBaseVertexSharpness(TopologyRefiner & newRefiner, Index v, float sharpness);

    /// \brief Tag a face as a hole
    static void setBaseFaceHole(TopologyRefiner & newRefiner, Index f, bool isHole);
    //@}

    //@{
    ///  @name  Base level assembly methods to be used within assignFaceVaryingTopology()
    ///
    /// Face-varying data is assigned to faces in much the same way as face-vertex
    /// topology is assigned -- indices for face-varying values are assigned to the
    /// corners of each face just as indices for vertices were assigned.
    ///
    /// Independent sets of face-varying data are stored in channels.  The identifier
    /// of each channel (an integer) is expected whenever referring to face-varying
    /// data in any form.
    ///

    /// \brief  Create a new face-varying channel with the given number of values
    static int createBaseFVarChannel(TopologyRefiner & newRefiner, int numValues);

    /// \brief  Create a new face-varying channel with the given number of values and independent interpolation options
    static int createBaseFVarChannel(TopologyRefiner & newRefiner, int numValues, Sdc::Options const& fvarOptions);

    /// \brief Assign the face-varying values for the corners of each face
    static IndexArray getBaseFaceFVarValues(TopologyRefiner & newRefiner, Index face, int channel = 0);

    //@}

protected:
    //
    //  Not to be specialized:
    //
    static bool populateBaseLevel(TopologyRefiner& refiner, MESH const& mesh, Options options);

private:
    //
    //  An oversight in the interfaces of the error reporting function between the factory
    //  class and the Vtr::Level requires this adapter function to avoid warnings.
    //
    //  The static class method requires a reference as the MESH argument, but the interface
    //  for Vtr::Level requires a pointer (void*). So this adapter with a MESH* argument is
    //  used to effectively cast the function pointer required by Vtr::Level error reporting:
    //
    static void reportInvalidTopologyAdapter(TopologyError errCode, char const * msg, MESH const * mesh) {
        reportInvalidTopology(errCode, msg, *mesh);
    }
};


//
//  Generic implementations:
//
template <class MESH>
TopologyRefiner*
TopologyRefinerFactory<MESH>::Create(MESH const& mesh, Options options) {

    TopologyRefiner * refiner = new TopologyRefiner(options.schemeType, options.schemeOptions);

    if (! populateBaseLevel(*refiner, mesh, options)) {
        delete refiner;
        return 0;
    }

    //  Eventually want to move the Refiner's inventory initialization here.  Currently it
    //  is handled after topology assignment, but if the inventory is to include additional
    //  features (e.g. holes, etc.) it is better off deferred to here.

    return refiner;
}

template <class MESH>
TopologyRefiner*
TopologyRefinerFactory<MESH>::Create(TopologyRefiner const & source) {

    return new TopologyRefiner(source);
}

template <class MESH>
bool
TopologyRefinerFactory<MESH>::populateBaseLevel(TopologyRefiner& refiner, MESH const& mesh, Options options) {

    //
    //  Construction of a specialized topology refiner involves four steps, each of which
    //  involves a method specialized for MESH followed by one that takes an action in
    //  response to it or in preparation for the next step.
    //
    //  Both the specialized methods and those that follow them may find fault in the
    //  construction and trigger failure at any time:
    //

    //
    //  Sizing of the topology -- this is a required specialization for MESH.  This defines
    //  an inventory of all components and their relations that is used to allocate buffers
    //  to be efficiently populated in the subsequent topology assignment step.
    //
    if (! resizeComponentTopology(refiner, mesh)) return false;
    if (! prepareComponentTopologySizing(refiner)) return false;

    //
    //  Assignment of the topology -- this is a required specialization for MESH.  If edges
    //  are specified, all other topological relations are expected to be defined for them.
    //  Otherwise edges and remaining topology will be completed from the face-vertices:
    //
    bool             validate = options.validateFullTopology;
    TopologyCallback callback = reinterpret_cast<TopologyCallback>(reportInvalidTopologyAdapter);
    void const *     userData = &mesh;
        
    if (! assignComponentTopology(refiner, mesh)) return false;
    if (! prepareComponentTopologyAssignment(refiner, validate, callback, userData)) return false;

    //
    //  User assigned and internal tagging of components -- an optional specialization for
    //  MESH.  Allows the specification of sharpness values, holes, etc.
    //
    if (! assignComponentTags(refiner, mesh)) return false;
    if (! prepareComponentTagsAndSharpness(refiner)) return false;

    //
    //  Defining channels of face-varying primvar data -- an optional specialization for MESH.
    //
    if (! assignFaceVaryingTopology(refiner, mesh)) return false;
    if (! prepareFaceVaryingChannels(refiner)) return false;

    return true;
}

template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setNumBaseFaces(TopologyRefiner & newRefiner, int count) {
    newRefiner._levels[0]->resizeFaces(count);
}
template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setNumBaseEdges(TopologyRefiner & newRefiner, int count) {
    newRefiner._levels[0]->resizeEdges(count);
}
template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setNumBaseVertices(TopologyRefiner & newRefiner, int count) {
    newRefiner._levels[0]->resizeVertices(count);
}

template <class MESH>
inline int
TopologyRefinerFactory<MESH>::getNumBaseFaces(TopologyRefiner const & newRefiner) {
    return newRefiner._levels[0]->getNumFaces();
}
template <class MESH>
inline int
TopologyRefinerFactory<MESH>::getNumBaseEdges(TopologyRefiner const & newRefiner) {
    return newRefiner._levels[0]->getNumEdges();
}
template <class MESH>
inline int
TopologyRefinerFactory<MESH>::getNumBaseVertices(TopologyRefiner const & newRefiner) {
    return newRefiner._levels[0]->getNumVertices();
}

template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setNumBaseFaceVertices(TopologyRefiner & newRefiner, Index f, int count) {
    newRefiner._levels[0]->resizeFaceVertices(f, count);
    newRefiner._hasIrregFaces = newRefiner._hasIrregFaces || (count != newRefiner._regFaceSize);
}
template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setNumBaseEdgeFaces(TopologyRefiner & newRefiner, Index e, int count) {
    newRefiner._levels[0]->resizeEdgeFaces(e, count);
}
template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setNumBaseVertexFaces(TopologyRefiner & newRefiner, Index v, int count) {
    newRefiner._levels[0]->resizeVertexFaces(v, count);
}
template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setNumBaseVertexEdges(TopologyRefiner & newRefiner, Index v, int count) {
    newRefiner._levels[0]->resizeVertexEdges(v, count);
}

template <class MESH>
inline IndexArray
TopologyRefinerFactory<MESH>::getBaseFaceVertices(TopologyRefiner & newRefiner, Index f) {
    return newRefiner._levels[0]->getFaceVertices(f);
}
template <class MESH>
inline IndexArray
TopologyRefinerFactory<MESH>::getBaseFaceEdges(TopologyRefiner & newRefiner,    Index f) {
    return newRefiner._levels[0]->getFaceEdges(f);
}
template <class MESH>
inline IndexArray
TopologyRefinerFactory<MESH>::getBaseEdgeVertices(TopologyRefiner & newRefiner, Index e) {
    return newRefiner._levels[0]->getEdgeVertices(e);
}
template <class MESH>
inline IndexArray
TopologyRefinerFactory<MESH>::getBaseEdgeFaces(TopologyRefiner & newRefiner,    Index e) {
    return newRefiner._levels[0]->getEdgeFaces(e);
}
template <class MESH>
inline IndexArray
TopologyRefinerFactory<MESH>::getBaseVertexFaces(TopologyRefiner & newRefiner,  Index v) {
    return newRefiner._levels[0]->getVertexFaces(v);
}
template <class MESH>
inline IndexArray
TopologyRefinerFactory<MESH>::getBaseVertexEdges(TopologyRefiner & newRefiner,  Index v) {
    return newRefiner._levels[0]->getVertexEdges(v);
}

template <class MESH>
inline LocalIndexArray
TopologyRefinerFactory<MESH>::getBaseEdgeFaceLocalIndices(TopologyRefiner & newRefiner, Index e)   {
    return newRefiner._levels[0]->getEdgeFaceLocalIndices(e);
}
template <class MESH>
inline LocalIndexArray
TopologyRefinerFactory<MESH>::getBaseVertexFaceLocalIndices(TopologyRefiner & newRefiner, Index v) {
    return newRefiner._levels[0]->getVertexFaceLocalIndices(v);
}
template <class MESH>
inline LocalIndexArray
TopologyRefinerFactory<MESH>::getBaseVertexEdgeLocalIndices(TopologyRefiner & newRefiner, Index v) {
    return newRefiner._levels[0]->getVertexEdgeLocalIndices(v);
}

template <class MESH>
inline Index
TopologyRefinerFactory<MESH>::findBaseEdge(TopologyRefiner const & newRefiner, Index v0, Index v1) {
    return newRefiner._levels[0]->findEdge(v0, v1);
}

template <class MESH>
inline void
TopologyRefinerFactory<MESH>::populateBaseLocalIndices(TopologyRefiner & newRefiner) {
    newRefiner._levels[0]->populateLocalIndices();
}

template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setBaseEdgeNonManifold(TopologyRefiner & newRefiner, Index e, bool b) {
    newRefiner._levels[0]->setEdgeNonManifold(e, b);
}
template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setBaseVertexNonManifold(TopologyRefiner & newRefiner, Index v, bool b) {
    newRefiner._levels[0]->setVertexNonManifold(v, b);
}

template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setBaseEdgeSharpness(TopologyRefiner & newRefiner, Index e, float s)   {
    newRefiner._levels[0]->getEdgeSharpness(e) = s;
}
template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setBaseVertexSharpness(TopologyRefiner & newRefiner, Index v, float s) {
    newRefiner._levels[0]->getVertexSharpness(v) = s;
}
template <class MESH>
inline void
TopologyRefinerFactory<MESH>::setBaseFaceHole(TopologyRefiner & newRefiner, Index f, bool b) {
    newRefiner._levels[0]->setFaceHole(f, b);
    newRefiner._hasHoles = newRefiner._hasHoles || b;
}

template <class MESH>
inline int
TopologyRefinerFactory<MESH>::createBaseFVarChannel(TopologyRefiner & newRefiner, int numValues) {
    return newRefiner._levels[0]->createFVarChannel(numValues, newRefiner._subdivOptions);
}
template <class MESH>
inline int
TopologyRefinerFactory<MESH>::createBaseFVarChannel(TopologyRefiner & newRefiner, int numValues, Sdc::Options const& fvarOptions) {
    Sdc::Options newOptions = newRefiner._subdivOptions;
    newOptions.SetFVarLinearInterpolation(fvarOptions.GetFVarLinearInterpolation());
    return newRefiner._levels[0]->createFVarChannel(numValues, newOptions);
}
template <class MESH>
inline IndexArray
TopologyRefinerFactory<MESH>::getBaseFaceFVarValues(TopologyRefiner & newRefiner, Index face, int channel) {
    return newRefiner._levels[0]->getFaceFVarValues(face, channel);
}


template <class MESH>
bool
TopologyRefinerFactory<MESH>::resizeComponentTopology(TopologyRefiner& /* refiner */, MESH const& /* mesh */) {

    Error(FAR_RUNTIME_ERROR,
        "Failure in TopologyRefinerFactory<>::resizeComponentTopology() -- no specialization provided.");

    //
    //  Sizing the topology tables:
    //      This method is for determining the sizes of the various topology tables (and other
    //  data) associated with the mesh.  Once completed, appropriate memory will be allocated
    //  and an additional method invoked to populate it accordingly.
    //
    //  The following methods should be called -- first those to specify the number of faces,
    //  edges and vertices in the mesh:
    //
    //      void setBaseFaceCount(  TopologyRefiner& newRefiner, int count)
    //      void setBaseEdgeCount(  TopologyRefiner& newRefiner, int count)
    //      void setBaseVertexCount(TopologyRefiner& newRefiner, int count)
    //
    //  and then for each face, edge and vertex, the number of its incident components:
    //
    //      void setBaseFaceVertexCount(TopologyRefiner& newRefiner, Index face, int count)
    //      void setBaseEdgeFaceCount(  TopologyRefiner& newRefiner, Index edge, int count)
    //      void setBaseVertexFaceCount(TopologyRefiner& newRefiner, Index vertex, int count)
    //      void setBaseVertexEdgeCount(TopologyRefiner& newRefiner, Index vertex, int count)
    //
    //  The count/size for a component type must be set before indices associated with that
    //  component type can be used.
    //
    //  Note that it is only necessary to size 4 of the 6 supported topological relations --
    //  the number of edge-vertices is fixed at two per edge, and the number of face-edges is
    //  the same as the number of face-vertices.
    //
    //  So a single pass through your mesh to gather up all of this sizing information will
    //  allow the Tables to be allocated appropriately once and avoid any dynamic resizing as
    //  it grows.
    //
    return false;
}

template <class MESH>
bool
TopologyRefinerFactory<MESH>::assignComponentTopology(TopologyRefiner& /* refiner */, MESH const& /* mesh */) {

    Error(FAR_RUNTIME_ERROR,
        "Failure in TopologyRefinerFactory<>::assignComponentTopology() -- no specialization provided.");

    //
    //  Assigning the topology tables:
    //      Once the topology tables have been allocated, the six required topological
    //  relations can be directly populated using the following methods:
    //
    //      IndexArray setBaseFaceVertices(TopologyRefiner& newRefiner, Index face)
    //      IndexArray setBaseFaceEdges(TopologyRefiner& newRefiner, Index face)
    //
    //      IndexArray setBaseEdgeVertices(TopologyRefiner& newRefiner, Index edge)
    //      IndexArray setBaseEdgeFaces(TopologyRefiner& newRefiner, Index edge)
    //
    //      IndexArray setBaseVertexEdges(TopologyRefiner& newRefiner, Index vertex)
    //      IndexArray setBaseVertexFaces(TopologyRefiner& newRefiner, Index vertex)
    //
    //  For the last two relations -- the faces and edges incident a vertex -- there are
    //  also "local indices" that must be specified (considering doing this internally),
    //  where the "local index" of each incident face or edge is the index of the vertex
    //  within that face or edge, and so ranging from 0-3 for incident quads and 0-1 for
    //  incident edges.  These are assigned through similarly retrieved arrays:
    //
    //      LocalIndexArray setBaseVertexFaceLocalIndices(TopologyRefiner& newRefiner, Index vertex)
    //      LocalIndexArray setBaseVertexEdgeLocalIndices(TopologyRefiner& newRefiner, Index vertex)
    //      LocalIndexArray setBaseEdgeFaceLocalIndices(  TopologyRefiner& newRefiner, Index edge)
    //
    //  or, if the mesh is manifold, explicit assignment of these can be deferred and
    //  all can be determined by calling:
    //
    //      void populateBaseLocalIndices(TopologyRefiner& newRefiner)
    //
    //  All components are assumed to be locally manifold and ordering of components in
    //  the above relations is expected to be counter-clockwise.
    //
    //  For non-manifold components, no ordering/orientation of incident components is
    //  assumed or required, but be sure to explicitly tag such components (vertices and
    //  edges) as non-manifold:
    //
    //      void setBaseEdgeNonManifold(TopologyRefiner& newRefiner, Index edge, bool b);
    //
    //      void setBaseVertexNonManifold(TopologyRefiner& newRefiner, Index vertex, bool b);
    //
    //  Also consider using TopologyLevel::ValidateTopology() when debugging to ensure
    //  that topology has been completely and correctly specified.
    //
    return false;
}

template <class MESH>
bool
TopologyRefinerFactory<MESH>::assignFaceVaryingTopology(TopologyRefiner& /* refiner */, MESH const& /* mesh */) {

    //
    //  Optional assigning face-varying topology tables:
    //
    //  Create independent face-varying primitive variable channels:
    //      int createBaseFVarChannel(TopologyRefiner& newRefiner, int numValues)
    //
    //  For each channel, populate the face-vertex values:
    //      IndexArray setBaseFaceFVarValues(TopologyRefiner& newRefiner, Index face, int channel = 0)
    //
    return true;
}

template <class MESH>
bool
TopologyRefinerFactory<MESH>::assignComponentTags(TopologyRefiner& /* refiner */, MESH const& /* mesh */) {

    //
    //  Optional tagging:
    //      This is where any additional feature tags -- sharpness, holes, etc. -- can be
    //  specified using:
    //
    //      void setBaseEdgeSharpness(TopologyRefiner& newRefiner, Index edge, float sharpness)
    //      void setBaseVertexSharpness(TopologyRefiner& newRefiner, Index vertex, float sharpness)
    //
    //      void setBaseFaceHole(TopologyRefiner& newRefiner, Index face, bool hole)
    //
    return true;
}

template <class MESH>
void
TopologyRefinerFactory<MESH>::reportInvalidTopology(
    TopologyError /* errCode */, char const * /* msg */, MESH const& /* mesh */) {

    //
    //  Optional topology validation error reporting:
    //      This method is called whenever the factory encounters topology validation
    //  errors. By default, nothing is reported
    //
}

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_FAR_TOPOLOGY_REFINER_FACTORY_H */
