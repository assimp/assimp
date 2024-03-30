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

#ifndef OPENSUBDIV3_HBRSUBDIVISION_H
#define OPENSUBDIV3_HBRSUBDIVISION_H

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrFace;
template <class T> class HbrVertex;
template <class T> class HbrHalfedge;
template <class T> class HbrMesh;
template <class T> class HbrSubdivision {
public:
    HbrSubdivision<T>()
        : creaseSubdivision(k_CreaseNormal) {}

    virtual ~HbrSubdivision<T>() {}

    virtual HbrSubdivision<T>* Clone() const = 0;

    // How to subdivide a face
    virtual void Refine(HbrMesh<T>* mesh, HbrFace<T>* face) = 0;

    // Subdivide a face only at a particular vertex (creating one child)
    virtual HbrFace<T>* RefineFaceAtVertex(HbrMesh<T>* mesh, HbrFace<T>* face, HbrVertex<T>* vertex) = 0;

    // Refine all faces around a particular vertex
    virtual void RefineAtVertex(HbrMesh<T>* mesh, HbrVertex<T>* vertex);

    // Given an edge, try to ensure the edge's opposite exists by
    // forcing refinement up the hierarchy
    virtual void GuaranteeNeighbor(HbrMesh<T>* mesh, HbrHalfedge<T>* edge) = 0;

    // Given an vertex, ensure all faces in the ring around it exist
    // by forcing refinement up the hierarchy
    virtual void GuaranteeNeighbors(HbrMesh<T>* mesh, HbrVertex<T>* vertex) = 0;

    // Returns true if the vertex, edge, or face has a limit point,
    // curve, or surface associated with it
    virtual bool HasLimit(HbrMesh<T>* /* mesh */, HbrFace<T>* /* face */) { return true; }
    virtual bool HasLimit(HbrMesh<T>* /* mesh */, HbrHalfedge<T>* /* edge */) { return true; }
    virtual bool HasLimit(HbrMesh<T>* /* mesh */, HbrVertex<T>* /* vertex */) { return true; }

    // How to turn faces, edges, and vertices into vertices
    virtual HbrVertex<T>* Subdivide(HbrMesh<T>* mesh, HbrFace<T>* face) = 0;
    virtual HbrVertex<T>* Subdivide(HbrMesh<T>* mesh, HbrHalfedge<T>* edge) = 0;
    virtual HbrVertex<T>* Subdivide(HbrMesh<T>* mesh, HbrVertex<T>* vertex) = 0;

    // Returns true if the vertex is extraordinary in the subdivision scheme
    virtual bool VertexIsExtraordinary(HbrMesh<T> const * /* mesh */, HbrVertex<T>* /* vertex */) { return false; }

    // Returns true if the face is extraordinary in the subdivision scheme
    virtual bool FaceIsExtraordinary(HbrMesh<T> const * /* mesh */, HbrFace<T>* /* face */) { return false; }

    // Crease subdivision rules. When subdividing a edge with a crease
    // strength, we get two child subedges, and we need to determine
    // what weights to assign these subedges. The "normal" rule
    // is to simply assign the current edge's crease strength - 1
    // to both of the child subedges. The "Chaikin" rule looks at the
    // current edge and incident edges to the current edge's end
    // vertices, and weighs them; for more information consult
    // the Geri's Game paper.
    enum CreaseSubdivision {
        k_CreaseNormal,
        k_CreaseChaikin
    };
    CreaseSubdivision GetCreaseSubdivisionMethod() const { return creaseSubdivision; }
    void SetCreaseSubdivisionMethod(CreaseSubdivision method) { creaseSubdivision = method; }

    // Figures out how to assign a crease weight on an edge to its
    // subedge. The subedge must be a child of the parent edge
    // (either subedge->GetOrgVertex() or subedge->GetDestVertex()
    // == edge->Subdivide()). The vertex supplied must NOT be
    // a parent of the subedge; it is either the origin or
    // destination vertex of edge.
    void SubdivideCreaseWeight(HbrHalfedge<T>* edge, HbrVertex<T>* vertex, HbrHalfedge<T>* subedge);

    // Returns the expected number of children faces after subdivision
    // for a face with the given number of vertices.
    virtual int GetFaceChildrenCount(int nvertices) const = 0;

protected:
    CreaseSubdivision creaseSubdivision;

    // Helper routine for subclasses: for a given vertex, sums
    // contributions from surrounding vertices
    void AddSurroundingVerticesWithWeight(HbrMesh<T>* mesh, HbrVertex<T>* vertex, float weight, T* data);

    // Helper routine for subclasses: for a given vertex with a crease
    // mask, adds contributions from the two crease edges
    void AddCreaseEdgesWithWeight(HbrMesh<T>* mesh, HbrVertex<T>* vertex, bool next, float weight, T* data);

private:
    // Helper class used by AddSurroundingVerticesWithWeight
    class SmoothSubdivisionVertexOperator : public HbrVertexOperator<T> {
    public:
        SmoothSubdivisionVertexOperator(T* data, bool meshHasEdits, float weight)
            : m_data(data),
              m_meshHasEdits(meshHasEdits),
              m_weight(weight)
        {
        }
        virtual void operator() (HbrVertex<T> &vertex) {
            // Must ensure vertex edits have been applied
            if (m_meshHasEdits) {
                vertex.GuaranteeNeighbors();
            }
            m_data->AddWithWeight(vertex.GetData(), m_weight);
        }
    private:
        T* m_data;
        const bool m_meshHasEdits;
        const float m_weight;
    };

    // Helper class used by AddCreaseEdgesWithWeight
    class CreaseSubdivisionHalfedgeOperator : public HbrHalfedgeOperator<T> {
    public:
        CreaseSubdivisionHalfedgeOperator(HbrVertex<T> *vertex, T* data, bool meshHasEdits, bool next, float weight)
            : m_vertex(vertex),
              m_data(data),
              m_meshHasEdits(meshHasEdits),
              m_next(next),
              m_weight(weight),
              m_count(0)
        {
        }
        virtual void operator() (HbrHalfedge<T> &edge) {
            if (m_count < 2 && edge.IsSharp(m_next)) {
                HbrVertex<T>* a = edge.GetDestVertex();
                if (a == m_vertex) a = edge.GetOrgVertex();
                // Must ensure vertex edits have been applied
                if (m_meshHasEdits) {
                    a->GuaranteeNeighbors();
                }
                m_data->AddWithWeight(a->GetData(), m_weight);
                m_count++;
            }
        }
    private:
        HbrVertex<T>* m_vertex;
        T* m_data;
        const bool m_meshHasEdits;
        const bool m_next;
        const float m_weight;
        int m_count;
    };

private:
    // Helper class used by RefineAtVertex.
    class RefineFaceAtVertexOperator : public HbrFaceOperator<T> {
    public:
        RefineFaceAtVertexOperator(HbrSubdivision<T>* subdivision, HbrMesh<T>* mesh, HbrVertex<T> *vertex)
            : m_subdivision(subdivision),
              m_mesh(mesh),
              m_vertex(vertex)
        {
        }
        virtual void operator() (HbrFace<T> &face) {
            m_subdivision->RefineFaceAtVertex(m_mesh, &face, m_vertex);
        }
    private:
        HbrSubdivision<T>* const m_subdivision;
        HbrMesh<T>* const m_mesh;
        HbrVertex<T>* const m_vertex;
    };

};

template <class T>
void
HbrSubdivision<T>::RefineAtVertex(HbrMesh<T>* mesh, HbrVertex<T>* vertex) {
    GuaranteeNeighbors(mesh, vertex);
    RefineFaceAtVertexOperator op(this, mesh, vertex);
    vertex->ApplyOperatorSurroundingFaces(op);
}

template <class T>
void
HbrSubdivision<T>::SubdivideCreaseWeight(HbrHalfedge<T>* edge, HbrVertex<T>* vertex, HbrHalfedge<T>* subedge) {

    float sharpness = edge->GetSharpness();

    // In all methods, if the parent edge is infinitely sharp, the
    // child edge is also infinitely sharp
    if (sharpness >= HbrHalfedge<T>::k_InfinitelySharp) {
        subedge->SetSharpness(HbrHalfedge<T>::k_InfinitelySharp);
    }

    // Chaikin's curve subdivision: use 3/4 of the parent sharpness,
    // plus 1/4 of crease sharpnesses incident to vertex
    else if (creaseSubdivision == HbrSubdivision<T>::k_CreaseChaikin) {

        float childsharp = 0.0f;
        
        int n = 0;

        // Add 1/4 of the sharpness of all crease edges incident to
        // the vertex (other than this crease edge)
        class ChaikinEdgeCreaseOperator : public HbrHalfedgeOperator<T> {
        public:

            ChaikinEdgeCreaseOperator(
                HbrHalfedge<T> const * edge, float & childsharp, int & count) : 
                    m_edge(edge), m_childsharp(childsharp), m_count(count) { }

            virtual void operator() (HbrHalfedge<T> &edge) {

                // Skip original edge or it's opposite
                if ((&edge==m_edge) || (&edge==m_edge->GetOpposite()))
                    return;
                if (edge.GetSharpness() > HbrHalfedge<T>::k_Smooth) {
                    m_childsharp += edge.GetSharpness();
                    ++m_count;
                }
            }

        private:
            HbrHalfedge<T> const * m_edge;
            float & m_childsharp;
            int & m_count;
        };

        ChaikinEdgeCreaseOperator op(edge, childsharp, n);
        vertex->GuaranteeNeighbors();
        vertex->ApplyOperatorSurroundingEdges(op);

        if (n) {
            childsharp = childsharp * 0.25f / float(n);
        }

        // Add 3/4 of the sharpness of this crease edge
        childsharp += sharpness * 0.75f;
        childsharp -= 1.0f;
        if (childsharp < (float) HbrHalfedge<T>::k_Smooth) {
            childsharp = (float) HbrHalfedge<T>::k_Smooth;
        }
        subedge->SetSharpness(childsharp);

    } else {
        sharpness -= 1.0f;
        if (sharpness < (float) HbrHalfedge<T>::k_Smooth) {
            sharpness = (float) HbrHalfedge<T>::k_Smooth;
        }
        subedge->SetSharpness(sharpness);
    }
}

template <class T>
void
HbrSubdivision<T>::AddSurroundingVerticesWithWeight(HbrMesh<T>* mesh, HbrVertex<T>* vertex, float weight, T* data) {
    SmoothSubdivisionVertexOperator op(data, mesh->HasVertexEdits(), weight);
    vertex->ApplyOperatorSurroundingVertices(op);
}

template <class T>
void
HbrSubdivision<T>::AddCreaseEdgesWithWeight(HbrMesh<T>* mesh, HbrVertex<T>* vertex, bool next, float weight, T* data) {
    CreaseSubdivisionHalfedgeOperator op(vertex, data, mesh->HasVertexEdits(), next, weight);
    vertex->ApplyOperatorSurroundingEdges(op);
}

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRSUBDIVISION_H */
