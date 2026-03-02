#include <assimp/allocator.h>

aiAllocator gAllocatoMod;

void *aiAlloc(size_t size) {
    return aiAllocator::allocFunc(size);
}

void aiFree(void *ptr) {
    aiAllocator::freeFunc(ptr);
}

