/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
*/

#ifndef AI_ALLOCATOR_H_INC
#define AI_ALLOCATOR_H_INC

#include <assimp/types.h>

#include <stddef.h>
#include <stdlib.h>
#include <memory.h>

typedef void* (*alloc_func)(size_t size);
typedef void  (*free_func)(void* ptr);

static void *aiDefaultAlloc(size_t size) {
    return malloc(size);
}

static void aiDefaultFree(void *ptr) {
    free(ptr);
}

struct aiAllocator {
    static alloc_func allocFunc;
    static free_func freeFunc;

    static void DefaultInit() {
        allocFunc = aiDefaultAlloc;
        freeFunc = aiDefaultFree;
    }

    static void init(alloc_func alloc_fn, free_func free_fn) {
        allocFunc = alloc_fn;
        freeFunc = free_fn;
    }
} gAllocatoMod;

static void aiRegisterAllocator(alloc_func alloc_fn, free_func free_fn) {
    aiAllocator::init(alloc_fn, free_fn);
}

ASSIMP_API void *aiAlloc(size_t size);
ASSIMP_API void aiFree(void *ptr);

#ifdef __cplusplus
extern "C" {
#endif

#define AI_NEW(ai_type, ...) new ai_type(__VA_ARGS__)
#define AI_NEW_ARRAY(ai_type, count) new ai_type[count]()
#define AI_DELETE(ptr) delete ptr
#define AI_DELETE_ARRAY(ptr) delete[] ptr

#ifdef __cplusplus
}
#endif

#endif // AI_ALLOCATOR_H_INC
