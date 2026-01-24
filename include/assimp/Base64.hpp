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
#ifndef AI_BASE64_HPP_INC
#define AI_BASE64_HPP_INC

#include <assimp/defs.h>

#include <stdint.h>
#include <vector>
#include <string>

namespace Assimp {
namespace Base64 {

/// @brief Will encode the given character buffer from UTF64 to ASCII
/// @param in           The UTF-64 buffer.
/// @param inLength     The size of the buffer
/// @param out          The encoded ASCII string.
ASSIMP_API void Encode(const uint8_t *in, size_t inLength, std::string &out);

/// @brief Will encode the given character buffer from UTF64 to ASCII.
/// @param in   A vector, which contains the buffer for encoding.
/// @param out  The encoded ASCII string.
ASSIMP_API void Encode(const std::vector<uint8_t> &in, std::string &out);

/// @brief Will encode the given character buffer from UTF64 to ASCII.
/// @param in   A vector, which contains the buffer for encoding.
/// @return The encoded ASCII string.
ASSIMP_API std::string Encode(const std::vector<uint8_t> &in);

/// @brief Will decode the given character buffer from ASCII to UTF64.
/// @param in           The ASCII buffer to decode.
/// @param inLength     The size of the buffer.
/// @param out          The decoded buffer.
/// @return The new buffer size.
ASSIMP_API size_t Decode(const char *in, size_t inLength, uint8_t *&out);

/// @brief Will decode the given character buffer from ASCII to UTF64.
/// @param in   The ASCII buffer to decode as a std::string.
/// @param out  The decoded buffer.
/// @return The new buffer size.
ASSIMP_API size_t Decode(const std::string &in, std::vector<uint8_t> &out);

/// @brief Will decode the given character buffer from ASCII to UTF64.
/// @param in   The ASCII string.
/// @return The decoded buffer in a vector.
ASSIMP_API std::vector<uint8_t> Decode(const std::string &in);

} // namespace Base64
} // namespace Assimp

#endif // AI_BASE64_HPP_INC
