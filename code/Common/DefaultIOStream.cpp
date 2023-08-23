/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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
/** @file  DefaultIOStream.cpp
 *  @brief Default File I/O implementation for #Importer
 */

#include <assimp/DefaultIOStream.h>
#include <assimp/ai_assert.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace Assimp;

namespace {

template <size_t sizeOfPointer>
inline size_t select_ftell(FILE *file) {
    return ::ftell(file);
}

template <size_t sizeOfPointer>
inline int select_fseek(FILE *file, int64_t offset, int origin) {
    return ::fseek(file, static_cast<long>(offset), origin);
}



#if defined _WIN32 && (!defined __GNUC__ || !defined __CLANG__ && __MSVCRT_VERSION__ >= 0x0601)
template <>
inline size_t select_ftell<8>(FILE *file) {
    return (size_t)::_ftelli64(file);
}

template <>
inline int select_fseek<8>(FILE *file, int64_t offset, int origin) {
    return ::_fseeki64(file, offset, origin);
}

#endif

} // namespace

// ----------------------------------------------------------------------------------
DefaultIOStream::~DefaultIOStream() {
    if (mFile) {
        ::fclose(mFile);
    }
}

// ----------------------------------------------------------------------------------
size_t DefaultIOStream::Read(void *pvBuffer,
        size_t pSize,
        size_t pCount) {
    if (0 == pCount) {
        return 0;
    }
    ai_assert(nullptr != pvBuffer);
    ai_assert(0 != pSize);

    return (mFile ? ::fread(pvBuffer, pSize, pCount, mFile) : 0);
}

// ----------------------------------------------------------------------------------
size_t DefaultIOStream::Write(const void *pvBuffer,
        size_t pSize,
        size_t pCount) {
    ai_assert(nullptr != pvBuffer);
    ai_assert(0 != pSize);

    return (mFile ? ::fwrite(pvBuffer, pSize, pCount, mFile) : 0);
}

// ----------------------------------------------------------------------------------
aiReturn DefaultIOStream::Seek(size_t pOffset,
        aiOrigin pOrigin) {
    if (!mFile) {
        return AI_FAILURE;
    }

    // Just to check whether our enum maps one to one with the CRT constants
    static_assert(aiOrigin_CUR == SEEK_CUR &&
                          aiOrigin_END == SEEK_END && aiOrigin_SET == SEEK_SET,
            "aiOrigin_CUR == SEEK_CUR && \
        aiOrigin_END == SEEK_END && aiOrigin_SET == SEEK_SET");

    // do the seek
    return (0 == select_fseek<sizeof(void *)>(mFile, (int64_t)pOffset, (int)pOrigin) ? AI_SUCCESS : AI_FAILURE);
}

// ----------------------------------------------------------------------------------
size_t DefaultIOStream::Tell() const {
    if (!mFile) {
        return 0;
    }
    return select_ftell<sizeof(void *)>(mFile);
}

// ----------------------------------------------------------------------------------
size_t DefaultIOStream::FileSize() const {
    if (!mFile || mFilename.empty()) {
        return 0;
    }

    if (SIZE_MAX == mCachedSize) {

        // Although fseek/ftell would allow us to reuse the existing file handle here,
        // it is generally unsafe because:
        //  - For binary streams, it is not technically well-defined
        //  - For text files the results are meaningless
        // That's why we use the safer variant fstat here.
        //
        // See here for details:
        // https://www.securecoding.cert.org/confluence/display/seccode/FIO19-C.+Do+not+use+fseek()+and+ftell()+to+compute+the+size+of+a+regular+file
#if defined _WIN32 && (!defined __GNUC__ || !defined __CLANG__ && __MSVCRT_VERSION__ >= 0x0601)
        struct __stat64 fileStat;
        //using fileno + fstat avoids having to handle the filename
        int err = _fstat64(_fileno(mFile), &fileStat);
        if (0 != err)
            return 0;
        mCachedSize = (size_t)(fileStat.st_size);
#elif defined _WIN32
        struct _stat32 fileStat;
        //using fileno + fstat avoids having to handle the filename
        int err = _fstat32(_fileno(mFile), &fileStat);
        if (0 != err)
            return 0;
        mCachedSize = (size_t)(fileStat.st_size);
#elif defined __GNUC__ || defined __APPLE__ || defined __MACH__ || defined __FreeBSD__
        struct stat fileStat;
        int err = stat(mFilename.c_str(), &fileStat);
        if (0 != err)
            return 0;
        const unsigned long long cachedSize = fileStat.st_size;
        mCachedSize = static_cast<size_t>(cachedSize);
#else
#error "Unknown platform"
#endif
    }
    return mCachedSize;
}

// ----------------------------------------------------------------------------------
void DefaultIOStream::Flush() {
    if (mFile) {
        ::fflush(mFile);
    }
}

// ----------------------------------------------------------------------------------
