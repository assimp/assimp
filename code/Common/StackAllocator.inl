/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

#include "StackAllocator.h"
#include <assimp/ai_assert.h>
#include <algorithm>

using namespace Assimp;

inline StackAllocator::StackAllocator() {
}

inline StackAllocator::~StackAllocator() {
    FreeAll();
}

inline void *StackAllocator::Allocate(size_t byteSize) {
    if (m_subIndex + byteSize > m_blockAllocationSize) // start a new block
    {
        // double block size every time, up to maximum of g_maxBytesPerBlock.
        // Block size must be at least as large as byteSize, but we want to use this for small allocations anyway.
        m_blockAllocationSize = std::max<std::size_t>(std::min<std::size_t>(m_blockAllocationSize * 2, g_maxBytesPerBlock), byteSize);
        uint8_t *data = new uint8_t[m_blockAllocationSize];
        m_storageBlocks.emplace_back(data);
        m_subIndex = byteSize;
        return data;
    }

    uint8_t *data = m_storageBlocks.back();
    data += m_subIndex;
    m_subIndex += byteSize;

    return data;
}

inline void StackAllocator::FreeAll() {
    for (size_t i = 0; i < m_storageBlocks.size(); i++) {
        delete [] m_storageBlocks[i];
    }
    std::vector<uint8_t *> empty;
    m_storageBlocks.swap(empty);
    // start over:
    m_blockAllocationSize = g_startBytesPerBlock;
    m_subIndex = g_maxBytesPerBlock;
}
