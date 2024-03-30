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
#include "../sdc/crease.h"
#include "../sdc/catmarkScheme.h"
#include "../sdc/bilinearScheme.h"
#include "../vtr/types.h"
#include "../vtr/level.h"
#include "../vtr/refinement.h"
#include "../vtr/fvarLevel.h"
#include "../vtr/fvarRefinement.h"
#include "../vtr/stackBuffer.h"

#include <cassert>
#include <cstdio>
#include <utility>


namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {
namespace internal {

//
//  Simple constructor, destructor and basic initializers:
//
Refinement::Refinement(Level const & parentArg, Level & childArg, Sdc::Options const& options) :
    _parent(&parentArg),
    _child(&childArg),
    _options(options),
    _regFaceSize(-1),
    _uniform(false),
    _faceVertsFirst(false),
    _childFaceFromFaceCount(0),
    _childEdgeFromFaceCount(0),
    _childEdgeFromEdgeCount(0),
    _childVertFromFaceCount(0),
    _childVertFromEdgeCount(0),
    _childVertFromVertCount(0),
    _firstChildFaceFromFace(0),
    _firstChildEdgeFromFace(0),
    _firstChildEdgeFromEdge(0),
    _firstChildVertFromFace(0),
    _firstChildVertFromEdge(0),
    _firstChildVertFromVert(0) {

    assert((childArg.getDepth() == 0) && (childArg.getNumVertices() == 0));
    childArg._depth = 1 + parentArg.getDepth();
}

Refinement::~Refinement() {

    for (int i = 0; i < (int)_fvarChannels.size(); ++i) {
        delete _fvarChannels[i];
    }
}

void
Refinement::initializeChildComponentCounts() {

    //
    //  Assign the child's component counts/inventory based on the child components identified:
    //
    _child->_faceCount = _childFaceFromFaceCount;
    _child->_edgeCount = _childEdgeFromFaceCount + _childEdgeFromEdgeCount;
    _child->_vertCount = _childVertFromFaceCount + _childVertFromEdgeCount + _childVertFromVertCount;
}

void
Refinement::initializeSparseSelectionTags() {

    _parentFaceTag.resize(_parent->getNumFaces());
    _parentEdgeTag.resize(_parent->getNumEdges());
    _parentVertexTag.resize(_parent->getNumVertices());
}


//
//  The main refinement method -- provides a high-level overview of refinement:
//
//  The refinement process is as follows:
//      - determine a mapping from parent components to their potential child components
//          - for sparse refinement this mapping will be partial
//      - determine the reverse mapping from chosen child components back to their parents
//          - previously this was optional -- not strictly necessary and comes at added cost
//          - does simplify iteration of child components when refinement is sparse
//      - propagate/initialize component Tags from parents to their children
//          - knowing these Tags for a child component simplifies dealing with it later
//      - subdivide the topology, i.e. populate all topology relations for the child Level
//          - any subset of the 6 relations in a Level can be created
//          - using the minimum required in the last Level is very advantageous
//      - subdivide the sharpness values in the child Level
//      - subdivide face-varying channels in the child Level
//
void
Refinement::refine(Options refineOptions) {

    //  This will become redundant when/if assigned on construction:
    assert(_parent && _child);

    _uniform        = !refineOptions._sparse;
    _faceVertsFirst =  refineOptions._faceVertsFirst;

    //  We may soon have an option here to suppress refinement of FVar channels...
    bool refineOptions_ignoreFVarChannels = false;

    bool optionallyRefineFVar = (_parent->getNumFVarChannels() > 0) && !refineOptions_ignoreFVarChannels;

    //
    //  Initialize the parent-to-child and reverse child-to-parent mappings and propagate
    //  component tags to the new child components:
    //
    populateParentToChildMapping();

    initializeChildComponentCounts();

    populateChildToParentMapping();

    propagateComponentTags();

    //
    //  Subdivide the topology -- populating only those of the 6 relations specified
    //  (though we do require the vertex-face relation for refining FVar channels):
    //
    Relations relationsToPopulate;
    if (refineOptions._minimalTopology) {
        relationsToPopulate.setAll(false);
        relationsToPopulate._faceVertices = true;
    } else {
        relationsToPopulate.setAll(true);
    }
    if (optionallyRefineFVar) {
        relationsToPopulate._vertexFaces = true;
    }

    subdivideTopology(relationsToPopulate);

    //
    //  Subdivide the sharpness values and face-varying channels:
    //    - note there is some dependency of the vertex tag/Rule for semi-sharp vertices
    //
    subdivideSharpnessValues();

    if (optionallyRefineFVar) {
        subdivideFVarChannels();
    }

    //  Various debugging support:
    //
    //printf("Vertex refinement to level %d completed...\n", _child->getDepth());
    //_child->print();
    //printf("  validating refinement to level %d...\n", _child->getDepth());
    //_child->validateTopology();
    //assert(_child->validateTopology());
}


//
//  Methods to construct the parent-to-child mapping
//
void
Refinement::populateParentToChildMapping() {

    allocateParentChildIndices();

    //
    //  If sparse refinement, mark indices of any components in addition to those selected
    //  so that we have the full neighborhood for selected components:
    //
    if (!_uniform) {
        //  Make sure the selection was non-empty -- currently unsupported...
        if (_parentVertexTag.size() == 0) {
            assert("Unsupported empty sparse refinement detected in Refinement" == 0);
        }
        markSparseChildComponentIndices();
    }

    populateParentChildIndices();
}

namespace {
    inline bool isSparseIndexMarked(Index index)   { return index != 0; }

    inline int
    sequenceSparseIndexVector(IndexVector& indexVector, int baseValue = 0) {
        int validCount = 0;
        for (int i = 0; i < (int) indexVector.size(); ++i) {
            indexVector[i] = isSparseIndexMarked(indexVector[i])
                           ? (baseValue + validCount++) : INDEX_INVALID;
        }
        return validCount;
    }

    inline int
    sequenceFullIndexVector(IndexVector& indexVector, int baseValue = 0) {
        int indexCount = (int) indexVector.size();
        for (int i = 0; i < indexCount; ++i) {
            indexVector[i] = baseValue++;
        }
        return indexCount;
    }
}

void
Refinement::populateParentChildIndices() {

    //
    //  Two vertex orderings are currently supported -- ordering vertices refined
    //  from vertices first, or those refined from faces first.  It's possible this
    //  may be extended to more possibilities.  Once the ordering is defined here,
    //  other than analogous initialization in FVarRefinement, the treatment of
    //  vertices in blocks based on origin should make the rest of the code
    //  invariant to ordering changes.
    //
    //  These two blocks now differ only in the utility function that assigns the
    //  sequential values to the index vectors -- so parameterization/simplification
    //  is now possible...
    //
    if (_uniform) {
        //  child faces:
        _firstChildFaceFromFace = 0;
        _childFaceFromFaceCount = sequenceFullIndexVector(_faceChildFaceIndices, _firstChildFaceFromFace);

        //  child edges:
        _firstChildEdgeFromFace = 0;
        _childEdgeFromFaceCount = sequenceFullIndexVector(_faceChildEdgeIndices, _firstChildEdgeFromFace);

        _firstChildEdgeFromEdge = _childEdgeFromFaceCount;
        _childEdgeFromEdgeCount = sequenceFullIndexVector(_edgeChildEdgeIndices, _firstChildEdgeFromEdge);

        //  child vertices:
        if (_faceVertsFirst) {
            _firstChildVertFromFace = 0;
            _childVertFromFaceCount = sequenceFullIndexVector(_faceChildVertIndex, _firstChildVertFromFace);

            _firstChildVertFromEdge = _firstChildVertFromFace + _childVertFromFaceCount;
            _childVertFromEdgeCount = sequenceFullIndexVector(_edgeChildVertIndex, _firstChildVertFromEdge);

            _firstChildVertFromVert = _firstChildVertFromEdge + _childVertFromEdgeCount;
            _childVertFromVertCount = sequenceFullIndexVector(_vertChildVertIndex, _firstChildVertFromVert);
        } else {
            _firstChildVertFromVert = 0;
            _childVertFromVertCount = sequenceFullIndexVector(_vertChildVertIndex, _firstChildVertFromVert);

            _firstChildVertFromFace = _firstChildVertFromVert + _childVertFromVertCount;
            _childVertFromFaceCount = sequenceFullIndexVector(_faceChildVertIndex, _firstChildVertFromFace);

            _firstChildVertFromEdge = _firstChildVertFromFace + _childVertFromFaceCount;
            _childVertFromEdgeCount = sequenceFullIndexVector(_edgeChildVertIndex, _firstChildVertFromEdge);
        }
    } else {
        //  child faces:
        _firstChildFaceFromFace = 0;
        _childFaceFromFaceCount = sequenceSparseIndexVector(_faceChildFaceIndices, _firstChildFaceFromFace);

        //  child edges:
        _firstChildEdgeFromFace = 0;
        _childEdgeFromFaceCount = sequenceSparseIndexVector(_faceChildEdgeIndices, _firstChildEdgeFromFace);

        _firstChildEdgeFromEdge = _childEdgeFromFaceCount;
        _childEdgeFromEdgeCount = sequenceSparseIndexVector(_edgeChildEdgeIndices, _firstChildEdgeFromEdge);

        //  child vertices:
        if (_faceVertsFirst) {
            _firstChildVertFromFace = 0;
            _childVertFromFaceCount = sequenceSparseIndexVector(_faceChildVertIndex, _firstChildVertFromFace);

            _firstChildVertFromEdge = _firstChildVertFromFace + _childVertFromFaceCount;
            _childVertFromEdgeCount = sequenceSparseIndexVector(_edgeChildVertIndex, _firstChildVertFromEdge);

            _firstChildVertFromVert = _firstChildVertFromEdge + _childVertFromEdgeCount;
            _childVertFromVertCount = sequenceSparseIndexVector(_vertChildVertIndex, _firstChildVertFromVert);
        } else {
            _firstChildVertFromVert = 0;
            _childVertFromVertCount = sequenceSparseIndexVector(_vertChildVertIndex, _firstChildVertFromVert);

            _firstChildVertFromFace = _firstChildVertFromVert + _childVertFromVertCount;
            _childVertFromFaceCount = sequenceSparseIndexVector(_faceChildVertIndex, _firstChildVertFromFace);

            _firstChildVertFromEdge = _firstChildVertFromFace + _childVertFromFaceCount;
            _childVertFromEdgeCount = sequenceSparseIndexVector(_edgeChildVertIndex, _firstChildVertFromEdge);
        }
    }
}

void
Refinement::printParentToChildMapping() const {

    printf("Parent-to-child component mapping:\n");
    for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
        printf("  Face %d:\n", pFace);
        printf("    Child vert:  %d\n", _faceChildVertIndex[pFace]);

        printf("    Child faces: ");
        ConstIndexArray childFaces = getFaceChildFaces(pFace);
        for (int i = 0; i < childFaces.size(); ++i) {
            printf(" %d", childFaces[i]);
        }
        printf("\n");

        printf("    Child edges: ");
        ConstIndexArray childEdges = getFaceChildEdges(pFace);
        for (int i = 0; i < childEdges.size(); ++i) {
            printf(" %d", childEdges[i]);
        }
        printf("\n");
    }
    for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
        printf("  Edge %d:\n", pEdge);
        printf("    Child vert:  %d\n", _edgeChildVertIndex[pEdge]);

        ConstIndexArray childEdges = getEdgeChildEdges(pEdge);
        printf("    Child edges: %d %d\n", childEdges[0], childEdges[1]);
    }
    for (Index pVert = 0; pVert < _parent->getNumVertices(); ++pVert) {
        printf("  Vert %d:\n", pVert);
        printf("    Child vert:  %d\n", _vertChildVertIndex[pVert]);
    }
}


//
//  Methods to construct the child-to-parent mapping:
//
void
Refinement::populateChildToParentMapping() {

    ChildTag initialChildTags[2][4];
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 4; ++j) {
            ChildTag & tag = initialChildTags[i][j];

            tag._incomplete    = (unsigned char)i;
            tag._parentType    = 0;
            tag._indexInParent = (unsigned char)j;
        }
    }

    populateFaceParentVectors(initialChildTags);
    populateEdgeParentVectors(initialChildTags);
    populateVertexParentVectors(initialChildTags);
}

void
Refinement::populateFaceParentVectors(ChildTag const initialChildTags[2][4]) {

    _childFaceTag.resize(_child->getNumFaces());
    _childFaceParentIndex.resize(_child->getNumFaces());

    populateFaceParentFromParentFaces(initialChildTags);
}
void
Refinement::populateFaceParentFromParentFaces(ChildTag const initialChildTags[2][4]) {

    if (_uniform) {
        Index cFace = getFirstChildFaceFromFaces();
        for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
            ConstIndexArray cFaces = getFaceChildFaces(pFace);
            if (cFaces.size() == 4) {
                _childFaceTag[cFace + 0] = initialChildTags[0][0];
                _childFaceTag[cFace + 1] = initialChildTags[0][1];
                _childFaceTag[cFace + 2] = initialChildTags[0][2];
                _childFaceTag[cFace + 3] = initialChildTags[0][3];

                _childFaceParentIndex[cFace + 0] = pFace;
                _childFaceParentIndex[cFace + 1] = pFace;
                _childFaceParentIndex[cFace + 2] = pFace;
                _childFaceParentIndex[cFace + 3] = pFace;

                cFace += 4;
            } else {
                bool childTooLarge = (cFaces.size() > 4);
                for (int i = 0; i < cFaces.size(); ++i, ++cFace) {
                    _childFaceTag[cFace] = initialChildTags[0][childTooLarge ? 0 : i];
                    _childFaceParentIndex[cFace] = pFace;
                }
            }
        }
    } else {
        //  Child faces of faces:
        for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
            bool incomplete = !_parentFaceTag[pFace]._selected;

            IndexArray cFaces = getFaceChildFaces(pFace);
            if (!incomplete && (cFaces.size() == 4)) {
                _childFaceTag[cFaces[0]] = initialChildTags[0][0];
                _childFaceTag[cFaces[1]] = initialChildTags[0][1];
                _childFaceTag[cFaces[2]] = initialChildTags[0][2];
                _childFaceTag[cFaces[3]] = initialChildTags[0][3];

                _childFaceParentIndex[cFaces[0]] = pFace;
                _childFaceParentIndex[cFaces[1]] = pFace;
                _childFaceParentIndex[cFaces[2]] = pFace;
                _childFaceParentIndex[cFaces[3]] = pFace;
            } else {
                bool childTooLarge = (cFaces.size() > 4);
                for (int i = 0; i < cFaces.size(); ++i) {
                    if (IndexIsValid(cFaces[i])) {
                        _childFaceTag[cFaces[i]] = initialChildTags[incomplete][childTooLarge ? 0 : i];
                        _childFaceParentIndex[cFaces[i]] = pFace;
                    }
                }
            }
        }
    }
}

void
Refinement::populateEdgeParentVectors(ChildTag const initialChildTags[2][4]) {

    _childEdgeTag.resize(_child->getNumEdges());
    _childEdgeParentIndex.resize(_child->getNumEdges());

    populateEdgeParentFromParentFaces(initialChildTags);
    populateEdgeParentFromParentEdges(initialChildTags);
}
void
Refinement::populateEdgeParentFromParentFaces(ChildTag const initialChildTags[2][4]) {

    if (_uniform) {
        Index cEdge = getFirstChildEdgeFromFaces();
        for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
            ConstIndexArray cEdges = getFaceChildEdges(pFace);
            if (cEdges.size() == 4) {
                _childEdgeTag[cEdge + 0] = initialChildTags[0][0];
                _childEdgeTag[cEdge + 1] = initialChildTags[0][1];
                _childEdgeTag[cEdge + 2] = initialChildTags[0][2];
                _childEdgeTag[cEdge + 3] = initialChildTags[0][3];

                _childEdgeParentIndex[cEdge + 0] = pFace;
                _childEdgeParentIndex[cEdge + 1] = pFace;
                _childEdgeParentIndex[cEdge + 2] = pFace;
                _childEdgeParentIndex[cEdge + 3] = pFace;

                cEdge += 4;
            } else {
                bool childTooLarge = (cEdges.size() > 4);
                for (int i = 0; i < cEdges.size(); ++i, ++cEdge) {
                    _childEdgeTag[cEdge] = initialChildTags[0][childTooLarge ? 0 : i];
                    _childEdgeParentIndex[cEdge] = pFace;
                }
            }
        }
    } else {
        for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
            bool incomplete = !_parentFaceTag[pFace]._selected;

            IndexArray cEdges = getFaceChildEdges(pFace);
            if (!incomplete && (cEdges.size() == 4)) {
                _childEdgeTag[cEdges[0]] = initialChildTags[0][0];
                _childEdgeTag[cEdges[1]] = initialChildTags[0][1];
                _childEdgeTag[cEdges[2]] = initialChildTags[0][2];
                _childEdgeTag[cEdges[3]] = initialChildTags[0][3];

                _childEdgeParentIndex[cEdges[0]] = pFace;
                _childEdgeParentIndex[cEdges[1]] = pFace;
                _childEdgeParentIndex[cEdges[2]] = pFace;
                _childEdgeParentIndex[cEdges[3]] = pFace;
            } else {
                bool childTooLarge = (cEdges.size() > 4);
                for (int i = 0; i < cEdges.size(); ++i) {
                    if (IndexIsValid(cEdges[i])) {
                        _childEdgeTag[cEdges[i]] = initialChildTags[incomplete][childTooLarge ? 0 : i];
                        _childEdgeParentIndex[cEdges[i]] = pFace;
                    }
                }
            }
        }
    }
}
void
Refinement::populateEdgeParentFromParentEdges(ChildTag const initialChildTags[2][4]) {

    if (_uniform) {
        Index cEdge = getFirstChildEdgeFromEdges();
        for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge, cEdge += 2) {
            _childEdgeTag[cEdge + 0] = initialChildTags[0][0];
            _childEdgeTag[cEdge + 1] = initialChildTags[0][1];

            _childEdgeParentIndex[cEdge + 0] = pEdge;
            _childEdgeParentIndex[cEdge + 1] = pEdge;
        }
    } else {
        for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
            bool incomplete = !_parentEdgeTag[pEdge]._selected;

            IndexArray cEdges = getEdgeChildEdges(pEdge);
            if (!incomplete) {
                _childEdgeTag[cEdges[0]] = initialChildTags[0][0];
                _childEdgeTag[cEdges[1]] = initialChildTags[0][1];

                _childEdgeParentIndex[cEdges[0]] = pEdge;
                _childEdgeParentIndex[cEdges[1]] = pEdge;
            } else {
                for (int i = 0; i < 2; ++i) {
                    if (IndexIsValid(cEdges[i])) {
                        _childEdgeTag[cEdges[i]] = initialChildTags[incomplete][i];
                        _childEdgeParentIndex[cEdges[i]] = pEdge;
                    }
                }
            }
        }
    }
}

void
Refinement::populateVertexParentVectors(ChildTag const initialChildTags[2][4]) {

    if (_uniform) {
        _childVertexTag.resize(_child->getNumVertices(), initialChildTags[0][0]);
    } else {
        _childVertexTag.resize(_child->getNumVertices(), initialChildTags[1][0]);
    }
    _childVertexParentIndex.resize(_child->getNumVertices());

    populateVertexParentFromParentFaces(initialChildTags);
    populateVertexParentFromParentEdges(initialChildTags);
    populateVertexParentFromParentVertices(initialChildTags);
}
void
Refinement::populateVertexParentFromParentFaces(ChildTag const initialChildTags[2][4]) {

    if (getNumChildVerticesFromFaces() == 0) return;

    if (_uniform) {
        Index cVert = getFirstChildVertexFromFaces();
        for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace, ++cVert) {
            //  Child tag was initialized as the complete and only child when allocated

            _childVertexParentIndex[cVert] = pFace;
        }
    } else {
        ChildTag const & completeChildTag = initialChildTags[0][0];

        for (Index pFace = 0; pFace < _parent->getNumFaces(); ++pFace) {
            Index cVert = _faceChildVertIndex[pFace];
            if (IndexIsValid(cVert)) {
                //  Child tag was initialized as incomplete -- reset if complete:
                if (_parentFaceTag[pFace]._selected) {
                    _childVertexTag[cVert] = completeChildTag;
                }
                _childVertexParentIndex[cVert] = pFace;
            }
        }
    }
}
void
Refinement::populateVertexParentFromParentEdges(ChildTag const initialChildTags[2][4]) {

    if (_uniform) {
        Index cVert = getFirstChildVertexFromEdges();
        for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge, ++cVert) {
            //  Child tag was initialized as the complete and only child when allocated

            _childVertexParentIndex[cVert] = pEdge;
        }
    } else {
        ChildTag const & completeChildTag = initialChildTags[0][0];

        for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
            Index cVert = _edgeChildVertIndex[pEdge];
            if (IndexIsValid(cVert)) {
                //  Child tag was initialized as incomplete -- reset if complete:
                if (_parentEdgeTag[pEdge]._selected) {
                    _childVertexTag[cVert] = completeChildTag;
                }
                _childVertexParentIndex[cVert] = pEdge;
            }
        }
    }
}
void
Refinement::populateVertexParentFromParentVertices(ChildTag const initialChildTags[2][4]) {

    if (_uniform) {
        Index cVert = getFirstChildVertexFromVertices();
        for (Index pVert = 0; pVert < _parent->getNumVertices(); ++pVert, ++cVert) {
            //  Child tag was initialized as the complete and only child when allocated

            _childVertexParentIndex[cVert] = pVert;
        }
    } else {
        ChildTag const & completeChildTag = initialChildTags[0][0];

        for (Index pVert = 0; pVert < _parent->getNumVertices(); ++pVert) {
            Index cVert = _vertChildVertIndex[pVert];
            if (IndexIsValid(cVert)) {
                //  Child tag was initialized as incomplete but these should be complete:
                if (_parentVertexTag[pVert]._selected) {
                    _childVertexTag[cVert] = completeChildTag;
                }
                _childVertexParentIndex[cVert] = pVert;
            }
        }
    }
}


//
//  Methods to propagate/initialize child component tags from their parent component:
//
void
Refinement::propagateComponentTags() {

    populateFaceTagVectors();
    populateEdgeTagVectors();
    populateVertexTagVectors();
}

void
Refinement::populateFaceTagVectors() {

    _child->_faceTags.resize(_child->getNumFaces());

    populateFaceTagsFromParentFaces();
}
void
Refinement::populateFaceTagsFromParentFaces() {

    //
    //  Tags for faces originating from faces are inherited from the parent face:
    //
    Index cFace    = getFirstChildFaceFromFaces();
    Index cFaceEnd = cFace + getNumChildFacesFromFaces();
    for ( ; cFace < cFaceEnd; ++cFace) {
        _child->_faceTags[cFace] = _parent->_faceTags[_childFaceParentIndex[cFace]];
    }
}

void
Refinement::populateEdgeTagVectors() {

    _child->_edgeTags.resize(_child->getNumEdges());

    populateEdgeTagsFromParentFaces();
    populateEdgeTagsFromParentEdges();
}
void
Refinement::populateEdgeTagsFromParentFaces() {

    //
    //  Tags for edges originating from faces are all constant:
    //
    Level::ETag eTag;
    eTag.clear();

    Index cEdge    = getFirstChildEdgeFromFaces();
    Index cEdgeEnd = cEdge + getNumChildEdgesFromFaces();
    for ( ; cEdge < cEdgeEnd; ++cEdge) {
        _child->_edgeTags[cEdge] = eTag;
    }
}
void
Refinement::populateEdgeTagsFromParentEdges() {

    //
    //  Tags for edges originating from edges are inherited from the parent edge:
    //
    Index cEdge    = getFirstChildEdgeFromEdges();
    Index cEdgeEnd = cEdge + getNumChildEdgesFromEdges();
    for ( ; cEdge < cEdgeEnd; ++cEdge) {
        _child->_edgeTags[cEdge] = _parent->_edgeTags[_childEdgeParentIndex[cEdge]];
    }
}

void
Refinement::populateVertexTagVectors() {

    _child->_vertTags.resize(_child->getNumVertices());

    populateVertexTagsFromParentFaces();
    populateVertexTagsFromParentEdges();
    populateVertexTagsFromParentVertices();

    if (!_uniform) {
        for (Index cVert = 0; cVert < _child->getNumVertices(); ++cVert) {
            if (_childVertexTag[cVert]._incomplete) {
                _child->_vertTags[cVert]._incomplete = true;
            }
        }
    }
}
void
Refinement::populateVertexTagsFromParentFaces() {

    //
    //  Similarly, tags for vertices originating from faces are all constant -- with the
    //  unfortunate exception of refining level 0, where the faces may be N-sided and so
    //  introduce new vertices that need to be tagged as extra-ordinary:
    //
    if (getNumChildVerticesFromFaces() == 0) return;

    Level::VTag vTag;
    vTag.clear();
    vTag._rule = Sdc::Crease::RULE_SMOOTH;

    Index cVert    = getFirstChildVertexFromFaces();
    Index cVertEnd = cVert + getNumChildVerticesFromFaces();

    if (_parent->_depth > 0) {
        for ( ; cVert < cVertEnd; ++cVert) {
            _child->_vertTags[cVert] = vTag;
        }
    } else {
        for ( ; cVert < cVertEnd; ++cVert) {
            _child->_vertTags[cVert] = vTag;

            if (_parent->getNumFaceVertices(_childVertexParentIndex[cVert]) != _regFaceSize) {
                _child->_vertTags[cVert]._xordinary = true;
            }
        }
    }
}
void
Refinement::populateVertexTagsFromParentEdges() {

    //
    //  Tags for vertices originating from edges are initialized according to the tags
    //  of the parent edge:
    //
    Level::VTag vTag;
    vTag.clear();

    for (Index pEdge = 0; pEdge < _parent->getNumEdges(); ++pEdge) {
        Index cVert = _edgeChildVertIndex[pEdge];
        if (!IndexIsValid(cVert)) continue;

        //  From the cleared local VTag, we just need to assign properties dependent
        //  on the parent edge:
        Level::ETag const& pEdgeTag = _parent->_edgeTags[pEdge];

        vTag._nonManifold    = pEdgeTag._nonManifold;
        vTag._boundary       = pEdgeTag._boundary;
        vTag._semiSharpEdges = pEdgeTag._semiSharp;
        vTag._infSharpEdges  = pEdgeTag._infSharp;
        vTag._infSharpCrease = pEdgeTag._infSharp;
        vTag._infIrregular   = pEdgeTag._infSharp && pEdgeTag._nonManifold;

        vTag._rule = (Level::VTag::VTagSize)((pEdgeTag._semiSharp || pEdgeTag._infSharp)
                       ? Sdc::Crease::RULE_CREASE : Sdc::Crease::RULE_SMOOTH);

        _child->_vertTags[cVert] = vTag;
    }
}
void
Refinement::populateVertexTagsFromParentVertices() {

    //
    //  Tags for vertices originating from vertices are inherited from the parent vertex:
    //
    Index cVert    = getFirstChildVertexFromVertices();
    Index cVertEnd = cVert + getNumChildVerticesFromVertices();
    for ( ; cVert < cVertEnd; ++cVert) {
        _child->_vertTags[cVert] = _parent->_vertTags[_childVertexParentIndex[cVert]];
        _child->_vertTags[cVert]._incidIrregFace = 0;
    }
}



//
//  Methods to subdivide the topology:
//
//  The main method to subdivide topology is fairly simple -- given a set of relations
//  to populate it simply tests and populates each relation separately.  The method for
//  each relation is responsible for appropriate allocation and initialization of all
//  data involved, and these are virtual -- provided by a quad- or tri-split subclass.
//
void
Refinement::subdivideTopology(Relations const& applyTo) {

    if (applyTo._faceVertices) {
        populateFaceVertexRelation();
    }
    if (applyTo._faceEdges) {
        populateFaceEdgeRelation();
    }
    if (applyTo._edgeVertices) {
        populateEdgeVertexRelation();
    }
    if (applyTo._edgeFaces) {
        populateEdgeFaceRelation();
    }
    if (applyTo._vertexFaces) {
        populateVertexFaceRelation();
    }
    if (applyTo._vertexEdges) {
        populateVertexEdgeRelation();
    }

    //
    //  Additional members of the child Level not specific to any relation...
    //      - note in the case of max-valence, the child's max-valence may be less
    //  than the parent if that maximal parent vertex was not included in the sparse
    //  refinement (possible when sparse refinement is more general).
    //      - it may also be more if the base level was fairly trivial, i.e. less
    //  than the regular valence, or contains non-manifold edges with many faces.
    //      - NOTE that when/if we support N-gons for tri-splitting, that the valence
    //  of edge-vertices introduced on the N-gon may be 7 rather than 6, while N may
    //  be less than both.
    //
    //  In general, we need a better way to deal with max-valence.  The fact that 
    //  each topology relation is independent/optional complicates the issue of 
    //  where to keep track of it...
    //
    if (_splitType == Sdc::SPLIT_TO_QUADS) {
        _child->_maxValence = std::max(_parent->_maxValence, 4);
        _child->_maxValence = std::max(_child->_maxValence, 2 + _parent->_maxEdgeFaces);
    } else {
        _child->_maxValence = std::max(_parent->_maxValence, 6);
        _child->_maxValence = std::max(_child->_maxValence, 2 + _parent->_maxEdgeFaces * 2);
    }
}


//
//  Methods to subdivide sharpness values:
//
void
Refinement::subdivideSharpnessValues() {

    //
    //  Subdividing edge and vertex sharpness values are independent, but in order
    //  to maintain proper classification/tagging of components as semi-sharp, both
    //  must be computed and the neighborhood inspected to properly update the
    //  status.
    //
    //  It is possible to clear the semi-sharp status when propagating the tags and
    //  to reset it (potentially multiple times) when updating the sharpness values.
    //  The vertex subdivision Rule is also affected by this, which complicates the
    //  process.  So for now we apply a post-process to explicitly handle all
    //  semi-sharp vertices.
    //

    //  These methods will update sharpness tags local to the edges and vertices:
    subdivideEdgeSharpness();
    subdivideVertexSharpness();

    //  This method uses local sharpness tags (set above) to update vertex tags that
    //  reflect the neighborhood of the vertex (e.g. its rule):
    reclassifySemisharpVertices();
}

void
Refinement::subdivideEdgeSharpness() {

    Sdc::Crease creasing(_options);

    _child->_edgeSharpness.clear();
    _child->_edgeSharpness.resize(_child->getNumEdges(), Sdc::Crease::SHARPNESS_SMOOTH);

    //
    //  Edge sharpness is passed to child-edges using the parent edge and the
    //  parent vertex for which the child corresponds.  Child-edges are created
    //  from both parent faces and parent edges, but those child-edges created
    //  from a parent face should be within the face's interior and so smooth
    //  (and so previously initialized).
    //
    //  The presence/validity of each parent edges child vert indicates one or
    //  more child edges.
    //
    //  NOTE -- It is also useful at this time to classify the child vert of
    //  this edge based on the creasing information here, particularly when a
    //  non-trivial creasing method like Chaikin is used.  This is not being
    //  done now but is worth considering...
    //
    internal::StackBuffer<float,16> pVertEdgeSharpness;
    if (!creasing.IsUniform()) {
        pVertEdgeSharpness.Reserve(_parent->getMaxValence());
    }

    Index cEdge    = getFirstChildEdgeFromEdges();
    Index cEdgeEnd = cEdge + getNumChildEdgesFromEdges();
    for ( ; cEdge < cEdgeEnd; ++cEdge) {
        float&       cSharpness = _child->_edgeSharpness[cEdge];
        Level::ETag& cEdgeTag   = _child->_edgeTags[cEdge];

        if (cEdgeTag._infSharp) {
            cSharpness = Sdc::Crease::SHARPNESS_INFINITE;
        } else if (cEdgeTag._semiSharp) {
            Index pEdge      = _childEdgeParentIndex[cEdge];
            float pSharpness = _parent->_edgeSharpness[pEdge];

            if (creasing.IsUniform()) {
                cSharpness = creasing.SubdivideUniformSharpness(pSharpness);
            } else {
                ConstIndexArray pEdgeVerts = _parent->getEdgeVertices(pEdge);
                Index           pVert      = pEdgeVerts[_childEdgeTag[cEdge]._indexInParent];
                ConstIndexArray pVertEdges = _parent->getVertexEdges(pVert);

                for (int i = 0; i < pVertEdges.size(); ++i) {
                    pVertEdgeSharpness[i] = _parent->_edgeSharpness[pVertEdges[i]];
                }
                cSharpness = creasing.SubdivideEdgeSharpnessAtVertex(pSharpness, pVertEdges.size(),
                                                                         pVertEdgeSharpness);
            }
            if (! Sdc::Crease::IsSharp(cSharpness)) {
                cEdgeTag._semiSharp = false;
            }
        }
    }
}

void
Refinement::subdivideVertexSharpness() {

    Sdc::Crease creasing(_options);

    _child->_vertSharpness.clear();
    _child->_vertSharpness.resize(_child->getNumVertices(), Sdc::Crease::SHARPNESS_SMOOTH);

    //
    //  All child-verts originating from faces or edges are initialized as smooth
    //  above.  Only those originating from vertices require "subdivided" values:
    //
    //  Only deal with the subrange of vertices originating from vertices:
    Index cVertBegin = getFirstChildVertexFromVertices();
    Index cVertEnd   = cVertBegin + getNumChildVerticesFromVertices();

    for (Index cVert = cVertBegin; cVert < cVertEnd; ++cVert) {
        float&       cSharpness = _child->_vertSharpness[cVert];
        Level::VTag& cVertTag   = _child->_vertTags[cVert];

        if (cVertTag._infSharp) {
            cSharpness = Sdc::Crease::SHARPNESS_INFINITE;
        } else if (cVertTag._semiSharp) {
            Index pVert      = _childVertexParentIndex[cVert];
            float pSharpness = _parent->_vertSharpness[pVert];

            cSharpness = creasing.SubdivideVertexSharpness(pSharpness);
            if (! Sdc::Crease::IsSharp(cSharpness)) {
                cVertTag._semiSharp = false;
            }
        }
    }
}

void
Refinement::reclassifySemisharpVertices() {

    typedef Level::VTag::VTagSize VTagSize;

    Sdc::Crease creasing(_options);

    //
    //  Inspect all vertices derived from edges -- for those whose parent edges were semisharp,
    //  reset the semisharp tag and the associated Rule according to the sharpness pair for the
    //  subdivided edges (note this may be better handled when the edge sharpness is computed):
    //
    Index vertFromEdgeBegin = getFirstChildVertexFromEdges();
    Index vertFromEdgeEnd   = vertFromEdgeBegin + getNumChildVerticesFromEdges();

    for (Index cVert = vertFromEdgeBegin; cVert < vertFromEdgeEnd; ++cVert) {
        Level::VTag& cVertTag = _child->_vertTags[cVert];
        if (!cVertTag._semiSharpEdges) continue;

        Index pEdge = _childVertexParentIndex[cVert];

        ConstIndexArray cEdges = getEdgeChildEdges(pEdge);

        if (_childVertexTag[cVert]._incomplete) {
            //  One child edge likely missing -- assume Crease if remaining edge semi-sharp:
            cVertTag._semiSharpEdges = (IndexIsValid(cEdges[0]) && _child->_edgeTags[cEdges[0]]._semiSharp) ||
                                       (IndexIsValid(cEdges[1]) && _child->_edgeTags[cEdges[1]]._semiSharp);
            cVertTag._rule = (VTagSize)(cVertTag._semiSharpEdges ? Sdc::Crease::RULE_CREASE : Sdc::Crease::RULE_SMOOTH);
        } else {
            int sharpEdgeCount = _child->_edgeTags[cEdges[0]]._semiSharp + _child->_edgeTags[cEdges[1]]._semiSharp;

            cVertTag._semiSharpEdges = (sharpEdgeCount > 0);
            cVertTag._rule = (VTagSize)(creasing.DetermineVertexVertexRule(0.0, sharpEdgeCount));
        }
    }

    //
    //  Inspect all vertices derived from vertices -- for those whose parent vertices were
    //  semisharp (inherited in the child vert's tag), inspect and reset the semisharp tag
    //  and the associated Rule (based on neighboring child edges around the child vertex).
    //
    //  We should never find such a vertex "incomplete" in a sparse refinement as a parent
    //  vertex is either selected or not, but never neighboring.  So the only complication
    //  here is whether the local topology of child edges exists -- it may have been pruned
    //  from the last level to reduce memory.  If so, we use the parent to identify the
    //  child edges.
    //
    //  In both cases, we count the number of sharp and semisharp child edges incident the
    //  child vertex and adjust the "semisharp" and "rule" tags accordingly.
    //
    Index vertFromVertBegin = getFirstChildVertexFromVertices();
    Index vertFromVertEnd   = vertFromVertBegin + getNumChildVerticesFromVertices();

    for (Index cVert = vertFromVertBegin; cVert < vertFromVertEnd; ++cVert) {
        Index pVert = _childVertexParentIndex[cVert];
        Level::VTag const& pVertTag = _parent->_vertTags[pVert];

        //  Skip if parent not semi-sharp:
        if (!pVertTag._semiSharp && !pVertTag._semiSharpEdges) continue;

        //
        //  We need to inspect the child neighborhood's sharpness when either semi-sharp
        //  edges were present around the parent vertex, or the parent vertex sharpness
        //  decayed:
        //
        Level::VTag& cVertTag = _child->_vertTags[cVert];

        bool sharpVertexDecayed = pVertTag._semiSharp && !cVertTag._semiSharp;

        if (pVertTag._semiSharpEdges || sharpVertexDecayed) {
            int infSharpEdgeCount = 0;
            int semiSharpEdgeCount = 0;

            bool cVertEdgesPresent = (_child->getNumVertexEdgesTotal() > 0);
            if (cVertEdgesPresent) {
                ConstIndexArray cEdges = _child->getVertexEdges(cVert);

                for (int i = 0; i < cEdges.size(); ++i) {
                    Level::ETag cEdgeTag = _child->_edgeTags[cEdges[i]];

                    infSharpEdgeCount  += cEdgeTag._infSharp;
                    semiSharpEdgeCount += cEdgeTag._semiSharp;
                }
            } else {
                ConstIndexArray      pEdges      = _parent->getVertexEdges(pVert);
                ConstLocalIndexArray pVertInEdge = _parent->getVertexEdgeLocalIndices(pVert);

                for (int i = 0; i < pEdges.size(); ++i) {
                    ConstIndexArray cEdgePair = getEdgeChildEdges(pEdges[i]);

                    Index       cEdge    = cEdgePair[pVertInEdge[i]];
                    Level::ETag cEdgeTag = _child->_edgeTags[cEdge];

                    infSharpEdgeCount  += cEdgeTag._infSharp;
                    semiSharpEdgeCount += cEdgeTag._semiSharp;
                }
            }
            cVertTag._semiSharpEdges = (semiSharpEdgeCount > 0);

            if (!cVertTag._semiSharp && !cVertTag._infSharp) {
                cVertTag._rule = (VTagSize)(creasing.DetermineVertexVertexRule(0.0,
                                        infSharpEdgeCount + semiSharpEdgeCount));
            }
        }
    }
}

//
//  Methods to subdivide face-varying channels:
//
void
Refinement::subdivideFVarChannels() {

    assert(_child->_fvarChannels.size() == 0);
    assert(this->_fvarChannels.size() == 0);

    int channelCount = _parent->getNumFVarChannels();

    for (int channel = 0; channel < channelCount; ++channel) {
        FVarLevel* parentFVar = _parent->_fvarChannels[channel];

        FVarLevel*      childFVar  = new FVarLevel(*_child);
        FVarRefinement* refineFVar = new FVarRefinement(*this, *parentFVar, *childFVar);

        refineFVar->applyRefinement();

        _child->_fvarChannels.push_back(childFVar);
        this->_fvarChannels.push_back(refineFVar);
    }
}

//
//  Marking of sparse child components -- including those selected and those neighboring...
//
//      For schemes requiring neighboring support, this is the equivalent of the "guarantee
//  neighbors" in Hbr -- it ensures that all components required to define the limit of
//  those "selected" are also generated in the refinement.
//
//  The difference with Hbr is that we do this in a single pass for all components once
//  "selection" of components of interest has been completed.
//
//  Considering two approaches:
//      1) By Vertex neighborhoods:
//          - for each base vertex
//              - for each incident face
//                  - test and mark components for its child face
//  or
//      2) By Edge and Face contents:
//          - for each base edge
//              - test and mark local components
//          - for each base face
//              - test and mark local components
//
//  Given a typical quad mesh with N verts, N faces and 2*N edges, determine which is more
//  efficient...
//
//  Going with (2) initially for simplicity -- certain aspects of (1) are awkward, i.e. the
//  identification of child-edges to be marked (trivial in (2).  We are also guaranteed with
//  (2) that we only visit each component once, i.e. each edge and each face.
//
//  Revising the above assessment... (2) has gotten WAY more complicated once the ability to
//  select child faces is provided.  Given that feature is important to Manuel for support
//  of the FarStencilTables we have to assume it will be needed.  So we'll try (1) out as it
//  will be simpler to get it correct -- we can work on improving performance later.
//
//  Complexity added by child component selection:
//      - the child vertex of the component can now be selected as part of a child face or
//  edge, and so the parent face or edge is not fully selected.  So we've had to add another
//  bit to the marking masks to indicate when a parent component is "fully selected".
//      - selecting a child face creates the situation where child edges of parent edges do
//  not have any selected vertex at their ends -- both can be neighboring.  This complicated
//  the marking of neighboring child edges, which was otherwise trivial -- if any end vertex
//  of a child edge (of a parent edge) was selected, the child edge was at least neighboring.
//
//  Final note on the marking technique:
//      There are currently two values to the marking of child components, which are no
//  longer that useful.  It is now sufficient, and not likely to be necessary, to distinguish
//  between what was selected or added to support it.  Ultimately that will be determined by
//  inspecting the selected flag on the parent component once the child-to-parent map is in
//  place.
//
namespace {
    Index const IndexSparseMaskNeighboring = (1 << 0);
    Index const IndexSparseMaskSelected    = (1 << 1);

    inline void markSparseIndexNeighbor(Index& index) { index = IndexSparseMaskNeighboring; }
    inline void markSparseIndexSelected(Index& index) { index = IndexSparseMaskSelected; }
}

void
Refinement::markSparseChildComponentIndices() {

    //
    //  There is an explicit ordering here as the work done for vertices is a subset
    //  of what is required for edges, which in turn is a subset of what is required
    //  for faces.  This ordering and their related implementations tries to avoid
    //  doing redundant work and accomplishing everything necessary in a single
    //  iteration through each component type.
    //
    markSparseVertexChildren();
    markSparseEdgeChildren();
    markSparseFaceChildren();
}


void
Refinement::markSparseVertexChildren() {

    assert(_parentVertexTag.size() > 0);

    //
    //  For each parent vertex:
    //      - mark the descending child vertex for each selected vertex
    //
    for (Index pVert = 0; pVert < parent().getNumVertices(); ++pVert) {
        if (_parentVertexTag[pVert]._selected) {
            markSparseIndexSelected(_vertChildVertIndex[pVert]);
        }
    }
}

void
Refinement::markSparseEdgeChildren() {

    assert(_parentEdgeTag.size() > 0);

    //
    //  For each parent edge:
    //      - mark the descending child edges and vertex for each selected edge
    //      - test each end vertex of unselected edges to see if selected:
    //          - mark both the child edge and the middle child vertex if so
    //      - set transitional bit for all edges based on selection of incident faces
    //
    //  Note that no edges have been marked "fully selected" -- only their vertices have
    //  been marked and marking of their child edges deferred to visiting each edge only
    //  once here.
    //
    for (Index pEdge = 0; pEdge < parent().getNumEdges(); ++pEdge) {
        IndexArray      eChildEdges = getEdgeChildEdges(pEdge);
        ConstIndexArray eVerts      = parent().getEdgeVertices(pEdge);

        SparseTag& pEdgeTag = _parentEdgeTag[pEdge];

        if (pEdgeTag._selected) {
            markSparseIndexSelected(eChildEdges[0]);
            markSparseIndexSelected(eChildEdges[1]);
            markSparseIndexSelected(_edgeChildVertIndex[pEdge]);
        } else {
            if (_parentVertexTag[eVerts[0]]._selected) {
                markSparseIndexNeighbor(eChildEdges[0]);
                markSparseIndexNeighbor(_edgeChildVertIndex[pEdge]);
            }
            if (_parentVertexTag[eVerts[1]]._selected) {
                markSparseIndexNeighbor(eChildEdges[1]);
                markSparseIndexNeighbor(_edgeChildVertIndex[pEdge]);
            }
        }

        //
        //  TAG the parent edges as "transitional" here if only one was selected (or in
        //  the more general non-manifold case, they are not all selected the same way).
        //  We use the transitional tags on the edges to TAG the parent face below.
        //
        //  Note -- this is best done now rather than as a post-process as we have more
        //  explicit information about the selected components.  Unless we also tag the
        //  parent faces as selected, we can't easily tell from the child-faces of the
        //  edge's incident faces which were generated by selection or neighboring...
        //
        ConstIndexArray eFaces = parent().getEdgeFaces(pEdge);
        if (eFaces.size() == 2) {
            pEdgeTag._transitional = (_parentFaceTag[eFaces[0]]._selected !=
                                      _parentFaceTag[eFaces[1]]._selected);
        } else if (eFaces.size() < 2) {
            pEdgeTag._transitional = false;
        } else {
            bool isFace0Selected = _parentFaceTag[eFaces[0]]._selected;

            pEdgeTag._transitional = false;
            for (int i = 1; i < eFaces.size(); ++i) {
                if (_parentFaceTag[eFaces[i]]._selected != isFace0Selected) {
                    pEdgeTag._transitional = true;
                    break;
                }
            }
        }
    }
}

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
