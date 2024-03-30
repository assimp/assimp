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

#ifndef OPENSUBDIV3_HBRFACE_H
#define OPENSUBDIV3_HBRFACE_H

#include <assert.h>
#include <cstdio>
#include <functional>
#include <iostream>
#include <algorithm>
#include <vector>

#include "../hbr/fvarData.h"
#include "../hbr/allocator.h"
#ifdef HBRSTITCH
#include "libgprims/stitch.h"
#include "libgprims/stitchInternal.h"
#endif

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrVertex;
template <class T> class HbrHalfedge;
template <class T> class HbrFace;
template <class T> class HbrMesh;
template <class T> class HbrHierarchicalEdit;

template <class T> std::ostream& operator<<(std::ostream& out, const HbrFace<T>& face);

// A descriptor for a path to a face
struct HbrFacePath {
    void Print() const {
        printf("%d", topface);
        for (std::vector<int>::const_reverse_iterator i = remainder.rbegin(); i != remainder.rend(); ++i) {
            printf(" %d", *i);
        }
        printf("\n");
    }

    int topface;
    // Note that the elements in remainder are stored in reverse order.
    std::vector<int> remainder;
    friend bool operator< (const HbrFacePath& x, const HbrFacePath& y);
};

inline bool operator< (const HbrFacePath& x, const HbrFacePath& y) {
    if (x.topface != y.topface) {
        return x.topface < y.topface;
    } else if (x.remainder.size() != y.remainder.size()) {
        return x.remainder.size() < y.remainder.size();
    } else {
        std::vector<int>::const_reverse_iterator i = x.remainder.rbegin();
        std::vector<int>::const_reverse_iterator j = y.remainder.rbegin();
        for ( ; i != x.remainder.rend(); ++i, ++j) {
            if (*i != *j) return (*i < *j);
        }
        return true;
    }
}

// A simple wrapper around an array of four children. Used to block
// allocate pointers to children of HbrFace in the common case
template <class T>
class HbrFaceChildren {
public:
    HbrFace<T> *& operator[](const int index) {
        return children[index];
    }

    const HbrFace<T> *& operator[](const int index) const {
        return children[index];
    }
    

private:
    friend class HbrAllocator<HbrFaceChildren<T> >;

    // Used by block allocator
    HbrFaceChildren<T>*& GetNext() { return (HbrFaceChildren<T>*&) children; }
    
    HbrFaceChildren() {}

    ~HbrFaceChildren() {}

    HbrFace<T> *children[4];
};

template <class T> class HbrFace {

private:
    friend class HbrAllocator<HbrFace<T> >;
    friend class HbrHalfedge<T>;
    HbrFace();
    ~HbrFace();

public:

    void Initialize(HbrMesh<T>* mesh, HbrFace<T>* parent, int childindex, int id, int uindex, int nvertices, HbrVertex<T>** vertices, int fvarwidth = 0, int depth = 0);
    void Destroy();

    // Returns the mesh to which this face belongs
    HbrMesh<T>* GetMesh() const { return mesh; }

    // Return number of vertices
    int GetNumVertices() const { return nvertices; }

    // Return face ID
    int GetID() const { return id; }

    // Return the first halfedge of the face
    HbrHalfedge<T>* GetFirstEdge() const {
        if (nvertices > 4) {
            return (HbrHalfedge<T>*)(extraedges);
        } else {
            return const_cast<HbrHalfedge<T>*>(&edges[0]);
        }
    }

    // Return the halfedge which originates at the vertex with the
    // indicated origin index
    HbrHalfedge<T>* GetEdge(int index) const;

    // Return the vertex with the indicated index
    HbrVertex<T>* GetVertex(int index) const;

    // Return the ID of the vertex with the indicated index
    int GetVertexID(int index) const;

    // Return the parent of this face
    HbrFace<T>* GetParent() const {
        if (parent == -1) return NULL;
        return mesh->GetFace(parent);
    }

    // Set the child
    void SetChild(int index, HbrFace<T>* face);

    // Return the child with the indicated index
    HbrFace<T>* GetChild(int index) const {
        int nchildren = mesh->GetSubdivision()->GetFaceChildrenCount(nvertices);
        if (!children.children || index < 0 || index >= nchildren) return 0;
        if (nchildren > 4) {
            return children.extrachildren[index];
        } else {
            return (*children.children)[index];
        }
    }

    // Subdivide the face into a vertex if needed and return
    HbrVertex<T>* Subdivide();

    bool HasChildVertex() const { return vchild!=-1; }

    // Remove the reference to subdivided vertex
    void RemoveChild() { vchild = -1; }

    // "Hole" flags used by subdivision to drop faces
    bool IsHole() const { return hole; }
    void SetHole(bool h=1) { hole = h; }

    // Coarse faces are the top level faces of a mesh. This will be
    // set by mesh->Finish()
    bool IsCoarse() const { return coarse; }
    void SetCoarse() { coarse = 1; }

    // Protected faces cannot be garbage collected; this may be set on
    // coarse level faces if the mesh is shared
    bool IsProtected() const { return protect; }
    void SetProtected() { protect = 1; }
    void ClearProtected() { protect = 0; }

    // Simple bookkeeping needed for garbage collection by HbrMesh
    bool IsCollected() const { return collected; }
    void SetCollected() { collected = 1; }
    void ClearCollected() { collected = 0; }

    // Refine the face
    void Refine();

    // Unrefine the face
    void Unrefine();

    // Returns true if the face has a limit surface
    bool HasLimit();

    // Returns memory statistics
    unsigned long GetMemStats() const;

    // Return facevarying data from the appropriate vertex index
    // registered to this face. Note that this may either be "generic"
    // facevarying item (data.GetFace() == 0) or one specifically
    // registered to the face (data.GetFace() == this) - this is
    // important when trying to figure out whether the vertex has
    // created some storage for the item designed to store
    // discontinuous values for this face.
    HbrFVarData<T>& GetFVarData(int index) {
        return GetVertex(index)->GetFVarData(this);
    }

    // Mark this face as being used, which in turn increments the
    // usage counter of all vertices in the support for the face. A
    // used face can not be garbage collected
    void MarkUsage();

    // Clears the usage of this face, which in turn decrements the
    // usage counter of all vertices in the support for the face and
    // marks the face as a candidate for garbage collection
    void ClearUsage();

    // A face can be cleaned if all of its vertices are not being
    // used; has no children; and (for top level faces) deletion of
    // its edges will not leave singular vertices
    bool GarbageCollectable() const;

    // Connect this face to a list of hierarchical edits
    void SetHierarchicalEdits(HbrHierarchicalEdit<T>** edits);

    // Return the list of hierarchical edits associated with this face
    HbrHierarchicalEdit<T>** GetHierarchicalEdits() const {
        if (editOffset == -1) {
            return NULL;
        }
        return mesh->GetHierarchicalEditsAtOffset(editOffset);
    }

    // Whether the face has certain types of edits (not necessarily
    // local - could apply to a subface)
    bool HasVertexEdits() const { return hasVertexEdits; }
    void MarkVertexEdits() { hasVertexEdits = 1; }

    // Return the depth of the face
    int GetDepth() const { return static_cast<int>(depth); }

    // Return the uniform index of the face. This is different
    // from the ID because it may be shared with other faces
    int GetUniformIndex() const { return uindex; }

    // Set the uniform index of the face
    void SetUniformIndex(int i) { uindex = i; }

    // Return the ptex index
    int GetPtexIndex() const { return ptexindex; }

    // Set the ptex index of the face
    void SetPtexIndex(int i) { ptexindex = i; }

    // Used by block allocator
    HbrFace<T>*& GetNext() { return (HbrFace<T>*&) mesh; }

    HbrFacePath GetPath() const {
        HbrFacePath path;
        path.remainder.reserve(GetDepth());
        const HbrFace<T>* f = this, *p = GetParent();
        while (p) {
            int nchildren = mesh->GetSubdivision()->GetFaceChildrenCount(p->nvertices);
            if (nchildren > 4) {
                for (int i = 0; i < nchildren; ++i) {
                    if (p->children.extrachildren[i] == f) {
                        path.remainder.push_back(i);
                        break;
                    }
                }
            } else {
                for (int i = 0; i < nchildren; ++i) {
                    if ((*p->children.children)[i] == f) {
                        path.remainder.push_back(i);
                        break;
                    }
                }
            }
            f = p;
            p = f->GetParent();
        }
        path.topface = f->GetID();
        assert(GetDepth() == 0 || static_cast<int>(path.remainder.size()) == GetDepth());
        return path;
    }

    void PrintPath() const {
        GetPath().Print();
    }

    // Returns the blind pointer to client data
    void *GetClientData() const {
        return mesh->GetFaceClientData(id);
    }

    // Sets the blind pointer to client data
    void SetClientData(void *data) {
        mesh->SetFaceClientData(id, data);
    }

    // Gets the list of vertices which are in the support for the face.
    void GetSupportingVertices(std::vector<int> &support);

private:

    // Mesh to which this face belongs
    HbrMesh<T>* mesh;

    // Unique id for this face
    int id;

    // Uniform index
    int uindex;

    // Ptex index
    int ptexindex;

    // Number of vertices (and number of edges)
    int nvertices;

    // Halfedge array for this face
    HbrHalfedge<T> edges[4];

    // Edge storage if this face is not a triangle or quad
    char* extraedges;

    // Pointer to children array. If there are four children or less,
    // we use the HbrFaceChildren pointer, otherwise we use
    // extrachildren
    union {
        HbrFaceChildren<T>* children;
        HbrFace<T>** extrachildren;
    } children;

    // Bits used by halfedges to track facevarying sharpnesses
    unsigned int *fvarbits;

#ifdef HBRSTITCH
    // Pointers to stitch edges used by the half edges.
    StitchEdge **stitchEdges;
#endif

    // Index of parent face
    int parent;

    // Index of subdivided vertex child
    int vchild;

    // Offset to the mesh' list of hierarchical edits applicable to this face
    int editOffset;

    // Depth of the face in the mesh hierarchy - coarse faces are
    // level 0. (Hmmm.. is it safe to assume that we'll never
    // subdivide to greater than 255?)
    unsigned char depth;

    unsigned short hole:1;
    unsigned short coarse:1;
    unsigned short protect:1;
    unsigned short collected:1;
    unsigned short hasVertexEdits:1;
    unsigned short initialized:1;
    unsigned short destroyed:1;

#ifdef HBR_ADAPTIVE
public:
    enum PatchType {  kUnknown=0,
                         kFull=1,
                          kEnd=2,
                      kGregory=3 };
                     
    enum TransitionType { kTransition0=0,
                          kTransition1=1,
                          kTransition2=2,
                          kTransition3=3,
                          kTransition4=4,
                                 kNone=5 };
 
    struct AdaptiveFlags {
        unsigned patchType:2;
        unsigned transitionType:3;
        unsigned rots:2; 
        unsigned brots:2; 
        unsigned bverts:2; 
        unsigned isCritical:1;
        unsigned isExtraordinary:1;
        unsigned isTagged:1;
        
        AdaptiveFlags() : patchType(0), transitionType(5), rots(0), brots(0), bverts(0), isCritical(0), isExtraordinary(0), isTagged(0) { }
    };
    
    AdaptiveFlags _adaptiveFlags;

    bool isTransitionPatch() const {
        return (_adaptiveFlags.transitionType!=kNone);
    }
        
    bool hasTaggedVertices() {
        int nv = GetNumVertices();
        for (int i=0; i<nv; ++i) {
            if (GetVertex(i)->_adaptiveFlags.wasTagged)
                return true;
        }
        return false;
    }    
#endif
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#include "../hbr/mesh.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T>
HbrFace<T>::HbrFace()
    : mesh(0), id(-1), uindex(-1), ptexindex(-1), nvertices(0), extraedges(0), fvarbits(0), parent(-1), vchild(-1), 
#ifdef HBRSTITCH
      stitchEdges(0),
#endif
      editOffset(-1), depth(0), hole(0), coarse(0), protect(0), collected(0), hasVertexEdits(0), initialized(0), destroyed(0) {
    children.children = 0;
}

template <class T>
void
HbrFace<T>::Initialize(HbrMesh<T>* m, HbrFace<T>* _parent, int childindex, int fid, int _uindex, int nv, HbrVertex<T>** vertices, int /* fvarwidth */, int _depth) {
    mesh = m;
    id = fid;
    uindex = _uindex;
    ptexindex = -1;
    nvertices = nv;
    extraedges = 0;
    children.children = 0;
    vchild = -1;
    fvarbits = 0;
#ifdef HBRSTITCH
    stitchEdges = 0;
#endif
    editOffset = -1;
    depth = static_cast<unsigned char>(_depth);
    hole = 0;
    coarse = 0;
    protect = 0;
    collected = 0;
    hasVertexEdits = 0;
    initialized = 1;
    destroyed = 0;

    int i;
    const int fvarcount = mesh->GetFVarCount();
    int fvarbitsSizePerEdge = ((fvarcount + 15) / 16);

    if (nv > 4) {

        // If we have more than four vertices, we ignore the
        // overallocation and allocate our own buffers for stitch
        // edges and facevarying data.
#ifdef HBRSTITCH
        if (mesh->GetStitchCount()) {
            const size_t buffersize = nv * (mesh->GetStitchCount() * sizeof(StitchEdge*));
            char *buffer = (char *) malloc(buffersize);
            memset(buffer, 0, buffersize);
            stitchEdges = (StitchEdge**) buffer;
        }
#endif
        if (fvarcount) {
            // We allocate fvarbits in one chunk.
            // fvarbits needs capacity for two bits per fvardatum per edge,
            // minimum size one integer per edge
            const size_t fvarbitsSize = nv * (fvarbitsSizePerEdge * sizeof(unsigned int));
            char *buffer = (char*) malloc(fvarbitsSize);
            fvarbits = (unsigned int*) buffer;
        }

        // We also ignore the edge array and allocate extra storage -
        // this simplifies GetNext and GetPrev math in HbrHalfedge
        const size_t edgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
        extraedges = (char *) malloc(nv * edgesize);
        for (i = 0; i < nv; ++i) {
            HbrHalfedge<T>* edge = (HbrHalfedge<T>*)(extraedges + i * edgesize);
            new (edge) HbrHalfedge<T>();
        }

    } else {
        // Under four vertices: upstream allocation for the class has
        // been over allocated to include storage for stitchEdges
        // and fvarbits. Just point our pointers at it.
        char *buffer = ((char *) this + sizeof(*this));
#ifdef HBRSTITCH
        if (mesh->GetStitchCount()) {
            const size_t buffersize = 4 * (mesh->GetStitchCount() * sizeof(StitchEdge*));
            memset(buffer, 0, buffersize);
            stitchEdges = (StitchEdge**) buffer;
            buffer += buffersize;
        }
#endif
        if (fvarcount) {
            fvarbits = (unsigned int*) buffer;
        }
    }

    // Must do this before we create edges
    if (_parent) {
        _parent->SetChild(childindex, this);
    }

    // Edges must be constructed in this two part approach: we must
    // ensure that opposite/next/previous ptrs are all set up
    // correctly, before we can begin adding incident edges to
    // vertices.
    int next;
    unsigned int *curfvarbits = fvarbits;
    HbrHalfedge<T>* edge;
    size_t edgesize;
    if (nv > 4) {
        edge = (HbrHalfedge<T>*)(extraedges);
        edgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
    } else {
        edge = edges;
        edgesize = sizeof(HbrHalfedge<T>);
    }
    for (i = 0, next = 1; i < nv; ++i, ++next) {
        if (next == nv) next = 0;
        HbrHalfedge<T>* opposite = vertices[next]->GetEdge(vertices[i]->GetID());
        edge->Initialize(opposite, i, vertices[i], curfvarbits, this);
        if (opposite) opposite->SetOpposite(edge);
        if (fvarbits) {
            curfvarbits = curfvarbits + fvarbitsSizePerEdge;
        }
        edge = (HbrHalfedge<T>*)((char *) edge + edgesize);
    }
    if (nv > 4) {
        edge = (HbrHalfedge<T>*)(extraedges);
    } else {
        edge = edges;
    }
    for (i = 0; i < nv; ++i) {
        vertices[i]->AddIncidentEdge(edge);
        edge = (HbrHalfedge<T>*)((char *) edge + edgesize);        
    }
}

template <class T>
HbrFace<T>::~HbrFace() {
    Destroy();
}

template <class T>
void
HbrFace<T>::Destroy() {
    if (initialized && !destroyed) {
        int i;
#ifdef HBRSTITCH
        const int stitchCount = mesh->GetStitchCount();
#endif

        // Remove children's references to self
        if (children.children) {
            int nchildren = mesh->GetSubdivision()->GetFaceChildrenCount(nvertices);
            if (nchildren > 4) {
                for (i = 0; i < nchildren; ++i) {
                    if (children.extrachildren[i]) {
                        children.extrachildren[i]->parent = -1;
                        children.extrachildren[i] = 0;
                    }
                }
                delete[] children.extrachildren;
                children.extrachildren = 0;
            } else {
                for (i = 0; i < nchildren; ++i) {
                    if ((*children.children)[i]) {
                        (*children.children)[i]->parent = -1;
                        (*children.children)[i] = 0;
                    }
                }
                mesh->DeleteFaceChildren(children.children);
                children.children = 0;
            }
        }

        // Deleting the incident edges from the vertices in this way is
        // the safest way of doing things. Doing it in the halfedge
        // destructor will not work well because it disrupts cycle
        // finding/incident edge replacement in the vertex code.
        // We also take this time to clean up any orphaned stitches
        // still belonging to the edges.
        HbrHalfedge<T>* edge;
        size_t edgesize;
        if (nvertices > 4) {
            edge = (HbrHalfedge<T>*)(extraedges);
            edgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
        } else {
            edge = edges;
            edgesize = sizeof(HbrHalfedge<T>);
        }
        for (i = 0; i < nvertices; ++i) {
#ifdef HBRSTITCH
            edge->DestroyStitchEdges(stitchCount);
#endif
            HbrVertex<T>* vertex = mesh->GetVertex(edge->GetOrgVertexID());
            if (fvarbits) {
                HbrFVarData<T>& fvt = vertex->GetFVarData(this);
                if (fvt.GetFaceID() == GetID()) {
                    fvt.SetFaceID(-1);
                }
            }
            vertex->RemoveIncidentEdge(edge);
            vertex->UnGuaranteeNeighbors();
            edge = (HbrHalfedge<T>*)((char *) edge + edgesize);
        }
        if (extraedges) {
            edge = (HbrHalfedge<T>*)(extraedges);            
            for (i = 0; i < nvertices; ++i) {
                edge->~HbrHalfedge<T>();
                edge = (HbrHalfedge<T>*)((char *) edge + edgesize);
            }
            free(extraedges);
            extraedges = 0;
        }

        // Remove parent's reference to self
        HbrFace<T> *parentFace = GetParent();
        if (parentFace) {
            bool parentHasOtherKids = false;
            int nchildren = mesh->GetSubdivision()->GetFaceChildrenCount(parentFace->nvertices);
            if (nchildren > 4) {
                for (i = 0; i < nchildren; ++i) {
                    if (parentFace->children.extrachildren[i] == this) {
                        parentFace->children.extrachildren[i] = 0;
                    } else if (parentFace->children.extrachildren[i]) parentHasOtherKids = true;
                }
                // After cleaning the parent's reference to self, the parent
                // may be able to clean itself up
                if (!parentHasOtherKids) {
                    delete[] parentFace->children.extrachildren;
                    parentFace->children.extrachildren = 0;
                    if (parentFace->GarbageCollectable()) {
                        mesh->DeleteFace(parentFace);
                    }
                }
            } else {
                for (i = 0; i < nchildren; ++i) {
                    if ((*parentFace->children.children)[i] == this) {
                        (*parentFace->children.children)[i] = 0;
                    } else if ((*parentFace->children.children)[i]) parentHasOtherKids = true;
                }
                // After cleaning the parent's reference to self, the parent
                // may be able to clean itself up
                if (!parentHasOtherKids) {
                    mesh->DeleteFaceChildren(parentFace->children.children);
                    parentFace->children.children = 0;
                    if (parentFace->GarbageCollectable()) {
                        mesh->DeleteFace(parentFace);
                    }
                }
            }
            parent = -1;
        }

        // Orphan the child vertex
        if (vchild != -1) {
            HbrVertex<T> *vchildVert = mesh->GetVertex(vchild);
            vchildVert->SetParent(static_cast<HbrFace*>(0));
            vchild = -1;
        }

        if (nvertices > 4 && fvarbits) {
            free(fvarbits);
#ifdef HBRSTITCH
            if (stitchEdges) {
                free(stitchEdges);
            }
#endif
        }
        fvarbits = 0;
#ifdef HBRSTITCH
        stitchEdges = 0;
#endif

        // Make sure the four edges intrinsic to face are properly cleared
        // if they were used
        if (nvertices <= 4) {
            for (i = 0; i < nvertices; ++i) {
                edges[i].Clear();
            }
        }
        nvertices = 0;
        initialized = 0;
        mesh = 0;
        destroyed = 1;
    }
}

template <class T>
HbrHalfedge<T>*
HbrFace<T>::GetEdge(int index) const {
    assert(index >= 0 && index < nvertices);
    if (nvertices > 4) {
        const size_t edgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
        return (HbrHalfedge<T>*)(extraedges + index * edgesize);
    } else {
        return const_cast<HbrHalfedge<T>*>(edges + index);
    }
}

template <class T>
HbrVertex<T>*
HbrFace<T>::GetVertex(int index) const {
    assert(index >= 0 && index < nvertices);
    if (nvertices > 4) {
        const size_t edgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);        
        HbrHalfedge<T>* edge = (HbrHalfedge<T>*)(extraedges +
            index * edgesize);
        return mesh->GetVertex(edge->GetOrgVertexID());
    } else {
        return mesh->GetVertex(edges[index].GetOrgVertexID());
    }
}

template <class T>
int
HbrFace<T>::GetVertexID(int index) const {
    assert(index >= 0 && index < nvertices);    
    if (nvertices > 4) {
        const size_t edgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);        
        HbrHalfedge<T>* edge = (HbrHalfedge<T>*)(extraedges +
            index * edgesize);
        return edge->GetOrgVertexID();
    } else {
        return edges[index].GetOrgVertexID();
    }
}

template <class T>
void
HbrFace<T>::SetChild(int index, HbrFace<T>* face) {
    assert(id != -1);
    int nchildren = mesh->GetSubdivision()->GetFaceChildrenCount(nvertices);
    // Construct the children array if it doesn't already exist
    if (!children.children) {
        int i;
        if (nchildren > 4) {
            children.extrachildren = new HbrFace<T>*[nchildren];
            for (i = 0; i < nchildren; ++i) {
                children.extrachildren[i] = 0;
            }
        } else {
            children.children = mesh->NewFaceChildren();
            for (i = 0; i < nchildren; ++i) {
                (*children.children)[i] = 0;
            }
        }
    }
    if (nchildren > 4) {
        children.extrachildren[index] = face;
    } else {
        (*children.children)[index] = face;
    }
    face->parent = this->id;
}

template <class T>
HbrVertex<T>*
HbrFace<T>::Subdivide() {
    if (vchild != -1) return mesh->GetVertex(vchild);
    HbrVertex<T>* vchildVert = mesh->GetSubdivision()->Subdivide(mesh, this);
    vchild = vchildVert->GetID();
    vchildVert->SetParent(this);
    return vchildVert;
}

template <class T>
void
HbrFace<T>::Refine() {
    mesh->GetSubdivision()->Refine(mesh, this);
}

template <class T>
void
HbrFace<T>::Unrefine() {
    // Delete the children, via the mesh (so that the mesh loses
    // references to the children)
    if (children.children) {
        int nchildren = mesh->GetSubdivision()->GetFaceChildrenCount(nvertices);
        if (nchildren > 4) {
            for (int i = 0; i < nchildren; ++i) {
                if (children.extrachildren[i]) mesh->DeleteFace(children.extrachildren[i]);
            }
            delete[] children.extrachildren;
            children.extrachildren = 0;
        } else {
            for (int i = 0; i < nchildren; ++i) {
                if ((*children.children)[i]) mesh->DeleteFace((*children.children)[i]);
            }
            mesh->DeleteFaceChildren(children.children);
            children.children = 0;
        }
    }
}

template <class T>
bool
HbrFace<T>::HasLimit() {
    return mesh->GetSubdivision()->HasLimit(mesh, this);
}

template <class T>
unsigned long
HbrFace<T>::GetMemStats() const {
    return sizeof(HbrFace<T>);
}

template <class T>
void
HbrFace<T>::MarkUsage() {
    // Must increment the usage on all vertices which are in the
    // support for this face. Note well: this will increment vertices
    // more than once. This doesn't really matter as long as
    // ClearUsage also does the same number of decrements. If we
    // really were concerned about ensuring single increments, we can
    // use GetSupportingVertices, but that's slower.
    HbrVertex<T>* v;
    HbrHalfedge<T>* e, *ee, *eee, *start;
    size_t edgesize, eedgesize;
    if (nvertices > 4) {
        e = (HbrHalfedge<T>*)(extraedges);
        edgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
    } else {
        e = edges;
        edgesize = sizeof(HbrHalfedge<T>);
    }
    for (int i = 0; i < nvertices; ++i) {
        v = mesh->GetVertex(e->GetOrgVertexID());
        v->GuaranteeNeighbors();
        start = v->GetIncidentEdge();
        ee = start;
        do {
            HbrFace<T>* f = ee->GetLeftFace();
            int nv = f->GetNumVertices();
            if (nv > 4) {
                eee = (HbrHalfedge<T>*)(f->extraedges);
                eedgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
            } else {
                eee = f->edges;
                eedgesize = sizeof(HbrHalfedge<T>);
            }
            for (int j = 0; j < nv; ++j) {
                mesh->GetVertex(eee->GetOrgVertexID())->IncrementUsage();
                eee = (HbrHalfedge<T>*)((char *) eee + eedgesize);
            }
            ee = v->GetNextEdge(ee);
            if (ee == start) break;
        } while (ee);
        e = (HbrHalfedge<T>*)((char *) e + edgesize);
    }
}

template <class T>
void
HbrFace<T>::ClearUsage() {
    bool gc = false;
    // Must mark all vertices which may affect this face
    HbrVertex<T>* v, *vv;
    HbrHalfedge<T>* e, *ee, *eee, *start;
    size_t edgesize, eedgesize;
    if (nvertices > 4) {
        e = (HbrHalfedge<T>*)(extraedges);
        edgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
    } else {
        e = edges;
        edgesize = sizeof(HbrHalfedge<T>);
    }
    for (int i = 0; i < nvertices; ++i) {
        v = mesh->GetVertex(e->GetOrgVertexID());
        start = v->GetIncidentEdge();
        ee = start;
        do {
            HbrFace<T>* f = ee->GetLeftFace();
            int nv = f->GetNumVertices();
            if (nv > 4) {
                eee = (HbrHalfedge<T>*)(f->extraedges);
                eedgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
            } else {
                eee = f->edges;
                eedgesize = sizeof(HbrHalfedge<T>);
            }
            for (int j = 0; j < nv; ++j) {
                HbrVertex<T>* vert = mesh->GetVertex(eee->GetOrgVertexID());
                vert->DecrementUsage();
                if (!vert->IsUsed()) {
                    mesh->AddGarbageCollectableVertex(vert);
                    gc = true;
                }
                eee = (HbrHalfedge<T>*)((char *) eee + eedgesize);
            }
            ee = v->GetNextEdge(ee);
            if (ee == start) break;
        } while (ee);
        e = (HbrHalfedge<T>*)((char *) e + edgesize);
    }
    if (gc) mesh->GarbageCollect();
}

template <class T>
bool
HbrFace<T>::GarbageCollectable() const {
    if (children.children || protect) return false;
    for (int i = 0; i < nvertices; ++i) {
        HbrHalfedge<T>* edge = GetEdge(i);
        HbrVertex<T>* vertex = edge->GetOrgVertex(mesh);
        if (vertex->IsUsed()) return false;
        if (!GetParent() && vertex->EdgeRemovalWillMakeSingular(edge)) {
            return false;
        }
    }
    return true;
}

template <class T>
void
HbrFace<T>::SetHierarchicalEdits(HbrHierarchicalEdit<T>** edits) {
    HbrHierarchicalEdit<T>** faceedits = edits;
    HbrHierarchicalEdit<T>** baseedit = mesh->GetHierarchicalEditsAtOffset(0);
    editOffset = int(faceedits - baseedit);

    // Walk the list of edits and look for any which apply locally.
    while (HbrHierarchicalEdit<T>* edit = *faceedits) {
        if (!edit->IsRelevantToFace(this)) break;
        edit->ApplyEditToFace(this);
        faceedits++;
    }
}

template <class T>
void
HbrFace<T>::GetSupportingVertices(std::vector<int> &support) {
    support.reserve(16);
    HbrVertex<T>* v;    
    HbrHalfedge<T>* e, *ee, *eee, *start;
    size_t edgesize, eedgesize;
    if (nvertices > 4) {
        e = (HbrHalfedge<T>*)(extraedges);
        edgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
    } else {
        e = edges;
        edgesize = sizeof(HbrHalfedge<T>);
    }
    for (int i = 0; i < nvertices; ++i) {
        v = mesh->GetVertex(e->GetOrgVertexID());
        v->GuaranteeNeighbors();
        start = v->GetIncidentEdge();
        ee = start;
        do {
            HbrFace<T>* f = ee->GetLeftFace();
            int nv = f->GetNumVertices();
            if (nv > 4) {
                eee = (HbrHalfedge<T>*)(f->extraedges);
                eedgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
            } else {
                eee = f->edges;
                eedgesize = sizeof(HbrHalfedge<T>);
            }
            for (int j = 0; j < nv; ++j) {
                int id = eee->GetOrgVertexID();
                std::vector<int>::iterator vi =
                    std::lower_bound(support.begin(), support.end(), id);
                if (vi == support.end() || *vi != id) {
                    support.insert(vi, id);
                }
                eee = (HbrHalfedge<T>*)((char *) eee + eedgesize);
            }
            ee = v->GetNextEdge(ee);
            if (ee == start) break;
        } while (ee);
        e = (HbrHalfedge<T>*)((char *) e + edgesize);
    }
}

template <class T>
std::ostream& operator<<(std::ostream& out, const HbrFace<T>& face) {
    out << "face " << face.GetID() << ", " << face.GetNumVertices() << " vertices (";
    for (int i = 0; i < face.GetNumVertices(); ++i) {
        HbrHalfedge<T>* e = face.GetEdge(i);
        out << *(e->GetOrgVertex());
        if (e->IsBoundary()) {
            out << " -/-> ";
        } else {
            out << " ---> ";
        }
    }
    out << ")";
    return out;
}

template <class T>
class HbrFaceOperator {
public:
    virtual void operator() (HbrFace<T> &face) = 0;
    virtual ~HbrFaceOperator() {}
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRFACE_H */
