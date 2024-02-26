/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

/** @file Provides cheat implementations for IOSystem and IOStream to
 *  redirect exporter output to a blob chain.*/

#pragma once
#ifndef AI_BLOBIOSYSTEM_H_INCLUDED
#define AI_BLOBIOSYSTEM_H_INCLUDED

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/cexport.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <cstdint>
#include <set>
#include <vector>

namespace Assimp {
class BlobIOSystem;

// --------------------------------------------------------------------------------------------
/** Redirect IOStream to a blob */
// --------------------------------------------------------------------------------------------
class BlobIOStream : public IOStream {
public:
    /// @brief The class constructor with all needed parameters
    /// @param creator  Pointer to the creator instance
    /// @param file     The filename
    /// @param initial  The initial size
    BlobIOStream(BlobIOSystem *creator, const std::string &file, size_t initial = 4096) :
            buffer(),
            cur_size(),
            file_size(),
            cursor(),
            initial(initial),
            file(file),
            creator(creator) {
        // empty
    }

    ///	@brief  The class destructor.
    ~BlobIOStream() override;

public:
    // -------------------------------------------------------------------
    aiExportDataBlob *GetBlob() {
        aiExportDataBlob *blob = new aiExportDataBlob();
        blob->size = file_size;
        blob->data = buffer;

        buffer = nullptr;

        return blob;
    }

    // -------------------------------------------------------------------
    size_t Read(void *, size_t, size_t) override {
        return 0;
    }

    // -------------------------------------------------------------------
    size_t Write(const void *pvBuffer, size_t pSize, size_t pCount) override {
        pSize *= pCount;
        if (cursor + pSize > cur_size) {
            Grow(cursor + pSize);
        }

        memcpy(buffer + cursor, pvBuffer, pSize);
        cursor += pSize;

        file_size = std::max(file_size, cursor);
        return pCount;
    }

    // -------------------------------------------------------------------
    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
        switch (pOrigin) {
            case aiOrigin_CUR:
                cursor += pOffset;
                break;

            case aiOrigin_END:
                cursor = file_size - pOffset;
                break;

            case aiOrigin_SET:
                cursor = pOffset;
                break;

            default:
                return AI_FAILURE;
        }

        if (cursor > file_size) {
            Grow(cursor);
        }

        file_size = std::max(cursor, file_size);

        return AI_SUCCESS;
    }

    // -------------------------------------------------------------------
    size_t Tell() const override {
        return cursor;
    }

    // -------------------------------------------------------------------
    size_t FileSize() const override {
        return file_size;
    }

    // -------------------------------------------------------------------
    void Flush() override {
        // ignore
    }

private:
    // -------------------------------------------------------------------
    void Grow(size_t need = 0) {
        // 1.5 and phi are very heap-friendly growth factors (the first
        // allows for frequent re-use of heap blocks, the second
        // forms a fibonacci sequence with similar characteristics -
        // since this heavily depends on the heap implementation
        // and other factors as well, i'll just go with 1.5 since
        // it is quicker to compute).
        size_t new_size = std::max(initial, std::max(need, cur_size + (cur_size >> 1)));

        const uint8_t *const old = buffer;
        buffer = new uint8_t[new_size];

        if (old) {
            memcpy(buffer, old, cur_size);
            delete[] old;
        }

        cur_size = new_size;
    }

private:
    uint8_t *buffer;
    size_t cur_size, file_size, cursor, initial;

    const std::string file;
    BlobIOSystem *const creator;
};

#define AI_BLOBIO_MAGIC "$blobfile"

// --------------------------------------------------------------------------------------------
/** Redirect IOSystem to a blob */
// --------------------------------------------------------------------------------------------
class BlobIOSystem : public IOSystem {

    friend class BlobIOStream;
    typedef std::pair<std::string, aiExportDataBlob *> BlobEntry;


public:
    /// @brief The default class constructor.
    BlobIOSystem() :
            baseName{AI_BLOBIO_MAGIC} {
    }

    ///	@brief  The class constructor with the base name.
    /// @param baseName     The base name.
    BlobIOSystem(const std::string &baseName) :
            baseName(baseName) {
        // empty
    }

    ~BlobIOSystem() override {
        for (BlobEntry &blobby : blobs) {
            delete blobby.second;
        }
    }

public:
    // -------------------------------------------------------------------
    const char *GetMagicFileName() const {
        return baseName.c_str();
    }

    // -------------------------------------------------------------------
    aiExportDataBlob *GetBlobChain() {
        const auto magicName = std::string(this->GetMagicFileName());
        const bool hasBaseName = baseName != AI_BLOBIO_MAGIC;

        // one must be the master
        aiExportDataBlob *master = nullptr, *cur;

        for (const BlobEntry &blobby : blobs) {
            if (blobby.first == magicName) {
                master = blobby.second;
                master->name.Set(hasBaseName ? blobby.first : "");
                break;
            }
        }

        if (!master) {
            ASSIMP_LOG_ERROR("BlobIOSystem: no data written or master file was not closed properly.");
            return nullptr;
        }

        cur = master;

        for (const BlobEntry &blobby : blobs) {
            if (blobby.second == master) {
                continue;
            }

            cur->next = blobby.second;
            cur = cur->next;

            if (hasBaseName) {
                cur->name.Set(blobby.first);
            } else {
                // extract the file extension from the file written
                const std::string::size_type s = blobby.first.find_first_of('.');
                cur->name.Set(s == std::string::npos ? blobby.first : blobby.first.substr(s + 1));
            }
        }

        // give up blob ownership
        blobs.clear();
        return master;
    }

public:
    // -------------------------------------------------------------------
    bool Exists(const char *pFile) const override {
        return created.find(std::string(pFile)) != created.end();
    }

    // -------------------------------------------------------------------
    char getOsSeparator() const override {
        return '/';
    }

    // -------------------------------------------------------------------
    IOStream *Open(const char *pFile, const char *pMode) override {
        if (pMode[0] != 'w') {
            return nullptr;
        }

        created.insert(std::string(pFile));
        return new BlobIOStream(this, std::string(pFile));
    }

    // -------------------------------------------------------------------
    void Close(IOStream *pFile) override {
        delete pFile;
    }

private:
    // -------------------------------------------------------------------
    void OnDestruct(const std::string &filename, BlobIOStream *child) {
        // we don't know in which the files are closed, so we
        // can't reliably say that the first must be the master
        // file ...
        blobs.emplace_back(filename, child->GetBlob());
    }

private:
    std::string baseName;
    std::set<std::string> created;
    std::vector<BlobEntry> blobs;
};

// --------------------------------------------------------------------------------------------
BlobIOStream::~BlobIOStream() {
    if (nullptr != creator) {
        creator->OnDestruct(file, this);
    }
    delete[] buffer;
}

} // namespace Assimp

#endif
