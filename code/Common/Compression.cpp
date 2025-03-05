/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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

#include "Compression.h"
#include <assimp/ai_assert.h>
#include <assimp/Exceptional.h>

namespace Assimp {

struct Compression::impl {
    bool mOpen;
    z_stream mZSstream;
    FlushMode mFlushMode;

    impl() :
            mOpen(false),
            mZSstream(),
            mFlushMode(Compression::FlushMode::NoFlush) {
        // empty
    }
};

Compression::Compression() :
        mImpl(new impl) {
    // empty
}

Compression::~Compression() {
    ai_assert(mImpl != nullptr);

    if (mImpl->mOpen) {
        close();
    }

    delete mImpl;
}

bool Compression::open(Format format, FlushMode flush, int windowBits) {
    ai_assert(mImpl != nullptr);

    if (mImpl->mOpen) {
        return false;
    }

    // build a zlib stream
    mImpl->mZSstream.opaque = Z_NULL;
    mImpl->mZSstream.zalloc = Z_NULL;
    mImpl->mZSstream.zfree = Z_NULL;
    mImpl->mFlushMode = flush;
    if (format == Format::Binary) {
        mImpl->mZSstream.data_type = Z_BINARY;
    } else {
        mImpl->mZSstream.data_type = Z_ASCII;
    }

    // raw decompression without a zlib or gzip header
    if (windowBits == 0) {
        inflateInit(&mImpl->mZSstream);
    } else {
        inflateInit2(&mImpl->mZSstream, windowBits);
    }
    mImpl->mOpen = true;

    return mImpl->mOpen;
}

static int getFlushMode(Compression::FlushMode flush) {
    int z_flush = 0;
    switch (flush) {
        case Compression::FlushMode::NoFlush:
            z_flush = Z_NO_FLUSH;
            break;
        case Compression::FlushMode::Block:
            z_flush = Z_BLOCK;
            break;
        case Compression::FlushMode::Tree:
            z_flush = Z_TREES;
            break;
        case Compression::FlushMode::SyncFlush:
            z_flush = Z_SYNC_FLUSH;
            break;
        case Compression::FlushMode::Finish:
            z_flush = Z_FINISH;
            break;
        default:
            ai_assert(false);
            break;
    }

    return z_flush;
}

static constexpr size_t MYBLOCK = 32786;

size_t Compression::decompress(const void *data, size_t in, std::vector<char> &uncompressed) {
    ai_assert(mImpl != nullptr);
    if (data == nullptr || in == 0) {
        return 0l;
    }

    mImpl->mZSstream.next_in = (Bytef*)(data);
    mImpl->mZSstream.avail_in = (uInt)in;

    int ret = 0;
    size_t total = 0l;
    const int flushMode = getFlushMode(mImpl->mFlushMode);
    if (flushMode == Z_FINISH) {
        mImpl->mZSstream.avail_out = static_cast<uInt>(uncompressed.size());
        mImpl->mZSstream.next_out = reinterpret_cast<Bytef *>(&*uncompressed.begin());
        ret = inflate(&mImpl->mZSstream, Z_FINISH);

        if (ret != Z_STREAM_END && ret != Z_OK) {
            throw DeadlyImportError("Compression", "Failure decompressing this file using gzip.");
        }
        total = mImpl->mZSstream.avail_out;
    } else {
        do {
            Bytef block[MYBLOCK] = {};
            mImpl->mZSstream.avail_out = MYBLOCK;
            mImpl->mZSstream.next_out = block;

            ret = inflate(&mImpl->mZSstream, flushMode);

            if (ret != Z_STREAM_END && ret != Z_OK) {
                throw DeadlyImportError("Compression", "Failure decompressing this file using gzip.");
            }
            const size_t have = MYBLOCK - mImpl->mZSstream.avail_out;
            total += have;
            uncompressed.resize(total);
            ::memcpy(uncompressed.data() + total - have, block, have);
        } while (ret != Z_STREAM_END);
    }

    return total;
}

size_t Compression::decompressBlock(const void *data, size_t in, char *out, size_t availableOut) {
    ai_assert(mImpl != nullptr);
    if (data == nullptr || in == 0 || out == nullptr || availableOut == 0) {
        return 0l;
    }

    // push data to the stream
    mImpl->mZSstream.next_in = (Bytef *)data;
    mImpl->mZSstream.avail_in = (uInt)in;
    mImpl->mZSstream.next_out = (Bytef *)out;
    mImpl->mZSstream.avail_out = (uInt)availableOut;

    // and decompress the data ....
    int ret = ::inflate(&mImpl->mZSstream, Z_SYNC_FLUSH);
    if (ret != Z_OK && ret != Z_STREAM_END) {
        throw DeadlyImportError("X: Failed to decompress MSZIP-compressed data");
    }

    ::inflateReset(&mImpl->mZSstream);
    ::inflateSetDictionary(&mImpl->mZSstream, (const Bytef *)out, (uInt)availableOut - mImpl->mZSstream.avail_out);

    return availableOut - (size_t)mImpl->mZSstream.avail_out;
}

bool Compression::isOpen() const {
    ai_assert(mImpl != nullptr);

    return mImpl->mOpen;
}

bool Compression::close() {
    ai_assert(mImpl != nullptr);

    if (!mImpl->mOpen) {
        return false;
    }

    inflateEnd(&mImpl->mZSstream);
    mImpl->mOpen = false;

    return true;
}

} // namespace Assimp
