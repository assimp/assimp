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
#ifndef OPENSUBDIV3_FAR_TOPOLOGY_DESCRIPTOR_H
#define OPENSUBDIV3_FAR_TOPOLOGY_DESCRIPTOR_H

#include "../version.h"

#include "../far/topologyRefiner.h"
#include "../far/topologyRefinerFactory.h"
#include "../far/error.h"

#include <cassert>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

///
/// \brief  A simple reference to raw topology data for use with TopologyRefinerFactory
/// 
/// TopologyDescriptor is a simple struct containing references to raw topology data used
/// to construct a TopologyRefiner.  It is not a requirement but a convenience for use
/// with TopologyRefinerFactory when mesh topology is not available in an existing mesh
/// data structure.  It should be functionally complete and simple to use, but for more
/// demanding situations, writing a custom Factory is usually warranted.
///
struct TopologyDescriptor {

    int           numVertices,
                  numFaces;

    int const   * numVertsPerFace;
    Index const * vertIndicesPerFace;

    int           numCreases;
    Index const * creaseVertexIndexPairs;
    float const * creaseWeights;

    int           numCorners;
    Index const * cornerVertexIndices;
    float const * cornerWeights;
        
    int           numHoles;
    Index const * holeIndices;

    bool          isLeftHanded;

    //  Face-varying data channel -- value indices correspond to vertex indices,
    //  i.e. one for every vertex of every face:
    //
    struct FVarChannel {

        int           numValues;
        Index const * valueIndices;

        FVarChannel() : numValues(0), valueIndices(0) { }
    };
        
    int                 numFVarChannels;
    FVarChannel const * fvarChannels;

    TopologyDescriptor();
};


//
//  Forward declarations of required TopologyRefinerFactory<TopologyDescriptor>
//  specializations (defined internally):
//
// @cond EXCLUDE_DOXYGEN

template <>
bool
TopologyRefinerFactory<TopologyDescriptor>::resizeComponentTopology(
    TopologyRefiner & refiner, TopologyDescriptor const & desc);

template <>
bool
TopologyRefinerFactory<TopologyDescriptor>::assignComponentTopology(
    TopologyRefiner & refiner, TopologyDescriptor const & desc);

template <>
bool
TopologyRefinerFactory<TopologyDescriptor>::assignComponentTags(
    TopologyRefiner & refiner, TopologyDescriptor const & desc);

template <>
bool
TopologyRefinerFactory<TopologyDescriptor>::assignFaceVaryingTopology(
    TopologyRefiner & refiner, TopologyDescriptor const & desc);

template <>
void
TopologyRefinerFactory<TopologyDescriptor>::reportInvalidTopology(
    TopologyError errCode, char const * msg, TopologyDescriptor const & desc);

// @endcond

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_FAR_TOPOLOGY_DESCRIPTOR_H */
