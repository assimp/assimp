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

#ifndef OPENSUBDIV3_HBRBILINEAR_H
#define OPENSUBDIV3_HBRBILINEAR_H

/*#define HBR_DEBUG */
#include "../hbr/subdivision.h"

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T>
class HbrBilinearSubdivision : public HbrSubdivision<T> {
public:
    HbrBilinearSubdivision<T>()
        : HbrSubdivision<T>() {}

    virtual HbrSubdivision<T>* Clone() const {
        return new HbrBilinearSubdivision<T>();
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

    virtual bool VertexIsExtraordinary(HbrMesh<T> const * /* mesh */, HbrVertex<T>* vertex) { return vertex->GetValence() != 4; }
    virtual bool FaceIsExtraordinary(HbrMesh<T> const * /* mesh */, HbrFace<T>* face) { return face->GetNumVertices() != 4; }

    virtual int GetFaceChildrenCount(int nvertices) const { return nvertices; }

private:

    // Transfers facevarying data from a parent face to a child face
    void transferFVarToChild(HbrMesh<T>* mesh, HbrFace<T>* face, HbrFace<T>* child, int index);

    // Transfers vertex and edge edits from a parent face to a child face
    void transferEditsToChild(HbrFace<T>* face, HbrFace<T>* child, int index);
};

template <class T>
void
HbrBilinearSubdivision<T>::transferFVarToChild(HbrMesh<T>* mesh, HbrFace<T>* face, HbrFace<T>* child, int index) {

    typename HbrMesh<T>::InterpolateBoundaryMethod fvarinterp = mesh->GetFVarInterpolateBoundaryMethod();
    const int fvarcount = mesh->GetFVarCount();
    int fvarindex = 0;
    const int nv = face->GetNumVertices();
    bool extraordinary = (nv != 4);
    HbrVertex<T> *v = face->GetVertex(index), *childVertex;
    HbrHalfedge<T>* edge;

    // We do the face subdivision rule first, because we may reuse the
    // result (stored in fv2) for the other subdivisions.
    float weight =  1.0f / nv;

    // For the face center vertex, the facevarying data can be cleared
    // and averaged en masse, since the subdivision rules don't change
    // for any of the data - we use the smooth rule for all of it.
    // And since we know that the fvardata for this particular vertex
    // is smooth and therefore shareable amongst all incident faces,
    // we don't have to allocate extra storage for it. We also don't
    // have to compute it if some other face got to it first (as
    // indicated by the IsInitialized() flag).
    HbrFVarData<T>& fv2 = child->GetFVarData(extraordinary ? 2 : (index+2)%4);
    if (!fv2.IsInitialized()) {
        const int totalfvarwidth = mesh->GetTotalFVarWidth();
        fv2.ClearAll(totalfvarwidth);
        for (int j = 0; j < nv; ++j) {
            fv2.AddWithWeightAll(face->GetFVarData(j), totalfvarwidth, weight);
        }
    }
    assert(fv2.IsInitialized());

    v->GuaranteeNeighbors();

    // Make sure that that each of the vertices of the child face have
    // the appropriate facevarying storage as needed. If there are
    // discontinuities in any facevarying datum, the vertex must
    // allocate a new block of facevarying storage specific to the
    // child face.
    bool fv0IsSmooth, fv1IsSmooth, fv3IsSmooth;

    childVertex = child->GetVertex(extraordinary ? 0 : (index+0)%4);
    fv0IsSmooth = v->IsFVarAllSmooth();
    if (!fv0IsSmooth) {
        childVertex->NewFVarData(child);
    }
    HbrFVarData<T>& fv0 = childVertex->GetFVarData(child);

    edge = face->GetEdge(index);
    GuaranteeNeighbor(mesh, edge);
    assert(edge->GetOrgVertex() == v);
    childVertex = child->GetVertex(extraordinary ? 1 : (index+1)%4);
    fv1IsSmooth = !edge->IsFVarInfiniteSharpAnywhere();
    if (!fv1IsSmooth) {
        childVertex->NewFVarData(child);
    }
    HbrFVarData<T>& fv1 = childVertex->GetFVarData(child);

    edge = edge->GetPrev();
    GuaranteeNeighbor(mesh, edge);
    assert(edge == face->GetEdge((index + nv - 1) % nv));
    assert(edge->GetDestVertex() == v);
    childVertex = child->GetVertex(extraordinary ? 3 : (index+3)%4);
    fv3IsSmooth = !edge->IsFVarInfiniteSharpAnywhere();
    if (!fv3IsSmooth) {
        childVertex->NewFVarData(child);
    }
    HbrFVarData<T>& fv3 = childVertex->GetFVarData(child);
    fvarindex = 0;
    for (int fvaritem = 0; fvaritem < fvarcount; ++fvaritem) {
        // Vertex subdivision rule. Analyze whether the vertex is on the
        // boundary and whether it's an infinitely sharp corner. We
        // determine the last by checking the propagate corners flag on
        // the mesh; if it's off, we check the two edges of this face
        // incident to that vertex and determining whether they are
        // facevarying boundary edges - this is analogous to what goes on
        // for the interpolateboundary tag (which when set to
        // EDGEANDCORNER marks vertices with a valence of two as being
        // sharp corners). If propagate corners is on, we check *all*
        // faces to see if two edges side by side are facevarying boundary
        // edges. The facevarying boundary check ignores geometric
        // sharpness, otherwise we may swim at geometric creases which
        // aren't actually discontinuous.
        bool infcorner = false;
        const int fvarwidth = mesh->GetFVarWidths()[fvaritem];
        const unsigned char fvarmask = v->GetFVarMask(fvaritem);
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
            assert(!v->OnBoundary());

            // Use 0.75 of the current vert
            fv0.SetWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.75f);

            // 0.125 of "two adjacent edge vertices", which in actuality
            // are the facevarying values of the same vertex but on each
            // side of the single incident facevarying sharp edge
            HbrHalfedge<T>* start = v->GetIncidentEdge(), *nextedge;
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
        // Boundary vertex rule
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
        // Smooth rule. Here, we can take a shortcut if we know that
        // the vertex is smooth and some other vertex has completely
        // computed the facevarying values
        else if (!fv0IsSmooth || !fv0.IsInitialized()) {
            int valence = v->GetValence();
            float invvalencesquared = 1.0f / (valence * valence);

            // Use n-2/n of the current vertex value
            fv0.SetWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, invvalencesquared * valence * (valence - 2));

            // Add 1/n^2 of surrounding edge vertices and surrounding face
            // averages. We loop over all surrounding faces..
            HbrHalfedge<T>* start = v->GetIncidentEdge(), *edge;
            edge = start;
            while (edge) {
                HbrFace<T>* g = edge->GetLeftFace();
                weight = invvalencesquared / g->GetNumVertices();
                // .. and compute the average of each face. At the same
                // time, we look for the edge on that face whose origin is
                // the same as v, and add a contribution from its
                // destination vertex value; this takes care of the
                // surrounding edge vertex addition.
                for (int j = 0; j < g->GetNumVertices(); ++j) {
                    fv0.AddWithWeight(g->GetFVarData(j), fvarindex, fvarwidth, weight);
                    if (g->GetEdge(j)->GetOrgVertex() == v) {
                        fv0.AddWithWeight(g->GetFVarData((j + 1) % g->GetNumVertices()), fvarindex, fvarwidth, invvalencesquared);
                    }
                }
                edge = v->GetNextEdge(edge);
                if (edge == start) break;
            }
        }

        // Edge subdivision rule
        edge = face->GetEdge(index);

        if (fvarinterp == HbrMesh<T>::k_InterpolateBoundaryNone ||
            edge->GetFVarSharpness(fvaritem) || edge->IsBoundary()) {

            // Sharp edge rule
            fv1.SetWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.5f);
            fv1.AddWithWeight(face->GetFVarData((index + 1) % nv), fvarindex, fvarwidth, 0.5f);
        } else if (!fv1IsSmooth || !fv1.IsInitialized()) {
            // Smooth edge subdivision. Add 0.25 of adjacent vertices
            fv1.SetWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.25f);
            fv1.AddWithWeight(face->GetFVarData((index + 1) % nv), fvarindex, fvarwidth, 0.25f);
            // Local subdivided face vertex
            fv1.AddWithWeight(fv2, fvarindex, fvarwidth, 0.25f);
            // Add 0.25 * average of neighboring face vertices
            HbrFace<T>* oppFace = edge->GetRightFace();
            weight = 0.25f / oppFace->GetNumVertices();
            for (int j = 0; j < oppFace->GetNumVertices(); ++j) {
                fv1.AddWithWeight(oppFace->GetFVarData(j), fvarindex, fvarwidth, weight);
            }
        }


        // Edge subdivision rule
        edge = edge->GetPrev();

        if (fvarinterp == HbrMesh<T>::k_InterpolateBoundaryNone ||
            edge->GetFVarSharpness(fvaritem) || edge->IsBoundary()) {

            // Sharp edge rule
            fv3.SetWithWeight(face->GetFVarData((index + nv - 1) % nv), fvarindex, fvarwidth, 0.5f);
            fv3.AddWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.5f);
        } else if (!fv3IsSmooth || !fv3.IsInitialized()) {
            // Smooth edge subdivision. Add 0.25 of adjacent vertices
            fv3.SetWithWeight(face->GetFVarData((index + nv - 1) % nv), fvarindex, fvarwidth, 0.25f);
            fv3.AddWithWeight(face->GetFVarData(index), fvarindex, fvarwidth, 0.25f);
            // Local subdivided face vertex
            fv3.AddWithWeight(fv2, fvarindex, fvarwidth, 0.25f);
            // Add 0.25 * average of neighboring face vertices
            HbrFace<T>* oppFace = edge->GetRightFace();
            weight = 0.25f / oppFace->GetNumVertices();
            for (int j = 0; j < oppFace->GetNumVertices(); ++j) {
                fv3.AddWithWeight(oppFace->GetFVarData(j), fvarindex, fvarwidth, weight);
            }
        }

        fvarindex += fvarwidth;
    }
    fv0.SetInitialized();
    fv1.SetInitialized();
    fv3.SetInitialized();
}

template <class T>
void
HbrBilinearSubdivision<T>::transferEditsToChild(HbrFace<T>* face, HbrFace<T>* child, int index) {

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
HbrBilinearSubdivision<T>::Refine(HbrMesh<T>* mesh, HbrFace<T>* face) {

    // Create new quadrilateral children faces from this face
    HbrFace<T>* child;
    HbrVertex<T>* vertices[4];
    HbrHalfedge<T>* edge = face->GetFirstEdge();
    HbrHalfedge<T>* prevedge = edge->GetPrev();
    HbrHalfedge<T>* childedge;
    int nv = face->GetNumVertices();
    float sharpness;
    bool extraordinary = (nv != 4);
    // The funny indexing on vertices is done only for
    // non-extraordinary faces in order to correctly preserve
    // parametric space through the refinement. If we split an
    // extraordinary face then it doesn't matter.
    for (int i = 0; i < nv; ++i) {
        if (!face->GetChild(i)) {
#ifdef HBR_DEBUG
            std::cerr << "Kid " << i << "\n";
#endif
            HbrVertex<T>* vertex = edge->GetOrgVertex();
            if (extraordinary) {
                vertices[0] = vertex->Subdivide();
                vertices[1] = edge->Subdivide();
                vertices[2] = face->Subdivide();
                vertices[3] = prevedge->Subdivide();
            } else {
                vertices[i] = vertex->Subdivide();
                vertices[(i+1)%4] = edge->Subdivide();
                vertices[(i+2)%4] = face->Subdivide();
                vertices[(i+3)%4] = prevedge->Subdivide();
            }
            child = mesh->NewFace(4, vertices, face, i);
#ifdef HBR_DEBUG
            std::cerr << "Creating face " << *child << " during refine\n";
#endif

            // Hand down edge sharpnesses
            childedge = vertex->Subdivide()->GetEdge(edge->Subdivide());
            assert(childedge);
            if ((sharpness = edge->GetSharpness()) > HbrHalfedge<T>::k_Smooth) {
                HbrSubdivision<T>::SubdivideCreaseWeight(edge, edge->GetOrgVertex(), childedge);
            }
            childedge->CopyFVarInfiniteSharpness(edge);

            childedge = prevedge->Subdivide()->GetEdge(vertex->Subdivide());
            assert(childedge);
            if ((sharpness = prevedge->GetSharpness()) > HbrHalfedge<T>::k_Smooth) {
                HbrSubdivision<T>::SubdivideCreaseWeight(prevedge, prevedge->GetDestVertex(), childedge);
            }
            childedge->CopyFVarInfiniteSharpness(prevedge);

            if (mesh->GetTotalFVarWidth()) {
                transferFVarToChild(mesh, face, child, i);
            }

            // Special handling of ptex index for extraordinary faces: make
            // sure the children get their indices reassigned to be
            // consecutive within the block reserved for the parent.
            if (face->GetNumVertices() != 4 && face->GetPtexIndex() != -1) {
                child->SetPtexIndex(face->GetPtexIndex() + i);
            }

            transferEditsToChild(face, child, i);
        }
        prevedge = edge;
        edge = edge->GetNext();
    }
}

template <class T>
HbrFace<T>*
HbrBilinearSubdivision<T>::RefineFaceAtVertex(HbrMesh<T>* mesh, HbrFace<T>* face, HbrVertex<T>* vertex) {

#ifdef HBR_DEBUG
    std::cerr << "    forcing refine on " << *face << " at " << *vertex << '\n';
#endif

    // Create new quadrilateral children faces from this face
    HbrHalfedge<T>* edge = face->GetFirstEdge();
    HbrHalfedge<T>* prevedge = edge->GetPrev();
    HbrHalfedge<T>* childedge;
    int nv = face->GetNumVertices();
    float sharpness;
    bool extraordinary = (nv != 4);
    // The funny indexing on vertices is done only for
    // non-extraordinary faces in order to correctly preserve
    // parametric space through the refinement. If we split an
    // extraordinary face then it doesn't matter.
    for (int i = 0; i < nv; ++i) {
        if (edge->GetOrgVertex() == vertex) {
            if (!face->GetChild(i)) {
                HbrFace<T>* child;
                HbrVertex<T>* vertices[4];
                if (extraordinary) {
                    vertices[0] = vertex->Subdivide();
                    vertices[1] = edge->Subdivide();
                    vertices[2] = face->Subdivide();
                    vertices[3] = prevedge->Subdivide();
                } else {
                    vertices[i] = vertex->Subdivide();
                    vertices[(i+1)%4] = edge->Subdivide();
                    vertices[(i+2)%4] = face->Subdivide();
                    vertices[(i+3)%4] = prevedge->Subdivide();
                }
#ifdef HBR_DEBUG
                std::cerr << "Kid " << i << "\n";
                std::cerr << "  subdivision created " << *vertices[0] << '\n';
                std::cerr << "  subdivision created " << *vertices[1] << '\n';
                std::cerr << "  subdivision created " << *vertices[2] << '\n';
                std::cerr << "  subdivision created " << *vertices[3] << '\n';
#endif
                child = mesh->NewFace(4, vertices, face, i);
#ifdef HBR_DEBUG
                std::cerr << "Creating face " << *child << " during refine\n";
#endif
                // Hand down edge sharpness
                childedge = vertex->Subdivide()->GetEdge(edge->Subdivide());
                assert(childedge);
                if ((sharpness = edge->GetSharpness()) > HbrHalfedge<T>::k_Smooth) {
                    HbrSubdivision<T>::SubdivideCreaseWeight(edge, edge->GetOrgVertex(), childedge);
                }
                childedge->CopyFVarInfiniteSharpness(edge);

                childedge = prevedge->Subdivide()->GetEdge(vertex->Subdivide());
                assert(childedge);
                if ((sharpness = prevedge->GetSharpness()) > HbrHalfedge<T>::k_Smooth) {
                    HbrSubdivision<T>::SubdivideCreaseWeight(prevedge, prevedge->GetDestVertex(), childedge);
                }
                childedge->CopyFVarInfiniteSharpness(prevedge);

                if (mesh->GetTotalFVarWidth()) {
                    transferFVarToChild(mesh, face, child, i);
                }

                // Special handling of ptex index for extraordinary faces: make
                // sure the children get their indices reassigned to be
                // consecutive within the block reserved for the parent.
                if (face->GetNumVertices() != 4 && face->GetPtexIndex() != -1) {
                    child->SetPtexIndex(face->GetPtexIndex() + i);
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
HbrBilinearSubdivision<T>::GuaranteeNeighbor(HbrMesh<T>* mesh, HbrHalfedge<T>* edge) {
    if (edge->GetOpposite()) {
        return;
    }

    // For the given edge: if the parent of either of its incident
    // vertices is itself a _face_, then ensuring that this parent
    // face has refined at a particular vertex is sufficient to
    // ensure that both of the faces on each side of the edge have
    // been created.
    bool destParentWasEdge = true;
    HbrFace<T>* parentFace = edge->GetOrgVertex()->GetParentFace();
    HbrHalfedge<T>* parentEdge = edge->GetDestVertex()->GetParentEdge();
    if (!parentFace) {
        destParentWasEdge = false;
        parentFace = edge->GetDestVertex()->GetParentFace();
        parentEdge = edge->GetOrgVertex()->GetParentEdge();
    }

    if (parentFace) {

        // Make sure we deal with a parent halfedge which is
        // associated with the parent face
        if (parentEdge->GetFace() != parentFace) {
            parentEdge = parentEdge->GetOpposite();
        }
        // If one of the vertices had a parent face, the other one MUST
        // have been a child of an edge
        assert(parentEdge && parentEdge->GetFace() == parentFace);
#ifdef HBR_DEBUG
        std::cerr << "\nparent edge is " << *parentEdge << "\n";
#endif

        // The vertex to refine at depends on whether the
        // destination or origin vertex of this edge had a parent
        // edge
        if (destParentWasEdge) {
            RefineFaceAtVertex(mesh, parentFace, parentEdge->GetOrgVertex());
        } else {
            RefineFaceAtVertex(mesh, parentFace, parentEdge->GetDestVertex());
        }

        // It should always be the case that the opposite now exists -
        // we can't have a boundary case here
        assert(edge->GetOpposite());
    } else {
        HbrVertex<T>* parentVertex = edge->GetOrgVertex()->GetParentVertex();
        parentEdge = edge->GetDestVertex()->GetParentEdge();
        if (!parentVertex) {
            parentVertex = edge->GetDestVertex()->GetParentVertex();
            parentEdge = edge->GetOrgVertex()->GetParentEdge();
        }

        if (parentVertex) {

            assert(parentEdge);

#ifdef HBR_DEBUG
            std::cerr << "\nparent edge is " << *parentEdge << "\n";
#endif

            // 1. Go up to the parent of my face

            parentFace = edge->GetFace()->GetParent();
#ifdef HBR_DEBUG
            std::cerr << "\nparent face is " << *parentFace << "\n";
#endif

            // 2. Ask the opposite face (if it exists) to refine
            if (parentFace) {

                // A vertex can be associated with either of two
                // parent halfedges. If the parent edge that we're
                // interested in doesn't match then we should look at
                // its opposite
                if (parentEdge->GetFace() != parentFace)
                    parentEdge = parentEdge->GetOpposite();
                assert(parentEdge->GetFace() == parentFace);

                // Make sure the parent edge has its neighbor as well
                GuaranteeNeighbor(mesh, parentEdge);

                // Now access that neighbor and refine it
                if (parentEdge->GetRightFace()) {
                    RefineFaceAtVertex(mesh, parentEdge->GetRightFace(), parentVertex);

                    // FIXME: assertion?
                    assert(edge->GetOpposite());
                }
            }
        }
    }
}

template <class T>
void
HbrBilinearSubdivision<T>::GuaranteeNeighbors(HbrMesh<T>* mesh, HbrVertex<T>* vertex) {

#ifdef HBR_DEBUG
    std::cerr << "\n\nneighbor guarantee at " << *vertex << " invoked\n";
#endif

    // If the vertex is a child of a face, guaranteeing the neighbors
    // of the vertex is simply a matter of ensuring the parent face
    // has refined.
    HbrFace<T>* parentFace = vertex->GetParentFace();
    if (parentFace) {

#ifdef HBR_DEBUG
        std::cerr << "  forcing full refine on parent face\n";
#endif
        Refine(mesh, parentFace);
        return;
    }

    // Otherwise if the vertex is a child of an edge, we need to
    // ensure that the parent faces on either side of the parent edge
    // 1) exist, and 2) have refined at both vertices of the parent
    // edge
    HbrHalfedge<T>* parentEdge = vertex->GetParentEdge();
    if (parentEdge) {

#ifdef HBR_DEBUG
        std::cerr << "  forcing full refine on adjacent faces of parent edge\n";
#endif
        HbrVertex<T>* dest = parentEdge->GetDestVertex();
        HbrVertex<T>* org = parentEdge->GetOrgVertex();
        GuaranteeNeighbor(mesh, parentEdge);
        parentFace = parentEdge->GetLeftFace();
        RefineFaceAtVertex(mesh, parentFace, dest);
        RefineFaceAtVertex(mesh, parentFace, org);

#ifdef HBR_DEBUG
        std::cerr << "    on the right face?\n";
#endif
        parentFace = parentEdge->GetRightFace();
        // The right face may not necessarily exist even after
        // GuaranteeNeighbor
        if (parentFace) {
            RefineFaceAtVertex(mesh, parentFace, dest);
            RefineFaceAtVertex(mesh, parentFace, org);
        }
#ifdef HBR_DEBUG
        std::cerr << "  end force\n";
#endif
        return;
    }

    // The last case: the vertex is a child of a vertex. In this case
    // we have to first recursively guarantee that the parent's
    // adjacent faces also exist.
    HbrVertex<T>* parentVertex = vertex->GetParentVertex();
    if (parentVertex) {

#ifdef HBR_DEBUG
        std::cerr << "  recursive parent vertex guarantee call\n";
#endif
        parentVertex->GuaranteeNeighbors();

        // And then we refine all the face neighbors of the
        // parentVertex
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
HbrBilinearSubdivision<T>::HasLimit(HbrMesh<T>* mesh, HbrFace<T>* face) {

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
HbrBilinearSubdivision<T>::HasLimit(HbrMesh<T>* /* mesh */, HbrHalfedge<T>* /* edge */) {
    return true;
}

template <class T>
bool
HbrBilinearSubdivision<T>::HasLimit(HbrMesh<T>* /* mesh */, HbrVertex<T>* vertex) {
    vertex->GuaranteeNeighbors();
    switch (vertex->GetMask(false)) {
        case HbrVertex<T>::k_Smooth:
        case HbrVertex<T>::k_Dart:
            return !vertex->OnBoundary();
            break;
        case HbrVertex<T>::k_Crease:
        case HbrVertex<T>::k_Corner:
        default:
            return true;
    }
}

template <class T>
HbrVertex<T>*
HbrBilinearSubdivision<T>::Subdivide(HbrMesh<T>* mesh, HbrFace<T>* face) {

    // Face rule: simply average all vertices on the face
    HbrVertex<T>* v = mesh->NewVertex();
    T& data = v->GetData();
    int nv = face->GetNumVertices();
    float weight = 1.0f / nv;

    HbrHalfedge<T>* edge = face->GetFirstEdge();
    for (int i = 0; i < face->GetNumVertices(); ++i) {
        HbrVertex<T>* w = edge->GetOrgVertex();
        // If there are vertex edits we have to make sure the edit
        // has been applied
        if (mesh->HasVertexEdits()) {
            w->GuaranteeNeighbors();
        }
        data.AddWithWeight(w->GetData(), weight);
        data.AddVaryingWithWeight(w->GetData(), weight);
        edge = edge->GetNext();
    }
#ifdef HBR_DEBUG
    std::cerr << "Subdividing at " << *face << "\n";
#endif

    // Set the extraordinary flag if the face had anything other than
    // 4 vertices
    if (nv != 4) v->SetExtraordinary();

#ifdef HBR_DEBUG
    std::cerr << "  created " << *v << "\n";
#endif
    return v;
}

template <class T>
HbrVertex<T>*
HbrBilinearSubdivision<T>::Subdivide(HbrMesh<T>* mesh, HbrHalfedge<T>* edge) {

#ifdef HBR_DEBUG
    float esharp = edge->GetSharpness();
    std::cerr << "Subdividing at " << *edge << " (sharpness = " << esharp << ")";
#endif

    HbrVertex<T>* v = mesh->NewVertex();
    T& data = v->GetData();


    // If there's the possibility of a crease edits, make sure the
    // edit has been applied
    if (mesh->HasCreaseEdits()) {
        edge->GuaranteeNeighbor();
    }

    // If there's the possibility of vertex edits on either vertex, we
    // have to make sure the edit has been applied
    if (mesh->HasVertexEdits()) {
        edge->GetOrgVertex()->GuaranteeNeighbors();
        edge->GetDestVertex()->GuaranteeNeighbors();
    }

    // Average the two end points
    data.AddWithWeight(edge->GetOrgVertex()->GetData(), 0.5f);
    data.AddWithWeight(edge->GetDestVertex()->GetData(), 0.5f);

    // Varying data is always the average of two end points
    data.AddVaryingWithWeight(edge->GetOrgVertex()->GetData(), 0.5f);
    data.AddVaryingWithWeight(edge->GetDestVertex()->GetData(), 0.5f);

#ifdef HBR_DEBUG
    std::cerr << "  created " << *v << "\n";
#endif
    return v;
}

template <class T>
HbrVertex<T>*
HbrBilinearSubdivision<T>::Subdivide(HbrMesh<T>* mesh, HbrVertex<T>* vertex) {

    HbrVertex<T>* v;

    // If there are vertex edits we have to make sure the edit has
    // been applied by guaranteeing the neighbors of the
    // vertex. Unfortunately in this case, we can't share the data
    // with the parent
    if (mesh->HasVertexEdits()) {
        vertex->GuaranteeNeighbors();

        v = mesh->NewVertex();
        T& data = v->GetData();

        // Just copy the old value
        data.AddWithWeight(vertex->GetData(), 1.0f);

        // Varying data is always just propagated down
        data.AddVaryingWithWeight(vertex->GetData(), 1.0f);

    } else {
        // Create a new vertex that just shares the same data
        v = mesh->NewVertex(vertex->GetData());
    }

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
        sharp -= 1.0f;
        if (sharp < (float) HbrVertex<T>::k_Smooth) {
            sharp = (float) HbrVertex<T>::k_Smooth;
        }
        v->SetSharpness(sharp);
    } else {
        v->SetSharpness(HbrVertex<T>::k_Smooth);
    }
    return v;
}

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRBILINEAR_H */
