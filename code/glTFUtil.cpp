/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2015, assimp team
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
#include "glTFUtil.h"


using namespace Assimp;
using namespace Assimp::glTF;


bool Assimp::glTF::IsDataURI(const char* uri)
{
    return strncmp(uri, "data:", 5) == 0;
}


static const uint8_t tableDecodeBase64[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x3F,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x17, 0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
    0x31, 0x32, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00
};

static inline char EncodeCharBase64(uint8_t b)
{
    return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="[b];
}

static inline uint8_t DecodeCharBase64(char c)
{
    return tableDecodeBase64[c]; // TODO faster with lookup table or ifs?
    /*if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    return c == '+' ? 62 : 63;*/
}

std::size_t Assimp::glTF::DecodeBase64(
    const char* in, uint8_t*& out)
{
    return DecodeBase64(in, strlen(in), out);
}

std::size_t Assimp::glTF::DecodeBase64(
    const char* in, std::size_t inLength, uint8_t*& out)
{
    ai_assert(dataLen % 4 == 0);

    if (inLength < 4) {
        out = 0;
        return 0;
    }

    int nEquals = int(in[inLength - 1] == '=') +
                  int(in[inLength - 2] == '=');

    std::size_t outLength = (inLength * 3) / 4 - nEquals;
    out = new uint8_t[outLength];
    memset(out, 0, outLength);

    std::size_t j = 0;

    for (std::size_t i = 0; i < inLength; i += 4) {
        uint8_t b0 = DecodeCharBase64(in[i]);
        uint8_t b1 = DecodeCharBase64(in[i + 1]);
        uint8_t b2 = DecodeCharBase64(in[i + 2]);
        uint8_t b3 = DecodeCharBase64(in[i + 3]);

        out[j++] = (uint8_t)((b0 << 2) | (b1 >> 4));
        out[j++] = (uint8_t)((b1 << 4) | (b2 >> 2));
        out[j++] = (uint8_t)((b2 << 6) | b3);
    }

    return outLength;
}



void Assimp::glTF::EncodeBase64(
    const uint8_t* in, std::size_t inLength,
    std::string& out)
{
    std::size_t outLength = ((inLength + 2) / 3) * 4;

    out.resize(outLength);

    std::size_t j = 0;
    for (std::size_t i = 0; i <  inLength; i += 3) {
        uint8_t b = (in[i] & 0xFC) >> 2;
        out[j++] = EncodeCharBase64(b);

        b = (in[i] & 0x03) << 4;
        if (i + 1 < inLength) {
            b |= (in[i + 1] & 0xF0) >> 4;
            out[j++] = EncodeCharBase64(b);

            b = (in[i + 1] & 0x0F) << 2;
            if (i + 2 < inLength) {
                b |= (in[i + 2] & 0xC0) >> 6;
                out[j++] = EncodeCharBase64(b);

                b = in[i + 2] & 0x3F;
                out[j++] = EncodeCharBase64(b);
            }
            else {
                out[j++] = EncodeCharBase64(b);
                out[j++] = '=';
            }
        }
        else {
            out[j++] = EncodeCharBase64(b);
            out[j++] = '=';
            out[j++] = '=';
        }
    }
}
