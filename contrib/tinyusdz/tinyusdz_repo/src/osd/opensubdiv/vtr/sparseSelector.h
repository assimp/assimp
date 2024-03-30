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
#ifndef OPENSUBDIV3_VTR_SPARSE_SELECTOR_H
#define OPENSUBDIV3_VTR_SPARSE_SELECTOR_H

#include "../version.h"

#include "../vtr/types.h"
#include "../vtr/refinement.h"

#include <vector>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {
namespace internal {

//
//  SparseSelector:
//      Class supporting "selection" of components in a Level for sparse Refinement.
//  The term "selection" here implies interest in the limit for that component, i.e.
//  the limit point for a selected vertex, the limit patch for a face, etc.  So this
//  class is responsible for ensuring that all neighboring components required to
//  support the limit of those selected are included in the refinement.
//
//  This class is associated with (and constructed given) a Refinement and its role
//  is to initialize that Refinement instance for eventual sparse refinement.  So it
//  is a friend of and expected to modify the Refinement as part of the selection.
//  Given its simplicity and scope it may be worth nesting it in Vtr::Refinement.
//
//  While all three component types -- vertices, edges and faces -- can be selected,
//  only selection of faces is currently used and actively supported as part of the
//  feature-adaptive refinement.
//
class SparseSelector {

public:
    SparseSelector(Refinement& refine) : _refine(&refine), _selected(false) { }
    ~SparseSelector() { }

    void        setRefinement(Refinement& refine) { _refine = &refine; }
    Refinement& getRefinement() const             { return *_refine; }

    bool isSelectionEmpty() const { return !_selected; }

    //
    //  Methods for selecting (and marking) components for refinement.  All component indices
    //  refer to components in the parent:
    //
    void selectVertex(Index pVertex);
    void selectEdge(  Index pEdge);
    void selectFace(  Index pFace);

private:
    SparseSelector() : _refine(0), _selected(false) { }

    bool wasVertexSelected(Index pVertex) const { return _refine->getParentVertexSparseTag(pVertex)._selected; }
    bool wasEdgeSelected(  Index pEdge) const   { return _refine->getParentEdgeSparseTag(pEdge)._selected; }
    bool wasFaceSelected(  Index pFace) const   { return _refine->getParentFaceSparseTag(pFace)._selected; }

    void markVertexSelected(Index pVertex) const { _refine->getParentVertexSparseTag(pVertex)._selected = true; }
    void markEdgeSelected(  Index pEdge) const   { _refine->getParentEdgeSparseTag(pEdge)._selected = true; }
    void markFaceSelected(  Index pFace) const   { _refine->getParentFaceSparseTag(pFace)._selected = true; }

    void initializeSelection();

private:
    Refinement* _refine;
    bool        _selected;
};

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_VTR_SPARSE_SELECTOR_H */
