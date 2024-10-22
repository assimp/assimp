/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team
Copyright (c) 2019 bzt

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
#if !defined ASSIMP_BUILD_NO_M3D_IMPORTER || !(defined ASSIMP_BUILD_NO_EXPORT || defined ASSIMP_BUILD_NO_M3D_EXPORTER)

#include "M3DWrapper.h"

#include <assimp/DefaultIOSystem.h>
#include <assimp/IOStreamBuffer.h>
#include <assimp/ai_assert.h>

#ifdef ASSIMP_USE_M3D_READFILECB

#if (__cplusplus >= 201103L) || !defined(_MSC_VER) || (_MSC_VER >= 1900) // C++11 and MSVC 2015 onwards
#define threadlocal thread_local
#else
#if defined(_MSC_VER) && (_MSC_VER >= 1800) // there's an alternative for MSVC 2013
#define threadlocal __declspec(thread)
#else
#define threadlocal
#endif
#endif

extern "C" {

// workaround: the M3D SDK expects a C callback, but we want to use Assimp::IOSystem to implement that
threadlocal void *m3dimporter_pIOHandler;

unsigned char *m3dimporter_readfile(char *fn, unsigned int *size) {
    ai_assert(nullptr != fn);
    ai_assert(nullptr != size);
    std::string file(fn);
    std::unique_ptr<Assimp::IOStream> pStream(
            (reinterpret_cast<Assimp::IOSystem *>(m3dimporter_pIOHandler))->Open(file, "rb"));
    size_t fileSize = 0;
    unsigned char *data = nullptr;
    // sometimes pStream is nullptr in a single-threaded scenario too for some reason
    // (should be an empty object returning nothing I guess)
    if (pStream) {
        fileSize = pStream->FileSize();
        // should be allocated with malloc(), because the library will call free() to deallocate
        data = (unsigned char *)malloc(fileSize);
        if (!data || !pStream.get() || !fileSize || fileSize != pStream->Read(data, 1, fileSize)) {
            pStream.reset();
            *size = 0;
            // don't throw a deadly exception, it's not fatal if we can't read an external asset
            return nullptr;
        }
        pStream.reset();
    }
    *size = (int)fileSize;
    return data;
}
}
#endif

namespace Assimp {
M3DWrapper::M3DWrapper() {
    // use malloc() here because m3d_free() will call free()
    m3d_ = (m3d_t *)calloc(1, sizeof(m3d_t));
}

M3DWrapper::M3DWrapper(IOSystem *pIOHandler, const std::vector<unsigned char> &buffer) {
    if (nullptr == pIOHandler) {
        ai_assert(nullptr != pIOHandler);
    }

#ifdef ASSIMP_USE_M3D_READFILECB
    // pass this IOHandler to the C callback in a thread-local pointer
    m3dimporter_pIOHandler = pIOHandler;
    m3d_ = m3d_load(const_cast<unsigned char *>(buffer.data()), m3dimporter_readfile, free, nullptr);
    // Clear the C callback
    m3dimporter_pIOHandler = nullptr;
#else
    m3d_ = m3d_load(const_cast<unsigned char *>(buffer.data()), nullptr, nullptr, nullptr);
#endif
}

M3DWrapper::~M3DWrapper() {
    reset();
}

void M3DWrapper::reset() {
    ClearSave();
    if (m3d_) {
        m3d_free(m3d_);
    }
    m3d_ = nullptr;
}

unsigned char *M3DWrapper::Save(int quality, int flags, unsigned int &size) {
#if (!(ASSIMP_BUILD_NO_EXPORT || ASSIMP_BUILD_NO_M3D_EXPORTER))
    ClearSave();
    saved_output_ = m3d_save(m3d_, quality, flags, &size);
    return saved_output_;
#else
    (void)quality;
    (void)flags;
    (void)size;
    return nullptr;
#endif
}

void M3DWrapper::ClearSave() {
    if (saved_output_) {
        M3D_FREE(saved_output_);
    }
    saved_output_ = nullptr;
}
} // namespace Assimp

#endif
