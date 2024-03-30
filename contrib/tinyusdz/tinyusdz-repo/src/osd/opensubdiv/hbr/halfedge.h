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

#ifndef OPENSUBDIV3_HBRHALFEDGE_H
#define OPENSUBDIV3_HBRHALFEDGE_H

#include <assert.h>
#include <stddef.h>
#include <cstring>
#include <iostream>


#ifdef HBRSTITCH
#include "libgprims/stitch.h"
#include "libgprims/stitchInternal.h"
#endif

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrFace;
template <class T> class HbrHalfedge;
template <class T> class HbrVertex;
template <class T> class HbrMesh;

template <class T> std::ostream& operator<<(std::ostream& out, const HbrHalfedge<T>& edge);

template <class T> class HbrHalfedge {

private:
    HbrHalfedge(): opposite(0), incidentVertex(-1), vchild(-1), sharpness(0.0f)
#ifdef HBRSTITCH
    , stitchccw(1), raystitchccw(1)
#endif
    , coarse(1)
    {
    }

    HbrHalfedge(const HbrHalfedge &/* edge */) {}
    
    ~HbrHalfedge();

    void Clear();

    // Finish the initialization of the halfedge. Should only be
    // called by HbrFace
    void Initialize(HbrHalfedge<T>* opposite, int index, HbrVertex<T>* origin, unsigned int *fvarbits, HbrFace<T>* face);
public:

    // Returns the opposite half edge
    HbrHalfedge<T>* GetOpposite() const { return opposite; }

    // Sets the opposite half edge
    void SetOpposite(HbrHalfedge<T>* opposite) { this->opposite = opposite; sharpness = opposite->sharpness; }

    // Returns the next clockwise halfedge around the incident face
    HbrHalfedge<T>* GetNext() const {
        if (m_index == 4) {
            const size_t edgesize = sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*);
            if (lastedge) {
                return (HbrHalfedge<T>*) ((char*) this - (GetFace()->GetNumVertices() - 1) * edgesize);
            } else {
                return (HbrHalfedge<T>*) ((char*) this + edgesize);
            }
        } else {
            if (lastedge) {
                return (HbrHalfedge<T>*) ((char*) this - (m_index) * sizeof(HbrHalfedge<T>));
            } else {
                return (HbrHalfedge<T>*) ((char*) this + sizeof(HbrHalfedge<T>));
            }
        }
    }

    // Returns the previous counterclockwise halfedge around the incident face
    HbrHalfedge<T>* GetPrev() const {
        const size_t edgesize = (m_index == 4) ? 
            (sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*)) :
            sizeof(HbrHalfedge<T>);
        if (firstedge) {
            return (HbrHalfedge<T>*) ((char*) this + (GetFace()->GetNumVertices() - 1) * edgesize);
        } else {
            return (HbrHalfedge<T>*) ((char*) this - edgesize);
        }
    }

    // Returns the incident vertex
    HbrVertex<T>* GetVertex() const {
        return GetMesh()->GetVertex(incidentVertex);
        }

    // Returns the incident vertex
    HbrVertex<T>* GetVertex(HbrMesh<T> *mesh) const {
        return mesh->GetVertex(incidentVertex);
    }

    // Returns the incident vertex
    int GetVertexID() const {
        return incidentVertex;
    }

    // Returns the source vertex
    HbrVertex<T>* GetOrgVertex() const {
        return GetVertex();
    }

    // Returns the source vertex
    HbrVertex<T>* GetOrgVertex(HbrMesh<T> *mesh) const {
        return GetVertex(mesh);
    }

    // Returns the source vertex id
    int GetOrgVertexID() const {
        return incidentVertex;
    }

    // Changes the origin vertex. Generally not a good idea to do
    void SetOrgVertex(HbrVertex<T>* v) { incidentVertex = v->GetID(); }

    // Returns the destination vertex
    HbrVertex<T>* GetDestVertex() const { return GetNext()->GetOrgVertex(); }

    // Returns the destination vertex
    HbrVertex<T>* GetDestVertex(HbrMesh<T> *mesh) const { return GetNext()->GetOrgVertex(mesh); }

    // Returns the destination vertex ID
    int GetDestVertexID() const { return GetNext()->GetOrgVertexID(); }
    
    // Returns the incident facet
    HbrFace<T>* GetFace() const {
        if (m_index == 4) {
            // Pointer to face is stored after the data for the edge
            return *(HbrFace<T>**)((char *) this + sizeof(HbrHalfedge<T>));
        } else {
            return (HbrFace<T>*) ((char*) this - (m_index) * sizeof(HbrHalfedge<T>) -
                offsetof(HbrFace<T>, edges));
        }
    }

    // Returns the mesh to which this edge belongs
    HbrMesh<T>* GetMesh() const { return GetFace()->GetMesh(); }

    // Returns the face on the right
    HbrFace<T>* GetRightFace() const { return opposite ? opposite->GetLeftFace() : NULL; }

    // Return the face on the left of the halfedge
    HbrFace<T>* GetLeftFace() const { return GetFace(); }

    // Returns whether this is a boundary edge
    bool IsBoundary() const { return opposite == 0; }

    // Tag the edge as being an infinitely sharp facevarying edge
    void SetFVarInfiniteSharp(int datum, bool infsharp) {
        int intindex = datum >> 4;
        unsigned int bits = infsharp << ((datum & 15) * 2);
        getFVarInfSharp()[intindex] |= bits;
        if (opposite) {
            opposite->getFVarInfSharp()[intindex] |= bits;
        }
    }

    // Copy fvar infinite sharpness flags from another edge
    void CopyFVarInfiniteSharpness(HbrHalfedge<T>* edge) {
        unsigned int *fvarinfsharp = getFVarInfSharp();
        if (fvarinfsharp) {
            const int fvarcount = GetMesh()->GetFVarCount();
            int fvarbitsSizePerEdge = ((fvarcount + 15) / 16);
            
            if (edge->IsSharp(true)) {
                memset(fvarinfsharp, 0x55555555, fvarbitsSizePerEdge * sizeof(unsigned int));
            } else {
                memcpy(fvarinfsharp, edge->getFVarInfSharp(), fvarbitsSizePerEdge * sizeof(unsigned int));
            }
        }
    }

    // Returns whether the edge is infinitely sharp in facevarying for
    // a particular facevarying datum
    bool GetFVarInfiniteSharp(int datum);

    // Returns whether the edge is infinitely sharp in any facevarying
    // datum
    bool IsFVarInfiniteSharpAnywhere();

    // Get the sharpness relative to facevarying data
    float GetFVarSharpness(int datum, bool ignoreGeometry=false);

    // Returns the (raw) sharpness of the edge
    float GetSharpness() const { return sharpness; }

    // Sets the sharpness of the edge
    void SetSharpness(float sharp) { sharpness = sharp; if (opposite) opposite->sharpness = sharp; ClearMask(); }

    // Returns whether the edge is sharp at the current level of
    // subdivision (next = false) or at the next level of subdivision
    // (next = true).
    bool IsSharp(bool next) const { return (next ? (sharpness > 0.0f) : (sharpness >= 1.0f)); }

    // Clears the masks of the adjacent edge vertices. Usually called
    // when a change in edge sharpness occurs.
    void ClearMask() { GetOrgVertex()->ClearMask(); GetDestVertex()->ClearMask(); }

    // Subdivide the edge into a vertex if needed and return
    HbrVertex<T>* Subdivide();

    // Make sure the edge has its opposite face
    void GuaranteeNeighbor();
    
    // True if the edge has a subdivided child vertex
    bool HasChild() const { return vchild!=-1; }

    // Remove the reference to subdivided vertex
    void RemoveChild() { vchild = -1; }

    // Sharpness constants
    enum Mask {
        k_Smooth = 0,
        k_Sharp = 1,
        k_InfinitelySharp = 10
    };

#ifdef HBRSTITCH
    StitchEdge* GetStitchEdge(int i) {
        StitchEdge **stitchEdge = getStitchEdges();
        // If the stitch edge exists, the ownership is transferred to
        // the caller. Make sure the opposite edge loses ownership as
        // well.
        if (stitchEdge[i]) {
            if (opposite) {
                opposite->getStitchEdges()[i] = 0;
            }
            return StitchGetEdge(&stitchEdge[i]);
        }
        // If the stitch edge does not exist then we create one now.
        // Make sure the opposite edge gets a copy of it too
        else {
            StitchGetEdge(&stitchEdge[i]);
            if (opposite) {
                opposite->getStitchEdges()[i] = stitchEdge[i];
            }
            return stitchEdge[i];
        }
    }

    // If stitch edge exists, and this edge has no opposite, destroy
    // it
    void DestroyStitchEdges(int stitchcount) {
        if (!opposite) {
            StitchEdge **stitchEdge = getStitchEdges();
            for (int i = 0; i < stitchcount; ++i) {
                if (stitchEdge[i]) {
                    StitchFreeEdge(stitchEdge[i]);
                    stitchEdge[i] = 0;
                }
            }
        }
    }

    StitchEdge* GetRayStitchEdge(int i) {
        return GetStitchEdge(i + 2);
    }

    // Splits our split edge between our children. We'd better have
    // subdivided this edge by this point
    void SplitStitchEdge(int i) {
        StitchEdge* se = GetStitchEdge(i);
        HbrHalfedge<T>* ea = GetOrgVertex()->Subdivide()->GetEdge(Subdivide());
        HbrHalfedge<T>* eb = Subdivide()->GetEdge(GetDestVertex()->Subdivide());
        StitchEdge **ease = ea->getStitchEdges();
        StitchEdge **ebse = eb->getStitchEdges();
        if (i >= 2) { // ray tracing stitches
            if (!raystitchccw) {
                StitchSplitEdge(se, &ease[i], &ebse[i], false, 0, 0, 0);
            } else {
                StitchSplitEdge(se, &ebse[i], &ease[i], true, 0, 0, 0);
            }
            ea->raystitchccw = eb->raystitchccw = raystitchccw;
            if (eb->opposite) {
                eb->opposite->getStitchEdges()[i] = ebse[i];
                eb->opposite->raystitchccw = raystitchccw;
            }
            if (ea->opposite) {
                ea->opposite->getStitchEdges()[i] = ease[i];
                ea->opposite->raystitchccw = raystitchccw;
            }
        } else {
            if (!stitchccw) {
                StitchSplitEdge(se, &ease[i], &ebse[i], false, 0, 0, 0);
            } else {
                StitchSplitEdge(se, &ebse[i], &ease[i], true, 0, 0, 0);
            }
            ea->stitchccw = eb->stitchccw = stitchccw;
            if (eb->opposite) {
                eb->opposite->getStitchEdges()[i] = ebse[i];
                eb->opposite->stitchccw = stitchccw;
            }
            if (ea->opposite) {
                ea->opposite->getStitchEdges()[i] = ease[i];
                ea->opposite->stitchccw = stitchccw;
            }
        }
    }

    void SplitRayStitchEdge(int i) {
        SplitStitchEdge(i + 2);
    }

    void SetStitchEdge(int i, StitchEdge* edge) {
        StitchEdge **stitchEdges = getStitchEdges();
        stitchEdges[i] = edge;
        if (opposite) {
            opposite->getStitchEdges()[i] = edge;
        }
    }

    void SetRayStitchEdge(int i, StitchEdge* edge) {
        StitchEdge **stitchEdges = getStitchEdges();
        stitchEdges[i+2] = edge;
        if (opposite) {
            opposite->getStitchEdges()[i+2] = edge;
        }
    }

    void* GetStitchData() const {
        if (stitchdatavalid) return GetMesh()->GetStitchData(this);
        else return 0;
    }

    void SetStitchData(void* data) {
        GetMesh()->SetStitchData(this, data);
        stitchdatavalid = data ? 1 : 0;
        if (opposite) {
            opposite->GetMesh()->SetStitchData(opposite, data);
            opposite->stitchdatavalid = stitchdatavalid;
        }
    }

    bool GetStitchCCW(bool raytraced) const { return raytraced ? raystitchccw : stitchccw; }

    void ClearStitchCCW(bool raytraced) {
        if (raytraced) {
            raystitchccw = 0;
            if (opposite) opposite->raystitchccw = 0;
        } else {
            stitchccw = 0;
            if (opposite) opposite->stitchccw = 0;
        }
    }

    void ToggleStitchCCW(bool raytraced) {
        if (raytraced) {
            raystitchccw = 1 - raystitchccw;
            if (opposite) opposite->raystitchccw = raystitchccw;
        } else {
            stitchccw = 1 - stitchccw;
            if (opposite) opposite->stitchccw = stitchccw;
        }
    }

#endif

    // Marks the edge as being "coarse" (belonging to the control
    // mesh). Generally this distinction only needs to be made if
    // we're worried about interpolateboundary behaviour
    void SetCoarse(bool c) { coarse = c; }
    bool IsCoarse() const { return coarse; }

    friend class HbrFace<T>;

private:
    HbrHalfedge<T>* opposite;
    // Index of incident vertex
    int incidentVertex;

    // Index of subdivided vertex child
    int vchild;
    float sharpness;

#ifdef HBRSTITCH
    unsigned short stitchccw:1;
    unsigned short raystitchccw:1;
    unsigned short stitchdatavalid:1;
#endif
    unsigned short coarse:1;
    unsigned short lastedge:1;
    unsigned short firstedge:1;

    // If m_index = 0, 1, 2 or 3: we are the m_index edge of an
    // incident face with 3 or 4 vertices.
    // If m_index = 4: our incident face has more than 4 vertices, and
    // we must do some extra math to determine what our actual index
    // is. See getIndex()
    unsigned short m_index:3;

    // Returns the index of the edge relative to its incident face.
    // This relies on knowledge of the face's edge allocation pattern
    int getIndex() const {
        if (m_index < 4) {
            return m_index;
        } else {
            // We allocate room for up to 4 values (to handle tri or
            // quad) in the edges array.  If there are more than that,
            // they _all_ go in the faces' extraedges array.
            HbrFace<T>* incidentFace = *(HbrFace<T>**)((char *) this + sizeof(HbrHalfedge<T>));
            return int(((char *) this - incidentFace->extraedges) /
                (sizeof(HbrHalfedge<T>) + sizeof(HbrFace<T>*)));
        }
    }

    // Returns bitmask indicating whether a given facevarying datum
    // for the edge is infinitely sharp. Each datum has two bits, and
    // if those two bits are set to 3, it means the status has not
    // been computed yet.
    unsigned int *getFVarInfSharp() {
        unsigned int *fvarbits = GetFace()->fvarbits;
        if (fvarbits) {
            int fvarbitsSizePerEdge = ((GetMesh()->GetFVarCount() + 15) / 16);
            return fvarbits + getIndex() * fvarbitsSizePerEdge;
        } else {
            return 0;
        }
    }

#ifdef HBRSTITCH
    StitchEdge **getStitchEdges() {
        return GetFace()->stitchEdges + GetMesh()->GetStitchCount() * getIndex();
    }
#endif

#ifdef HBR_ADAPTIVE
public:
    struct adaptiveFlags {
        unsigned isTransition:1;
        unsigned isTriangleHead:1;
        unsigned isWatertightCritical:1;
        
        adaptiveFlags() : isTransition(0),isTriangleHead(0),isWatertightCritical(0) { }
    };
    
    adaptiveFlags _adaptiveFlags;
    
    bool IsInsideHole() const {

        HbrFace<T> * left = GetLeftFace();       
        if (left and (not left->IsHole()))
            return false;
        
        HbrFace<T> * right = GetRightFace();
        if (right and (not right->IsHole()))
            return false;
                        
        return true;
    }
    
    bool IsTransition() const { return _adaptiveFlags.isTransition; }

    bool IsTriangleHead() const { return _adaptiveFlags.isTriangleHead; }

    bool IsWatertightCritical() const { return _adaptiveFlags.isWatertightCritical; }
#endif
};

template <class T>
void
HbrHalfedge<T>::Initialize(HbrHalfedge<T>* opposite, int index, HbrVertex<T>* origin,
    unsigned int *fvarbits, HbrFace<T>* face) {
    HbrMesh<T> *mesh = face->GetMesh();
    if (face->GetNumVertices() <= 4) {
        m_index = index;
    } else {
        m_index = 4;
        // Assumes upstream allocation ensured we have extra storage
        // for pointer to face after the halfedge data structure
        // itself
        *(HbrFace<T>**)((char *) this + sizeof(HbrHalfedge<T>)) = face;
    }
    
    this->opposite = opposite;
    incidentVertex = origin->GetID();
    lastedge = (index == face->GetNumVertices() - 1);
    firstedge = (index == 0);
    if (opposite) {
        sharpness = opposite->sharpness;
#ifdef HBRSTITCH
        StitchEdge **stitchEdges = face->stitchEdges +
            mesh->GetStitchCount() * index;
        for (int i = 0; i < mesh->GetStitchCount(); ++i) {
            stitchEdges[i] = opposite->getStitchEdges()[i];
        }
        stitchccw = opposite->stitchccw;
        raystitchccw = opposite->raystitchccw;
        stitchdatavalid = 0;
        if (stitchEdges && opposite->GetStitchData()) {
            mesh->SetStitchData(this, opposite->GetStitchData());
            stitchdatavalid = 1;
        }
#endif
        if (fvarbits) {
            const int fvarcount = mesh->GetFVarCount();
            int fvarbitsSizePerEdge = ((fvarcount + 15) / 16);
            memcpy(fvarbits, opposite->getFVarInfSharp(), fvarbitsSizePerEdge * sizeof(unsigned int));
        }
    } else {
        sharpness = 0.0f;
#ifdef HBRSTITCH
        StitchEdge **stitchEdges = getStitchEdges();
        for (int i = 0; i < mesh->GetStitchCount(); ++i) {
            stitchEdges[i] = 0;
        }
        stitchccw = 1;
        raystitchccw = 1;
        stitchdatavalid = 0;
#endif
        if (fvarbits) {
            const int fvarcount = mesh->GetFVarCount();
            int fvarbitsSizePerEdge = ((fvarcount + 15) / 16);
            memset(fvarbits, 0xff, fvarbitsSizePerEdge * sizeof(unsigned int));
        }
    }
}

template <class T>
HbrHalfedge<T>::~HbrHalfedge() {
    Clear();
}

template <class T>
void
HbrHalfedge<T>::Clear() {
    if (opposite) {
        opposite->opposite = 0;
        if (vchild != -1) {
            // Transfer ownership of the vchild to the opposite ptr
            opposite->vchild = vchild;

            HbrVertex<T> *vchildVert = GetMesh()->GetVertex(vchild);
            // Done this way just for assertion sanity
            vchildVert->SetParent(static_cast<HbrHalfedge*>(0));
            vchildVert->SetParent(opposite);
            vchild = -1;
        }
        opposite = 0;
    }
    // Orphan the child vertex
    else if (vchild != -1) {
        HbrVertex<T> *vchildVert = GetMesh()->GetVertex(vchild);        
        vchildVert->SetParent(static_cast<HbrHalfedge*>(0));
        vchild = -1;
    }
}

template <class T>
HbrVertex<T>*
HbrHalfedge<T>::Subdivide() {
    HbrMesh<T>* mesh = GetMesh();
    if (vchild != -1) return mesh->GetVertex(vchild);
    // Make sure that our opposite doesn't "own" a subdivided vertex
    // already. If it does, use that
    if (opposite && opposite->vchild != -1) return mesh->GetVertex(opposite->vchild);
    HbrVertex<T>* vchildVert = mesh->GetSubdivision()->Subdivide(mesh, this);
    vchild = vchildVert->GetID();
    vchildVert->SetParent(this);
    return vchildVert;
}

template <class T>
void
HbrHalfedge<T>::GuaranteeNeighbor() {
    HbrMesh<T>* mesh = GetMesh();
    mesh->GetSubdivision()->GuaranteeNeighbor(mesh, this);
}

// Determines whether an edge is infinitely sharp as far as its
// facevarying data is concerned. Happens if the faces on both sides
// disagree on the facevarying data at either of the shared vertices
// on the edge.
template <class T>
bool
HbrHalfedge<T>::GetFVarInfiniteSharp(int datum) {

    // Check to see if already initialized
    int intindex = datum >> 4;
    int shift = (datum & 15) << 1;
    unsigned int *fvarinfsharp = getFVarInfSharp();
    unsigned int bits = (fvarinfsharp[intindex] >> shift) & 0x3;
    if (bits != 3) {
        assert (bits != 2);
        return bits ? true : false;
    }

    // If there is no face varying data it can't be infinitely sharp!
    const int fvarwidth = GetMesh()->GetTotalFVarWidth();
    if (!fvarwidth) {
        bits = ~(0x3 << shift);
        fvarinfsharp[intindex] &= bits;
        if (opposite) opposite->getFVarInfSharp()[intindex] &= bits;
        return false;
    }

    // If either incident face is missing, it's a geometric boundary
    // edge, and also a facevarying boundary edge
    HbrFace<T>* left = GetLeftFace(), *right = GetRightFace();
    if (!left || !right) {
        bits = ~(0x2 << shift);
        fvarinfsharp[intindex] &= bits;
        if (opposite) opposite->getFVarInfSharp()[intindex] &= bits;
        return true;
    }

    // Look for the indices on each face which correspond to the
    // origin and destination vertices of the edge
    int lorg = -1, ldst = -1, rorg = -1, rdst = -1, i, nv;
    HbrHalfedge<T>* e;
    e = left->GetFirstEdge();
    nv = left->GetNumVertices();
    for (i = 0; i < nv; ++i) {
        if (e->GetOrgVertex() == GetOrgVertex()) lorg = i;
        if (e->GetOrgVertex() == GetDestVertex()) ldst = i;
        e = e->GetNext();
    }
    e = right->GetFirstEdge();
    nv = right->GetNumVertices();
    for (i = 0; i < nv; ++i) {
        if (e->GetOrgVertex() == GetOrgVertex()) rorg = i;
        if (e->GetOrgVertex() == GetDestVertex()) rdst = i;
        e = e->GetNext();
    }
    assert(lorg >= 0 && ldst >= 0 && rorg >= 0 && rdst >= 0);
    // Compare the facevarying data to some tolerance
    const int startindex = GetMesh()->GetFVarIndices()[datum];
    const int width = GetMesh()->GetFVarWidths()[datum];
    if (!right->GetFVarData(rorg).Compare(left->GetFVarData(lorg), startindex, width, 0.001f) ||
        !right->GetFVarData(rdst).Compare(left->GetFVarData(ldst), startindex, width, 0.001f)) {
        bits = ~(0x2 << shift);
        fvarinfsharp[intindex] &= bits;
        if (opposite) opposite->getFVarInfSharp()[intindex] &= bits;
        return true;
    }

    bits = ~(0x3 << shift);
    fvarinfsharp[intindex] &= bits;
    if (opposite) opposite->getFVarInfSharp()[intindex] &= bits;
    return false;
}

template <class T>
bool
HbrHalfedge<T>::IsFVarInfiniteSharpAnywhere() {

    if (sharpness > k_Smooth) {
        return true;
    }

    for (int i = 0; i < GetMesh()->GetFVarCount(); ++i) {
        if (GetFVarInfiniteSharp(i)) return true;
    }
    return false;
}

template <class T>
float
HbrHalfedge<T>::GetFVarSharpness(int datum, bool ignoreGeometry) {

    if (GetFVarInfiniteSharp(datum)) return k_InfinitelySharp;

    if (!ignoreGeometry) {
        // If it's a geometrically sharp edge it's going to be a
        // facevarying sharp edge too
        if (sharpness > k_Smooth) {
            SetFVarInfiniteSharp(datum, true);
            return k_InfinitelySharp;
        }
    }
    return k_Smooth;
}


template <class T>
std::ostream&
operator<<(std::ostream& out, const HbrHalfedge<T>& edge) {
    if (edge.IsBoundary()) out << "boundary ";
    out << "edge connecting ";
    if (edge.GetOrgVertex())
        out << *edge.GetOrgVertex();
    else
        out << "(none)";
    out << " to ";
    if (edge.GetDestVertex()) {
        out << *edge.GetDestVertex();
    } else {
        out << "(none)";
    }
    return out;
}

// Sorts half edges by the relative ordering of the incident faces'
// paths.
template <class T>
class HbrHalfedgeCompare {
public:
    bool operator() (const HbrHalfedge<T>* a, HbrHalfedge<T>* b) const {
        return (a->GetFace()->GetPath() < b->GetFace()->GetPath());
    }
};

template <class T>
class HbrHalfedgeOperator {
public:
    virtual void operator() (HbrHalfedge<T> &edge) = 0;
    virtual ~HbrHalfedgeOperator() {}
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRHALFEDGE_H */
