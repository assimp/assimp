/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

/** @file Defines small vector with inplace storage.
Based on CppCon 2016: Chandler Carruth "High Performance Code 201: Hybrid Data Structures" */

#pragma once
#ifndef AI_SMALLVECTOR_H_INC
#define AI_SMALLVECTOR_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

namespace Assimp {

// --------------------------------------------------------------------------------------------
/// @brief Small vector with inplace storage.
///
/// Reduces heap allocations when list is shorter. It uses a small array for a dedicated size.
/// When the growing gets bigger than this small cache a dynamic growing algorithm will be
/// used.
// --------------------------------------------------------------------------------------------
template<typename T, unsigned int Capacity>
class SmallVector {
public:
    /// @brief  The default class constructor.
    SmallVector() :
            mStorage(mInplaceStorage),
            mSize(0),
            mCapacity(Capacity) {
        // empty
    }

    /// @brief  The class destructor.
    ~SmallVector() {
        if (mStorage != mInplaceStorage) {
            delete [] mStorage;
        }
    }

    /// @brief  Will push a new item. The capacity will grow in case of a too small capacity.
    /// @param  item    [in] The item to push at the end of the vector.
    void push_back(const T& item) {
        if (mSize < mCapacity) {
            mStorage[mSize++] = item;
            return;
        }

        push_back_and_grow(item);
    }

    /// @brief  Will resize the vector.
    /// @param  newSize     [in] The new size.
    void resize(size_t newSize) {
        if (newSize > mCapacity) {
            grow(newSize);
        }
        mSize = newSize;
    }

    /// @brief  Returns the current size of the vector.
    /// @return The current size.
    size_t size() const {
        return mSize;
    }

    /// @brief  Returns a pointer to the first item.
    /// @return The first item as a pointer.
    T* begin() {
        return mStorage;
    }

    /// @brief  Returns a pointer to the end.
    /// @return The end as a pointer.
    T* end() {
        return &mStorage[mSize];
    }

    /// @brief  Returns a const pointer to the first item.
    /// @return The first item as a const pointer.
    T* begin() const {
        return mStorage;
    }

    /// @brief  Returns a const pointer to the end.
    /// @return The end as a const pointer.
    T* end() const {
        return &mStorage[mSize];
    }

    SmallVector(const SmallVector &) = delete;
    SmallVector(SmallVector &&) = delete;
    SmallVector &operator = (const SmallVector &) = delete;
    SmallVector &operator = (SmallVector &&) = delete;

private:
    void grow( size_t newCapacity) {
        T* oldStorage = mStorage;
        T* newStorage = new T[newCapacity];

        std::memcpy(newStorage, oldStorage, mSize * sizeof(T));

        mStorage = newStorage;
        mCapacity = newCapacity;

        if (oldStorage != mInplaceStorage) {
            delete [] oldStorage;
        }
    }

    void push_back_and_grow(const T& item) {
        grow(mCapacity + Capacity);

        mStorage[mSize++] = item;
    }

    T* mStorage;
    size_t mSize;
    size_t mCapacity;
    T mInplaceStorage[Capacity];
};

} // end namespace Assimp

#endif // !! AI_SMALLVECTOR_H_INC
