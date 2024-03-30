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
#include "../sdc/types.h"
#include "../sdc/crease.h"
#include "../vtr/array.h"
#include "../vtr/level.h"
#include "../vtr/refinement.h"
#include "../vtr/fvarLevel.h"
#include "../vtr/stackBuffer.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>
#include <map>

#ifdef _MSC_VER
    #define snprintf _snprintf
#endif

//
//  Level:
//      This is intended to be a fairly simple container of topology, sharpness and
//  other information that is useful to retain for subdivision.  It is intended to
//  be constructed by other friend classes, i.e. factories and class specialized to
//  construct topology based on various splitting schemes.  So its interface consists
//  of simple methods for inspection, and low-level protected methods for populating
//  it rather than high-level modifiers.
//
namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Vtr {
namespace internal {

//
//  Simple (for now) constructor and destructor:
//
Level::Level() :
    _faceCount(0),
    _edgeCount(0),
    _vertCount(0),
    _depth(0),
    _maxEdgeFaces(0),
    _maxValence(0) {
}

Level::~Level() {
    for (int i = 0; i < (int)_fvarChannels.size(); ++i) {
        delete _fvarChannels[i];
    }
}


char const *
Level::getTopologyErrorString(TopologyError errCode) {

    switch (errCode) {
        case TOPOLOGY_MISSING_EDGE_FACES :
            return "MISSING_EDGE_FACES";
        case TOPOLOGY_MISSING_EDGE_VERTS :
            return "MISSING_EDGE_VERTS";
        case TOPOLOGY_MISSING_FACE_EDGES :
            return "MISSING_FACE_EDGES";
        case TOPOLOGY_MISSING_FACE_VERTS :
            return "MISSING_FACE_VERTS";
        case TOPOLOGY_MISSING_VERT_FACES :
            return "MISSING_VERT_FACES";
        case TOPOLOGY_MISSING_VERT_EDGES :
            return "MISSING_VERT_EDGES";

        case TOPOLOGY_FAILED_CORRELATION_EDGE_FACE :
            return "FAILED_CORRELATION_EDGE_FACE";
        case TOPOLOGY_FAILED_CORRELATION_FACE_VERT :
            return "FAILED_CORRELATION_FACE_VERT";
        case TOPOLOGY_FAILED_CORRELATION_FACE_EDGE :
            return "FAILED_CORRELATION_FACE_EDGE";

        case TOPOLOGY_FAILED_ORIENTATION_INCIDENT_EDGE :
            return "FAILED_ORIENTATION_INCIDENT_EDGE";
        case TOPOLOGY_FAILED_ORIENTATION_INCIDENT_FACE :
            return "FAILED_ORIENTATION_INCIDENT_FACE";
        case TOPOLOGY_FAILED_ORIENTATION_INCIDENT_FACES_EDGES :
            return "FAILED_ORIENTATION_INCIDENT_FACES_EDGES";

        case TOPOLOGY_DEGENERATE_EDGE :
            return "DEGENERATE_EDGE";
        case TOPOLOGY_NON_MANIFOLD_EDGE :
            return "NON_MANIFOLD_EDGE";

        case TOPOLOGY_INVALID_CREASE_EDGE :
            return "INVALID_CREASE_EDGE";
        case TOPOLOGY_INVALID_CREASE_VERT :
            return "INVALID_CREASE_VERT";

        default:
            assert(0);
    }
    return 0;
}

//
//  Debugging method to validate topology, i.e. verify appropriate symmetry
//  between the relations, etc.
//
//  Additions that need to be made in the near term:
//      * verifying user-applied tags relating to topology:
//          - non-manifold in particular (ordering above can be part of this)
//          - face holes don't require anything
//      - verifying orientation of components, particularly vert-edges and faces:
//          - both need to be ordered correctly (when manifold)
//          - both need to be in sync for an interior vertex
//              ? is a rotation allowed for the interior case?
//              - I don't see why not...
//      ? verifying sharpness:
//          - values < Smooth or > Infinite
//          - sharpening of boundary edges (is this necessary, since we do it?)
//              - it does ensure our work was not corrupted by client assignments
//
//  Possibilities:
//      - single validate() method, which will call all of:
//          - validateTopology()
//          - validateSharpness()
//          - validateTagging()
//      - consider using a mask/struct to choose what to validate, i.e.:
//          - bool validate(ValidateOptions const& options) const;
//

#define REPORT(code, format, ...) \
    if (callback) { \
        char const * errStr = getTopologyErrorString(code); \
        char msg[1024]; \
        snprintf(msg, 1024, "%s - " format, errStr, ##__VA_ARGS__); \
        callback(code, msg, clientData); \
    }

bool
Level::validateTopology(ValidationCallback callback, void const * clientData) const {

    //
    //  Verify internal topological consistency (eventually a Level method?):
    //      - each face-vert has corresponding vert-face (and child)
    //      - each face-edge has corresponding edge-face
    //      - each edge-vert has corresponding vert-edge (and child)
    //  The above three are enough for most cases, but it is still possible
    //  the latter relation in each above has no correspondent in the former,
    //  so apply the symmetric tests:
    //      - each edge-face has corresponding face-edge
    //      - each vert-face has corresponding face-vert
    //      - each vert-edge has corresponding edge-vert
    //  We are still left with the possibility of duplicate references in
    //  places we don't want them.  Currently a component can exist multiple
    //  times in a component of higher dimension.
    //      - each vert-face <face,child> pair is unique
    //      - each vert-edge <edge,child> pair is unique
    //
    bool returnOnFirstError = true;
    bool isValid = true;

    //  Verify each face-vert has corresponding vert-face and child:
    if ((getNumFaceVerticesTotal() == 0) || (getNumVertexFacesTotal() == 0)) {
        if (getNumFaceVerticesTotal() == 0) {
            REPORT(TOPOLOGY_MISSING_FACE_VERTS, "missing face-verts");
        }
        if (getNumVertexFacesTotal() == 0) {
            REPORT(TOPOLOGY_MISSING_VERT_FACES, "missing vert-faces");
        }
        return false;
    }
    for (int fIndex = 0; fIndex < getNumFaces(); ++fIndex) {
        ConstIndexArray     fVerts      = getFaceVertices(fIndex);
        int                 fVertCount  = fVerts.size();

        for (int i = 0; i < fVertCount; ++i) {
            Index vIndex = fVerts[i];

            ConstIndexArray        vFaces = getVertexFaces(vIndex);
            ConstLocalIndexArray  vInFace = getVertexFaceLocalIndices(vIndex);

            bool vertFaceOfFaceExists = false;
            for (int j = 0; j < vFaces.size(); ++j) {
                if ((vFaces[j] == fIndex) && (vInFace[j] == i)) {
                    vertFaceOfFaceExists = true;
                    break;
                }
            }
            if (!vertFaceOfFaceExists) {
                REPORT(TOPOLOGY_FAILED_CORRELATION_FACE_VERT,
                    "face %d correlation of vert %d failed", fIndex, i);
                if (returnOnFirstError) return false;
                isValid = false;
            }
        }
    }

    //  Verify each face-edge has corresponding edge-face:
    if ((getNumEdgeFacesTotal() == 0) || (getNumFaceEdgesTotal() == 0)) {
        if (getNumEdgeFacesTotal() == 0) {
            REPORT(TOPOLOGY_MISSING_EDGE_FACES, "missing edge-faces");
        }
        if (getNumFaceEdgesTotal() == 0) {
            REPORT(TOPOLOGY_MISSING_FACE_EDGES, "missing face-edges");
        }
        return false;
    }
    for (int fIndex = 0; fIndex < getNumFaces(); ++fIndex) {
        ConstIndexArray  fEdges      = getFaceEdges(fIndex);
        int              fEdgeCount  = fEdges.size();

        for (int i = 0; i < fEdgeCount; ++i) {
            int eIndex = fEdges[i];

            ConstIndexArray       eFaces = getEdgeFaces(eIndex);
            ConstLocalIndexArray eInFace = getEdgeFaceLocalIndices(eIndex);

            bool edgeFaceOfFaceExists = false;
            for (int j = 0; j < eFaces.size(); ++j) {
                if ((eFaces[j] == fIndex) && (eInFace[j] == i)) {
                    edgeFaceOfFaceExists = true;
                    break;
                }
            }
            if (!edgeFaceOfFaceExists) {
                REPORT(TOPOLOGY_FAILED_CORRELATION_FACE_EDGE,
                     "face %d correlation of edge %d failed", fIndex, i);
                if (returnOnFirstError) return false;
                isValid = false;
            }
        }
    }

    //  Verify each edge-vert has corresponding vert-edge and child:
    if ((getNumEdgeVerticesTotal() == 0) || (getNumVertexEdgesTotal() == 0)) {
        if (getNumEdgeVerticesTotal() == 0) {
            REPORT(TOPOLOGY_MISSING_EDGE_VERTS, "missing edge-verts");
        }
        if (getNumVertexEdgesTotal() == 0) {
            REPORT(TOPOLOGY_MISSING_VERT_EDGES, "missing vert-edges");
        }
        return false;
    }
    for (int eIndex = 0; eIndex < getNumEdges(); ++eIndex) {
        ConstIndexArray  eVerts = getEdgeVertices(eIndex);

        for (int i = 0; i < 2; ++i) {
            Index vIndex = eVerts[i];

            ConstIndexArray       vEdges = getVertexEdges(vIndex);
            ConstLocalIndexArray  vInEdge = getVertexEdgeLocalIndices(vIndex);

            bool vertEdgeOfEdgeExists = false;
            for (int j = 0; j < vEdges.size(); ++j) {
                if ((vEdges[j] == eIndex) && (vInEdge[j] == i)) {
                    vertEdgeOfEdgeExists = true;
                    break;
                }
            }
            if (!vertEdgeOfEdgeExists) {
                REPORT(TOPOLOGY_FAILED_CORRELATION_FACE_VERT,
                    "edge %d correlation of vert %d failed", eIndex, i);
                if (returnOnFirstError) return false;
                isValid = false;
            }
        }
    }

    //  Verify that vert-faces and vert-edges are properly ordered and in sync:
    //      - currently this requires the relations exactly match those that we construct from
    //        the ordering method, i.e. we do not allow rotations for interior vertices.
    internal::StackBuffer<Index,32> indexBuffer(2 * _maxValence);

    for (int vIndex = 0; vIndex < getNumVertices(); ++vIndex) {
        if (_vertTags[vIndex]._incomplete || _vertTags[vIndex]._nonManifold) continue;

        ConstIndexArray  vFaces = getVertexFaces(vIndex);
        ConstIndexArray  vEdges = getVertexEdges(vIndex);

        Index * vFacesOrdered = indexBuffer;
        Index * vEdgesOrdered = indexBuffer + vFaces.size();

        if (!orderVertexFacesAndEdges(vIndex, vFacesOrdered, vEdgesOrdered)) {
            REPORT(TOPOLOGY_FAILED_ORIENTATION_INCIDENT_FACES_EDGES,
                "vertex %d cannot orient incident faces and edges", vIndex);
            if (returnOnFirstError) return false;
            isValid = false;
        }
        for (int i = 0; i < vFaces.size(); ++i) {
            if (vFaces[i] != vFacesOrdered[i]) {
                REPORT(TOPOLOGY_FAILED_ORIENTATION_INCIDENT_FACE,
                    "vertex %d orientation failure at incident face %d", vIndex, i);
                if (returnOnFirstError) return false;
                isValid = false;
                break;
            }
        }
        for (int i = 0; i < vEdges.size(); ++i) {
            if (vEdges[i] != vEdgesOrdered[i]) {
                REPORT(TOPOLOGY_FAILED_ORIENTATION_INCIDENT_EDGE,
                    "vertex %d orientation failure at incident edge %d", vIndex, i);
                if (returnOnFirstError) return false;
                isValid = false;
                break;
            }
        }
    }

    //  Verify non-manifold tags are appropriately assigned to edges and vertices:
    //      - note we have to validate orientation of vertex neighbors to do this rigorously
    for (int eIndex = 0; eIndex < getNumEdges(); ++eIndex) {
        Level::ETag const& eTag = _edgeTags[eIndex];
        if (eTag._nonManifold) continue;

        ConstIndexArray  eVerts = getEdgeVertices(eIndex);
        if (eVerts[0] == eVerts[1]) {
            REPORT(TOPOLOGY_DEGENERATE_EDGE,
                "Error in eIndex = %d:  degenerate edge not tagged marked non-manifold", eIndex);
            if (returnOnFirstError) return false;
            isValid = false;
        }

        ConstIndexArray  eFaces = getEdgeFaces(eIndex);
        if ((eFaces.size() < 1) || (eFaces.size() > 2)) {
            REPORT(TOPOLOGY_NON_MANIFOLD_EDGE,
                "edge %d with %d incident faces not tagged non-manifold", eIndex, eFaces.size());
            if (returnOnFirstError) return false;
            isValid = false;
        }
    }
    return isValid;
}

//
//  Anonymous helper functions for debugging output -- yes, using printf(), this is not
//  intended to serve anyone other than myself for now and I favor its formatting control
//
namespace {
    template <typename INT_TYPE>
    void
    printIndexArray(ConstArray<INT_TYPE> const& array) {

        printf("%d [%d", array.size(), array[0]);
        for (int i = 1; i < array.size(); ++i) {
            printf(" %d", array[i]);
        }
        printf("]\n");
    }

    const char*
    ruleString(Sdc::Crease::Rule rule) {
        switch (rule) {
            case Sdc::Crease::RULE_UNKNOWN: return "<uninitialized>";
            case Sdc::Crease::RULE_SMOOTH:  return "Smooth";
            case Sdc::Crease::RULE_DART:    return "Dart";
            case Sdc::Crease::RULE_CREASE:  return "Crease";
            case Sdc::Crease::RULE_CORNER:  return "Corner";
            default:
                assert(0);
        }
        return 0;
    }

#ifdef __INTEL_COMPILER
#pragma warning (push)
#pragma warning disable 1572
#endif
    inline bool isSharpnessEqual(float s1, float s2) { return (s1 == s2); }
#ifdef __INTEL_COMPILER
#pragma warning (pop)
#endif
}

void
Level::print(const Refinement* pRefinement) const {

    bool printFaceVerts      = true;
    bool printFaceEdges      = true;
    bool printFaceChildVerts = false;
    bool printFaceTags       = true;

    bool printEdgeVerts      = true;
    bool printEdgeFaces      = true;
    bool printEdgeChildVerts = true;
    bool printEdgeSharpness  = true;
    bool printEdgeTags       = true;

    bool printVertFaces      = true;
    bool printVertEdges      = true;
    bool printVertChildVerts = false;
    bool printVertSharpness  = true;
    bool printVertTags       = true;

    printf("Level (0x%p):\n", this);
    printf("  Depth = %d\n", _depth);

    printf("  Primary component counts:\n");
    printf("    faces = %d\n", _faceCount);
    printf("    edges = %d\n", _edgeCount);
    printf("    verts = %d\n", _vertCount);

    printf("  Topology relation sizes:\n");

    printf("    Face relations:\n");
    printf("      face-vert counts/offset = %lu\n", (unsigned long)_faceVertCountsAndOffsets.size());
    printf("      face-vert indices = %lu\n", (unsigned long)_faceVertIndices.size());
    if (_faceVertIndices.size()) {
        for (int i = 0; printFaceVerts && i < getNumFaces(); ++i) {
            printf("        face %4d verts:  ", i);
            printIndexArray(getFaceVertices(i));
        }
    }
    printf("      face-edge indices = %lu\n", (unsigned long)_faceEdgeIndices.size());
    if (_faceEdgeIndices.size()) {
        for (int i = 0; printFaceEdges && i < getNumFaces(); ++i) {
            printf("        face %4d edges:  ", i);
            printIndexArray(getFaceEdges(i));
        }
    }
    printf("      face tags = %lu\n", (unsigned long)_faceTags.size());
    for (int i = 0; printFaceTags && i < (int)_faceTags.size(); ++i) {
        FTag const& fTag = _faceTags[i];
        printf("        face %4d:", i);
        printf("  hole = %d",  (int)fTag._hole);
        printf("\n");
    }
    if (pRefinement) {
        printf("      face child-verts = %lu\n", (unsigned long)pRefinement->_faceChildVertIndex.size());
        for (int i = 0; printFaceChildVerts && i < (int)pRefinement->_faceChildVertIndex.size(); ++i) {
            printf("        face %4d child vert:  %d\n", i, pRefinement->_faceChildVertIndex[i]);
        }
    }

    printf("    Edge relations:\n");
    printf("      edge-vert indices = %lu\n", (unsigned long)_edgeVertIndices.size());
    if (_edgeVertIndices.size()) {
        for (int i = 0; printEdgeVerts && i < getNumEdges(); ++i) {
            printf("        edge %4d verts:  ", i);
            printIndexArray(getEdgeVertices(i));
        }
    }
    printf("      edge-face counts/offset = %lu\n", (unsigned long)_edgeFaceCountsAndOffsets.size());
    printf("      edge-face indices       = %lu\n", (unsigned long)_edgeFaceIndices.size());
    printf("      edge-face local-indices = %lu\n", (unsigned long)_edgeFaceLocalIndices.size());
    if (_edgeFaceIndices.size()) {
        for (int i = 0; printEdgeFaces && i < getNumEdges(); ++i) {
            printf("        edge %4d faces:  ", i);
            printIndexArray(getEdgeFaces(i));

            printf("             face-edges:  ");
            printIndexArray(getEdgeFaceLocalIndices(i));
        }
    }
    if (pRefinement) {
        printf("      edge child-verts = %lu\n", (unsigned long)pRefinement->_edgeChildVertIndex.size());
        for (int i = 0; printEdgeChildVerts && i < (int)pRefinement->_edgeChildVertIndex.size(); ++i) {
            printf("        edge %4d child vert:  %d\n", i, pRefinement->_edgeChildVertIndex[i]);
        }
    }
    printf("      edge sharpness = %lu\n", (unsigned long)_edgeSharpness.size());
    for (int i = 0; printEdgeSharpness && i < (int)_edgeSharpness.size(); ++i) {
        printf("        edge %4d sharpness:  %f\n", i, _edgeSharpness[i]);
    }
    printf("      edge tags = %lu\n", (unsigned long)_edgeTags.size());
    for (int i = 0; printEdgeTags && i < (int)_edgeTags.size(); ++i) {
        ETag const& eTag = _edgeTags[i];
        printf("        edge %4d:", i);
        printf("  boundary = %d",  (int)eTag._boundary);
        printf(", nonManifold = %d", (int)eTag._nonManifold);
        printf(", semiSharp = %d", (int)eTag._semiSharp);
        printf(", infSharp = %d",  (int)eTag._infSharp);
        printf("\n");
    }

    printf("    Vert relations:\n");
    printf("      vert-face counts/offset = %lu\n", (unsigned long)_vertFaceCountsAndOffsets.size());
    printf("      vert-face indices       = %lu\n", (unsigned long)_vertFaceIndices.size());
    printf("      vert-face local-indices = %lu\n", (unsigned long)_vertFaceLocalIndices.size());
    if (_vertFaceIndices.size()) {
        for (int i = 0; printVertFaces && i < getNumVertices(); ++i) {
            printf("        vert %4d faces:  ", i);
            printIndexArray(getVertexFaces(i));

            printf("             face-verts:  ");
            printIndexArray(getVertexFaceLocalIndices(i));
        }
    }
    printf("      vert-edge counts/offset = %lu\n", (unsigned long)_vertEdgeCountsAndOffsets.size());
    printf("      vert-edge indices       = %lu\n", (unsigned long)_vertEdgeIndices.size());
    printf("      vert-edge local-indices = %lu\n", (unsigned long)_vertEdgeLocalIndices.size());
    if (_vertEdgeIndices.size()) {
        for (int i = 0; printVertEdges && i < getNumVertices(); ++i) {
            printf("        vert %4d edges:  ", i);
            printIndexArray(getVertexEdges(i));

            printf("             edge-verts:  ");
            printIndexArray(getVertexEdgeLocalIndices(i));
        }
    }
    if (pRefinement) {
        printf("      vert child-verts = %lu\n", (unsigned long)pRefinement->_vertChildVertIndex.size());
        for (int i = 0; printVertChildVerts && i < (int)pRefinement->_vertChildVertIndex.size(); ++i) {
            printf("        vert %4d child vert:  %d\n", i, pRefinement->_vertChildVertIndex[i]);
        }
    }
    printf("      vert sharpness = %lu\n", (unsigned long)_vertSharpness.size());
    for (int i = 0; printVertSharpness && i < (int)_vertSharpness.size(); ++i) {
        printf("        vert %4d sharpness:  %f\n", i, _vertSharpness[i]);
    }
    printf("      vert tags = %lu\n", (unsigned long)_vertTags.size());
    for (int i = 0; printVertTags && i < (int)_vertTags.size(); ++i) {
        VTag const& vTag = _vertTags[i];
        printf("        vert %4d:", i);
        printf("  rule = %s",           ruleString((Sdc::Crease::Rule)vTag._rule));
        printf(", boundary = %d",       (int)vTag._boundary);
        printf(", corner = %d",         (int)vTag._corner);
        printf(", xordinary = %d",      (int)vTag._xordinary);
        printf(", nonManifold = %d",    (int)vTag._nonManifold);
        printf(", infSharp = %d",       (int)vTag._infSharp);
        printf(", infSharpEdges = %d",  (int)vTag._infSharpEdges);
        printf(", infSharpCrease = %d", (int)vTag._infSharpCrease);
        printf(", infIrregular = %d",   (int)vTag._infIrregular);
        printf(", semiSharp = %d",      (int)vTag._semiSharp);
        printf(", semiSharpEdges = %d", (int)vTag._semiSharpEdges);
        printf("\n");
    }
    fflush(stdout);
}

//
//  Methods for retrieving and combining tags:
//
bool
Level::doesVertexFVarTopologyMatch(Index vIndex, int fvarChannel) const {

    return getFVarLevel(fvarChannel).valueTopologyMatches(
             getFVarLevel(fvarChannel).getVertexValueOffset(vIndex));
}
bool
Level::doesEdgeFVarTopologyMatch(Index eIndex, int fvarChannel) const {

    return getFVarLevel(fvarChannel).edgeTopologyMatches(eIndex);
}
bool
Level::doesFaceFVarTopologyMatch(Index fIndex, int fvarChannel) const {

    return ! getFVarLevel(fvarChannel).getFaceCompositeValueTag(fIndex).isMismatch();
}

void
Level::getFaceVTags(Index fIndex, VTag vTags[], int fvarChannel) const {

    ConstIndexArray fVerts = getFaceVertices(fIndex);
    if (fvarChannel < 0) {
        for (int i = 0; i < fVerts.size(); ++i) {
            vTags[i] = getVertexTag(fVerts[i]);
        }
    } else {
        FVarLevel const & fvarLevel = getFVarLevel(fvarChannel);
        ConstIndexArray fValues = fvarLevel.getFaceValues(fIndex);
        for (int i = 0; i < fVerts.size(); ++i) {
            Index valueIndex = fvarLevel.findVertexValueIndex(fVerts[i], fValues[i]);
            FVarLevel::ValueTag valueTag = fvarLevel.getValueTag(valueIndex);

            vTags[i] = valueTag.combineWithLevelVTag(getVertexTag(fVerts[i]));
        }
    }
}
void
Level::getFaceETags(Index fIndex, ETag eTags[], int fvarChannel) const {

    ConstIndexArray fEdges = getFaceEdges(fIndex);
    if (fvarChannel < 0) {
        for (int i = 0; i < fEdges.size(); ++i) {
            eTags[i] = getEdgeTag(fEdges[i]);
        }
    } else {
        FVarLevel const & fvarLevel = getFVarLevel(fvarChannel);
        for (int i = 0; i < fEdges.size(); ++i) {
            FVarLevel::ETag fvarETag = fvarLevel.getEdgeTag(fEdges[i]);

            eTags[i] = fvarETag.combineWithLevelETag(getEdgeTag(fEdges[i]));
        }
    }
}

Level::VTag
Level::VTag::BitwiseOr(VTag const vTags[], int size) {

    VTag::VTagSize tagBits = vTags[0].getBits();
    for (int i = 1; i < size; ++i) {
        tagBits |= vTags[i].getBits();
    }
    return VTag(tagBits);
}
Level::ETag
Level::ETag::BitwiseOr(ETag const eTags[], int size) {

    ETag::ETagSize tagBits = eTags[0].getBits();
    for (int i = 1; i < size; ++i) {
        tagBits |= eTags[i].getBits();
    }
    return ETag(tagBits);
}

Level::VTag
Level::getFaceCompositeVTag(ConstIndexArray & fVerts) const {

    VTag::VTagSize tagBits = _vertTags[fVerts[0]].getBits();
    for (int i = 1; i < fVerts.size(); ++i) {
        tagBits |= _vertTags[fVerts[i]].getBits();
    }
    return VTag(tagBits);
}
Level::VTag
Level::getFaceCompositeVTag(Index fIndex, int fvarChannel) const {

    ConstIndexArray fVerts = getFaceVertices(fIndex);
    if (fvarChannel < 0) {
        return getFaceCompositeVTag(fVerts);
    } else {
        FVarLevel const & fvarLevel = getFVarLevel(fvarChannel);
        internal::StackBuffer<FVarLevel::ValueTag,64> fvarTags(fVerts.size());
        fvarLevel.getFaceValueTags(fIndex, fvarTags);

        VTag::VTagSize tagBits = fvarTags[0].combineWithLevelVTag(_vertTags[fVerts[0]]).getBits();
        for (int i = 1; i < fVerts.size(); ++i) {
            tagBits |= fvarTags[i].combineWithLevelVTag(_vertTags[fVerts[i]]).getBits();
        }
        return VTag(tagBits);
    }
}

Level::VTag
Level::getVertexCompositeFVarVTag(Index vIndex, int fvarChannel) const {

    FVarLevel const & fvarLevel = getFVarLevel(fvarChannel);

    FVarLevel::ConstValueTagArray fvTags = fvarLevel.getVertexValueTags(vIndex);

    VTag vTag = getVertexTag(vIndex);
    if (fvTags[0].isMismatch()) {
        VTag::VTagSize tagBits = fvTags[0].combineWithLevelVTag(vTag).getBits();
        for (int i = 1; i < fvTags.size(); ++i) {
            tagBits |= fvTags[i].combineWithLevelVTag(vTag).getBits();
        }
        return VTag(tagBits);
    } else {
        return vTag;
    }
}

//
//  High-level topology gathering functions -- used mainly in patch construction.  These
//  may eventually be moved elsewhere, possibly to classes specialized for quad- and tri-
//  patch identification and construction, but for now somewhere more accessible than the
//  patch tables factory is preferable.
//
//  Note a couple of nuisances...
//      - debatable whether we should include the face's face-verts in the face functions
//          - we refer to the result as a "patch" when we do
//          - otherwise a "ring" of vertices is more appropriate
//
namespace {
    template <typename INT_TYPE>
    inline INT_TYPE fastMod4(INT_TYPE value) {

        return (value & 0x3);
    }

    template <class ARRAY_OF_TYPE, class TYPE>
    inline TYPE otherOfTwo(ARRAY_OF_TYPE const& arrayOfTwo, TYPE const& value) {

        return arrayOfTwo[value == arrayOfTwo[0]];
    }
}

//
//  Gathering the one-ring of vertices from quads surrounding a vertex:
//      - the neighborhood of the vertex is assumed to be quad-regular (manifold)
//
//  Ordering of resulting vertices:
//      The surrounding one-ring follows the ordering of the incident faces.  For each
//  incident quad, the two vertices in CCW order within that quad are added.  If the
//  vertex is on a boundary, a third vertex on the boundary edge will be contributed from
//  the last face.
//
int
Level::gatherQuadRegularRingAroundVertex(
    Index vIndex, int ringPoints[], int fvarChannel) const {

    Level const& level = *this;

    ConstIndexArray vEdges = level.getVertexEdges(vIndex);

    ConstIndexArray vFaces = level.getVertexFaces(vIndex);
    ConstLocalIndexArray vInFaces = level.getVertexFaceLocalIndices(vIndex);

    bool isBoundary = (vEdges.size() > vFaces.size());

    int ringIndex = 0;
    for (int i = 0; i < vFaces.size(); ++i) {
        //
        //  For every incident quad, we want the two vertices clockwise in each face, i.e.
        //  the vertex at the end of the leading edge and the vertex opposite this one:
        //
        ConstIndexArray fPoints = (fvarChannel < 0)
                                ? level.getFaceVertices(vFaces[i])
                                : level.getFaceFVarValues(vFaces[i], fvarChannel);

        int vInThisFace = vInFaces[i];

        ringPoints[ringIndex++] = fPoints[fastMod4(vInThisFace + 1)];
        ringPoints[ringIndex++] = fPoints[fastMod4(vInThisFace + 2)];

        if (isBoundary && (i == (vFaces.size() - 1))) {
            ringPoints[ringIndex++] = fPoints[fastMod4(vInThisFace + 3)];
        }
    }
    return ringIndex;
}

int
Level::gatherQuadRegularPartialRingAroundVertex(
    Index vIndex, VSpan const & span, int ringPoints[], int fvarChannel) const {

    Level const& level = *this;

    assert(! level.isVertexNonManifold(vIndex));

    ConstIndexArray      vFaces   = level.getVertexFaces(vIndex);
    ConstLocalIndexArray vInFaces = level.getVertexFaceLocalIndices(vIndex);

    int nFaces    = span._numFaces;
    int startFace = span._startFace;

    int ringIndex = 0;
    for (int i = 0; i < nFaces; ++i) {
        //
        //  For every incident quad, we want the two vertices clockwise in each face, i.e.
        //  the vertex at the end of the leading edge and the vertex opposite this one:
        //
        int fIncident = (startFace + i) % vFaces.size();

        ConstIndexArray fPoints = (fvarChannel < 0)
                                ? level.getFaceVertices(vFaces[fIncident])
                                : level.getFaceFVarValues(vFaces[fIncident], fvarChannel);

        int vInThisFace = vInFaces[fIncident];

        ringPoints[ringIndex++] = fPoints[fastMod4(vInThisFace + 1)];
        ringPoints[ringIndex++] = fPoints[fastMod4(vInThisFace + 2)];

        if ((i == nFaces - 1) && !span._periodic) {
            ringPoints[ringIndex++] = fPoints[fastMod4(vInThisFace + 3)];
        }
    }
    return ringIndex;
}

//
//  Gathering the 4 vertices of a quad:
//      
//        |     |  
//      --0-----3--
//        |x   x|  
//        |x   x|  
//      --1-----2--
//        |     |  
//      
int
Level::gatherQuadLinearPatchPoints(
    Index thisFace, Index patchPoints[], int rotation, int fvarChannel) const {

    Level const& level = *this;

    assert((0 <= rotation) && (rotation < 4));
    static int const   rotationSequence[7] = { 0, 1, 2, 3, 0, 1, 2 };
    int const * rotatedVerts = &rotationSequence[rotation];

    ConstIndexArray facePoints = (fvarChannel < 0) ?
                                 level.getFaceVertices(thisFace) :
                                 level.getFaceFVarValues(thisFace, fvarChannel);

    patchPoints[0] = facePoints[rotatedVerts[0]];
    patchPoints[1] = facePoints[rotatedVerts[1]];
    patchPoints[2] = facePoints[rotatedVerts[2]];
    patchPoints[3] = facePoints[rotatedVerts[3]];

    return 4;
}

//
//  Gathering the 16 vertices of a quad-regular interior patch:
//      - the neighborhood of the face is assumed to be quad-regular
//
//  Ordering of resulting vertices:
//      It was debatable whether to include the vertices of the original face for a complete
//  "patch" or just the surrounding ring -- clearly we ended up with a function for the entire
//  patch, but that may change.
//      The latter ring of vertices around the face (potentially returned on its own) was
//  oriented with respect to the face.  The ring of 12 vertices is gathered as 4 groups of 3
//  vertices -- one for each corner vertex, and each group forming the quad opposite each
//  corner vertex when combined with that corner vertex.  The four vertices of the face begin
//  the patch.
//
//         |     |     |     |
//      ---5-----4-----15----14---
//         |     |     |     |
//         |     |     |     |
//      ---6-----0-----3-----13---
//         |     |x   x|     |
//         |     |x   x|     |
//      ---7-----1-----2-----12---
//         |     |     |     |
//         |     |     |     |
//      ---8-----9-----10----11---
//         |     |     |     |
//
int
Level::gatherQuadRegularInteriorPatchPoints(
    Index thisFace, Index patchPoints[], int rotation, int fvarChannel) const {

    Level const& level = *this;

    assert((0 <= rotation) && (rotation < 4));
    static int const   rotationSequence[7] = { 0, 1, 2, 3, 0, 1, 2 };
    int const * rotatedVerts = &rotationSequence[rotation];

    ConstIndexArray thisFaceVerts = level.getFaceVertices(thisFace);

    ConstIndexArray facePoints = (fvarChannel < 0) ? thisFaceVerts :
                                 level.getFaceFVarValues(thisFace, fvarChannel);

    patchPoints[0] = facePoints[rotatedVerts[0]];
    patchPoints[1] = facePoints[rotatedVerts[1]];
    patchPoints[2] = facePoints[rotatedVerts[2]];
    patchPoints[3] = facePoints[rotatedVerts[3]];

    //
    //  For each of the four corner vertices, there is a face diagonally opposite
    //  the given/central face.  Each of these faces contains three points of the
    //  entire ring of points around that given/central face.
    //
    int pointIndex = 4;
    for (int i = 0; i < 4; ++i) {
        Index v = thisFaceVerts[rotatedVerts[i]];

        ConstIndexArray      vFaces   = level.getVertexFaces(v);
        ConstLocalIndexArray vInFaces = level.getVertexFaceLocalIndices(v);

        int thisFaceInVFaces = vFaces.FindIndexIn4Tuple(thisFace);
        int intFaceInVFaces  = fastMod4(thisFaceInVFaces + 2);

        Index intFace    = vFaces[intFaceInVFaces];
        int   vInIntFace = vInFaces[intFaceInVFaces];

        facePoints = (fvarChannel < 0) ? level.getFaceVertices(intFace) :
                     level.getFaceFVarValues(intFace, fvarChannel);

        patchPoints[pointIndex++] = facePoints[fastMod4(vInIntFace + 1)];
        patchPoints[pointIndex++] = facePoints[fastMod4(vInIntFace + 2)];
        patchPoints[pointIndex++] = facePoints[fastMod4(vInIntFace + 3)];
    }
    assert(pointIndex == 16);
    return 16;
}

//
//  Gathering the 12 vertices of a quad-regular boundary patch:
//      - the neighborhood of the face is assumed to be quad-regular
//      - the single edge of the face that lies on the boundary is specified
//      - only one edge of the face is a boundary edge
//
//  Ordering of resulting vertices:
//      It was debatable whether to include the vertices of the original face for a complete
//  "patch" or just the surrounding ring -- clearly we ended up with a function for the entire
//  patch, but that may change.
//      The latter ring of vertices around the face (potentially returned on its own) was
//  oriented beginning from the leading CCW boundary edge and ending at the trailing edge.
//  The four vertices of the face begin the patch and are oriented similarly to this outer
//  ring -- forming an inner ring that begins and ends in the same manner.
//
//      ---4-----0-----3-----11---
//         |     |x   x|     |
//         |     |x   x|     |
//      ---5-----1-----2-----10---
//         |     |v0 v1|     |
//         |     |     |     |
//      ---6-----7-----8-----9----
//         |     |     |     |
//
int
Level::gatherQuadRegularBoundaryPatchPoints(
    Index face, Index patchPoints[], int boundaryEdgeInFace, int fvarChannel) const {

    Level const& level = *this;

    int interiorEdgeInFace = fastMod4(boundaryEdgeInFace + 2);

    //
    //  V0 and V1 are the two interior vertices (opposite the boundary edge) around
    //  which we will gather most of the ring:
    //
    int intV0InFace = interiorEdgeInFace;
    int intV1InFace = fastMod4(interiorEdgeInFace + 1);

    ConstIndexArray faceVerts = level.getFaceVertices(face);

    Index v0 = faceVerts[intV0InFace];
    Index v1 = faceVerts[intV1InFace];

    ConstIndexArray v0Faces = level.getVertexFaces(v0);
    ConstIndexArray v1Faces = level.getVertexFaces(v1);

    ConstLocalIndexArray v0InFaces = level.getVertexFaceLocalIndices(v0);
    ConstLocalIndexArray v1InFaces = level.getVertexFaceLocalIndices(v1);

    int boundaryFaceInV0Faces = -1;
    int boundaryFaceInV1Faces = -1;
    for (int i = 0; i < 4; ++i) {
        if (face == v0Faces[i]) boundaryFaceInV0Faces = i;
        if (face == v1Faces[i]) boundaryFaceInV1Faces = i;
    }
    assert((boundaryFaceInV0Faces >= 0) && (boundaryFaceInV1Faces >= 0));

    //  Identify the four faces of interest -- previous to and opposite V0 and
    //  opposite and next from V1 -- relative to V0 and V1:
    int prevFaceInV0Faces = fastMod4(boundaryFaceInV0Faces + 1);
    int intFaceInV0Faces  = fastMod4(boundaryFaceInV0Faces + 2);
    int intFaceInV1Faces  = fastMod4(boundaryFaceInV1Faces + 2);
    int nextFaceInV1Faces = fastMod4(boundaryFaceInV1Faces + 3);

    //  Identify the indices of the four faces:
    Index prevFace  = v0Faces[prevFaceInV0Faces];
    Index intV0Face = v0Faces[intFaceInV0Faces];
    Index intV1Face = v1Faces[intFaceInV1Faces];
    Index nextFace  = v1Faces[nextFaceInV1Faces];

    //  Identify V0 and V1 relative to these four faces:
    LocalIndex v0InPrevFace = v0InFaces[prevFaceInV0Faces];
    LocalIndex v0InIntFace  = v0InFaces[intFaceInV0Faces];
    LocalIndex v1InIntFace  = v1InFaces[intFaceInV1Faces];
    LocalIndex v1InNextFace = v1InFaces[nextFaceInV1Faces];

    //
    //  Now that all faces of interest have been found, identify the point
    //  indices within each face (i.e. the vertex or fvar-value index arrays)
    //  and copy them into the patch points:
    //
    ConstIndexArray thisFacePoints,
                    prevFacePoints,
                    intV0FacePoints,
                    intV1FacePoints,
                    nextFacePoints;

    if (fvarChannel < 0) {
        thisFacePoints  = faceVerts;
        prevFacePoints  = level.getFaceVertices(prevFace);
        intV0FacePoints = level.getFaceVertices(intV0Face);
        intV1FacePoints = level.getFaceVertices(intV1Face);
        nextFacePoints  = level.getFaceVertices(nextFace);
    } else {
        thisFacePoints  = level.getFaceFVarValues(face, fvarChannel);
        prevFacePoints  = level.getFaceFVarValues(prevFace, fvarChannel);
        intV0FacePoints = level.getFaceFVarValues(intV0Face, fvarChannel);
        intV1FacePoints = level.getFaceFVarValues(intV1Face, fvarChannel);
        nextFacePoints  = level.getFaceFVarValues(nextFace, fvarChannel);
    }

    patchPoints[0] = thisFacePoints[fastMod4(boundaryEdgeInFace + 1)];
    patchPoints[1] = thisFacePoints[fastMod4(boundaryEdgeInFace + 2)];
    patchPoints[2] = thisFacePoints[fastMod4(boundaryEdgeInFace + 3)];
    patchPoints[3] = thisFacePoints[         boundaryEdgeInFace];

    patchPoints[4] = prevFacePoints[fastMod4(v0InPrevFace + 2)];

    patchPoints[5] = intV0FacePoints[fastMod4(v0InIntFace + 1)];
    patchPoints[6] = intV0FacePoints[fastMod4(v0InIntFace + 2)];
    patchPoints[7] = intV0FacePoints[fastMod4(v0InIntFace + 3)];

    patchPoints[8]  = intV1FacePoints[fastMod4(v1InIntFace + 1)];
    patchPoints[9]  = intV1FacePoints[fastMod4(v1InIntFace + 2)];
    patchPoints[10] = intV1FacePoints[fastMod4(v1InIntFace + 3)];

    patchPoints[11] = nextFacePoints[fastMod4(v1InNextFace + 2)];

    return 12;
}

//
//  Gathering the 9 vertices of a quad-regular corner patch:
//      - the neighborhood of the face is assumed to be quad-regular
//      - the single corner vertex is specified
//      - only one vertex of the face is a corner
//
//  Ordering of resulting vertices:
//      It was debatable whether to include the vertices of the original face for a complete
//  "patch" or just the surrounding ring -- clearly we ended up with a function for the entire
//  patch, but that may change.
//      Like the boundary case, the latter ring of vertices around the face was oriented
//  beginning from the leading CCW boundary edge and ending at the trailing edge.  The four
//  face vertices begin the patch, and begin with the corner vertex.
//
//      0-----3-----8---
//      |x   x|     |
//      |x   x|     |
//      1-----2-----7---
//      |     |     |
//      |     |     |
//      4-----5-----6---
//      |     |     |
//
int
Level::gatherQuadRegularCornerPatchPoints(
    Index face, Index patchPoints[], int cornerVertInFace, int fvarChannel) const {

    Level const& level = *this;

    int interiorFaceVert = fastMod4(cornerVertInFace + 2);

    ConstIndexArray faceVerts = level.getFaceVertices(face);
    Index intVert = faceVerts[interiorFaceVert];

    ConstIndexArray      intVertFaces   = level.getVertexFaces(intVert);
    ConstLocalIndexArray intVertInFaces = level.getVertexFaceLocalIndices(intVert);

    int cornerFaceInIntVertFaces = -1;
    for (int i = 0; i < intVertFaces.size(); ++i) {
        if (face == intVertFaces[i]) {
            cornerFaceInIntVertFaces = i;
            break;
        }
    }
    assert(cornerFaceInIntVertFaces >= 0);

    //  Identify the three faces relative to the interior vertex:
    int prevFaceInIntVertFaces = fastMod4(cornerFaceInIntVertFaces + 1);
    int intFaceInIntVertFaces  = fastMod4(cornerFaceInIntVertFaces + 2);
    int nextFaceInIntVertFaces = fastMod4(cornerFaceInIntVertFaces + 3);

    //  Identify the indices of the three other faces:
    Index prevFace = intVertFaces[prevFaceInIntVertFaces];
    Index intFace  = intVertFaces[intFaceInIntVertFaces];
    Index nextFace = intVertFaces[nextFaceInIntVertFaces];

    //  Identify the interior vertex relative to these three faces:
    LocalIndex intVertInPrevFace = intVertInFaces[prevFaceInIntVertFaces];
    LocalIndex intVertInIntFace  = intVertInFaces[intFaceInIntVertFaces];
    LocalIndex intVertInNextFace = intVertInFaces[nextFaceInIntVertFaces];

    //
    //  Now that all faces of interest have been found, identify the point
    //  indices within each face (i.e. the vertex or fvar-value index arrays)
    //  and copy them into the patch points:
    //
    ConstIndexArray thisFacePoints,
                    prevFacePoints,
                    intFacePoints,
                    nextFacePoints;

    if (fvarChannel < 0) {
        thisFacePoints = faceVerts;
        prevFacePoints = level.getFaceVertices(prevFace);
        intFacePoints  = level.getFaceVertices(intFace);
        nextFacePoints = level.getFaceVertices(nextFace);
    } else {
        thisFacePoints = level.getFaceFVarValues(face, fvarChannel);
        prevFacePoints = level.getFaceFVarValues(prevFace, fvarChannel);
        intFacePoints  = level.getFaceFVarValues(intFace, fvarChannel);
        nextFacePoints = level.getFaceFVarValues(nextFace, fvarChannel);
    }

    patchPoints[0] = thisFacePoints[         cornerVertInFace];
    patchPoints[1] = thisFacePoints[fastMod4(cornerVertInFace + 1)];
    patchPoints[2] = thisFacePoints[fastMod4(cornerVertInFace + 2)];
    patchPoints[3] = thisFacePoints[fastMod4(cornerVertInFace + 3)];

    patchPoints[4] = prevFacePoints[fastMod4(intVertInPrevFace + 2)];

    patchPoints[5] = intFacePoints[fastMod4(intVertInIntFace + 1)];
    patchPoints[6] = intFacePoints[fastMod4(intVertInIntFace + 2)];
    patchPoints[7] = intFacePoints[fastMod4(intVertInIntFace + 3)];

    patchPoints[8] = nextFacePoints[fastMod4(intVertInNextFace + 2)];

    return 9;
}

//
//  Gathering the 12 vertices of a tri-regular interior patch:
//      - the neighborhood of the face is assumed to be tri-regular
//
//  Ordering of resulting vertices:
//      The three vertices of the triangle begin the sequence, followed by counter-clockwise
//  traversal of the outer ring of vertices -- beginning with the vertex incident V0 such
//  that the ring is symmetric about the triangle.
/*
//                   3           11
//                   X - - - - - X
//                 /   \       /   \
//               /       \ 0 /       \
//          4  X - - - - - X - - - - - X 10
//           /   \       / * \       /   \
//         /       \   / * * * \   /       \
//    5  X - - - - - X - - - - - X - - - - - X  9
//         \       / 1 \       / 2 \       /
//           \   /       \   /       \   /
//             X - - - - - X - - - - - X
//             6           7           8
*/
int
Level::gatherTriRegularInteriorPatchPoints(Index fIndex, Index points[12], int rotation) const
{
    ConstIndexArray  fVerts = getFaceVertices(fIndex);
    ConstIndexArray  fEdges = getFaceEdges(fIndex);

    int index0 = 0;
    int index1 = 1;
    int index2 = 2;
    if (rotation) {
        index0 = rotation % 3;
        index1 = (rotation + 1) % 3;
        index2 = (rotation + 2) % 3;
    }

    Index v0 = fVerts[index0];
    Index v1 = fVerts[index1];
    Index v2 = fVerts[index2];

    ConstIndexArray  v0Edges = getVertexEdges(v0);
    ConstIndexArray  v1Edges = getVertexEdges(v1);
    ConstIndexArray  v2Edges = getVertexEdges(v2);

    int e0InV0Edges = v0Edges.FindIndex(fEdges[index0]);
    int e1InV1Edges = v1Edges.FindIndex(fEdges[index1]);
    int e2InV2Edges = v2Edges.FindIndex(fEdges[index2]);

    points[ 0] = v0;
    points[ 1] = v1;
    points[ 2] = v2;

    points[11] = otherOfTwo(getEdgeVertices(v0Edges[(e0InV0Edges + 3) % 6]), v0);
    points[ 3] = otherOfTwo(getEdgeVertices(v0Edges[(e0InV0Edges + 4) % 6]), v0);
    points[ 4] = otherOfTwo(getEdgeVertices(v0Edges[(e0InV0Edges + 5) % 6]), v0);

    points[ 5] = otherOfTwo(getEdgeVertices(v1Edges[(e1InV1Edges + 3) % 6]), v1);
    points[ 6] = otherOfTwo(getEdgeVertices(v1Edges[(e1InV1Edges + 4) % 6]), v1);
    points[ 7] = otherOfTwo(getEdgeVertices(v1Edges[(e1InV1Edges + 5) % 6]), v1);

    points[ 8] = otherOfTwo(getEdgeVertices(v2Edges[(e2InV2Edges + 3) % 6]), v2);
    points[ 9] = otherOfTwo(getEdgeVertices(v2Edges[(e2InV2Edges + 4) % 6]), v2);
    points[10] = otherOfTwo(getEdgeVertices(v2Edges[(e2InV2Edges + 5) % 6]), v2);

    return 12;
}

//
//  Gathering the 9 vertices of a tri-regular "boundary edge" patch:
//      - the neighborhood of the face is assumed to be tri-regular
//      - referred to as "boundary edge" as the boundary occurs on the edge of the triangle
//
//  Boundary edge:
//
/*                   6           5
//                   X - - - - - X
//                 /   \       /   \
//               /       \ 2 /       \
//          7  X - - - - - X - - - - - X  4
//           /   \       / * \       /   \
//         /       \   / * * * \   /       \
//    8  X - - - - - X - - - - - X - - - - - X  3
//                   0           1
*/
int
Level::gatherTriRegularBoundaryEdgePatchPoints(Index fIndex, Index points[], int boundaryFaceEdge) const
{
    ConstIndexArray  fVerts = getFaceVertices(fIndex);

    Index v0 = fVerts[boundaryFaceEdge];
    Index v1 = fVerts[(boundaryFaceEdge + 1) % 3];
    Index v2 = fVerts[(boundaryFaceEdge + 2) % 3];

    ConstIndexArray  v0Edges = getVertexEdges(v0);
    ConstIndexArray  v1Edges = getVertexEdges(v1);
    ConstIndexArray  v2Edges = getVertexEdges(v2);

    int e1InV2Edges = v2Edges.FindIndex(v1Edges[2]);

    points[0] = v0;
    points[1] = v1;
    points[2] = v2;

    points[3] = otherOfTwo(getEdgeVertices(v1Edges[0]), v1);

    points[4] = otherOfTwo(getEdgeVertices(v2Edges[(e1InV2Edges + 1) % 6]), v2);
    points[5] = otherOfTwo(getEdgeVertices(v2Edges[(e1InV2Edges + 2) % 6]), v2);
    points[6] = otherOfTwo(getEdgeVertices(v2Edges[(e1InV2Edges + 3) % 6]), v2);
    points[7] = otherOfTwo(getEdgeVertices(v2Edges[(e1InV2Edges + 4) % 6]), v2);

    points[8] = otherOfTwo(getEdgeVertices(v0Edges[3]), v0);

    return 9;
}

//
//  Gathering the 10 vertices of a tri-regular "boundary vertex" patch:
//      - the neighborhood of the face is assumed to be tri-regular
//      - referred to as "boundary vertex" as the boundary occurs on the vertex of the triangle
//
//  Boundary vertex:
/*
//                         0
//          3  X - - - - - X - - - - - X  9
//           /   \       / * \       /   \
//         /       \   / * * * \   /       \
//    4  X - - - - - X - - - - - X - - - - - X  8
//         \       / 1 \       / 2 \       /
//           \   /       \   /       \   /
//             X - - - - - X - - - - - X
//             5           6           7
*/
int
Level::gatherTriRegularBoundaryVertexPatchPoints(Index fIndex, Index points[], int boundaryFaceVert) const
{
    ConstIndexArray  fVerts = getFaceVertices(fIndex);
    ConstIndexArray  fEdges = getFaceEdges(fIndex);

    Index v0 = fVerts[boundaryFaceVert];
    Index v1 = fVerts[(boundaryFaceVert + 1) % 3];
    Index v2 = fVerts[(boundaryFaceVert + 2) % 3];

    Index e1 = fEdges[boundaryFaceVert];
    Index e2 = fEdges[(boundaryFaceVert + 2) % 3];

    ConstIndexArray  v1Edges = getVertexEdges(v1);
    ConstIndexArray  v2Edges = getVertexEdges(v2);

    int e1InV1Edges = v1Edges.FindIndex(e1);
    int e2InV2Edges = v2Edges.FindIndex(e2);

    points[0] = v0;
    points[1] = v1;
    points[2] = v2;

    points[3] = otherOfTwo(getEdgeVertices(v1Edges[(e1InV1Edges + 1) % 6]), v1);
    points[4] = otherOfTwo(getEdgeVertices(v1Edges[(e1InV1Edges + 2) % 6]), v1);
    points[5] = otherOfTwo(getEdgeVertices(v1Edges[(e1InV1Edges + 3) % 6]), v1);
    points[6] = otherOfTwo(getEdgeVertices(v1Edges[(e1InV1Edges + 4) % 6]), v1);

    points[7] = otherOfTwo(getEdgeVertices(v2Edges[(e2InV2Edges + 3) % 6]), v2);
    points[8] = otherOfTwo(getEdgeVertices(v2Edges[(e2InV2Edges + 4) % 6]), v2);
    points[9] = otherOfTwo(getEdgeVertices(v2Edges[(e2InV2Edges + 5) % 6]), v2);

    return 10;
}

//
//  Gathering the 6 vertices of a tri-regular "corner vertex" patch:
//      - the neighborhood of the face is assumed to be tri-regular
//      - referred to as "corner vertex" to disambiguate it from another corner case
//          - another boundary case shares the edge with the corner triangle
//
//  Corner vertex:
/*
//                         0
//                         X
//                       / * \
//                     / * * * \
//                   X - - - - - X
//                 / 1 \       / 2 \
//               /       \   /       \
//             X - - - - - X - - - - - X
//             3           4           5
*/
int
Level::gatherTriRegularCornerVertexPatchPoints(Index fIndex, Index points[], int cornerFaceVert) const
{
    ConstIndexArray  fVerts = getFaceVertices(fIndex);

    Index v0 = fVerts[cornerFaceVert];
    Index v1 = fVerts[(cornerFaceVert + 1) % 3];
    Index v2 = fVerts[(cornerFaceVert + 2) % 3];

    ConstIndexArray  v1Edges = getVertexEdges(v1);
    ConstIndexArray  v2Edges = getVertexEdges(v2);

    points[0] = v0;
    points[1] = v1;
    points[2] = v2;

    points[3] = otherOfTwo(getEdgeVertices(v1Edges[0]), v1);
    points[4] = otherOfTwo(getEdgeVertices(v1Edges[1]), v1);
    points[5] = otherOfTwo(getEdgeVertices(v2Edges[3]), v2);

    return 6;
}

//
//  Gathering the 8 vertices of a tri-regular "corner edge" patch:
//      - the neighborhood of the face is assumed to be tri-regular
//      - referred to as "corner edge" to disambiguate it from the vertex corner case
//          - this faces shares the edge with a corner triangle
//
//  Corner edge:
/*
//                   6           5
//                   X - - - - - X
//                 /   \       /   \
//               /       \ 2 /       \
//          7  X - - - - - X - - - - - X  4
//               \       / * \       /
//                 \   / * * * \   /
//                   X - - - - - X
//                   0 \       / 1
//                       \   /
//                         X
//                         3
*/
int
Level::gatherTriRegularCornerEdgePatchPoints(Index fIndex, Index points[], int cornerFaceEdge) const
{
    ConstIndexArray  fVerts = getFaceVertices(fIndex);

    Index v0 = fVerts[cornerFaceEdge];
    Index v1 = fVerts[(cornerFaceEdge + 1) % 3];
    Index v2 = fVerts[(cornerFaceEdge + 2) % 3];

    ConstIndexArray  v0Edges = getVertexEdges(v0);
    ConstIndexArray  v1Edges = getVertexEdges(v1);

    points[0] = v0;
    points[1] = v1;
    points[2] = v2;

    points[3] = otherOfTwo(getEdgeVertices(v1Edges[3]), v1);
    points[4] = otherOfTwo(getEdgeVertices(v1Edges[0]), v1);
    points[7] = otherOfTwo(getEdgeVertices(v0Edges[3]), v0);

    ConstIndexArray  v4Edges = getVertexEdges(points[4]);
    ConstIndexArray  v7Edges = getVertexEdges(points[7]);

    points[5] = otherOfTwo(getEdgeVertices(v4Edges[v4Edges.size() - 3]), v1);
    points[6] = otherOfTwo(getEdgeVertices(v7Edges[2]), v1);

    return 8;
}

bool
Level::isSingleCreasePatch(Index face, float *sharpnessOut, int *sharpEdgeInFaceOut) const {

    //  Using the composite tag for all face vertices, first make sure that some
    //  face-vertices are Crease vertices, and quickly reject this case based on the
    //  presence of other features.  Ultimately we want a regular interior face with
    //  two (adjacent) Crease vertics and two Smooth vertices.  This first test
    //  quickly ensures a regular interior face with some number of Crease vertices
    //  and any remaining as Smooth.
    //
    ConstIndexArray fVerts = getFaceVertices(face);

    VTag allCornersTag = getFaceCompositeVTag(fVerts);
    if (!(allCornersTag._rule & Sdc::Crease::RULE_CREASE) ||
         (allCornersTag._rule & Sdc::Crease::RULE_CORNER) ||
         (allCornersTag._rule & Sdc::Crease::RULE_DART) ||
          allCornersTag._boundary ||
          allCornersTag._xordinary ||
          allCornersTag._nonManifold) {
        return false;
    }

    //  Identify the crease vertices in a 4-bit mask and use it as an index to
    //  verify that we have exactly two adjacent crease vertices while identifying
    //  the edge between them -- reject any case not returning a valid edge.
    //
    int creaseCornerMask = ((getVertexTag(fVerts[0])._rule == Sdc::Crease::RULE_CREASE) << 0) |
                           ((getVertexTag(fVerts[1])._rule == Sdc::Crease::RULE_CREASE) << 1) |
                           ((getVertexTag(fVerts[2])._rule == Sdc::Crease::RULE_CREASE) << 2) |
                           ((getVertexTag(fVerts[3])._rule == Sdc::Crease::RULE_CREASE) << 3);
    static const int sharpEdgeFromCreaseMask[16] = { -1, -1, -1,  0, -1, -1,  1, -1,
                                                     -1,  3, -1, -1,  2, -1, -1, -1 };

    int sharpEdgeInFace = sharpEdgeFromCreaseMask[creaseCornerMask];
    if (sharpEdgeInFace < 0) {
        return false;
    }

    //  Reject if the crease at the two crease vertices A and B is not regular, i.e.
    //  any pair of opposing edges does not have the same sharpness value (one pair
    //  sharp, the other smooth).  The resulting two regular creases must be "colinear"
    //  (sharing the edge between them, and so its common sharpness value) otherwise
    //  we would have more than two crease vertices.
    //
    ConstIndexArray vAEdges = getVertexEdges(fVerts[         sharpEdgeInFace]);
    ConstIndexArray vBEdges = getVertexEdges(fVerts[fastMod4(sharpEdgeInFace + 1)]);

    if (!isSharpnessEqual(getEdgeSharpness(vAEdges[0]), getEdgeSharpness(vAEdges[2])) ||
        !isSharpnessEqual(getEdgeSharpness(vAEdges[1]), getEdgeSharpness(vAEdges[3])) ||
        !isSharpnessEqual(getEdgeSharpness(vBEdges[0]), getEdgeSharpness(vBEdges[2])) ||
        !isSharpnessEqual(getEdgeSharpness(vBEdges[1]), getEdgeSharpness(vBEdges[3]))) {
        return false;
    }
    if (sharpnessOut) {
        *sharpnessOut = getEdgeSharpness(getFaceEdges(face)[sharpEdgeInFace]);
    }
    if (sharpEdgeInFaceOut) {
        *sharpEdgeInFaceOut = sharpEdgeInFace;
    }
    return true;
}

//
//  What follows is an internal/anonymous class and protected methods to complete all
//  topological relations when only the face-vertex relations are defined.
//
//  In keeping with the original idea that Level is just data and relies on other
//  classes to construct it, this functionality may be warranted elsewhere, but we are
//  collectively unclear as to where that should be at present.  In the meantime, the
//  implementation is provided here so that we can test and make use of it.
//
namespace {
    //
    //  This is an internal helper class to manage the assembly of the topological relations
    //  that do not have a predictable size, i.e. faces-per-edge, faces-per-vertex and
    //  edges-per-vertex.  Level manages these with two vectors:
    //
    //      - a vector of integer pairs for the "counts" and "offsets"
    //      - a vector of incident members accessed by the "offset" of each
    //
    //  The "dynamic relation" allocates the latter vector of members based on a typical
    //  number of members per component, e.g. we expect valence 4 vertices in a typical
    //  quad-mesh, and so an "expected" number might be 6 to accommodate a few x-ordinary
    //  vertices.  The member vector is allocated with this number per component and the
    //  counts and offsets initialized to refer to them -- but with the counts set to 0.
    //  The count will be incremented as members are identified and entered, and if any
    //  component "overflows" the expected number of members, the members are moved to a
    //  separate vector in an std::map for the component.
    //
    //  Once all incident members have been added, the main vector is compressed and may
    //  need to merge entries from the map in the process.
    //
    typedef std::map<Index, IndexVector> IrregIndexMap;

    class DynamicRelation {
    public:
        DynamicRelation(IndexVector& countAndOffsets, IndexVector& indices, int membersPerComp);
        ~DynamicRelation() { }

    public:
        //  Methods dealing with the members for each component:
        IndexArray getCompMembers(Index index);
        void       appendCompMember(Index index, Index member);

        //  Methods dealing with the components:
        void appendComponent();
        int  compressMemberIndices();

    public:
        int _compCount;
        int _memberCountPerComp;

        IndexVector & _countsAndOffsets;
        IndexVector & _regIndices;

        IrregIndexMap _irregIndices;
    };

    inline
    DynamicRelation::DynamicRelation(IndexVector& countAndOffsets, IndexVector& indices, int membersPerComp) :
            _compCount(0),
            _memberCountPerComp(membersPerComp),
            _countsAndOffsets(countAndOffsets),
            _regIndices(indices) {

        _compCount = (int) _countsAndOffsets.size() / 2;

        for (int i = 0; i < _compCount; ++i) {
            _countsAndOffsets[2*i]   = 0;
            _countsAndOffsets[2*i+1] = i * _memberCountPerComp;
        }
        _regIndices.resize(_compCount * _memberCountPerComp);
    }

    inline IndexArray
    DynamicRelation::getCompMembers(Index compIndex) {

        int count = _countsAndOffsets[2*compIndex];
        if (count > _memberCountPerComp) {
            IndexVector & irregMembers = _irregIndices[compIndex];
            return IndexArray(&irregMembers[0], (int)irregMembers.size());
        } else {
            int offset = _countsAndOffsets[2*compIndex+1];
            return IndexArray(&_regIndices[offset], count);
        }
    }
    inline void
    DynamicRelation::appendCompMember(Index compIndex, Index memberValue) {

        int count  = _countsAndOffsets[2*compIndex];
        int offset = _countsAndOffsets[2*compIndex+1];

        if (count < _memberCountPerComp) {
            _regIndices[offset + count] = memberValue;
        } else {
            IndexVector& irregMembers = _irregIndices[compIndex];

            if (count > _memberCountPerComp) {
                irregMembers.push_back(memberValue);
            } else {
                irregMembers.resize(_memberCountPerComp + 1);
                std::memcpy(&irregMembers[0], &_regIndices[offset], sizeof(Index) * _memberCountPerComp);
                irregMembers[_memberCountPerComp] = memberValue;
            }
        }
        _countsAndOffsets[2*compIndex] ++;
    }
    inline void
    DynamicRelation::appendComponent() {

        _countsAndOffsets.push_back(0);
        _countsAndOffsets.push_back(_compCount * _memberCountPerComp);

        ++ _compCount;
        _regIndices.resize(_compCount * _memberCountPerComp);
    }
    int
    DynamicRelation::compressMemberIndices() {

        if (_irregIndices.size() == 0) {
            int memberCount = _countsAndOffsets[0];
            int memberMax   = _countsAndOffsets[0];
            for (int i = 1; i < _compCount; ++i) {
                int count  = _countsAndOffsets[2*i];
                int offset = _countsAndOffsets[2*i + 1];

                memmove(&_regIndices[memberCount], &_regIndices[offset], count * sizeof(Index));

                _countsAndOffsets[2*i + 1] = memberCount;
                memberCount += count;
                memberMax    = std::max(memberMax, count);
            }
            _regIndices.resize(memberCount);
            return memberMax;
        } else {
            //  Assign new offsets-per-component while determining if we can trivially compress in place:
            bool cannotBeCompressedInPlace = false;

            int memberCount = _countsAndOffsets[0];
            for (int i = 1; i < _compCount; ++i) {
                _countsAndOffsets[2*i + 1] = memberCount;

                cannotBeCompressedInPlace |= (memberCount > (_memberCountPerComp * i));

                memberCount += _countsAndOffsets[2*i];
            }
            cannotBeCompressedInPlace |= (memberCount > (_memberCountPerComp * _compCount));

            //  Copy members into the original or temporary vector accordingly:
            IndexVector  tmpIndices;
            if (cannotBeCompressedInPlace) {
                tmpIndices.resize(memberCount);
            }
            IndexVector& dstIndices = cannotBeCompressedInPlace ? tmpIndices : _regIndices;

            int memberMax = _memberCountPerComp;
            for (int i = 0; i < _compCount; ++i) {
                int count = _countsAndOffsets[2*i];

                Index *dstMembers = &dstIndices[0] + _countsAndOffsets[2*i + 1];
                Index *srcMembers = 0;
                
                if (count <= _memberCountPerComp) {
                     srcMembers = &_regIndices[i * _memberCountPerComp];
                } else {
                     srcMembers = &_irregIndices[i][0];
                     memberMax = std::max(memberMax, count);
                }
                memmove(dstMembers, srcMembers, count * sizeof(Index));
            }
            if (cannotBeCompressedInPlace) {
                _regIndices.swap(tmpIndices);
            } else {
                _regIndices.resize(memberCount);
            }
            return memberMax;
        }
    }
}


//
//  Methods to populate the missing topology relations of the Level:
//
inline Index
Level::findEdge(Index v0Index, Index v1Index, ConstIndexArray v0Edges) const {

    if (v0Index != v1Index) {
        for (int j = 0; j < v0Edges.size(); ++j) {
            ConstIndexArray eVerts = this->getEdgeVertices(v0Edges[j]);
            if ((eVerts[0] == v1Index) || (eVerts[1] == v1Index)) {
                return v0Edges[j];
            }
        }
    } else {
        for (int j = 0; j < v0Edges.size(); ++j) {
            ConstIndexArray eVerts = this->getEdgeVertices(v0Edges[j]);
            if (eVerts[0] == eVerts[1]) {
                return v0Edges[j];
            }
        }
    }
    return INDEX_INVALID;
}

Index
Level::findEdge(Index v0Index, Index v1Index) const {
    return this->findEdge(v0Index, v1Index, this->getVertexEdges(v0Index));
}

bool
Level::completeTopologyFromFaceVertices() {

    //
    //  It's assumed (a pre-condition) that face-vertices have been fully specified and that we
    //  are to construct the remaining relations:  including the edge list.  We may want to
    //  support the existence of the edge list too in future:
    //
    int vCount = this->getNumVertices();
    int fCount = this->getNumFaces();
    int eCount = this->getNumEdges();
    assert((vCount > 0) && (fCount > 0) && (eCount == 0));

    //  May be unnecessary depending on how the vertices and faces were defined, but worth a
    //  call to ensure all data related to verts and faces is available -- this will be a
    //  harmless call if all has been taken care of.
    //
    //  Remember to resize edges similarly after the edge list has been assembled...
    this->resizeVertices(vCount);
    this->resizeFaces(fCount);
    this->resizeEdges(0);

    //
    //  Resize face-edges to match face-verts and reserve for edges based on an estimate:
    //
    this->_faceEdgeIndices.resize(this->getNumFaceVerticesTotal());

    int eCountEstimate = (vCount << 1);

    this->_edgeVertIndices.reserve(eCountEstimate * 2);
    this->_edgeFaceIndices.reserve(eCountEstimate * 2);

    this->_edgeFaceCountsAndOffsets.reserve(eCountEstimate * 2);

    //
    //  Create the dynamic relations to be populated (edge-faces will remain empty as reserved
    //  above since there are currently no edges) and iterate through the faces to do so:
    //
    const int avgSize = 6;

    DynamicRelation dynEdgeFaces(this->_edgeFaceCountsAndOffsets, this->_edgeFaceIndices, 2);
    DynamicRelation dynVertFaces(this->_vertFaceCountsAndOffsets, this->_vertFaceIndices, avgSize);
    DynamicRelation dynVertEdges(this->_vertEdgeCountsAndOffsets, this->_vertEdgeIndices, avgSize);

    //  Inspect each edge created and identify those that are non-manifold as we go:
    IndexVector nonManifoldEdges;

    for (Index fIndex = 0; fIndex < fCount; ++fIndex) {
        IndexArray fVerts = this->getFaceVertices(fIndex);
        IndexArray fEdges = this->getFaceEdges(fIndex);

        for (int i = 0; i < fVerts.size(); ++i) {
            Index v0Index = fVerts[i];
            Index v1Index = fVerts[(i+1) % fVerts.size()];

            //
            //  If not degenerate, search for a previous occurrence of this edge [v0,v1]
            //  in v0's incident edge members.  Otherwise, set the edge index as invalid
            //  to trigger creation of a new/unique instance of the degenerate edge:
            //
            Index eIndex;
            if (v0Index != v1Index) {
                eIndex = this->findEdge(v0Index, v1Index, dynVertEdges.getCompMembers(v0Index));
            } else {
                eIndex = INDEX_INVALID;
                nonManifoldEdges.push_back(this->_edgeCount);
            }

            //
            //  If the edge already exists, see if it is non-manifold, i.e. it has already been
            //  added to two faces, or this face has the edge in the same orientation as the
            //  first face (indicating opposite winding orders between the two faces).
            //
            //  Otherwise, create a new edge, append the new vertex pair [v0,v1] and update
            //  the incidence relations for the edge and its end vertices and this face.
            //
            //  Regardless of whether or not the edge was new, update the edge-faces, the
            //  face-edges and the vertex-faces for this vertex.
            //
            if (IndexIsValid(eIndex)) {
                IndexArray eFaces = dynEdgeFaces.getCompMembers(eIndex);
                if (eFaces[eFaces.size() - 1] == fIndex) {
                    //  If the edge already occurs in this face, create a new instance:
                    nonManifoldEdges.push_back(eIndex);
                    nonManifoldEdges.push_back(this->_edgeCount);
                    eIndex = INDEX_INVALID;
                } else if (eFaces.size() > 1) {
                    nonManifoldEdges.push_back(eIndex);
                } else if (v0Index == this->getEdgeVertices(eIndex)[0]) {
                    nonManifoldEdges.push_back(eIndex);
                }
            }
            if (!IndexIsValid(eIndex)) {
                eIndex = (Index) this->_edgeCount;

                this->_edgeCount ++;
                this->_edgeVertIndices.push_back(v0Index);
                this->_edgeVertIndices.push_back(v1Index);

                dynEdgeFaces.appendComponent();

                dynVertEdges.appendCompMember(v0Index, eIndex);
                dynVertEdges.appendCompMember(v1Index, eIndex);
            }

            dynEdgeFaces.appendCompMember(eIndex,  fIndex);
            dynVertFaces.appendCompMember(v0Index, fIndex);

            fEdges[i] = eIndex;
        }
    }

    //
    //  Compress the incident member vectors while determining the maximum for each.
    //  Use these to set maximum relation count members and to test for valence or
    //  other incident member overflow:  max edge-faces is simple, but for max-valence,
    //  remember it was first initialized with the maximum of face-verts, so use its
    //  existing value -- and some non-manifold cases can have #faces > #edges, so be
    //  sure to consider both.
    //
    int maxEdgeFaces = dynEdgeFaces.compressMemberIndices();
    int maxVertFaces = dynVertFaces.compressMemberIndices();
    int maxVertEdges = dynVertEdges.compressMemberIndices();

    _maxEdgeFaces = maxEdgeFaces;

    assert(_maxValence > 0);
    _maxValence = std::max(maxVertFaces, _maxValence);
    _maxValence = std::max(maxVertEdges, _maxValence);

    //  If max-edge-faces too large, max-valence must also be, so just need the one:
    if (_maxValence > VALENCE_LIMIT) {
        return false;
    }

    //
    //  At this point all incident members are associated with each component.  We still
    //  need to populate the "local indices" for each and orient manifold components in
    //  counter-clockwise order.  First tag non-manifold edges and their incident
    //  vertices so that we can trivially skip orienting these -- though some vertices
    //  will be determined non-manifold as a result of a failure to orient them (and
    //  will be marked accordingly when so detected).
    //
    //  Finally, the local indices are assigned.  This is trivial for manifold components
    //  as if component V is in component F, V will only occur once in F.  For non-manifold
    //  cases V may occur multiple times in F -- we rely on such instances being successive
    //  based on their original assignment above, which simplifies the task.
    //
    //  First resize edges to the new count to ensure anything related to edges is created:
    eCount = this->getNumEdges();
    this->resizeEdges(eCount);

    for (int i = 0; i < (int)nonManifoldEdges.size(); ++i) {
        Index eIndex = nonManifoldEdges[i];

        _edgeTags[eIndex]._nonManifold = true;

        IndexArray eVerts = getEdgeVertices(eIndex);
        _vertTags[eVerts[0]]._nonManifold = true;
        _vertTags[eVerts[1]]._nonManifold = true;
    }

    orientIncidentComponents();

    populateLocalIndices();

//printf("Vertex topology completed...\n");
//this->print();
//printf("  validating vertex topology...\n");
//this->validateTopology();
//assert(this->validateTopology());
    return true;
}

void
Level::populateLocalIndices() {

    //
    //  We have three sets of local indices -- edge-faces, vert-faces and vert-edges:
    //
    int eCount = this->getNumEdges();
    int vCount = this->getNumVertices();

    this->_vertFaceLocalIndices.resize(this->_vertFaceIndices.size());
    this->_vertEdgeLocalIndices.resize(this->_vertEdgeIndices.size());
    this->_edgeFaceLocalIndices.resize(this->_edgeFaceIndices.size());

    for (Index vIndex = 0; vIndex < vCount; ++vIndex) {
        IndexArray      vFaces   = this->getVertexFaces(vIndex);
        LocalIndexArray vInFaces = this->getVertexFaceLocalIndices(vIndex);

        //
        //  We keep track of the last face during the iteration to detect when two
        //  (or more) successive faces are the same -- indicating a degenerate edge
        //  or other non-manifold situation.  If so, we continue to search from the
        //  point of the last face's local index:
        //
        Index vFaceLast = INDEX_INVALID;
        for (int i = 0; i < vFaces.size(); ++i) {
            IndexArray fVerts = this->getFaceVertices(vFaces[i]);

            int vStart = (vFaces[i] == vFaceLast) ? ((int)vInFaces[i-1] + 1) : 0;

            int vInFaceIndex = (int)(std::find(fVerts.begin() + vStart, fVerts.end(), vIndex) - fVerts.begin());
            vInFaces[i] = (LocalIndex) vInFaceIndex;

            vFaceLast = vFaces[i];
        }
    }

    for (Index vIndex = 0; vIndex < vCount; ++vIndex) {
        IndexArray      vEdges   = this->getVertexEdges(vIndex);
        LocalIndexArray vInEdges = this->getVertexEdgeLocalIndices(vIndex);

        for (int i = 0; i < vEdges.size(); ++i) {
            IndexArray eVerts = this->getEdgeVertices(vEdges[i]);

            //
            //  For degenerate edges, the first occurrence of the edge (which
            //  are presumed successive) will get local index 0, the second 1.
            //
            if (eVerts[0] != eVerts[1]) {
                vInEdges[i] = (vIndex == eVerts[1]);
            } else {
                vInEdges[i] = (i && (vEdges[i] == vEdges[i-1]));
            }
        }
        _maxValence = std::max(_maxValence, vEdges.size());
    }

    for (Index eIndex = 0; eIndex < eCount; ++eIndex) {
        IndexArray      eFaces   = this->getEdgeFaces(eIndex);
        LocalIndexArray eInFaces = this->getEdgeFaceLocalIndices(eIndex);

        //
        //  We keep track of the last face during the iteration to detect when two
        //  (or more) successive faces are the same -- indicating a degenerate edge
        //  or other non-manifold situation.  If so, we continue to search from the
        //  point of the last face's local index:
        //
        Index eFaceLast = INDEX_INVALID;
        for (int i = 0; i < eFaces.size(); ++i) {
            IndexArray fEdges = this->getFaceEdges(eFaces[i]);

            int eStart = (eFaces[i] == eFaceLast) ? ((int)eInFaces[i-1] + 1) : 0;

            int eInFaceIndex = (int)(std::find(fEdges.begin() + eStart, fEdges.end(), eIndex) - fEdges.begin());
            eInFaces[i] = (LocalIndex) eInFaceIndex;

            eFaceLast = eFaces[i];
        }
    }
}

void
Level::orientIncidentComponents() {

    int vCount = getNumVertices();

    for (Index vIndex = 0; vIndex < vCount; ++vIndex) {
        Level::VTag & vTag = _vertTags[vIndex];
        if (!vTag._nonManifold) {
            if (!orderVertexFacesAndEdges(vIndex)) {
                vTag._nonManifold = true;
            }
        }
    }
}

namespace {
    inline int
    findInArray(ConstIndexArray array, Index value) {
        return (int)(std::find(array.begin(), array.end(), value) - array.begin());
    }
}

bool
Level::orderVertexFacesAndEdges(Index vIndex, Index * vFacesOrdered, Index * vEdgesOrdered) const {

    ConstIndexArray  vEdges = this->getVertexEdges(vIndex);
    ConstIndexArray  vFaces = this->getVertexFaces(vIndex);

    int fCount = vFaces.size();
    int eCount = vEdges.size();

    if ((fCount == 0) || (eCount < 2) || ((eCount - fCount) > 1)) return false;

    //
    //  Note we have already eliminated the possibility of incident degenerate edges
    //  and other bad edges earlier -- marking its vertices non-manifold as a result
    //  and explicitly avoiding this method:
    //
    Index fStart  = INDEX_INVALID;
    Index eStart  = INDEX_INVALID;
    int      fvStart = 0;

    if (eCount == fCount) {
        //  Interior case -- start with the first face

        fStart  = vFaces[0];
        fvStart = findInArray(this->getFaceVertices(fStart), vIndex);
        eStart  = this->getFaceEdges(fStart)[fvStart];
    } else {
        //  Boundary case -- start with (identify) the leading of two boundary edges:

        for (int i = 0; i < eCount; ++i) {
            ConstIndexArray  eFaces = this->getEdgeFaces(vEdges[i]);
            if (eFaces.size() == 1) {
                eStart = vEdges[i];
                fStart = eFaces[0];
                fvStart = findInArray(this->getFaceVertices(fStart), vIndex);

                //  Singular edge -- look for forward edge to this vertex:
                if (eStart == (this->getFaceEdges(fStart)[fvStart])) {
                    break;
                }
            }
        }
    }

    //
    //  We have identified a starting face, face-vert and leading edge from
    //  which to walk counter clockwise to identify manifold neighbors.  If
    //  this vertex is really locally manifold, we will end up back at the
    //  starting edge or at the other singular edge of a boundary:
    //
    int eCountOrdered = 1;
    int fCountOrdered = 1;

    vFacesOrdered[0] = fStart;
    vEdgesOrdered[0] = eStart;

    Index eFirst = eStart;

    while (eCountOrdered < eCount) {
        //
        //  Find the next edge, i.e. the one counter-clockwise to the last:
        //
        ConstIndexArray  fVerts = this->getFaceVertices(fStart);
        ConstIndexArray  fEdges = this->getFaceEdges(fStart);

        int      feStart = fvStart;
        int      feNext  = feStart ? (feStart - 1) : (fVerts.size() - 1);
        Index eNext   = fEdges[feNext];

        //  Two non-manifold situations detected:
        //      - two subsequent edges the same, i.e. a "repeated edge" in a face
        //      - back at the start before all edges processed
        if ((eNext == eStart) || (eNext == eFirst)) return false;

        //
        //  Add the next edge and if more faces to visit (not at the end of
        //  a boundary) look to its opposite face:
        //
        vEdgesOrdered[eCountOrdered++] = eNext;

        if (fCountOrdered < fCount) {
            ConstIndexArray  eFaces = this->getEdgeFaces(eNext);

            if (eFaces.size() == 0) return false;
            if ((eFaces.size() == 1) && (eFaces[0] == fStart)) return false;

            fStart  = eFaces[eFaces[0] == fStart];
            fvStart = findInArray(this->getFaceEdges(fStart), eNext);

            vFacesOrdered[fCountOrdered++] = fStart;
        }
        eStart = eNext;
    }
    assert(eCountOrdered == eCount);
    assert(fCountOrdered == fCount);
    return true;
}

bool
Level::orderVertexFacesAndEdges(Index vIndex) {

    IndexArray vFaces = this->getVertexFaces(vIndex);
    IndexArray vEdges = this->getVertexEdges(vIndex);

    internal::StackBuffer<Index,32> indexBuffer(vFaces.size() + vEdges.size());

    Index * vFacesOrdered = indexBuffer;
    Index * vEdgesOrdered = indexBuffer + vFaces.size();

    if (orderVertexFacesAndEdges(vIndex, vFacesOrdered, vEdgesOrdered)) {
        std::memcpy(&vFaces[0], vFacesOrdered, vFaces.size() * sizeof(Index));
        std::memcpy(&vEdges[0], vEdgesOrdered, vEdges.size() * sizeof(Index));
        return true;
    }
    return false;
}

//
//  In development -- methods for accessing face-varying data channels...
//
int
Level::createFVarChannel(int fvarValueCount, Sdc::Options const& fvarOptions) {

    FVarLevel* fvarLevel = new FVarLevel(*this);

    fvarLevel->setOptions(fvarOptions);
    fvarLevel->resizeValues(fvarValueCount);
    fvarLevel->resizeComponents();

    _fvarChannels.push_back(fvarLevel);
    return (int)_fvarChannels.size() - 1;
}

void
Level::destroyFVarChannel(int channel) {

    delete _fvarChannels[channel];
    _fvarChannels.erase(_fvarChannels.begin() + channel);
}

int
Level::getNumFVarValues(int channel) const {
    return _fvarChannels[channel]->getNumValues();
}

Sdc::Options
Level::getFVarOptions(int channel) const {
    return _fvarChannels[channel]->getOptions();
}

ConstIndexArray
Level::getFaceFVarValues(Index faceIndex, int channel) const {
    return _fvarChannels[channel]->getFaceValues(faceIndex);
}

IndexArray
Level::getFaceFVarValues(Index faceIndex, int channel) {
    return _fvarChannels[channel]->getFaceValues(faceIndex);
}

void
Level::completeFVarChannelTopology(int channel, int regBoundaryValence) {
    return _fvarChannels[channel]->completeTopologyFromFaceValues(regBoundaryValence);
}

} // end namespace internal
} // end namespace Vtr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv
