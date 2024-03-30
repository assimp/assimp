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
#include "../far/topologyDescriptor.h"
#include "../far/topologyRefinerFactory.h"
#include "../far/topologyRefiner.h"

//  Unfortunately necessary for error codes that should be more accessible...
#include "../vtr/level.h"

#include <cstdio>
#ifdef _MSC_VER
    #define snprintf _snprintf
#endif


namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

//
//  Definitions for TopologyDescriptor:
//
TopologyDescriptor::TopologyDescriptor() {
    memset(this, 0, sizeof(TopologyDescriptor));
}


//
//  Definitions/specializations for its RefinerFactory<TopologyDescriptor>:
//
template <>
bool
TopologyRefinerFactory<TopologyDescriptor>::resizeComponentTopology(
    TopologyRefiner & refiner, TopologyDescriptor const & desc) {

    setNumBaseVertices(refiner, desc.numVertices);
    setNumBaseFaces(refiner, desc.numFaces);

    for (int face=0; face<desc.numFaces; ++face) {

        setNumBaseFaceVertices(refiner, face, desc.numVertsPerFace[face]);
    }
    return true;
}

template <>
bool
TopologyRefinerFactory<TopologyDescriptor>::assignComponentTopology(
    TopologyRefiner & refiner, TopologyDescriptor const & desc) {

    for (int face=0, idx=0; face<desc.numFaces; ++face) {

        IndexArray dstFaceVerts = getBaseFaceVertices(refiner, face);

        if (desc.isLeftHanded) {
            dstFaceVerts[0] = desc.vertIndicesPerFace[idx++];
            for (int vert=dstFaceVerts.size()-1; vert > 0; --vert) {

                dstFaceVerts[vert] = desc.vertIndicesPerFace[idx++];
            }
        } else {
            for (int vert=0; vert<dstFaceVerts.size(); ++vert) {

                dstFaceVerts[vert] = desc.vertIndicesPerFace[idx++];
            }
        }
    }
    return true;
}

template <>
bool
TopologyRefinerFactory<TopologyDescriptor>::assignComponentTags(
    TopologyRefiner & refiner, TopologyDescriptor const & desc) {

    if ((desc.numCreases>0) && desc.creaseVertexIndexPairs && desc.creaseWeights) {

        int const * vertIndexPairs = desc.creaseVertexIndexPairs;
        for (int edge=0; edge<desc.numCreases; ++edge, vertIndexPairs+=2) {

            Index idx = findBaseEdge(refiner, vertIndexPairs[0], vertIndexPairs[1]);

            if (idx!=INDEX_INVALID) {
                setBaseEdgeSharpness(refiner, idx, desc.creaseWeights[edge]);
            } else {
                char msg[1024];
                snprintf(msg, 1024, "Edge %d specified to be sharp does not exist (%d, %d)",
                    edge, vertIndexPairs[0], vertIndexPairs[1]);
                reportInvalidTopology(Vtr::internal::Level::TOPOLOGY_INVALID_CREASE_EDGE, msg, desc);
            }
        }
    }

    if ((desc.numCorners>0) && desc.cornerVertexIndices && desc.cornerWeights) {

        for (int vert=0; vert<desc.numCorners; ++vert) {

            int idx = desc.cornerVertexIndices[vert];

            if (idx >= 0 && idx < getNumBaseVertices(refiner)) {
                setBaseVertexSharpness(refiner, idx, desc.cornerWeights[vert]);
            } else {
                char msg[1024];
                snprintf(msg, 1024, "Vertex %d specified to be sharp does not exist", idx);
                reportInvalidTopology(Vtr::internal::Level::TOPOLOGY_INVALID_CREASE_VERT, msg, desc);
            }
        }
    }
    if (desc.numHoles>0) {
        for (int i=0; i<desc.numHoles; ++i) {
            setBaseFaceHole(refiner, desc.holeIndices[i], true);
        }
    }
    return true;
}

template <>
bool
TopologyRefinerFactory<TopologyDescriptor>::assignFaceVaryingTopology(
    TopologyRefiner & refiner, TopologyDescriptor const & desc) {

    if (desc.numFVarChannels>0) {

        for (int channel=0; channel<desc.numFVarChannels; ++channel) {

            int        numFVarValues = desc.fvarChannels[channel].numValues;
            int const* srcFVarValues = desc.fvarChannels[channel].valueIndices;

            createBaseFVarChannel(refiner, numFVarValues);

            for (int face = 0, srcNext = 0; face < desc.numFaces; ++face) {

                IndexArray dstFaceFVarValues = getBaseFaceFVarValues(refiner, face, channel);

                if (desc.isLeftHanded) {
                    dstFaceFVarValues[0] = srcFVarValues[srcNext++];
                    for (int vert = dstFaceFVarValues.size() - 1; vert > 0; --vert) {
                        
                        dstFaceFVarValues[vert] = srcFVarValues[srcNext++];
                    }
                } else {
                    for (int vert = 0; vert < dstFaceFVarValues.size(); ++vert) {
                        
                        dstFaceFVarValues[vert] = srcFVarValues[srcNext++];
                    }
                }
            }
        }
    }
    return true;
}

template <>
void
TopologyRefinerFactory<TopologyDescriptor>::reportInvalidTopology(
    TopologyError /* errCode */, char const * msg, TopologyDescriptor const& /* mesh */) {
    Warning(msg);
}

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
