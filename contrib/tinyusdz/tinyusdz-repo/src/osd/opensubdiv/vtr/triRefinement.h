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
#ifndef OPENSUBDIV3_VTR_TRI_REFINEMENT_H
#define OPENSUBDIV3_VTR_TRI_REFINEMENT_H

#include "../version.h"

#include "../vtr/refinement.h"


namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {
namespace internal {

//
//  TriRefinement:
//      A TriRefinement is a subclass of Refinement that splits all faces into tris.
//  It provides the configuration of parent-to-child components and the population of
//  all required topological relations in order to complete a valid Refinement.
//
class TriRefinement : public Refinement {

public:
    TriRefinement(Level const & parent, Level & child, Sdc::Options const & options);
    ~TriRefinement();

protected:
    //
    //  Virtual methods to complete the configuration of the parent-to-child mapping:
    //
    virtual void allocateParentChildIndices();

    virtual void markSparseFaceChildren();

    //
    //  Virtual methods to populate the six topological relations:
    //
    virtual void populateFaceVertexRelation();
    virtual void populateFaceEdgeRelation();
    virtual void populateEdgeVertexRelation();
    virtual void populateEdgeFaceRelation();
    virtual void populateVertexFaceRelation();
    virtual void populateVertexEdgeRelation();

    //
    //  Internal helper methods for populating the topology -- a few of these are
    //  identical to what is used for quad-splitting, so we may move them to the
    //  base class...
    //
    void populateFaceVertexCountsAndOffsets();
    void populateFaceVerticesFromParentFaces();

    void populateFaceEdgesFromParentFaces();

    void populateEdgeVerticesFromParentFaces();
    void populateEdgeVerticesFromParentEdges();

    void populateEdgeFacesFromParentFaces();
    void populateEdgeFacesFromParentEdges();

    void populateVertexFacesFromParentEdges();
    void populateVertexFacesFromParentVertices();

    void populateVertexEdgesFromParentEdges();
    void populateVertexEdgesFromParentVertices();

private:
    //
    //  Unlike the quad-split, which can share some vectors with the parent Level since
    //  child components correspond to face-vertices, the tri-split must define its
    //  own local vectors to identify the children for each parent component -- to
    //  be referenced within the base class for more immediate/inline access:
    //
    IndexVector _localFaceChildFaceCountsAndOffsets;
};

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_VTR_REFINEMENT_H */
