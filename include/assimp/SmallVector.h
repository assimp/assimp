/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team


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
/** \brief Small vector with inplace storage. Reduces heap allocations when list is shorter
than initial capasity
 */
template<typename T, unsigned int Capasity>
class SmallVector
{
public:
    SmallVector()
        : mStorage(mInplaceStorage)
        , mSize(0)
        , mCapasity(Capasity)
    {
    }

    ~SmallVector()
    {
        if (mStorage != mInplaceStorage) {
            delete [] mStorage;
        }
    }

    void push_back(const T& item)
    {
        if (mSize < mCapasity) {
            mStorage[mSize++] = item;
        }
        else push_back_and_grow(item);
    }

    void resize(unsigned int newSize)
    {
        if (newSize > mCapasity)
            grow(newSize);
        mSize = newSize;
    }

    unsigned int size() const
    {
        return mSize;
    }

    T* begin()
    {
        return mStorage;
    }

    T* end()
    {
        return &mStorage[mSize];
    }

    T* begin() const
    {
        return mStorage;
    }

    T* end() const
    {
        return &mStorage[mSize];
    }

private:
    void grow(unsigned int newCapasity)
    {
        T* pOldStorage = mStorage;
        T* pNewStorage = new T[newCapasity];

        std::memcpy(pNewStorage, pOldStorage, mSize * sizeof(T));

        mStorage = pNewStorage;
        mCapasity = newCapasity;

        if (pOldStorage != mInplaceStorage)
            delete [] pOldStorage;
    }

    void push_back_and_grow(const T& item)
    {
        grow(mCapasity + Capasity);

        mStorage[mSize++] = item;
    }

    T* mStorage;
    unsigned int mSize;
    unsigned int mCapasity;
    T mInplaceStorage[Capasity];
};

} // end namespace Assimp

#endif // !! AI_SMALLVECTOR_H_INC
