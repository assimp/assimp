/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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

#include <assimp/Base64.hpp>
#include <assimp/Exceptional.h>

namespace Assimp {

namespace Base64 {

static constexpr uint8_t tableDecodeBase64[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 64, 0, 0,
    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0,
    0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0
};

static constexpr char tableEncodeBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static inline char EncodeChar(uint8_t b) {
    return tableEncodeBase64[size_t(b)];
}

inline uint8_t DecodeChar(char c) {
    if (c & 0x80) {
        throw DeadlyImportError("Invalid base64 char value: ", size_t(c));
    }
    return tableDecodeBase64[size_t(c & 0x7F)]; // TODO faster with lookup table or ifs?
}

void Encode(const uint8_t *in, size_t inLength, std::string &out) {
    if (in == nullptr || inLength==0) {
        out.clear();
        return;
    }

    size_t outLength = ((inLength + 2) / 3) * 4;

    size_t j = out.size();
    out.resize(j + outLength);

    for (size_t i = 0; i < inLength; i += 3) {
        uint8_t b = (in[i] & 0xFC) >> 2;
        out[j++] = EncodeChar(b);

        b = (in[i] & 0x03) << 4;
        if (i + 1 < inLength) {
            b |= (in[i + 1] & 0xF0) >> 4;
            out[j++] = EncodeChar(b);

            b = (in[i + 1] & 0x0F) << 2;
            if (i + 2 < inLength) {
                b |= (in[i + 2] & 0xC0) >> 6;
                out[j++] = EncodeChar(b);

                b = in[i + 2] & 0x3F;
                out[j++] = EncodeChar(b);
            } else {
                out[j++] = EncodeChar(b);
                out[j++] = '=';
            }
        } else {
            out[j++] = EncodeChar(b);
            out[j++] = '=';
            out[j++] = '=';
        }
    }
}

void Encode(const std::vector<uint8_t> &in, std::string &out) {
    Encode(in.data(), in.size(), out);
}

std::string Encode(const std::vector<uint8_t> &in) {
    std::string encoded;
    Encode(in, encoded);
    return encoded;
}

size_t Decode(const char *in, size_t inLength, uint8_t *&out) {
    if (in == nullptr) {
        out = nullptr;
        return 0;
    }

    if (inLength % 4 != 0) {
        throw DeadlyImportError("Invalid base64 encoded data: \"", std::string(in, std::min(size_t(32), inLength)),
            "\", length:", inLength);
    }

    if (inLength < 4) {
        out = nullptr;
        return 0;
    }

    int nEquals = int(in[inLength - 1] == '=') +
                  int(in[inLength - 2] == '=');

    size_t outLength = (inLength * 3) / 4 - nEquals;
    out = new uint8_t[outLength];
    memset(out, 0, outLength);

    size_t i, j = 0;

    for (i = 0; i + 4 < inLength; i += 4) {
        uint8_t b0 = DecodeChar(in[i]);
        uint8_t b1 = DecodeChar(in[i + 1]);
        uint8_t b2 = DecodeChar(in[i + 2]);
        uint8_t b3 = DecodeChar(in[i + 3]);

        out[j++] = (uint8_t)((b0 << 2) | (b1 >> 4));
        out[j++] = (uint8_t)((b1 << 4) | (b2 >> 2));
        out[j++] = (uint8_t)((b2 << 6) | b3);
    }

    {
        uint8_t b0 = DecodeChar(in[i]);
        uint8_t b1 = DecodeChar(in[i + 1]);
        uint8_t b2 = DecodeChar(in[i + 2]);
        uint8_t b3 = DecodeChar(in[i + 3]);

        out[j++] = (uint8_t)((b0 << 2) | (b1 >> 4));
        if (b2 < 64) out[j++] = (uint8_t)((b1 << 4) | (b2 >> 2));
        if (b3 < 64) out[j++] = (uint8_t)((b2 << 6) | b3);
    }

    return outLength;
}

size_t Decode(const std::string &in, std::vector<uint8_t> &out) {
    uint8_t *outPtr = nullptr;
    size_t decodedSize = Decode(in.data(), in.size(), outPtr);
    if (outPtr == nullptr) {
        return 0;
    }
    out.assign(outPtr, outPtr + decodedSize);
    delete[] outPtr;
    return decodedSize;
}

std::vector<uint8_t> Decode(const std::string &in) {
    std::vector<uint8_t> result;
    Decode(in, result);
    return result;
}

} // namespace Base64
} // namespace Assimp
