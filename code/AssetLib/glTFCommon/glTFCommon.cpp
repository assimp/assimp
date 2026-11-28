/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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
#include "AssetLib/glTFCommon/glTFCommon.h"

#include <limits>

using namespace glTFCommon::Util;

namespace glTFCommon::Util {

bool ParseDataURI(const char *const_uri, size_t uriLen, DataURI &out) {
    if (nullptr == const_uri) {
        return false;
    }

    if (const_uri[0] != 0x10) { // we already parsed this uri?
        if (strncmp(const_uri, "data:", 5) != 0) // not a data uri?
            return false;
    }

    // set defaults
    out.mediaType = "text/plain";
    out.charset = "US-ASCII";
    out.base64 = false;

    char *uri = const_cast<char *>(const_uri);
    if (uri[0] != 0x10) {
        size_t mediaTypeOffset = 0, charsetOffset = 0, base64Offset = 0, dataOffset = 0;

        size_t i = 5, j;
        if (uri[i] != ';' && uri[i] != ',') { // has media type?
            mediaTypeOffset = i;
            for (;i < uriLen && uri[i] != ';' && uri[i] != ','; ++i) {
                // nothing to do!
            }
        }
        while (i < uriLen && uri[i] == ';') {
            uri[i++] = '\0';
            for (j = i; i < uriLen && uri[i] != ';' && uri[i] != ','; ++i) {
                // nothing to do!
            }

            if (strncmp(uri + j, "charset=", 8) == 0) {
                charsetOffset = j + 8;
            } else if (strncmp(uri + j, "base64", 6) == 0) {
                base64Offset = j;
            }
        }
        if (i < uriLen) {
            uri[i++] = '\0';
            dataOffset = i;
        } else {
            mediaTypeOffset = charsetOffset = base64Offset = 0;
            dataOffset = 5;
        }

        // The offsets are packed into single bytes of the uri itself, so they have to fit into
        // one. The data offset is the largest of them.
        if (dataOffset > std::numeric_limits<unsigned char>::max()) {
            throw DeadlyImportError("GLTF: the header of a data URI is too long");
        }

        uri[0] = 0x10;
        uri[1] = static_cast<char>(mediaTypeOffset);
        uri[2] = static_cast<char>(charsetOffset);
        uri[3] = static_cast<char>(base64Offset);
        uri[4] = static_cast<char>(dataOffset);
    }

    // The packed offsets are unsigned bytes. A uri that merely starts with the marker byte was
    // never parsed by this function, so what is stored in it must be validated before it is used.
    if (uriLen < 5) {
        return false;
    }
    const size_t mediaTypeOffset = static_cast<unsigned char>(uri[1]);
    const size_t charsetOffset = static_cast<unsigned char>(uri[2]);
    const size_t base64Offset = static_cast<unsigned char>(uri[3]);
    const size_t dataOffset = static_cast<unsigned char>(uri[4]);
    if (mediaTypeOffset > uriLen || charsetOffset > uriLen || base64Offset > uriLen || dataOffset > uriLen) {
        throw DeadlyImportError("GLTF: malformed data URI");
    }

    if (mediaTypeOffset != 0) {
        out.mediaType = uri + mediaTypeOffset;
    }
    if (charsetOffset != 0) {
        out.charset = uri + charsetOffset;
    }
    if (base64Offset != 0) {
        out.base64 = true;
    }
    out.data = uri + dataOffset;
    out.dataLength = uriLen - dataOffset;

    return true;
}

} // namespace glTFCommon::Uti
