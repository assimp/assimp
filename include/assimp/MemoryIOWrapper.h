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

/** @file MemoryIOWrapper.h
 *  Handy IOStream/IOSystem implementation to read directly from a memory buffer */
#pragma once
#ifndef AI_MEMORYIOSTREAM_H_INC
#define AI_MEMORYIOSTREAM_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/ai_assert.h>

#include <stdint.h>

namespace Assimp    {

#define AI_MEMORYIO_MAGIC_FILENAME "$$$___magic___$$$"
#define AI_MEMORYIO_MAGIC_FILENAME_LENGTH 17

// ----------------------------------------------------------------------------------
/** Implementation of IOStream to read directly from a memory buffer */
// ----------------------------------------------------------------------------------
class MemoryIOStream : public IOStream {
public:
    MemoryIOStream (const uint8_t* buff, size_t len, bool own = false) : 
            buffer (buff), 
            length(len),
            pos(static_cast<size_t>(0)),
            own(own) {
        // empty
    }

    ~MemoryIOStream() override  {
        if(own) {
            delete[] buffer;
        }
    }

    size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override {
        ai_assert(nullptr != pvBuffer);
        ai_assert(0 != pSize);

        const size_t cnt = std::min( pCount, (length-pos) / pSize);
        const size_t ofs = pSize * cnt;

        ::memcpy(pvBuffer,buffer+pos,ofs);
        pos += ofs;

        return cnt;
    }

    size_t Write(const void*, size_t, size_t ) override {
        ai_assert(false); // won't be needed
        return 0;
    }

    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
        if (aiOrigin_SET == pOrigin) {
            if (pOffset > length) {
                return AI_FAILURE;
            }
            pos = pOffset;
        } else if (aiOrigin_END == pOrigin) {
            if (pOffset > length) {
                return AI_FAILURE;
            }
            pos = length-pOffset;
        } else {
            if (pOffset+pos > length) {
                return AI_FAILURE;
            }
            pos += pOffset;
        }
        return AI_SUCCESS;
    }

    size_t Tell() const override {
        return pos;
    }

    size_t FileSize() const override {
        return length;
    }

    void Flush() override{
        ai_assert(false); // won't be needed
    }

private:
    const uint8_t* buffer;
    size_t length,pos;
    bool own;
};

// ---------------------------------------------------------------------------
/// @brief Dummy IO system to read from a memory buffer.
class MemoryIOSystem : public IOSystem {
public:
    /// @brief Constructor.
    MemoryIOSystem(const uint8_t* buff, size_t len, IOSystem* io) : buffer(buff), length(len), existing_io(io) {
        // empty
    }

    /// @brief Destructor.
    ~MemoryIOSystem() = default;

    // -------------------------------------------------------------------
    /// @brief Tests for the existence of a file at the given path.
    bool Exists(const char* pFile) const override {
        if (0 == strncmp( pFile, AI_MEMORYIO_MAGIC_FILENAME, AI_MEMORYIO_MAGIC_FILENAME_LENGTH ) ) {
            return true;
        }
        return existing_io ? existing_io->Exists(pFile) : false;
    }

    // -------------------------------------------------------------------
    /// @brief Returns the directory separator.
    char getOsSeparator() const override {
        return existing_io ? existing_io->getOsSeparator()
                           : '/';  // why not? it doesn't care
    }

    // -------------------------------------------------------------------
    /// @brief Open a new file with a given path.
    IOStream* Open(const char* pFile, const char* pMode = "rb") override {
        if ( 0 == strncmp( pFile, AI_MEMORYIO_MAGIC_FILENAME, AI_MEMORYIO_MAGIC_FILENAME_LENGTH ) ) {
            created_streams.emplace_back(new MemoryIOStream(buffer, length));
            return created_streams.back();
        }
        return existing_io ? existing_io->Open(pFile, pMode) : nullptr;
    }

    // -------------------------------------------------------------------
    /// @brief Closes the given file and releases all resources associated with it.
    void Close( IOStream* pFile) override {
        auto it = std::find(created_streams.begin(), created_streams.end(), pFile);
        if (it != created_streams.end()) {
            delete pFile;
            created_streams.erase(it);
        } else if (existing_io) {
            existing_io->Close(pFile);
        }
    }

    // -------------------------------------------------------------------
    /// @brief Compare two paths
    bool ComparePaths(const char* one, const char* second) const override {
        return existing_io ? existing_io->ComparePaths(one, second) : false;
    }
    
    /// @brief Will push the directory.
    bool PushDirectory( const std::string &path ) override {
        return existing_io ? existing_io->PushDirectory(path) : false;
    }

    /// @brief Will return the current directory from the stack top.
    const std::string &CurrentDirectory() const override {
        static std::string empty;
        return existing_io ? existing_io->CurrentDirectory() : empty;
    }

    /// @brief Returns the stack size.
    size_t StackSize() const override {
        return existing_io ? existing_io->StackSize() : 0;
    }

    /// @brief Will pop the upper directory.
    bool PopDirectory() override {
        return existing_io ? existing_io->PopDirectory() : false;
    }

    /// @brief Will create the directory.
    bool CreateDirectory( const std::string &path ) override {
        return existing_io ? existing_io->CreateDirectory(path) : false;
    }
    
    /// @brief Will change the directory.
    bool ChangeDirectory( const std::string &path ) override {
        return existing_io ? existing_io->ChangeDirectory(path) : false;
    }

    /// @brief Will delete the file.
    bool DeleteFile( const std::string &file ) override {
        return existing_io ? existing_io->DeleteFile(file) : false;
    }

private:
    const uint8_t* buffer;
    size_t length;
    IOSystem* existing_io;
    std::vector<IOStream*> created_streams;
};

} // end namespace Assimp

#endif // AI_MEMORYIOSTREAM_H_INC
