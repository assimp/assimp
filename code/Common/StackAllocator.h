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

/** @file  StackAllocator.h
 *  @brief A very bare-bone allocator class that is suitable when
 *      allocating many small objects, e.g. during parsing.
 *      Individual objects are not freed, instead only the whole memory
 *      can be deallocated.
 */
#ifndef AI_STACK_ALLOCATOR_H_INC
#define AI_STACK_ALLOCATOR_H_INC

#include <vector>
#include <stdint.h>
#include <stddef.h>

namespace Assimp {

/** @brief A very bare-bone allocator class that is suitable when
 *      allocating many small objects, e.g. during parsing.
 *      Individual objects are not freed, instead only the whole memory
 *      can be deallocated.
*/
class StackAllocator {
public:
    /// @brief Constructs the allocator
    StackAllocator();

    /// @brief Destructs the allocator and frees all memory
    ~StackAllocator();

    // non copyable
    StackAllocator(const StackAllocator &) = delete;
    StackAllocator &operator=(const StackAllocator &) = delete;

    /// @brief Returns a pointer to byteSize bytes of heap memory that persists
    ///        for the lifetime of the allocator (or until FreeAll is called).
    inline void *Allocate(size_t byteSize);

    /// @brief Releases all the memory owned by this allocator.
    //         Memory provided through function Allocate is not valid anymore after this function has been called.
    inline void FreeAll();

private:
    constexpr const static size_t g_maxBytesPerBlock = 64 * 1024 * 1024; // The maximum size (in bytes) of a block
    constexpr const static size_t g_startBytesPerBlock = 16 * 1024;  // Size of the first block. Next blocks will double in size until maximum size of g_maxBytesPerBlock
    size_t m_blockAllocationSize = g_startBytesPerBlock; // Block size of the current block
    size_t m_subIndex = g_maxBytesPerBlock; // The current byte offset in the current block
    std::vector<uint8_t *> m_storageBlocks;  // A list of blocks
};

} // namespace Assimp

#include "StackAllocator.inl"

#endif // include guard
