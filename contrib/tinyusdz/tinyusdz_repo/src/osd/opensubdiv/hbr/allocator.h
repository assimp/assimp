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

#ifndef OPENSUBDIV3_HBRALLOCATOR_H
#define OPENSUBDIV3_HBRALLOCATOR_H

#include "../version.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

typedef void (*HbrMemStatFunction)(size_t bytes);

/**
 * HbrAllocator - derived from UtBlockAllocator.h, but embedded in
 * libhbrep.
 */
template <typename T> class HbrAllocator {

public:

    /// Constructor
    HbrAllocator(size_t *memorystat, int blocksize, void (*increment)(size_t bytes), void (*decrement)(size_t bytes), size_t elemsize = sizeof(T));

    /// Destructor
    ~HbrAllocator();

    /// Create an allocated object
    T * Allocate();

    /// Return an allocated object to the block allocator
    void Deallocate(T *);

    /// Clear the allocator, deleting all allocated objects.
    void Clear();

    void SetMemStatsIncrement(void (*increment)(size_t bytes)) { m_increment = increment; }

    void SetMemStatsDecrement(void (*decrement)(size_t bytes)) { m_decrement = decrement; }

private:
    size_t *m_memorystat;
    const int m_blocksize;
    int m_elemsize;
    T** m_blocks;

    // Number of actually allocated blocks
    int m_nblocks;

    // Size of the m_blocks array (which is NOT the number of actually
    // allocated blocks)
    int m_blockCapacity;

    int m_freecount;
    T * m_freelist;

    // Memory statistics tracking routines
    HbrMemStatFunction m_increment;
    HbrMemStatFunction m_decrement;
};

template <typename T>
HbrAllocator<T>::HbrAllocator(size_t *memorystat, int blocksize, void (*increment)(size_t bytes), void (*decrement)(size_t bytes), size_t elemsize)
    : m_memorystat(memorystat), m_blocksize(blocksize), m_elemsize((int)elemsize), m_blocks(0), m_nblocks(0), m_blockCapacity(0), m_freecount(0), m_increment(increment), m_decrement(decrement) {
}

template <typename T>
HbrAllocator<T>::~HbrAllocator() {
    Clear();
}

template <typename T>
void HbrAllocator<T>::Clear() {
    for (int i = 0; i < m_nblocks; ++i) {
        // Run the destructors (placement)
        T* blockptr = m_blocks[i];
        T* startblock = blockptr;
        for (int j = 0; j < m_blocksize; ++j) {
            blockptr->~T();
            blockptr = (T*) ((char*) blockptr + m_elemsize);
        }
        free(startblock);
        if (m_decrement) m_decrement(m_blocksize * m_elemsize);
        *m_memorystat -= m_blocksize * m_elemsize;
    }
    free(m_blocks);
    m_blocks = 0;
    m_nblocks = 0;
    m_blockCapacity = 0;
    m_freecount = 0;
    m_freelist = NULL;
}

template <typename T>
T*
HbrAllocator<T>::Allocate() {
    if (!m_freecount) {

        // Allocate a new block
        T* block = (T*) malloc(m_blocksize * m_elemsize);
        T* blockptr = block;
        // Run the constructors on each element using placement new
        for (int i = 0; i < m_blocksize; ++i) {
            new (blockptr) T();
            blockptr = (T*) ((char*) blockptr + m_elemsize);
        }
        if (m_increment) m_increment(m_blocksize * m_elemsize);
        *m_memorystat += m_blocksize * m_elemsize;

        // Put the block's entries on the free list
        blockptr = block;
        for (int i = 0; i < m_blocksize - 1; ++i) {
            T* next = (T*) ((char*) blockptr + m_elemsize);
            blockptr->GetNext() = next;
            blockptr = next;
        }
        blockptr->GetNext() = 0;
        m_freelist = block;

        // Keep track of the newly allocated block
        if (m_nblocks + 1 >= m_blockCapacity) {
            m_blockCapacity = m_blockCapacity * 2;
            if (m_blockCapacity < 1) m_blockCapacity = 1;
            m_blocks = (T**) realloc(m_blocks, m_blockCapacity * sizeof(T*));
        }
        m_blocks[m_nblocks] = block;
        m_nblocks++;
        m_freecount += m_blocksize;
    }
    T* obj = m_freelist;
    m_freelist = obj->GetNext();
    obj->GetNext() = 0;
    m_freecount--;
    return obj;
}

template <typename T>
void
HbrAllocator<T>::Deallocate(T * obj) {
    assert(!obj->GetNext());
    obj->GetNext() = m_freelist;
    m_freelist = obj;
    m_freecount++;
}

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_HBRALLOCATOR_H */
