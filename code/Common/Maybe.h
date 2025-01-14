/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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
#pragma once

#include <assimp/ai_assert.h>
#include <utility>

namespace Assimp {

/// @brief This class implements an optional type
/// @tparam T   The type to store.
template <typename T>
struct Maybe {
    /// @brief
    Maybe() = default;

    /// @brief
    /// @param val
    template <typename U>
    explicit Maybe(U &&val) :
            _val(std::forward<U>(val)), _valid(true) {}

    /// @brief Validate the value
    /// @return true if valid.
    operator bool() const {
        return _valid;
    }

    /// @brief Will assign a value.
    /// @param v The new valid value.
    template <typename U>
    void Set(U &&v) {
        ai_assert(!_valid);

        _valid = true;
        _val = std::forward<U>(v);
    }

    /// @brief  Will return the value when it is valid.
    /// @return The value.
    const T &Get() const {
        ai_assert(_valid);
        return _val;
    }

    Maybe &operator&() = delete;
    Maybe(const Maybe &) = delete;
    Maybe &operator=(const Maybe &) = default;

private:
    T _val;
    bool _valid = false;
};

} // namespace Assimp
