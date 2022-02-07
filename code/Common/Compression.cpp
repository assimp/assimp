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

#include "Compression.h"
#include <assimp/ai_assert.h>
#include <assimp/Exceptional.h>

#ifdef ASSIMP_BUILD_NO_OWN_ZLIB
#include <zlib.h>
#else
#include "../contrib/zlib/zlib.h"
#endif

namespace Assimp {

struct Compression::impl {
    bool mOpen;
    z_stream mZSstream;

    impl() :
            mOpen(false) {}
};

Compression::Compression() :
        mImpl(new impl) {
    // empty
}

Compression::~Compression() {
    ai_assert(mImpl != nullptr);

    delete mImpl;
}

bool Compression::open() {
    ai_assert(mImpl != nullptr);

    if (mImpl->mOpen) {
        return false;
    }

    // build a zlib stream
    mImpl->mZSstream.opaque = Z_NULL;
    mImpl->mZSstream.zalloc = Z_NULL;
    mImpl->mZSstream.zfree = Z_NULL;
    mImpl->mZSstream.data_type = Z_BINARY;

    // raw decompression without a zlib or gzip header
    inflateInit2(&mImpl->mZSstream, -MAX_WBITS);
    mImpl->mOpen = true;

    return mImpl->mOpen;
}

constexpr size_t MYBLOCK = 1024;

size_t Compression::decompress(unsigned char *data, size_t in, std::vector<unsigned char> &uncompressed) {
    ai_assert(mImpl != nullptr);

    mImpl->mZSstream.next_in = reinterpret_cast<Bytef *>(data);
    mImpl->mZSstream.avail_in = (uInt)in;

    Bytef block[MYBLOCK] = {};
    int ret = 0;
    size_t total = 0l;
    do {
        mImpl->mZSstream.avail_out = MYBLOCK;
        mImpl->mZSstream.next_out = block;
        ret = inflate(&mImpl->mZSstream, Z_NO_FLUSH);

        if (ret != Z_STREAM_END && ret != Z_OK) {
            throw DeadlyImportError("Compression", "Failure decompressing this file using gzip.");

        }
        const size_t have = MYBLOCK - mImpl->mZSstream.avail_out;
        total += have;
        uncompressed.resize(total);
        ::memcpy(uncompressed.data() + total - have, block, have);
    } while (ret != Z_STREAM_END);

    return total;
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
