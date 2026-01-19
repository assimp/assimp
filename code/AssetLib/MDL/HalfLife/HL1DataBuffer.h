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

/** @file HL1DataBuffer.h
 *  @brief Declaration of the Half-Life 1 data buffer.
 */

#ifndef AI_HL1DATABUFFER_INCLUDED
#define AI_HL1DATABUFFER_INCLUDED

#include <assimp/Exceptional.h>
#include <assimp/defs.h>
#include <cstddef>
#include <memory>

namespace Assimp {
namespace MDL {
namespace HalfLife {

/**
 * \brief Lightweight byte buffer wrapper for HL1 binary parsing.
 *
 * Acts as either:
 *  - a non-owning view into external memory, or
 *  - an owning buffer backed by a unique_ptr.
 *
 * Copy is disabled to avoid accidental double-ownership; move is supported.
 */
class HL1DataBuffer {
public:
    /** \brief Construct an empty buffer (null view). */
    HL1DataBuffer() AI_NO_EXCEPT : data_(nullptr),
                                   length_(0),
                                   owner_(nullptr) {}

    /** \brief Non-copyable (buffer may own memory). */
    HL1DataBuffer(const HL1DataBuffer &) = delete;

    /** \brief Non-copyable (buffer may own memory). */
    HL1DataBuffer &operator=(const HL1DataBuffer &) = delete;

    /** \brief Move-construct, transferring ownership/view state. */
    HL1DataBuffer(HL1DataBuffer &&other) AI_NO_EXCEPT : data_(other.data_),
                                                        length_(other.length_),
                                                        owner_(std::move(other.owner_)) {
        other.data_ = nullptr;
        other.length_ = 0;
        other.owner_ = nullptr;
    }

    /** \brief Move-assign, transferring ownership/view state. */
    HL1DataBuffer &operator=(HL1DataBuffer &&other) AI_NO_EXCEPT {
        if (this != &other) {
            data_ = other.data_;
            length_ = other.length_;
            owner_ = std::move(other.owner_);

            other.data_ = nullptr;
            other.length_ = 0;
            other.owner_ = nullptr;
        }

        return *this;
    }

    /**
     * \brief Create a non-owning view into external bytes.
     *
     * \param[in] data Pointer to raw bytes (must outlive the view).
     * \param[in] length Size in bytes.
     */
    static HL1DataBuffer view(const unsigned char *data, size_t length) {
        HL1DataBuffer b;
        b.data_ = data;
        b.length_ = length;
        b.owner_ = nullptr;
        return b;
    }

    /**
     * \brief Create a non-owning view of another buffer.
     *
     * \param[in] other Source buffer.
     */
    static HL1DataBuffer view(const HL1DataBuffer &other) {
        HL1DataBuffer b;
        b.data_ = other.data_;
        b.length_ = other.length_;
        b.owner_ = nullptr;
        return b;
    }

    /**
     * \brief Create an owning buffer by taking ownership of allocated storage.
     *
     * \param[in] buffer Unique buffer storage.
     * \param[in] length Size in bytes.
     */
    static HL1DataBuffer owning(std::unique_ptr<unsigned char[]> buffer, size_t length) {
        HL1DataBuffer b;
        b.data_ = buffer.get();
        b.length_ = length;
        b.owner_ = std::move(buffer);
        return b;
    }

    /**
     * \brief Return a typed pointer into the buffer with bounds checks.
     *
     * \param[in] offset Byte offset from the start of the buffer.
     * \param[in] elements Number of elements to access.
     * \return Pointer to the requested typed data inside the buffer.
     * \throws DeadlyImportError if the request is out of range.
     */
    template <typename DataType>
    const DataType *get_data(int offset, int elements) const {
        if (offset < 0 || elements < 0) {
            throw DeadlyImportError("MDL file contains invalid data");
        }

        const size_t uoffset = static_cast<size_t>(offset);
        const size_t uelements = static_cast<size_t>(elements);

        if (uoffset > length_ || uelements > (length_ - uoffset) / sizeof(DataType)) {
            throw DeadlyImportError("MDL file contains invalid data");
        }

        return reinterpret_cast<const DataType *>(data_ + uoffset);
    }

private:
    /** Raw byte pointer (points to owner_.get() when owning, otherwise external). */
    const unsigned char *data_;
    /** Buffer length in bytes. */
    size_t length_;
    /** Owning storage (null for views). */
    std::unique_ptr<unsigned char[]> owner_;
};

} // namespace HalfLife
} // namespace MDL
} // namespace Assimp

#endif // AI_HL1DATABUFFER_INCLUDED
