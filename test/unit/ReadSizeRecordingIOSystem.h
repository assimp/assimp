/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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
#pragma once

#include <assimp/IOSystem.hpp>
#include <assimp/MemoryIOWrapper.h>

#include <algorithm>
#include <cstring>

namespace Assimp {

// Serves one file from a buffer and records the largest single read a loader asks for,
// to check that a loader does not trust a size field beyond what the file holds.
class ReadSizeRecordingIOSystem : public IOSystem {
public:
    ReadSizeRecordingIOSystem(const char *name, const uint8_t *data, size_t length, size_t &largestRead) :
            mName(name), mData(data), mLength(length), mLargestRead(largestRead) {}

    bool Exists(const char *pFile) const override {
        return pFile != nullptr && strcmp(pFile, mName) == 0;
    }

    char getOsSeparator() const override {
        return '/';
    }

    IOStream *Open(const char *pFile, const char * = "rb") override {
        return Exists(pFile) ? new Stream(mData, mLength, mLargestRead) : nullptr;
    }

    void Close(IOStream *pFile) override {
        delete pFile;
    }

private:
    class Stream : public MemoryIOStream {
    public:
        Stream(const uint8_t *data, size_t length, size_t &largestRead) :
                MemoryIOStream(data, length), mLargestRead(largestRead) {}

        size_t Read(void *pvBuffer, size_t pSize, size_t pCount) override {
            mLargestRead = std::max(mLargestRead, pSize * pCount);
            return MemoryIOStream::Read(pvBuffer, pSize, pCount);
        }

    private:
        size_t &mLargestRead;
    };

    const char *mName;
    const uint8_t *mData;
    size_t mLength;
    size_t &mLargestRead;
};

} // namespace Assimp
