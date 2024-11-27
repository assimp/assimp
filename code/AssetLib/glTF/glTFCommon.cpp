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
#ifndef ASSIMP_BUILD_NO_GLTF_IMPORTER

#include "AssetLib/glTF/glTFCommon.h"

namespace glTFCommon {

using namespace glTFCommon::Util;

namespace Util {

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
        uri[0] = 0x10;
        uri[1] = uri[2] = uri[3] = uri[4] = 0;

        size_t i = 5, j;
        if (uri[i] != ';' && uri[i] != ',') { // has media type?
            uri[1] = char(i);
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
                uri[2] = char(j + 8);
            } else if (strncmp(uri + j, "base64", 6) == 0) {
                uri[3] = char(j);
            }
        }
        if (i < uriLen) {
            uri[i++] = '\0';
            uri[4] = char(i);
        } else {
            uri[1] = uri[2] = uri[3] = 0;
            uri[4] = 5;
        }
    }

    if (uri[1] != 0) {
        out.mediaType = uri + uri[1];
    }
    if (uri[2] != 0) {
        out.charset = uri + uri[2];
    }
    if (uri[3] != 0) {
        out.base64 = true;
    }
    out.data = uri + uri[4];
    out.dataLength = (uri + uriLen) - out.data;

    return true;
}

} // namespace Util
} // namespace glTFCommon

#endif
