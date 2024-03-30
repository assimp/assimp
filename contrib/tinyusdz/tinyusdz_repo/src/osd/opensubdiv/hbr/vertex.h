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

#ifndef OPENSUBDIV3_HBRVERTEX_H
#define OPENSUBDIV3_HBRVERTEX_H

#include <assert.h>
#include <iostream>
#include <iterator>
#include <vector>
#include "../hbr/fvarData.h"
#include "../hbr/face.h"

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrHalfedge;
template <class T> class HbrHalfedgeCompare;
template <class T> class HbrVertex;
template <class T> class HbrVertexOperator;
template <class T> class HbrFaceOperator;
template <class T> class HbrHalfedgeOperator;

template <class T> class HbrVertex {

public:
    HbrVertex();
    HbrVertex(int vid, const T &data, int fvarwidth) {
        Initialize(vid, data, fvarwidth);
    }
    void Initialize(int vid, const T &data, int fvarwidth);
    ~HbrVertex();
    void Destroy(HbrMesh<T> *mesh = 0);

    // Registers an incident edge with the vertex
    void AddIncidentEdge(HbrHalfedge<T>* edge);

    // Unregister an incident edge with the vertex
    void RemoveIncidentEdge(HbrHalfedge<T>* edge);

    // Checks if removal of the indicated incident edge will result
    // in a singular vertex
    bool EdgeRemovalWillMakeSingular(HbrHalfedge<T>* edge) const;

    // Sets up vertex flags after the vertex has been bound to a mesh
    void Finish();

    // Compute the valence of this vertex
    int GetValence() const;

    // Compute the valence of this vertex including only edges which
    // are "coarse" (highest level edges)
    int GetCoarseValence() const;

    // Return vertex ID
    int GetID() const { return id; }

    // Return vertex data
    T& GetData() { return data; }

    // Return vertex data
    const T& GetData() const { return data; }

    // Returns the facevarying data which is matched to the face.
    // This may either be the "generic" facevarying item (fvardata, so
    // data.GetFace() == face) or one specifically registered to the
    // face (in the middle of morefvardata, so data.GetFace() ==
    // face). If we require storage for a facevarying data designed to
    // store discontinuous values for this face, we must have called
    // NewFVarData before GetFVarData will give it to us.
    HbrFVarData<T>& GetFVarData(const HbrFace<T>* face);

    // Returns new facevarying data matched to the face
    HbrFVarData<T>& NewFVarData(const HbrFace<T>* face);

    // Return any incident face attached to the vertex
    HbrFace<T>* GetFace() const;

    // Return the mesh to which this vertex belongs
    HbrMesh<T>* GetMesh() const;

    // Return an edge connected to dest
    HbrHalfedge<T>* GetEdge(const HbrVertex<T>* dest) const;

    // Return an edge connected to vertex with id dest
    HbrHalfedge<T>* GetEdge(int dest) const;
    
    // Given an edge, returns the next edge in counterclockwise order
    // around this vertex. Note well: this is only the next halfedge,
    // which means that all edges returned by this function are
    // guaranteed to have the same origin vertex (ie this vertex).  In
    // boundary cases if you are interested in all edges you will not
    // get the last edge with this function. For that reason,
    // GetSurroundingEdges is preferred.
    HbrHalfedge<T>* GetNextEdge(const HbrHalfedge<T>* edge) const;

    // Given an edge, returns the previous edge (ie going clockwise)
    // around this vertex
    HbrHalfedge<T>* GetPreviousEdge(const HbrHalfedge<T>* edge) const;

    // Quadedge-like algebra subset. Since we are dealing with
    // halfedges and not symmetric edges, these functions accept a
    // destination vertex which indicates the (possibly imaginary)
    // halfedge we are considering, and return the destination vertex
    // of the desired (also possibly imaginary) halfedge. Also,
    // currently they are potentially very inefficient and should be
    // avoided.
    HbrVertex<T>* GetQEONext(const HbrVertex<T>* dest) const;
    HbrVertex<T>* GetQEONext(const HbrHalfedge<T>* edge) const;
    HbrVertex<T>* GetQEOPrev(const HbrHalfedge<T>* edge) const;
    HbrVertex<T>* GetQEOPrev(const HbrVertex<T>* dest) const;
    HbrVertex<T>* GetQELNext(const HbrVertex<T>* dest) const;

    // Returns true if the vertex is on a boundary edge
    bool OnBoundary() const;

    // Returns true if the vertex has a facevarying mask which is
    // smooth (0).
    bool IsFVarSmooth(int datum);

    // Returns true if all facevarying data has facevarying mask which
    // is smooth
    bool IsFVarAllSmooth();

    // Returns true if the vertex has a facevarying mask which is dart
    // (1).
    bool IsFVarDart(int datum);

    // Returns true if the vertex is a facevarying corner for any
    // incident face, where "cornerness" is defined as having two
    // incident edges that make up a face both being facevarying
    // edges.
    bool IsFVarCorner(int datum);

    // Returns the sharpness of the vertex
    float GetSharpness() const { return sharpness; }

    // Sets the sharpness of the vertex
    void SetSharpness(float sharp) { sharpness = sharp; ClearMask(); }

    // Returns whether the corner is sharp at the current level of
    // subdivision (next = false) or at the next level of subdivision
    // (next = true).
    bool IsSharp(bool next) const { return (next ? (sharpness > 0.0f) : (sharpness >= 1.0f)); }

    // Sets the vertex mask if the vertex is sharp to reflect that
    // it's a corner
    void ClearMask() {
        mask0 = mask1 = 0; validmask = 0; volatil = 0;
    }

    // Returns the integer mask of the vertex at the current level of
    // subdivision (next = false) or at the next level of subdivision
    // (next = true)
    unsigned char GetMask(bool next);

    // Returns the facevarying integer mask of the vertex
    unsigned char GetFVarMask(int datum);

    // Computes the "fractional mask" of the vertex at the current
    // subdivision level, based on the fractional sharpnesses of any
    // adjacent sharp edges. The fractional mask is a value between 0
    // and 1
    float GetFractionalMask() const;

    // Returns whether the vertex is singular (has two separate
    // incident halfedge cycles)
    bool IsSingular() const { return nIncidentEdges > 1; }

    // Collect the ring of edges around this vertex. Note well: not
    // all edges in this list will have an orientation where the
    // origin of the edge is this vertex!  This function requires an
    // output iterator; to get the edges into a std::vector, use
    // GetSurroundingEdges(std::back_inserter(myvector))
    template <typename OutputIterator>
    void GetSurroundingEdges(OutputIterator edges) const;

    // Apply an edge operator to each edge in the ring of edges
    // around this vertex
    void ApplyOperatorSurroundingEdges(HbrHalfedgeOperator<T> &op) const;

    // Collect the ring of vertices around this vertex (the ones that
    // share an edge with this vertex). This function requires an
    // output iterator; to get the vertices into a std::vector, use
    // GetSurroundingVertices(std::back_inserter(myvector))
    template <typename OutputIterator>
    void GetSurroundingVertices(OutputIterator vertices) const;

    // Apply a vertex operator to each vertex in the ring of vertices
    // around this vertex
    void ApplyOperatorSurroundingVertices(HbrVertexOperator<T> &op) const;

    // Applys an operator to the ring of faces around this vertex
    void ApplyOperatorSurroundingFaces(HbrFaceOperator<T> &op) const;

    // Returns the parent, which can be a edge, face, or vertex
    HbrHalfedge<T>* GetParentEdge() const { return (parentType == k_ParentEdge ? parent.edge : 0); }
    HbrFace<T>* GetParentFace() const { return (parentType == k_ParentFace ? parent.face : 0); }
    HbrVertex<T>* GetParentVertex() const { return (parentType == k_ParentVertex ? parent.vertex : 0); }

    // Set the parent pointer
    void SetParent(HbrHalfedge<T>* edge) { assert(!edge || !parent.vertex); parentType = k_ParentEdge; parent.edge = edge; }
    void SetParent(HbrFace<T>* face) { assert(!face || !parent.vertex); parentType = k_ParentFace; parent.face = face; }
    void SetParent(HbrVertex<T>* vertex) { assert(!vertex || !parent.vertex); parentType = k_ParentVertex; parent.vertex = vertex; }

    // Subdivides the vertex and returns the child vertex
    HbrVertex<T>* Subdivide();

    // Refines the ring of faces around this vertex
    void Refine();

    // Make sure the vertex has all faces in the ring around it
    void GuaranteeNeighbors();

    // Indicates that the vertex may have a missing face neighbor and
    // may need to guarantee its neighbors in the future
    void UnGuaranteeNeighbors() {
        neighborsguaranteed = 0;
        // Its mask is also invalidated
        validmask = 0;
    }

    // True if the edge has a subdivided child vertex
    bool HasChild() const { return vchild!=-1; }

    // Remove the reference to subdivided vertex
    void RemoveChild() { vchild = -1; }

    // Returns true if the vertex still has an incident edge (in other
    // words, it belongs to a face)
    bool IsReferenced() const { return references != 0; }

    // Returns true if the vertex is extraordinary
    bool IsExtraordinary() const { return extraordinary; }

    // Tag the vertex as being extraordinary
    void SetExtraordinary() { extraordinary = 1; }

    // Returns whether the vertex is volatile (incident to a semisharp
    // edge or semisharp corner)
    bool IsVolatile() { if (!validmask) GetMask(false); return volatil; }

    // Simple bookkeeping needed for garbage collection by HbrMesh
    bool IsCollected() const { return collected; }
    void SetCollected() { collected = 1; }
    void ClearCollected() { collected = 0; }

    // Bookkeeping to see if a vertex edit exists for this vertex
    bool HasVertexEdit() const { return hasvertexedit; }
    void SetVertexEdit() { hasvertexedit = 1; }
    void ClearVertexEdit() { hasvertexedit = 0; }

    // Returns memory statistics
    unsigned long GetMemStats() const;

    // Returns true if the vertex is connected. This means that it has
    // an incident edge
    bool IsConnected() const { return nIncidentEdges > 0; }

    // Return an incident edge to this vertex, which happens to be the
    // first halfedge of the cycles.
    HbrHalfedge<T>* GetIncidentEdge() const {
        if (nIncidentEdges > 1) {
            return incident.edges[0];
        } else if (nIncidentEdges == 1) {
            return incident.edge;
        } else {
            return 0;
        }
    }

    // Sharpness and mask constants
    enum Mask {
        k_Smooth = 0,
        k_Dart = 1,
        k_Crease = 2,
        k_Corner = 3,
        k_InfinitelySharp = 10
    };

    // Increment the usage counter on the vertex
    void IncrementUsage() { used++; }

    // Decrement the usage counter on the vertex
    void DecrementUsage() { used--; }

    // Check the usage counter on the vertex
    bool IsUsed() const { return used || (vchild != -1); }

    // Used by block allocator
    HbrVertex<T>*& GetNext() { return parent.vertex; }

    // Returns the blind pointer to client data
    void *GetClientData(HbrMesh<T>* mesh) const {
        return mesh->GetVertexClientData(id);
    }

    // Sets the blind pointer to client data
    void SetClientData(HbrMesh<T> *mesh, void *data) {
        mesh->SetVertexClientData(id, data);
    }

    enum ParentType {
        k_ParentNone, k_ParentFace, k_ParentEdge, k_ParentVertex
    };

private:
    // Splits a singular vertex into multiple nonsingular vertices
    void splitSingular();

    // Data
    T data;

    // Pointer to extra facevarying data. Space for this is allocated
    // by NewFVarData. This struct is actually overpadded.
    struct morefvardata {
        int count;
    } *morefvar;

    // Unique ID of this vertex
    int id;

    // The number of halfedges which have this vertex as the incident
    // edge. When references == 0, the vertex is safe to delete
    int references;

    // The number of faces marked "used" which share this vertex.  May
    // not be the same as references!
    int used;

    // Sharpness
    float sharpness;

    // Index of child vertex
    int vchild;

    // Size of incident array
    unsigned short nIncidentEdges;
    
    // Vertex masks, at this level of subdivision and at the next
    // level of subdivision. Valid only when validmask = 1.
    unsigned short mask0:3;
    unsigned short mask1:3;

    // Extraordinary bit
    unsigned short extraordinary:1;
    // Whether the current mask value is correct or should be recalculated
    unsigned short validmask:1;
    // Whether the vertex is "volatile" (is incident to a semisharp edge,
    // or is a semisharp corner)
    unsigned short volatil:1;
    // Whether we can guarantee the existence of neighboring faces on
    // this vertex
    unsigned short neighborsguaranteed:1;
    // Bookkeeping for HbrMesh
    unsigned short collected:1;

    // Whether the vertex has an edit. The edit is owned by a face
    // so this is just a tag that indicates we need to search the
    // vertex's neighboring faces for an edit
    unsigned short hasvertexedit:1;
    // Whether the vertex edit (if any) has been applied
    unsigned short editsapplied:1;
    // Whether Destroy() has been called
    unsigned short destroyed:1;

    // Parent type - can be face, edge, or vertex
    unsigned short parentType:2;

    // List of edge cycles. For "singular" vertices, the corresponding
    // set of adjacent halfedges may consist of several cycles, and we
    // need to account for all of them here. In cases where
    // nIncidentEdges is 1, the edge field of the union points
    // directly at the edge which starts the only incident cycle. If
    // nIncidnetEdges is 2 or more, the edges field of the union is a
    // separate allocated array and edge member of the array points at
    // separate cycles.
    union {
        HbrHalfedge<T>* edge;
        HbrHalfedge<T>** edges;
    } incident;

    union {
        HbrFace<T>* face;
        HbrHalfedge<T>* edge;
        HbrVertex<T>* vertex;
    } parent;

#ifdef HBR_ADAPTIVE
public:
    struct adaptiveFlags {
        unsigned isTagged:1;
        unsigned wasTagged:1;
        
        adaptiveFlags() : isTagged(0), wasTagged(0) { }
    };
    
    adaptiveFlags _adaptiveFlags;
#endif
};

template <class T>
HbrVertex<T>::HbrVertex() :
    morefvar(0), id(-1), references(0), used(0),
    sharpness(0.0f), vchild(-1), nIncidentEdges(0), extraordinary(0), validmask(0),
    volatil(0), neighborsguaranteed(0), collected(0), hasvertexedit(0),
    editsapplied(0), destroyed(0), parentType(k_ParentNone) {
    ClearMask();
    parent.vertex = 0;
    incident.edge = 0;
}

template <class T>
void
HbrVertex<T>::Initialize(int vid, const T &vdata, int fvarwidth) {
    data = vdata;
    morefvar = 0 ;
    id = vid;
    references = 0;
    used = 0;
    extraordinary = 0;
    ClearMask();
    neighborsguaranteed = 0;
    collected = 0;
    hasvertexedit = 0;
    editsapplied = 0;
    destroyed = 0;
    sharpness = 0.0f;
    nIncidentEdges = 0;
    vchild = -1;
    assert(!parent.vertex);
    parentType = k_ParentVertex;
    parent.vertex = 0;

    if (fvarwidth) {
        // Upstream allocator ensured the class was padded by the
        // appropriate size. GetFVarData will return a pointer to this
        // memory, but it needs to be properly initialized.
        // Run placement new to initialize datum
        char *buffer = ((char*) this + sizeof(*this));
        new (buffer) HbrFVarData<T>();
    }
}

template <class T>
HbrVertex<T>::~HbrVertex() {
    Destroy();
}

template <class T>
void
HbrVertex<T>::Destroy(HbrMesh<T> *mesh) {
    if (!destroyed) {
        // Vertices are only safe for deletion if the number of incident
        // edges is exactly zero.
        assert(references == 0);

        // Delete parent reference to self
        if (parentType == k_ParentEdge && parent.edge) {
            parent.edge->RemoveChild();
            parent.edge = 0;
        } else if (parentType == k_ParentFace && parent.face) {
            parent.face->RemoveChild();
            parent.face = 0;
        } else if (parentType == k_ParentVertex && parent.vertex) {
            parent.vertex->RemoveChild();
            parent.vertex = 0;
        }

        // Orphan the child vertex
        if (vchild != -1) {
            if (mesh) {
                HbrVertex<T> *vchildVert = mesh->GetVertex(vchild);
                vchildVert->SetParent(static_cast<HbrVertex*>(0));
            }
            vchild = -1;
        }
        // We're skipping the placement destructors here, in the
        // assumption that HbrFVarData's destructor doesn't actually do
        // anything much
        if (morefvar) {
            free(morefvar);
        }
        destroyed = 1;
    }
}

template <class T>
void
HbrVertex<T>::AddIncidentEdge(HbrHalfedge<T>* edge) {
    assert(edge->GetOrgVertex() == this);

    // First, maintain the property that all of the incident edges
    // will always be a boundary edge if possible. If any of the
    // incident edges are no longer boundaries at this point then they
    // can be immediately removed.
    int i;
    unsigned short newEdgeCount = 0;
    bool edgeFound = false;
    HbrHalfedge<T>** incidentEdges =
        (nIncidentEdges > 1) ? incident.edges : &incident.edge;
    
    for (i = 0; i < nIncidentEdges; ++i) {
        if (incidentEdges[i] == edge) {
            edgeFound = true;
        }
        if (incidentEdges[i]->IsBoundary()) {
            incidentEdges[newEdgeCount++] = incidentEdges[i];
        } else {
            // Did this edge suddenly stop being a boundary because
            // the newly introduced edge (or something close to it)
            // closed a cycle? If so, we don't want to lose a pointer
            // to this edge cycle!  So check to see if this cycle is
            // complete, and if so, keep it.
            HbrHalfedge<T>* start = incidentEdges[i];
            HbrHalfedge<T>* edge = start;
            bool prevmatch = false;
            do {
                edge = GetNextEdge(edge);
                // Check all previous incident edges, if already
                // encountered then we have an edge to this cycle and
                // don't need to proceed further with this check
                for (int j = 0; j < i; ++j) {
                    if (incidentEdges[j] == edge) {
                        prevmatch = true;
                        break;
                    }
                }
            } while (!prevmatch && edge && edge != start);
            if (!prevmatch && edge && edge == start) {

                incidentEdges[newEdgeCount++] = incidentEdges[i];
            }
        }
    }

    // If we are now left with no incident edges, then this edge
    // becomes the sole incident edge (since we always need somewhere
    // to start, even if it's a uninterrupted cycle [ie it doesn't
    // matter whether the edge is a boundary]). Restore incidentEdges
    // array to point to the end of the object.
    if (newEdgeCount == 0) {
        if (!(edgeFound && nIncidentEdges == 1)) {
            if (nIncidentEdges > 1) {
                delete [] incidentEdges;
            }
            incidentEdges = &incident.edge;
            incidentEdges[0] = edge;
            nIncidentEdges = 1;
        }
    }

    // Otherwise, we already have a set of incident edges - we only
    // add this edge if it's a boundary edge, which would begin a new
    // cycle.
    else if (edge->IsBoundary()) {
        if (!edgeFound) {
            // Must add the new edge. May need to reallocate here.
            if (newEdgeCount + 1 != nIncidentEdges) {
                HbrHalfedge<T>** newIncidentEdges = 0;
                if (newEdgeCount + 1 > 1) {
                    newIncidentEdges = new HbrHalfedge<T>*[newEdgeCount + 1];
                } else {
                    newIncidentEdges = &incident.edge;
                }
                for (i = 0; i < newEdgeCount; ++i) {
                    newIncidentEdges[i] = incidentEdges[i];
                }
                if (nIncidentEdges > 1) {
                    delete[] incidentEdges;
                }
                nIncidentEdges = newEdgeCount + 1;
                incidentEdges = newIncidentEdges;
                if (nIncidentEdges > 1) {
                    incident.edges = newIncidentEdges;
                }
            }
            incidentEdges[newEdgeCount] = edge;
        } else {
            // Edge is already in our list, so we don't need to add it
            // again. However, we may need to reallocate due to above
            // cleaning of nonboundary edges
            if (newEdgeCount != nIncidentEdges) {
                HbrHalfedge<T>** newIncidentEdges = 0;
                if (newEdgeCount > 1) {
                    newIncidentEdges = new HbrHalfedge<T>*[newEdgeCount];
                } else {
                    newIncidentEdges = &incident.edge;
                }
                for (i = 0; i < newEdgeCount; ++i) {
                    newIncidentEdges[i] = incidentEdges[i];
                }
                if (nIncidentEdges > 1) {
                    delete[] incidentEdges;
                }
                nIncidentEdges = newEdgeCount;
                incidentEdges = newIncidentEdges;
                if (nIncidentEdges > 1) {
                    incident.edges = newIncidentEdges;
                }
            }
        }
    }
    else {
        // Again, we may need to reallocate due to above cleaning of
        // nonboundary edges
        if (newEdgeCount != nIncidentEdges) {
            HbrHalfedge<T>** newIncidentEdges = 0;
            if (newEdgeCount > 1) {
                newIncidentEdges = new HbrHalfedge<T>*[newEdgeCount];
            } else {
                newIncidentEdges = &incident.edge;
            }
            for (i = 0; i < newEdgeCount; ++i) {
                newIncidentEdges[i] = incidentEdges[i];
            }
            if (nIncidentEdges > 1) {
                delete[] incidentEdges;
            }
            nIncidentEdges = newEdgeCount;
            incidentEdges = newIncidentEdges;
            if (nIncidentEdges > 1) {
                incident.edges = newIncidentEdges;
            }
        }
    }

    // For non-boundary edges, ensure that the incident edge starting
    // the cycle is the lowest possible edge. By doing this,
    // operations like GetSurroundingEdges will be guaranteed to
    // return the same order of edges/faces through multi-threading.
    if (!incidentEdges[0]->IsBoundary()) {
        HbrHalfedge<T>* start = GetIncidentEdge();
        incidentEdges[0] = start;
        HbrFacePath incidentEdgePath = incidentEdges[0]->GetFace()->GetPath();
        HbrHalfedge<T>* e = GetNextEdge(start);
        while (e) {
            if (e == start) break;
            HbrFacePath ePath = e->GetFace()->GetPath();
            if (ePath < incidentEdgePath) {
                incidentEdges[0] = e;
                incidentEdgePath = ePath;
            }
            HbrHalfedge<T>* next = GetNextEdge(e);
            if (!next) {
                e = e->GetPrev();
                if (e->GetFace()->GetPath() < incidentEdges[0]->GetFace()->GetPath()) {
                    incidentEdges[0] = e;
                }
                break;
            } else {
                e = next;
            }
        }
    }

    references++;
}

template <class T>
void
HbrVertex<T>::RemoveIncidentEdge(HbrHalfedge<T>* edge) {

    int i, j;
    HbrHalfedge<T>** incidentEdges =
        (nIncidentEdges > 1) ? incident.edges : &incident.edge;

    references--;
    if (references) {

        HbrHalfedge<T>* next;

        // We may need to shuffle our halfedge cycles. First we check
        // whether the edge being erased begins any edge cycles
        bool edgeFound = false;
        next = GetNextEdge(edge);

        for (i = 0; i < nIncidentEdges; ++i) {
            if (incidentEdges[i] == edge) {

                // Edge cycle found. Replace the edge with the next edge
                // in the cycle if possible.
                if (next) {
                    incidentEdges[i] = next;
                    // We are done.
                    return;
                }

                // If no next edge is found it means the entire cycle
                // has gone away.
                edgeFound = true;
                break;
            }
        }

        // The edge cycle needs to disappear
        if (edgeFound) {
            assert(nIncidentEdges > 1);

            HbrHalfedge<T>** newIncidentEdges = 0;
            if (nIncidentEdges - 1 > 1) {
                newIncidentEdges = new HbrHalfedge<T>*[nIncidentEdges - 1];
            } else {
                newIncidentEdges = &incident.edge;
            }
            j = 0;
            for (i = 0; i < nIncidentEdges; ++i) {
                if (incidentEdges[i] != edge) {
                    newIncidentEdges[j++] = incidentEdges[i];
                }
            }
            assert(j == nIncidentEdges - 1);
            if (nIncidentEdges > 1) {
                delete[] incidentEdges;
            }
            nIncidentEdges--;
            if (nIncidentEdges > 1) {
                incident.edges = newIncidentEdges;
            }
            return;
        }
        // Now deal with the case where we remove an edge
        // which did not begin a boundary edge cycle. If this
        // happens then the resulting unbroken cycle does
        // get broken; in that case we replace the incident
        // edge with the next one after this.
        else if (nIncidentEdges == 1 && !incidentEdges[0]->IsBoundary()) {
            if (next) {
                incidentEdges[0] = next;
            } else {
                // hm, what does this mean for us? Not sure at the
                // moment.
                std::cout << "Could not split cycle!\n";
                assert(0);
            }
        }

        // (Is this another case or a specialization of the above?)
        // When an edge in the middle of a boundary cycle goes away we
        // need to mark a new cycle.
        //
        // If there is no next edge, it means that we didn't
        // actually split the cycle, we just deleted the last edge
        // in the cycle. As such nothing needs to occur because
        // the "split" is already present.

        else if (!edge->IsBoundary() && next) {
            HbrHalfedge<T>** newIncidentEdges = 0;
            if (nIncidentEdges + 1 > 1) {
                newIncidentEdges = new HbrHalfedge<T>*[nIncidentEdges + 1];
            } else {
                newIncidentEdges = &incident.edge;
            }
            for (i = 0; i < nIncidentEdges; ++i) {
                newIncidentEdges[i] = incidentEdges[i];
            }
            newIncidentEdges[nIncidentEdges] = next;
            if (nIncidentEdges > 1) {
                delete[] incidentEdges;
            }
            nIncidentEdges++;
            if (nIncidentEdges > 1) {
                incident.edges = newIncidentEdges;
            }
        }
    } else {
        // No references left, we can just clear all the cycles
        if (nIncidentEdges > 1) {
            delete[] incidentEdges;
        }
        nIncidentEdges = 0;
    }
}

template <class T>
bool
HbrVertex<T>::EdgeRemovalWillMakeSingular(HbrHalfedge<T>* edge) const {
    // Only edge left, or no incident edges at all (how?)
    if (references <= 1 || nIncidentEdges <= 0) {
        return false;
    }
    // There are at least two existing cycles. We could maybe consider
    // the case where removal of this edge will actually make one of
    // the edge cycles go away, possibly leaving behind just one, but
    // we'll ignore that possibility for now
    else if (nIncidentEdges > 1) {
        return true;
    }
     // This is the incident edge starting a single cycle. Removal of
     // the edge will replace the start of the cycle with the next
     // edge, and we keep a single cycle.
    else if (nIncidentEdges == 1 && incident.edge == edge) {
        return false;
    }
    // Check the single cycle: was it interrupted? (i.e. a
    // boundary). If not interrupted, then deletion of any edge still
    // leaves a single cycle. Otherwise: if the edge is the *last*
    // edge in the cycle, we still don't need to split the any further
    // cycle. Otherwise we must split the cycle, which would result in
    // a singular vertex
    else if (!GetIncidentEdge()->IsBoundary()) {
        return false;
    } else if (GetNextEdge(edge)) {
        return true;
    } else {
        return false;
    }
}

template <class T>
void
HbrVertex<T>::Finish() {
    extraordinary = false;
    if (HbrMesh<T>* mesh = GetMesh()) {
        if (IsSingular()) splitSingular();
        assert(!IsSingular());
        if (mesh->GetSubdivision()) {
            extraordinary = mesh->GetSubdivision()->VertexIsExtraordinary(mesh, this);
        }
    }
}

template <class T>
int
HbrVertex<T>::GetValence() const {
    int valence = 0;
    assert(!IsSingular());
    HbrHalfedge<T>* start =
        (nIncidentEdges > 1) ? incident.edges[0] : incident.edge;
    HbrHalfedge<T>* edge = start;
    if (edge) do {
        valence++;
        edge = GetNextEdge(edge);
    } while (edge && edge != start);
    // In boundary cases, we increment the valence count by
    // one more
    if (!edge) valence++;
    return valence;
}

template <class T>
int
HbrVertex<T>::GetCoarseValence() const {
    int valence = 0;
    assert(!IsSingular());
    HbrHalfedge<T>* start =
        (nIncidentEdges > 1) ? incident.edges[0] : incident.edge;
    HbrHalfedge<T>* edge = start;
    if (edge) do {
        if (edge->IsCoarse()) {
            valence++;
        }
        edge = GetNextEdge(edge);
    } while (edge && edge != start);
    // In boundary cases, we increment the valence count by one more
    // (this assumes the last edge is coarse, which it had better be
    // in the boundary case!)
    if (!edge) valence++;
    return valence;
}

template <class T>
HbrFVarData<T>&
HbrVertex<T>::GetFVarData(const HbrFace<T>* face) {
    // See if there are any extra facevarying datum associated with
    // this vertex, and whether any of them match the face.
    if (morefvar) {
        size_t fvtsize = sizeof(HbrFVarData<T>) + sizeof(float) * (GetMesh()->GetTotalFVarWidth() - 1);
        HbrFVarData<T> *fvt = (HbrFVarData<T> *)((char *) morefvar + sizeof(int));
        for (int i = 0; i < morefvar->count; ++i) {
            if (fvt->GetFaceID() == face->GetID()) {
                return *fvt;
            }
            fvt = (HbrFVarData<T>*)((char*) fvt + fvtsize);
        }
    }
    // Otherwise, return the default facevarying datum, which lives
    // in the overallocated space after the end of this object
    return *((HbrFVarData<T>*) ((char*) this + sizeof(*this)));
}

template <class T>
HbrFVarData<T>&
HbrVertex<T>::NewFVarData(const HbrFace<T>* face) {
    const int fvarwidth = GetMesh()->GetTotalFVarWidth();
    size_t fvtsize = sizeof(HbrFVarData<T>) + (fvarwidth - 1) * sizeof(float);
    if (morefvar) {
        struct morefvardata *newmorefvar =
            (struct morefvardata *) malloc(sizeof(int) + (morefvar->count + 1) * fvtsize);
        HbrFVarData<T> *newfvt = (HbrFVarData<T> *)((char *) newmorefvar + sizeof(int));
        HbrFVarData<T> *oldfvt = (HbrFVarData<T> *)((char *) morefvar + sizeof(int));
        for (int i = 0; i < morefvar->count; ++i) {
            new (newfvt) HbrFVarData<T>();
            newfvt->SetAllData(fvarwidth, oldfvt->GetData(0));
            newfvt->SetFaceID(oldfvt->GetFaceID());
            oldfvt = (HbrFVarData<T>*)((char*) oldfvt + fvtsize);
            newfvt = (HbrFVarData<T>*)((char*) newfvt + fvtsize);
        }
        new (newfvt) HbrFVarData<T>();
        newfvt->SetFaceID(face->GetID());
        newmorefvar->count = morefvar->count + 1;
        free(morefvar);
        morefvar = newmorefvar;
        return *newfvt;
    } else {
        morefvar = (struct morefvardata *) malloc(sizeof(int) + fvtsize);
        HbrFVarData<T> *newfvt = (HbrFVarData<T> *)((char *) morefvar + sizeof(int));
        new (newfvt) HbrFVarData<T>();
        newfvt->SetFaceID(face->GetID());
        morefvar->count = 1;
        return *newfvt;
    }
}



template <class T>
HbrFace<T>*
HbrVertex<T>::GetFace() const {
    return GetIncidentEdge()->GetFace();
}

template <class T>
HbrMesh<T>*
HbrVertex<T>::GetMesh() const {
    return GetFace()->GetMesh();
}

template <class T>
HbrHalfedge<T>*
HbrVertex<T>::GetEdge(const HbrVertex<T>* dest) const {
    // Here, we generally want to go through all halfedge cycles
    for (int i = 0; i < nIncidentEdges; ++i) {
        HbrHalfedge<T>* cycle =
            (nIncidentEdges > 1) ? incident.edges[i] : incident.edge;
        HbrHalfedge<T>* edge = cycle;
        if (edge) do {
            if (edge->GetDestVertex() == dest) {
                return edge;
            }
            edge = GetNextEdge(edge);
        } while (edge && edge != cycle);
    }
    return 0;
}

template <class T>
HbrHalfedge<T>*
HbrVertex<T>::GetEdge(int dest) const {
    // Here, we generally want to go through all halfedge cycles
    for (int i = 0; i < nIncidentEdges; ++i) {
        HbrHalfedge<T>* cycle =
            (nIncidentEdges > 1) ? incident.edges[i] : incident.edge;
        HbrHalfedge<T>* edge = cycle;
        if (edge) do {
            if (edge->GetDestVertexID() == dest) {
                return edge;
            }
            edge = GetNextEdge(edge);
        } while (edge && edge != cycle);
    }
    return 0;
}

template <class T>
HbrHalfedge<T>*
HbrVertex<T>::GetNextEdge(const HbrHalfedge<T>* edge) const {
    // Paranoia:
    //    if (edge->GetOrgVertex() != this) return 0;
    return edge->GetPrev()->GetOpposite();
}

template <class T>
HbrHalfedge<T>*
HbrVertex<T>::GetPreviousEdge(const HbrHalfedge<T>* edge) const {
    // Paranoia:
    //    if (edge->GetOrgVertex() != this) return 0;
    return edge->GetOpposite()->GetNext();
}

template <class T>
HbrVertex<T>*
HbrVertex<T>::GetQEONext(const HbrVertex<T>* dest) const {
    HbrHalfedge<T>* edge = GetEdge(dest);
    if (edge) {
        return edge->GetPrev()->GetOrgVertex();
    }
    HbrHalfedge<T>* start = GetIncidentEdge(), *next;
    edge = start;
    while (edge) {
        next = GetNextEdge(edge);
        if (edge->GetDestVertex() == dest) {
            if (!next) {
                return edge->GetPrev()->GetOrgVertex();
            } else {
                return next->GetDestVertex();
            }
        }
        if (next == start) {
            return 0;
        } else if (!next) {
            if (edge->GetPrev()->GetOrgVertex() == dest) {
                return start->GetDestVertex();
            } else {
                return 0;
            }
        } else {
            edge = next;
        }
    }
    // Shouldn't get here
    return 0;
}

template <class T>
HbrVertex<T>*
HbrVertex<T>::GetQEONext(const HbrHalfedge<T>* edge) const {
    assert(edge->GetOrgVertex() == this);
    return edge->GetPrev()->GetOrgVertex();
}

template <class T>
HbrVertex<T>*
HbrVertex<T>::GetQEOPrev(const HbrVertex<T>* dest) const {
    HbrHalfedge<T>* edge = GetEdge(dest);
    if (edge) {
        if (edge->GetOpposite()) {
            return edge->GetOpposite()->GetNext()->GetDestVertex();
        } else {
            HbrHalfedge<T>* start = GetIncidentEdge(), *next;
            edge = start;
            while (edge) {
                next = GetNextEdge(edge);
                if (next == start) {
                    if (next->GetDestVertex() == dest) {
                        return edge->GetDestVertex();
                    } else {
                        return 0;
                    }
                } else if (!next) {
                    if (edge->GetPrev()->GetOrgVertex() == dest) {
                        return edge->GetDestVertex();
                    } else if (start->GetDestVertex() == dest) {
                        return edge->GetPrev()->GetOrgVertex();
                    } else {
                        return 0;
                    }
                } else if (next->GetDestVertex() == dest) {
                    return edge->GetDestVertex();
                } else {
                    edge = next;
                }
            }
            return 0;
        }
    }
    edge = dest->GetEdge(this);
    if (edge) {
        return edge->GetNext()->GetDestVertex();
    }
    return 0;
}

template <class T>
HbrVertex<T>*
HbrVertex<T>::GetQEOPrev(const HbrHalfedge<T>* edge) const {
    assert(edge->GetOrgVertex() == this);
    if (edge->GetOpposite()) {
        return edge->GetOpposite()->GetNext()->GetDestVertex();
    } else {
        return GetQEOPrev(edge->GetDestVertex());
    }
}

template <class T>
HbrVertex<T>*
HbrVertex<T>::GetQELNext(const HbrVertex<T>* dest) const {
    HbrHalfedge<T>* edge = GetEdge(dest);
    if (edge) {
        return edge->GetNext()->GetDestVertex();
    }
    edge = dest->GetEdge(this);
    if (edge) {
        return edge->GetPrev()->GetOrgVertex();
    }
    return 0;
}

template <class T>
bool
HbrVertex<T>::OnBoundary() const {
    // We really only need to check the first incident edge, since
    // singular vertices by definition are on the boundary
    return GetIncidentEdge()->IsBoundary();
}

template <class T>
bool
HbrVertex<T>::IsFVarSmooth(int datum) {
    return (GetFVarMask(datum) == k_Smooth);
}

template <class T>
bool
HbrVertex<T>::IsFVarAllSmooth() {
    for (int i = 0; i < GetMesh()->GetFVarCount(); ++i) {
        if (!IsFVarSmooth(i)) return false;
    }
    return true;
}

template <class T>
bool
HbrVertex<T>::IsFVarDart(int datum) {
    return (GetFVarMask(datum) == k_Dart);
}

template <class T>
bool
HbrVertex<T>::IsFVarCorner(int datum) {

    // If it's a dart, it's a corner
    if (IsFVarDart(datum)) return true;

    // Run through surrounding edges, looking for two adjacent
    // facevarying boundary edges
    HbrHalfedge<T>* start = GetIncidentEdge(), *edge, *nextedge;
    edge = start;
    bool lastedgewassharp = false;
    while (edge) {
        if (edge->GetFVarSharpness(datum)) {
            if (lastedgewassharp) {
                return true;
            } else {
                lastedgewassharp = true;
            }
        } else {
            lastedgewassharp = false;
        }
        nextedge = GetNextEdge(edge);
        if (nextedge == start) {
            return start->GetFVarSharpness(datum) && lastedgewassharp;
        } else if (!nextedge) {
            // Special case for the last edge in a cycle.
            edge = edge->GetPrev();
            return edge->GetFVarSharpness(datum) && lastedgewassharp;
        } else {
            edge = nextedge;
        }
    }
    return false;
}

template <class T>
unsigned char
HbrVertex<T>::GetMask(bool next) {

    if (validmask) {
        return (unsigned char)(next ? mask1 : mask0);
    }

    mask0 = mask1 = 0;

    // Mark volatility
    if (sharpness > k_Smooth && sharpness < k_InfinitelySharp)
        volatil = 1;

    // If the vertex is tagged as sharp immediately promote its mask
    // to corner
    if (IsSharp(false)) {
        mask0 += k_Corner;
    }
    if (IsSharp(true)) {
        mask1 += k_Corner;
    }

    // Count the number of surrounding sharp edges
    HbrHalfedge<T>* start = GetIncidentEdge(), *edge, *nextedge;
    edge = start;
    while (edge) {
        float esharp = edge->GetSharpness();

        if (edge->IsSharp(false)) {
            if (mask0 < k_Corner) {
                mask0++;
            }
        }
        if (edge->IsSharp(true)) {
            if (mask1 < k_Corner) {
                mask1++;
            }
        }
        // If any incident edge is semisharp, mark the vertex as volatile
        if (esharp > HbrHalfedge<T>::k_Smooth && esharp < HbrHalfedge<T>::k_InfinitelySharp) {
            volatil = 1;
        }
        nextedge = GetNextEdge(edge);
        if (nextedge == start) {
            break;
        } else if (!nextedge) {
            // Special case for the last edge in a cycle.
            edge = edge->GetPrev();
            esharp = edge->GetSharpness();
            if (edge->IsSharp(false)) {
                if (mask0 < k_Corner) {
                    mask0++;
                }
            }
            if (edge->IsSharp(true)) {
                if (mask1 < k_Corner) {
                    mask1++;
                }
            }
            if (esharp > HbrHalfedge<T>::k_Smooth && esharp < HbrHalfedge<T>::k_InfinitelySharp) {
                volatil = 1;
            }
            break;
        } else {
            edge = nextedge;
        }
    }
    validmask = 1;
    return (unsigned char)(next ? mask1 : mask0);
}

template <class T>
unsigned char
HbrVertex<T>::GetFVarMask(int datum) {

    unsigned char mask = 0;

    // If the vertex is tagged as sharp immediately promote its mask
    // to corner
    if (IsSharp(false)) {
        mask += k_Corner;
    }

    // Count the number of surrounding facevarying boundary edges
    HbrHalfedge<T>* start = GetIncidentEdge(), *edge, *nextedge;
    edge = start;
    while (edge) {
        if (edge->GetFVarSharpness(datum)) {
            if (mask < k_Corner) {
                mask++;
            } else {
                // Can't get any sharper, so give up early
                break;
            }
        }
        nextedge = GetNextEdge(edge);
        if (nextedge == start) {
            break;
        } else if (!nextedge) {
            // Special case for the last edge in a cycle.
            edge = edge->GetPrev();
            if (edge->GetFVarSharpness(datum)) {
                if (mask < k_Corner) {
                    mask++;
                }
            }
            break;
        } else {
            edge = nextedge;
        }
    }
    return mask;
}

template <class T>
float
HbrVertex<T>::GetFractionalMask() const {
    float mask = 0;
    float n = 0;

    if (sharpness > k_Smooth && sharpness < k_Dart) {
        mask += sharpness; ++n;
    }

    // Add up the strengths of surrounding fractional sharp edges
    HbrHalfedge<T>* start = GetIncidentEdge(), *edge, *next;
    edge = start;
    while (edge) {
        float esharp = edge->GetSharpness();
        if (esharp > HbrHalfedge<T>::k_Smooth && esharp < HbrHalfedge<T>::k_Sharp) {
            mask += esharp; ++n;
        }
        next = GetNextEdge(edge);
        if (next == start) {
            break;
        } else if (!next) {
            // Special case for the last edge in a cycle.
            esharp = edge->GetPrev()->GetSharpness();
            if (esharp > HbrHalfedge<T>::k_Smooth && esharp < HbrHalfedge<T>::k_Sharp) {
                mask += esharp; ++n;
            }
            break;
        } else {
            edge = next;
        }
    }
    assert (n > 0.0f && mask < n);
    return (mask / n);
}

template <class T>
template <typename OutputIterator>
void
HbrVertex<T>::GetSurroundingEdges(OutputIterator edges) const {
    HbrHalfedge<T>* start = GetIncidentEdge(), *edge, *next;
    edge = start;
    while (edge) {
        *edges++ = edge;
        next = GetNextEdge(edge);
        if (next == start) {
            break;
        } else if (!next) {
            // Special case for the last edge in a cycle.
            *edges++ = edge->GetPrev();
            break;
        } else {
            edge = next;
        }
    }
}

template <class T>
void
HbrVertex<T>::ApplyOperatorSurroundingEdges(HbrHalfedgeOperator<T> &op) const {
    HbrHalfedge<T>* start = GetIncidentEdge(), *edge, *next;
    edge = start;
    while (edge) {
        op(*edge);
        next = GetNextEdge(edge);
        if (next == start) {
            break;
        } else if (!next) {
            op(*edge->GetPrev());
            break;
        } else {
            edge = next;
        }
    }
}

template <class T>
template <typename OutputIterator>
void
HbrVertex<T>::GetSurroundingVertices(OutputIterator vertices) const {
    HbrMesh<T>* mesh = GetMesh();
    HbrHalfedge<T>* start = GetIncidentEdge(), *edge, *next;
    edge = start;
    while (edge) {
        *vertices++ = edge->GetDestVertex(mesh);
        next = GetNextEdge(edge);
        if (next == start) {
            break;
        } else if (!next) {
            // Special case for the last edge in a cycle: the last
            // vertex on that cycle is not the destination of an
            // outgoing halfedge
            *vertices++ = edge->GetPrev()->GetOrgVertex(mesh);
            break;
        } else {
            edge = next;
        }
    }
}

template <class T>
void
HbrVertex<T>::ApplyOperatorSurroundingVertices(HbrVertexOperator<T> &op) const {
    HbrMesh<T>* mesh = GetMesh();
    HbrHalfedge<T>* start = GetIncidentEdge(), *edge, *next;
    edge = start;
    while (edge) {
        op(*edge->GetDestVertex(mesh));
        next = GetNextEdge(edge);
        if (next == start) return;
        else if (!next) {
            op(*edge->GetPrev()->GetOrgVertex(mesh));
            return;
        } else {
            edge = next;
        }
    }
}

template <class T>
void
HbrVertex<T>::ApplyOperatorSurroundingFaces(HbrFaceOperator<T> &op) const {
    HbrHalfedge<T>* start = GetIncidentEdge(), *edge;
    edge = start;
    while (edge) {
        op(*edge->GetLeftFace());
        edge = GetNextEdge(edge);
        if (edge == start) break;
    }
}

template <class T>
HbrVertex<T>*
HbrVertex<T>::Subdivide() {
    HbrMesh<T>* mesh = GetMesh();
    if (vchild != -1) return mesh->GetVertex(vchild);
    HbrVertex<T>* vchildVert = mesh->GetSubdivision()->Subdivide(mesh, this);
    vchild = vchildVert->GetID();
    vchildVert->SetParent(this);
    return vchildVert;
}

template <class T>
void
HbrVertex<T>::Refine() {
    HbrMesh<T>* mesh = GetMesh();
    mesh->GetSubdivision()->RefineAtVertex(mesh, this);
}

template <class T>
void
HbrVertex<T>::GuaranteeNeighbors() {
    if (!neighborsguaranteed) {
        HbrMesh<T>* mesh = GetMesh();
        mesh->GetSubdivision()->GuaranteeNeighbors(mesh, this);
        neighborsguaranteed = 1;

        // At this point we can apply vertex edits because we have all
        // surrounding faces, and know whether any of them has
        // necessary edit information (they would have set our
        // hasvertexedit bit)
        if (hasvertexedit && !editsapplied) {
            HbrHalfedge<T>* start = GetIncidentEdge(), *edge;
            edge = start;
            while (edge) {
                HbrFace<T>* face = edge->GetLeftFace();
                if (HbrHierarchicalEdit<T>** edits = face->GetHierarchicalEdits()) {
                    while (HbrHierarchicalEdit<T>* edit = *edits) {
                        if (!edit->IsRelevantToFace(face)) break;
                        edit->ApplyEditToVertex(face, this);
                        edits++;
                    }
                }
                edge = GetNextEdge(edge);
                if (edge == start) break;
            }
            editsapplied = 1;
        }
    }
}

template <class T>
unsigned long
HbrVertex<T>::GetMemStats() const {
    return sizeof(HbrVertex<T>);
}


template <class T>
void
HbrVertex<T>::splitSingular() {
    HbrMesh<T>* mesh = GetMesh();
    HbrHalfedge<T>* e;
    HbrHalfedge<T>** incidentEdges =
        (nIncidentEdges > 1) ? incident.edges : &incident.edge;

    // Go through each edge cycle after the first
    std::vector<HbrHalfedge<T>*> edges;
    for (int i = 1; i < nIncidentEdges; ++i) {

        // Create duplicate vertex
        HbrVertex<T>* w = mesh->NewVertex();
        w->GetData().AddWithWeight(GetData(), 1.0);
        w->SetSharpness(GetSharpness());

        // Walk all edges in this cycle and reattach them to duplicate
        // vertex
        HbrHalfedge<T>* start = incidentEdges[i];
        e = start;
        edges.clear();
        do {
            edges.push_back(e);
            e = GetNextEdge(e);
        } while (e && e != start);

        for (typename std::vector<HbrHalfedge<T>*>::iterator ei = edges.begin(); ei != edges.end(); ++ei) {
            e = *ei;
            if (e->GetOpposite()) {
                HbrHalfedge<T>* next = e->GetOpposite()->GetNext();
                if (next->GetOrgVertex() == this) {
                    references--;
                    next->SetOrgVertex(w);
                    w->AddIncidentEdge(next);
                }
            }
            // Check again, because sometimes it's been relinked by
            // previous clause already
            if (e->GetOrgVertex() == this) {
                references--;
                e->SetOrgVertex(w);
                w->AddIncidentEdge(e);
            }
        }
        w->Finish();
#ifdef HBR_ADAPTIVE
        mesh->addSplitVertex(w->GetID(), this->GetID());
#endif
    }

    e = incidentEdges[0];
    if (nIncidentEdges > 1) {
        delete[] incidentEdges;
    }
    nIncidentEdges = 1;
    incident.edge = e;
}

template <class T>
std::ostream&
operator<<(std::ostream& out, const HbrVertex<T>& vertex) {
    return out << "vertex " << vertex.GetID();
}

template <class T>
class HbrVertexOperator {
public:
    virtual void operator() (HbrVertex<T> &vertex) = 0;
    virtual ~HbrVertexOperator() {}
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRVERTEX_H */

