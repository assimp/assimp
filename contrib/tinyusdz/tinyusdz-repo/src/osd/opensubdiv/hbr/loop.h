//
//   Copyright 2013 Pixar
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

#ifndef OPENSUBDIV3_HBRLOOP_H
#define OPENSUBDIV3_HBRLOOP_H

#include <cmath>
#include <assert.h>
#include <algorithm>

#include "../hbr/subdivision.h"

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

/* #define HBR_DEBUG */

template <class T>
class HbrLoopSubdivision : public HbrSubdivision<T>{
public:
    HbrLoopSubdivision<T>()
        : HbrSubdivision<T>() {}

    virtual HbrSubdivision<T>* Clone() const {
        return new HbrLoopSubdivision<T>();
    }

    virtual void Refine(HbrMesh<T>* mesh, HbrFace<T>* face);
    virtual HbrFace<T>* RefineFaceAtVertex(HbrMesh<T>* mesh, HbrFace<T>* face, HbrVertex<T>* vertex);
    virtual void GuaranteeNeighbor(HbrMesh<T>* mesh, HbrHalfedge<T>* edge);
    virtual void GuaranteeNeighbors(HbrMesh<T>* mesh, HbrVertex<T>* vertex);

    virtual bool HasLimit(HbrMesh<T>* mesh, HbrFace<T>* face);
    virtual bool HasLimit(HbrMesh<T>* mesh, HbrHalfedge<T>* edge);
    virtual bool HasLimit(HbrMesh<T>* mesh, HbrVertex<T>* vertex);

    virtual HbrVertex<T>* Subdivide(HbrMesh<T>* mesh, HbrFace<T>* face);
    virtual HbrVertex<T>* Subdivide(HbrMesh<T>* mesh, HbrHalfedge<T>* edge);
    virtual HbrVertex<T>* Subdivide(HbrMesh<T>* mesh, HbrVertex<T>* vertex);

    virtual bool VertexIsExtraordinary(HbrMesh<T> const * /* mesh */, HbrVertex<T>* vertex) { return vertex->GetValence() != 6; }
    virtual bool FaceIsExtraordinary(HbrMesh<T> const * /* mesh */, HbrFace<T>* face) { return face->GetNumVertices() != 3; }

    virtual int GetFaceChildrenCount(int /* nvertices */) const { return 4; }

private:

    // Transfers facevarying data from a parent face to a child face
    void transferFVarToChild(HbrMesh<T>* mesh, HbrFace<T>* face, HbrFace<T>* child, int index);

    // Transfers vertex and edge edits from a parent face to a child face
    void transferEditsToChild(HbrFace<T>* face, HbrFace<T>* child, int index);

    // Generates the fourth child of a triangle: the triangle in the
    // middle whose vertices have parents which are all edges
    void refineFaceAtMiddle(HbrMesh<T>* mesh, HbrFace<T>* face);

};

template <class T>
void
HbrLoopSubdivision<T>::transferFVarToChild(HbrMesh<T>* mesh, HbrFace<T>* face, HbrFace<T>* child, int index) {
    typename HbrMesh<T>::InterpolateBoundaryMethod fvarinterp = mesh->GetFVarInterpolateBoundaryMethod();
    HbrVertex<T>* childVertex;

    // In the case of index == 3, this is the middle face, and so
    // we need to do three edge subdivision rules
    if (index == 3) {
        const int fvarcount = mesh->GetFVarCount();
        for (int i = 0; i < 3; ++i) {
            HbrHalfedge<T> *edge = face->GetEdge(i);
            GuaranteeNeighbor(mesh, edge);
            childVertex = child->GetVertex((i + 2) % 3);
            bool fvIsSmooth = !edge->IsFVarInfiniteSharpAnywhere();
            if (!fvIsSmooth) {
                childVertex->NewFVarData(child);
            }
            HbrFVarData<T>& fv = childVertex->GetFVarData(child);
            int fvarindex = 0;
            for (int fvaritem = 0; fvaritem < fvarcount; ++fvaritem) {
                const int fvarwidth = mesh->GetFVarWidths()[fvaritem];

                if (fvarinterp == HbrMesh<T>::k_InterpolateBoundaryNone ||
                    face->GetEdge(i)->GetFVarSharpness(fvaritem) || face->GetEdge(i)->IsBoundary()) {

                    // Sharp edge rule
                    fv.SetWithWeight(face->GetFVarData(i), fvarindex, fvarwidth, 0.5f);
                    fv.AddWithWeight(face->GetFVarData((i + 1) % 3), fvarindex, fvarwidth, 0.5f);
                } else if (!fvIsSmooth || !fv.IsInitialized()) {
                    // Smooth edge subdivision. Add 0.375 of adjacent vertices
                    fv.SetWithWeight(face->GetFVarData(i), fvarindex, fvarwidth, 0.375f);
                    fv.AddWithWeight(face->GetFVarData((i + 1) % 3), fvarindex, fvarwidth, 0.375f);
                    // Add 0.125 of opposite vertices
                    fv.AddWithWeight(face->GetFVarData((i + 2) % 3), fvarindex, fvarwidth, 0.125f);
                    HbrFace<T>* oppFace = face->GetEdge(i)->GetRightFace();
                    for (int j = 0; j < oppFace->GetNumVertices(); ++j) {
                        if (oppFace->GetVertex(j) == face->GetVertex(i)) {
                            fv.AddWithWeight(oppFace->GetFVarData((j+1)%oppFace->GetNumVertices()), fvarindex, fvarwidth, 0.125f);
                            break;
                        }
                    }
                }
                fvarindex += fvarwidth;
            }
            fv.SetInitialized();
        }
        return;
    }

    HbrHalfedge<T>* edge;
    HbrVertex<T>* v = face->GetVertex(index);

    // Otherwise we proceed with one vertex and two edge subdivision
    // applications. First the vertex subdivision rule. Analyze
    // whether the vertex is on the boundary and whether it's an
    // infinitely sharp corner.  We determine the last by checking the
    // propagate corners flag on the mesh; if it's off, we check the
    // two edges of this face incident to that vertex and determining
    // whether they are facevarying boundary edges - this is analogous
    // to what goes on for the interpolateboundary tag (which when set
    // to EDGEANDCORNER marks vertices with a valence of two as being
    // sharp corners). If propagate corners is on, we check *all*
    // faces to see if two edges side by side are facevarying boundary
    // edges. The facevarying boundary check ignores geometric
    // sharpness, otherwise we may swim at geometric creases which
    // aren't actually discontinuous.
    //
    // We need to make sure that that each of the vertices of the
    // child face have the appropriate facevarying storage as
    // needed. If there are discontinuities in any facevarying datum,
    // the vertex must allocate a new block of facevarying storage
    // specific to the child face.

    v->GuaranteeNeighbors();


    bool fv0IsSmooth, fv1IsSmooth, fv2IsSmooth;

    childVertex = child->GetVertex(index);
    fv0IsSmooth = v->IsFVarAllSmooth();
    if (!fv0IsSmooth) {
        childVertex->NewFVarData(child);
    }
    HbrFVarData<T>& fv0 = childVertex->GetFVarData(child);

    edge = face->GetEdge(index);
    GuaranteeNeighbor(mesh, edge);
    assert(edge->GetOrgVertex() == v);
    childVertex = child->GetVertex((index + 1) % 3);
    fv1IsSmooth = !edge->IsFVarInfiniteSharpAnywhere();
    if (!fv1IsSmooth) {
        childVertex->NewFVarData(child);
    }
    HbrFVarData<T>& fv1 = childVertex->GetFVarData(child);

    edge = edge->GetPrev();
    GuaranteeNeighbor(mesh, edge);
    assert(edge == face->GetEdge((index + 2) % 3));
    assert(edge->GetDestVertex() == v);
    childVertex = child->GetVertex((index + 2) % 3);
    fv2IsSmooth = !edge->IsFVarInfiniteSharpAnywhere();
    if (!fv2IsSmooth) {
        childVertex->NewFVarData(child);
    }
    HbrFVarData<T>& fv2 = childVertex->GetFVarData(child);

    const int fvarcount = mesh->GetFVarCount();
    int fvarindex = 0;
    for (int fvaritem = 0; fvaritem < fvarcount; ++fvaritem) {
        bool infcorner = false;
        const int fvarwidth = mesh->GetFVarWidths()[fvaritem];
        const char fvarmask = v->GetFVarMask(fvaritem);
        if (fvarinterp == HbrMesh<T>::k_InterpolateBoundaryEdgeAndCorner) {
            if (fvarmask >= HbrVertex<T>::k_Corner) {
                infcorner = true;
            } else if (mesh->GetFVarPropagateCorners()) {
                if (v->IsFVarCorner(fvaritem)) {
                    infcorner = true;
                }
            } else {
                if (face->GetEdge(index)->GetFVarSharpness(fvaritem, true) && face->GetEdge(index)->GetPrev()->GetFVarSharpness(fvaritem, true)) {
                    infcorner = true;
                }
            }
        }

        // Infinitely sharp vertex rule. Applied if the vertex is:
        // - undergoing no facevarying boundary interpolation;
        // - at a geometric crease, in either boundary interpolation case; or
        // - is an infinitely sharp facevarying vertex, in the EDGEANDCORNER case; or
        // - has a mask equal or greater than one, in the "always
        //   sharp" interpolate boundary case
        if (fvarinterp == HbrMesh<T>::k_InterpolateBoundaryNone ||
            (fvarinterp == HbrMesh<T>::k_InterpolateBoundaryAlwaysSharp &&
             fvarmask >= 1) ||
            v->GetSharpness() > HbrVertex<T>::k_Smooth ||
            infcorner) {
            fv0.SetWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 1.0f);
        }
        // Dart rule: unlike geometric creases, because there's two
        // discontinuous values for the one incident edge, we use the
        // boundary rule and not the smooth rule
        else if (fvarmask == 1) {
            // Use 0.75 of the current vert
            fv0.SetWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.75f);

            // 0.125 of "two adjacent edge vertices", which in actuality
            // are the facevarying values of the same vertex but on each
            // side of the single incident facevarying sharp edge
            HbrHalfedge<T>* start = v->GetIncidentEdge(), *edge, *nextedge;
            edge = start;
            while (edge) {
                if (edge->GetFVarSharpness(fvaritem)) {
                    break;
                }
                nextedge = v->GetNextEdge(edge);
                if (nextedge == start) {
                    assert(0); // we should have found it by now
                    break;
                } else if (!nextedge) {
                    // should never get into this case - if the vertex is
                    // on a boundary, it can never be a facevarying dart
                    // vertex
                    assert(0);
                    edge = edge->GetPrev();
                    break;
                } else {
                    edge = nextedge;
                }
            }
            HbrVertex<T>* w = edge->GetDestVertex();
            HbrFace<T>* bestface = edge->GetLeftFace();
            int j;
            for (j = 0; j < bestface->GetNumVertices(); ++j) {
                if (bestface->GetVertex(j) == w) break;
            }
            assert(j != bestface->GetNumVertices());
            fv0.AddWithWeight(bestface->GetFVarData(j), fvarindex, fvarwidth, 0.125f);
            bestface = edge->GetRightFace();
            for (j = 0; j < bestface->GetNumVertices(); ++j) {
                if (bestface->GetVertex(j) == w) break;
            }
            assert(j != bestface->GetNumVertices());
            fv0.AddWithWeight(bestface->GetFVarData(j), fvarindex, fvarwidth, 0.125f);
        }
        // Boundary vertex rule (can use FVarSmooth, which is equivalent
        // to checking that it's sharper than a dart)
        else if (fvarmask != 0) {

            // Use 0.75 of the current vert
            fv0.SetWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.75f);

            // Compute 0.125 of two adjacent edge vertices. However the
            // two adjacent edge vertices we use must be part of the
            // facevarying "boundary". To find the first edge we cycle
            // counterclockwise around the current vertex v and look for
            // the first boundary edge

            HbrFace<T>* bestface = face;
            HbrHalfedge<T>* bestedge = face->GetEdge(index)->GetPrev();
            HbrHalfedge<T>* starte = bestedge->GetOpposite();
            HbrVertex<T>* w = 0;
            if (!starte) {
                w = face->GetEdge(index)->GetPrev()->GetOrgVertex();
            } else {
                HbrHalfedge<T>* e = starte, *next;
                assert(starte->GetOrgVertex() == v);
                do {
                    if (e->GetFVarSharpness(fvaritem) || !e->GetLeftFace()) {
                        bestface = e->GetRightFace();
                        bestedge = e;
                        break;
                    }
                    next = v->GetNextEdge(e);
                    if (!next) {
                        bestface = e->GetLeftFace();
                        w = e->GetPrev()->GetOrgVertex();
                        break;
                    }
                    e = next;
                } while (e && e != starte);
            }
            if (!w) w = bestedge->GetDestVertex();
            int j;
            for (j = 0; j < bestface->GetNumVertices(); ++j) {
                if (bestface->GetVertex(j) == w) break;
            }
            assert(j != bestface->GetNumVertices());
            fv0.AddWithWeight(bestface->GetFVarData(j), fvarindex, fvarwidth, 0.125f);

            // Look for the other edge by cycling clockwise around v
            bestface = face;
            bestedge = face->GetEdge(index);
            starte = bestedge;
            w = 0;
            if (HbrHalfedge<T>* e = starte) {
                assert(starte->GetOrgVertex() == v);
                do {
                    if (e->GetFVarSharpness(fvaritem) || !e->GetRightFace()) {
                        bestface = e->GetLeftFace();
                        bestedge = e;
                        break;
                    }
                    assert(e->GetOpposite());
                    e = v->GetPreviousEdge(e);
                } while (e && e != starte);
            }
            if (!w) w = bestedge->GetDestVertex();
            for (j = 0; j < bestface->GetNumVertices(); ++j) {
                if (bestface->GetVertex(j) == w) break;
            }
            assert(j != bestface->GetNumVertices());
            fv0.AddWithWeight(bestface->GetFVarData(j), fvarindex, fvarwidth, 0.125f);

        }
        // Smooth rule
        else if (!fv0IsSmooth || !fv0.IsInitialized()) {
            int valence = v->GetValence();
            float invvalence = 1.0f / valence;
            float beta = 0.25f * cosf((float)M_PI * 2.0f * invvalence) + 0.375f;
            beta = beta * beta;
            beta = (0.625f - beta) * invvalence;

            // Use 1 - beta * valence of the current vertex value
            fv0.SetWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 1 - (beta * valence));

            // Add beta of surrounding vertices averages. We loop over all
            // surrounding faces..
            HbrHalfedge<T>* start = v->GetIncidentEdge(), *edge;
            edge = start;
            while (edge) {
                HbrFace<T>* g = edge->GetLeftFace();

                // .. and look for the edge on that face whose origin is
                // the same as v, and add a contribution from its
                // destination vertex value; this takes care of the
                // surrounding edge vertex addition.
                for (int j = 0; j < g->GetNumVertices(); ++j) {
                    if (g->GetEdge(j)->GetOrgVertex() == v) {
                        fv0.AddWithWeight(g->GetFVarData((j + 1) % g->GetNumVertices()), fvarindex, fvarwidth, beta);
                        break;
                    }
                }
                edge = v->GetNextEdge(edge);
                if (edge == start) break;
            }
        }

        // Edge subdivision rule
        HbrHalfedge<T>* edge = face->GetEdge(index);

        if (fvarinterp == HbrMesh<T>::k_InterpolateBoundaryNone ||
            edge->GetFVarSharpness(fvaritem) || edge->IsBoundary()) {

            // Sharp edge rule
            fv1.SetWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.5f);
            fv1.AddWithWeight(face->GetFVarData((index + 1) % 3), fvarindex, fvarwidth, 0.5f);
        } else if (!fv1IsSmooth || !fv1.IsInitialized()) {
            // Smooth edge subdivision. Add 0.375 of adjacent vertices
            fv1.SetWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.375f);
            fv1.AddWithWeight(face->GetFVarData((index + 1) % 3), fvarindex, fvarwidth, 0.375f);
            // Add 0.125 of opposite vertices
            fv1.AddWithWeight(face->GetFVarData((index + 2) % 3), fvarindex, fvarwidth, 0.125f);
            HbrFace<T>* oppFace = edge->GetRightFace();
            for (int j = 0; j < oppFace->GetNumVertices(); ++j) {
                if (oppFace->GetVertex(j) == v) {
                    fv1.AddWithWeight(oppFace->GetFVarData((j+1)%oppFace->GetNumVertices()), fvarindex, fvarwidth, 0.125f);
                    break;
                }
            }
        }


        // Edge subdivision rule
        edge = edge->GetPrev();

        if (fvarinterp == HbrMesh<T>::k_InterpolateBoundaryNone ||
            edge->GetFVarSharpness(fvaritem) || edge->IsBoundary()) {

            // Sharp edge rule
            fv2.SetWithWeight(face->GetFVarData((index + 2) % 3), fvarindex, fvarwidth, 0.5f);
            fv2.AddWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.5f);
        } else if (!fv2IsSmooth || !fv2.IsInitialized()) {
            // Smooth edge subdivision. Add 0.375 of adjacent vertices
            fv2.SetWithWeight(face->GetFVarData((index + 2) % 3), fvarindex, fvarwidth, 0.375f);
            fv2.AddWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.375f);
            // Add 0.125 of opposite vertices
            fv2.AddWithWeight(face->GetFVarData((index + 1) % 3), fvarindex, fvarwidth, 0.125f);

            HbrFace<T>* oppFace = edge->GetRightFace();
            for (int j = 0; j < oppFace->GetNumVertices(); ++j) {
                if (oppFace->GetVertex(j) == v) {
                    fv2.AddWithWeight(oppFace->GetFVarData((j+2)%oppFace->GetNumVertices()), fvarindex, fvarwidth, 0.125f);
                    break;
                }
            }
        }

        fvarindex += fvarwidth;
    }
    fv0.SetInitialized();
    fv1.SetInitialized();
    fv2.SetInitialized();
}

template <class T>
void
HbrLoopSubdivision<T>::transferEditsToChild(HbrFace<T>* face, HbrFace<T>* child, int index) {

    // Hand down hole tag
    child->SetHole(face->IsHole());
 
    // Hand down pointers to hierarchical edits
    if (HbrHierarchicalEdit<T>** edits = face->GetHierarchicalEdits()) {
        while (HbrHierarchicalEdit<T>* edit = *edits) {
            if (!edit->IsRelevantToFace(face)) break;
            if (edit->GetNSubfaces() > face->GetDepth() &&
                (edit->GetSubface(face->GetDepth()) == index)) {
                child->SetHierarchicalEdits(edits);
                break;
            }
            edits++;
        }
    }
}

template <class T>
void
HbrLoopSubdivision<T>::Refine(HbrMesh<T>* mesh, HbrFace<T>* face) {

#ifdef HBR_DEBUG
    std::cerr << "\n\nRefining face " << *face << "\n";
#endif

    assert(face->GetNumVertices() == 3); // or triangulate it?

    HbrHalfedge<T>* edge = face->GetFirstEdge();
    HbrHalfedge<T>* prevedge = edge->GetPrev();
    for (int i = 0; i < 3; ++i) {
        HbrVertex<T>* vertex = edge->GetOrgVertex();
        if (!face->GetChild(i)) {
#ifdef HBR_DEBUG
            std::cerr << "Kid " << i << "\n";
#endif
            HbrFace<T>* child;
            HbrVertex<T>* vertices[3];

            vertices[i] = vertex->Subdivide();
            vertices[(i + 1) % 3] = edge->Subdivide();
            vertices[(i + 2) % 3] = prevedge->Subdivide();
            child = mesh->NewFace(3, vertices, face, i);
#ifdef HBR_DEBUG
            std::cerr << "Creating face " << *child << " during refine\n";
#endif

            // Hand down edge sharpness
            float sharpness;
            HbrHalfedge<T>* childedge;

            childedge = child->GetEdge(i);
            if ((sharpness = edge->GetSharpness()) > HbrHalfedge<T>::k_Smooth) {
                HbrSubdivision<T>::SubdivideCreaseWeight(
                    edge, edge->GetOrgVertex(), childedge);
            }
            childedge->CopyFVarInfiniteSharpness(edge);

            childedge = child->GetEdge((i+2)%3);
            if ((sharpness = prevedge->GetSharpness()) > HbrHalfedge<T>::k_Smooth) {
                HbrSubdivision<T>::SubdivideCreaseWeight(
                    prevedge, prevedge->GetDestVertex(), childedge);
            }
            childedge->CopyFVarInfiniteSharpness(prevedge);

            if (mesh->GetTotalFVarWidth()) {
                transferFVarToChild(mesh, face, child, i);
            }

            transferEditsToChild(face, child, i);

        }
        prevedge = edge;
        edge = edge->GetNext();
    }

    refineFaceAtMiddle(mesh, face);
}

template <class T>
HbrFace<T>*
HbrLoopSubdivision<T>::RefineFaceAtVertex(HbrMesh<T>* mesh, HbrFace<T>* face, HbrVertex<T>* vertex) {

#ifdef HBR_DEBUG
    std::cerr << "    forcing refine on " << *face << " at " << *vertex << '\n';
#endif
    HbrHalfedge<T>* edge = face->GetFirstEdge();
    HbrHalfedge<T>* prevedge = edge->GetPrev();

    for (int i = 0; i < 3; ++i) {
        if (edge->GetOrgVertex() == vertex) {
            if (!face->GetChild(i)) {
#ifdef HBR_DEBUG
                std::cerr << "Kid " << i << "\n";
#endif
                HbrFace<T>* child;
                HbrVertex<T>* vertices[3];

                vertices[i] = vertex->Subdivide();
                vertices[(i + 1) % 3] = edge->Subdivide();
                vertices[(i + 2) % 3] = prevedge->Subdivide();
                child = mesh->NewFace(3, vertices, face, i);
#ifdef HBR_DEBUG
                std::cerr << "Creating face " << *child << " during refine\n";
#endif

                // Hand down edge sharpness
                float sharpness;
                HbrHalfedge<T>* childedge;

                childedge = child->GetEdge(i);
                if ((sharpness = edge->GetSharpness()) > HbrHalfedge<T>::k_Smooth) {
                    HbrSubdivision<T>::SubdivideCreaseWeight(
                        edge, edge->GetOrgVertex(), childedge);
                }
                childedge->CopyFVarInfiniteSharpness(edge);

                childedge = child->GetEdge((i+2)%3);
                if ((sharpness = prevedge->GetSharpness()) > HbrHalfedge<T>::k_Smooth) {
                    HbrSubdivision<T>::SubdivideCreaseWeight(
                        prevedge, prevedge->GetDestVertex(), childedge);
                }
                childedge->CopyFVarInfiniteSharpness(prevedge);

                if (mesh->GetTotalFVarWidth()) {
                    transferFVarToChild(mesh, face, child, i);
                }

                transferEditsToChild(face, child, i);

                return child;
            } else {
                return face->GetChild(i);
            }
        }
        prevedge = edge;
        edge = edge->GetNext();
    }
    return 0;
}

template <class T>
void
HbrLoopSubdivision<T>::GuaranteeNeighbor(HbrMesh<T>* mesh, HbrHalfedge<T>* edge) {
    if (edge->GetOpposite()) {
        return;
    }

#ifdef HBR_DEBUG
    std::cerr << "\n\nneighbor guarantee at " << *edge << " invoked\n";
#endif

    /*
      Imagine the following:

                      X
                     / \
                    /   \
                   /     \
                  X       \
                 /\        \
               2/  \3       \
               /    \        \
              X------X--------X
                 1

     If the parent of _both_ incident vertices are themselves edges,
     (like the edge marked 3 above), then this edge is in the center
     of the parent face. Refining the parent face in the middle or
     refining the parent face at one vertex (where the two parent
     edges meet) should suffice
    */
    HbrHalfedge<T>* parentEdge1 = edge->GetOrgVertex()->GetParentEdge();
    HbrHalfedge<T>* parentEdge2 = edge->GetDestVertex()->GetParentEdge();
    if (parentEdge1 && parentEdge2) {
#ifdef HBR_DEBUG
        std::cerr << "two parent edge situation\n";
#endif
        HbrFace<T>* parentFace = parentEdge1->GetFace();
        assert(parentFace == parentEdge2->GetFace());
        if(parentEdge1->GetOrgVertex() == parentEdge2->GetDestVertex()) {
            refineFaceAtMiddle(mesh, parentFace);
        } else {
            RefineFaceAtVertex(mesh, parentFace, parentEdge1->GetOrgVertex());
        }
        assert(edge->GetOpposite());
        return;
    }

    // Otherwise we're in the situation of edge 1 or edge 2 in the
    // diagram above.
    if (parentEdge1) {
#ifdef HBR_DEBUG
        std::cerr << "parent edge 1 " << *parentEdge1 << "\n";
#endif
        HbrVertex<T>* parentVertex2 = edge->GetDestVertex()->GetParentVertex();
        assert(parentVertex2);
        RefineFaceAtVertex(mesh, parentEdge1->GetLeftFace(), parentVertex2);
        if (parentEdge1->GetRightFace()) {
            RefineFaceAtVertex(mesh, parentEdge1->GetRightFace(), parentVertex2);
        }
    } else if (parentEdge2) {
#ifdef HBR_DEBUG
        std::cerr << "parent edge 2 " << *parentEdge2 << "\n";
#endif
        HbrVertex<T>* parentVertex1 = edge->GetOrgVertex()->GetParentVertex();
        assert(parentVertex1);
        RefineFaceAtVertex(mesh, parentEdge2->GetLeftFace(), parentVertex1);
        if (parentEdge2->GetRightFace()) {
            RefineFaceAtVertex(mesh, parentEdge2->GetRightFace(), parentVertex1);
        }
    }
}

template <class T>
void
HbrLoopSubdivision<T>::GuaranteeNeighbors(HbrMesh<T>* mesh, HbrVertex<T>* vertex) {

#ifdef HBR_DEBUG
    std::cerr << "\n\nneighbor guarantee at " << *vertex << " invoked\n";
#endif

    assert(vertex->GetParentFace() == 0);

    // The first case: the vertex is a child of an edge. Make sure
    // that the parent faces on either side of the parent edge exist,
    // and have 1) refined at both vertices of the parent edge, and 2)
    // have refined their "middle" face (which doesn't live at either
    // vertex).

    HbrHalfedge<T>* parentEdge = vertex->GetParentEdge();
    if (parentEdge) {
#ifdef HBR_DEBUG
        std::cerr << "parent edge situation " << *parentEdge << "\n";
#endif
        HbrVertex<T>* dest = parentEdge->GetDestVertex();
        HbrVertex<T>* org = parentEdge->GetOrgVertex();
        GuaranteeNeighbor(mesh, parentEdge);
        HbrFace<T>* parentFace = parentEdge->GetLeftFace();
        RefineFaceAtVertex(mesh, parentFace, dest);
        RefineFaceAtVertex(mesh, parentFace, org);
        refineFaceAtMiddle(mesh, parentFace);
        parentFace = parentEdge->GetRightFace();
        // The right face may not necessarily exist even after
        // GuaranteeNeighbor
        if (parentFace) {
            RefineFaceAtVertex(mesh, parentFace, dest);
            RefineFaceAtVertex(mesh, parentFace, org);
            refineFaceAtMiddle(mesh, parentFace);
        }
        return;
    }

    // The second case: the vertex is a child of a vertex. In this case
    // we have to recursively guarantee that the parent's adjacent
    // faces also exist.
    HbrVertex<T>* parentVertex = vertex->GetParentVertex();
    if (parentVertex) {
#ifdef HBR_DEBUG
        std::cerr << "parent vertex situation " << *parentVertex << "\n";
#endif
        parentVertex->GuaranteeNeighbors();

        // And then we refine all the face neighbors of the parent
        // vertex
        HbrHalfedge<T>* start = parentVertex->GetIncidentEdge(), *edge;
        edge = start;
        while (edge) {
            HbrFace<T>* f = edge->GetLeftFace();
            RefineFaceAtVertex(mesh, f, parentVertex);
            edge = parentVertex->GetNextEdge(edge);
            if (edge == start) break;
        }
    }
}

template <class T>
bool
HbrLoopSubdivision<T>::HasLimit(HbrMesh<T>* mesh, HbrFace<T>* face) {

    if (face->IsHole()) return false;
    // A limit face exists if all the bounding edges have limit curves
    for (int i = 0; i < face->GetNumVertices(); ++i) {
        if (!HasLimit(mesh, face->GetEdge(i))) {
            return false;
        }
    }
    return true;
}

template <class T>
bool
HbrLoopSubdivision<T>::HasLimit(HbrMesh<T>* mesh, HbrHalfedge<T>* edge) {
    //  A sharp edge has a limit curve if both endpoints have limits.
    //  A smooth edge has a limit if both endpoints have limits and
    //  the edge isn't on the boundary.

    if (edge->GetSharpness() >= HbrHalfedge<T>::k_InfinitelySharp) return true;

    if (!HasLimit(mesh, edge->GetOrgVertex()) || !HasLimit(mesh, edge->GetDestVertex())) return false;

    return !edge->IsBoundary();
}

template <class T>
bool
HbrLoopSubdivision<T>::HasLimit(HbrMesh<T>* /* mesh */, HbrVertex<T>* vertex) {
    vertex->GuaranteeNeighbors();
    switch (vertex->GetMask(false)) {
        case HbrVertex<T>::k_Smooth:
        case HbrVertex<T>::k_Dart:
            return !vertex->OnBoundary();
            break;
        case HbrVertex<T>::k_Crease:
        case HbrVertex<T>::k_Corner:
        default:
            if (vertex->IsVolatile()) {
                // Search for any incident semisharp boundary edge
                HbrHalfedge<T>* start = vertex->GetIncidentEdge(), *edge, *next;
                edge = start;
                while (edge) {
                    if (edge->IsBoundary() && edge->GetSharpness() < HbrHalfedge<T>::k_InfinitelySharp) {
                        return false;
                    }
                    next = vertex->GetNextEdge(edge);
                    if (next == start) {
                        break;
                    } else if (!next) {
                        edge = edge->GetPrev();
                        if (edge->IsBoundary() && edge->GetSharpness() < HbrHalfedge<T>::k_InfinitelySharp) {
                            return false;
                        }
                        break;
                    } else {
                        edge = next;
                    }
                }
            }
            return true;
    }
}

template <class T>
HbrVertex<T>*
HbrLoopSubdivision<T>::Subdivide(HbrMesh<T>* /* mesh */, HbrFace<T>* /* face */) {
    // In loop subdivision, faces never subdivide
    assert(0);
    return 0;
}

template <class T>
HbrVertex<T>*
HbrLoopSubdivision<T>::Subdivide(HbrMesh<T>* mesh, HbrHalfedge<T>* edge) {

#ifdef HBR_DEBUG
    std::cerr << "Subdividing at " << *edge << "\n";
#endif
    // Ensure the opposite face exists.
    GuaranteeNeighbor(mesh, edge);

    float esharp = edge->GetSharpness();
    HbrVertex<T>* v = mesh->NewVertex();
    T& data = v->GetData();

    // If there's the possibility of vertex edits on either vertex, we
    // have to make sure the edit has been applied
    if (mesh->HasVertexEdits()) {
        edge->GetOrgVertex()->GuaranteeNeighbors();
        edge->GetDestVertex()->GuaranteeNeighbors();
    }

    if (!edge->IsBoundary() && esharp <= 1.0f) {

        // Of the two half-edges, pick one of them consistently such
        // that the org and dest vertices are also consistent through
        // multi-threading. It doesn't matter as far as the
        // theoretical calculation is concerned, but it is desirable
        // to be consistent about it in the face of the limitations of
        // floating point commutativity. So we always pick the
        // half-edge such that its incident face is the smallest of
        // the two faces, as far as the face paths are concerned.
        if (edge->GetOpposite() && edge->GetOpposite()->GetFace()->GetPath() < edge->GetFace()->GetPath()) {
            edge = edge->GetOpposite();
        }

        // Handle both the smooth and fractional sharpness cases.  We
        // lerp between the sharp case (average of the two end points)
        // and the unsharp case (3/8 of each of the two end points
        // plus 1/8 of the two opposite face averages).

        // Lerp end point weight between non sharp contribution of
        // 3/8 and the sharp contribution of 0.5.
        float endPtWeight = 0.375f + esharp * (0.5f - 0.375f);
        data.AddWithWeight(edge->GetOrgVertex()->GetData(), endPtWeight);
        data.AddWithWeight(edge->GetDestVertex()->GetData(), endPtWeight);

        // Lerp the opposite pt weights between non sharp contribution
        // of 1/8 and the sharp contribution of 0.
        float oppPtWeight = 0.125f * (1 - esharp);
        HbrHalfedge<T>* ee = edge->GetNext();
        data.AddWithWeight(ee->GetDestVertex()->GetData(), oppPtWeight);
        ee = edge->GetOpposite()->GetNext();
        data.AddWithWeight(ee->GetDestVertex()->GetData(), oppPtWeight);
    } else {
        // Fully sharp edge, just average the two end points
        data.AddWithWeight(edge->GetOrgVertex()->GetData(), 0.5f);
        data.AddWithWeight(edge->GetDestVertex()->GetData(), 0.5f);
    }

    // Varying data is always the average of two end points
    data.AddVaryingWithWeight(edge->GetOrgVertex()->GetData(), 0.5f);
    data.AddVaryingWithWeight(edge->GetDestVertex()->GetData(), 0.5f);

#ifdef HBR_DEBUG
    std::cerr << "  created " << *v << "\n";
#endif

    // Only boundary edges will create extraordinary vertices
    if (edge->IsBoundary()) {
        v->SetExtraordinary();
    }
    return v;
}

template <class T>
HbrVertex<T>*
HbrLoopSubdivision<T>::Subdivide(HbrMesh<T>* mesh, HbrVertex<T>* vertex) {

    // Ensure the ring of faces around this vertex exists before
    // we compute the valence
    vertex->GuaranteeNeighbors();

    float valence = static_cast<float>(vertex->GetValence());
    float invvalence = 1.0f / valence;

    HbrVertex<T>* v = mesh->NewVertex();
    T& data = v->GetData();

    // Due to fractional weights we may need to do two subdivision
    // passes
    int masks[2];
    float weights[2];
    int passes;
    masks[0] = vertex->GetMask(false);
    masks[1] = vertex->GetMask(true);
    // If the masks are different, we subdivide twice: once using the
    // current mask, once using the mask at the next level of
    // subdivision, then use fractional mask weights to weigh
    // each weighing
    if (masks[0] != masks[1]) {
        weights[1] = vertex->GetFractionalMask();
        weights[0] = 1.0f - weights[1];
        passes = 2;
    } else {
        weights[0] = 1.0f;
        weights[1] = 0.0f;
        passes = 1;
    }
    for (int i = 0; i < passes; ++i) {
        switch (masks[i]) {
            case HbrVertex<T>::k_Smooth:
            case HbrVertex<T>::k_Dart: {
                float beta = 0.25f * cosf((float)M_PI * 2.0f * invvalence) + 0.375f;
                beta = beta * beta;
                beta = (0.625f - beta) * invvalence;

                data.AddWithWeight(vertex->GetData(), weights[i] * (1 - (beta * valence)));

                HbrSubdivision<T>::AddSurroundingVerticesWithWeight(
                    mesh, vertex, weights[i] * beta, &data);
                break;
            }
            case HbrVertex<T>::k_Crease: {
                // Compute 3/4 of old vertex value
                data.AddWithWeight(vertex->GetData(), weights[i] * 0.75f);

                // Add 0.125f of the (hopefully only two!) neighbouring
                // sharp edges
                HbrSubdivision<T>::AddCreaseEdgesWithWeight(
                    mesh, vertex, i == 1, weights[i] * 0.125f, &data);
                break;
            }
            case HbrVertex<T>::k_Corner:
            default: {
                // Just copy the old value
                data.AddWithWeight(vertex->GetData(), weights[i]);
                break;
            }
        }
    }

    // Varying data is always just propagated down
    data.AddVaryingWithWeight(vertex->GetData(), 1.0f);

#ifdef HBR_DEBUG
    std::cerr << "Subdividing at " << *vertex << "\n";
    std::cerr << "  created " << *v << "\n";
#endif
    // Inherit extraordinary flag and sharpness
    if (vertex->IsExtraordinary()) v->SetExtraordinary();
    float sharp = vertex->GetSharpness();
    if (sharp >= HbrVertex<T>::k_InfinitelySharp) {
        v->SetSharpness(HbrVertex<T>::k_InfinitelySharp);
    } else if (sharp > HbrVertex<T>::k_Smooth) {
        v->SetSharpness(std::max((float) HbrVertex<T>::k_Smooth, sharp - 1.0f));
    } else {
        v->SetSharpness(HbrVertex<T>::k_Smooth);
    }
    return v;
}

template <class T>
void
HbrLoopSubdivision<T>::refineFaceAtMiddle(HbrMesh<T>* mesh, HbrFace<T>* face) {

#ifdef HBR_DEBUG
    std::cerr << "Refining middle face of " << *face << "\n";
#endif

    if (!face->GetChild(3)) {
        HbrFace<T>* child;
        HbrVertex<T>* vertices[3];

        // The fourth face is not an obvious child of any vertex. We
        // assign it index 3 despite there being no fourth vertex in
        // the triangle. The ordering of vertices here is done to
        // preserve parametric space as best we can
        vertices[0] = face->GetEdge(1)->Subdivide();
        vertices[1] = face->GetEdge(2)->Subdivide();
        vertices[2] = face->GetEdge(0)->Subdivide();
        child = mesh->NewFace(3, vertices, face, 3);
#ifdef HBR_DEBUG
        std::cerr << "Creating face " << *child << "\n";
#endif
        if (mesh->GetTotalFVarWidth()) {
            transferFVarToChild(mesh, face, child, 3);
        }

        transferEditsToChild(face, child, 3);
    }
}

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRLOOP_H */
