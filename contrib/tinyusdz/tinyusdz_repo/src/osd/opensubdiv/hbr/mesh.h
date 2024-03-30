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

#ifndef OPENSUBDIV3_HBRMESH_H
#define OPENSUBDIV3_HBRMESH_H

#ifdef PRMAN
#include "libtarget/TgMalloc.h" // only for alloca
#include "libtarget/TgThread.h"
#ifdef HBRSTITCH
#include "libtarget/TgHashMap.h"
#endif
#endif

#include <algorithm>
#include <cstring>
#include <iterator>
#include <vector>
#include <set>
#include <iostream>

#include "../hbr/vertex.h"
#include "../hbr/face.h"
#include "../hbr/hierarchicalEdit.h"
#include "../hbr/vertexEdit.h"
#include "../hbr/creaseEdit.h"
#include "../hbr/allocator.h"

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T> class HbrSubdivision;
template <class T> class HbrHalfedge;

template <class T> class HbrMesh {
public:
    HbrMesh(HbrSubdivision<T>* subdivision = 0, int fvarcount = 0, const int *fvarindices = 0, const int *fvarwidths = 0, int totalfvarwidth = 0
#ifdef HBRSTITCH
    , int stitchCount = 0
#endif
    );
    ~HbrMesh();

    // Create vertex with the indicated ID and data
    HbrVertex<T>* NewVertex(int id, const T &data);

    // Create vertex with the indicated data. The ID will be assigned
    // by the mesh.
    HbrVertex<T>* NewVertex(const T &data);

    // Create vertex without an ID - one will be assigned by the mesh,
    // and the data implicitly created will share the same id
    HbrVertex<T>* NewVertex();

    // Ask for vertex with the indicated ID
    HbrVertex<T>* GetVertex(int id) const {
        if (id >= nvertices) {
            return 0;
        } else {
            return vertices[id];
        }
    }        

    // Ask for client data associated with the vertex with the indicated ID
    void* GetVertexClientData(int id) const {
        if (id >= vertexClientData.size()) {        
            return 0;
        } else {
            return vertexClientData[id];
        }
    }

    // Set client data associated with the vertex with the indicated ID
    void SetVertexClientData(int id, void *data) {
        if (id >= vertexClientData.size()) {
            size_t oldsize = vertexClientData.size();
            vertexClientData.resize(nvertices);
            if (s_memStatsIncrement) {
                s_memStatsIncrement((vertexClientData.size() - oldsize) * sizeof(void*));
            }
        }
        vertexClientData[id] = data;
    }

    // Create face from a list of vertex IDs
    HbrFace<T>* NewFace(int nvertices, const int *vtx, int uindex);

    // Create face from a list of vertices
    HbrFace<T>* NewFace(int nvertices, HbrVertex<T>** vtx, HbrFace<T>* parent, int childindex);

    // "Create" a new uniform index
    int NewUniformIndex() { return ++maxUniformIndex; }

    // Finishes initialization of the mesh
    void Finish();

    // Remove the indicated face from the mesh
    void DeleteFace(HbrFace<T>* face);

    // Remove the indicated vertex from the mesh
    void DeleteVertex(HbrVertex<T>* vertex);

    // Returns number of vertices in the mesh
    int GetNumVertices() const;

    // Returns number of disconnected vertices in the mesh
    int GetNumDisconnectedVertices() const;

    // Returns number of faces in the mesh
    int GetNumFaces() const;

    // Returns number of coarse faces in the mesh
    int GetNumCoarseFaces() const;

    // Ask for face with the indicated ID
    HbrFace<T>* GetFace(int id) const;

    // Ask for client data associated with the face with the indicated ID
    void* GetFaceClientData(int id) const {
        if (id >= faceClientData.size()) {        
            return 0;
        } else {
            return faceClientData[id];
        }
    }

    // Set client data associated with the face with the indicated ID
    void SetFaceClientData(int id, void *data) {
        if (id >= faceClientData.size()) {
            size_t oldsize = faceClientData.size();
            faceClientData.resize(nfaces);
            if (s_memStatsIncrement) {
                s_memStatsIncrement((faceClientData.size() - oldsize) * sizeof(void*));
            }
        }
        faceClientData[id] = data;
    }
    
    // Returns a collection of all vertices in the mesh. This function
    // requires an output iterator; to get the vertices into a
    // std::vector, use GetVertices(std::back_inserter(myvector))
    template <typename OutputIterator>
    void GetVertices(OutputIterator vertices) const;

    // Applies operator to all vertices
    void ApplyOperatorAllVertices(HbrVertexOperator<T> &op) const;

    // Returns a collection of all faces in the mesh.  This function
    // requires an output iterator; to get the faces into a
    // std::vector, use GetFaces(std::back_inserter(myvector))
    template <typename OutputIterator>
    void GetFaces(OutputIterator faces) const;

    // Returns the subdivision method
    HbrSubdivision<T>* GetSubdivision() const { return subdivision; }

    // Return the number of facevarying variables
    int GetFVarCount() const { return fvarcount; }

    // Return a table of the start index of each facevarying variable
    const int *GetFVarIndices() const { return fvarindices; }

    // Return a table of the size of each facevarying variable
    const int *GetFVarWidths() const { return fvarwidths; }

    // Return the sum size of facevarying variables per vertex
    int GetTotalFVarWidth() const { return totalfvarwidth; }

#ifdef HBRSTITCH
    int GetStitchCount() const { return stitchCount; }
#endif

    void PrintStats(std::ostream& out);

    // Returns memory statistics
    size_t GetMemStats() const { return m_memory; }

    // Interpolate boundary management
    enum InterpolateBoundaryMethod {
        k_InterpolateBoundaryNone,
        k_InterpolateBoundaryEdgeOnly,
        k_InterpolateBoundaryEdgeAndCorner,
        k_InterpolateBoundaryAlwaysSharp
    };

    InterpolateBoundaryMethod GetInterpolateBoundaryMethod() const { return interpboundarymethod; }
    void SetInterpolateBoundaryMethod(InterpolateBoundaryMethod method) { interpboundarymethod = method; }
    InterpolateBoundaryMethod GetFVarInterpolateBoundaryMethod() const { return fvarinterpboundarymethod; }
    void SetFVarInterpolateBoundaryMethod(InterpolateBoundaryMethod method) { fvarinterpboundarymethod = method; }

    bool GetFVarPropagateCorners() const { return fvarpropagatecorners; }
    void SetFVarPropagateCorners(bool p) { fvarpropagatecorners = p; }

    // Register routines for keeping track of memory usage
    void RegisterMemoryRoutines(void (*increment)(unsigned long bytes), void (*decrement)(unsigned long bytes)) {
        m_faceAllocator.SetMemStatsIncrement(increment);
        m_faceAllocator.SetMemStatsDecrement(decrement);
        m_vertexAllocator.SetMemStatsIncrement(increment);
        m_vertexAllocator.SetMemStatsDecrement(decrement);
        s_memStatsIncrement = increment;
        s_memStatsDecrement = decrement;
    }

    // Add a vertex to consider for garbage collection. All
    // neighboring faces of that vertex will be examined to see if
    // they can be deleted
    void AddGarbageCollectableVertex(HbrVertex<T>* vertex) {
        if (!m_transientMode) {
            assert(vertex);
            if (!vertex->IsCollected()) {
                gcVertices.push_back(vertex); vertex->SetCollected();
            }
        }
    }

    // Apply garbage collection to the mesh
    void GarbageCollect();

    // Add a new hierarchical edit to the mesh
    void AddHierarchicalEdit(HbrHierarchicalEdit<T>* edit);

    // Return the hierarchical edits associated with the mesh
    const std::vector<HbrHierarchicalEdit<T>*> &GetHierarchicalEdits() const {
        return hierarchicalEdits;
    }

    // Return the hierarchical edits associated with the mesh at an
    // offset
    HbrHierarchicalEdit<T>** GetHierarchicalEditsAtOffset(int offset) {
        return &hierarchicalEdits[offset];
    }

    // Whether the mesh has certain types of edits
    bool HasVertexEdits() const { return hasVertexEdits; }
    bool HasCreaseEdits() const { return hasCreaseEdits; }

    void Unrefine(int numCoarseVerts, int numCoarseFaces) {

        for (int i = numCoarseFaces; i < maxFaceID; ++i) {
            HbrFace<T>* f = GetFace(i);
                if(f and not f->IsCoarse())
                    DeleteFace(f);
        }
        
        maxFaceID = numCoarseFaces;

        for(int i=numCoarseVerts; i<(int)vertices.size(); ++i ) {
            HbrVertex<T>* v = GetVertex(i);
            if(v and not v->IsReferenced())
                DeleteVertex(v);
        }
    }

    // When mode is true, the mesh is put in a "transient" mode,
    // i.e. all subsequent intermediate vertices/faces that are
    // created by subdivision are deemed temporary. This transient
    // data can be entirely freed by a subsequent call to
    // FreeTransientData(). Essentially, the mesh is checkpointed and
    // restored. This is useful when space is at a premium and
    // subdivided results are cached elsewhere. On the other hand,
    // repeatedly putting the mesh in and out of transient mode and
    // performing the same evaluations comes at a significant compute
    // cost.
    void SetTransientMode(bool mode) {
        m_transientMode = mode;
    }

    // Frees transient subdivision data; returns the mesh to a
    // checkpointed state prior to a call to SetTransientMode.
    void FreeTransientData();

    // Create new face children block for use by HbrFace
    HbrFaceChildren<T>* NewFaceChildren() {
        return m_faceChildrenAllocator.Allocate();
    }

    // Recycle face children block used by HbrFace
    void DeleteFaceChildren(HbrFaceChildren<T>* facechildren) {
        m_faceChildrenAllocator.Deallocate(facechildren);
    }

#ifdef HBRSTITCH
    void * GetStitchData(const HbrHalfedge<T>* edge) const {
        typename TgHashMap<const HbrHalfedge<T>*, void *>::const_iterator i =
            stitchData.find(edge);
        if (i != stitchData.end()) {
            return i->second;
        } else {
            return NULL;
        }
    }

    void SetStitchData(const HbrHalfedge<T>* edge, void *data) {
        stitchData[edge] = data;
    }
#endif

private:

    // Subdivision method used in this mesh
    HbrSubdivision<T>* subdivision;

    // Number of facevarying datums
    int fvarcount;

    // Start indices of the facevarying data we want to store
    const int *fvarindices;

    // Individual widths of the facevarying data we want to store
    const int *fvarwidths;

    // Total widths of the facevarying data
    const int totalfvarwidth;

#ifdef HBRSTITCH
    // Number of stitch edges per halfedge 
    const int stitchCount;

    // Client (sparse) data used on some halfedges
    TgHashMap<const HbrHalfedge<T>*, void *> stitchData;
#endif

    // Vertices which comprise this mesh
    std::vector<HbrVertex<T> *> vertices;
    int nvertices;

    // Client data associated with each face
    std::vector<void *> vertexClientData;

    // Faces which comprise this mesh
    std::vector<HbrFace<T> *> faces;
    int nfaces;

    // Client data associated with each face
    std::vector<void *> faceClientData;

    // Maximum vertex ID - may be needed when generating a unique
    // vertex ID
    int maxVertexID;

    // Maximum face ID - needed when generating a unique face ID
    int maxFaceID;

    // Maximum uniform index - needed to generate a new uniform index
    int maxUniformIndex;

    // Boundary interpolation method
    InterpolateBoundaryMethod interpboundarymethod;

    // Facevarying boundary interpolation method
    InterpolateBoundaryMethod fvarinterpboundarymethod;

    // Whether facevarying corners propagate their sharpness
    bool fvarpropagatecorners;

    // Memory statistics tracking routines
    HbrMemStatFunction s_memStatsIncrement;
    HbrMemStatFunction s_memStatsDecrement;

    // Vertices which may be garbage collected
    std::vector<HbrVertex<T>*> gcVertices;

    // List of vertex IDs which may be recycled
    std::set<int> recycleIDs;

    // Hierarchical edits. This vector is left unsorted until Finish()
    // is called, at which point it is sorted. After that point,
    // HbrFaces have pointers directly into this array so manipulation
    // of it should be avoided.
    std::vector<HbrHierarchicalEdit<T>*> hierarchicalEdits;

    // Size of faces (including 4 facevarying bits and stitch edges)
    const size_t m_faceSize;
    HbrAllocator<HbrFace<T> > m_faceAllocator;

    // Size of vertices (includes storage for one piece of facevarying data)
    const size_t m_vertexSize;
    HbrAllocator<HbrVertex<T> > m_vertexAllocator;

    // Allocator for face children blocks used by HbrFace
    HbrAllocator<HbrFaceChildren<T> > m_faceChildrenAllocator;
    
    // Memory used by this mesh alone, plus all its faces and vertices
    size_t m_memory;

    // Number of coarse faces. Initialized at Finish()
    int m_numCoarseFaces;

    // Flags which indicate whether the mesh has certain types of
    // edits
    unsigned hasVertexEdits:1;
    unsigned hasCreaseEdits:1;

    // True if the mesh is in "transient" mode, meaning all
    // vertices/faces that are created via NewVertex/NewFace should be
    // deemed temporary
    bool m_transientMode;

    // Vertices which are transient
    std::vector<HbrVertex<T>*> m_transientVertices;

    // Faces which are transient
    std::vector<HbrFace<T>*> m_transientFaces;

#ifdef HBR_ADAPTIVE
public:
    std::vector<std::pair<int, int> > const & GetSplitVertices() const {
        return m_splitVertices;
    }

protected:
    friend class HbrVertex<T>;
    
    void addSplitVertex(int splitIdx, int orgIdx) {
        m_splitVertices.push_back(std::pair<int,int>(splitIdx, orgIdx));
    }
    
private:
    std::vector<std::pair<int, int> >  m_splitVertices;
#endif
};

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#include <algorithm>
#include "../hbr/mesh.h"
#include "../hbr/halfedge.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

template <class T>
HbrMesh<T>::HbrMesh(HbrSubdivision<T>* s, int _fvarcount, const int *_fvarindices, const int *_fvarwidths, int _totalfvarwidth
#ifdef HBRSTITCH
    , int _stitchCount
#endif
    )
    : subdivision(s), fvarcount(_fvarcount), fvarindices(_fvarindices),
      fvarwidths(_fvarwidths), totalfvarwidth(_totalfvarwidth),
#ifdef HBRSTITCH
      stitchCount(_stitchCount),
#endif
      nvertices(0), nfaces(0), maxVertexID(0), maxFaceID(0), maxUniformIndex(0),
      interpboundarymethod(k_InterpolateBoundaryNone),
      fvarinterpboundarymethod(k_InterpolateBoundaryNone),
      fvarpropagatecorners(false),
      s_memStatsIncrement(0), s_memStatsDecrement(0),
      m_faceSize(sizeof(HbrFace<T>) + 4 *
                 ((fvarcount + 15) / 16 * sizeof(unsigned int)
#ifdef HBRSTITCH
                 + stitchCount * sizeof(StitchEdge*)
#endif
                  )),
      m_faceAllocator(&m_memory, 512, 0, 0, m_faceSize),
      m_vertexSize(sizeof(HbrVertex<T>) +
          (totalfvarwidth ? (sizeof(HbrFVarData<T>) + (totalfvarwidth - 1) * sizeof(float)) : 0)),
      m_vertexAllocator(&m_memory, 512, 0, 0, m_vertexSize),
      m_faceChildrenAllocator(&m_memory, 512, 0, 0),
      m_memory(0),
      m_numCoarseFaces(-1),
      hasVertexEdits(0),
      hasCreaseEdits(0),
      m_transientMode(false) {
}

template <class T>
HbrMesh<T>::~HbrMesh() {
    GarbageCollect();

    int i;
    if (!faces.empty()) {
        for (i = 0; i < nfaces; ++i) {
            if (faces[i]) {
                faces[i]->Destroy();
                m_faceAllocator.Deallocate(faces[i]);
            }
        }
        if (s_memStatsDecrement) {
            s_memStatsDecrement(faces.size() * sizeof(HbrFace<T>*));
        }
    }
    if (!vertices.empty()) {
        for (i = 0; i < nvertices; ++i) {
            if (vertices[i]) {
                vertices[i]->Destroy(this);
                m_vertexAllocator.Deallocate(vertices[i]);
            }
        }
        if (s_memStatsDecrement) {
            s_memStatsDecrement(vertices.size() * sizeof(HbrVertex<T>*));
        }
    }
    if (!vertexClientData.empty() && s_memStatsDecrement) {
        s_memStatsDecrement(vertexClientData.size() * sizeof(void*));
    }
    if (!faceClientData.empty() && s_memStatsDecrement) {
        s_memStatsDecrement(faceClientData.size() * sizeof(void*));
    }
    for (typename std::vector<HbrHierarchicalEdit<T>* >::iterator hi =
             hierarchicalEdits.begin(); hi != hierarchicalEdits.end(); ++hi) {
        delete *hi;
    }
}

template <class T>
HbrVertex<T>*
HbrMesh<T>::NewVertex(int id, const T &data) {
    HbrVertex<T>* v = 0;
    if (nvertices <= id) {
        while (nvertices <= maxVertexID) {
            nvertices *= 2;
            if (nvertices < 1) nvertices = 1;
        }
        size_t oldsize = vertices.size();
        vertices.resize(nvertices);
        if (s_memStatsIncrement) {
            s_memStatsIncrement((vertices.size() - oldsize) * sizeof(HbrVertex<T>*));
        }
    }
    v = vertices[id];
    if (v) {
        v->Destroy(this);
    } else {
        v = m_vertexAllocator.Allocate();
    }
    v->Initialize(id, data, GetTotalFVarWidth());
    vertices[id] = v;

    if (id >= maxVertexID) {
        maxVertexID = id + 1;
    }

    // Newly created vertices are always candidates for garbage
    // collection, until they get "owned" by someone who
    // IncrementsUsage on the vertex.
    AddGarbageCollectableVertex(v);

    // If mesh is in transient mode, add vertex to transient list
    if (m_transientMode) {
        m_transientVertices.push_back(v);
    }
    return v;
}

template <class T>
HbrVertex<T>*
HbrMesh<T>::NewVertex(const T &data) {
    // Pick an ID - either the maximum vertex ID or a recycled ID if
    // we can
    int id = maxVertexID;
    if (!recycleIDs.empty()) {
        id = *recycleIDs.begin();
        recycleIDs.erase(recycleIDs.begin());
    }
    if (id >= maxVertexID) {
        maxVertexID = id + 1;
    }
    return NewVertex(id, data);
}

template <class T>
HbrVertex<T>*
HbrMesh<T>::NewVertex() {
    // Pick an ID - either the maximum vertex ID or a recycled ID if
    // we can
    int id = maxVertexID;
    if (!recycleIDs.empty()) {
        id = *recycleIDs.begin();
        recycleIDs.erase(recycleIDs.begin());
    }
    if (id >= maxVertexID) {
        maxVertexID = id + 1;
    }
    T data(id);
    data.Clear();
    return NewVertex(id, data);
}

template <class T>
HbrFace<T>*
HbrMesh<T>::NewFace(int nv, const int *vtx, int uindex) {
    HbrVertex<T>** facevertices = reinterpret_cast<HbrVertex<T>**>(alloca(sizeof(HbrVertex<T>*) * nv));
    int i;
    for (i = 0; i < nv; ++i) {
        facevertices[i] = GetVertex(vtx[i]);
        if (!facevertices[i]) {
            return 0;
        }
    }
    HbrFace<T> *f = 0;
    // Resize if needed
    if (nfaces <= maxFaceID) {
        while (nfaces <= maxFaceID) {
            nfaces *= 2;
            if (nfaces < 1) nfaces = 1;
        }
        size_t oldsize = faces.size();
        faces.resize(nfaces);
        if (s_memStatsIncrement) {
            s_memStatsIncrement((faces.size() - oldsize) * sizeof(HbrVertex<T>*));
        }
    }
    f = faces[maxFaceID];
    if (f) {
        f->Destroy();
    } else {
        f = m_faceAllocator.Allocate();
    }
    f->Initialize(this, NULL, -1, maxFaceID, uindex, nv, facevertices, totalfvarwidth, 0);
    faces[maxFaceID] = f;
    maxFaceID++;
    // Update the maximum encountered uniform index
    if (uindex > maxUniformIndex) maxUniformIndex = uindex;

    // If mesh is in transient mode, add face to transient list
    if (m_transientMode) {
        m_transientFaces.push_back(f);
    }
    return f;
}

template <class T>
HbrFace<T>*
HbrMesh<T>::NewFace(int nv, HbrVertex<T> **vtx, HbrFace<T>* parent, int childindex) {
    HbrFace<T> *f = 0;
    // Resize if needed
    if (nfaces <= maxFaceID) {
        while (nfaces <= maxFaceID) {
            nfaces *= 2;
            if (nfaces < 1) nfaces = 1;
        }
        size_t oldsize = faces.size();        
        faces.resize(nfaces);
        if (s_memStatsIncrement) {
            s_memStatsIncrement((faces.size() - oldsize) * sizeof(HbrVertex<T>*));
        }
    }
    f = faces[maxFaceID];
    if (f) {
        f->Destroy();
    } else {
        f = m_faceAllocator.Allocate();
    }
    f->Initialize(this, parent, childindex, maxFaceID, parent ? parent->GetUniformIndex() : 0, nv, vtx, totalfvarwidth, parent ? parent->GetDepth() + 1 : 0);
    if (parent) {
        f->SetPtexIndex(parent->GetPtexIndex());
    }
    faces[maxFaceID] = f;
    maxFaceID++;

    // If mesh is in transient mode, add face to transient list
    if (m_transientMode) {
        m_transientFaces.push_back(f);
    }
    return f;
}

template <class T>
void
HbrMesh<T>::Finish() {
    int i, j;
    m_numCoarseFaces = 0;
    for (i = 0; i < nfaces; ++i) {
        if (faces[i]) {
            faces[i]->SetCoarse();
            m_numCoarseFaces++;
        }
    }

    std::vector<HbrVertex<T>*> vertexlist;
    GetVertices(std::back_inserter(vertexlist));
    for (typename std::vector<HbrVertex<T>*>::iterator vi = vertexlist.begin();
         vi != vertexlist.end(); ++vi) {
        HbrVertex<T>* vertex = *vi;
        if (vertex->IsConnected()) vertex->Finish();
    }
    // Finish may have added new vertices
    vertexlist.clear();
    GetVertices(std::back_inserter(vertexlist));

    // If interpolateboundary is on, process boundary edges
    if (interpboundarymethod == k_InterpolateBoundaryEdgeOnly || interpboundarymethod == k_InterpolateBoundaryEdgeAndCorner) {
        for (i = 0; i < nfaces; ++i) {
            if (HbrFace<T>* face = faces[i]) {
                int nv = face->GetNumVertices();
                for (int k = 0; k < nv; ++k) {
                    HbrHalfedge<T>* edge = face->GetEdge(k);
                    if (edge->IsBoundary()) {
                        edge->SetSharpness(HbrHalfedge<T>::k_InfinitelySharp);
                    }
                }
            }
        }
    }
    // Process corners
    if (interpboundarymethod == k_InterpolateBoundaryEdgeAndCorner) {
        for (typename std::vector<HbrVertex<T>*>::iterator vi = vertexlist.begin();
             vi != vertexlist.end(); ++vi) {
            HbrVertex<T>* vertex = *vi;
            if (vertex && vertex->IsConnected() && vertex->OnBoundary() && vertex->GetCoarseValence() == 2) {
                vertex->SetSharpness(HbrVertex<T>::k_InfinitelySharp);
            }
        }
    }

    // Sort the hierarchical edits
    if (!hierarchicalEdits.empty()) {
        HbrHierarchicalEditComparator<T> cmp;
        int nHierarchicalEdits = (int)hierarchicalEdits.size();
        std::sort(hierarchicalEdits.begin(), hierarchicalEdits.end(), cmp);
        // Push a sentinel null value - we rely upon this sentinel to
        // ensure face->GetHierarchicalEdits knows when to terminate
        hierarchicalEdits.push_back(0);
        j = 0;
        // Link faces to hierarchical edits
        for (i = 0; i < nfaces; ++i) {
            if (faces[i]) {
                while (j < nHierarchicalEdits && hierarchicalEdits[j]->GetFaceID() < i) {
                    ++j;
                }
                if (j < nHierarchicalEdits && hierarchicalEdits[j]->GetFaceID() == i) {
                    faces[i]->SetHierarchicalEdits(&hierarchicalEdits[j]);
                }
            }
        }
    }
}

template <class T>
void
HbrMesh<T>::DeleteFace(HbrFace<T>* face) {
    if (face->GetID() < nfaces) {
        HbrFace<T>* f = faces[face->GetID()];
        if (f == face) {
            faces[face->GetID()] = 0;
            face->Destroy();
            m_faceAllocator.Deallocate(face);
        }
    }
}

template <class T>
void
HbrMesh<T>::DeleteVertex(HbrVertex<T>* vertex) {
    HbrVertex<T> *v = GetVertex(vertex->GetID());
    if (v == vertex) {
        recycleIDs.insert(vertex->GetID());
        int id = vertex->GetID();
        vertices[id] = 0;
        vertex->Destroy(this);
        m_vertexAllocator.Deallocate(vertex);
    }
}

template <class T>
int
HbrMesh<T>::GetNumVertices() const {
    int count = 0;
    for (int i = 0; i < nvertices; ++i) {
        if (vertices[i]) count++;
    }
    return count;
}

template <class T>
int
HbrMesh<T>::GetNumDisconnectedVertices() const {
    int disconnected = 0;
    for (int i = 0; i < nvertices; ++i) {
        if (HbrVertex<T>* v = vertices[i]) {
                if (!v->IsConnected()) {
                    disconnected++;
                }
            }
        }
    return disconnected;
}

template <class T>
int
HbrMesh<T>::GetNumFaces() const {
    int count = 0;
    for (int i = 0; i < nfaces; ++i) {
        if (faces[i]) count++;
    }
    return count;
}

template <class T>
int
HbrMesh<T>::GetNumCoarseFaces() const {
    // Use the value computed by Finish() if it exists
    if (m_numCoarseFaces >= 0) return m_numCoarseFaces;
    // Otherwise we have to just count it up now
    int count = 0;
    for (int i = 0; i < nfaces; ++i) {
        if (faces[i] && faces[i]->IsCoarse()) count++;
    }
    return count;
}

template <class T>
HbrFace<T>*
HbrMesh<T>::GetFace(int id) const {
    if (id < nfaces) {
        return faces[id];
    }
    return 0;
}

template <class T>
template <typename OutputIterator>
void
HbrMesh<T>::GetVertices(OutputIterator lvertices) const {
    for (int i = 0; i < nvertices; ++i) {
        if (vertices[i]) *lvertices++ = vertices[i];
    }
}

template <class T>
void
HbrMesh<T>::ApplyOperatorAllVertices(HbrVertexOperator<T> &op) const {
    for (int i = 0; i < nvertices; ++i) {    
        if (vertices[i]) op(*vertices[i]);
    }
}

template <class T>
template <typename OutputIterator>
void
HbrMesh<T>::GetFaces(OutputIterator lfaces) const {
    for (int i = 0; i < nfaces; ++i) {
        if (faces[i]) *lfaces++ = faces[i];
    }
}

template <class T>
void
HbrMesh<T>::PrintStats(std::ostream &out) {
    int singular = 0;
    int sumvalence = 0;
    int i, nv = 0;
    int disconnected = 0;
    int extraordinary = 0;
    for (i = 0; i < nvertices; ++i) {
        if (HbrVertex<T>* v = vertices[i]) {
            nv++;
            if (v->IsSingular()) {
                out << "  singular: " << *v << "\n";
                singular++;
            } else if (!v->IsConnected()) {
                out << "  disconnected: " << *v << "\n";
                disconnected++;
            } else {
                if (v->IsExtraordinary()) {
                    extraordinary++;
                }
                sumvalence += v->GetValence();
            }
        }
    }
    out << "Mesh has " << nv << " vertices\n";
    out << "Total singular vertices " << singular << "\n";
    out << "Total disconnected vertices " << disconnected << "\n";
    out << "Total extraordinary vertices " << extraordinary << "\n";
    out << "Average valence " << (float) sumvalence / nv << "\n";

    int sumsides = 0;
    int numfaces = 0;
    for (i = 0; i < nfaces; ++i) {
        if (HbrFace<T>* f = faces[i]) {
            numfaces++;
            sumsides += f->GetNumVertices();
        }
    }
    out << "Mesh has " << nfaces << " faces\n";
    out << "Average sidedness " << (float) sumsides / nfaces << "\n";
}

template <class T>
void
HbrMesh<T>::GarbageCollect() {
    if (gcVertices.empty()) return;

    static const size_t gcthreshold = 4096;

    if (gcVertices.size() <= gcthreshold) return;
    // Go through the list of garbage collectable vertices and gather
    // up the neighboring faces of those vertices which can be garbage
    // collected.
    std::vector<HbrFace<T>*> killlist;
    std::vector<HbrVertex<T>*> vlist;

    // Process the vertices in the same order as they were collected
    // (gcVertices used to be declared as a std::deque, but that was
    // causing unnecessary heap traffic).
    int numprocessed = (int)gcVertices.size() - gcthreshold / 2;
    for (int i = 0; i < numprocessed; ++i) {
        HbrVertex<T>* v = gcVertices[i];
        v->ClearCollected();
        if (v->IsUsed()) continue;
        vlist.push_back(v);
        HbrHalfedge<T>* start = v->GetIncidentEdge(), *edge;
        edge = start;
        while (edge) {
            HbrFace<T>* f = edge->GetLeftFace();
            if (!f->IsCollected()) {
                f->SetCollected();
                killlist.push_back(f);
            }
            edge = v->GetNextEdge(edge);
            if (edge == start) break;
        }
    }

    gcVertices.erase(gcVertices.begin(), gcVertices.begin() + numprocessed);

    // Delete those faces
    for (typename std::vector<HbrFace<T>*>::iterator fi = killlist.begin(); fi != killlist.end(); ++fi) {
        if ((*fi)->GarbageCollectable()) {
            DeleteFace(*fi);
        } else {
            (*fi)->ClearCollected();
        }
    }

    // Delete as many vertices as we can
    for (typename std::vector<HbrVertex<T>*>::iterator vi = vlist.begin(); vi != vlist.end(); ++vi) {
        HbrVertex<T>* v = *vi;
        if (!v->IsReferenced()) {
            DeleteVertex(v);
        }
    }
}

template <class T>
void
HbrMesh<T>::AddHierarchicalEdit(HbrHierarchicalEdit<T>* edit) {
    hierarchicalEdits.push_back(edit);
    if (dynamic_cast<HbrVertexEdit<T>*>(edit) ||
        dynamic_cast<HbrMovingVertexEdit<T>*>(edit)) {
        hasVertexEdits = 1;
    } else if (dynamic_cast<HbrCreaseEdit<T>*>(edit)) {
        hasCreaseEdits = 1;
    }
}

template <class T>
void
HbrMesh<T>::FreeTransientData() {
    // When purging transient data, we must clear the faces first
    for (typename std::vector<HbrFace<T>*>::iterator fi = m_transientFaces.begin();
         fi != m_transientFaces.end(); ++fi) {
        DeleteFace(*fi);
    }
    // The vertices should now be trivial to purge after the transient
    // faces have been cleared
    for (typename std::vector<HbrVertex<T>*>::iterator vi = m_transientVertices.begin();
         vi != m_transientVertices.end(); ++vi) {
        DeleteVertex(*vi);
    }
    m_transientVertices.clear();
    m_transientFaces.clear();
    // Reset max face ID
    int i;
    for (i = nfaces - 1; i >= 0; --i) {
        if (faces[i]) {
            maxFaceID = i + 1;
            break;
        }
    }
    // Reset max vertex ID
    for (i = nvertices - 1; i >= 0; --i) {
        if (vertices[i]) {
            maxVertexID = i + 1;
            break;
        }
    }
}

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRMESH_H */
